
/*
 *  Common BOOPSI frontend code
 */

#include <config.h>

#include <exec/alerts.h>
#include <exec/libraries.h>
#include <exec/resident.h>
#include <utility/utility.h>

#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>

#include "boopsi-stubs.h"
#include "boopsi-impl.h"
#include "ahiclass.h"
#include "util.h"
#include "version.h"

#ifndef INTUITIONNAME
#define INTUITIONNAME "intuition.library"
#endif

#if !defined (__AROS__) && !defined (__amithlon__)
extern void _etext;
#else
# define _etext RomTag+1 // Fake it
#endif

/******************************************************************************
** Module entry and false exit ************************************************
******************************************************************************/

int
_start(void) {
  return -1;
}

void
exit(int rc) {
  Req(_AHI_CLASS_NAME " called exit() with return code %ld\n"
       "Can't terminate myself; becomming a zombie", rc);

  while(TRUE) {
    Wait(0);
  }
}

/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

const char  ClassName[]     = _AHI_CLASS_NAME;
const char  ClassIDString[] = _AHI_CLASS_NAME " " VERS "\r\n";
const UWORD ClassVersion    = VERSION;
const UWORD ClassRevision   = REVISION;

struct DosLibrary*    DOSBase;
struct ExecBase*      SysBase;
struct IntuitionBase* IntuitionBase;
struct UtilityBase*   UtilityBase;


#if defined (__MORPHOS__)
ULONG   __abox__=1;
ULONG   __amigappc__=1;  // deprecated, used in MOS 0.4
#endif

/* linker can use symbol b for symbol a if a is not defined */
#define ALIAS(a,b) asm(".stabs \"_" #a "\",11,0,0,0\n.stabs \"_" #b "\",1,0,0,0")

ALIAS(__UtilityBase, UtilityBase);
ALIAS(__DOSBase, DOSBase);

/******************************************************************************
** Module resident structure **************************************************
******************************************************************************/

static const APTR FuncTable[] = {
#if defined (__MORPHOS__) || defined (__amithlon__)
  (APTR) FUNCARRAY_32BIT_NATIVE,
#endif

  gwClassOpen,
  gwClassClose,
  gwClassExpunge,
  gwClassNull,
  gwObtainEngine,
  (APTR) -1
};


static const APTR InitTable[4] = {
  (APTR) sizeof(struct AHIClassBase),
  (APTR) &FuncTable,
  NULL,
#if defined (__MORPHOS__) || defined (__amithlon__)
  (APTR) _ClassInit
#else
  (APTR) gwClassInit
#endif

};

// This structure must reside in the text segment or the read-only
// data segment!  "const" makes it happen.

static const struct Resident RomTag = {
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &_etext,
#if defined (__MORPHOS__) 
  RTF_EXTENDED | RTF_PPC | RTF_AUTOINIT,
#elif defined (__amithlon__)
  RTF_PPC | RTF_AUTOINIT,
#else
  RTF_AUTOINIT,
#endif
  VERSION,
  NT_LIBRARY,
  0,                      /* priority */
  (BYTE *) ClassName,
  (BYTE *) ClassIDString,
  (APTR) &InitTable
#if defined (__MORPHOS__)
  , REVISION, NULL
#endif
};

/******************************************************************************
** HookEntry ******************************************************************
******************************************************************************/

#if defined (__MORPHOS__)

/* Should be in libamiga, but isn't? */

static ULONG
gw_HookEntry(void) {
  struct Hook* h   = (struct Hook*) REG_A0;
  void*        o   = (void*)        REG_A2; 
  void*        msg = (void*)        REG_A1;

  return (((ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)(h, o, msg));
}

struct EmulLibEntry _HookEntry = {
  TRAP_LIB, 0, (void (*)(void)) &gw_HookEntry
};

__asm(".globl HookEntry;HookEntry=_HookEntry");

#endif

/******************************************************************************
** Class init *****************************************************************
******************************************************************************/

struct ClassLibrary*
_ClassInit(struct ClassLibrary*  library,
	   BPTR                  seglist,
	   struct ExecBase*      sysbase) {
  static const char classname[] = _AHI_CLASS_NAME;
  static const char supername[] = _AHI_SUPER_NAME;
  static const char superlib[]  = "AHI/" _AHI_SUPER_NAME;
  static const int  supervers   = _AHI_SUPER_VERS;
  
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) library;
  SysBase = sysbase;

  AHIClassBase->common.cl.cl_Lib.lib_Node.ln_Type = NT_LIBRARY;
  AHIClassBase->common.cl.cl_Lib.lib_Node.ln_Name = (STRPTR) ClassName;
  AHIClassBase->common.cl.cl_Lib.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  AHIClassBase->common.cl.cl_Lib.lib_Version      = VERSION;
  AHIClassBase->common.cl.cl_Lib.lib_Revision     = REVISION;
  AHIClassBase->common.cl.cl_Lib.lib_IdString     = (STRPTR) ClassIDString;
  AHIClassBase->common.seglist                    = seglist;

  IntuitionBase = (struct IntuitionBase*) OpenLibrary(INTUITIONNAME, 37);
  DOSBase       = (struct DosLibrary*)    OpenLibrary(DOSNAME, 37);
  UtilityBase   = (struct UtilityBase*)   OpenLibrary(UTILITYNAME, 37);

  if (IntuitionBase == NULL) {
    Alert(AN_Unknown|AG_OpenLib|AO_Intuition);
    goto error;
  }

  if (DOSBase == NULL) {
    Req("Unable to open 'dos.library' version 37.\n");
    goto error;
  }

  if (UtilityBase == NULL) {
    Req("Unable to open 'utility.library' version 37.\n");
    goto error;
  }

  if (supername[0] != '\0') {
    AHIClassBase->common.super = (struct ClassLibrary*)
      OpenLibrary(superlib, supervers);

    if (AHIClassBase->common.super == NULL) { 
      Req("Unable to open super class library\n"
	  "'%s' version %ld", (ULONG) superlib, supervers );
      goto error;
    }

    AHIClassBase->common.cl.cl_Class =
      MakeClass(classname, NULL, AHIClassBase->common.super->cl_Class,
		sizeof (struct AHIClassData), 0);
  }
  else {
    AHIClassBase->common.cl.cl_Class =
      MakeClass(classname, ROOTCLASS, NULL,
		sizeof (struct AHIClassData), 0);
  }
  
  if (AHIClassBase->common.cl.cl_Class == NULL)
  {
    Req("Unable to create " _AHI_CLASS_NAME " Class.");
    goto error;
  }
  
  if (! AHIClassInit(AHIClassBase)) {
    goto error;
  }

  AHIClassBase->common.cl.cl_Class->cl_Dispatcher.h_Entry    = HookEntry;
  AHIClassBase->common.cl.cl_Class->cl_Dispatcher.h_SubEntry = AHIClassDispatch;
  AHIClassBase->common.cl.cl_Class->cl_UserData              = (ULONG) AHIClassBase;

  // Go public
  AddClass(AHIClassBase->common.cl.cl_Class);
  return &AHIClassBase->common.cl;

error:
  _ClassExpunge(AHIClassBase);
  return NULL;
}


/******************************************************************************
** Class clean-up *************************************************************
******************************************************************************/

BPTR
_ClassExpunge(struct AHIClassBase* AHIClassBase) {
  BPTR seglist = 0;

  if (AHIClassBase->common.cl.cl_Lib.lib_OpenCnt == 0) {
    if (AHIClassBase->common.cl.cl_Class != NULL) {
      KPrintF("Freeing class " _AHI_CLASS_NAME "\n");

      // FreeClass() will also RemoveClass() us
      if (! FreeClass(AHIClassBase->common.cl.cl_Class)) {
	Req("Unable to free BOOPSI class.");

	/* What to do?? */
	AHIClassBase->common.cl.cl_Lib.lib_Flags |= LIBF_DELEXP;
	return 0;
      }
    }
    
    seglist = AHIClassBase->common.seglist;

    /* Since ClassInit() calls us on failure, we have to check if we're
       really added to the library list before removing us. */

    if (AHIClassBase->common.cl.cl_Lib.lib_Node.ln_Succ != NULL) {
      Remove((struct Node *) AHIClassBase);
    }

    AHIClassCleanup(AHIClassBase);

    /* Close super class */
    CloseLibrary((struct Library*) AHIClassBase->common.super);
    
    /* Close libraries */
    CloseLibrary((struct Library*) DOSBase);
    CloseLibrary((struct Library*) IntuitionBase);
    CloseLibrary((struct Library*) UtilityBase);

    FreeMem((APTR) (((char*) AHIClassBase) -
		      AHIClassBase->common.cl.cl_Lib.lib_NegSize),
             (AHIClassBase->common.cl.cl_Lib.lib_NegSize +
	       AHIClassBase->common.cl.cl_Lib.lib_PosSize));
  }
  else {
    AHIClassBase->common.cl.cl_Lib.lib_Flags |= LIBF_DELEXP;
  }

  return seglist;
}


/******************************************************************************
** Class opening **************************************************************
******************************************************************************/

struct ClassLibrary*
_ClassOpen(ULONG version, struct AHIClassBase* AHIClassBase) {
  AHIClassBase->common.cl.cl_Lib.lib_Flags &= ~LIBF_DELEXP;
  AHIClassBase->common.cl.cl_Lib.lib_OpenCnt++;

  return &AHIClassBase->common.cl;
}


/******************************************************************************
** Class closing **************************************************************
******************************************************************************/

BPTR
_ClassClose(struct AHIClassBase* AHIClassBase) {
  BPTR seglist = 0;

  AHIClassBase->common.cl.cl_Lib.lib_OpenCnt--;

  if (AHIClassBase->common.cl.cl_Lib.lib_OpenCnt == 0) {
    if (AHIClassBase->common.cl.cl_Lib.lib_Flags & LIBF_DELEXP) {
      seglist = _ClassExpunge(AHIClassBase);
    }
  }

  return seglist;
}


/******************************************************************************
** GetClassEngine *************************************************************
******************************************************************************/

Class*
_ObtainEngine(struct AHIClassBase* AHIClassBase) {
  return AHIClassBase->common.cl.cl_Class;
}


/******************************************************************************
** Unused function ************************************************************
******************************************************************************/

ULONG
_ClassNull(struct AHIClassBase* AHIClassBase) {
  return 0;
}
