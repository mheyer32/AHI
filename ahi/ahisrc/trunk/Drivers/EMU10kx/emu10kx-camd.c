/*
     emu10kx.audio - AHI driver for SoundBlaster Live! series
     Copyright (C) 2002-2003 Martin Blom <martin@blom.org>

     This program is free software; you can redistribute it and/or
     modify it under the terms of the GNU General Public License
     as published by the Free Software Foundation; either version 2
     of the License, or (at your option) any later version.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <proto/utility.h>

#include "library.h"
#include "8010.h"
#include "hwaccess.h"

/******************************************************************************
** Open CAMD Port  ************************************************************
******************************************************************************/

ULONG
OpenCAMDPort( struct Hook*        hook,
	      struct EMU10kxBase* EMU10kxBase,
	      struct OpenMessage* msg )
{
  struct DriverBase*  AHIsubBase = (struct DriverBase*) EMU10kxBase;
  struct EMU10kxData* dd;

  BOOL in_use;

  KPrintF( "OpenCAMDPort(%ld)\n", msg->PortNum );
  
  if( msg->PortNum >= EMU10kxBase->cards_found ||
      EMU10kxBase->driverdatas[ msg->PortNum ] == NULL )
  {
    Req( "No valid EMU10kxData for CAMD port %ld.", msg->PortNum );
    return FALSE;
  }

  dd = EMU10kxBase->driverdatas[ msg->PortNum ];
  
  ObtainSemaphore( &EMU10kxBase->semaphore );
  in_use = ( dd->camd_transmitfunc != NULL ||
	     dd->camd_receivefunc != NULL );
  if( !in_use )
  {
    dd->camd_transmitfunc = msg->TransmitFunc;
    dd->camd_receivefunc  = msg->ReceiveFunc;
  }
  ReleaseSemaphore( &EMU10kxBase->semaphore );

  if( in_use )
  {
    return FALSE;
  }

  emu10k1_mpu_reset( &dd->card );
  emu10k1_irq_enable( &dd->card, INTE_MIDIRXENABLE );
  emu10k1_irq_enable( &dd->card, INTE_MIDITXENABLE );

  return TRUE;
}


/******************************************************************************
** Close CAMD Port  ***********************************************************
******************************************************************************/

VOID
CloseCAMDPort( struct Hook*         hook,
	       struct EMU10kxBase*  EMU10kxBase,
	       struct CloseMessage* msg )
{
  struct DriverBase*  AHIsubBase = (struct DriverBase*) EMU10kxBase;
  struct EMU10kxData* dd         = EMU10kxBase->driverdatas[ msg->PortNum ];

  KPrintF( "CloseCAMDPort(%ld)\n", msg->PortNum );
  
  emu10k1_irq_disable( &dd->card, INTE_MIDIRXENABLE );
  emu10k1_irq_disable( &dd->card, INTE_MIDITXENABLE );
  emu10k1_mpu_reset( &dd->card );
  
  ObtainSemaphore( &EMU10kxBase->semaphore );
  dd->camd_transmitfunc = NULL;
  dd->camd_receivefunc  = NULL;
  ReleaseSemaphore( &EMU10kxBase->semaphore );
}


/******************************************************************************
** Start CAMD transmission  ***************************************************
******************************************************************************/

VOID
ActivateCAMDXmit( struct Hook*            hook,
		  struct EMU10kxBase*     EMU10kxBase,
		  struct ActivateMessage* msg )
{
  struct DriverBase*  AHIsubBase = (struct DriverBase*) EMU10kxBase;
  struct EMU10kxData* dd         = EMU10kxBase->driverdatas[ msg->PortNum ];
  ULONG               b;

  KPrintF( "ActivateCAMDXmit(%ld)\n", msg->PortNum );

  b = CallHookA( dd->camd_transmitfunc, (Object*) EMU10kxBase, NULL );

  if( b != 0x100 )
  {
    emu10k1_mpu_write_data( &dd->card, b );
  }

  // The remaining bytes, if any, will be handled by the interrupt
  // handler.
}
