/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2003 Martin Blom <martin@blom.org>
     
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

#include <exec/memory.h>
#include <exec/resident.h>
#include <exec/alerts.h>
#include <exec/execbase.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include <proto/iffparse.h>
#include "ahi_def.h"

#include "header.h"
#include "gateway.h"
#include "gatestubs.h"
#include "localize.h"
#include "misc.h"
#include "version.h"
#ifdef __AMIGAOS4__
#include "devcommands.h"
#include "device.h"
#endif

static BOOL
OpenLibs ( void );

static void
CloseLibs ( void );

#define GetSymbol( name ) AHIGetELFSymbol( #name, (void*) &name ## Ptr )

/******************************************************************************
** Device entry ***************************************************************
******************************************************************************/

#if defined( __amithlon__ )
__asm( "\
         .text;\
         .byte 0x4e, 0xfa, 0x00, 0x03\
         jmp _start" );
#endif

int
_start( void )
{
  return -1;
}

#if defined( __MORPHOS__ )
ULONG   __abox__=1;
ULONG   __amigappc__=1;  // deprecated, used in MOS 0.4
#endif

/******************************************************************************
** Device resident structure **************************************************
******************************************************************************/

extern const char DevName[];
extern const char IDString[];
static const APTR InitTable[4];

#if defined( __AMIGAOS4__  )
static struct TagItem libCreateTags[];
#endif

// This structure must reside in the text segment or the read-only data segment!
// "const" makes it happen.
static const struct Resident RomTag =
{
  RTC_MATCHWORD,
  (struct Resident *) &RomTag,
  (struct Resident *) &RomTag + 1,
#if defined( __MORPHOS__ ) 
  RTF_EXTENDED | RTF_PPC | RTF_AUTOINIT,
#elif defined( __amithlon__ )
  RTF_PPC | RTF_AUTOINIT,
#elif defined( __AMIGAOS4__ )
  RTF_NATIVE | RTF_AUTOINIT,
#else
  RTF_AUTOINIT,
#endif
  VERSION,
  NT_DEVICE,
  0,                      /* priority */
  (BYTE *) DevName,
  (BYTE *) IDString,
#if defined( __AMIGAOS4__ )
  libCreateTags
#else
  (APTR) &InitTable
# if defined( __MORPHOS__ )
  , REVISION, NULL
# endif
#endif
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

#if defined( __AMIGAOS4__ )
struct ExecIFace          *IExec          = NULL;
struct AHIIFace           *IAHI           = NULL;
struct DOSIFace           *IDOS           = NULL;
struct GadToolsIFace      *IGadTools      = NULL;
struct GraphicsIFace      *IGraphics      = NULL;
struct IFFParseIFace      *IIFFParse      = NULL;
struct IntuitionIFace     *IIntuition     = NULL;
struct LocaleIFace        *ILocale        = NULL;
struct TimerIFace         *ITimer         = NULL;
struct UtilityIFace       *IUtility       = NULL;
struct AHIsubIFace        *IAHIsub        = NULL;
#endif

struct Resident           *MorphOSRes     = NULL;
static struct timerequest *TimerIO        = NULL;

#if defined( ENABLE_WARPUP )
struct Library            *PowerPCBase    = NULL;
void                      *PPCObject      = NULL;
#endif

ADDFUNC* AddByteMonoPtr                   = NULL;
ADDFUNC* AddByteStereoPtr                 = NULL;
ADDFUNC* AddByte71Ptr                     = NULL;
ADDFUNC* AddBytesMonoPtr                  = NULL;
ADDFUNC* AddBytesStereoPtr                = NULL;
ADDFUNC* AddBytes71Ptr                    = NULL;
ADDFUNC* AddWordMonoPtr                   = NULL;
ADDFUNC* AddWordStereoPtr                 = NULL;
ADDFUNC* AddWord71Ptr                     = NULL;
ADDFUNC* AddWordsMonoPtr                  = NULL;
ADDFUNC* AddWordsStereoPtr                = NULL;
ADDFUNC* AddWords71Ptr                    = NULL;
ADDFUNC* AddLongMonoPtr                   = NULL;
ADDFUNC* AddLongStereoPtr                 = NULL;
ADDFUNC* AddLong71Ptr                     = NULL;
ADDFUNC* AddLongsMonoPtr                  = NULL;
ADDFUNC* AddLongsStereoPtr                = NULL;
ADDFUNC* Add71MonoPtr                     = NULL;
ADDFUNC* Add71StereoPtr                   = NULL;
ADDFUNC* AddLongs71Ptr                    = NULL;
ADDFUNC* Add7171Ptr                       = NULL;
ADDFUNC* AddByteMonoBPtr                  = NULL;
ADDFUNC* AddByteStereoBPtr                = NULL;
ADDFUNC* AddByte71BPtr                    = NULL;
ADDFUNC* AddBytesMonoBPtr                 = NULL;
ADDFUNC* AddBytesStereoBPtr               = NULL;
ADDFUNC* AddBytes71BPtr                   = NULL;
ADDFUNC* AddWordMonoBPtr                  = NULL;
ADDFUNC* AddWordStereoBPtr                = NULL;
ADDFUNC* AddWord71BPtr                    = NULL;
ADDFUNC* AddWordsMonoBPtr                 = NULL;
ADDFUNC* AddWordsStereoBPtr               = NULL;
ADDFUNC* AddWords71BPtr                   = NULL;
ADDFUNC* AddLongMonoBPtr                  = NULL;
ADDFUNC* AddLongStereoBPtr                = NULL;
ADDFUNC* AddLong71BPtr                    = NULL;
ADDFUNC* AddLongsMonoBPtr                 = NULL;
ADDFUNC* AddLongsStereoBPtr               = NULL;
ADDFUNC* AddLongs71BPtr                   = NULL;
ADDFUNC* Add71MonoBPtr                    = NULL;
ADDFUNC* Add71StereoBPtr                  = NULL;
ADDFUNC* Add7171BPtr                      = NULL;

ADDFUNC* AddLofiByteMonoPtr               = NULL;
ADDFUNC* AddLofiByteStereoPtr             = NULL;
ADDFUNC* AddLofiBytesMonoPtr              = NULL;
ADDFUNC* AddLofiBytesStereoPtr            = NULL;
ADDFUNC* AddLofiWordMonoPtr               = NULL;
ADDFUNC* AddLofiWordStereoPtr             = NULL;
ADDFUNC* AddLofiWordsMonoPtr              = NULL;
ADDFUNC* AddLofiWordsStereoPtr            = NULL;
ADDFUNC* AddLofiLongMonoPtr               = NULL;
ADDFUNC* AddLofiLongStereoPtr             = NULL;
ADDFUNC* AddLofiLongsMonoPtr              = NULL;
ADDFUNC* AddLofiLongsStereoPtr            = NULL;
ADDFUNC* AddLofiByteMonoBPtr              = NULL;
ADDFUNC* AddLofiByteStereoBPtr            = NULL;
ADDFUNC* AddLofiBytesMonoBPtr             = NULL;
ADDFUNC* AddLofiBytesStereoBPtr           = NULL;
ADDFUNC* AddLofiWordMonoBPtr              = NULL;
ADDFUNC* AddLofiWordStereoBPtr            = NULL;
ADDFUNC* AddLofiWordsMonoBPtr             = NULL;
ADDFUNC* AddLofiWordsStereoBPtr           = NULL;
ADDFUNC* AddLofiLongMonoBPtr              = NULL;
ADDFUNC* AddLofiLongStereoBPtr            = NULL;
ADDFUNC* AddLofiLongsMonoBPtr             = NULL;
ADDFUNC* AddLofiLongsStereoBPtr           = NULL;

const ULONG  DriverVersion  = 2;
const ULONG  Version        = VERSION;
const ULONG  Revision       = REVISION;

const ULONG	 __LIB_Version  = VERSION;
const ULONG	 __LIB_Revision = REVISION;

const char DevName[]   = AHINAME;
const char IDString[]  = AHINAME " " VERS "\r\n";

#ifndef __AMIGAOS4__
static const char VersTag[] =
 "$VER: " AHINAME " " VERS " �1994-2003 Martin Blom. "
 CPU 
 " version.\r\n";
#else
static const char VersTag[] =
 "$VER: " AHINAME " " VERS " �1994-2003 Martin Blom. "
 "603e" 
 " version.\r\n";
#endif

enum MixBackend_t          MixBackend     = MB_NATIVE;

/* linker can use symbol b for symbol a if a is not defined */
#define ALIAS(a,b) asm(".stabs \"_" #a "\",11,0,0,0\n.stabs \"_" #b "\",1,0,0,0")

ALIAS( __UtilityBase, UtilityBase );

/******************************************************************************
** Device code ****************************************************************
******************************************************************************/

struct AHIBase*
_DevInit( struct AHIBase*  device,
	  APTR             seglist,
	  struct ExecBase* sysbase )
{
  SysBase = sysbase;
  AHIBase = device;

#ifndef __AMIGAOS4__
  AHIBase->ahib_Library.lib_Node.ln_Type = NT_DEVICE;
  AHIBase->ahib_Library.lib_Node.ln_Name = (STRPTR) DevName;
  AHIBase->ahib_Library.lib_Flags        = LIBF_SUMUSED | LIBF_CHANGED;
  AHIBase->ahib_Library.lib_Version      = VERSION;
  AHIBase->ahib_Library.lib_IdString     = (STRPTR) IDString;
#endif
  AHIBase->ahib_Library.lib_Revision     = REVISION;
  
  AHIBase->ahib_SysLib  = sysbase;
  AHIBase->ahib_SegList = (BPTR) seglist;

#if defined( __mc68000__ )
  // Make sure we're running on a M68020 or better
  
  if( ( SysBase->AttnFlags & AFF_68020 ) == 0 )
  {
    Alert( ( AN_Unknown | ACPU_InstErr ) & (~AT_DeadEnd) );

    FreeMem( (APTR) ( ( (char*) device ) - device->ahib_Library.lib_NegSize ),
             device->ahib_Library.lib_NegSize + device->ahib_Library.lib_PosSize );
    return NULL;
  }
#endif

  InitSemaphore( &AHIBase->ahib_Lock );

  if( !OpenLibs() )
  {
    CloseLibs();
    FreeMem( (APTR) ( ( (char*) device ) - device->ahib_Library.lib_NegSize ),
             device->ahib_Library.lib_NegSize + device->ahib_Library.lib_PosSize );
    return NULL;
  }

  return AHIBase;
}


BPTR
_DevExpunge( struct AHIBase* device )
{
  BPTR seglist = 0;

   //DebugPrintF("AHI: _DevExpunge\n");

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
_DevNull( void ) {
  return 0;
}

static const APTR funcTable[] =
{

#if defined( __MORPHOS__ ) || defined( __amithlon__ )
  (APTR) FUNCARRAY_32BIT_NATIVE,
#endif

  gwDevOpen,
  gwDevClose,
  gwDevExpunge,
  gwDevNull,

  gwDevBeginIO,
  gwDevAbortIO,

  gwAHI_AllocAudioA,
  gwAHI_FreeAudio,
  gwAHI_KillAudio,
  gwAHI_ControlAudioA,
  gwAHI_SetVol,
  gwAHI_SetFreq,
  gwAHI_SetSound,
  gwAHI_SetEffect,
  gwAHI_LoadSound,
  gwAHI_UnloadSound,
  gwAHI_NextAudioID,
  gwAHI_GetAudioAttrsA,
  gwAHI_BestAudioIDA,
  gwAHI_AllocAudioRequestA,
  gwAHI_AudioRequestA,
  gwAHI_FreeAudioRequest,
  gwAHI_PlayA,
  gwAHI_SampleFrameSize,
  gwAHI_AddAudioMode,
  gwAHI_RemoveAudioMode,
  gwAHI_LoadModeFile,
  (APTR) -1
};


static const APTR InitTable[4] =
{
  (APTR) sizeof( struct AHIBase ),
  (APTR) &funcTable,
  0,
#if defined( __MORPHOS__ ) || defined( __amithlon__ )
  (APTR) _DevInit
#else
  (APTR) gwDevInit
#endif
};


#ifdef __AMIGAOS4__
struct AHIBase *dev_init(struct AHIBase *dev, APTR seglist, struct ExecIFace *exec)
{
    IExec = exec;
    SysBase = (struct ExecBase *)exec->Data.LibBase;
    
    return _DevInit( dev, seglist, SysBase);
}

VOID dev_begin_io(struct DeviceManagerInterface *Self, struct IORequest *ior)
{
    _DevBeginIO((struct AHIRequest*) ior, (struct AHIBase*) Self->Data.LibBase);
}

LONG dev_abort_io(struct DeviceManagerInterface *Self, struct IORequest *ior)
{
    return _DevAbortIO((struct AHIRequest*) ior, (struct AHIBase*) Self->Data.LibBase);
}

LONG dev_open(struct DeviceManagerInterface *Self,
              struct IORequest *ior,
              ULONG unit,
              ULONG flags)
{
    return _DevOpen((struct AHIRequest *) ior, unit, flags, (struct AHIBase*) Self->Data.LibBase);
}

APTR dev_close(struct DeviceManagerInterface *Self, struct IORequest *ior)
{
    return (APTR) _DevClose ((struct AHIRequest*) ior, (struct AHIBase*) Self->Data.LibBase);

}

APTR dev_expunge(struct DeviceManagerInterface *Self)
{
    return (APTR) _DevExpunge((struct AHIBase*) Self->Data.LibBase);
}

ULONG generic_Obtain (struct Interface *Self)
{
	return Self->Data.RefCount++;
}

ULONG generic_Release (struct Interface *Self)
{
	return Self->Data.RefCount--;
}

static void *dev_manager_vectors[] =
{
	(void *)generic_Obtain,
	(void *)generic_Release,
	(void *)NULL,
	(void *)NULL,
	(void *)dev_open,
	(void *)dev_close,
	(void *)dev_expunge,
	(void *)NULL,
	(void *)dev_begin_io,
	(void *)dev_abort_io,
	(void *)-1,
};

static struct TagItem devmanagerTags[] = 
{
	{MIT_Name,             (ULONG)"__device"},
	{MIT_VectorTable,      (ULONG)dev_manager_vectors},
	{MIT_Version,          1},
	{TAG_DONE,             0}
};

static void *main_vectors[] = {
	(void *)generic_Obtain,
	(void *)generic_Release,
	(void *)NULL,
	(void *)NULL,
	(void *)gwAHI_AllocAudioA,
	(void *)gwAHI_AllocAudio,
	(void *)gwAHI_FreeAudio,
	(void *)gwAHI_KillAudio,
	(void *)gwAHI_ControlAudioA,
	(void *)gwAHI_ControlAudio,
	(void *)gwAHI_SetVol,
	(void *)gwAHI_SetFreq,
	(void *)gwAHI_SetSound,
	(void *)gwAHI_SetEffect,
	(void *)gwAHI_LoadSound,
	(void *)gwAHI_UnloadSound,
	(void *)gwAHI_NextAudioID,
	(void *)gwAHI_GetAudioAttrsA,
	(void *)gwAHI_GetAudioAttrs,
	(void *)gwAHI_BestAudioIDA,
	(void *)gwAHI_BestAudioID,
	(void *)gwAHI_AllocAudioRequestA,
	(void *)gwAHI_AllocAudioRequest,
	(void *)gwAHI_AudioRequestA,
	(void *)gwAHI_AudioRequest,
	(void *)gwAHI_FreeAudioRequest,
	(void *)gwAHI_PlayA,
	(void *)gwAHI_Play,
	(void *)gwAHI_SampleFrameSize,
	(void *)gwAHI_AddAudioMode,
	(void *)gwAHI_RemoveAudioMode,
	(void *)gwAHI_LoadModeFile,
	(void *)-1
};

static struct TagItem mainTags[] =
{
	{MIT_Name,              (ULONG)"main"},
	{MIT_VectorTable,       (ULONG)main_vectors},
	{MIT_Version,           1},
	{TAG_DONE,              0}
};

/* MLT_INTERFACES array */

static ULONG devInterfaces[] =
{
	(ULONG)devmanagerTags,
	(ULONG)mainTags,
	0
};

extern ULONG VecTable68K;
// tbd extern

/* CreateLibrary tag list */
static struct TagItem libCreateTags[] =
{
	{CLT_DataSize,         (ULONG)sizeof(struct AHIBase)},
	{CLT_Interfaces,       (ULONG)devInterfaces},
	{CLT_Vector68K,        (ULONG)&VecTable68K},
	{CLT_InitFunc,         (ULONG)dev_init},
	{TAG_DONE,             0}
};
#endif

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

  if( OpenDevice( "timer.device",
                  UNIT_VBLANK,
                  (struct IORequest *)
                  TimerIO,
                  0) != 0 )
  {
    Req( "Unable to open 'timer.device'." );
//    return FALSE; 
  }
  else
  {
    TimerBase = (struct Device *) TimerIO->tr_node.io_Device;
  }

  /* Utility Library */

  UtilityBase = (struct UtilityBase *) OpenLibrary( "utility.library", 37 );

  if( UtilityBase == NULL)
  {
    Req( "Unable to open 'utility.library'." );
    return FALSE;
  }


#ifdef __AMIGAOS4__
  if ((IIntuition = (struct IntuitionIFace *) GetInterface((struct Library *) IntuitionBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IIntuition interface!\n");
       return FALSE;
  }


  if ((IDOS = (struct DOSIFace *) GetInterface((struct Library *) DOSBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IDOS interface!\n");
       return FALSE;
  }


  if ((IGraphics = (struct GraphicsIFace *) GetInterface((struct Library *) GfxBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open Graphics interface!\n");
       return FALSE;
  }
  

  if ((IGadTools = (struct GadToolsIFace *) GetInterface((struct Library *) GadToolsBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IGadTools interface!\n");
       return FALSE;
  }


  if ((IIFFParse = (struct IFFParseIFace *) GetInterface((struct Library *) IFFParseBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open IFFParse interface!\n");
       return FALSE;
  }

  
  if ((ILocale = (struct LocaleIFace *) GetInterface((struct Library *) LocaleBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open ILocale interface!\n");
       return FALSE;
  }

  if ((ITimer = (struct TimerIFace *) GetInterface((struct Library *) TimerBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open Timer interface!\n");
       return FALSE;
  }
  
  if ((IUtility = (struct UtilityIFace *) GetInterface((struct Library *) UtilityBase, "main", 1, NULL)) == NULL)
  {
       Req("Couldn't open Utility interface!\n");
       return FALSE;
  }

#endif

  // Fill in some defaults...

  AddByteMonoPtr         = AddByteMono;
  AddByteStereoPtr       = AddByteStereo;
  AddByte71Ptr           = AddByte71;
  AddBytesMonoPtr        = AddBytesMono;
  AddBytesStereoPtr      = AddBytesStereo;
  AddBytes71Ptr          = AddBytes71;
  AddWordMonoPtr         = AddWordMono;
  AddWordStereoPtr       = AddWordStereo;
  AddWord71Ptr           = AddWord71;
  AddWordsMonoPtr        = AddWordsMono;
  AddWordsStereoPtr      = AddWordsStereo;
  AddWords71Ptr          = AddWords71;
  AddLongMonoPtr         = AddLongMono;
  AddLongStereoPtr       = AddLongStereo;
  AddLong71Ptr           = AddLong71;
  AddLongsMonoPtr        = AddLongsMono;
  AddLongsStereoPtr      = AddLongsStereo;
  Add71MonoPtr           = Add71Mono;
  Add71StereoPtr         = Add71Stereo;
  AddLongs71Ptr          = AddLongs71;
  Add7171Ptr             = Add7171;
 
  AddByteMonoBPtr        = AddByteMonoB;
  AddByteStereoBPtr      = AddByteStereoB;
  AddByte71BPtr          = AddByte71B;
  AddBytesMonoBPtr       = AddBytesMonoB;
  AddBytesStereoBPtr     = AddBytesStereoB;
  AddBytes71BPtr         = AddBytes71B;
  AddWordMonoBPtr        = AddWordMonoB;
  AddWordStereoBPtr      = AddWordStereoB;
  AddWord71BPtr          = AddWord71B;
  AddWordsMonoBPtr       = AddWordsMonoB;
  AddWordsStereoBPtr     = AddWordsStereoB;
  AddWords71BPtr         = AddWords71B;
  AddLongMonoBPtr        = AddLongMonoB;
  AddLongStereoBPtr      = AddLongStereoB;
  AddLong71BPtr          = AddLong71B;
  AddLongsMonoBPtr       = AddLongsMonoB;
  AddLongsStereoBPtr     = AddLongsStereoB;
  AddLongs71BPtr         = AddLongs71B;
  Add71MonoBPtr          = Add71MonoB;
  Add71StereoBPtr        = Add71StereoB;
  Add7171BPtr            = Add7171B;

  AddLofiByteMonoPtr     = AddLofiByteMono;
  AddLofiByteStereoPtr   = AddLofiByteStereo;
  AddLofiBytesMonoPtr    = AddLofiBytesMono;
  AddLofiBytesStereoPtr  = AddLofiBytesStereo;
  AddLofiWordMonoPtr     = AddLofiWordMono;
  AddLofiWordStereoPtr   = AddLofiWordStereo;
  AddLofiWordsMonoPtr    = AddLofiWordsMono;
  AddLofiWordsStereoPtr  = AddLofiWordsStereo;
  AddLofiLongMonoPtr     = AddLofiLongMono;
  AddLofiLongStereoPtr   = AddLofiLongStereo;
  AddLofiLongsMonoPtr    = AddLofiLongsMono;
  AddLofiLongsStereoPtr  = AddLofiLongsStereo;
  AddLofiByteMonoBPtr    = AddLofiByteMonoB;
  AddLofiByteStereoBPtr  = AddLofiByteStereoB;
  AddLofiBytesMonoBPtr   = AddLofiBytesMonoB;
  AddLofiBytesStereoBPtr = AddLofiBytesStereoB;
  AddLofiWordMonoBPtr    = AddLofiWordMonoB;
  AddLofiWordStereoBPtr  = AddLofiWordStereoB;
  AddLofiWordsMonoBPtr   = AddLofiWordsMonoB;
  AddLofiWordsStereoBPtr = AddLofiWordsStereoB;
  AddLofiLongMonoBPtr    = AddLofiLongMonoB;
  AddLofiLongStereoBPtr  = AddLofiLongStereoB;
  AddLofiLongsMonoBPtr   = AddLofiLongsMonoB;
  AddLofiLongsStereoBPtr = AddLofiLongsStereoB;

  /* MorphOS/PowerUp/WarpOS loading

     Strategy:

      1) If MorphOS is running, use it.
      2) If PowerUp is running, but not WarpUp, use the m68k core
      3) If neither of them are running, try WarpUp.

  */

  // Check if MorpOS/PowerUp/WarpUp is running.
  {
#if defined( ENABLE_WARPUP )
    struct Library* ppclib     = NULL;
    struct Library* powerpclib = NULL;
#endif

    Forbid();
    MorphOSRes  = FindResident( "MorphOS" );
    
#if defined( ENABLE_WARPUP )
    powerpclib = (struct Library *) FindName( &SysBase->LibList,
                                              "powerpc.library" );
    ppclib     = (struct Library *) FindName( &SysBase->LibList,
                                              "ppc.library" );
#endif

    Permit();

#if defined( ENABLE_WARPUP )
    if( MorphOSRes == NULL && ! ( ppclib != NULL && powerpclib == NULL ) )
    {
      // Open WarpUp (but not if MorphOS or PowerUp is active)

      PowerPCBase = OpenLibrary( "powerpc.library", 15 );
    }
#endif
  }

  if( MorphOSRes != NULL )
  {
    MixBackend  = MB_NATIVE;
  }

#if defined( ENABLE_WARPUP )

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
    
      if( version != NULL && revision != NULL )
      {
        if( *version == Version && *revision == Revision )
        {
          r &= GetSymbol( AddByteMono     );
          r &= GetSymbol( AddByteStereo   );
          r &= GetSymbol( AddBytesMono    );
          r &= GetSymbol( AddBytesStereo  );
          r &= GetSymbol( AddWordMono     );
          r &= GetSymbol( AddWordStereo   );
          r &= GetSymbol( AddWordsMono    );
          r &= GetSymbol( AddWordsStereo  );
          r &= GetSymbol( AddLongMono     );
          r &= GetSymbol( AddLongStereo   );
          r &= GetSymbol( AddLongsMono    );
          r &= GetSymbol( AddLongsStereo  );
          r &= GetSymbol( AddByteMonoB    );
          r &= GetSymbol( AddByteStereoB  );
          r &= GetSymbol( AddBytesMonoB   );
          r &= GetSymbol( AddBytesStereoB );
          r &= GetSymbol( AddWordMonoB    );
          r &= GetSymbol( AddWordStereoB  );
          r &= GetSymbol( AddWordsMonoB   );
          r &= GetSymbol( AddWordsStereoB );
          r &= GetSymbol( AddLongMonoB    );
          r &= GetSymbol( AddLongStereoB  );
          r &= GetSymbol( AddLongsMonoB   );
          r &= GetSymbol( AddLongsStereoB );

          r &= GetSymbol( AddLofiByteMono     );
          r &= GetSymbol( AddLofiByteStereo   );
          r &= GetSymbol( AddLofiBytesMono    );
          r &= GetSymbol( AddLofiBytesStereo  );
          r &= GetSymbol( AddLofiWordMono     );
          r &= GetSymbol( AddLofiWordStereo   );
          r &= GetSymbol( AddLofiWordsMono    );
          r &= GetSymbol( AddLofiWordsStereo  );
          r &= GetSymbol( AddLofiLongMono     );
          r &= GetSymbol( AddLofiLongStereo   );
          r &= GetSymbol( AddLofiLongsMono    );
          r &= GetSymbol( AddLofiLongsStereo  );
          r &= GetSymbol( AddLofiByteMonoB    );
          r &= GetSymbol( AddLofiByteStereoB  );
          r &= GetSymbol( AddLofiBytesMonoB   );
          r &= GetSymbol( AddLofiBytesStereoB );
          r &= GetSymbol( AddLofiWordMonoB    );
          r &= GetSymbol( AddLofiWordStereoB  );
          r &= GetSymbol( AddLofiWordsMonoB   );
          r &= GetSymbol( AddLofiWordsStereoB );
          r &= GetSymbol( AddLofiLongMonoB    );
          r &= GetSymbol( AddLofiLongStereoB  );
          r &= GetSymbol( AddLofiLongsMonoB   );
          r &= GetSymbol( AddLofiLongsStereoB );

          if( r != 0 )
          {
            char buffer[ 2 ] = "0";
        
            GetVar( "PowerPC/UseDisable", buffer, sizeof buffer, 0 );
        
            if( buffer[ 0 ] == '1' )
            {
              // OK, then...
            }
            else
            {
              Req( "The WarpUp variable 'PowerPC/UseDisable' must be '1'." );

              AHIUnloadObject( PPCObject );
              PPCObject  = NULL;
              MixBackend = MB_NATIVE;
            }
          }
          else
          {
            Req( "Unable to fetch all symbols from ELF object." );

            AHIUnloadObject( PPCObject );
            PPCObject  = NULL;
            MixBackend = MB_NATIVE;
          }
        }
        else
        {
          Req( "'ahi.device.elf' version %ld.%ld doesn't match "
               "'ahi.device' version %ld.%ld.",
               *version, *revision, Version, Revision );

          AHIUnloadObject( PPCObject );
          PPCObject  = NULL;
          MixBackend = MB_NATIVE;
        }
      }
      else
      {
        Req( "Unable to fetch version information from 'ahi.device.elf'." );

        AHIUnloadObject( PPCObject );
        PPCObject  = NULL;
        MixBackend = MB_NATIVE;
      }
    }
    else
    {
      MixBackend = MB_NATIVE;
    }
  }

#endif

  else 
  {
    MixBackend = MB_NATIVE;
  }

  OpenahiCatalog(NULL, NULL);

/* #if defined( __amithlon__ ) */
/*   Req( "This is a *beta* release of AHI/x86,\n" */
/*        "using the generic 'C' mixing routines.\n" */
/*        "\n" */
/*        "Detailed bug reports and patches are welcome.\n" */
/* 	 "/Martin Blom <martin@blom.org>\n" ); */
/* #endif */

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

#if defined( ENABLE_WARPUP )
  if( PPCObject != NULL )
  {
    AHIUnloadObject( PPCObject );
  }

  CloseLibrary( PowerPCBase );
#endif

  CloseLibrary( (struct Library *) UtilityBase );
  if( TimerIO  != NULL )
  {
    CloseDevice( (struct IORequest *) TimerIO );
  }
  FreeVec( TimerIO );


#ifdef __AMIGAOS4__
  if (IIntuition)
  {
       DropInterface((struct Interface *) IIntuition );
  }


  if (IDOS)
  {
       DropInterface((struct Interface *) IDOS );
  }


  if (IGraphics)
  {
       DropInterface((struct Interface *) IGraphics );
  }
  

  if (IGadTools)
  {
       DropInterface((struct Interface *) IGadTools );
  }


  if (IIFFParse)
  {
       DropInterface((struct Interface *) IIFFParse );
  }

  
  if (ILocale)
  {
       DropInterface((struct Interface *) ILocale );
  }

  if (ITimer)
  {
       DropInterface((struct Interface *) ITimer );
  }
  
  if (IUtility)
  {
       DropInterface((struct Interface *) IUtility );
  }
#endif


  CloseLibrary( (struct Library *) LocaleBase );
  CloseLibrary( (struct Library *) IntuitionBase );
  CloseLibrary( IFFParseBase );
  CloseLibrary( GadToolsBase );
  CloseLibrary( (struct Library *) GfxBase );
  CloseLibrary( (struct Library *) DOSBase );
}
