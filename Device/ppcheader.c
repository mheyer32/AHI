/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
     
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

#if defined( VERSIONPPC )

#include <config.h>
#include <CompilerSpecific.h>

#include <exec/types.h>
#include <hardware/intbits.h>
#include <powerup/ppclib/tasks.h>
#include <powerup/gcclib/powerup_protos.h>

#include "ahi_def.h"
#include "mixer.h"

int
entry( struct Hook *Hook, 
       void *dst, 
       struct AHIPrivAudioCtrl *audioctrl );

void
FlushCache( void* address, unsigned long length );

int
entry( struct Hook *Hook, 
       void *dst, 
       struct AHIPrivAudioCtrl *audioctrl )
{
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_START );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_INIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

  MixGeneric( Hook, dst, audioctrl );
  FlushCache( dst, audioctrl->ahiac_BuffSizeNow );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_QUIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_FINISHED;
  return 0;
}

asm( "
/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */

        .globl  FlushCache
      	.type   FlushCache,@function

FlushCache:
        addi    4, 4, 31
        srwi    4, 4, 5       /* convert to cache blocks to flush */
        mtctr   4
        li      4, 0
loop:
        dcbf    3, 4          /* flush data cache block to mem */
        addi    4, 4, 32
        bdnz    loop

        sync                    /* force mem transactions to complete */
        blr                     /* return to calling routine */
");

#endif /* defined( VERSIONPPC ) */
