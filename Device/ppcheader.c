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
#include <powerpc/powerpc.h>
#include <powerup/gcclib/powerup_protos.h>

#include "version.h"
#include "ahi_def.h"
#include "mixer.h"
#include "ppcheader.h"

/******************************************************************************
** Prototypes *****************************************************************
******************************************************************************/

int
CallMixroutine( unsigned int             magic,
                struct Hook*             Hook, 
                void*                    dst, 
                struct AHIPrivAudioCtrl* audioctrl,
                int                      flush_result );

void
FlushCache( void* address, unsigned long length );

void
FlushCacheAll( void  );

void
InvalidateCache( void* address, unsigned long length );

void WarpUpInt( void );

/******************************************************************************
** First address **************************************************************
******************************************************************************/

#ifdef POWERUP_USE_MIXTASK
void main( void )
{  
  struct AHIPrivAudioCtrl* audioctrl;
  ULONG                    signals;

  audioctrl = (struct AHIPrivAudioCtrl*) 
      PPCGetTaskAttr( PPCTASKTAG_STARTUP_MSGDATA );

//  PPCkprintf( "Got here.... 0x%08lx\n", audioctrl );
  
  while( TRUE )
  {
    signals = PPCWait( SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_F );

//    PPCkprintf( "Got signal!\n" );

    if( signals & SIGBREAKF_CTRL_C )
    {
//      PPCkprintf( "***Break\n" );
      break;
    }
    else if( signals & SIGBREAKF_CTRL_F )
    {
//      PPCkprintf( "***Mix\n" );
      CallMixroutine( 0xC0DECAFE,
                      audioctrl->ahiac_PPCPowerUpContext->Hook,
                      audioctrl->ahiac_PPCPowerUpContext->Dst,
                      audioctrl,
                      TRUE );
    }
  }
}

#else

// This must be the first code in the ELF object due to a bug in
// ppc.library < 46.26

asm("
        .text

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

#endif


/******************************************************************************
** Function used to call the actual mixing routine ****************************
******************************************************************************/

int
CallMixroutine( unsigned int             magic,
                struct Hook*             Hook, 
                void*                    dst, 
                struct AHIPrivAudioCtrl* audioctrl,
                int                      flush_result )
{
  struct AHISoundData *sd;
  int                  i;

  if( magic != 0xC0DECAFE )
  {
    // If the magic cookie was not correct, return error.

    return 20; // RETURN_FAIL
  }

//PPCkprintf( "Start?\n" );
  // Wait for start signal...

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_START );

//PPCkprintf( "Start!\n" );

  // Start m68k interrupt handler

  audioctrl->ahiac_PPCCommand = AHIAC_COM_INIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

//PPCkprintf( "Int trigger!\n" );

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
        // *Flush* all and exit (add an L2 cache and watch this code break!)

#ifdef POWERUP_USE_MIXTASK
        PPCCacheFlushAll();
#else
        FlushCacheAll();
#endif
        break;
      }
      else
      {
#ifdef POWERUP_USE_MIXTASK
        // Flushing the block block is the best we can do as task....

        PPCCacheFlush( sd->sd_Addr,
                       sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ) );
#else
        // *Invalidate* block

        InvalidateCache( sd->sd_Addr,
                         sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ) );
#endif
      }
    }
    sd++;
  }


//PPCkprintf( "Wait!\n" );

  // Wait for m68k interrupt handler to go active

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

//PPCkprintf( "Mixing...!\n" );
  // Mix

  MixGeneric( Hook, dst, audioctrl );

  // Flush mixed samples to memory (PowerUp only!)

  if( flush_result )
  {
    FlushCache( dst, audioctrl->ahiac_BuffSizeNow );
  }

  // Kill the m68k interrupt handler

  audioctrl->ahiac_PPCCommand = AHIAC_COM_QUIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  // Wait for it

  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_FINISHED;
  return 0;
}

/******************************************************************************
** Cache manipulation routines ************************************************
******************************************************************************/

asm( "
        .text

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

");


/******************************************************************************
** WarpUp stuff ***************************************************************
******************************************************************************/

static void* blinkbase = (void*) 0xbfe001;
static ULONG magic     = 0xC0DECAFE;

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

asm( "
          .text

_LVOSetExcHandler = -516
_LVORemExcHandler = -522
_LVOSetExcMMU     = -576
_LVOClearExcMMU   = -582

EXCRETURN_ABORT   = 1

# struct WarpUpContext

wc_Active        = 0
wc_AudioCtrl     = 4
wc_PowerPCBase   = 8
wc_XLock         = 12
wc_Hook          = 16
wc_Dst           = 20
"
#ifndef WARPUP_INVALIDATE_CACHE
"
wc_MixBuffer     = 24;
wc_MixLongWords  = 28;
"
#endif
"
/* InitWarpUp ****************************************************************/

        .align  2
        .globl  InitWarpUp
        .type   InitWarpUp,@function

/*     r3 = struct WarpUpContext*
 */

InitWarpUp:
        mflr    0
        stw     0,8(1)
        mfcr    0
        stw     0,4(1)
        stw     13,-4(1)
        subi    13,1,4
        stwu    1,-(28+14*4+1*4)(1)

        stw     14,-4(13)

        mr      14,3                        # Save WarpUpContext in r14

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

        stw     14,28+3*4(1)                # Store WarpUpContext in tag list

# Register the exception handler

        lwz     3,wc_PowerPCBase(14)
        addi    4,1,28
        lwz     0,_LVOSetExcHandler+2(3)
        mtlr    0
        blrl

        stw     3,wc_XLock(14)

        lwz     14,-4(13)

        lwz     1,0(1)
        lwz     13,-4(1)
        lwz     0,4(1)
        mtcr    0
        lwz     0,8(1)
        mtlr    0
        blr


/* WarpUpInt *****************************************************************/

        .align  2
        .globl  WarpUpInt
        .type   WarpUpInt,@function

WarpUpInt:
        mflr    0
        stw     0,8(1)
        mfcr    0
        stw     0,4(1)
        stw     13,-4(1)
        subi    13,1,4
        stwu    1,-(28+1*4)(1)

        stw     14,-4(13)

        mr      14,2

# Set up MMU

        lwz     3,wc_PowerPCBase(14)
        lwz     0,_LVOSetExcMMU+2(3)
        mtlr    0
        blrl

# Test and clear activation flag (is this out interrupt or somebody elses?)

        addi    3,14,wc_Active
        li      4,0
1:
        lwarx   5,0,3
        stwcx.  4,0,3
        bne-    1b

        cmpwi   0,5,0
        beq     2f

# Call the CallMixroutine (V.4 ABI)

        stwu    1,-16(1)
        stw     2,8(1)
        stw     13,12(1)

        lis     3,magic@ha
        addi    3,3,magic@l
        lwz     3,0(3)
        lwz     4,wc_Hook(14)
        lwz     6,wc_AudioCtrl(14)
"
#ifdef WARPUP_INVALIDATE_CACHE
"
        lwz     5,wc_Dst(14)
        li      7,1                          # Do flush the buffer!
"
#else
"
        lwz     5,wc_MixBuffer(14)
        li      7,0                          # No need to flush the buffer!
"
#endif
"
        bl      CallMixroutine

        lwz     2,8(1)
        lwz     13,12(1)
        addi    1,1,16
"
#ifndef WARPUP_INVALIDATE_CACHE

"
# Copy the cachable mixing buffer to the non-cachable (so the m68k can read it)

        lwz     3,wc_MixBuffer(14)
        lwz     4,wc_Dst(14)
        subi    3,3,4
        subi    4,4,4
        lwz     5,wc_MixLongWords(14)
        mtctr   5
3:
        lwzu    5,4(3)
        stwu    5,4(4)
        bdnz    3b
"
#endif
"        
2:

# Restore MMU

        lwz     3,wc_PowerPCBase(14)
        lwz     0,_LVOClearExcMMU+2(3)
        mtlr    0
        blrl

        li      3,EXCRETURN_ABORT

        lwz     14,-4(13)

        lwz     1,0(1)
        lwz     13,-4(1)
        lwz     0,4(1)
        mtcr    0
        lwz     0,8(1)
        mtlr    0
        blr

/* CleanUpWarpUp *************************************************************/

        .align  2
        .globl  CleanUpWarpUp
        .type   CleanUpWarpUp,@function

/*     r3 = struct WarpUpContext*
 */

CleanUpWarpUp:
        mflr    0
        stw     0,8(1)
        mfcr    0
        stw     0,4(1)
        stw     13,-4(1)
        subi    13,1,4
        stwu    1,-28(1)

# Unregister the exception handler

        lwz     4,wc_XLock(3)
        lwz     3,wc_PowerPCBase(3)
        lwz     0,_LVORemExcHandler+2(3)
        mtlr    0
        blrl

        lwz     1,0(1)
        lwz     13,-4(1)
        lwz     0,4(1)
        mtcr    0
        lwz     0,8(1)
        mtlr    0
        blr
");


/******************************************************************************
** Library & Linking **********************************************************
******************************************************************************/

// Just some library stuff... All the stuff will have to be added 
// in the final release. TODO!

ULONG	__LIB_Version  = VERSION;
ULONG	__LIB_Revision = REVISION;

static const char VersTag[] = 
 "$VER: ahi.elf " VERS " ©1994-1999 Martin Blom. " CPU " version.\r\n";


// Make sure all add-routines are fetched.

static void* a1 = AddByteMono;
static void* a2 = AddLofiByteMono;

#endif /* defined( VERSIONPPC ) */
