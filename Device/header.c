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

#if defined( morphos )
# include <exec/types.h>
# include <emul/emulregs.h>
#endif

#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <powerup/ppclib/object.h>
#include <powerup/ppclib/interface.h>

#include "ahi_def.h"

#include "addroutines.h"
#include "audioctrl.h"
#include "database.h"
#include "devcommands.h"
#include "device.h"
#include "header.h"
#include "localize.h"
#include "misc.h"
#include "modeinfo.h"
#include "requester.h"
#include "sound.h"

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
struct Resident           *MorphOSRes     = NULL;
struct Library            *PowerPCBase    = NULL;
struct Library            *PPCLibBase     = NULL;
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

static struct AHIBase*
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


ULONG
Null( void )
{
  return 0;
}

/******************************************************************************
** Entry gateway functions ****************************************************
******************************************************************************/

#if defined( morphos )

/* MorphOS *******************************************************************/

static struct AHIBase *
gw_initRoutine( void )
{
  struct AHIBase*  device  = (struct AHIBase*)  REG_D0;
  APTR             seglist = (APTR)             REG_A0;
  struct ExecBase* sysbase = (struct ExecBase*) REG_A6;

  return initRoutine( device, seglist, sysbase );
}

BPTR
DevExpunge( void )
{
  struct AHIBase* device = (struct AHIBase*) REG_A6;

  return initRoutine( device, seglist, sysbase );
}

#else

/* AmigaOS *******************************************************************/

static struct AHIBase * ASMCALL
gw_initRoutine( REG( d0, struct AHIBase*  device ),
                REG( a0, APTR             seglist ),
                REG( a6, struct ExecBase* sysbase ) )
{
  return initRoutine( device, seglist, sysbase );
}


BPTR ASMCALL
gw_DevExpunge( REG( a6, struct AHIBase* device ) )
{
  return DevExpunge( device );
}

ULONG ASMCALL
gw_DevOpen( REG( d0, ULONG              unit ),
            REG( d1, ULONG              flags ),
            REG( a1, struct AHIRequest* ioreq ),
            REG( a6, struct AHIBase*    AHIBase ) )
{
  return DevOpen( unit, flags, ioreq, AHIBase );
}


BPTR ASMCALL
gw_DevClose( REG( a1, struct AHIRequest* ioreq ),
             REG( a6, struct AHIBase*    AHIBase ) )
{
  return DevClose( ioreq, AHIBase );
}


void ASMCALL
gw_DevBeginIO( REG( a1, struct AHIRequest* ioreq ),
               REG( a6, struct AHIBase*    AHIBase ) )
{
  DevBeginIO( ioreq, AHIBase );
}


ULONG ASMCALL
gw_DevAbortIO( REG( a1, struct AHIRequest* ioreq ),
               REG( a6, struct AHIBase*    AHIBase ) )
{
  return DevAbortIO( ioreq, AHIBase );
}


struct AHIAudioCtrl * ASMCALL
gw_AllocAudioA( REG(a1, struct TagItem* tags),
                REG(a6, struct AHIBase* AHIBase) )
{
  return AllocAudioA( tags, AHIBase );
}


ULONG ASMCALL
gw_FreeAudio( REG(a2, struct AHIPrivAudioCtrl* audioctrl),
              REG(a6, struct AHIBase*          AHIBase) )
{
  return FreeAudio( audioctrl, AHIBase );
}


ULONG ASMCALL
gw_KillAudio( REG(a6, struct AHIBase* AHIBase) )
{
  return KillAudio( AHIBase );
}


ULONG ASMCALL
gw_ControlAudioA( REG(a2, struct AHIPrivAudioCtrl* audioctrl),
                  REG(a1, struct TagItem*          tags),
                  REG(a6, struct AHIBase*          AHIBase) )
{
  return ControlAudioA( audioctrl, tags, AHIBase );
}


ULONG ASMCALL
gw_SetVol( REG(d0, UWORD                    channel),
           REG(d1, Fixed                    volume),
           REG(d2, sposition                pan),
           REG(a2, struct AHIPrivAudioCtrl* audioctrl),
           REG(d3, ULONG                    flags),
           REG(a6, struct AHIBase*          AHIBase) )
{
  return SetVol( channel, volume, pan, audioctrl, flags, AHIBase );
}


ULONG ASMCALL
gw_SetFreq( REG( d0, UWORD                    channel ),
            REG( d1, ULONG                    freq ),
            REG( a2, struct AHIPrivAudioCtrl* audioctrl ),
            REG( d2, ULONG                    flags ),
            REG( a6, struct AHIBase*          AHIBase ) )
{
  return SetFreq( channel, freq, audioctrl, flags, AHIBase );
}


ULONG ASMCALL
gw_SetSound( REG(d0, UWORD                    channel),
             REG(d1, UWORD                    sound),
             REG(d2, ULONG                    offset),
             REG(d3, LONG                     length),
             REG(a2, struct AHIPrivAudioCtrl* audioctrl),
             REG(d4, ULONG                    flags),
             REG(a6, struct AHIBase*          AHIBase) )
{
  return SetSound( channel, sound, offset, length, audioctrl, flags, AHIBase );
}


ULONG ASMCALL
gw_SetEffect( REG(a0, ULONG*                   effect),
              REG(a2, struct AHIPrivAudioCtrl* audioctrl),
              REG(a6, struct AHIBase*          AHIBase) )
{
  return SetEffect( effect, audioctrl, AHIBase );
}


ULONG ASMCALL
gw_LoadSound( REG(d0, UWORD                    sound),
              REG(d1, ULONG                    type),
              REG(a0, APTR                     info),
              REG(a2, struct AHIPrivAudioCtrl* audioctrl),
              REG(a6, struct AHIBase*          AHIBase) )
{
  return LoadSound( sound, type, info, audioctrl, AHIBase );
}


ULONG ASMCALL
gw_UnloadSound( REG(d0, UWORD                    sound),
                REG(a2, struct AHIPrivAudioCtrl* audioctrl),
                REG(a6, struct AHIBase*          AHIBase) )
{
  return UnloadSound( sound, audioctrl, AHIBase );
}


ULONG ASMCALL 
gw_PlayA( REG(a2, struct AHIAudioCtrl* audioctrl),
          REG(a1, struct TagItem*      tags),
          REG(a6, struct AHIBase*      AHIBase) )
{
  return PlayA( audioctrl, tags, AHIBase );
}


ULONG ASMCALL
gw_SampleFrameSize( REG(d0, ULONG           sampletype),
                    REG(a6, struct AHIBase* AHIBase) )
{
  return SampleFrameSize( sampletype, AHIBase );
}


ULONG ASMCALL 
gw_GetAudioAttrsA( REG(d0, ULONG                   id),
                   REG(a2, struct AHIAudioCtrlDrv* actrl),
                   REG(a1, struct TagItem*         tags),
                   REG(a6, struct AHIBase*         AHIBase) )
{
  return GetAudioAttrsA( id, actrl, tags, AHIBase );
}


ULONG ASMCALL 
gw_BestAudioIDA( REG(a1, struct TagItem* tags),
                 REG(a6, struct AHIBase* AHIBase) )
{
  return BestAudioIDA( tags, AHIBase );
}


struct AHIAudioModeRequester* ASMCALL
gw_AllocAudioRequestA( REG(a0, struct TagItem* tags),
                       REG(a6, struct AHIBase* AHIBase) )
{
  return AllocAudioRequestA( tags, AHIBase );
}


ULONG ASMCALL 
gw_AudioRequestA( REG(a0, struct AHIAudioModeRequester* req_in),
                  REG(a1, struct TagItem*               tags ),
                  REG(a6, struct AHIBase*               AHIBase) )
{
  return AudioRequestA( req_in, tags, AHIBase );
}


void ASMCALL 
gw_FreeAudioRequest( REG(a0, struct AHIAudioModeRequester* req),
                     REG(a6, struct AHIBase*               AHIBase) )
{
  return FreeAudioRequest( req, AHIBase );
}


ULONG ASMCALL
gw_NextAudioID( REG(d0, ULONG           id),
                REG(a6, struct AHIBase* AHIBase) )
{
  return NextAudioID( id, AHIBase );
}


ULONG ASMCALL
gw_AddAudioMode( REG(a0, struct TagItem* DBtags),
                 REG(a6, struct AHIBase* AHIBase) )
{
  return AddAudioMode( DBtags, AHIBase );
}


ULONG ASMCALL
gw_RemoveAudioMode( REG(d0, ULONG           id),
                    REG(a6, struct AHIBase* AHIBase) )
{
  return RemoveAudioMode( id, AHIBase );
}


ULONG ASMCALL
gw_LoadModeFile( REG(a0, UBYTE*          name),
                 REG(a6, struct AHIBase* AHIBase) )
{
  return LoadModeFile( name, AHIBase );
}



#endif

static const APTR funcTable[] =
{

#if defined( morphos )
  (APTR) FUNCARRAY_32BIT_NATIVE
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

  MixBackend = MB_NATIVE;

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
      2) If WarpUp is running, use it.
      3) If WarpUp is is not running, but PowerUp is, use it
      4) If neither of them are running, try WarpUp.
      5) Finally, try PowerUp.

     Result:

      If both kernels are running, WarpUp will be used, since the PowerUp
      kernel is the ppc.library emulation. (This is going to work until
      somebody writes a WarpUp emulation for ppc.library....)

      If only one kernel is already running, it will be used.

      WarpUp will be used if no kernel is loaded, and WarpUp exists, since
      WarpUp was selected for OS 3.5.  If WarpUp was not found, PowerUp
      will be used.

  */

  // Check if WarpUp or PowerUp are already installed...

  Forbid();
  MorphOSRes  = FindResident( "MorphOS" );

  PowerPCBase = (struct Library *) FindName( &SysBase->LibList,
                                             "powerpc.library" );
  PPCLibBase  = (struct Library *) FindName( &SysBase->LibList,
                                             "ppc.library" );
  Permit();

  if( PowerPCBase != NULL 
      || ( PowerPCBase == NULL && PPCLibBase == NULL ) )
  {
    // Open WarpUp
    PowerPCBase = OpenLibrary( "powerpc.library", 14 );
    PPCLibBase  = NULL;
  }

  if( PPCLibBase != NULL 
      || ( PPCLibBase == NULL && PowerPCBase == NULL ) )
  {
    // Open PoweUp
    PPCLibBase  = OpenLibrary( "ppc.library", 46 );
    PowerPCBase = NULL;
  }

  if( PPCLibBase != NULL 
      && PPCLibBase->lib_Version == 46 
      && PPCLibBase->lib_Revision < 24 )
  {
    Req( "Need at least version 46.24 of 'ppc.library'." );
    return FALSE;
  }

  if( MorphOSRes != NULL )
  {
    MixBackend  = MB_MORPHOS;
  }
  else if( PPCLibBase != NULL )
  {
    MixBackend  = MB_POWERUP;
  }
  else if( PowerPCBase != NULL )
  {
    MixBackend  = MB_WARPUP;
  }

  if( PPCLibBase != NULL || PowerPCBase != NULL )
  {
    ULONG* version = NULL;
    ULONG* revision = NULL;

    /* Load our code to PPC..  */

    PPCObject = AHILoadObject( "DEVS:ahi.elf" );

    if( PPCObject != NULL )
    {
      int r = ~0;

      AHIGetELFSymbol( "__LIB_Version", (void*) &version );
      AHIGetELFSymbol( "__LIB_Revision", (void*) &revision );
    
      if( version == NULL || revision == NULL )
      {
        Req( "Unable to fetch version information from 'ahi.elf'." );
      }
      if( *version != Version || *revision != Revision )
      {
        Req( "'ahi.elf' version %ld.%ld doesn't match 'ahi.device' %ld.%ld.",
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
      // PPC kernel found, but no ELF object. m68k mixing will be used.
    }
  }

  OpenahiCatalog(NULL, NULL);

  {
    char buf[2];
    
    if( GetVar( "AHINoBetaRequester", buf, sizeof( buf ), 0 ) == -1 )
    {
      Req( "This is a beta release of AHI. The latest supported \n"
           "version is 4.180, which can be found at\n"
           "<URL:http://www.lysator.liu.se/~lcs/ahi.html>.\n"
           "\n"
           "Detailed bug reports and patches are welcome.\n"
           "Sound kernel in use: %s.\n"
           "\n"
           "/Martin Blom <martin@blom.org>\n",
           PPCObject == NULL ? CPU :
             ( PPCLibBase == NULL ? "WarpUp" : 
               ( MorphOSRes == NULL ? "PowerUp" : "MorphOS" ) ) );
           
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

  CloseLibrary( PPCLibBase );
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
