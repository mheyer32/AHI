/* $Id$ */

#ifndef _ASMFUNCS_H_
#define _ASMFUNCS_H_

#include <config.h>
#include <CompilerSpecific.h>
#include "ahi_def.h"

char STDARGS *Sprintf(char *dst, const char *fmt, ...);
BOOL ASMCALL PreTimer ( void );
void ASMCALL PostTimer ( void );
BOOL ASMCALL DummyPreTimer ( void );
void ASMCALL DummyPostTimer ( void );

void ASMCALL Mix ( REG(a0, struct Hook *Hook), REG(a1, void *dst), REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

#endif /* _ASMFUNCS_H_ */
