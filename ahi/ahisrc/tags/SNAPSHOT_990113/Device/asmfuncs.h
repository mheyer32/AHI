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
LONG ASMCALL Fixed2Shift ( REG(d0, Fixed val) );

void ASMCALL Mix ( REG(a0, struct Hook *Hook), REG(a1, void *dst), REG(a2, struct AHIPrivAudioCtrl *audioctrl) );

void ASMCALL Add64p( REG(a0, LONGLONG *ValueAPtr), REG(a1, LONGLONG *ValueBPtr) );
LONG ASMCALL Cmp64p( REG(a0, LONGLONG *ValueAPtr), REG(a1, LONGLONG *ValueBPtr) );
// void ASMCALL Divs64p( REG(a0, LONGLONG *ValueAPtr), REG(a1, LONGLONG *ValueBPtr) );
// void ASMCALL Divu64p( REG(a0, ULONGLONG *ValueAPtr), REG(a1, ULONGLONG *ValueBPtr) );
// void ASMCALL Muls64p( REG(a0, LONGLONG *ValueAPtr), REG(a1, LONGLONG *ValueBPtr) );
// void ASMCALL Mulu64p( REG(a0, ULONGLONG *ValueAPtr), REG(a1, ULONGLONG *ValueBPtr) );
void ASMCALL Neg64p( REG(a0, LONGLONG *ValueAPtr) );
void ASMCALL Sub64p( REG(a0, LONGLONG *ValueAPtr), REG(a1, LONGLONG *ValueBPtr) );

#endif /* _ASMFUNCS_H_ */
