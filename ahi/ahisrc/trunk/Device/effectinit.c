/* $Id$
* $Log$
* Revision 4.5  1997/10/23 01:10:03  lcs
* Better debug output.
*
* Revision 4.4  1997/08/02 17:11:59  lcs
* Right. Now echo should work!
*
* Revision 4.3  1997/08/02 16:32:39  lcs
* Fixed a memory trashing error. Will change it yet again now...
*
* Revision 4.2  1997/06/02 18:15:02  lcs
* Renamed it from dspinit.c to effectinit.c
*
* Revision 4.1  1997/04/02 22:29:53  lcs
* Bumped to version 4
*
* Revision 1.3  1997/03/26 00:14:32  lcs
* Echo is finally working!
*
* Revision 1.1  1997/03/24 12:41:51  lcs
* Initial revision
*
*/

#include <exec/memory.h>
#include <proto/exec.h>

#include "ahi_def.h"
#include "dsp.h"

#ifndef  noprotos

#ifndef _GENPROTO
#include "effectinit_protos.h"
#endif

#endif


__asm extern void do_DSPEchoMono16(void);
__asm extern void do_DSPEchoMono16Fast(void);
__asm extern void do_DSPEchoStereo16(void);
__asm extern void do_DSPEchoStereo16Fast(void);
__asm extern void do_DSPEchoMono32(void);
__asm extern void do_DSPEchoStereo32(void);
__asm extern void do_DSPEchoMono16NCFM(void);
__asm extern void do_DSPEchoStereo16NCFM(void);
__asm extern void do_DSPEchoMono16NCFMFast(void);
__asm extern void do_DSPEchoStereo16NCFMFast(void);

__asm extern LONG Fixed2Shift(register __d0 Fixed);

__asm extern void update_MasterVolume(register __a2 struct AHIPrivAudioCtrl *,
    register __a5 struct AHIBase *AHIBase);

/**
*** DSPECHO
**/

#define mode_stereo 1
#define mode_32bit  2
#define mode_ncnm   4       // No cross, no mix
#define mode_fast   8

// NOTE: The mixing routine might execute while we are inside these
// functions!

__asm BOOL update_DSPEcho(
    register __a0 struct AHIDSPEcho *echo,
    register __a2 struct AHIPrivAudioCtrl *actrl,
    register __a5 struct AHIBase *AHIBase)
{
  ULONG length, samplesize;
  struct Echo *es;

  free_DSPEcho(actrl, AHIBase);

  switch(actrl->ac.ahiac_BuffType)
  {
    case AHIST_M16S:
    case AHIST_M32S:
      samplesize = 2;
      break;

    case AHIST_S16S:
    case AHIST_S32S:
      samplesize = 4;
      break;

    default:
      return FALSE;
      break;
  }

  length = samplesize * (echo->ahiede_Delay + actrl->ac.ahiac_MaxBuffSamples);

  es = AllocVec(sizeof(struct Echo) + length, MEMF_PUBLIC|MEMF_CLEAR);
  
  if(es)
  {
    ULONG mode = 0;

    es->ahiecho_Offset       = 0;
    es->ahiecho_SrcPtr       = es->ahiecho_Buffer;
    es->ahiecho_DstPtr       = es->ahiecho_Buffer + (samplesize * echo->ahiede_Delay);
    es->ahiecho_EndPtr       = es->ahiecho_Buffer + length;
    es->ahiecho_BufferLength = echo->ahiede_Delay + actrl->ac.ahiac_MaxBuffSamples;
    es->ahiecho_BufferSize   = length;

    switch(actrl->ac.ahiac_BuffType)
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


    if((echo->ahiede_Cross == 0) && (echo->ahiede_Mix == 0x10000))
    {
      mode |= mode_ncnm;
      actrl->ahiac_EchoMasterVolume = es->ahiecho_FeedbackNS;
    }
    else
    {
      actrl->ahiac_EchoMasterVolume = 0x10000;
    }

    update_MasterVolume(actrl,AHIBase);

    // No fast echo in 32 bit (HiFi) modes!
    switch(actrl->ac.ahiac_BuffType)
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

    switch(mode)
    {
      case 0:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16;
        break;

      // stereo
      case 1:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16;
        break;

      // 32bit
      case 2:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;

      // stereo 32bit
      case 3:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;

      // ncnm
      case 4:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16NCFM;
        break;

      // stereo ncnm
      case 5:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16NCFM;
        break;

      // 32bit ncnm
      case 6:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;

      // stereo 32bit ncnm
      case 7:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;

      // fast
      case 8:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16Fast;
        break;

      // stereo fast
      case 9:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16Fast;
        break;

      // 32bit fast
      case 10:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;

      // stereo 32bit fast
      case 11:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;

      // ncnm fast
      case 12:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16NCFMFast;
        break;

      // stereo ncnm fast
      case 13:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16NCFMFast;
        break;

      // 32bit ncnm fast
      case 14:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;

      // stereo 32bit ncnm fast
      case 15:
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;
    }

    // Structure filled, make it available to the mixing routine
    actrl->ahiac_EffDSPEchoStruct = es;
  }
  return FALSE;
}


__asm void free_DSPEcho(
    register __a2 struct AHIPrivAudioCtrl *actrl,
    register __a5 struct AHIBase *AHIBase)
{
  void *p = actrl->ahiac_EffDSPEchoStruct;

  // Hide it from mixing routine before freeing it!
  actrl->ahiac_EffDSPEchoStruct = NULL;
  FreeVec(p);

  actrl->ahiac_EchoMasterVolume = 0x10000;
  update_MasterVolume(actrl,AHIBase);
}
