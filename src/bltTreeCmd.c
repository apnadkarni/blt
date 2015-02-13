/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltTreeCmd.c --
 *
 *	Copyright 1998-2004 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person obtaining
 *	a copy of this software and associated documentation files (the
 *	"Software"), to deal in the Software without restriction, including
 *	without limitation the rights to use, copy, modify, merge, publish,
 *	distribute, sublicense, and/or sell copies of the Software, and to
 *	permit persons to whom the Software is furnished to do so, subject to
 *	the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *	LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *	OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
  tree create t0 t1 t2
  tree names
  t0 destroy
     -or-
  tree destroy t0
  tree copy tree@node tree@node -recurse -tags

  tree move node after|before|into t2@node

  $t apply -recurse $root command arg arg			

  $t attach treename				

  $t children $n
  t0 copy node1 node2 node3 node4 node5 destName 
  $t delete $n...				
  $t depth $n
  $t dump $root
  $t dumpfile $root fileName
  $t dup $t2		
  $t find $root -name pat -name pattern
  $t firstchild $n
  $t get $n $key
  $t get $n $key(abc)
  $t index $n
  $t insert $parent $switches?
  $t isancestor $n1 $n2
  $t isbefore $n1 $n2
  $t isleaf $n
  $t lastchild $n
  $t move $n1 after|before|into $n2
  $t next $n
  $t nextsibling $n
  $t path $n1 $n2 $n3...
  $t parent $n
  $t previous $n
  $t prevsibling $n
  $t restore $root -data data -overwrite
  $t root ?$n?

  $t set $n $key $value ?$key $value?
  $t size $n
  $t slink $n $t2@$node				???
  $t sort -recurse $root		

  $t tag delete tag1 tag2 tag3...
  $t tag names
  $t tag nodes $tag
  $t tag set $n tag1 tag2 tag3...
  $t tag unset $n tag1 tag2 tag3...

  $t trace create $n $key how -whenidle command 
  $t trace delete id1 id2 id3...
  $t trace names
  $t trace info $id

  $t unset $n key1 key2 key3...
  
  $t notify create -oncreate -ondelete -onmove command 
  $t notify create -oncreate -ondelete -onmove -onsort command arg arg arg 
  $t notify delete id1 id2 id3
  $t notify names
  $t notify info id

  for { set n [$t firstchild $node] } { $n >= 0 } { 
        set n [$t nextsibling $n] } {
  }
  foreach n [$t children $node] { 
	  
  }
  set n [$t next $node]
  set n [$t previous $node]

*/

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#ifndef NO_TREE

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif	/* HAVE_SYS_STAT_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
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
    const char *name;			/* Name of format. */
    int isLoaded;			
    Blt_TreeImportProc *importProc;
    Blt_TreeExportProc *exportProc;
} DataFormat;

typedef struct {
    Tcl_Interp *interp;
    Blt_HashTable treeTable;		/* Hash table of trees keyed by
					 * address. */
    Blt_HashTable fmtTable;
} TreeCmdInterpData;

typedef struct {
    Tcl_Interp *interp;
    Tcl_Command cmdToken;		/* Token for tree's TCL command. */
    Blt_Tree tree;			/* Token holding internal tree. */
    Blt_HashEntry *hashPtr;
    Blt_HashTable *tablePtr;
    TreeCmdInterpData *tdPtr;		/*  */
    int traceCounter;			/* Used to generate trace id
					 * strings.  */
    Blt_HashTable traceTable;		/* Table of active traces. Maps trace
					 * ids back to their TraceInfo
					 * records. */
    int notifyCounter;			/* Used to generate notify id
					 * strings. */
    Blt_HashTable notifyTable;		/* Table of event handlers. Maps
					 * notify ids back to their Notifier
					 * records. */
    Blt_Chain notifiers;
    Tcl_Obj *defPathSepObjPtr;          /* Path separator. */
} TreeCmd;

typedef struct {
    TreeCmd *cmdPtr;
    Blt_TreeNode node;
    Blt_TreeTrace traceToken;
    const char *withTag;		/* If non-NULL, the event or trace was
					 * specified with this tag.  This
					 * value is saved for informational
					 * purposes.  The tree's trace
					 * matching routines do the real
					 * checking, not the client's
					 * callback.  */
    char command[1];			/* Command prefix for the trace or
					 * notify TCL callback.  Extra
					 * arguments will be appended to the
					 * end. Extra space will be allocated
					 * for the length of the string */
} TraceInfo;

typedef struct {
    TreeCmd *cmdPtr;
    int mask;				/* Requested event mask. */
    long inode;				/* If >= 0, inode to match.  */
    const char *tag;			/* If non-NULL, tag to match. */
    Tcl_Obj *cmdObjPtr;			/* Command to be executed when
					 * matching event is found. */
    Blt_TreeNode node;			/* (out) Node affected by event. */
    Blt_TreeTrace notifyToken;

    Blt_HashEntry *hashPtr;		/* Pointer to entry in hash table. */
    Blt_ChainLink link;			/* Pointer to entry in list of
					 * notifiers. */
} Notifier;

BLT_EXTERN Blt_SwitchParseProc Blt_TreeNodeSwitchProc;
static Blt_SwitchCustom nodeSwitch = {
    Blt_TreeNodeSwitchProc, NULL, NULL, (ClientData)0,
};

typedef struct {
    int mask;
} AttachSwitches;

static Blt_SwitchSpec attachSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-newtags", "", (char *)NULL,
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
    {BLT_SWITCH_BITMASK, "-create", "",  (char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_CREATE},
    {BLT_SWITCH_BITMASK, "-delete", "",  (char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_DELETE},
    {BLT_SWITCH_BITMASK, "-move", "",	(char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_MOVE},
    {BLT_SWITCH_BITMASK, "-sort", "",	(char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_SORT},
    {BLT_SWITCH_BITMASK, "-relabel", "", (char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_RELABEL},
    {BLT_SWITCH_BITMASK, "-allevents", "", (char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_ALL},
    {BLT_SWITCH_BITMASK, "-whenidle", "", (char *)NULL,
	Blt_Offset(NotifySwitches, mask), 0, TREE_NOTIFY_WHENIDLE},
    {BLT_SWITCH_CUSTOM,    "-node",  "node", (char *)NULL,
	Blt_Offset(NotifySwitches, node),    0, 0, &nodeSwitch},
    {BLT_SWITCH_STRING, "-tag", "string", (char *)NULL,
	Blt_Offset(NotifySwitches, tag), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc ChildSwitch;
#define INSERT_BEFORE	(ClientData)0
#define INSERT_AFTER	(ClientData)1
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
    unsigned int perm, type;		/* Indicate the permission and type of
					 * directory entries to search for.*/
    unsigned int mask;			/* Indicates which fields to copy into
					 * the tree node as data. */
    Tcl_Obj *patternsObjPtr;		/* If non-NULL, is a list of patterns
					 * to match contents of each
					 * directory.  */
    unsigned int flags;
} DirSwitches;

#define DIR_TYPE	(1<<0)
#define DIR_MODE	(1<<1)
#define DIR_SIZE	(1<<2)
#define DIR_UID		(1<<3)
#define DIR_GID		(1<<4)
#define DIR_ATIME	(1<<5)
#define DIR_CTIME	(1<<6)
#define DIR_MTIME	(1<<7)
#define DIR_NLINK	(1<<8)
#define DIR_DEV		(1<<9)
#define DIR_INO		(1<<10)
#define DIR_ALL		(DIR_ATIME|DIR_CTIME|DIR_MTIME|DIR_UID|DIR_GID|\
			 DIR_TYPE|DIR_MODE|DIR_SIZE|DIR_NLINK|DIR_DEV|DIR_INO)

#define DIR_DEFAULT	(DIR_MTIME|DIR_TYPE|DIR_MODE|DIR_SIZE)
#define DIR_RECURSE	(1<<11)
#define DIR_NOCASE	(1<<12)

#define DIR_TYPE_FILE	(1<<0)
#define DIR_TYPE_DIR	(1<<1)
#define DIR_TYPE_LINK	(1<<2)
#define DIR_TYPE_SOCK	(1<<3)
#define DIR_TYPE_BLOCK	(1<<4)
#define DIR_TYPE_CHAR	(1<<5)

static Blt_SwitchParseProc FieldsSwitchProc;
static Blt_SwitchCustom fieldsSwitch = {
    FieldsSwitchProc, NULL, NULL, (ClientData)0,
};
static Blt_SwitchParseProc TypeSwitchProc;
static Blt_SwitchCustom typeSwitch = {
    TypeSwitchProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec dirSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-fields",  "list", (char *)NULL,
	Blt_Offset(DirSwitches, mask),    0, 0, &fieldsSwitch},
    {BLT_SWITCH_BITMASK, "-readable", "", (char *)NULL,
	Blt_Offset(DirSwitches, perm), 0, TCL_GLOB_PERM_R},
    {BLT_SWITCH_BITMASK, "-readonly", "", (char *)NULL,
	Blt_Offset(DirSwitches, perm), 0, TCL_GLOB_PERM_RONLY},
    {BLT_SWITCH_BITMASK, "-writable", "", (char *)NULL,
	Blt_Offset(DirSwitches, perm), 0, TCL_GLOB_PERM_W},
    {BLT_SWITCH_BITMASK, "-executable", "", (char *)NULL,
	Blt_Offset(DirSwitches, perm), 0, TCL_GLOB_PERM_X},
    {BLT_SWITCH_BITMASK, "-file", "", (char *)NULL,
	Blt_Offset(DirSwitches, type), 0, TCL_GLOB_TYPE_FILE},
    {BLT_SWITCH_BITMASK, "-directory", "", (char *)NULL,
	Blt_Offset(DirSwitches, type), 0, TCL_GLOB_TYPE_DIR},
    {BLT_SWITCH_BITMASK, "-link", "", (char *)NULL,
	Blt_Offset(DirSwitches, type), 0, TCL_GLOB_TYPE_LINK},
    {BLT_SWITCH_OBJ, "-patterns",     "list", (char *)NULL,
	Blt_Offset(DirSwitches, patternsObjPtr), 0},
    {BLT_SWITCH_BITMASK, "-nocase",       "", (char *)NULL,
        Blt_Offset(DirSwitches, flags), 0, DIR_NOCASE},
    {BLT_SWITCH_BITMASK, "-recurse", "", (char *)NULL,
	Blt_Offset(DirSwitches, flags), 0, DIR_RECURSE},
    {BLT_SWITCH_CUSTOM,  "-type",    "list", (char *)NULL,
	Blt_Offset(DirSwitches, type),    0, 0, &typeSwitch},
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
    {BLT_SWITCH_LIST, "-data", "{name value ?name value?...}", (char *)NULL,
	Blt_Offset(InsertSwitches, dataPairs), 0},
    {BLT_SWITCH_STRING, "-label", "string", (char *)NULL,
	Blt_Offset(InsertSwitches, label), 0},
    {BLT_SWITCH_LONG_NNEG, "-node", "number", (char *)NULL,
	Blt_Offset(InsertSwitches, inode), 0},
    {BLT_SWITCH_OBJ, "-tags", "{?tagName?...}", (char *)NULL,
	Blt_Offset(InsertSwitches, tagsObjPtr), 0},
    {BLT_SWITCH_END}
};

#define MATCH_INVERT		(1<<8)
#define MATCH_LEAFONLY		(1<<4)
#define MATCH_NOCASE		(1<<5)
#define MATCH_PATHNAME		(1<<6)
#define PATTERN_NONE		(0)
#define PATTERN_EXACT		(1)
#define PATTERN_GLOB		(2)
#define PATTERN_REGEXP		(3)
#define PATTERN_MASK		(0x3)

typedef struct {
    TreeCmd *cmdPtr;			/* Tree to examine. */
    Tcl_Obj *listObjPtr;		/* List to accumulate the indices of
					 * matching nodes. */
    Tcl_Obj *cmdObjPtr;			/* If non-NULL, command to be executed
					 * for each found node. */
    unsigned int flags;			/* See flags definitions above. */
    size_t numMatches;			/* Current number of matches. */
    size_t maxMatches;			/* If > 0, stop after this many
					 * matches. */
    unsigned int order;			/* Order of search: Can be either
					 * TREE_PREORDER, TREE_POSTORDER,
					 * TREE_INORDER, TREE_BREADTHFIRST. */
    long maxDepth;			/* If >= 0, don't descend more than
					 * this many levels. */
    Blt_List patternList;		/* List of patterns to compare with
					 * labels or values.  */
    const char *addTag;			/* If non-NULL, tag added to selected
					 * nodes. */
    Blt_List keyList;			/* List of key name patterns. */
    Blt_List tagList;			/* List of tag names. */
    Blt_HashTable excludeTable;		/* Table of nodes to exclude. */
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
    {BLT_SWITCH_CUSTOM, "-excludes", "nodes", (char *)NULL,
	Blt_Offset(FindSwitches, excludeTable),0, 0, &nodesSwitch},
    {BLT_SWITCH_OBJ, "-exec", "command", (char *)NULL,
        Blt_Offset(FindSwitches, cmdObjPtr),    0}, 
    {BLT_SWITCH_CUSTOM, "-glob", "pattern", (char *)NULL,
	Blt_Offset(FindSwitches, patternList),0, 0, &globSwitch},
    {BLT_SWITCH_BITMASK, "-invert", "", (char *)NULL,
	Blt_Offset(FindSwitches, flags), 0, MATCH_INVERT},
    {BLT_SWITCH_CUSTOM, "-key", "string",  (char *)NULL,
	Blt_Offset(FindSwitches, keyList),    0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyexact", "string", (char *)NULL,
	Blt_Offset(FindSwitches, keyList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyglob", "pattern", (char *)NULL,
	Blt_Offset(FindSwitches, keyList),    0, 0, &globSwitch},
    {BLT_SWITCH_CUSTOM, "-keyregexp","pattern", (char *)NULL,
	Blt_Offset(FindSwitches, keyList), 0, 0, &regexpSwitch},
    {BLT_SWITCH_BITMASK, "-leafonly", "", (char *)NULL,
	Blt_Offset(FindSwitches, flags), 0, MATCH_LEAFONLY},
    {BLT_SWITCH_BITMASK, "-nocase", "", (char *)NULL,
	Blt_Offset(FindSwitches, flags), 0, MATCH_NOCASE},
    {BLT_SWITCH_CUSTOM, "-order", "order", (char *)NULL,
	Blt_Offset(FindSwitches, order), 0, 0, &orderSwitch},
    {BLT_SWITCH_BITMASK, "-path", "", (char *)NULL,
	Blt_Offset(FindSwitches, flags), 0, MATCH_PATHNAME},
    {BLT_SWITCH_CUSTOM, "-regexp", "pattern",  (char *)NULL,
	Blt_Offset(FindSwitches, patternList),0, 0, &regexpSwitch},
    {BLT_SWITCH_CUSTOM, "-tag",	"{?tag?...}", (char *)NULL,
	Blt_Offset(FindSwitches, tagList), 0, 0, &tagSwitch},
    {BLT_SWITCH_END}
};
#undef _off


typedef struct {
    TreeCmd *cmdPtr;			/* Tree to move nodes. */
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
#undef _off

typedef struct {
    Blt_TreeNode srcNode;
    Blt_Tree srcTree, destTree;
    TreeCmd *srcPtr, *destPtr;
    const char *label;
    unsigned int flags;
} CopySwitches;

#define COPY_RECURSE	(1<<0)
#define COPY_TAGS	(1<<1)
#define COPY_OVERWRITE	(1<<2)

static Blt_SwitchSpec copySwitches[] = 
{
    {BLT_SWITCH_STRING, "-label", "string", (char *)NULL,
	Blt_Offset(CopySwitches, label), 0},
    {BLT_SWITCH_BITMASK, "-recurse", "", (char *)NULL,
	Blt_Offset(CopySwitches, flags), 0, COPY_RECURSE},
    {BLT_SWITCH_BITMASK, "-tags", "", (char *)NULL,
	Blt_Offset(CopySwitches, flags), 0, COPY_TAGS},
    {BLT_SWITCH_BITMASK, "-overwrite", "", (char *)NULL,
	Blt_Offset(CopySwitches, flags), 0, COPY_OVERWRITE},
    {BLT_SWITCH_END}
};

typedef struct {
    TreeCmd *cmdPtr;			/* Tree to examine. */
    unsigned int flags;			/* See flags definitions above. */

    long maxDepth;			/* If >= 0, don't descend more than
					 * this many levels. */
    /* String options. */
    Blt_List patternList;		/* List of label or value patterns. */
    Tcl_Obj *preCmdObjPtr;		/* Pre-command. */
    Tcl_Obj *postCmdObjPtr;		/* Post-command. */
    Blt_List keyList;			/* List of key-name patterns. */
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
    {BLT_SWITCH_BITMASK, "-invert", "", (char *)NULL,
	Blt_Offset(ApplySwitches, flags), 0, MATCH_INVERT},
    {BLT_SWITCH_CUSTOM, "-key", "pattern", (char *)NULL,
	Blt_Offset(ApplySwitches, keyList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyexact", "string", (char *)NULL,
	Blt_Offset(ApplySwitches, keyList), 0, 0, &exactSwitch},
    {BLT_SWITCH_CUSTOM, "-keyglob", "pattern", (char *)NULL,
	Blt_Offset(ApplySwitches, keyList), 0, 0, &globSwitch},
    {BLT_SWITCH_CUSTOM, "-keyregexp", "pattern", (char *)NULL,
	Blt_Offset(ApplySwitches, keyList), 0, 0, &regexpSwitch},
    {BLT_SWITCH_BITMASK, "-leafonly", "", (char *)NULL,
	Blt_Offset(ApplySwitches, flags), 0, MATCH_LEAFONLY},
    {BLT_SWITCH_BITMASK, "-nocase", "", (char *)NULL,
	Blt_Offset(ApplySwitches, flags), 0, MATCH_NOCASE},
    {BLT_SWITCH_BITMASK, "-path", "", (char *)NULL,
	Blt_Offset(ApplySwitches, flags), 0, MATCH_PATHNAME},
    {BLT_SWITCH_CUSTOM, "-regexp", "pattern", (char *)NULL,
	Blt_Offset(ApplySwitches, patternList), 0, 0, &regexpSwitch},
    {BLT_SWITCH_CUSTOM, "-tag", "{?tag?...}", (char *)NULL,
	Blt_Offset(ApplySwitches, tagList), 0, 0, &tagSwitch},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;
    Tcl_Obj *fileObjPtr;
    Tcl_Obj *dataObjPtr;
} RestoreSwitches;


#define RESTORE_NO_TAGS		(1<<0)
#define RESTORE_OVERWRITE	(1<<1)

static Blt_SwitchSpec restoreSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
	Blt_Offset(RestoreSwitches, dataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
	Blt_Offset(RestoreSwitches, fileObjPtr), 0, 0},
    {BLT_SWITCH_BITMASK, "-notags", "", (char *)NULL,
	Blt_Offset(RestoreSwitches, flags), 0, RESTORE_NO_TAGS},
    {BLT_SWITCH_BITMASK, "-overwrite", "", (char *)NULL,
	Blt_Offset(RestoreSwitches, flags), 0, RESTORE_OVERWRITE},
    {BLT_SWITCH_END}
};

typedef struct {
    Tcl_Obj *fileObjPtr;
    Tcl_Obj *dataObjPtr;
} DumpSwitches;

static Blt_SwitchSpec dumpSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-data", "data", (char *)NULL,
	Blt_Offset(DumpSwitches, dataObjPtr), 0, 0},
    {BLT_SWITCH_OBJ, "-file", "fileName", (char *)NULL,
	Blt_Offset(DumpSwitches, fileObjPtr), 0, 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc FormatSwitch;
static Blt_SwitchCustom formatSwitch =
{
    FormatSwitch, NULL, NULL, (ClientData)0,
};

typedef struct {
    int sort;				/* If non-zero, sort the nodes.  */
    int withParent;			/* If non-zero, add the parent node id
					 * to the output of the command.*/
    int withId;				/* If non-zero, echo the node id in
					 * the output of the command. */
} PositionSwitches;

#define POSITION_SORTED		(1<<0)

static Blt_SwitchSpec positionSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-sort", "",  (char *)NULL,
	Blt_Offset(PositionSwitches, sort), 0, POSITION_SORTED},
    {BLT_SWITCH_CUSTOM, "-format", "format",  (char *)NULL,
        0, 0, 0, &formatSwitch},
    {BLT_SWITCH_END}
};

#define PATH_PARENTS		(1<<1)
#define PATH_NOCOMPLAIN		(1<<2)

typedef struct {
    unsigned int flags;			/* Parse flags. */
    Tcl_Obj *pathSepObjPtr;             /* Path separator. */
    Blt_TreeNode root;			/* Starting node of path. */
} PathCreateSwitches;

static Blt_SwitchSpec pathCreateSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-from",  "node", (char *)NULL,
        Blt_Offset(PathCreateSwitches, root),  0, 0, &nodeSwitch},
    {BLT_SWITCH_BITMASK, "-nocomplain", "", (char *)NULL,
	Blt_Offset(PathCreateSwitches, flags), 0, PATH_NOCOMPLAIN},
    {BLT_SWITCH_BITMASK, "-parents", "", (char *)NULL,
	Blt_Offset(PathCreateSwitches, flags), 0, PATH_PARENTS},
    {BLT_SWITCH_OBJ, "-separator", "char", (char *)NULL,
	Blt_Offset(PathCreateSwitches, pathSepObjPtr), 0}, 
    {BLT_SWITCH_END}
};

typedef struct {
    Tcl_Obj *pathSepObjPtr;		/* Path separator. */
} PathPrintSwitches;

static Blt_SwitchSpec pathPrintSwitches[] = 
{
    {BLT_SWITCH_OBJ, "-separator", "char", (char *)NULL,
	Blt_Offset(PathPrintSwitches, pathSepObjPtr), 0}, 
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;			/* Parse flags. */
    Tcl_Obj *pathSepObjPtr;             /* Path separator. */
    Blt_TreeNode root;			/* Starting node of path. */
} PathParseSwitches;

static Blt_SwitchSpec pathParseSwitches[] = 
{
    {BLT_SWITCH_CUSTOM,  "-from",  "node", (char *)NULL,
        Blt_Offset(PathParseSwitches, root),  0, 0, &nodeSwitch},
    {BLT_SWITCH_BITMASK, "-nocomplain", "", (char *)NULL,
	Blt_Offset(PathParseSwitches, flags), 0, PATH_NOCOMPLAIN},
    {BLT_SWITCH_OBJ, "-separator", "char", (char *)NULL,
	Blt_Offset(PathParseSwitches, pathSepObjPtr), 0}, 
    {BLT_SWITCH_END}
};

typedef struct {
    int mask;
} TraceSwitches;

static Blt_SwitchSpec traceSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-whenidle", "", (char *)NULL,
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
static Tcl_ObjCmdProc CompareDictionaryCmd;
static Tcl_ObjCmdProc ExitCmd;
static Blt_TreeNotifyEventProc TreeEventProc;

typedef int (TreeCmdProc)(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc,
	Tcl_Obj *const *objv);

static int GetNodeFromObj(Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr, 
	Blt_TreeNode *nodePtr);

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
 *	Convert a Tcl_Obj representing the label of a child node into its
 *	integer node id.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ChildSwitch(
    ClientData clientData,		/* Flag indicating if the node is
					 * considered before or after the
					 * insertion position. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
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
 *	Convert a Tcl_Obj representing a node number into its integer value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
int
Blt_TreeNodeSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
{
    Blt_TreeNode *nodePtr = (Blt_TreeNode *)(record + offset);
    Blt_TreeNode node;
    Blt_Tree tree  = clientData;
    int result;

    result = GetNodeFromObj(interp, tree, objPtr, &node);
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
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
OrderSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PatternSwitch(
    ClientData clientData,	/* Flag indicating type of pattern. */
    Tcl_Interp *interp,		/* Not used. */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
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
 *	Convert a string represent a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard TCL result.
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
	 * appends to this list. Mark it NULL, so we don't try to destroy the
	 * list again.
	 */
	*listPtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * FormatSwitch --
 *
 *	Convert a string represent a node number into its integer value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FormatSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
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
 *	Convert a string representing a list of field names into a mask.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FieldsSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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
	    mask |= DIR_SIZE;
	} else if ((c == 'm') && (strcmp(string, "mode") == 0)) {
	    mask |= DIR_MODE;
	} else if ((c == 't') && (strcmp(string, "type") == 0)) {
	    mask |= DIR_TYPE;
	} else if ((c == 'u') && (strcmp(string, "uid") == 0)) {
	    mask |= DIR_UID;
	} else if ((c == 'g') && (strcmp(string, "gid") == 0)) {
	    mask |= DIR_GID;
	} else if ((c == 'a') && (strcmp(string, "atime") == 0)) {
	    mask |= DIR_ATIME;
	} else if ((c == 'c') && (strcmp(string, "ctime") == 0)) {
	    mask |= DIR_CTIME;
	} else if ((c == 'm') && (strcmp(string, "mtime") == 0)) {
	    mask |= DIR_MTIME;
	} else if ((c == 'i') && (strcmp(string, "ino") == 0)) {
	    mask |= DIR_INO;
	} else if ((c == 'd') && (strcmp(string, "dev") == 0)) {
	    mask |= DIR_DEV;
	} else if ((c == 'n') && (strcmp(string, "nlink") == 0)) {
	    mask |= DIR_NLINK;
	} else if ((c == 'a') && (strcmp(string, "all") == 0)) {
	    mask |= DIR_ALL;
	} else {
	    Tcl_AppendResult(interp, "unknown field name \"", string, "\"",
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (mask == 0) {
	mask = DIR_DEFAULT;
    }
    *maskPtr = mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TypeSwitch --
 *
 *	Convert a string representing a list of type into a mask.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeSwitchProc(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Offset to field in structure */
    int flags)				/* Not used. */
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
	if ((c == 'f') && (strcmp(string, "file") == 0)) {
	    mask |= DIR_TYPE_FILE;
	} else if ((c == 'd') && (strcmp(string, "directory") == 0)) {
	    mask |= DIR_TYPE_DIR;
	} else if ((c == 'l') && (strcmp(string, "link") == 0)) {
	    mask |= DIR_TYPE_LINK;
	} else if ((c == 's') && (strcmp(string, "socket") == 0)) {
	    mask |= DIR_TYPE_SOCK;
	} else if ((c == 'b') && (strcmp(string, "block") == 0)) {
	    mask |= DIR_TYPE_BLOCK;
	} else if ((c == 'c') && (strcmp(string, "character") == 0)) {
	    mask |= DIR_TYPE_CHAR;
	} else {
	    Tcl_AppendResult(interp, "unknown type name \"", string, "\"",
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    if (mask == 0) {
	mask = DIR_DEFAULT;
    }
    *maskPtr = mask;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeNodes --
 *
 *	Convert a string represent a node number into its integer value.
 *
 * Results:
 *	The return value is a standard TCL result.
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
 *	Convert a Tcl_Obj representing a node number into its integer
 *	value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NodesSwitch(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Interpreter to send results back
					 * to */
    const char *switchName,		/* Not used. */
    Tcl_Obj *objPtr,			/* String representation */
    char *record,			/* Structure record */
    int offset,				/* Not used. */
    int flags)				/* Not used. */
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

	if (GetNodeFromObj(interp, findPtr->cmdPtr->tree, objv[i], &node) 
	    != TCL_OK) {
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
 *	Find the tree command associated with the TCL command "string".
 *	
 *	We have to do multiple lookups to get this right.  
 *
 *	The first step is to generate a canonical command name.  If an
 *	unqualified command name (i.e. no namespace qualifier) is given, we
 *	should search first the current namespace and then the global one.
 *	Most TCL commands (like Tcl_GetCmdInfo) look only at the global
 *	namespace.
 *
 *	Next check if the string is 
 *		a) a TCL command and 
 *		b) really is a command for a tree object.  
 *	Tcl_GetCommandInfo will get us the objClientData field that should be
 *	a cmdPtr.  We can verify that by searching our hashtable of cmdPtr
 *	addresses.
 *
 * Results:
 *	A pointer to the tree command.  If no associated tree command can be
 *	found, NULL is returned.  It's up to the calling routines to generate
 *	an error message.
 *
 *---------------------------------------------------------------------------
 */
static TreeCmd *
GetTreeCmd(
    TreeCmdInterpData *tdPtr, 
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
	return NULL;			/* No such parent namespace. */
    }
    /* Rebuild the fully qualified name. */
    treeName = Blt_MakeQualifiedName(&objName, &ds);
    result = Tcl_GetCommandInfo(interp, treeName, &cmdInfo);
    Tcl_DStringFree(&ds);

    if (!result) {
	return NULL;
    }
    hPtr = Blt_FindHashEntry(&tdPtr->treeTable, 
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
	p += 2;			/* Skip the initial "->" */
	token = strstr(p, "->");
	if (token != NULL) {
	    *token = '\0';
	}
	if (IsNodeId(p)) {
	    long inode;
	    
	    if (Blt_GetLong(interp, p, &inode) != TCL_OK) {
		node = NULL;
	    } else {
		node = Blt_Tree_GetNode(tree, inode);
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
	    *token = '-';	/* Repair the string */
	}
	p = token;
    } while (token != NULL);
    return node;
 error:
    if (token != NULL) {
	*token = '-';		/* Repair the string */
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

    save = '\0';		/* Suppress compiler warning. */
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
	node = Blt_Tree_GetNode(tree, inode);
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
	*p = save;		/* Restore the string */
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetNodeFromObj --
 *
 *---------------------------------------------------------------------------
 */
static int 
GetNodeFromObj(Tcl_Interp *interp, Blt_Tree tree, Tcl_Obj *objPtr, 
	       Blt_TreeNode *nodePtr)
{
    Blt_TreeNode node;
    char *string;
    char *p;
    char save;

    node = NULL;		/* Suppress compiler warnings. */
    save = '\0';

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
	node = Blt_Tree_GetNode(tree, inode);
    }  else if (tree != NULL) {
	if (strcmp(string, "all") == 0) {
	    if (Blt_Tree_Size(Blt_Tree_RootNode(tree)) > 1) {
		if (interp != NULL) {
		    Tcl_AppendResult(interp, "more than one node tagged as \"", 
				     string, "\"", (char *)NULL);
		}
		goto error;
	    }
	    node = Blt_Tree_RootNode(tree);
	} else if (strcmp(string, "root") == 0) {
	    node = Blt_Tree_RootNode(tree);
	} else {
	    Blt_HashTable *tablePtr;
	    Blt_HashSearch cursor;
	    Blt_HashEntry *hPtr;

	    node = NULL;
	    tablePtr = Blt_Tree_TagHashTable(tree, string);
	    if (tablePtr == NULL) {
		if (interp != NULL) {
		    Tcl_AppendResult(interp, "can't find tag or id \"", string, 
			"\" in ", Blt_Tree_Name(tree), (char *)NULL);
		}
		goto error;
	    } else if (tablePtr->numEntries > 1) {
		if (interp != NULL) {
		    Tcl_AppendResult(interp, "more than one node tagged as \"", 
			 string, "\"", (char *)NULL);
		}
		goto error;
	    } else if (tablePtr->numEntries > 0) {
		hPtr = Blt_FirstHashEntry(tablePtr, &cursor);
		node = Blt_GetHashValue(hPtr);
		if (p != NULL) {
		    *p = save;
		}
	    }
	}
    }
    if (node != NULL) {
	if (p != NULL) {
	    node = ParseModifiers(interp, tree, node, p);
	    if (node == NULL) {
		if (interp != NULL) {
		    *p = save;	/* Need entire string. */
		    Tcl_AppendResult(interp, "can't find tag or id \"", string, 
			     "\" in ", Blt_Tree_Name(tree), (char *)NULL);
		}
		goto error;
	    }
	}
	if (p != NULL) {
	    *p = save;
	}
	*nodePtr = node;
	return TCL_OK;
    } 
    if (interp != NULL) {
	Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in ", 
			 Blt_Tree_Name(tree), (char *)NULL);
    }
 error:
    if (p != NULL) {
	*p = save;
    }
    return TCL_ERROR;
}


typedef struct {
    int tagType;
    Blt_TreeNode root;
    Blt_HashSearch cursor;
} TagSearch;

/*
 *---------------------------------------------------------------------------
 *
 * FirstTaggedNode --
 *
 *	Returns the id of the first node tagged by the given tag in objPtr.
 *	It basically hides much of the cumbersome special case details.  For
 *	example, the special tags "root" and "all" always exist, so they don't
 *	have entries in the tag hashtable.  If it's a hashed tag (not "root"
 *	or "all"), we have to save the place of where we are in the table for
 *	the next call to NextTaggedNode.
 *
 *---------------------------------------------------------------------------
 */
static int
FirstTaggedNode(Tcl_Interp *interp, TreeCmd *cmdPtr, Tcl_Obj *objPtr,
		TagSearch *cursorPtr, Blt_TreeNode *nodePtr)
{
    const char *string;

    *nodePtr = NULL;
    string = Tcl_GetString(objPtr);
    cursorPtr->tagType = TAG_TYPE_NONE;
    cursorPtr->root = Blt_Tree_RootNode(cmdPtr->tree);

    /* Process strings with modifiers or digits as simple ids, not tags. */
    if (GetNodeFromObj(NULL, cmdPtr->tree, objPtr, nodePtr) == TCL_OK) {
	return TCL_OK;
    }

    if (strcmp(string, "all") == 0) {
	cursorPtr->tagType = TAG_TYPE_ALL;
	*nodePtr = cursorPtr->root;
	return TCL_OK;
    } else if (strcmp(string, "root") == 0)  {
	*nodePtr = cursorPtr->root;
	return TCL_OK;
    } else {
	Blt_HashTable *tablePtr;
	
	tablePtr = Blt_Tree_TagHashTable(cmdPtr->tree, string);
	if (tablePtr != NULL) {
	    Blt_HashEntry *hPtr;
	    
	    cursorPtr->tagType = TAG_TYPE_TAG;
	    hPtr = Blt_FirstHashEntry(tablePtr, &cursorPtr->cursor); 
	    if (hPtr == NULL) {
		*nodePtr = NULL;
		return TCL_OK;
	    }
	    *nodePtr = Blt_GetHashValue(hPtr);
	    return TCL_OK;
	}
    }
    Tcl_AppendResult(interp, "can't find tag or id \"", string, "\" in ", 
	Blt_Tree_Name(cmdPtr->tree), (char *)NULL);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * SkipSeparators --
 *
 *	Moves the character pointer past one of more separators.
 *
 * Results:
 *	Returns the updates character pointer.
 *
 *---------------------------------------------------------------------------
 */
static const char *
SkipSeparators(const char *path, const char *separator, int length)
{
    while ((*path == *separator) && (strncmp(path, separator, length) == 0)) {
	path += length;
    }
    return path;
}

/*
 *---------------------------------------------------------------------------
 *
 * SplitPath --
 *
 *	Returns the trailing component of the given path.  Trailing separators
 *	are ignored.
 *
 * Results:
 *	Returns the string of the tail component.
 *
 *---------------------------------------------------------------------------
 */
static int
SplitPath(Tcl_Interp *interp, const char *path, Tcl_Obj *pathSepObjPtr, 
	  int *argcPtr, const char ***argvPtr)
{
    int skipLen, pathLen;
    int depth;
    size_t listSize;
    const char **components;
    char *p;
    char *sp;
    const char *pathSeparator;

    if ((pathSepObjPtr == NULL) || 
        ((Tcl_GetString(pathSepObjPtr))[0] == '\0')) {
	if (Tcl_SplitList(interp, path, argcPtr, argvPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    pathSeparator = Tcl_GetStringFromObj(pathSepObjPtr, &skipLen);
    pathLen = strlen(path);
    path = SkipSeparators(path, pathSeparator, skipLen);
    depth = pathLen / skipLen;

    listSize = (depth + 1) * sizeof(char *);
    components = Blt_AssertMalloc(listSize + (pathLen + 1));
    p = (char *)components + listSize;
    strcpy(p, path);

    depth = 0;
    for (sp = strstr(p, pathSeparator); ((*p != '\0') && (sp != NULL)); 
	 sp = strstr(p, pathSeparator)) {
	*sp = '\0';
	components[depth++] = p;
	p = (char *)SkipSeparators(sp + skipLen, pathSeparator, skipLen);
    }
    if (*p != '\0') {
	components[depth++] = p;
    }
    components[depth] = NULL;
    *argcPtr = depth;
    *argvPtr = components;
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * NextTaggedNode --
 *
 *---------------------------------------------------------------------------
 */
static Blt_TreeNode
NextTaggedNode(Blt_TreeNode node, TagSearch *cursorPtr)
{
    if (cursorPtr->tagType == TAG_TYPE_ALL) {
	return Blt_Tree_NextNode(NULL, node);
    }
    if (cursorPtr->tagType == TAG_TYPE_TAG) {
	Blt_HashEntry *hPtr;

	hPtr = Blt_NextHashEntry(&cursorPtr->cursor);
	if (hPtr == NULL) {
	    return NULL;
	}
	return Blt_GetHashValue(hPtr);
    }
    return NULL;
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
	const char *string;
	
	string = Tcl_GetString(objv[i]);
	if ((i + 1) == objc) {
	    Tcl_AppendResult(cmdPtr->interp, "missing value for field \"", 
		string, "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	if (Blt_Tree_SetValue(cmdPtr->interp, cmdPtr->tree, node, string, 
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
 *	Generates an unique tree command name.  Tree names are in the form
 *	"treeN", where N is a non-negative integer. Check each name generated
 *	to see if it is already a tree. We want to recycle names if possible.
 *	
 * Results:
 *	Returns the unique name.  The string itself is stored in the dynamic
 *	string passed into the routine.
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
     *	t1         <current namespace>::t1
     *	n1::t1     <current namespace>::n1::t1
     *	::t1	   ::t1
     *  ::n1::t1   ::n1::t1
     */
    treeName = NULL;		/* Suppress compiler warning. */
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
	    continue;	       /* A tree by this name already exists. */
	}
	if (Blt_CommandExists(interp, treeName)) {
	    continue;		/* A command by this name already exists. */
	}
	break;
    }
    return treeName;
}

static TreeCmd *
CreateTreeCmd(ClientData clientData, Tcl_Interp *interp, const char *name)
{
    TreeCmdInterpData *tdPtr = clientData;
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
	     *	t1         <current namespace>::t1
	     *	n1::t1     <current namespace>::n1::t1
	     *	::t1	   ::t1
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
	cmdPtr->tdPtr = tdPtr;
	cmdPtr->tree = tree;
	cmdPtr->interp = interp;
	Blt_InitHashTable(&cmdPtr->traceTable, BLT_STRING_KEYS);
	Blt_InitHashTable(&cmdPtr->notifyTable, BLT_STRING_KEYS);
	cmdPtr->notifiers = Blt_Chain_Create();
	cmdPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)name, 
		(Tcl_ObjCmdProc *)TreeInstObjCmd, cmdPtr, TreeInstDeleteProc);
	cmdPtr->tablePtr = &tdPtr->treeTable;
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
	Blt_TreeNode newNode;	/* Newly created copy. */
	
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
	return TCL_OK;			/* Stop if we've exceeded the
					 * maximum depth requested. */
    }
    result = TRUE;
    Tcl_DStringInit(&ds);
    if (findPtr->keyList != NULL) {
	Blt_TreeKey key;
	Blt_TreeKeyIterator iter;

	result = FALSE;		/* It's false if no keys match. */
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

	result = FALSE;		/* It's false if no keys match. */
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
     * When the tree token is released, all the traces and notification events
     * are automatically removed.  But we still need to clean up the
     * bookkeeping kept for traces. Clear all the tags and trace information.
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
    Blt_TreeNode node,		/* Node that has just been updated. */
    Blt_TreeKey key,		/* Field that's updated. */
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
	node = Blt_Tree_GetNode(cmdPtr->tree, eventPtr->inode);
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
		continue;		/* No match. */
	    }
	    if (eventPtr->type == TREE_NOTIFY_DELETE) {
		remove = TRUE;		/* Must destroy notifier. Node no
					* longer exists. */
	    }
	}
	if ((notifyPtr->tag != NULL) && 
	    (!Blt_Tree_HasTag(cmdPtr->tree, eventPtr->node, notifyPtr->tag))) {
	    goto next;			/* Doesn't have the tag. */
	}
	if ((notifyPtr->mask & eventPtr->type) == 0) {
	    goto next; 			/* Event not matching.  */
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

/* Tree command operations. */

/*
 *---------------------------------------------------------------------------
 *
 * ApplyOp --
 *
 * t0 apply root -precommand {command} -postcommand {command}
 *
 *---------------------------------------------------------------------------
 */
static int
ApplyOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    int result;
    Blt_TreeNode node;
    ApplySwitches switches;
    int order;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
AncestorOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    long i, d1, d2, minDepth;
    Blt_TreeNode node1, node2;

    if ((GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node1) != TCL_OK) ||
	(GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node2) != TCL_OK)) {
	return TCL_ERROR;
    }
    if (node1 == node2) {
        Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_NodeId(node1));
        return TCL_OK;
    }
    d1 = Blt_Tree_NodeDepth(node1);
    d2 = Blt_Tree_NodeDepth(node2);
    minDepth = MIN(d1, d2);
    if (minDepth == 0) {		/* One of the nodes is root. */
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
     * First find the mutual ancestor of both nodes.  Look at each preceding
     * ancestor level-by-level for both nodes.  Eventually we'll find a node
     * that's the parent of both ancestors.  Then find the first ancestor in
     * the parent's list of subnodes.
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
 *	$tree append $node "field" value value value 
 *---------------------------------------------------------------------------
 */
static int
AppendOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TagSearch iter;
    const char *key;

    if (FirstTaggedNode(interp, cmdPtr, objv[2], &iter, &node) != TCL_OK) {
	return TCL_ERROR;
    }
    key = Tcl_GetString(objv[3]);
    for (/*empty*/; node != NULL; node = NextTaggedNode(node, &iter)) {
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
AttachOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    const char *treeName;
    AttachSwitches switches;

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
ChildrenOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
    Blt_TreeNode node,		/* Node to be copied. */
    Blt_TreeNode parent)	/* New parent for the copied node. */
{
    Blt_TreeNode newNode;	/* Newly created copy. */
    const char *label;

    newNode = NULL;
    label = Blt_Tree_NodeLabel(node);
    if (switchesPtr->flags & COPY_OVERWRITE) {
	newNode = Blt_Tree_FindChild(parent, label);
    }
    if (newNode == NULL) {	/* Create node in new parent. */
	newNode = Blt_Tree_CreateNode(switchesPtr->destTree, parent, label, -1);
    }
    /* Copy the data fields. */
    {
	Blt_TreeKey key;
	Blt_TreeKeyIterator iter;

	for (key = Blt_Tree_FirstKey(switchesPtr->srcTree, node, &iter); 
	     key != NULL; key = Blt_Tree_NextKey(switchesPtr->srcTree, &iter)) {
	    Tcl_Obj *objPtr;

	    if (Blt_Tree_GetValueByKey((Tcl_Interp *)NULL, switchesPtr->srcTree, 
			node, key, &objPtr) == TCL_OK) {
		Blt_Tree_SetValueByKey((Tcl_Interp *)NULL, switchesPtr->destTree, 
			newNode, key, objPtr);
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
		if (AddTag(switchesPtr->destPtr, newNode, tPtr->tagName)!= TCL_OK) {
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
 *	t0 copy node tree node 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
CopyOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TreeCmd *srcPtr, *destPtr;
    Blt_Tree srcTree, destTree;
    Blt_TreeNode copyNode;
    Blt_TreeNode parent;
    CopySwitches switches;
    int numArgs, numSwitches;
    Blt_TreeNode root;
    int i;
    
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent) != TCL_OK) {
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
			 " copy parent ?tree? node ?switches?", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    if (numArgs == 3) {
	char *string;

	/* 
	 * The tree name is either the name of a tree command (first choice)
	 * or an internal tree object.
	 */
	string = Tcl_GetString(objv[3]);
	srcPtr = GetTreeCmd(cmdPtr->tdPtr, interp, string);
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
	if (GetNodeFromObj(interp, srcPtr->tree, objv[3], &copyNode) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    memset((char *)&switches, 0, sizeof(switches));
    switches.destPtr = destPtr;
    switches.destTree = destTree;
    switches.srcPtr = srcPtr;
    switches.srcTree = srcTree;

    /* Process switches  */
    if (Blt_ParseSwitches(interp, copySwitches, numSwitches, objv + 4, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
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
DegreeOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *	Deletes one or more nodes from the tree.  Nodes may be specified by
 *	their id (a number) or a tag.
 *	
 *	Tags have to be handled carefully here.  We can't use the normal
 *	GetTaggedNode, NextTaggedNode, etc. routines because they walk
 *	hashtables while we're deleting nodes.  Also, remember that deleting a
 *	node recursively deletes all its children. If a parent and its
 *	children have the same tag, its possible that the tag list may contain
 *	nodes than no longer exist. So save the node indices in a list and
 *	then delete then in a second pass.
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int i;
    char *string;

    string = NULL;
    for (i = 2; i < objc; i++) {
	string = Tcl_GetString(objv[i]);
	if (IsNodeIdOrModifier(string)) {
	    Blt_TreeNode node;

	    if (GetNodeFromObj(interp, cmdPtr->tree, objv[i], &node) != TCL_OK){
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
	     * Generate a list of tagged nodes. Save the inode instead of the
	     * node itself since a pruned branch may contain more tagged
	     * nodes.
	     */
	    chain = Blt_Chain_Create();
	    for (hPtr = Blt_FirstHashEntry(tablePtr, &iter); 
		hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
		Blt_TreeNode node;

		node = Blt_GetHashValue(hPtr);
		Blt_Chain_Append(chain, (ClientData)Blt_Tree_NodeId(node));
	    }   
	    /*  
	     * Iterate through this list to delete the nodes.  By side-effect
	     * the tag table is deleted and Uids are released.
	     */
	    for (link = Blt_Chain_FirstLink(chain); link != NULL;
		 link = next) {
		Blt_TreeNode node;
		long inode;

		next = Blt_Chain_NextLink(link);
		inode = (long)Blt_Chain_GetValue(link);
		node = Blt_Tree_GetNode(cmdPtr->tree, inode);
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
 *	$tree depth
 *	$tree depth $node
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DepthOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;

    if (objc == 2) {
	Tcl_SetLongObj(Tcl_GetObjResult(interp), Blt_Tree_Depth(cmdPtr->tree));
	return TCL_OK;
    }
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *	tree dump node -file fileName
 *	tree dump node -data variable
 *	tree dump node 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DumpOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode root;
    DumpSwitches switches;
    int result;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &root) != TCL_OK) {
	return TCL_ERROR;
    }
    memset((char *)&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, dumpSwitches, objc - 3, objv + 3, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
	Tcl_AppendResult(interp, "can't set both -file and -data switches.",
			 (char *)NULL);
	Blt_FreeSwitches(dumpSwitches, (char *)&switches, 0);
	return TCL_ERROR;
    }
    if (switches.dataObjPtr != NULL) {
	Tcl_DString ds;
	Tcl_Obj *objPtr;

	Tcl_DStringInit(&ds);
	Blt_Tree_Dump(cmdPtr->tree, root, &ds);
	/* Write the image into the designated TCL variable. */
	objPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds),Tcl_DStringLength(&ds));
	objPtr = Tcl_ObjSetVar2(interp, switches.dataObjPtr, NULL, objPtr, 0);
	result = (objPtr == NULL) ? TCL_ERROR : TCL_OK;
	Tcl_SetStringObj(switches.dataObjPtr, Tcl_DStringValue(&ds),
			 Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
    } else if (switches.fileObjPtr != NULL) {
	const char *fileName;

	fileName = Tcl_GetString(switches.fileObjPtr);
	result = Blt_Tree_DumpToFile(interp, cmdPtr->tree, root, fileName);
    } else {
	Tcl_DString ds;

	Tcl_DStringInit(&ds);
	Blt_Tree_Dump(cmdPtr->tree, root, &ds);
	Tcl_DStringResult(interp, &ds);
	result = TCL_OK;
    }
    Blt_FreeSwitches(dumpSwitches, (char *)&switches, 0);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * DupOp --
 * 
 *	t0 dup node
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DupOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TreeCmd *srcPtr, *destPtr;
    Blt_TreeNode node;
    
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    srcPtr = cmdPtr;
    destPtr = CreateTreeCmd(cmdPtr->tdPtr, interp, NULL);
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
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    int bool;
    
    bool = TRUE;
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *---------------------------------------------------------------------------
 */
static int
ExportOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    DataFormat *fmtPtr;
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
	Tcl_AppendResult(interp, "no export procedure registered for \"", 
			 fmtPtr->name, "\"", (char *)NULL);
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
FindOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    FindSwitches switches;
    int result;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FindChildOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode parent, child;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent) != TCL_OK) {
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
FirstChildOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *---------------------------------------------------------------------------
 */
static int
GetOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
	Tcl_Obj *valueObjPtr, *listObjPtr;
	Blt_TreeKeyIterator iter;

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


static const char *
GetTypeFromMode(int mode)
{
#ifdef WIN32
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
	       Tcl_StatBuf *statPtr, DirSwitches *switchesPtr)
{    
    if (switchesPtr->mask & DIR_SIZE) {
	Blt_Tree_SetValue(interp, tree, node, "size",  
		Tcl_NewWideIntObj((Tcl_WideInt)statPtr->st_size));
    }
    if (switchesPtr->mask & DIR_MTIME) {
	Blt_Tree_SetValue(interp, tree, node, "mtime",  
		Tcl_NewLongObj((long)statPtr->st_mtime));
    }
    if (switchesPtr->mask & DIR_CTIME) {
	Blt_Tree_SetValue(interp, tree, node, "ctime",  
		Tcl_NewLongObj((long)statPtr->st_ctime));
    }
    if (switchesPtr->mask & DIR_ATIME) {
	Blt_Tree_SetValue(interp, tree, node, "atime",  
		Tcl_NewLongObj((long)statPtr->st_atime));
    }
    if (switchesPtr->mask & DIR_MODE) {
	Blt_Tree_SetValue(interp, tree, node, "mode", 
		Tcl_NewIntObj(statPtr->st_mode));
    }
    if (switchesPtr->mask & DIR_UID) {
	Blt_Tree_SetValue(interp, tree, node, "uid", 
		Tcl_NewIntObj(statPtr->st_uid));
    }
    if (switchesPtr->mask & DIR_GID) {
	Blt_Tree_SetValue(interp, tree, node, "gid", 
		Tcl_NewIntObj(statPtr->st_gid));
    }
    if (switchesPtr->mask & DIR_TYPE) {
	Blt_Tree_SetValue(interp, tree, node, "type", 
		Tcl_NewStringObj(GetTypeFromMode(statPtr->st_mode), -1));
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * TreeReadDirectory --
 *
 *	Loads contents of directory into the specified node, creating
 *	a new node for each entry.
 *
 *---------------------------------------------------------------------------
 */
static int
TreeReadDirectory(Tcl_Interp *interp, TreeCmd *cmdPtr, Tcl_Obj *dirObjPtr,
		  Blt_TreeNode parent, DirSwitches *switchesPtr)
{
    Tcl_GlobTypeData readableFiles = {
	0, TCL_GLOB_PERM_R, NULL, NULL
    };
    Tcl_Obj *listObjPtr;
    int i, objc, numPatterns, patternFlags, length;
    Tcl_Obj **objv, **patterns;

    readableFiles.perm = switchesPtr->perm;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    numPatterns = 0;
    if (switchesPtr->patternsObjPtr != NULL) {
	if (Tcl_ListObjGetElements(interp, switchesPtr->patternsObjPtr, 
		&numPatterns, &patterns) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    patternFlags = (switchesPtr->flags & DIR_NOCASE) ? TCL_MATCH_NOCASE : 0;

    /* Get all the entries directories in the directory. */
    if (Tcl_FSMatchInDirectory(interp, listObjPtr, dirObjPtr, "*", 
		&readableFiles) != TCL_OK) {
    }
    if (Tcl_ListObjGetElements(interp, listObjPtr, &objc, &objv) != TCL_OK) {
	return TCL_OK;
    }
    Tcl_GetStringFromObj(dirObjPtr, &length);
    for (i = 0; i < objc; i++) {
	Tcl_StatBuf stat;
	int numComponents;
	Tcl_Obj *objPtr, *tailPtr;
	Blt_TreeNode child;
	const char *label;

	if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
	    return TCL_ERROR;
	}
	memset(&stat, 0, sizeof(Tcl_StatBuf));
	if (Tcl_FSStat(objv[i], &stat) < 0) {
	    continue;
	}
	/* Get the tail of the path. */
	objPtr = Tcl_FSSplitPath(objv[i], &numComponents);
	if (objPtr == NULL) {
	    return TCL_ERROR;
	}
	Tcl_IncrRefCount(objPtr);
	Tcl_ListObjIndex(NULL, objPtr, numComponents-1, &tailPtr);
	label = Tcl_GetString(tailPtr);

	if ((switchesPtr->flags & DIR_RECURSE) && (S_ISDIR(stat.st_mode))) {
	    /* Create a node for the subdirectory and recursively call this
             * routine. */
	    Tcl_Obj *subDirObjPtr;
            int result;

	    child = Blt_Tree_CreateNode(cmdPtr->tree, parent, label, -1);
	    FillEntryData(interp, cmdPtr->tree, child, &stat, switchesPtr);
	    
	    subDirObjPtr = Tcl_DuplicateObj(dirObjPtr);
	    Tcl_IncrRefCount(subDirObjPtr);
	    /* Now recurse into subdirectories. */
	    Tcl_AppendStringsToObj(subDirObjPtr, "/", label, (char *)NULL);
	    result = TreeReadDirectory(interp, cmdPtr, subDirObjPtr, child, 
                switchesPtr);
	    Tcl_DecrRefCount(subDirObjPtr);
            if (result != TCL_OK) {
		Tcl_DecrRefCount(objPtr);
		Tcl_DecrRefCount(listObjPtr);
		return TCL_ERROR;
	    }
	} else {
	    /* Match files or subdirectories against patterns. */
	    if (numPatterns > 0) {
		int j;
		
		for (j = 0; j < numPatterns; j++) {
		    const char *pattern;
		    
		    pattern = Tcl_GetString(patterns[j]);
		    if (Tcl_StringCaseMatch(label, pattern, patternFlags)) {
			goto found;
		    }
		}
                Tcl_DecrRefCount(objPtr);
		continue;
	    }
	    /* Match against type. */
	found:
	    child = Blt_Tree_CreateNode(cmdPtr->tree, parent, label, -1);
	    FillEntryData(interp, cmdPtr->tree, child, &stat, switchesPtr);
	}
	Tcl_DecrRefCount(objPtr);
    }
    Tcl_DecrRefCount(listObjPtr);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * DirOp --
 *
 *	tree dir $path $node ?switches? -recurse -types { read 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
DirOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode parent;
    DirSwitches switches;
    int result;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent) != TCL_OK) {
	return TCL_ERROR;
    }
    memset(&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, dirSwitches, objc - 4, objv + 4, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (switches.mask == 0) {
	switches.mask = DIR_DEFAULT;
    }
    result = TreeReadDirectory(interp, cmdPtr, objv[3], parent, &switches);
    Blt_FreeSwitches(dirSwitches, (char *)&switches, 0);
    return result;
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
ImportOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    DataFormat *fmtPtr;
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
	Tcl_AppendResult(interp, "no import procedure registered for \"", 
			 fmtPtr->name, "\"", (char *)NULL);
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
IndexOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    inode = -1;
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) == TCL_OK) {
	if (node != NULL) {
	    inode = Blt_Tree_NodeId(node);
	}
    } else {
	int i;
	Blt_TreeNode parent;
	Tcl_Obj **pathv;
	int pathc;

	if (Tcl_ListObjGetElements(interp, objv[2], &pathc, &pathv) != TCL_OK) {
	    goto done;		/* Can't split object. */
	}
	/* Start from the root and verify each path component. */
	parent = Blt_Tree_RootNode(cmdPtr->tree);
	for (i = 0; i < pathc; i++) {
            const char *string;

	    string = Tcl_GetString(pathv[i]);
	    if (string[0] == '\0') {
		continue;	/* Skip null separators. */
	    }
	    node = Blt_Tree_FindChild(parent, string);
	    if (node == NULL) {
		goto done;	/* Can't find component */
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
 *---------------------------------------------------------------------------
 */
static int
InsertOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode parent, child;
    InsertSwitches switches;

    child = NULL;
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &parent) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Initialize switch flags */
    memset(&switches, 0, sizeof(switches));
    switches.position = -1;	/* Default to append node. */
    switches.parent = parent;
    switches.inode = -1;

    if (Blt_ParseSwitches(interp, insertSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	goto error;
    }
    if (switches.inode > 0) {
	Blt_TreeNode node;

	node = Blt_Tree_GetNode(cmdPtr->tree, switches.inode);
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
 *	$tree isancestor $node $parent
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsAncestorOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Blt_TreeNode node, parent;
    int isAncestor;

    if ((GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) ||
	(GetNodeFromObj(interp, cmdPtr->tree, objv[3], &parent) != TCL_OK)) {
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
 *	$tree isbefore $node1 $node2
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsBeforeOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node1, node2;
    int isBefore;

    if ((GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node1) != TCL_OK) ||
	(GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node2) != TCL_OK)) {
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
 *	$tree isleaf $node
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsLeafOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *	$tree isroot $node
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
IsRootOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    int isRoot;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *	Returns the key names of values for a node or array value.
 *
 *---------------------------------------------------------------------------
 */
static int
KeysOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_HashTable keyTable;
    int i;

    Blt_InitHashTableWithPool(&keyTable, BLT_ONE_WORD_KEYS);
    for (i = 2; i < objc; i++) {
	Blt_TreeNode node;
	TagSearch iter;
	int isNew;

	if (FirstTaggedNode(interp, cmdPtr, objv[i], &iter, &node) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	for (/* empty */; node != NULL; node = NextTaggedNode(node, &iter)) {
	    Blt_TreeKey key;
	    Blt_TreeKeyIterator keyIter;

	    for (key = Blt_Tree_FirstKey(cmdPtr->tree, node, &keyIter); 
		 key != NULL; key = Blt_Tree_NextKey(cmdPtr->tree, &keyIter)) {
		Blt_CreateHashEntry(&keyTable, key, &isNew);
	    }
	}
    }
    {
	Blt_HashSearch tagSearch;
	Blt_HashEntry *hPtr;
	Tcl_Obj *listObjPtr;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (hPtr = Blt_FirstHashEntry(&keyTable, &tagSearch); hPtr != NULL;
	     hPtr = Blt_NextHashEntry(&tagSearch)) {
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
LabelOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 * LappendOp --
 *
 *	$tree lappend $node "field" value value value 
 *---------------------------------------------------------------------------
 */
static int
LappendOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TagSearch iter;
    const char *key;

    if (FirstTaggedNode(interp, cmdPtr, objv[2], &iter, &node) != TCL_OK) {
	return TCL_ERROR;
    }
    key = Tcl_GetString(objv[3]);
    while (node != NULL) {
	int i;

	for (i = 4; i < objc; i++) {
	    if (Blt_Tree_ListAppendValue(interp, cmdPtr->tree, node, key, 
		objv[i]) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	node = NextTaggedNode(node, &iter);
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
LastChildOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 * MoveOp --
 *
 *	The trick here is to not consider the node to be moved in determining
 *	it's new location.  Ideally, you would temporarily pull it from the
 *	tree and replace it (back in its old location if something went
 *	wrong), but you could still pick the node by its serial number.  So
 *	here we make lots of checks for the node to be moved.
 * 
 *---------------------------------------------------------------------------
 */
static int
MoveOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode root, parent, node;
    Blt_TreeNode before;
    MoveSwitches switches;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[3], &parent) != TCL_OK) {
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
    before = NULL;		/* If before is NULL, this appends the node to
				 * the parent's child list.  */

    if (switches.node != NULL) {	/* -before or -after */
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
	int count;		/* Tracks the current list index. */
	Blt_TreeNode child;

	/* 
	 * If the node is in the list, ignore it when determining the "before"
	 * node using the -at index.  An index of -1 means to append the node
	 * to the list.
	 */
	count = 0;
	for(child = Blt_Tree_FirstChild(parent); child != NULL; 
	    child = Blt_Tree_NextSibling(child)) {
	    if (child == node) {
		continue;	/* Ignore the node to be moved. */
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
 * NextOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NextOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
NextSiblingOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *	tree0 notify create ?flags? command arg
 *---------------------------------------------------------------------------
 */
static int
NotifyCreateOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
    Notifier *notifyPtr;
    NotifySwitches switches;
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
NotifyDeleteOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	       Tcl_Obj *const *objv)
{
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
NotifyInfoOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Notifier *notifyPtr;
    Blt_HashEntry *hPtr;
    Tcl_DString ds;
    char *string;

    string = Tcl_GetString(objv[3]);
    hPtr = Blt_FindHashEntry(&cmdPtr->notifyTable, string);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "unknown notify name \"", string, "\"", 
			 (char *)NULL);
	return TCL_ERROR;
    }
    notifyPtr = Blt_GetHashValue(hPtr);

    Tcl_DStringInit(&ds);
    Tcl_DStringAppendElement(&ds, string);	/* Copy notify Id */
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
NotifyNamesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)	/* Not used. */
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&cmdPtr->notifyTable, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	Tcl_Obj *objPtr;
	char *notifyId;

	notifyId = Blt_GetHashKey(&cmdPtr->notifyTable, hPtr);
	objPtr = Tcl_NewStringObj(notifyId, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
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
    {"create", 1, NotifyCreateOp, 4, 0, "?flags? command",},
    {"delete", 1, NotifyDeleteOp, 3, 0, "notifyId...",},
    {"info",   1, NotifyInfoOp,   4, 4, "notifyId",},
    {"names",  1, NotifyNamesOp,  3, 3, "",},
};

static int numNotifyOps = sizeof(notifyOps) / sizeof(Blt_OpSpec);

static int
NotifyOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numNotifyOps, notifyOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}


/*ARGSUSED*/
static int
ParentOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *   $tree path create $path -separator / -leaf -parents -nocomplain -from node
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathCreateOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Blt_TreeNode parent;
    long inode;
    Blt_TreeNode child;
    PathCreateSwitches switches;
    const char **argv;
    const char *path;
    int argc;
    int i;
    int result;
    
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    switches.root = Blt_Tree_RootNode(cmdPtr->tree);
    switches.pathSepObjPtr = cmdPtr->defPathSepObjPtr;
    if (switches.pathSepObjPtr != NULL) {
        Tcl_IncrRefCount(switches.pathSepObjPtr);
    }
    if (Blt_ParseSwitches(interp, pathCreateSwitches, objc - 4, objv + 4, 
			  &switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    parent = switches.root;
    path = Tcl_GetString(objv[3]);
    result = SplitPath(interp, path, switches.pathSepObjPtr, &argc, &argv);
    Blt_FreeSwitches(pathCreateSwitches, (char *)&switches, 0);
    if (argc == 0) {
	goto done;
    }
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < (argc - 1); i++) {
	Blt_TreeNode child;
	    
	child = Blt_Tree_FindChild(parent, argv[i]);
	if (child == NULL) {
	    if (switches.flags & PATH_PARENTS) {
		child = Blt_Tree_CreateNode(cmdPtr->tree, parent, argv[i], -1);
	    } else if (switches.flags & PATH_NOCOMPLAIN) {
		parent = NULL;
		goto done;
	    } else {
		Tcl_DString ds;

		Tcl_DStringInit(&ds);
		Tcl_AppendResult(interp, "can't find parent node \"", 
			argv[i], "\" in \"", Blt_Tree_NodePath(parent, &ds), 
			"\"", (char *)NULL);
		Tcl_DStringFree(&ds);
		Blt_Free(argv);
		return TCL_ERROR;
	    }
	} 
	parent = child;
    }
    child = Blt_Tree_FindChild(parent, argv[i]);
    if (child != NULL) {
	if (switches.flags & PATH_NOCOMPLAIN) {
	    Tcl_DString ds;
		
	    Tcl_DStringInit(&ds);
	    Tcl_AppendResult(interp, "can't create node: \"", argv[i], 
		"\" already exists in ", Blt_Tree_NodePath(parent, &ds), 
		(char *)NULL);
	    Tcl_DStringFree(&ds);
	    Blt_Free(argv);
	    return TCL_ERROR;
	}
    } else {
	child = Blt_Tree_CreateNode(cmdPtr->tree, parent, argv[i], -1);
    }
    parent = child;
 done:
    Blt_Free(argv);
    inode = -1;
    if (parent != NULL) {
	inode = Blt_Tree_NodeId(parent);
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PathParseOp --
 *
 *	$tree path get $path -separator / -from root -nocomplain
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathParseOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode parent;
    long inode;
    Blt_TreeNode child;
    PathParseSwitches switches;
    const char **argv;
    const char *path;
    int argc;
    int i;
    int result;
    
    /* Process switches  */
    nodeSwitch.clientData = cmdPtr->tree;
    memset(&switches, 0, sizeof(switches));
    switches.root = Blt_Tree_RootNode(cmdPtr->tree);
    switches.pathSepObjPtr = cmdPtr->defPathSepObjPtr;
    if (switches.pathSepObjPtr != NULL) {
        Tcl_IncrRefCount(switches.pathSepObjPtr);
    }
    if (Blt_ParseSwitches(interp, pathParseSwitches, objc - 4, objv + 4, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    path = Tcl_GetString(objv[3]);
    result = SplitPath(interp, path, switches.pathSepObjPtr, &argc, &argv);
    Blt_FreeSwitches(pathParseSwitches, (char *)&switches, 0);
    if (argc == 0) {
        inode = Blt_Tree_NodeId(switches.root);
	Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
	Blt_Free(argv);
	return TCL_OK;
    }
    if (result != TCL_OK) {
	return TCL_ERROR;
    }
    parent = switches.root;
    for (i = 0; i < (argc - 1); i++) {
	Blt_TreeNode child;
	    
	child = Blt_Tree_FindChild(parent, argv[i]);
	if (child == NULL) {
	    if (switches.flags & PATH_NOCOMPLAIN) {
		Tcl_SetLongObj(Tcl_GetObjResult(interp), -1);
		Blt_Free(argv);
		return TCL_OK;
	    } else {
		Tcl_DString ds;
		
		Tcl_DStringInit(&ds);
		Tcl_AppendResult(interp, "can't find parent node \"", argv[i], 
			"\" in \"", Blt_Tree_NodePath(parent, &ds), "\"", 
			(char *)NULL);
		Tcl_DStringFree(&ds);
		Blt_Free(argv);
		return TCL_ERROR;
	    }
	} 
	parent = child;
    }
    child = Blt_Tree_FindChild(parent, argv[i]);
    if (child == NULL) {
	if (switches.flags & PATH_NOCOMPLAIN) {
	    Tcl_SetLongObj(Tcl_GetObjResult(interp), -1);
	    Blt_Free(argv);
	    return TCL_OK;
	} else {
	    Tcl_DString ds;
	    
	    Tcl_DStringInit(&ds);
	    Tcl_AppendResult(interp, "can't find node \"", argv[i], "\" in \"", 
		Blt_Tree_NodePath(parent, &ds), "\"", (char *)NULL);
	    Tcl_DStringFree(&ds);
	    Blt_Free(argv);
	    return TCL_ERROR;
	}
    }
    Blt_Free(argv);
    inode = Blt_Tree_NodeId(child);
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * PathPrintOp --
 *
 *	$tree path print $node -separator / 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathPrintOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Blt_TreeNode node, root;
    int flags;
    Tcl_DString ds;
    PathPrintSwitches switches;
    const char *pathSeparator;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    /* Process switches  */
    memset(&switches, 0, sizeof(switches));
    switches.pathSepObjPtr = cmdPtr->defPathSepObjPtr;
    if (switches.pathSepObjPtr != NULL) {
        Tcl_IncrRefCount(switches.pathSepObjPtr);
    }
    if (Blt_ParseSwitches(interp, pathPrintSwitches, objc - 4, objv + 4, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    root = Blt_Tree_RootNode(cmdPtr->tree);
    flags = 0;
    pathSeparator = NULL;
    if (switches.pathSepObjPtr != NULL) {
	flags = TREE_INCLUDE_ROOT;
        pathSeparator = Tcl_GetString(switches.pathSepObjPtr);
    }
    Blt_Tree_NodeRelativePath(root, node, pathSeparator, flags, &ds);
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
 *	$tree path separator value
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PathSeparatorOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    if (objc == 4) { 
        int length;

        if (cmdPtr->defPathSepObjPtr != NULL) {
            Tcl_DecrRefCount(cmdPtr->defPathSepObjPtr);
            cmdPtr->defPathSepObjPtr = NULL;
        }
        Tcl_GetStringFromObj(objv[3], &length);
        if (length > 0) {
            cmdPtr->defPathSepObjPtr = objv[3];
            Tcl_IncrRefCount(cmdPtr->defPathSepObjPtr);
        }
    }
    if (cmdPtr->defPathSepObjPtr != NULL) {
        Tcl_IncrRefCount(cmdPtr->defPathSepObjPtr);
        Tcl_SetObjResult(interp, cmdPtr->defPathSepObjPtr);
    }
    return TCL_OK;
}



/*
 *---------------------------------------------------------------------------
 *
 * PathOp --
 *
 *   $tree path parse $path -from node -separator / 
 *   $tree path print $node -separator /
 *   $tree path create $path -parents -from node -separator /
 *   $tree path separator ?string?
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec pathOps[] =
{
    {"create",     1, PathCreateOp,     4, 0, "path ?switches?",},
    {"parse",      2, PathParseOp,      4, 0, "path ?switches?",},
    {"print",      2, PathPrintOp,      4, 0, "node ?switches?",},
    {"separator",  1, PathSeparatorOp,  3, 4, "?string?",},
};

static int numPathOps = sizeof(pathOps) / sizeof(Blt_OpSpec);

static int
PathOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numPathOps, pathOps, BLT_OP_ARG2, objc, objv,
	0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
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
 * PositionOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
PositionOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    PositionSwitches switches;
    Blt_TreeNode *nodeArr, *nodePtr;
    Blt_TreeNode lastParent;
    Tcl_Obj *listObjPtr, *objPtr;
    int i;
    long position;
    Tcl_DString ds;
    int n;

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

	if (GetNodeFromObj(interp, cmdPtr->tree, objv[i], &node) != TCL_OK) {
	    Blt_Free(nodeArr);
	    return TCL_ERROR;
	}
	nodeArr[i] = node;
    }
    nodeArr[i] = NULL;

    if (switches.sort) {		/* Sort the nodes by depth-first order 
					 * if requested. */
	qsort((char *)nodeArr, objc, sizeof(Blt_TreeNode), 
	      (QSortCompareProc *)ComparePositions);
    }

    position = 0;		/* Suppress compiler warning. */
    lastParent = NULL;
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    Tcl_DStringInit(&ds);
    for (nodePtr = nodeArr; *nodePtr != NULL; nodePtr++) {
	Blt_TreeNode parent;

	parent = Blt_Tree_ParentNode(*nodePtr);
	if ((parent != NULL) && (parent == lastParent)) {
	    Blt_TreeNode node;

	    /* 
	     * Since we've sorted the nodes already, we can safely assume that
	     * if two consecutive nodes have the same parent, the first node
	     * came before the second. If this is the case, use the last node
	     * as a starting point.
	     */
	    
	    /*
	     * Note that we start comparing from the last node, not its
	     * successor.  Some one may give us the same node more than once.
	     */
	    node = *(nodePtr - 1); /* Can't get here unless there's more than
				    * one node. */
	    for(/*empty*/; node != NULL; node = Blt_Tree_NextSibling(node)) {
		if (node == *nodePtr) {
		    break;
		}
		position++;
	    }
	} else {
	    /* 
	     * The fallback is to linearly search through the parent's list of
	     * children, counting the number of preceding siblings. Except for
	     * nodes with many siblings (100+), this should be okay.
	     */
	    position = Blt_Tree_NodePosition(*nodePtr);
	}
	if (switches.sort) {
	    lastParent = parent; /* Update the last parent. */
	}	    
	/* 
	 * Add an element in the form "parent -at position" to the list that
	 * we're generating.
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
PreviousOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
	return TCL_ERROR;
    }
    node = Blt_Tree_PrevNode(NULL, node);
    inode = (node != NULL) ? Blt_Tree_NodeId(node) : -1;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), inode);
    return TCL_OK;
}

/*ARGSUSED*/
static int
PrevSiblingOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long inode;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 * RestoreOp --
 *
 *	tree restore node -file fileName
 *	tree restore node -data string
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
RestoreOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode root;		/* Root node of restored subtree. */
    RestoreSwitches switches;
    int result;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &root) != TCL_OK) {
	return TCL_ERROR;
    }
    memset((char *)&switches, 0, sizeof(switches));
    if (Blt_ParseSwitches(interp, restoreSwitches, objc - 3, objv + 3, 
	&switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if ((switches.dataObjPtr != NULL) && (switches.fileObjPtr != NULL)) {
	Tcl_AppendResult(interp, "can't set both -file and -data switches.",
			 (char *)NULL);
	Blt_FreeSwitches(restoreSwitches, (char *)&switches, 0);
	return TCL_ERROR;
    }
    if (switches.dataObjPtr != NULL) {
	const char *string;

	string = Tcl_GetString(switches.dataObjPtr);
	result = Blt_Tree_Restore(interp, cmdPtr->tree, root, string, 
		switches.flags);
    } else if (switches.fileObjPtr != NULL) {
	const char *fileName;

	fileName = Tcl_GetString(switches.fileObjPtr);
	result = Blt_Tree_RestoreFromFile(interp, cmdPtr->tree, root, fileName,
		switches.flags);
    } else {
	result = TCL_OK;
    }
    Blt_FreeSwitches(restoreSwitches, (char *)&switches, 0);
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
RootOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode root;

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
SetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TagSearch iter;

    if (FirstTaggedNode(interp, cmdPtr, objv[2], &iter, &node) != TCL_OK) {
	return TCL_ERROR;
    }
    while (node != NULL) {
	if (SetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
	    return TCL_ERROR;
	}
	node = NextTaggedNode(node, &iter);
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
SizeOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 *	.t tag add tagName node1 node2 node3
 *
 *---------------------------------------------------------------------------
 */
static int
TagAddOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
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
	    TagSearch iter;
	    
	    if (FirstTaggedNode(interp, cmdPtr, objv[i], &iter, &node) 
		!= TCL_OK) {
		return TCL_ERROR;
	    }
	    for (/* empty */; node != NULL; 
			    node = NextTaggedNode(node, &iter)) {
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
 *	.t tag delete tag node1 node2 node3
 *
 *---------------------------------------------------------------------------
 */
static int
TagDeleteOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    const char *string;
    Blt_HashTable *tablePtr;
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
	    TagSearch iter;

	    if (FirstTaggedNode(interp, cmdPtr, objv[i], &iter, &node) 
		!= TCL_OK) {
	        return TCL_ERROR;
	    }
	    for (/* empty */; node != NULL; 
			    node = NextTaggedNode(node, &iter)) {
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
 * TagDumpOp --
 *
 *	Like "dump", but dumps only the contents of nodes tagged by a given
 *	tag.
 *
 *---------------------------------------------------------------------------
 */
static int
TagDumpOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode root;
    Tcl_DString ds;
    int i;

    Tcl_DStringInit(&ds);
    root = Blt_Tree_RootNode(cmdPtr->tree);
    for (i = 3; i < objc; i++) {
	Blt_TreeNode node;
	TagSearch iter;

	if (FirstTaggedNode(interp, cmdPtr, objv[i], &iter, &node) 
	    != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	for (/* empty */; node != NULL; node = NextTaggedNode(node, &iter)) {
	    Blt_Tree_DumpNode(cmdPtr->tree, root, node, &ds);
	}
    }
    Tcl_DStringResult(interp, &ds);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TagExistsOp --
 *
 *	Returns the existence of the one or more tags in the given node.  If
 *	the node has any the tags, true is return in the interpreter.
 *
 *	.t tag exists tag1 node
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagExistsOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int bool;
    const char *tagName;

    tagName = Tcl_GetString(objv[3]);
    bool = (Blt_Tree_TagHashTable(cmdPtr->tree, tagName) != NULL);
    if (objc == 5) {
	Blt_TreeNode node;

	if (GetNodeFromObj(interp, cmdPtr->tree, objv[4], &node) != TCL_OK) {
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
 *	Removes the given tags from all nodes.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TagForgetOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
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
 *	Returns tag names for a given node.  If one of more pattern arguments
 *	are provided, then only those matching tags are returned.
 *
 *	.t tag get node pat1 pat2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagGetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node; 
    Tcl_Obj *listObjPtr;
   
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node) != TCL_OK) {
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
 *	Returns the names of all the tags in the tree.  If one of more node
 *	arguments are provided, then only the tags found in those nodes are
 *	returned.
 *
 *	.t tag names node node node...
 *
 *---------------------------------------------------------------------------
 */
static int
TagNamesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Obj *listObjPtr, *objPtr;

    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    objPtr = Tcl_NewStringObj("all", -1);
    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    if (objc == 3) {
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	objPtr = Tcl_NewStringObj("root", -1);
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

	    if (GetNodeFromObj(interp, cmdPtr->tree, objv[i], &node) != TCL_OK) {
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
 *	Returns the node ids for the given tags.  The ids returned will
 *	represent the union of nodes for all the given tags.
 *
 *	.t nodes tag1 tag2 tag3...
 *
 *---------------------------------------------------------------------------
 */
static int
TagNodesOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_HashTable nodeTable;
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
 *	Sets one or more tags for a given node.  Tag names can't start with a
 *	digit (to distinquish them from node ids) and can't be a reserved tag
 *	("root" or "all").
 *
 *	.t tag set node tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagSetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    int i;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node) != TCL_OK) {
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
 *	Removes one or more tags from a given node. If a tag doesn't exist or
 *	is a reserved tag ("root" or "all"), nothing will be done and no error
 *	message will be returned.
 *
 *	.t tag unset node tag1 tag2...
 *
 *---------------------------------------------------------------------------
 */
static int
TagUnsetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    int i;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node) != TCL_OK) {
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
    {"add",    1, TagAddOp,    4, 0, "tag ?node...?",},
    {"delete", 2, TagDeleteOp, 5, 0, "tag node...",},
    {"dump",   2, TagDumpOp,   4, 0, "tag...",},
    {"exists", 1, TagExistsOp, 4, 5, "tag ?node?",},
    {"forget", 1, TagForgetOp, 4, 0, "tag...",},
    {"get",    1, TagGetOp,    4, 0, "node ?pattern...?",},
    {"names",  2, TagNamesOp,  3, 0, "?node...?",},
    {"nodes",  2, TagNodesOp,  4, 0, "tag ?tag...?",},
    {"set",    1, TagSetOp,    4, 0, "node tag...",},
    {"unset",  1, TagUnsetOp,  4, 0, "node tag...",},
};

static int numTagOps = sizeof(tagOps) / sizeof(Blt_OpSpec);

static int
TagOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTagOps, tagOps, BLT_OP_ARG2, objc, objv, 
	0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraceCreateOp --
 *
 * $tree trace create nodeIdOrTag key rwu cmd switches
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TraceCreateOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	      Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    TraceInfo *tracePtr;
    const char *key, *command;
    const char *string;
    const char *tagName;
    int flags;
    int length;
    TraceSwitches switches;
    long nodeId;

    string = Tcl_GetString(objv[3]);
    tagName = NULL;
    node = NULL;
    if (Blt_GetLongFromObj(NULL, objv[3], &nodeId) == TCL_OK) {
	if (GetNodeFromObj(interp, cmdPtr->tree, objv[3], &node) != TCL_OK) {
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
 *---------------------------------------------------------------------------
 */
static int
TraceDeleteOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc,
	      Tcl_Obj *const *objv)
{
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
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TraceNamesOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, 
	     Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;

    for (hPtr = Blt_FirstHashEntry(&cmdPtr->traceTable, &iter);
	 hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	Tcl_AppendElement(interp, Blt_GetHashKey(&cmdPtr->traceTable, hPtr));
    }
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
TraceInfoOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TraceInfo *tracePtr;
    struct _Blt_TreeTrace *tokenPtr;
    Blt_HashEntry *hPtr;
    char string[5];
    char *key;
    Tcl_Obj *listObjPtr, *objPtr;

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
    {"create", 1, TraceCreateOp, 7, 0, "node key how command ?-whenidle?",},
    {"delete", 1, TraceDeleteOp, 3, 0, "id...",},
    {"info",   1, TraceInfoOp,   4, 4, "id",},
    {"names",  1, TraceNamesOp,  3, 3, "",},
};

static int numTraceOps = sizeof(traceOps) / sizeof(Blt_OpSpec);

static int
TraceOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    TreeCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numTraceOps, traceOps, BLT_OP_ARG2, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    result = (*proc) (cmdPtr, interp, objc, objv);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TypeOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,				/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *valueObjPtr;
    const char *string;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
UnsetOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    long nodeId;

    if (Blt_GetLongFromObj(NULL, objv[2], &nodeId) == TCL_OK) {
	if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (UnsetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	TagSearch iter;

	if (FirstTaggedNode(interp, cmdPtr, objv[2], &iter, &node) 
	    != TCL_OK) {
	    return TCL_ERROR;
	}
	for (/* empty */; node != NULL; node = NextTaggedNode(node, &iter)) {
	    if (UnsetValues(cmdPtr, node, objc - 3, objv + 3) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}


typedef struct {
    TreeCmd *cmdPtr;
    unsigned int flags;
    int type;
    int mode;
    char *key;
    const char *command;
} SortSwitches;

#define SORT_RECURSE		(1<<2)
#define SORT_DECREASING		(1<<3)
#define SORT_PATHNAME		(1<<4)

enum SortTypes { SORT_DICTIONARY, SORT_REAL, SORT_INTEGER, SORT_ASCII, 
	SORT_COMMAND };

enum SortModes { SORT_FLAT, SORT_REORDER };

#define _off(x) Blt_Offset(SortSwitches, x)
static Blt_SwitchSpec sortSwitches[] = 
{
    {BLT_SWITCH_VALUE,   "-ascii",	"", (char *)NULL,
	Blt_Offset(SortSwitches, type),    0, SORT_ASCII},
    {BLT_SWITCH_STRING,  "-command",	"command", (char *)NULL,
	Blt_Offset(SortSwitches, command), 0},
    {BLT_SWITCH_BITMASK, "-decreasing", "", (char *)NULL,
	Blt_Offset(SortSwitches, flags),   0, SORT_DECREASING},
    {BLT_SWITCH_VALUE,   "-dictionary", "", (char *)NULL,
	Blt_Offset(SortSwitches, type),    0, SORT_DICTIONARY},
    {BLT_SWITCH_VALUE,   "-integer",	"", (char *)NULL,
	Blt_Offset(SortSwitches, type),    0, SORT_INTEGER},
    {BLT_SWITCH_STRING,  "-key",	"string", (char *)NULL,
	Blt_Offset(SortSwitches, key),     0},
    {BLT_SWITCH_BITMASK, "-path",	"", (char *)NULL,
	Blt_Offset(SortSwitches, flags),   0, SORT_PATHNAME},
    {BLT_SWITCH_VALUE,   "-real",	"", (char *)NULL,
	Blt_Offset(SortSwitches, type),    0, SORT_REAL},
    {BLT_SWITCH_VALUE,   "-recurse",	"", (char *)NULL,
	Blt_Offset(SortSwitches, flags),   0, SORT_RECURSE},
    {BLT_SWITCH_VALUE,   "-reorder",	"", (char *)NULL,
	Blt_Offset(SortSwitches, mode),    0, SORT_REORDER},
    {BLT_SWITCH_END}
};
#undef _off

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

	    result = 0;	/* Hopefully this will be okay even if the TCL command
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
 *	Sorts the subnodes at a given node.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SortApplyProc(
    Blt_TreeNode node,
    ClientData clientData,
    int order)			/* Not used. */
{
    TreeCmd *cmdPtr = clientData;

    if (!Blt_Tree_IsLeaf(node)) {
	Blt_Tree_SortNode(cmdPtr->tree, node, CompareNodes);
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
SortOp(
    TreeCmd *cmdPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Blt_TreeNode top;
    SortSwitches switches;
    int result;

    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &top) != TCL_OK) {
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
 * NamesOp --
 *
 *	Returns the names of values for a node or array value.
 *
 *---------------------------------------------------------------------------
 */
static int
NamesOp(TreeCmd *cmdPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    Blt_TreeNode node;
    Tcl_Obj *listObjPtr;
    
    if (GetNodeFromObj(interp, cmdPtr->tree, objv[2], &node) != TCL_OK) {
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
 * TreeInstObjCmd --
 *
 * 	This procedure is invoked to process commands on behalf of the tree
 * 	object.
 *
 * Results:
 *	A standard TCL result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec treeOps[] =
{
    {"ancestor",    2, AncestorOp,    4, 4, "node1 node2",},
    {"append",      4, AppendOp,      4, 0, "node key ?value...?",},
    {"apply",       4, ApplyOp,       3, 0, "node ?switches?",},
    {"attach",      2, AttachOp,      3, 0, "tree ?switches?",},
    {"children",    2, ChildrenOp,    3, 5, "node ?first? ?last?",},
    {"copy",        2, CopyOp,        4, 0, "parent ?tree? node ?switches?",},
    {"degree",      3, DegreeOp,      3, 0, "node",},
    {"delete",      3, DeleteOp,      2, 0, "?node...?",},
    {"depth",       3, DepthOp,       2, 3, "?node?",},
    {"dir",         2, DirOp,         4, 0, "node path ?switches?",},
    {"dump",        3, DumpOp,        3, 0, "node ?switches?",},
    {"dup",         3, DupOp,         2, 3, "node",},
    {"exists",      3, ExistsOp,      3, 4, "node ?key?",},
    {"export",      3, ExportOp,      2, 0, "format ?switches?",},
    {"find",        4, FindOp,        3, 0, "node ?switches?",},
    {"findchild",   5, FindChildOp,   4, 4, "node label",},
    {"firstchild",  3, FirstChildOp,  3, 3, "node",},
    {"get",         1, GetOp,         3, 5, "node ?key? ?defValue?",},
    {"import",      2, ImportOp,      2, 0, "format ?switches?",},
    {"index",       3, IndexOp,       3, 3, "label|list",},
    {"insert",      3, InsertOp,      3, 0, "parent ?switches?",},
    {"isancestor",  3, IsAncestorOp,  4, 4, "node1 node2",},
    {"isbefore",    3, IsBeforeOp,    4, 4, "node1 node2",},
    {"isleaf",      3, IsLeafOp,      3, 3, "node",},
    {"isroot",      3, IsRootOp,      3, 3, "node",},
    {"keys",        1, KeysOp,        3, 0, "node ?node...?",},
    {"label",       3, LabelOp,       3, 4, "node ?newLabel?",},
    {"lappend",     3, LappendOp,     4, 0, "node key ?value...?",},
    {"lastchild",   3, LastChildOp,   3, 3, "node",},
    {"move",        1, MoveOp,        4, 0, "node newParent ?switches?",},
    {"names",       2, NamesOp,       3, 4, "node ?key?",},
    {"next",        4, NextOp,        3, 3, "node",},
    {"nextsibling", 5, NextSiblingOp, 3, 3, "node",},
    {"notify",      2, NotifyOp,      2, 0, "args...",},
    {"parent",      3, ParentOp,      3, 3, "node",},
    {"path",        3, PathOp,        3, 0, "path ?args?",},
    {"position",    2, PositionOp,    3, 0, "?switches? node...",},
    {"previous",    5, PreviousOp,    3, 3, "node",},
    {"prevsibling", 5, PrevSiblingOp, 3, 3, "node",},
    {"restore",     2, RestoreOp,     3, 0, "node ?switches?",},
    {"root",        2, RootOp,        2, 2, "",},
    {"set",         2, SetOp,         3, 0, "node ?key value...?",},
    {"size",        2, SizeOp,        3, 3, "node",},
    {"sort",        2, SortOp,        3, 0, "node ?flags...?",},
    {"tag",         2, TagOp,         3, 0, "args...",},
    {"trace",       2, TraceOp,       2, 0, "args...",},
    {"type",        2, TypeOp,        4, 4, "node key",},
    {"unset",       1, UnsetOp,       3, 0, "node ?key...?",},
};

static int numTreeOps = sizeof(treeOps) / sizeof(Blt_OpSpec);

static int
TreeInstObjCmd(
    ClientData clientData,	/* Information about the widget. */
    Tcl_Interp *interp,		/* Interpreter to report errors back to. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Vector of argument strings. */
{
    TreeCmdProc *proc;
    TreeCmd *cmdPtr = clientData;
    int result;

    proc = Blt_GetOpFromObj(interp, numTreeOps, treeOps, BLT_OP_ARG1, objc, objv,
	0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(cmdPtr);
    result = (*proc) (cmdPtr, interp, objc, objv);
    Tcl_Release(cmdPtr);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TreeInstDeleteProc --
 *
 *	Deletes the command associated with the tree.  This is called only
 *	when the command associated with the tree is destroyed.
 *
 * Results:
 *	None.
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
    if (cmdPtr->defPathSepObjPtr != NULL) {
        Tcl_DecrRefCount(cmdPtr->defPathSepObjPtr);
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
TreeCreateOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
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
TreeDestroyOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    TreeCmdInterpData *tdPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
	TreeCmd *cmdPtr;
	char *string;

	string = Tcl_GetString(objv[i]);
	cmdPtr = GetTreeCmd(tdPtr, interp, string);
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
 * TreeNamesOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeNamesOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    TreeCmdInterpData *tdPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&tdPtr->treeTable, &iter); hPtr != NULL; 
	hPtr = Blt_NextHashEntry(&iter)) {
	Blt_ObjectName objName;
	TreeCmd *cmdPtr;
	const char *qualName;
	Tcl_Obj *objPtr;

	cmdPtr = Blt_GetHashValue(hPtr);
	objName.name = Tcl_GetCommandName(interp, cmdPtr->cmdToken);
	objName.nsPtr = Blt_GetCommandNamespace(cmdPtr->cmdToken);
	qualName = Blt_MakeQualifiedName(&objName, &ds);
	if (objc == 3) {
	    if (!Tcl_StringMatch(qualName, Tcl_GetString(objv[2]))) {
		continue;
	    }
	}
	objPtr = Tcl_NewStringObj(qualName, -1);
	Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*ARGSUSED*/
static int
TreeLoadOp(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    Blt_HashEntry *hPtr;
    TreeCmdInterpData *dataPtr = clientData;
    Tcl_DString libName;
    char *fmt;
    char *safeProcName, *initProcName;
    int result;
    int length;

    fmt = Tcl_GetStringFromObj(objv[2], &length);
    hPtr = Blt_FindHashEntry(&dataPtr->fmtTable, fmt);
    if (hPtr != NULL) {
	return TCL_OK;		/* Converter for format is already loaded. */
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
    Blt_FormatString(safeProcName, 8 + length + 8 + 1, "Blt_Tree%sSafeInit", fmt);
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
    {"create",  1, TreeCreateOp,  2, 3, "?name?",},
    {"destroy", 1, TreeDestroyOp, 3, 0, "name...",},
    {"load",    1, TreeLoadOp,    4, 4, "name libpath",},
    {"names",   1, TreeNamesOp,   2, 3, "?pattern?...",},
};

static int numCmdOps = sizeof(treeCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
TreeObjCmd(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
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
 *	This is called when the interpreter hosting the "tree" command
 *	is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the hash table managing all tree names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
TreeInterpDeleteProc(
    ClientData clientData,	/* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    TreeCmdInterpData *tdPtr = clientData;

    /* 
     * All tree instances should already have been destroyed when their
     * respective TCL commands were deleted. 
     */
    Blt_DeleteHashTable(&tdPtr->treeTable);
    Tcl_DeleteAssocData(interp, TREE_THREAD_KEY);
    Blt_Free(tdPtr);
}

/*ARGSUSED*/
static int
CompareDictionaryCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    int result;
    const char *s1, *s2;

    s1 = Tcl_GetString(objv[1]);
    s2 = Tcl_GetString(objv[2]);
    result = Blt_DictionaryCompare(s1, s2);
    Tcl_SetIntObj(Tcl_GetObjResult(interp), result);
    return TCL_OK;
}

/*ARGSUSED*/
static int
ExitCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,
    int objc,			/* Not used. */
    Tcl_Obj *const *objv)
{
    int code;

    if (Tcl_GetIntFromObj(interp, objv[1], &code) != TCL_OK) {
	return TCL_ERROR;
    }
#ifdef TCL_THREADS
    Tcl_Exit(code);
#else 
    exit(code);
#endif
    /*NOTREACHED*/
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_TreeCmdInitProc --
 *
 *	This procedure is invoked to initialize the "tree" command.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates the new command and adds a new entry into a global Tcl
 *	associative array.
 *
 *---------------------------------------------------------------------------
 */
int
Blt_TreeCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
	"tree", TreeObjCmd, 
    };
    static Blt_CmdSpec utilSpecs[] = { 
	{ "compare", CompareDictionaryCmd, },
	{ "_exit", ExitCmd, }
    };
    if (Blt_InitCmds(interp, "::blt::util", utilSpecs, 2) != TCL_OK) {
	return TCL_ERROR;
    }
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
