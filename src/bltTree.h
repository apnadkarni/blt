/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltTree.h --
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

#ifndef _BLT_TREE_H
#define _BLT_TREE_H

#include <bltChain.h>
#include <bltHash.h>
#include <bltPool.h>

typedef struct _Blt_TreeSlink *Blt_TreeSlink;
typedef struct _Blt_TreeNode *Blt_TreeNode;
typedef struct _Blt_TreeObject *Blt_TreeObject;
typedef struct _Blt_Tree *Blt_Tree;
typedef struct _Blt_TreeTrace *Blt_TreeTrace;
typedef struct _Blt_TreeValue *Blt_TreeValue;
typedef struct _Blt_TreeTagEntry Blt_TreeTagEntry;
typedef struct _Blt_TreeTagTable Blt_TreeTagTable;
typedef struct _Blt_TreeInterpData Blt_TreeInterpData;

typedef const char *Blt_TreeKey;

#define TREE_CREATE             (1<<0)
#define TREE_NEWTAGS            (1<<1)

#define TREE_PREORDER           (1<<0)
#define TREE_POSTORDER          (1<<1)
#define TREE_INORDER            (1<<2)
#define TREE_BREADTHFIRST       (1<<3)

#define TREE_NODE_LINK          (1<<0)

#define TREE_TRACE_UNSETS       (1<<3)
#define TREE_TRACE_WRITES       (1<<4)
#define TREE_TRACE_READS        (1<<5)
#define TREE_TRACE_CREATES      (1<<6)
#define TREE_TRACE_WCU  \
    (TREE_TRACE_UNSETS | TREE_TRACE_WRITES | TREE_TRACE_CREATES)
#define TREE_TRACE_MASK         \
    (TREE_TRACE_UNSETS | TREE_TRACE_WRITES | TREE_TRACE_READS | \
     TREE_TRACE_CREATES)

#define TREE_TRACE_WHENIDLE     (1<<8)
#define TREE_TRACE_FOREIGN_ONLY (1<<9)
#define TREE_TRACE_ACTIVE       (1<<10)

#define TREE_NOTIFY_CREATE      (1<<0)
#define TREE_NOTIFY_DELETE      (1<<1)
#define TREE_NOTIFY_MOVE        (1<<2)
#define TREE_NOTIFY_SORT        (1<<3)
#define TREE_NOTIFY_RELABEL     (1<<4)
#define TREE_NOTIFY_ALL         \
    (TREE_NOTIFY_CREATE | TREE_NOTIFY_DELETE | TREE_NOTIFY_MOVE | \
        TREE_NOTIFY_SORT | TREE_NOTIFY_RELABEL)
#define TREE_NOTIFY_MASK        (TREE_NOTIFY_ALL)

#define TREE_NOTIFY_WHENIDLE     (1<<8)
#define TREE_NOTIFY_FOREIGN_ONLY (1<<9)
#define TREE_NOTIFY_ACTIVE       (1<<10)

#define TREE_RESTORE_NO_TAGS    (1<<0)
#define TREE_RESTORE_OVERWRITE  (1<<1)

#define TREE_INCLUDE_ROOT       (1<<0)

typedef struct {
    int type;
    Blt_Tree tree;
    long inode;                         /* Node of event */
    Blt_TreeNode node;
    Tcl_Interp *interp;
} Blt_TreeNotifyEvent;

typedef struct {
    Blt_TreeNode node;                  /* Node being searched. */
    unsigned long nextIndex;            /* Index of next bucket to be
                                         * enumerated after present one. */
    Blt_TreeValue nextValue;            /* Next entry to be enumerated in the
                                         * the current bucket. */
} Blt_TreeKeyIterator;

/*
 * Blt_TreeObject --
 *
 *      Structure providing the internal representation of the tree object. A
 *      tree is uniquely identified by a combination of its name and
 *      originating namespace.  Two trees in the same interpreter can have the
 *      same names but must reside in different namespaces.
 *
 *      The tree object represents a general-ordered tree of nodes.  Each node
 *      may contain a heterogeneous collection of data values. Each value is
 *      identified by a field name and nodes do not need to contain the same
 *      data fields. Data field names are saved as reference counted strings
 *      and can be shared among nodes.
 *
 *      The tree is threaded.  Each node contains pointers to back its parents
 *      to its next sibling.  
 * 
 *      A tree object can be shared by several clients.  When a client wants
 *      to use a tree object, it is given a token that represents the tree.
 *      The tree object uses the tokens to keep track of its clients.  When
 *      all clients have released their tokens the tree is automatically
 *      destroyed.
 */

struct _Blt_TreeObject {
    Blt_TreeNode root;                  /* Root of the entire tree. */
    const char *sortNodesCmd;           /* TCL command to invoke to sort
                                         * entries */
    Blt_Chain clients;                  /* List of clients using this tree */
    Blt_Pool nodePool;
    Blt_Pool valuePool;
    Blt_HashTable nodeTable;            /* Table of node identifiers. Used to
                                         * search for a node pointer given an
                                         * inode.*/
    Blt_HashTable keyTable;             /* Table of string keys. */
    Blt_TreeInterpData *dataPtr;
    long nextInode;
    long numNodes;                      /* Always counts root node. */
    long depth;                         /* Maximum depth of the tree. */
    unsigned int flags;                 /* Internal flags. See definitions
                                         * below. */
    unsigned int notifyFlags;           /* Notification flags. See definitions
                                         * below. */
};

/*
 * _Blt_TreeNode --
 *
 *      Structure representing a node in a general ordered tree.  Nodes are
 *      identified by their index, or inode.  Nodes also have names, but nodes
 *      names are not unique and can be changed.  Inodes are valid even if the
 *      node is moved.
 *
 *      Each node can contain a list of data fields.  Fields are name-value
 *      pairs.  The values are represented by Tcl_Objs.
 *      
 */
struct _Blt_TreeNode {
    Blt_TreeNode parent;                /* Parent node. If NULL, then this is
                                         * the root node. */
    Blt_TreeNode next, prev;            /* Next/previous sibling nodes. */
    Blt_TreeNode hnext;                 /* Next node in the hash bucket. */
    Blt_TreeKey label;                  /* Node label (doesn't have to be
                                         * unique). */
    long inode;                         /* Serial number of the node. */
    Blt_TreeObject corePtr;             /* Pointer back to the tree object
                                         * that contains this node. */
    long depth;                         /* The depth of this node in the
                                         * tree. */
    long numChildren;                   /* # of children for this node. */
    Blt_TreeNode first, last;           /* First/last nodes of child nodes
                                         * stored as a linked list. */
    Blt_TreeNode *nodeTable;            /* Hash table of child nodes. */
    size_t nodeTableSize2;              /* Log2 size of child node hash
                                         * table. */
    Blt_TreeValue values;               /* Chain of Blt_TreeValue structures.
                                         * Each value structure contains a
                                         * key/value data pair.  The data
                                         * value is a Tcl_Obj. */
    Blt_TreeValue *valueTable;          /* Hash table for values. When the
                                         * number of values reaches exceeds a
                                         * threshold, values will also be
                                         * linked into this hash table. */
    unsigned short numValues;           /* # of values for this node. */
    unsigned short valueTableSize2;     /* Size of hash table indicated as a
                                         * power of 2 (e.g. if logSize=3, then
                                         * table size is 8). If 0, this
                                         * indicates that the node's values
                                         * are stored as a list. */
    unsigned int flags;                 /* Indicates if this node is currently
                                         * used within an active trace. */
};

struct _Blt_TreeTagEntry {
    const char *tagName;
    Blt_HashEntry *hashPtr;
    Blt_HashTable nodeTable;
};

struct _Blt_TreeTagTable {
    Blt_HashTable tagTable;
    int refCount;
};

/*
 * _Blt_Tree --
 *
 *      A tree may be shared by several clients.  Each client allocates this
 *      structure which acts as a ticket for using the tree.  Each client can
 *
 *      - Designate notifier routines that are automatically invoked by the
 *        tree object when nodes are created, deleted, moved, etc. by other
 *        clients.
 *      - Place traces on the values of specific nodes.
 *      - Manage its own set or common of tags for nodes of the tree. By
 *        default, clients share tags.
 */

struct _Blt_Tree {
    unsigned int magic;                 /* Magic value indicating whether a
                                         * generic pointer is really a
                                         * datatable token or not. */
    const char *name;                   /* Fully namespace-qualified name of
                                         * the client. */
    Blt_TreeObject corePtr;             /* Pointer to the structure containing
                                         * the master information about the
                                         * tree used by the client.  If NULL,
                                         * this indicates that the tree has
                                         * been destroyed (but as of yet, this
                                         * client hasn't recognized it). */
    Tcl_Interp *interp;                 /* Interpreter associated with this
                                         * tree. */
    Blt_HashEntry *hPtr;                /* This client's entry in the above
                                         * table. This is a list of clients
                                         * that all have the same qualified
                                         * table name (i.e. are sharing the
                                         * same table. */
    Blt_ChainLink link;                 /* Pointer to this link in the
                                         * server's chain of clients. */
    Blt_Chain events;                   /* Chain of node event handlers. */
    Blt_Chain readTraces;               /* List of possible callbacks when a
                                         * data field is read. */
    Blt_Chain writeTraces;              /* List of possible callbacks when a
                                         * data field is created, set, or
                                         * unset. */
    Blt_TreeNode root;                  /* Designated root for this client */
    Blt_TreeTagTable *tagTablePtr;      /* Tag table used by this client. */ 
};


typedef int (Blt_TreeNotifyEventProc)(ClientData clientData, 
        Blt_TreeNotifyEvent *eventPtr);

typedef int (Blt_TreeTraceProc)(ClientData clientData, Tcl_Interp *interp, 
        Blt_TreeNode node, Blt_TreeKey key, unsigned int flags);

typedef int (Blt_TreeEnumProc)(Blt_TreeNode node, Blt_TreeKey key, 
        Tcl_Obj *valuePtr);

typedef int (Blt_TreeCompareNodesProc)(Blt_TreeNode *n1Ptr, 
        Blt_TreeNode *n2Ptr);

typedef int (Blt_TreeApplyProc)(Blt_TreeNode node, ClientData clientData, 
        int order);

struct _Blt_TreeTrace {
    ClientData clientData;
    Blt_TreeKey key;
    Blt_TreeNode node;
    unsigned int mask;
    Blt_TreeTraceProc *proc;
};

typedef enum _Blt_TreeIterTypes {
    ITER_TYPE_SINGLE, ITER_TYPE_ALL, ITER_TYPE_TAG,
} Blt_TreeIterType;

typedef struct {
    Blt_TreeIterType type;
    Blt_TreeNode current;
    Blt_TreeNode root;
    Blt_HashSearch cursor;
} Blt_TreeIterator;


/*
 * Structure definition for information used to keep track of searches through
 * hash tables:
 */
struct _Blt_TreeKeyIterator {
    Blt_TreeNode node;                  /* Table being searched. */
    unsigned long nextIndex;            /* Index of next bucket to be
                                         * enumerated after present one. */
    Blt_TreeValue nextValue;            /* Next entry to be enumerated in the
                                         * the current bucket. */
};

BLT_EXTERN Blt_TreeKey Blt_Tree_GetKey(Blt_Tree tree, const char *string);
BLT_EXTERN Blt_TreeKey Blt_Tree_GetKeyFromNode(Blt_TreeNode node, 
        const char *string);
BLT_EXTERN Blt_TreeKey Blt_Tree_GetKeyFromInterp(Tcl_Interp *interp,
        const char *string);

BLT_EXTERN Blt_TreeNode Blt_Tree_CreateNode(Blt_Tree tree, Blt_TreeNode parent, 
        const char *name, long position); 

BLT_EXTERN Blt_TreeNode Blt_Tree_CreateNodeWithId(Blt_Tree tree, 
        Blt_TreeNode parent, const char *name, long inode, long position); 

BLT_EXTERN int Blt_Tree_DeleteNode(Blt_Tree tree, Blt_TreeNode node);

BLT_EXTERN int Blt_Tree_MoveNode(Blt_Tree tree, Blt_TreeNode node, 
        Blt_TreeNode parent, Blt_TreeNode before);

BLT_EXTERN Blt_TreeNode Blt_Tree_GetNodeFromIndex(Blt_Tree tree, long inode);

BLT_EXTERN Blt_TreeNode Blt_Tree_FindChild(Blt_TreeNode parent, 
        const char *name);

BLT_EXTERN Blt_TreeNode Blt_Tree_NextNode(Blt_TreeNode root, Blt_TreeNode node);

BLT_EXTERN Blt_TreeNode Blt_Tree_PrevNode(Blt_TreeNode root, Blt_TreeNode node);

BLT_EXTERN Blt_TreeNode Blt_Tree_FirstChild(Blt_TreeNode parent);

BLT_EXTERN Blt_TreeNode Blt_Tree_LastChild(Blt_TreeNode parent);

BLT_EXTERN int Blt_Tree_IsBefore(Blt_TreeNode node1, Blt_TreeNode node2);

BLT_EXTERN int Blt_Tree_IsAncestor(Blt_TreeNode node1, Blt_TreeNode node2);

BLT_EXTERN int Blt_Tree_PrivateValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key);

BLT_EXTERN int Blt_Tree_PublicValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key);

BLT_EXTERN int Blt_Tree_GetValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *string, Tcl_Obj **valuePtr);

BLT_EXTERN int Blt_Tree_ValueExists(Blt_Tree tree, Blt_TreeNode node, 
        const char *string);

BLT_EXTERN int Blt_Tree_SetValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr);

BLT_EXTERN int Blt_Tree_UnsetValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *string);

BLT_EXTERN int Blt_Tree_AppendValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *string, const char *value);

BLT_EXTERN int Blt_Tree_ListAppendValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *string, Tcl_Obj *valuePtr);

BLT_EXTERN int Blt_Tree_GetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *arrayName, const char *elemName, 
        Tcl_Obj **valueObjPtrPtr);

BLT_EXTERN int Blt_Tree_SetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *arrayName, const char *elemName, 
        Tcl_Obj *valueObjPtr);

BLT_EXTERN int Blt_Tree_UnsetArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *arrayName, const char *elemName);

BLT_EXTERN int Blt_Tree_AppendArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *arrayName, const char *elemName, 
        const char *value);

BLT_EXTERN int Blt_Tree_ListAppendArrayValue(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *arrayName, const char *elemName, 
        Tcl_Obj *valueObjPtr);

BLT_EXTERN int Blt_Tree_ArrayValueExists(Blt_Tree tree, Blt_TreeNode node, 
        const char *arrayName, const char *elemName);

BLT_EXTERN int Blt_Tree_ArrayNames(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, const char *arrayName, Tcl_Obj *listObjPtr);

BLT_EXTERN int Blt_Tree_GetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj **valuePtr);

BLT_EXTERN int Blt_Tree_SetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr);

BLT_EXTERN int Blt_Tree_UnsetValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key);

BLT_EXTERN int Blt_Tree_AppendValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key, const char *value);

BLT_EXTERN int Blt_Tree_ListAppendValueByKey(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode node, Blt_TreeKey key, Tcl_Obj *valuePtr);

BLT_EXTERN int Blt_Tree_ValueExistsByKey(Blt_Tree tree, Blt_TreeNode node, 
        Blt_TreeKey key);

BLT_EXTERN Blt_TreeKey Blt_Tree_FirstKey(Blt_Tree tree, Blt_TreeNode node, 
        Blt_TreeKeyIterator *iterPtr);

BLT_EXTERN Blt_TreeKey Blt_Tree_NextKey(Blt_Tree tree, 
        Blt_TreeKeyIterator *iterPtr);

BLT_EXTERN int Blt_Tree_Apply(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
        ClientData clientData);

BLT_EXTERN int Blt_Tree_ApplyDFS(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
        ClientData clientData, int order);

BLT_EXTERN int Blt_Tree_ApplyBFS(Blt_TreeNode root, Blt_TreeApplyProc *proc, 
        ClientData clientData);

BLT_EXTERN int Blt_Tree_SortNode(Blt_Tree tree, Blt_TreeNode node, 
        Blt_TreeCompareNodesProc *proc);

BLT_EXTERN int Blt_Tree_Exists(Tcl_Interp *interp, const char *name);

BLT_EXTERN Blt_Tree Blt_Tree_Open(Tcl_Interp *interp, const char *name, 
        int flags);

BLT_EXTERN void Blt_Tree_Close(Blt_Tree tree);

BLT_EXTERN int Blt_Tree_Attach(Tcl_Interp *interp, Blt_Tree tree, 
        const char *name);

BLT_EXTERN Blt_Tree Blt_Tree_GetFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr);

BLT_EXTERN int Blt_Tree_Size(Blt_TreeNode node);

BLT_EXTERN Blt_TreeTrace Blt_Tree_CreateTrace(Blt_Tree tree, Blt_TreeNode node, 
        const char *keyPattern, const char *tagName, unsigned int mask, 
        Blt_TreeTraceProc *proc, ClientData clientData);

BLT_EXTERN void Blt_Tree_DeleteTrace(Blt_TreeTrace token);

BLT_EXTERN void Blt_Tree_CreateEventHandler(Blt_Tree tree, unsigned int mask, 
        Blt_TreeNotifyEventProc *proc, ClientData clientData);

BLT_EXTERN void Blt_Tree_DeleteEventHandler(Blt_Tree tree, unsigned int mask, 
        Blt_TreeNotifyEventProc *proc, ClientData clientData);

BLT_EXTERN void Blt_Tree_RelabelNode(Blt_Tree tree, Blt_TreeNode node, 
        const char *string);
BLT_EXTERN void Blt_Tree_RelabelNodeWithoutNotify(Blt_TreeNode node,
        const char *string);

BLT_EXTERN const char *Blt_Tree_NodeIdAscii(Blt_TreeNode node);

BLT_EXTERN const char *Blt_Tree_NodePath(Blt_TreeNode node, 
        Tcl_DString *resultPtr);        

BLT_EXTERN const char *Blt_Tree_NodeRelativePath(Blt_TreeNode root, 
        Blt_TreeNode node, const char *separator, unsigned int flags, 
        Tcl_DString *resultPtr);

BLT_EXTERN long Blt_Tree_NodePosition(Blt_TreeNode node);

BLT_EXTERN void Blt_Tree_ClearTags(Blt_Tree tree, Blt_TreeNode node);
BLT_EXTERN int Blt_Tree_HasTag(Blt_Tree tree, Blt_TreeNode node, 
        const char *tagName);
BLT_EXTERN void Blt_Tree_AddTag(Blt_Tree tree, Blt_TreeNode node, 
        const char *tagName);
BLT_EXTERN void Blt_Tree_RemoveTag(Blt_Tree tree, Blt_TreeNode node,
        const char *tagName);
BLT_EXTERN void Blt_Tree_ForgetTag(Blt_Tree tree, const char *tagName);
BLT_EXTERN Blt_HashTable *Blt_Tree_TagHashTable(Blt_Tree tree, 
        const char *tagName);
BLT_EXTERN int Blt_Tree_TagTableIsShared(Blt_Tree tree);
BLT_EXTERN void Blt_Tree_NewTagTable(Blt_Tree tree);

BLT_EXTERN Blt_HashEntry *Blt_Tree_FirstTag(Blt_Tree tree, 
        Blt_HashSearch *searchPtr);

BLT_EXTERN void Blt_Tree_DumpNode(Blt_Tree tree, Blt_TreeNode root, 
        Blt_TreeNode node, Tcl_DString *resultPtr);

BLT_EXTERN int Blt_Tree_Dump(Blt_Tree tree, Blt_TreeNode root, 
        Tcl_DString *resultPtr);

BLT_EXTERN int Blt_Tree_DumpToFile(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode root, const char *fileName);

BLT_EXTERN int Blt_Tree_Restore(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode root, const char *string, unsigned int flags);

BLT_EXTERN int Blt_Tree_RestoreFromFile(Tcl_Interp *interp, Blt_Tree tree, 
        Blt_TreeNode root, const char *fileName, unsigned int flags);

BLT_EXTERN long Blt_Tree_Depth(Blt_Tree tree);

#define Blt_Tree_Name(token)    ((token)->name)
#define Blt_Tree_RootNode(token) ((token)->root)

#define Blt_Tree_NodeDegree(node) ((node)->numChildren)
#define Blt_Tree_NodeValues(node) ((node)->numValues)
#define Blt_Tree_NodeDepth(node) ((node)->depth)
#define Blt_Tree_NodeLabel(node) ((node)->label)
#define Blt_Tree_NodeId(node)    ((node)->inode)
#define Blt_Tree_NextNodeId(token)     ((token)->corePtr->nextInode)
#define Blt_Tree_ParentNode(node) ((node == NULL) ? NULL : (node)->parent)

#define Blt_Tree_IsLeaf(node)     ((node)->numChildren == 0)
#define Blt_Tree_IsLink(node)     ((node)->flags & TREE_NODE_LINK)

#define Blt_Tree_NextSibling(node) (((node) == NULL) ? NULL : (node)->next)
#define Blt_Tree_PrevSibling(node) (((node) == NULL) ? NULL : (node)->prev)

typedef int (Blt_TreeImportProc)(Tcl_Interp *interp, Blt_Tree tree, int objc, 
        Tcl_Obj *const *objv);

typedef int (Blt_TreeExportProc)(Tcl_Interp *interp, Blt_Tree tree, int objc, 
        Tcl_Obj *const *objv);

BLT_EXTERN int Blt_Tree_RegisterFormat(Tcl_Interp *interp, const char *fmtName, 
        Blt_TreeImportProc *importProc, Blt_TreeExportProc *exportProc);

BLT_EXTERN Blt_TreeTagEntry *Blt_Tree_RememberTag(Blt_Tree tree, 
        const char *name);

BLT_EXTERN int Blt_Tree_GetNodeFromObj(Tcl_Interp *interp, Blt_Tree tree,
        Tcl_Obj *objPtr, Blt_TreeNode *nodePtr);

BLT_EXTERN int Blt_Tree_GetNodeIterator(Tcl_Interp *interp, Blt_Tree tree,
        Tcl_Obj *objPtr, Blt_TreeIterator *iterPtr);

BLT_EXTERN Blt_TreeNode Blt_Tree_FirstTaggedNode(Blt_TreeIterator *iterPtr);
BLT_EXTERN Blt_TreeNode Blt_Tree_NextTaggedNode(Blt_TreeIterator *iterPtr);


#endif /* _BLT_TREE_H */

