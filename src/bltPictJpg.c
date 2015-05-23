/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltPictJpg.c --
 *
 * This module implements JPEG file format conversion routines for the
 * picture image type in the BLT toolkit.
 *
 * Copyright 2015 George A. Howlett. All rights reserved.  
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions are
 *   met:
 *
 *   1) Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
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
 *
 * The JPEG reader/writer is adapted from jdatasrc.c and jdatadst.c in the
 * Independent JPEG Group (version 6b) library distribution.
 *
 * Copyright (C) 1991-1998, Thomas G. Lane.  All Rights Reserved except as
 * specified below.
 * 
 *   Permission is hereby granted to use, copy, modify, and distribute this
 *   software (or portions thereof) for any purpose, without fee, subject
 *   to these conditions: (1) If any part of the source code for this
 *   software is distributed, then this README file must be included, with
 *   this copyright and no-warranty notice unaltered; and any additions,
 *   deletions, or changes to the original files must be clearly indicated
 *   in accompanying documentation.  (2) If only executable code is
 *   distributed, then the accompanying documentation must state that "this
 *   software is based in part on the work of the Independent JPEG Group".
 *   (3) Permission for use of this software is granted only if the user
 *   accepts full responsibility for any undesirable consequences; the
 *   authors accept NO LIABILITY for damages of any kind.
 *
 *   The authors make NO WARRANTY or representation, either express or
 *   implied, with respect to this software, its quality, accuracy,
 *   merchantability, or fitness for a particular purpose.  This software
 *   is provided "AS IS", and you, its user, assume the entire risk as to
 *   its quality and accuracy.
 *
 */

#include "bltInt.h"

#include "config.h"
#ifdef HAVE_LIBJPG

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

#define PIC_PROGRESSIVE (1<<0)
#define PIC_NOQUANTIZE  (1<<1)

#define PIC_FMT_ISASCII (1<<3)

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int quality;                        /* Value 0..100 */
    int smoothing;                      /* Value 0..100 */
    int compress;                       /* Value 0..N */
    int flags;                          /* Flag. */
    Blt_Pixel bg;
    int index;
} JpgExportSwitches;

typedef struct {
    Tcl_Obj *dataObjPtr;
    Tcl_Obj *fileObjPtr;
    int method;                         /* DCT method. */
} JpgImportSwitches;

static Blt_SwitchParseProc ColorSwitchProc;

static Blt_SwitchCustom colorSwitch = {
    ColorSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc DctSwitchProc;

static Blt_SwitchCustom dctSwitch = {
    DctSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PercentSwitchProc;

static Blt_SwitchCustom percentSwitch = {
    PercentSwitchProc, NULL, NULL, (ClientData)0
};

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,    "-background", "color", (char *)NULL,
        Blt_Offset(JpgExportSwitches, bg), 0, 0, &colorSwitch},
    {BLT_SWITCH_OBJ,       "-data",        "varName", (char *)NULL,
        Blt_Offset(JpgExportSwitches, dataObjPtr),0},
    {BLT_SWITCH_OBJ,       "-file",        "fileName", (char *)NULL,
        Blt_Offset(JpgExportSwitches, fileObjPtr),0},
    {BLT_SWITCH_INT_NNEG, "-index", "int", (char *)NULL,
        Blt_Offset(JpgExportSwitches, index), 0},
    {BLT_SWITCH_CUSTOM,  "-quality",     "percent", (char *)NULL,
        Blt_Offset(JpgExportSwitches, quality), 0, 0, &percentSwitch},
    {BLT_SWITCH_INT_NNEG,  "-smooth",      "percent", (char *)NULL,
        Blt_Offset(JpgExportSwitches, smoothing), 0, 0, &percentSwitch},
    {BLT_SWITCH_BITMASK,   "-progressive", "", (char *)NULL,
        Blt_Offset(JpgExportSwitches, flags), 0, PIC_PROGRESSIVE},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
        Blt_Offset(JpgImportSwitches, dataObjPtr), 0},
    {BLT_SWITCH_CUSTOM, "-dct", "method", (char *)NULL,
       Blt_Offset(JpgImportSwitches, method), 0, 0, &dctSwitch},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(JpgImportSwitches, fileObjPtr), 0},
    {BLT_SWITCH_END}
};

#define JPG_BUF_SIZE  4096             /* Choose an efficiently fwrite'able
                                        * size */

typedef struct {
    struct jpeg_source_mgr pub;         /* Public fields */

    Blt_DBuffer dBuffer;                /* Collects the converted data. */
    Tcl_Interp *interp;
    const char *varName;
} JpgReader;

typedef struct {
    struct jpeg_destination_mgr pub;    /* Public fields */
    Blt_DBuffer dBuffer;                /* Target stream */
    JOCTET *bytes;                      /* Start of buffer */
} JpgWriter;

typedef struct {
    struct jpeg_error_mgr pub;          /* "Public" fields */
    jmp_buf jmpbuf;
    Tcl_DString ds;
} JpgErrorHandler;

DLLEXPORT extern Tcl_AppInitProc Blt_PictureJpgInit;
DLLEXPORT extern Tcl_AppInitProc Blt_PictureJpgSafeInit;

/*
 *---------------------------------------------------------------------------
 *
 * ColorSwitchProc --
 *
 *      Convert a Tcl_Obj representing a Blt_Pixel color.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ColorSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)  
{
    Blt_Pixel *pixelPtr = (Blt_Pixel *)(record + offset);
    const char *string;

    string = Tcl_GetString(objPtr);
    if (string[0] == '\0') {
        pixelPtr->u32 = 0x00;
        return TCL_OK;
    }
    if (Blt_GetPixelFromObj(interp, objPtr, pixelPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DctSwitchProc --
 *
 *      Convert a Tcl_Obj representing a DCT method.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DctSwitchProc(ClientData clientData, Tcl_Interp *interp, const char *switchName,
              Tcl_Obj *objPtr, char *record, int offset, int flags)     
{
    int *methodPtr = (int *)(record + offset);
    const char *string;
    char c;
    
    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'f') && (strcmp(string, "fast") == 0)) {
        *methodPtr = JDCT_IFAST;
    } else if ((c == 'f') && (strcmp(string, "slow") == 0)) {
        *methodPtr = JDCT_ISLOW;
    } else if ((c == 'f') && (strcmp(string, "float") == 0)) {
        *methodPtr = JDCT_FLOAT;
    } else {
        Tcl_AppendResult(interp, "bad DCT method \"", string, "\" should be ",
                         " fast, slow, or float.", (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PercentSwitchProc --
 *
 *      Convert a Tcl_Obj representing an integer percentage 0 to 100.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PercentSwitchProc(ClientData clientData, Tcl_Interp *interp,
                  const char *switchName, Tcl_Obj *objPtr, char *record,
                  int offset, int flags)        
{
    int *percentPtr = (int *)(record + offset);
    double value;
    
    if (Tcl_GetDoubleFromObj(interp, objPtr, &value) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((value < 0.0) || (value > 100.0)) {
        Tcl_AppendResult(interp, "bad percent value \"", Tcl_GetString(objPtr),
                         "\" should be between 0 and 100.", (char *)NULL);
        return TCL_ERROR;
    }
    *percentPtr = (int)(value + 0.5);
    return TCL_OK;
}

typedef struct  {
    int bigendian;
    int numEntries;
    unsigned int ifd0;
    Blt_DBuffer dbuffer;
    unsigned char *start;
} Tif;

static unsigned short
TifGetShort(Tif *tifPtr, unsigned char *buf)
{
    if (tifPtr->bigendian) {
        return (buf[0] << 8) | buf[1];
    } else {
        return buf[0] | (buf[1] << 8);
    }
}

static unsigned int
TifGetLong(Tif *tifPtr, unsigned char *buf)
{
    if (tifPtr->bigendian) {
        return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    } else {
        return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
    }        
}

enum TifTypes {
    TIF_UNKNOWN,
    TIF_BYTE,
    TIF_ASCII,
    TIF_SHORT,
    TIF_LONG,
    TIF_RATIONAL,
    TIF_SBYTE,
    TIF_UNDEFINED,
    TIF_SSHORT,
    TIF_SLONG,
    TIF_SRATIONAL,
    TIF_FLOAT,
    TIF_DOUBLE
};

#ifdef notdef
static const char *tifTypeStrings[] = {
    "unknown",                          /* Unknown. */
    "byte",                             /* Unsigned (1-byte) integer. */
    "ascii",                            /* 8-bit byte containing 7-bit
                                         * ASCII character. */
    "short",                            /* Unsigned (2-byte) integer.  */
    "long",                             /* Unsigned (4-byte) integer. */
    "rational",                         /* 2 unsigned long integers
                                         * representing numerator and
                                         * denominator. */
    "signed byte",                      /* 8-bit 2's complement integer. */
    "undefined",                        /* 8-bit, 1-byte value. */
    "signed short",                     /* Signed (2-byte) integer. */
    "signed long",                      /* Signed (4-byte) integer. */
    "signed rational",                  /* 2 signed long integers
                                         * representing numerator and
                                         * denominator */
    "float",                            /* IEEE (4-byte) floating number */
    "double"                            /* IEEE (8-byte) floating number */
};
#endif

typedef struct {
    unsigned int id;
    const char *tagName;
    enum TifTypes type;
    unsigned int size;
    void *dummy;
} TifTag;

/* Partial set of tags defined by Exif 2.2 standard. */
static TifTag tifTags[] = {
    {    11,  "ProcessingSoftware",             TIF_ASCII,     0},
    {   254,  "NewSubFileType",                 TIF_LONG,      1},
    {   255,  "SubFileType",                    TIF_SHORT,     1},
    {   256,  "ImageWidth",                     TIF_LONG,      1},
    {   257,  "ImageLength",                    TIF_LONG,      1},
    {   258,  "BitsPerSample",                  TIF_SHORT,     1},
    {   259,  "Compression",                    TIF_SHORT,     1},
    {   262,  "PhotometricInterpretation",      TIF_SHORT,     1},
    {   263,  "Thresholding",                   TIF_SHORT,     1},
    {   264,  "CellWidth",                      TIF_SHORT,     1},
    {   265,  "CellLength",                     TIF_SHORT,     1},
    {   266,  "FillOrder",                      TIF_SHORT,     1},
    {   269,  "DocumentName",                   TIF_ASCII,     0},
    {   270,  "ImageDescription",               TIF_ASCII,     0},
    {   271,  "Make",                           TIF_ASCII,     0},
    {   272,  "Model",                          TIF_ASCII,     0},
    {   273,  "StripOffsets",                   TIF_LONG,      0},
    {   274,  "Orientation",                    TIF_SHORT,     1},
    {   277,  "SamplesPerPixel",                TIF_SHORT,     1},
    {   278,  "RowsPerStrip",                   TIF_LONG,      1},
    {   279,  "StripByteCounts",                TIF_LONG,      1},
    {   282,  "XResolution",                    TIF_RATIONAL,  1},
    {   283,  "YResolution",                    TIF_RATIONAL,  1},
    {   284,  "PlanarConfiguration",            TIF_SHORT,     1},
    {   290,  "GrayResponseUnit",               TIF_SHORT,     1},
    {   291,  "GrayResponseCurve",              TIF_SHORT,     1},
    {   292,  "T4Encoding",                     TIF_LONG,      1},
    {   293,  "T6Encoding",                     TIF_LONG,      1},
    {   296,  "ResolutionUnit",                 TIF_SHORT,     1},
    {   301,  "TransferFunction",               TIF_SHORT,     1},
    {   305,  "Software",                       TIF_ASCII,     0},
    {   306,  "DateTime",                       TIF_ASCII,    20},
    {   315,  "Artist",                         TIF_ASCII,     0},
    {   316,  "HostComputer",                   TIF_ASCII,     0},
    {   317,  "Predictor",                      TIF_ASCII,     0},
    {   318,  "WhitePoint",                     TIF_RATIONAL,  2},
    {   319,  "PrimaryChromaticities",          TIF_RATIONAL,  6},
    {   320,  "ColorMap",                       TIF_SHORT,     0},
    {   321,  "HalftoneHints",                  TIF_SHORT,     0},
    {   322,  "TileWidth",                      TIF_SHORT,     0},
    {   323,  "TileHeight",                     TIF_SHORT,     0},
    {   324,  "TileOffsets",                    TIF_SHORT,     0},
    {   325,  "TileByteCounts",                 TIF_SHORT,     0},
    {   330,  "SubIFDs",                        TIF_LONG,      0},
    {   332,  "InkSet",                         TIF_SHORT,     0},
    {   333,  "InkNames",                       TIF_ASCII,     0},
    {   334,  "NumberOfInks",                   TIF_SHORT,     0},
    {   336,  "DotRange",                       TIF_BYTE,      0},
    {   337,  "TargetPrinter",                  TIF_ASCII,     0},
    {   338,  "ExtraSamples",                   TIF_SHORT,     0},
    {   339,  "SampleFormat",                   TIF_SHORT,     0},
    {   340,  "SMinSampleValue",                TIF_SHORT,     0},
    {   341,  "SMaxSampleValue",                TIF_SHORT,     0},
    {   342,  "TransferRange",                  TIF_SHORT,     0},
    {   343,  "ClipPath",                       TIF_BYTE,      0},
    {   344,  "XClipPathUnits",                 TIF_SSHORT,    1},
    {   345,  "YClipPathUnits",                 TIF_SSHORT,    1},
    {   346,  "Indexed",                        TIF_SHORT,     1},
    {   512,  "JPEGProc",                       TIF_LONG,      0},
    {   513,  "JPEGInterchangeFormat",          TIF_LONG,      1},
    {   514,  "JPEGInterchangeFormatLength",    TIF_LONG,      1},
    {   529,  "YCbCrCoefficients",              TIF_RATIONAL,  3},
    {   530,  "YCbCrSubSampling",               TIF_SHORT,     2},
    {   531,  "YCbCrPositioning",               TIF_SHORT,     1},
    {   532,  "ReferenceBlackWhite",            TIF_RATIONAL,  6},
    {   700,  "XMLPacket",                      TIF_BYTE,      1},
    {  4096,  "RelatedImageFileFormat",         TIF_ASCII,     1},
    { 33421,  "CFARepeatPatternDim",            TIF_SHORT,     0},
    { 33422,  "CFAPattern",                     TIF_BYTE,      0},
    { 33423,  "BatteryLevel",                   TIF_RATIONAL,  0},
    { 33432,  "Copyright",                      TIF_ASCII,     0},
    { 33434,  "ExposureTime",                   TIF_RATIONAL,  1},
    { 33437,  "FNumber",                        TIF_RATIONAL,  1},
    { 33723,  "IPTC/NAA",                       TIF_LONG,      0},
    { 34373,  "ImageResources",                 TIF_BYTE,      0},
    { 34665,  "ExifTag",                        TIF_LONG,      1},
    { 34675,  "InterColorProfile",              TIF_UNDEFINED, 0},
    { 34850,  "ExposureProgram",                TIF_SHORT,     1},
    { 34852,  "SpectralSensitivity",            TIF_ASCII,     0},
    { 34853,  "GPSTag",                         TIF_LONG,      1},
    { 34855,  "ISOSpeedRatings",                TIF_SHORT,     0},
    { 34856,  "OECF",                           TIF_UNDEFINED, 0},
    { 34857,  "Interlace",                      TIF_SHORT,     0},
    { 34858,  "TimeZoneOffset",                 TIF_SSHORT,    0},
    { 34859,  "SelfTimerMode",                  TIF_SHORT,     0},
    { 34864,  "SensitivityType",                TIF_SHORT,     0},
    { 34865,  "StandardOutputSensitivity",      TIF_LONG,      0},
    { 34866,  "RecommendedExposureIndex",       TIF_LONG,      0},
    { 34867,  "ISOSpeed",                       TIF_LONG,      0},
    { 34868,  "ISOSpeedLatitudeyyy",            TIF_LONG,      0},
    { 34869,  "ISOSpeedLatitudezzz",            TIF_LONG,      0},
    { 36864,  "ExifVersion",                    TIF_UNDEFINED, 4},
    { 36867,  "DateTimeOriginal",               TIF_ASCII,     20},
    { 36868,  "DateTimeDigitized",              TIF_ASCII,     20},
    { 37121,  "ComponentsConfiguration",        TIF_UNDEFINED, 4},
    { 37122,  "CompressedBitsPerPixel",         TIF_RATIONAL,  1},
    { 37377,  "ShutterSpeedValue",              TIF_SRATIONAL, 1},
    { 37378,  "ApertureValue",                  TIF_RATIONAL,  1},
    { 37379,  "BrightnessValue",                TIF_SRATIONAL, 1},
    { 37380,  "ExposureBiasValue",              TIF_SRATIONAL, 1},
    { 37381,  "MaxApertureValue",               TIF_RATIONAL,  1},
    { 37382,  "SubjectDistance",                TIF_RATIONAL,  1},
    { 37383,  "MeteringMode",                   TIF_SHORT,     1},
    { 37384,  "LightSource",                    TIF_SHORT,     1},
    { 37385,  "Flash",                          TIF_SHORT,     1},
    { 37386,  "FocalLength",                    TIF_RATIONAL,  1},
    { 37387,  "FlashEnergy",                    TIF_RATIONAL,  1},
    { 37388,  "SpatialFrequencyResponse",       TIF_UNDEFINED, 1},
    { 37389,  "Noise",                          TIF_UNDEFINED, 1},
    { 37390,  "FocalPlaneXResolution",          TIF_RATIONAL,  1},
    { 37391,  "FocalPlaneYResolution",          TIF_RATIONAL,  1},
    { 37392,  "FocalPlaneResolutionUnit",       TIF_SHORT,     1},
    { 37394,  "ImageNumber",                    TIF_LONG,      1},
    { 37393,  "SecurityClassification",         TIF_ASCII,     1},
    { 37395,  "ImageHistory",                   TIF_ASCII,     1},
    { 37396,  "SubjectLocation",                TIF_SHORT,     1},
    { 37397,  "ExposureIndex",                  TIF_RATIONAL,  1},
    { 37398,  "TIFFEPStandardID",               TIF_BYTE,      1},
    { 37399,  "SensingMethod",                  TIF_SHORT,     1},
    { 37500,  "MakerNote",                      TIF_UNDEFINED, 0},
    { 37510,  "UserComment",                    TIF_UNDEFINED, 0},
    { 37520,  "SubsecTime",                     TIF_ASCII,     0},
    { 37521,  "SubsecTimeOrginal",              TIF_ASCII,     0},
    { 37522,  "SubsecTimeDigitized",            TIF_ASCII,     0},
    { 40091,  "XPTitle",                        TIF_BYTE,      0},
    { 40092,  "XPComment",                      TIF_BYTE,      0},
    { 40093,  "XPAuthor",                       TIF_BYTE,      0},
    { 40094,  "XPKeywords",                     TIF_BYTE,      0},
    { 40095,  "XPSubject",                      TIF_BYTE,      0},
    { 40960,  "FlashPixVersion",                TIF_UNDEFINED, 4},
    { 40961,  "ColorSpace",                     TIF_SHORT,     1},
    { 40962,  "PixelXDimension",                TIF_UNKNOWN,   1},
    { 40963,  "PixelYDimension",                TIF_UNKNOWN,   1},
    { 40964,  "RelatedSoundFile",               TIF_ASCII,     13},
    { 40965,  "InteroperabilityTag",            TIF_LONG,      1},
    { 41838,  "FlashEnergy",                    TIF_RATIONAL,  1},
    { 41484,  "SpatialFrequencyResponse",       TIF_UNDEFINED, 0},
    { 41486,  "FocalPlaneXResolution",          TIF_RATIONAL,  1},
    { 41487,  "FocalPlaneYResolution",          TIF_RATIONAL,  1},
    { 41488,  "FocalPlaneResolutionUnit",       TIF_SHORT,     1},
    { 41492,  "SubjectLocation",                TIF_SHORT,     2},
    { 41493,  "ExposureIndex",                  TIF_RATIONAL,  1},
    { 41495,  "SensingMethod",                  TIF_SHORT,     1},
    { 41728,  "FileSource",                     TIF_UNDEFINED, 1},
    { 41729,  "SceneType",                      TIF_UNDEFINED, 1},
    { 41730,  "CFAPattern",                     TIF_UNDEFINED, 0},
    { 41985,  "CustomRendered",                 TIF_SHORT,     1},
    { 41986,  "ExposureMode",                   TIF_SHORT,     1},
    { 41987,  "WhiteBalance",                   TIF_SHORT,     1},
    { 41988,  "DigitalZoomRatio",               TIF_RATIONAL,  1},
    { 41989,  "FocalLenIn35mmFilm",             TIF_SHORT,     1},
    { 41990,  "SceneCaptureType",               TIF_SHORT,     1},
    { 41991,  "GainControl",                    TIF_SHORT,     1},
    { 41992,  "Contrast",                       TIF_SHORT,     1},
    { 41993,  "Saturation",                     TIF_SHORT,     1},
    { 41994,  "Sharpness",                      TIF_SHORT,     1},
    { 41995,  "DeviceSettingDescription",       TIF_UNDEFINED, 0},
    { 41996,  "SubjectDistRange",               TIF_SHORT,     1},
    { 42016,  "ImageUniqueID",                  TIF_ASCII,     33},
    { 42032,  "CameraOwnerName",                TIF_ASCII,     33},
    { 42033,  "BodySerialNumber",               TIF_ASCII,     33},
    { 42034,  "LensSpecification",              TIF_RATIONAL,  1},
    { 42035,  "LensMake",                       TIF_ASCII,     1},
    { 42036,  "LensModel",                      TIF_ASCII,     1},
    { 42037,  "LensSerialNumber",               TIF_ASCII,     1},
    { 50341,  "PrintImageMatching",             TIF_UNDEFINED, 0},
    { 50708,  "UniqueCameraModel",              TIF_ASCII,     1},
    { 50709,  "LocalizedCameraModel",           TIF_BYTE,      1},
};
static int numTifTags = sizeof(tifTags) / sizeof(TifTag);

static Tcl_Obj *
ConvertValueToObj(Tif *tifPtr, unsigned char *bp, unsigned int offset,
                  size_t length, enum TifTypes type)
{
    Tcl_Obj *objPtr;
    
    switch (type) {
    case TIF_BYTE:
        {
            unsigned int i;
            
            i = (unsigned int)bp[0];
            objPtr = Tcl_NewIntObj(i);
        }            
        break;

    case TIF_ASCII:
        {
            const char *string;
            char *p;
            int i;
            
            string = (const char *)tifPtr->start + offset;
            p = (char *)string;
            for (i = 0; i < length; i++) {
                if (*p == '\0') {
                    *p = '*';
                }
                p++;
            }
            objPtr = Tcl_NewStringObj(string, length);
        }
        break;

    case TIF_SHORT:
        {
            unsigned short s;

            s = TifGetShort(tifPtr, bp);
            objPtr = Tcl_NewIntObj(s);
        }
        break;

    case TIF_LONG:
        {
            unsigned long l;

            l = TifGetLong(tifPtr, bp);
            objPtr = Tcl_NewIntObj(l);
        }
        break;

    case TIF_RATIONAL:
        {
            unsigned long num, denom;
            double d;
            
            num = TifGetLong(tifPtr, bp);
            denom = TifGetLong(tifPtr, bp + 4);
            d = (double)num / (double)denom;
            objPtr = Tcl_NewDoubleObj(d);
        }
        break;
        
    case TIF_UNDEFINED:
        {
            unsigned char *bytes;
            
            bytes = bp + offset;
            objPtr = Tcl_NewByteArrayObj(bytes, length);
            break;
        }
        break;

    case TIF_SBYTE:
        {
            int i;
            
            i = (int)bp[0];
            objPtr = Tcl_NewIntObj(i);
        }            
        break;

    case TIF_SSHORT:
        {
            unsigned short s;

            s = TifGetShort(tifPtr, bp);
            objPtr = Tcl_NewIntObj(s);
        }
        break;

    case TIF_SLONG:
        {
            long l;

            l = TifGetLong(tifPtr, bp);
            objPtr = Tcl_NewIntObj(l);
        }
        break;
        
    case TIF_SRATIONAL:
        {
            long num, denom;
            double d;
            
            num = TifGetLong(tifPtr, bp);
            denom = TifGetLong(tifPtr, bp + 4);
            d = (double)num / (double)denom;
            objPtr = Tcl_NewDoubleObj(d);
        }
        break;

    case TIF_FLOAT:
    case TIF_DOUBLE:
    case TIF_UNKNOWN:
        fprintf(stderr, "not implemented\n");
        return NULL;
    }
    return objPtr;
}

static TifTag *
SearchForTagId(int id)
{
    int low, high;

    low = 0;
    high = numTifTags - 1;
    while (low <= high) {
        int median;
        
        median = (low + high) >> 1;
        if (id < tifTags[median].id) {
            high = median - 1;
        } else if (id > tifTags[median].id) {
            low = median + 1;
        } else {
            return tifTags + median;
        }
    }
    return NULL;                        /* Can't find id. */
}


static int
SetTagVariable(Tcl_Interp *interp, const char *varName, Tif *tifPtr,
               unsigned char *bp)
{
    int id, type, offset, numBytes;
    TifTag *tagPtr;
    Tcl_Obj *objPtr, *resultObjPtr;
    
    id = TifGetShort(tifPtr, bp);
    tagPtr = SearchForTagId(id);
    if (tagPtr == NULL) {
        fprintf(stderr, "can't find id %x\n", id);
        return TCL_CONTINUE;            /* Unknown tag id. */
    }
    type = TifGetShort(tifPtr, bp + 2);
    if (type != tagPtr->type) {
        fprintf(stderr, "tag=%s types don't match type=%d tagPtr->type=%d\n",
                tagPtr->tagName, type, tagPtr->type);
        return TCL_CONTINUE;            /* Tag types don't match. */
    }
    numBytes = TifGetLong(tifPtr, bp + 4);
    if ((tagPtr->size > 0) && (numBytes != tagPtr->size)) {
        fprintf(stderr, "tag=%s sizes don't match numbytes=%d size=%d\n",
                tagPtr->tagName, numBytes, tagPtr->size);
        return TCL_CONTINUE;            /* Sizes don't match. */
    }
    offset = TifGetLong(tifPtr, bp + 8);
#ifdef notdef
    fprintf(stderr, "tag=%s type=%s count=%d offset=%d\n",
            tagPtr->tagName, tifTypeStrings[tagPtr->type], numBytes, offset);
#endif
    objPtr = ConvertValueToObj(tifPtr, bp + 8, offset, numBytes, type);
    resultObjPtr = Tcl_SetVar2Ex(interp, varName,
                tagPtr->tagName, objPtr, TCL_LEAVE_ERR_MSG);
    if (resultObjPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
static int
ParseExif(Tcl_Interp *interp, const char *varName, Blt_DBuffer dbuffer)
{
    Tif tif;
    int id;
    int next;
    unsigned char *bp;
    
    /* 8-byte TIFF header */
    if (Blt_DBuffer_BytesLeft(dbuffer) < 14) {
        return FALSE;                     /* No Exif + TIF header. */
    }
    bp = Blt_DBuffer_Bytes(dbuffer);
    if (memcmp(bp, "Exif\0\0", 6) != 0) {
        fprintf(stderr, "no Exif header (%c%c%c%c) string=(%.*s)\n",
                bp[0], bp[1], bp[2], bp[3], Blt_DBuffer_Length(dbuffer),
                Blt_DBuffer_String(dbuffer));
        return FALSE;                     /* Bad Exif header */
    }
    bp += 6;
    if ((bp[0] == 'I') && (bp[1] == 'I')) {
        tif.bigendian = FALSE;          /* little endian */
    } else if ((bp[0] == 'M') && (bp[1] == 'M')) {
        tif.bigendian = TRUE;             /* big endian */
    } else {
        fprintf(stderr, "invalid byte order designation (%x%x)\n",
                bp[0], bp[1]);
        return FALSE;                     /* Invalid byte order
                                           * designation. */
    }
    id = TifGetShort(&tif, bp + 2);
    if (id != 42) {
        return FALSE;                     /* Incorrect byte order. */
    }
    tif.ifd0 = TifGetLong(&tif, bp + 4);
    if (tif.ifd0 >= Blt_DBuffer_Length(dbuffer)) {
        fprintf(stderr, "not enough room for ifd0 offset (%d)\n",
                tif.ifd0);
        return FALSE;                     /* Not enough room for offset. */
    }
    tif.dbuffer = dbuffer;
    next = tif.ifd0;
    tif.start = Blt_DBuffer_Bytes(dbuffer) + 6;
    do {
        int i;
        
        bp = Blt_DBuffer_Bytes(dbuffer) + 6 + next;
        tif.numEntries = TifGetShort(&tif, bp);
        if ((next + 4 + (tif.numEntries * 12)) >= Blt_DBuffer_Length(dbuffer)) {
            fprintf(stderr, "not enough room for next ifd directory (%d)\n",
                next);
            return FALSE;               /* Not enough room for directoy. */
        }
#ifdef notdef
        fprintf(stderr, "next=%d, number of entries=%d\n", next,
                tif.numEntries);
#endif
        bp += 2;
        for (i = 0; i < tif.numEntries; i++) {
            int result;
            
            result = SetTagVariable(interp, varName, &tif, bp);
            if (result == TCL_ERROR) {
                return FALSE;
            }
            bp += 12;
        }      
        next = TifGetLong(&tif, bp);
    } while (next > 0);  
    return TRUE;
}

/*
 * Marker processor for COM and interesting APPn markers.
 * This replaces the library's built-in processor, which just skips the marker.
 * We want to print out the marker as text, to the extent possible.
 * Note this code relies on a non-suspending data source.
 */
static int
jpeg_getc(j_decompress_ptr cinfo)
/* Read next byte */
{
  struct jpeg_source_mgr * datasrc = cinfo->src;

  if (datasrc->bytes_in_buffer == 0) {
    if (! (*datasrc->fill_input_buffer) (cinfo))
      ERREXIT(cinfo, JERR_CANT_SUSPEND);
  }
  datasrc->bytes_in_buffer--;
  return GETJOCTET(*datasrc->next_input_byte++);
}

static int
JpgPrintTextMarkerProc(j_decompress_ptr commPtr)
{
  int traceit = (commPtr->err->trace_level >= 1);
  int length;
  struct jpeg_source_mgr *datasrc = commPtr->src;
  JpgReader *readerPtr = (JpgReader *)commPtr->src;
  Blt_DBuffer dbuffer;
  
  traceit = 0;
  length = jpeg_getc(commPtr) << 8;
  length += jpeg_getc(commPtr);
  length -= 2;			/* discount the length word itself */

  if (traceit) {
    if (commPtr->unread_marker == JPEG_COM)
      fprintf(stderr, "Comment, length %ld:\n", (long) length);
    else			/* assume it is an APPn otherwise */
      fprintf(stderr, "APP%d, length %ld:\n",
	      commPtr->unread_marker - JPEG_APP0, (long) length);
  }
  traceit = 0;
  
  if (datasrc->bytes_in_buffer < length) {
      if (! (*datasrc->fill_input_buffer) (commPtr)) {
          ERREXIT(commPtr, JERR_CANT_SUSPEND);
      }
  }
  dbuffer = Blt_DBuffer_Create();
  Blt_DBuffer_AppendData(dbuffer, datasrc->next_input_byte, length);
  ParseExif(readerPtr->interp, readerPtr->varName, dbuffer);
  Blt_DBuffer_Destroy(dbuffer);
  datasrc->bytes_in_buffer -= length;
  datasrc->next_input_byte += length;
  return TRUE;
}

static void
JpgErrorProc(j_common_ptr commPtr)
{
    JpgErrorHandler *errorPtr = (JpgErrorHandler *)commPtr->err;

    (*errorPtr->pub.output_message) (commPtr);
    longjmp(errorPtr->jmpbuf, 1);
}

static void
JpgMessageProc(j_common_ptr commPtr)
{
    JpgErrorHandler *errorPtr = (JpgErrorHandler *)commPtr->err;
    char mesg[JMSG_LENGTH_MAX];

    /* Create the message and append it into the dynamic string. */
    (*errorPtr->pub.format_message) (commPtr, mesg);
    Tcl_DStringAppend(&errorPtr->ds, " ", -1);
    Tcl_DStringAppend(&errorPtr->ds, mesg, -1);
}

static void
JpgInitSource(j_decompress_ptr commPtr)
{
    JpgReader *readerPtr = (JpgReader *)commPtr->src;

    readerPtr->pub.next_input_byte = Blt_DBuffer_Bytes(readerPtr->dBuffer);
    readerPtr->pub.bytes_in_buffer = Blt_DBuffer_Length(readerPtr->dBuffer);
}

static boolean
JpgFillInputBuffer(j_decompress_ptr commPtr)
{
    JpgReader *readerPtr = (JpgReader *)commPtr->src; 
    static unsigned char eoi[2] = { 0xFF, JPEG_EOI };

    /* Insert a fake EOI marker */
    readerPtr->pub.next_input_byte = eoi;
    readerPtr->pub.bytes_in_buffer = 2;
    return TRUE;
}

static void
JpgSkipInputData(j_decompress_ptr commPtr, long numBytes)
{
    if (numBytes > 0) {
        JpgReader *readerPtr = (JpgReader *)commPtr->src; 

        if ((readerPtr->pub.next_input_byte + numBytes) >= 
            (Blt_DBuffer_Bytes(readerPtr->dBuffer) + 
             Blt_DBuffer_Length(readerPtr->dBuffer))) {
            char mesg[200];
            JpgErrorHandler *errorPtr = (JpgErrorHandler *)commPtr->err;
            
            sprintf(mesg, "short buffer: wanted %lu bytes, bytes left is %lu",
                    (unsigned long)numBytes,
                    (unsigned long)Blt_DBuffer_Length(readerPtr->dBuffer));
            Tcl_DStringAppend(&errorPtr->ds, " ", -1);
            Tcl_DStringAppend(&errorPtr->ds, mesg, -1);
            ERREXIT(commPtr, 10);
        }
        readerPtr->pub.next_input_byte += (size_t)numBytes;
        readerPtr->pub.bytes_in_buffer -= (size_t)numBytes;
    }
}

static void
JpgTermSource (j_decompress_ptr commPtr)
{
    /* Nothing to do. */
}

static void
JpgSetSourceFromBuffer(j_decompress_ptr commPtr, Blt_DBuffer buffer)
{
    JpgReader *readerPtr;
    
    /* The source object is made permanent so that a series of JPEG images
     * can be read from the same file by calling jpeg_stdio_src only before
     * the first one.  (If we discarded the buffer at the end of one image,
     * we'd likely lose the start of the next one.)  This makes it unsafe
     * to use this manager and a different source manager serially with the
     * same JPEG object.  Caveat programmer.
     */
    if (commPtr->src == NULL) {      /* First time for this JPEG object? */
        commPtr->src = (struct jpeg_source_mgr *)
            (*commPtr->mem->alloc_small) ((j_common_ptr)commPtr, 
                JPOOL_PERMANENT, sizeof(JpgReader));
        readerPtr = (JpgReader *)commPtr->src;
    }
    readerPtr = (JpgReader *)commPtr->src;
    readerPtr->dBuffer = buffer;
    readerPtr->pub.init_source = JpgInitSource;
    readerPtr->pub.fill_input_buffer = JpgFillInputBuffer;
    readerPtr->pub.skip_input_data = JpgSkipInputData;
    /* use default method */
    readerPtr->pub.resync_to_restart = jpeg_resync_to_restart; 
    readerPtr->pub.term_source = JpgTermSource;
}

static void
JpgInitDestination (j_compress_ptr commPtr)
{
    JpgWriter *writerPtr = (JpgWriter *)commPtr->dest;

    writerPtr->bytes = (JOCTET *)(*commPtr->mem->alloc_small) 
        ((j_common_ptr) commPtr, JPOOL_IMAGE, JPG_BUF_SIZE * sizeof(JOCTET));
    writerPtr->pub.next_output_byte = writerPtr->bytes;
    writerPtr->pub.free_in_buffer = JPG_BUF_SIZE;
}

static boolean
JpgEmptyOutputBuffer(j_compress_ptr commPtr)
{
    JpgWriter *writerPtr = (JpgWriter *)commPtr->dest;

    if (!Blt_DBuffer_AppendData(writerPtr->dBuffer, writerPtr->bytes, 
        JPG_BUF_SIZE)) {
        ERREXIT(commPtr, 10);
    }
    writerPtr->pub.next_output_byte = writerPtr->bytes;
    writerPtr->pub.free_in_buffer = JPG_BUF_SIZE;
    return TRUE;
}

static void
JpgTermDestination (j_compress_ptr commPtr)
{
    JpgWriter *writerPtr = (JpgWriter *)commPtr->dest;
    size_t numBytes = JPG_BUF_SIZE - writerPtr->pub.free_in_buffer;
    
    /* Write any data remaining in the buffer */
    if (numBytes > 0) {
        if (!Blt_DBuffer_AppendData(writerPtr->dBuffer, writerPtr->bytes, 
                numBytes)) {
            ERREXIT(commPtr, 10);
        }
    }
}

static void
JpgSetDestinationToBuffer(j_compress_ptr commPtr, Blt_DBuffer buffer)
{
    JpgWriter *writerPtr;
    
    /* The destination object is made permanent so that multiple JPEG
     * images can be written to the same file without re-executing
     * jpeg_stdio_dest.  This makes it dangerous to use this manager and a
     * different destination manager serially with the same JPEG object,
     * because their private object sizes may be different.  Caveat
     * programmer.
     */
    if (commPtr->dest == NULL) {        /* first time for this JPEG object? */
        commPtr->dest = (struct jpeg_destination_mgr *)
            (*commPtr->mem->alloc_small) ((j_common_ptr)commPtr, 
                        JPOOL_PERMANENT, sizeof(JpgWriter));
    }
    writerPtr = (JpgWriter *)commPtr->dest;
    writerPtr->pub.init_destination = JpgInitDestination;
    writerPtr->pub.empty_output_buffer = JpgEmptyOutputBuffer;
    writerPtr->pub.term_destination = JpgTermDestination;
    writerPtr->dBuffer = buffer;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsJpg --
 *
 *      Attempts to parse a JPG file header.
 *
 * Results:
 *      Returns 1 is the header is JPG and 0 otherwise.  Note that the
 *      validity of the header contents is not checked here.  That's done
 *      in JpgToPicture.
 *
 *---------------------------------------------------------------------------
 */
static int
IsJpg(Blt_DBuffer buffer)
{
    JpgErrorHandler error;
    struct jpeg_decompress_struct cinfo;
    int bool;

    /* Step 1: allocate and initialize JPEG decompression object */
    
    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.dct_method = JDCT_IFAST;
    cinfo.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpgErrorProc;
    error.pub.output_message = JpgMessageProc;
    
    /* Initialize possible error message */
    bool = FALSE;
    Tcl_DStringInit(&error.ds);
    if (setjmp(error.jmpbuf)) {
        goto done;
    }
    jpeg_create_decompress(&cinfo);
    JpgSetSourceFromBuffer(&cinfo, buffer);
    bool = (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK);
 done:
    jpeg_destroy_decompress(&cinfo);
    Tcl_DStringFree(&error.ds);
    return bool;
}

/*
 *---------------------------------------------------------------------------
 *
 * JpgToPicture --
 *
 *      Reads a JPEG data buffer and converts it into a picture.
 *
 * Results:
 *      The picture is returned.  If an error occured, such as the
 *      designated file could not be opened, NULL is returned.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
JpgToPicture(
    Tcl_Interp *interp,         /* Interpreter to report errors back to. */
    const char *fileName,       /* Name of file used to fill the dynamic
                                 * buffer.  */
    Blt_DBuffer buffer,         /* Contents of the above file. */
    JpgImportSwitches *switchesPtr)
{
    Blt_Chain chain;
    JSAMPLE **rows;
    JpgErrorHandler error;
    Picture *destPtr;
    int samplesPerRow;
    struct jpeg_decompress_struct cinfo;
    unsigned int width, height;
    Blt_Pixel *destRowPtr;
    JpgReader *readerPtr;
    
    destPtr = NULL;

    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.dct_method = switchesPtr->method;
    cinfo.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpgErrorProc;
    error.pub.output_message = JpgMessageProc;

    /* Initialize possible error message */
    Tcl_DStringInit(&error.ds);
    Tcl_DStringAppend(&error.ds, "error reading \"", -1);
    Tcl_DStringAppend(&error.ds, fileName, -1);
    Tcl_DStringAppend(&error.ds, "\": ", -1);

    if (setjmp(error.jmpbuf)) {
    error:
        jpeg_destroy_decompress(&cinfo);
        Tcl_DStringResult(interp, &error.ds);
        return NULL;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_set_marker_processor(&cinfo, JPEG_COM, JpgPrintTextMarkerProc);
    jpeg_set_marker_processor(&cinfo, JPEG_APP0+1, JpgPrintTextMarkerProc);

    JpgSetSourceFromBuffer(&cinfo, buffer);
    readerPtr = (JpgReader *)cinfo.src;
    readerPtr->interp = interp;
    readerPtr->varName = "exiftags";
    jpeg_read_header(&cinfo, TRUE);     /* Step 3: read file parameters */

    jpeg_start_decompress(&cinfo);      /* Step 5: Start decompressor */
    width = cinfo.output_width;
    height = cinfo.output_height;
    if ((width < 1) || (height < 1)) {
        Tcl_AppendResult(interp, "error reading \"", fileName, 
                "\": bad JPEG image size", (char *)NULL);
        return NULL;
    }
    /* JSAMPLEs per row in output buffer */
    samplesPerRow = width * cinfo.output_components;

    /* Make a one-row-high sample array that will go away when done with
     * image */
    rows = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, 
        samplesPerRow, 1);
    destPtr = Blt_CreatePicture(width, height);
    destRowPtr = destPtr->bits;
    switch (cinfo.output_components) {
    case 1:
        while (cinfo.output_scanline < height) {
            JSAMPLE *bp;
            Blt_Pixel *dp;
            int i;

            dp = destRowPtr;
            jpeg_read_scanlines(&cinfo, rows, 1);
            bp = rows[0];
            for (i = 0; i < (int)width; i++) {
                dp->Red = dp->Green = dp->Blue = *bp++;
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        break;
    case 3:
        while (cinfo.output_scanline < height) {
            JSAMPLE *bp;
            Blt_Pixel *dp;
            int i;
            
            dp = destRowPtr;
            jpeg_read_scanlines(&cinfo, rows, 1);
            bp = rows[0];
            for (i = 0; i < (int)width; i++) {
                dp->Red = *bp++;
                dp->Green = *bp++;
                dp->Blue = *bp++;
                dp->Alpha = ALPHA_OPAQUE;
                dp++;
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        destPtr->flags |= BLT_PIC_COLOR;
        break;
    case 4:
        while (cinfo.output_scanline < height) {
            JSAMPLE *bp;
            Blt_Pixel *dp;
            int i;
            
            dp = destRowPtr;
            jpeg_read_scanlines(&cinfo, rows, 1);
            bp = rows[0];
            for (i = 0; i < (int)width; i++) {
                dp->Red = *bp++;
                dp->Green = *bp++;
                dp->Blue = *bp++;
                dp->Alpha = *bp++;
                dp++;
            }
            destRowPtr += destPtr->pixelsPerRow;
        }
        destPtr->flags |= BLT_PIC_COLOR | BLT_PIC_BLEND;
        break;
    default:
        Tcl_AppendResult(interp, "\"", fileName, "\": ",
                         "don't know how to handle JPEG image with ", 
                        Blt_Itoa(cinfo.output_components), 
                        " output components.", (char *)NULL);
        Blt_FreePicture(destPtr);
        goto error;
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    if (error.pub.num_warnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "JPG_READ_WARNINGS", 
                Tcl_DStringValue(&error.ds), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&error.ds);
    chain = Blt_Chain_Create();
    /* Opaque image, set associate colors flag.  */
    destPtr->flags |= BLT_PIC_ASSOCIATED_COLORS;
    Blt_Chain_Append(chain, destPtr);
    destPtr->flags &= ~BLT_PIC_UNINITIALIZED;
    return chain;
}

/*
 *---------------------------------------------------------------------------
 *
 * PictureToJpg --
 *
 *      Writes a JPEG format image to the provided dynamic buffer.
 *
 * Results:
 *      A standard TCL result.  If an error occured, TCL_ERROR is returned
 *      and an error message will be place in the interpreter
 *      result. Otherwise, the dynamic buffer will contain the binary
 *      output of the image.
 *
 * Side Effects:
 *      Memory is allocated for the dynamic buffer.
 *
 *---------------------------------------------------------------------------
 */
static int
PictureToJpg(
    Tcl_Interp *interp,         /* Interpreter to report errors back to. */
    Blt_Picture original,       /* Picture source. */
    Blt_DBuffer buffer,         /* Destination buffer to contain the JPEG
                                 * image.  */
    JpgExportSwitches *switchesPtr)
{
    JpgErrorHandler error;
    int result;
    struct jpeg_compress_struct cinfo;
    Picture *srcPtr;

    result = TCL_ERROR;
    srcPtr = original;

    /* Step 1: allocate and initialize JPEG compression object */
    cinfo.err = jpeg_std_error(&error.pub);
    error.pub.error_exit = JpgErrorProc;
    error.pub.output_message = JpgMessageProc;

    /* Initialize possible error message */
    Tcl_DStringInit(&error.ds);
    Tcl_DStringAppend(&error.ds, "error writing jpg: ", -1);

    if (setjmp(error.jmpbuf)) {
        /* Transfer the error message to the interpreter result. */
        Tcl_DStringResult(interp, &error.ds);
        goto bad;
    }

    /* Now we can initialize the JPEG compression object. */
    jpeg_create_compress(&cinfo);
    
    /* Step 2: specify data destination (eg, a file) */
    
    JpgSetDestinationToBuffer(&cinfo, buffer);
    
    /* Step 3: set parameters for compression */
    
    /* First we supply a description of the input image.  Four fields of the
     * cinfo struct must be filled in:
     */
    cinfo.image_width = srcPtr->width;
    cinfo.image_height = srcPtr->height;

    if (!Blt_Picture_IsOpaque(srcPtr)) {
        Blt_Picture background;

        /* Blend picture with solid color background. */
        background = Blt_CreatePicture(srcPtr->width, srcPtr->height);
        Blt_BlankPicture(background, switchesPtr->bg.u32); 
        Blt_BlendRegion(background, srcPtr, 0, 0, srcPtr->width, srcPtr->height,
                        0, 0);
        if (srcPtr != original) {
            Blt_FreePicture(srcPtr);
        }
        srcPtr = background;
    }
    if (srcPtr->flags & BLT_PIC_ASSOCIATED_COLORS) {
        Blt_Picture unassoc;
        /* 
         * The picture has an alpha burned into the components.  Create a
         * temporary copy removing pre-multiplied alphas.
         */ 
        unassoc = Blt_ClonePicture(srcPtr);
        Blt_UnassociateColors(unassoc);
        if (srcPtr != original) {
            Blt_FreePicture(srcPtr);
        }
        srcPtr = unassoc;
    }
    Blt_QueryColors(srcPtr, (Blt_HashTable *)NULL);
    if (Blt_Picture_IsColor(srcPtr)) {
        cinfo.input_components = 3;   /* # of color components per pixel */
        cinfo.in_color_space = JCS_RGB; /* Colorspace of input image */
    } else {
        cinfo.input_components = 1;   /* # of color components per pixel */
        cinfo.in_color_space = JCS_GRAYSCALE; /* Colorspace of input image */
    }   
    jpeg_set_defaults(&cinfo);

    /* 
     * Now you can set any non-default parameters you wish to.  Here we
     * just illustrate the use of quality (quantization table) scaling:
     */

    /* limit to baseline-JPEG values */
    jpeg_set_quality(&cinfo, switchesPtr->quality, TRUE);
    if (switchesPtr->flags & PIC_PROGRESSIVE) {
        jpeg_simple_progression(&cinfo);
    }
    if (switchesPtr->smoothing > 0) {
        cinfo.smoothing_factor = switchesPtr->smoothing;
    }
    /* Step 4: Start compressor */

    jpeg_start_compress(&cinfo, TRUE);

    /* Step 5: while (scan lines remain to be written) */
    /*           jpeg_write_scanlines(...); */
    {
        int y;
        int row_stride;
        JSAMPLE *destRow;
        Blt_Pixel *srcRowPtr;
        JSAMPROW row_pointer[1];        /* pointer to JSAMPLE row[s] */

        /* JSAMPLEs per row in image_buffer */
        row_stride = srcPtr->width * cinfo.input_components;
        destRow = Blt_AssertMalloc(sizeof(JSAMPLE) * row_stride);
        srcRowPtr = srcPtr->bits;
        if (cinfo.input_components == 3) {
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                JSAMPLE *dp;
                
                dp = destRow;
                for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; 
                     sp++) {
                    dp[0] = sp->Red;
                    dp[1] = sp->Green;
                    dp[2] = sp->Blue;
                    dp += 3;
                }
                row_pointer[0] = destRow;
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
                srcRowPtr += srcPtr->pixelsPerRow;
            }
        } else {
            for (y = 0; y < srcPtr->height; y++) {
                Blt_Pixel *sp, *send;
                JSAMPLE *dp;
                
                dp = destRow;
                for (sp = srcRowPtr, send = sp + srcPtr->width; sp < send; 
                     sp++) {
                    *dp++ = sp->Red;
                }
                row_pointer[0] = destRow;
                jpeg_write_scanlines(&cinfo, row_pointer, 1);
                srcRowPtr += srcPtr->pixelsPerRow;
            }
        }
        Blt_Free(destRow);
    }
    /* Step 6: Finish compression */
    jpeg_finish_compress(&cinfo);
    result = TCL_OK;
 bad:
    /* Step 7: release JPEG compression object */
    jpeg_destroy_compress(&cinfo);

    if (error.pub.num_warnings > 0) {
        Tcl_SetErrorCode(interp, "PICTURE", "JPG_WRITE_WARNINGS", 
                Tcl_DStringValue(&error.ds), (char *)NULL);
    } else {
        Tcl_SetErrorCode(interp, "NONE", (char *)NULL);
    }
    Tcl_DStringFree(&error.ds);
    if (srcPtr != original) {
        Blt_FreePicture(srcPtr);
    }
    return result;
}

static Blt_Chain
ReadJpg(Tcl_Interp *interp, const char *fileName, Blt_DBuffer dbuffer)
{
    JpgImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    return JpgToPicture(interp, fileName, dbuffer, &switches);
}

static Tcl_Obj *
WriteJpg(Tcl_Interp *interp, Blt_Picture picture)
{
    Tcl_Obj *objPtr;
    Blt_DBuffer dbuffer;
    JpgExportSwitches switches;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.quality = 100;
    switches.smoothing = 0;
    switches.flags = 0;                /* No progressive or compression. */
    switches.bg.u32 = 0xFFFFFFFF;      /* White */

    objPtr = NULL;
    dbuffer = Blt_DBuffer_Create();
    if (PictureToJpg(interp, picture, dbuffer, &switches) == TCL_OK) {
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
    }
    Blt_DBuffer_Destroy(dbuffer);
    return objPtr;
}

static Blt_Chain
ImportJpg(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv, 
          const char **fileNamePtr)
{
    Blt_DBuffer dbuffer;
    Blt_Chain chain;
    const char *string;
    JpgImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    switches.method = JDCT_ISLOW;       /* Default method. */
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
        return NULL;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one import source: ",
                "use only one -file or -data flag.", (char *)NULL);
        Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
        return NULL;
    }
    dbuffer = Blt_DBuffer_Create();
    chain = NULL;
    if (switches.dataObjPtr != NULL) {
        unsigned char *bytes;
        int numBytes;

        bytes = Tcl_GetByteArrayFromObj(switches.dataObjPtr, &numBytes);
        string = (const char *)bytes;
        if (Blt_IsBase64(string, numBytes)) {
            if (Blt_DBuffer_Base64Decode(interp, string, numBytes, dbuffer) 
                != TCL_OK) {
                goto error;
            }
        } else {
            Blt_DBuffer_AppendData(dbuffer, bytes, numBytes);
        } 
        string = "data buffer";
        *fileNamePtr = NULL;
    } else {
        string = Tcl_GetString(switches.fileObjPtr);
        *fileNamePtr = string;
        if (Blt_DBuffer_LoadFile(interp, string, dbuffer) != TCL_OK) {
            goto error;
        }
    }
    chain = JpgToPicture(interp, string, dbuffer, &switches);
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return chain;
}

static int
ExportJpg(Tcl_Interp *interp, int index, Blt_Chain chain, int objc,
          Tcl_Obj *const *objv)
{
    Blt_DBuffer dbuffer;
    Blt_Picture picture;
    JpgExportSwitches switches;
    int result;

    /* Default export switch settings. */
    memset(&switches, 0, sizeof(switches));
    switches.quality = 100;
    switches.smoothing = 0;
    switches.flags = 0;                /* No progressive or compression. */
    switches.bg.u32 = 0xFFFFFFFF;      /* White */
    switches.index = index;

    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
        return TCL_ERROR;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "more than one export destination: ",
                "use only one -file or -data flag.", (char *)NULL);
        Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
        return TCL_ERROR;
    }
    picture = Blt_GetNthPicture(chain, switches.index);
    if (picture == NULL) {
        Tcl_AppendResult(interp, "no picture at index ", 
                Blt_Itoa(switches.index), (char *)NULL);
        Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
        return TCL_ERROR;
    }
    if (switches.quality == 0) {
        switches.quality = 100;         /* Default quality setting. */
    } else if (switches.quality > 100) {
        switches.quality = 100;         /* Maximum quality setting. */
    }
    if (switches.smoothing > 100) {
        switches.smoothing = 100;       /* Maximum smoothing setting. */
    }

    dbuffer = Blt_DBuffer_Create();
    result = PictureToJpg(interp, picture, dbuffer, &switches);
    if (result != TCL_OK) {
        Tcl_AppendResult(interp, "can't convert \"", 
                Tcl_GetString(objv[2]), "\"", (char *)NULL);
        goto error;
    }

    if (switches.fileObjPtr != NULL) {
        const char *fileName;

        /* Write the image into the designated file. */
        fileName = Tcl_GetString(switches.fileObjPtr);
        result = Blt_DBuffer_SaveFile(interp, fileName, dbuffer);
    } else if (switches.dataObjPtr != NULL) {
        Tcl_Obj *objPtr;

        /* Write the image into the designated TCL variable. */
        objPtr = Tcl_ObjSetVar2(interp, switches.dataObjPtr, NULL, 
                Blt_DBuffer_ByteArrayObj(dbuffer), 0);
        result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
    } else {
        Tcl_Obj *objPtr;

        /* Return the image as a base64 string in the interpreter result. */
        result = TCL_ERROR;
        objPtr = Blt_DBuffer_Base64EncodeToObj(dbuffer);
        if (objPtr != NULL) {
            Tcl_SetObjResult(interp, objPtr);
            result = TCL_OK;
        }
    }
 error:
    Blt_FreeSwitches(exportSwitches, (char *)&switches, 0);
    Blt_DBuffer_Destroy(dbuffer);
    return result;
}

int
Blt_PictureJpgInit(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_COMPILED, PKG_ANY) == NULL) {
        return TCL_ERROR;
    };
#endif
#ifdef USE_BLT_STUBS
    if (Blt_InitTclStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
    if (Blt_InitTkStubs(interp, BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    };
#else
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
    if (Tcl_PkgRequire(interp, "blt_tk", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
#endif    
    if (Tcl_PkgProvide(interp, "blt_picture_jpg", BLT_VERSION) != TCL_OK) { 
        return TCL_ERROR;
    }
    return Blt_PictureRegisterFormat(interp,
        "jpg",                          /* Name of format. */
        IsJpg,                          /* Format discovery procedure. */
        ReadJpg,                        /* Read format procedure. */
        WriteJpg,                       /* Write format procedure. */
        ImportJpg,                      /* Import format procedure. */
        ExportJpg);                     /* Export format procedure. */
}

int 
Blt_PictureJpgSafeInit(Tcl_Interp *interp) 
{
    return Blt_PictureJpgInit(interp);
}

#endif /* HAVE_LIBJPG */

