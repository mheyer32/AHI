/* $Id$ */

#ifndef _MIXER_H_
#define _MIXER_H_

#include <config.h>
#include <CompilerSpecific.h>
#include "ahi_def.h"

#ifndef VERSION68K

#define ADDARGS LONG      Samples,\
                Fixed     ScaleLeft,\
                Fixed     ScaleRight,\
                Fixed64  *Offset,\
                Fixed64   Add,\
                struct    AHIPrivAudioCtrl *audioctrl,\
                void     *Src,\
                void    **Dst,\
                struct    AHIChannelData *cd

typedef void (ADDFUNC)(ADDARGS);

BOOL
InitMixroutine ( struct AHIPrivAudioCtrl *audioctrl );

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

void ASMCALL
MixGeneric ( REG(a0, struct Hook *Hook), 
             REG(a1, void *dst), 
             REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

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
