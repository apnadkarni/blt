/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTreeCmd.c --
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

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#ifndef NO_TREE

#ifdef HAVE_SYS_STAT_H
  #include <sys/stat.h>
#endif  /* HAVE_SYS_STAT_H */

#ifdef HAVE_CTYPE_H
  #include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include "bltAlloc.h"
#include "bltMath.h"
#include "bltString.h"
#include <bltTree.h>
#include <bltHash.h>
#include <bltList.h>
#include "bltNsUtil.h"
#include "bltSwitch.h"
#include "bltOp.h"
#include "bltInitCmd.h"

#define TREE_THREAD_KEY "BLT Tree Command Data"
#define TREE_MAGIC ((unsigned int) 0x46170277)

enum TagTypes { TAG_TYPE_NONE, TAG_TYPE_ALL, TAG_TYPE_TAG };

typedef struct {
    const char *name;                   /* Name of format. */
    int isLoaded;                       
    Blt_TreeImportProc *importProc;
    Blt_TreeExportProc *exportProc;
} DataFormat;

typedef struct {
    Tcl_Interp *interp;
    Blt_HashTable treeTable;            /* Hash table of trees keyed by
                                         * address. */
    Blt_HashTable fmtTable;
} TreeCmdInterpData;

typedef struct {
    Tcl_Interp *interp;
    Tcl_Command cmdToken;               /* Token for tree's TCL command. */
    Blt_Tree tree;                      /* Token holding internal tree. */
    Blt_HashEntry *hashPtr;
    Blt_HashTable *tablePtr;
    TreeCmdInterpData *interpDataPtr;   /*  */
    int traceCounter;                   /* Used to generate trace id
                                         * strings.  */
    Blt_HashTable traceTable;           /* Table of active traces. Maps
                                         * trace ids back to their
                                         * TraceInfo records. */
    int notifyCounter;                  /* Used to generate notify id
                                         * strings. */
    Blt_HashTable notifyTable;          /* Table of event handlers. Maps
                                         * notify ids back to their
                                         * Notifier records. */
    Blt_Chain notifiers;
    const char *defPathSep;             /* Path separator. */
} TreeCmd;

typedef struct {
    TreeCmd *cmdPtr;
    Blt_TreeNode node;
    Blt_TreeTrace traceToken;
    const char *withTag;                /* If non-NULL, the event or trace
                                         * was specified with this tag.
                                         * This value is saved for
                                         * informational purposes.  The
                                         * tree's trace matching routines
                                         * do the real checking, not the
                                         * client's callback.  */
    char command[1];                    /* Command prefix for the trace or
                                         * notify TCL callback.  Extra
                                         * arguments will be appended to
                                         * the end. Extra space will be
                                         * allocated for the length of the
                                         * string */
} TraceInfo;

typedef struct {
    TreeCmd *cmdPtr;
    int mask;                           /* Requested event mask. */
    long inode;                         /* If >= 0, inode to match.  */
    const char *tag;                    /* If non-NULL, tag to match. */
    Tcl_Obj *cmdObjPtr;                 /* Command to be executed when
                                         * matching event is found. */
    Blt_TreeNode node;                  /* (out) Node affected by event. */
    Blt_TreeTrace notifyToken;

    Blt_HashEntry *hashPtr;             /* Pointer to entry in hash table. */
    Blt_ChainLink link;                 /* Pointer to entry in list of
                                         * notifiers. */
} Notifier;

typedef struct {
    Blt_HashTable dataTable;            /* Hash table of data values.  Used
                                         * to create one shared Tcl_Obj for
                                         * each data unique data value. */
    Blt_HashTable idTable;              /* Hash table of node ids used to
                                         * map previous node ids to new
                                         * node ids. */
    Blt_Tree tree;                      /* Destination tree where nodes are
                                         * restored.  */
    Blt_TreeNode node;                  /* Last node restored. This is used
                                         * to when setting data values and
                                         * tags for version 3 dump
                                         * files. */
    Blt_TreeNode root;                  /* Top node of branch where new 
                                         * nodes will be created.  */
    Tcl_Channel channel;                /* Represents file containing the
                                         * tree dump. */
    const char *nextLine;               /* Points to the next line to be
                                         * processed. */
    const char **argv;                  /* Vector of strings of the
                                         * last dump record. */
    int argc;                           /* # of strings in above vector. */
    int numLines;                       /* Current line number. */
    double version;                     /* Version of the tree dump. */
    unsigned int flags;                 /* Flags: see below. */
    Tcl_Obj *fileObjPtr;                /* Name of dump file. */
    Tcl_Obj *dataObjPtr;                /* String containing dump
                                         * information. */
} RestoreInfo;

#define RESTORE_NO_TAGS         (1<<0)
#define RESTORE_OVERWRITE       (1<<1)

typedef struct {
    Blt_Tree tree;                      /* Source tree from which nodes are
                                         * dumped.  */
    Blt_TreeNode root;                  /* Top node of branch from which
                                         * nodes will be dumped.  */
    double version;                     /* Version of the tree dump. */
    unsigned int flags;
    Tcl_Channel channel;                /* Represents file to write the
                                         * tree dump. */
    Tcl_DString ds;                     /* Dynamic string containing the
                                         * tree dump.  */
    Tcl_Obj *fileObjPtr;                /* Name of dump file. */
    Tcl_Obj *dataObjPtr;                /* String to eventually hold the
                                         * dump information. */
} DumpInfo;

#define DUMP_NO_TAGS         (1<<0)

BLT_EXTERN Blt_SwitchParseProc Blt_TreeNodeSwitchProc;
static Blt_SwitchCustom nodeSwitch = {
    Blt_TreeNodeSwitchProc, NULL, NULL, (ClientData)0,
};

typedef struct {
    int mask;
} AttachSwitches;

static Blt_SwitchSpec attachSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-newtags", "", (char *)NULL,
        Blt_Offset(AttachSwitches, mask), 0, TREE_NEWTAGS},
    {BLT_SWITCH_END}
};

typedef struct {
    int mask;
    Blt_TreeNode node;
    const char *tag;
} NotifySwitches;

static Blt_SwitchSpec notifySwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-allevents", "", (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_ALL},
    {BLT_SWITCH_BITS_NOARG, "-create", "",  (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_CREATE},
    {BLT_SWITCH_BITS_NOARG, "-delete", "",  (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_DELETE},
    {BLT_SWITCH_BITS_NOARG, "-move", "",   (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_MOVE},
    {BLT_SWITCH_CUSTOM,  "-node",  "node", (char *)NULL,
        Blt_Offset(NotifySwitches, node),    0, 0, &nodeSwitch},
    {BLT_SWITCH_BITS_NOARG, "-relabel", "", (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_RELABEL},
    {BLT_SWITCH_BITS_NOARG, "-sort", "",   (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_SORT},
    {BLT_SWITCH_STRING,  "-tag", "string", (char *)NULL,
        Blt_Offset(NotifySwitches, tag), 0},
    {BLT_SWITCH_BITS_NOARG, "-whenidle", "", (char *)NULL,
        Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_WHENIDLE},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc ChildSwitch;
#define INSERT_BEFORE   (ClientData)0
#define INSERT_AFTER    (ClientData)1
static Blt_SwitchCustom beforeSwitch = {
    ChildSwitch, NULL, NULL, INSERT_BEFORE,
};
static Blt_SwitchCustom afterSwitch = {
    ChildSwitch, NULL, NULL, INSERT_AFTER,
};

typedef struct {
    const char *label;
    long position;
    long inode;
    Tcl_Obj *tagsObjPtr;
    char **dataPairs;
    Blt_TreeNode parent;
} InsertSwitches;

typedef struct {
    unsigned int perm, type;            /* Indicate the permission and type
                                         * of directory entries to search
                                         * for.*/
    unsigned int mask;                  /* Indicates which fields to copy
                                         * into the tree node as data. */
    Tcl_Obj *patternsObjPtr;            /* If non-NULL, is a list of
                                         * patterns to match contents of
                                         * each directory.  */
    unsigned int flags;
} ReadDirectory;

/* Status flags for read dir operations. */
#define READ_DIR_MATCH       (1)
#define READ_DIR_NOMATCH     (0)
#define READ_DIR_ERROR       (-1)

/* Flags to indicate what output fields to create. */
#define READ_DIR_TYPE        (1<<0)
#define READ_DIR_MODE        (1<<1)
#define READ_DIR_SIZE        (1<<2)
#define READ_DIR_UID         (1<<3)
#define READ_DIR_GID         (1<<4)
#define READ_DIR_ATIME       (1<<5)
#define READ_DIR_CTIME       (1<<6)
#define READ_DIR_MTIME       (1<<7)
#define READ_DIR_INO         (1<<8)
#define READ_DIR_NLINK       (1<<9)
#define READ_DIR_DEV         (1<<10)
#define READ_DIR_PERMS       (1<<11)

#define READ_DIR_ALL         \
    (READ_DIR_ATIME|READ_DIR_CTIME|READ_DIR_MTIME|READ_DIR_UID|READ_DIR_GID|\
     READ_DIR_TYPE|READ_DIR_MODE|READ_DIR_SIZE|READ_DIR_INO|READ_DIR_NLINK|\
     READ_DIR_DEV|READ_DIR_PERMS)

#define READ_DIR_DEFAULT     \
    (READ_DIR_MTIME|READ_DIR_TYPE|READ_DIR_PERMS|READ_DIR_SIZE)

/* Various flags for read dir operation */
#define READ_DIR_RECURSE             (1<<11)
#define READ_DIR_NOCASE              (1<<12)
#define READ_DIR_IGNORE_HIDDEN_DIRS  (1<<13)

static Blt_SwitchParseProc FieldsSwitchProc;
static Blt_SwitchCustom fieldsSwitch = {
    FieldsSwitchProc, NULL, NULL, (ClientData)0,
};
static Blt_SwitchParseProc TypeSwitchProc;
static Blt_SwitchCustom typeSwitch = {
    TypeSwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchParseProc PermSwitchProc;
static Blt_SwitchCustom permSwitch = {
    PermSwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec dirSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-fields",  "fieldList", (char *)NULL,
        Blt_Offset(ReadDirectory, mask),    0, 0, &fieldsSwitch},
    {BLT_SWITCH_BITS_NOARG, "-hidden", "", (char *)NULL,
        Blt_Offset(ReadDirectory, perm), 0, TCL_GLOB_PERM_HIDDEN},
    {BLT_SWITCH_BITS_NOARG, "-ignorehiddendirs", "", (char *)NULL,
        Blt_Offset(ReadDirectory, flags), 0, READ_DIR_IGNORE_HIDDEN_DIRS},
#if (_TCL_VERSION > _VERSION(8,5,0)) 
    {BLT_SWITCH_BITS_NOARG, "-nocase",       "", (char *)NULL,
        Blt_Offset(ReadDirectory, flags), 0, READ_DIR_NOCASE},
#endif
    {BLT_SWITCH_OBJ,     "-patterns",     "list", (char *)NULL,
        Blt_Offset(ReadDirectory, patternsObjPtr), 0},
    {BLT_SWITCH_CUSTOM,  "-permissions", "permList", (char *)NULL,
        Blt_Offset(ReadDirectory, perm),    0, 0, &permSwitch},
    {BLT_SWITCH_BITS_NOARG, "-readonly", "", (char *)NULL,
        Blt_Offset(ReadDirectory, perm), 0, TCL_GLOB_PERM_RONLY},
    {BLT_SWITCH_BITS_NOARG, "-recurse", "", (char *)NULL,
        Blt_Offset(ReadDirectory, flags), 0, READ_DIR_RECURSE},
    {BLT_SWITCH_CUSTOM,  "-type",    "typeList", (char *)NULL,
        Blt_Offset(ReadDirectory, type),    0, 0, &typeSwitch},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec insertSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", "position", (char *)NULL,
        Blt_Offset(InsertSwitches, position), 0, 0, &afterSwitch},
    {BLT_SWITCH_LONG_NNEG, "-at", "position", (char *)NULL,
        Blt_Offset(InsertSwitches, position), 0},
    {BLT_SWITCH_CUSTOM, "-before", "position", (char *)NULL,
        Blt_Offset(InsertSwitches, position), 0, 0, &beforeSwitch},
    {BLT_SWITCH_LIST, "-data", "{name value ?name value ...?}", (char *)NULL,
        Blt_Offset(InsertSwitches, dataPairs), 0},
    {BLT_SWITCH_STRING, "-label", "string", (char *)NULL,
        Blt_Offset(InsertSwitches, label), 0},
    {BLT_SWITCH_LONG_NNEG, "-node", "number", (char *)NULL,
        Blt_Offset(InsertSwitches, inode), 0},
    {BLT_SWITCH_OBJ, "-tags", "tagList", (char *)NULL,
        Blt_Offset(InsertSwitches, tagsObjPtr), 0},
    {BLT_SWITCH_END}
};

#define MATCH_INVERT            (1<<8)
#define MATCH_LEAFONLY          (1<<4)
#define MATCH_NOCASE            (1<<5)
#define MATCH_PATHNAME          (1<<6)
#define PATTERN_NONE            (0)
#define PATTERN_EXACT           (1)
#define PATTERN_GLOB            (2)
#define PATTERN_REGEXP          (3)
#define PATTERN_MASK            (0x3)

typedef struct {
    TreeCmd *cmdPtr;                    /* Tree to examine. */
    Tcl_Obj *listObjPtr;                /* List to accumulate the indices
                                         * of matching nodes. */
    size_t numMatches;                  /* Current number of matches. */

    /* User-set values. */
    unsigned int flags;                 /* See flags definitions above. */

    Tcl_Obj *cmdObjPtr;                 /* If non-NULL, command to be
                                         * executed for each found node. */
    long maxDepth;                      /* If >= 0, don't descend more than
                                         * this many levels. */
    size_t maxMatches;                  /* If > 0, stop after this many
                                         * matches. */
    unsigned int order;                 /* Order of search: Can be either
                                         * TREE_PREORDER, TREE_POSTORDER,
                                         * TREE_INORDER,
                                         * TREE_BREADTHFIRST. */
    Tcl_Obj *exprObjPtr;                /* Expression to be evalated at
                                         * each node. */
    Tcl_Obj *varNameObjPtr;             /* Name of TCL array variable to be
                                         * dynamically set with the current
                                         * values at each node. */
    Blt_List patternList;               /* List of patterns to compare with
                                         * labels or values.  */
    const char *addTag;                 /* If non-NULL, tag added to
                                         * selected nodes. */
    Blt_List keyList;                   /* List of key name patterns. */
    Blt_List tagList;                   /* List of tag names. */
    Blt_HashTable excludeTable;         /* Table of nodes to exclude. */
    Blt_HashTable includeTable;         /* Table of nodes to include. */
} FindSwitches;

static Blt_SwitchParseProc OrderSwitch;
static Blt_SwitchCustom orderSwitch = {
    OrderSwitch, NULL, NULL, (ClientData)0
};

static Blt_SwitchParseProc PatternSwitch;
static Blt_SwitchFreeProc FreePatterns;

static Blt_SwitchCustom regexpSwitch = {
    PatternSwitch, NULL, FreePatterns, (ClientData)PATTERN_REGEXP
};
static Blt_SwitchCustom globSwitch = {
    PatternSwitch, NULL, FreePatterns, (ClientData)PATTERN_GLOB
};
static Blt_SwitchCustom exactSwitch = {
    PatternSwitch, NULL, FreePatterns, (ClientData)PATTERN_EXACT
};

static Blt_SwitchCustom tagSwitch = {
    PatternSwitch, NULL, FreePatterns, (ClientData)PATTERN_EXACT
};


static Blt_SwitchParseProc NodesSwitch;
static Blt_SwitchFreeProc FreeNodes;
static Blt_SwitchCustom nodesSwitch = {
    NodesSwitch, NULL, FreeNodes, (ClientData)0
};


static Blt_SwitchSpec findSwitches[] = {
    {BLT_SWITCH_STRING, "-addtag", "tagName", (char *)NULL,
        Blt_Offset(FindSwitches, addTag), 0},
    {BLT_SWITCH_LONG_NNEG, "-count", "number", (char *)NULL,
     Blt_Offset(FindSwitches, maxMatches), 0}, 
    {BLT_SWITCH_LONG_NNEG, "-depth", "number", (char *)NULL,
        Blt_Offset(FindSwitches, maxDepth), 0},
    {BLT_SWITCH_CUSTOM, "-exact", "string",  (char *)NULL,
        Blt_Offset(FindSwitches, patternList),0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-excludes", "nodeList", (char *)NULL,
        Blt_Offset(FindSwitches, excludeTable),0, 0, &nodesSwitch},
    {BLT_SWITCH_OBJ, "-exec", "command", (char *)NULL,
        Blt_Offset(FindSwitches, cmdObjPtr),    0}, 
    {BLT_SWITCH_OBJ, "-expr", "exprString", (char *)NULL,
        Blt_Offset(FindSwitches, exprObjPtr),0},
    {BLT_SWITCH_CUSTOM, "-glob", "pattern", (char *)NULL,
        Blt_Offset(FindSwitches, patternList),0, 0, &globSwitch},
    {BLT_SWITCH_CUSTOM, "-includes", "nodeList", (char *)NULL,
        Blt_Offset(FindSwitches, includeTable),0, 0, &nodesSwitch},
    {BLT_SWITCH_BITS_NOARG, "-invert", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, MATCH_INVERT},
    {BLT_SWITCH_CUSTOM, "-key", "string",  (char *)NULL,
        Blt_Offset(FindSwitches, keyList),    0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyexact", "string", (char *)NULL,
        Blt_Offset(FindSwitches, keyList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyglob", "pattern", (char *)NULL,
        Blt_Offset(FindSwitches, keyList),    0, 0, &globSwitch},
    {BLT_SWITCH_CUSTOM, "-keyregexp","pattern", (char *)NULL,
        Blt_Offset(FindSwitches, keyList), 0, 0, &regexpSwitch},
    {BLT_SWITCH_BITS_NOARG, "-leafonly", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, MATCH_LEAFONLY},
    {BLT_SWITCH_BITS_NOARG, "-nocase", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, MATCH_NOCASE},
    {BLT_SWITCH_CUSTOM, "-order", "orderName", (char *)NULL,
        Blt_Offset(FindSwitches, order), 0, 0, &orderSwitch},
    {BLT_SWITCH_BITS_NOARG, "-path", "", (char *)NULL,
        Blt_Offset(FindSwitches, flags), 0, MATCH_PATHNAME},
    {BLT_SWITCH_CUSTOM, "-regexp", "pattern",  (char *)NULL,
        Blt_Offset(FindSwitches, patternList),0, 0, &regexpSwitch},
    {BLT_SWITCH_CUSTOM, "-tag", "tagList", (char *)NULL,
        Blt_Offset(FindSwitches, tagList), 0, 0, &tagSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    TreeCmd *cmdPtr;                    /* Tree to move nodes. */
    Blt_TreeNode node;
    long movePos;
} MoveSwitches;

static Blt_SwitchSpec moveSwitches[] = 
{
    {BLT_SWITCH_CUSTOM, "-after", "child", (char *)NULL,
        Blt_Offset(MoveSwitches, node), 0, 0, &nodeSwitch},
    {BLT_SWITCH_LONG_NNEG, "-at", "position", (char *)NULL,
        Blt_Offset(MoveSwitches, movePos), 0}, 
    {BLT_SWITCH_CUSTOM, "-before", "child", (char *)NULL,
        Blt_Offset(MoveSwitches, node), 0, 0, &nodeSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_TreeNode srcNode;
    Blt_Tree srcTree, destTree;
    TreeCmd *srcPtr, *destPtr;
    const char *label;
    unsigned int flags;
} CopySwitches;

#define COPY_RECURSE    (1<<0)
#define COPY_TAGS       (1<<1)
#define COPY_OVERWRITE  (1<<2)

static Blt_SwitchSpec copySwitches[] = 
{
    {BLT_SWITCH_STRING, "-label", "string", (char *)NULL,
        Blt_Offset(CopySwitches, label), 0},
    {BLT_SWITCH_BITS_NOARG, "-recurse", "", (char *)NULL,
        Blt_Offset(CopySwitches, flags), 0, COPY_RECURSE},
    {BLT_SWITCH_BITS_NOARG, "-tags", "", (char *)NULL,
        Blt_Offset(CopySwitches, flags), 0, COPY_TAGS},
    {BLT_SWITCH_BITS_NOARG, "-overwrite", "", (char *)NULL,
        Blt_Offset(CopySwitches, flags), 0, COPY_OVERWRITE},
    {BLT_SWITCH_OBJ, "-tree", "", (char *)NULL,
        Blt_Offset(CopySwitches, destTree), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    TreeCmd *cmdPtr;                    /* Tree to examine. */
    unsigned int flags;                 /* See flags definitions above. */

    long maxDepth;                      /* If >= 0, don't descend more than
                                         * this many levels. */
    /* String options. */
    Blt_List patternList;               /* List of label or value
                                           patterns. */
    Tcl_Obj *preCmdObjPtr;              /* Pre-command. */
    Tcl_Obj *postCmdObjPtr;             /* Post-command. */
    Blt_List keyList;                   /* List of key-name patterns. */
    Blt_List tagList;
} ApplySwitches;

static Blt_SwitchSpec applySwitches[] = 
{
    {BLT_SWITCH_OBJ, "-precommand", "command", (char *)NULL,
        Blt_Offset(ApplySwitches, preCmdObjPtr), 0},
    {BLT_SWITCH_OBJ, "-postcommand", "command", (char *)NULL,
        Blt_Offset(ApplySwitches, postCmdObjPtr), 0},
    {BLT_SWITCH_LONG_NNEG, "-depth", "number", (char *)NULL,
        Blt_Offset(ApplySwitches, maxDepth), 0},
    {BLT_SWITCH_CUSTOM, "-exact", "string", (char *)NULL,
        Blt_Offset(ApplySwitches, patternList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-glob", "pattern", (char *)NULL,
        Blt_Offset(ApplySwitches, patternList), 0, 0, &globSwitch},
    {BLT_SWITCH_BITS_NOARG, "-invert", "", (char *)NULL,
        Blt_Offset(ApplySwitches, flags), 0, MATCH_INVERT},
    {BLT_SWITCH_CUSTOM, "-key", "pattern", (char *)NULL,
        Blt_Offset(ApplySwitches, keyList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyexact", "string", (char *)NULL,
        Blt_Offset(ApplySwitches, keyList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyglob", "pattern", (char *)NULL,
        Blt_Offset(ApplySwitches, keyList), 0, 0, &globSwitch},
    {BLT_SWITCH_CUSTOM, "-keyregexp", "pattern", (char *)NULL,
        Blt_Offset(ApplySwitches, keyList), 0, 0, &regexpSwitch},
    {BLT_SWITCH_BITS_NOARG, "-leafonly", "", (char *)NULL,
        Blt_Offset(ApplySwitches, flags), 0, MATCH_LEAFONLY},
    {BLT_SWITCH_BITS_NOARG, "-nocase", "", (char *)NULL,
        Blt_Offset(ApplySwitches, flags), 0, MATCH_NOCASE},
    {BLT_SWITCH_BITS_NOARG, "-path", "", (char *)NULL,
        Blt_Offset(ApplySwitches, flags), 0, MATCH_PATHNAME},
    {BLT_SWITCH_CUSTOM, "-regexp", "pattern", (char *)NULL,
        Blt_Offset(ApplySwitches, patternList), 0, 0, &regexpSwitch},
    {BLT_SWITCH_CUSTOM, "-tag", "tagList", (char *)NULL,
        Blt_Offset(ApplySwitches, tagList), 0, 0, &tagSwitch},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec restoreSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
        Blt_Offset(RestoreInfo, dataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(RestoreInfo, fileObjPtr), 0, 0},
    {BLT_SWITCH_BITS_NOARG, "-notags", "", (char *)NULL,
        Blt_Offset(RestoreInfo, flags), 0, RESTORE_NO_TAGS},
    {BLT_SWITCH_BITS_NOARG, "-overwrite", "", (char *)NULL,
        Blt_Offset(RestoreInfo, flags), 0, RESTORE_OVERWRITE},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec dumpSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
        Blt_Offset(DumpInfo, dataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
        Blt_Offset(DumpInfo, fileObjPtr), 0, 0},
    {BLT_SWITCH_DOUBLE, "-version", "versionNum", (char *)NULL,
        Blt_Offset(DumpInfo, version), 0, 0},
    {BLT_SWITCH_BITS_NOARG, "-notags", "", (char *)NULL,
        Blt_Offset(DumpInfo, flags), 0, DUMP_NO_TAGS},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc FormatSwitch;
static Blt_SwitchCustom formatSwitch =
{
    FormatSwitch, NULL, NULL, (ClientData)0,
};

typedef struct {
    int sort;                           /* If non-zero, sort the nodes.  */
    int withParent;                     /* If non-zero, add the parent node
                                         * id to the output of the
                                         * command.*/
    int withId;                         /* If non-zero, echo the node id in
                                         * the output of the command. */
} PositionSwitches;

#define POSITION_SORTED         (1<<0)

static Blt_SwitchSpec positionSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-sort", "",  (char *)NULL,
        Blt_Offset(PositionSwitches, sort), 0, POSITION_SORTED},
    {BLT_SWITCH_CUSTOM, "-format", "format",  (char *)NULL,
        0, 0, 0, &formatSwitch},
    {BLT_SWITCH_END}
};

#define PATH_PARENTS            (1<<1)
#define PATH_NOCOMPLAIN         (1<<2)

typedef struct {
    unsigned int flags;                 /* Parse flags. */
    const char *pathSep;                /* Path separator. */
    Blt_TreeNode root;                  /* Starting node of path. */
} PathCreateSwitches;

static Blt_SwitchSpec pathCreateSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-from",  "node", (char *)NULL,
        Blt_Offset(PathCreateSwitches, root),  0, 0, &nodeSwitch},
    {BLT_SWITCH_BITS_NOARG, "-nocomplain", "", (char *)NULL,
        Blt_Offset(PathCreateSwitches, flags), 0, PATH_NOCOMPLAIN},
    {BLT_SWITCH_BITS_NOARG, "-parents", "", (char *)NULL,
        Blt_Offset(PathCreateSwitches, flags), 0, PATH_PARENTS},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
        Blt_Offset(PathCreateSwitches, pathSep), BLT_SWITCH_NULL_OK}, 
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_TreeNode root;                  /* Starting node of path. */
    const char *pathSep;                /* Path separator. */
    unsigned int flags;
} PathPrintSwitches;

#define PATH_NO_LEADING_SEPARATOR            (1<<3)

static Blt_SwitchSpec pathPrintSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-from",  "node", (char *)NULL,
        Blt_Offset(PathPrintSwitches, root),  0, 0, &nodeSwitch},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
        Blt_Offset(PathPrintSwitches, pathSep), BLT_SWITCH_NULL_OK}, 
    {BLT_SWITCH_BITS_NOARG,  "-noleadingseparator", "", (char *)NULL,
        Blt_Offset(PathPrintSwitches, flags),  0, PATH_NO_LEADING_SEPARATOR},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;                 /* Parse flags. */
    const char *pathSep;                /* Path separator. */
    Blt_TreeNode root;                  /* Starting node of path. */
} PathParseSwitches;

static Blt_SwitchSpec pathParseSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-from",  "node", (char *)NULL,
        Blt_Offset(PathParseSwitches, root),  0, 0, &nodeSwitch},
    {BLT_SWITCH_BITS_NOARG, "-nocomplain", "", (char *)NULL,
        Blt_Offset(PathParseSwitches, flags), 0, PATH_NOCOMPLAIN},
    {BLT_SWITCH_STRING, "-separator", "char", (char *)NULL,
        Blt_Offset(PathParseSwitches, pathSep), BLT_SWITCH_NULL_OK}, 
    {BLT_SWITCH_END}
};


typedef struct {
    TreeCmd *cmdPtr;
    unsigned int flags;
    int type;
    int mode;
    char *key;
    const char *command;
} SortSwitches;

#define SORT_RECURSE            (1<<2)
#define SORT_DECREASING         (1<<3)
#define SORT_PATHNAME           (1<<4)

enum SortTypes { SORT_DICTIONARY, SORT_REAL, SORT_INTEGER, SORT_ASCII, 
        SORT_COMMAND };

enum SortModes { SORT_FLAT, SORT_REORDER };

static Blt_SwitchSpec sortSwitches[] = 
{
    {BLT_SWITCH_VALUE,   "-ascii",      "", (char *)NULL,
        Blt_Offset(SortSwitches, type),    0, SORT_ASCII},
    {BLT_SWITCH_STRING,  "-command",    "command", (char *)NULL,
        Blt_Offset(SortSwitches, command), 0},
    {BLT_SWITCH_BITS_NOARG, "-decreasing", "", (char *)NULL,
        Blt_Offset(SortSwitches, flags),   0, SORT_DECREASING},
    {BLT_SWITCH_VALUE,   "-dictionary", "", (char *)NULL,
        Blt_Offset(SortSwitches, type),    0, SORT_DICTIONARY},
    {BLT_SWITCH_VALUE,   "-integer",    "", (char *)NULL,
        Blt_Offset(SortSwitches, type),    0, SORT_INTEGER},
    {BLT_SWITCH_STRING,  "-key",        "string", (char *)NULL,
        Blt_Offset(SortSwitches, key),     0},
    {BLT_SWITCH_BITS_NOARG, "-path",       "", (char *)NULL,
        Blt_Offset(SortSwitches, flags),   0, SORT_PATHNAME},
    {BLT_SWITCH_VALUE,   "-real",       "", (char *)NULL,
        Blt_Offset(SortSwitches, type),    0, SORT_REAL},
    {BLT_SWITCH_VALUE,   "-recurse",    "", (char *)NULL,
        Blt_Offset(SortSwitches, flags),   0, SORT_RECURSE},
    {BLT_SWITCH_VALUE,   "-reorder",    "", (char *)NULL,
        Blt_Offset(SortSwitches, mode),    0, SORT_REORDER},
    {BLT_SWITCH_END}
};

typedef struct {
    int mask;
} TraceSwitches;

static Blt_SwitchSpec traceSwitches[] = 
{
    {BLT_SWITCH_BITS_NOARG, "-whenidle", "", (char *)NULL,
        Blt_Offset(TraceSwitches, mask), 0, TREE_NOTIFY_WHENIDLE},
    {BLT_SWITCH_END}
};

static Tcl_InterpDeleteProc TreeInterpDeleteProc;
static Blt_TreeApplyProc MatchNodeProc, SortApplyProc;
static Blt_TreeApplyProc ApplyNodeProc;
static Blt_TreeTraceProc TreeTraceProc;
static Tcl_CmdDeleteProc TreeInstDeleteProc;
static Blt_TreeCompareNodesProc CompareNodes;

static Tcl_ObjCmdProc TreeObjCmd;
static Tcl_ObjCmdProc TreeInstObjCmd;
static Blt_TreeNotifyEventProc TreeEventProc;

#ifdef notdef
static int
IsTag(Blt_Tree tree, const char *string)
{
    if (strcmp(string, "all") == 0) {
        return TRUE;
    } else if (strcmp(string, "root") == 0) {
        return TRUE;
    } else {
        Blt_HashTable *tablePtr;
        
        tablePtr = Blt_Tree_TagHashTable(tree, string);
        if (tablePtr == NULL) {
            return FALSE;
        }
    }
    return TRUE;
}
#endif

static int
IsNodeId(const char *string)
{
    long value;

    return (Blt_GetLong(NULL, string, &value) == TCL_OK);
}

static int
IsNodeIdOrModifier(const char *string)
{
    if (strstr(string, "->") == NULL) {
        return IsNodeId(string);
    }
    return TRUE;
}


/*
 *---------------------------------------------------------------------------
 *
 * ChildSwitch --
 *
 *      Convert a Tcl_Obj representing the label of a child node into its
 *      integer node id.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ChildSwitch(
    ClientData clientData,              /* Flag indicating if the node is
                                         * considered before or after the
                                         * insertion position. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Not used. */
    int flags)                          /* Not used. */
{
    InsertSwitches *insertPtr = (InsertSwitches *)record;
    Blt_TreeNode node;
    const char *string;

    string = Tcl_GetString(objPtr);
    node = Blt_Tree_FindChild(insertPtr->parent, string);
    if (node == NULL) {
        Tcl_AppendResult(interp, "can't find a child named \"", string, 
                 "\" in \"", Blt_Tree_NodeLabel(insertPtr->parent), "\"",
                 (char *)NULL);  
        return TCL_ERROR;
    }                     
    insertPtr->position = Blt_Tree_NodeDegree(node);
    if (clientData == INSERT_AFTER) {
        insertPtr->position++;
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TreeNodeSwitchProc --
 *
 *      Convert a Tcl_Obj representing a node number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_TreeNodeSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Blt_TreeNode *nodePtr = (Blt_TreeNode *)(record + offset);
    Blt_TreeNode node;
    Blt_Tree tree  = clientData;
    int result;

    result = Blt_Tree_GetNodeFromObj(interp, tree, objPtr, &node);
    if (result != TCL_OK) {
        return TCL_ERROR;
    }
    *nodePtr = node;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * OrderSwitch --
 *
 *      Convert a string represent a node number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OrderSwitch(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *orderPtr = (int *)(record + offset);
    char c;
    char *string;

    string = Tcl_GetString(objPtr);
    c = string[0];
    if ((c == 'b') && (strcmp(string, "breadthfirst") == 0)) {
        *orderPtr = TREE_BREADTHFIRST;
    } else if ((c == 'i') && (strcmp(string, "inorder") == 0)) {
        *orderPtr = TREE_INORDER;
    } else if ((c == 'p') && (strcmp(string, "preorder") == 0)) {
        *orderPtr = TREE_PREORDER;
    } else if ((c == 'p') && (strcmp(string, "postorder") == 0)) {
        *orderPtr = TREE_POSTORDER;
    } else {
        Tcl_AppendResult(interp, "bad order \"", string, 
                 "\": should be breadthfirst, inorder, preorder, or postorder",
                 (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PatternSwitch --
 *
 *      Convert a string represent a node number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PatternSwitch(
    ClientData clientData,              /* Flag indicating type of
                                         * pattern. */
    Tcl_Interp *interp,                 /* Not used. */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    Blt_List *listPtr = (Blt_List *)(record + offset);

    if (*listPtr == NULL) {
        *listPtr = Blt_List_Create(BLT_STRING_KEYS);
    }
    Blt_List_Append(*listPtr, Tcl_GetString(objPtr), clientData);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreePatterns --
 *
 *      Convert a string represent a node number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreePatterns(ClientData clientData, char *record, int offset, int flags)
{
    Blt_List *listPtr = (Blt_List *)(record + offset);

    if (*listPtr != NULL) {
        Blt_List_Destroy(*listPtr);
        /* 
         * This routine can be called several times for each switch that
         * appends to this list. Mark it NULL, so we don't try to destroy
         * the list again.
         */
        *listPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatSwitch --
 *
 *      Convert a string represent a node number into its integer value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FormatSwitch(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Not used. */
    int flags)                          /* Not used. */
{
    PositionSwitches *pdPtr = (PositionSwitches *)record;
    char *string;

    string = Tcl_GetString(objPtr);
    if (strcmp(string, "position") == 0) {
        pdPtr->withParent = FALSE;
        pdPtr->withId = FALSE;
    } else if (strcmp(string, "id+position") == 0) {
        pdPtr->withParent = FALSE;
        pdPtr->withId = TRUE;
    } else if (strcmp(string, "parent-at-position") == 0) {
        pdPtr->withParent = TRUE;
        pdPtr->withId = FALSE;
    } else if (strcmp(string, "id+parent-at-position") == 0) {
        pdPtr->withParent = TRUE;
        pdPtr->withId  = TRUE;
    } else {
        Tcl_AppendResult(interp, "bad format \"", string, 
 "\": should be position, parent-at-position, id+position, or id+parent-at-position",
                 (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FieldsSwitch --
 *
 *      Convert a string representing a list of field names into a mask.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FieldsSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *maskPtr = (int *)(record + offset);
    Tcl_Obj **objv;
    int objc, i;
    unsigned int mask;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    mask = 0;
    for (i = 0; i < objc; i++) {
        const char *string;
        char c;

        string = Tcl_GetString(objv[i]);
        c = string[0];
        if ((c == 's') && (strcmp(string, "size") == 0)) {
            mask |= READ_DIR_SIZE;
        } else if ((c == 'm') && (strcmp(string, "mode") == 0)) {
            mask |= READ_DIR_MODE;
        } else if ((c == 'p') && (strcmp(string, "perms") == 0)) {
            mask |= READ_DIR_PERMS;
        } else if ((c == 't') && (strcmp(string, "type") == 0)) {
            mask |= READ_DIR_TYPE;
        } else if ((c == 'u') && (strcmp(string, "uid") == 0)) {
            mask |= READ_DIR_UID;
        } else if ((c == 'g') && (strcmp(string, "gid") == 0)) {
            mask |= READ_DIR_GID;
        } else if ((c == 'a') && (strcmp(string, "atime") == 0)) {
            mask |= READ_DIR_ATIME;
        } else if ((c == 'c') && (strcmp(string, "ctime") == 0)) {
            mask |= READ_DIR_CTIME;
        } else if ((c == 'm') && (strcmp(string, "mtime") == 0)) {
            mask |= READ_DIR_MTIME;
        } else if ((c == 'i') && (strcmp(string, "ino") == 0)) {
            mask |= READ_DIR_INO;
        } else if ((c == 'd') && (strcmp(string, "dev") == 0)) {
            mask |= READ_DIR_DEV;
        } else if ((c == 'n') && (strcmp(string, "nlink") == 0)) {
            mask |= READ_DIR_NLINK;
        } else if ((c == 'a') && (strcmp(string, "all") == 0)) {
            mask |= READ_DIR_ALL;
        } else {
            Tcl_AppendResult(interp, "unknown field name \"", string, "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    if (mask == 0) {
        mask = READ_DIR_DEFAULT;
    }
    *maskPtr = mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeSwitch --
 *
 *      Convert a string representing a list of type into a mask.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *maskPtr = (int *)(record + offset);
    Tcl_Obj **objv;
    int objc, i;
    unsigned int mask;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    mask = 0;
    for (i = 0; i < objc; i++) {
        const char *string;
        char c;
        int length;
        
        string = Tcl_GetStringFromObj(objv[i], &length);
        c = string[0];
        if ((c == 'f') && (strncmp(string, "file", length) == 0)) {
            mask |= TCL_GLOB_TYPE_FILE;
        } else if ((c == 'd') && (strncmp(string, "directory", length) == 0)) {
            mask |= TCL_GLOB_TYPE_DIR;
        } else if ((c == 'l') && (strncmp(string, "link", length) == 0)) {
            mask |= TCL_GLOB_TYPE_LINK;
        } else if ((c == 'p') && (strncmp(string, "pipe", length) == 0)) {
            mask |= TCL_GLOB_TYPE_PIPE;
        } else if ((c == 's') && (strncmp(string, "socket", length) == 0)) {
            mask |= TCL_GLOB_TYPE_SOCK;
        } else if ((c == 'b') && (strncmp(string, "block", length) == 0)) {
            mask |= TCL_GLOB_TYPE_BLOCK;
        } else if ((c == 'c') && (strncmp(string, "character", length) == 0)) {
            mask |= TCL_GLOB_TYPE_CHAR;
        } else {
            Tcl_AppendResult(interp, "unknown type name \"", string, "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    *maskPtr = mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PermSwitch --
 *
 *      Convert a string representing a list of permissions into a mask.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PermSwitchProc(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Offset to field in structure */
    int flags)                          /* Not used. */
{
    int *maskPtr = (int *)(record + offset);
    int i, length;
    unsigned int mask;
    const char *string;

    string = Tcl_GetStringFromObj(objPtr, &length);
    mask = 0;
    for (i = 0; i < length; i++) {
        char c;
        
        c = string[i];
        if (c == 'r') {
            mask |= TCL_GLOB_PERM_R;
        } else if (c == 'w') {
            mask |= TCL_GLOB_PERM_W;
        } else if (c == 'x') {
            mask |= TCL_GLOB_PERM_X;
        } else {
            Tcl_AppendResult(interp, "unknown permssions \"", string, "\"",
                (char *)NULL);
            return TCL_ERROR;
        }
    }
    *maskPtr = mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeNodes --
 *
 *      Convert a string represent a node number into its integer value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
FreeNodes(ClientData clientData, char *record, int offset, int flags)
{
    Blt_HashTable *tablePtr = (Blt_HashTable *)(record + offset);

    Blt_DeleteHashTable(tablePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * NodesSwitch --
 *
 *      Convert a Tcl_Obj representing a node number into its integer
 *      value.
 *
 * Results:
 *      The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NodesSwitch(
    ClientData clientData,              /* Not used. */
    Tcl_Interp *interp,                 /* Interpreter to send results back
                                         * to */
    const char *switchName,             /* Not used. */
    Tcl_Obj *objPtr,                    /* String representation */
    char *record,                       /* Structure record */
    int offset,                         /* Not used. */
    int flags)                          /* Not used. */
{
    FindSwitches *findPtr = (FindSwitches *)record;
    int objc;
    Tcl_Obj **objv;
    int i;
    
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 0; i < objc; i++) {
        int isNew;
        Blt_TreeNode node;

        if (Blt_Tree_GetNodeFromObj(interp, findPtr->cmdPtr->tree, objv[i],
                &node) != TCL_OK) {
            Blt_DeleteHashTable(&findPtr->excludeTable);
            return TCL_ERROR;
        }
        Blt_CreateHashEntry(&findPtr->excludeTable, (char *)node, &isNew);
    }
    return TCL_OK;
}

static void 
FreeNotifier(TreeCmd *cmdPtr, Notifier *notifyPtr)
{
    if (notifyPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(&cmdPtr->notifyTable, notifyPtr->hashPtr);
    }
    if (notifyPtr->link != NULL) {
        Blt_Chain_DeleteLink(cmdPtr->notifiers, notifyPtr->link);
    }
    Tcl_DecrRefCount(notifyPtr->cmdObjPtr);
    if (notifyPtr->tag != NULL) {
        Blt_Free(notifyPtr->tag);
    }
    Blt_Free(notifyPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTreeCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static TreeCmdInterpData *
GetTreeCmdInterpData(Tcl_Interp *interp)
{
    TreeCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (TreeCmdInterpData *)
        Tcl_GetAssocData(interp, TREE_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
        dataPtr = Blt_AssertMalloc(sizeof(TreeCmdInterpData));
        dataPtr->interp = interp;
        Tcl_SetAssocData(interp, TREE_THREAD_KEY, TreeInterpDeleteProc,
                 dataPtr);
        Blt_InitHashTable(&dataPtr->treeTable, BLT_ONE_WORD_KEYS);
        Blt_InitHashTable(&dataPtr->fmtTable, BLT_STRING_KEYS);
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTreeCmd --
 *
 *      Find the tree command associated with the TCL command "string".
 *      
 *      We have to do multiple lookups to get this right.  
 *
 *      The first step is to generate a canonical command name.  If an
 *      unqualified command name (i.e. no namespace qualifier) is given, we
 *      should search first the current namespace and then the global one.
 *      Most TCL commands (like Tcl_GetCmdInfo) look only at the global
 *      namespace.
 *
 *      Next check if the string is 
 *              a) a TCL command and 
 *              b) really is a command for a tree object.  
 *      Tcl_GetCommandInfo will get us the objClientData field that should be
 *      a cmdPtr.  We can verify that by searching our hashtable of cmdPtr
 *      addresses.
 *
 * Results:
 *      A pointer to the tree command.  If no associated tree command can be
 *      found, NULL is returned.  It's up to the calling routines to generate
 *      an error message.
 *
 *---------------------------------------------------------------------------
 */
static TreeCmd *
GetTreeCmd(
    TreeCmdInterpData *dataPtr, 
    Tcl_Interp *interp, 
    const char *string)
{
    Blt_ObjectName objName;
    Tcl_CmdInfo cmdInfo;
    Blt_HashEntry *hPtr;
    Tcl_DString ds;
    const char *treeName;
    int result;

    /* Pull apart the tree name and put it back together in a standard
     * format. */
    if (!Blt_ParseObjectName(interp, string, &objName, BLT_NO_ERROR_MSG)) {
        return NULL;                    /* No such parent namespace. */
    }
    /* Rebuild the fully qualified name. */
    treeName = Blt_MakeQualifiedName(&objName, &ds);
    result = Tcl_GetCommandInfo(interp, treeName, &cmdInfo);
    Tcl_DStringFree(&ds);

    if (!result) {
        return NULL;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->treeTable, 
                             (char *)(cmdInfo.objClientData));
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static Blt_TreeNode 
ParseModifiers(Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node,
               char *modifiers)
{
    char *p, *token;

    p = modifiers;
    do {
        p += 2;                         /* Skip the initial "->" */
        token = strstr(p, "->");
        if (token != NULL) {
            *token = '\0';
        }
        if (IsNodeId(p)) {
            long inode;
            
            if (Blt_GetLong(interp, p, &inode) != TCL_OK) {
                node = NULL;
            } else {
                node = Blt_Tree_GetNodeFromIndex(tree, inode);
            }
        } else if ((*p == 'p') && (strcmp(p, "parent") == 0)) {
            node = Blt_Tree_ParentNode(node);
        } else if ((*p == 'f') && (strcmp(p, "firstchild") == 0)) {
            node = Blt_Tree_FirstChild(node);
        } else if ((*p == 'l') && (strcmp(p, "lastchild") == 0)) {
            node = Blt_Tree_LastChild(node);
        } else if ((*p == 'n') && (strcmp(p, "next") == 0)) {
            node = Blt_Tree_NextNode(NULL, node);
        } else if ((*p == 'n') && (strcmp(p, "nextsibling") == 0)) {
            node = Blt_Tree_NextSibling(node);
        } else if ((*p == 'p') && (strcmp(p, "previous") == 0)) {
            node = Blt_Tree_PrevNode(NULL, node);
        } else if ((*p == 'p') && (strcmp(p, "prevsibling") == 0)) {
            node = Blt_Tree_PrevSibling(node);
        } else {
            int length;

            length = strlen(p);
            if (length > 0) {
                char *endp;

                endp = p + length - 1;
                if ((*p == '"') && (*endp == '"')) {
                    *endp = '\0';
                    node = Blt_Tree_FindChild(node, p + 1);
                    *endp = '"';
                } else {
                    node = Blt_Tree_FindChild(node, p);
                }               
            }
        }
        if (node == NULL) {
            goto error;
        }
        if (token != NULL) {
            *token = '-';               /* Repair the string */
        }
        p = token;
    } while (token != NULL);
    return node;
 error:
    if (token != NULL) {
        *token = '-';                   /* Repair the string */
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetForeignNode --
 *
 *---------------------------------------------------------------------------
 */
static int 
GetForeignNode(Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr,
               Blt_TreeNode *nodePtr)
{
    Blt_TreeNode node;
    char *string;
    char *p;
    char save;

    save = '\0';                        /* Suppress compiler warning. */
    string = Tcl_GetString(objPtr);

    /* Check if modifiers are present. */
    p = strstr(string, "->");
    if (p != NULL) {
        save = *p;
        *p = '\0';
    }
    if (IsNodeId(string)) {
        long inode;

        if (p != NULL) {
            if (Blt_GetLong(interp, string, &inode) != TCL_OK) {
                goto error;
            }
        } else {
            if (Blt_GetLongFromObj(interp, objPtr, &inode) != TCL_OK) {
                goto error;
            }
        }
        node = Blt_Tree_GetNodeFromIndex(tree, inode);
        if (p != NULL) {
            node = ParseModifiers(interp, tree, node, p);
        }
        if (node != NULL) {
            *nodePtr = node;
            if (p != NULL) {
                *p = save;
            }
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in \"",
                Blt_Tree_Name(tree), "\"", (char *)NULL);
 error:
    if (p != NULL) {
        *p = save;                      /* Restore the string */
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * SkipSeparators --
 *
 *      Moves the character pointer past one of more separators.
 *
 * Results:
 *      Returns the updates character pointer.
 *
 *---------------------------------------------------------------------------
 */
static const char *
SkipSeparators(const char *path, const char *sep, int length)
{
    while ((*path == *sep) && (strncmp(path, sep, length) == 0)) {
        path += length;
    }
    return path;
}

/*
 *---------------------------------------------------------------------------
 *
 * SplitPath --
 *
 *      Returns a Tcl_Obj list of the path components.  Trailing and
 *      multiple separators are ignored.
 *
 *---------------------------------------------------------------------------
 */
static Tcl_Obj *
SplitPath(Tcl_Interp *interp, Tcl_Obj *pathObjPtr, const char *sep)
{
    const char *path, *p, *endPtr;
    int sepLen;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    path = Tcl_GetString(pathObjPtr);
    sepLen = strlen(sep);

    /* Skip the first separator. */
    p = SkipSeparators(path, sep, sepLen);
    for (endPtr = strstr(p, sep); ((endPtr != NULL) && (*endPtr != '\0'));
         endPtr = strstr(p, sep)) {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(p, endPtr - p);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        p = SkipSeparators(endPtr + sepLen, sep, sepLen);
    }
    /* Pick up last path component */
    if (p[0] != '\0') {
        Tcl_Obj *objPtr;
        
        objPtr = Tcl_NewStringObj(p, -1);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    return listObjPtr;
}

static int
AddTag(TreeCmd *cmdPtr, Blt_TreeNode node, const char *tagName)
{
    if (strcmp(tagName, "root") == 0) {
        Tcl_AppendResult(cmdPtr->interp, "can't add reserved tag \"",
                         tagName, "\"", (char *)NULL);
        return TCL_ERROR;
    }
    Blt_Tree_AddTag(cmdPtr->tree, node, tagName);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteNode --
 *
 *---------------------------------------------------------------------------
 */
static void
DeleteNode(TreeCmd *cmdPtr, Blt_TreeNode node)
{
    Blt_TreeNode root;

    if (!Blt_Tree_TagTableIsShared(cmdPtr->tree)) {
        Blt_Tree_ClearTags(cmdPtr->tree, node);
    }
    root = Blt_Tree_RootNode(cmdPtr->tree);
    if (node == root) {
        Blt_TreeNode next;

        /* Don't delete the root node. Simply clean out the tree. */
        for (node = Blt_Tree_FirstChild(node); node != NULL; node = next) {
            next = Blt_Tree_NextSibling(node);
            Blt_Tree_DeleteNode(cmdPtr->tree, node);
        }           
    } else if (Blt_Tree_IsAncestor(root, node)) {
        Blt_Tree_DeleteNode(cmdPtr->tree, node);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * PrintTraceFlags --
 *
 *---------------------------------------------------------------------------
 */
static void
PrintTraceFlags(unsigned int flags, char *string)
{
    char *s;

    s = string;
    if (flags & TREE_TRACE_READS) {
        *s++ = 'r';
    } 
    if (flags & TREE_TRACE_WRITES) {
        *s++ = 'w';
    } 
    if (flags & TREE_TRACE_UNSETS) {
        *s++ = 'u';
    } 
    if (flags & TREE_TRACE_CREATES) {
        *s++ = 'c';
    } 
    *s = '\0';
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTraceFlags --
 *
 *---------------------------------------------------------------------------
 */
static int
GetTraceFlags(const char *string)
{
    const char *s;
    unsigned int flags;

    flags = 0;
    for (s = string; *s != '\0'; s++) {
        int c;

        c = toupper(*s);
        switch (c) {
        case 'R':
            flags |= TREE_TRACE_READS;
            break;
        case 'W':
            flags |= TREE_TRACE_WRITES;
            break;
        case 'U':
            flags |= TREE_TRACE_UNSETS;
            break;
        case 'C':
            flags |= TREE_TRACE_CREATES;
            break;
        default:
            return -1;
        }
    }
    return flags;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetValues --
 *
 *---------------------------------------------------------------------------
 */
static int
SetValues(TreeCmd *cmdPtr, Blt_TreeNode node, int objc, Tcl_Obj *const *objv)
{
    int i;

    for (i = 0; i < objc; i += 2) {
        Blt_TreeKey key;

        if ((i + 1) == objc) {
            Tcl_AppendResult(cmdPtr->interp, "missing value for field \"", 
                             Tcl_GetString(objv[i]), "\"", (char *)NULL);
            return TCL_ERROR;
        }
        key = Blt_Tree_GetKey(cmdPtr->tree, Tcl_GetString(objv[i]));
        if (Blt_Tree_SetValueByKey(cmdPtr->interp, cmdPtr->tree, node, key, 
                             objv[i + 1]) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * UnsetValues --
 *
 *---------------------------------------------------------------------------
 */
static int
UnsetValues(TreeCmd *cmdPtr, Blt_TreeNode node, int objc, Tcl_Obj *const *objv)
{
    if (objc == 0) {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;

        for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &iter); key != NULL;
             key = Blt_Tree_NextKey(cmdPtr->tree, &iter)) {
            if (Blt_Tree_UnsetValueByKey(cmdPtr->interp, cmdPtr->tree, node, 
                        key) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    } else {
        int i;

        for (i = 0; i < objc; i ++) {
            if (Blt_Tree_UnsetValue(cmdPtr->interp, cmdPtr->tree, node, 
                Tcl_GetString(objv[i])) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

static int
ComparePatterns(Blt_List patternList, const char *string, int nocase)
{
    Blt_ListNode node;
    int result;

    if (nocase) {
        string = Blt_AssertStrdup(string);
        Blt_LowerCase((char *)string);
    }
    result = FALSE;
    for (node = Blt_List_FirstNode(patternList); node != NULL; 
        node = Blt_List_NextNode(node)) {
        size_t type;
        char *pattern;
                
        type = (size_t)Blt_List_GetValue(node);
        pattern = (char *)Blt_List_GetKey(node);
        switch (type) {
        case PATTERN_EXACT:
            result = (strcmp(string, pattern) == 0);
            break;
            
        case PATTERN_GLOB:
            result = Tcl_StringMatch(string, pattern);
            break;
                    
        case PATTERN_REGEXP:
            result = Tcl_RegExpMatch((Tcl_Interp *)NULL, string, pattern); 
            break;
        }
    }
    if (nocase) {
        Blt_Free((char *)string);
    }
    return result;
}


static int
CompareTags(
    Blt_Tree tree,
    Blt_TreeNode node,
    Blt_List tagList)
{
    Blt_ListNode tn;

    for (tn = Blt_List_FirstNode(tagList); tn != NULL; 
         tn = Blt_List_NextNode(tn)) {
        char *tag;

        tag = (char *)Blt_List_GetKey(tn);
                        if (Blt_Tree_HasTag(tree, node, tag)) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 *---------------------------------------------------------------------------
 *
 * GenerateName --
 *
 *      Generates an unique tree command name.  Tree names are in the form
 *      "treeN", where N is a non-negative integer. Check each name
 *      generated to see if it is already a tree. We want to recycle names
 *      if possible.
 *      
 * Results:
 *      Returns the unique name.  The string itself is stored in the
 *      dynamic string passed into the routine.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GenerateName(Tcl_Interp *interp, const char *prefix, const char *suffix,
             Tcl_DString *resultPtr)
{

    int i;
    const char *treeName;

    /* 
     * Parse the command and put back so that it's in a consistent
     * format.  
     *
     *  t1         <current namespace>::t1
     *  n1::t1     <current namespace>::n1::t1
     *  ::t1       ::t1
     *  ::n1::t1   ::n1::t1
     */
    treeName = NULL;            /* Suppress compiler warning. */
    for (i = 0; i < INT_MAX; i++) {
        Blt_ObjectName objName;
        Tcl_DString ds;
        char string[200];

        Tcl_DStringInit(&ds);
        Tcl_DStringAppend(&ds, prefix, -1);
        Blt_FormatString(string, 200, "tree%d", i);
        Tcl_DStringAppend(&ds, string, -1);
        Tcl_DStringAppend(&ds, suffix, -1);
        if (!Blt_ParseObjectName(interp, Tcl_DStringValue(&ds), &objName, 0)) {
            Tcl_DStringFree(&ds);
            return NULL;
        }
        treeName = Blt_MakeQualifiedName(&objName, resultPtr);
        Tcl_DStringFree(&ds);

        if (Blt_Tree_Exists(interp, treeName)) {
            continue;          /* A tree by this name already exists. */
        }
        if (Blt_CommandExists(interp, treeName)) {
            continue;           /* A command by this name already exists. */
        }
        break;
    }
    return treeName;
}

static TreeCmd *
CreateTreeCmd(ClientData clientData, Tcl_Interp *interp, const char *name)
{
    TreeCmdInterpData *dataPtr = clientData;
    Tcl_DString ds;
    Blt_Tree tree;

    Tcl_DStringInit(&ds);
    if (name == NULL) {
        name = GenerateName(interp, "", "", &ds);
    } else {
        char *p;

        p = strstr(name, "#auto");
        if (p != NULL) {
            *p = '\0';
            name = GenerateName(interp, name, p + 5, &ds);
            *p = '#';
        } else {
            Blt_ObjectName objName;

            /* 
             * Parse the command and put back so that it's in a consistent
             * format.
             *
             *  t1         <current namespace>::t1
             *  n1::t1     <current namespace>::n1::t1
             *  ::t1       ::t1
             *  ::n1::t1   ::n1::t1
             */
            if (!Blt_ParseObjectName(interp, name, &objName, 0)) {
                return NULL;
            }
            name = Blt_MakeQualifiedName(&objName, &ds);
            /* 
             * Check if the command already exists. 
             */
            if (Blt_CommandExists(interp, name)) {
                Tcl_AppendResult(interp, "a command \"", name,
                                 "\" already exists", (char *)NULL);
                goto error;
            }
            if (Blt_Tree_Exists(interp, name)) {
                Tcl_AppendResult(interp, "a tree \"", name, 
                        "\" already exists", (char *)NULL);
                goto error;
            }
        } 
    } 
    if (name == NULL) {
        goto error;
    }
    tree = Blt_Tree_Open(interp, name, TREE_CREATE);
    if (tree != NULL) {
        int isNew;
        TreeCmd *cmdPtr;

        cmdPtr = Blt_AssertCalloc(1, sizeof(TreeCmd));
        cmdPtr->interpDataPtr = dataPtr;
        cmdPtr->tree = tree;
        cmdPtr->interp = interp;
        Blt_InitHashTable(&cmdPtr->traceTable, BLT_STRING_KEYS);
        Blt_InitHashTable(&cmdPtr->notifyTable, BLT_STRING_KEYS);
        cmdPtr->notifiers = Blt_Chain_Create();
        cmdPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)name, 
                (Tcl_ObjCmdProc *)TreeInstObjCmd, cmdPtr, TreeInstDeleteProc);
        cmdPtr->tablePtr = &dataPtr->treeTable;
        cmdPtr->hashPtr = Blt_CreateHashEntry(cmdPtr->tablePtr, (char *)cmdPtr,
              &isNew);
        Blt_SetHashValue(cmdPtr->hashPtr, cmdPtr);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), (char *)name, -1);
        Tcl_DStringFree(&ds);
        /* 
         * Since we store the TCL command and notifier information on the
         * client side, we need to also cleanup when we see a delete event.
         * So just register a callback for all tree events to catch
         * anything we need to know about.
         */
        Blt_Tree_CreateEventHandler(cmdPtr->tree, TREE_NOTIFY_ALL, 
             TreeEventProc, cmdPtr);
        return cmdPtr;
    }
 error:
    Tcl_DStringFree(&ds);
    return NULL;
}

static void
DupNode(TreeCmd *srcPtr, Blt_TreeNode srcNode, 
        TreeCmd *destPtr, Blt_TreeNode destNode)
{
    const char *label;
    Blt_TreeKey key;
    Blt_TreeKeyIterator kiter;
    Blt_HashSearch iter;
    Blt_HashEntry *hPtr;
    Blt_TreeNode child;

    label = Blt_Tree_NodeLabel(srcNode);
    Blt_Tree_RelabelNode(destPtr->tree, destNode, label);

    /* Copy the data fields. */
    for (key = Blt_Tree_FirstKey(srcPtr->tree, srcNode, &kiter); key != NULL;
         key = Blt_Tree_NextKey(srcPtr->tree, &kiter)) {
        Tcl_Obj *objPtr;
        
        if (Blt_Tree_GetValueByKey((Tcl_Interp *)NULL, srcPtr->tree, 
                                   srcNode, key, &objPtr) == TCL_OK) {
            Blt_Tree_SetValueByKey((Tcl_Interp *)NULL, destPtr->tree, 
                                   destNode, key, objPtr);
        } 
    }
    /* Add tags to destination tree node. */
    for (hPtr = Blt_Tree_FirstTag(srcPtr->tree, &iter); 
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        Blt_HashEntry *h2Ptr;
        Blt_TreeTagEntry *tPtr;
        
        tPtr = Blt_GetHashValue(hPtr);
        h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)srcNode);
        if (h2Ptr != NULL) {
            if (AddTag(destPtr, destNode, tPtr->tagName)!= TCL_OK) {
                return;
            }
        }
    }
    for (child = Blt_Tree_FirstChild(srcNode); child != NULL;
         child = Blt_Tree_NextSibling(child)) {
        const char *label;
        Blt_TreeNode newNode;   /* Newly created copy. */
        
        label = Blt_Tree_NodeLabel(child);
        newNode = Blt_Tree_CreateNode(destPtr->tree, destNode, label, -1);
        DupNode(srcPtr, child, destPtr, newNode);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * MatchNodeProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
MatchNodeProc(Blt_TreeNode node, ClientData clientData, int order)
{
    FindSwitches *findPtr = clientData;
    Tcl_DString ds;
    TreeCmd *cmdPtr = findPtr->cmdPtr;
    Tcl_Interp *interp = findPtr->cmdPtr->interp;
    int result, invert;

    if ((findPtr->flags & MATCH_LEAFONLY) && (!Blt_Tree_IsLeaf(node))) {
        return TCL_OK;
    }
    if ((findPtr->maxDepth >= 0) &&
        (Blt_Tree_NodeDepth(node) > findPtr->maxDepth)) {
        return TCL_OK;                  /* Stop if we've exceeded the
                                         * maximum depth requested. */
    }
    result = TRUE;
    Tcl_DStringInit(&ds);
    if (findPtr->keyList != NULL) {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;

        result = FALSE;         /* It's false if no keys match. */
        for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &iter); key != NULL;
             key = Blt_Tree_NextKey(cmdPtr->tree, &iter)) {
            
            result = ComparePatterns(findPtr->keyList, key, 0);
            if (!result) {
                continue;
            }
            if (findPtr->patternList != NULL) {
                const char *string;
                Tcl_Obj *objPtr;

                Blt_Tree_GetValue(interp, cmdPtr->tree, node, key, &objPtr);
                string = (objPtr == NULL) ? "" : Tcl_GetString(objPtr);
                result = ComparePatterns(findPtr->patternList, string, 
                         findPtr->flags & MATCH_NOCASE);
                if (!result) {
                    continue;
                }
            }
            break;
        }
    } else if (findPtr->patternList != NULL) {      
        const char *string;

        if (findPtr->flags & MATCH_PATHNAME) {
            string = Blt_Tree_NodePath(node, &ds);
        } else {
            string = Blt_Tree_NodeLabel(node);
        }
        result = ComparePatterns(findPtr->patternList, string, 
                findPtr->flags & MATCH_NOCASE);              
    }
    if (findPtr->tagList != NULL) {
        result = CompareTags(cmdPtr->tree, node, findPtr->tagList);
    }
    Tcl_DStringFree(&ds);
    
    invert = (findPtr->flags & MATCH_INVERT) ? TRUE : FALSE;
    if ((result != invert) &&
        /* Check if the matching node is on the exclude list. */        
        ((findPtr->excludeTable.numEntries == 0) || 
        (Blt_FindHashEntry(&findPtr->excludeTable, (char *)node) == NULL))) {
        Tcl_Obj *objPtr;

        if (findPtr->addTag != NULL) {
            if (AddTag(cmdPtr, node, findPtr->addTag) != TCL_OK) {
                return TCL_ERROR;
            }
        }
        /* Save the node id in our list. */
        objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
        Tcl_ListObjAppendElement(interp, findPtr->listObjPtr, objPtr);
        
        /* Execute a procedure for the matching node. */
        if (findPtr->cmdObjPtr != NULL) {
            Tcl_Obj *cmdObjPtr;

            cmdObjPtr = Tcl_DuplicateObj(findPtr->cmdObjPtr);
            Tcl_ListObjAppendElement(interp, cmdObjPtr, objPtr);
            Tcl_IncrRefCount(cmdObjPtr);
            result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
            Tcl_DecrRefCount(cmdObjPtr);
            if (result != TCL_OK) {
                return result;
            }
        }
        findPtr->numMatches++;
        if ((findPtr->maxMatches > 0) && 
            (findPtr->numMatches >= findPtr->maxMatches)) {
            return TCL_BREAK;
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ApplyNodeProc --
 *
 *---------------------------------------------------------------------------
 */
static int
ApplyNodeProc(Blt_TreeNode node, ClientData clientData, int order)
{
    ApplySwitches *applyPtr = clientData;
    TreeCmd *cmdPtr = applyPtr->cmdPtr;
    Tcl_Interp *interp = cmdPtr->interp;
    int invert, result;
    Tcl_DString ds;

    if ((applyPtr->flags & MATCH_LEAFONLY) && (!Blt_Tree_IsLeaf(node))) {
        return TCL_OK;
    }
    if ((applyPtr->maxDepth >= 0) &&
        (applyPtr->maxDepth < Blt_Tree_NodeDepth(node))) {
        return TCL_OK;
    }
    Tcl_DStringInit(&ds);
    result = TRUE;
    if (applyPtr->keyList != NULL) {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;

        result = FALSE;                 /* It's false if no keys match. */
        for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &iter);
             key != NULL; key = Blt_Tree_NextKey(cmdPtr->tree, &iter)) {
            
            result = ComparePatterns(applyPtr->keyList, key, 0);
            if (!result) {
                continue;
            }
            if (applyPtr->patternList != NULL) {
                const char *string;
                Tcl_Obj *objPtr;

                Blt_Tree_GetValue(interp, cmdPtr->tree, node, key, &objPtr);
                string = (objPtr == NULL) ? "" : Tcl_GetString(objPtr);
                result = ComparePatterns(applyPtr->patternList, string, 
                         applyPtr->flags & MATCH_NOCASE);
                if (!result) {
                    continue;
                }
            }
            break;
        }
    } else if (applyPtr->patternList != NULL) {     
        const char *string;

        if (applyPtr->flags & MATCH_PATHNAME) {
            string = Blt_Tree_NodePath(node, &ds);
        } else {
            string = Blt_Tree_NodeLabel(node);
        }
        result = ComparePatterns(applyPtr->patternList, string, 
                applyPtr->flags & MATCH_NOCASE);                     
    }
    Tcl_DStringFree(&ds);
    if (applyPtr->tagList != NULL) {
        result = CompareTags(cmdPtr->tree, node, applyPtr->tagList);
    }
    invert = (applyPtr->flags & MATCH_INVERT) ? 1 : 0;
    if (result != invert) {
        Tcl_Obj *cmdObjPtr;

        if (order == TREE_PREORDER) {
            cmdObjPtr = Tcl_DuplicateObj(applyPtr->preCmdObjPtr);       
        } else if (order == TREE_POSTORDER) {
            cmdObjPtr = Tcl_DuplicateObj(applyPtr->postCmdObjPtr);
        } else {
            return TCL_OK;
        }
        Tcl_ListObjAppendElement(interp, cmdObjPtr, 
                                 Tcl_NewLongObj(Blt_Tree_NodeId(node)));
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        return result;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ClearTracesAndEvents --
 *
 *---------------------------------------------------------------------------
 */
static void
ClearTracesAndEvents(TreeCmd *cmdPtr)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Blt_ChainLink link, next;

    /* 
     * When the tree token is released, all the traces and notification
     * events are automatically removed.  But we still need to clean up the
     * bookkeeping kept for traces. Clear all the tags and trace
     * information.
     */
    for (hPtr = Blt_FirstHashEntry(&cmdPtr->traceTable, &iter); hPtr != NULL;
        hPtr = Blt_NextHashEntry(&iter)) {
        TraceInfo *tracePtr;

        tracePtr = Blt_GetHashValue(hPtr);
        Blt_Free(tracePtr);
    }
    Blt_DeleteHashTable(&cmdPtr->traceTable);
    Blt_InitHashTable(&cmdPtr->traceTable, BLT_STRING_KEYS);
    for (link = Blt_Chain_FirstLink(cmdPtr->notifiers); link != NULL;
         link = next) {
        Notifier *notifyPtr;

        next = Blt_Chain_NextLink(link);
        notifyPtr = Blt_Chain_GetValue(link);
        FreeNotifier(cmdPtr, notifyPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ReleaseTreeObject --
 *
 *---------------------------------------------------------------------------
 */
static void
ReleaseTreeObject(TreeCmd *cmdPtr)
{
    ClearTracesAndEvents(cmdPtr);
    Blt_Tree_Close(cmdPtr->tree);
    cmdPtr->tree = NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeTraceProc --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeTraceProc(
    ClientData clientData,
    Tcl_Interp *interp,
    Blt_TreeNode node,                  /* Node that has just been
                                         * updated. */
    Blt_TreeKey key,                    /* Field that's updated. */
    unsigned int flags)
{
    TraceInfo *tracePtr = clientData; 
    Tcl_DString dsName;
    char string[5];
    const char *qualName;
    int result;
    Blt_ObjectName objName;
    Tcl_Obj *cmdObjPtr;

    cmdObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, 
                Tcl_NewStringObj(tracePtr->command, -1));
    Tcl_DStringInit(&dsName);
    objName.name = Tcl_GetCommandName(interp, tracePtr->cmdPtr->cmdToken);
    objName.nsPtr = Blt_GetCommandNamespace(tracePtr->cmdPtr->cmdToken);
    qualName = Blt_MakeQualifiedName(&objName, &dsName);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, 
                Tcl_NewStringObj(qualName, -1));
    Tcl_DStringFree(&dsName);
    if (node != NULL) {
        Tcl_ListObjAppendElement(interp, cmdObjPtr, 
                                 Tcl_NewLongObj(Blt_Tree_NodeId(node)));
    } else {
        Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewStringObj("", -1));
    }
    Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewStringObj(key, -1));
    PrintTraceFlags(flags, string);
    Tcl_ListObjAppendElement(interp, cmdObjPtr, Tcl_NewStringObj(string, -1));
    Tcl_IncrRefCount(cmdObjPtr);
    result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
    Tcl_DecrRefCount(cmdObjPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeEventProc --
 *
 *---------------------------------------------------------------------------
 */
static int
TreeEventProc(ClientData clientData, Blt_TreeNotifyEvent *eventPtr)
{
    TreeCmd *cmdPtr = clientData; 
    Blt_TreeNode node;
    const char *string;
    Blt_ChainLink link, next;

    switch (eventPtr->type) {
    case TREE_NOTIFY_CREATE:
        string = "-create";
        break;

    case TREE_NOTIFY_DELETE:
        node = Blt_Tree_GetNodeFromIndex(cmdPtr->tree, eventPtr->inode);
        if (node != NULL) {
            Blt_Tree_ClearTags(cmdPtr->tree, node);
        }
        string = "-delete";
        break;

    case TREE_NOTIFY_MOVE:
        string = "-move";
        break;

    case TREE_NOTIFY_SORT:
        string = "-sort";
        break;

    case TREE_NOTIFY_RELABEL:
        string = "-relabel";
        break;

    default:
        /* empty */
        string = "???";
        break;
    }   
    for (link = Blt_Chain_FirstLink(cmdPtr->notifiers); link != NULL; 
         link = next) {
        Notifier *notifyPtr;
        int result;
        Tcl_Obj *cmdObjPtr, *objPtr;
        int remove;

        result = TCL_OK;
        next = Blt_Chain_NextLink(link);
        notifyPtr = Blt_Chain_GetValue(link);
        remove = FALSE;
        if (notifyPtr->inode >= 0) {
            /* Test for specific node id. */
            if (notifyPtr->inode != eventPtr->inode) {
                continue;               /* No match. */
            }
            if (eventPtr->type == TREE_NOTIFY_DELETE) {
                remove = TRUE;          /* Must destroy notifier. Node no
                                         * longer exists. */
            }
        }
        if ((notifyPtr->tag != NULL) && 
            (!Blt_Tree_HasTag(cmdPtr->tree, eventPtr->node, notifyPtr->tag))) {
            goto next;                  /* Doesn't have the tag. */
        }
        if ((notifyPtr->mask & eventPtr->type) == 0) {
            goto next;                  /* Event not matching.  */
        }
        cmdObjPtr = Tcl_DuplicateObj(notifyPtr->cmdObjPtr);
        objPtr = Tcl_NewStringObj(string, -1);
        Tcl_ListObjAppendElement(cmdPtr->interp, cmdObjPtr, objPtr);
        objPtr = Tcl_NewLongObj(eventPtr->inode);
        Tcl_ListObjAppendElement(cmdPtr->interp, cmdObjPtr, objPtr);
        Tcl_IncrRefCount(cmdObjPtr);
        result = Tcl_EvalObjEx(cmdPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
        Tcl_DecrRefCount(cmdObjPtr);
        if (result != TCL_OK) {
            Tcl_BackgroundError(cmdPtr->interp);
        }
    next:
        if (remove) {
            FreeNotifier(cmdPtr, notifyPtr);
        }
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
        Tcl_ResetResult(cmdPtr->interp);
    }
    return TCL_OK;
}


static int
LoadFormat(Tcl_Interp *interp, const char *fmt)
{
    Tcl_DString ds;
    const char *version;
    char *pkg;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, "blt_tree_", 9);
    Tcl_DStringAppend(&ds, fmt, -1);
    pkg = Tcl_DStringValue(&ds);
    Blt_LowerCase(pkg);
    version = Tcl_PkgRequire(interp, pkg, BLT_VERSION, PKG_EXACT);
    Tcl_DStringFree(&ds);
    if (version == NULL) {
        Tcl_ResetResult(interp);
        return FALSE;
    }
    return TRUE;
}


static const char *
GetTypeFromMode(int mode)
{
#ifdef notdef
   if (mode == -1) {
       return "unknown";
   } else if (mode & FILE_ATTRIBUTE_DIRECTORY) {
        return "directory";
   } else if (mode &  FILE_ATTRIBUTE_HIDDEN) {
        return "hidden";
   } else if (mode &  FILE_ATTRIBUTE_READONLY) {
        return "readonly";
   } else {
       return "file";
   }
#else
    if (S_ISREG(mode)) {
        return "file";
    } else if (S_ISDIR(mode)) {
        return "directory";
    } else if (S_ISCHR(mode)) {
        return "characterSpecial";
    } else if (S_ISBLK(mode)) {
        return "blockSpecial";
    } else if (S_ISFIFO(mode)) {
        return "fifo";
#ifdef S_ISLNK
    } else if (S_ISLNK(mode)) {
        return "link";
#endif
#ifdef S_ISSOCK
    } else if (S_ISSOCK(mode)) {
        return "socket";
#endif
    }
    return "unknown";
#endif
}

        
static void
FillEntryData(Tcl_Interp *interp, Blt_Tree tree, Blt_TreeNode node, 
               Tcl_StatBuf *statPtr, ReadDirectory *readPtr)
{    
    if (readPtr->mask & READ_DIR_SIZE) {
        Blt_Tree_SetValue(interp, tree, node, "size",  
                Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_size));
    }
    if (readPtr->mask & READ_DIR_MTIME) {
        Blt_Tree_SetValue(interp, tree, node, "mtime",  
                Tcl_NewLongObj((long)statPtr->st_mtime));
    }
    if (readPtr->mask & READ_DIR_CTIME) {
        Blt_Tree_SetValue(interp, tree, node, "ctime",  
                Tcl_NewLongObj((long)statPtr->st_ctime));
    }
    if (readPtr->mask & READ_DIR_ATIME) {
        Blt_Tree_SetValue(interp, tree, node, "atime",  
                Tcl_NewLongObj((long)statPtr->st_atime));
    }
    if (readPtr->mask & READ_DIR_MODE) {
        Blt_Tree_SetValue(interp, tree, node, "mode", 
                Tcl_NewIntObj(statPtr->st_mode));
    }
    if (readPtr->mask & READ_DIR_PERMS) {
        Blt_Tree_SetValue(interp, tree, node, "perms", 
                Tcl_NewIntObj(statPtr->st_mode & 07777));
    }
    if (readPtr->mask & READ_DIR_UID) {
        Blt_Tree_SetValue(interp, tree, node, "uid", 
                Tcl_NewIntObj(statPtr->st_uid));
    }
    if (readPtr->mask & READ_DIR_GID) {
        Blt_Tree_SetValue(interp, tree, node, "gid", 
                Tcl_NewIntObj(statPtr->st_gid));
    }
    if (readPtr->mask & READ_DIR_TYPE) {
        Blt_Tree_SetValue(interp, tree, node, "type", 
                Tcl_NewStringObj(GetTypeFromMode(statPtr->st_mode), -1));
    }
    if (readPtr->mask & READ_DIR_INO) {
        Blt_Tree_SetValue(interp, tree, node, "ino",  
                Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_ino));
    }
    if (readPtr->mask & READ_DIR_NLINK) {
        Blt_Tree_SetValue(interp, tree, node, "nlink",  
                Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_nlink));
    }
    if (readPtr->mask & READ_DIR_DEV) {
        Blt_Tree_SetValue(interp, tree, node, "dev",  
                Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_rdev));
    }
}

static int ReadDirectoryIntoTree(Tcl_Interp *interp, TreeCmd *cmdPtr,
        Tcl_Obj *dirObjPtr, Blt_TreeNode parent, ReadDirectory *readPtr);

static int
MakeSubdirs(Tcl_Interp *interp, TreeCmd *cmdPtr, Tcl_Obj *objPtr,
                Blt_TreeNode parent, ReadDirectory *readPtr, int hidden)
{
    Tcl_GlobTypeData data = {
        TCL_GLOB_TYPE_DIR, TCL_GLOB_PERM_R, NULL, NULL
    };
    Tcl_Obj **objv, *listObjPtr;
    int objc, length, i;
    int result, count;
    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (hidden) {
        data.perm |= TCL_GLOB_PERM_HIDDEN;
    }
    result = READ_DIR_ERROR;
    if (Tcl_FSMatchInDirectory(interp, listObjPtr, objPtr, "*", &data)
        != TCL_OK) {
        goto error;                     /* Can't match directory. */
    }
    if (Tcl_ListObjGetElements(interp, listObjPtr, &objc, &objv)!= TCL_OK) {
        goto error;                     /* Can't split entry list. */
    }
    Tcl_GetStringFromObj(objPtr, &length);
    count = 0;
    for (i = 0; i < objc; i++) {
        Tcl_StatBuf stat;
        int numParts;
        Tcl_Obj *tailObjPtr, *partsObjPtr;
        Blt_TreeNode child;
        const char *label;
        
        if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
            goto error;                 /* Can't convert path. */
        }
        memset(&stat, 0, sizeof(Tcl_StatBuf));
        if (Tcl_FSStat(objv[i], &stat) < 0) {
            continue;                   /* Can't stat entry. */
        }
        /* Get the tail of the path. */
        partsObjPtr = Tcl_FSSplitPath(objv[i], &numParts);
        if ((partsObjPtr == NULL) || (numParts == 0)) {
            goto error;                 /* Can't split path. */
        }
        Tcl_IncrRefCount(partsObjPtr);
        Tcl_ListObjIndex(NULL, partsObjPtr, numParts - 1, &tailObjPtr);
        label = Tcl_GetString(tailObjPtr);
        if (label[0] == '.') {
            if (label[1] == '\0') {
                Tcl_DecrRefCount(partsObjPtr);
                continue;               /* Ignore . */
            }
            if ((label[1] == '.') && (label[2] == '\0')) {
                Tcl_DecrRefCount(partsObjPtr);
                continue;               /* Ignore .. */
            }
            /* Workaround bug in Tcl_FSSplitPath. Files that start with "~"
             * are prepended with "./" */
            if (label[1] == '/') {
                label += 2;
            }
        }
        child = Blt_Tree_CreateNode(cmdPtr->tree, parent, label, -1);
        Tcl_DecrRefCount(partsObjPtr);
        FillEntryData(interp, cmdPtr->tree, child, &stat, readPtr);

        /* Recursively read the subdirectory into the tree. */
        result = ReadDirectoryIntoTree(interp, cmdPtr, objv[i], child, readPtr);
        if (result == READ_DIR_ERROR) {
            goto error;                 /* Error while reading subdir. */
        }
        if ((result == READ_DIR_NOMATCH) &&
            ((readPtr->patternsObjPtr != NULL) || (readPtr->perm != 0) ||
             (readPtr->type != 0))) {
            DeleteNode(cmdPtr, child);
        } else {
            count++;
        }
    }
    Tcl_DecrRefCount(listObjPtr);
    return (count > 0) ? READ_DIR_MATCH : READ_DIR_NOMATCH;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return READ_DIR_ERROR;
}

static int
MatchEntries(Tcl_Interp *interp, TreeCmd *cmdPtr, Tcl_Obj *objPtr,
             Blt_TreeNode parent, ReadDirectory *readPtr)
{
    Tcl_Obj **objv, *listObjPtr, **patterns;
    int objc, i, numMatches, numPatterns;
    unsigned int patternFlags;
    Tcl_GlobTypeData data = {
        0, 0, NULL, NULL
    };

    numPatterns = 0;
    if (readPtr->patternsObjPtr != NULL) {
        if (Tcl_ListObjGetElements(interp, readPtr->patternsObjPtr, 
                &numPatterns, &patterns) != TCL_OK) {
            return READ_DIR_ERROR;           /* Can't split patterns. */
        }
    }
    patternFlags = 0;
#if (_TCL_VERSION > _VERSION(8,5,0)) 
    if (readPtr->flags & READ_DIR_NOCASE) {
        patternFlags =  TCL_MATCH_NOCASE;
    }
#endif
    data.perm = readPtr->perm;
    data.type = readPtr->type;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (Tcl_FSMatchInDirectory(interp, listObjPtr, objPtr, "*", &data)
        != TCL_OK) {
        goto error;                     /* Can't find match in directory. */
    }
     if (Tcl_ListObjGetElements(interp, listObjPtr, &objc, &objv) != TCL_OK) {
        goto error;                     /* Can't split entry list. */
    }
    numMatches = 0;                     /* Count # of matches. */
    for (i = 0; i < objc; i++) {
        Tcl_Obj *partsObjPtr, *tailObjPtr;
        Tcl_StatBuf stat;
        const char *label;
        int isMatch, numParts;
        
        if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
           goto error;                 /* Can't convert path. */
        }
        memset(&stat, 0, sizeof(Tcl_StatBuf));
        if (Tcl_FSStat(objv[i], &stat) < 0) {
            continue;                   /* Can't stat entry. */
        }
        /* Get the tail of the path. */
        partsObjPtr = Tcl_FSSplitPath(objv[i], &numParts);
        if ((partsObjPtr == NULL) || (numParts == 0)) {
            goto error;                 /* Can't split path. */
        }
        Tcl_IncrRefCount(partsObjPtr);
        Tcl_ListObjIndex(NULL, partsObjPtr, numParts - 1, &tailObjPtr);
        label = Tcl_GetString(tailObjPtr);

        /* Workaround bug in Tcl_FSSplitPath. Files that start with "~" are
         * prepended with "./" */
        if ((label[0] == '.') && (label[1] == '/')) {
            label += 2;
        }

        isMatch = TRUE;
        if (numPatterns > 0) {
            /* Match files or subdirectories against patterns. */
            int j;
            
            isMatch = FALSE;
            for (j = 0; j < numPatterns; j++) {
                const char *pattern;
                
                pattern = Tcl_GetString(patterns[j]);
                if (Tcl_StringCaseMatch(label, pattern, patternFlags)) {
                    isMatch = TRUE;
                    break;              /* Found a match. */
                }
            }
        }
        if (isMatch) {
            Blt_TreeNode child;
            
            numMatches++;
            child = Blt_Tree_FindChild(parent, label);
            if (child == NULL) {
                child = Blt_Tree_CreateNode(cmdPtr->tree, parent, label, -1);
                assert(child != NULL);
                FillEntryData(interp, cmdPtr->tree, child, &stat, readPtr);
            }
        }
        Tcl_DecrRefCount(partsObjPtr);
    }
    Tcl_DecrRefCount(listObjPtr);
    return (numMatches > 0) ? READ_DIR_MATCH : READ_DIR_NOMATCH;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return READ_DIR_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * ReadDirectoryIntoTree --
 *
 *      Loads contents of directory into the specified node, creating a new
 *      node for each entry.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadDirectoryIntoTree(Tcl_Interp *interp, TreeCmd *cmdPtr, Tcl_Obj *objPtr,
                      Blt_TreeNode node, ReadDirectory *readPtr)
{
    int numMatches;
    int result;
    
    /* Pass 1: Recurse downward, creating directory nodes */
    numMatches = 0;
    if (readPtr->flags & READ_DIR_RECURSE) {
        /* Tcl_FSMatchInDirectory can only match hidden or non-hidden
         * subdirectories, but not both at the same time.  This means we
         * have to make separate calls to MakeSubdirs, once for
         * non-hidden and again for hidden (that start with a ".")
         * subdirectories.  */
        result = MakeSubdirs(interp, cmdPtr, objPtr, node, readPtr, FALSE);
        if (result == READ_DIR_ERROR) {
            return READ_DIR_ERROR;
        }
        if (result == READ_DIR_MATCH) {
            numMatches++;
        }
        if ((readPtr->flags & READ_DIR_IGNORE_HIDDEN_DIRS) == 0) {
            result = MakeSubdirs(interp, cmdPtr, objPtr, node, readPtr, TRUE);
            if (result == READ_DIR_ERROR) {
                return READ_DIR_ERROR;
            }
            if (result == READ_DIR_MATCH) {
                numMatches++;
            }
        }
    }
    /* Pass 2:  Search directory for matching entries. */
    result = MatchEntries(interp, cmdPtr, objPtr, node, readPtr);
    if (result == READ_DIR_ERROR) {
        return READ_DIR_ERROR;
    }
    if (result == READ_DIR_MATCH) {
        numMatches++;
    }
    return (numMatches > 0) ? READ_DIR_MATCH : READ_DIR_NOMATCH;
}


static SortSwitches sortData;

static int
CompareNodes(Blt_TreeNode *n1Ptr, Blt_TreeNode *n2Ptr)
{
    TreeCmd *cmdPtr = sortData.cmdPtr;
    const char *s1, *s2;
    int result;
    Tcl_DString ds1, ds2;

    s1 = s2 = "";
    result = 0;

    if (sortData.flags & SORT_PATHNAME) {
        Tcl_DStringInit(&ds1);
        Tcl_DStringInit(&ds2);
    }
    if (sortData.key != NULL) {
        Tcl_Obj *valueObjPtr;

        if (Blt_Tree_GetValue((Tcl_Interp *)NULL, cmdPtr->tree, *n1Ptr, 
             sortData.key, &valueObjPtr) == TCL_OK) {
            s1 = Tcl_GetString(valueObjPtr);
        }
        if (Blt_Tree_GetValue((Tcl_Interp *)NULL, cmdPtr->tree, *n2Ptr, 
             sortData.key, &valueObjPtr) == TCL_OK) {
            s2 = Tcl_GetString(valueObjPtr);
        }
    } else if (sortData.flags & SORT_PATHNAME)  {
        Blt_TreeNode root;
        
        root = Blt_Tree_RootNode(cmdPtr->tree);
        Tcl_DStringInit(&ds1), Tcl_DStringInit(&ds2);
        s1 = Blt_Tree_NodeRelativePath(root, *n1Ptr, NULL, 0, &ds1);
        s2 = Blt_Tree_NodeRelativePath(root, *n2Ptr, NULL, 0, &ds2);
    } else {
        s1 = Blt_Tree_NodeLabel(*n1Ptr);
        s2 = Blt_Tree_NodeLabel(*n2Ptr);
    }
    switch (sortData.type) {
    case SORT_ASCII:
        result = strcmp(s1, s2);
        break;

    case SORT_COMMAND:
        if (sortData.command == NULL) {
            result = Blt_DictionaryCompare(s1, s2);
        } else {
            Blt_ObjectName objName;
            Tcl_DString dsCmd, dsName;
            const char *qualName;

            result = 0; /* Hopefully this will be okay even if the TCL command
                         * fails to return the correct result. */
            Tcl_DStringInit(&dsCmd);
            Tcl_DStringAppend(&dsCmd, sortData.command, -1);
            Tcl_DStringInit(&dsName);
            objName.name = Tcl_GetCommandName(cmdPtr->interp, cmdPtr->cmdToken);
            objName.nsPtr = Blt_GetCommandNamespace(cmdPtr->cmdToken);
            qualName = Blt_MakeQualifiedName(&objName, &dsName);
            Tcl_DStringAppendElement(&dsCmd, qualName);
            Tcl_DStringFree(&dsName);
            Tcl_DStringAppendElement(&dsCmd, Blt_Tree_NodeIdAscii(*n1Ptr));
            Tcl_DStringAppendElement(&dsCmd, Blt_Tree_NodeIdAscii(*n2Ptr));
            Tcl_DStringAppendElement(&dsCmd, s1);
            Tcl_DStringAppendElement(&dsCmd, s2);
            result = Tcl_GlobalEval(cmdPtr->interp, Tcl_DStringValue(&dsCmd));
            Tcl_DStringFree(&dsCmd);
            
            if ((result != TCL_OK) ||
                (Tcl_GetInt(cmdPtr->interp, 
                    Tcl_GetStringResult(cmdPtr->interp), &result) != TCL_OK)) {
                Tcl_BackgroundError(cmdPtr->interp);
            }
            Tcl_ResetResult(cmdPtr->interp);
        }
        break;

    case SORT_DICTIONARY:
        result = Blt_DictionaryCompare(s1, s2);
        break;

    case SORT_INTEGER:
        {
            int i1, i2;

            if (Tcl_GetInt(NULL, s1, &i1) == TCL_OK) {
                if (Tcl_GetInt(NULL, s2, &i2) == TCL_OK) {
                    result = i1 - i2;
                } else {
                    result = -1;
                } 
            } else if (Tcl_GetInt(NULL, s2, &i2) == TCL_OK) {
                result = 1;
            } else {
                result = Blt_DictionaryCompare(s1, s2);
            }
        }
        break;

    case SORT_REAL:
        {
            double r1, r2;

            if (Tcl_GetDouble(NULL, s1, &r1) == TCL_OK) {
                if (Tcl_GetDouble(NULL, s2, &r2) == TCL_OK) {
                    result = (r1 < r2) ? -1 : (r1 > r2) ? 1 : 0;
                } else {
                    result = -1;
                } 
            } else if (Tcl_GetDouble(NULL, s2, &r2) == TCL_OK) {
                result = 1;
            } else {
                result = Blt_DictionaryCompare(s1, s2);
            }
        }
        break;
    }
    if (result == 0) {
        result = Blt_Tree_NodeId(*n1Ptr) - Blt_Tree_NodeId(*n2Ptr);
    }
    if (sortData.flags & SORT_DECREASING) {
        result = -result;
    } 
    if (sortData.flags & SORT_PATHNAME) {
        Tcl_DStringFree(&ds1);
        Tcl_DStringFree(&ds2);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortApplyProc --
 *
 *      Sorts the subnodes at a given node.
 *
 * Results:
 *      Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortApplyProc(
    Blt_TreeNode node,
    ClientData clientData,
    int order)                  /* Not used. */
{
    TreeCmd *cmdPtr = clientData;

    if (!Blt_Tree_IsLeaf(node)) {
        Blt_Tree_SortNode(cmdPtr->tree, node, CompareNodes);
    }
    return TCL_OK;
}

static int
ComparePositions(Blt_TreeNode *n1Ptr, Blt_TreeNode *n2Ptr)
{
    if (*n1Ptr == *n2Ptr) {
        return 0;
    }
    if (Blt_Tree_IsBefore(*n1Ptr, *n2Ptr)) {
        return -1;
    }
    return 1;
}


/*
 *---------------------------------------------------------------------------
 *
 * ParseDumpFileHeader --
 *
 *      Reads the next full record from the given channel, returning the
 *      record as a list. Blank lines and comments are ignored.
 *
 *---------------------------------------------------------------------------
 */
static int
ParseDumpFileHeader(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    if (restorePtr->channel != NULL) {
        Tcl_DString ds;
        const char *string;
        double version;
        int numChars;

        /* Get first line. */
        Tcl_DStringInit(&ds);
        numChars = Tcl_Gets(restorePtr->channel, &ds);
        if (numChars < 0) {
            if (Tcl_Eof(restorePtr->channel)) {
                return TCL_OK;
            }
            return TCL_ERROR;
        }
        string = Tcl_DStringValue(&ds);
        if (strncmp(string, "# V", 3) != 0) {
            Tcl_Seek(restorePtr->channel, 0, SEEK_SET);
            Tcl_DStringFree(&ds);
            return TCL_OK;
        }
        if (Tcl_GetDouble(interp, string + 3, &version) != TCL_OK) {
            Tcl_DStringFree(&ds);
            return TCL_ERROR;
        }
        Tcl_DStringFree(&ds);
        restorePtr->version = version;
    } else {
        const char *p;
        const char *string;
        double version;

        string = restorePtr->nextLine;
        if (strncmp(string, "# V", 3) != 0) {
            return TCL_OK;
        }
        if (Tcl_GetDouble(interp, string + 3, &version) != TCL_OK) {
            return TCL_ERROR;
        }
        for (p = string + 3; *p != '\0' ; p++) {
            if (*p == '\n') {
                break;
            }
        }
        restorePtr->nextLine = p;
        restorePtr->version = version;
    }
    return TCL_OK;
}

static int
ReadNextRecord(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    int result;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    /* Get first line, ignoring blank lines and comments. */
    for (;;) {
        char *cp;
        int numChars;

        Tcl_DStringSetLength(&ds, 0);
        numChars = Tcl_Gets(restorePtr->channel, &ds);
        if (numChars < 0) {
            if (Tcl_Eof(restorePtr->channel)) {
                Tcl_DStringFree(&ds);
                return TCL_RETURN;
            }
            return TCL_ERROR;
        }
        restorePtr->numLines++;
        for (cp = Tcl_DStringValue(&ds); *cp != '\0'; cp++) {
            if (!isspace(UCHAR(*cp))) {
                break;
            }
        }
        if ((*cp != '\0') && (*cp != '#')) {
            break;                      /* Not a comment or blank line. */
        }
    }

    Tcl_DStringAppend(&ds, "\n", 1);
    while (!Tcl_CommandComplete(Tcl_DStringValue(&ds))) {
        int numChars;

        if (Tcl_Eof(restorePtr->channel)) {
            Tcl_AppendResult(interp, "unexpected EOF: short record.", 
                             (char *)NULL);
            Tcl_DStringFree(&ds);
            return TCL_ERROR;           /* Found EOF (incomplete entry) or
                                         * error. */
        }
        /* Process additional lines if needed */
        numChars = Tcl_Gets(restorePtr->channel, &ds);
        if (numChars < 0) {
            Tcl_AppendResult(interp, "read error: ", 
                             Tcl_PosixError(interp), (char *)NULL);
            Tcl_DStringFree(&ds);
            return TCL_ERROR;           /* Found EOF (incomplete entry) or
                                         * error. */
        }
        restorePtr->numLines++;
        Tcl_DStringAppend(&ds, "\n", 1);
    }
    result = Tcl_SplitList(interp, Tcl_DStringValue(&ds), &restorePtr->argc, 
                   &restorePtr->argv);
    Tcl_DStringFree(&ds);
    return result;
}

static int
GetNextRecord(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    char *entry, *eol;
    char saved;
    int result;

    entry = (char *)restorePtr->nextLine;
    /* Get first line, ignoring blank lines and comments. */
    for (;;) {
        char *first;

        first = NULL;
        restorePtr->numLines++;
        /* Find the end of the first line. */
        for (eol = entry; (*eol != '\n') && (*eol != '\0'); eol++) {
            if ((first == NULL) && (!isspace(UCHAR(*eol)))) {
                first = eol;            /* Track the first non-whitespace
                                         * character. */
            }
        }
        if (first == NULL) {
            if (*eol == '\0') {
                return TCL_RETURN;
            }
        } else if (*first != '#') {
            break;                      /* Not a comment or blank line. */
        }
        entry = eol + 1;
    }
    saved = *eol;
    *eol = '\0';
    while (!Tcl_CommandComplete(entry)) {
        *eol = saved;
        if (*eol == '\0') {
            Tcl_AppendResult(interp, "incomplete dump record: \"", entry, 
                "\"", (char *)NULL);
            return TCL_ERROR;           /* Found EOF (incomplete entry) or
                                         * error. */
        }
        /* Get the next line. */
        for (eol = eol + 1; (*eol != '\n') && (*eol != '\0'); eol++) {
            /*empty*/
        }
        restorePtr->numLines++;
        saved = *eol;
        *eol = '\0';
    }
    if (entry == eol) {
        return TCL_RETURN;
    }
    result = Tcl_SplitList(interp, entry, &restorePtr->argc, &restorePtr->argv);
    *eol = saved;
    restorePtr->nextLine = eol + 1;
    return result;
}

static Tcl_Obj *
GetStringObj(RestoreInfo *restorePtr, const char *string, int length)
{
    Blt_HashEntry *hPtr;
    int isNew;

    hPtr = Blt_CreateHashEntry(&restorePtr->dataTable, string, &isNew);
    if (isNew) {
        Tcl_Obj *objPtr;

        if (length == -1) {
            length = strlen(string);
        }
        objPtr = Tcl_NewStringObj(string, length);
        Blt_SetHashValue(hPtr, objPtr);
        return objPtr;
    }
    return Blt_GetHashValue(hPtr);
}

static int
RestoreValues(RestoreInfo *restorePtr, Tcl_Interp *interp, Blt_TreeNode node, 
              int numValues, const char **values)
{
    int i;

    for (i = 0; i < numValues; i += 2) {
        Tcl_Obj *valueObjPtr;
        int result;
        Blt_TreeKey key;

        if ((i + 1) < numValues) {
            valueObjPtr = GetStringObj(restorePtr, values[i + 1], -1);
        } else {
            valueObjPtr = Tcl_NewStringObj("", -1);
        }
        key = Blt_Tree_GetKey(restorePtr->tree, values[i]);
        /* Increment/decrement tje generated/shared valueObj in case it's the
         * current value. */
        Tcl_IncrRefCount(valueObjPtr);
        result = Blt_Tree_SetValueByKey(interp, restorePtr->tree, node, key, 
                valueObjPtr);
        Tcl_DecrRefCount(valueObjPtr);
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int 
RestoreTags(Blt_Tree tree, Blt_TreeNode node, int numTags, const char **tags) 
{
    int i;

    for (i = 0; i < numTags; i++) {
        Blt_Tree_AddTag(tree, node, tags[i]);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreNode5 --
 *
 *      Parses and creates a node based upon the first 3 fields of a five
 *      field entry.  This is the new restore file format.
 *
 *         parentId nodeId pathList dataList tagList 
 *
 *      The purpose is to attempt to save and restore the node ids embedded in
 *      the restore file information.  The old format could not distinquish
 *      between two sibling nodes with the same label unless they were both
 *      leaves.  I'm trying to avoid dependencies upon labels.
 *
 *      If you're starting from an empty tree, this obviously should work
 *      without a hitch.  We only need to map the file's root id to 0.  It's a
 *      little more complicated when adding node to an already full tree.
 *
 *      First see if the node id isn't already in use.  Otherwise, map the
 *      node id (via a hashtable) to the real node. We'll need it later when
 *      subsequent entries refer to their parent id.
 *
 *      If a parent id is unknown (the restore file may be out of order), then
 *      follow plan B and use its path.
 *      
 *---------------------------------------------------------------------------
 */
static int
RestoreNode5(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    Blt_Tree tree;
    Blt_HashEntry *hPtr;
    Blt_TreeNode node, parent;
    int isNew;
    long pid, id;
    const char **tags, **values, **names;
    int numTags, numValues, numNames;

    tree = restorePtr->tree;

    /* 
     * The second and first fields respectively are the ids of the node and
     * its parent.  The parent id of the root node is always -1.
     */
    if ((Blt_GetLong(interp, restorePtr->argv[0], &pid) != TCL_OK) ||
        (Blt_GetLong(interp, restorePtr->argv[1], &id) != TCL_OK)) {
        return TCL_ERROR;
    }
    names = values = tags = NULL;
    node = NULL;

    /* 
     * The third, fourth, and fifth fields respectively are the list of
     * component names representing the path to the node including the name of
     * the node, a key-value list of data values, and a list of tag names.
     */     

    if ((Tcl_SplitList(interp, restorePtr->argv[2], &numNames, &names)!= TCL_OK) ||
        (Tcl_SplitList(interp, restorePtr->argv[3], &numValues, &values) != TCL_OK) || 
        (Tcl_SplitList(interp, restorePtr->argv[4], &numTags,&tags) != TCL_OK)) {
        goto error;
    }    

    /* Get the parent of the node. */

    if (pid == -1) {                    /* Map -1 id to the root node of the
                                         * subtree. */
        node = restorePtr->root;
        hPtr = Blt_CreateHashEntry(&restorePtr->idTable, (char *)id, &isNew);
        Blt_SetHashValue(hPtr, node);
        Blt_Tree_RelabelNode(tree, node, names[0]);
    } else {

        /* 
         * Check if the parent has been mapped to another id in the tree.
         * This can happen when there's a id collision with an existing node.
         */

        hPtr = Blt_FindHashEntry(&restorePtr->idTable, (char *)pid);
        if (hPtr != NULL) {
            parent = Blt_GetHashValue(hPtr);
        } else {
            parent = Blt_Tree_GetNodeFromIndex(tree, pid);
            if (parent == NULL) {
                /* 
                 * Normally the parent node should already exist in the tree,
                 * but in a partial restore it might not.  "Plan B" is to use
                 * the list of path components to create the missing
                 * components, including the parent.
                 */
                if (numNames == 0) {
                    parent = restorePtr->root;
                } else {
                    int i;

                    for (i = 1; i < (numNames - 2); i++) {
                        node = Blt_Tree_FindChild(parent, names[i]);
                        if (node == NULL) {
                            node = Blt_Tree_CreateNode(tree, parent, 
                                names[i], -1);
                        }
                        parent = node;
                    }
                    /* 
                     * If there's a node with the same label as the parent,
                     * we'll use that node. Otherwise, try to create a new
                     * node with the desired parent id.
                     */
                    node = Blt_Tree_FindChild(parent, names[numNames-2]);
                    if (node == NULL) {
                        node = Blt_Tree_CreateNodeWithId(tree, parent,
                                names[numNames - 2], pid, -1);
                        if (node == NULL) {
                            goto error;
                        }
                    }
                    parent = node;
                }
            }
        } 

        /* 
         * It's an error if the desired id has already been remapped.  That
         * means there were two nodes in the dump with the same id.
         */
        hPtr = Blt_FindHashEntry(&restorePtr->idTable, (char *)id);
        if (hPtr != NULL) {
            Tcl_AppendResult(interp, "node \"", Blt_Ltoa(id), 
                "\" has already been restored", (char *)NULL);
            goto error;
        }


        if (restorePtr->flags & TREE_RESTORE_OVERWRITE) {
            /* Can you find the child by name. */
            node = Blt_Tree_FindChild(parent, names[numNames - 1]);
            if (node != NULL) {
                hPtr = Blt_CreateHashEntry(&restorePtr->idTable, (char *)id,
                        &isNew);
                Blt_SetHashValue(hPtr, node);
            }
        }

        if (node == NULL) {
            node = Blt_Tree_GetNodeFromIndex(tree, id);
            if (node == NULL) {
                node = Blt_Tree_CreateNodeWithId(tree, parent, 
                        names[numNames - 1], id, -1);
            } else {
                node = Blt_Tree_CreateNode(tree, parent, 
                        names[numNames - 1], -1);
                hPtr = Blt_CreateHashEntry(&restorePtr->idTable, (char *)id,
                        &isNew);
                Blt_SetHashValue(hPtr, node);
            }
        }
    } 
        
    if (node == NULL) {
        goto error;                     /* Couldn't create node with requested
                                         * id. */
    }
    Tcl_Free((char *)names);
    names = NULL;

    /* Values */
    if (RestoreValues(restorePtr, interp, node, numValues, values) != TCL_OK) {
        goto error;
    }
    Tcl_Free((char *)values);
    values = NULL;

    /* Tags */
    if (!(restorePtr->flags & TREE_RESTORE_NO_TAGS)) {
        RestoreTags(tree, node, numTags, tags);
    }
    Tcl_Free((char *)tags);
    tags = NULL;
    return TCL_OK;

 error:
    if (tags != NULL) {
        Tcl_Free((char *)tags);
    }
    if (values != NULL) {
        Tcl_Free((char *)values);
    }
    if (names != NULL) {
        Tcl_Free((char *)names);
    }
    if (node != NULL) {
        Blt_Tree_DeleteNode(tree, node);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreNode3 --
 *
 *      Parses and creates a node based upon the first field of a three field
 *      entry.  This is the old restore file format.
 *
 *              pathList dataList tagList
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RestoreNode3(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    Blt_Tree tree;
    Blt_TreeNode node, parent;
    int i;
    const char **names, **values, **tags;
    int numNames, numValues, numTags;

    /* The first field is a list of component names representing the path to
     * the node, including the name of the node. */

    if (Tcl_SplitList(interp, restorePtr->argv[0], &numNames, &names) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    node = parent = restorePtr->root;
    tree = restorePtr->tree;
    /* Automatically create ancestor nodes as needed. */
    for (i = 0; i < (numNames - 1); i++) {
        node = Blt_Tree_FindChild(parent, names[i]);
        if (node == NULL) {
            node = Blt_Tree_CreateNode(tree, parent, names[i], -1);
        }
        parent = node;
    }
    if (numNames > 0) {

        /* 
         * By default duplicate nodes (two sibling nodes with the same label)
         * unless the -overwrite switch was given.
         */

        node = NULL;
        if (restorePtr->flags & TREE_RESTORE_OVERWRITE) {
            node = Blt_Tree_FindChild(parent, names[i]);
        }
        if (node == NULL) {
            node = Blt_Tree_CreateNode(tree, parent, names[i], -1);
        }
    }
    Tcl_Free((char *)names);

    /* The second field is a key-value list of the node's values. */

    if (Tcl_SplitList(interp, restorePtr->argv[1], &numValues, &values) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (RestoreValues(restorePtr, interp, node, numValues, values)!=TCL_OK) {
        goto error;
    }
    Tcl_Free((char *)values);

    /* The third field is a list of tags. */

    if (!(restorePtr->flags & TREE_RESTORE_NO_TAGS)) {
        /* Parse the tag list. */
        if (Tcl_SplitList(interp, restorePtr->argv[2], &numTags, &tags) 
            != TCL_OK) {
            goto error;
        }
        RestoreTags(tree, node, numTags, tags);
        Tcl_Free((char *)tags);
    }
    return TCL_OK;

 error:
    Tcl_Free((char *)restorePtr->argv);
    restorePtr->argv = NULL;
    Blt_Tree_DeleteNode(tree, node);
    return TCL_ERROR;
}

static int
RestoreTreeV2(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    int result;

    for (;;) {
        if (restorePtr->channel != NULL) {
            result = ReadNextRecord(interp, restorePtr);
        } else {
            result = GetNextRecord(interp, restorePtr);
        }
        if (result != TCL_OK) {
            break;                      /* Found error or EOF */
        }
        if (restorePtr->argc == 0) {
            result = TCL_OK;            /* Do nothing. */
        } else if (restorePtr->argc == 3) {
            result = RestoreNode3(interp, restorePtr);
        } else if ((restorePtr->argc == 5) || (restorePtr->argc == 6)) {
            result = RestoreNode5(interp, restorePtr);
        } else {
            Tcl_AppendResult(interp, "line #", Blt_Itoa(restorePtr->numLines), 
                ": wrong # elements in restore entry", (char *)NULL);
            result = TCL_ERROR;
        }
        Tcl_Free((char *)restorePtr->argv);
        restorePtr->argv = NULL;
        if (result != TCL_OK) {
            break;
        }
    } 
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreNodeCmd --
 *
 *      Restores a node to the current tree. The format of the command is
 *      
 *              n label pid id
 *
 *      where "label" is the new node label, "pid" is the node id of the 
 *      parent node, and "id" is the requested id for the node.
 *
 *      The node ids from the dump file and the current tree may collide.
 *      We use a hash table to map the old node ids to new ones in the
 *      tree.  It is an error the dump file contains duplicate node ids
 *      or if there are multiple root nodes (parent id is -1).
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      New nodes are created in the tree and may possibly generate notify
 *      callbacks.
 *
 *---------------------------------------------------------------------------
 */
static int
RestoreNodeCmd(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    Blt_TreeNode node;
    long pid, id;

    if (restorePtr->argc != 4) {
        const char *cmdString;

        cmdString = Tcl_Concat(restorePtr->argc, restorePtr->argv);
        Tcl_AppendResult(interp, "line ", Blt_Itoa(restorePtr->numLines), ": ",
                "wrong # args in restore node command: \"", cmdString, 
                "\" should be \"n label pid id\"", (char *)NULL);
        Tcl_Free((char *)cmdString);
        return TCL_ERROR;
    }
    if ((Blt_GetLong(interp, restorePtr->argv[2], &pid) != TCL_OK) ||
        (Blt_GetLong(interp, restorePtr->argv[3], &id) != TCL_OK)) {
        return TCL_ERROR;
    }
    if (pid == -1) {
        Blt_HashEntry *hPtr;
        int isNew;

        node = restorePtr->root;
        hPtr = Blt_CreateHashEntry(&restorePtr->idTable, (char *)id, &isNew);
        if (!isNew) {
            Tcl_AppendResult(interp, "Found more than root node in tree dump.", 
                        (char *)NULL);
            return TCL_ERROR;
        }
        Blt_SetHashValue(hPtr, node);
        Blt_Tree_RelabelNode(restorePtr->tree, node, restorePtr->argv[1]);
    } else {
        Blt_HashEntry *hPtr;
        Blt_TreeNode parent;
        int isNew;

        /* 
         * Check that we can find the parent node in the node id table.
         * We can't process nodes out of order.
         */
        hPtr = Blt_FindHashEntry(&restorePtr->idTable, (char *)pid);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "Can't find parent node \"", 
                  restorePtr->argv[2],  "\" in tree.",  (char *)NULL);
            return TCL_ERROR;
        }
        parent = Blt_GetHashValue(hPtr);
        hPtr = Blt_CreateHashEntry(&restorePtr->idTable, (char *)id, &isNew);
        if (!isNew) {
            Tcl_AppendResult(interp, "Duplicate id \"",  Blt_Ltoa(id), 
                             "\" in tree dump.", (char *)NULL);
            return TCL_ERROR;
        }
        /* Try to restore the node to its previous id. */
        node = Blt_Tree_GetNodeFromIndex(restorePtr->tree, id);
        if ((node == NULL) || (restorePtr->flags & TREE_RESTORE_OVERWRITE)) {
            node = Blt_Tree_CreateNodeWithId(restorePtr->tree, parent, 
                restorePtr->argv[1], id, -1);
        } else {
            node = Blt_Tree_CreateNode(restorePtr->tree, parent, 
                restorePtr->argv[1], -1);
        }
        assert(node != NULL);
        if (node == NULL) {
            Tcl_AppendResult(interp, "Can't create node \"", 
                restorePtr->argv[1], "\" in tree.", (char *)NULL);
            return TCL_ERROR;
        }
        Blt_SetHashValue(hPtr, node);   /* Save the mapping.  */
    }
    restorePtr->node = node;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreTagCmd --
 *
 *      Restores a tag to the current node. The format of the command is
 *      
 *              t tagName
 *
 *      where "tagName" is the tag for the current node. The current node
 *      is the last node processed by RestoreNodeCmd.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      New tags are added in the tree.
 *
 *      t tag
 *---------------------------------------------------------------------------
 */
static int
RestoreTagCmd(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    if (restorePtr->argc != 2) {
        const char *cmdString;

        cmdString = Tcl_Concat(restorePtr->argc, restorePtr->argv);
        Tcl_AppendResult(interp, "line ", Blt_Itoa(restorePtr->numLines), ": ",
                "wrong # args in restore tag command: \"", cmdString, 
                "\" should be \"t tag\"", (char *)NULL);
        Tcl_Free((char *)cmdString);
        return TCL_ERROR;
    }
    Blt_Tree_AddTag(restorePtr->tree, restorePtr->node, restorePtr->argv[1]);
    return TCL_OK;
}

typedef struct _TclList {
    int refCount;
    int maxElemCount;		/* Total number of element array slots. */
    int elemCount;		/* Current number of list elements. */
    int canonicalFlag;		/* Set if the string representation was
				 * derived from the list representation. May
				 * be ignored if there is no string rep at
				 * all.*/
    Tcl_Obj *elements;		/* First list element; the struct is grown to
				 * accomodate all elements. */
} TclList;

/*
 *---------------------------------------------------------------------------
 *
 * RestoreDataCmd --
 *
 *      Restores a data value to the current node. The format of the
 *      command is
 *      
 *              d fieldName value
 *
 *      where "fieldName" is the name for the data field to be set in the
 *      current node. The current node is the last node processed by
 *      RestoreNodeCmd.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      New data fields are created.
 *
 *      d name value
 *---------------------------------------------------------------------------
 */
static int
RestoreDataCmd(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    Tcl_Obj *valueObjPtr;
    int result;
    Blt_TreeKey key;
    int count;

    if (restorePtr->argc != 3) {
        const char *cmdString;

        cmdString = Tcl_Concat(restorePtr->argc, restorePtr->argv);
        Tcl_AppendResult(interp, "line ", Blt_Itoa(restorePtr->numLines), ": ",
                "wrong # args in restore data command: \"", cmdString, 
                "\" should be \"d name value\"", (char *)NULL);
        Tcl_Free((char *)cmdString);
        return TCL_ERROR;
    }
    key = Blt_Tree_GetKey(restorePtr->tree, restorePtr->argv[1]);
    count = 2;
    while (Blt_Tree_ValueExistsByKey(restorePtr->tree, restorePtr->node, key)) {
        char string[200];
        
        sprintf(string, "%s#%d", restorePtr->argv[1], count);
        count++;
        key = Blt_Tree_GetKey(restorePtr->tree, string);
    }
    valueObjPtr = GetStringObj(restorePtr, restorePtr->argv[2], -1);
    result = Blt_Tree_SetValueByKey(interp, restorePtr->tree, restorePtr->node, 
        key, valueObjPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreAppendToListCmd --
 *
 *      Restores an element a list of data values to the current node. The
 *      format of the command is
 *      
 *              a fieldName value
 *
 *      where "fieldName" is the name for the data field to be set in the
 *      current node. "value" is the new element to be appended to the list
 *      of values for the current node. The current node is the last node
 *      processed by RestoreNodeCmd.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      New data fields are possibly created and elements are appended to
 *      the tree.
 *
 *---------------------------------------------------------------------------
 */
static int
RestoreAppendToListCmd(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    Blt_TreeKey key;
    Tcl_Obj *valueObjPtr;
    
    if (restorePtr->argc != 3) {
        const char *cmdString;

        cmdString = Tcl_Concat(restorePtr->argc, restorePtr->argv);
        Tcl_AppendResult(interp, "line ", Blt_Itoa(restorePtr->numLines), ": ",
                "wrong # args in restore append command: \"", cmdString, 
                "\" should be \"a name value\"", (char *)NULL);
        Tcl_Free((char *)cmdString);
        return TCL_ERROR;
    }
    key = Blt_Tree_GetKey(restorePtr->tree, restorePtr->argv[1]);
    valueObjPtr = GetStringObj(restorePtr, restorePtr->argv[2], -1);
    return Blt_Tree_ListAppendValueByKey(interp, restorePtr->tree, 
        restorePtr->node, key, valueObjPtr);
}

static int
RestoreTreeV3(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    int result;

    for (;;) {
        char c;

        if (restorePtr->channel == NULL) {
            result = GetNextRecord(interp, restorePtr);
        } else {
            result = ReadNextRecord(interp, restorePtr);
        }
        if (result != TCL_OK) {
            break;                      /* Found error or EOF */
        }
        result = TCL_ERROR;
        c = restorePtr->argv[0][0];
        if  (restorePtr->argv[0][1] == '\0') {
            if (c == 'n') {
                result = RestoreNodeCmd(interp, restorePtr);
            } else if (c == 'd') {
                result = RestoreDataCmd(interp, restorePtr);
            } else if (c == 'a') {
                result = RestoreAppendToListCmd(interp, restorePtr);
            } else if (c == 't') {
                if (restorePtr->flags & RESTORE_NO_TAGS) {
                    result = TCL_OK;
                } else {
                    result = RestoreTagCmd(interp, restorePtr);
                }
            } else {
                Tcl_AppendResult(interp, "line #", 
                             Blt_Itoa(restorePtr->numLines), 
                             ": unknown key \"", restorePtr->argv[0], "\"",
                             (char *)NULL);
            }
        } 
        Tcl_Free((char *)restorePtr->argv);
        restorePtr->argv = NULL;
        if (result != TCL_OK) {
            break;
        }
    } 
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreTreeFromFile --
 *
 *      Restores nodes to the given tree based upon the dump file
 *      provided. The dump file should have been generated by
 *      DumpTree.  If the file name starts with an '@', then it is
 *      the name of an already opened channel to be used. Nodes are added
 *      relative to the node *root* as the root of the sub-tree.  Two bit
 *      flags may be set.
 *      
 *      TREE_RESTORE_NO_TAGS    Don't restore tag information.
 *      TREE_RESTORE_OVERWRITE  Look for nodes with the same label.
 *                              Overwrite if necessary.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      New nodes are created in the tree and may possibly generate notify
 *      callbacks.
 *
 *---------------------------------------------------------------------------
 */
static int
RestoreTreeFromFile(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    Tcl_Channel channel;
    int closeChannel;
    int result;
    const char *fileName;

    fileName = Tcl_GetString(restorePtr->fileObjPtr);
    closeChannel = TRUE;
    if ((fileName[0] == '@') && (fileName[1] != '\0')) {
        int mode;
        
        channel = Tcl_GetChannel(interp, fileName+1, &mode);
        if (channel == NULL) {
            return TCL_ERROR;
        }
        if ((mode & TCL_READABLE) == 0) {
            Tcl_AppendResult(interp, "channel \"", fileName, 
                "\" not opened for reading", (char *)NULL);
            return TCL_ERROR;
        }
        closeChannel = FALSE;
    } else {
        channel = Tcl_OpenFileChannel(interp, fileName, "r", 0);
        if (channel == NULL) {
            return TCL_ERROR;           /* Can't open dump file. */
        }
    }
    restorePtr->channel = channel;
    result = ParseDumpFileHeader(interp, restorePtr);
    if (restorePtr->version > 2.9) {
        result = RestoreTreeV3(interp, restorePtr);
    } else {
        result = RestoreTreeV2(interp, restorePtr);
    }
    if (closeChannel) {
        Tcl_Close(interp, channel);
    }
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreTreeFromData --
 *
 *      Restores nodes to the given tree based upon the dump string.  The
 *      dump string should have been generated by DumpTree.  Nodes are
 *      added relative to the node *root* as the root of the sub-tree.  Two
 *      bit flags may be set.
 *      
 *      TREE_RESTORE_NO_TAGS    Don't restore tag information.
 *      TREE_RESTORE_OVERWRITE  Look for nodes with the same label.
 *                              Overwrite if necessary.
 *
 * Results:
 *      A standard TCL result.  If the restore was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      New nodes are created in the tree and may possibly generate
 *      notify callbacks.
 *
 *---------------------------------------------------------------------------
 */
static int
RestoreTreeFromData(Tcl_Interp *interp, RestoreInfo *restorePtr)
{
    int result;
    int length;

    restorePtr->nextLine = Tcl_GetStringFromObj(restorePtr->dataObjPtr,
                                                &length);
    if (length > 4) {
        result = ParseDumpFileHeader(interp, restorePtr);
    }
    if (restorePtr->version > 2.9) {
        result = RestoreTreeV3(interp, restorePtr);
    } else {
        result = RestoreTreeV2(interp, restorePtr);
    }
    /* result will be TCL_RETURN if successful, TCL_ERROR otherwise. */
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
WriteDumpRecord(DumpInfo *dumpPtr, Tcl_DString *dataPtr)
{
    const char *string;
    int length;

    string = Tcl_DStringValue(dataPtr);
    length = Tcl_DStringLength(dataPtr);
    if (dumpPtr->channel == NULL) {
        Tcl_DStringAppend(&dumpPtr->ds, string, length);
        Tcl_DStringAppend(&dumpPtr->ds, "\n", 1);
    } else {
        ssize_t numWritten;

#if HAVE_UTF
        numWritten = Tcl_WriteChars(dumpPtr->channel, string, length);
        numWritten = Tcl_WriteChars(dumpPtr->channel, "\n", 1);
#else
        numWritten = Tcl_Write(dumpPtr->channel, string, length);
        numWritten = Tcl_Write(dumpPtr->channel, "\n", 1);
#endif
        if (numWritten < 0) {
            return TCL_ERROR;
        }
    }
    Tcl_DStringSetLength(dataPtr, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpNodeV2 --
 *
 *---------------------------------------------------------------------------
 */
static int
DumpNodeV2(DumpInfo *dumpPtr, Blt_TreeNode node)
{
    Blt_TreeKey key;
    Blt_TreeKeyIterator iter;
    Tcl_DString ds;
    int result;
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;


    Tcl_DStringInit(&ds);
    if (node == dumpPtr->root) {
        Tcl_DStringAppendElement(&ds, "-1");
    } else {
        Tcl_DStringAppendElement(&ds, 
              Blt_Tree_NodeIdAscii(Blt_Tree_ParentNode(node)));
    }   
    Tcl_DStringAppendElement(&ds, Blt_Tree_NodeIdAscii(node));

    Tcl_DStringStartSublist(&ds);
    Blt_Tree_NodeRelativePath(dumpPtr->root, node, NULL, TREE_INCLUDE_ROOT, &ds);
    Tcl_DStringEndSublist(&ds);

    Tcl_DStringStartSublist(&ds);
    /* Add list of data fields. key-value pairs. */
    for (key = Blt_Tree_FirstKey(dumpPtr->tree, node, &iter); key != NULL; 
         key = Blt_Tree_NextKey(dumpPtr->tree, &iter)) {
        Tcl_Obj *objPtr;
        
        if (Blt_Tree_GetValueByKey((Tcl_Interp *)NULL, dumpPtr->tree, node, 
                                   key, &objPtr) == TCL_OK) {
            Tcl_DStringAppendElement(&ds, key);
            Tcl_DStringAppendElement(&ds, Tcl_GetString(objPtr));
        }
    }           
    Tcl_DStringEndSublist(&ds);
    if ((dumpPtr->flags & DUMP_NO_TAGS) == 0) {
        Tcl_DStringStartSublist(&ds);
        /* Add list of tags. */
        for (hPtr = Blt_Tree_FirstTag(dumpPtr->tree, &cursor); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&cursor)) {
            Blt_TreeTagEntry *tePtr;
            
            tePtr = Blt_GetHashValue(hPtr);
            if (Blt_FindHashEntry(&tePtr->nodeTable, (char *)node) != NULL) {
                Tcl_DStringAppendElement(&ds, tePtr->tagName);
            }
        }
        Tcl_DStringEndSublist(&ds);
    }
    result = WriteDumpRecord(dumpPtr, &ds);
    Tcl_DStringFree(&ds);
    return result;
}

static int
DumpListValues(DumpInfo *dumpPtr, Tcl_Obj *valueObjPtr, const char *key)
{
    TclList *listPtr;
    int i;
    Tcl_DString ds;
    Tcl_Obj **objv;

    listPtr = (TclList *)valueObjPtr->internalRep.twoPtrValue.ptr1;
    objv = &listPtr->elements;
    Tcl_DStringInit(&ds);
    for (i = 0; i < listPtr->elemCount; i++) {
        Tcl_DStringAppendElement(&ds, "a");
        Tcl_DStringAppendElement(&ds, key);
        Tcl_DStringAppendElement(&ds, Tcl_GetString(objv[i]));
        if (WriteDumpRecord(dumpPtr, &ds) != TCL_OK) {
            goto error;
        }
        Tcl_DStringSetLength(&ds, 0);
    }
    Tcl_DStringFree(&ds);
    return TCL_OK;
 error:
    Tcl_DStringFree(&ds);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpNodeV3 --
 *
 *---------------------------------------------------------------------------
 */
static int
DumpNodeV3(DumpInfo *dumpPtr, Blt_TreeNode node)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch cursor;
    Blt_TreeKey key;
    Blt_TreeKeyIterator iter;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    Tcl_DStringAppendElement(&ds, "n");
    Tcl_DStringAppendElement(&ds, Blt_Tree_NodeLabel(node));
    if (node == dumpPtr->root) {
        Tcl_DStringAppendElement(&ds, "-1");
    } else {
        Tcl_DStringAppendElement(&ds, 
              Blt_Tree_NodeIdAscii(Blt_Tree_ParentNode(node)));
    }   
    Tcl_DStringAppendElement(&ds, Blt_Tree_NodeIdAscii(node));
    if (WriteDumpRecord(dumpPtr, &ds) != TCL_OK) {
        goto error;
    }
    /* Add list of data fields. key-value pairs. */
    for (key = Blt_Tree_FirstKey(dumpPtr->tree, node, &iter); key != NULL; 
         key = Blt_Tree_NextKey(dumpPtr->tree, &iter)) {
        Tcl_Obj *valueObjPtr;
        
        if (Blt_Tree_GetValueByKey((Tcl_Interp *)NULL, dumpPtr->tree, node, 
                                   key, &valueObjPtr) == TCL_OK) {
            if ((valueObjPtr->typePtr != NULL) &&
                (strcmp(valueObjPtr->typePtr->name, "list") == 0)) {
                if (DumpListValues(dumpPtr, valueObjPtr, key) != TCL_OK) {
                    goto error;
                }
            } else {
                Tcl_DStringAppendElement(&ds, "d");
                Tcl_DStringAppendElement(&ds, key);
                Tcl_DStringAppendElement(&ds, Tcl_GetString(valueObjPtr));
                if (WriteDumpRecord(dumpPtr, &ds) != TCL_OK) {
                    goto error;
                }
            }
        }
    }           
    if ((dumpPtr->flags & DUMP_NO_TAGS) == 0) {
        /* Add list of tags. */
        for (hPtr = Blt_Tree_FirstTag(dumpPtr->tree, &cursor); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&cursor)) {
            Blt_TreeTagEntry *tePtr;
            
            tePtr = Blt_GetHashValue(hPtr);
            if (Blt_FindHashEntry(&tePtr->nodeTable, (char *)node) != NULL) {
                Tcl_DStringAppendElement(&ds, "t");
                Tcl_DStringAppendElement(&ds, tePtr->tagName);
                if (WriteDumpRecord(dumpPtr, &ds) != TCL_OK) {
                    goto error;
                }
            }
        }
    }
    Tcl_DStringFree(&ds);

    return TCL_OK;
 error:
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpTree --
 *
 *      Dumps node information recursively from the given tree based
 *      starting at *rootPtr*. The dump information is written to the file
 *      named. If the file name starts with an '@', then it is the name of
 *      an already opened channel to be used.
 *      
 * Results:
 *      A standard TCL result.  If the dump was successful, TCL_OK is
 *      returned.  Otherwise, TCL_ERROR is returned and an error message is
 *      left in the interpreter result.
 *
 * Side Effects:
 *      Dump information is written to the named file.
 *
 *---------------------------------------------------------------------------
 */
static int
DumpTree(Tcl_Interp *interp, DumpInfo *dumpPtr)
{
    Blt_TreeNode node;
    Tcl_DString ds;
    
    Tcl_DStringInit(&ds);
    if (dumpPtr->version > 2.9) {
        Tcl_DStringAppend(&ds, "# V3.0", 6);
    } else {
        Tcl_DStringAppend(&ds, "# V2.0", 6);
    }            
    if (WriteDumpRecord(dumpPtr, &ds) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_DStringFree(&ds);
    for (node = dumpPtr->root; node != NULL; 
         node = Blt_Tree_NextNode(dumpPtr->root, node)) {
        int result;

        if (dumpPtr->version > 2.9) {
            result = DumpNodeV3(dumpPtr, node);
        } else {
            result = DumpNodeV2(dumpPtr, node);
        }
        if (result != TCL_OK) {
            return TCL_ERROR;
        }
    }
    return TCL_OK;
}

static int
ReplaceNode(TreeCmd *cmdPtr, Blt_TreeNode srcNode, Blt_TreeNode destNode)
{
    const char *label;
    Blt_TreeKey key;
    Blt_TreeKeyIterator keyIter;
    Blt_HashSearch iter;
    Blt_HashEntry *hPtr;

    label = Blt_Tree_NodeLabel(srcNode);
    Blt_Tree_RelabelNode(cmdPtr->tree, destNode, label);
    
    /* First unset all the values in the destination. */
    for (key = Blt_Tree_FirstKey(cmdPtr->tree, destNode, &keyIter); key != NULL;
         key = Blt_Tree_NextKey(cmdPtr->tree, &keyIter)) {
        if (Blt_Tree_UnsetValueByKey(cmdPtr->interp, cmdPtr->tree, destNode, 
                 key) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    /* Now copy the source node's data fields to the destination. */
    for (key = Blt_Tree_FirstKey(cmdPtr->tree, srcNode, &keyIter); key != NULL; 
        key = Blt_Tree_NextKey(cmdPtr->tree, &keyIter)) {
        Tcl_Obj *valueObjPtr;
        
        if (Blt_Tree_GetValueByKey(cmdPtr->interp, cmdPtr->tree, srcNode, key, 
                &valueObjPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        if (Blt_Tree_SetValueByKey(cmdPtr->interp, cmdPtr->tree, destNode, key,
                                   valueObjPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    Blt_Tree_ClearTags(cmdPtr->tree, destNode);
    /* Add tags to destination node. */
    for (hPtr = Blt_Tree_FirstTag(cmdPtr->tree, &iter); hPtr != NULL; 
         hPtr = Blt_NextHashEntry(&iter)) {
        Blt_HashEntry *h2Ptr;
        Blt_TreeTagEntry *tPtr;

        tPtr = Blt_GetHashValue(hPtr);
        h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)srcNode);
        if (h2Ptr == NULL) {
            continue;
        }
        Blt_Tree_AddTag(cmdPtr->tree, destNode, tPtr->tagName);
    }
    return TCL_OK;
}

/* Tree command operations. */

/*
 *---------------------------------------------------------------------------
 *
 * ApplyOp --
 *
 *      treeName apply nodeName -precommand cmdList -postcommand cmdList
 *
 *---------------------------------------------------------------------------
 */
static int
ApplyOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    ApplySwitches switches;
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    int order, result;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.maxDepth = -1;
    switches.cmdPtr = cmdPtr;
    
    /* Process switches  */
    if (Blt_ParseSwitches(interp, applySwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    order = 0;
    if (switches.flags & MATCH_NOCASE) {
        Blt_ListNode ln;

        for (ln = Blt_List_FirstNode(switches.patternList); ln != NULL;
             ln = Blt_List_NextNode(ln)) {
            Blt_LowerCase((char *)Blt_List_GetKey(ln));
        }
    }
    if (switches.preCmdObjPtr != NULL) {
        order |= TREE_PREORDER;
    }
    if (switches.postCmdObjPtr != NULL) {
        order |= TREE_POSTORDER;
    }
    result = Blt_Tree_ApplyDFS(node, ApplyNodeProc, &switches, order);
    Blt_FreeSwitches(applySwitches, (char *)&switches, 0);
    if (result == TCL_ERROR) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*ARGSUSED*/
static int
AncestorOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_TreeNode node1, node2;
    TreeCmd *cmdPtr = clientData;
    long i, d1, d2, minDepth;

    if ((Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node1)
         != TCL_OK) ||
        (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node2)
         != TCL_OK)) {
        return TCL_ERROR;
    }
    if (node1 == node2) {
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(node1));
        return TCL_OK;
    }
    d1 = Blt_Tree_NodeDepth(node1);
    d2 = Blt_Tree_NodeDepth(node2);
    minDepth = MIN(d1, d2);
    if (minDepth == 0) {                /* One of the nodes is root. */
        Blt_TreeNode ancestor;

        ancestor = Blt_Tree_RootNode(cmdPtr->tree);
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(ancestor));
        return TCL_OK;
    }
    /* 
     * Traverse back from the deepest node, until the both nodes are at the
     * same depth.  Check if the ancestor node found is the other node.
     */
    for (i = d1; i > minDepth; i--) {
        node1 = Blt_Tree_ParentNode(node1);
    }
    if (node1 == node2) {
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(node2));
        return TCL_OK;
    }
    for (i = d2; i > minDepth; i--) {
        node2 = Blt_Tree_ParentNode(node2);
    }
    if (node2 == node1) {
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(node1));
        return TCL_OK;
    }

    /* 
     * First find the mutual ancestor of both nodes.  Look at each
     * preceding ancestor level-by-level for both nodes.  Eventually we'll
     * find a node that's the parent of both ancestors.  Then find the
     * first ancestor in the parent's list of subnodes.
     */
    for (i = minDepth; i > 0; i--) {
        node1 = Blt_Tree_ParentNode(node1);
        node2 = Blt_Tree_ParentNode(node2);
        if (node1 == node2) {
            Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(node2));
            return TCL_OK;
        }
    }
    Tcl_AppendResult(interp, "unknown ancestor", (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * AppendOp --
 *
 *     treeName append nodeName fieldName ?value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
AppendOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeIterator iter;
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    const char *key;

    if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[2], &iter)
        != TCL_OK) {
        return TCL_ERROR;
    }
    key = Tcl_GetString(objv[3]);
    for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL;
         node = Blt_Tree_NextTaggedNode(&iter)) {
        int i;

        for (i = 4; i < objc; i++) {
            const char *value;

            value = Tcl_GetString(objv[i]);
            if (Blt_Tree_AppendValue(interp, cmdPtr->tree, node, key, value) 
                != TCL_OK) {
                return TCL_ERROR;
            }
        }
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * AttachOp --
 *
 *---------------------------------------------------------------------------
 */
static int
AttachOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    AttachSwitches switches;
    TreeCmd *cmdPtr = clientData;
    const char *treeName;

    treeName = Tcl_GetString(objv[2]);
    switches.mask = 0;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, attachSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (Blt_Tree_Attach(interp, cmdPtr->tree, treeName) != TCL_OK) {
        return TCL_ERROR;
    }
    if (switches.mask & TREE_NEWTAGS) {
        Blt_Tree_NewTagTable(cmdPtr->tree);
    }
    ClearTracesAndEvents(cmdPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ChildrenOp --
 *
 *---------------------------------------------------------------------------
 */
static int
ChildrenOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 3) {
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (node = Blt_Tree_FirstChild(node); node != NULL;
             node = Blt_Tree_NextSibling(node)) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    } else if (objc == 4) {
        long childPos;
        long inode;
        long count;
        
        /* Get the node at  */
        if (Blt_GetLongFromObj(interp, objv[3], &childPos) != TCL_OK) {
            return TCL_ERROR;
        }
        count = 0;
        inode = -1;
        for (node = Blt_Tree_FirstChild(node); node != NULL;
             node = Blt_Tree_NextSibling(node)) {
            if (count == childPos) {
                inode = Blt_Tree_NodeId(node);
                break;
            }
            count++;
        }
        Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
        return TCL_OK;
    } else if (objc == 5) {
        long firstPos, lastPos;
        long count;
        Tcl_Obj *listObjPtr;
        char *string;

        firstPos = lastPos = Blt_Tree_NodeDegree(node) - 1;
        string = Tcl_GetString(objv[3]);
        if ((strcmp(string, "end") != 0) &&
            (Blt_GetLongFromObj(interp, objv[3], &firstPos) != TCL_OK)) {
            return TCL_ERROR;
        }
        string = Tcl_GetString(objv[4]);
        if ((strcmp(string, "end") != 0) &&
            (Blt_GetLongFromObj(interp, objv[4], &lastPos) != TCL_OK)) {
            return TCL_ERROR;
        }

        count = 0;
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (node = Blt_Tree_FirstChild(node); node != NULL;
             node = Blt_Tree_NextSibling(node)) {
            if ((count >= firstPos) && (count <= lastPos)) {
                Tcl_Obj *objPtr;

                objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
            count++;
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

static Blt_TreeNode 
CopyNodes(
    CopySwitches *switchesPtr,
    Blt_TreeNode node,                  /* Node to be copied. */
    Blt_TreeNode parent)                /* New parent for the copied
                                         * node. */
{
    Blt_TreeNode newNode;               /* Newly created copy. */
    const char *label;

    newNode = NULL;
    label = Blt_Tree_NodeLabel(node);
    if (switchesPtr->flags & COPY_OVERWRITE) {
        newNode = Blt_Tree_FindChild(parent, label);
    }
    if (newNode == NULL) {      /* Create node in new parent. */
        newNode = Blt_Tree_CreateNode(switchesPtr->destTree, parent, label, -1);
    }
    /* Copy the data fields. */
    {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;

        for (key = Blt_Tree_FirstKey(switchesPtr->srcTree, node, &iter); 
             key != NULL; key = Blt_Tree_NextKey(switchesPtr->srcTree, &iter)) {
            Tcl_Obj *objPtr;

            if (Blt_Tree_GetValueByKey((Tcl_Interp *)NULL,
                        switchesPtr->srcTree, node, key, &objPtr) == TCL_OK) {
                Blt_Tree_SetValueByKey((Tcl_Interp *)NULL,
                        switchesPtr->destTree, newNode, key, objPtr);
            } 
        }
    }
    /* Add tags to destination tree command. */
    if ((switchesPtr->destPtr != NULL) && (switchesPtr->flags & COPY_TAGS)) {
        Blt_HashSearch iter;
        Blt_HashEntry *hPtr;

        for (hPtr = Blt_Tree_FirstTag(switchesPtr->srcPtr->tree, &iter); 
             hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
            Blt_HashEntry *h2Ptr;
            Blt_TreeTagEntry *tPtr;

            tPtr = Blt_GetHashValue(hPtr);
            h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)node);
            if (h2Ptr != NULL) {
                if (AddTag(switchesPtr->destPtr, newNode, tPtr->tagName)
                    != TCL_OK) {
                    return NULL;
                }
            }
        }
    }
    if (switchesPtr->flags & COPY_RECURSE) {
        Blt_TreeNode child;

        for (child = Blt_Tree_FirstChild(node); child != NULL;
             child = Blt_Tree_NextSibling(child)) {
            if (CopyNodes(switchesPtr, child, newNode) == NULL) {
                return NULL;
            }
        }
    }
    return newNode;
}

/*
 *---------------------------------------------------------------------------
 *
 * CopyOp --
 * 
 *      treeName copy parentNode srcNode ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CopyOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_Tree srcTree, destTree;
    Blt_TreeNode copyNode, parent, root;
    CopySwitches switches;
    TreeCmd *cmdPtr = clientData;
    TreeCmd *srcPtr, *destPtr;
    int i;
    int numArgs, numSwitches;
    
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent)
        != TCL_OK) {
        return TCL_ERROR;
    }
    srcTree = destTree = cmdPtr->tree;
    srcPtr = destPtr = cmdPtr;

    /* Find the first switch. */
    for(i = 3; i < objc; i++) {
        char *string;

        string = Tcl_GetString(objv[i]);
        if (string[0] == '-') {
            break;
        }
    }
    numArgs = i - 2;
    numSwitches = objc - i;
    if ((numArgs < 2) || (numArgs > 3)) {
        Tcl_AppendResult(interp, "wrong # args: should be \"", 
                        Tcl_GetString(objv[0]), 
                         " copy parent ?tree? node ?switches ...?", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    if (numArgs == 3) {
        char *string;

        /* 
         * The tree name is either the name of a tree command (first
         * choice) or an internal tree object.
         */
        string = Tcl_GetString(objv[3]);
        srcPtr = GetTreeCmd(cmdPtr->interpDataPtr, interp, string);
        if (srcPtr != NULL) {
            srcTree = srcPtr->tree;
        } else {
            /* Try to get the tree as an internal tree data object. */
            srcTree = Blt_Tree_GetFromObj(interp, objv[3]);
            if (srcTree == NULL) {
                return TCL_ERROR;
            }
        }
        objv++;
    }

    root = NULL;
    if (srcPtr == NULL) {
        if (GetForeignNode(interp, srcTree, objv[3], &copyNode) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        if (Blt_Tree_GetNodeFromObj(interp, srcPtr->tree, objv[3], &copyNode)
                != TCL_OK) {
            return TCL_ERROR;
        }
    }
    memset((char *)&switches, 0, sizeof(switches));
    switches.destPtr = destPtr;
    switches.destTree = destTree;
    switches.srcPtr = srcPtr;
    switches.srcTree = srcTree;

    /* Process switches  */
    if (Blt_ParseSwitches(interp, copySwitches, numSwitches, objv + 4,
                &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((switches.flags & COPY_OVERWRITE) && 
        (Blt_Tree_ParentNode(copyNode) == parent)) {
        Tcl_AppendResult(interp, "source and destination nodes are the same",
                 (char *)NULL);      
        return TCL_ERROR;
    }
    if ((srcTree == destTree) && (switches.flags & COPY_RECURSE) &&
        (Blt_Tree_IsAncestor(copyNode, parent))) {    
        Tcl_AppendResult(interp, "can't make cyclic copy: ",
                         "source node is an ancestor of the destination",
                         (char *)NULL);      
        return TCL_ERROR;
    }

    /* Copy nodes to destination. */
    root = CopyNodes(&switches, copyNode, parent);
    if (root != NULL) {
        if (switches.label != NULL) {
            Blt_Tree_RelabelNode(switches.destTree, root, switches.label);
        }
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(root));
    }
    return TCL_OK;

}

/*
 *---------------------------------------------------------------------------
 *
 * DegreeOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DegreeOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeDegree(node));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *      Deletes one or more nodes from the tree.  Nodes may be specified by
 *      their id (a number) or a tag.
 *      
 *      Tags have to be handled carefully here.  We can't use the normal
 *      GetTaggedNode, NextTaggedNode, etc. routines because they walk
 *      hashtables while we're deleting nodes.  Also, remember that
 *      deleting a node recursively deletes all its children. If a parent
 *      and its children have the same tag, its possible that the tag list
 *      may contain nodes than no longer exist. So save the node indices in
 *      a list and then delete then in a second pass.
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;
    char *string;
    int i;

    string = NULL;
    for (i = 2; i < objc; i++) {
        string = Tcl_GetString(objv[i]);
        if (IsNodeIdOrModifier(string)) {
            Blt_TreeNode node;

            if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[i], &node)
                != TCL_OK){
                return TCL_ERROR;
            }
            DeleteNode(cmdPtr, node);
        } else {
            Blt_Chain chain;
            Blt_ChainLink link, next;
            Blt_HashEntry *hPtr;
            Blt_HashSearch iter;
            Blt_HashTable *tablePtr;

            if ((strcmp(string, "all") == 0) || (strcmp(string, "root") == 0)) {
                Blt_TreeNode node;

                node = Blt_Tree_RootNode(cmdPtr->tree);
                DeleteNode(cmdPtr, node);
                continue;
            }
            tablePtr = Blt_Tree_TagHashTable(cmdPtr->tree, string);
            if (tablePtr == NULL) {
                goto error;
            }
            /* 
             * Generate a list of tagged nodes. Save the inode instead of
             * the node itself since a pruned branch may contain more
             * tagged nodes.
             */
            chain = Blt_Chain_Create();
            for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); 
                hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
                Blt_TreeNode node;

                node = Blt_GetHashValue(hPtr);
                Blt_Chain_Append(chain, (ClientData)Blt_Tree_NodeId(node));
            }   
            /*  
             * Iterate through this list to delete the nodes.  By
             * side-effect the tag table is deleted and Uids are released.
             */
            for (link = Blt_Chain_FirstLink(chain); link != NULL;
                 link = next) {
                Blt_TreeNode node;
                long inode;

                next = Blt_Chain_NextLink(link);
                inode = (long)Blt_Chain_GetValue(link);
                node = Blt_Tree_GetNodeFromIndex(cmdPtr->tree, inode);
                if (node != NULL) {
                    DeleteNode(cmdPtr, node);
                }
            }
            Blt_Chain_Destroy(chain);
        }
    }
    return TCL_OK;
 error:
    Tcl_AppendResult(interp, "can't find tag or id \"", string, 
        "\" to delete in ", Blt_Tree_Name(cmdPtr->tree), (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DepthOp --
 *
 *      treeName depth
 *      treeName depth nodeName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DepthOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (objc == 2) {
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_Depth(cmdPtr->tree));
        return TCL_OK;
    }
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeDepth(node));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DumpOp --
 *
 *      treeName dump nodeName -file fileName
 *      treeName dump nodeName -data varName
 *      treeName dump nodeName 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DumpOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode root;
    DumpInfo dump;
    TreeCmd *cmdPtr = clientData;
    int result;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &root)
        != TCL_OK) {
        return TCL_ERROR;
    }
    memset((char *)&dump, 0, sizeof(DumpInfo));
    dump.tree = cmdPtr->tree;
    dump.root = root;
    dump.version = 3.0;
    Tcl_DStringInit(&dump.ds);
    if (Blt_ParseSwitches(interp, dumpSwitches, objc - 3, objv + 3, 
        &dump, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((dump.dataObjPtr != NULL) && (dump.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "can't set both -file and -data switches.",
                         (char *)NULL);
        Blt_FreeSwitches(dumpSwitches, (char *)&dump, 0);
        return TCL_ERROR;
    }
    if (dump.dataObjPtr != NULL) {
        Tcl_Obj *objPtr;

        result = DumpTree(interp, &dump);
        if (result != TCL_OK) {
            goto error;
        }

        /* Write the image into the designated TCL variable. */
        objPtr = Tcl_NewStringObj(Tcl_DStringValue(&dump.ds),
                                  Tcl_DStringLength(&dump.ds));
        objPtr = Tcl_ObjSetVar2(interp, dump.dataObjPtr, NULL, objPtr, 0);
        result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
        Tcl_SetStringObj(dump.dataObjPtr, Tcl_DStringValue(&dump.ds),
                         Tcl_DStringLength(&dump.ds));
    } else if (dump.fileObjPtr != NULL) {
        Tcl_Channel channel;
        int closeChannel;
        const char *fileName;

        fileName =  Tcl_GetString(dump.fileObjPtr);
        closeChannel = TRUE;
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
                goto error;
            }
        }
        dump.channel = channel;
        result = DumpTree(interp, &dump);
        if (closeChannel) {
            Tcl_Close(NULL, channel);
        }
    } else {
        result = DumpTree(interp, &dump);
        if (result == TCL_OK) {
            Tcl_DStringResult(interp, &dump.ds);
        }
    }
    Tcl_DStringFree(&dump.ds);
    Blt_FreeSwitches(dumpSwitches, (char *)&dump, 0);
    return result;
 error:
    Tcl_DStringFree(&dump.ds);
    Blt_FreeSwitches(dumpSwitches, (char *)&dump, 0);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * DupOp --
 * 
 *      treeName dup nodeName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DupOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    TreeCmd *srcPtr, *destPtr;
    
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    srcPtr = cmdPtr;
    destPtr = CreateTreeCmd(cmdPtr->interpDataPtr, interp, NULL);
    if (destPtr == NULL) {
        return TCL_ERROR;
    }
    DupNode(srcPtr, node, destPtr, Blt_Tree_RootNode(destPtr->tree));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      treeName exists nodeName ?dataName?
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    int bool;
    
    bool = TRUE;
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        bool = FALSE;
    } else if (objc == 4) { 
        Tcl_Obj *valueObjPtr;
        char *string;
        
        string = Tcl_GetString(objv[3]);
        if (Blt_Tree_GetValue((Tcl_Interp *)NULL, cmdPtr->tree, node, 
                             string, &valueObjPtr) != TCL_OK) {
            bool = FALSE;
        }
    } 
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExportOp --
 *
 *      treeName export fmtName ?switches...?
 *
 *---------------------------------------------------------------------------
 */
static int
ExportOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    DataFormat *fmtPtr;
    TreeCmd *cmdPtr = clientData;
    TreeCmdInterpData *dataPtr;
    const char *fmt;

    dataPtr = GetTreeCmdInterpData(interp);
    if (objc == 2) {
        Blt_HashSearch iter;

        for (hPtr = Blt_FirstHashEntry(&dataPtr->fmtTable, &iter); 
             hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
            fmtPtr = Blt_GetHashValue(hPtr);
            if (fmtPtr->exportProc != NULL) {
                Tcl_AppendElement(interp, fmtPtr->name);
            }
        }
        return TCL_OK;
    }
    fmt = Tcl_GetString(objv[2]);
    hPtr = Blt_FindHashEntry(&dataPtr->fmtTable, fmt);
    if (hPtr == NULL) {
        LoadFormat(interp, fmt);
        hPtr = Blt_FindHashEntry(&dataPtr->fmtTable, fmt);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "can't export \"", Tcl_GetString(objv[2]),
                         "\": format not registered", (char *)NULL);
            return TCL_ERROR;
        }
    }
    fmtPtr = Blt_GetHashValue(hPtr);
    if (fmtPtr->exportProc == NULL) {
        Tcl_AppendResult(interp, "can't find tree export procedure for \"", 
                         fmtPtr->name, "\" format.", (char *)NULL);
        return TCL_ERROR;
    }
    return (*fmtPtr->exportProc) (interp, cmdPtr->tree, objc, objv);
}


/*
 *---------------------------------------------------------------------------
 *
 * FindOp --
 *
 *---------------------------------------------------------------------------
 */
static int
FindOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    FindSwitches switches;
    TreeCmd *cmdPtr = clientData;
    int result;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    switches.maxDepth = -1;
    switches.order = TREE_POSTORDER;
    switches.cmdPtr = cmdPtr;
    Blt_InitHashTable(&switches.excludeTable, BLT_ONE_WORD_KEYS);

    /* Process switches  */
    if (Blt_ParseSwitches(interp, findSwitches, objc - 3, objv + 3, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.maxDepth >= 0) {
        switches.maxDepth += Blt_Tree_NodeDepth(node);
    }
    if (switches.flags & MATCH_NOCASE) {
        Blt_ListNode lnode;

        for (lnode = Blt_List_FirstNode(switches.patternList); lnode != NULL;
             lnode = Blt_List_NextNode(lnode)) {
            Blt_LowerCase((char *)Blt_List_GetKey(lnode));
        }
    }
    switches.listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    switches.cmdPtr = cmdPtr;
    if (switches.order == TREE_BREADTHFIRST) {
        result = Blt_Tree_ApplyBFS(node, MatchNodeProc, &switches);
    } else {
        result = Blt_Tree_ApplyDFS(node, MatchNodeProc, &switches, 
                switches.order);
    }
    Blt_FreeSwitches(findSwitches, (char *)&switches, 0);
    if (result == TCL_ERROR) {
        Tcl_DecrRefCount(switches.listObjPtr);
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, switches.listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FindChildOp --
 *
 *      treeName findchild nodeName childName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FindChildOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_TreeNode parent, child;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent)
        != TCL_OK) {
        return TCL_ERROR;
    }
    inode = -1;
    child = Blt_Tree_FindChild(parent, Tcl_GetString(objv[3]));
    if (child != NULL) {
        inode = Blt_Tree_NodeId(child);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FirstChildOp --
 *
 *      treeName firstchild nodeName
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FirstChildOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    inode = -1;
    node = Blt_Tree_FirstChild(node);
    if (node != NULL) {
        inode = Blt_Tree_NodeId(node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *      treeName get nodeName 
 *      treeName get nodeName ?dataName?
 *      treeName get nodeName ?dataName? ?defValue?
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        if (objc == 5) {
            Tcl_SetObjResult(interp, objv[4]);
            return TCL_OK;              /* If there's a default value,
                                         * don't generate an error if we
                                         * can't find the node. */
        }
        return TCL_ERROR;
    }
    if (objc == 3) {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;
        Tcl_Obj *valueObjPtr, *listObjPtr;

        /* Add the key-value pairs to a new Tcl_Obj */
        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &iter); key != NULL; 
             key = Blt_Tree_NextKey(cmdPtr->tree, &iter)) {
            if (Blt_Tree_GetValue((Tcl_Interp *)NULL, cmdPtr->tree, node, key,
                                 &valueObjPtr) == TCL_OK) {
                Tcl_Obj *objPtr;

                objPtr = Tcl_NewStringObj(key, -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                Tcl_ListObjAppendElement(interp, listObjPtr, valueObjPtr);
            }
        }           
        Tcl_SetObjResult(interp, listObjPtr);
        return TCL_OK;
    } else {
        Tcl_Obj *valueObjPtr;
        const char *string;

        string = Tcl_GetString(objv[3]); 
        if (Blt_Tree_GetValue((Tcl_Interp *)NULL, cmdPtr->tree, node, string,
                     &valueObjPtr) != TCL_OK) {
            if (objc == 4) {
                Tcl_DString ds;
                const char *path;

                Tcl_DStringInit(&ds);
                path = Blt_Tree_NodePath(node, &ds);            
                Tcl_AppendResult(interp, "can't find field \"", string, 
                        "\" in \"", path, "\"", (char *)NULL);
                Tcl_DStringFree(&ds);
                return TCL_ERROR;
            } 
            /* Default to given value */
            valueObjPtr = objv[4];
        } 
        Tcl_SetObjResult(interp, valueObjPtr);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * DirOp --
 *
 *      treeName dir pathName nodeName ?switches ...? 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DirOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode parent;
    ReadDirectory reader;
    TreeCmd *cmdPtr = clientData;
    int result;
    
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent)
        != TCL_OK) {
        return TCL_ERROR;
    }
    memset(&reader, 0, sizeof(reader));
    if (Blt_ParseSwitches(interp, dirSwitches, objc - 4, objv + 4, &reader,
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (reader.mask == 0) {
        reader.mask = READ_DIR_DEFAULT;
    }
    result = ReadDirectoryIntoTree(interp, cmdPtr, objv[3], parent, &reader);
    Blt_FreeSwitches(dirSwitches, &reader, 0);
    return (result == READ_DIR_ERROR) ? TCL_ERROR: TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * ImportOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ImportOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    DataFormat *fmtPtr;
    TreeCmd *cmdPtr = clientData;
    TreeCmdInterpData *dataPtr;
    const char *fmt;

    dataPtr = GetTreeCmdInterpData(interp);
    hPtr = NULL;
    if (objc == 2) {
        Blt_HashSearch iter;

        for (hPtr = Blt_FirstHashEntry(&dataPtr->fmtTable, &iter); 
             hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
            fmtPtr = Blt_GetHashValue(hPtr);
            if (fmtPtr->importProc != NULL) {
                Tcl_AppendElement(interp, fmtPtr->name);
            }
        }
        return TCL_OK;
    }
    fmt = Tcl_GetString(objv[2]);
    if (hPtr == NULL) {
        LoadFormat(interp, fmt);
        hPtr = Blt_FindHashEntry(&dataPtr->fmtTable, fmt);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "can't import \"", Tcl_GetString(objv[2]),
                         "\": format not registered", (char *)NULL);
            return TCL_ERROR;
        }
    }
    fmtPtr = Blt_GetHashValue(hPtr);
    if (fmtPtr->importProc == NULL) {
        Tcl_AppendResult(interp, "can't find tree import procedure for \"", 
                         fmtPtr->name, "\" format.", (char *)NULL);
        return TCL_ERROR;
    }
    return (*fmtPtr->importProc) (interp, cmdPtr->tree, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * IndexOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IndexOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    inode = -1;
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) ==
        TCL_OK) {
        if (node != NULL) {
            inode = Blt_Tree_NodeId(node);
        }
    } else {
        int i;
        Blt_TreeNode parent;
        Tcl_Obj **pathv;
        int pathc;

        if (Tcl_ListObjGetElements(interp, objv[2], &pathc, &pathv) != TCL_OK) {
            goto done;          /* Can't split object. */
        }
        /* Start from the root and verify each path component. */
        parent = Blt_Tree_RootNode(cmdPtr->tree);
        for (i = 0; i < pathc; i++) {
            const char *string;

            string = Tcl_GetString(pathv[i]);
            if (string[0] == '\0') {
                continue;       /* Skip null separators. */
            }
            node = Blt_Tree_FindChild(parent, string);
            if (node == NULL) {
                goto done;      /* Can't find component */
            }
            parent = node;
        }
        inode = Blt_Tree_NodeId(node);
    }
 done:
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InsertOp --
 *
 *      treeName insert parentNode ?switches ...?
 *---------------------------------------------------------------------------
 */
static int
InsertOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode parent, child;
    InsertSwitches switches;
    TreeCmd *cmdPtr = clientData;

    child = NULL;
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent)
        != TCL_OK) {
        return TCL_ERROR;
    }
    /* Initialize switch flags */
    memset(&switches, 0, sizeof(switches));
    switches.position = -1;             /* Default to append node. */
    switches.parent = parent;
    switches.inode = -1;

    if (Blt_ParseSwitches(interp, insertSwitches, objc - 3, objv + 3, &switches,
        BLT_SWITCH_DEFAULTS) < 0) {
        goto error;
    }
    if (switches.inode > 0) {
        Blt_TreeNode node;

        node = Blt_Tree_GetNodeFromIndex(cmdPtr->tree, switches.inode);
        if (node != NULL) {
            Tcl_AppendResult(interp, "can't reissue node id \"", 
                Blt_Ltoa(switches.inode), "\": id already exists.", 
                (char *)NULL);
            goto error;
        }
        child = Blt_Tree_CreateNodeWithId(cmdPtr->tree, parent, switches.label, 
                switches.inode, switches.position);
    } else {
        child = Blt_Tree_CreateNode(cmdPtr->tree, parent, switches.label, 
                switches.position);
    }
    if (child == NULL) {
        Tcl_AppendResult(interp, "can't allocate new node", (char *)NULL);
        goto error;
    }
    if (switches.label == NULL) {
        char string[200];

        Blt_FormatString(string, 200, "node%ld", Blt_Tree_NodeId(child));
        Blt_Tree_RelabelNodeWithoutNotify(child, string);
    } 
    if (switches.tagsObjPtr != NULL) {
        int i, numTags;
        Tcl_Obj **tags;

        if (Tcl_ListObjGetElements(interp, switches.tagsObjPtr, &numTags, &tags)
            != TCL_OK) {
            goto error;
        }
        for (i = 0; i < numTags; i++) {
            if (AddTag(cmdPtr, child, Tcl_GetString(tags[i])) != TCL_OK) {
                goto error;
            }
        }
    }
    if (switches.dataPairs != NULL) {
        char **p;

        for (p = switches.dataPairs; *p != NULL; p++) {
            Tcl_Obj *objPtr;
            char *key;

            key = *p;
            p++;
            if (*p == NULL) {
                Tcl_AppendResult(interp, "missing value for \"", key, "\"",
                                 (char *)NULL);
                goto error;
            }
            objPtr = Tcl_NewStringObj(*p, -1);
            if (Blt_Tree_SetValue(interp, cmdPtr->tree, child, key, objPtr) 
                != TCL_OK) {
                Tcl_DecrRefCount(objPtr);
                goto error;
            }
        }
    }
    Tcl_SetObjResult(interp, Tcl_NewLongObj(Blt_Tree_NodeId(child)));
    Blt_FreeSwitches(insertSwitches, (char *)&switches, 0);
    return TCL_OK;

 error:
    if (child != NULL) {
        Blt_Tree_DeleteNode(cmdPtr->tree, child);
    }
    Blt_FreeSwitches(insertSwitches, (char *)&switches, 0);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsAncestorOp --
 *
 *      treeName isancestor nodeName $parent
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsAncestorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Blt_TreeNode node, parent;
    TreeCmd *cmdPtr = clientData;
    int isAncestor;

    if ((Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
         != TCL_OK) ||
        (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &parent)
         != TCL_OK)) {
        return TCL_ERROR;
    }
    isAncestor = Blt_Tree_IsAncestor(node, parent);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), isAncestor);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsBeforeOp --
 *
 *      treeName isbefore nodeName1 nodeName2
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsBeforeOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_TreeNode node1, node2;
    TreeCmd *cmdPtr = clientData;
    int isBefore;

    if ((Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node1)
         != TCL_OK) ||
        (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node2)
         != TCL_OK)) {
        return TCL_ERROR;
    }
    isBefore = Blt_Tree_IsBefore(node1, node2);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), isBefore);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsLeafOp --
 *
 *      treeName isleaf nodeName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsLeafOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), Blt_Tree_IsLeaf(node));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * IsRootOp --
 *
 *      treeName isroot nodeName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsRootOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    int isRoot;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    isRoot = (node == Blt_Tree_RootNode(cmdPtr->tree));
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), isRoot);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * KeysOp --
 *
 *      Returns the key names of values for a node or array value.
 *
 *---------------------------------------------------------------------------
 */
static int
KeysOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_HashTable keyTable;
    TreeCmd *cmdPtr = clientData;
    int i;

    Blt_InitHashTableWithPool(&keyTable, BLT_ONE_WORD_KEYS);
    for (i = 2; i < objc; i++) {
        Blt_TreeIterator iter;
        Blt_TreeNode node;
        int isNew;

        if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[i], &iter)
            != TCL_OK) {
            return TCL_ERROR;
        }
        for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL;
             node = Blt_Tree_NextTaggedNode(&iter)) {
            Blt_TreeKey key;
            Blt_TreeKeyIterator keyIter;

            for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &keyIter); 
                 key != NULL; key = Blt_Tree_NextKey(cmdPtr->tree, &keyIter)) {
                Blt_CreateHashEntry(&keyTable, key, &isNew);
            }
        }
    }
    {
        Blt_HashSearch iter;
        Blt_HashEntry *hPtr;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (hPtr = Blt_FirstHashEntry(&keyTable, &iter); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&iter)) {
            Tcl_Obj *objPtr;
            
            objPtr = Tcl_NewStringObj(Blt_GetHashKey(&keyTable, hPtr), -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&keyTable);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LabelOp --
 *
 *---------------------------------------------------------------------------
 */
static int
LabelOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 4) {
        Blt_Tree_RelabelNode(cmdPtr->tree, node, Tcl_GetString(objv[3]));
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), Blt_Tree_NodeLabel(node), -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListAppendOp --
 *
 *      treeName lappend nodeName fieldName ?value ...?
 *
 *---------------------------------------------------------------------------
 */
static int
ListAppendOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Blt_TreeIterator iter;
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    const char *key;

    if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[2], &iter)
        != TCL_OK) {
        return TCL_ERROR;
    }
    key = Tcl_GetString(objv[3]);
    for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL; 
         node = Blt_Tree_NextTaggedNode(&iter)) {
        int i;

        for (i = 4; i < objc; i++) {
            if (Blt_Tree_ListAppendValue(interp, cmdPtr->tree, node, key, 
                objv[i]) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * LastChildOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
LastChildOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    node = Blt_Tree_LastChild(node);
    inode = (node != NULL) ? Blt_Tree_NodeId(node) : -1 ;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListIndexOp --
 *
 *      pathName lindex nodeName fieldName indexNum
 *---------------------------------------------------------------------------
 */
static int
ListIndexOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *valueObjPtr, *objPtr;
    TreeCmd *cmdPtr = clientData;
    Blt_TreeKey key;
    int index;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    key = Blt_Tree_GetKey(cmdPtr->tree, Tcl_GetString(objv[3]));
    if (Blt_Tree_GetValueByKey(interp, cmdPtr->tree, node, key, &valueObjPtr) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[4], &index) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_ListObjIndex(interp, valueObjPtr, index, &objPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ListLengthOp --
 *
 *      pathName llength nodeName fieldName
 *---------------------------------------------------------------------------
 */
static int
ListLengthOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *valueObjPtr;
    TreeCmd *cmdPtr = clientData;
    Blt_TreeKey key;
    int length;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    key = Blt_Tree_GetKey(cmdPtr->tree, Tcl_GetString(objv[3]));
    if (Blt_Tree_GetValueByKey(interp, cmdPtr->tree, node, key, &valueObjPtr) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (Tcl_ListObjLength(interp, valueObjPtr, &length) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), length);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * MoveOp --
 *
 *      The trick here is to not consider the node to be moved in
 *      determining it's new location.  Ideally, you would temporarily pull
 *      it from the tree and replace it (back in its old location if
 *      something went wrong), but you could still pick the node by its
 *      serial number.  So here we make lots of checks for the node to be
 *      moved.
 * 
 *---------------------------------------------------------------------------
 */
static int
MoveOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode before;
    Blt_TreeNode root, parent, node;
    MoveSwitches switches;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &parent)
        != TCL_OK) {
        return TCL_ERROR;
    }
    root = Blt_Tree_RootNode(cmdPtr->tree);
    if (node == root) {
        Tcl_AppendResult(interp, "can't move root node", (char *)NULL);
        return TCL_ERROR;
    }
    if (parent == node) {
        Tcl_AppendResult(interp, "can't move node to self", (char *)NULL);
        return TCL_ERROR;
    }
    switches.node = NULL;
    switches.cmdPtr = cmdPtr;
    switches.movePos = -1;
    nodeSwitch.clientData = cmdPtr->tree;
    /* Process switches  */
    if (Blt_ParseSwitches(interp, moveSwitches, objc - 4, objv + 4, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    /* Verify they aren't ancestors. */
    if (Blt_Tree_IsAncestor(node, parent)) {
        Tcl_AppendResult(interp, "can't move node: \"", 
                 Tcl_GetString(objv[2]), (char *)NULL);
        Tcl_AppendResult(interp, "\" is an ancestor of \"", 
                 Tcl_GetString(objv[3]), "\"", (char *)NULL);
        return TCL_ERROR;
    }
    before = NULL;                      /* If before is NULL, this appends
                                         * the node to the parent's child
                                         * list.  */

    if (switches.node != NULL) {        /* -before or -after */
        if (Blt_Tree_ParentNode(switches.node) != parent) {
            Tcl_AppendResult(interp, Tcl_GetString(objv[2]), 
                     " isn't the parent of ", Blt_Tree_NodeLabel(switches.node),
                     (char *)NULL);
            return TCL_ERROR;
        }
        if (Blt_SwitchChanged(moveSwitches, "-before", (char *)NULL)) {
            before = switches.node;
            if (before == node) {
                Tcl_AppendResult(interp, "can't move node before itself", 
                                 (char *)NULL);
                return TCL_ERROR;
            }
        } else {
            before = Blt_Tree_NextSibling(switches.node);
            if (before == node) {
                Tcl_AppendResult(interp, "can't move node after itself", 
                                 (char *)NULL);
                return TCL_ERROR;
            }
        }
    } else if (switches.movePos >= 0) { /* -at */
        int count;                     /* Tracks the current list index. */
        Blt_TreeNode child;

        /* 
         * If the node is in the list, ignore it when determining the
         * "before" node using the -at index.  An index of -1 means to
         * append the node to the list.
         */
        count = 0;
        for(child = Blt_Tree_FirstChild(parent); child != NULL; 
            child = Blt_Tree_NextSibling(child)) {
            if (child == node) {
                continue;               /* Ignore the node to be moved. */
            }
            if (count == switches.movePos) {
                before = child;
                break;          
            }
            count++;    
        }
    }
    if (Blt_Tree_MoveNode(cmdPtr->tree, node, parent, before) != TCL_OK) {
        Tcl_AppendResult(interp, "can't move node ", Tcl_GetString(objv[2]), 
                 " to ", Tcl_GetString(objv[3]), (char *)NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * NamesOp --
 *
 *      Returns the names of values for a node or array value.
 *
 *      treeName names
 *      treeName names fieldName
 *---------------------------------------------------------------------------
 */
static int
NamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr;
    TreeCmd *cmdPtr = clientData;
    
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (objc == 4) { 
        char *string;

        string = Tcl_GetString(objv[3]);
        if (Blt_Tree_ArrayNames(interp, cmdPtr->tree, node, string, listObjPtr)
            != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Blt_TreeKey key;
        Blt_TreeKeyIterator iter;

        for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &iter); key != NULL; 
             key = Blt_Tree_NextKey(cmdPtr->tree, &iter)) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(key, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }           
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NextOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    node = Blt_Tree_NextNode(NULL, node);
    inode = (node != NULL) ? Blt_Tree_NodeId(node) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextSiblingOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NextSiblingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    inode = -1;
    node = Blt_Tree_NextSibling(node);
    if (node != NULL) {
        inode = Blt_Tree_NodeId(node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyCreateOp --
 *
 *      tree0 notify create ?switches ...? command arg
 *---------------------------------------------------------------------------
 */
static int
NotifyCreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    Notifier *notifyPtr;
    NotifySwitches switches;
    TreeCmd *cmdPtr = clientData;
    int i;

    nodeSwitch.clientData = cmdPtr->tree;
    memset(&switches, 0, sizeof(switches));
    /* Process switches  */
    i = Blt_ParseSwitches(interp, notifySwitches, objc - 3, objv + 3, &switches,
                          BLT_SWITCH_OBJV_PARTIAL);
    if (i < 0) {
        return TCL_ERROR;
    }
    objc -= 3 + i;
    objv += 3 + i;
    notifyPtr = Blt_AssertCalloc(1, sizeof(Notifier));
    notifyPtr->inode = -1;
    if (switches.node != NULL) {
        notifyPtr->inode = Blt_Tree_NodeId(switches.node);
    }
    if (switches.tag != NULL) {
        notifyPtr->tag = Blt_Strdup(switches.tag);
    }
    notifyPtr->cmdObjPtr = Tcl_NewListObj(objc, objv);
    Tcl_IncrRefCount(notifyPtr->cmdObjPtr);
    notifyPtr->cmdPtr = cmdPtr;
    if (switches.mask == 0) {
        switches.mask = TREE_NOTIFY_ALL;
    }
    notifyPtr->mask = switches.mask;

    {
        Blt_HashEntry *hPtr;
        char string[200];
        int isNew;

        Blt_FormatString(string, 200, "notify%d", cmdPtr->notifyCounter++);
        hPtr = Blt_CreateHashEntry(&cmdPtr->notifyTable, string, &isNew);
        assert(isNew);
        notifyPtr->link = Blt_Chain_Append(cmdPtr->notifiers, notifyPtr);
        Blt_SetHashValue(hPtr, notifyPtr);
        notifyPtr->hashPtr = hPtr;
        Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyDeleteOp --
 *
 *---------------------------------------------------------------------------
 */
static int
NotifyDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
               Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        Blt_HashEntry *hPtr;
        Notifier *notifyPtr;
        char *string;

        string = Tcl_GetString(objv[i]);
        hPtr = Blt_FindHashEntry(&cmdPtr->notifyTable, string);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "unknown notify name \"", string, "\"", 
                             (char *)NULL);
            return TCL_ERROR;
        }
        notifyPtr = Blt_GetHashValue(hPtr);
        FreeNotifier(cmdPtr, notifyPtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyInfoOp --
 *
 *---------------------------------------------------------------------------
 */

/*ARGSUSED*/
static int
NotifyInfoOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Notifier *notifyPtr;
    Tcl_DString ds;
    TreeCmd *cmdPtr = clientData;
    const char *string;

    string = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&cmdPtr->notifyTable, string);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "unknown notify name \"", string, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    notifyPtr = Blt_GetHashValue(hPtr);

    Tcl_DStringInit(&ds);
    Tcl_DStringAppendElement(&ds, string);      /* Copy notify Id */
    Tcl_DStringStartSublist(&ds);
    if (notifyPtr->mask & TREE_NOTIFY_CREATE) {
        Tcl_DStringAppendElement(&ds, "-create");
    }
    if (notifyPtr->mask & TREE_NOTIFY_DELETE) {
        Tcl_DStringAppendElement(&ds, "-delete");
    }
    if (notifyPtr->mask & TREE_NOTIFY_MOVE) {
        Tcl_DStringAppendElement(&ds, "-move");
    }
    if (notifyPtr->mask & TREE_NOTIFY_SORT) {
        Tcl_DStringAppendElement(&ds, "-sort");
    }
    if (notifyPtr->mask & TREE_NOTIFY_RELABEL) {
        Tcl_DStringAppendElement(&ds, "-relabel");
    }
    if (notifyPtr->mask & TREE_NOTIFY_WHENIDLE) {
        Tcl_DStringAppendElement(&ds, "-whenidle");
    }
    Tcl_DStringEndSublist(&ds);
    Tcl_DStringStartSublist(&ds);
    Tcl_DStringAppendElement(&ds, Tcl_GetString(notifyPtr->cmdObjPtr));
    Tcl_DStringEndSublist(&ds);
    Tcl_DStringResult(interp, &ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyNamesOp --
 *
 *      treeName notify names ?pattern ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NotifyNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;
    TreeCmd *cmdPtr = clientData;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&cmdPtr->notifyTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        int i;
        int match;
        const char *name;

        name = Blt_GetHashKey(&cmdPtr->notifyTable, hPtr);
        match = (objc == 3);
        for (i = 3; i < objc; i++) {
            char *pattern;

            pattern = Tcl_GetString(objv[i]);
            if (Tcl_StringMatch(name, pattern)) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NotifyOp --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec notifyOps[] =
{
    {"create", 1, NotifyCreateOp, 4, 0, "?switches ...? command"},
    {"delete", 1, NotifyDeleteOp, 3, 0, "?notifyName ...?"},
    {"info",   1, NotifyInfoOp,   4, 4, "notifyName"},
    {"names",  1, NotifyNamesOp,  3, 0, "?pattern ...?"},
};

static int numNotifyOps = sizeof(notifyOps) / sizeof(Blt_OpSpec);

static int
NotifyOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numNotifyOps, notifyOps, BLT_OP_ARG2, objc, 
        objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}


/*ARGSUSED*/
static int
ParentOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    inode = -1;
    node = Blt_Tree_ParentNode(node);
    if (node != NULL) {
        inode = Blt_Tree_NodeId(node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PathCreateOp --
 *
 *   treeName path create pathName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathCreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Blt_TreeNode child, parent;
    PathCreateSwitches switches;
    TreeCmd *cmdPtr = clientData;
    Tcl_Obj **elems;
    int numElems;
    int i;
    int result;
    long inode;
    const char *name;
    Tcl_Obj *listObjPtr;

    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    switches.root = Blt_Tree_RootNode(cmdPtr->tree);
    if (cmdPtr->defPathSep != NULL) {
        switches.pathSep = Blt_AssertStrdup(cmdPtr->defPathSep);
    }
    if (Blt_ParseSwitches(interp, pathCreateSwitches, objc - 4, objv + 4, 
                          &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    parent = switches.root;
    if ((switches.pathSep == NULL) || (switches.pathSep[0] == '\0')) {
        listObjPtr = NULL;
        result = Tcl_ListObjGetElements(interp, objv[3], &numElems, &elems);
    } else {
        listObjPtr = SplitPath(interp, objv[3], switches.pathSep);
        result = Tcl_ListObjGetElements(interp, listObjPtr, &numElems, &elems);
    }
    if (result != TCL_OK) {
        goto error;
    }
    if (numElems == 0) {
        goto done;
    }
    for (i = 0; i < (numElems - 1); i++) {
        Blt_TreeNode child;
        const char *name;

        name = Tcl_GetString(elems[i]);
        child = Blt_Tree_FindChild(parent, name);
        if (child == NULL) {
            if (switches.flags & PATH_PARENTS) {
                child = Blt_Tree_CreateNode(cmdPtr->tree, parent, name,-1);
            } else if (switches.flags & PATH_NOCOMPLAIN) {
                parent = NULL;
                goto done;
            } else {
                Tcl_DString ds;

                Tcl_DStringInit(&ds);
                Tcl_AppendResult(interp, "can't find parent node \"", 
                        name, "\" in \"", Blt_Tree_NodePath(parent, &ds), 
                        "\"", (char *)NULL);
                Tcl_DStringFree(&ds);
                goto error;
            }
        } 
        parent = child;
    }
    name = Tcl_GetString(elems[i]);
    child = Blt_Tree_FindChild(parent, name);
    if (child != NULL) {
        if ((switches.flags & PATH_NOCOMPLAIN) == 0) {
            Tcl_DString ds;
                
            Tcl_DStringInit(&ds);
            Tcl_AppendResult(interp, "can't create node: \"", name, 
                "\" already exists in ", Blt_Tree_NodePath(parent, &ds), 
                (char *)NULL);
            Tcl_DStringFree(&ds);
            goto error;
        }
    } else {
        child = Blt_Tree_CreateNode(cmdPtr->tree, parent, name, -1);
    }
    parent = child;
 done:
    inode = -1;
    if (parent != NULL) {
        inode = Blt_Tree_NodeId(parent);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    Blt_FreeSwitches(pathCreateSwitches, (char *)&switches, 0);
    return TCL_OK;
 error:
    Blt_FreeSwitches(pathCreateSwitches, (char *)&switches, 0);
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * PathParseOp --
 *
 *      treeName path parse pathName ?switches...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathParseOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_TreeNode child, parent;
    PathParseSwitches switches;
    TreeCmd *cmdPtr = clientData;
    Tcl_Obj **elems;
    int numElems;
    const char *name;
    int i;
    int result;
    long inode;
    Tcl_Obj *listObjPtr;

    /* Process switches  */
    nodeSwitch.clientData = cmdPtr->tree;
    memset(&switches, 0, sizeof(switches));
    switches.root = Blt_Tree_RootNode(cmdPtr->tree);
    if (cmdPtr->defPathSep != NULL) {
        switches.pathSep = Blt_AssertStrdup(cmdPtr->defPathSep);
    }
    if (Blt_ParseSwitches(interp, pathParseSwitches, objc - 4, objv + 4, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((switches.pathSep == NULL) || (switches.pathSep[0] == '\0')) {
        listObjPtr = NULL;
        result = Tcl_ListObjGetElements(interp, objv[3], &numElems, &elems);
    } else {
        listObjPtr = SplitPath(interp, objv[3], switches.pathSep);
        result = Tcl_ListObjGetElements(interp, listObjPtr, &numElems, &elems);
    }
    if (result != TCL_OK) {
        goto error;
    }
    if (numElems == 0) {
        inode = Blt_Tree_NodeId(switches.root);
        goto done;
    }
    parent = switches.root;
    for (i = 0; i < (numElems - 1); i++) {
        Blt_TreeNode child;
        const char *name;

        name = Tcl_GetString(elems[i]);
        child = Blt_Tree_FindChild(parent, name);
        if (child == NULL) {
            if (switches.flags & PATH_NOCOMPLAIN) {
                inode = -1;
                goto done;
            } else {
                Tcl_DString ds;
                
                Tcl_DStringInit(&ds);
                Tcl_AppendResult(interp, "can't find child labeled \"", name, 
                        "\" in \"", Blt_Tree_NodePath(parent, &ds), "\"", 
                        (char *)NULL);
                Tcl_DStringFree(&ds);
                goto error;
            }
        } 
        parent = child;
    }
    name = Tcl_GetString(elems[i]);
    child = Blt_Tree_FindChild(parent, name);
    if (child != NULL) {
        inode = Blt_Tree_NodeId(child);
    } else {
        if (switches.flags & PATH_NOCOMPLAIN) {
            inode = -1;
            goto done;
        } else {
            Tcl_DString ds;
            
            Tcl_DStringInit(&ds);
            Tcl_AppendResult(interp, "can't find child labeled \"", name, 
                             "\" in parent \"", 
                             Blt_Tree_NodePath(parent, &ds), "\" ", 
                             Blt_Itoa(Blt_Tree_NodeId(parent)),
                             (char *)NULL);
            Tcl_DStringFree(&ds);
            goto error;
        }
    }
 done:
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    Blt_FreeSwitches(pathParseSwitches, (char *)&switches, 0);
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    return TCL_OK;
 error:
    Blt_FreeSwitches(pathParseSwitches, (char *)&switches, 0);
    if (listObjPtr != NULL) {
        Tcl_DecrRefCount(listObjPtr);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * PathPrintOp --
 *
 *      treeName path print nodeName ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathPrintOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    PathPrintSwitches switches;
    Tcl_DString ds;
    TreeCmd *cmdPtr = clientData;
    int flags;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    switches.root = Blt_Tree_RootNode(cmdPtr->tree);
    if (cmdPtr->defPathSep != NULL) {
        switches.pathSep = Blt_AssertStrdup(cmdPtr->defPathSep);
    }
    if (Blt_ParseSwitches(interp, pathPrintSwitches, objc - 4, objv + 4, 
        &switches, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    flags = 0;
    if ((switches.pathSep != NULL) && (switches.pathSep[0] != '\0')) {
        flags = TREE_INCLUDE_ROOT;
    }
    if (switches.flags & PATH_NO_LEADING_SEPARATOR) {
        flags = 0;
    }
    Blt_Tree_NodeRelativePath(switches.root, node, switches.pathSep, flags,&ds);
    Tcl_DStringResult(interp, &ds);
    Blt_FreeSwitches(pathPrintSwitches, (char *)&switches, 0);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PathSeparatorOp --
 *
 *      Defines the default separator for path operations.
 *
 *      treeName path separator ?sepString?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathSeparatorOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;

    if (objc == 4) { 
        int length;
        const char *string;
        
        if (cmdPtr->defPathSep != NULL) {
            Blt_Free(cmdPtr->defPathSep);
            cmdPtr->defPathSep = NULL;
        }
        string = Tcl_GetStringFromObj(objv[3], &length);
        if (length > 0) {
            cmdPtr->defPathSep = Blt_AssertStrdup(string);
        }
    }
    if (cmdPtr->defPathSep != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), cmdPtr->defPathSep, -1);
    }
    return TCL_OK;
}



/*
 *---------------------------------------------------------------------------
 *
 * PathOp --
 *
 *   treeName path parse pathName ?switches ...?
 *   treeName path print nodeName ?switches ...?
 *   treeName path create pathName ?switches ...?
 *   treeName path separator ?sepString?
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec pathOps[] =
{
    {"create",     1, PathCreateOp,     4, 0, "pathName ?switches ...?"},
    {"parse",      2, PathParseOp,      4, 0, "pathName ?switches ...?"},
    {"print",      2, PathPrintOp,      4, 0, "nodeName ?switches ...?"},
    {"separator",  1, PathSeparatorOp,  3, 4, "?sepString?"},
};

static int numPathOps = sizeof(pathOps) / sizeof(Blt_OpSpec);

static int
PathOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numPathOps, pathOps, BLT_OP_ARG2, objc,
                            objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * PositionOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PositionOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_TreeNode *nodeArr, *nodePtr;
    Blt_TreeNode lastParent;
    PositionSwitches switches;
    Tcl_DString ds;
    Tcl_Obj *listObjPtr, *objPtr;
    TreeCmd *cmdPtr = clientData;
    int i, n;
    long position;

    memset((char *)&switches, 0, sizeof(switches));
    /* Process switches  */
    n = Blt_ParseSwitches(interp, positionSwitches, objc - 2, objv + 2, 
        &switches, BLT_SWITCH_OBJV_PARTIAL);
    if (n < 0) {
        return TCL_ERROR;
    }
    objc -= n + 2, objv += n + 2;

    /* Collect the node ids into an array */
    nodeArr = Blt_AssertMalloc((objc + 1) * sizeof(Blt_TreeNode));
    for (i = 0; i < objc; i++) {
        Blt_TreeNode node;

        if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[i], &node)
            != TCL_OK) {
            Blt_Free(nodeArr);
            return TCL_ERROR;
        }
        nodeArr[i] = node;
    }
    nodeArr[i] = NULL;

    if (switches.sort) {                /* Sort the nodes by depth-first
                                         * order if requested. */
        qsort((char *)nodeArr, objc, sizeof(Blt_TreeNode), 
              (QSortCompareProc *)ComparePositions);
    }

    position = 0;               /* Suppress compiler warning. */
    lastParent = NULL;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_DStringInit(&ds);
    for (nodePtr = nodeArr; *nodePtr != NULL; nodePtr++) {
        Blt_TreeNode parent;

        parent = Blt_Tree_ParentNode(*nodePtr);
        if ((parent != NULL) && (parent == lastParent)) {
            Blt_TreeNode node;

            /* 
             * Since we've sorted the nodes already, we can safely assume
             * that if two consecutive nodes have the same parent, the
             * first node came before the second. If this is the case, use
             * the last node as a starting point.
             */
            
            /*
             * Note that we start comparing from the last node, not its
             * successor.  Some one may give us the same node more than
             * once.
             */
            node = *(nodePtr - 1);      /* Can't get here unless there's
                                         * more than one node. */
            for(/*empty*/; node != NULL; node = Blt_Tree_NextSibling(node)) {
                if (node == *nodePtr) {
                    break;
                }
                position++;
            }
        } else {
            /* 
             * The fallback is to linearly search through the parent's list
             * of children, counting the number of preceding
             * siblings. Except for nodes with many siblings (100+), this
             * should be okay.
             */
            position = Blt_Tree_NodePosition(*nodePtr);
        }
        if (switches.sort) {
            lastParent = parent; /* Update the last parent. */
        }           
        /* 
         * Add an element in the form "parent -at position" to the list
         * that we're generating.
         */
        if (switches.withId) {
            objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(*nodePtr));
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        if (switches.withParent) {
            const char *string;
            
            Tcl_DStringSetLength(&ds, 0); /* Clear the string. */
            string = (parent == NULL) ? "" : Blt_Tree_NodeIdAscii(parent);
            Tcl_DStringAppendElement(&ds, string);
            Tcl_DStringAppendElement(&ds, "-at");
            Tcl_DStringAppendElement(&ds, Blt_Ltoa(position));
            objPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        } else {
            objPtr = Tcl_NewLongObj(position);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_DStringFree(&ds);
    Blt_Free(nodeArr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PreviousOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PreviousOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    node = Blt_Tree_PrevNode(NULL, node);
    inode = (node != NULL) ? Blt_Tree_NodeId(node) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*ARGSUSED*/
static int
PrevSiblingOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long inode;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    inode = -1;
    node = Blt_Tree_PrevSibling(node);
    if (node != NULL) {
        inode = Blt_Tree_NodeId(node);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReplaceOp --
 * 
 *      treeName replace destNode srcNode ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ReplaceOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode destNode, srcNode;
    TreeCmd *cmdPtr = clientData;
    
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &destNode)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &srcNode)
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (destNode == srcNode) {
        return TCL_OK;
    }
    return ReplaceNode(cmdPtr, srcNode, destNode);

}

/*
 *---------------------------------------------------------------------------
 *
 * RestoreOp --
 *
 *      tree restore node -file fileName
 *      tree restore node -data string
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RestoreOp(ClientData clientData, Tcl_Interp *interp, int objc,
          Tcl_Obj *const *objv)
{
    Blt_TreeNode root;                  /* Root node of restored
                                         * subtree. */
    RestoreInfo restore;
    TreeCmd *cmdPtr = clientData;
    int result;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &root)
        != TCL_OK) {
        return TCL_ERROR;
    }
    memset((char *)&restore, 0, sizeof(RestoreInfo));
    restore.tree = cmdPtr->tree;
    restore.root = root;
    if (Blt_ParseSwitches(interp, restoreSwitches, objc - 3, objv + 3, 
        &restore, BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if ((restore.dataObjPtr != NULL) && (restore.fileObjPtr != NULL)) {
        Tcl_AppendResult(interp, "can't set both -file and -data switches.",
                         (char *)NULL);
        Blt_FreeSwitches(restoreSwitches, (char *)&restore, 0);
        return TCL_ERROR;
    }
    Blt_InitHashTable(&restore.idTable, BLT_ONE_WORD_KEYS);
    Blt_InitHashTable(&restore.dataTable, BLT_STRING_KEYS);
    if (restore.dataObjPtr != NULL) {
        result = RestoreTreeFromData(interp, &restore);
    } else if (restore.fileObjPtr != NULL) {
        result = RestoreTreeFromFile(interp, &restore);
    } else {
        result = TCL_OK;
    }
    Blt_DeleteHashTable(&restore.idTable);
    Blt_DeleteHashTable(&restore.dataTable);
    Blt_FreeSwitches(restoreSwitches, (char *)&restore, 0);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * RootOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RootOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode root;
    TreeCmd *cmdPtr = clientData;

    root = Blt_Tree_RootNode(cmdPtr->tree);
    Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(root));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetOp --
 *
 *---------------------------------------------------------------------------
 */
static int
SetOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeIterator iter;
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[2], &iter)
        != TCL_OK) {
        return TCL_ERROR;
    }
    for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL; 
         node = Blt_Tree_NextTaggedNode(&iter)) {
        if (SetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
            return TCL_ERROR;
        }
    } 
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SizeOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SizeOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_Size(node));
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagAddOp --
 *
 *      .t tag add tagName node1 node2 node3
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;
    const char *string;
    long nodeId;

    string = Tcl_GetString(objv[3]);
    if (Blt_GetLongFromObj(NULL, objv[3], &nodeId) == TCL_OK) {
        Tcl_AppendResult(interp, "bad tag \"", string, 
                         "\": can't be a number", (char *)NULL);
        return TCL_ERROR;
    }
    if ((strcmp(string, "all") == 0) || (strcmp(string, "root") == 0)) {
        Tcl_AppendResult(cmdPtr->interp, "can't add reserved tag \"",
                         string, "\"", (char *)NULL);
        return TCL_ERROR;
    }
    if (objc == 4) {
        /* No nodes specified.  Just add the tag. */
        if (AddTag(cmdPtr, NULL, string) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        int i;

        for (i = 4; i < objc; i++) {
            Blt_TreeNode node;
            Blt_TreeIterator iter;
            
            if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[i], &iter) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL; 
                 node = Blt_Tree_NextTaggedNode(&iter)) {
                if (AddTag(cmdPtr, node, string) != TCL_OK) {
                    return TCL_ERROR;
                }
            }
        }
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TagDeleteOp --
 *
 *      .t tag delete tag node1 node2 node3
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_HashTable *tablePtr;
    TreeCmd *cmdPtr = clientData;
    const char *string;
    long nodeId;

    string = Tcl_GetString(objv[3]);
    if (Blt_GetLongFromObj(NULL, objv[3], &nodeId) == TCL_OK) {
        Tcl_AppendResult(interp, "bad tag \"", string, 
                         "\": can't be a number", (char *)NULL);
        return TCL_ERROR;
    }
    if ((strcmp(string, "all") == 0) || (strcmp(string, "root") == 0)) {
        Tcl_AppendResult(interp, "can't delete reserved tag \"", string, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    tablePtr = Blt_Tree_TagHashTable(cmdPtr->tree, string);
    if (tablePtr != NULL) {
        int i;
      
        for (i = 4; i < objc; i++) {
            Blt_TreeNode node;
            Blt_TreeIterator iter;

            if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[i], &iter) 
                != TCL_OK) {
                return TCL_ERROR;
            }
            for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL; 
                 node = Blt_Tree_NextTaggedNode(&iter)) {
                Blt_HashEntry *hPtr;

                hPtr = Blt_FindHashEntry(tablePtr, (char *)node);
                if (hPtr != NULL) {
                    Blt_DeleteHashEntry(tablePtr, hPtr);
                }
           }
       }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *      Returns the existence of the one or more tags in the given node.
 *      If the node has any the tags, true is return in the interpreter.
 *
 *      .t tag exists tag1 node
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;
    const char *tagName;
    int bool;

    tagName = Tcl_GetString(objv[3]);
    bool = (Blt_Tree_TagHashTable(cmdPtr->tree, tagName) != NULL);
    if (objc == 5) {
        Blt_TreeNode node;

        if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[4], &node)
            != TCL_OK) {
            return TCL_ERROR;
        }
        bool = Blt_Tree_HasTag(cmdPtr->tree, node, tagName);
    } 
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagForgetOp --
 *
 *      Removes the given tags from all nodes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        const char *string;
        long nodeId;

        string = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &nodeId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", string, 
                             "\": can't be a number", (char *)NULL);
            return TCL_ERROR;
        }
        Blt_Tree_ForgetTag(cmdPtr->tree, string);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagGetOp --
 *
 *      Returns tag names for a given node.  If one of more pattern
 *      arguments are provided, then only those matching tags are returned.
 *
 *      .t tag get node pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node; 
    Tcl_Obj *listObjPtr;
    TreeCmd *cmdPtr = clientData;
   
    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    if (objc == 4) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        Tcl_Obj *objPtr;

        /* Dump all tags for this node. */
        if (node == Blt_Tree_RootNode(cmdPtr->tree)) {
            objPtr = Tcl_NewStringObj("root", 4);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        for (hPtr = Blt_Tree_FirstTag(cmdPtr->tree, &iter); hPtr != NULL;
             hPtr = Blt_NextHashEntry(&iter)) {
            Blt_TreeTagEntry *tPtr;
            Blt_HashEntry *h2Ptr;

            tPtr = Blt_GetHashValue(hPtr);
            h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)node);
            if (h2Ptr != NULL) {
                objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
        objPtr = Tcl_NewStringObj("all", 3);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    } else {
        int i;
        Tcl_Obj *objPtr;

        /* Check if we need to add the special tags "all" and "root" */
        for (i = 4; i < objc; i++) {
            char *pattern;

            pattern = Tcl_GetString(objv[i]);
            if (Tcl_StringMatch("all", pattern)) {
                objPtr = Tcl_NewStringObj("all", 3);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                break;
            }
        }
        if (node == Blt_Tree_RootNode(cmdPtr->tree)) {
            for (i = 4; i < objc; i++) {
                char *pattern;

                pattern = Tcl_GetString(objv[i]);
                if (Tcl_StringMatch("root", pattern)) {
                    objPtr = Tcl_NewStringObj("root", 4);
                    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    break;
                }
            }
        }
        /* Now process any standard tags. */
        for (i = 4; i < objc; i++) {
            Blt_HashEntry *hPtr;
            Blt_HashSearch iter;
            char *pattern;

            pattern = Tcl_GetString(objv[i]);
            for (hPtr = Blt_Tree_FirstTag(cmdPtr->tree, &iter); hPtr != NULL;
                 hPtr = Blt_NextHashEntry(&iter)) {
                Blt_TreeTagEntry *tPtr;

                tPtr = Blt_GetHashValue(hPtr);
                if (Tcl_StringMatch(tPtr->tagName, pattern)) {
                    Blt_HashEntry *h2Ptr;

                    h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)node);
                    if (h2Ptr != NULL) {
                        objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
                        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
                    }
                }
            }
        }
    }    
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNamesOp --
 *
 *      Returns the names of all the tags in the tree.  If one of more node
 *      arguments are provided, then only the tags found in those nodes are
 *      returned.
 *
 *      .t tag names node node node...
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr, *objPtr;
    TreeCmd *cmdPtr = clientData;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", 3);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 3) {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;

        objPtr = Tcl_NewStringObj("root", 4);
        Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        for (hPtr = Blt_Tree_FirstTag(cmdPtr->tree, &iter); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&iter)) {
            Blt_TreeTagEntry *tPtr;

            tPtr = Blt_GetHashValue(hPtr);
            objPtr = Tcl_NewStringObj(tPtr->tagName, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    } else {
        Blt_HashTable uniqTable;
        int i;

        Blt_InitHashTable(&uniqTable, BLT_STRING_KEYS);
        for (i = 3; i < objc; i++) {
            Blt_HashEntry *hPtr;
            Blt_HashSearch iter;
            Blt_TreeNode node;
            int isNew;

            if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[i], &node)
                != TCL_OK) {
                goto error;
            }
            if (node == Blt_Tree_RootNode(cmdPtr->tree)) {
                Blt_CreateHashEntry(&uniqTable, "root", &isNew);
            }
            for (hPtr = Blt_Tree_FirstTag(cmdPtr->tree, &iter); hPtr != NULL;
                 hPtr = Blt_NextHashEntry(&iter)) {
                Blt_TreeTagEntry *tPtr;
                Blt_HashEntry *h2Ptr;

                tPtr = Blt_GetHashValue(hPtr);
                h2Ptr = Blt_FindHashEntry(&tPtr->nodeTable, (char *)node);
                if (h2Ptr != NULL) {
                    Blt_CreateHashEntry(&uniqTable, tPtr->tagName, &isNew);
                }
            }
        }
        {
            Blt_HashEntry *hPtr;
            Blt_HashSearch iter;

            for (hPtr = Blt_FirstHashEntry(&uniqTable, &iter); hPtr != NULL;
                 hPtr = Blt_NextHashEntry(&iter)) {
                objPtr = Tcl_NewStringObj(Blt_GetHashKey(&uniqTable, hPtr), -1);
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
        }
        Blt_DeleteHashTable(&uniqTable);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
 error:
    Tcl_DecrRefCount(listObjPtr);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagNodesOp --
 *
 *      Returns the node ids for the given tags.  The ids returned will
 *      represent the union of nodes for all the given tags.
 *
 *      .t nodes tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
static int
TagNodesOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_HashTable nodeTable;
    TreeCmd *cmdPtr = clientData;
    int i;
        
    Blt_InitHashTable(&nodeTable, BLT_ONE_WORD_KEYS);
    for (i = 3; i < objc; i++) {
        const char *string;
        int isNew;
        long nodeId;

        string = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &nodeId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", string, 
                             "\": can't be a number", (char *)NULL);
            goto error;
        }
        if (strcmp(string, "all") == 0) {
            break;
        } else if (strcmp(string, "root") == 0) {
            Blt_CreateHashEntry(&nodeTable, 
                (char *)Blt_Tree_RootNode(cmdPtr->tree), &isNew);
            continue;
        } else {
            Blt_HashTable *tablePtr;
            
            tablePtr = Blt_Tree_TagHashTable(cmdPtr->tree, string);
            if (tablePtr != NULL) {
                Blt_HashEntry *hPtr;
                Blt_HashSearch iter;

                for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); 
                     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
                    Blt_TreeNode node;

                    node = Blt_GetHashValue(hPtr);
                    Blt_CreateHashEntry(&nodeTable, (char *)node, &isNew);
                }
                continue;
            }
        }
        Blt_DeleteHashTable(&nodeTable);
        return TCL_OK;
    }
    {
        Blt_HashEntry *hPtr;
        Blt_HashSearch iter;
        Tcl_Obj *listObjPtr;

        listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
        for (hPtr = Blt_FirstHashEntry(&nodeTable, &iter); hPtr != NULL; 
             hPtr = Blt_NextHashEntry(&iter)) {
            Blt_TreeNode node;
            Tcl_Obj *objPtr;

            node = (Blt_TreeNode)Blt_GetHashKey(&nodeTable, hPtr);
            objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(node));
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
        Tcl_SetObjResult(interp, listObjPtr);
    }
    Blt_DeleteHashTable(&nodeTable);
    return TCL_OK;

 error:
    Blt_DeleteHashTable(&nodeTable);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagSetOp --
 *
 *      Sets one or more tags for a given node.  Tag names can't start with
 *      a digit (to distinquish them from node ids) and can't be a reserved
 *      tag ("root" or "all").
 *
 *      .t tag set node tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    int i;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        const char *string;
        long nodeId;

        string = Tcl_GetString(objv[i]);
        if (Blt_GetLongFromObj(NULL, objv[i], &nodeId) == TCL_OK) {
            Tcl_AppendResult(interp, "bad tag \"", string, 
                             "\": can't be a number", (char *)NULL);
            return TCL_ERROR;
        }
        if ((strcmp(string, "all") == 0) || (strcmp(string, "root") == 0)) {
            Tcl_AppendResult(interp, "can't add reserved tag \"", string, "\"",
                 (char *)NULL); 
            return TCL_ERROR;
        }
        if (AddTag(cmdPtr, node, string) != TCL_OK) {
            return TCL_ERROR;
        }
    }    
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagUnsetOp --
 *
 *      Removes one or more tags from a given node. If a tag doesn't exist
 *      or is a reserved tag ("root" or "all"), nothing will be done and no
 *      error message will be returned.
 *
 *      .t tag unset node tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    int i;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    for (i = 4; i < objc; i++) {
        Blt_Tree_RemoveTag(cmdPtr->tree, node, Tcl_GetString(objv[i]));
    }    
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagOp --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec tagOps[] = {
    {"add",    1, TagAddOp,    4, 0, "tag ?nodeName...?"},
    {"delete", 1, TagDeleteOp, 5, 0, "tag nodeName..."},
    {"exists", 1, TagExistsOp, 4, 5, "tag ?nodeName?"},
    {"forget", 1, TagForgetOp, 4, 0, "tag..."},
    {"get",    1, TagGetOp,    4, 0, "nodeName ?pattern...?"},
    {"names",  2, TagNamesOp,  3, 0, "?nodeName...?"},
    {"nodes",  2, TagNodesOp,  4, 0, "tag ?tag...?"},
    {"set",    1, TagSetOp,    4, 0, "nodeName tag..."},
    {"unset",  1, TagUnsetOp,  4, 0, "nodeName tag..."},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG2, objc, objv, 
        0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceCreateOp --
 *
 *      treeName trace create nodeName key rwu cmd ?switches ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TraceCreateOp(ClientData clientData, Tcl_Interp *interp, int objc, 
              Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TraceInfo *tracePtr;
    TraceSwitches switches;
    TreeCmd *cmdPtr = clientData;
    const char *key, *command;
    const char *string;
    const char *tagName;
    int flags;
    int length;
    long nodeId;

    string = Tcl_GetString(objv[3]);
    tagName = NULL;
    node = NULL;
    if (Blt_GetLongFromObj(NULL, objv[3], &nodeId) == TCL_OK) {
        if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node)
            != TCL_OK) {
            return TCL_ERROR;
        }
        tagName = NULL;
    } else {
        tagName = Blt_AssertStrdup(string);
        node = NULL;
    }
    key = Tcl_GetString(objv[4]);
    string = Tcl_GetString(objv[5]);
    flags = GetTraceFlags(string);
    if (flags < 0) {
        Tcl_AppendResult(interp, "unknown flag in \"", string, "\"", 
                     (char *)NULL);
        return TCL_ERROR;
    }
    command = Tcl_GetStringFromObj(objv[6], &length);
    /* Process switches  */
    switches.mask = 0;
    if (Blt_ParseSwitches(interp, traceSwitches, objc-7, objv+7, &switches, 
        BLT_SWITCH_DEFAULTS | BLT_SWITCH_OBJV_PARTIAL) < 0) {
        return TCL_ERROR;
    }
    /* Stash away the command in structure and pass that to the trace. */
    tracePtr = Blt_AssertCalloc(1, length + sizeof(TraceInfo));
    strcpy(tracePtr->command, command);
    tracePtr->cmdPtr = cmdPtr;
    tracePtr->withTag = tagName;
    tracePtr->node = node;
    flags |= switches.mask;
    tracePtr->traceToken = Blt_Tree_CreateTrace(cmdPtr->tree, node, key, 
        tagName, flags, TreeTraceProc, tracePtr);

    {
        Blt_HashEntry *hPtr;
        char string[200];
        int isNew;

        Blt_FormatString(string, 200, "trace%d", cmdPtr->traceCounter++);
        hPtr = Blt_CreateHashEntry(&cmdPtr->traceTable, string, &isNew);
        Blt_SetHashValue(hPtr, tracePtr);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceDeleteOp --
 *
 *      treeName trace delete ?traceId ...?
 *
 *---------------------------------------------------------------------------
 */
static int
TraceDeleteOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr = clientData;
    int i;

    for (i = 3; i < objc; i++) {
        Blt_HashEntry *hPtr;
        TraceInfo *tracePtr;
        char *key;

        key = Tcl_GetString(objv[i]);
        hPtr = Blt_FindHashEntry(&cmdPtr->traceTable, key);
        if (hPtr == NULL) {
            Tcl_AppendResult(interp, "unknown trace \"", key, "\"", 
                             (char *)NULL);
            return TCL_ERROR;
        }
        tracePtr = Blt_GetHashValue(hPtr);
        Blt_DeleteHashEntry(&cmdPtr->traceTable, hPtr); 
        Blt_Tree_DeleteTrace(tracePtr->traceToken);
        if (tracePtr->withTag != NULL) {
            Blt_Free(tracePtr->withTag);
        }
        Blt_Free(tracePtr);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceNamesOp --
 *
 *      treeName trace names ?pattern ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TraceNamesOp(ClientData clientData, Tcl_Interp *interp, int objc, 
             Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;
    TreeCmd *cmdPtr = clientData;
    
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&cmdPtr->traceTable, &iter);
         hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
        int match;
        const char *name;
        int i;

        name = Blt_GetHashKey(&cmdPtr->traceTable, hPtr);
        match = (objc == 3);
        for (i = 3; i < objc; i++) {
            char *pattern;

            pattern = Tcl_GetString(objv[i]);
            if (Tcl_StringMatch(name, pattern)) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            Tcl_Obj *objPtr;

            objPtr = Tcl_NewStringObj(name, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceInfoOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TraceInfoOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Tcl_Obj *listObjPtr, *objPtr;
    TraceInfo *tracePtr;
    TreeCmd *cmdPtr = clientData;
    char *key;
    char string[5];
    struct _Blt_TreeTrace *tokenPtr;

    key = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&cmdPtr->traceTable, key);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "unknown trace \"", key, "\"", 
                         (char *)NULL);
        return TCL_ERROR;
    }
    tracePtr = Blt_GetHashValue(hPtr);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (tracePtr->withTag != NULL) {
        objPtr = Tcl_NewStringObj(tracePtr->withTag, -1);
    } else {
        objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(tracePtr->node));
    }
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    tokenPtr = (struct _Blt_TreeTrace *)tracePtr->traceToken;
    objPtr = Tcl_NewStringObj(tokenPtr->key, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    PrintTraceFlags(tokenPtr->mask, string);
    objPtr = Tcl_NewStringObj(string, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    objPtr = Tcl_NewStringObj(tracePtr->command, -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceOp --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec traceOps[] =
{
    {"create", 1, TraceCreateOp, 7, 0, "nodeName key how command ?-whenidle?"},
    {"delete", 1, TraceDeleteOp, 3, 0, "traceName ..."},
    {"info",   1, TraceInfoOp,   4, 4, "traceName"},
    {"names",  1, TraceNamesOp,  3, 0, "?pattern ...?"},
};

static int numTraceOps = sizeof(traceOps) / sizeof(Blt_OpSpec);

static int
TraceOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTraceOps, traceOps, BLT_OP_ARG2, objc, 
        objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    result = (*proc) (clientData, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *valueObjPtr;
    TreeCmd *cmdPtr = clientData;
    const char *string;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
        != TCL_OK) {
        return TCL_ERROR;
    }
    string = Tcl_GetString(objv[3]);
    if (Blt_Tree_GetValue(interp, cmdPtr->tree, node, string, &valueObjPtr) 
        != TCL_OK) {
        return TCL_ERROR;
    }
    if (valueObjPtr->typePtr != NULL) {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), valueObjPtr->typePtr->name, 
                         -1);
    } else {
        Tcl_SetStringObj(Tcl_GetObjResult(interp), "string", 6);
    }
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * UnsetOp --
 *
 *---------------------------------------------------------------------------
 */
static int
UnsetOp(ClientData clientData, Tcl_Interp *interp, int objc,
        Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TreeCmd *cmdPtr = clientData;
    long nodeId;

    if (Blt_GetLongFromObj(NULL, objv[2], &nodeId) == TCL_OK) {
        if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node)
            != TCL_OK) {
            return TCL_ERROR;
        }
        if (UnsetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
            return TCL_ERROR;
        }
    } else {
        Blt_TreeIterator iter;

        if (Blt_Tree_GetNodeIterator(interp, cmdPtr->tree, objv[2], &iter) 
            != TCL_OK) {
            return TCL_ERROR;
        }
        for (node = Blt_Tree_FirstTaggedNode(&iter); node != NULL;
             node = Blt_Tree_NextTaggedNode(&iter)) {
            if (UnsetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SortOp --
 *  
 *---------------------------------------------------------------------------
 */
static int
SortOp(ClientData clientData, Tcl_Interp *interp, int objc,
       Tcl_Obj *const *objv)
{
    Blt_TreeNode top;
    SortSwitches switches;
    TreeCmd *cmdPtr = clientData;
    int result;

    if (Blt_Tree_GetNodeFromObj(interp, cmdPtr->tree, objv[2], &top)
        != TCL_OK) {
        return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    switches.cmdPtr = cmdPtr;
    if (Blt_ParseSwitches(interp, sortSwitches, objc - 3, objv + 3, &switches, 
        BLT_SWITCH_DEFAULTS) < 0) {
        return TCL_ERROR;
    }
    if (switches.command != NULL) {
        switches.type = SORT_COMMAND;
    }
    switches.cmdPtr = cmdPtr;
    sortData = switches;
    if (switches.mode == SORT_FLAT) {
        Blt_TreeNode *nodeArr;
        long numNodes;

        if (switches.flags & SORT_RECURSE) {
            numNodes = Blt_Tree_Size(top);
        } else {
            numNodes = Blt_Tree_NodeDegree(top);
        }
        nodeArr = Blt_AssertMalloc(numNodes * sizeof(Blt_TreeNode));
        if (switches.flags & SORT_RECURSE) {
            Blt_TreeNode node, *p;

            for(p = nodeArr, node = top; node != NULL; 
                node = Blt_Tree_NextNode(top, node)) {
                *p++ = node;
            }
        } else {
            Blt_TreeNode node, *p;

            for (p = nodeArr, node = Blt_Tree_FirstChild(top); node != NULL;
                 node = Blt_Tree_NextSibling(node)) {
                *p++ = node;
            }
        }
        qsort((char *)nodeArr, numNodes, sizeof(Blt_TreeNode),
              (QSortCompareProc *)CompareNodes);
        {
            Tcl_Obj *listObjPtr;
            Blt_TreeNode *p;
            long i;

            listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
            for (p = nodeArr, i = 0; i < numNodes; i++, p++) {
                Tcl_Obj *objPtr;

                objPtr = Tcl_NewLongObj(Blt_Tree_NodeId(*p));
                Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
            }
            Tcl_SetObjResult(interp, listObjPtr);
        }
        Blt_Free(nodeArr);
        result = TCL_OK;
    } else {
        if (switches.flags & SORT_RECURSE) {
            result = Blt_Tree_Apply(top, SortApplyProc, cmdPtr);
        } else {
            result = SortApplyProc(top, cmdPtr, TREE_PREORDER);
        }
    }
    Blt_FreeSwitches(sortSwitches, (char *)&switches, 0);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeInstObjCmd --
 *
 *      This procedure is invoked to process commands on behalf of the tree
 *      object.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec treeOps[] =
{
    {"ancestor",    2, AncestorOp,    4, 4, "node1 node2"},
    {"append",      4, AppendOp,      4, 0, "nodeName key ?value ...?"},
    {"apply",       4, ApplyOp,       3, 0, "nodeName ?switches ...?"},
    {"attach",      2, AttachOp,      3, 0, "treeName ?switches ...?"},
    {"children",    2, ChildrenOp,    3, 5, "nodeName ?first? ?last?"},
    {"copy",        2, CopyOp,        4, 0, "parentNode ?treeName? nodeName ?switches ...?"},
    {"degree",      3, DegreeOp,      3, 0, "nodeName"},
    {"delete",      3, DeleteOp,      2, 0, "?nodeName ...?"},
    {"depth",       3, DepthOp,       2, 3, "?nodeName?"},
    {"dir",         2, DirOp,         4, 0, "nodeName path ?switches ...?"},
    {"dump",        3, DumpOp,        3, 0, "nodeName ?switches ...?"},
    {"dup",         3, DupOp,         2, 3, "nodeName"},
    {"exists",      3, ExistsOp,      3, 4, "nodeName ?fileName?"},
    {"export",      3, ExportOp,      2, 0, "formatName ?switches ...?"},
    {"find",        4, FindOp,        3, 0, "nodeName ?switches ...?"},
    {"findchild",   5, FindChildOp,   4, 4, "nodeName label"},
    {"firstchild",  3, FirstChildOp,  3, 3, "nodeName"},
    {"get",         1, GetOp,         3, 5, "nodeName ?fieldName? ?defValue?"},
    {"import",      2, ImportOp,      2, 0, "formatName ?switches ...?"},
    {"index",       3, IndexOp,       3, 3, "label|list"},
    {"insert",      3, InsertOp,      3, 0, "parentNode ?switches ...?"},
    {"isancestor",  3, IsAncestorOp,  4, 4, "node1 node2"},
    {"isbefore",    3, IsBeforeOp,    4, 4, "node1 node2"},
    {"isleaf",      3, IsLeafOp,      3, 3, "nodeName"},
    {"isroot",      3, IsRootOp,      3, 3, "nodeName"},
    {"keys",        1, KeysOp,        3, 0, "nodeName ?nodeName...?"},
    {"label",       3, LabelOp,       3, 4, "nodeName ?newLabel?"},
    {"lappend",     3, ListAppendOp,  4, 0, "nodeName fieldName ?value ...?"},
    {"lastchild",   3, LastChildOp,   3, 3, "nodeName"},
    {"lindex",      2, ListIndexOp,   5, 5, "nodeName fieldName indexNum"},
    {"llength",     2, ListLengthOp,  4, 4, "nodeName fieldName"},
    {"move",        1, MoveOp,        4, 0, "nodeName destNode ?switches ...?"},
    {"names",       2, NamesOp,       3, 4, "nodeName ?fieldName?"},
    {"next",        4, NextOp,        3, 3, "nodeName"},
    {"nextsibling", 5, NextSiblingOp, 3, 3, "nodeName"},
    {"notify",      2, NotifyOp,      2, 0, "args ..."},
    {"parent",      3, ParentOp,      3, 3, "nodeName"},
    {"path",        3, PathOp,        3, 0, "?args ...?"},
    {"position",    2, PositionOp,    3, 0, "?switches ...? nodeName..."},
    {"previous",    5, PreviousOp,    3, 3, "nodeName"},
    {"prevsibling", 5, PrevSiblingOp, 3, 3, "nodeName"},
    {"replace",     3, ReplaceOp,     4, 4, "nodeName destNode"},
    {"restore",     3, RestoreOp,     3, 0, "nodeName ?switches ...?"},
    {"root",        2, RootOp,        2, 2, ""},
    {"set",         2, SetOp,         3, 0, "nodeName ?fieldName value ...?"},
    {"size",        2, SizeOp,        3, 3, "nodeName"},
    {"sort",        2, SortOp,        3, 0, "nodeName ?switches ...?"},
    {"tag",         2, TagOp,         3, 0, "args ..."},
    {"trace",       2, TraceOp,       2, 0, "args ..."},
    {"type",        2, TypeOp,        4, 4, "nodeName fieldName"},
    {"unset",       1, UnsetOp,       3, 0, "nodeName ?fieldName ...?"},
};

static int numTreeOps = sizeof(treeOps) / sizeof(Blt_OpSpec);

static int
TreeInstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
               Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;
    TreeCmd *cmdPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numTreeOps, treeOps, BLT_OP_ARG1, objc,
                            objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    Tcl_Preserve(cmdPtr);
    result = (*proc) (clientData, interp, objc, objv);
    Tcl_Release(cmdPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeInstDeleteProc --
 *
 *      Deletes the command associated with the tree.  This is called only
 *      when the command associated with the tree is destroyed.
 *
 * Results:
 *      None.
 *
 *---------------------------------------------------------------------------
 */
static void
TreeInstDeleteProc(ClientData clientData)
{
    TreeCmd *cmdPtr = clientData;

    ReleaseTreeObject(cmdPtr);
    if (cmdPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(cmdPtr->tablePtr, cmdPtr->hashPtr);
    }
    Blt_Chain_Destroy(cmdPtr->notifiers);
    Blt_DeleteHashTable(&cmdPtr->notifyTable);
    Blt_DeleteHashTable(&cmdPtr->traceTable);
    if (cmdPtr->defPathSep != NULL) {
        Blt_Free(cmdPtr->defPathSep);
    }
    Blt_Free(cmdPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeCreateOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeCreateOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    const char *name;
    TreeCmd *cmdPtr;

    name = NULL;
    if (objc == 3) {
        name = Tcl_GetString(objv[2]);
    }
    cmdPtr = CreateTreeCmd(clientData, interp, name);
    if (cmdPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeDestroyOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeDestroyOp(ClientData clientData, Tcl_Interp *interp, int objc,
              Tcl_Obj *const *objv)
{
    TreeCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        TreeCmd *cmdPtr;
        char *string;

        string = Tcl_GetString(objv[i]);
        cmdPtr = GetTreeCmd(dataPtr, interp, string);
        if (cmdPtr == NULL) {
            Tcl_AppendResult(interp, "can't find a tree named \"", string,
                             "\"", (char *)NULL);
            return TCL_ERROR;
        }
        Tcl_DeleteCommandFromToken(interp, cmdPtr->cmdToken);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeExistsOp --
 *
 *      blt::tree exists treeName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    TreeCmd *cmdPtr;
    TreeCmdInterpData *dataPtr = clientData;
    const char *string;
    int state;
    
    string = Tcl_GetString(objv[3]);
    cmdPtr = GetTreeCmd(dataPtr, interp, string);
    state = (cmdPtr != NULL);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeNamesOp --
 *
 *      blt::tree names ?pattern ...?
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_DString ds;
    Tcl_Obj *listObjPtr;
    TreeCmdInterpData *dataPtr = clientData;

    Tcl_DStringInit(&ds);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->treeTable, &iter); hPtr != NULL; 
        hPtr = Blt_NextHashEntry(&iter)) {
        Blt_ObjectName objName;
        TreeCmd *cmdPtr;
        const char *qualName;
        Tcl_Obj *objPtr;
        int match;
        int i;
        
        cmdPtr = Blt_GetHashValue(hPtr);
        objName.name = Tcl_GetCommandName(interp, cmdPtr->cmdToken);
        objName.nsPtr = Blt_GetCommandNamespace(cmdPtr->cmdToken);
        qualName = Blt_MakeQualifiedName(&objName, &ds);
        match = (objc == 2);
        for (i = 2; i < objc; i++) {
            if (Tcl_StringMatch(qualName, Tcl_GetString(objv[i]))) {
                match = TRUE;
                break;
            }
        }
        if (match) {
            objPtr = Tcl_NewStringObj(qualName, -1);
            Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
        }
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*ARGSUSED*/
static int
TreeLoadOp(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Tcl_DString libName;
    TreeCmdInterpData *dataPtr = clientData;
    char *fmt, *initProcName, *safeProcName;
    int length, result;

    fmt = Tcl_GetStringFromObj(objv[2], &length);
    hPtr = Blt_FindHashEntry(&dataPtr->fmtTable, fmt);
    if (hPtr != NULL) {
        return TCL_OK;                  /* Converter for format is already
                                         * loaded. */
    }
    Tcl_DStringInit(&libName);
    {
        Tcl_DString pathName;
        const char *path;

        Tcl_DStringInit(&pathName);
        path = Tcl_TranslateFileName(interp, Tcl_GetString(objv[3]), &pathName);
        if (path == NULL) {
            Tcl_DStringFree(&pathName);
            return TCL_ERROR;
        }
        Tcl_DStringAppend(&libName, path, -1);
        Tcl_DStringFree(&pathName);
    }
    Tcl_DStringAppend(&libName, "/", -1);
    Tcl_UtfToTitle(fmt);
    Tcl_DStringAppend(&libName, "Tree", 4);
    Tcl_DStringAppend(&libName, fmt, -1);
    Tcl_DStringAppend(&libName, Blt_Itoa(BLT_MAJOR_VERSION), 1);
    Tcl_DStringAppend(&libName, Blt_Itoa(BLT_MINOR_VERSION), 1);
    Tcl_DStringAppend(&libName, BLT_LIB_SUFFIX, -1);
    Tcl_DStringAppend(&libName, BLT_SO_EXT, -1);

    initProcName = Blt_AssertMalloc(8 + length + 4 + 1);
    Blt_FormatString(initProcName, 8 + length + 4 + 1, "Blt_Tree%sInit", fmt);
    safeProcName = Blt_AssertMalloc(8 + length + 8 + 1);
    Blt_FormatString(safeProcName, 8 + length + 8 + 1, "Blt_Tree%sSafeInit",
                     fmt);
    result = Blt_LoadLibrary(interp, Tcl_DStringValue(&libName), initProcName, 
        safeProcName); 
    Tcl_DStringFree(&libName);
    if (safeProcName != NULL) {
        Blt_Free(safeProcName);
    }
    if (initProcName != NULL) {
        Blt_Free(initProcName);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeObjCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec treeCmdOps[] =
{
    {"create",  1, TreeCreateOp,  2, 3, "?treeName?"},
    {"destroy", 1, TreeDestroyOp, 2, 0, "?treeName ...?"},
    {"exists",  1, TreeExistsOp,  3, 3, "treeName"},
    {"load",    1, TreeLoadOp,    4, 4, "treeName libpath"},
    {"names",   1, TreeNamesOp,   2, 3, "?pattern ...?"},
};

static int numCmdOps = sizeof(treeCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
TreeObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numCmdOps, treeCmdOps, BLT_OP_ARG1, objc, 
        objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the "tree" command
 *      is deleted.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Removes the hash table managing all tree names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
TreeInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    TreeCmdInterpData *dataPtr = clientData;

    /* 
     * All tree instances should already have been destroyed when their
     * respective TCL commands were deleted.
     */
    Blt_DeleteHashTable(&dataPtr->treeTable);
    Tcl_DeleteAssocData(interp, TREE_THREAD_KEY);
    Blt_Free(dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TreeCmdInitProc --
 *
 *      This procedure is invoked to initialize the "tree" command.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Creates the new command and adds a new entry into a global Tcl
 *      associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_TreeCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
        "tree", TreeObjCmd, 
    };
    cmdSpec.clientData = GetTreeCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}

/* Dump tree to dbm */
/* Convert node data to datablock */

int
Blt_Tree_RegisterFormat(Tcl_Interp *interp, const char *fmt,
    Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc)
{
    Blt_HashEntry *hPtr;
    DataFormat *fmtPtr;
    TreeCmdInterpData *dataPtr;
    int isNew;

    dataPtr = GetTreeCmdInterpData(interp);
    hPtr = Blt_CreateHashEntry(&dataPtr->fmtTable, fmt, &isNew);
    if (isNew) {
        fmtPtr = Blt_AssertMalloc(sizeof(DataFormat));
        fmtPtr->name = Blt_AssertStrdup(fmt);
        Blt_SetHashValue(hPtr, fmtPtr);
    } else {
        fmtPtr = Blt_GetHashValue(hPtr);
    }
    fmtPtr->isLoaded = TRUE;
    fmtPtr->importProc = importProc;
    fmtPtr->exportProc = exportProc;
    return TCL_OK;
}

#endif /* NO_TREE */
