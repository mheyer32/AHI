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

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/lists.h>
#include <exec/nodes.h>
#include <powerup/ppclib/memory.h>
#include <powerup/ppclib/interface.h>
#include <powerup/ppclib/object.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/ppc.h>

#include "ahi_def.h"
#include "header.h"


/******************************************************************************
** Findnode *******************************************************************
******************************************************************************/

// Find a node in a list

struct Node *
FindNode ( struct List *list,
           struct Node *node )
{
  struct Node *currentnode;

  for(currentnode = list->lh_Head;
      currentnode->ln_Succ;
      currentnode = currentnode->ln_Succ)
  {
    if(currentnode == node)
    {
      return currentnode;
    }
  }
  return NULL;
}


/******************************************************************************
** Fixed2Shift ****************************************************************
******************************************************************************/

// Returns host many steps to right-shift a value to approximate
// scaling with the Fixed argument.

int
Fixed2Shift( Fixed f )
{
  int   i   = 0;
  Fixed ref = 0x10000;

  while( f < ref )
  {
    i++;
    ref >>= 1;
  }

  return i;
}

/******************************************************************************
** Req ************************************************************************
******************************************************************************/

void
Req( const char* text )
{
  struct EasyStruct es = 
  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) DevName,
    (STRPTR) text,
    "OK"
  };

  EasyRequest( NULL, &es, NULL );
}

/******************************************************************************
** AHIAllocVec ****************************************************************
******************************************************************************/

APTR
AHIAllocVec( ULONG byteSize, ULONG requirements )
{
#ifndef VERSION68K
  if( PPCLibBase != NULL )
  {
    return PPCAllocVec( byteSize, requirements );
  }
  else
#endif
  {
    return AllocVec( byteSize, requirements & ~MEMF_PPCMASK );
  }
}

/******************************************************************************
** AHIFreeVec *****************************************************************
******************************************************************************/

void
AHIFreeVec( APTR memoryBlock )
{
#ifndef VERSION68K
  if( PPCLibBase != NULL )
  {
    PPCFreeVec( memoryBlock );
  }
  else
#endif
  {
    FreeVec( memoryBlock );
  }
}
