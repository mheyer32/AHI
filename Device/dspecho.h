/* $Id$ */

#ifndef _DSPECHO_H_
#define _DSPECHO_H_

#include <config.h>
#include <CompilerSpecific.h>

#include "ahi_def.h"
#include "dsp.h"

#ifndef VERSION68K


void
do_DSPEchoMono32 ( struct Echo *es,
                   void *buf,
                   struct AHIPrivAudioCtrl *audioctrl );

void
do_DSPEchoStereo32 ( struct Echo *es,
                     void *buf,
                     struct AHIPrivAudioCtrl *audioctrl );


#else /* VERSION68K */


#define REGS REG(a0, struct Echo *es),\
             REG(a1, void *buf),\
             REG(a2, struct AHIPrivAudioCtrl *audioctrl)

void ASMCALL do_DSPEchoMono16 ( REGS ) {}
void ASMCALL do_DSPEchoMono16Fast ( REGS ) {}
void ASMCALL do_DSPEchoStereo16 ( REGS ) {}
void ASMCALL do_DSPEchoStereo16Fast ( REGS ) {}
void ASMCALL do_DSPEchoMono32 ( REGS ) {}
void ASMCALL do_DSPEchoStereo32 ( REGS ) {}
void ASMCALL do_DSPEchoMono16NCFM ( REGS ) {}
void ASMCALL do_DSPEchoStereo16NCFM ( REGS ) {}
void ASMCALL do_DSPEchoMono16NCFMFast ( REGS ) {}
void ASMCALL do_DSPEchoStereo16NCFMFast ( REGS ) {}


#endif /* VERSION68K */

#endif /* _DSPECHO_H_ */
