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

void
FlushCacheAll( void  );

void
InvalidateCache( void* address, unsigned long length );

ULONG
InternalSampleFrameSize( ULONG sampletype );

int
entry( struct Hook *Hook, 
       void *dst, 
       struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHISoundData *sd;
  int                  i;

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_START );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_INIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;


  // Invalidate dynamic sample sounds.
  // Currently, the PPC is assumed not to modify dynamic samples.
  // It makes sense as long as no PPC hooks can be called from AHI.
  // Anyway, each dynamic sample is flushed on the m68k side before
  // this routine is called, and invalidated here on the PPC side.
  // However, should a dynamic sample start at address 0, which
  // probably means that the whole address space is used for that
  // sample, all of the data caches are instead flushed.

  sd = audioctrl->ahiac_SoundDatas;

  for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
  {
    if( sd->sd_Type == AHIST_DYNAMICSAMPLE )
    {
      if( sd->sd_Addr == NULL )
      {
        // *Flush* all and exit

        FlushCacheAll();
        break;
      }
      else
      {
        // *Invalidate* block

        InvalidateCache( sd->sd_Addr,
                         sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ) );
      }
    }
    sd++;
  }


  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

/*
  for( i = 0; i < 15000; i++ )
  {
    audioctrl->ahiac_PPCCommand  = AHIAC_COM_DEBUG;
    *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;
    while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );
  }
*/

  MixGeneric( Hook, dst, audioctrl );

  FlushCache( dst, audioctrl->ahiac_BuffSizeNow );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_QUIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_FINISHED;
  return 0;
}

void
WarpInit( void )
{

}

asm( "
/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */

        .globl  FlushCache
      	.type   FlushCache,@function

FlushCache:
        addi    4,4,31
        srwi    4,4,5           /* convert to cache blocks to flush */
        mtctr   4
        li      4,0
fc_loop:
        dcbf    3,4             /* flush data cache block to mem */
        addi    4,4,32
        bdnz    fc_loop

        sync                    /* force mem transactions to complete */
        blr                     /* return to calling routine */


        .globl  FlushCacheAll
      	.type   FlushCacheAll,@function

FlushCacheAll:
/* Load the entire data cache with known contents. */
        li      3,-16           /* Start at address 0 */
        li      4,2*256         /* 2 ways, 256 sets per way */
        mtctr   4               /* (use CTR register to save an instruction) */
fca_l1:
        lwzu    5,16(3)         /* load a cache line if it's not already present */
        bdnz    fca_l1

/* Flush those known contents from the cache. */
        li      3,0             /* Read 2*128*16 bytes at address 0 */
        mtctr   4               /* (use CTR register to save an instruction) */
fca_l2:
        dcbf    0,3             /* flush a cache line */
        addi    3,3,16          /* next line: assumes cache lines are 16 bytes */
        bdnz    fca_l2
        sync
        blr


/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */
        .globl  InvalidateCache
      	.type   InvalidateCache,@function

InvalidateCache:
        addi    4,4,31
        srwi    4,4,5           /* convert to cache blocks to invalidate */
        mtctr   4
        li      4,0
ic_loop:
        dcbi    3,4             /* invalidate data cache block */
        addi    4,4,32
        bdnz    ic_loop

        sync                    /* force mem transactions to complete */
        blr                     /* return to calling routine */
");

#endif /* defined( VERSIONPPC ) */
