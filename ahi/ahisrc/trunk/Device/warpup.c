/* $Id: */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1996-2000 Martin Blom <martin@blom.org>

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

#ifdef __PPC__
# include <exec/types.h>
# include <hardware/intbits.h>
#else
# include <powerpc/powerpc.h>
#endif

#include "ahi_def.h"

#ifdef __PPC__
# include "mixer.h"
#endif

#ifdef __PPC__

/******************************************************************************
** PPC prototypes *************************************************************
******************************************************************************/

static int
CallMixroutine( struct PowerPCContext*   context );

static void
FlushCache( void* address, unsigned long length );

static void
FlushCacheAll( void  );

static void
InvalidateCache( void* address, unsigned long length );


/******************************************************************************
** PPC function used to call the actual mixing routine ************************
******************************************************************************/

static int
CallMixroutine( struct PowerPCContext* context )
{
  struct AHIPrivAudioCtrl* audioctrl;
  struct AHISoundData*     sd;
  int                      i;

  audioctrl = context->AudioCtrl;

  // Wait for start signal...

  while( audioctrl->ahiac_PowerPCContext->Command != PPCC_COM_START );

  // Start m68k interrupt handler

  audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_INIT;
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
        // *Flush* all and exit (add an L2 cache and watch this code break!)

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

  // Wait for m68k interrupt handler to go active

  while( audioctrl->ahiac_PowerPCContext->Command != PPCC_COM_ACK );

  // Mix

  Mix( context->Hook, context->Dst, audioctrl );

  /*** AHIET_MASTERVOLUME ***/

  DoMasterVolume( dst, audioctrl );

  // Flush mixed samples to memory

  FlushCache( context->Dst, audioctrl->ahiac_BuffSizeNow );

  // Kill the m68k interrupt handler

  audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_QUIT;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;

  // Wait for it

  while( audioctrl->ahiac_PowerPCContext->Command != PPCC_COM_ACK );

  audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_FINISHED;
  return 0;
}


/******************************************************************************
** PPC cache manipulation routines ********************************************
******************************************************************************/

asm( "
        .text

/* FlushCache ****************************************************************/

/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */

        .align  2
;       .globl  FlushCache
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


/* FlushCacheAll *************************************************************/

        .align  2
;       .globl  FlushCacheAll
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

/* InvalidateCache ***********************************************************/

/*     r3 = beginning address of data block to flush
 *     r4 = size of data block to flush (in bytes)
 *     assumes cache block granule is 32 bytes
 */
        .align  2
;       .globl  InvalidateCache
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

void WarpUpInterrupt( void );

struct TagItem InitTags[] =
{
  { EXCATTR_CODE,  (ULONG) &WarpUpInt,                    },
  { EXCATTR_DATA,  0,                                     },
  { EXCATTR_NAME,  (ULONG) AHINAME " Exception Handler"   },
  { EXCATTR_PRI,   32,                                    },
  { EXCATTR_EXCID, EXCF_INTERRUPT,                        },
  { EXCATTR_FLAGS, EXCF_GLOBAL | EXCF_LARGECONTEXT,       },
  { TAG_DONE,      0                                      }
};

asm( "
          .text

_LVOSetExcHandler = -516
_LVORemExcHandler = -522
_LVOSetExcMMU     = -576
_LVOClearExcMMU   = -582

EXCRETURN_ABORT   = 1

# struct PowerPCContext

ppcc_Command          = 0*4
ppcc_Argument         = 1*4
ppcc_CurrentMixBuffer = 2*4
ppcc_Active           = 3*4
ppcc_Hook             = 4*4
ppcc_Dst              = 5*4
ppcc_XLock            = 6*4
ppcc_AudioCtrl        = 7*4
ppcc_PowerPCBase      = 8*4
ppcc_MixInterrupt     = 9*4
ppcc_MixBuffer1       = 10*4
ppcc_MixBuffer2       = 11*4

/* WarpUpRegisterExcHandler **************************************************/

        .align  2
        .globl  WarpUpRegisterExcHandler
        .type   WarpUpRegisterExcHandler,@function

/*     r3 = struct PowerPCContext*
 */

WarpUpRegisterExcHandler:
        mflr    0
        stw     0,8(1)
        mfcr    0
        stw     0,4(1)
        stw     13,-4(1)
        subi    13,1,4
        stwu    1,-(28+14*4+1*4)(1)

        stw     14,-4(13)

        mr      14,3                        # Save PowerPCContext in r14

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

        stw     14,28+3*4(1)                # Store PowerPCContext in tag list

# Register the exception handler

        lwz     3,ppcc_PowerPCBase(14)
        addi    4,1,28
        lwz     0,_LVOSetExcHandler+2(3)
        mtlr    0
        blrl

        stw     3,ppcc_XLock(14)

        lwz     14,-4(13)

        lwz     1,0(1)
        lwz     13,-4(1)
        lwz     0,4(1)
        mtcr    0
        lwz     0,8(1)
        mtlr    0
        blr


/* WarpUpInterrupt ***********************************************************/

        .align  2
        .globl  WarpUpInterrupt
        .type   WarpUpInterrupt,@function

WarpUpInterrupt:
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

        lwz     3,ppcc_PowerPCBase(14)
        lwz     0,_LVOSetExcMMU+2(3)
        mtlr    0
        blrl

# Test and clear activation flag (is this our interrupt or somebody elses?)

        addi    3,14,ppcc_Active
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

        mr      3,14
        bl      CallMixroutine

        lwz     2,8(1)
        lwz     13,12(1)
        addi    1,1,16
2:

# Restore MMU

        lwz     3,ppcc_PowerPCBase(14)
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

/* WarpUpRemoveExcHandler ****************************************************/

        .align  2
        .globl  WarpUpRemoveExcHandler
        .type   WarpUpRemoveExcHandler,@function

/*     r3 = struct PowerPCContext*
 */

WarpUpRemoveExcHandler:
        mflr    0
        stw     0,8(1)
        mfcr    0
        stw     0,4(1)
        stw     13,-4(1)
        subi    13,1,4
        stwu    1,-28(1)

# Unregister the exception handler

        lwz     4,ppcc_XLock(3)
        lwz     3,ppcc_PowerPCBase(3)
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


#else /* __PPC__ */


/******************************************************************************
** m68k/native client code ****************************************************
******************************************************************************/


/* Interrupt *****************************************************************/

static INTERRUPT SAVEDS int
Interrupt( struct AHIPrivAudioCtrl *audioctrl __asm( "a1" ) )
{
  if( audioctrl->ahiac_PowerPCContext->Command != PPCC_COM_INIT )
  {
    /* Not for us, continue */
    return 0;
  }
  else
  {
    BOOL running = TRUE;
//kprintf("I");
    while( running )
    {
//kprintf("0");
      switch( audioctrl->ahiac_PowerPCContext->Command )
      {
        case PPCC_COM_INIT:
//kprintf("1");
          // Keep looping
          audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_ACK;
          break;

        case PPCC_COM_ACK:
//kprintf("2");
          // Keep looping, try not to waste to much memory bandwidth...
          asm( "stop #(1<<13) | (2<<8)" : );
          break;

        case PPCC_COM_SOUNDFUNC:
//kprintf("3");
          CallHookPkt( audioctrl->ac.ahiac_SoundFunc,
                       audioctrl,
                       (APTR) audioctrl->ahiac_PowerPCContext->Argument );
          audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_ACK;
          break;

        case PPCC_COM_DEBUG:
//kprintf("4");
          KPrintF( "%lx ", (ULONG) audioctrl->ahiac_PowerPCContext->Argument );
          audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_ACK;
          break;

        case PPCC_COM_QUIT:
//kprintf("5");
          running = FALSE;
          audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_ACK;
          break;
        
        case PPCC_COM_NONE:
        default:
//kprintf("6");
          // Error
          running  = FALSE;
          audioctrl->ahiac_PowerPCContext->Command = PPCC_COM_ACK;
          break;
      }
    }
//kprintf("i");

    /* End chain! */
    return 1;
  }
};


/* WarpUpInit ****************************************************************/

BOOL
WarpUpInit( struct AHIPrivAudioCtrl* audioctrl )
{
  BOOL rc = FALSE;

  audioctrl->ahiac_PowerPCContext = 
    AllocVec32( sizeof( struct PowerPCContext ), 
                        MEMF_PUBLIC | MEMF_CHIP | MEMF_CLEAR );

  if( audioctrl->ahiac_PowerPCContext == NULL )
  {
    Req( "Out of memory." );
  }
  else
  {
    audioctrl->ahiac_PowerPCContext->Command      = PPCC_COM_NONE;
    audioctrl->ahiac_PowerPCContext->Active       = FALSE;

    audioctrl->ahiac_PowerPCContext->AudioCtrl    = audioctrl;
    audioctrl->ahiac_PowerPCContext->PowerPCBase  = PowerPCBase;

    audioctrl->ahiac_PowerPCContext->MixBuffer1 =
        AllocVec32( audioctrl->ac.ahiac_BuffSize,
                    MEMF_PUBLIC | MEMF_CLEAR );

    audioctrl->ahiac_PowerPCContext->MixBuffer2 =
        AllocVec32( audioctrl->ac.ahiac_BuffSize,
                    MEMF_PUBLIC | MEMF_CLEAR );
    
    audioctrl->ahiac_PowerPCContext->CurrentMixBuffer = 
        audioctrl->ahiac_PowerPCContext->MixBuffer1;

    if( mixbuffer1 == NULL || mixbuffer2 == NULL )
    {
      Req( "Out of memory." );
    }
    else
    {
      // Initialize the WarpUp side
      
      struct PPCArgs args = 
      {
        NULL,
        0,
        0,
        NULL,
        0,
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
      };
    
      args.PP_Regs[ 0 ] = (ULONG) audioctrl->ahiac_PowerPCContext;

      if( ! AHIGetELFSymbol( "WarpUpRegisterExcHandler", &args.PP_Code ) )
      {
        Req( "Unable to fetch symbol 'WarpUpRegisterExcHandler'." );
      }
      else
      {
        if( RunPPC( &args ) != PPERR_SUCCESS )
        {
          Req( "Call to WarpUpRegisterExcHandler() failed." );
        }
        else
        {
          audioctrl->ahiac_PowerPCContext->MixInterrupt = 
              AllocVec( sizeof( struct Interrupt ),
                                MEMF_PUBLIC | MEMF_CLEAR );

          if( audioctrl->ahiac_PowerPCContext->MixInterrupt == NULL )
          {
            Req( "Out of memory." );
          }
          else
          {
            struct Interrupt* mi = audioctrl->ahiac_PowerPCContext->MixInterrupt;

            mi->is_Node.ln_Type = NT_INTERRUPT;
            mi->is_Node.ln_Pri  = 127;
            mi->is_Node.ln_Name = AHINAME " PPC Handler Interrupt";
            mi->is_Data         = audioctrl;
            mi->is_Code         = (void(*)(void)) Interrupt;

            AddIntServer( INTB_PORTS, mi );
            
            rc = TRUE;
          }
        }
      }
    }
  }

  return rc;
}


/* WarpUpCallMixer ***********************************************************/

void
WarpUpCallMixer( struct AHIPrivAudioCtrl* audioctrl,
                 void* dst )
{
  // Calls the PPC mixing code to fill a buffer with mixed samples

  struct AHISoundData *sd;
  int                  i;
  BOOL                 flushed = FALSE;

//kprintf("M");
  // Flush all DYNAMICSAMPLE's

  sd = audioctrl->ahiac_SoundDatas;

  for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
  {
    if( sd->sd_Type == AHIST_DYNAMICSAMPLE )
    {
      if( sd->sd_Addr == NULL )
      {
//kprintf("a");
        // Flush all and exit
        CacheClearU();
        flushed = TRUE;
        break;
      }
      else
      {
//kprintf("b");
        SetCache68K( CACHE_DCACHEFLUSH,
                     sd->sd_Addr,
                     sd->sd_Length * AHI_SampleFrameSize( sd->sd_Type ) );
      }
//kprintf("c");
    }
    sd++;
  }

//kprintf("d");
  if( ! flushed )
  {
    /* Since the PPC mix buffer is m68k cacheable in WarpUp, we have to
       flush, or better, *invalidate* the cache before mixing starts. */

    SetCache68K( CACHE_DCACHEFLUSH,
                 mixbuffer,
                 audioctrl->ahiac_BuffSizeNow );
  }
//kprintf("e");

  audioctrl->ahiac_PowerPCContext->Hook         = audioctrl->ac.ahiac_MixerFunc;
  audioctrl->ahiac_PowerPCContext->Dst          = dst;
  audioctrl->ahiac_PowerPCContext->Active       = TRUE;
  audioctrl->ahiac_PowerPCContext->Command      = PPCC_COM_START;

  CausePPCInterrupt();

//kprintf("f");
  while( audioctrl->ahiac_PowerPCContext->Command != PPCC_COM_FINISHED );
//kprintf("g");

}


/* WarpUpCleanUp *************************************************************/

void
WarpUpCleanUp( struct AHIPrivAudioCtrl* audioctrl )
{
  if( audioctrl->ahiac_PowerPCContext != NULL )
  {
    if( audioctrl->ahiac_PowerPCContext->MixInterrupt != NULL )
    {
      RemIntServer( INTB_PORTS, mixinterrupt );
    }

    // Clean up the WarpUp side

    struct PPCArgs args = 
    {
      NULL,
      0,
      0,
      NULL,
      0,
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 }
    };

    args.PP_Regs[ 0 ] = (ULONG) audioctrl->ahiac_PowerPCContext;
       
    if( ! AHIGetELFSymbol( "WarpUpRemoveExcHandler", &args.PP_Code ) )
    {
      Req( "Unable to fetch symbol 'WarpUpRemoveExcHandler'." );
    }
    else
    {
      RunPPC( &args );
    }
  }

  FreeVec( audioctrl->ahiac_PowerPCContext->MixInterrupt );

  AHIFreeVec( audioctrl->ahiac_PowerPCContext->MixBuffer1 );
  AHIFreeVec( audioctrl->ahiac_PowerPCContext->MixBuffer2 );

  AHIFreeVec( audioctrl->ahiac_PowerPCContext );
  audioctrl->ahiac_PowerPCContext = NULL;
}


#endif /* __PPC__ */
