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

#include "ahi_def.h"

#if defined( morphos )
# include <emul/emulregs.h>
#endif

#include "audioctrl.h"
#include "database.h"
#include "devcommands.h"
#include "device.h"
#include "header.h"
#include "modeinfo.h"
#include "requester.h"
#include "sound.h"

/******************************************************************************
** Entry gateway functions ****************************************************
******************************************************************************/

#if defined( morphos )

/* MorphOS *******************************************************************/

struct AHIBase*
gw_initRoutine( void )
{
  struct AHIBase*  device  = (struct AHIBase*)  REG_D0;
  APTR             seglist = (APTR)             REG_A0;
  struct ExecBase* sysbase = (struct ExecBase*) REG_A6;

  return initRoutine( device, seglist, sysbase );
}


BPTR 
gw_DevExpunge( void )
{
  struct AHIBase* device = (struct AHIBase*) REG_A6;

  return DevExpunge( device );
}


ULONG 
gw_DevOpen( void )
{
  ULONG              unit    = (ULONG)              REG_D0;
  ULONG              flags   = (ULONG)              REG_D1;
  struct AHIRequest* ioreq   = (struct AHIRequest*) REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)    REG_A6;

  return DevOpen( unit, flags, ioreq, AHIBase );
}


BPTR
gw_DevClose( void )
{
  struct AHIRequest* ioreq   = (struct AHIRequest*)  REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)     REG_A6;

  return DevClose( ioreq, AHIBase );
}


void 
gw_DevBeginIO( void )
{
  struct AHIRequest* ioreq   = (struct AHIRequest*) REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)    REG_A6;

  DevBeginIO( ioreq, AHIBase );
}


ULONG 
gw_DevAbortIO( void )
{
  struct AHIRequest* ioreq   = (struct AHIRequest*) REG_A1;
  struct AHIBase*    AHIBase = (struct AHIBase*)    REG_A6;

  return DevAbortIO( ioreq, AHIBase );
}


struct AHIAudioCtrl* 
gw_AllocAudioA( void )
{
  struct TagItem* tags    = (struct TagItem*) REG_A1;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return AllocAudioA( tags, AHIBase );
}


ULONG 
gw_FreeAudio( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return FreeAudio( audioctrl, AHIBase );
}


ULONG 
gw_KillAudio( void )
{
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return KillAudio( AHIBase );
}


ULONG 
gw_ControlAudioA( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct TagItem*          tags      = (struct TagItem*)          REG_A1;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return ControlAudioA( audioctrl, tags, AHIBase );
}


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


ULONG 
gw_SetEffect( void )
{
  ULONG*                   effect    = (ULONG*)                   REG_A0;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return SetEffect( effect, audioctrl, AHIBase );
}


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


ULONG 
gw_UnloadSound( void )
{
  UWORD                    sound     = (UWORD)                    REG_D0;
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return UnloadSound( sound, audioctrl, AHIBase );
}


ULONG  
gw_PlayA( void )
{
  struct AHIPrivAudioCtrl* audioctrl = (struct AHIPrivAudioCtrl*) REG_A2;
  struct TagItem*          tags      = (struct TagItem*)          REG_A1;
  struct AHIBase*          AHIBase   = (struct AHIBase*)          REG_A6;

  return PlayA( audioctrl, tags, AHIBase );
}


ULONG 
gw_SampleFrameSize( void )
{
  ULONG           sampletype = (ULONG)           REG_D0;
  struct AHIBase* AHIBase    = (struct AHIBase*) REG_A6;

  return SampleFrameSize( sampletype, AHIBase );
}


ULONG  
gw_GetAudioAttrsA( void )
{
  ULONG                    id      = (ULONG)                    REG_D0;
  struct AHIPrivAudioCtrl* actrl   = (struct AHIPrivAudioCtrl*) REG_A2;
  struct TagItem*          tags    = (struct TagItem*)          REG_A1;
  struct AHIBase*          AHIBase = (struct AHIBase*)          REG_A6;

  return GetAudioAttrsA( id, actrl, tags, AHIBase );
}


ULONG  
gw_BestAudioIDA( void )
{
  struct TagItem* tags    = (struct TagItem*) REG_A1;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return BestAudioIDA( tags, AHIBase );
}


struct AHIAudioModeRequester* 
gw_AllocAudioRequestA( void )
{
  struct TagItem* tags    = (struct TagItem*) REG_A0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return AllocAudioRequestA( tags, AHIBase );
}


ULONG  
gw_AudioRequestA( void )
{
  struct AHIAudioModeRequester* req_in  = (struct AHIAudioModeRequester*) REG_A0;
  struct TagItem*               tags    = (struct TagItem*)               REG_A1;
  struct AHIBase*               AHIBase = (struct AHIBase*)               REG_A6;

  return AudioRequestA( req_in, tags, AHIBase );
}


void  
gw_FreeAudioRequest( void )
{
  struct AHIAudioModeRequester* req     = (struct AHIAudioModeRequester*) REG_A0;
  struct AHIBase*               AHIBase = (struct AHIBase*)               REG_A6;

  FreeAudioRequest( req, AHIBase );
}


ULONG 
gw_NextAudioID( void )
{
  ULONG           id      = (ULONG)           REG_D0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return NextAudioID( id, AHIBase );
}


ULONG 
gw_AddAudioMode( void )
{
  struct TagItem* DBtags  = (struct TagItem*) REG_A0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return AddAudioMode( DBtags, AHIBase );
}


ULONG 
gw_RemoveAudioMode( void )
{
  ULONG           id      = (ULONG)           REG_D0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return RemoveAudioMode( id, AHIBase );
}


ULONG 
gw_LoadModeFile( void )
{
  UBYTE*          name    = (UBYTE*)          REG_A0;
  struct AHIBase* AHIBase = (struct AHIBase*) REG_A6;

  return LoadModeFile( name, AHIBase );
}


#else

/* AmigaOS *******************************************************************/

struct AHIBase* ASMCALL
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


struct AHIAudioCtrl* ASMCALL
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
gw_PlayA( REG(a2, struct AHIPrivAudioCtrl* audioctrl),
          REG(a1, struct TagItem*          tags),
          REG(a6, struct AHIBase*          AHIBase) )
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
gw_GetAudioAttrsA( REG(d0, ULONG                    id),
                   REG(a2, struct AHIPrivAudioCtrl* actrl),
                   REG(a1, struct TagItem*          tags),
                   REG(a6, struct AHIBase*          AHIBase) )
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
