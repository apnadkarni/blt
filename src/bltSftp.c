/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *
 * bltSftp.c --
 *
 *	Copyright 2012 George A Howlett.
 *
 *	Permission is hereby granted, free of charge, to any person
 *	obtaining a copy of this software and associated documentation
 *	files (the "Software"), to deal in the Software without
 *	restriction, including without limitation the rights to use, copy,
 *	modify, merge, publish, distribute, sublicense, and/or sell copies
 *	of the Software, and to permit persons to whom the Software is
 *	furnished to do so, subject to the following conditions:
 *
 *	The above copyright notice and this permission notice shall be
 *	included in all copies or substantial portions of the Software.
 *
 *	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *	MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 *	BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 *	ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *	CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *	SOFTWARE.
 */

/*
 *	sftp create ?name? \
 *		-host host \
 *		-user user \
 *		-password pass \
 *		-prompt cmd \
 *		-publickey file \
 *		-timeout seconds 
 *	sftp names
 *	sftp destroy stfp
 *
 *	set sftp [sftp create]
 *
 * TODO:
 *	-resume on put and get.
 *	sftp df path
 */
#include "bltInt.h"
#include "bltInitCmd.h"

#include "config.h"
#define _TCL_VERSION _VERSION(TCL_MAJOR_VERSION, TCL_MINOR_VERSION, TCL_RELEASE_SERIAL)
#ifndef NO_SFTP

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>
#endif	/* HAVE_SYS_STAT_H */

#ifdef HAVE_CTYPE_H
#  include <ctype.h>
#endif /* HAVE_CTYPE_H */

#include <libssh2.h>
#include <libssh2_sftp.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
# ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#ifndef WIN32
#include <netdb.h>
#endif


#define SFTP_THREAD_KEY "BLT Sftp Command Data"
#define KEYFILE		"~/.ssh/id_rsa.pub"
#define PATH_SEPARATOR	'/'

#define TRUE		1
#define FALSE		0
#undef MIN
#define MIN(a,b)	(((a)<(b))?(a):(b))
#undef MAX
#define MAX(a,b)	(((a)>(b))?(a):(b))

#define READ_TRACED	(1<<0)
#define WRITE_TRACED	(1<<1)
#define AUTH_PASSWORD	(1<<2)
#define AUTH_PUBLICKEY  (1<<3)
#define AUTH_MASK	(AUTH_PASSWORD|AUTH_PUBLICKEY)

#define APPEND		(1<<1)
#define RESUME		(1<<2)
#define FORCE		(1<<4)
#define RECURSE		(1<<5)

#define MAXPATHLEN	(1<<12)
	
#define DIR_OVERWRITE	(1<<0)
#define DIR_DOTFILES	(1<<1)
#define DIR_IGNORE	(1<<2)
#define DIR_LISTING	(1<<3)
#define DIR_LONG	(1<<12)

#define DIR_ATIME	(1<<4)
#define DIR_GID		(1<<5)
#define DIR_MODE	(1<<6)
#define DIR_MTIME	(1<<7)
#define DIR_NAME	(1<<8)
#define DIR_SIZE	(1<<9)
#define DIR_TYPE	(1<<10)
#define DIR_UID		(1<<11)
#define DIR_LONGENTRY	(1<<12)
#define DIR_DEFAULT	(DIR_SIZE|DIR_TYPE|DIR_MTIME|DIR_MODE|DIR_NAME)
#define DIR_ALL		(DIR_ATIME|DIR_GID|DIR_MODE|DIR_MTIME| \
			 DIR_NAME|DIR_SIZE|DIR_TYPE|DIR_UID|DIR_LONGENTRY)

#define TRACE_FLAGS (TCL_TRACE_WRITES | TCL_TRACE_UNSETS | TCL_GLOBAL_ONLY)

typedef struct {
    Tcl_Interp *interp;
    Blt_HashTable sessionTable;		/* Hash table of sftp sessions keyed by
					 * address. */
    size_t nextId;
} SftpCmdInterpData;

typedef struct {
    unsigned int flags;
    Tcl_Command cmdToken;		/* Token for tree's TCL command. */
    Tcl_Interp *interp;
    const char *name;
    char *homedir;
    int uid, gid;			/* User id and group id on sftp
					 * server. */
    const char *groupName;
    const char *userName;
    const char *groups;

    Blt_HashTable gidTable;		/* gid->groupName */
    Blt_HashEntry *hashPtr;
    Blt_HashTable *tablePtr;
    SftpCmdInterpData *dataPtr;    	/* Bookkeeping data associated with
					 * this interpreter.  */
    LIBSSH2_SESSION *session;		/*  */
    LIBSSH2_SFTP *sftp;			/*  */
    int sock;				/* Socket used to connected to sftp
					 * server.  */
    const char *cwd;                    /* Current home directory on remote. */
    int cwdLength;
    const char *user;			/* User name on sftp server. */
    const char *host;			/* Host name of sftp server. */
    const char *password;		
    const char *publickey;		/* Path of public key file. Defaults
					* to "~/.ssh/id_rsa.pub". */
    Tcl_Obj *cmdObjPtr;
    Tcl_Obj *promptCmdObjPtr;		/* TCL command to request a username
					 * and password combination from the
					 * user. */
    int idleTimeout;			/* If non-zero, number of seconds of
					 * idleness (no sftp activity) before
					 * disconnecting remote session.  */
    int numPasswordAttempts;		/* If promptCmdObjPtr is not NULL,
					 * this is the maximum number password
					 * attempts allowed. 0 indicates no
					 * limit. */
    Tcl_TimerToken idleTimerToken;	/* Token for timer handler which sets
					 * an idle timeout for the current
					 * sftp session. If zero, there's no
					 * timer handler queued. */
    Tcl_DString ds;
} Remote;

typedef struct {
    Tcl_Interp *interp;			/* Interpreter used to invoke progress
					 * commands. */
    Remote *remotePtr;			/* Current sftp session. */
    LIBSSH2_SFTP_HANDLE *handle;	/* Handle of file that we are reading
					 * from the remote. */
    int *donePtr;			/* Points to variable indicating status
					 * of last read. */
    FILE *f;				/* If non-NULL, open file where remote
					 * file's contents are saved. */
    /* User-defined fields. */
    unsigned int flags;
    const char *cancelVarName;		/* If non-NULL, name of TCL variable
					 * to be traced to indicate the read
					 * operation is canceled. */
    Tcl_Obj *progCmdObjPtr;		/* TCL command to be invoked when new
					 * data has been read from the
					 * server. */
    int timeout;			/* If non-zero, timeout reading after
					 * this many seconds. */
    long startTime;
    /* Reader-specific fields. */
    size_t maxSize; 			/* If non-zero, stop reading the file
					 * after we've retrieved this many
					 * bytes. */
    Blt_DBuffer dbuffer;		/* If non-NULL, buffer where remote
					 * file's contents are saved. */
    size_t size;			/* Size in bytes of the remote file. */
    size_t numRead;			/* The current number of bytes read. */
    size_t offset;			/* If > 0, this is number of bytes in
					 * the local file.  Seek this many
					 * bytes to resume reading. */
} FileReader;

typedef struct {
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    char name[1];
} RemoteEntry;

typedef struct {
    Tcl_Interp *interp;			/* Interpreter used to invoke progress
					 * commands. */
    Remote *remotePtr;			/* Current sftp session. */
    LIBSSH2_SFTP_HANDLE *handle;	/* Handle of file that we are reading
					 * from the remote. */
    int *donePtr;			/* Points to variable indicating status
					 * of last write. */
    FILE *f;				/* If non-NULL, open file where remote
					 * file's contents are saved. */
    /* User-defined fields. */
    unsigned int flags;
    const char *cancelVarName;		/* If non-NULL, name of TCL variable
					 * to be traced to indicate the write
					 * operation is canceled. */
    Tcl_Obj *progCmdObjPtr;		/* TCL command to be invoked when new
					 * data has been read from the
					 * server. */
    int timeout;			/* If non-zero, timeout writing after
					 * this many seconds. */
    long startTime;
    /* Writer-specific fields. */
    unsigned long mode;
    char *string;			/* If non-NULL, buffer where remote
					 * file's contents are saved. */
    size_t size;			/* Size of the remote file. */
    size_t totalBytesWritten;		/* Total bytes read. */

    ssize_t numBytesRead;		/* Current number of bytes in the read
					 * buffer. */
    char buffer[1<<15];
    char *bp;

} FileWriter;

typedef struct {
    Tcl_Interp *interp;
    Remote *remotePtr;
    LIBSSH2_SFTP_HANDLE *handle;
    int *donePtr;			/* Points to variable indicating status
					 * of last read. */
    Tcl_Obj *listObjPtr;

    /* User-defined fields. */
    unsigned int flags;
    int timeout;			/* If non-zero, timeout reading after
					 * this many seconds. */
    const char *match;
    int fileCount;
    BLT_TABLE table;
    Tcl_Obj *excludeObjPtr;		/* List of excluding patterns. */
} DirectoryReader;


typedef struct _ApplyData ApplyData;
typedef int (ApplyProc)(Tcl_Interp *interp, Remote *remotePtr, const char *path,
	int length, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr, ClientData clientData);
struct _ApplyData {
    ClientData clientData;			/*  */
    ApplyProc *fileProc;
    ApplyProc *dirProc;
};

static ApplyProc ChgrpRemoteFile;
static ApplyProc ChmodRemoteFile;
static ApplyProc RemoveRemoteDirectory;
static ApplyProc RemoveRemoteFile;

static Blt_SwitchParseProc TableSwitchParseProc;
static Blt_SwitchFreeProc TableSwitchFreeProc;
static Blt_SwitchCustom tableSwitch = {
    TableSwitchParseProc, NULL, TableSwitchFreeProc, NULL
};

static Blt_SwitchSpec sftpSwitches[] = 
{
    {BLT_SWITCH_STRING, "-user", "string",  (char *)NULL,
	Blt_Offset(Remote, user), 0},
    {BLT_SWITCH_STRING, "-host", "string",  (char *)NULL,
	Blt_Offset(Remote, host), 0},
    {BLT_SWITCH_INT_NNEG, "-numtries",  "number", (char *)NULL,
	Blt_Offset(Remote, numPasswordAttempts), 0},
    {BLT_SWITCH_STRING, "-password", "string",  (char *)NULL,
	Blt_Offset(Remote, password), 0},
    {BLT_SWITCH_OBJ,    "-prompt", "command", (char *)NULL,
	Blt_Offset(Remote, promptCmdObjPtr), 0},
    {BLT_SWITCH_STRING, "-publickey", "fileName", (char *)NULL,
	Blt_Offset(Remote, publickey), 0},
    {BLT_SWITCH_INT_NNEG, "-timeout", "seconds", (char *)NULL,
	Blt_Offset(Remote, idleTimeout), 0},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;			/* Indicates if -recurse was
					 * specified. */
    int gid;				/* Group to change to. */
} ChgrpSwitches;

static Blt_SwitchSpec chgrpSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-recurse", "", (char *)NULL,
	Blt_Offset(ChgrpSwitches, flags), 0, RECURSE},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;			/* Indicates if -recurse was
					 * specified. */
    unsigned int clearFlags;		/* Permission bits to clear.  */
    unsigned int setFlags;		/* Permission bits to set.  */
} ChmodSwitches;

static Blt_SwitchSpec chmodSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-recurse", "", (char *)NULL,
	Blt_Offset(ChmodSwitches, flags), 0, RECURSE},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int flags;
} DeleteSwitches;

static Blt_SwitchSpec deleteSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-force", "", (char *)NULL,
	Blt_Offset(DeleteSwitches, flags), 0, FORCE},
    {BLT_SWITCH_END}
};

static Blt_SwitchParseProc FieldsSwitchParseProc;
static Blt_SwitchCustom fieldsSwitch = {
    FieldsSwitchParseProc, NULL, NULL, (ClientData)0,
};

static Blt_SwitchSpec dirListSwitches[] = 
{
    {BLT_SWITCH_BOOLEAN,  "-dotfiles", "bool",	  (char *)NULL,
	Blt_Offset(DirectoryReader, flags), 0, DIR_DOTFILES},
    {BLT_SWITCH_OBJ,      "-exclude",	"list",		(char *)NULL,
	Blt_Offset(DirectoryReader, excludeObjPtr), 0},
    {BLT_SWITCH_CUSTOM,   "-fields",   "list",    (char *)NULL,
	Blt_Offset(DirectoryReader, flags), 0, 0, &fieldsSwitch},
    {BLT_SWITCH_BITMASK,  "-listing",  "",    (char *)NULL,
	Blt_Offset(DirectoryReader, flags), 0, DIR_LISTING},
    {BLT_SWITCH_BITMASK,  "-long",     "",    (char *)NULL,
	Blt_Offset(DirectoryReader, flags), 0, DIR_LONG},
    {BLT_SWITCH_CUSTOM,   "-table",    "name",    (char *)NULL,
	Blt_Offset(DirectoryReader, table), 0, 0, &tableSwitch},
    {BLT_SWITCH_INT_NNEG, "-timeout",  "seconds", (char *)NULL,
	Blt_Offset(DirectoryReader, timeout), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec getSwitches[] = 
{
    {BLT_SWITCH_STRING,    "-cancel",  "varName",   (char *)NULL,
	Blt_Offset(FileReader, cancelVarName), 0},
    {BLT_SWITCH_LONG_NNEG, "-maxsize",  "number",   (char *)NULL,
	Blt_Offset(FileReader, maxSize),       0},
    {BLT_SWITCH_OBJ,       "-progress", "command",  (char *)NULL,
	Blt_Offset(FileReader, progCmdObjPtr), 0},
    {BLT_SWITCH_BITMASK,   "-resume",   "",         (char *)NULL,
	Blt_Offset(FileReader, flags),         0, RESUME},
    {BLT_SWITCH_INT_NNEG,  "-timeout",  "seconds",  (char *)NULL,
	Blt_Offset(FileReader, timeout),       0},
    {BLT_SWITCH_END}
};

typedef struct {
    unsigned int mode;
} MkdirSwitches;

static Blt_SwitchSpec mkdirSwitches[] = 
{
    {BLT_SWITCH_INT, "-mode", "mode", (char *)NULL,
	Blt_Offset(MkdirSwitches, mode), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec putSwitches[] = 
{
    {BLT_SWITCH_STRING,    "-cancel",  "varName",   (char *)NULL,
	Blt_Offset(FileWriter, cancelVarName), 0},
    {BLT_SWITCH_BITMASK,   "-force",   "",          (char *)NULL,
	Blt_Offset(FileWriter, flags),         0, FORCE},
    {BLT_SWITCH_OBJ,       "-progress", "command",  (char *)NULL,
	Blt_Offset(FileWriter, progCmdObjPtr), 0},
    {BLT_SWITCH_INT,      "-mode",      "mode",     (char *)NULL,
	Blt_Offset(FileWriter, mode),          0},
    {BLT_SWITCH_BITMASK,  "-resume",    "",         (char *)NULL,
	Blt_Offset(FileWriter, flags),         0, RESUME},
    {BLT_SWITCH_INT_NNEG, "-timeout",   "seconds",  (char *)NULL,
	Blt_Offset(FileWriter, timeout),       0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec readSwitches[] = 
{
    {BLT_SWITCH_STRING,    "-cancel",  "varName",  (char *)NULL,
	Blt_Offset(FileReader, cancelVarName), 0},
    {BLT_SWITCH_OBJ,      "-progress", "command",  (char *)NULL,
	Blt_Offset(FileReader, progCmdObjPtr), 0},
    {BLT_SWITCH_LONG_NNEG,"-maxsize",  "size",     (char *)NULL,
	Blt_Offset(FileReader, maxSize),       0},
    {BLT_SWITCH_INT_NNEG, "-timeout",   "seconds", (char *)NULL,
	Blt_Offset(FileReader, timeout),       0},
    {BLT_SWITCH_END}
};

typedef struct {
    int flags;
} RenameSwitches;

static Blt_SwitchSpec renameSwitches[] = 
{
    {BLT_SWITCH_BITMASK, "-force", "", (char *)NULL,
	Blt_Offset(RenameSwitches, flags), 0, FORCE},
    {BLT_SWITCH_END}
};

typedef struct {
    Blt_Tree tree;
    Blt_TreeNode root;
    const char *rootPath;
    int rootOffset;
    unsigned int flags;
    int timeout;			/* If > 0, timeout reads after this
					 * amount of milliseconds. */
    int maxDepth;			/* If > 0, maximum depth to
					 * recurse. */
    Tcl_Obj *progCmdObjPtr;
    Tcl_Obj *excludeObjPtr;		/* List of excluding patterns. */
    const char *cancelVarName;
} TreeWriter;

static Blt_SwitchParseProc TreeNodeSwitchParseProc;
static Blt_SwitchCustom treeNodeSwitch = {
    TreeNodeSwitchParseProc, NULL, NULL, NULL
};

static Blt_SwitchSpec dirTreeSwitches[] = 
{
    {BLT_SWITCH_STRING,   "-cancel",	"varName",	(char *)NULL,
	Blt_Offset(TreeWriter, cancelVarName), 0},
    {BLT_SWITCH_INT_NNEG, "-depth",	"number",	(char *)NULL,
	Blt_Offset(TreeWriter, maxDepth),       0},
    {BLT_SWITCH_BITMASK,  "-dotfiles",	"",		(char *)NULL,
	Blt_Offset(TreeWriter, flags), 0, DIR_DOTFILES},
    {BLT_SWITCH_OBJ,      "-exclude",	"list",		(char *)NULL,
	Blt_Offset(TreeWriter, excludeObjPtr),	0},
    {BLT_SWITCH_CUSTOM,   "-fields",	"list",		(char *)NULL,
	Blt_Offset(TreeWriter, flags), 0, 0, &fieldsSwitch},
    {BLT_SWITCH_BITMASK,  "-overwrite",	"",		(char *)NULL,
	Blt_Offset(TreeWriter, flags), 0, DIR_OVERWRITE},
    {BLT_SWITCH_CUSTOM,   "-root",      "node",		(char *)NULL,
	Blt_Offset(TreeWriter, root),  0, 0, &treeNodeSwitch},
    {BLT_SWITCH_INT_NNEG, "-timeout",   "seconds",	(char *)NULL,
	Blt_Offset(TreeWriter, timeout),       0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec writeSwitches[] = 
{
    {BLT_SWITCH_BITMASK,   "-append",   "",        (char *)NULL,
	Blt_Offset(FileWriter, flags), 0, APPEND},
    {BLT_SWITCH_STRING,    "-cancel",   "varName", (char *)NULL,
	Blt_Offset(FileWriter, cancelVarName), 0},
    {BLT_SWITCH_OBJ,       "-progress", "command", (char *)NULL,
	Blt_Offset(FileWriter, progCmdObjPtr), 0},
    {BLT_SWITCH_INT,       "-mode",     "mode",    (char *)NULL,
	Blt_Offset(FileWriter, mode), 0},
    {BLT_SWITCH_INT_NNEG, "-timeout",   "seconds", (char *)NULL,
	Blt_Offset(FileWriter, timeout), 0},
    {BLT_SWITCH_END}
};

static const char *sftpErrList[] = {
    "unexpected OK response",		/* SSH_ERROR_OK */
    "end of file",			/* SSH_ERROR_EOF */
    "no such file or directory",	/* SSH_ERROR_NO_SUCH_FILE */
    "permission denied",		/* SSH_ERROR_PERMISSION_DENIED */
    "failure",				/* SSH_ERROR_FAILURE */
    "bad message",			/* SSH_ERROR_BAD_MESSAGE */
    "no connection",			/* SSH_ERROR_NO_CONNECTION */
    "connection lost",			/* SSH_ERROR_CONNECTION_LOST */
    "operation unsupported",		/* SSH_ERROR_OP_UNSUPPORTED */
    "invalid handle",			/* SSH_ERROR_INVALID_HANDLE */
    "no such path",			/* SSH_ERROR_NO_SUCH_PATH */
    "file already exists",		/* SSH_ERROR_FILE_ALREADY_EXISTS */
    "write protect",			/* SSH_ERROR_WRITE_PROTECT */
    "no media",				/* SSH_ERROR_NO_MEDIA */
    "no space on filesystem",		/* SSH_ERROR_NO_SPACE_ON_FILESYSTEM */
    "quota exceeded",			/* SSH_ERROR_QUOTA_EXCEEDED */
    "unknown principal",		/* SSH_ERROR_UNKNOWN_PRINCIPAL */
    "lock conflict",			/* SSH_ERROR_LOCK_CONFLICT */
    "directory is not empty",		/* SSH_ERROR_DIR_NOT_EMPTY */
    "not a directory",			/* SSH_ERROR_NOT_A_DIRECTORY */
    "invalid filename",			/* SSH_ERROR_INVALID_FILENAME */
    "link loop"				/* SSH_ERROR_LINK_LOOP */
    "cannot delete",			/* SSH_ERROR_CANNOT_DELETE */
    "invalid parameter",		/* SSH_ERROR_INVALID_PARAMETER	*/
    "file is a directory",		/* SSH_ERROR_FILE_IS_A_DIRECTORY */
    "byte range lock conflict",	      /* SSH_ERROR_BYTE_RANGE_LOCK_CONFLICT */
    "byte range lock refused",	       /* SSH_ERROR_BYTE_RANGE_LOCK_REFUSED */
    "delete pending",			/* SSH_ERROR_DELETE_PENDING */
    "file corrupt",			/* SSH_ERROR_FILE_CORRUPT */
    "owner invalid",			/* SSH_ERROR_OWNER_INVALID */
    "group invalid",			/* SSH_ERROR_GROUP_INVALID */
    "unsupported version",		/* SSH_ERROR_UNSUPPORTED_VERSION */
    "invalid packet",			/* SSH_ERROR_INVALID_PACKET */
    "tunnel error",			/* SSH_ERROR_TUNNEL_ERROR */
};
static int numRemoteErrors = sizeof(sftpErrList) / sizeof(char *);

static Tcl_InterpDeleteProc SftpCmdInterpDeleteProc;
static Tcl_CmdDeleteProc SftpCmdInstDeleteProc;
static Tcl_TimerProc RemoteIdleTimerProc;
static Tcl_VarTraceProc CancelRemoteReadVarProc;
static Tcl_VarTraceProc CancelRemoteWriteVarProc;

static Tcl_ObjCmdProc SftpObjCmdProc;
static Tcl_ObjCmdProc SftpCmdInstObjCmdProc;

DLLEXPORT extern Tcl_AppInitProc Blt_sftp_Init;

static const char *
SaveStringFromObj(Tcl_Obj *objPtr)
{
    int length;
    char *string, *copy;

    string = Tcl_GetStringFromObj(objPtr, &length);
    copy = Blt_AssertMalloc(length + 1);
    strcpy(copy, string);
    copy[length] = '\0';
    return copy;
}
/*
 *---------------------------------------------------------------------------
 *
 * FieldsSwitchParseProc --
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
FieldsSwitchParseProc(
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
	} else if ((c == 'n') && (strcmp(string, "name") == 0)) {
	    mask |= DIR_NAME;
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
	} else if ((c == 'm') && (strcmp(string, "mtime") == 0)) {
	    mask |= DIR_MTIME;
	} else if ((c == 'l') && (strcmp(string, "longentry") == 0)) {
	    mask |= DIR_LONGENTRY;
	} else if ((c == 'a') && (strcmp(string, "all") == 0)) {
	    mask |= DIR_ALL;
	} else if ((c == 'd') && (strcmp(string, "default") == 0)) {
	    mask |= DIR_DEFAULT;
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
 * TableSwitchFreeProc --
 *
 *	Free the table used.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
TableSwitchFreeProc(ClientData clientData, char *record, int offset, int flags)
{
    BLT_TABLE *tablePtr = (BLT_TABLE *)(record + offset);

    if (*tablePtr != NULL) {
	blt_table_close(*tablePtr);
	*tablePtr = NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TableSwitchParseProc --
 *
 *	Convert a string representing a datatable.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TableSwitchParseProc(
    ClientData clientData,	/* Flag indicating type of pattern. */
    Tcl_Interp *interp,		/* Not used. */
    const char *switchName,	/* Not used. */
    Tcl_Obj *objPtr,		/* String representation */
    char *record,		/* Structure record */
    int offset,			/* Offset to field in structure */
    int flags)			/* Not used. */
{
    BLT_TABLE *tablePtr = (BLT_TABLE *)(record + offset);
    BLT_TABLE table;
    const char *string;

    table = NULL;
    string = Tcl_GetString(objPtr);
    if (string[0] != '\0') {
	if (blt_table_open(interp, string, &table) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (*tablePtr != NULL) {
	blt_table_close(*tablePtr);
    }
    *tablePtr = table;
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * TreeNodeSwitchParseProc --
 *
 *	Convert a Tcl_Obj representing a node number into its integer value.
 *
 * Results:
 *	The return value is a standard TCL result.
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
TreeNodeSwitchParseProc(
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
    Blt_Tree tree  = clientData;
    long inode;

    if (Tcl_GetLongFromObj(interp, objPtr, &inode) != TCL_OK) {
	return TCL_ERROR;
    }
    *nodePtr = Blt_Tree_GetNode(tree, inode);
    return TCL_OK;
}


/*
 *---------------------------------------------------------------------------
 *
 * RemoteError --
 *
 *	Returns an error message for the current sftp error code.  Note
 *	that the message buffer is static and subsequent calls will
 *	overwrite the buffer.
 *
 *---------------------------------------------------------------------------
 */
static const char *
RemoteError(Remote *remotePtr) 
{
    static char mesg[200];
    int code;

    code = libssh2_sftp_last_error(remotePtr->sftp);
    if ((code >= 0) && (code < numRemoteErrors)) {
	return sftpErrList[code];
    }
    sprintf(mesg, "error code = %d", code);
    return mesg;
}

/*
 *---------------------------------------------------------------------------
 *
 * RemoteError --
 *
 *	Returns an error message for the current sftp error code.  Note
 *	that the message buffer is static and subsequent calls will
 *	overwrite the buffer.
 *
 *---------------------------------------------------------------------------
 */
static const char *
RemoteSessionError(Remote *remotePtr) 
{
    char *string;
    int length;

    libssh2_session_last_error(remotePtr->session, &string, &length, 0);
    return string;
}


/*
 *---------------------------------------------------------------------------
 *
 * GetSftpCmdInterpData --
 *
 *---------------------------------------------------------------------------
 */
static SftpCmdInterpData *
GetSftpCmdInterpData(Tcl_Interp *interp)
{
    SftpCmdInterpData *dataPtr;
    Tcl_InterpDeleteProc *proc;

    dataPtr = (SftpCmdInterpData *)
	Tcl_GetAssocData(interp, SFTP_THREAD_KEY, &proc);
    if (dataPtr == NULL) {
	dataPtr = Blt_AssertMalloc(sizeof(SftpCmdInterpData));
	dataPtr->interp = interp;
	dataPtr->nextId = 0;
	Tcl_SetAssocData(interp, SFTP_THREAD_KEY, SftpCmdInterpDeleteProc,
		 dataPtr);
	Blt_InitHashTable(&dataPtr->sessionTable, BLT_ONE_WORD_KEYS);
    }
    return dataPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRemote --
 *
 *	Find the sftp command associated with the TCL command "string".
 *	
 * Results:
 *	A pointer to the sftp command.  If no associated sftp command can be
 *	found, NULL is returned.  It's up to the calling routines to generate
 *	an error message.
 *
 *---------------------------------------------------------------------------
 */
static Remote *
GetRemote(SftpCmdInterpData *dataPtr, Tcl_Interp *interp, const char *string)
{
    Blt_ObjectName objName;
    Tcl_CmdInfo cmdInfo;
    Blt_HashEntry *hPtr;
    Tcl_DString ds;
    const char *sftpName;
    int result;

    /* Pull apart the sftp name and put it back together in a standard
     * format. */
    if (!Blt_ParseObjectName(interp, string, &objName, BLT_NO_ERROR_MSG)) {
	return NULL;			/* No such parent namespace. */
    }
    /* Rebuild the fully qualified name. */
    sftpName = Blt_MakeQualifiedName(&objName, &ds);
    result = Tcl_GetCommandInfo(interp, sftpName, &cmdInfo);
    Tcl_DStringFree(&ds);

    if (!result) {
	return NULL;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->sessionTable, 
			     (char *)(cmdInfo.objClientData));
    if (hPtr == NULL) {
	return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * FileHead --
 *
 *	Returns the "head" of the file path.
 *
 *---------------------------------------------------------------------------
 */
static const char *
FileHead(char *fileName)
{
    char *p;

    p = strrchr(fileName, '/');
    if (p != NULL) {
	*p = '\0';
    }
    return fileName;
}

/*
 *---------------------------------------------------------------------------
 *
 * FileTail --
 *
 *	Returns the "tail" of the file path.
 *
 *---------------------------------------------------------------------------
 */
static const char *
FileTail(const char *fileName)
{
    char *p;

    p = strrchr(fileName, '/');
    if (p == NULL) {
	return fileName;
    }
    return p + 1;
}

static const char *
FileJoin(const char *path1, const char *path2, Tcl_DString *resultPtr)
{
    Tcl_DStringInit(resultPtr);
    Tcl_DStringAppend(resultPtr, path1, -1);
    Tcl_DStringAppend(resultPtr, "/", 1);
    Tcl_DStringAppend(resultPtr, path2, -1);
    return Tcl_DStringValue(resultPtr);
}

#ifdef notdef
static int
FileSplit(const char *path, int length, int *argcPtr, char ***argvPtr)
{
    const char *p, *pend;
    int count, argc;
    char **argv;
    size_t addrSize;
    char *q, *buffer;

    count = 0;
    for (p = path, pend = p + length; p < pend; p++) {
	if (*p == PATH_SEPARATOR) {
	    count++;
	}
    }
    addrSize = (count + 1) * sizeof(char **);
    buffer = Blt_AssertMalloc(addrSize + length + 3);
    q = buffer + addrSize;
    argv = (char **)buffer;
    argc = 0;
    if (*path == '/') {
	argv[argc++] = q;
	*q++ = '/';
	*q++ = '\0';
	path++;
    }
    for (p = path; p < pend; /*empty*/) {
	argv[argc++] = q;
	/* Copy the component name into the next slot.*/
	for (/*empty*/; p < pend; p++) {
	    if (*p == PATH_SEPARATOR) {
		break;
	    }
	    *q++ = *p;
	}
	*q++ = '\0';
	if (p == pend) {
	    /* Trailing separators. */
	    break;
	}
	/* Skip the next bunch of separators. */
	for (/*empty*/; p < pend; p++) {
	    if (*p != PATH_SEPARATOR) {
		break;
	    }
	}
    }	
    argv[argc] = NULL;
    *argcPtr = argc;
    *argvPtr = argv;
    return TCL_OK;
}
#endif

static int
PromptUser(Tcl_Interp *interp, Remote *remotePtr)
{
    int i;
    int numAttempts;
    const char *user, *pass;

    numAttempts = remotePtr->numPasswordAttempts;
    if (numAttempts ==  0) {
	numAttempts = 1000;
    }
    user = remotePtr->user;
    pass = remotePtr->password;
    for (i = 0; i < numAttempts; i++) {
	Tcl_Obj **objv;
	Tcl_Obj *cmdObjPtr;
	Tcl_Obj *objPtr;
	int objc;
	int result;

	cmdObjPtr = Tcl_DuplicateObj(remotePtr->promptCmdObjPtr);
	Tcl_ListObjAppendElement(remotePtr->interp, cmdObjPtr, 
				 Tcl_NewStringObj(user, -1));
	Tcl_IncrRefCount(cmdObjPtr);
	Tcl_SetObjResult(interp, Tcl_NewStringObj("", -1));
	result = Tcl_EvalObjEx(interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    return TCL_ERROR;		/* Error in callback routine */
	}
	objPtr = Tcl_GetObjResult(interp);
	if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;		/* Bad list. */
	}
	if (objc == 0) {
	    return TCL_BREAK;		/* User gives up. */
	}
	if (objc != 2) {
	    Tcl_AppendResult(interp, "wrong # of elements for prompt result: ",
			     "should be \"user password\"", (char *)NULL);
	    return TCL_ERROR;
	}
	user = Tcl_GetString(objv[0]);
	pass = Tcl_GetString(objv[1]);
	if (libssh2_userauth_password(remotePtr->session, user, pass) == 0) {
	    if (remotePtr->user != NULL) {
		Blt_Free(remotePtr->user);
		remotePtr->user = NULL;
	    }
	    remotePtr->user = SaveStringFromObj(objv[0]);
	    if (remotePtr->password != NULL) {
		Blt_Free(remotePtr->password);
		remotePtr->password = NULL;
	    }
	    remotePtr->password = SaveStringFromObj(objv[1]);
	    Tcl_DecrRefCount(objPtr);
	    remotePtr->flags &= ~AUTH_MASK;
	    remotePtr->flags |= AUTH_PASSWORD;
	    return TCL_OK;
	}
    }
    Tcl_AppendResult(interp, "password authorization to \"", 
	remotePtr->host, "\" failed: ", RemoteSessionError(remotePtr), 
	"\n", (char *)NULL);
    return TCL_ERROR;
}

static const char *
GetFileTypeFromAttributes(LIBSSH2_SFTP_ATTRIBUTES *attrsPtr)
{
    const char *type;

    if (LIBSSH2_SFTP_S_ISREG(attrsPtr->permissions)) {
	type = "file";
    } else if (LIBSSH2_SFTP_S_ISDIR(attrsPtr->permissions)) {
	type = "directory";
    } else if (LIBSSH2_SFTP_S_ISFIFO(attrsPtr->permissions)) {
	type = "fifo";
    } else if (LIBSSH2_SFTP_S_ISBLK(attrsPtr->permissions)) {
	type = "blockSpecial";
    } else if (LIBSSH2_SFTP_S_ISLNK(attrsPtr->permissions)) {
	type = "link";
    } else if (LIBSSH2_SFTP_S_ISCHR(attrsPtr->permissions)) {
	type = "characterSpecial";
    } else if (LIBSSH2_SFTP_S_ISSOCK(attrsPtr->permissions)) {
	type = "socket";
    } else {
	type = "???";
    }
    return type;
}

static RemoteEntry *
NewRemoteEntry(const char *name, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr)
{
    RemoteEntry *entryPtr;
    Blt_ChainLink link;

    link = Blt_Chain_AllocLink(sizeof(RemoteEntry) + strlen(name));
    entryPtr = Blt_Chain_GetValue(link);
    strcpy(entryPtr->name, name);
    memcpy(&entryPtr->attrs, attrsPtr, sizeof(LIBSSH2_SFTP_ATTRIBUTES));
    return entryPtr;
}

static void INLINE
FreeRemoteEntries(Blt_Chain entries) 
{
    Blt_Chain_Destroy(entries);
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadRemoteEntry --
 *
 *---------------------------------------------------------------------------
 */
static int
ReadRemoteEntry(Tcl_Interp *interp, LIBSSH2_SFTP_HANDLE *handle, 
	      Blt_Chain entries)
{
    char bytes[2048];
    char longentry[2048];
    ssize_t numBytes;
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    numBytes = libssh2_sftp_readdir_ex(handle, bytes, sizeof(bytes),
		longentry, sizeof(longentry), &attrs);
    if (numBytes < 0) {
	if (numBytes == LIBSSH2_ERROR_EAGAIN) {
	    return TCL_OK;		/* Would block.  */
	}
	return TCL_ERROR;		/* Error occurred. */
    }
    if (numBytes == 0) {
	return TCL_BREAK;		/* End-of-directory. */
    }	
    if ((strcmp(bytes, ".") != 0) && (strcmp(bytes, "..") != 0)) {
	RemoteEntry *entryPtr;

	entryPtr = NewRemoteEntry(bytes, &attrs);
	Blt_Chain_Append(entries, entryPtr);
    }
    return TCL_OK; 
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadRemoteEntries --
 *
 *	Retrieves the listing of the designed directory on the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static Blt_Chain
ReadRemoteEntries(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
		  int length)
{
    LIBSSH2_SFTP_HANDLE *handle;
    int result;
    Blt_Chain chain;

    libssh2_session_set_blocking(remotePtr->session, FALSE);
    do {
	handle = libssh2_sftp_open_ex(remotePtr->sftp, path, length,
		LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENDIR);
	if (handle == NULL) {
	    if (libssh2_session_last_errno(remotePtr->session) != 
		LIBSSH2_ERROR_EAGAIN) {
		break;
	    }
	}
    } while (handle == NULL);
    if (!handle) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't open directory \"", path, "\": ",
		RemoteError(remotePtr), (char *)NULL);
	}
	libssh2_session_set_blocking(remotePtr->session, TRUE);
	return NULL;
    }
    chain = Blt_Chain_Create();
    result = ReadRemoteEntry(interp, handle, chain);
    while (result == TCL_OK) {
	result = ReadRemoteEntry(interp, handle, chain);
	Tcl_DoOneEvent(TCL_DONT_WAIT); 
    }
    libssh2_sftp_closedir(handle);
    libssh2_session_set_blocking(remotePtr->session, TRUE);
    if (result == TCL_ERROR) {
	FreeRemoteEntries(chain);
	return NULL;
    }
    return chain;
}

	
static int
ApplyToRemote(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
	      int length, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr, Blt_Chain chain, 
	      ApplyData *dataPtr)
{
    Blt_ChainLink link, next;
    
    /* Pass 1: Apply to files. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
	RemoteEntry *entryPtr;
	const char *fullPath;
	Tcl_DString ds;
	int length;
	
	next = Blt_Chain_NextLink(link);
	entryPtr = Blt_Chain_GetValue(link);
	if (LIBSSH2_SFTP_S_ISDIR(entryPtr->attrs.permissions)) {
	    continue;
	}
	fullPath = FileJoin(path, entryPtr->name, &ds);
	length = Tcl_DStringLength(&ds);
	if ((*dataPtr->fileProc)(interp, remotePtr, fullPath, length, 
		&entryPtr->attrs, dataPtr->clientData) != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	Tcl_DStringFree(&ds);
    }
    /* Pass 2: Recursively apply to subdirectories. */
    for (link = Blt_Chain_FirstLink(chain); link != NULL; link = next) {
	RemoteEntry *entryPtr;
	const char *fullPath;
	Tcl_DString ds;
	int fullLen;
	Blt_Chain subentries;
	int result;

	next = Blt_Chain_NextLink(link);
	entryPtr = Blt_Chain_GetValue(link);
	if ((*dataPtr->dirProc)(interp, remotePtr, path, length, attrsPtr,
		 dataPtr->clientData) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (!LIBSSH2_SFTP_S_ISDIR(entryPtr->attrs.permissions)) {
	    continue;
	}
	/* This is a subdirectory. */
	fullPath = FileJoin(path, entryPtr->name, &ds);
	fullLen = Tcl_DStringLength(&ds);
	subentries = ReadRemoteEntries(NULL, remotePtr, fullPath, fullLen);
	if (subentries == NULL) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	result = ApplyToRemote(interp, remotePtr, fullPath, fullLen, 
		&entryPtr->attrs, subentries, dataPtr);
	Tcl_DStringFree(&ds);
	FreeRemoteEntries(subentries);
	if (result != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (dataPtr->dirProc != NULL) {
	if ((*dataPtr->dirProc)(interp, remotePtr, path, length, attrsPtr,
		 dataPtr->clientData) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadEntryIntoTree --
 *
 *---------------------------------------------------------------------------
 */
static int
ReadEntryIntoTree(Tcl_Interp *interp, LIBSSH2_SFTP_HANDLE *handle, 
		  TreeWriter *writerPtr, Blt_TreeNode parent)
{
    char bytes[2048];
    char longentry[2048];
    ssize_t numBytes;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    Tcl_Obj *objPtr;
    Blt_TreeNode node;
    Blt_Tree tree;

    numBytes = libssh2_sftp_readdir_ex(handle, bytes, sizeof(bytes),
		longentry, sizeof(longentry), &attrs);
    if (numBytes < 0) {
	if (numBytes == LIBSSH2_ERROR_EAGAIN) {
	    return TCL_OK;		/* Would block.  */
	}
	return TCL_ERROR;		/* Error occurred. */
    }
    if (numBytes == 0) {
	return TCL_BREAK;		/* End-of-directory. */
    }	
    if (bytes[0] == '.') {
	if ((writerPtr->flags & DIR_DOTFILES) == 0) {
	    return TCL_OK;		/* Ignore dotfiles. */
	}
	if ((bytes[1] == '\0') || ((bytes[1] == '.') && (bytes[2] == '\0'))) {
	    return TCL_OK;		/* Ignore "." and ".." */
	}
    }
    if (writerPtr->excludeObjPtr != NULL) {
	int i;
	Tcl_Obj **objv;
	int objc;

	if (Tcl_ListObjGetElements(interp, writerPtr->excludeObjPtr, 
				   &objc, &objv) != TCL_OK) {
	    return TCL_ERROR;
	}
	for (i = 0; i < objc; i++) {
	    const char *pattern;

	    pattern = Tcl_GetString(objv[i]);
	    if (Tcl_StringMatch(bytes, pattern)) {
		return TCL_OK;
	    }
	}
    }
    tree = writerPtr->tree;
    if (writerPtr->flags & DIR_OVERWRITE) {
	node = Blt_Tree_FindChild(parent, "bytes");
	if (node == NULL) {
	    node = Blt_Tree_CreateNode(tree, parent, bytes, -1);
	}
    } else {
	node = Blt_Tree_CreateNode(tree, parent, bytes, -1);
    }
    if (node == NULL) {	
	return TCL_ERROR;
    }
    /* type */
    if ((writerPtr->flags & DIR_TYPE) &&
	(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)) {
	objPtr = Tcl_NewStringObj(GetFileTypeFromAttributes(&attrs), -1);
	if (Blt_Tree_SetValue(interp, tree, node, "type", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /* size */
    if ((writerPtr->flags & DIR_SIZE) &&
	(attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)) {
	objPtr = Tcl_NewLongObj(attrs.filesize);
	if (Blt_Tree_SetValue(interp, tree, node, "size", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /* uid */
    if ((writerPtr->flags & DIR_UID) &&
	(attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID)) {
	objPtr = Tcl_NewLongObj(attrs.uid);
	if (Blt_Tree_SetValue(interp, tree, node, "uid", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /* gid */
    if ((writerPtr->flags & DIR_GID) && 
	(attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID)) {
	objPtr = Tcl_NewLongObj(attrs.gid);
	if (Blt_Tree_SetValue(interp, tree, node, "gid", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /* atime */
    if ((writerPtr->flags & DIR_ATIME) && 
	(attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)) {
	objPtr = Tcl_NewLongObj(attrs.atime);
	if (Blt_Tree_SetValue(interp, tree, node, "atime", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /* mtime */
    if ((writerPtr->flags & DIR_MTIME) && 
	(attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME)) {
	objPtr = Tcl_NewLongObj(attrs.mtime);
	if (Blt_Tree_SetValue(interp, tree, node, "mtime", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    /* mode */
    if ((writerPtr->flags & DIR_MODE) && 
	(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)) {
	objPtr = Tcl_NewLongObj(attrs.permissions & 07777);
	if (Blt_Tree_SetValue(interp, tree, node, "mode", objPtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadDirectoryIntoTree --
 *
 *	Retrieves the listing of the designed directory on the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadDirectoryIntoTree(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
		      int length, TreeWriter *writerPtr, Blt_TreeNode parent)
{
    LIBSSH2_SFTP_HANDLE *handle;
    int result, depth;
    Blt_TreeNode child;
    Tcl_DString ds;

    libssh2_session_set_blocking(remotePtr->session, TRUE); 
    do {
	handle = libssh2_sftp_open_ex(remotePtr->sftp, path, length,
		LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENDIR);
	if (handle == NULL) {
	    if (libssh2_session_last_errno(remotePtr->session) != 
		LIBSSH2_ERROR_EAGAIN) {
		break;
	    }
	}
    } while (handle == NULL);
    if (!handle) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't open directory \"", path, "\": ",
		RemoteError(remotePtr), (char *)NULL);
	}
	libssh2_session_set_blocking(remotePtr->session, TRUE);
	return TCL_OK;			/* Ignore errors on remote
					 * directories.*/
    }
    result = ReadEntryIntoTree(interp, handle, writerPtr, parent);
    while (result == TCL_OK) {
	result = ReadEntryIntoTree(interp, handle, writerPtr, parent);
	Tcl_DoOneEvent(TCL_DONT_WAIT); 
    }

    libssh2_sftp_closedir(handle);
    libssh2_session_set_blocking(remotePtr->session, TRUE);
    if (result == TCL_ERROR) {
	return TCL_ERROR;
    }

    depth = Blt_Tree_NodeDepth(parent) - Blt_Tree_NodeDepth(writerPtr->root);
    if ((writerPtr->maxDepth >= 0) && (writerPtr->maxDepth <= depth)) {
	return TCL_OK;
    }

    /* Now recurse into subdirectories. */
    Tcl_DStringInit(&ds);
    Tcl_DStringAppend(&ds, path, length);
    for (child = Blt_Tree_FirstChild(parent); child != NULL; 
	 child = Blt_Tree_NextSibling(child)) {
	Tcl_Obj *objPtr;
	const char *label;
	const char *value;

	/* Type field must be "directory". */
	if (Blt_Tree_GetValue(interp, writerPtr->tree, child, "type", &objPtr)
	    != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	value = Tcl_GetString(objPtr);
	if ((value[0] != 'd') || (strcmp(value, "directory") != 0)) {
	    continue;
	}
	label = Blt_Tree_NodeLabel(child);
	Tcl_DStringAppend(&ds, "/", 1);
	Tcl_DStringAppend(&ds, label, -1);
	result = ReadDirectoryIntoTree(interp, remotePtr, 
				       Tcl_DStringValue(&ds), 
				       Tcl_DStringLength(&ds),
				       writerPtr, child);
	Tcl_DStringSetLength(&ds, length);
	if (result != TCL_OK) {
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
    }
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

static int
GetPermsFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, unsigned int *setFlagsPtr,
		unsigned int *clearFlagsPtr)
{
    const char *string;

    string = Tcl_GetString(objPtr);
    if (isdigit(string[0])) {
	unsigned int mode;
	char *endPtr;
	
	mode = strtoul(string, &endPtr, 8);
	if (*endPtr != '\0') {
	    Tcl_AppendResult(interp, "can't parse \"", 
		Tcl_GetString(objPtr), "\"", (char *)NULL);
		return TCL_ERROR;
	}
	*clearFlagsPtr = 07777;
	*setFlagsPtr = mode;
	return TCL_OK;
    } else {
	const char *copy;
	char *p;
	unsigned int accumSetFlags, accumClearFlags;

	accumSetFlags = accumClearFlags = 0;
	copy = Blt_AssertStrdup(string);
	for (p = strtok((char *)copy, ",");  p != NULL; p = strtok(NULL, ",")) {
	    unsigned int bits, sticky, setid, setFlags, clearFlags, setMask;
	    char *q, *modePtr, *operPtr;
	    
	    for (q = p; *q != '\0'; q++) {
		if ((*q == '=') || (*q == '-') || (*q == '+')) {
		    break;
		}
	    }
	    if (*q == '\0') {
		Tcl_AppendResult(interp, "bad mode \"", string, "\"",
				 (char *)NULL);
		Blt_Free(copy);
		return TCL_ERROR;	/* Invalid mode flags  */
	    }
	    operPtr = q;
	    modePtr = operPtr + 1;
	    sticky = setid = bits = 0;
	    for (q = modePtr; *q != '\0'; q++) {
		switch (*q) {
		case 'r':
		    bits |= 4;	break;
		case 'w':
		    bits |= 2;	break;
		case 'x':
		    bits |= 1;	break;
		case 's':
		    setid = 1;	break;
		case 't':
		    sticky = 1;	break;
		}
	    }
	    if (bits == 0) {
		continue;		/* No bits to be set/unset */
	    }
	    setFlags = clearFlags = setMask = 0;
	    if (operPtr == p) {
		setFlags |= (bits) | (bits << 3) | (bits << 6); 
		setMask |= 07777;
	    } else {
		for (q = p; q < operPtr; q++) {
		    switch (*q) {
		    case 'o':
			setFlags |= bits;	
			setMask |= 00007;
			if (sticky) {
			    setFlags |= 01000;
			}
			break;
		    case 'g':
			setFlags |= (bits << 3);  
			setMask |= 00070;
			if (setid) {
			    setFlags |= 02000;
			}
			break;
		    case 'u':
			setFlags |= (bits << 6); 
			setMask |= 00700;
			if (setid) {
			    setFlags |= 04000;
			}
			break;
		    case 'a':
			setFlags |= (bits) | (bits << 3) | (bits << 6); 
			setMask |= 07777;
			if (sticky) {
			    setFlags |= 01000;
			}
			if (setid) {
			    setFlags |= 06000;
			}
			break;
		    }
		}
	    }
	    switch (*operPtr) {
	    case '=':
		clearFlags = setMask;
		break;
	    case '+':
		clearFlags = 0;
		break;
	    case '-':
		clearFlags = setFlags;
		setFlags = 0;
		break;
	    }
	    accumClearFlags |= clearFlags;
	    accumSetFlags |= setFlags;
	}
	Blt_Free(copy);
	*setFlagsPtr = accumSetFlags;
	*clearFlagsPtr = accumClearFlags;
    }
    return TCL_OK;
}

#ifdef notdef
/*
 *---------------------------------------------------------------------------
 *
 * WaitForSocket --
 *
 *	Adds a delay when polling for data in non-blocking mode.  This
 *	adds significant delay to reading files.
 *
 *---------------------------------------------------------------------------
 */
static int 
WaitForSocket(int socket_fd, LIBSSH2_SESSION *session)
{
    struct timeval timeout;
    int rc;
    fd_set fd;
    fd_set *writefd = NULL;
    fd_set *readfd = NULL;
    int dir;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;

    FD_ZERO(&fd);

    FD_SET(socket_fd, &fd);

    /* now make sure we wait in the correct direction */
    dir = libssh2_session_block_directions(session);

    if(dir & LIBSSH2_SESSION_BLOCK_INBOUND) {
	readfd = &fd;
    }
    if(dir & LIBSSH2_SESSION_BLOCK_OUTBOUND) {
	writefd = &fd;
    }
    rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

    return rc;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * CreateSocketAddress --
 *
 *	This function initializes a sockaddr structure for a host and port.
 *
 * Results:
 *	1 if the host was valid, 0 if the host could not be converted to an IP
 *	address.
 *
 * Side effects:
 *	Fills in the *sockaddrPtr structure.
 *
 *----------------------------------------------------------------------
 */

static int
CreateSocketAddress(
    struct sockaddr_in *sockaddrPtr,	/* Socket address */
    const char *host,			/* Host. NULL implies INADDR_ANY */
    int port)				/* Port number */
{
    struct hostent *hostent;		/* Host database entry */
    struct in_addr addr;		/* For 64/32 bit madness */

    memset(sockaddrPtr, 0, sizeof(struct sockaddr_in));
    sockaddrPtr->sin_family = AF_INET;
    sockaddrPtr->sin_port = htons((u_short) (port & 0xFFFF));
    if (host == NULL) {
	addr.s_addr = INADDR_ANY;
    } else {
	addr.s_addr = inet_addr(host);
	if (addr.s_addr == INADDR_NONE) {
	    hostent = gethostbyname(host);
	    if (hostent != NULL) {
		memcpy(&addr, hostent->h_addr, (size_t) hostent->h_length);
	    } else {
#ifdef	EHOSTUNREACH
		Tcl_SetErrno(EHOSTUNREACH);
#else
#ifdef ENXIO
		Tcl_SetErrno(ENXIO);
#endif
#endif
		return 0;		/* Error. */
	    }
	}
    }
    sockaddrPtr->sin_addr.s_addr = addr.s_addr;
    return 1;				/* Success. */
}

static int
AuthenticateRemote(Tcl_Interp *interp, Remote *remotePtr)
{
    const char *authtypes, *p, *copy;

    /* Check what authentication methods are available */
    authtypes = libssh2_userauth_list(remotePtr->session, remotePtr->user, 
	strlen(remotePtr->user));
    copy = Blt_AssertStrdup(authtypes);
    for (p = strtok((char *)copy, ","); p != NULL; p = strtok(NULL, ",")) {
	if (strcmp(p, "password") == 0) {
	    int result;

	    if (remotePtr->promptCmdObjPtr != NULL) {
		result = PromptUser(interp, remotePtr);
	    } else {
		result = libssh2_userauth_password(remotePtr->session, 
			remotePtr->user, remotePtr->password);
		if (result == 0) {
		    result = TCL_OK;
		}
	    }
	    if (result == TCL_OK) {
		Blt_Free(copy);
		remotePtr->flags &= ~AUTH_MASK;
		remotePtr->flags |= AUTH_PASSWORD;
		return TCL_OK;
	    }
	    Tcl_AppendResult(interp, "password authorization to \"", 
		remotePtr->host, "\" failed: ", RemoteSessionError(remotePtr), 
		"\n", (char *)NULL);
	} else if (strcmp(p, "publickey") == 0) {
	    Tcl_DString ds1, ds2;
	    char *p, *publicKey, *privateKey;
	    int result, length;
	    
	    Tcl_DStringInit(&ds1);
	    publicKey = Tcl_TranslateFileName(interp, remotePtr->publickey,
		&ds1);
	    Tcl_DStringInit(&ds2);
	    p = strrchr(publicKey, '.');
	    length = (p == NULL) ? Tcl_DStringLength(&ds1) : (p - publicKey);
	    Tcl_DStringAppend(&ds2, publicKey, length);
	    privateKey = Tcl_DStringValue(&ds2);
	    result = libssh2_userauth_publickey_fromfile(remotePtr->session, 
		remotePtr->user, publicKey, privateKey, remotePtr->password);
	    Tcl_DStringFree(&ds1);
	    Tcl_DStringFree(&ds2);
	    if (result == 0) {
		Blt_Free(copy);
		remotePtr->flags &= ~AUTH_MASK;
		remotePtr->flags |= AUTH_PUBLICKEY;
		return TCL_OK;
	    }
	    Tcl_AppendResult(interp, "public key authorization to \"", 
		remotePtr->host, "\" failed: ", RemoteSessionError(remotePtr), 
			"\n", (char *)NULL);
	} else if (strcmp(p, "keyboard-interactive") == 0) {
	    Tcl_AppendResult(interp, "keyboard-interactive authorization to \"",
		remotePtr->host, "\" failed: ", RemoteSessionError(remotePtr), 
		"\n", (char *)NULL);
	    /* empty */
	} 
    }
    if (interp != NULL) {
	Tcl_AppendResult(interp, "can't connect to \"", remotePtr->host, 
		"\": support types \"", authtypes, "\" all failed",
		(char *)NULL);
    }
    Blt_Free(copy);
    return TCL_ERROR;
}

static Tcl_Obj *
ExecOnRemote(Tcl_Interp *interp, Remote *remotePtr, const char *command, 
	     int length) 
{
    char bytes[MAXPATHLEN+1];
    ssize_t numBytes;
    LIBSSH2_CHANNEL *channel;
    int result;
    Tcl_Obj *objPtr;

    channel = libssh2_channel_open_session(remotePtr->session);
    if (channel == NULL) {
	Tcl_AppendResult(interp, "can't get channel from session", 
			 (char *)NULL);
	return NULL;
    }
    result = libssh2_channel_process_startup(channel, "exec", 4,  command, 
					     length);
    if (result < 0) {
	Tcl_AppendResult(interp, "can't start \"", command, "\" process", 
			 (char *)NULL);
	return NULL;
    } 
    objPtr = Tcl_NewStringObj("", 0);
    for (;;) {
	numBytes = libssh2_channel_read(channel, bytes, MAXPATHLEN);
	if (numBytes < 0) {
	    Tcl_AppendResult(interp, "error reading \"", command, "\": ",
			RemoteError(remotePtr), (char *)NULL);
	    goto error;
	}	
	if (numBytes == 0) {
	    break;
	}
	Tcl_AppendToObj(objPtr, bytes, numBytes);
    }
    libssh2_channel_free(channel);
    libssh2_channel_close(channel);
    return objPtr;
 error:
    libssh2_channel_free(channel);
    libssh2_channel_close(channel);
    Tcl_DecrRefCount(objPtr);
    return NULL;
}

static int
GetRemoteHome(Tcl_Interp *interp, Remote *remotePtr) 
{
    Tcl_Obj *objPtr;
    const char *string;
    int length;

    /* This only works on servers where there is a "pwd" command. */
    objPtr = ExecOnRemote(interp, remotePtr, "pwd", 3);
    if (objPtr == NULL) {
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objPtr, &length);
    remotePtr->homedir = Blt_AssertMalloc(length + 1);
    strncpy(remotePtr->homedir, string, (size_t)length);
    remotePtr->homedir[length] = '\0';
    if (remotePtr->homedir[length - 1] == '\n') {
	remotePtr->homedir[length - 1] = '\0';
    }
    Tcl_DecrRefCount(objPtr);
    return TCL_OK;
}

static int
ParseGroups(Tcl_Interp *interp, Remote *remotePtr, char *groups)
{
    char *p;

    /* Parse the list of group entries, separated by commas. */
    for (p = strtok(groups, ","); p != NULL; p = strtok(NULL, ",")) {
	Blt_HashEntry *hPtr;
	int isNew;
	char *q, *name;
	unsigned long id;

	/* Each group entry is in the form "id(name)". */
	q = NULL;
	id = strtoul(p, &q, 10);
	if ((q == NULL) || (*q != '(')) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "invalid group record \"", p, "\"", 
			(char *)NULL);
	    }
	    return TCL_ERROR;;
	}
	name = q + 1;
	q = strchr(name, ')');
	if (q == NULL) {
	    if (interp != NULL) {
		Tcl_AppendResult(interp, "invalid group record \"", p, "\"", 
			(char *)NULL);
	    }
	    return TCL_ERROR;;
	}
	*q = '\0';

	/* Add the group entry to the hash table. */
	hPtr = Blt_CreateHashEntry(&remotePtr->gidTable, (char *)id, &isNew);
	assert(isNew);
	Blt_SetHashValue(hPtr, name);
    }
    return TCL_OK;
}

static int
GetRemoteUidGids(Tcl_Interp *interp, Remote *remotePtr) 
{
    Tcl_Obj *objPtr;
    char *p;
    const char *copy, *string;
    int length;

    /* This only works on servers where there is an "id" command. */
    objPtr = ExecOnRemote(interp, remotePtr, "id", 2);
    if (objPtr == NULL) {
	return TCL_ERROR;
    }
    string = Tcl_GetStringFromObj(objPtr, &length);

    /* Parse the id output string. Make a copy of the string and point to the
     * substrings inside of it. The parts are in the form: type=id(name) */
    copy = Blt_Strndup(string, length);
    Tcl_DecrRefCount(objPtr);
    for (p = strtok((char *)copy, " "); p != NULL; p = strtok(NULL, " ")) {
	int id;
	char *q, *name, *type;

	type = p;			/* Type is uid, gid, or groups. */
	q = strchr(p, '=');
	if (q == NULL) {
	    Tcl_AppendResult(interp, "can't parse type \"", p, "\"", 
			(char *)NULL);
	    goto error;
	}
	*q++ = '\0';
	if (strcmp(type, "groups") == 0) {
	    /* Handle groups specially */
	    if (ParseGroups(interp, remotePtr, q) != TCL_OK) {
		goto error;
	    }
	    continue;
	} 
	/* Get the user of group id.  */
	id = strtoul(q, &q, 10);
	if (*q != '(') {
	    Tcl_AppendResult(interp, "can't parse id \"", q, "\"", 
			(char *)NULL);
	    goto error;
	}
	/* Now look for the name of the user or group. */
	name = q + 1;
	q = strchr(name, ')');
	if (q == NULL) {
	    Tcl_AppendResult(interp, "can't parse name \"", name, "\"", 
			(char *)NULL);
	    goto error;
	}
	*q = '\0';
	if (strcmp(type, "uid") == 0) {
	    remotePtr->uid = id;
	    remotePtr->userName = name;
	} else if (strcmp(type, "gid") == 0) {
	    remotePtr->gid = id;
	    remotePtr->groupName = name;
	} else {
	    Tcl_AppendResult(interp, "unknown type \"", type, "\"", 
			(char *)NULL);
	    goto error;
	}
    }
    remotePtr->groups = copy;
    return TCL_OK;
 error:
    Blt_Free(copy);
    return TCL_ERROR;
}

#ifdef notdef
static int
InitializeSockets(Tcl_Interp *interp)
{
#ifdef WIN32
    int result;
    WSADATA wsaData;
    /*
     * Initialize the winsock library and check the interface version
     * actually loaded. We only ask for the 1.1 interface and do require
     * that it not be less than 1.1.
     */
    result = WSAStartup(MAKEWORD(2,0), &wsaData);
    if (result) {
	Tcl_AppendResult(interp, "can't start up sockets", (char *)NULL);
	return TCL_ERROR;
    }
#endif
    return TCL_OK;
}
#endif
/*
 *---------------------------------------------------------------------------
 *
 * ConnectToRemote --
 *
 *	Connects to the designated sftp server and starts a new session.
 *
 *---------------------------------------------------------------------------
 */
static int
ConnectToRemote(Tcl_Interp *interp, Remote *remotePtr)
{
    struct sockaddr_in sin;
    const char *fingerprint;
    int result;

    if (TclpHasSockets(interp) != TCL_OK) {
	return TCL_ERROR;
    }
    if (!CreateSocketAddress(&sin, remotePtr->host, 22)) {
	Tcl_AppendResult(interp, "can't connect to \"", remotePtr->host, 
			 "\": unknown hostname", (char *)NULL);
	return TCL_ERROR;
    }
    remotePtr->sock = socket(AF_INET, SOCK_STREAM, 0);
    if (connect(remotePtr->sock, (struct sockaddr*)&sin, 
		sizeof(struct sockaddr_in)) < 0) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't connect to \"", remotePtr->host, 
			 "\": ", Tcl_PosixError(interp));
	}
	return TCL_ERROR;
    }
    /* Create a session instance */
    remotePtr->session = libssh2_session_init();
    if (remotePtr->session == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't create a sftp session with \"", 
		remotePtr->host, "\": ", Tcl_PosixError(interp));
	}
	return TCL_ERROR;
    }
    /* Since we have set non-blocking, tell libssh2 we are blocking */
    libssh2_session_set_blocking(remotePtr->session, TRUE);
    
    /* ... start it up. This will trade welcome banners, exchange keys,
     * and setup crypto, compression, and MAC layers
     */
    result = libssh2_session_handshake(remotePtr->session, remotePtr->sock);
    if (result != 0) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "Failure establishing SSH session: ",
		RemoteError(remotePtr), (char *)NULL);
	}
	return TCL_ERROR;
    }

    /* At this point we havn't yet authenticated.  The first thing to do
     * is check the hostkey's fingerprint against our known hosts Your app
     * may have it hard coded, may go to a file, may present it to the
     * user, that's your call
     */
    fingerprint = libssh2_hostkey_hash(remotePtr->session, 
	LIBSSH2_HOSTKEY_HASH_SHA1);
    assert(fingerprint != NULL);
#ifdef notdef
    { 
	int i;

	fprintf(stderr, "Fingerprint: ");
	for(i = 0; i < 20; i++) {
	    fprintf(stderr, "%02X ", (unsigned char)fingerprint[i]);
	}
	fprintf(stderr, "\n");
    }
#endif
    if (AuthenticateRemote(interp, remotePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    remotePtr->sftp = libssh2_sftp_init(remotePtr->session);
    if (remotePtr->sftp == NULL) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't connect to \"", remotePtr->host, 
		"\": unable to initialize SFTP session", (char *)NULL);
	}
	return TCL_ERROR;
    }
    if (GetRemoteHome(interp, remotePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    if (GetRemoteUidGids(interp, remotePtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRemotePath --
 *
 *	Returns the new path as a string from a given path.  If the
 *	path is relative, the current working directory will be prepended.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GetRemotePath(Remote *remotePtr, const char *string, int *lengthPtr)
{
    Tcl_DStringSetLength(&remotePtr->ds, 0);
    if (string[0] == '/') {		/* Absolute path. */
	Tcl_DStringAppend(&remotePtr->ds, string, -1);
    } else if (string[0] == '~') {	/* Home directory. */
	Tcl_DStringAppend(&remotePtr->ds, remotePtr->homedir, -1);
	if (string[1] != '\0') {
	    Tcl_DStringAppend(&remotePtr->ds, "/", 1);
	    Tcl_DStringAppend(&remotePtr->ds, string + 1, -1);
	}
    }  else {				/* Relative path. Append to cwd. */
	Tcl_DStringAppend(&remotePtr->ds, remotePtr->cwd, remotePtr->cwdLength);
	Tcl_DStringAppend(&remotePtr->ds, "/", 1);
	Tcl_DStringAppend(&remotePtr->ds, string, -1);
    }
    *lengthPtr = strlen(Tcl_DStringValue(&remotePtr->ds));
    return Tcl_DStringValue(&remotePtr->ds);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRemotePathFromObj --
 *
 *	Returns the new path as a Tcl_Obj from a given path.  If the
 *	path is relative, the current working directory will be prepended.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GetRemotePathFromObj(Remote *remotePtr, Tcl_Obj *objPtr, int *lengthPtr)
{
    const char *string;
    int length;

    string = Tcl_GetStringFromObj(objPtr, &length);
    Tcl_DStringSetLength(&remotePtr->ds, 0);
    if (string[0] == '/') {		/* Absolute path. */
	Tcl_DStringAppend(&remotePtr->ds, string, length);
    } else if (string[0] == '~') {	/* Home directory. */
	Tcl_DStringAppend(&remotePtr->ds, remotePtr->homedir, -1);
	if (string[1] != '\0') {
	    Tcl_DStringAppend(&remotePtr->ds, "/", 1);
	    Tcl_DStringAppend(&remotePtr->ds, string + 1, -1);
	}
    }  else {				/* Relative path. Append to cwd. */
	Tcl_DStringAppend(&remotePtr->ds, remotePtr->cwd, remotePtr->cwdLength);
	Tcl_DStringAppend(&remotePtr->ds, "/", 1);
	Tcl_DStringAppend(&remotePtr->ds, string, length);
    }
    *lengthPtr = Tcl_DStringLength(&remotePtr->ds);
    return Tcl_DStringValue(&remotePtr->ds);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRemoteAttributes --
 *
 *	Retrieves the attributes of a file or directory from the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static int
GetRemoteAttributes(Remote *remotePtr, const char *path, unsigned int length, 
		  LIBSSH2_SFTP_ATTRIBUTES *attrsPtr)
{
    for (;;) {
	int result;

	result = libssh2_sftp_stat_ex(remotePtr->sftp, path, length, 
		LIBSSH2_SFTP_STAT, attrsPtr);
	if (result == 0) {
	    return TCL_OK;
	}
	if (libssh2_session_last_errno(remotePtr->session) == 
	    LIBSSH2_ERROR_EAGAIN) {
	    continue;
	}
	return TCL_ERROR;
    }	
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetRemoteAttributes --
 *
 *	Sets the attributes of a file or directory on the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static int
SetRemoteAttributes(Remote *remotePtr, const char *path, int length,
		  LIBSSH2_SFTP_ATTRIBUTES *attrsPtr)
{
    if (libssh2_sftp_stat_ex(remotePtr->sftp, path, length, 
		LIBSSH2_SFTP_SETSTAT, attrsPtr) < 0) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


static int
RemoteMkdir(Tcl_Interp *interp, Remote *remotePtr, const char *path, int length,
	    int mode)
{
    char *partial;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *p, *pend;
    int count;
    
    if (*path != PATH_SEPARATOR) {
	Tcl_AppendResult(interp, "path \"", path, "\" must be absolute.", 
			 (char *)NULL); 
	return TCL_ERROR;
    }
    partial = Blt_AssertMalloc(length + 1);
    count = 0;
    for (p = path, pend = p + length; p < pend; /*empty*/) {
	int result;

	/* Skip leading separators. */
	for (/*empty*/; p < pend; p++) {
	    if (*p != PATH_SEPARATOR) {
		break;
	    }
	}
	if (p == pend) {
	    break;			/* Ignore trailing separators. */
	}
	/* Prepend a single path separator */
	partial[count++] = '/';

	/* Skip the component name, appending it to the partial path. */
	for (/*empty*/; p < pend; p++) {
	    if (*p == PATH_SEPARATOR) {
		break;
	    }
	    partial[count++] = *p;
	}
	partial[count] = '\0';
	result = GetRemoteAttributes(remotePtr, partial, count, &attrs);
	if (result == TCL_OK) {
	    if (!LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
		Tcl_AppendResult(interp, "can't create directory \"", partial, 
				 "\": file exists.", (char *)NULL); 
		goto error;
	    }	    
	    continue;			/* Directory already exists. */
	}
	/* Try to create the directory */
	result = libssh2_sftp_mkdir_ex(remotePtr->sftp, partial, count, mode);
	if (result != TCL_OK) {
	    Tcl_AppendResult(interp, "can't create directory \"", partial, 
			     "\": ", RemoteError(remotePtr), (char *)NULL); 
	    goto error;
	}
    }
    Blt_Free(partial);
    return TCL_OK;
 error:
    Blt_Free(partial);
    return TCL_ERROR;
}


/*
 *---------------------------------------------------------------------------
 *
 * DisconnectFromRemote --
 *
 *	Disconnects from the sftp server and frees the current session.
 *	The socket is closed.
 *
 *---------------------------------------------------------------------------
 */
static void
DisconnectFromRemote(Remote *remotePtr)
{
    libssh2_sftp_shutdown(remotePtr->sftp);
    libssh2_session_disconnect(remotePtr->session, 
	"Normal Shutdown, Thank you for playing");
    libssh2_session_free(remotePtr->session);

#ifdef WIN32
    closesocket(remotePtr->sock);
#else
    close(remotePtr->sock);
#endif
}

/*
 *---------------------------------------------------------------------------
 *
 * DestroyRemote --
 *
 *	Frees the resources associated with the sftp command.
 *
 *---------------------------------------------------------------------------
 */
static void
DestroyRemote(Remote *remotePtr)
{
    if (remotePtr->sftp != NULL) {
	DisconnectFromRemote(remotePtr);
    }
    if (remotePtr->hashPtr != NULL) {
	Blt_DeleteHashEntry(remotePtr->tablePtr, remotePtr->hashPtr);
    }
    if (remotePtr->homedir != NULL) {
	Blt_Free(remotePtr->homedir);
    }
    Tcl_DStringFree(&remotePtr->ds);
    Blt_FreeSwitches(sftpSwitches, (char *)remotePtr, 0);
    if (remotePtr->groups != NULL) {
	Blt_Free(remotePtr->groups);
    }
    Blt_DeleteHashTable(&remotePtr->gidTable);
    Blt_Free(remotePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GenerateName --
 *
 *	Generates an unique sftp command name.  Sftp names are in the form
 *	"sftpN", where N is a non-negative integer. 
 *	We want to recycle names if possible.
 *	
 * Results:
 *	Returns the unique name.  The string itself is stored in the dynamic
 *	string passed into the routine.
 *
 *---------------------------------------------------------------------------
 */
static const char *
GenerateName(Tcl_Interp *interp, SftpCmdInterpData *dataPtr, const char *prefix,
	     const char *suffix, Tcl_DString *resultPtr)
{
    const char *sftpName;

    /* 
     * Parse the command and put back so that it's in a consistent
     * format.  
     *
     *	t1         <current namespace>::t1
     *	n1::t1     <current namespace>::n1::t1
     *	::t1	   ::t1
     *  ::n1::t1   ::n1::t1
     */
    sftpName = NULL;		/* Suppress compiler warning. */
    for (;;) {
	Blt_ObjectName objName;
	Tcl_DString ds;
	char string[200];

	Tcl_DStringInit(&ds);
	Tcl_DStringAppend(&ds, prefix, -1);
	Blt_FormatString(string, 200, "sftp%d", dataPtr->nextId);
	dataPtr->nextId++;
	Tcl_DStringAppend(&ds, string, -1);
	Tcl_DStringAppend(&ds, suffix, -1);
	if (!Blt_ParseObjectName(interp, Tcl_DStringValue(&ds), &objName, 0)) {
	    Tcl_DStringFree(&ds);
	    return NULL;
	}
	sftpName = Blt_MakeQualifiedName(&objName, resultPtr);
	Tcl_DStringFree(&ds);

	if (Blt_CommandExists(interp, sftpName)) {
	    continue;		/* A command by this name already exists. */
	}
	break;
    }
    return sftpName;
}

/*
 *---------------------------------------------------------------------------
 *
 * NewRemote --
 *
 *	Allocates a new sftp command structure and adds it to the hash
 *	table of sftp connections specific to the TCL interpreter.
 *
 *---------------------------------------------------------------------------
 */
static Remote *
NewRemote(ClientData clientData, Tcl_Interp *interp)
{
    SftpCmdInterpData *dataPtr = clientData;
    int isNew;
    Remote *remotePtr;

    remotePtr = Blt_AssertCalloc(1, sizeof(Remote));
    remotePtr->dataPtr = dataPtr;
    remotePtr->interp = interp;
    remotePtr->tablePtr = &dataPtr->sessionTable;
    remotePtr->hashPtr = Blt_CreateHashEntry(remotePtr->tablePtr, 
	(char *)remotePtr, &isNew);
    Tcl_DStringInit(&remotePtr->ds);
    Blt_SetHashValue(remotePtr->hashPtr, remotePtr);
    Blt_InitHashTable(&remotePtr->gidTable, BLT_ONE_WORD_KEYS);
    remotePtr->uid = remotePtr->gid = -1;
    return remotePtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadFileContents --
 *
 *---------------------------------------------------------------------------
 */
static void
ReadFileContents(FileReader *readerPtr)
{
    unsigned char bytes[1<<15];
    ssize_t numBytes;

    /* Read */
    numBytes = libssh2_sftp_read(readerPtr->handle, (char *)bytes, 
				 sizeof(bytes));
    if (numBytes < 0) {
	if (numBytes != LIBSSH2_ERROR_EAGAIN) {
	    *readerPtr->donePtr = -1;	/* Bail if blocking. */
	}
	return;
    }
    if (numBytes == 0) {
	*readerPtr->donePtr = 1;		/* We're done. */
	return;
    }	
    /* Append read bytes to either the file or buffer. */
    if (readerPtr->f == NULL) {
	if (!Blt_DBuffer_AppendData(readerPtr->dbuffer, bytes, numBytes)) {
	    Tcl_AppendResult(readerPtr->interp, 
		"can't append file contents to buffer: out of memory", 
		(char *)NULL);
	    *readerPtr->donePtr = -1;
	}
    } else {
	if (fwrite(bytes, 1, numBytes, readerPtr->f) != numBytes) {
	    Tcl_AppendResult(readerPtr->interp, "short write to local file: ",
		Tcl_PosixError(readerPtr->interp), (char *)NULL);
	    *readerPtr->donePtr = -1;
	}
    }
    readerPtr->numRead += numBytes;
    if (readerPtr->progCmdObjPtr != NULL) {
	Tcl_Obj *cmdObjPtr;
	int result;

	cmdObjPtr = Tcl_DuplicateObj(readerPtr->progCmdObjPtr);
	Tcl_ListObjAppendElement(readerPtr->interp, cmdObjPtr, 
				 Tcl_NewLongObj(readerPtr->numRead));
	Tcl_ListObjAppendElement(readerPtr->interp, cmdObjPtr, 
				 Tcl_NewLongObj(readerPtr->size));
	Tcl_IncrRefCount(cmdObjPtr);
	result = Tcl_EvalObjEx(readerPtr->interp, cmdObjPtr, TCL_EVAL_GLOBAL);
	Tcl_DecrRefCount(cmdObjPtr);
	if (result != TCL_OK) {
	    Tcl_BackgroundError(readerPtr->interp);
	}
    }
    if ((readerPtr->maxSize > 0) && (numBytes > readerPtr->maxSize)) {
	*readerPtr->donePtr = 1;	/* Maximum number of bytes to read
					 * reached. */
    }
    if (readerPtr->timeout > 0) {
	Tcl_Time time;

	Tcl_GetTime(&time);
	if ((time.sec - readerPtr->startTime)  > readerPtr->timeout) {
	    *readerPtr->donePtr = 1;	/* Timeout. Maximum number of seconds 
					 * to read reached. */
	}
    }
}

static void
ExportToTable(DirectoryReader *readerPtr, const char *bytes, 
	      const char *longentry, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr)
{
    BLT_TABLE_ROW row;
    BLT_TABLE_COLUMN col;
    Tcl_Interp *interp;
    BLT_TABLE table;

    interp = readerPtr->interp;
    table = readerPtr->table;
    row = blt_table_create_row(interp, table, NULL);
    if (row == NULL) {
	return;
    }
    /* name */
    if (readerPtr->flags & DIR_NAME) {
	col = blt_table_get_column_by_label(table, "name");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "name");
	}
	blt_table_set_string_rep(interp, table, row, col, bytes, -1);
    }
    /* type */
    if (readerPtr->flags & DIR_TYPE) {
	col = blt_table_get_column_by_label(table, "type");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "type");
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
	    blt_table_set_string_rep(interp, table, row, col,
                GetFileTypeFromAttributes(attrsPtr), -1);
	}
    }
    /* size */ 
    if (readerPtr->flags & DIR_SIZE) {
	col = blt_table_get_column_by_label(table, "size");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "size");
	    blt_table_set_column_type(interp, table, col,
                TABLE_COLUMN_TYPE_LONG);
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_SIZE) {
	    blt_table_set_long(interp, table, row, col, attrsPtr->filesize);
	}
    }
    /* uid */
    if (readerPtr->flags & DIR_UID) {
	col = blt_table_get_column_by_label(table, "uid");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "uid"); 
	    blt_table_set_column_type(interp, table, col,
                        TABLE_COLUMN_TYPE_LONG);
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_UIDGID) {
	    blt_table_set_long(interp, table, row, col, attrsPtr->uid);
	}
    }
    /* gid */
    if (readerPtr->flags & DIR_GID) {
	col = blt_table_get_column_by_label(table, "gid");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "gid");
	    blt_table_set_column_type(interp, table, col,
                        TABLE_COLUMN_TYPE_LONG);
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_UIDGID) {
	    blt_table_set_long(interp, table, row, col, attrsPtr->gid);
	}
    }
    /* atime */
    if (readerPtr->flags & DIR_ATIME) {
	col = blt_table_get_column_by_label(table, "atime");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "atime");
	    blt_table_set_column_type(interp, table, col,
                TABLE_COLUMN_TYPE_LONG);
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
	    blt_table_set_long(interp, table, row, col, attrsPtr->atime);
	}
    }
    /* mtime */
    if (readerPtr->flags & DIR_MTIME) {
	col = blt_table_get_column_by_label(table, "mtime");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "mtime");
	    blt_table_set_column_type(interp, table, col,
                TABLE_COLUMN_TYPE_LONG);
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
	    blt_table_set_long(interp, table, row, col, attrsPtr->mtime);
	}
    }
    /* mode */
    if (readerPtr->flags & DIR_MODE) { 
	col = blt_table_get_column_by_label(table, "mode");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "mode");
	    blt_table_set_column_type(interp, table, col,
                        TABLE_COLUMN_TYPE_LONG);
	}
	if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
	    blt_table_set_long(interp, table, row, col, 
			       attrsPtr->permissions & 07777);
	}
    }
    /* longentry */
    if (readerPtr->flags & DIR_LONGENTRY) {
	col = blt_table_get_column_by_label(table, "longentry");
	if (col == NULL) {
	    col = blt_table_create_column(interp, table, "longentry");
	}
	blt_table_set_string_rep(interp, table, row, col, longentry, -1);
    }
}
    
static void
ExportToList(DirectoryReader *readerPtr, const char *bytes, 
	     const char *longentry, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr)
{
    Tcl_Obj *objPtr;
    Tcl_Interp *interp;

    interp = readerPtr->interp;

    /* Name */
    objPtr = Tcl_NewStringObj(bytes, -1);
    Tcl_ListObjAppendElement(interp, readerPtr->listObjPtr, objPtr);

    if (readerPtr->flags & DIR_LONG) {
	Tcl_Obj *listObjPtr;
	
	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
	/* type */
	if (readerPtr->flags & DIR_TYPE) {
	    objPtr = Tcl_NewStringObj("type", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
		objPtr = Tcl_NewStringObj(GetFileTypeFromAttributes(attrsPtr), -1);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	/* uid */
	if (readerPtr->flags & DIR_UID) {
	    objPtr = Tcl_NewStringObj("uid", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_UIDGID) {
		objPtr = Tcl_NewIntObj(attrsPtr->uid);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	/* gid */
	if (readerPtr->flags & DIR_GID) {
	    objPtr = Tcl_NewStringObj("gid", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_UIDGID) {
		objPtr = Tcl_NewIntObj(attrsPtr->gid);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	
	/* atime */
	if (readerPtr->flags & DIR_ATIME) {
	    objPtr = Tcl_NewStringObj("atime", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
		objPtr = Tcl_NewLongObj(attrsPtr->atime);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	/* mtime */
	if (readerPtr->flags & DIR_MTIME) {
	    objPtr = Tcl_NewStringObj("mtime", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_ACMODTIME) {
		objPtr = Tcl_NewLongObj(attrsPtr->mtime);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	if (readerPtr->flags & DIR_SIZE) {
	    objPtr = Tcl_NewStringObj("size", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_UIDGID) {
		objPtr = Tcl_NewLongObj(attrsPtr->filesize);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	if (readerPtr->flags & DIR_MODE) {
	    objPtr = Tcl_NewStringObj("mode", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    if (attrsPtr->flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
		char out[200];
		
		sprintf(out, "%0#5lo", attrsPtr->permissions & 07777);
		objPtr = Tcl_NewStringObj(out, -1);
	    } else {
		objPtr = Tcl_NewStringObj("???", -1);
	    }
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	if (readerPtr->flags & DIR_LONGENTRY) {
	    objPtr = Tcl_NewStringObj("longentry", -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	    objPtr = Tcl_NewStringObj(longentry, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
	Tcl_ListObjAppendElement(interp, readerPtr->listObjPtr, listObjPtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadEntryIntoList --
 *
 *---------------------------------------------------------------------------
 */
static void
ReadEntryIntoList(DirectoryReader *readerPtr)
{
    char bytes[2048];
    char longentry[2048];
    ssize_t numBytes;
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    numBytes = libssh2_sftp_readdir_ex(readerPtr->handle, bytes, sizeof(bytes),
		longentry, sizeof(longentry), &attrs);
    if (numBytes < 0) {
	if (numBytes != LIBSSH2_ERROR_EAGAIN) {
	    *readerPtr->donePtr = -1;	/* Bail if blocking. */
	}
	return;
    }
    if (numBytes == 0) {
	*readerPtr->donePtr = 1;	/* We're done. */
	return;
    }	
    if (bytes[0] == '.') {
	if ((readerPtr->flags & DIR_DOTFILES) == 0) {
	    return;			/* Ignore dotfiles. */
	}
	if ((bytes[1] == '\0') || ((bytes[1] == '.') && (bytes[2] == '\0'))) {
	    return;			/* Ignore "." and ".." */
	}
    }
    if ((readerPtr->match != NULL) && (strcmp(bytes, readerPtr->match) != 0)) {
	return;				/* Doesn't match the entry we're
					 * looking for. */
    }
    if (readerPtr->excludeObjPtr != NULL) {
	int i;
	Tcl_Obj **objv;
	int objc;

	if (Tcl_ListObjGetElements(readerPtr->interp, readerPtr->excludeObjPtr, 
				   &objc, &objv) != TCL_OK) {
	    return;
	}
	for (i = 0; i < objc; i++) {
	    const char *pattern;

	    pattern = Tcl_GetString(objv[i]);
	    if (Tcl_StringMatch(bytes, pattern)) {
		return;
	    }
	}
    }
    if (readerPtr->flags & DIR_LISTING) {
	if (readerPtr->fileCount > 0) {
	    Tcl_AppendToObj(readerPtr->listObjPtr, "\n", 1);
	}
	Tcl_AppendToObj(readerPtr->listObjPtr, longentry, -1);
	readerPtr->fileCount++;
	return;				/* Just return the listing only. */
    }
    if (readerPtr->table != NULL) {
	ExportToTable(readerPtr, bytes, longentry, &attrs);
    } else {
	ExportToList(readerPtr, bytes, longentry, &attrs);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * GetRemoteFile --
 *
 *	Retrieves the contents of the designed file on the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static int
GetRemoteFile(Tcl_Interp *interp, const char *path, int length, 
	      FileReader *readerPtr)
{
    LIBSSH2_SFTP_HANDLE *handle;
    int done;
    Remote *remotePtr = readerPtr->remotePtr;
    Tcl_Time time;

    libssh2_session_set_blocking(remotePtr->session, FALSE);
    do {
	handle = libssh2_sftp_open_ex(remotePtr->sftp, path, length,
		LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENFILE);
	if (handle == NULL) {
	    if (libssh2_session_last_errno(remotePtr->session) != 
		LIBSSH2_ERROR_EAGAIN) {
		break;
	    }
	}
    } while (handle == NULL);
    if (!handle) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't open file \"", path, "\": ", 
			 RemoteError(remotePtr), (char *)NULL);
	}
	libssh2_session_set_blocking(remotePtr->session, TRUE);
	return TCL_ERROR;
    }
    readerPtr->handle = handle;
    readerPtr->donePtr = &done;
    Tcl_GetTime(&time);
    readerPtr->startTime = time.sec;
    if (readerPtr->offset > 0) {
	/* If the offset is > 0, then this is number of bytes already read.
	 * Seek to the offset, to resume reading.  */
	libssh2_sftp_seek64(handle, readerPtr->offset); 
    }
    done = 0;
    if (readerPtr->cancelVarName != NULL) {
	Tcl_TraceVar(interp, readerPtr->cancelVarName, TRACE_FLAGS, 
		CancelRemoteReadVarProc, readerPtr);
	remotePtr->flags |= READ_TRACED;
    }
    while (!done) {
	ReadFileContents(readerPtr);
	Tcl_DoOneEvent(TCL_DONT_WAIT);
    }
    if (readerPtr->cancelVarName != NULL) {
	Tcl_UntraceVar(interp, readerPtr->cancelVarName, TRACE_FLAGS, 
		CancelRemoteReadVarProc, readerPtr);
	remotePtr->flags &= ~READ_TRACED;
    }
    libssh2_session_set_blocking(remotePtr->session, TRUE);
    libssh2_sftp_close(readerPtr->handle);
    return (done == 1) ? TCL_OK : TCL_ERROR;
}

static int
ChgrpRemoteFile(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
	      int length, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr, 
	      ClientData clientData)
{
    ChgrpSwitches *switchesPtr = clientData;

    if (attrsPtr->gid == switchesPtr->gid) {
	return TCL_OK;
    }
    attrsPtr->gid = switchesPtr->gid;
    attrsPtr->flags = LIBSSH2_SFTP_ATTR_UIDGID;
    if (SetRemoteAttributes(remotePtr, path, length, attrsPtr) != TCL_OK) {
	Tcl_AppendResult(interp, "can't set group for \"", path, "\": ", 
		RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}


static int
ChmodRemoteFile(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
	      int length, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr, 
	      ClientData clientData)
{
    ChmodSwitches *switchesPtr = clientData;

    unsigned int long newPerms;
    newPerms = (attrsPtr->permissions & 07777);
    newPerms &= ~switchesPtr->clearFlags;
    newPerms |= switchesPtr->setFlags;
    if (newPerms != attrsPtr->permissions) {
	attrsPtr->permissions &= ~07777;
	attrsPtr->permissions |= newPerms;
	attrsPtr->flags = LIBSSH2_SFTP_ATTR_PERMISSIONS;
	if (SetRemoteAttributes(remotePtr, path, length, attrsPtr) != TCL_OK) {
	    Tcl_AppendResult(interp, "can't set mode for \"", 
		path, "\": ", RemoteError(remotePtr), (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}


static int
RemoveRemoteDirectory(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
		    int length, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr, 
		    ClientData clientData)
{
    if (libssh2_sftp_rmdir_ex(remotePtr->sftp, path, length) < 0) {
	unsigned long flags = (unsigned long)clientData;
	if ((flags & FORCE) == 0) {
	    Tcl_AppendResult(interp, "can't delete directory \"", path, "\": ", 
		RemoteError(remotePtr), (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

static int
RemoveRemoteFile(Tcl_Interp *interp, Remote *remotePtr, const char *path, 
	       int length, LIBSSH2_SFTP_ATTRIBUTES *attrsPtr, 
	       ClientData clientData)
{
    if (libssh2_sftp_unlink_ex(remotePtr->sftp, path, length) < 0) {
	unsigned long flags = (unsigned long)clientData;
	if ((flags & FORCE) == 0) {
	    Tcl_AppendResult(interp, "can't delete file \"", path, "\": ", 
			 RemoteError(remotePtr), (char *)NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ReadDirectoryIntoList --
 *
 *	Retrieves the listing of the designed directory on the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static int
ReadDirectoryIntoList(Tcl_Interp *interp, const char *path, int length, 
		      DirectoryReader *readerPtr)
{
    LIBSSH2_SFTP_HANDLE *handle;
    int done;
    Remote *remotePtr = readerPtr->remotePtr;

    libssh2_session_set_blocking(remotePtr->session, FALSE);
    do {
	handle = libssh2_sftp_open_ex(remotePtr->sftp, path, length,
		LIBSSH2_FXF_READ, 0, LIBSSH2_SFTP_OPENDIR);
	if (handle == NULL) {
	    if (libssh2_session_last_errno(remotePtr->session) != 
		LIBSSH2_ERROR_EAGAIN) {
		break;
	    }
	}
    } while (handle == NULL);
    if (!handle) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't open directory \"", path, "\": ",
		RemoteError(remotePtr), (char *)NULL);
	}
	libssh2_session_set_blocking(remotePtr->session, TRUE);
	return TCL_ERROR;
    }
    readerPtr->handle = handle;
    readerPtr->donePtr = &done;
    readerPtr->fileCount = 0;
    done = 0;
    ReadEntryIntoList(readerPtr);
    while (!done) {
	ReadEntryIntoList(readerPtr);
	Tcl_DoOneEvent(TCL_DONT_WAIT); 
    }
    libssh2_session_set_blocking(remotePtr->session, TRUE);
    libssh2_sftp_closedir(readerPtr->handle);
    return (done == 1) ? TCL_OK : TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * WriteFileContents --
 *
 *---------------------------------------------------------------------------
 */
static void
WriteFileContents(FileWriter *writerPtr)
{
    /* Write data in a loop until we block */
    ssize_t numBytes;
    
    if (writerPtr->f != NULL) {
	if (writerPtr->numBytesRead == 0) {
	    writerPtr->numBytesRead = fread(writerPtr->buffer, sizeof(char),
		sizeof(writerPtr->buffer), writerPtr->f);
	    if (writerPtr->numBytesRead < 0) {
		*writerPtr->donePtr = -1;
		return;			/* Error while reading local file. */
	    } else if (writerPtr->numBytesRead == 0) {
		*writerPtr->donePtr = 1; /* end of file */
		return;
	    }
	    writerPtr->bp = writerPtr->buffer;
	} 
    } else if (writerPtr->string != NULL) {
	if (writerPtr->totalBytesWritten == writerPtr->size) {
	    *writerPtr->donePtr = 1;
	    return;			/* Finished. */
	}
	if (writerPtr->numBytesRead == 0) { 
	    size_t bytesLeft;

	    bytesLeft = writerPtr->bp - writerPtr->string;
	    writerPtr->numBytesRead = MIN(1<<15, bytesLeft);
	    writerPtr->bp = writerPtr->string + writerPtr->totalBytesWritten;
	}
    }

    numBytes = libssh2_sftp_write(writerPtr->handle, writerPtr->bp, 
	writerPtr->numBytesRead);
    if (numBytes < 0) {
	if (numBytes != LIBSSH2_ERROR_EAGAIN) {
	    *writerPtr->donePtr = -1;
	}
	return;				/* Error or no data ready. */
    }
    writerPtr->numBytesRead -= numBytes;
    writerPtr->bp += numBytes;
    writerPtr->totalBytesWritten += numBytes;
    if (writerPtr->timeout > 0) {
	Tcl_Time time;

	Tcl_GetTime(&time);
	if ((time.sec - writerPtr->startTime)  > writerPtr->timeout) {
	    *writerPtr->donePtr = 1;	/* Timeout. Maximum number of seconds 
					 * to write reached. */
	}
    }
} 

/*
 *---------------------------------------------------------------------------
 *
 * PutRemoteFile --
 *
 *	Retrieves the contents of the designed file on the sftp server.
 *
 *---------------------------------------------------------------------------
 */
static int
PutRemoteFile(Tcl_Interp *interp, const char *path, int length, 
	    FileWriter *writerPtr)
{
    LIBSSH2_SFTP_HANDLE *handle;
    int result;
    int done;
    Remote *remotePtr = writerPtr->remotePtr;
    unsigned int flags;
    Tcl_Time time;

    flags = LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT; 
    if (writerPtr->flags & APPEND) {
	flags |= LIBSSH2_FXF_APPEND;
    } else {
	flags |= LIBSSH2_FXF_TRUNC;
    }
    libssh2_session_set_blocking(remotePtr->session, FALSE);
    /* Request a file via SFTP */
    do {
	handle = libssh2_sftp_open_ex(remotePtr->sftp, path, length, flags,
		writerPtr->mode, LIBSSH2_SFTP_OPENFILE);
	if (handle == NULL) {
	    if (libssh2_session_last_errno(remotePtr->session) != 
		LIBSSH2_ERROR_EAGAIN) {
		break;
	    }
	}
    } while (handle == NULL);
    if (!handle) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "can't open file \"", path, "\": ", 
			 RemoteError(remotePtr), (char *)NULL);
	}
	libssh2_session_set_blocking(remotePtr->session, TRUE);
	return TCL_ERROR;
    }
    if (writerPtr->cancelVarName != NULL) {
	Tcl_TraceVar(interp, writerPtr->cancelVarName, TRACE_FLAGS, 
		CancelRemoteWriteVarProc, writerPtr);
	remotePtr->flags |= WRITE_TRACED;
    }
    writerPtr->handle = handle;
    writerPtr->donePtr = &done;
    Tcl_GetTime(&time);
    writerPtr->startTime = time.sec;
    done = 0;
    while (!done) {
	WriteFileContents(writerPtr);
	Tcl_DoOneEvent(TCL_DONT_WAIT);
    }
    if (writerPtr->cancelVarName != NULL) {
	Tcl_UntraceVar(interp, writerPtr->cancelVarName, TRACE_FLAGS, 
		CancelRemoteWriteVarProc, writerPtr);
	remotePtr->flags &= ~WRITE_TRACED;
    }
    result = (done == 1) ? TCL_OK : TCL_ERROR;
    libssh2_session_set_blocking(remotePtr->session, TRUE);
    libssh2_sftp_close(writerPtr->handle);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * CancelRemoteReadVarProc --
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static char *
CancelRemoteReadVarProc(ClientData clientData, Tcl_Interp *interp, 
			const char *part1, const char *part2, int flags)
{
    if (flags & TRACE_FLAGS) {
	FileReader *readerPtr = clientData;
	*readerPtr->donePtr = 1;
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * CancelRemoteWriteVarProc --
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static char *
CancelRemoteWriteVarProc(ClientData clientData, Tcl_Interp *interp, 
			 const char *part1, const char *part2, int flags)
{
    if (flags & TRACE_FLAGS) {
	FileWriter *writerPtr = clientData;

	*writerPtr->donePtr = 1;
    }
    return NULL;
}

/* 
 *---------------------------------------------------------------------------
 *
 * AtimeOp --
 *
 *	sftp atime path ?seconds?
 *
 *---------------------------------------------------------------------------
 */
static int
AtimeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    unsigned long atime;
    const char *path;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 4) {
	long l;

	if (Tcl_GetLongFromObj(interp, objv[3], &l) != TCL_OK) {
	    return TCL_ERROR;
	}
	attrs.atime = (unsigned long)l;
	attrs.flags = LIBSSH2_SFTP_ATTR_ACMODTIME;
	if (SetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	    Tcl_AppendResult(interp, "can't set access time for \"", 
		Tcl_GetString(objv[2]), "\": ", RemoteError(remotePtr), 
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    atime = (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) ? attrs.atime : 0L;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), atime);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * AuthOp --
 *
 *	sftp auth
 *
 *---------------------------------------------------------------------------
 */
static int
AuthOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    const char *string;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (remotePtr->flags & AUTH_PASSWORD) {
	string = "password";
    } else if (remotePtr->flags & AUTH_PUBLICKEY) {
	string = "pubickey";
    } else {
	string = "???";
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), string, -1);
    return TCL_OK;
}


/* 
 *---------------------------------------------------------------------------
 *
 * ChdirOp --
 *
 *	sftp chdir ?path?
 *
 *---------------------------------------------------------------------------
 */
static int
ChdirOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;

    if (objc == 3) {
	LIBSSH2_SFTP_ATTRIBUTES attrs;
	const char *path;
	int length;

	if (remotePtr->sftp == NULL) {
	    if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	}
	path = GetRemotePathFromObj(remotePtr, objv[2], &length);
	if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	    Tcl_AppendResult(interp, 
		"can't change current working directory to \"", 
		Tcl_GetString(objv[2]), 
		"\": no such directory", (char *)NULL);
	    return TCL_ERROR;
	}
	if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	    Tcl_AppendResult(interp, 
		"server does not report permissions for \"", path, "\"", 
		(char *)NULL);
	    return TCL_ERROR;
	}
	if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	    if (remotePtr->cwd != NULL) {
		Blt_Free(remotePtr->cwd);
	    }
	    remotePtr->cwdLength = length;
	    remotePtr->cwd = Blt_AssertStrdup(path);
	} else {
	    Tcl_AppendResult(interp, 
		"can't change current working directory to \"", 
		Tcl_GetString(objv[2]), 
		"\": not a directory.", (char *)NULL);
	    return TCL_ERROR;
	}
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), remotePtr->cwd,
			remotePtr->cwdLength);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ChgrpOp --
 *
 *	sftp chgrp path ?gid? ?-recurse?
 *
 *---------------------------------------------------------------------------
 */
static int
ChgrpOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    ApplyData data;
    ChgrpSwitches switches;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    Remote *remotePtr = clientData;
    const char *path;
    int id;
    int length;
    unsigned long gid;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    gid = (attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) ? attrs.gid : 0L;
    if (objc == 3) {
	Tcl_SetLongObj(Tcl_GetObjResult(interp), gid);
	return TCL_OK;
    }
    if (Tcl_GetIntFromObj(interp, objv[3], &id) != TCL_OK) {
	return TCL_ERROR;
    }
    switches.flags = 0;
    if (Blt_ParseSwitches(interp, chgrpSwitches, objc - 4, objv + 4, 
			  &switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    switches.gid = id;
    data.fileProc = ChgrpRemoteFile;
    data.dirProc = ChgrpRemoteFile;
    data.clientData = &switches;
    if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions) && (switches.flags & RECURSE)) {
	Blt_Chain entries;
	int result;
	
	entries = ReadRemoteEntries(NULL, remotePtr, path, length);
	if (entries == NULL) {
	    return TCL_ERROR;		/* Can't get directory entries. */
	}
	result = TCL_OK;
	if (Blt_Chain_GetLength(entries) > 0) {
	    result = ApplyToRemote(interp, remotePtr, path, length, &attrs, 
		entries, &data);
	}
	FreeRemoteEntries(entries);
	if (result !=TCL_OK) {
	    return TCL_ERROR;		/* Error chmod-ing entries. */
	}
    }
    if (ChgrpRemoteFile(interp, remotePtr, path, length, &attrs, &switches) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ChmodOp --
 *
 *	sftp chmod path ?mode? ?-recurse?
 *
 *---------------------------------------------------------------------------
 */
static int
ChmodOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    ApplyData data;
    ChmodSwitches switches;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    Remote *remotePtr = clientData;
    const char *path;
    int length;
    unsigned int setFlags, clearFlags;
    
    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 3) {
	char out[200];

	sprintf(out, "%0#5lo", attrs.permissions & 07777);
	Tcl_SetStringObj(Tcl_GetObjResult(interp), out, -1);
	return TCL_OK;
    }
    if (GetPermsFromObj(interp, objv[3], &setFlags, &clearFlags) != TCL_OK){
	return TCL_ERROR;
    }
    switches.flags = 0;
    if (Blt_ParseSwitches(interp, chmodSwitches, objc - 4, objv + 4, 
			  &switches, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    switches.clearFlags = clearFlags;
    switches.setFlags = setFlags;
    data.fileProc   = ChmodRemoteFile;
    data.dirProc    = ChmodRemoteFile;
    data.clientData = &switches;
    if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions) && (switches.flags & RECURSE)) {
	Blt_Chain entries;
	int result;
	
	entries = ReadRemoteEntries(NULL, remotePtr, path, length);
	if (entries == NULL) {
	    return TCL_ERROR;
	}
	result = TCL_OK;
	if (Blt_Chain_GetLength(entries) > 0) {
	    result = ApplyToRemote(interp, remotePtr, path, length, &attrs, 
		entries, &data);
	}
	FreeRemoteEntries(entries);
	if (result !=TCL_OK) {
	    return TCL_ERROR;
	}
    }
    if (ChmodRemoteFile(interp, remotePtr, path, length, &attrs, &switches) 
	!= TCL_OK) {
	return TCL_ERROR;
    }
    return TCL_OK;
}


/* 
 *---------------------------------------------------------------------------
 *
 * DeleteOp --
 *
 *	sftp delete path ?-force?
 *
 *---------------------------------------------------------------------------
 */
static int
DeleteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    ApplyData data;
    DeleteSwitches switches;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    Remote *remotePtr = clientData;
    const char *path;
    int length;
    int result;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    switches.flags = 0;
    if (Blt_ParseSwitches(interp, deleteSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	if (switches.flags & FORCE) {
	    return TCL_OK;
	}
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
			     "\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    data.fileProc = RemoveRemoteFile;
    data.dirProc = RemoveRemoteDirectory;
    data.clientData = (void *)((unsigned long)switches.flags & FORCE);
    if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	Blt_Chain entries;

	entries = ReadRemoteEntries(NULL, remotePtr, path, length);
	if (entries == NULL) {
	    return TCL_ERROR;
	}
	if (Blt_Chain_GetLength(entries) == 0) {
	    result = RemoveRemoteDirectory(interp, remotePtr, path, length, 
		&attrs, NULL);
	    FreeRemoteEntries(entries);
	    return result;
	}
	if ((switches.flags & FORCE) == 0) {
	    Tcl_AppendResult(interp, "can't delete directory \"", 
		Tcl_GetString(objv[2]), "\": is not empty", (char *)NULL);
	    FreeRemoteEntries(entries);
	    return TCL_ERROR;
	}
	result = ApplyToRemote(interp, remotePtr, path, length, &attrs, 
		entries, &data);
	FreeRemoteEntries(entries);
    } else {
	result = RemoveRemoteFile(interp, remotePtr, path, length, &attrs,NULL);
    }
    return result;
}

/* 
 *---------------------------------------------------------------------------
 *
 * DirListOp --
 *
 *	sftp dirlist path -listing yes 
 *
 *---------------------------------------------------------------------------
 */
static int
DirListOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	  Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    int length;
    DirectoryReader reader;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    memset(&reader, 0, sizeof(reader));
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (!LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	reader.match = FileTail(path);
	path = (const char *)FileHead((char *)path);
    }
    reader.flags = DIR_DEFAULT;
    reader.interp = interp;
    reader.remotePtr = remotePtr;
    reader.listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **)NULL);
    if (Blt_ParseSwitches(interp, dirListSwitches, objc - 3, objv + 3, &reader,
	BLT_SWITCH_DEFAULTS) < 0) {
	goto error;
    }
    if (ReadDirectoryIntoList(interp, path, length, &reader) != TCL_OK) {
	goto error;
    }
    Tcl_SetObjResult(interp, reader.listObjPtr);
    Blt_FreeSwitches(dirListSwitches, (char *)&reader, 0);
    return TCL_OK;
 error:
    if (reader.listObjPtr != NULL) {
	Tcl_DecrRefCount(reader.listObjPtr);
    }
    Blt_FreeSwitches(dirListSwitches, (char *)&reader, 0);
    return TCL_ERROR;
}

/* 
 *---------------------------------------------------------------------------
 *
 * DirTreeOp --
 *
 *	$sftp dirtree $path $tree ?-switches?
 *
 *---------------------------------------------------------------------------
 */
static int
DirTreeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Blt_Tree tree;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    Remote *remotePtr = clientData;
    TreeWriter writer;
    const char *path;
    int length, result;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    tree = Blt_Tree_GetFromObj(interp, objv[3]);
    if (tree == NULL) {
	return TCL_ERROR;
    }
    memset(&writer, 0, sizeof(writer));
    writer.flags = DIR_DEFAULT;
    writer.tree = tree;
    writer.root = Blt_Tree_RootNode(tree);
    writer.maxDepth = -1;
    if (Blt_ParseSwitches(interp, dirTreeSwitches, objc - 4, objv + 4, 
			  &writer, BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    result = ReadDirectoryIntoTree(interp, remotePtr, path, length, &writer, 
				   writer.root);
    Blt_FreeSwitches(dirTreeSwitches, (char *)&writer, 0);
    return result;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ExecOp --
 *
 *	sftp exec cmd
 *
 *---------------------------------------------------------------------------
 */
static int
ExecOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    const char *string;
    int length;
    Tcl_Obj *objPtr;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    string = Tcl_GetStringFromObj(objv[2], &length);
    objPtr = ExecOnRemote(interp, remotePtr, string, length);
    if (objPtr == NULL) {
	return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *	sftp exists path
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    int state, length, result;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    result = GetRemoteAttributes(remotePtr, path, length, &attrs);
    state = (result == TCL_OK);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}


/* 
 *---------------------------------------------------------------------------
 *
 * GetOp --
 *
 *	sftp get remoteFile ?localFile? \
 *		-progress cmd \
 *		-timeout 10 \
 *		-resume \
 *		-cancel variable 
 *
 *---------------------------------------------------------------------------
 */
static int
GetOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    int length, result;
    const char *path, *string, *localFile;
    FileReader reader;
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    memset(&reader, 0, sizeof(reader));
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) == 0) {
	Tcl_AppendResult(interp, "server does not report file size for \"",
			path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	Tcl_AppendResult(interp, 
		"recursive get not implemented for directories yet",
		(char *)NULL);
	return TCL_ERROR;
    }
    reader.interp = interp;
    reader.size = attrs.filesize;
    reader.remotePtr = remotePtr;
    if (objc > 3) {
	string = Tcl_GetString(objv[3]);
	if (string[0] == '-') {
	    goto localSameAsRemote;
	}
	localFile = string;
	objv++; objc--;
    } else {
    localSameAsRemote:
	localFile = FileTail(path);
    }
    if (Blt_ParseSwitches(interp, getSwitches, objc - 3, objv + 3, &reader,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    reader.f = Blt_OpenFile(interp, localFile, "w");
    if (reader.f == NULL) {
	fprintf(stderr, "localFile=%s\n", localFile);
	goto error;
    }
    if (reader.flags & RESUME) {
	struct stat stat;

	if (fstat(fileno(reader.f), &stat) < 0) {
	    Tcl_AppendResult(interp, "can't stat \"", localFile, "\": ", 
		Tcl_PosixError(interp), (char *)NULL);
	    goto error;
	}
	reader.offset = stat.st_size;
	reader.size -= stat.st_size;
    }
    if (GetRemoteFile(interp, path, length, &reader) != TCL_OK) {
	goto error;
    }
    if (reader.numRead != reader.size) {
	fprintf(stderr, "invalid file read: read=%ld wanted=%ld\n",
		reader.numRead, reader.size);
    }
    result = TCL_OK;
    fclose(reader.f);
    Blt_FreeSwitches(getSwitches, (char *)&reader, 0);
    return result;
 error:
    if (reader.f != NULL) {
	fclose(reader.f);
    }
    Blt_FreeSwitches(getSwitches, (char *)&reader, 0);
    return TCL_ERROR;
}



/* 
 *---------------------------------------------------------------------------
 *
 * GroupsOp --
 *
 *	sftp groups ?id?
 *
 *---------------------------------------------------------------------------
 */
static int
GroupsOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	 Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;

    if (objc == 3) {
	long gid;
	Blt_HashEntry *hPtr;

	if (Tcl_GetLongFromObj(interp, objv[2], &gid) != TCL_OK) {
	    return TCL_ERROR;
	}
	hPtr = Blt_FindHashEntry(&remotePtr->gidTable, (char *)gid);
	if (hPtr != NULL) {
	    const char *name;

	    name = Blt_GetHashValue(hPtr);
	    Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
	}
	return TCL_OK;
    } else {
	Tcl_Obj *listObjPtr;
	Blt_HashEntry *hPtr;
	Blt_HashSearch iter;

	listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
	for (hPtr = Blt_FirstHashEntry(&remotePtr->gidTable, &iter); 
	     hPtr != NULL; hPtr = Blt_NextHashEntry(&iter)) {
	    long gid;
	    const char *name;

	    gid = (long)Blt_GetHashKey(&remotePtr->gidTable, hPtr);
	    name = Blt_GetHashValue(hPtr);
	    Tcl_ListObjAppendElement(interp, listObjPtr, Tcl_NewLongObj(gid));
	    Tcl_ListObjAppendElement(interp, listObjPtr, 
		Tcl_NewStringObj(name, -1));
	}
	Tcl_SetObjResult(interp, listObjPtr);
    }
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * IsDirectoryOp --
 *
 *	sftp isdirectory path
 *
 *---------------------------------------------------------------------------
 */
static int
IsDirectoryOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    int state;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    state = LIBSSH2_SFTP_S_ISDIR(attrs.permissions);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * IsFileOp --
 *
 *	sftp isfile path
 *
 *---------------------------------------------------------------------------
 */
static int
IsFileOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    int state;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    state = LIBSSH2_SFTP_S_ISREG(attrs.permissions);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * LstatOp --
 *
 *	sftp lstat path varName
 *
 *---------------------------------------------------------------------------
 */
static int
LstatOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path, *varName, *type;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (libssh2_sftp_stat_ex(remotePtr->sftp, path, length, LIBSSH2_SFTP_LSTAT, 
	     &attrs) < 0) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    varName = Tcl_GetString(objv[3]);
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetVar2Ex(interp, varName, "atime", Tcl_NewLongObj(attrs.atime), 0);
    Tcl_SetVar2Ex(interp, varName, "mtime", Tcl_NewLongObj(attrs.mtime), 0);
    Tcl_SetVar2Ex(interp, varName, "size", Tcl_NewLongObj(attrs.filesize), 0);
    Tcl_SetVar2Ex(interp, varName, "gid", Tcl_NewIntObj(attrs.gid), 0);
    Tcl_SetVar2Ex(interp, varName, "uid", Tcl_NewIntObj(attrs.uid), 0);
    type = GetFileTypeFromAttributes(&attrs);
    Tcl_SetVar2Ex(interp, varName, "type", Tcl_NewStringObj(type, -1), 0);
    {
	char out[200];

	sprintf(out, "%0#5lo", attrs.permissions & 07777);
	Tcl_SetVar2Ex(interp, varName, "mode", Tcl_NewStringObj(out, -1), 0);
    }
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * MkdirOp --
 *
 *	Create the directories specified by the path if needed.  This is 
 *	the equivalent of "mkdir -p".
 *
 *	sftp mkdir path ?-mode mode?
 *
 *---------------------------------------------------------------------------
 */
static int
MkdirOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    MkdirSwitches switches;
    Remote *remotePtr = clientData;
    const char *path;
    int length, result;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    switches.mode = 0770;
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (Blt_ParseSwitches(interp, mkdirSwitches, objc - 3, objv + 3, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    result = RemoteMkdir(interp, remotePtr, path, length, switches.mode);
    Blt_FreeSwitches(mkdirSwitches, (char *)&switches, 0);
    return result;
}


/* 
 *---------------------------------------------------------------------------
 *
 * MtimeOp --
 *
 *	sftp mtime path ?seconds?
 *
 *---------------------------------------------------------------------------
 */
static int
MtimeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    unsigned long mtime;
    const char *path;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if (objc == 4) {
	long l;

	if (Tcl_GetLongFromObj(interp, objv[3], &l) != TCL_OK) {
	    return TCL_ERROR;
	}
	attrs.mtime = (unsigned long)l;
	attrs.flags = LIBSSH2_SFTP_ATTR_ACMODTIME;
	if (SetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	    Tcl_AppendResult(interp, "can't set access time for \"", 
		Tcl_GetString(objv[2]), "\": ", RemoteError(remotePtr), 
		(char *)NULL);
	    return TCL_ERROR;
	}
    }
    mtime = (attrs.flags & LIBSSH2_SFTP_ATTR_ACMODTIME) ? attrs.mtime : 0L;
    Tcl_SetLongObj(Tcl_GetObjResult(interp), mtime);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * NormalizeOp --
 *
 *	sftp normalize path
 *
 *---------------------------------------------------------------------------
 */
static int
NormalizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    char realPath[MAXPATHLEN+1];
    int length, numBytes;
    
    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    numBytes = libssh2_sftp_symlink_ex(remotePtr->sftp, path, length, realPath, 
	MAXPATHLEN, LIBSSH2_SFTP_REALPATH); 
    if (numBytes < 0) {
	Tcl_AppendResult(interp, "can't normalize \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), realPath, numBytes);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * OwnedOp --
 *
 *	sftp owned path
 *
 *---------------------------------------------------------------------------
 */
static int
OwnedOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    int state, length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) == 0) {
	Tcl_AppendResult(interp, "server does not report ownership of \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    state = (attrs.uid == remotePtr->uid);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * PutOp --
 *
 *	sftp put localFile ?remoteFile? \
 *		-progress cmd \
 *		-timeout 10 \
 *		-resume \
 *		-cancel variable 
 *
 *---------------------------------------------------------------------------
 */
static int
PutOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    int length, result;
    const char *localFile, *path;
    FileWriter writer;
    struct stat stat;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    Tcl_DString ds;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    memset(&writer, 0, sizeof(writer));
    writer.interp = interp;
    writer.remotePtr = remotePtr;
    writer.mode = 0640;
    localFile = Tcl_GetString(objv[2]);
    writer.f = Blt_OpenFile(interp, localFile, "r");
    if (writer.f == NULL) {
	return TCL_ERROR;
    }
    if (fstat(fileno(writer.f), &stat) < 0) {
	Tcl_AppendResult(interp, "can't stat \"", localFile, "\": ", 
		Tcl_PosixError(interp), (char *)NULL);
	fclose(writer.f);
	return TCL_ERROR;
    }
    writer.size = stat.st_size;
    if (objc > 3) {
	const char *string;
	
	string = Tcl_GetString(objv[3]);
	if (string[0] == '-') {
	    goto remoteSameAsLocal;
	}
	path = GetRemotePathFromObj(remotePtr, objv[3], &length);
	objv++, objc--;
    } else {
	const char *name;

    remoteSameAsLocal:
	name = FileTail(localFile);
	path = GetRemotePath(remotePtr, name, &length);
    }
    if (Blt_ParseSwitches(interp, putSwitches, objc - 3, objv + 3, &writer,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    Tcl_DStringInit(&ds);
    memset(&attrs, 0, sizeof(attrs));
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) == TCL_OK) {
	if (attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) {
	    if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
		/* Remote path is a directory. Append file name. */
		path = FileJoin(path, FileTail(localFile), &ds);
		length = Tcl_DStringLength(&ds);
	    }
	}
    }
    if ((GetRemoteAttributes(remotePtr, path, length, &attrs) == TCL_OK) &&
	(attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS)) {
	if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	    Tcl_AppendResult(interp, "can't put file \"", localFile, 
		"\": is a directory on remote", (char *)NULL);
	    return TCL_ERROR;
	}
	if ((writer.flags & FORCE) == 0) {
	    Tcl_AppendResult(interp, "can't put file \"", localFile, 
		"\": already exists on remote", (char *)NULL);
	    Tcl_DStringFree(&ds);
	    return TCL_ERROR;
	}
	if (writer.flags & RESUME) {
	    /* Seek to the same spot in the local file. */
	    if (fseek(writer.f, attrs.filesize, SEEK_SET) == 0) {
		writer.flags |= APPEND;
		writer.totalBytesWritten += attrs.filesize;
	    }	    
	}
    }
    if (PutRemoteFile(interp, path, length, &writer) != TCL_OK) {
	goto error;
    }
    if (writer.totalBytesWritten != writer.size) {
	fprintf(stderr, "invalid file write: written=%ld wanted=%ld\n",
		writer.totalBytesWritten, writer.size);
    }
    result = TCL_OK;
    fclose(writer.f);
    Blt_FreeSwitches(putSwitches, (char *)&writer, 0);
    Tcl_DStringFree(&ds);
    return result;
 error:
    if (writer.f != NULL) {
	fclose(writer.f);
    }
    Blt_FreeSwitches(putSwitches, (char *)&writer, 0);
    Tcl_DStringFree(&ds);
    return TCL_ERROR;
}

/* 
 *---------------------------------------------------------------------------
 *
 * PwdOp --
 *
 *	sftp pwd
 *
 *---------------------------------------------------------------------------
 */
static int
PwdOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;

    Tcl_SetStringObj(Tcl_GetObjResult(interp), remotePtr->cwd,
		     remotePtr->cwdLength);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ReadOp --
 *
 *	sftp read file -progress cmd -timeout 10 -cancel variable 
 *
 *---------------------------------------------------------------------------
 */
static int
ReadOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    int length, result;
    const char *path;
    FileReader reader;
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    memset(&reader, 0, sizeof(reader));
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) == 0) {
	Tcl_AppendResult(interp, "server does not report file size for \"",
			path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	Tcl_AppendResult(interp, "can't read from \"", Tcl_GetString(objv[2]), 
		"\" : is a directory.", (char *)NULL);
	return TCL_ERROR;
    }
    reader.interp = interp;
    reader.size = attrs.filesize;
    reader.remotePtr = remotePtr;
    if (Blt_ParseSwitches(interp, readSwitches, objc - 3, objv + 3, &reader,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    reader.dbuffer = Blt_DBuffer_Create();
    result = GetRemoteFile(interp, path, length, &reader);
    if (result == TCL_OK) {
	if (reader.numRead != reader.size) {
	    fprintf(stderr, "invalid file read: read=%ld wanted=%ld\n",
		reader.numRead, reader.size);
	}
	Tcl_SetObjResult(interp, Blt_DBuffer_StringObj(reader.dbuffer));
    }
    Blt_DBuffer_Destroy(reader.dbuffer);
    Blt_FreeSwitches(readSwitches, (char *)&reader, 0);
    return result;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ReadableOp --
 *
 *	sftp readable path
 *
 *---------------------------------------------------------------------------
 */
static int
ReadableOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int state;
    int length;
    const char *path;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_UIDGID) == 0) {
	Tcl_AppendResult(interp, "server does not report ownership of \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    /*
     * FIXME:  Check usr perms if we have the same uid.
     *	       Check grp perms if one of ours group ids matches.
     *	       Check oth perms if none of the above.
     *	       Need to sftp_exec "id" command and parse its output.
     */
    if (remotePtr->uid == attrs.uid) {
	state = (attrs.permissions & LIBSSH2_SFTP_S_IRUSR);
    } else if ((remotePtr->gid == attrs.gid) || 
	       (Blt_FindHashEntry(&remotePtr->gidTable, (char *)attrs.gid))) {
	state = (attrs.permissions & LIBSSH2_SFTP_S_IRGRP);
    } else {
	state = (attrs.permissions & LIBSSH2_SFTP_S_IROTH);
    }	
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * ReadlinkOp --
 *
 *	sftp normalize path
 *
 *---------------------------------------------------------------------------
 */
static int
ReadlinkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	    Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    char linkPath[MAXPATHLEN+1];
    int length, numBytes;
    
    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, 
		"server does not report permissions for \"", path, "\"", 
		(char *)NULL);
	return TCL_ERROR;
    }
    if (!LIBSSH2_SFTP_S_ISLNK(attrs.permissions)) {
	Tcl_AppendResult(interp, "can't read link \"", Tcl_GetString(objv[2]), 
		"\": ", "not a link", (char *)NULL);
	return TCL_ERROR;
    }
    numBytes = libssh2_sftp_symlink_ex(remotePtr->sftp, path, length, linkPath, 
	MAXPATHLEN, LIBSSH2_SFTP_READLINK); 
    if (numBytes < 0) {
	Tcl_AppendResult(interp, "can't read link \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), linkPath, numBytes);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * RenameOp --
 *
 *	sftp rename srcpath dstpath -force
 *
 *---------------------------------------------------------------------------
 */
static int
RenameOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    int flags;
    const char *src, *dst;
    int srcLen, dstLen;
    RenameSwitches switches;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    switches.flags = 0;
    if (Blt_ParseSwitches(interp, renameSwitches, objc - 4, objv + 4, &switches,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    
    flags = LIBSSH2_SFTP_RENAME_ATOMIC | LIBSSH2_SFTP_RENAME_NATIVE;
    if (switches.flags & FORCE) {
	flags |= LIBSSH2_SFTP_RENAME_OVERWRITE;
    }
    src = GetRemotePathFromObj(remotePtr, objv[2], &srcLen);
    src = Blt_AssertStrdup(src);	/* Make a copy of the source path. The
					 * next call the GetRemotePathFromObj
					 * will overwrite the path.  */
    dst = GetRemotePathFromObj(remotePtr, objv[3], &dstLen);
    if (libssh2_sftp_rename_ex(remotePtr->sftp, src, srcLen, dst, dstLen, 
	flags) < 0) {
	Tcl_AppendResult(interp, "can't rename \"", Tcl_GetString(objv[2]), 
		"\" to \"", Tcl_GetString(objv[3]), "\": ", RemoteError(remotePtr), 
		(char *)NULL);
	Blt_Free(src);
	return TCL_ERROR;
    }
    Blt_Free(src);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * RmdirOp --
 *
 *	sftp rmdir path
 *
 *---------------------------------------------------------------------------
 */
static int
RmdirOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    int length, numEntries;
    Blt_Chain entries;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    if (!LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
	Tcl_AppendResult(interp, "can't remove \"", Tcl_GetString(objv[2]), 
		"\": not a directory", (char *)NULL);
	return TCL_ERROR;
    }	
    entries = ReadRemoteEntries(NULL, remotePtr, path, length);
    if (entries == NULL) {
	return TCL_ERROR;
    }
    numEntries = Blt_Chain_GetLength(entries);
    FreeRemoteEntries(entries);
    if (numEntries > 0) {
	Tcl_AppendResult(interp, "can't remove \"", Tcl_GetString(objv[2]), 
		"\": is not empty", (char *)NULL);
	return TCL_ERROR;
    }
    if (libssh2_sftp_rmdir_ex(remotePtr->sftp, path, length) < 0) {
	Tcl_AppendResult(interp, "can't remove directory \"", 
		Tcl_GetString(objv[2]), "\": ", RemoteError(remotePtr), 
		(char *)NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * SizeOp --
 *
 *	sftp size path
 *
 *---------------------------------------------------------------------------
 */
static int
SizeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    const char *path;
    int length;
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]),
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_SIZE) == 0) {
	if (interp != NULL) {
	    Tcl_AppendResult(interp, "server does not report file size for \"",
		path, "\"", (char *)NULL);
	}
	return TCL_ERROR;
    }
    Tcl_SetLongObj(Tcl_GetObjResult(interp), attrs.filesize);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * SlinkOp --
 *
 *	sftp slink linkName ?path?
 *
 *---------------------------------------------------------------------------
 */
static int
SlinkOp(ClientData clientData, Tcl_Interp *interp, int objc, 
	Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path;
    const char *linkName;
    int length, linkLen;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[3], &length);
    /* Create new symbolic link to path.  Link can't already exist. Path
     * must already exist. */
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) == TCL_OK) {
	Tcl_AppendResult(interp, "can't link to \"", Tcl_GetString(objv[3]), 
		"\": already exists.", (char *)NULL);
	return TCL_ERROR;
    }
    linkName = Blt_AssertStrdup(path);
    linkLen = length;
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	Blt_Free(linkName);
	return TCL_ERROR;
    }
    if (libssh2_sftp_symlink_ex(remotePtr->sftp, path, length, (char *)linkName,
                linkLen, LIBSSH2_SFTP_SYMLINK) < 0) {
	Tcl_AppendResult(interp, "can't symlink \"", Tcl_GetString(objv[2]),
		"\": ", RemoteError(remotePtr), (char *)NULL);
	Blt_Free(linkName);
	return TCL_ERROR;
    }
    Blt_Free(linkName);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * StatOp --
 *
 *	sftp stat path varName
 *
 *---------------------------------------------------------------------------
 */
static int
StatOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path, *varName, *type;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    varName = Tcl_GetString(objv[3]);
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    Tcl_SetVar2Ex(interp, varName, "atime", Tcl_NewLongObj(attrs.atime), 0);
    Tcl_SetVar2Ex(interp, varName, "mtime", Tcl_NewLongObj(attrs.mtime), 0);
    Tcl_SetVar2Ex(interp, varName, "size", Tcl_NewLongObj(attrs.filesize), 0);
    Tcl_SetVar2Ex(interp, varName, "gid", Tcl_NewIntObj(attrs.gid), 0);
    Tcl_SetVar2Ex(interp, varName, "uid", Tcl_NewIntObj(attrs.uid), 0);
    type = GetFileTypeFromAttributes(&attrs);
    Tcl_SetVar2Ex(interp, varName, "type", Tcl_NewStringObj(type, -1), 0);
    {
	char out[200];

	sprintf(out, "%0#5lo", attrs.permissions & 07777);
	Tcl_SetVar2Ex(interp, varName, "mode", Tcl_NewStringObj(out, -1), 0);
    }
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * TypeOp --
 *
 *	sftp type path
 *
 *---------------------------------------------------------------------------
 */
static int
TypeOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    const char *path, *type;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    type = GetFileTypeFromAttributes(&attrs);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), type, -1);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * WritableOp --
 *
 *	sftp writable path
 *
 *---------------------------------------------------------------------------
 */
static int
WritableOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int state;
    const char *path;
    int length;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    }
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    if (GetRemoteAttributes(remotePtr, path, length, &attrs) != TCL_OK) {
	Tcl_AppendResult(interp, "can't stat \"", Tcl_GetString(objv[2]), 
		"\": ", RemoteError(remotePtr), (char *)NULL);
	return TCL_ERROR;
    }
    if ((attrs.flags & LIBSSH2_SFTP_ATTR_PERMISSIONS) == 0) {
	Tcl_AppendResult(interp, "server does not report permissions for \"",
			 path, "\"", (char *)NULL);
	return TCL_ERROR;
    }
    /*
     * FIXME:  Check usr perms if we have the same uid.
     *	       Check grp perms if one of ours group ids matches.
     *	       Check oth perms if none of the above.
     *	       Need to sftp_exec "id" command and parse its output.
     */
    if (remotePtr->uid == attrs.uid) {
	state = (attrs.permissions & LIBSSH2_SFTP_S_IWUSR);
    } else if ((remotePtr->gid == attrs.gid) || 
	       (Blt_FindHashEntry(&remotePtr->gidTable, (char *)attrs.gid))) {
	state = (attrs.permissions & LIBSSH2_SFTP_S_IWGRP);
    } else {
	state = (attrs.permissions & LIBSSH2_SFTP_S_IWOTH);
    }	
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/* 
 *---------------------------------------------------------------------------
 *
 * WriteOp --
 *
 *	sftp write path string -progress cmd -timeout 10 -append yes
 *		
 *---------------------------------------------------------------------------
 */
static int
WriteOp(ClientData clientData, Tcl_Interp *interp, int objc, 
       Tcl_Obj *const *objv) 
{
    Remote *remotePtr = clientData;
    int length, result, numBytes;
    const char *path;
    FileWriter writer;

    if (remotePtr->sftp == NULL) {
	if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	    return TCL_ERROR;
	}
    } 
    path = GetRemotePathFromObj(remotePtr, objv[2], &length);
    memset(&writer, 0, sizeof(writer));
    writer.interp = interp;
    writer.remotePtr = remotePtr;
    writer.mode = 0640;
    writer.string = Tcl_GetStringFromObj(objv[3], &numBytes);
    writer.size = numBytes;
    if (Blt_ParseSwitches(interp, writeSwitches, objc - 4, objv + 4, &writer,
	BLT_SWITCH_DEFAULTS) < 0) {
	return TCL_ERROR;
    }
    result = PutRemoteFile(interp, path, length, &writer);
    if (writer.totalBytesWritten != writer.size) {
	fprintf(stderr, "invalid file write: written=%ld wanted=%ld\n",
		writer.totalBytesWritten, writer.size);
    }
    Blt_FreeSwitches(writeSwitches, (char *)&writer, 0);
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpCmdInstObjCmdProc --
 *
 * 	This procedure is invoked to process commands on behalf of the sftp
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
static Blt_OpSpec sftpOps[] =
{
    {"atime",       2, AtimeOp,         3, 4, "path ?seconds?",},
    {"auth",	    2, AuthOp,		2, 2, "",},
    {"chdir",       3, ChdirOp,         2, 3, "?path?",},
    {"chgrp",       3, ChgrpOp,         3, 0, "path ?gid ?-recurse??",},
    {"chmod",       3, ChmodOp,         3, 0, "path ?mode ?-recurse??",},
    {"delete",      2, DeleteOp,        3, 0, "path ?switches?",},
    {"dirlist",     4, DirListOp,       3, 0, "path ?switches?",},
    {"dirtree",     4, DirTreeOp,	4, 0, "path tree ?switches?",},
    {"exec",        3, ExecOp,          3, 3, "command",},
    {"exists",      3, ExistsOp,        3, 3, "path",},
    {"get",         2, GetOp,           3, 0, "path ?file? ?switches?",},
    {"groups",      2, GroupsOp,        2, 3, "?gid?",},
    {"isdirectory", 3, IsDirectoryOp,   3, 3, "path",},
    {"isfile",      3, IsFileOp,        3, 3, "path",},
    {"lstat",       1, LstatOp,		4, 4, "path varName",},
    {"mkdir",       2, MkdirOp,         3, 0, "path ?-mode mode?",},
    {"mtime",       2, MtimeOp,         3, 4, "path ?seconds?",},
    {"normalize",   1, NormalizeOp,     3, 3, "path",},
    {"owned",       1, OwnedOp,		3, 3, "path",},
    {"put",         2, PutOp,		3, 0, "file ?path? ?switches?",},
    {"pwd",         2, PwdOp,		2, 2, "",},
    {"read",	    4, ReadOp,          4, 0, "path ?switches?",},
    {"readable",    5, ReadableOp,      3, 3, "path",},
    {"readlink",    5, ReadlinkOp,      3, 3, "path",},
    {"rename",      3, RenameOp,        3, 0, "old new ?-force?",},
    {"rmdir",       2, RmdirOp,         3, 3, "path",},
    {"size",        2, SizeOp,		3, 3, "path",},
    {"slink",	    2, SlinkOp,		4, 4, "path link",},
    {"stat",        2, StatOp,		4, 4, "path varName",},
    {"type",        1, TypeOp,		3, 3, "path",},
    {"writable",    5, WritableOp,	3, 3, "path",},
    {"write",       5, WriteOp,		4, 0, "path string ?switches?",},
};

static int numSftpOps = sizeof(sftpOps) / sizeof(Blt_OpSpec);

static int
SftpCmdInstObjCmdProc(
    ClientData clientData,		/* Information about the widget. */
    Tcl_Interp *interp,			/* Interpreter to report errors. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const *objv)		/* Vector of argument strings. */
{
    Tcl_ObjCmdProc *proc;
    Remote *remotePtr = clientData;
    int result;

    /* Delete current timeout timer. */
    if (remotePtr->idleTimerToken != (Tcl_TimerToken) 0) {
	Tcl_DeleteTimerHandler(remotePtr->idleTimerToken);
	remotePtr->idleTimerToken = 0;
    }
    proc = Blt_GetOpFromObj(interp, numSftpOps, sftpOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    Tcl_Preserve(remotePtr);
    result = (*proc) (remotePtr, interp, objc, objv);
    Tcl_Release(remotePtr);
    /* Restore idle timeout timer. */
    if (remotePtr->idleTimeout > 0) {
	remotePtr->idleTimerToken = 
	    Tcl_CreateTimerHandler(remotePtr->idleTimeout * 1000, 
		RemoteIdleTimerProc, remotePtr);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpCmdInstDeleteProc --
 *
 *	Deletes the command associated with the sftp connection.  This is
 *	called only when the command associated with the sftp connection is
 *	destroyed.
 *
 * Results:
 *	None.
 *
 *---------------------------------------------------------------------------
 */
static void
SftpCmdInstDeleteProc(ClientData clientData)
{
    Remote *remotePtr = clientData;

    DestroyRemote(remotePtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * RemoteIdleTimerProc --
 *
 *	This procedure is called when the sftp session have been idle
 *	for the designated interval.  The session is then disconnected.
 *	It will be automatically restored on the next sftp operation.
 *
 *---------------------------------------------------------------------------
 */
static void
RemoteIdleTimerProc(ClientData clientData)
{
    Remote *remotePtr = clientData;

    if (remotePtr->sftp != NULL) {
	DisconnectFromRemote(remotePtr);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpCreateOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SftpCreateOp(
    ClientData clientData,		/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    const char *name;
    Remote *remotePtr;
    SftpCmdInterpData *dataPtr = clientData;
    Tcl_DString ds;
    int isNew;

    name = NULL;
    remotePtr = NULL;
    if (objc > 2) {
	const char *string;

	string = Tcl_GetString(objv[2]);
	if (string[0] != '-') {
	    objv++, objc--;
	    name = string;
	}
    }
    Tcl_DStringInit(&ds);
    if (name == NULL) {
	name = GenerateName(interp, dataPtr, "", "", &ds);
    } else {
	char *p;

	p = strstr(name, "#auto");
	if (p != NULL) {
	    *p = '\0';
	    Tcl_DStringInit(&ds);
	    name = GenerateName(interp, dataPtr, name, p + 5, &ds);
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
		goto error;
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
	} 
    } 
    if (name == NULL) {
	goto error;
    }
    remotePtr = NewRemote(clientData, interp);
    if (remotePtr == NULL) {
	goto error;
    }
    remotePtr->password = Blt_AssertStrdup("");
#ifdef WIN32
    remotePtr->user = Blt_AssertStrdup(getenv("USERNAME"));
#else
    remotePtr->user = Blt_AssertStrdup(getenv("USER"));
#endif /*WIN32*/
    remotePtr->host = Blt_AssertStrdup("localhost");
    remotePtr->publickey = Blt_AssertStrdup(KEYFILE);
    /* Process switches  */
    if (Blt_ParseSwitches(interp, sftpSwitches, objc - 2, objv + 2, remotePtr,
	BLT_SWITCH_DEFAULTS) < 0) {
	goto error;
    }
    /* Try to connect to the server. */
    if (ConnectToRemote(interp, remotePtr) != TCL_OK) {
	goto error;
    }
    remotePtr->cwd = Blt_AssertStrdup(remotePtr->homedir);
    remotePtr->cwdLength = strlen(remotePtr->homedir);
    remotePtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)name, 
	SftpCmdInstObjCmdProc, remotePtr, SftpCmdInstDeleteProc);
    remotePtr->tablePtr = &dataPtr->sessionTable;
    remotePtr->hashPtr = Blt_CreateHashEntry(remotePtr->tablePtr, 
	(char *)remotePtr, &isNew);
    remotePtr->name = Blt_GetHashKey(remotePtr->tablePtr, remotePtr->hashPtr);
    Blt_SetHashValue(remotePtr->hashPtr, remotePtr);
    Tcl_SetStringObj(Tcl_GetObjResult(interp), name, -1);
    /* Setup initial idle timeout timer. */
    if (remotePtr->idleTimeout > 0) {
	remotePtr->idleTimerToken = 
	    Tcl_CreateTimerHandler(remotePtr->idleTimeout * 1000, 
		RemoteIdleTimerProc, remotePtr);
    }
    return TCL_OK;
 error:
    if (remotePtr != NULL) {
	DestroyRemote(remotePtr);
    }
    Tcl_DStringFree(&ds);
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpDestroyOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SftpDestroyOp(
    ClientData clientData,		/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    SftpCmdInterpData *dataPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
	Remote *remotePtr;
	char *string;

	string = Tcl_GetString(objv[i]);
	remotePtr = GetRemote(dataPtr, interp, string);
	if (remotePtr == NULL) {
	    Tcl_AppendResult(interp, "can't find a sftp session named \"", 
			     string, "\"", (char *)NULL);
	    return TCL_ERROR;
	}
	Tcl_DeleteCommandFromToken(interp, remotePtr->cmdToken);
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpNamesOp --
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
SftpNamesOp(
    ClientData clientData,		/* Interpreter-specific data. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    SftpCmdInterpData *dataPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&dataPtr->sessionTable, &iter); hPtr != NULL;
	hPtr = Blt_NextHashEntry(&iter)) {
	Blt_ObjectName objName;
	Remote *remotePtr;
	const char *qualName;
	int i, found;

	remotePtr = Blt_GetHashValue(hPtr);
	objName.name = Tcl_GetCommandName(interp, remotePtr->cmdToken);
	objName.nsPtr = Blt_GetCommandNamespace(remotePtr->cmdToken);
	qualName = Blt_MakeQualifiedName(&objName, &ds);
	found = FALSE;
	for (i = 2; i < objc; i++) {
	    if (Tcl_StringMatch(qualName, Tcl_GetString(objv[i]))) {
		found = TRUE;
		break;
	    }
	}
	if ((objc == 2) || (found)) {
	    Tcl_Obj *objPtr;

	    objPtr = Tcl_NewStringObj(qualName, -1);
	    Tcl_ListObjAppendElement(interp, listObjPtr, objPtr);
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    Tcl_DStringFree(&ds);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpObjCmdProc --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec sftpCmdOps[] =
{
    {"create",  1, SftpCreateOp,  2, 0, "?name? ?switches?",},
    {"destroy", 1, SftpDestroyOp, 3, 0, "name...",},
    {"names",   1, SftpNamesOp,   2, 0, "?pattern?...",},
};

static int numCmdOps = sizeof(sftpCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
SftpObjCmdProc(
    ClientData clientData,		/* Pointer to Remote structure. */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numCmdOps, sftpCmdOps, BLT_OP_ARG1, objc, 
	objv, 0);
    if (proc == NULL) {
	return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * SftpCmdInterpDeleteProc --
 *
 *	This is called when the interpreter hosting the "sftp" command
 *	is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes the hash table managing all sftp names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
SftpCmdInterpDeleteProc(
    ClientData clientData,		/* Interpreter-specific data. */
    Tcl_Interp *interp)
{
    SftpCmdInterpData *dataPtr = clientData;

    /* 
     * All sftp instances should already have been destroyed when their
     * respective TCL commands were deleted.
     */
    Blt_DeleteHashTable(&dataPtr->sessionTable);
    Tcl_DeleteAssocData(interp, SFTP_THREAD_KEY);
    libssh2_exit();
    Blt_Free(dataPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * Blt_sftp_Init --
 *
 *	This procedure is invoked to initialize the "sftp" command.
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
Blt_sftp_Init(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
	"sftp", SftpObjCmdProc, 
    };
    int result;

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
    result = libssh2_init(0);
    if (result != 0) {
	Tcl_AppendResult(interp, "libssh2 initialization failed: code = %d", 
			 Blt_Itoa(result), (char *)NULL);
	return TCL_ERROR;
    }
    cmdSpec.clientData = GetSftpCmdInterpData(interp);
    if (Blt_InitCmd(interp, "::blt", &cmdSpec) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_PkgProvide(interp, "blt_sftp", BLT_VERSION) != TCL_OK) { 
	return TCL_ERROR;
    }
    return TCL_OK;
}

#endif /* NO_SFTP */
