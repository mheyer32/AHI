/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2003 Martin Blom <martin@blom.org>
     
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

#include <stdarg.h>

#include <exec/alerts.h>
#include <exec/execbase.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/timer.h>

#include "ahi_def.h"
#include "header.h"
#include "misc.h"

#if defined( __AROS__ )
void
ReadEClock( struct EClockVal* ecv )
{
  ecv->ev_hi = 0;
  ecv->ev_lo = 0;
}
#endif

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
** ReqA ***********************************************************************
******************************************************************************/

void
ReqA( const char* text, APTR args )
{
  struct EasyStruct es = 
  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) DevName,
    (STRPTR) text,
    "OK"
#ifdef __AMIGAOS4__
    ,0,0
#endif
  };

  EasyRequestArgs( NULL, &es, NULL, args );
}

/******************************************************************************
** SprintfA *******************************************************************
******************************************************************************/

static UWORD struffChar[] =
{
  0x16c0,     // moveb %d0,%a3@+
  0x4e75      // rts
};

char*
SprintfA( char *dst, const char *fmt, ULONG* args )
{
#ifndef __AMIGAOS4__
  return RawDoFmt( (UBYTE*) fmt,
                   args, 
                   (void(*)(void)) &struffChar,
                   dst );
#else
  return RawDoFmt( (UBYTE*) fmt,
                   args, 
                   0,
                   dst );
#endif
}

/******************************************************************************
** AHIInitSemaphore ***********************************************************
******************************************************************************/

void
AHIInitSemaphore( struct SignalSemaphore* sigSem )
{
  // TODO: Verify license compatibility (Code mostly stolen from AROS).

  sigSem->ss_WaitQueue.mlh_Head     = (struct MinNode *)&sigSem->ss_WaitQueue.mlh_Tail;
  sigSem->ss_WaitQueue.mlh_Tail     = NULL;
  sigSem->ss_WaitQueue.mlh_TailPred = (struct MinNode *)&sigSem->ss_WaitQueue.mlh_Head;
  sigSem->ss_Link.ln_Type = NT_SIGNALSEM;
  sigSem->ss_NestCount = 0;
  sigSem->ss_Owner = 0;
  sigSem->ss_QueueCount = -1;
}


/******************************************************************************
** AHIObtainSemaphore *********************************************************
******************************************************************************/

void
AHIObtainSemaphore( struct SignalSemaphore* sigSem )
{
  // TODO: Verify license compatibility (Code mostly stolen from AROS).

  struct Task *me;

  Disable(); // Not Forbid()!
  me=SysBase->ThisTask;
  sigSem->ss_QueueCount++;
  if( sigSem->ss_QueueCount == 0 )
  {
    sigSem->ss_Owner = me;
    sigSem->ss_NestCount++;
  }
  else if( sigSem->ss_Owner == me )
  {
    sigSem->ss_NestCount++;
  }
  else
  {
    struct SemaphoreRequest sr;
    sr.sr_Waiter = me;
    me->tc_SigRecvd &= ~SIGF_SINGLE;
    AddTail((struct List *)&sigSem->ss_WaitQueue, (struct Node *)&sr);
    Wait(SIGF_SINGLE);
  }
  Enable();
}


/******************************************************************************
** AHIReleaseSemaphore ********************************************************
******************************************************************************/
#include "debug.h"
void
AHIReleaseSemaphore( struct SignalSemaphore* sigSem )
{
  // TODO: Verify license compatibility (Code mostly stolen from AROS).
  
  Disable(); // Not Forbid()!

  sigSem->ss_NestCount--;
  sigSem->ss_QueueCount--;
  if(sigSem->ss_NestCount == 0)
  {
    if( sigSem->ss_QueueCount >= 0
	&& sigSem->ss_WaitQueue.mlh_Head->mln_Succ != NULL )
    {
      struct SemaphoreRequest *sr, *srn;
      struct SemaphoreMessage *sm;
      sr = (struct SemaphoreRequest *)sigSem->ss_WaitQueue.mlh_Head;

      // Note that shared semaphores are not supported!

      sm = (struct SemaphoreMessage *)sr;

      Remove((struct Node *)sr);
      sigSem->ss_NestCount++;
      if(sr->sr_Waiter != NULL)
      {
	sigSem->ss_Owner = sr->sr_Waiter;
	Signal(sr->sr_Waiter, SIGF_SINGLE);
      }
      else
      {
	sigSem->ss_Owner = (struct Task *)sm->ssm_Semaphore;
	sm->ssm_Semaphore = sigSem;
	ReplyMsg((struct Message *)sr);
      }
    }
    else
    {
      sigSem->ss_Owner = NULL;
      sigSem->ss_QueueCount = -1;
    }
  }
  else if(sigSem->ss_NestCount < 0)
  {
    Alert( AN_SemCorrupt );
  }

  Enable();
}


/******************************************************************************
** AHIAttemptSemaphore ********************************************************
******************************************************************************/

LONG
AHIAttemptSemaphore( struct SignalSemaphore* sigSem )
{
  // TODO: Verify license compatibility (Code mostly stolen from AROS).

  LONG rc = FALSE;

  Disable(); // Not Forbid()!

  sigSem->ss_QueueCount++;
  if( sigSem->ss_QueueCount == 0 )
  {
    sigSem->ss_Owner = (APTR) ~0;
    sigSem->ss_NestCount++;
    rc = TRUE;
  }
  else
  {
    sigSem->ss_QueueCount--;
  }

  Enable();

  return rc;
}


/******************************************************************************
**** Endian support code. *****************************************************
******************************************************************************/

/* See the header file for macros */

#if !defined( WORDS_BIGENDIAN )

static UWORD
EndianSwapUWORD( UWORD x ) {
  return ((((x) >> 8) & 0x00ffU) |
	  (((x) << 8) & 0xff00U) );
}

static ULONG
EndianSwapULONG( ULONG x ) {
  return ((((x) >> 24) & 0x000000ffUL) |
	  (((x) >> 8)  & 0x0000ff00UL) |
	  (((x) << 8)  & 0x00ff0000UL) |
	  (((x) << 24) & 0xff000000UL) );
}

void
EndianSwap( size_t size, void* data) {
  switch( size ) {
    case 1:
      break;

    case 2:
      *((UWORD*) data) = EndianSwapUWORD( *((UWORD*) data) );
      break;

    case 4:
      *((ULONG*) data) = EndianSwapULONG( *((ULONG*) data) );
      break;

    case 8: {
      ULONG tmp;

      tmp = EndianSwapULONG( *((ULONG*) data) );
      *((ULONG*) data) = EndianSwapULONG( *((ULONG*) (data + 4)) );
      *((ULONG*) (data + 4)) = tmp;
      break;
    }
      
    default:
      break;
  }
}

#endif

/******************************************************************************
** PreTimer  ******************************************************************
******************************************************************************/

BOOL
PreTimerFunc( struct Hook*             hook,
	      struct AHIPrivAudioCtrl* audioctrl,
	      void*                    null )
{
  return PreTimer( audioctrl );
}

BOOL
PreTimer( struct AHIPrivAudioCtrl* audioctrl )
{
  ULONG pretimer_period;  // Clocks between PreTimer calls
  ULONG mixer_time;       // Clocks spent in mixer

  if( TimerBase == NULL )
  {
    return FALSE;
  }

  pretimer_period = audioctrl->ahiac_Timer.EntryTime.ev_lo;

  ReadEClock( &audioctrl->ahiac_Timer.EntryTime );

  pretimer_period = audioctrl->ahiac_Timer.EntryTime.ev_lo - pretimer_period;

  mixer_time = pretimer_period - ( audioctrl->ahiac_Timer.ExitTime.ev_lo
                                   - audioctrl->ahiac_Timer.EntryTime.ev_lo );

  if( pretimer_period != 0 )
  {
    audioctrl->ahiac_UsedCPU = ( mixer_time << 8 ) / pretimer_period;

    return ( audioctrl->ahiac_UsedCPU <= audioctrl->ahiac_MaxCPU );
  }
  else
  {
    return FALSE;
  }
}

/******************************************************************************
** PostTimer  *****************************************************************
******************************************************************************/

void
PostTimerFunc( struct Hook*             hook,
	       struct AHIPrivAudioCtrl* audioctrl,
	       void*                    null )
{
  PostTimer( audioctrl );
}

void
PostTimer( struct AHIPrivAudioCtrl* audioctrl )
{
  if( TimerBase == NULL )
  {
    return;
  }

  ReadEClock( &audioctrl->ahiac_Timer.ExitTime );
}
