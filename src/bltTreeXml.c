/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeXml.c --
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
 */

#include <bltInt.h>
#include "config.h"

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#ifdef HAVE_STDDEF_H
#  include <stddef.h>
#endif /* HAVE_STDDEF_H */

#include <tcl.h>
#include <bltSwitch.h>
#include <bltAssert.h>
#include <bltAlloc.h>
#include <bltTree.h>
#include <string.h>


#define TRUE    1
#define FALSE   0
/*
 * The macro below is used to modify a "char" value (e.g. by casting
 * it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */
#define UCHAR(c) ((unsigned char) (c))

DLLEXPORT extern Tcl_AppInitProc Blt_TreeXmlInit;
DLLEXPORT extern Tcl_AppInitProc Blt_TreeXmlSafeInit;

static Blt_TreeImportProc ImportXmlProc;
static Blt_TreeExportProc ExportXmlProc;

/*
 * Format       Import          Export
 * xml          file/data       file/data
 * html         file/data       file/data
 *
 * $tree import xml $node fileName -data dataString 
 * $table export xml $node -file defaultFileName 
 * $tree import html $node -file fileName -data dataString 
 * $table export html $node -file defaultFileName 
 */

/*
 * ImportSwitches --
 */
typedef struct {
    Tcl_Obj *fileObj;   /* Name of file representing the channel. */
    Tcl_Obj *dataObj;
    Tcl_Interp *interp;
    unsigned int flags;
    Blt_TreeNode root;
} ImportSwitches;

#define IMPORT_TRIMCDATA  (1<<0)
#define IMPORT_OVERWRITE  (1<<1)

#define IMPORT_ATTRIBUTES (1L<<2)
#define IMPORT_BASEURI    (1L<<3)
#define IMPORT_CDATA      (1L<<4)
#define IMPORT_COMMENTS   (1L<<5)
#define IMPORT_DECL       (1L<<6)
#define IMPORT_DTD        (1L<<7)
#define IMPORT_LOCATION   (1L<<8)
#define IMPORT_PI         (1L<<9)
#define IMPORT_NS         (1L<<10)
#define IMPORT_EXTREF     (1L<<11)
#define IMPORT_ALL        (IMPORT_ATTRIBUTES | IMPORT_COMMENTS | IMPORT_CDATA |\
                           IMPORT_DTD | IMPORT_PI | IMPORT_LOCATION | \
                           IMPORT_BASEURI | IMPORT_DECL | IMPORT_EXTREF)

#define SYM_BASEURI       "#baseuri"
#define SYM_BYTEIDX       "#byteindex"
#define SYM_CDATA         "#cdata"
#define SYM_COLNO         "#column"
#define SYM_COMMENT       "#comment"
#define SYM_LINENO        "#line"
#define SYM_NS            "#namespace"
#define SYM_NOTATION      "#notation"
#define SYM_PI            "#pi"
#define SYM_PUBID         "#publicid"
#define SYM_SYSID         "#systemid"
#define SYM_VERSION       "#version"
#define SYM_ENCODING      "#encoding"
#define SYM_STANDALONE    "#standalone"

static Blt_SwitchParseProc TreeNodeSwitchProc;

static Blt_SwitchCustom nodeSwitch = {
    TreeNodeSwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec importSwitches[] = 
{
    {BLT_SWITCH_BITMASK,  "-all",               "", (char *)NULL, 
        Blt_Offset(ImportSwitches, flags), 0, IMPORT_ALL},
    {BLT_SWITCH_BOOLEAN,  "-comments",          "bool", (char *)NULL, 
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_COMMENTS},
    {BLT_SWITCH_OBJ,      "-data",              "data", (char *)NULL,
        Blt_Offset(ImportSwitches, dataObj),    0, 0},
    {BLT_SWITCH_BOOLEAN,  "-declaration",       "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_DECL},
    {BLT_SWITCH_BOOLEAN,  "-extref",            "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_EXTREF},
    {BLT_SWITCH_OBJ,      "-file",              "fileName", (char *)NULL,
        Blt_Offset(ImportSwitches, fileObj),    0, 0},
    {BLT_SWITCH_BOOLEAN,  "-locations",         "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_LOCATION},
    {BLT_SWITCH_CUSTOM,   "-root",              "node", (char *)NULL,
        Blt_Offset(ImportSwitches, root),       0, 0, &nodeSwitch},
    {BLT_SWITCH_BOOLEAN,  "-attributes",        "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_ATTRIBUTES},
    {BLT_SWITCH_BOOLEAN,  "-namespace",         "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_NS},
    {BLT_SWITCH_BOOLEAN,  "-cdata",             "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_CDATA},
    {BLT_SWITCH_BOOLEAN,  "-overwrite", "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_OVERWRITE},
    {BLT_SWITCH_BOOLEAN,  "-processinginstructions",  "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_PI},
    {BLT_SWITCH_BOOLEAN,  "-trimwhitespace",    "bool", (char *)NULL,
        Blt_Offset(ImportSwitches, flags),      0, IMPORT_TRIMCDATA},
    {BLT_SWITCH_END}
};

/*
 * XmlWriter --
 */
typedef struct {
    Tcl_Obj *fileObj;
    Tcl_Obj *dataObj;
    Blt_TreeNode root;

    /* Private fields. */
    Tcl_Interp *interp;
    unsigned int flags;
    Tcl_Channel channel;        /* If non-NULL, channel to write output to. */
    Blt_DBuffer dbuffer;
    int indent;
} XmlWriter;

#define LAST_END_TAG           (1<<2)
#define EXPORT_ROOT            (1<<3)
#define EXPORT_DECLARATION     (1<<4)

static Blt_SwitchSpec exportSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
        Blt_Offset(XmlWriter, dataObj), 0, 0},
    {BLT_SWITCH_BITMASK, "-declaration", "", (char *)NULL,
        Blt_Offset(XmlWriter, flags),   0, EXPORT_DECLARATION},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(XmlWriter, fileObj), 0, 0},
    {BLT_SWITCH_BITMASK_INVERT, "-hideroot", "", (char *)NULL,
        Blt_Offset(XmlWriter, flags),   0, EXPORT_ROOT},
    {BLT_SWITCH_INT_POS, "-indent", "number", (char *)NULL,
        Blt_Offset(XmlWriter, indent),  0, 0},
    {BLT_SWITCH_CUSTOM,   "-root",              "node", (char *)NULL,
        Blt_Offset(XmlWriter, root),    0, 0, &nodeSwitch},
    {BLT_SWITCH_END}
};


#ifdef HAVE_LIBEXPAT
#include <expat.h>

typedef struct {
    Blt_Tree tree;
    Blt_TreeNode root;
    Blt_TreeNode parent;
    Tcl_Interp *interp;
    int flags;
    Blt_HashTable stringTable;
    XML_Parser parser;
} XmlReader;

/*
 *---------------------------------------------------------------------------
 *
 * TreeNodeSwitchProc --
 *
 *      Convert a Tcl_Obj representing a node number into its integer value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeNodeSwitchProc(ClientData clientData, Tcl_Interp *interp,
                   const char *switchName, Tcl_Obj *objPtr, char *record,
                   int offset, int flags)
{
    Blt_TreeNode *nodePtr = (Blt_TreeNode *)(record + offset);
    Blt_Tree tree  = clientData;

    return Blt_Tree_GetNodeFromObj(interp, tree, objPtr, nodePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetStringObj --
 *
 *      Returns a hashed Tcl_Obj from the given string. Many character
 *      strings in the XML tree will be the same.  we generate only one
 *      Tcl_Obj for each unique string.  Returns a reference counted
 *      Tcl_Obj.
 *
 * Results:
 *      The pointer to the string Tcl_Obj.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
GetStringObj(XmlReader *readerPtr, const char *string)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&readerPtr->stringTable, string, &isNew);
    if (isNew) {
        Tcl_Obj *objPtr;

        objPtr = Tcl_NewStringObj(string, -1);
        Tcl_IncrRefCount(objPtr);
        Blt_SetHashValue(hPtr, objPtr);
        return objPtr;
    }
    return Blt_GetHashValue(hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetBaseUri --
 *
 *      Searches for the closest SYM_BASEURI data field in the tree to the
 *      given node.  
 *
 * Results:
 *      The base URI is returned if found, otherwise NULL.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GetBaseUri(XmlReader *readerPtr, Blt_TreeNode node)
{
    Blt_TreeNode top;

    top = Blt_Tree_ParentNode(readerPtr->root);
    do {
        if (Blt_Tree_ValueExists(readerPtr->tree, node, SYM_BASEURI)) {
            Tcl_Obj *objPtr;

            if (Blt_Tree_GetValue((Tcl_Interp *)NULL, readerPtr->tree, node, 
                        SYM_BASEURI, &objPtr) == TCL_OK) {
                return Tcl_GetString(objPtr);
            }
        }
        node = Blt_Tree_ParentNode(node);
    } while (node != top);
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetLocation --
 *
 *      Adds the line, column, and byte index of the XML to the tree. This
 *      makes it easier to debug in the XML specific information came from.
 *
 *---------------------------------------------------------------------------
 */
static void
SetLocation(XmlReader *readerPtr, Blt_TreeNode node)
{
    Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, node, SYM_LINENO, 
        Tcl_NewIntObj(XML_GetCurrentLineNumber(readerPtr->parser)));
    Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, node, SYM_COLNO, 
        Tcl_NewIntObj(XML_GetCurrentColumnNumber(readerPtr->parser)));
    Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, node, SYM_BYTEIDX, 
        Tcl_NewLongObj(XML_GetCurrentByteIndex(readerPtr->parser)));
}


/*
 *---------------------------------------------------------------------------
 *
 * TrimWhitespace --
 *
 *      Trims leading and trailing whitespace from all the CDATA nodes 
 *      in the tree.  This is done after the entire XML input has been
 *      processed.
 *
 *---------------------------------------------------------------------------
 */
static void
TrimWhitespace(XmlReader *readerPtr)
{
    Blt_TreeNode root, node;

    root = readerPtr->root;
    for (node = root; node != NULL; node = Blt_Tree_NextNode(root, node)) {
        if (strcmp(Blt_Tree_NodeLabel(node), SYM_CDATA) == 0) {
            Tcl_Obj *objPtr, *newPtr;
            int length;
            const char *first, *last, *pend, *string;
            if (Blt_Tree_GetValue(readerPtr->interp, readerPtr->tree, node,
                        SYM_CDATA, &objPtr) != TCL_OK) {
                continue;
            }
            string = Tcl_GetStringFromObj(objPtr, &length);
            for (first = string, pend = string+length; first < pend; first++) {
                if (!isspace(*first)) {
                    break;
                }
            }
            for (last = pend; last > first; last--) {
                if (!isspace(*(last - 1))) {
                    break;
                }
            }
            newPtr = Tcl_NewStringObj(first, last - first);
            Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, node,
                              SYM_CDATA, newPtr);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpStringTable --
 *
 *      Frees the hash table used for tracking unique character strings.
 *      The Tcl_Obj pointer are decremented.  So that Tcl_Obj's no longer
 *      used by the tree are freed.
 *
 *---------------------------------------------------------------------------
 */
static void
DumpStringTable(Blt_HashTable *tablePtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); hPtr != NULL;
         hPtr = Blt_NextHashEntry(&iter)) {
        Tcl_Obj *objPtr;

        objPtr = Blt_GetHashValue(hPtr);
        Tcl_DecrRefCount(objPtr);
    }
    Blt_DeleteHashTable(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetDeclProc --
 *
 *      This routine is called from the expat parser when it encounters
 *      an XML declaration.  The version, encoding, standalone flags
 *      are recorded into tree.
 *
 *---------------------------------------------------------------------------
 */
static void
GetDeclProc(void *userData, const XML_Char  *version, const XML_Char  *encoding,
            int standalone)
{
    XmlReader *readerPtr = userData;
    
    if (version != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, readerPtr->parent,
                 SYM_VERSION, Tcl_NewStringObj(version, -1));
    } 
    if (encoding != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, readerPtr->parent,
                SYM_ENCODING, Tcl_NewStringObj(encoding,-1));
    }
    Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, readerPtr->parent, 
        SYM_STANDALONE, Tcl_NewIntObj(standalone));
}

/*
 *---------------------------------------------------------------------------
 *
 * GetNotationProc --
 *
 *      This routine is called from the expat parser when it encounters
 *      XML notation.  The public ID, system ID, base URI, and notation
 *      name are recorded into tree.
 *
 *---------------------------------------------------------------------------
 */
static void
GetNotationProc(void *userData, const XML_Char *notationName,
                const XML_Char *base, const XML_Char *systemId,
                const XML_Char *publicId)
{
    XmlReader *readerPtr = userData;

    if (publicId != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, 
                readerPtr->parent, SYM_PUBID, Tcl_NewStringObj(publicId, -1));
    }
    if (systemId != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, 
                readerPtr->parent, SYM_SYSID, Tcl_NewStringObj(systemId, -1));
    } 
    if (base != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, 
                readerPtr->parent, SYM_BASEURI, Tcl_NewStringObj(base, -1));
    }
    if (notationName != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, 
                readerPtr->parent, SYM_NOTATION, 
                Tcl_NewStringObj(notationName, -1));
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetCommentProc --
 *
 *      This routine is called from the expat parser when it encounters
 *      an XML comment.  The comment is recorded into tree.
 *
 *---------------------------------------------------------------------------
 */
static void
GetCommentProc(void *userData, const XML_Char *string) 
{
    XmlReader *readerPtr = userData;

    if ((readerPtr->flags & IMPORT_DTD) == 0) {
        Blt_Tree tree;
        Blt_TreeNode child;
        Tcl_Obj *objPtr;

        tree = readerPtr->tree;
        objPtr = GetStringObj(readerPtr, string);
        child = Blt_Tree_CreateNode(tree, readerPtr->parent, SYM_COMMENT, -1);
        Blt_Tree_SetValue(readerPtr->interp, tree, child, SYM_COMMENT, objPtr);
        if (readerPtr->flags & IMPORT_LOCATION) {
            SetLocation(readerPtr, child);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetProcessingInstructionProc --
 *
 *      This routine is called from the expat parser when it encounters
 *      an XML processing instruction.  The target is recorded into tree.
 *
 *---------------------------------------------------------------------------
 */
static void
GetProcessingInstructionProc(void *userData, const char *target,
                             const char *data)
{
    XmlReader *readerPtr = userData;

    if ((readerPtr->flags & IMPORT_DTD) == 0) {
        Tcl_Obj *objPtr;
        Blt_Tree tree;
        Blt_TreeNode child;

        tree = readerPtr->tree;
        objPtr = GetStringObj(readerPtr, data);
        child = Blt_Tree_CreateNode(tree, readerPtr->parent, SYM_PI, -1);
        Blt_Tree_SetValue(readerPtr->interp, tree, child, target, objPtr);
        if (readerPtr->flags & IMPORT_LOCATION) {
            SetLocation(readerPtr, child);
        }
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetCharacterDataProc --
 *
 *      This routine is called from the expat parser when it encounters XML
 *      character data.  The character data is recorded into the tree.
 *
 *      If the last node created was also a CDATA node append data to it.
 *
 *---------------------------------------------------------------------------
 */
static void
GetCharacterDataProc(void *userData, const XML_Char *string, int length) 
{
    XmlReader *readerPtr = userData;
    Blt_Tree tree;
    Blt_TreeNode child;
    Tcl_Obj *objPtr;

    tree = readerPtr->tree;
    child = Blt_Tree_LastChild(readerPtr->parent);
    if ((child != NULL) && (strcmp(Blt_Tree_NodeLabel(child), SYM_CDATA)==0)) {

        /* Last child added was a CDATA node, append new data to it.  */
        
        if (Blt_Tree_GetValue(readerPtr->interp, tree, child, SYM_CDATA, 
                              &objPtr) == TCL_OK) {
            Tcl_AppendToObj(objPtr, string, length);
            return;
        }
    } 
    objPtr = Tcl_NewStringObj(string, length);
    child = Blt_Tree_CreateNode(tree, readerPtr->parent, SYM_CDATA,-1);
    Blt_Tree_SetValue(readerPtr->interp, tree, child, SYM_CDATA, objPtr);
    if (readerPtr->flags & IMPORT_LOCATION) {
        SetLocation(readerPtr, child);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * StartDocTypeProc --
 *
 *      This routine is called from the expat parser when it encounters XML
 *      character data.  The character data is recorded into the tree.
 *
 *      If the last node created was also a CDATA node append data to it.
 *
 *---------------------------------------------------------------------------
 */
static void
StartDocTypeProc(void *userData, const char *doctypeName, const char *systemId,
                 const char *publicId, int has_internal_subset)
{   
    XmlReader *readerPtr = userData;

    if (publicId != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, readerPtr->root, 
                SYM_PUBID, Tcl_NewStringObj(publicId, -1));
    }
    if (systemId != NULL) {
        Blt_Tree_SetValue(readerPtr->interp, readerPtr->tree, readerPtr->root, 
                SYM_SYSID, Tcl_NewStringObj(systemId, -1));
    } 
    readerPtr->flags |= IMPORT_DTD;
}

static void
EndDocTypeProc(void *userData)
{
    XmlReader *readerPtr = userData;

    readerPtr->flags &= ~IMPORT_DTD;
}


static void
StartElementProc(void *userData, const char *element, const char **attr) 
{
    Blt_TreeNode child;
    XmlReader *readerPtr = userData;
    Blt_Tree tree;

    tree = readerPtr->tree;
    child = NULL;
    if (readerPtr->flags & IMPORT_OVERWRITE) {
        child = Blt_Tree_FindChild(readerPtr->parent, element);
    }
    if (child == NULL) {
        child = Blt_Tree_CreateNode(tree, readerPtr->parent, element, -1);
    }
    if (readerPtr->flags & IMPORT_ATTRIBUTES) {
        const char **p;

        for (p = attr; *p != NULL; p += 2) {
            Tcl_Obj *objPtr;
            
            objPtr = GetStringObj(readerPtr, *(p+1));
            Blt_Tree_SetValue(readerPtr->interp, tree, child, *p, objPtr);
        }
    }
    if (readerPtr->flags & IMPORT_LOCATION) {
        SetLocation(readerPtr, child);
    }
    if (readerPtr->flags & IMPORT_BASEURI) {
        const char *oldBase, *newBase;

        newBase = XML_GetBase(readerPtr->parser);
        oldBase = GetBaseUri(readerPtr, readerPtr->parent);
        assert(oldBase != NULL);
        if (strcmp(oldBase, newBase) != 0) {
            Blt_Tree_SetValue(readerPtr->interp, tree, readerPtr->parent, 
                SYM_BASEURI, Tcl_NewStringObj(newBase, -1));
        }
    }
    readerPtr->parent = child;  /* Increase depth.  */
}

static void
EndElementProc(void *userData, const char *element) 
{
    XmlReader *readerPtr = userData;

    readerPtr->parent = Blt_Tree_ParentNode(readerPtr->parent);
}

static int
ReadXmlFromFile(Tcl_Interp *interp, XML_Parser parser, const char *fileName)
{
    int closeChannel;
    int done;
    Tcl_Channel channel;

    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
        int mode;
        
        channel = Tcl_GetChannel(interp, fileName+1, &mode);
        if (channel == NULL) {
            return FALSE;
        }
        if ((mode & TCL_READABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", fileName, 
                "\" not opened for reading", (char *)NULL);
            return FALSE;
        }
        closeChannel = FALSE;
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
        if (channel == NULL) {
            return FALSE;       /* Can't open dump file. */
        }
    }
    done = FALSE;
    while (!done) {
        int length;
#define BUFFSIZE        8191
        char buffer[BUFFSIZE+1];
        
        length = Tcl_Read(channel, buffer, sizeof(char) * BUFFSIZE);
        if (length < 0) {
            Tcl_AppendResult(interp, "\nread error: ", Tcl_PosixError(interp),
                             (char *)NULL);
            if (closeChannel) {
                Tcl_Close(interp, channel);
            }
            return FALSE;
        }
        done = Tcl_Eof(channel);
        if (!XML_Parse(parser, buffer, length, done)) {
            Tcl_AppendResult(interp, "\n", fileName, ":",
                        Blt_Itoa(XML_GetCurrentLineNumber(parser)), ": ",
                        "error: ", 
                        XML_ErrorString(XML_GetErrorCode(parser)), 
                        (char *)NULL);
            Tcl_AppendResult(interp, "\n", fileName, ":",
                        Blt_Itoa(XML_GetCurrentByteIndex(parser)), ": ",
                        "error: ", 
                        XML_ErrorString(XML_GetErrorCode(parser)), 
                        (char *)NULL);
            if (closeChannel) {
                Tcl_Close(interp, channel);
            }
            return FALSE;
        }
    }
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    return TRUE;
}


static int
GetExternalEntityRefProc(XML_Parser parser, const XML_Char *context,
                         const XML_Char *base, const XML_Char *systemId,
                         const XML_Char *publicId)
{
    XmlReader *readerPtr;
    Tcl_DString ds;
    Tcl_Interp *interp;
    XML_Parser newParser, oldParser;
    int result;

    readerPtr = XML_GetUserData(parser);
    assert(readerPtr != NULL);
    interp = readerPtr->interp;
    if (strncmp(systemId, "http:", 5) == 0) {
        Tcl_AppendResult(interp, "can't handle external entity reference \"", 
                         systemId, "\"", (char *)NULL);
        return FALSE;
    }
    Tcl_DStringInit(&ds);
    if ((base != NULL) && (Tcl_GetPathType(systemId) == TCL_PATH_RELATIVE)) {
        const char **argv;
        const char **baseNames, **sysIdNames;
        int argc;
        int i, j;
        int numBase, numSysId;

        Tcl_SplitPath(base, &numBase, &baseNames);
        Tcl_SplitPath(systemId, &numSysId, &sysIdNames);
        argc = numBase + numSysId;
        argv = Blt_Malloc(sizeof(char *) * (argc + 1));
        if (argv == NULL) {
            return FALSE;
        }
        for (i = 0; i < numBase; i++) {
            argv[i] = baseNames[i];
        }
        for (j = 0; j < numSysId; j++, i++) {
            argv[i] = sysIdNames[j];
        }
        argv[i] = NULL;
        Tcl_JoinPath(argc, argv, &ds);
        Blt_Free(baseNames);
        Blt_Free(sysIdNames);
        Blt_Free(argv);
    } else {
        Tcl_DStringAppend(&ds, systemId, -1);
    }
    newParser = XML_ExternalEntityParserCreate(parser, context, NULL);
    if (newParser == NULL) {
        Tcl_AppendResult(interp, "can't create external entity ref parser", 
                         (char *)NULL);
        return FALSE;
    }
#ifdef notdef
    XML_SetParamEntityParsing(newParser, XML_PARAM_ENTITY_PARSING_ALWAYS);
#endif
    oldParser = readerPtr->parser;
    readerPtr->parser = newParser;
    result = ReadXmlFromFile(interp, newParser, Tcl_DStringValue(&ds));
    readerPtr->parser = oldParser;
    Tcl_DStringFree(&ds);
    XML_ParserFree(newParser);
    return result;
}

static int
ImportXmlFile(Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode parent, 
              Tcl_Obj *objPtr, unsigned int flags) 
{
    XmlReader reader;
    XML_Parser parser;
    int result;
    char *fileName;

    if (flags & IMPORT_NS) {
        parser = XML_ParserCreateNS(NULL, ':');
    } else {
        parser = XML_ParserCreate(NULL);
    }
    if (parser == NULL) {
        Tcl_AppendResult(interp, "can't create XML parser", (char *)NULL);
        return TCL_ERROR;
    }
    reader.flags = flags;
    reader.interp = interp;
    reader.parent = parent;
    reader.parser = parser;
    reader.root = parent;
    reader.tree = tree;
    Blt_InitHashTable(&reader.stringTable, BLT_STRING_KEYS);
    XML_SetUserData(parser, &reader);
    fileName = Tcl_GetString(objPtr);
    /* Set baseURI */
    {
        Tcl_DString ds;
        int argc;
        const char **argv;

        Tcl_DStringInit(&ds);
        Tcl_SplitPath(fileName, &argc, &argv);
        Tcl_JoinPath(argc - 1, argv, &ds);
        XML_SetBase(parser, Tcl_DStringValue(&ds));
        if (flags & IMPORT_BASEURI) {
            Blt_Tree_SetValue(interp, tree, parent, SYM_BASEURI, 
               Tcl_NewStringObj(Tcl_DStringValue(&ds), Tcl_DStringLength(&ds)));
        }
        Blt_Free(argv);
        Tcl_DStringFree(&ds);
    }
    if (flags & IMPORT_EXTREF) {
        XML_SetExternalEntityRefHandler(parser, GetExternalEntityRefProc);
        XML_SetParamEntityParsing(parser, 
                XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
    }

    XML_SetElementHandler(parser, StartElementProc, EndElementProc);
    if (flags & IMPORT_CDATA) {
        XML_SetCharacterDataHandler(parser, GetCharacterDataProc);
    }
    if (flags & IMPORT_BASEURI) {
        XML_SetNotationDeclHandler(parser, GetNotationProc);
        XML_SetDoctypeDeclHandler(parser, StartDocTypeProc, EndDocTypeProc);
    }
    if (flags & IMPORT_DECL) {
        XML_SetXmlDeclHandler(parser, GetDeclProc);
    }
    if (flags & IMPORT_PI) {
        XML_SetProcessingInstructionHandler(parser, 
                GetProcessingInstructionProc);
    }
    if (flags & IMPORT_COMMENTS) {
        XML_SetCommentHandler(parser, GetCommentProc);
    }
    result = ReadXmlFromFile(interp, parser, fileName);
    XML_ParserFree(parser);
    if (flags & IMPORT_TRIMCDATA) {
        TrimWhitespace(&reader);
    }
    DumpStringTable(&reader.stringTable);
    return (result) ? TCL_OK : TCL_ERROR;
} 


static int
ImportXmlData(Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode parent, 
              Tcl_Obj *dataObjPtr, unsigned int flags) 
{
    XmlReader reader;
    XML_Parser parser;
    char *string;
    int length;
    int result;

    if (flags & IMPORT_NS) {
        parser = XML_ParserCreateNS(NULL, ':');
    } else {
        parser = XML_ParserCreate(NULL);
    }
    if (parser == NULL) {
        Tcl_AppendResult(interp, "can't create parser", (char *)NULL);
        return TCL_ERROR;
    }
    reader.flags = flags;
    reader.interp = interp;
    reader.parent = parent;
    reader.parser = parser;
    reader.root = parent;
    reader.tree = tree;
    Blt_InitHashTable(&reader.stringTable, BLT_STRING_KEYS);
    XML_SetBase(parser, ".");
    XML_SetUserData(parser, &reader);
    if (flags & IMPORT_EXTREF) {
        XML_SetExternalEntityRefHandler(parser, GetExternalEntityRefProc);
        XML_SetParamEntityParsing(parser, 
                XML_PARAM_ENTITY_PARSING_UNLESS_STANDALONE);
    }
    XML_SetElementHandler(parser, StartElementProc, EndElementProc);
    if (flags & IMPORT_DECL) {
        XML_SetXmlDeclHandler(parser, GetDeclProc);
    }
    if (flags & IMPORT_CDATA) {
        XML_SetCharacterDataHandler(parser, GetCharacterDataProc);
    }
    if (flags & IMPORT_BASEURI) {
        XML_SetNotationDeclHandler(parser, GetNotationProc);
        XML_SetDoctypeDeclHandler(parser, StartDocTypeProc, EndDocTypeProc);
    }
    if (flags & IMPORT_PI) {
        XML_SetProcessingInstructionHandler(parser, 
                GetProcessingInstructionProc);
    }
    if (flags & IMPORT_COMMENTS) {
        XML_SetCommentHandler(parser, GetCommentProc);
    }
    string = Tcl_GetStringFromObj(dataObjPtr, &length);
    result = XML_Parse(parser, string, length, 1);
    if (!result) {
        Tcl_AppendResult(interp, "\nparse error at line ",
                Blt_Itoa(XML_GetCurrentLineNumber(parser)), ":  ",
                XML_ErrorString(XML_GetErrorCode(parser)),
                (char *)NULL);
    }
    if (flags & IMPORT_TRIMCDATA) {
        TrimWhitespace(&reader);
    }
    XML_ParserFree(parser);
    DumpStringTable(&reader.stringTable);
    return (result) ? TCL_OK : TCL_ERROR;
} 

static int
ImportXmlProc(Tcl_Interp *interp, Blt_Tree tree, int objc, Tcl_Obj *const *objv)
{
    int result;
    ImportSwitches switches;

    memset(&switches, 0, sizeof(switches));
    nodeSwitch.clientData = tree;
    switches.root = Blt_Tree_RootNode(tree);
    switches.flags = IMPORT_ATTRIBUTES | IMPORT_CDATA;
    if (Blt_ParseSwitches(interp, importSwitches, objc - 3, objv + 3, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = TCL_ERROR;
    if ((switches.dataObj != NULL) && (switches.fileObj != NULL)) {
        Tcl_AppendResult(interp, "can't set both -file and -data switches.",
                         (char *)NULL);
        goto error;
    }
    if (switches.fileObj != NULL) {
        result = ImportXmlFile(interp, tree, switches.root, switches.fileObj, 
                switches.flags);
    } else if (switches.dataObj != NULL) {
        result = ImportXmlData(interp, tree, switches.root, switches.dataObj, 
                switches.flags);
    } else {
        result = TCL_OK;
    }
 error:
    Blt_FreeSwitches(importSwitches, (char *)&switches, 0);
    return result;
}

#endif /* HAVE_LIBEXPAT */

/*
 *---------------------------------------------------------------------------
 *
 * XmlExportData --
 *
 *      Appends a string into the buffer storing XML output.
 *
 *---------------------------------------------------------------------------
 */
static INLINE void
XmlExportData(XmlWriter *writerPtr, const char *string, size_t numBytes)
{
    Blt_DBuffer_AppendString(writerPtr->dbuffer, string, numBytes);
}

/*
 *---------------------------------------------------------------------------
 *
 * XmlIndentLine --
 *
 *      Adds a newline and indent to line to the proper indent level based
 *      on the depth of the given node and the indentation increment.  If
 *      we are not exporting the root node, don't add an extra newline.
 *      Also adjust the depth if we aren't exporting the root node,
 *
 *---------------------------------------------------------------------------
 */
static void
XmlIndentLine(XmlWriter *writerPtr, Blt_TreeNode node)
{
    long depth;
    
    if ((writerPtr->flags & EXPORT_ROOT) || (writerPtr->root != node)) {
        XmlExportData(writerPtr, "\n", 1);
    }
    depth = Blt_Tree_NodeDepth(node);
    if ((writerPtr->flags & EXPORT_ROOT) == 0) {
        depth--;
    }
    Blt_DBuffer_Format(writerPtr->dbuffer, "%*.s", writerPtr->indent * depth,
                       "");
}

/*
 *---------------------------------------------------------------------------
 *
 * XmlFlush --
 *
 *      Writes any data stored in the buffer to the file channel.  If we're
 *      writing XML output to a file, we only store pieces of the XML in the
 *      buffer and write the buffer the file when we encounter an start of
 *      end tag.  The buffer is also reset.
 *
 *---------------------------------------------------------------------------
 */
static int
XmlFlush(XmlWriter *writerPtr) 
{
    size_t length;

    length = Blt_DBuffer_Length(writerPtr->dbuffer);
    if (length > 0) {
        ssize_t numWritten;
        const char *line;

        line = Blt_DBuffer_String(writerPtr->dbuffer);
        numWritten = Tcl_Write(writerPtr->channel, line, length);
        if (numWritten != length) {
            Tcl_AppendResult(writerPtr->interp, "can't write xml element: ",
                Tcl_PosixError(writerPtr->interp), (char *)NULL);
            return TCL_ERROR;
        }
    }
    Blt_DBuffer_SetLength(writerPtr->dbuffer, 0);
    return TCL_OK;
}

static void
XmlPutEscapeString(const char *from, size_t length, XmlWriter *writerPtr)
{
    const char *p, *pend;

    for (p = from, pend = from + length; p < pend; /*empty*/) {
        switch (*p) {
        case '\'': 
            if (p > from) {
                XmlExportData(writerPtr, from, p - from);
            }
            from = ++p;
            XmlExportData(writerPtr, "&apos;", 6);
            break;
        case '&':  
            if (p > from) {
                XmlExportData(writerPtr, from, p - from);
            }
            from = ++p;
            XmlExportData(writerPtr, "&amp;", 5);
            break;
        case '>':  
            if (p > from) {
                XmlExportData(writerPtr, from, p - from);
            }
            from = ++p;
            XmlExportData(writerPtr, "&gt;", 4);
            break; 
        case '<':  
            if (p > from) {
                XmlExportData(writerPtr, from, p - from);
            }
            from = ++p;
            XmlExportData(writerPtr, "&lt;", 4);
            break; 
        case '"':  
            if (p > from) {
                XmlExportData(writerPtr, from, p - from);
            }
            from = ++p;
            XmlExportData(writerPtr, "&quot;", 6);
            break;
        default:  
            p++;
            break;
        }
    }   
    if (p > from) {
        XmlExportData(writerPtr, from, p - from);
    }
}

static void
XmlOpenStartElement(XmlWriter *writerPtr, Blt_TreeNode node)
{
    const char *label;
    
    if (writerPtr->channel != NULL) {
        XmlFlush(writerPtr);
    }
    writerPtr->flags &= ~LAST_END_TAG;
    /* Always indent starting element tags */
    XmlIndentLine(writerPtr, node);
    label = Blt_Tree_NodeLabel(node);
    if (writerPtr->root == node) {
        if (writerPtr->flags & EXPORT_ROOT) {
            if (label[0] == '\0') {
                XmlExportData(writerPtr, "<root", 5);
            } else {
                Blt_DBuffer_Format(writerPtr->dbuffer, "<%s", label);
            }                
        }
    } else {
        Blt_DBuffer_Format(writerPtr->dbuffer, "<%s", label);
    }
}

static int
XmlCloseStartElement(XmlWriter *writerPtr, Blt_TreeNode node)
{
    if ((writerPtr->root != node) || (writerPtr->flags & EXPORT_ROOT)) {
        XmlExportData(writerPtr, ">", 1);
    }
    if (writerPtr->channel != NULL) {
        return XmlFlush(writerPtr);
    }
    return TCL_OK;
}

static int
XmlEndElement(XmlWriter *writerPtr, Blt_TreeNode node)
{
    const char *label;
    
    if (writerPtr->flags & LAST_END_TAG) {
        XmlIndentLine(writerPtr, node);
    }
    writerPtr->flags |= LAST_END_TAG;
    label = Blt_Tree_NodeLabel(node);
    if (writerPtr->root == node) {
        if (writerPtr->flags & EXPORT_ROOT) {
            if (label[0] == '\0') {
                XmlExportData(writerPtr, "</root>\n", 8);
            } else {
                Blt_DBuffer_Format(writerPtr->dbuffer, "</%s>\n", label);
            }
        } else {
            XmlExportData(writerPtr, "\n", 1);
        }
    } else {
        Blt_DBuffer_Format(writerPtr->dbuffer, "</%s>", label);
    }
    if (writerPtr->channel != NULL) {
        return XmlFlush(writerPtr);
    }
    return TCL_OK;
}

static void
XmlAppendAttribute(XmlWriter *writerPtr, const char *attrName,
                   const char *value, int length)
{
    size_t valueLen;

    if (length < 0) {
        valueLen = strlen(value);
    } else {
        valueLen = (size_t)length;
    }
    Blt_DBuffer_Format(writerPtr->dbuffer, " %s=\"", attrName);
    XmlPutEscapeString(value, valueLen, writerPtr);
    XmlExportData(writerPtr, "\"", 1);
}

static void
XmlAppendCharacterData(XmlWriter *writerPtr, const char *string, int length)
{
    if (length < 0) {
        length = strlen(string);
    } 
    XmlPutEscapeString(string, length, writerPtr);
}

static int
XmlExportElement(Blt_Tree tree, Blt_TreeNode parent, XmlWriter *writerPtr)
{
    Blt_TreeKey key;
    Blt_TreeKeyIterator iter;
    Blt_TreeNode child;

    if (strcmp(Blt_Tree_NodeLabel(parent), SYM_CDATA) == 0) {
        Tcl_Obj *valueObjPtr;
        const char *value;
        int length;

        /* Just output the CDATA field. */
        if (Blt_Tree_GetValue(writerPtr->interp, tree, parent, SYM_CDATA, 
                &valueObjPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        value = Tcl_GetStringFromObj(valueObjPtr, &length);
        XmlAppendCharacterData(writerPtr, value, length);
        return TCL_OK;
    } 
    XmlOpenStartElement(writerPtr, parent);
    for (key = Blt_Tree_FirstKey(tree, parent, &iter); key != NULL; 
         key = Blt_Tree_NextKey(tree, &iter)) {
        Tcl_Obj *valueObjPtr;
        const char *value;
        int numBytes;

        if (Blt_Tree_GetValueByKey(writerPtr->interp, tree, parent, key,
                &valueObjPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        value = Tcl_GetStringFromObj(valueObjPtr, &numBytes);
        XmlAppendAttribute(writerPtr, key, value, numBytes);
    }
    XmlCloseStartElement(writerPtr, parent);
    for (child = Blt_Tree_FirstChild(parent); child != NULL; 
         child = Blt_Tree_NextSibling(child)) {
        if (XmlExportElement(tree, child, writerPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    XmlEndElement(writerPtr, parent);
    return TCL_OK;
}

static int
XmlExport(Blt_Tree tree, XmlWriter *writerPtr)
{
    if (writerPtr->flags & EXPORT_DECLARATION) {
        XmlExportData(writerPtr, "<?xml version='1.0' encoding='utf-8'?>", 38);
    }
    if (XmlExportElement(tree, writerPtr->root, writerPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (writerPtr->channel != NULL) {
        return XmlFlush(writerPtr);
    }
    return TCL_OK;
}

static int
ExportXmlProc(Tcl_Interp *interp, Blt_Tree tree, int objc, Tcl_Obj *const *objv)
{
    XmlWriter writer;
    Tcl_Channel channel;
    int closeChannel;
    int result;

    closeChannel = FALSE;
    channel = NULL;

    memset(&writer, 0, sizeof(writer));
    nodeSwitch.clientData = tree;
    writer.root = Blt_Tree_RootNode(tree);
    writer.indent = 1;
    writer.flags = IMPORT_ATTRIBUTES | IMPORT_CDATA;
    if (Blt_ParseSwitches(interp, exportSwitches, objc - 3 , objv + 3, 
        &writer, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    result = TCL_ERROR;
    if (writer.fileObj != NULL) {
        char *fileName;

        closeChannel = TRUE;
        fileName = Tcl_GetString(writer.fileObj);
        if ((fileName[0] == '@') && (fileName[1] != '\0')) {
            int mode;
            
            channel = Tcl_GetChannel(interp, fileName+1, &mode);
            if (channel == NULL) {
                goto error;
            }
            if ((mode & TCL_WRITABLE) == 0) {
                Tcl_AppendResult(interp, "channel \"", fileName, 
                        "\" not opened for writing", (char *)NULL);
                goto error;
            }
            closeChannel = FALSE;
        } else {
            channel = Tcl_OpenFileChannel(interp, fileName, "w", 0666);
            if (channel == NULL) {
                goto error;     /* Can't open export file. */
            }
        }
    }
    writer.interp = interp;
    writer.dbuffer = Blt_DBuffer_Create();
    writer.channel = channel;
    result = XmlExport(tree, &writer); 
    if ((writer.channel == NULL) && (result == TCL_OK)) {
        Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(writer.dbuffer));
    } 
 error:
    if (writer.dbuffer != NULL) {
        Blt_DBuffer_Destroy(writer.dbuffer);
    }
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    Blt_FreeSwitches(exportSwitches, (char *)&writer, 0);
    return result;
}

int 
Blt_TreeXmlInit(Tcl_Interp *interp)
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
#else
    if (Tcl_PkgRequire(interp, "blt_tcl", BLT_VERSION, PKG_EXACT) == NULL) {
        return TCL_ERROR;
    }
#endif    
    if (Tcl_PkgProvide(interp, "blt_tree_xml", BLT_VERSION) != TCL_OK) { 
        return TCL_ERROR;
    }
    return Blt_Tree_RegisterFormat(interp,
        "xml",                  /* Name of format. */
#ifdef HAVE_LIBEXPAT
        ImportXmlProc,          /* Import procedure. */
#else
        NULL,                   /* Import procedure. */
#endif  /* HAVE_LIBEXPAT */
        ExportXmlProc);         /* Export procedure. */

}

int 
Blt_TreeXmlSafeInit(Tcl_Interp *interp)
{
    return Blt_TreeXmlInit(interp);
}
