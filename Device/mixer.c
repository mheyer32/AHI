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

# include <exec/interrupts.h>
# include <hardware/intbits.h>

#if !defined( VERSIONPPC )
# include <exec/memory.h>
# include <exec/execbase.h>
# include <powerup/ppclib/memory.h>
# include <powerup/ppclib/interface.h>
# include <powerup/ppclib/object.h>
# include <powerpc/powerpc.h>

# include <proto/exec.h>
# include <proto/utility.h>
# include <proto/ppc.h> 
# include <proto/powerpc.h> 
# include <clib/ahi_protos.h>
# include <pragmas/ahi_pragmas.h>
# include <proto/ahi_sub.h>
#endif

#include <string.h>
#include <stddef.h>

#include "ahi_def.h"
#include "dsp.h"
#include "mixer.h"
#include "misc.h"
#include "header.h"


/******************************************************************************
** Prototypes *****************************************************************
******************************************************************************/

static void
DoMasterVolume ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl );

#if !defined( VERSIONPPC )

static void
DoOutputBuffer ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl );

static void
DoChannelInfo ( struct AHIPrivAudioCtrl *audioctrl );

#endif /* !defined( VERSIONPPC ) */

LONG AddSilence( ADDARGS );
LONG AddSilenceB( ADDARGS );

LONG AddByteMVH( ADDARGS );
LONG AddByteSVPH( ADDARGS );
LONG AddBytesMVH( ADDARGS );
LONG AddBytesSVPH( ADDARGS );
LONG AddWordMVH( ADDARGS );
LONG AddWordSVPH( ADDARGS );
LONG AddWordsMVH( ADDARGS );
LONG AddWordsSVPH( ADDARGS );

LONG AddByteMVHB( ADDARGS );
LONG AddByteSVPHB( ADDARGS );
LONG AddBytesMVHB( ADDARGS );
LONG AddBytesSVPHB( ADDARGS );
LONG AddWordMVHB( ADDARGS );
LONG AddWordSVPHB( ADDARGS );
LONG AddWordsMVHB( ADDARGS );
LONG AddWordsSVPHB( ADDARGS );

ADDFUNC* AddSilencePtr    = NULL;
ADDFUNC* AddSilenceBPtr   = NULL;
ADDFUNC* AddByteMVHPtr    = NULL;
ADDFUNC* AddByteSVPHPtr   = NULL;
ADDFUNC* AddBytesMVHPtr   = NULL;
ADDFUNC* AddBytesSVPHPtr  = NULL;
ADDFUNC* AddWordMVHPtr    = NULL;
ADDFUNC* AddWordSVPHPtr   = NULL;
ADDFUNC* AddWordsMVHPtr   = NULL;
ADDFUNC* AddWordsSVPHPtr  = NULL;
ADDFUNC* AddByteMVHBPtr   = NULL;
ADDFUNC* AddByteSVPHBPtr  = NULL;
ADDFUNC* AddBytesMVHBPtr  = NULL;
ADDFUNC* AddBytesSVPHBPtr = NULL;
ADDFUNC* AddWordMVHBPtr   = NULL;
ADDFUNC* AddWordSVPHBPtr  = NULL;
ADDFUNC* AddWordsMVHBPtr  = NULL;
ADDFUNC* AddWordsSVPHBPtr = NULL;

static const UBYTE type2bytes[]=
{
  1,    // AHIST_M8S  (0)
  2,    // AHIST_M16S (1)
  2,    // AHIST_S8S  (2)
  4,    // AHIST_S16S (3)
  1,    // AHIST_M8U  (4)
  0,
  0,
  0,
  4,    // AHIST_M32S (8)
  0,
  8     // AHIST_S32S (10)
};

inline static ULONG
SampleFrameSize( ULONG sampletype )
{
  return type2bytes[sampletype];
}

/******************************************************************************
** PowerUp Support code *******************************************************
******************************************************************************/

#if defined( VERSIONPPC )

/* PPC code ******************************************************************/

static void
CallSoundHook( struct AHIPrivAudioCtrl *audioctrl,
               void* arg )
{
  audioctrl->ahiac_PPCCommand  = AHIAC_COM_SOUNDFUNC;
  audioctrl->ahiac_PPCArgument = arg;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );
}

static void
CallDebug( struct AHIPrivAudioCtrl *audioctrl, long value )
{
  audioctrl->ahiac_PPCCommand  = AHIAC_COM_DEBUG;
  audioctrl->ahiac_PPCArgument = (void*) value;
  *((WORD*) 0xdff09C)  = INTF_SETCLR | INTF_PORTS;
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_ACK );
}

#else

/* M68k code *****************************************************************/

static void
CallSoundHook( struct AHIPrivAudioCtrl *audioctrl,
               void* arg )
{
  CallHookPkt( audioctrl->ac.ahiac_SoundFunc,
               audioctrl,
               arg );
}

static void
CallDebug( struct AHIPrivAudioCtrl *audioctrl, long value )
{
  kprintf( "%lx ", value );
}


INTERRUPT SAVEDS int
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

//kprintf( "Interrupt\n" );
    audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;

    while( running )
    {
      switch( audioctrl->ahiac_PPCCommand )
      {
        case AHIAC_COM_INIT:
        case AHIAC_COM_ACK:
//kprintf( "AHIAC_COM_INITACK\n" );
          // Keep looping
          asm( "stop #(1<<13) | (2<<8)" : );
          break;

        case AHIAC_COM_SOUNDFUNC:
//kprintf( "AHIAC_COM_SOUNDFUNC\n" );
          CallHookPkt( audioctrl->ac.ahiac_SoundFunc,
                       (struct AHIPrivAudioCtrl*) audioctrl,
                       (APTR) audioctrl->ahiac_PPCArgument );
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_DEBUG:
          //CallDebug( audioctrl, (ULONG) audioctrl->ahiac_PPCArgument );
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_QUIT:
//kprintf( "AHIAC_COM_QUIT\n" );
          running = FALSE;
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;
        
        case AHIAC_COM_NONE:
        default:
          // Error
          running  = FALSE;
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;
      }
    }

//kprintf( "Exiting\n" );
    /* End chain! */
    return 1;
  }
};

void ASMCALL
MixPowerUp( REG(a0, struct Hook *Hook), 
            REG(a1, void *dst), 
            REG(a2, struct AHIPrivAudioCtrl *audioctrl) )
{
  struct ModuleArgs mod =
  {
    IF_CACHEFLUSHNO, 0, 0,
    IF_CACHEFLUSHNO | IF_ASYNC, 0, 0,

    (ULONG) Hook, (ULONG) audioctrl->ahiac_PPCMixBuffer, (ULONG) audioctrl,
    0, 0, 0, 0, 0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
  };

//kprintf( "MixPowerUp\n" );

  // Flush all DYNAMICSAMPLE's 
  // TODO: Only flush if there are DYNAMICSAMPLE's loaded!

  //CacheClearU();

  if( PPCLibBase == NULL )
  {
    /* Since the PPC mix buffer is m68k cacheable in WarpUp, we have to
       flush the cache before mixing starts. :( */

    SetCache68K( CACHE_DCACHEFLUSH,
                 audioctrl->ahiac_PPCMixBuffer,
                 audioctrl->ahiac_BuffSizeNow );
  }


  audioctrl->ahiac_PPCCommand = AHIAC_COM_NONE;
//kprintf( "1: audioctrl->ahiac_PPCCommand = %ld\n", audioctrl->ahiac_PPCCommand );

  if( PPCLibBase != NULL )
  {
    PPCRunKernelObject( PPCObject, &mod );
  }
  else
  {
    CausePPCInterrupt();
  }
//kprintf( "Ran KernelObject\n" );

  audioctrl->ahiac_PPCCommand = AHIAC_COM_START;
//kprintf( "2: audioctrl->ahiac_PPCCommand = %ld\n", audioctrl->ahiac_PPCCommand );
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_FINISHED );
//kprintf( "Waited\n" );
//kprintf( "3: audioctrl->ahiac_PPCCommand = %ld\n", audioctrl->ahiac_PPCCommand );

  // The PPC mix buffer is not m68k-cachable (or cleared); just read from it.

  memcpy( dst, audioctrl->ahiac_PPCMixBuffer, audioctrl->ahiac_BuffSizeNow );

  /*** AHIET_OUTPUTBUFFER ***/

  DoOutputBuffer(dst, audioctrl);

  /*** AHIET_CHANNELINFO ***/

  DoChannelInfo(audioctrl);
}

#endif /* defined( VERSIONPPC ) */


#if !defined( VERSIONPPC )

/******************************************************************************
** InitMixroutine *************************************************************
******************************************************************************/

// This function is used to initialize the mixer routine (called from 
// AHI_AllocAudio()).

BOOL
InitMixroutine ( struct AHIPrivAudioCtrl *audioctrl )
{
  BOOL rc = FALSE;
kprintf( "InitMixroutine\n" );

  // Allocate and initialize the AHIChannelData structures
  // This structure could be accessed from from interrupts!

  audioctrl->ahiac_ChannelDatas = AHIAllocVec(
      audioctrl->ac.ahiac_Channels * sizeof(struct AHIChannelData),
      MEMF_PUBLIC | MEMF_CLEAR | MEMF_NOCACHESYNCPPC | MEMF_NOCACHESYNCM68K );

  // Allocate and initialize the AHISoundData structures
  // This structure could be accessed from from interrupts!

  audioctrl->ahiac_SoundDatas = AHIAllocVec(
      audioctrl->ac.ahiac_Sounds * sizeof(struct AHISoundData),
      MEMF_PUBLIC | MEMF_CLEAR | MEMF_NOCACHESYNCPPC | MEMF_NOCACHESYNCM68K );

kprintf( "InitMixroutine #2\n" );
  // Allocate structures specific to the PPC version

  if( PPCObject != NULL )
  {
    audioctrl->ahiac_PPCMixBuffer = AHIAllocVec(
        audioctrl->ac.ahiac_BuffSize,
        MEMF_PUBLIC | MEMF_NOCACHEM68K );

    audioctrl->ahiac_PPCMixInterrupt = AllocVec(
        sizeof( struct Interrupt ),
        MEMF_PUBLIC | MEMF_CLEAR );
  }

kprintf( "InitMixroutine #3\n" );
  // Now link the list and fill in the channel number for each structure.

  if( audioctrl->ahiac_ChannelDatas != NULL &&
      audioctrl->ahiac_SoundDatas != NULL &&
      ( PPCObject == NULL || audioctrl->ahiac_PPCMixBuffer != NULL ) &&
      ( PPCObject == NULL || audioctrl->ahiac_PPCMixInterrupt != NULL ) )
  {
    struct AHIChannelData *cd;
    struct AHISoundData   *sd;
    int                    i;

    cd = audioctrl->ahiac_ChannelDatas;

    audioctrl->ahiac_WetList = cd;
    audioctrl->ahiac_DryList = NULL;

    for(i = 0; i < audioctrl->ac.ahiac_Channels - 1; i++)
    {
      // Set Channel No
      cd->cd_ChannelNo = i;

      // Set link to next channel
      cd->cd_Succ = cd + 1;
      cd++;
    }

    // Set the last No
    cd->cd_ChannelNo = i;

    // Clear the last link;
    cd->cd_Succ = NULL;


    sd = audioctrl->ahiac_SoundDatas;

    for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
    {
      sd->sd_Type = AHIST_NOTYPE;
    }

kprintf( "InitMixroutine #4\n" );
    if( PPCObject != NULL )
    {
      int r = ~0;

      audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Type = NT_INTERRUPT;
      audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Pri  = 127;
      audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Name = (STRPTR) DevName;
      audioctrl->ahiac_PPCMixInterrupt->is_Data         = audioctrl;
      audioctrl->ahiac_PPCMixInterrupt->is_Code         = (void(*)(void)) Interrupt;

      AddIntServer( INTB_PORTS, audioctrl->ahiac_PPCMixInterrupt );

kprintf( "InitMixroutine #5\n" );
#define GetSymbol( name ) r &= AHIGetELFSymbol( #name, (void*) &name ## Ptr )

      GetSymbol( AddSilence    );
      GetSymbol( AddSilence    );
      GetSymbol( AddSilenceB   );
      GetSymbol( AddByteMVH    );
      GetSymbol( AddByteSVPH   );
      GetSymbol( AddBytesMVH   );
      GetSymbol( AddBytesSVPH  );
      GetSymbol( AddWordMVH    );
      GetSymbol( AddWordSVPH   );
      GetSymbol( AddWordsMVH   );
      GetSymbol( AddWordsSVPH  );
      GetSymbol( AddByteMVHB   );
      GetSymbol( AddByteSVPHB  );
      GetSymbol( AddBytesMVHB  );
      GetSymbol( AddBytesSVPHB );
      GetSymbol( AddWordMVHB   );
      GetSymbol( AddWordSVPHB  );
      GetSymbol( AddWordsMVHB  );
      GetSymbol( AddWordsSVPHB );

#undef GetSymbol

kprintf( "InitMixroutine #6\n" );
      // Sucess?

      if( r != 0 )
      {
        if( PPCLibBase != NULL )
        {
          rc = TRUE;
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
kprintf( "finding WarpInit..." );
          if( AHIGetELFSymbol( "WarpInit", args.PP_Code ) )
          {
            int status;
kprintf( "%08lx\n", args.PP_Code );
            status = RunPPC( &args );
            
            if( status == PPERR_SUCCESS )
            {
              rc = TRUE;
            }
          }

        }
      }
    }
    else // PPCObject
    {

#define GetSymbol( name ) name ## Ptr = name;

      GetSymbol( AddSilence    );
      GetSymbol( AddSilence    );
      GetSymbol( AddSilenceB   );
      GetSymbol( AddByteMVH    );
      GetSymbol( AddByteSVPH   );
      GetSymbol( AddBytesMVH   );
      GetSymbol( AddBytesSVPH  );
      GetSymbol( AddWordMVH    );
      GetSymbol( AddWordSVPH   );
      GetSymbol( AddWordsMVH   );
      GetSymbol( AddWordsSVPH  );
      GetSymbol( AddByteMVHB   );
      GetSymbol( AddByteSVPHB  );
      GetSymbol( AddBytesMVHB  );
      GetSymbol( AddBytesSVPHB );
      GetSymbol( AddWordMVHB   );
      GetSymbol( AddWordSVPHB  );
      GetSymbol( AddWordsMVHB  );
      GetSymbol( AddWordsSVPHB );

#undef GetSymbol

      // Sucess!
    
      rc = TRUE;
    }
  }

  return rc;
}

/******************************************************************************
** CleanUpMixroutine **********************************************************
******************************************************************************/

// This function is used to clean up after the mixer routine (called from 
// AHI_FreeAudio()).

void
CleanUpMixroutine( struct AHIPrivAudioCtrl *audioctrl )
{
//kprintf( "CleanUpMixroutine\n" );

  RemIntServer( INTB_PORTS, audioctrl->ahiac_PPCMixInterrupt );
  FreeVec( audioctrl->ahiac_PPCMixInterrupt );
  AHIFreeVec( audioctrl->ahiac_PPCMixBuffer );
  AHIFreeVec( audioctrl->ahiac_SoundDatas );
  AHIFreeVec( audioctrl->ahiac_ChannelDatas );
}

/******************************************************************************
** calcMasterVolumeTable ******************************************************
******************************************************************************/

// This function is called each time the master volume changes.

void
calcMasterVolumeTable ( struct AHIPrivAudioCtrl *audioctrl )
{
  // Do nothing, no tables are used!
}


/******************************************************************************
** initSignedTable ************************************************************
******************************************************************************/

// This function sets up the multiplication tables used when mixing signed
// samples.

BOOL
initSignedTable ( struct AHIPrivAudioCtrl *audioctrl )
{
  // No tables are used, return success.
  return TRUE;
}


/******************************************************************************
** calcSignedTable ************************************************************
******************************************************************************/

// This function is called each time the master volume changes

void
calcSignedTable ( struct AHIPrivAudioCtrl *audioctrl )
{
  // Do nothing, no tables are used!
}


/******************************************************************************
** initUnsignedTable **********************************************************
******************************************************************************/

// This function sets up the multiplication tables used when mixing unsigned
// samples (obsolete since V4, but kept for backward compability with
// Delitracker II).

BOOL
initUnsignedTable ( struct AHIPrivAudioCtrl *audioctrl )
{
  // No tables are used, return success.
  return TRUE;
}


/******************************************************************************
** calcUnsignedTable **********************************************************
******************************************************************************/

// This function is called each time the master volume changes

void
calcUnsignedTable ( struct AHIPrivAudioCtrl *audioctrl )
{
  // Do nothing, no tables are used!
}


/******************************************************************************
** SelectAddRoutine ***********************************************************
******************************************************************************/

// This routine gets called each time there is reason to believe that a new
// add-routine should be used (new sound selected, volume changed,
// mastervolume changed)

// Based on VolumeLeft, VolumeRight and SampleType, fill in ScaleLeft,
// ScaleRight and AddRoutine.

void
SelectAddRoutine ( Fixed     VolumeLeft,
                   Fixed     VolumeRight,
                   ULONG     SampleType,
                   struct    AHIPrivAudioCtrl *audioctrl,
                   LONG     *ScaleLeft,
                   LONG     *ScaleRight,
                   ADDFUNC **AddRoutine )

{
  // This version only cares about the sample format and does not use any
  // optimized add-routines.

  // Scale the volume

  VolumeLeft  = VolumeLeft  * (audioctrl->ahiac_MasterVolume >> 8) / 
                              (audioctrl->ahiac_Channels2 << 8);

  VolumeRight = VolumeRight * (audioctrl->ahiac_MasterVolume >> 8) / 
                              (audioctrl->ahiac_Channels2 << 8);

  // Now, check the output format...

  switch(audioctrl->ac.ahiac_BuffType)
  {

    case AHIST_M32S:

      // ...and then the source format.

      switch(SampleType)
      {
        case AHIST_M8S:
        case AHIST_BW|AHIST_M8S:
          *ScaleLeft  = VolumeLeft + VolumeRight;
          *ScaleRight = 0;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddByteMVHBPtr;
          else
            *AddRoutine = AddByteMVHPtr;
          break;

        case AHIST_S8S:
        case AHIST_BW|AHIST_S8S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddBytesMVHBPtr;
          else
            *AddRoutine = AddBytesMVHPtr;
          break;

        case AHIST_M16S:
        case AHIST_BW|AHIST_M16S:
          *ScaleLeft  = VolumeLeft + VolumeRight;
          *ScaleRight = 0;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddWordMVHBPtr;
          else
            *AddRoutine = AddWordMVHPtr;
          break;

        case AHIST_S16S:
        case AHIST_BW|AHIST_S16S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddWordsMVHBPtr;
          else
            *AddRoutine = AddWordsMVHPtr;
          break;

        default:
          *ScaleLeft  = 0;
          *ScaleRight = 0;
          *AddRoutine = NULL;
          break;
      }
      break;

    case AHIST_S32S:

      // ...and then the source format.

      switch(SampleType)
      {
        case AHIST_M8S:
        case AHIST_BW|AHIST_M8S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddByteSVPHBPtr;
          else
            *AddRoutine = AddByteSVPHPtr;
          break;

        case AHIST_S8S:
        case AHIST_BW|AHIST_S8S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddBytesSVPHBPtr;
          else
            *AddRoutine = AddBytesSVPHPtr;
          break;

        case AHIST_M16S:
        case AHIST_BW|AHIST_M16S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddWordSVPHBPtr;
          else
            *AddRoutine = AddWordSVPHPtr;
          break;

        case AHIST_S16S:
        case AHIST_BW|AHIST_S16S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = AddWordsSVPHBPtr;
          else
            *AddRoutine = AddWordsSVPHPtr;
          break;

        default:
          *ScaleLeft  = 0;
          *ScaleRight = 0;
          *AddRoutine = NULL;
          break;
      }
      break;

    default:
      *ScaleLeft  = 0;
      *ScaleRight = 0;
      *AddRoutine = NULL;
      break;
  }

  if(*AddRoutine != NULL && *ScaleLeft == 0 && *ScaleRight == 0)
  {
    if(SampleType & AHIST_BW)
      *AddRoutine = AddSilenceBPtr;
    else
      *AddRoutine = AddSilencePtr;
  }
}

#endif /* !defined( VERSIONPPC ) */


/******************************************************************************
** Mix ************************************************************************
******************************************************************************/

// This is the function that the driver calls each time it want more data
// to play. 

// There is a stub function in asmfuncs.s called Mix() that saves d0-d1/a0-a1
// and calls MixGeneric. This stub is only assembled if VERSIONGEN is set.

#if !defined( VERSIONPPC )
void ASMCALL
MixGeneric ( REG(a0, struct Hook *Hook), 
             REG(a1, void *dst), 
             REG(a2, struct AHIPrivAudioCtrl *audioctrl) )
#else
void
MixGeneric ( struct Hook *Hook, 
             void *dst, 
             struct AHIPrivAudioCtrl *audioctrl )
#endif
{
  struct AHIChannelData	*cd;
  void                  *dstptr;
  LONG                   samplesleft;
//kprintf(".");
  /* Clear the buffer */

  memset( dst, 0, audioctrl->ahiac_BuffSizeNow );

  /* Mix the samples */

  audioctrl->ahiac_WetOrDry = AHIEDM_WET;

  cd = audioctrl->ahiac_WetList;

  while(TRUE)
  {
    while(cd != NULL) // .nextchannel
    {
      samplesleft = audioctrl->ac.ahiac_BuffSamples;
      dstptr      = dst;

      while(TRUE) // .contchannel
      {
        LONG samples;
        LONG processed;

        /* Call Sound Hook */

        if(cd->cd_EOS)
        {
          cd->cd_EOS = FALSE;
          if(audioctrl->ac.ahiac_SoundFunc != NULL)
          {
            CallSoundHook( audioctrl, &cd->cd_ChannelNo );
          }
        }

        processed = 0;

        if( cd->cd_AntiClickCount > 0 && cd->cd_FreqOK && cd->cd_SoundOK )
        {
          // Sound is ok and we're looking for a zero-crossing.

          LONG try_samples;

          samples     = min( samplesleft, cd->cd_Samples );
          try_samples = min( samples, cd->cd_AntiClickCount );

//kprintf("<search %ld = min( %ld, %ld )> ",try_samples, samples, cd->cd_AntiClickCount );
          processed = ((ADDFUNC *) cd->cd_AddRoutine)( try_samples,
                                                       cd->cd_ScaleLeft,
                                                       cd->cd_ScaleRight,
                                                      &cd->cd_Offset, 
                                                       cd->cd_Add,
                                                       audioctrl,
                                                       cd->cd_DataStart,
                                                      &dstptr,
                                                       cd,
                                                       TRUE );
          cd->cd_Samples -= processed;
          samplesleft    -= processed;
//kprintf("<searched %ld> ", processed);

          if( try_samples == cd->cd_AntiClickCount ||
              processed != samples )
          {
            // We either found a zero-crossing or looked as far as
            // we were allowed to.

            // Note that the sample end was NOT reached! If it was,
            // cd_Samples will be zero and the second cd_AddRoutine
            // call below will have no effect, and the cd_Next#?
            // variables will be copied instead.

            // Now start the delayed sound.

            if( cd->cd_VolDelayed )
            {
              cd->cd_VolDelayed = FALSE;
//kprintf("<vol> ");
              cd->cd_VolumeLeft  = cd->cd_DelayedVolumeLeft;
              cd->cd_VolumeRight = cd->cd_DelayedVolumeRight;
              cd->cd_ScaleLeft   = cd->cd_DelayedScaleLeft;
              cd->cd_ScaleRight  = cd->cd_DelayedScaleRight;
              cd->cd_AddRoutine  = cd->cd_DelayedAddRoutine;
            }

            if( cd->cd_FreqDelayed )
            {
              cd->cd_FreqDelayed = FALSE;
//kprintf("<freq> ");
              cd->cd_FreqOK      = cd->cd_DelayedFreqOK;
              cd->cd_Add         = cd->cd_DelayedAdd;
              
              // Since we have advanced, cd_Samples must be recalculated!
              cd->cd_Samples     = CalcSamples( cd->cd_Add,
                                                cd->cd_Type,
                                                cd->cd_LastOffset,
                                                cd->cd_Offset );
            }

            if( cd->cd_SoundDelayed )
            {
              cd->cd_SoundDelayed = FALSE;
//kprintf("<sound> ");
              cd->cd_Offset        = cd->cd_DelayedOffset;
              cd->cd_FirstOffsetI  = cd->cd_DelayedFirstOffsetI;
              cd->cd_LastOffset    = cd->cd_DelayedLastOffset;
              cd->cd_DataStart     = cd->cd_DelayedDataStart;
              cd->cd_Type          = cd->cd_DelayedType;
              cd->cd_SoundOK       = cd->cd_DelayedSoundOK;
              cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;
              cd->cd_Samples       = cd->cd_DelayedSamples;
              cd->cd_ScaleLeft     = cd->cd_DelayedScaleLeft;
              cd->cd_ScaleRight    = cd->cd_DelayedScaleRight;
              cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;
            }

//            cd->cd_AntiClickCount = 0;
//kprintf("<new %ld %ld> ", cd->cd_Samples, cd->cd_AntiClickCount );
          }

          if( cd->cd_VolDelayed || cd->cd_FreqDelayed || cd->cd_SoundDelayed )
          {
            cd->cd_AntiClickCount -= processed;
          }
          else
          {
            cd->cd_AntiClickCount = 0;
          }
//kprintf("acc: %ld\n",cd->cd_AntiClickCount);
        }

        if( cd->cd_FreqOK && cd->cd_SoundOK )
        {
          // Sound is still ok, let's rock'n roll.

          samples = min( samplesleft, cd->cd_Samples );
//kprintf("<mixing %ld: %08lx> ", samples, cd->cd_AddRoutine);
//kprintf("<%ld, %lx, %lx, %08lx, %08lx> ", samples, cd->cd_ScaleLeft, cd->cd_ScaleRight, cd->cd_DataStart,dstptr );
          processed = ((ADDFUNC *) cd->cd_AddRoutine)( samples,
                                                       cd->cd_ScaleLeft,
                                                       cd->cd_ScaleRight,
                                                      &cd->cd_Offset, 
                                                       cd->cd_Add,
                                                       audioctrl,
                                                       cd->cd_DataStart,
                                                      &dstptr,
                                                       cd,
                                                       FALSE );
//kprintf("<mixed %ld> ", processed );
          cd->cd_Samples -= processed;
          samplesleft    -= processed;

          if( cd->cd_Samples == 0 )
          {
            /* Linear interpol. stuff */

            cd->cd_StartPointL = cd->cd_TempStartPointL;
            cd->cd_StartPointR = cd->cd_TempStartPointR;
//kprintf("1 ");
            /*
            ** Offset always points OUTSIDE the sample after this
            ** call.  Ie, if we read a sample at offset (Offset.I)
            ** now, it does not belong to the sample just played.
            ** This is true for both backward and forward mixing.
            */


            /* What we do now is to calculate how much futher we have
               advanced. */

              cd->cd_Offset -= cd->cd_LastOffset;

            /*
            ** Offset should now be added to the NEXT Offset. Offset
            ** is positive of the sample was mixed forwards, and
            ** negative if the sample was mixed backwards.  There is
            ** one catch, however.  If the direction is about to
            ** change now, Offset should instead be SUBTRACTED.
            ** Let's check:
            */

            if( (cd->cd_Type ^ cd->cd_NextType) & AHIST_BW )
            {
              cd->cd_Offset = -cd->cd_Offset;
            }

            cd->cd_Offset += cd->cd_NextOffset;

            cd->cd_FirstOffsetI = cd->cd_Offset >> 32;

            /*
            ** But what if the next sample is so short that we just
            ** passed it!?  Here is the nice part.  CalcSamples
            ** checks this, and sets cd_Samples to 0 in that case.
            ** And the add routines doesn't do anything when asked to
            ** mix 0 samples.  Assume we have passed a sample with 4
            ** samples, and the next one is only 3.  CalcSamples
            ** returns 0.  The (ADDFUNC) call above does not do
            ** anything at all, OffsetI is still 4.  Now we subtract
            ** LastOffsetI, which is 3.  Result:  We have passed the
            ** sample with 1.  And guess what?  That's in range.
            */

            /* Now, let's copy the rest of the cd_Next#? stuff... */

            cd->cd_FreqOK        = cd->cd_NextFreqOK;
            cd->cd_SoundOK       = cd->cd_NextSoundOK;
            cd->cd_Add           = cd->cd_NextAdd;
            cd->cd_DataStart     = cd->cd_NextDataStart;
            cd->cd_LastOffset    = cd->cd_NextLastOffset;
            cd->cd_ScaleLeft     = cd->cd_NextScaleLeft;
            cd->cd_ScaleRight    = cd->cd_NextScaleRight;
            cd->cd_AddRoutine    = cd->cd_NextAddRoutine;
            cd->cd_VolumeLeft    = cd->cd_NextVolumeLeft;
            cd->cd_VolumeRight   = cd->cd_NextVolumeRight;
            cd->cd_Type          = cd->cd_NextType;

            cd->cd_Samples = CalcSamples( cd->cd_Add,
                                          cd->cd_Type,
                                          cd->cd_LastOffset,
                                          cd->cd_Offset );

            /* Also update all cd_Delayed#? stuff */

            cd->cd_DelayedFreqOK        = cd->cd_NextFreqOK;
            cd->cd_DelayedSoundOK       = cd->cd_NextSoundOK;
            cd->cd_DelayedDataStart     = cd->cd_NextDataStart;
            cd->cd_DelayedOffset        = cd->cd_NextOffset;
            cd->cd_DelayedAdd           = cd->cd_NextAdd;
            cd->cd_DelayedLastOffset    = cd->cd_NextLastOffset;
            cd->cd_DelayedScaleLeft     = cd->cd_NextScaleLeft;
            cd->cd_DelayedScaleRight    = cd->cd_NextScaleRight;
            cd->cd_DelayedVolumeLeft    = cd->cd_NextVolumeLeft;
            cd->cd_DelayedVolumeRight   = cd->cd_NextVolumeRight;
            cd->cd_DelayedAddRoutine    = cd->cd_NextAddRoutine;
            cd->cd_DelayedType          = cd->cd_NextType;

            cd->cd_DelayedSamples       = cd->cd_Samples;
            cd->cd_DelayedFirstOffsetI  = cd->cd_FirstOffsetI;

            cd->cd_EOS = TRUE;      // signal End-Of-Sample
            continue;               // .contchannel (same channel, new sound)
          }
        } // FreqOK && SoundOK

        break; // .contchannel

      } // while(TRUE)

      cd = cd->cd_Succ;
    } // while(cd)

    if(audioctrl->ahiac_WetOrDry == AHIEDM_WET)
    {
      audioctrl->ahiac_WetOrDry = AHIEDM_DRY;

      /*** AHIET_DSPECHO ***/
      if(audioctrl->ahiac_EffDSPEchoStruct != NULL)
      {
        audioctrl->ahiac_EffDSPEchoStruct->ahiecho_Code(
            audioctrl->ahiac_EffDSPEchoStruct, dst, audioctrl);
      }

      cd = audioctrl->ahiac_DryList;

      if(audioctrl->ac.ahiac_Flags & AHIACF_POSTPROC)
      {
        /*** AHIET_MASTERVOLUME ***/

        DoMasterVolume(dst, audioctrl);

        /*
        ** When AHIACB_POSTPROC is set, the dry data shall be placed
        ** immediate after the wet data. This is done by modifying the
        ** dst pointer
        */

        dst = (char *) dst + audioctrl->ac.ahiac_BuffSamples * 
                             SampleFrameSize(audioctrl->ac.ahiac_BuffType);
      }

      continue; /* while(TRUE) */
    }
    else
    {
      break; /* while(TRUE) */
    }
  } // while(TRUE)

  /*** AHIET_MASTERVOLUME ***/

  DoMasterVolume(dst, audioctrl);

#if !defined( VERSIONPPC )

  // This is handled in m68k code, in order to minimize cache flushes.

  /*** AHIET_OUTPUTBUFFER ***/

  DoOutputBuffer(dst, audioctrl);

  /*** AHIET_CHANNELINFO ***/

  DoChannelInfo(audioctrl);

#endif /* !defined( VERSIONPPC ) */

  return;
}

/*
** This function would be better if it was written in assembler,
** since overflow could then be detected. Instead we reduce the
** number of bits to 20 and then scale and compare.
*/

static void
DoMasterVolume ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl )
{
  LONG *dst = buffer;
  int   cnt;
  LONG  vol;
  LONG  sample;

  cnt = audioctrl->ac.ahiac_BuffSamples;

  switch(audioctrl->ac.ahiac_BuffType)
  {

    case AHIST_M32S:
      break;

    case AHIST_S32S:
      cnt *= 2;
      break;

    default:
      return; // Panic
  }

  vol = audioctrl->ahiac_SetMasterVolume >> 8;

  while(cnt > 0)
  {
    cnt--;
    
    sample = (*dst >> 12) * vol;

    if(sample > (LONG) 0x07ffffff)
      sample = 0x07ffffff;
    else if(sample < (LONG) 0xf8000000)
      sample = 0xf8000000;

    *dst++ = sample << 4;
  }
}


#if !defined( VERSIONPPC )

static void
DoOutputBuffer ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHIEffOutputBuffer *ob;

  ob = audioctrl->ahiac_EffOutputBufferStruct;

  if(ob != NULL)
  {
    ob->ahieob_Buffer = buffer;
    ob->ahieob_Length = audioctrl->ac.ahiac_BuffSamples;
    ob->ahieob_Type   = audioctrl->ac.ahiac_BuffType;

    CallHookPkt( ob->ahieob_Func,
                 audioctrl,
                 ob);
  }
}

static void
DoChannelInfo ( struct AHIPrivAudioCtrl *audioctrl )
{
  struct AHIEffChannelInfo *ci;
  struct AHIChannelData    *cd;
  ULONG                    *offsets;

  ci = audioctrl->ahiac_EffChannelInfoStruct;

  if(ci != NULL)
  {
    int i;
    
    cd      = audioctrl->ahiac_ChannelDatas;
    offsets = ci->ahieci_Offset;

    for(i = ci->ahieci_Channels; i > 0; i--)
    {
      *offsets++ = cd->cd_Offset >> 32;
      cd++;
    }
    
    CallHookPkt( ci->ahieci_Func,
                 audioctrl,
                 ci );
  }
}

#endif /* !defined( VERSIONPPC ) */


/******************************************************************************
** CalcSamples ****************************************************************
******************************************************************************/

LONG
CalcSamples ( Fixed64 Add,
              ULONG   Type,
              Fixed64 LastOffset,
              Fixed64 Offset )

{
  Fixed64 len;

  if(Type & AHIST_BW)
  {
    len = Offset - LastOffset; 
  }
  else
  {
    len = LastOffset - Offset;
  }

  if(len < 0 || Add == 0) return 0; // Error!

  return (LONG) (len / Add) + 1;
}


/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/*
** LONG      Samples
** Fixed     ScaleLeft
** Fixed     ScaleRight (Not used in all routines)
** Fixed64  *Offset
** Fixed64   Add
** struct    AHIPrivAudioCtrl *audioctrl
** void     *Src
** void    **Dst
** struct    AHIChannelData *cd
*/

/*****************************************************************************/

LONG
AddSilence( ADDARGS )
{
  double offset, add;

  if( StopAtZero ) return 0;

  *Offset += Samples * Add;

  *Dst    = (char *) *Dst + Samples * SampleFrameSize(audioctrl->ac.ahiac_BuffType);
  
  return Samples;
}

LONG
AddSilenceB( ADDARGS )
{
  double offset, add;

  if( StopAtZero ) return 0;

  *Offset -= Samples * Add;

  *Dst    = (char *) *Dst + Samples * SampleFrameSize(audioctrl->ac.ahiac_BuffType);
  
  return Samples;
}


/*****************************************************************************/

/*

Notes:

The fraction offset is divided by two in order to make sure that the
calculation of linearsample fits a LONG (0 =< offsetf <= 32767).

The routines could be faster, of course.  One idea is to split the for loop
into two loops in order to eliminate the FirstOffsetI test in the second loop.

*/ 

/*****************************************************************************/

/* Forward mixing code */

#define offseti ( (long) ( offset >> 32 ) )

#define offsetf ( (long) ( (unsigned long) ( offset & 0xffffffffULL ) >> 17) )

LONG
AddByteMVH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddByteSVPH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesMVH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 - 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesSVPH( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 - 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordMVH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordSVPH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti - 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

LONG
AddWordsMVH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsSVPH( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 - 2 ];
      startpointR = src[ offseti * 2 + 1 - 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset += Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}

#undef offsetf

/*****************************************************************************/

/* Backward mixing code */

#define offsetf ( (long) ( 32768 - ( (unsigned long) ( offset & 0xffffffffULL ) >> 17 ) ) )

LONG
AddByteMVHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddByteSVPHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ] << 8;
    }

    endpoint = src[ offseti ] << 8;

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesMVHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 + 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddBytesSVPHB( ADDARGS )
{
  BYTE    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ] << 8;
      startpointR = src[ offseti * 2 + 1 + 2 ] << 8;
    }

    endpointL = src[ offseti * 2 + 0 ] << 8;
    endpointR = src[ offseti * 2 + 1 ] << 8;

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordMVHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordSVPHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpoint, endpoint;
  LONG     lastpoint;

  endpoint  = cd->cd_TempStartPointL;
  lastpoint = 0;                      // 0 doesn't affect the StopAtZero code
  
  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpoint = cd->cd_StartPointL;
    }
    else
    {
      startpoint = src[ offseti + 1 ];
    }

    endpoint = src[ offseti ];

    startpoint += (((endpoint - startpoint) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpoint < 0 && startpoint >= 0 ) ||
          ( lastpoint > 0 && startpoint <= 0 ) ) )
    {
      break;
    }

    lastpoint = startpoint;

    *dst++ += ScaleLeft * startpoint;
    *dst++ += ScaleRight * startpoint;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpoint;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsMVHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL + ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


LONG
AddWordsSVPHB( ADDARGS )
{
  WORD    *src    = Src;
  LONG    *dst    = *Dst;
  Fixed64  offset = *Offset;
  int      i;
  LONG     startpointL, startpointR, endpointL, endpointR;
  LONG     lastpointL, lastpointR;

  endpointL  = cd->cd_TempStartPointL;
  endpointR  = cd->cd_TempStartPointR;
  lastpointL = lastpointR = 0;        // 0 doesn't affect the StopAtZero code

  for( i = 0; i < Samples; i++)
  {
    if( offseti == cd->cd_FirstOffsetI) {
      startpointL = cd->cd_StartPointL;
      startpointR = cd->cd_StartPointR;
    }
    else
    {
      startpointL = src[ offseti * 2 + 0 + 2 ];
      startpointR = src[ offseti * 2 + 1 + 2 ];
    }

    endpointL = src[ offseti * 2 + 0 ];
    endpointR = src[ offseti * 2 + 1 ];

    startpointL += (((endpointL - startpointL) * offsetf ) >> 15);
    startpointR += (((endpointR - startpointR) * offsetf ) >> 15);

    if( StopAtZero &&
        ( ( lastpointL < 0 && startpointL >= 0 ) ||
          ( lastpointR < 0 && startpointR >= 0 ) ||
          ( lastpointL > 0 && startpointL <= 0 ) ||
          ( lastpointR > 0 && startpointR <= 0 ) ) )
    {
      break;
    }

    lastpointL = startpointL;
    lastpointR = startpointR;

    *dst++ += ScaleLeft * startpointL;
    *dst++ += ScaleRight * startpointR;

    offset -= Add;
  }

  cd->cd_TempStartPointL = endpointL;
  cd->cd_TempStartPointR = endpointR;

  *Dst    = dst;
  *Offset = offset;

  return i;
}


#undef offseti
#undef offsetf

/*****************************************************************************/
