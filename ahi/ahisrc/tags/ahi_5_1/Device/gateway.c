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


#if defined( ENABLE_MORPHOS )
# include <exec/types.h> 
# define EMUL_NOQUICKMODE
# include <emul/emulregs.h>
#endif

#include <proto/exec.h>

#include "ahi_def.h"

#include "audioctrl.h"
#include "database.h"
#include "devcommands.h"
#include "device.h"
#include "header.h"
#include "misc.h"
#include "mixer.h"
#include "modeinfo.h"
#include "requester.h"
#include "sound.h"


/*
 * All these functions are supposed to be called by the m68k "processor",
 * with the arguments in m68k registers d0-d7/a0-a6.
 * The functions relay each call to a standard C function.
 *
 */

#if defined( ENABLE_MORPHOS )

/******************************************************************************
** MorphOS gateway functions **************************************************
******************************************************************************/

/* gw_initRoutine ************************************************************/

struct AHIBase*
gw_initRoutine( struct AHIBase*  device,
                APTR             seglist,
                struct ExecBase* sysbase )
{
  return initRoutine( device, seglist, sysbase );
}


/* gw_DevExpunge *************************************************************/

BPTR 
gw_DevExpunge( void )
{
  struct AHIBase* device = (struct AHIBase*) REG_A6;

  return DevExpunge( device );
}


/* gw_DevOpen ****************************************************************/

ULONG 
gw_DevOpen( void )
{
  ULONG              unit    = (ULONG)              REG_D0;
  ULONG              flags   = (ULONG)              REG_D1;
  struct AHIRequest* ioreq   = (struct AHIRequest*) REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)    REG_A6;

  return DevOpen( unit, flags, ioreq, AHIBase );
}


/* gw_DevClose ***************************************************************/

BPTR
gw_DevClose( void )
{
  struct AHIRequest* ioreq   = (struct AHIRequest*)  REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)     REG_A6;

  return DevClose( ioreq, AHIBase );
}


/* gw_DevBeginIO *************************************************************/

void 
gw_DevBeginIO( void )
{
  struct AHIRequest* ioreq   = (struct AHIRequest*) REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)    REG_A6;

  DevBeginIO( ioreq, AHIBase );
}


/* gw_DevAbortIO *************************************************************/

ULONG 
gw_DevAbortIO( void )
{
  struct AHIRequest* ioreq   = (struct AHIRequest*) REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)    REG_A6;

  return DevAbortIO( ioreq, AHIBase );
}


/* gw_AllocAudioA ************************************************************/

struct AHIAudioCtrl* 
gw_AllocAudioA( void )
{
  struct TagItem* tags    = (struct TagItem*) REG_A1;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return AllocAudioA( tags, AHIBase );
}


/* gw_FreeAudio **************************************************************/

ULONG 
gw_FreeAudio( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return FreeAudio( audioctrl, AHIBase );
}


/* gw_KillAudio **************************************************************/

ULONG 
gw_KillAudio( void )
{
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return KillAudio( AHIBase );
}


/* gw_ControlAudioA **********************************************************/

ULONG 
gw_ControlAudioA( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct TagItem*          tags      = (struct TagItem*)          REG_A1;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return ControlAudioA( audioctrl, tags, AHIBase );
}


/* gw_SetVol *****************************************************************/

ULONG 
gw_SetVol( void )
{
  UWORD                    channel   = (UWORD)                    REG_D0;
  Fixed                    volume    = (Fixed)                    REG_D1;
  sposition                pan       = (sposition)                REG_D2;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  ULONG                    flags     = (ULONG)                    REG_D3;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return SetVol( channel, volume, pan, audioctrl, flags, AHIBase );
}


/* gw_SetFreq ****************************************************************/

ULONG 
gw_SetFreq( void )
{
  UWORD                    channel   = (UWORD)                    REG_D0;
  ULONG                    freq      = (ULONG)                    REG_D1;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  ULONG                    flags     = (ULONG)                    REG_D2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return SetFreq( channel, freq, audioctrl, flags, AHIBase );
}


/* gw_SetSound ***************************************************************/

ULONG 
gw_SetSound( void )
{
  UWORD                    channel   = (UWORD)                    REG_D0;
  UWORD                    sound     = (UWORD)                    REG_D1;
  ULONG                    offset    = (ULONG)                    REG_D2;
  LONG                     length    = (LONG)                     REG_D3;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  ULONG                    flags     = (ULONG)                    REG_D4;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return SetSound( channel, sound, offset, length, audioctrl, flags, AHIBase );
}


/* gw_SetEffect **************************************************************/

ULONG 
gw_SetEffect( void )
{
  ULONG*                   effect    = (ULONG*)                   REG_A0;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return SetEffect( effect, audioctrl, AHIBase );
}


/* gw_LoadSound **************************************************************/

ULONG 
gw_LoadSound( void )
{
  UWORD                    sound     = (UWORD)                    REG_D0;
  ULONG                    type      = (ULONG)                    REG_D1;
  APTR                     info      = (APTR)                     REG_A0;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return LoadSound( sound, type, info, audioctrl, AHIBase );
}


/* gw_UnloadSound ************************************************************/

ULONG 
gw_UnloadSound( void )
{
  UWORD                    sound     = (UWORD)                    REG_D0;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return UnloadSound( sound, audioctrl, AHIBase );
}


/* gw_PlayA ******************************************************************/

ULONG  
gw_PlayA( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct TagItem*          tags      = (struct TagItem*)          REG_A1;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return PlayA( audioctrl, tags, AHIBase );
}


/* gw_SampleFrameSize ********************************************************/

ULONG 
gw_SampleFrameSize( void )
{
  ULONG           sampletype = (ULONG)           REG_D0;
  struct AHIBase* AHIBase    = (struct AHIBase*) REG_A6;

  return SampleFrameSize( sampletype, AHIBase );
}


/* gw_GetAudioAttrsA *********************************************************/

ULONG  
gw_GetAudioAttrsA( void )
{
  ULONG                    id      = (ULONG)                    REG_D0;
  struct AHIPrivAudioCtrl* actrl   = (struct AHIPrivAudioCtrl*) REG_A2;
  struct TagItem*          tags    = (struct TagItem*)          REG_A1;
  struct AHIBase*          AHIBase = (struct AHIBase*)          REG_A6;

  return GetAudioAttrsA( id, actrl, tags, AHIBase );
}


/* gw_BestAudioIDA ***********************************************************/

ULONG  
gw_BestAudioIDA( void )
{
  struct TagItem* tags    = (struct TagItem*) REG_A1;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return BestAudioIDA( tags, AHIBase );
}


/* gw_AllocAudioRequestA *****************************************************/

struct AHIAudioModeRequester* 
gw_AllocAudioRequestA( void )
{
  struct TagItem* tags    = (struct TagItem*) REG_A0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return AllocAudioRequestA( tags, AHIBase );
}


/* gw_AudioRequestA **********************************************************/

ULONG  
gw_AudioRequestA( void )
{
  struct AHIAudioModeRequester* req_in  = (struct AHIAudioModeRequester*) REG_A0;
  struct TagItem*               tags    = (struct TagItem*)               REG_A1;
  struct AHIBase*               AHIBase = (struct AHIBase*)               REG_A6;

  return AudioRequestA( req_in, tags, AHIBase );
}


/* gw_FreeAudioRequest *******************************************************/

void  
gw_FreeAudioRequest( void )
{
  struct AHIAudioModeRequester* req     = (struct AHIAudioModeRequester*) REG_A0;
  struct AHIBase*               AHIBase = (struct AHIBase*)               REG_A6;

  FreeAudioRequest( req, AHIBase );
}


/* gw_NextAudioID ************************************************************/

ULONG 
gw_NextAudioID( void )
{
  ULONG           id      = (ULONG)           REG_D0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return NextAudioID( id, AHIBase );
}


/* gw_AddAudioMode ***********************************************************/

ULONG 
gw_AddAudioMode( void )
{
  struct TagItem* DBtags  = (struct TagItem*) REG_A0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return AddAudioMode( DBtags, AHIBase );
}


/* gw_RemoveAudioMode ********************************************************/

ULONG 
gw_RemoveAudioMode( void )
{
  ULONG           id      = (ULONG)           REG_D0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return RemoveAudioMode( id, AHIBase );
}


/* gw_LoadModeFile ***********************************************************/

ULONG 
gw_LoadModeFile( void )
{
  UBYTE*          name    = (UBYTE*)          REG_A0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return LoadModeFile( name, AHIBase );
}


/* m68k_IndexToFrequency *****************************************************/

static LONG gw_IndexToFrequency( void )
{
  struct Gadget* gad   = (struct Gadget*) ((ULONG*) REG_A7)[1];
  WORD           level = (WORD)           ((ULONG*) REG_A7)[2];

  return IndexToFrequency( gad , level );
}

struct EmulLibEntry m68k_IndexToFrequency =
{
  TRAP_LIB, 0, (void (*)(void)) gw_IndexToFrequency
};


/* m68k_DevProc **************************************************************/

struct EmulLibEntry m68k_DevProc =
{
  TRAP_LIB, 0, (void (*)(void)) DevProc
};


/* HookEntry *****************************************************************/

ULONG
HookEntry( struct Hook* h,
           void*        o, 
           void*        msg )
{
  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}


/* m68k_HookEntry ************************************************************/

static ULONG
gw_HookEntry( void )
{
  struct Hook* h   = (struct Hook*) REG_A0;
  void*        o   = (void*)        REG_A2; 
  void*        msg = (void*)        REG_A1;

  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}

struct EmulLibEntry m68k_HookEntry =
{
  TRAP_LIB, 0, (void (*)(void)) &gw_HookEntry
};


/* m68k_HookEntryPreserveAllRegs  ********************************************/

struct EmulLibEntry m68k_HookEntryPreserveAllRegs =
{
  TRAP_LIBNR, 0, (void (*)(void)) &gw_HookEntry
};


/* m68k_PreTimer  ************************************************************/

static BOOL
gw_PreTimer( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;

  return PreTimer( audioctrl );
}

struct EmulLibEntry m68k_PreTimer =
{
  TRAP_LIB, 0, (void (*)(void)) &gw_PreTimer
};


/* m68k_PostTimer  ***********************************************************/

static void
gw_PostTimer( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;

  PostTimer( audioctrl );
}

struct EmulLibEntry m68k_PostTimer =
{
  TRAP_LIBNR, 0, (void (*)(void)) &gw_PostTimer
};

#else

/******************************************************************************
** AmigaOS gateway functions **************************************************
******************************************************************************/

/* gw_initRoutine ************************************************************/

struct AHIBase* ASMCALL
gw_initRoutine( REG( d0, struct AHIBase*  device ),
                REG( a0, APTR             seglist ),
                REG( a6, struct ExecBase* sysbase ) )
{
  return initRoutine( device, seglist, sysbase );
}


/* gw_DevExpunge *************************************************************/

BPTR ASMCALL
gw_DevExpunge( REG( a6, struct AHIBase* device ) )
{
  return DevExpunge( device );
}

/* gw_DevOpen ****************************************************************/

ULONG ASMCALL
gw_DevOpen( REG( d0, ULONG              unit ),
            REG( d1, ULONG              flags ),
            REG( a1, struct AHIRequest* ioreq ),
            REG( a6, struct AHIBase*    AHIBase ) )
{
  return DevOpen( unit, flags, ioreq, AHIBase );
}


/* gw_DevClose ***************************************************************/

BPTR ASMCALL
gw_DevClose( REG( a1, struct AHIRequest* ioreq ),
             REG( a6, struct AHIBase*    AHIBase ) )
{
  return DevClose( ioreq, AHIBase );
}


/* gw_DevBeginIO *************************************************************/

void ASMCALL
gw_DevBeginIO( REG( a1, struct AHIRequest* ioreq ),
               REG( a6, struct AHIBase*    AHIBase ) )
{
  DevBeginIO( ioreq, AHIBase );
}


/* gw_DevAbortIO *************************************************************/

ULONG ASMCALL
gw_DevAbortIO( REG( a1, struct AHIRequest* ioreq ),
               REG( a6, struct AHIBase*    AHIBase ) )
{
  return DevAbortIO( ioreq, AHIBase );
}


/* gw_AllocAudioA ************************************************************/

struct AHIAudioCtrl* ASMCALL
gw_AllocAudioA( REG(a1, struct TagItem* tags),
                REG(a6, struct AHIBase* AHIBase) )
{
  return AllocAudioA( tags, AHIBase );
}


/* gw_FreeAudio **************************************************************/

ULONG ASMCALL
gw_FreeAudio( REG(a2, struct AHIPrivAudioCtrl* audioctrl),
              REG(a6, struct AHIBase*          AHIBase) )
{
  return FreeAudio( audioctrl, AHIBase );
}


/* gw_KillAudio **************************************************************/

ULONG ASMCALL
gw_KillAudio( REG(a6, struct AHIBase* AHIBase) )
{
  return KillAudio( AHIBase );
}


/* gw_ControlAudioA **********************************************************/

ULONG ASMCALL
gw_ControlAudioA( REG(a2, struct AHIPrivAudioCtrl* audioctrl),
                  REG(a1, struct TagItem*          tags),
                  REG(a6, struct AHIBase*          AHIBase) )
{
  return ControlAudioA( audioctrl, tags, AHIBase );
}


/* gw_SetVol *****************************************************************/

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


/* gw_SetFreq ****************************************************************/

ULONG ASMCALL
gw_SetFreq( REG( d0, UWORD                    channel ),
            REG( d1, ULONG                    freq ),
            REG( a2, struct AHIPrivAudioCtrl* audioctrl ),
            REG( d2, ULONG                    flags ),
            REG( a6, struct AHIBase*          AHIBase ) )
{
  return SetFreq( channel, freq, audioctrl, flags, AHIBase );
}


/* gw_SetSound ***************************************************************/

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


/* gw_SetEffect **************************************************************/

ULONG ASMCALL
gw_SetEffect( REG(a0, ULONG*                   effect),
              REG(a2, struct AHIPrivAudioCtrl* audioctrl),
              REG(a6, struct AHIBase*          AHIBase) )
{
  return SetEffect( effect, audioctrl, AHIBase );
}


/* gw_LoadSound **************************************************************/

ULONG ASMCALL
gw_LoadSound( REG(d0, UWORD                    sound),
              REG(d1, ULONG                    type),
              REG(a0, APTR                     info),
              REG(a2, struct AHIPrivAudioCtrl* audioctrl),
              REG(a6, struct AHIBase*          AHIBase) )
{
  return LoadSound( sound, type, info, audioctrl, AHIBase );
}


/* gw_UnloadSound ************************************************************/

ULONG ASMCALL
gw_UnloadSound( REG(d0, UWORD                    sound),
                REG(a2, struct AHIPrivAudioCtrl* audioctrl),
                REG(a6, struct AHIBase*          AHIBase) )
{
  return UnloadSound( sound, audioctrl, AHIBase );
}


/* gw_PlayA ******************************************************************/

ULONG ASMCALL 
gw_PlayA( REG(a2, struct AHIPrivAudioCtrl* audioctrl),
          REG(a1, struct TagItem*          tags),
          REG(a6, struct AHIBase*          AHIBase) )
{
  return PlayA( audioctrl, tags, AHIBase );
}


/* gw_SampleFrameSize ********************************************************/

ULONG ASMCALL
gw_SampleFrameSize( REG(d0, ULONG           sampletype),
                    REG(a6, struct AHIBase* AHIBase) )
{
  return SampleFrameSize( sampletype, AHIBase );
}


/* gw_GetAudioAttrsA *********************************************************/

ULONG ASMCALL 
gw_GetAudioAttrsA( REG(d0, ULONG                    id),
                   REG(a2, struct AHIPrivAudioCtrl* actrl),
                   REG(a1, struct TagItem*          tags),
                   REG(a6, struct AHIBase*          AHIBase) )
{
  return GetAudioAttrsA( id, actrl, tags, AHIBase );
}


/* gw_BestAudioIDA ***********************************************************/

ULONG ASMCALL 
gw_BestAudioIDA( REG(a1, struct TagItem* tags),
                 REG(a6, struct AHIBase* AHIBase) )
{
  return BestAudioIDA( tags, AHIBase );
}


/* gw_AllocAudioRequestA *****************************************************/

struct AHIAudioModeRequester* ASMCALL
gw_AllocAudioRequestA( REG(a0, struct TagItem* tags),
                       REG(a6, struct AHIBase* AHIBase) )
{
  return AllocAudioRequestA( tags, AHIBase );
}


/* gw_AudioRequestA **********************************************************/

ULONG ASMCALL 
gw_AudioRequestA( REG(a0, struct AHIAudioModeRequester* req_in),
                  REG(a1, struct TagItem*               tags ),
                  REG(a6, struct AHIBase*               AHIBase) )
{
  return AudioRequestA( req_in, tags, AHIBase );
}


/* gw_FreeAudioRequest *******************************************************/

void ASMCALL 
gw_FreeAudioRequest( REG(a0, struct AHIAudioModeRequester* req),
                     REG(a6, struct AHIBase*               AHIBase) )
{
  FreeAudioRequest( req, AHIBase );
}


/* gw_NextAudioID ************************************************************/

ULONG ASMCALL
gw_NextAudioID( REG(d0, ULONG           id),
                REG(a6, struct AHIBase* AHIBase) )
{
  return NextAudioID( id, AHIBase );
}


/* gw_AddAudioMode ***********************************************************/

ULONG ASMCALL
gw_AddAudioMode( REG(a0, struct TagItem* DBtags),
                 REG(a6, struct AHIBase* AHIBase) )
{
  return AddAudioMode( DBtags, AHIBase );
}


/* gw_RemoveAudioMode ********************************************************/

ULONG ASMCALL
gw_RemoveAudioMode( REG(d0, ULONG           id),
                    REG(a6, struct AHIBase* AHIBase) )
{
  return RemoveAudioMode( id, AHIBase );
}


/* gw_LoadModeFile ***********************************************************/

ULONG ASMCALL
gw_LoadModeFile( REG(a0, UBYTE*          name),
                 REG(a6, struct AHIBase* AHIBase) )
{
  return LoadModeFile( name, AHIBase );
}


/* m68k_IndexToFrequency *****************************************************/

LONG STDARGS SAVEDS
m68k_IndexToFrequency( struct Gadget *gad, WORD level)
{
  return IndexToFrequency( gad, level );
}


/* m68k_DevProc **************************************************************/

void
m68k_DevProc( void )
{
  DevProc();
}


/* HookEntry *****************************************************************/

ULONG ASMCALL
HookEntry( REG( a0, struct Hook* h ),
           REG( a2, void*        o ), 
           REG( a1, void*        msg ) )
{
  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}


/* m68k_HookEntry ************************************************************/

ULONG ASMCALL
m68k_HookEntry( REG( a0, struct Hook* h ),
                REG( a2, void*        o ), 
                REG( a1, void*        msg ) )
{
  return ( ( (ULONG(*)(struct Hook*, void*, void*)) *h->h_SubEntry)( h, o, msg ) );
}


/* m68k_HookEntryPreserveAllRegs  ********************************************/

asm("
        .even
        .globl _m68k_HookEntryPreserveAllRegs
_m68k_HookEntryPreserveAllRegs:

        moveml d0/d1/a0/a1,sp@-
        jsr    _m68k_HookEntry
        moveml sp@+,d0/d1/a0/a1
        rts
");


/* m68k_PreTimer  ************************************************************/

asm("
        .even
        .globl _m68k_PreTimer
_m68k_PreTimer:

        moveml d1/a0/a1,sp@-
        movel  a2,sp@-
        jsr    _PreTimer
        extl   d0
        addql  #4,%sp
        moveml sp@+,d1/a0/a1
        rts
");


/* m68k_PostTimer  ***********************************************************/

asm("
        .even
        .globl _m68k_PostTimer
_m68k_PostTimer:

        moveml d0/d1/a0/a1,sp@-
        movel  a2,sp@-
        jsr    _PostTimer
        addql  #4,%sp
        moveml sp@+,d0/d1/a0/a1
        rts
");

#endif // ENABLE_MORPHOS
