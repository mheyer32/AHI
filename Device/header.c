/* $Id$ */

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>

#ifndef VERSION68K
# include <powerup/ppclib/object.h>
# include <powerup/ppclib/interface.h>
# include <proto/ppc.h>
#endif

#include "ahi_def.h"
#include "version.h"
#include "device.h"
#include "localize.h"
#include "misc.h"

static BOOL
OpenLibs ( void );

static void
CloseLibs ( void );


/******************************************************************************
** Device entry ***************************************************************
******************************************************************************/

int Start( void )
{
  return -1;
}


/******************************************************************************
** Device residend strcuture **************************************************
******************************************************************************/

extern void _etext;
extern const char DevName[];
extern const char IDString[];
static const APTR InitTable[4];

static const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (APTR) &_etext,
  RTF_AUTOINIT,
  VERSION,
  NT_DEVICE,
  0,                      /* priority */
  (BYTE *) DevName,
  (BYTE *) IDString,
  (APTR) &InitTable
};


/******************************************************************************
** Globals ********************************************************************
******************************************************************************/

struct ExecBase           *SysBase        = NULL;
struct AHIBase            *AHIBase        = NULL;
struct DosLibrary         *DOSBase        = NULL;
struct Library            *GadToolsBase   = NULL;
struct GfxBase            *GfxBase        = NULL;
struct Library            *IFFParseBase   = NULL;
struct IntuitionBase      *IntuitionBase  = NULL;
struct LocaleBase         *LocaleBase     = NULL;
struct Device             *TimerBase      = NULL;
struct UtilityBase        *UtilityBase    = NULL;
#ifndef VERSION68K
struct Library            *PPCLibBase     = NULL;
void                      *PPCObject      = NULL;
#endif

/* linker can use symbol b for symbol a if a is not defined */
#define ALIAS(a,b) asm(".stabs \"_" #a "\",11,0,0,0\n.stabs \"_" #b "\",1,0,0,0")

ALIAS( __UtilityBase, UtilityBase );

const ULONG			           DriverVersion  = 2;
const ULONG			           Version        = VERSION;
const ULONG			           Revision       = REVISION;

const char DevName[]   = AHINAME;
const char IDString[]  = "ahi.device " VERS "\r\n";
const char VersTag[]   = "$VER: ahi.device " VERS " "
                         "©1994-1999 Martin Blom. "
#ifdef mc68060
                         "68060"
#else
# ifdef mc68040
                         "68040"
# else
#  ifdef mc68030
                         "68030"
#  else
#   ifdef mc68020
                         "68020"
#   else
#    ifdef mc68000
                         "68000"
#    endif /* mc68000 */
#   endif /* mc68020 */
#  endif /* mc68030 */
# endif /* mc68040 */
#endif /* mc68060 */

#ifndef VERSION68K
                         "/PPC"
#endif
                         " version.\r\n";


/******************************************************************************
** Device code ****************************************************************
******************************************************************************/

static struct AHIBase * ASMCALL
initRoutine( REG( d0, struct AHIBase* device ),
	     REG( a0, APTR seglist ),
	     REG( a6, struct ExecBase* sysbase ) )
{
  SysBase = sysbase;
  AHIBase = device;

  AHIBase->ahib_Library.lib_Node.ln_Type = NT_DEVICE;
  AHIBase->ahib_Library.lib_Node.ln_Name = (STRPTR) DevName;
  AHIBase->ahib_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  AHIBase->ahib_Library.lib_Version      = VERSION;
  AHIBase->ahib_Library.lib_Revision     = REVISION;
  AHIBase->ahib_Library.lib_IdString     = (STRPTR) IDString;
  AHIBase->ahib_SysLib  = sysbase;
  AHIBase->ahib_SegList = (ULONG) seglist;

#ifdef MC68020_PLUS
  if( ( SysBase->AttnFlags & AFF_68020 ) == 0 )
  {
    Alert( ( AN_Unknown | ACPU_InstErr ) & (~AT_DeadEnd) );
    return NULL;
  }
#endif

  InitSemaphore( &AHIBase->ahib_Lock );

  if( !OpenLibs() )
  {
    return NULL;
  }

  return AHIBase;
}


BPTR ASMCALL
DevExpunge( REG( a6, struct AHIBase* device ) )
{
  BPTR seglist = 0;

  device->ahib_Library.lib_Flags |= LIBF_DELEXP;

  if( device->ahib_Library.lib_OpenCnt == 0)
  {
    seglist = device->ahib_SegList;

    Remove( (struct Node *) device );

    CloseLibs();

    FreeMem( (APTR) ( ( (char*) device ) - device->ahib_Library.lib_NegSize ),
	     device->ahib_Library.lib_NegSize + device->ahib_Library.lib_PosSize );
  }

  return seglist;
}


int Null( void )
{
  return 0;
}


extern APTR DevOpen;
extern APTR DevClose;

extern APTR DevBeginIO;
extern APTR DevAbortIO;

extern APTR AllocAudioA;
extern APTR FreeAudio;
extern APTR KillAudio;
extern APTR ControlAudioA;
extern APTR SetVol;
extern APTR SetFreq;
extern APTR SetSound;
extern APTR SetEffect;
extern APTR LoadSound;
extern APTR UnloadSound;
extern APTR NextAudioID;
extern APTR GetAudioAttrsA;
extern APTR BestAudioIDA;
extern APTR AllocAudioRequestA;
extern APTR AudioRequestA;
extern APTR FreeAudioRequest;
extern APTR PlayA;
extern APTR SampleFrameSize;
extern APTR AddAudioMode;
extern APTR RemoveAudioMode;
extern APTR LoadModeFile;


static const APTR funcTable[] =
{
  &DevOpen,
  &DevClose,
  &DevExpunge,
  &Null,

  &DevBeginIO,
  &DevAbortIO,

  &AllocAudioA,
  &FreeAudio,
  &KillAudio,
  &ControlAudioA,
  &SetVol,
  &SetFreq,
  &SetSound,
  &SetEffect,
  &LoadSound,
  &UnloadSound,
  &NextAudioID,
  &GetAudioAttrsA,
  &BestAudioIDA,
  &AllocAudioRequestA,
  &AudioRequestA,
  &FreeAudioRequest,
  &PlayA,
  &SampleFrameSize,
  &AddAudioMode,
  &RemoveAudioMode,
  &LoadModeFile,
  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct AHIBase ),
  (APTR) &funcTable,
  0,
  (APTR) initRoutine
};


static struct timerequest *TimerIO        = NULL;
static struct timeval     *timeval        = NULL;

/******************************************************************************
** OpenLibs *******************************************************************
******************************************************************************/

// This function is called by the device startup code when the device is
// first loaded into memory.

static BOOL
OpenLibs ( void )
{
  /* Intuition Library */

  IntuitionBase = (struct IntuitionBase *) OpenLibrary( "intuition.library", 37 );

  if( IntuitionBase == NULL)
  {
    Alert(AN_Unknown|AG_OpenLib|AO_Intuition);
    return FALSE;
  }

  /* DOS Library */

  DOSBase = (struct DosLibrary *) OpenLibrary( "dos.library", 37 );

  if( DOSBase == NULL)
  {
    Req( "Unable to open dos.library." );
    return FALSE;
  }

  /* Graphics Library */

  GfxBase = (struct GfxBase *) OpenLibrary( "graphics.library", 37 );

  if( GfxBase == NULL)
  {
    Req( "Unable to open graphics.library." );
    return FALSE;
  }

  /* GadTools Library */

  GadToolsBase = OpenLibrary( "gadtools.library", 37 );

  if( GadToolsBase == NULL)
  {
    Req( "Unable to open gadtools.library." );
    return FALSE;
  }

  /* IFFParse Library */

  IFFParseBase = OpenLibrary( "iffparse.library", 37 );

  if( IFFParseBase == NULL)
  {
    Req( "Unable to open iffparse.library." );
    return FALSE;
  }

  /* Locale Library */

  LocaleBase = (struct LocaleBase*) OpenLibrary( "locale.library", 38 );

  /* Timer Device */

  TimerIO = (struct timerequest *) AllocVec( sizeof(struct timerequest),
                                             MEMF_PUBLIC | MEMF_CLEAR );

  if( TimerIO == NULL)
  {
    Req( "Out of memory." );
    return FALSE;
  }

  timeval = (struct timeval *) AllocVec( sizeof(struct timeval),
                                         MEMF_PUBLIC | MEMF_CLEAR);

  if( timeval == NULL)
  {
    Req( "Out of memory." );
    return FALSE;
  }

  if( OpenDevice( "timer.device",
                  UNIT_MICROHZ,
                  (struct IORequest *)
                  TimerIO,
                  0) != 0 )
  {
    Req( "Unable to open timer.device." );
    return FALSE;
  }

  TimerBase = (struct Device *) TimerIO->tr_node.io_Device;

  /* Utility Library */

  UtilityBase = (struct UtilityBase *) OpenLibrary( "utility.library", 37 );

  if( UtilityBase == NULL)
  {
    Req( "Unable to open utility.library." );
    return FALSE;
  }


#ifndef VERSION68K
  /* PPC library */

  PPCLibBase = OpenLibrary( "ppc.library", 46 );

  if( PPCLibBase != NULL )
  {
    /* No big deal if things fail. In that case, the 68K version
       will be used instead. */

    /* Load our code to PPC..  */

    PPCObject = PPCLoadObject( "DEVS:ahi.elf" );

    if( PPCObject == NULL )
    {
      /* This, however, is a problem. */

      Req( "Unable to load PPC module." );
      return FALSE;
    }    
  }
#endif 

  OpenahiCatalog(NULL, NULL);

  return TRUE;
}


/******************************************************************************
** CloseLibs *******************************************************************
******************************************************************************/

// This function is called by DevExpunge() when the device is about to be
// flushed

static void
CloseLibs ( void )
{
  CloseahiCatalog();

#ifndef VERSION68K
  if( PPCObject != NULL ) PPCUnLoadObject( PPCObject );
  CloseLibrary( PPCLibBase );
#endif
  CloseLibrary( (struct Library *) UtilityBase );
  if(TimerIO) CloseDevice( (struct IORequest *) TimerIO );
  FreeVec( timeval );
  FreeVec( TimerIO );
  CloseLibrary( (struct Library *) LocaleBase );
  CloseLibrary( (struct Library *) IntuitionBase );
  CloseLibrary( IFFParseBase );
  CloseLibrary( GadToolsBase );
  CloseLibrary( (struct Library *) GfxBase );
  CloseLibrary( (struct Library *) DOSBase );

}
