/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1997-1999 Martin Blom <martin@blom.org>
     
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

void ASMCALL do_DSPEchoMono16 ( REGS );
void ASMCALL do_DSPEchoMono16Fast ( REGS );
void ASMCALL do_DSPEchoStereo16 ( REGS );
void ASMCALL do_DSPEchoStereo16Fast ( REGS );
void ASMCALL do_DSPEchoMono32 ( REGS );
void ASMCALL do_DSPEchoStereo32 ( REGS );
void ASMCALL do_DSPEchoMono16NCFM ( REGS );
void ASMCALL do_DSPEchoStereo16NCFM ( REGS );
void ASMCALL do_DSPEchoMono16NCFMFast ( REGS );
void ASMCALL do_DSPEchoStereo16NCFMFast ( REGS );


#endif /* VERSION68K */

#endif /* _DSPECHO_H_ */
