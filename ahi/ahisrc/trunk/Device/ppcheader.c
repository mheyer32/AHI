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
//#include <powerup/gcclib/powerup_protos.h>
#include <powerpc/powerpc.h>

#include "version.h"
#include "ahi_def.h"
#include "mixer.h"

void WarpUpInt( void );

int
CallMixroutine( unsigned int magic,
                struct Hook *Hook, 
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


// This must be the first code in the ELF object!

// Function used to call the actual mixing routine.

int
CallMixroutine( unsigned int magic,
                struct Hook *Hook, 
                void *dst, 
                struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHISoundData *sd;
  int                  i;

  if( magic != 0xC0DECAFE )
  {
    // If the magic cookie was not correct, return error.

    return 20; // RETURN_FAIL
  }

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_START );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_INIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  // Invalidate dynamic sample sounds (which is faster than flushing).
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
        // *Flush* all and exit (add an L2 cache and listen to random noise!)

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
  for( i = 0; i < 150; i++ )
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

asm( "

EXCATTR_CODE      = 0x80101000
EXCATTR_DATA      = 0x80101001
EXCATTR_TASK      = 0x80101002
EXCATTR_EXCID     = 0x80101003
EXCATTR_FLAGS     = 0x80101004
EXCATTR_NAME      = 0x80101005
EXCATTR_PRI       = 0x80101006

EXCF_GLOBAL       = 1<<0
EXCF_LOCAL        = 1<<1
EXCF_SMALLCONTEXT = 1<<2
EXCF_LARGECONTEXT = 1<<3
EXCF_ACTIVE       = 1<<4

EXCF_MCHECK       = 1<<2
EXCF_DACCESS      = 1<<3
EXCF_IACCESS      = 1<<4
EXCF_INTERRUPT    = 1<<5
EXCF_ALIGN        = 1<<6
EXCF_PROGRAM      = 1<<7
EXCF_FPUN         = 1<<8
EXCF_TRACE        = 1<<13
EXCF_PERFMON      = 1<<15
EXCF_IABR         = 1<<19

TAG_DONE          = 0

/*     r3 = struct PowerPCBase*
 *     r4 = struct AudioCtrl*
 */

        .align  2
        .globl  InitWarpUp
        .type   InitWarpUp,@function

InitWarpUp:
        li      3,0xcafe
        li      4,0xbeef
        blr


        mflr    0
        stw     0,8(1)
        mfcr    0
        stw     0,4(1)
        stw     13,-4(1)
        subi    13,1,4
        stwu    1,-(28+13*4)(1)

        mr      8,3                         # Save _PowerPCBase in r8
        mr      9,4                         # Save AHIAudioCtrl in r9

        lis     4,_PowerPCBase@ha
        addi    4,4,_PowerPCBase@l
        stw     3,0(4)                      # Store _PowerPCBase for IntHandler

# Build the tag list on the stack

        lis     4,(InitTags-4)@ha
        addi    4,4,InitTags-4@l

        addi    5,1,28-4

1:
        lwzu    6,4(4)
        stwu    6,4(5)
        cmpwi   0,6,0
        lwzu    6,4(4)
        stwu    6,4(5)
        bne     1b

        stw     9,28+3*4(1)                 # Store AHIAudioCtrl in tag list

        lwz     1,0(1)
        lwz     13,-4(1)
        lwz     0,4(1)
        mtcr    0
        lwz     0,8(1)
        mtlr    0
        blr


        .align  2
        .globl  WarpUpInt
        .type   WarpUpInt,@function

WarpUpInt:


/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */

        .align  2
        .globl  FlushCache
      	.type   FlushCache,@function

FlushCache:
        addi    4,4,31
        srwi    4,4,5           /* convert to cache blocks to flush */
        mtctr   4
        li      4,0
1:
        dcbf    3,4             /* flush data cache block to mem */
        addi    4,4,32
        bdnz    1b

        sync                    /* force mem transactions to complete */
        blr                     /* return to calling routine */


        .align  2
        .globl  FlushCacheAll
      	.type   FlushCacheAll,@function

FlushCacheAll:

/* Load the entire data cache with known contents. */
        li      3,-16           /* Start at address 0 */
        li      4,2*256         /* 2 ways, 256 sets per way */
        mtctr   4               /* (use CTR register to save an instruction) */
1:
        lwzu    5,16(3)         /* load a cache line if it's not already present */
        bdnz    1b

/* Flush those known contents from the cache. */
        li      3,0             /* Read 2*128*16 bytes at address 0 */
        mtctr   4               /* (use CTR register to save an instruction) */
2:
        dcbf    0,3             /* flush a cache line */
        addi    3,3,16          /* next line: assumes cache lines are 16 bytes */
        bdnz    2b
        sync
        blr


/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */
        .align  2
        .globl  InvalidateCache
      	.type   InvalidateCache,@function

InvalidateCache:
        addi    4,4,31
        srwi    4,4,5           /* convert to cache blocks to invalidate */
        mtctr   4
        li      4,0
1:
        dcbi    3,4             /* invalidate data cache block */
        addi    4,4,32
        bdnz    1b

        sync                    /* force mem transactions to complete */
        blr                     /* return to calling routine */


        .align  2
        .globl  KernelObject
      	.type   KernelObject,@function

KernelObject:
        stwu    1,-24(1)
        mflr    0
        stw     0,28(1)
        stw     11,8(1)
        stw     12,12(1)
        stw     13,16(1)

        bl      CallMixroutine

        lwz     11,8(1)
        lwz     12,12(1)
        lwz     13,16(1)
        lwz     0,28(1)
        mtlr    0
        addi    1,1,24
        blr
");

// WarpUp stuff

static void* _PowerPCBase = NULL;

static char IntName[] = "AHI/WarpUp Exception Handler";

struct TagItem InitTags[] =
{
  { EXCATTR_CODE,  (ULONG) &WarpUpInt,                },
  { EXCATTR_DATA,  0,                                 },
  { EXCATTR_NAME,  (ULONG) &IntName,                  },
  { EXCATTR_PRI,   32,                                },
  { EXCATTR_EXCID, EXCF_INTERRUPT,                    },
  { EXCATTR_FLAGS, EXCF_GLOBAL | EXCF_LARGECONTEXT,   },
  { TAG_DONE,      0                                  }
};

// Just some library stuff... All the stuff will have to be added 
// in the final release.

ULONG	__LIB_Version  = VERSION;
ULONG	__LIB_Revision = REVISION;

static const char VersTag[] = 
 "$VER: ahi.elf " VERS " ©1994-1999 Martin Blom. " CPU " version.\r\n";


// Make sure all add-routines are fetched.

static void* a1 = AddByteMono;
static void* a2 = AddLofiByteMono;

#endif /* defined( VERSIONPPC ) */
