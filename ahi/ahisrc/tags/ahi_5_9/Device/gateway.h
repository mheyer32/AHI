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

#ifndef ahi_gateway_h
#define ahi_gateway_h

#include <config.h>
#include <CompilerSpecific.h>

/* Library entry points (native) */

#if !defined( AROS_SLIB_ENTRY )
#define AROS_SLIB_ENTRY( f, l ) f
#endif

void AROS_SLIB_ENTRY( gw_initRoutine, Ahi )( void );
void AROS_SLIB_ENTRY( gw_DevExpunge, Ahi )( void );
void AROS_SLIB_ENTRY( gw_DevOpen, Ahi )( void );
void AROS_SLIB_ENTRY( gw_DevClose, Ahi )( void );
void AROS_SLIB_ENTRY( gw_Null, Ahi )( void );
void AROS_SLIB_ENTRY( gw_DevBeginIO, Ahi )( void );
void AROS_SLIB_ENTRY( gw_DevAbortIO, Ahi )( void );
void AROS_SLIB_ENTRY( gw_AllocAudioA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_FreeAudio, Ahi )( void );
void AROS_SLIB_ENTRY( gw_KillAudio, Ahi )( void );
void AROS_SLIB_ENTRY( gw_ControlAudioA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_SetVol, Ahi )( void );
void AROS_SLIB_ENTRY( gw_SetFreq, Ahi )( void );
void AROS_SLIB_ENTRY( gw_SetSound, Ahi )( void );
void AROS_SLIB_ENTRY( gw_SetEffect, Ahi )( void );
void AROS_SLIB_ENTRY( gw_LoadSound, Ahi )( void );
void AROS_SLIB_ENTRY( gw_UnloadSound, Ahi )( void );
void AROS_SLIB_ENTRY( gw_PlayA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_SampleFrameSize, Ahi )( void );
void AROS_SLIB_ENTRY( gw_GetAudioAttrsA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_BestAudioIDA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_AllocAudioRequestA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_AudioRequestA, Ahi )( void );
void AROS_SLIB_ENTRY( gw_FreeAudioRequest, Ahi )( void );
void AROS_SLIB_ENTRY( gw_NextAudioID, Ahi )( void );
void AROS_SLIB_ENTRY( gw_AddAudioMode, Ahi )( void );
void AROS_SLIB_ENTRY( gw_RemoveAudioMode, Ahi )( void );
void AROS_SLIB_ENTRY( gw_LoadModeFile, Ahi )( void );

void m68k_IndexToFrequency( void );
void m68k_DevProc( void );


#if defined( __AROS__ ) && !defined( __mc68000__ )

BOOL
m68k_PreTimer( void );

void
m68k_PostTimer( void );

#define HookEntryPreserveAllRegs HookEntry
#define PreTimerPreserveAllRegs  m68k_PreTimer
#define PostTimerPreserveAllRegs m68k_PostTimer

#else

/* Special hook entry points */
void HookEntryPreserveAllRegs( void );

/* (Pre|Post)Timer entry points */
void PreTimerPreserveAllRegs( void );
void PostTimerPreserveAllRegs( void );

#endif /* defined( __AROS__ ) && !defined( __mc68000__ ) */

#endif /* ahi_gateway_h */
