/* $Id$
* $Log$
* Revision 4.7  1998/01/12 20:05:03  lcs
* More restruction, mixer in C added. (Just about to make fraction 32 bit!)
*
* Revision 4.6  1997/12/21 17:41:50  lcs
* Major source cleanup, moved some functions to separate files.
*
* Revision 4.5  1997/10/23 01:10:03  lcs
* Better debug output.
*
*/

#include <exec/memory.h>
#include <proto/exec.h>

#include <CompilerSpecific.h>
#include "ahi_def.h"
#include "dsp.h"

#ifndef  noprotos

#ifndef _GENPROTO
#include "effectinit_protos.h"
#endif

#include "asmfuncs_protos.h"
#include "dspecho_protos.h"
#include "mixer_protos.h"

#endif


/***********************************************
***** NOTE: The mixing routine might execute while we are inside these
***** functions!
***********************************************/


/******************************************************************************
** MASTERVOLUME ***************************************************************
******************************************************************************/

BOOL ASMCALL
update_MasterVolume ( REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                      REG(a5, struct AHIBase *AHIBase) )
{
  struct Library        *AHIsubBase;
  struct AHIChannelData *cd;
  Fixed                  volume;
  int                    i;

  if(audioctrl->ac.ahiac_Flags & AHIACF_CLIPPING)
  {
    volume = 0x10000;
  }
  else
  {
    volume = audioctrl->ahiac_SetMasterVolume;
  }

  /* Scale to what the echo effect think is best... */
  volume = (volume * (audioctrl->ahiac_EchoMasterVolume >> 8)) >> 8;

  /* This is the actual master volume in use */
  audioctrl->ahiac_MasterVolume = volume;

  /* Update the mastervolume table, and the volume tables */
  calcMasterVolumeTable(audioctrl, AHIBase);
  calcSignedTable(audioctrl, AHIBase);
  calcUnsignedTable(audioctrl, AHIBase);

  /* Update volume for channels */

  AHIsubBase = audioctrl->ahiac_SubLib;

  AHIsub_Disable(&audioctrl->ac);

  for(i = audioctrl->ac.ahiac_Channels, cd = audioctrl->ahiac_ChannelDatas;
      i > 0;
      i--, cd++)
  {
    SelectAddRoutine(cd->cd_VolumeLeft, cd->cd_VolumeRight, cd->cd_Type, audioctrl,
                     &cd->cd_ScaleLeft, &cd->cd_ScaleRight, &cd->cd_AddRoutine);
    SelectAddRoutine(cd->cd_NextVolumeLeft, cd->cd_NextVolumeRight, cd->cd_NextType, audioctrl,
                     &cd->cd_NextScaleLeft, &cd->cd_NextScaleRight, &cd->cd_NextAddRoutine);
  }

  AHIsub_Enable(&audioctrl->ac);

  return TRUE;
}


/******************************************************************************
** DSPECHO ********************************************************************
******************************************************************************/

#define mode_stereo 1
#define mode_32bit  2
#define mode_ncnm   4       // No cross, no mix
#define mode_fast   8

BOOL ASMCALL
update_DSPEcho ( REG(a0, struct AHIEffDSPEcho *echo),
                 REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                 REG(a5, struct AHIBase *AHIBase) )
{
  ULONG length, samplesize;
  struct Echo *es;

  free_DSPEcho(audioctrl, AHIBase);

  samplesize = AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType);

  length = samplesize * (echo->ahiede_Delay + audioctrl->ac.ahiac_MaxBuffSamples);

  es = AllocVec(sizeof(struct Echo) + length, MEMF_PUBLIC|MEMF_CLEAR);
  
  if(es)
  {
    ULONG mode = 0;

    es->ahiecho_Offset       = 0;
    es->ahiecho_SrcPtr       = es->ahiecho_Buffer;
    es->ahiecho_DstPtr       = es->ahiecho_Buffer + (samplesize * echo->ahiede_Delay);
    es->ahiecho_EndPtr       = es->ahiecho_Buffer + length;
    es->ahiecho_BufferLength = echo->ahiede_Delay + audioctrl->ac.ahiac_MaxBuffSamples;
    es->ahiecho_BufferSize   = length;

    switch(audioctrl->ac.ahiac_BuffType)
    {
      case AHIST_M16S:
        echo->ahiede_Cross = 0;
        break;
      case AHIST_S16S:
        mode |= mode_stereo;
        break;
      case AHIST_M32S:
        echo->ahiede_Cross = 0;
        mode |= mode_32bit;
        break;
      case AHIST_S32S:
        mode |= (mode_32bit | mode_stereo);
        break;
    }

    es->ahiecho_Delay      = echo->ahiede_Delay;
    es->ahiecho_MixD       = echo->ahiede_Mix;
    es->ahiecho_MixN       = 0x10000 - echo->ahiede_Mix;
    es->ahiecho_FeedbackDO = (echo->ahiede_Feedback >> 8) *
                             (echo->ahiede_Cross >> 8);
    es->ahiecho_FeedbackDS = (echo->ahiede_Feedback >> 8) *
                             ((0x10000 - echo->ahiede_Cross) >> 8);
    es->ahiecho_FeedbackNO = ((0x10000 - echo->ahiede_Feedback) >> 8) *
                             (echo->ahiede_Cross >> 8);
    es->ahiecho_FeedbackNS = ((0x10000 - echo->ahiede_Feedback) >> 8) *
                             ((0x10000 - echo->ahiede_Cross) >> 8);


#if defined(VERSIONGEN) || defined(VERSIONPPC)

    audioctrl->ahiac_EchoMasterVolume = 0x10000;

#else

    if((echo->ahiede_Cross == 0) && (echo->ahiede_Mix == 0x10000))
    {
      mode |= mode_ncnm;
      audioctrl->ahiac_EchoMasterVolume = es->ahiecho_FeedbackNS;
    }
    else
    {
      audioctrl->ahiac_EchoMasterVolume = 0x10000;
    }

#endif

    update_MasterVolume(audioctrl,AHIBase);

#if defined(VERSIONGEN) || defined(VERSIONPPC)

    /* No fast echo in generic or PPC version*/

#else

    // No fast echo in 32 bit (HiFi) modes!
    switch(audioctrl->ac.ahiac_BuffType)
    {
      case AHIST_M16S:
      case AHIST_S16S:
        if((AHIBase->ahib_Flags & AHIBF_FASTECHO))
        {
          es->ahiecho_MixD = Fixed2Shift(es->ahiecho_MixD);
          es->ahiecho_MixN = Fixed2Shift(es->ahiecho_MixN);
          es->ahiecho_FeedbackDO = Fixed2Shift(es->ahiecho_FeedbackDO);
          es->ahiecho_FeedbackDS = Fixed2Shift(es->ahiecho_FeedbackDS);
          es->ahiecho_FeedbackNO = Fixed2Shift(es->ahiecho_FeedbackNO);
          es->ahiecho_FeedbackNS = Fixed2Shift(es->ahiecho_FeedbackNS);
          mode |= mode_fast;
        }
        break;

      default:
        break;
    }

#endif

#if defined(VERSIONGEN) || defined(VERSIONPPC)

    switch(mode)
    {
      // 32bit
      case 2:
        es->ahiecho_Code   = do_DSPEchoMono32;
        break;

      // stereo 32bit
      case 3:
        es->ahiecho_Code   = do_DSPEchoStereo32;
        break;

      // Should not happen!
      default:
        FreeVec(es);
        return FALSE;
    }

#else

    switch(mode)
    {
      case 0:
        es->ahiecho_Code   = do_DSPEchoMono16;
        break;

      // stereo
      case 1:
        es->ahiecho_Code   = do_DSPEchoStereo16;
        break;

      // 32bit
      case 2:
        es->ahiecho_Code   = do_DSPEchoMono32;
        break;

      // stereo 32bit
      case 3:
        es->ahiecho_Code   = do_DSPEchoStereo32;
        break;

      // ncnm
      case 4:
        es->ahiecho_Code   = do_DSPEchoMono16NCFM;
        break;

      // stereo ncnm
      case 5:
        es->ahiecho_Code   = do_DSPEchoStereo16NCFM;
        break;

      // 32bit ncnm
      case 6:
        es->ahiecho_Code   = do_DSPEchoMono32;
        break;

      // stereo 32bit ncnm
      case 7:
        es->ahiecho_Code   = do_DSPEchoStereo32;
        break;

      // fast
      case 8:
        es->ahiecho_Code   = do_DSPEchoMono16Fast;
        break;

      // stereo fast
      case 9:
        es->ahiecho_Code   = do_DSPEchoStereo16Fast;
        break;

      // 32bit fast
      case 10:
        es->ahiecho_Code   = do_DSPEchoMono32;
        break;

      // stereo 32bit fast
      case 11:
        es->ahiecho_Code   = do_DSPEchoStereo32;
        break;

      // ncnm fast
      case 12:
        es->ahiecho_Code   = do_DSPEchoMono16NCFMFast;
        break;

      // stereo ncnm fast
      case 13:
        es->ahiecho_Code   = do_DSPEchoStereo16NCFMFast;
        break;

      // 32bit ncnm fast
      case 14:
        es->ahiecho_Code   = do_DSPEchoMono32;
        break;

      // stereo 32bit ncnm fast
      case 15:
        es->ahiecho_Code   = do_DSPEchoStereo32;
        break;

      // Should not happen!
      default:
        FreeVec(es);
        return FALSE;
    }

#endif

    // Structure filled, make it available to the mixing routine

    audioctrl->ahiac_EffDSPEchoStruct = es;

    return TRUE;
  }

  return FALSE;
}


void ASMCALL
free_DSPEcho ( REG(a2, struct AHIPrivAudioCtrl *audioctrl),
               REG(a5, struct AHIBase *AHIBase) )
{
  void *p = audioctrl->ahiac_EffDSPEchoStruct;

  // Hide it from mixing routine before freeing it!
  audioctrl->ahiac_EffDSPEchoStruct = NULL;
  FreeVec(p);

  audioctrl->ahiac_EchoMasterVolume = 0x10000;
  update_MasterVolume(audioctrl,AHIBase);
}




/******************************************************************************
** DSPMASK ********************************************************************
******************************************************************************/

static void
addchannel ( struct AHIChannelData **list, struct AHIChannelData *cd )
{
  struct AHIChannelData *ptr;

  if(*list == NULL)
  {
    *list = cd;
  }
  else
  {
    ptr = *list;
    while(ptr->cd_Succ != NULL)
    {
      ptr = ptr->cd_Succ;
    }
    ptr->cd_Succ = cd;
  }

  cd->cd_Succ = NULL;
}

BOOL ASMCALL
update_DSPMask ( REG(a0, struct AHIEffDSPMask *mask),
                 REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                 REG(a5, struct AHIBase *AHIBase) )
{
  struct AHIChannelData *cd, *wet = NULL, *dry = NULL;
  struct Library        *AHIsubBase;
  int                    i;
  UBYTE                 *flag;

  if(mask->ahiedm_Channels != audioctrl->ac.ahiac_Channels)
  {
    return FALSE;
  }

  cd = audioctrl->ahiac_ChannelDatas;

  flag = mask->ahiedm_Mask;

  AHIsubBase = audioctrl->ahiac_SubLib;

  AHIsub_Disable(&audioctrl->ac);

  for(i = 0; i < audioctrl->ac.ahiac_Channels; i++)
  {
    if(*flag == AHIEDM_WET)
    {
      addchannel(&wet, cd);
    }
    else
    {
      addchannel(&dry, cd);
    }
    
    flag++;
    cd++;
  }

  audioctrl->ahiac_WetList = wet;
  audioctrl->ahiac_DryList = dry;

  AHIsub_Enable(&audioctrl->ac);

  return TRUE;
}


void ASMCALL
clear_DSPMask ( REG(a2, struct AHIPrivAudioCtrl *audioctrl),
                REG(a5, struct AHIBase *AHIBase) )
{
  struct AHIChannelData *cd;
  struct Library        *AHIsubBase;
  int                    i;

  // Make all channels wet

  cd = audioctrl->ahiac_ChannelDatas;

  audioctrl->ahiac_WetList = cd;
  audioctrl->ahiac_DryList = NULL;

  AHIsubBase = audioctrl->ahiac_SubLib;

  AHIsub_Disable(&audioctrl->ac);

  for(i = 0; i < audioctrl->ac.ahiac_Channels - 1; i++)
  {
    // Set link to next channel
    cd->cd_Succ = cd + 1;
    cd++;
  }

  // Clear the last link;
  cd->cd_Succ = NULL;

  AHIsub_Enable(&audioctrl->ac);
}
