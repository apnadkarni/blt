/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTifTags.c --
 *
 * This module implements TIFF tag file format parsing routines for the
 * the BLT toolkit.
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
1 *      notice, this list of conditions and the following disclaimer.
 *   2) Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the
 *      distribution.
 *   3) Neither the name of the authors nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *   4) Products derived from this software may not be called "BLT" nor may
 *      "BLT" appear in their names without specific prior written
 *      permission from the author.
 *
 *   THIS SOFTWARE IS PROVIDED ''AS IS'' AND ANY EXPRESS OR IMPLIED
 *   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "bltInt.h"

#include "config.h"
#include <tcl.h>

#include "bltPicture.h"
#include "bltPictFmts.h"

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */
#ifdef HAVE_MEMORY_H
#  include <memory.h>
#endif /* HAVE_MEMORY_H */

typedef struct _Blt_Picture Picture;

#undef HAVE_STDLIB_H
#ifdef WIN32
#define XMD_H   1
#endif
#undef EXTERN
#undef FAR
#include "jpeglib.h"
#include "jerror.h"
#include <setjmp.h>

#define DEBUG 0

enum TifTypes {
    TIF_IGNORE,	                        /* 0 */
    TIF_BYTE,                           /* 1 */
    TIF_ASCII,                          /* 2 */
    TIF_SHORT,                          /* 3 */
    TIF_LONG,                           /* 4 */
    TIF_RATIONAL,                       /* 5 */
    TIF_SBYTE,                          /* 6 */
    TIF_UNDEFINED,                      /* 7 */
    TIF_SSHORT,                         /* 8 */
    TIF_SLONG,                          /* 9 */
    TIF_SRATIONAL,                      /* 10 */
    TIF_FLOAT,                          /* 11 */
    TIF_DOUBLE,                         /* 12 */
    TIF_IFD,				/* 13 32-bit unsigned offset. */
    TIF_LONG8,				/* 14 */
    TIF_SLONG8,				/* 15 */
    TIF_IFD8				/* 16 */
};

static const char *tifTypeStrings[] = {
    "ignore",            
    "byte",                             /* 1-byte unsigned integer. */
    "ascii",                            /* 1-byte containing 7-bit ASCII
                                         * character. */
    "short",                            /* 2-byte unsigned integer.  */
    "long",                             /* 4-byte unsigned integer. */
    "rational",                         /* 2 4-byte unsigned long integers
                                         * representing numerator and
                                         * denominator. */
    "sbyte",			        /* 1-byte 2's complement
					 * integer. */
    "undefined",                        /* 1-byte value. */
    "sshort",				/* 2-byte signed integer. */
    "slong",				/* 4-byte signed integer. */
    "srational",			/* 2 signed 4-byte integers
                                         * representing numerator and
                                         * denominator */
    "float",                            /* 4-byte IEEE floating point
					 * number. */
    "double"                            /* 8-byte IEEE floating point
					 * number. */
    "ifd", 				/* 4-byte unsigned offset. */
    "long8",				/* 8-byte unsigned long integer. */
    "slong8",				/* 8-byte signed long integer. */
    "ifd8",				/* 8-byte unsigned offset. */
};

typedef struct _TifParser TifParser;

typedef Tcl_Obj *(PrintProc)(TifParser *tifPtr, const unsigned char *bp,
			     int length);

typedef struct {
    unsigned int id;                    /* Tag ID. */
    const char *tagName;                /* Tag name. */
    enum TifTypes type;                 /* Type of value. */
    unsigned int count;                 /* # of above type. */
    PrintProc *proc;                    /* If non-NULL, procedure to be
                                         * called to parse value into
                                         * Tcl_Obj. */
} Tag;

struct _TifParser {
    int bigendian;			/* Byte order of TIFF file. */
    const unsigned char *bytes;		/* Contents of TIFF file. */
    size_t numBytes;			/* Number of bytes in TIFF file. */
    const unsigned char *start;		/* Start of TIFF file. */
    const char *varName;		/* Name of TCL array variable to
					 * store EXIF tags. */
    off_t exif;				/* If non-zero, offset to private
					 * EXIF directory. */
    off_t gps;				/* If non-zero, offset to GPS
					 * directory */
    off_t next;				/* Offset to current directory. */
    Tag *currTable;			/* Table of tags relevant for
					 * current directory. */
    int numTags;			/* Number of tags in above tag. */
    Tag *tagPtr;
};

#define EXIF_ID                34665
#define GPS_ID                 34853
#define EXIFVERSION_ID         36864
#define FLASHPIXVERSION_ID     40960
#define XPTITLE_ID             40091
#define XPSUBJECT_ID           40095

static PrintProc PrintApertureValue;
static PrintProc PrintColorMap;
static PrintProc PrintColorSpace;
static PrintProc PrintComponents;
static PrintProc PrintCompression;
static PrintProc PrintDegrees;
static PrintProc PrintFileSource;
static PrintProc PrintFocalPlaneResolutionUnit;
static PrintProc PrintGPSTimeStamp;
static PrintProc PrintGPSVersionId;
static PrintProc PrintGeoKeyDirectoryTag;
static PrintProc PrintImageMatching;
static PrintProc PrintMakerNote;
static PrintProc PrintMeteringMode;
static PrintProc PrintOrientation;
static PrintProc PrintResolutionUnit;
static PrintProc PrintSceneCaptureType;
static PrintProc PrintSceneType;
static PrintProc PrintSensingMethod;
static PrintProc PrintShutterSpeed;
static PrintProc PrintUserComment;
static PrintProc PrintWhiteBalance;
static PrintProc PrintYCbCrPositioning;

static Tag geoTags[] = {
    {  1024, "GTModelType",		TIF_SHORT,     1},
    {  1025, "GTRasterType",		TIF_ASCII,     2},
    {  1026, "GTCitation",		TIF_ASCII,     0},
    {  2048, "GeographicType",		TIF_SHORT,     0},
    {  2049, "GeogCitation",		TIF_ASCII,     0},
    {  2050, "GeogGeodeticDatum",	TIF_SHORT,     0},
    {  2051, "GeogPrimeMedian",		TIF_SHORT,     0},
    {  2052, "GeogLinearUnits",		TIF_SHORT,     0},
    {  2053, "GeogLinearUnitSize",	TIF_DOUBLE,    0},
    {  2054, "GeogAngularUnits",	TIF_SHORT,     0},
    {  2055, "GeogAngularUnitSize",	TIF_DOUBLE,    0},
    {  2056, "GeogEllispoid",		TIF_SHORT,     0},
    {  2057, "GeogSemiMajorAxis",	TIF_DOUBLE,    0},
    {  2058, "GeogSemiMinorAxis",	TIF_DOUBLE,    0},
    {  2059, "GeogSemiInvFlattening",	TIF_DOUBLE,    0},
    {  2060, "GeogAzimuthUnits",	TIF_DOUBLE,    0},
    {  2061, "GeogPrimeMedianLong",	TIF_DOUBLE,    0},
    {  3072, "ProjectedCSTType",	TIF_SHORT,     0},
    {  3073, "PCSCitation",		TIF_ASCII,     0},
    {  3074, "Projection",		TIF_SHORT,     0},
    {  3075, "ProjCoordTrans",		TIF_SHORT,     0},
    {  3076, "ProjLinearUnits",		TIF_SHORT,     0},
    {  3077, "ProjLinearUnitSize",	TIF_DOUBLE,    0},
    {  3078, "ProjStdParallel1",	TIF_DOUBLE,    0},
    {  3079, "ProjStdParallel2",	TIF_DOUBLE,    0},
    {  3080, "ProjNatOriginLong",	TIF_DOUBLE,    0},
    {  3081, "ProjNatOriginLat",	TIF_DOUBLE,    0},
    {  3082, "ProjFalseEasting",	TIF_DOUBLE,    0},
    {  3083, "ProjFalseNorthing",	TIF_DOUBLE,    0},
    {  3084, "ProjFalseOriginLong",	TIF_DOUBLE,    0},
    {  3085, "ProjFalseOriginLat",	TIF_DOUBLE,    0},
    {  3086, "ProjFalseOriginEasting",	TIF_DOUBLE,    0},
    {  3087, "ProjFalseOriginNorthing", TIF_DOUBLE,    0},
    {  3088, "ProjCenterLong",		TIF_DOUBLE,    0},
    {  3089, "ProjCenterLat",		TIF_DOUBLE,    0},
    {  3090, "ProjCenterEasting",	TIF_DOUBLE,    0},
    {  3091, "ProjCenterNorthing",	TIF_DOUBLE,    0},
    {  3092, "ProjScaleAtNatOrigin",	TIF_DOUBLE,    0},
    {  3093, "ProjScaleAtNatCenter",	TIF_DOUBLE,    0},
    {  3094, "ProjAzimuthAngle",	TIF_DOUBLE,    0},
    {  3095, "ProjStraightVertPoleLong",TIF_DOUBLE,    0},
    {  4096, "VerticalCSType",		TIF_SHORT,     0},
    {  4097, "VerticalCitation",	TIF_ASCII,     0},
    {  4098, "VerticalDatum",		TIF_SHORT,     0},
    {  4099, "VerticalUnits",		TIF_SHORT,     0}
};
static int numGeoTags = sizeof(geoTags) / sizeof(Tag);
    
static Tag gpsTags[] = {
    {     0, "GPSVersionID",        TIF_BYTE,      4, PrintGPSVersionId},
    {     1, "GPSLatitudeRef",      TIF_ASCII,     2},
    {     2, "GPSLatitude",         TIF_RATIONAL,  3, PrintDegrees},
    {     3, "GPSLongitudeRef",     TIF_ASCII,     2},
    {     4, "GPSLongitude",        TIF_RATIONAL,  3, PrintDegrees},
    {     5, "GPSAltitudeRef",      TIF_BYTE,      1},
    {     6, "GPSAltitude",         TIF_RATIONAL,  1},
    {     7, "GPSTimeStamp",        TIF_RATIONAL,  3, PrintGPSTimeStamp},
    {     8, "GPSSatelites",        TIF_ASCII,     0},
    {     9, "GPSStatus",           TIF_ASCII,     2},
    {    10, "GPSMeasureMode",      TIF_ASCII,     2},
    {    11, "GPSDOP",              TIF_RATIONAL,  1},
    {    12, "GPSSpeedRef",         TIF_ASCII,     2},
    {    13, "GPSSpeed",            TIF_RATIONAL,  1},
    {    14, "GPSTrackRef",         TIF_ASCII,     2},
    {    15, "GPSTrack",            TIF_RATIONAL,  1},
    {    16, "GPSImgDirectionRef",  TIF_ASCII,     2},
    {    17, "GPSImgDirection",     TIF_RATIONAL,  1},
    {    18, "MapDatum",            TIF_ASCII,     0},
    {    19, "GPSDestLatitudeRef",  TIF_ASCII,     2},
    {    20, "GPSDestLatitude",     TIF_RATIONAL,  3, PrintDegrees},
    {    21, "GPSDestLongitudeRef", TIF_ASCII,     2},
    {    22, "GPSDestLongitude",    TIF_RATIONAL,  3, PrintDegrees},
    {    23, "GPSDestBearingRef",   TIF_ASCII,     2},
    {    24, "GPSDestBearing",      TIF_RATIONAL,  1},
    {    25, "GPSDestDistanceRef",  TIF_ASCII,     2},
    {    26, "GPSDestDistance",     TIF_RATIONAL,  1},
    {    27, "GPSProcessingMethod", TIF_UNDEFINED, 0},
    {    28, "GPSAreaInformation",  TIF_UNDEFINED, 0},
    {    29, "GPSDateStamp",        TIF_ASCII,     11},
    {    30, "GPSDifferential",     TIF_SHORT,     1}
};
static int numGpsTags = sizeof(gpsTags) / sizeof(Tag);
    
/* Partial set of tags defined by Exif 2.2 standard. */
static Tag exifTags[] = {
    {    11, "ProcessingSoftware",  TIF_ASCII,     0},
    {   254, "NewSubFileType",      TIF_LONG,      1},
    {   255, "SubFileType",         TIF_SHORT,     1},
    {   256, "ImageWidth",          TIF_SHORT,      1},
    {   257, "ImageLength",         TIF_SHORT,      1},
    {   258, "BitsPerSample",       TIF_SHORT,     0},
    {   259, "Compression",         TIF_SHORT,     1, PrintCompression},
    {   262, "PhotometricInterpretation", TIF_SHORT,     1},
    {   263, "Thresholding",        TIF_SHORT,     1},
    {   264, "CellWidth",           TIF_SHORT,     1},
    {   265, "CellLength",          TIF_SHORT,     1},
    {   266, "FillOrder",           TIF_SHORT,     1},
    {   269, "DocumentName",        TIF_ASCII,     0},
    {   270, "ImageDescription",    TIF_ASCII,     0},
    {   271, "Make",                TIF_ASCII,     0},
    {   272, "Model",               TIF_ASCII,     0},
    {   273, "StripOffsets",        TIF_LONG,      0},
    {   274, "Orientation",         TIF_SHORT,     1, PrintOrientation},
    {   277, "SamplesPerPixel",     TIF_SHORT,     1},
    {   278, "RowsPerStrip",        TIF_SHORT,     1},
    {   279, "StripByteCounts",     TIF_LONG,      0},
    {   280, "MinSampleValue",      TIF_SHORT,	   0},
    {   281, "MaxSampleValue",      TIF_SHORT,	   0},
    {   282, "XResolution",         TIF_RATIONAL,  1},
    {   283, "YResolution",         TIF_RATIONAL,  1},
    {   284, "PlanarConfiguration", TIF_SHORT,     1},
    {   290, "GrayResponseUnit",    TIF_SHORT,     1},
    {   291, "GrayResponseCurve",   TIF_SHORT,     1},
    {   292, "T4Encoding",          TIF_LONG,      1},
    {   293, "T6Encoding",          TIF_LONG,      1},
    {   296, "ResolutionUnit",	    TIF_SHORT,     1, PrintResolutionUnit},
    {   297, "PageNumber",	    TIF_SHORT,	   2},
    {   301, "TransferFunction",    TIF_SHORT,     3*256},
    {   305, "Software",            TIF_ASCII,     0},
    {   306, "DateTime",            TIF_ASCII,    20},
    {   315, "Artist",              TIF_ASCII,     0},
    {   316, "HostComputer",        TIF_ASCII,     0},
    {   317, "Predictor",           TIF_SHORT,     0},
    {   318, "WhitePoint",          TIF_RATIONAL,  2},
    {   319, "PrimaryChromaticities", TIF_RATIONAL,  6},
    {   320, "ColorMap",            TIF_SHORT,     0, PrintColorMap},
    {   321, "HalftoneHints",       TIF_SHORT,     0},
    {   322, "TileWidth",           TIF_SHORT,     0},
    {   323, "TileHeight",          TIF_SHORT,     0},
    {   324, "TileOffsets",         TIF_SHORT,     0},
    {   325, "TileByteCounts",      TIF_SHORT,     0},
    {   330, "SubIFDs",             TIF_LONG,      0},
    {   332, "InkSet",              TIF_SHORT,     0},
    {   333, "InkNames",            TIF_ASCII,     0},
    {   334, "NumberOfInks",        TIF_SHORT,     0},
    {   336, "DotRange",            TIF_BYTE,      0},
    {   337, "TargetPrinter",       TIF_ASCII,     0},
    {   338, "ExtraSamples",        TIF_SHORT,     0},
    {   339, "SampleFormat",        TIF_SHORT,     0},
    {   340, "SMinSampleValue",     TIF_SHORT,     0},
    {   341, "SMaxSampleValue",     TIF_SHORT,     0},
    {   342, "TransferRange",       TIF_SHORT,     0},
    {   343, "ClipPath",            TIF_BYTE,      0},
    {   344, "XClipPathUnits",      TIF_SSHORT,    1},
    {   345, "YClipPathUnits",      TIF_SSHORT,    1},
    {   346, "Indexed",             TIF_SHORT,     1},
    {   347, "JPEGTables",	    TIF_SHORT,	   0},
    {   512, "JPEGProc",            TIF_LONG,      0},
    {   513, "JPEGInterchangeFormat",    TIF_LONG, 1},
    {   514, "JPEGInterchangeFormatLength", TIF_LONG, 1},
    {   529, "YCbCrCoefficients",   TIF_RATIONAL,  3},
    {   530, "YCbCrSubSampling",    TIF_SHORT,     2},
    {   531, "YCbCrPositioning",    TIF_SHORT,     1, PrintYCbCrPositioning},
    {   532, "ReferenceBlackWhite", TIF_RATIONAL,  6},
    {   700, "XMLPacket",           TIF_BYTE,      1},
    {  4096, "RelatedImageFileFormat",   TIF_ASCII,  1},
    { 33421, "CFARepeatPatternDim", TIF_SHORT,     0},
    { 33422, "CFAPattern",          TIF_BYTE,      0},
    { 33423, "BatteryLevel",        TIF_RATIONAL,  0},
    { 33432, "Copyright",           TIF_ASCII,     0},
    { 33434, "ExposureTime",        TIF_RATIONAL,  1},
    { 33437, "FNumber",             TIF_RATIONAL,  1},
    { 33550, "ModelPixelScaleTag",  TIF_DOUBLE,    3},
    { 33723, "IPTC/NAA",            TIF_LONG,      0},
    { 33922, "ModelTiepointTag",    TIF_DOUBLE,    0},
    { 34373, "ImageResources",      TIF_BYTE,      0},
    { 34377, "Photoshop",	    TIF_BYTE,	   0},
    { 34665, "ExifTag",             TIF_LONG,      1},
    { 34675, "InterColorProfile",   TIF_UNDEFINED, 0},
    { 34735, "GeoKeyDirectoryTag",  TIF_SHORT,	   0, PrintGeoKeyDirectoryTag},
    { 34850, "ExposureProgram",     TIF_SHORT,     1},
    { 34852, "SpectralSensitivity", TIF_ASCII,     0},
    { 34853, "GPSTag",              TIF_LONG,      1},
    { 34855, "ISOSpeedRatings",     TIF_SHORT,     0},
    { 34856, "OECF",                TIF_UNDEFINED, 0},
    { 34857, "Interlace",           TIF_SHORT,     0},
    { 34858, "TimeZoneOffset",      TIF_SSHORT,    0},
    { 34859, "SelfTimerMode",       TIF_SHORT,     0},
    { 34864, "SensitivityType",     TIF_SHORT,     0},
    { 34865, "StandardOutputSensitivity", TIF_LONG, 0},
    { 34866, "RecommendedExposureIndex",  TIF_LONG, 0},
    { 34867, "ISOSpeed",            TIF_LONG,      0},
    { 34868, "ISOSpeedLatitudeyyy", TIF_LONG,      0},
    { 34869, "ISOSpeedLatitudezzz", TIF_LONG,      0},
    { 36864, "ExifVersion",         TIF_ASCII,     0},
    { 36867, "DateTimeOriginal",    TIF_ASCII,     20},
    { 36868, "DateTimeDigitized",   TIF_ASCII,     20},
    { 37121, "ComponentsConfiguration", TIF_UNDEFINED, 4, PrintComponents},
    { 37122, "CompressedBitsPerPixel",  TIF_RATIONAL,  1},
    { 37377, "ShutterSpeedValue",   TIF_SRATIONAL, 1, PrintShutterSpeed},
    { 37378, "ApertureValue",       TIF_RATIONAL,  1, PrintApertureValue},
    { 37379, "BrightnessValue",     TIF_SRATIONAL, 1},
    { 37380, "ExposureBiasValue",   TIF_SRATIONAL, 1},
    { 37381, "MaxApertureValue",    TIF_RATIONAL,  1},
    { 37382, "SubjectDistance",     TIF_RATIONAL,  1},
    { 37383, "MeteringMode",        TIF_SHORT,     1, PrintMeteringMode},
    { 37384, "LightSource",         TIF_SHORT,     1},
    { 37385, "Flash",               TIF_SHORT,     1},
    { 37386, "FocalLength",         TIF_RATIONAL,  1},
    { 37387, "FlashEnergy",         TIF_RATIONAL,  1},
    { 37388, "SpatialFrequencyResponse", TIF_UNDEFINED, 1},
    { 37389, "Noise",               TIF_UNDEFINED, 1},
    { 37390, "FocalPlaneXResolution",    TIF_RATIONAL,  1},
    { 37391, "FocalPlaneYResolution",    TIF_RATIONAL,  1},
    { 37392, "FocalPlaneResolutionUnit", TIF_SHORT, 1, PrintFocalPlaneResolutionUnit},
    { 37393, "SecurityClassification",   TIF_ASCII,     1},
    { 37394, "ImageNumber",         TIF_LONG,      1},
    { 37395, "ImageHistory",        TIF_ASCII,     1},
    { 37396, "SubjectLocation",     TIF_SHORT,     1},
    { 37397, "ExposureIndex",       TIF_RATIONAL,  1},
    { 37398, "TIFFEPStandardID",    TIF_BYTE,      1},
    { 37399, "SensingMethod",       TIF_SHORT,     1, PrintSensingMethod},
    { 37500, "MakerNote",           TIF_UNDEFINED, 0, PrintMakerNote},
    { 37510, "UserComment",         TIF_UNDEFINED, 0, PrintUserComment},
    { 37520, "SubsecTime",          TIF_ASCII,     0},
    { 37521, "SubsecTimeOrginal",   TIF_ASCII,     0},
    { 37522, "SubsecTimeDigitized", TIF_ASCII,     0},
    { 40091, "XPTitle",             TIF_BYTE,      0},
    { 40092, "XPComment",           TIF_BYTE,      0},
    { 40093, "XPAuthor",            TIF_BYTE,      0},
    { 40094, "XPKeywords",          TIF_BYTE,      0},
    { 40095, "XPSubject",           TIF_BYTE,      0},
    { 40960, "FlashPixVersion",     TIF_ASCII,     0},
    { 40961, "ColorSpace",          TIF_SHORT,     1, PrintColorSpace},
    { 40962, "PixelXDimension",     TIF_LONG,      1},
    { 40963, "PixelYDimension",     TIF_LONG,     1},
    { 40964, "RelatedSoundFile",    TIF_ASCII,     13},
    { 40965, "InteroperabilityTag", TIF_LONG,      1},
    { 41484, "SpatialFrequencyResponse", TIF_UNDEFINED, 0},
    { 41486, "FocalPlaneXResolution", TIF_RATIONAL, 1},
    { 41487, "FocalPlaneYResolution", TIF_RATIONAL, 1},
    { 41488, "FocalPlaneResolutionUnit", TIF_SHORT, 1, PrintFocalPlaneResolutionUnit},
    { 41492, "SubjectLocation",     TIF_SHORT,     2},
    { 41493, "ExposureIndex",       TIF_RATIONAL,  1},
    { 41495, "SensingMethod",       TIF_SHORT,     1, PrintSensingMethod},
    { 41728, "FileSource",          TIF_UNDEFINED, 1, PrintFileSource},
    { 41729, "SceneType",           TIF_UNDEFINED, 1, PrintSceneType},
    { 41730, "CFAPattern",          TIF_UNDEFINED, 0},
    { 41838, "FlashEnergy",         TIF_RATIONAL,  1},
    { 41985, "CustomRendered",      TIF_SHORT,     1},
    { 41986, "ExposureMode",        TIF_SHORT,     1},
    { 41987, "WhiteBalance",        TIF_SHORT,     1, PrintWhiteBalance},
    { 41988, "DigitalZoomRatio",    TIF_RATIONAL,  1},
    { 41989, "FocalLenIn35mmFilm",  TIF_SHORT,     1},
    { 41990, "SceneCaptureType",    TIF_SHORT,     1, PrintSceneCaptureType},
    { 41991, "GainControl",         TIF_SHORT,     1},
    { 41992, "Contrast",            TIF_SHORT,     1},
    { 41993, "Saturation",          TIF_SHORT,     1},
    { 41994, "Sharpness",           TIF_SHORT,     1},
    { 41995, "DeviceSettingDescription", TIF_UNDEFINED, 0},
    { 41996, "SubjectDistRange",    TIF_SHORT,     1},
    { 42016, "ImageUniqueID",       TIF_ASCII,     33},
    { 42032, "CameraOwnerName",     TIF_ASCII,     33},
    { 42033, "BodySerialNumber",    TIF_ASCII,     33},
    { 42034, "LensSpecification",   TIF_RATIONAL,  1},
    { 42035, "LensMake",            TIF_ASCII,     1},
    { 42036, "LensModel",           TIF_ASCII,     1},
    { 42037, "LensSerialNumber",    TIF_ASCII,     1},
    { 50341, "PrintImageMatching",  TIF_UNDEFINED, 0, PrintImageMatching},
    { 50708, "UniqueCameraModel",   TIF_ASCII,     1},
    { 50709, "LocalizedCameraModel",TIF_BYTE,      1},
    { 59932, "Padding",             TIF_IGNORE,    0},
    { 59933, "OffsetSchema",        TIF_SLONG,     1}
};
static int numExifTags = sizeof(exifTags) / sizeof(Tag);

static Tag *
SearchForTagId(int id, Tag *table, int numTags)
{
    int low, high;

    low = 0;
    high = numTags - 1;
    while (low <= high) {
        int median;
        
        median = (low + high) >> 1;
        if (id < table[median].id) {
            high = median - 1;
        } else if (id > table[median].id) {
            low = median + 1;
        } else {
            return table + median;
        }
    }
    return NULL;                        /* Can't find id. */
}

static uint16_t
TifGetShort(TifParser *tifPtr, const unsigned char *bp)
{
    if (tifPtr->bigendian) {
        return (bp[0] << 8) | bp[1];
    } else {
        return bp[0] | (bp[1] << 8);
    }
}

static uint32_t
TifGetLong(TifParser *tifPtr, const unsigned char *bp)
{
    if (tifPtr->bigendian) {
        return (bp[0] << 24) | (bp[1] << 16) | (bp[2] << 8) | bp[3];
    } else {
        return bp[0] | (bp[1] << 8) | (bp[2] << 16) | (bp[3] << 24);
    }        
}

static double
TifGetRational(TifParser *tifPtr, const unsigned char *bp)
{
    uint32_t nom, denom;
    
    nom   = TifGetLong(tifPtr, bp);
    denom = TifGetLong(tifPtr, bp + 4);
    return (double)nom / (double)denom;
}

static uint64_t
TifGetLong8(TifParser *tifPtr, const unsigned char *bp)
{
    uint64_t l;
    
    if (tifPtr->bigendian) {
        l = (((uint64_t)bp[0] << 56) |
	     ((uint64_t)bp[1] << 48) |
	     ((uint64_t)bp[2] << 40) |
	     ((uint64_t)bp[3] << 32) |
	     ((uint64_t)bp[4] << 24) |
	     ((uint64_t)bp[5] << 16) |
	     ((uint64_t)bp[6] <<  8) |
	     ((uint64_t)bp[7]));
    } else {
        l = (((uint64_t)bp[0]) |
	     ((uint64_t)bp[1] << 8)  |
	     ((uint64_t)bp[2] << 16) |
	     ((uint64_t)bp[3] << 24) |
	     ((uint64_t)bp[4] << 32) |
	     ((uint64_t)bp[5] << 40) |
	     ((uint64_t)bp[6] << 48) |
	     ((uint64_t)bp[7] << 56));
    }        
    return l;
}

static float
TifGetFloat(TifParser *tifPtr, const unsigned char *bp)
{
    uint32_t *ip;
    float f;
    
    ip = (uint32_t *)&f;
    *ip = TifGetLong(tifPtr, bp);
    return f;
}


static double
TifGetDouble(TifParser *tifPtr, const unsigned char *bp)
{
    uint64_t *lp;
    double d;
    
    lp = (uint64_t *)&d;
    *lp = TifGetLong8(tifPtr, bp);
    return d;
}

static Tcl_Obj *
PrintColorMap(TifParser *tifPtr, const unsigned char *bp, int count)
{
    return Tcl_NewStringObj("yes", 3);
}
    
static Tcl_Obj *
PrintComponents(TifParser *tifPtr, const unsigned char *bp, int count)
{
    Tcl_Obj *objPtr;
    int i;
    static const char *names[] = {
        "", "Y", "Cb", "Cr", "R", "G", "B", "reserved"
    };
    
    objPtr = Tcl_NewStringObj("", 0);
    for (i = 0; i < 4; i++) {
        int index;

        index = bp[i];
        if (index > 6) {
            index = 7;
        }
        Tcl_AppendToObj(objPtr, names[index], -1);
    }
    return objPtr;
}

static Tcl_Obj *
PrintCompression(TifParser *tifPtr, const unsigned char *bp, int count)
{
    const char *string;
    int i;
    uint16_t s;
    struct compressTypes {
	int id;
	const char *name;
    };
    struct compressTypes types[] = {
	{     1, "none"		},
	{     2, "ccittrle"	},
	{     3, "ccittfax3"	},
	{     4, "ccittfax4"	},
	{     5, "lzw"		},
	{     6, "ojpeg"	},
	{     7, "jpeg"		},
	{     8, "adobedeflate" },
	{     9, "jbig"		},
	{ 32773, "packbits"     },
	{ 32809, "thunderscan"  },
	{ 32766, "next"         },
	{ 32721, "ccittrlew"	},
	{ 32946, "deflate"      },
	{ 32908, "pixarfilm"    },
	{ 32909, "pixarlog"     },
	{ 34676, "sgilog"       },
	{ 34677, "sgilog24"     },
	{ 34925, "lzma"         },
	{ 34712, "jp2000"       },
    };	
    static int numTypes = sizeof(types) / sizeof(struct compressTypes);

    s = TifGetShort(tifPtr, bp);
    string = "???";
    for (i = 0; i < numTypes; i++) {
	if (s == types[i].id) {
	    string = types[i].name;
	    break;
	}
    } 
    return Tcl_NewStringObj(string, -1);
}


static Tcl_Obj *
PrintFileSource(TifParser *tifPtr, const unsigned char *bp, int count)
{
    if (bp[0] == 3) {
        return Tcl_NewStringObj("DSC", 3);
    } else {
        return Tcl_NewStringObj("", 0);
    }
}

static Tcl_Obj *
PrintGeoKeyDirectoryTag(TifParser *tifPtr, const unsigned char *bp, int count)
{
    Tcl_Obj *listObjPtr, *objPtr;
    int i;
    int version, keyMajorRev, keyMinorRev, numKeys;
    
    version     = TifGetShort(tifPtr, bp);
    keyMajorRev = TifGetShort(tifPtr, bp + 2);
    keyMinorRev = TifGetShort(tifPtr, bp + 4);
    numKeys     = TifGetShort(tifPtr, bp + 6);

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewIntObj(version);
    Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(keyMajorRev);
    Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(keyMinorRev);
    Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
    objPtr = Tcl_NewIntObj(numKeys);
    Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);

    bp += 8;
    for (i = 0; i < numKeys; i++) {
	int id, type, count, offset;
	Tag *tagPtr;
        
	id     = TifGetShort(tifPtr, bp);   /* Id of tag. */
	type   = TifGetShort(tifPtr, bp+2); /* Tag location. */
	count  = TifGetShort(tifPtr,  bp+4); /* # of specified types. */
	offset = TifGetShort(tifPtr,  bp+6); /* Offset used only if value
					     * doesn't fit in 4-bytes. */

        tagPtr = SearchForTagId(id, geoTags, numGeoTags);
        if (tagPtr == NULL) {
            objPtr = Tcl_NewStringObj("???", -1);
        } else {
            objPtr = Tcl_NewStringObj(tagPtr->tagName, -1);
        }
	Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
        if (type == 0) {
            type = TIF_SHORT;
        }
	objPtr = Tcl_NewStringObj(tifTypeStrings[type], -1);
	Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
	objPtr = Tcl_NewIntObj(count);
	Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
	objPtr = Tcl_NewIntObj(offset);
	Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
	bp += 8;
    }
    return listObjPtr;
}


static Tcl_Obj *
PrintGPSVersionId(TifParser *tifPtr, const unsigned char *bp, int count)
{
    return Tcl_ObjPrintf("%d.%d.%d.%d", bp[0], bp[1], bp[2], bp[3]);
}

static Tcl_Obj *
PrintGPSTimeStamp(TifParser *tifPtr, const unsigned char *bp, int count)
{
    double h, m, s;

    h = TifGetRational(tifPtr, bp);
    m = TifGetRational(tifPtr, bp + 8);
    s = TifGetRational(tifPtr, bp + 12);
    return Tcl_ObjPrintf("%g:%g:%g", h, m, s);
}

static Tcl_Obj *
PrintApertureValue(TifParser *tifPtr, const unsigned char *bp, int count)
{
    double dval;
            
    dval = TifGetRational(tifPtr, bp);
    return Tcl_NewDoubleObj(pow(M_SQRT2, dval));
}

static Tcl_Obj *
PrintShutterSpeed(TifParser *tifPtr, const unsigned char *bp, int count)
{
    double dval;
            
    dval = TifGetRational(tifPtr, bp);
    return Tcl_NewDoubleObj(pow(2.0, dval));
}
	
static Tcl_Obj *
PrintDegrees(TifParser *tifPtr, const unsigned char *bp, int count)
{
    double degrees;

    degrees = ((TifGetRational(tifPtr, bp)) +
	       (TifGetRational(tifPtr, bp + 8) / 60.0) +  
	       (TifGetRational(tifPtr, bp + 16) / 3600.0));
    return Tcl_NewDoubleObj(degrees);
}

static Tcl_Obj *
PrintFocalPlaneResolutionUnit(TifParser *tifPtr, const unsigned char *bp,
			      int count)
{
    uint16_t s;

    s = TifGetShort(tifPtr, bp);
    if (s == 2) {
        return Tcl_NewStringObj("inch", 4);
    } else {
        return Tcl_NewIntObj(s);
    }
}

static Tcl_Obj *
PrintColorSpace(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;

    s = TifGetShort(tifPtr, bp);
    if (s == 1) {
        return Tcl_NewStringObj("sRGB", 4);
    } else if (s == 0xFFFF) {
        return Tcl_NewStringObj("Uncalibrated", -1);
    } else {
        return Tcl_NewStringObj("???", -1);
    }
}

static Tcl_Obj *
PrintOrientation(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;
    static const char *strings[] = {
        "top left",                     /* 1 */
        "top right",                    /* 2 */
        "bottom right",                 /* 3 */
        "bottom left",                  /* 4 */
        "left top"                      /* 5 */
        "right top",                    /* 6 */
        "right bottom",                 /* 7 */
        "left bottom"                   /* 8 */
    };
    s = TifGetShort(tifPtr, bp);
    if ((s > 8) || (s == 0)) {
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[s-1], -1);
}

static Tcl_Obj *
PrintMeteringMode(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;
    static const char *strings[] = {
        "Average",                      /* 1 */
        "CenterWeightedAverage",        /* 2 */
        "Spot",                         /* 3 */
        "MultiSpot",                    /* 4 */
        "Pattern",                      /* 5 */
        "Partial",                      /* 6 */
    };
    s = TifGetShort(tifPtr, bp);
    if ((s > 6) || (s == 0)) {
        if (s == 255) {
            return Tcl_NewStringObj("Other", 5);
        } 
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[s-1], -1);
}

static Tcl_Obj *
PrintSensingMethod(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;
    static const char *strings[] = {
        "not defined",                  /* 1 */
        "1-chip color sensor",          /* 2 */
        "2-chip color sensor",          /* 3 */
    };
    s = TifGetShort(tifPtr, bp);
    if ((s > 3) || (s == 0)) {
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[s-1], -1);
}

static Tcl_Obj *
PrintYCbCrPositioning(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;
    static const char *strings[] = {
        "centered",                     /* 1 */
        "co-sited",                     /* 2 */
    };
    s = TifGetShort(tifPtr, bp);
    if ((s > 2) || (s == 0)) {
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[s-1], -1);
}

static Tcl_Obj *
PrintWhiteBalance(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;
    static const char *strings[] = {
        "auto",                         /* 0 */
        "manual",                       /* 1 */
    };
    s = TifGetShort(tifPtr, bp);
    if (s > 1) {
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[s], -1);
}

static Tcl_Obj *
PrintResolutionUnit(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t s;
    static const char *strings[] = {
        "inches",                       /* 2 */
        "centimeters",                  /* 3 */
    };
    s = TifGetShort(tifPtr, bp);
    if ((s > 3) || (s < 2)) {
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[s-2], -1);
}

static Tcl_Obj *
PrintMakerNote(TifParser *tifPtr, const unsigned char *bp, int count)
{
    return Tcl_NewStringObj("???", 3);
    return Blt_Base64_EncodeToObj(bp, count);
}

static Tcl_Obj *
PrintImageMatching(TifParser *tifPtr, const unsigned char *bp, int count)
{
    return Blt_Base64_EncodeToObj(bp, count);
}

static Tcl_Obj *
PrintSceneCaptureType(TifParser *tifPtr, const unsigned char *bp, int count)
{
    uint16_t value;
    static const char *strings[] = {
        "Standard",                       /* 0 */
        "Landscape",                      /* 1 */
        "Portrait",                       /* 2 */
        "NightScene"                      /* 3 */
    };
    value = TifGetShort(tifPtr, bp);
    if (value > 3) {
        return Tcl_NewStringObj("???", 3);
    } 
    return Tcl_NewStringObj(strings[value], -1);
}

static Tcl_Obj *
PrintUserComment(TifParser *tifPtr, const unsigned char *bp, int count)
{
    if (memcmp("ASCII\0\0\0", bp, 8) == 0) {
        int slen;
        const char *s1, *last;
        
        s1 = (const char *)bp + 8;
        count -= 8;
        slen = strlen(s1);
        if (slen < count) {
            last = s1 + slen;
        } else {
            last = s1 - count;
        }
        return Tcl_NewStringObj(s1, last - s1);
    }
    return Tcl_NewStringObj("???", 3);
}

static Tcl_Obj *
PrintSceneType(TifParser *tifPtr, const unsigned char *bp, int count)
{
    return Tcl_NewIntObj(bp[0]);
}

static Tcl_Obj *
ValueToObj(TifParser *tifPtr, const unsigned char *bp, int count,
           enum TifTypes type)
{
    Tcl_Obj *resultObjPtr;
    
    switch (type) {
    case TIF_IGNORE:
        resultObjPtr = NULL;
        break;

    case TIF_BYTE:
        if (count == 1) {
            uint8_t ival;
            
            ival = (uint8_t)bp[0];
            resultObjPtr = Tcl_NewIntObj(ival);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                uint8_t ival;
                Tcl_Obj *objPtr;
		
                ival = (uint8_t)bp[i];
                objPtr = Tcl_NewIntObj(ival);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_ASCII:
        {
            const char *string;
            const char *p;
            
            /* Trim trailing whitespace. */
            string = (const char *)bp;
            for (p = string + (count - 1); p > string; p--) {
		if ((!isspace(*p)) && (*p != '\0')) {
		    break;
		}
            }
            resultObjPtr = Tcl_NewStringObj(string, p - string + 1);
        }
        break;

    case TIF_SHORT:
        if (count == 1) {
            uint16_t sval;
            
            sval = TifGetShort(tifPtr, bp);
            resultObjPtr = Tcl_NewIntObj(sval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                uint16_t sval;
                Tcl_Obj *objPtr;
		
                sval = TifGetShort(tifPtr, bp + (i * 4));
                objPtr = Tcl_NewIntObj(sval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_IFD:
    case TIF_LONG:
        if (count == 1) {
            uint32_t lval;
            
            lval = TifGetLong(tifPtr, bp);
            resultObjPtr = Tcl_NewIntObj(lval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                Tcl_Obj *objPtr;
                uint32_t lval;
                
                lval = TifGetLong(tifPtr, bp + (i * 4));
                objPtr = Tcl_NewIntObj(lval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_RATIONAL:
        if (count == 1) {
            double dval;
            
            dval = TifGetRational(tifPtr, bp);
            resultObjPtr = Tcl_NewDoubleObj(dval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                Tcl_Obj *objPtr;
                double dval;
                
                dval = TifGetRational(tifPtr, bp + (i * 8));
                objPtr = Tcl_NewDoubleObj(dval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;
        
    case TIF_UNDEFINED:
        resultObjPtr = Blt_Base64_EncodeToObj(bp, count);
        break;

    case TIF_SBYTE:
        if (count == 1) {
            int8_t ival;
            
            ival = (int8_t)bp[0];
            resultObjPtr = Tcl_NewIntObj(ival);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                int8_t ival;
                Tcl_Obj *objPtr;
		
                ival = (int8_t)bp[i];
                objPtr = Tcl_NewIntObj(ival);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_SSHORT:
        if (count == 1) {
            int16_t sval;
            
            sval = TifGetShort(tifPtr, bp);
            resultObjPtr = Tcl_NewIntObj(sval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                int16_t sval;
                Tcl_Obj *objPtr;
		
                sval = TifGetShort(tifPtr, bp + (i * 4));
                objPtr = Tcl_NewIntObj(sval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_SLONG:
        if (count == 1) {
            int32_t lval;
            
            lval = TifGetLong(tifPtr, bp);
            resultObjPtr = Tcl_NewIntObj(lval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                Tcl_Obj *objPtr;
                int32_t lval;
                
                lval = TifGetLong(tifPtr, bp + (i * 4));
                objPtr = Tcl_NewIntObj(lval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;
        
    case TIF_SRATIONAL:
        if (count == 1) {
            double dval;
            
            dval = TifGetRational(tifPtr, bp);
            resultObjPtr = Tcl_NewDoubleObj(dval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                double dval;
                Tcl_Obj *objPtr;
		
                dval = TifGetRational(tifPtr, bp + (i * 8));
                objPtr = Tcl_NewDoubleObj(dval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_FLOAT:
        if (count == 1) {
            float fval;
            
            fval = TifGetFloat(tifPtr, bp);
            resultObjPtr = Tcl_NewDoubleObj(fval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                float fval;
                Tcl_Obj *objPtr;
		
                fval = TifGetFloat(tifPtr, bp + (i * 8));
                objPtr = Tcl_NewDoubleObj(fval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
        break;

    case TIF_LONG8:
    case TIF_SLONG8:
    case TIF_IFD8:
        if (count == 1) {
            uint64_t lval;
            
            lval = TifGetLong(tifPtr, bp);
            resultObjPtr = Tcl_NewLongObj(lval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                uint64_t lval;
                Tcl_Obj *objPtr;
		
                lval = TifGetLong(tifPtr, bp + (i * 8));
                objPtr = Tcl_NewLongObj(lval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
	break;

    case TIF_DOUBLE:
        if (count == 1) {
            double dval;
            
            dval = TifGetDouble(tifPtr, bp);
            resultObjPtr = Tcl_NewDoubleObj(dval);
        } else {
            int i;
            Tcl_Obj *listObjPtr;
            
            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (i = 0; i < count; i++) {
                double dval;
                Tcl_Obj *objPtr;
		
                dval = TifGetDouble(tifPtr, bp + (i * 8));
                objPtr = Tcl_NewDoubleObj(dval);
                Tcl_ListObjAppendElement(NULL, listObjPtr, objPtr);
            }
            resultObjPtr = listObjPtr;
        }
	break;
    }
    return resultObjPtr;
}


static int
SetVariable(Tcl_Interp *interp, const char *varName, TifParser *tifPtr,
               const unsigned char *bp)
{
    Tag *tagPtr;
    Tcl_Obj *objPtr;
    enum TifTypes type;
    int id, offset, count, numBytes;
    
    id     = TifGetShort(tifPtr, bp);   /* Id of tag. */
    type   = TifGetShort(tifPtr, bp+2); /* Type of value */
    count  = TifGetLong(tifPtr,  bp+4); /* # of specified types. */
    offset = TifGetLong(tifPtr,  bp+8); /* Offset used only if value
                                         * doesn't fit in 4-bytes. */
    tagPtr = SearchForTagId(id, tifPtr->currTable, tifPtr->numTags);
#if DEBUG
    fprintf(stderr, "\t%s: id=%d type=\"%s\" (%d) count=%d offset=%d\n",
            (tagPtr) ? tagPtr->tagName : "???", id,
            (type > TIF_DOUBLE) ? "???" : tifTypeStrings[type], type,
            count, offset);
#endif
    if (tagPtr == NULL) {
        fprintf(stderr, "Warning: can't find id %x\n", id);
        return TCL_CONTINUE;            /* Unknown tag id. */
    }
    if (tagPtr->type == TIF_IGNORE) {
	return TCL_CONTINUE;
    }
    if (type != tagPtr->type) {
        fprintf(stderr, "Warning `%s`: types don't match: found=%s wanted=%s\n",
                tagPtr->tagName,
                (type > TIF_IFD8) ? "???" : tifTypeStrings[type],
                tifTypeStrings[tagPtr->type]);
        if (type > TIF_IFD8) {
            type = tagPtr->type;
        }
    }
    if ((tagPtr->count > 0) && (count != tagPtr->count)) {
        fprintf(stderr, "Warning `%s`: counts don't match: found=%d wanted=%d\n",
                tagPtr->tagName, count, tagPtr->count);
    }
    if (tagPtr->id == EXIFVERSION_ID || tagPtr->id == FLASHPIXVERSION_ID ||
	(tagPtr->id >= XPTITLE_ID && tagPtr->id <= XPSUBJECT_ID)) {
	type = TIF_ASCII;
    }
    if (tagPtr->id == EXIF_ID) {
        tifPtr->exif = offset;		/* Save the offset for later. */
    } else if (tagPtr->id == GPS_ID) {
        tifPtr->gps = offset;		/* Save the offset for later. */
    }

    numBytes = 0;                       /* Suppress compiler warning. */
    switch (type) {
    case TIF_DOUBLE:
    case TIF_IFD8:
    case TIF_LONG8:
    case TIF_RATIONAL:
    case TIF_SLONG8:
    case TIF_SRATIONAL:
	numBytes = count * 8;               break;
    case TIF_SHORT:
    case TIF_SSHORT:
	numBytes = count * 2;               break;
    case TIF_FLOAT:
    case TIF_IFD:
    case TIF_LONG:
    case TIF_SLONG:
	numBytes = count * 4;               break;
    case TIF_ASCII:
    case TIF_BYTE:
    case TIF_IGNORE:
    case TIF_SBYTE:
    case TIF_UNDEFINED:
	numBytes = count;                   break;
    }
    if (numBytes > 4) {
	bp = tifPtr->start + offset;
    } else {
	bp += 8;
    }
    if (tagPtr->proc != NULL) {
	objPtr = (*tagPtr->proc)(tifPtr, bp, count);
    } else {
	objPtr = ValueToObj(tifPtr, bp, count, type);
    }
    if (objPtr != NULL) {
        if (Tcl_SetVar2Ex(interp, varName, tagPtr->tagName, objPtr,
        	 TCL_LEAVE_ERR_MSG) == NULL) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
ParseDirectory(Tcl_Interp *interp, TifParser *tifPtr, int offset)
{
    const unsigned char *bp;
    int i, numEntries;
    
    bp = tifPtr->start + offset;
    numEntries = TifGetShort(tifPtr, bp);
    if ((offset + (numEntries * 12)) >= tifPtr->numBytes) {
	Tcl_AppendResult(interp, "not enough room for IFD directory",
			 (char *)NULL);
        return TCL_ERROR;	       
    }
#if DEBUG
    fprintf(stderr, "offset=%d, number of entries=%d\n", offset,
            numEntries);
    fprintf(stderr, "search for %d entries\n", numEntries);
#endif
    bp += 2;
    for (i = 0; i < numEntries; i++) {
        int result;
        
        result = SetVariable(interp, tifPtr->varName, tifPtr, bp);
        if (result == TCL_ERROR) {
            return TCL_ERROR;
        }
        bp += 12;
    }      
    tifPtr->next = TifGetLong(tifPtr, bp);
    return TCL_OK;
}

static int
ParseGPS(Tcl_Interp *interp, TifParser *tifPtr)
{
    if (tifPtr->gps >= tifPtr->numBytes) {
	Tcl_AppendResult(interp, "GPS directory offset is beyond the "
			 "end of the TIFF file.", (char *)NULL);
        return TCL_ERROR;
    }
    tifPtr->currTable = gpsTags;
    tifPtr->numTags = numGpsTags;
    return ParseDirectory(interp, tifPtr, tifPtr->gps);
}

static int
ParseExif(Tcl_Interp *interp, TifParser *tifPtr)
{
    if (tifPtr->exif >= tifPtr->numBytes) {
	Tcl_AppendResult(interp, "Exif directory offset is beyond the "
			 "end of the TIFF file.", (char *)NULL);
	return TCL_ERROR;	
    }
    tifPtr->currTable = exifTags;
    tifPtr->numTags = numExifTags;
    return ParseDirectory(interp, tifPtr, tifPtr->exif);
}

int
Blt_ParseTifTags(Tcl_Interp *interp, const char *varName,
	const unsigned char *bytes, off_t offset, size_t numBytes)
{
    TifParser tif;
    int id;
    const unsigned char *bp;
    
    /* 8-byte TIFF header */
    if (numBytes < 14) {
        return TCL_CONTINUE;    	/* Not enough space for header. */
    }
    memset(&tif, 0, sizeof(TifParser));
    tif.numBytes = numBytes;
    tif.bytes = bytes;
    bp = bytes + offset;

    if ((bp[0] == 'I') && (bp[1] == 'I')) {
        tif.bigendian = FALSE;          /* little endian */
    } else if ((bp[0] == 'M') && (bp[1] == 'M')) {
        tif.bigendian = TRUE;		/* big endian */
    } else {
	char mesg[20];

	sprintf(mesg, "%c%c", bp[0], bp[1]);
	Tcl_AppendResult(interp, "invalid header \"",  mesg, "\" in TIFF file",
			 (char *)NULL);
        return TCL_ERROR;
    }
    id = TifGetShort(&tif, bp + 2);
    if (id != 42) {
	Tcl_AppendResult(interp, "incorrect byte order specified in TIFF file",
		(char *)NULL);
        return TCL_ERROR;    
    }
    tif.start = tif.bytes + offset;
    tif.currTable = exifTags;
    tif.numTags = numExifTags;
    tif.varName = varName;

    tif.next = TifGetLong(&tif, bp + 4);
    do {
	if (tif.next >= tif.numBytes) {
	    Tcl_AppendResult(interp, "directory offset is beyond the "
			 "end of the TIFF file.", (char *)NULL);
	    return TCL_ERROR;	
	}
        if (ParseDirectory(interp, &tif, tif.next) == TCL_ERROR) {
            fprintf(stderr, "Failed ParseDirectory\n");
            return TCL_ERROR;
        }
    } while (tif.next > 0);  

    if (tif.exif > 0) {
        if (ParseExif(interp, &tif) == TCL_ERROR) {
            return TCL_ERROR;
        }
    }
    if (tif.gps > 0) {
	if (ParseGPS(interp, &tif) == TCL_ERROR) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}
