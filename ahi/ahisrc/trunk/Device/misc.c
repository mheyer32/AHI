/* $Id$ */

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/lists.h>
#include <exec/nodes.h>

#include "ahi_def.h"


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

