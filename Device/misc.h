/* $Id$ */

#ifndef _MISC_H_
#define _MISC_H_

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/lists.h>
#include <exec/nodes.h>

#include "ahi_def.h"


struct Node *
FindNode ( struct List *list,
           struct Node *node );

int
Fixed2Shift( Fixed f );

void
Req( const char* text );

APTR
AHIAllocVec( ULONG byteSize, ULONG requirements );

void
AHIFreeVec( APTR memoryBlock );


#endif /* _MISC_H_ */
