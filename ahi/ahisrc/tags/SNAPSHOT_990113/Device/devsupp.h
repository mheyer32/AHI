/* $Id$ */

#ifndef _DEVSUPP_H_
#define _DEVSUPP_H_

#include <config.h>
#include <CompilerSpecific.h>
#include <devices/ahi.h>

#define RecArgs REG(d0, ULONG size),    \
                REG(d1, ULONG add),     \
                REG(a0, APTR src),      \
                REG(a2, ULONG *offset), \
                REG(a3, void **dest)

void ASMCALL  RecM8S( RecArgs );
void ASMCALL  RecS8S( RecArgs );
void ASMCALL RecM16S( RecArgs );
void ASMCALL RecS16S( RecArgs );
void ASMCALL RecM32S( RecArgs );
void ASMCALL RecS32S( RecArgs );

ULONG ASMCALL 
MultFixed ( REG(d0, ULONG a),
            REG(d1, Fixed b) );

void ASMCALL
asmRecordFunc ( REG(d0, ULONG samples),
                REG(a0, void *data),
                REG(a1, void *buffer) );

#endif /*_DEVSUPP_H_ */
