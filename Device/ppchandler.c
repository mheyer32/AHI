/* $Id$ */

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

#include <config.h>
#include <CompilerSpecific.h>


#include <powerup/ppclib/interface.h>
#include <powerup/ppclib/object.h>
#include <powerup/ppclib/tasks.h>
#include <powerpc/powerpc.h>
#include <proto/ppc.h> 
#include <proto/powerpc.h> 

#include "ahi_def.h"

static INTERRUPT SAVEDS int
Interrupt( struct AHIPrivAudioCtrl *audioctrl __asm( "a1" ) )
{

  if( audioctrl->ahiac_PPCCommand != AHIAC_COM_INIT )
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
      switch( audioctrl->ahiac_PPCCommand )
      {
        case AHIAC_COM_INIT:
//kprintf("1");
          // Keep looping
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_ACK:
//kprintf("2");
          // Keep looping, try not to waste to much memory bandwidth...
          asm( "stop #(1<<13) | (2<<8)" : );
          break;

        case AHIAC_COM_SOUNDFUNC:
//kprintf("3");
          CallSoundHook( audioctrl, (void*) audioctrl->ahiac_PPCArgument );
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_DEBUG:
//kprintf("4");
          CallDebug( audioctrl, (ULONG) audioctrl->ahiac_PPCArgument );
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_QUIT:
//kprintf("5");
          running = FALSE;
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;
        
        case AHIAC_COM_NONE:
        default:
//kprintf("6");
          // Error
          running  = FALSE;
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;
      }
    }
//kprintf("i");

    /* End chain! */
    return 1;
  }
};


void
PPCHandler( void )
{
  struct AHIPrivAudioCtrl audioctrl = FindTask( NULL )->tc_UserData;


    switch( MixBackend )
    {
      case MB_NATIVE:
        Req( "Internal error: Illegal MixBackend in InitMixroutine()" );
        break;

      case MB_POWERUP:
        audioctrl->ahiac_PPCMixBuffer1 = AHIAllocVec(
            audioctrl->ac.ahiac_BuffSize,
            MEMF_PUBLIC | MEMF_NOCACHEM68K );
        audioctrl->ahiac_PPCMixBuffer2 = AHIAllocVec(
            audioctrl->ac.ahiac_BuffSize,
            MEMF_PUBLIC | MEMF_NOCACHEM68K );
        break;

      case MB_WARPUP:
        audioctrl->ahiac_PPCMixBuffer1 = AHIAllocVec(
            audioctrl->ac.ahiac_BuffSize,
            MEMF_PUBLIC );
        audioctrl->ahiac_PPCMixBuffer2 = AHIAllocVec(
            audioctrl->ac.ahiac_BuffSize,
            MEMF_PUBLIC );
        break;
    }
    audioctrl->ahiac_PPCMixInterrupt = AllocVec(
        sizeof( struct Interrupt ),
        MEMF_PUBLIC | MEMF_CLEAR );




    if( PPCObject != NULL )
    {
      switch( MixBackend )
      {
        case MB_NATIVE:
          Req( "Internal error: Illegal MixBackend in InitMixroutine()" );
          break;

        case MB_POWERUP:
        {
          rc = TRUE;
          break;
        }

        case MB_WARPUP:
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

          audioctrl->ahiac_PPCWarpUpContext = AHIAllocVec(
              sizeof( struct WarpUpContext ), 
              MEMF_PUBLIC | MEMF_CLEAR | MEMF_NOCACHEPPC | MEMF_NOCACHEM68K );

          if( audioctrl->ahiac_PPCWarpUpContext != NULL )
          {
              audioctrl->ahiac_PPCWarpUpContext->PowerPCBase = PowerPCBase;

              args.PP_Regs[ 0 ] = (ULONG) audioctrl->ahiac_PPCWarpUpContext;

              if( AHIGetELFSymbol( "InitWarpUp", &args.PP_Code ) )
              {
                if( RunPPC( &args ) == PPERR_SUCCESS )
                {
                  rc = TRUE;
                }
                else
                {
                  Req( "Call to InitWarpUp() failed." );
                }
              }
              else
              {
                Req( "Unable to fetch symbol 'InitWarpUp'." );
              }
          }
          else
          {
            Req( "Out of memory in InitMixroutine()." );
          }

          break;
        }
      }

      if( rc )
      {
        audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Type = NT_INTERRUPT;
        audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Pri  = 127;
        audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Name = (STRPTR) DevName;
        audioctrl->ahiac_PPCMixInterrupt->is_Data         = audioctrl;
        audioctrl->ahiac_PPCMixInterrupt->is_Code         = (void(*)(void)) Interrupt;

        AddIntServer( INTB_PORTS, audioctrl->ahiac_PPCMixInterrupt );
      }
      
    }
    else // PPCObject
    {




  struct AHISoundData *sd;
  int                  i;
  BOOL                 flushed = FALSE;

kprintf("M");
  // Flush all DYNAMICSAMPLE's

  sd = audioctrl->ahiac_SoundDatas;

  for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
  {
    if( sd->sd_Type == AHIST_DYNAMICSAMPLE )
    {
      if( sd->sd_Addr == NULL )
      {
kprintf("a");
        // Flush all and exit
        CacheClearU();
        flushed = TRUE;
        break;
      }
      else
      {
kprintf("b");
        switch( MixBackend )
        {
          case MB_POWERUP:
            PPCCacheClearE( sd->sd_Addr,
                            sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ),
                            CACRF_ClearD );
            break;

          case MB_WARPUP:
            CacheClearE( sd->sd_Addr,
                         sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ),
                         CACRF_ClearD );
//            SetCache68K( CACHE_DCACHEFLUSH,
//                         sd->sd_Addr,
//                         sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ) );
            break;

          case MB_NATIVE:
            // Ugh!
            break;
        }
      }
kprintf("c");
    }
    sd++;
  }

kprintf("d");
  if( ! flushed && MixBackend == MB_WARPUP )
  {
kprintf( "0x%08lx, 0x%08lx, %ld\n", audioctrl, audioctrl->ahiac_PPCMixBuffer, audioctrl->ahiac_BuffSizeNow );
    /* Since the PPC mix buffer is m68k cacheable in WarpUp, we have to
       flush, or better, *invalidate* the cache before mixing starts. */

      CacheClearE( audioctrl->ahiac_PPCMixBuffer,
                   audioctrl->ahiac_BuffSizeNow,
                   CACRF_ClearD );

//    SetCache68K( CACHE_DCACHEFLUSH,
//                 audioctrl->ahiac_PPCMixBuffer,
//                 audioctrl->ahiac_BuffSizeNow );
  }
kprintf("e");

  audioctrl->ahiac_PPCCommand = AHIAC_COM_NONE;

  switch( MixBackend )
  {
    case MB_POWERUP:
    {
      struct ModuleArgs mod =
      {
        IF_CACHEFLUSHNO, 0, 0,
        IF_CACHEFLUSHNO | IF_ASYNC, 0, 0,

        0xC0DECAFE,
        (ULONG) Hook,
        (ULONG) audioctrl->ahiac_PPCMixBuffer,
        (ULONG) audioctrl,
        TRUE,                                  // Flush buffer afterwards!
        0, 0, 0,
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
      };

      kprintf("K");
      PPCRunKernelObject( PPCObject, &mod );
      kprintf("k");

      audioctrl->ahiac_PPCCommand = AHIAC_COM_START; // I like it better after

      break;
    }

    case MB_WARPUP:
      audioctrl->ahiac_PPCWarpUpContext->AudioCtrl    = (struct AudioCtrl*) audioctrl;
      audioctrl->ahiac_PPCWarpUpContext->Hook         = Hook;
      audioctrl->ahiac_PPCWarpUpContext->Dst          = audioctrl->ahiac_PPCMixBuffer;
      audioctrl->ahiac_PPCWarpUpContext->Active       = TRUE;

      audioctrl->ahiac_PPCCommand = AHIAC_COM_START; // Must be before
      kprintf("C");
      CausePPCInterrupt();
      kprintf("c");
      break;

    case MB_NATIVE:
      // Ugh!
      break;
  }

kprintf("f");
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_FINISHED );
kprintf("g");

  // The PPC mix buffer is either not m68k-cachable or cleared;
  // just read from it.

  memcpy( dst, audioctrl->ahiac_PPCMixBuffer, audioctrl->ahiac_BuffSizeNow );

  /*** AHIET_OUTPUTBUFFER ***/

  DoOutputBuffer(dst, audioctrl);

  /*** AHIET_CHANNELINFO ***/

  DoChannelInfo(audioctrl);
kprintf("m");






  if( audioctrl->ahiac_PPCMixInterrupt != NULL )
  {
    RemIntServer( INTB_PORTS, audioctrl->ahiac_PPCMixInterrupt );
  }

  switch( MixBackend )
  {
    case MB_NATIVE:
      break;

    case MB_POWERUP:
      break;

    case MB_WARPUP:
    {
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

      if( audioctrl->ahiac_PPCWarpUpContext != NULL )
      {
        args.PP_Regs[ 0 ] = (ULONG) audioctrl->ahiac_PPCWarpUpContext;
        
        if( AHIGetELFSymbol( "CleanUpWarpUp", &args.PP_Code ) )
        {
          RunPPC( &args );
        }
        else
        {
          Req( "Unable to fetch symbol 'CleanUpWarpUp'." );
        }
        AHIFreeVec( audioctrl->ahiac_PPCWarpUpContext );
        audioctrl->ahiac_PPCWarpUpContext = NULL;
      }

      break;
    }
  }

  FreeVec( audioctrl->ahiac_PPCMixInterrupt );
  audioctrl->ahiac_PPCMixInterrupt = NULL;

  AHIFreeVec( audioctrl->ahiac_PPCMixBuffer );
  audioctrl->ahiac_PPCMixBuffer = NULL;

}
