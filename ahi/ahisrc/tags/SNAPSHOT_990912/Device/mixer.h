/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
     
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

#ifndef _MIXER_H_
#define _MIXER_H_

#include <config.h>
#include <CompilerSpecific.h>
#include "ahi_def.h"

#ifndef VERSION68K

#include "addroutines.h"

BOOL
InitMixroutine ( struct AHIPrivAudioCtrl *audioctrl );

void
CleanUpMixroutine ( struct AHIPrivAudioCtrl *audioctrl );

void
calcMasterVolumeTable ( struct AHIPrivAudioCtrl *audioctrl );

BOOL
initSignedTable ( struct AHIPrivAudioCtrl *audioctrl );

void
calcSignedTable ( struct AHIPrivAudioCtrl *audioctrl );

BOOL
initUnsignedTable ( struct AHIPrivAudioCtrl *audioctrl );

void
calcUnsignedTable ( struct AHIPrivAudioCtrl *audioctrl );

void
SelectAddRoutine ( Fixed     VolumeLeft,
                   Fixed     VolumeRight,
                   ULONG     SampleType,
                   struct    AHIPrivAudioCtrl *audioctrl,
                   LONG     *ScaleLeft,
                   LONG     *ScaleRight,
                   ADDFUNC **AddRoutine );

#if !defined( VERSIONPPC )
void ASMCALL
MixGeneric ( REG(a0, struct Hook *Hook),
             REG(a1, void *dst),
             REG(a2, struct AHIPrivAudioCtrl *audioctrl) );
#else
void
MixGeneric ( struct Hook *Hook,
             void *dst,
             struct AHIPrivAudioCtrl *audioctrl );
#endif

LONG
CalcSamples ( Fixed64 Add,
              ULONG   Type,
              Fixed64 LastOffset,
              Fixed64 Offset );

#else /* VERSION68K */


typedef void (ADDFUNC)(void);

BOOL ASMCALL
InitMixroutine ( REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

void ASMCALL
calcMasterVolumeTable ( REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

BOOL ASMCALL
initSignedTable ( REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

void ASMCALL
calcSignedTable ( REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

BOOL ASMCALL
initUnsignedTable ( REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

void ASMCALL
calcUnsignedTable ( REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

void ASMCALL
SelectAddRoutine ( REG(d0, Fixed     VolumeLeft),
                   REG(d1, Fixed     VolumeRight), 
                   REG(d2, ULONG     SampleType), 
                   REG(a2, struct    AHIPrivAudioCtrl *audioctrl), 
                   REG(a0, LONG     *ScaleLeft),
                   REG(a1, LONG     *ScaleRight), 
                   REG(a3, ADDFUNC **AddRoutine) );


void ASMCALL
Mix( REG(a0, struct Hook *Hook),
     REG(a1, void *dst),
     REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

LONG ASMCALL
CalcSamples ( REG(d0, LONG AddI),
              REG(d1, ULONG AddF ),
              REG(d2, ULONG Type),
              REG(d3, LONG LastOffsetI),
              REG(d4, ULONG LastOffsetF),
              REG(d5, LONG OffsetI),
              REG(d6, ULONG OffsetF) );

#endif /* VERSION68K */

#endif /* _MIXER_H_ */
