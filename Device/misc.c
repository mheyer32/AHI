/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2001 Martin Blom <martin@blom.org>
     
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

#if defined( ENABLE_WARPUP )
# include <powerpc/powerpc.h>
# include <powerpc/memoryPPC.h>
# include <proto/powerpc.h>
#include "elfloader.h"
#endif

#include "ahi_def.h"
#include "header.h"
#include "misc.h"


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
  return RawDoFmt( (UBYTE*) fmt,
                   args, 
                   (void(*)(void)) &struffChar, 
                   dst );
}

/******************************************************************************
** AHIInitSemaphore ***********************************************************
******************************************************************************/

void
AHIInitSemaphore( struct SignalSemaphore* sigSem )
{
  InitSemaphore( sigSem );
}


/******************************************************************************
** AHIInitSemaphore ***********************************************************
******************************************************************************/

void
AHIObtainSemaphore( struct SignalSemaphore* sigSem )
{
  // TODO: Verify license compatibility (Code mostly stolen from AROS).

  struct Task *me;

  me=SysBase->ThisTask;
  Disable(); // Not Forbid()!
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
** AHIInitSemaphore ***********************************************************
******************************************************************************/

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
  else
  {
    Alert( AN_SemCorrupt );
  }

  Enable();
}


/******************************************************************************
** AHIInitSemaphore ***********************************************************
******************************************************************************/

static const UWORD m68k_code[] =
{
  0x40C0, // MOVE.W SR,D0
  0x4E73  // RTE
};


static UWORD
GetSR(void)
{
  return Supervisor( (void*) m68k_code );
}

LONG
AHIAttemptSemaphore( struct SignalSemaphore* sigSem )
{
  // TODO: Verify license compatibility (Code mostly stolen from AROS).

  struct Task *owner = NULL;
  struct Task *me;

  if( GetSR() & 0x2000 ) 
  {
    me = NULL;
  }
  else
  {
    me = FindTask(NULL);
  }

  Disable(); // Not Forbid()!

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
    sigSem->ss_QueueCount--;
  }

  owner = sigSem->ss_Owner;

  Enable();

  return (sigSem->ss_Owner == me ? TRUE : FALSE);
}


/******************************************************************************
** AHIAllocVec ****************************************************************
******************************************************************************/

APTR
AHIAllocVec( ULONG byteSize, ULONG requirements )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      return AllocVec( byteSize, requirements );

#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
    {
      APTR v;

      v = AllocVec32( byteSize, requirements );
      CacheClearU();
      return v;
    }
#endif
  }

  Req( "Internal error: Unknown MixBackend in AHIAllocVec()." );
  return NULL;
}

/******************************************************************************
** AHIFreeVec *****************************************************************
******************************************************************************/

void
AHIFreeVec( APTR memoryBlock )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      FreeVec( memoryBlock );
      return;

#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      FreeVec32( memoryBlock );
      return;
#endif
  }

  Req( "Internal error: Unknown MixBackend in AHIFreeVec()." );
}


/******************************************************************************
** AHILoadObject **************************************************************
******************************************************************************/

void*
AHILoadObject( const char* objname )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      Req( "Internal error: Illegal MixBackend in AHILoadObject()" );
      return NULL;

#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
    {
      void* o;

      o = ELFLoadObject( objname );
      CacheClearU();

      return o;
    }
#endif
  }

  Req( "Internal error: Unknown MixBackend in AHILoadObject()." );
  return NULL;
}

/******************************************************************************
** AHIUnLoadObject ************************************************************
******************************************************************************/

void
AHIUnloadObject( void* obj )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      Req( "Internal error: Illegal MixBackend in AHIUnloadObject()" );
      return;

#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      ELFUnLoadObject( obj );
      return;
#endif
  }

  Req( "Internal error: Unknown MixBackend in AHIUnloadObject()" );
}

/******************************************************************************
** AHIGetELFSymbol ************************************************************
******************************************************************************/

BOOL
AHIGetELFSymbol( const char* name,
                 void** ptr )
{
  switch( MixBackend )
  {
    case MB_NATIVE:
      Req( "Internal error: Illegal MixBackend in AHIUnloadObject()" );
      return FALSE;

#if defined( ENABLE_WARPUP )
    case MB_WARPUP:
      return ELFGetSymbol( PPCObject, name, ptr );
#endif
  }

  Req( "Internal error: Unknown MixBackend in AHIUnloadObject()" );
  return FALSE;
}

/******************************************************************************
** PreTimer  ******************************************************************
******************************************************************************/

BOOL
PreTimer( struct AHIPrivAudioCtrl* audioctrl )
{
  ULONG pretimer_period;  // Clocks between PreTimer calls
  ULONG mixer_time;       // Clocks spent in mixer

  pretimer_period = audioctrl->ahiac_Timer.EntryTime.ev_lo;

  ReadEClock( &audioctrl->ahiac_Timer.EntryTime );

  pretimer_period = audioctrl->ahiac_Timer.EntryTime.ev_lo - pretimer_period;

  mixer_time = pretimer_period - ( audioctrl->ahiac_Timer.ExitTime.ev_lo
                                   - audioctrl->ahiac_Timer.EntryTime.ev_lo );

  if( pretimer_period != 0 )
  {
    audioctrl->ahiac_UsedCPU = ( mixer_time << 8 ) / pretimer_period;
  }
  
  return ( audioctrl->ahiac_UsedCPU <= audioctrl->ahiac_MaxCPU );
}

/******************************************************************************
** PostTimer  *****************************************************************
******************************************************************************/

void
PostTimer( struct AHIPrivAudioCtrl* audioctrl )
{
  ReadEClock( &audioctrl->ahiac_Timer.ExitTime );
}
