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
