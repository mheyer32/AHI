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

#include <exec/interrupts.h>
#include <hardware/intbits.h>

#if !defined( VERSIONPPC )
# include <string.h>
# include <stddef.h>

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
# include <inline/ahi.h>
#endif

#include "ahi_def.h"
#include "dsp.h"
#include "mixer.h"
#include "addroutines.h"
#include "misc.h"
#include "header.h"

#define GetSymbol( name ) AHIGetELFSymbol( #name, (void*) &name ## Ptr )

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

ADDFUNC* AddByteMonoPtr     = NULL;
ADDFUNC* AddByteStereoPtr   = NULL;
ADDFUNC* AddBytesMonoPtr    = NULL;
ADDFUNC* AddBytesStereoPtr  = NULL;
ADDFUNC* AddWordMonoPtr     = NULL;
ADDFUNC* AddWordStereoPtr   = NULL;
ADDFUNC* AddWordsMonoPtr    = NULL;
ADDFUNC* AddWordsStereoPtr  = NULL;
ADDFUNC* AddByteMonoBPtr    = NULL;
ADDFUNC* AddByteStereoBPtr  = NULL;
ADDFUNC* AddBytesMonoBPtr   = NULL;
ADDFUNC* AddBytesStereoBPtr = NULL;
ADDFUNC* AddWordMonoBPtr    = NULL;
ADDFUNC* AddWordStereoBPtr  = NULL;
ADDFUNC* AddWordsMonoBPtr   = NULL;
ADDFUNC* AddWordsStereoBPtr = NULL;

ADDFUNC* AddLofiByteMonoPtr     = NULL;
ADDFUNC* AddLofiByteStereoPtr   = NULL;
ADDFUNC* AddLofiBytesMonoPtr    = NULL;
ADDFUNC* AddLofiBytesStereoPtr  = NULL;
ADDFUNC* AddLofiWordMonoPtr     = NULL;
ADDFUNC* AddLofiWordStereoPtr   = NULL;
ADDFUNC* AddLofiWordsMonoPtr    = NULL;
ADDFUNC* AddLofiWordsStereoPtr  = NULL;
ADDFUNC* AddLofiByteMonoBPtr    = NULL;
ADDFUNC* AddLofiByteStereoBPtr  = NULL;
ADDFUNC* AddLofiBytesMonoBPtr   = NULL;
ADDFUNC* AddLofiBytesStereoBPtr = NULL;
ADDFUNC* AddLofiWordMonoBPtr    = NULL;
ADDFUNC* AddLofiWordStereoBPtr  = NULL;
ADDFUNC* AddLofiWordsMonoBPtr   = NULL;
ADDFUNC* AddLofiWordsStereoBPtr = NULL;


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

inline ULONG
InternalSampleFrameSize( ULONG sampletype )
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

static void*
memset( void* s, int c, unsigned int n )
{
  char* dst = s;

  while( n > 0 )
  {
    *dst++ = c;
    n--;
  }

  return s;
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

    while( running )
    {
      switch( audioctrl->ahiac_PPCCommand )
      {
        case AHIAC_COM_INIT:
          // Keep looping
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_ACK:
          // Keep looping, try not to waste to much memory bandwidth...
          asm( "stop #(1<<13) | (2<<8)" : );
          break;

        case AHIAC_COM_SOUNDFUNC:
          CallHookPkt( audioctrl->ac.ahiac_SoundFunc,
                       (struct AHIPrivAudioCtrl*) audioctrl,
                       (APTR) audioctrl->ahiac_PPCArgument );
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_DEBUG:
          CallDebug( audioctrl, (ULONG) audioctrl->ahiac_PPCArgument );
          audioctrl->ahiac_PPCCommand = AHIAC_COM_ACK;
          break;

        case AHIAC_COM_QUIT:
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

    /* End chain! */
    return 1;
  }
};

void ASMCALL
MixPowerUp( REG(a0, struct Hook *Hook), 
            REG(a1, void *dst), 
            REG(a2, struct AHIPrivAudioCtrl *audioctrl) )
{
  struct AHISoundData *sd;
  int                  i;
  BOOL                 flushed = FALSE;

  struct ModuleArgs mod =
  {
    IF_CACHEFLUSHNO, 0, 0,
    IF_CACHEFLUSHNO | IF_ASYNC, 0, 0,

    0xC0DECAFE,
    (ULONG) Hook, (ULONG) audioctrl->ahiac_PPCMixBuffer, (ULONG) audioctrl,
    0, 0, 0, 0,
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
  };

  // Flush all DYNAMICSAMPLE's

  sd = audioctrl->ahiac_SoundDatas;

  for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
  {
    if( sd->sd_Type == AHIST_DYNAMICSAMPLE )
    {
      if( sd->sd_Addr == NULL )
      {
        // Flush all and exit
        CacheClearU();
        flushed = TRUE;
        break;
      }
      else
      {
        if( PPCLibBase != NULL )
        {
          PPCCacheClearE( sd->sd_Addr,
                          sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ),
                          CACRF_ClearD );
        }
        else
        {
          SetCache68K( CACHE_DCACHEFLUSH,
                       sd->sd_Addr,
                       sd->sd_Length * InternalSampleFrameSize( sd->sd_Type ) );
        }
      }
    }
    sd++;
  }

  if( ! flushed && PPCLibBase == NULL )
  {

    /* Since the PPC mix buffer is m68k cacheable in WarpUp, we have to
       flush the cache before mixing starts. :( */

    SetCache68K( CACHE_DCACHEFLUSH,
                 audioctrl->ahiac_PPCMixBuffer,
                 audioctrl->ahiac_BuffSizeNow );
  }


  audioctrl->ahiac_PPCCommand = AHIAC_COM_NONE;

  if( PPCLibBase != NULL )
  {
    PPCRunKernelObject( PPCObject, &mod );
  }
  else
  {
//kprintf( "." );
//    CausePPCInterrupt();
  }

  audioctrl->ahiac_PPCCommand = AHIAC_COM_START;
  while( audioctrl->ahiac_PPCCommand != AHIAC_COM_FINISHED );

  // The PPC mix buffer is not m68k-cachable (or cleared); just read from it.

  memcpy( dst, audioctrl->ahiac_PPCMixBuffer, audioctrl->ahiac_BuffSizeNow );

  /*** AHIET_OUTPUTBUFFER ***/

  DoOutputBuffer(dst, audioctrl);

  /*** AHIET_CHANNELINFO ***/

  DoChannelInfo(audioctrl);

  return;
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
      sd++;
    }

    if( PPCObject != NULL )
    {
      int r = ~0;

      audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Type = NT_INTERRUPT;
      audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Pri  = 127;
      audioctrl->ahiac_PPCMixInterrupt->is_Node.ln_Name = (STRPTR) DevName;
      audioctrl->ahiac_PPCMixInterrupt->is_Data         = audioctrl;
      audioctrl->ahiac_PPCMixInterrupt->is_Code         = (void(*)(void)) Interrupt;

      AddIntServer( INTB_PORTS, audioctrl->ahiac_PPCMixInterrupt );

      r &= GetSymbol( AddByteMono     );
      r &= GetSymbol( AddByteStereo   );
      r &= GetSymbol( AddBytesMono    );
      r &= GetSymbol( AddBytesStereo  );
      r &= GetSymbol( AddWordMono     );
      r &= GetSymbol( AddWordStereo   );
      r &= GetSymbol( AddWordsMono    );
      r &= GetSymbol( AddWordsStereo  );
      r &= GetSymbol( AddByteMonoB    );
      r &= GetSymbol( AddByteStereoB  );
      r &= GetSymbol( AddBytesMonoB   );
      r &= GetSymbol( AddBytesStereoB );
      r &= GetSymbol( AddWordMonoB    );
      r &= GetSymbol( AddWordStereoB  );
      r &= GetSymbol( AddWordsMonoB   );
      r &= GetSymbol( AddWordsStereoB );

      r &= GetSymbol( AddLofiByteMono     );
      r &= GetSymbol( AddLofiByteStereo   );
      r &= GetSymbol( AddLofiBytesMono    );
      r &= GetSymbol( AddLofiBytesStereo  );
      r &= GetSymbol( AddLofiWordMono     );
      r &= GetSymbol( AddLofiWordStereo   );
      r &= GetSymbol( AddLofiWordsMono    );
      r &= GetSymbol( AddLofiWordsStereo  );
      r &= GetSymbol( AddLofiByteMonoB    );
      r &= GetSymbol( AddLofiByteStereoB  );
      r &= GetSymbol( AddLofiBytesMonoB   );
      r &= GetSymbol( AddLofiBytesStereoB );
      r &= GetSymbol( AddLofiWordMonoB    );
      r &= GetSymbol( AddLofiWordStereoB  );
      r &= GetSymbol( AddLofiWordsMonoB   );
      r &= GetSymbol( AddLofiWordsStereoB );

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

          if( AHIGetELFSymbol( "WarpInit", &args.PP_Code ) )
          {
            int status;

//            status = RunPPC( &args );
            
//            if( status == PPERR_SUCCESS )
            {
              rc = TRUE;
            }
          }

        }
      }
      else
      {
        Req( "Unable to fetch all symbols from ELF object." );
      }
    }
    else // PPCObject
    {
      AddByteMonoPtr         = AddByteMono;
      AddByteStereoPtr       = AddByteStereo;
      AddBytesMonoPtr        = AddBytesMono;
      AddBytesStereoPtr      = AddBytesStereo;
      AddWordMonoPtr         = AddWordMono;
      AddWordStereoPtr       = AddWordStereo;
      AddWordsMonoPtr        = AddWordsMono;
      AddWordsStereoPtr      = AddWordsStereo;
      AddByteMonoBPtr        = AddByteMonoB;
      AddByteStereoBPtr      = AddByteStereoB;
      AddBytesMonoBPtr       = AddBytesMonoB;
      AddBytesStereoBPtr     = AddBytesStereoB;
      AddWordMonoBPtr        = AddWordMonoB;
      AddWordStereoBPtr      = AddWordStereoB;
      AddWordsMonoBPtr       = AddWordsMonoB;
      AddWordsStereoBPtr     = AddWordsStereoB;

      AddLofiByteMonoPtr     = AddLofiByteMono;
      AddLofiByteStereoPtr   = AddLofiByteStereo;
      AddLofiBytesMonoPtr    = AddLofiBytesMono;
      AddLofiBytesStereoPtr  = AddLofiBytesStereo;
      AddLofiWordMonoPtr     = AddLofiWordMono;
      AddLofiWordStereoPtr   = AddLofiWordStereo;
      AddLofiWordsMonoPtr    = AddLofiWordsMono;
      AddLofiWordsStereoPtr  = AddLofiWordsStereo;
      AddLofiByteMonoBPtr    = AddLofiByteMonoB;
      AddLofiByteStereoBPtr  = AddLofiByteStereoB;
      AddLofiBytesMonoBPtr   = AddLofiBytesMonoB;
      AddLofiBytesStereoBPtr = AddLofiBytesStereoB;
      AddLofiWordMonoBPtr    = AddLofiWordMonoB;
      AddLofiWordStereoBPtr  = AddLofiWordStereoB;
      AddLofiWordsMonoBPtr   = AddLofiWordsMonoB;
      AddLofiWordsStereoBPtr = AddLofiWordsStereoB;

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
  if( audioctrl->ahiac_PPCMixInterrupt != NULL )
  {
    RemIntServer( INTB_PORTS, audioctrl->ahiac_PPCMixInterrupt );
  }
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

  // First, select HiFi or LoFi...

  if( audioctrl->ac.ahiac_Flags & AHIACF_HIFI )
  {

    // Then, check the output format...

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
              *AddRoutine = AddByteMonoBPtr;
            else
              *AddRoutine = AddByteMonoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddBytesMonoBPtr;
            else
              *AddRoutine = AddBytesMonoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordMonoBPtr;
            else
              *AddRoutine = AddWordMonoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordsMonoBPtr;
            else
              *AddRoutine = AddWordsMonoPtr;
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
              *AddRoutine = AddByteStereoBPtr;
            else
              *AddRoutine = AddByteStereoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddBytesStereoBPtr;
            else
              *AddRoutine = AddBytesStereoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordStereoBPtr;
            else
              *AddRoutine = AddWordStereoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddWordsStereoBPtr;
            else
              *AddRoutine = AddWordsStereoPtr;
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
  }
  else
  {

    // Then, check the output format...

    switch(audioctrl->ac.ahiac_BuffType)
    {

      case AHIST_M16S:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiByteMonoBPtr;
            else
              *AddRoutine = AddLofiByteMonoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiBytesMonoBPtr;
            else
              *AddRoutine = AddLofiBytesMonoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft + VolumeRight;
            *ScaleRight = 0;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordMonoBPtr;
            else
              *AddRoutine = AddLofiWordMonoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordsMonoBPtr;
            else
              *AddRoutine = AddLofiWordsMonoPtr;
            break;

          default:
            *ScaleLeft  = 0;
            *ScaleRight = 0;
            *AddRoutine = NULL;
            break;
        }
        break;

      case AHIST_S16S:

        // ...and then the source format.

        switch(SampleType)
        {
          case AHIST_M8S:
          case AHIST_BW|AHIST_M8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiByteStereoBPtr;
            else
              *AddRoutine = AddLofiByteStereoPtr;
            break;

          case AHIST_S8S:
          case AHIST_BW|AHIST_S8S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiBytesStereoBPtr;
            else
              *AddRoutine = AddLofiBytesStereoPtr;
            break;

          case AHIST_M16S:
          case AHIST_BW|AHIST_M16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordStereoBPtr;
            else
              *AddRoutine = AddLofiWordStereoPtr;
            break;

          case AHIST_S16S:
          case AHIST_BW|AHIST_S16S:
            *ScaleLeft  = VolumeLeft;
            *ScaleRight = VolumeRight;
            if(SampleType & AHIST_BW)
              *AddRoutine = AddLofiWordsStereoBPtr;
            else
              *AddRoutine = AddLofiWordsStereoPtr;
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

          if( try_samples > 0 )
          {
            cd->cd_TempStartPointL = cd->cd_StartPointL;
            cd->cd_TempStartPointR = cd->cd_StartPointR;

            processed = ((ADDFUNC *) cd->cd_AddRoutine)( try_samples,
                                                         cd->cd_ScaleLeft,
                                                         cd->cd_ScaleRight,
                                                        &cd->cd_TempStartPointL,
                                                        &cd->cd_TempStartPointR,
                                                         cd->cd_DataStart,
                                                        &dstptr,
                                                         cd->cd_FirstOffsetI,
                                                         cd->cd_Add,
                                                        &cd->cd_Offset, 
                                                         TRUE );
            cd->cd_Samples -= processed;
            samplesleft    -= processed;
          }
          else
          {
            processed = 0;
          }

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
              cd->cd_VolumeLeft  = cd->cd_DelayedVolumeLeft;
              cd->cd_VolumeRight = cd->cd_DelayedVolumeRight;
              cd->cd_ScaleLeft   = cd->cd_DelayedScaleLeft;
              cd->cd_ScaleRight  = cd->cd_DelayedScaleRight;
              cd->cd_AddRoutine  = cd->cd_DelayedAddRoutine;
            }

            if( cd->cd_FreqDelayed )
            {
              cd->cd_FreqDelayed = FALSE;
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

              cd->cd_SoundOK       = cd->cd_DelayedSoundOK;

              cd->cd_Offset        = cd->cd_DelayedOffset;
              cd->cd_FirstOffsetI  = cd->cd_DelayedFirstOffsetI;
              cd->cd_LastOffset    = cd->cd_DelayedLastOffset;
              cd->cd_DataStart     = cd->cd_DelayedDataStart;

              cd->cd_Type          = cd->cd_DelayedType;
              cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;

              cd->cd_Samples       = cd->cd_DelayedSamples;

              cd->cd_ScaleLeft     = cd->cd_DelayedScaleLeft;
              cd->cd_ScaleRight    = cd->cd_DelayedScaleRight;
              cd->cd_AddRoutine    = cd->cd_DelayedAddRoutine;
            }
          }

          if( cd->cd_VolDelayed || cd->cd_FreqDelayed || cd->cd_SoundDelayed )
          {
            cd->cd_AntiClickCount -= processed;
          }
          else
          {
            cd->cd_AntiClickCount = 0;
          }
        }

        if( cd->cd_FreqOK && cd->cd_SoundOK )
        {
          // Sound is still ok, let's rock'n roll.

          samples = min( samplesleft, cd->cd_Samples );

          if( samples > 0 )
          {
            cd->cd_TempStartPointL = cd->cd_StartPointL;
            cd->cd_TempStartPointR = cd->cd_StartPointR;

            processed = ((ADDFUNC *) cd->cd_AddRoutine)( samples,
                                                         cd->cd_ScaleLeft,
                                                         cd->cd_ScaleRight,
                                                        &cd->cd_TempStartPointL,
                                                        &cd->cd_TempStartPointR,
                                                         cd->cd_DataStart,
                                                        &dstptr,
                                                         cd->cd_FirstOffsetI,
                                                         cd->cd_Add,
                                                        &cd->cd_Offset,
                                                         FALSE );
            cd->cd_Samples -= processed;
            samplesleft    -= processed;
          }

          if( cd->cd_Samples == 0 )
          {
            /* Linear interpol. stuff */

            cd->cd_StartPointL = cd->cd_TempStartPointL;
            cd->cd_StartPointR = cd->cd_TempStartPointR;

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

            if( !cd->cd_VolDelayed )
            {
              cd->cd_DelayedVolumeLeft    = cd->cd_NextVolumeLeft;
              cd->cd_DelayedVolumeRight   = cd->cd_NextVolumeRight;
            }

            if( !cd->cd_FreqDelayed )
            {
              cd->cd_DelayedFreqOK        = cd->cd_NextFreqOK;
              cd->cd_DelayedAdd           = cd->cd_NextAdd;
            }

            if( !cd->cd_SoundDelayed )
            {
              cd->cd_DelayedSoundOK       = cd->cd_NextSoundOK;
              cd->cd_DelayedOffset        = cd->cd_NextOffset;
              cd->cd_DelayedFirstOffsetI  = cd->cd_FirstOffsetI;  // See above
              cd->cd_DelayedLastOffset    = cd->cd_NextLastOffset;
              cd->cd_DelayedType          = cd->cd_NextType;
              cd->cd_DelayedDataStart     = cd->cd_NextDataStart;
            }

            if( !cd->cd_VolDelayed && !cd->cd_SoundDelayed )
            {
              cd->cd_DelayedScaleLeft     = cd->cd_NextScaleLeft;
              cd->cd_DelayedScaleRight    = cd->cd_NextScaleRight;
              cd->cd_DelayedAddRoutine    = cd->cd_NextAddRoutine;
            }

            if( !cd->cd_FreqDelayed && !cd->cd_SoundDelayed )
            {
              cd->cd_DelayedSamples       = cd->cd_Samples;
            }

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
                             InternalSampleFrameSize(audioctrl->ac.ahiac_BuffType);
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
  int   cnt;
  LONG  vol;
  LONG  sample;

  cnt = audioctrl->ac.ahiac_BuffSamples;

  switch(audioctrl->ac.ahiac_BuffType)
  {

    case AHIST_M16S:
    case AHIST_M32S:
      break;

    case AHIST_S16S:
    case AHIST_S32S:
      cnt *= 2;
      break;

    default:
      return; // Panic
  }

  if( audioctrl->ac.ahiac_BuffType == AHIST_M32S
      || audioctrl->ac.ahiac_BuffType == AHIST_S32S )
  {
    LONG *dst = buffer;

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
  else
  {
    WORD *dst = buffer;

    vol = audioctrl->ahiac_SetMasterVolume >> 4;

    while(cnt > 0)
    {
      cnt--;
    
      sample = *dst * vol;

      if(sample > (LONG) 0x07ffffff)
        sample = 0x07ffffff;
      else if(sample < (LONG) 0xf8000000)
        sample = 0xf8000000;

      *dst++ = sample >> 12;
    }
  
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

