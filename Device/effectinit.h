/* $Id$ */

#ifndef _EFFECTINIT_H_
#define _EFFECTINIT_H_

#include <config.h>
#include <CompilerSpecific.h>
#include "ahi_def.h"

#ifndef VERSION68K

BOOL 
update_MasterVolume ( struct AHIPrivAudioCtrl *audioctrl,
                      struct AHIBase *AHIBase );

BOOL
update_DSPEcho ( struct AHIEffDSPEcho *echo,
                 struct AHIPrivAudioCtrl *audioctrl,
                 struct AHIBase *AHIBase );

void
free_DSPEcho ( struct AHIPrivAudioCtrl *audioctrl,
               struct AHIBase *AHIBase );

BOOL 
update_DSPMask ( struct AHIEffDSPMask *mask,
                 struct AHIPrivAudioCtrl *audioctrl,
                 struct AHIBase *AHIBase );

void
clear_DSPMask ( struct AHIPrivAudioCtrl *audioctrl,
                struct AHIBase *AHIBase );


#else /* VERSION68K */


BOOL ASMCALL
update_MasterVolume ( REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                      REG(a5, struct AHIBase *AHIBase) );

BOOL ASMCALL
update_DSPEcho ( REG(a0, struct AHIEffDSPEcho *echo),
                 REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                 REG(a5, struct AHIBase *AHIBase) );

void ASMCALL
free_DSPEcho ( REG(a2, struct AHIPrivAudioCtrl *audioctrl),
               REG(a5, struct AHIBase *AHIBase) );

BOOL ASMCALL
update_DSPMask ( REG(a0, struct AHIEffDSPMask *mask),
                 REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                 REG(a5, struct AHIBase *AHIBase) );

void ASMCALL
clear_DSPMask ( REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                REG(a5, struct AHIBase *AHIBase) );

#endif /* VERSION68K */

#endif /* _EFFECTINIT_H_ */
