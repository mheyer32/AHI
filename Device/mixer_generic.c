/* $Id$
* $Log$
* Revision 4.2  1998/01/12 20:07:28  lcs
* More restruction, mixer in C added. (Just about to make fraction 32 bit!)
*
* Revision 4.1  1997/12/21 17:40:21  lcs
* Rewrote the routines in C and gathered all sound files in one file.
*
*/

#include <CompilerSpecific.h>
#include "ahi_def.h"
#include "dsp.h"

#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/utility.h>

#include <math.h>

#ifndef noprotos

#ifndef _GENPROTO
#include "mixer_generic_protos.h"
#endif

#endif

#define ADDARGS ULONG   Samples,\
                Fixed   ScaleLeft,\
                Fixed   ScaleRight,\
                double *Offset,\
                double  Add,\
                struct  AHIPrivAudioCtrl *audioctrl,\
                void   *Src,\
                void  **Dst,\
                struct  AHIChannelData *cd


static void
DoMasterVolume ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl );

static void
DoOutputBuffer ( void *buffer,
                 struct AHIPrivAudioCtrl *audioctrl );

static void
DoChannelInfo ( struct AHIPrivAudioCtrl *audioctrl );


typedef void (*ADDFUNC)(ADDARGS);

static void AddSilence ( ADDARGS );
static void AddSilenceB ( ADDARGS );

static void AddByteMVH ( ADDARGS );
static void AddByteSVPH ( ADDARGS );
static void AddBytesMVH ( ADDARGS );
static void AddBytesSVPH ( ADDARGS );
static void AddWordMVH ( ADDARGS );
static void AddWordSVPH ( ADDARGS );
static void AddWordsMVH ( ADDARGS );
static void AddWordsSVPH ( ADDARGS );

static void AddByteMVHB ( ADDARGS );
static void AddByteSVPHB ( ADDARGS );
static void AddBytesMVHB ( ADDARGS );
static void AddBytesSVPHB ( ADDARGS );
static void AddWordMVHB ( ADDARGS );
static void AddWordSVPHB ( ADDARGS );
static void AddWordsMVHB ( ADDARGS );
static void AddWordsSVPHB ( ADDARGS );


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

  audioctrl->ahiac_ChannelDatas = AllocVec(
      audioctrl->ac.ahiac_Channels * sizeof(struct AHIChannelData),
      MEMF_PUBLIC|MEMF_CLEAR);
      

  // Now link the list and fill in the channel number for each structure.

  if(audioctrl->ahiac_ChannelDatas != NULL)
  {
    struct AHIChannelData *cd;
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



    // Allocate and initialize the AHISoundData structures

    // This structure could be accessed from from interrupts!

    audioctrl->ahiac_SoundDatas = AllocVec(
        audioctrl->ac.ahiac_Sounds * sizeof(struct AHISoundData),
        MEMF_PUBLIC|MEMF_CLEAR);


    if(audioctrl->ahiac_SoundDatas != NULL)
    {
      struct AHISoundData *sd;

      sd = audioctrl->ahiac_SoundDatas;

      for( i = 0; i < audioctrl->ac.ahiac_Sounds; i++)
      {
        sd->sd_Type = AHIST_NOTYPE;
      }

      // Sucess!
      
      rc = TRUE;
    }
  }

  return rc;
}


/******************************************************************************
** calcMasterVolumeTable ******************************************************
******************************************************************************/

// This function is called each time the master volume changes.

void
calcMasterVolumeTable ( struct AHIPrivAudioCtrl *audioctrl,
                        struct AHIBase *AHIBase )
{
  // Do nothing, no tables are used!
}


/******************************************************************************
** initSignedTable ************************************************************
******************************************************************************/

// This function sets up the multiplication tables used when mixing signed
// samples.

BOOL
initSignedTable ( struct AHIPrivAudioCtrl *audioctrl,
                  struct AHIBase *AHIBase )
{
  // No tables are used, return success.
  return TRUE;
}


/******************************************************************************
** calcSignedTable ************************************************************
******************************************************************************/

// This function is called each time the master volume changes

void
calcSignedTable ( struct AHIPrivAudioCtrl *audioctrl,
                  struct AHIBase *AHIBase )
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
initUnsignedTable ( struct AHIPrivAudioCtrl *audioctrl,
                    struct AHIBase *AHIBase )
{
  // No tables are used, return success.
  return TRUE;
}


/******************************************************************************
** calcUnsignedTable **********************************************************
******************************************************************************/

// This function is called each time the master volume changes

void
calcUnsignedTable ( struct AHIPrivAudioCtrl *audioctrl,
                    struct AHIBase *AHIBase )
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
SelectAddRoutine ( Fixed VolumeLeft,
                   Fixed VolumeRight,
                   ULONG SampleType,
                   struct AHIPrivAudioCtrl *audioctrl,
                   ULONG *ScaleLeft,
                   ULONG *ScaleRight,
                   void **AddRoutine )

{
  // This version only cares about the sample format and does not use any
  // optimized add-routines.


  // First, check the output format...

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
            *AddRoutine = (void *) AddByteMVHB;
          else
            *AddRoutine = (void *) AddByteMVH;
          break;

        case AHIST_S8S:
        case AHIST_BW|AHIST_S8S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = (void *) AddBytesMVHB;
          else
            *AddRoutine = (void *) AddBytesMVH;
          break;

        case AHIST_M16S:
        case AHIST_BW|AHIST_M16S:
          *ScaleLeft  = VolumeLeft + VolumeRight;
          *ScaleRight = 0;
          if(SampleType & AHIST_BW)
            *AddRoutine = (void *) AddWordMVHB;
          else
            *AddRoutine = (void *) AddWordMVH;
          break;

        case AHIST_S16S:
        case AHIST_BW|AHIST_S16S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = (void *) AddWordsMVHB;
          else
            *AddRoutine = (void *) AddWordsMVH;
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
            *AddRoutine = (void *) AddByteSVPHB;
          else
            *AddRoutine = (void *) AddByteSVPH;
          break;

        case AHIST_S8S:
        case AHIST_BW|AHIST_S8S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = (void *) AddBytesSVPHB;
          else
            *AddRoutine = (void *) AddBytesSVPH;
          break;

        case AHIST_M16S:
        case AHIST_BW|AHIST_M16S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = (void *) AddWordSVPHB;
          else
            *AddRoutine = (void *) AddWordSVPH;
          break;

        case AHIST_S16S:
        case AHIST_BW|AHIST_S16S:
          *ScaleLeft  = VolumeLeft;
          *ScaleRight = VolumeRight;
          if(SampleType & AHIST_BW)
            *AddRoutine = (void *) AddWordsSVPHB;
          else
            *AddRoutine = (void *) AddWordsSVPH;
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

  if(*ScaleLeft == 0 && *ScaleRight == 0)
  {
    if( (*AddRoutine != NULL) && (SampleType & AHIST_BW) )
      *AddRoutine = (void *) AddSilenceB;
    else
      *AddRoutine = (void *) AddSilence;
  }
}


/******************************************************************************
** Mix ************************************************************************
******************************************************************************/

// This is the function that the driver calls each time it want more data
// to play. 

// There is a stub function in asmfuncs.a called Mix() that saves d0-d1/a0-a1
// and calls MixMixGeneric. This stub is only assembled if VERSIONGEN is set.

void ASMCALL
MixGeneric ( REG(a0, struct Hook *Hook), 
             REG(a1, void *dst), 
             REG(a2, struct AHIPrivAudioCtrl *audioctrl) )
{
  struct AHIChannelData	*cd;
  void                  *dstptr;
  ULONG                  samplesleft;
  double                 Add, LastOffset, Offset, NextOffset;

  /* Clear the buffer */

  memset(dst, audioctrl->ahiac_BuffSizeNow, 0);


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
        /* Call Sound Hook */

        if(cd->cd_EOS)
        {
          cd->cd_EOS = FALSE;

          if(audioctrl->ac.ahiac_SoundFunc != NULL)
          {
            CallHookPkt(audioctrl->ac.ahiac_SoundFunc, audioctrl, &cd->cd_ChannelNo);
          }
        }

        // This should really be long longs instead...

        Add        = cd->cd_AddI        * 65536.0 + cd->cd_AddF;
        LastOffset = cd->cd_LastOffsetI * 65536.0 + cd->cd_LastOffsetF;
        Offset     = cd->cd_OffsetI     * 65536.0 + cd->cd_OffsetF;
        NextOffset = cd->cd_NextOffsetI * 65536.0 + cd->cd_NextOffsetF;

        if(cd->cd_FreqOK && cd->cd_SoundOK && cd->cd_ChannelNo == 0) se här!
        {

          if(cd->cd_AddRoutine == NULL)
          {
            break;  // Panic! Should never happen.
          }

          if(samplesleft >= cd->cd_Samples)
          {
            samplesleft -= cd->cd_Samples;

            /* Call AddRoutine (cd->cd_Samples) */

            ((ADDFUNC) cd->cd_AddRoutine)(
                cd->cd_Samples, cd->cd_ScaleLeft, cd->cd_ScaleRight,
                &Offset, Add, audioctrl, cd->cd_DataStart, &dstptr, cd);

            /* Linear interpol. stuff */

            cd->cd_LastSampleL = cd->cd_TempLastSampleL;
            cd->cd_LastSampleR = cd->cd_TempLastSampleR;

            /* Give AHIST_INPUT special treatment! */
          
            if(cd->cd_Type & AHIST_INPUT)
            {
              // Screw it... I doesn't work anyway.
              continue; // .contchannel
            }

            /*
            ** Offset always points OUTSIDE the sample after this
            ** call.  Ie, if we read a sample at offset (Offset /
            ** 65536) now, it does not belong to the sample just
            ** played.  This is true for both backward and forward
            ** mixing.
            */


            /* What we do now is to calculate how much futher we have
               advanced. */

               Offset -= LastOffset;

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
              /* Ok, we subtract. */

              NextOffset -= Offset;
            }
            else
            {
              /* Ok, we add. */

              NextOffset += Offset;
            }

            /* And convert double to I+F */

            cd->cd_OffsetI      = NextOffset / 65536.0;
            cd->cd_OffsetF      = fmod(NextOffset, 65536.0);
            cd->cd_FirstOffsetI = cd->cd_OffsetI;

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

            cd->cd_FreqOK       = cd->cd_NextFreqOK;
            cd->cd_SoundOK      = cd->cd_NextSoundOK;
            cd->cd_AddI         = cd->cd_NextAddI;
            cd->cd_AddF         = cd->cd_NextAddF;
            cd->cd_DataStart    = cd->cd_NextDataStart;
            cd->cd_LastOffsetI  = cd->cd_NextLastOffsetI;
            cd->cd_LastOffsetF  = cd->cd_NextLastOffsetF;
            cd->cd_ScaleLeft    = cd->cd_NextScaleLeft;
            cd->cd_ScaleRight   = cd->cd_NextScaleRight;
            cd->cd_AddRoutine   = cd->cd_NextAddRoutine;
            cd->cd_VolumeLeft   = cd->cd_NextVolumeLeft;
            cd->cd_VolumeRight  = cd->cd_NextVolumeRight;
            cd->cd_Type         = cd->cd_NextType;

            cd->cd_Samples = CalcSamples ( cd->cd_AddI,
                                           cd->cd_AddF,
                                           cd->cd_Type,
                                           cd->cd_LastOffsetI,
                                           cd->cd_LastOffsetF,
                                           cd->cd_OffsetI,
                                           cd->cd_OffsetF );

            cd->cd_EOS = TRUE;      // signal End-Of-Sample
            continue;               // .contchannel (same channel, new sound)
          }
          else
          {
            // .wont_reach_end

            cd->cd_Samples -= samplesleft;
          
            /*** Call AddRoutine (samplesleft) ***/

            ((ADDFUNC) cd->cd_AddRoutine)(
                samplesleft, cd->cd_ScaleLeft, cd->cd_ScaleRight,
                &Offset, Add, audioctrl, cd->cd_DataStart, &dstptr, cd);
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
#if 0
      if(audioctrl->ahiac_EffDSPEchoStruct != NULL)
      {
        audioctrl->ahiac_EffDSPEchoStruct->ahiecho_Code(
            audioctrl->ahiac_EffDSPEchoStruct, dst, audioctrl);
      }
#endif
      cd = audioctrl->ahiac_DryList;

      if(audioctrl->ac.ahiac_Flags & AHIACF_POSTPROC)
      {
        /*** AHIET_MASTERVOLUME ***/

//        DoMasterVolume(dst, audioctrl);

        /*
        ** When AHIACB_POSTPROC is set, the dry data shall be placed
        ** immediate after the wet data. This is done by modifying the
        ** dst pointer
        */

        dst = (char *) dst + audioctrl->ac.ahiac_BuffSamples * 
                             AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType);
      }

      continue; /* while(TRUE) */
    }
    else
    {
      break; /* while(TRUE) */
    }
  } // while(TRUE)

  /*** AHIET_MASTERVOLUME ***/

//  DoMasterVolume(dst, audioctrl);

  /*** AHIET_OUTPUTBUFFER ***/

//  DoOutputBuffer(dst, audioctrl);

  /*** AHIET_CHANNELINFO ***/

//  DoChannelInfo(audioctrl);

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
    
    if(sample > 0x07ffffff)
      sample = 0x07ffffff;
    else if(sample < 0xf8000000)
      sample = 0xf8000000;

    *dst++ = sample << 4;
  }
}



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

    CallHookPkt(ob->ahieob_Func, audioctrl, ob);
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
      *offsets++ = cd->cd_OffsetI;
      cd++;
    }
    
    CallHookPkt(ci->ahieci_Func, audioctrl, ci);
  }
}


/******************************************************************************
** CalcSamples ****************************************************************
******************************************************************************/

ULONG
CalcSamples ( ULONG AddI,
              UWORD AddF,
              ULONG Type,
              ULONG LastOffsetI,
              UWORD LastOffsetF,
              ULONG OffsetI,
              UWORD OffsetF )

{
  // We need 48 bits resolution (32 bit integer + 16 bit fraction); double
  // should be enough. SAS/C doesn't seem to handle >32 bit integers :(

  double Add, LastOffset, Offset, len;

  Add        = AddI        * 65536.0 + AddF;
  LastOffset = LastOffsetI * 65536.0 + LastOffsetF;
  Offset     = OffsetI     * 65536.0 + OffsetF;

  if(Type & AHIST_BW)
  {
    len = Offset - LastOffset; 
  }
  else
  {
    len = LastOffset - Offset;
  }

  // Compare with zero should be ok even if Add is double...
  if(len < 0 || Add == 0) return 0; // Error!

  return (ULONG) (len / Add);

}


/******************************************************************************
** Add-Routines ***************************************************************
******************************************************************************/

/*
** ULONG   Samples
** Fixed   ScaleLeft
** Fixed   ScaleRight (Not used in all routines)
** double *Offset
** double  Add
** struct  AHIPrivAudioCtrl *audioctrl
** void   *Src
** void  **Dst
** struct  AHIChannelData *cd
*/

static void
AddSilence ( ADDARGS )
{
  *Offset += Samples * Add;
  *Dst    = (char *) *Dst + Samples * AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType);
}

static void
AddSilenceB ( ADDARGS )
{
  *Offset -= Samples * Add;
  *Dst    = (char *) *Dst + Samples * AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType);
}


/*
** Some comments... the index to the source samples are calculated using
** doubles. It would probably be faster to use long long for that. Then
** the index can be extracted more easily. Maybe the FPU could be used
** in the actual mixing process instead, by having a float mixing buffer
** and volume (ScaleLeft & ScaleRight).
**
** The fraction offset is divided by two in order to make sure that
** the calculation of linearsample fits a LONG (0 =< offsetf <= 32767).
**
** The routines could be faster, of course. One idea is to split the
** for loop into two loops in order to eliminate the FirstOffsetI
** test in the second loop.
*/

static void
AddByteMVH ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti - 1 ] << 8;
    }

    currentsample = src[ offseti ] << 8;

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddByteSVPH ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;
  
  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti - 1 ] << 8;
    }

    currentsample = src[ offseti ] << 8;

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;
    *dst++ += ScaleRight * lastsample;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddBytesMVH ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 - 2 ] << 8;
      lastsampleR = src[ offseti + 1 - 2 ] << 8;
    }

    currentsampleL = src[ offseti + 0 ] << 8;
    currentsampleR = src[ offseti + 1 ] << 8;

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL + ScaleRight * lastsampleR;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddBytesSVPH ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 - 2 ] << 8;
      lastsampleR = src[ offseti + 1 - 2 ] << 8;
    }

    currentsampleL = src[ offseti + 0 ] << 8;
    currentsampleR = src[ offseti + 1 ] << 8;

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL;
    *dst++ += ScaleRight * lastsampleR;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordMVH ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti - 1 ];
    }

    currentsample = src[ offseti ];

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordSVPH ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;
  
  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti - 1 ];
    }

    currentsample = src[ offseti ];

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;
    *dst++ += ScaleRight * lastsample;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordsMVH ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 - 2 ];
      lastsampleR = src[ offseti + 1 - 2 ];
    }

    currentsampleL = src[ offseti + 0 ];
    currentsampleR = src[ offseti + 1 ];

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL + ScaleRight * lastsampleR;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordsSVPH ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = (long) fmod(offset * 65536.0, 65536.0) >> 1;

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 - 2 ];
      lastsampleR = src[ offseti + 1 - 2 ];
    }

    currentsampleL = src[ offseti + 0 ];
    currentsampleR = src[ offseti + 1 ];

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL;
    *dst++ += ScaleRight * lastsampleR;

    offset += add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddByteMVHB ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti + 1 ] << 8;
    }

    currentsample = src[ offseti ] << 8;

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddByteSVPHB ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;
  
  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti + 1 ] << 8;
    }

    currentsample = src[ offseti ] << 8;

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;
    *dst++ += ScaleRight * lastsample;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;

}


static void
AddBytesMVHB ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 + 2 ] << 8;
      lastsampleR = src[ offseti + 1 + 2 ] << 8;
    }

    currentsampleL = src[ offseti + 0 ] << 8;
    currentsampleR = src[ offseti + 1 ] << 8;

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL + ScaleRight * lastsampleR;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddBytesSVPHB ( ADDARGS )
{
  BYTE   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 + 2 ] << 8;
      lastsampleR = src[ offseti + 1 + 2 ] << 8;
    }

    currentsampleL = src[ offseti + 0 ] << 8;
    currentsampleR = src[ offseti + 1 ] << 8;

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL;
    *dst++ += ScaleRight * lastsampleR;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordMVHB ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti + 1 ];
    }

    currentsample = src[ offseti ];

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordSVPHB ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsample, currentsample;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsample = cd->cd_TempLastSampleL;
  
  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsample = cd->cd_LastSampleL;
    }
    else
    {
      lastsample = src[ offseti + 1 ];
    }

    currentsample = src[ offseti ];

    lastsample += (((currentsample - lastsample) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsample;
    *dst++ += ScaleRight * lastsample;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsample;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordsMVHB ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 + 2 ];
      lastsampleR = src[ offseti + 1 + 2 ];
    }

    currentsampleL = src[ offseti + 0 ];
    currentsampleR = src[ offseti + 1 ];

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL + ScaleRight * lastsampleR;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}


static void
AddWordsSVPHB ( ADDARGS )
{
  WORD   *src = Src;
  LONG   *dst = *Dst;
  int     i;
  LONG    lastsampleL, lastsampleR, currentsampleL, currentsampleR;
  double  offset = *Offset / 65536.0, add = Add / 65536.0;
  long    offseti;
  long    offsetf;

  currentsampleL = cd->cd_TempLastSampleL;
  currentsampleR = cd->cd_TempLastSampleR;

  for(i = Samples; i > 0; i--)
  {
    offseti = (long) offset << 1;
    offsetf = 32768 - ((long) fmod(offset * 65536.0, 65536.0) >> 1);

    if( offseti == cd->cd_FirstOffsetI) {
      lastsampleL = cd->cd_LastSampleL;
      lastsampleR = cd->cd_LastSampleR;
    }
    else
    {
      lastsampleL = src[ offseti + 0 + 2 ];
      lastsampleR = src[ offseti + 1 + 2 ];
    }

    currentsampleL = src[ offseti + 0 ];
    currentsampleR = src[ offseti + 1 ];

    lastsampleL += (((currentsampleL - lastsampleL) * offsetf ) >> 15);
    lastsampleR += (((currentsampleR - lastsampleR) * offsetf ) >> 15);

    *dst++ += ScaleLeft * lastsampleL;
    *dst++ += ScaleRight * lastsampleR;

    offset -= add;
  }

  cd->cd_TempLastSampleL = currentsampleL;
  cd->cd_TempLastSampleR = currentsampleR;

  *Offset = offset * 65536.0;
  *Dst    = dst;
}
