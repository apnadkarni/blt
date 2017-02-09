/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * bltArgsParse.c --
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

/* 
   set parser [blt::argsparse create parser \
        -description "Process some integers"]
   $parser add "integers" -metavar N -type int -nargs +
   $parser add "--sum" -dest accumulate -action store_const -const sum \
        -default max -help "sum the integers (default: find the max)'

   set results [$parser parse $argv]
   
   store_const
   append_const
   -type int | double | 
 */

#define BUILD_BLT_TCL_PROCS 1
#include <bltInt.h>

#ifndef NO_ARGSPARSE

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

#define PARSER_THREAD_KEY "BLT ArgsParse Command Data"
#define PARSER_MAGIC ((unsigned int) 0x46170277)

typedef struct {
    Tcl_Interp *interp;
    Blt_HashTable parserTable;		/* Hash table of parsers keyed by
                                         * address. */
} ParserCmdInterpData;

typedef struct {
    Tcl_Interp *interp;
    Tcl_Command cmdToken;               /* Token for parser's TCL command. */
    Blt_HashEntry *hashPtr;
    Blt_HashTable *tablePtr;
    ParserCmdInterpData *tdPtr;		/*  */
    Blt_HashTable argTable;		/* Table of arguments. Arguments
					 * are keys by their name. */
    Blt_Chain args;			/* List of arguments. */
} ParserCmd;

#define ALLOW_ABBREV    (1<<1)

typedef struct {
    ParserCmd *cmdPtr;			/* Parser this argument belongs to. */
    const char *name;			/* Name of the argument. */
    const char *metaName;		/* Meta variable name. */
    const char *desc;			/* Description of the argument. */
    int type;				/* Type of argument. */
    Tcl_Obj *varNameObjPtr;		/* Name of TCL variable to set with
					 * the arguments value. */
    int numTokens;			/* # of words required. */
    const char **shortSwitch;
    const char *defValue;		/* Default argument value. */
    int defValueType;			/* Type of default value. */
} ParserArg;

#define ARG_REQUIRED    (1<<0)

static Blt_SwitchSpec cmdSpecs[] = 
{
    {BLT_SWITCH_STRING, "-program", "string", (char *)NULL,
        Blt_Offset(ParseArg, progName), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-usage", "string", (char *)NULL,
        Blt_Offset(ParseArg, usage), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-epilog", "string", (char *)NULL,
        Blt_Offset(ParseArg, epilog), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-prefix_chars", "string", (char *)NULL,
        Blt_Offset(ParseArg, prefixChars), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-argument_default", "string", (char *)NULL,
        Blt_Offset(ParseArg, argDefault), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_BITS_NOARG, "-allow_abbreviations", "bool", (char *)NULL,
        Blt_Offset(ParseArg, flags), 0, ALLOW_ABBREV},
    {BLT_SWITCH_CUSTOM, "-addhelp", "???", (char *)NULL,
        Blt_Offset(ParseArg, position), 0, 0, &beforeSwitch},
    {BLT_SWITCH_BOOLEAN, "-error", "bool", (char *)NULL,
        Blt_Offset(ParseArg, dataPairs), 0},
    {BLT_SWITCH_END}
};

static Blt_SwitchSpec argSpecs[] = 
{
    {BLT_SWITCH_STRING, "-description", "string", (char *)NULL,
        Blt_Offset(ParseArg, desc), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_STRING, "-help", "string", (char *)NULL,
        Blt_Offset(ParseArg, helpMesg), BLT_SWITCH_NULL_OK},
    {BLT_SWITCH_CUSTOM, "-type", "typeName", (char *)NULL,
        Blt_Offset(ParseArg, position), 0, 0, &typeSwitch},
    {BLT_SWITCH_CUSTOM, "-default", "defValue", (char *)NULL,
        Blt_Offset(ParseArg, position), 0, 0, &defaultSwitch},
    {BLT_SWITCH_LIST, "-data", "{name value ?name value ...?}", (char *)NULL,
        Blt_Offset(ParseArg, dataPairs), 0},
    {BLT_SWITCH_CUSTOM, "-nargs", "number", (char *)NULL,
        Blt_Offset(ParseArg, numArgs), 0, 0, &numArgsSwitch},
    {BLT_SWITCH_STRING, "-short", "string", (char *)NULL,
        Blt_Offset(ParseArg, label), 0},
    {BLT_SWITCH_BOOLEAN, "-required", "bool", (char *)NULL,
        Blt_Offset(ParseArg, flags), 0, ARG_REQUIRED},
    {BLT_SWITCH_OBJ,    "-variable", "varName", (char *)NULL,
        Blt_Offset(ParseArg, varNameObjPtr), 0},
    {BLT_SWITCH_OBJ,    "-command",  "cmdPrefix", (char *)NULL,
        Blt_Offset(ParseArg, cmdObjPtr), 0},
    {BLT_SWITCH_OBJ, "-tags", "tagList", (char *)NULL,
        Blt_Offset(ParseArg, tagsObjPtr), 0},
    {BLT_SWITCH_END}
};


static ParseArg *
LookupSwitch(Tcl_Interp *interp, ParserCmd *parserPtr, Tcl_Obj *objPtr)
{
    const char *switchName;
    int length;
    int isLongName;
    
    isLongName = FALSE;
    switchName = Tcl_GetStringFromObj(objPtr, &length);
    if (length > 2) {
        isLongName = (switchName[1] == '-');
    } 
    if (isLongName) {
        int count;
        Blt_ChainLink link;
        
        count = 0;
        /* Long or short look up. */
        for (link = Blt_Chain_FirstLink(parserPtr->args); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            ParserArg *argPtr;
            
            argsPtr = Blt_Chain_GetValue(link);
            if (parserPtr->flags & ALLOW_ABBREV) {
                if (strncmp(switchName, argPtr->longName, length) == 0) {
                    if (length == strlen(argPtr->longName)) {
                        return argPtr;
                    }
                    foundPtr = argPtr;
                    count++;
                }
            } else {
                if (strcmp(switchName, argPtr->longName) == 0) {
                    return argPtr;
                }
            }
        }
        if (count == 0) {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "switch not found.", (char *)NULL);
            }
        } else if (count == 1) {
            return foundPtr;
        } else {
            if (interp != NULL) {
                Tcl_AppendResult(interp, "switch is ambiguous.", (char *)NULL);
            }
        }
    } else {
        int count;
        Blt_ChainLink link;
        
        count = 0;
        /* short look up. */
        for (link = Blt_Chain_FirstLink(parserPtr->args); link != NULL;
             link = Blt_Chain_NextLink(link)) {
            ParserArg *argPtr;
            
            argsPtr = Blt_Chain_GetValue(link);
            if (argPtr->shortName[1] == swithName[0]) {
                return argPtr;
            }
        }
        if (interp != NULL) {
            Tcl_AppendResult(interp, "switch not found.", (char *)NULL);
        }
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetParserCmd --
 *
 *      Find the parser command associated with the TCL command "string".
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
 *              b) really is a command for a parser object.  
 *      Tcl_GetCommandInfo will get us the objClientData field that should be
 *      a cmdPtr.  We can verify that by searching our hashtable of cmdPtr
 *      addresses.
 *
 * Results:
 *      A pointer to the parser command.  If no associated parser command
 *      can be found, NULL is returned.  It's up to the calling routines to
 *      generate an error message.
 *
 *---------------------------------------------------------------------------
 */
static ParserCmd *
GetParserCmd(ParserCmdInterpData *dataPtr, Tcl_Interp *interp, 
             const char *string)
{
    Blt_HashEntry *hPtr;
    Blt_ObjectName objName;
    Tcl_CmdInfo cmdInfo;
    Tcl_DString ds;
    const char *parserName;
    int result;

    /* Pull apart the parser name and put it back together in a standard
     * format. */
    if (!Blt_ParseObjectName(interp, string, &objName, BLT_NO_ERROR_MSG)) {
        return NULL;                    /* No such parent namespace. */
    }
    /* Rebuild the fully qualified name. */
    parserName = Blt_MakeQualifiedName(&objName, &ds);
    result = Tcl_GetCommandInfo(interp, parserName, &cmdInfo);
    Tcl_DStringFree(&ds);

    if (!result) {
        return NULL;
    }
    hPtr = Blt_FindHashEntry(&dataPtr->parserTable, 
                             (char *)(cmdInfo.objClientData));
    if (hPtr == NULL) {
        return NULL;
    }
    return Blt_GetHashValue(hPtr);
}

static ParserCmd *
NewParserCmd(ClientData clientData, Tcl_Interp *interp, const char *name)
{
    ParserCmdInterpData *dataPtr = clientData;
    Tcl_DString ds;
    ParserCmd *cmdPtr;

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
            if (ParserExists(interp, name)) {
                Tcl_AppendResult(interp, "an argument parser \"", name, 
                        "\" already exists", (char *)NULL);
                goto error;
            }
        } 
    } 
    if (name == NULL) {
        goto error;
    }
    {
        int isNew;
        ParserCmd *cmdPtr;

        cmdPtr = Blt_AssertCalloc(1, sizeof(ParserCmd));
        cmdPtr->dataPtr = dataPtr;
        cmdPtr->interp = interp;
        Blt_InitHashTable(&cmdPtr->argTable, BLT_STRING_KEYS);
        Blt_InitHashTable(&cmdPtr->notifyTable, BLT_STRING_KEYS);
        cmdPtr->args = Blt_Chain_Create();
        cmdPtr->cmdToken = Tcl_CreateObjCommand(interp, (char *)name, 
                (Tcl_ObjCmdProc *)ParserInstObjCmd, cmdPtr,
		ParserInstDeleteProc);
        cmdPtr->tablePtr = &tdPtr->parserTable;
        cmdPtr->hashPtr = Blt_CreateHashEntry(cmdPtr->tablePtr, (char *)cmdPtr,
              &isNew);
        Blt_SetHashValue(cmdPtr->hashPtr, cmdPtr);
        Tcl_SetStringObj(Tcl_GetObjResult(interp), (char *)name, -1);
        Tcl_DStringFree(&ds);
        return cmdPtr;
    }
 error:
    Tcl_DStringFree(&ds);
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * AddOp --
 *
 *      parserName add argName ?switches...?
 *
 *---------------------------------------------------------------------------
 */
static int
AddOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    ParserCmd *cmdPtr = clientData;
    ParserArg *argPtr;
    Blt_HashEntry *hPtr;
    const char *name;

    name = Tcl_GetString(objv[2]);
    hPtr = Blt_CreateHashEntry(&cmdPtr->argTable, name, &isNew);
    if (!isNew) {
        Tcl_AppendResult(interp, "argument \"", name, 
                         "\" already exists in the parser.", (char *)NULL);
        return TCL_ERROR;
    }
    argPtr = NewArg(interp, cmdPtr, hPtr);
    if (argPtr == NULL) {
        return TCL_ERROR;
    }
    if (Blt_ParseSwitches(interp, argSpecs, objc - 3, objv + 3, argPtr,
        BLT_SWITCH_DEFAULTS) < 0) {
        DestroyArg(argPtr);
        return TCL_ERROR;
    }
    Tcl_SetStringObj(Tcl_GetObjResult(interp), argsPtr->name, -1);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ExistsOp --
 *
 *      parserName exists argName
 *
 *---------------------------------------------------------------------------
 */
static int
ExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
         Tcl_Obj *const *objv)
{
    ParserCmd *cmdPtr = clientData;
    ParserArg *argPtr;
    int bool;
    
    bool = TRUE;
    if (GetArgFromObj(NULL, cmdPtr, objv[2], &argPtr) != TCL_OK) {
        bool = FALSE;
    } 
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), bool);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParserInstObjCmd --
 *
 *      This procedure is invoked to process commands on behalf of the
 *	argument parser object.
 *
 * Results:
 *      A standard TCL result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *	parserName add argName 
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec parserInstOps[] =
{
    {"add",         1, AddOp,         3, 0, "argName ?switches ...?",},
    {"cget",        2, CgetOp,        3, 0, "argNane ?switches ...?",},
    {"configure",   2, ConfigureOp,   4, 0, "argName key ?value ...?",},
    {"delete",      1, DeleteOp,      2, 0, "?argName ...?",},
    {"exists",      1, ExistsOp,      3, 3, "argName",},
    {"get",         1, GetOp,         3, 4, "argName ?defValue?",},
    {"help",        1, HelpOp,        2, 0, "",},
    {"parse",       1, ParseOp,       3, 0, "parse ?args ...?",},
    {"reset",	    1, ResetOp	      2, 2, "",},
    {"set",         1, SetOp,         4, 4, "argName varName",},
};

static int numParserInstOps = sizeof(parserInstOps) / sizeof(Blt_OpSpec);

static int
ParserInstObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
		 Tcl_Obj *const *objv)
{
    ParserCmd *cmdPtr = clientData;
    Tcl_ObjCmdProc *proc;
    int result;

    proc = Blt_GetOpFromObj(interp, numParserInstOps, parserInstOps,
			    BLT_OP_ARG1, objc, objv, 0);
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
 * ParserInstDeleteProc --
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
ParserInstDeleteProc(ClientData clientData)
{
    Parser *parserPtr = clientData;

    ReleaseTreeObject(parserPtr);
    if (parserPtr->hashPtr != NULL) {
        Blt_DeleteHashEntry(parserPtr->tablePtr, parserPtr->hashPtr);
    }
    Blt_Free(parserPtr);
}


/*
 *---------------------------------------------------------------------------
 *
 * ParserCreateOp --
 *
 *      blt::parseargs create ?name? ?switches...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ParserCreateOp(ClientData clientData, Tcl_Interp *interp, int objc,
	       Tcl_Obj *const *objv)
{
    const char *name;
    ParserCmd *cmdPtr;

    name = NULL;
    if (objc == 3) {
        name = Tcl_GetString(objv[2]);
    }
    cmdPtr = NewParserCmd(clientData, interp, name);
    if (cmdPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParserDestroyOp --
 *
 *      blt::parseargs destory ?parserName...? 
 *
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ParserDestroyOp(ClientData clientData, Tcl_Interp *interp, int objc,
		Tcl_Obj *const *objv)
{
    ParserCmdInterpData *tdPtr = clientData;
    int i;

    for (i = 2; i < objc; i++) {
        ParserCmd *cmdPtr;
        const char *string;

        string = Tcl_GetString(objv[i]);
        cmdPtr = GetParserCmd(tdPtr, interp, string);
        if (cmdPtr == NULL) {
            Tcl_AppendResult(interp, "can't find a parser named \"", string,
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
 * ParserExistsOp --
 *
 *      blt::parseargs exists parserName
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ParserExistsOp(ClientData clientData, Tcl_Interp *interp, int objc,
             Tcl_Obj *const *objv)
{
    ParserCmd *cmdPtr;
    ParserCmdInterpData *tdPtr = clientData;
    const char *string;
    int state;
    
    string = Tcl_GetString(objv[3]);
    cmdPtr = GetParserCmd(tdPtr, interp, string);
    state = (cmdPtr != NULL);
    Tcl_SetBooleanObj(Tcl_GetObjResult(interp), state);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * ParserNamesOp --
 *
 *      blt::argsparse names ?pattern ...?
 *---------------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
ParserNamesOp(ClientData clientData, Tcl_Interp *interp, int objc,
            Tcl_Obj *const *objv)
{
    ParserCmdInterpData *tdPtr = clientData;
    Blt_HashEntry *hPtr;
    Blt_HashSearch iter;
    Tcl_Obj *listObjPtr;
    Tcl_DString ds;

    Tcl_DStringInit(&ds);
    listObjPtr = Tcl_NewListObj(0, (Tcl_Obj **) NULL);
    for (hPtr = Blt_FirstHashEntry(&tdPtr->treeTable, &iter); hPtr != NULL; 
        hPtr = Blt_NextHashEntry(&iter)) {
        Blt_ObjectName objName;
        ParseCmd *cmdPtr;
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

/*
 *---------------------------------------------------------------------------
 *
 * ParserObjCmd --
 *
 *---------------------------------------------------------------------------
 */
static Blt_OpSpec parserCmdOps[] =
{
    {"create",  1, ParserCreateOp,  2, 3, "?parserName?",},
    {"destroy", 1, ParserDestroyOp, 2, 0, "?parserName ...?",},
    {"exists",  1, ParserExistsOp,  3, 3, "parserName",},
    {"names",   1, ParserNamesOp,   2, 3, "?pattern ...?",},
};

static int numCmdOps = sizeof(parserCmdOps) / sizeof(Blt_OpSpec);

/*ARGSUSED*/
static int
ParserObjCmd(ClientData clientData, Tcl_Interp *interp, int objc,
           Tcl_Obj *const *objv)
{
    Tcl_ObjCmdProc *proc;

    proc = Blt_GetOpFromObj(interp, numParserOps, parserCmdOps, BLT_OP_ARG1,
			    objc, objv, 0);
    if (proc == NULL) {
        return TCL_ERROR;
    }
    return (*proc) (clientData, interp, objc, objv);
}

/*
 *---------------------------------------------------------------------------
 *
 * ParserInterpDeleteProc --
 *
 *      This is called when the interpreter hosting the "argparse" command
 *      is deleted.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Removes the hash table managing all parser names.
 *
 *---------------------------------------------------------------------------
 */
/* ARGSUSED */
static void
ParserInterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    ParserCmdInterpData *tdPtr = clientData;

    /* 
     * All parser instances should already have been destroyed when their
     * respective TCL commands were deleted.
     */
    Blt_DeleteHashTable(&tdPtr->parserTable);
    Tcl_DeleteAssocData(interp, PARSER_THREAD_KEY);
    Blt_Free(tdPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Blt_ArgsParseCmdInitProc --
 *
 *      This procedure is invoked to initialize the "argsparse" command.
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
Blt_ArgsParseCmdInitProc(Tcl_Interp *interp)
{
    static Blt_CmdSpec cmdSpec = { 
        "argsparse", ParserObjCmd, 
    };
    cmdSpec.clientData = GetParserCmdInterpData(interp);
    return Blt_InitCmd(interp, "::blt", &cmdSpec);
}
