/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2000 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include "ahi_def.h"

#include "header.h"
#include "gateway.h"
#include "localize.h"
#include "misc.h"
#include "version.h"

static BOOL
OpenLibs ( void );

static void
CloseLibs ( void );

#define GetSymbol( name ) AHIGetELFSymbol( #name, (void*) &name ## Ptr )

/******************************************************************************
** Device entry ***************************************************************
******************************************************************************/

int Start( void )
{
  return -1;
}

#ifdef morphos
ULONG   __amigappc__=1;
#endif

/******************************************************************************
** Device resident structure **************************************************
******************************************************************************/

extern const char DevName[];
extern const char IDString[];
static const APTR InitTable[4];

static const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &RomTag + 1,
#ifdef morphos
  RTF_PPC | RTF_AUTOINIT,
#else
  RTF_AUTOINIT,
#endif
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
struct Resident           *MorphOSRes     = NULL;
struct Library            *PowerPCBase    = NULL;
void                      *PPCObject      = NULL;

ADDFUNC* AddByteMonoPtr                   = NULL;
ADDFUNC* AddByteStereoPtr                 = NULL;
ADDFUNC* AddBytesMonoPtr                  = NULL;
ADDFUNC* AddBytesStereoPtr                = NULL;
ADDFUNC* AddWordMonoPtr                   = NULL;
ADDFUNC* AddWordStereoPtr                 = NULL;
ADDFUNC* AddWordsMonoPtr                  = NULL;
ADDFUNC* AddWordsStereoPtr                = NULL;
ADDFUNC* AddByteMonoBPtr                  = NULL;
ADDFUNC* AddByteStereoBPtr                = NULL;
ADDFUNC* AddBytesMonoBPtr                 = NULL;
ADDFUNC* AddBytesStereoBPtr               = NULL;
ADDFUNC* AddWordMonoBPtr                  = NULL;
ADDFUNC* AddWordStereoBPtr                = NULL;
ADDFUNC* AddWordsMonoBPtr                 = NULL;
ADDFUNC* AddWordsStereoBPtr               = NULL;

ADDFUNC* AddLofiByteMonoPtr               = NULL;
ADDFUNC* AddLofiByteStereoPtr             = NULL;
ADDFUNC* AddLofiBytesMonoPtr              = NULL;
ADDFUNC* AddLofiBytesStereoPtr            = NULL;
ADDFUNC* AddLofiWordMonoPtr               = NULL;
ADDFUNC* AddLofiWordStereoPtr             = NULL;
ADDFUNC* AddLofiWordsMonoPtr              = NULL;
ADDFUNC* AddLofiWordsStereoPtr            = NULL;
ADDFUNC* AddLofiByteMonoBPtr              = NULL;
ADDFUNC* AddLofiByteStereoBPtr            = NULL;
ADDFUNC* AddLofiBytesMonoBPtr             = NULL;
ADDFUNC* AddLofiBytesStereoBPtr           = NULL;
ADDFUNC* AddLofiWordMonoBPtr              = NULL;
ADDFUNC* AddLofiWordStereoBPtr            = NULL;
ADDFUNC* AddLofiWordsMonoBPtr             = NULL;
ADDFUNC* AddLofiWordsStereoBPtr           = NULL;

/* linker can use symbol b for symbol a if a is not defined */
#define ALIAS(a,b) asm(".stabs \"_" #a "\",11,0,0,0\n.stabs \"_" #b "\",1,0,0,0")

ALIAS( __UtilityBase, UtilityBase );

const ULONG                                DriverVersion  = 2;
const ULONG                                Version        = VERSION;
const ULONG                                Revision       = REVISION;

const char DevName[]   = AHINAME;
const char IDString[]  = AHINAME " " VERS "\r\n";

static const char VersTag[] =
 "$VER: " AHINAME " " VERS " ©1994-2000 Martin Blom. "
 CPU 
 "/PPC"
 " version.\r\n";

enum MixBackend_t          MixBackend     = MB_NATIVE;

/******************************************************************************
** Device code ****************************************************************
******************************************************************************/

struct AHIBase*
initRoutine( struct AHIBase*  device,
             APTR             seglist,
             struct ExecBase* sysbase )
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


BPTR
DevExpunge( struct AHIBase* device )
{
  BPTR seglist = 0;

  if( device->ahib_Library.lib_OpenCnt == 0)
  {
    seglist = device->ahib_SegList;

    Remove( (struct Node *) device );

    CloseLibs();

    FreeMem( (APTR) ( ( (char*) device ) - device->ahib_Library.lib_NegSize ),
             device->ahib_Library.lib_NegSize + device->ahib_Library.lib_PosSize );
  }
  else
  {
    device->ahib_Library.lib_Flags |= LIBF_DELEXP;
  }

  return seglist;
}


ULONG
Null( void )
{
  return 0;
}

static const APTR funcTable[] =
{

#if defined( morphos )
  (APTR) FUNCARRAY_32BIT_NATIVE,
#endif

  &gw_DevOpen,
  &gw_DevClose,
  &gw_DevExpunge,
  &Null,

  &gw_DevBeginIO,
  &gw_DevAbortIO,

  &gw_AllocAudioA,
  &gw_FreeAudio,
  &gw_KillAudio,
  &gw_ControlAudioA,
  &gw_SetVol,
  &gw_SetFreq,
  &gw_SetSound,
  &gw_SetEffect,
  &gw_LoadSound,
  &gw_UnloadSound,
  &gw_NextAudioID,
  &gw_GetAudioAttrsA,
  &gw_BestAudioIDA,
  &gw_AllocAudioRequestA,
  &gw_AudioRequestA,
  &gw_FreeAudioRequest,
  &gw_PlayA,
  &gw_SampleFrameSize,
  &gw_AddAudioMode,
  &gw_RemoveAudioMode,
  &gw_LoadModeFile,
  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct AHIBase ),
  (APTR) &funcTable,
  0,
  (APTR) gw_initRoutine
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
    Req( "Unable to open 'dos.library'." );
    return FALSE;
  }

  /* Graphics Library */

  GfxBase = (struct GfxBase *) OpenLibrary( "graphics.library", 37 );

  if( GfxBase == NULL)
  {
    Req( "Unable to open 'graphics.library'." );
    return FALSE;
  }

  /* GadTools Library */

  GadToolsBase = OpenLibrary( "gadtools.library", 37 );

  if( GadToolsBase == NULL)
  {
    Req( "Unable to open 'gadtools.library'." );
    return FALSE;
  }

  /* IFFParse Library */

  IFFParseBase = OpenLibrary( "iffparse.library", 37 );

  if( IFFParseBase == NULL)
  {
    Req( "Unable to open 'iffparse.library'." );
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
    Req( "Unable to open 'timer.device'." );
    return FALSE;
  }

  TimerBase = (struct Device *) TimerIO->tr_node.io_Device;

  /* Utility Library */

  UtilityBase = (struct UtilityBase *) OpenLibrary( "utility.library", 37 );

  if( UtilityBase == NULL)
  {
    Req( "Unable to open 'utility.library'." );
    return FALSE;
  }

  // Fill in some defaults...

  AddByteMonoPtr         = AddByteMono;
  AddByteStereoPtr       = AddByteStereo;
  AddBytesMonoPtr        = AddBytesMono;
  AddBytesStereoPtr      = AddBytesStereo;
  AddWordMonoPtr         = AddWordMono;
  AddWordStereoPtr       = AddWordStereo;
  AddWordsMonoPtr        = AddWordsMono;
  AddWordsStereoPtr      = AddWordsStereo;
  AddByteMonoBPtr        = AddByteMonoB;
  AddByteStereoBPtr      = AddByteStereoB;
  AddBytesMonoBPtr       = AddBytesMonoB;
  AddBytesStereoBPtr     = AddBytesStereoB;
  AddWordMonoBPtr        = AddWordMonoB;
  AddWordStereoBPtr      = AddWordStereoB;
  AddWordsMonoBPtr       = AddWordsMonoB;
  AddWordsStereoBPtr     = AddWordsStereoB;

  AddLofiByteMonoPtr     = AddLofiByteMono;
  AddLofiByteStereoPtr   = AddLofiByteStereo;
  AddLofiBytesMonoPtr    = AddLofiBytesMono;
  AddLofiBytesStereoPtr  = AddLofiBytesStereo;
  AddLofiWordMonoPtr     = AddLofiWordMono;
  AddLofiWordStereoPtr   = AddLofiWordStereo;
  AddLofiWordsMonoPtr    = AddLofiWordsMono;
  AddLofiWordsStereoPtr  = AddLofiWordsStereo;
  AddLofiByteMonoBPtr    = AddLofiByteMonoB;
  AddLofiByteStereoBPtr  = AddLofiByteStereoB;
  AddLofiBytesMonoBPtr   = AddLofiBytesMonoB;
  AddLofiBytesStereoBPtr = AddLofiBytesStereoB;
  AddLofiWordMonoBPtr    = AddLofiWordMonoB;
  AddLofiWordStereoBPtr  = AddLofiWordStereoB;
  AddLofiWordsMonoBPtr   = AddLofiWordsMonoB;
  AddLofiWordsStereoBPtr = AddLofiWordsStereoB;

  /* MorphOS/PowerUp/WarpOS loading

     Strategy:

      1) If MorphOS is running, use it.
      2) If PowerUp is running, but not WarpUp, use the m68k core
      3) If neither of them are running, try WarpUp.

  */

  // Check if MorpOS/PowerUp/WarpUp is running.
  {
    struct Library* ppclib     = NULL;
    struct Library* powerpclib = NULL;

    Forbid();
    MorphOSRes  = FindResident( "MorphOS" );
    
    powerpclib = (struct Library *) FindName( &SysBase->LibList,
                                              "powerpc.library" );
    ppclib     = (struct Library *) FindName( &SysBase->LibList,
                                              "ppc.library" );

    Permit();

    if( MorphOSRes == NULL && ! ( ppclib != NULL && powerpclib == NULL ) )
    {
      // Open WarpUp (but not if MorphOS or PowerUp is active)

      PowerPCBase = OpenLibrary( "powerpc.library", 15 );
    }
  }

  if( MorphOSRes != NULL )
  {
    MixBackend  = MB_MORPHOS;
  }
  else if( PowerPCBase != NULL )
  {
    MixBackend = MB_WARPUP;

    /* Load our code to PPC..  */

    PPCObject = AHILoadObject( "DEVS:ahi.device.elf" );

    if( PPCObject != NULL )
    {
      ULONG* version = NULL;
      ULONG* revision = NULL;

      int r = ~0;

      AHIGetELFSymbol( "__LIB_Version", (void*) &version );
      AHIGetELFSymbol( "__LIB_Revision", (void*) &revision );
    
      if( version == NULL || revision == NULL )
      {
        Req( "Unable to fetch version information from 'ahi.device.elf'." );
      }
      if( *version != Version || *revision != Revision )
      {
        Req( "'ahi.device.elf' version %ld.%ld doesn't match "
             "'ahi.device' version %ld.%ld.",
             *version, *revision, Version, Revision );

        return FALSE;
      }

      r &= GetSymbol( AddByteMono     );
      r &= GetSymbol( AddByteStereo   );
      r &= GetSymbol( AddBytesMono    );
      r &= GetSymbol( AddBytesStereo  );
      r &= GetSymbol( AddWordMono     );
      r &= GetSymbol( AddWordStereo   );
      r &= GetSymbol( AddWordsMono    );
      r &= GetSymbol( AddWordsStereo  );
      r &= GetSymbol( AddByteMonoB    );
      r &= GetSymbol( AddByteStereoB  );
      r &= GetSymbol( AddBytesMonoB   );
      r &= GetSymbol( AddBytesStereoB );
      r &= GetSymbol( AddWordMonoB    );
      r &= GetSymbol( AddWordStereoB  );
      r &= GetSymbol( AddWordsMonoB   );
      r &= GetSymbol( AddWordsStereoB );

      r &= GetSymbol( AddLofiByteMono     );
      r &= GetSymbol( AddLofiByteStereo   );
      r &= GetSymbol( AddLofiBytesMono    );
      r &= GetSymbol( AddLofiBytesStereo  );
      r &= GetSymbol( AddLofiWordMono     );
      r &= GetSymbol( AddLofiWordStereo   );
      r &= GetSymbol( AddLofiWordsMono    );
      r &= GetSymbol( AddLofiWordsStereo  );
      r &= GetSymbol( AddLofiByteMonoB    );
      r &= GetSymbol( AddLofiByteStereoB  );
      r &= GetSymbol( AddLofiBytesMonoB   );
      r &= GetSymbol( AddLofiBytesStereoB );
      r &= GetSymbol( AddLofiWordMonoB    );
      r &= GetSymbol( AddLofiWordStereoB  );
      r &= GetSymbol( AddLofiWordsMonoB   );
      r &= GetSymbol( AddLofiWordsStereoB );

      if( r == 0 )
      {
        Req( "Unable to fetch all symbols from ELF object." );
        return FALSE;
      }
    }
    else
    {
      MixBackend = MB_NATIVE;
    }
  }
  else 
  {
    MixBackend = MB_NATIVE;
  }

  OpenahiCatalog(NULL, NULL);

  {
    char buf[2];
    
    if( GetVar( "AHINoBetaRequester", buf, sizeof( buf ), 0 ) == -1 )
    {
      const char* backend = "Internal error";

      switch( MixBackend )
      {
        case MB_NATIVE:
          backend = "AmigaOS/" CPU;
          break;

        case MB_WARPUP:
          backend = "WarpUp";
          break;
          
        case MB_MORPHOS:
          backend = "MorphOS/" CPU;
          break;
      }

      Req( "This is a beta release of AHI. The latest supported \n"
           "version is 4.180, which can be found at\n"
           "<URL:http://www.lysator.liu.se/~lcs/ahi.html>.\n"
           "\n"
           "Detailed bug reports and patches are welcome.\n"
           "Sound kernel in use: %s.\n"
           "\n"
           "/Martin Blom <martin@blom.org>\n",
           backend );
    }
  }

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

  if( PPCObject != NULL )
  {
    AHIUnloadObject( PPCObject );
  }

  CloseLibrary( PowerPCBase );

  CloseLibrary( (struct Library *) UtilityBase );
  if( TimerIO  != NULL )
  {
    CloseDevice( (struct IORequest *) TimerIO );
  }
  FreeVec( timeval );
  FreeVec( TimerIO );
  CloseLibrary( (struct Library *) LocaleBase );
  CloseLibrary( (struct Library *) IntuitionBase );
  CloseLibrary( IFFParseBase );
  CloseLibrary( GadToolsBase );
  CloseLibrary( (struct Library *) GfxBase );
  CloseLibrary( (struct Library *) DOSBase );
}
