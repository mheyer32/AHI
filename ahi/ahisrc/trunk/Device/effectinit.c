/* $Id$
* $Log$
* Revision 1.4  1997/03/27 12:16:27  lcs
* Major bug in the device interface code fixed.
*
* Revision 1.3  1997/03/26 00:14:32  lcs
* Echo is finally working!
*
* Revision 1.2  1997/03/25 22:27:49  lcs
* Tried to get AHIST_INPUT to work, but I cannot get it synced! :(
*
* Revision 1.1  1997/03/24 12:41:51  lcs
* Initial revision
*
*/

#include <exec/memory.h>
#include <proto/exec.h>

#include "ahi_def.h"
#include "dsp.h"

#ifndef _GENPROTO
#include "dspinit_protos.h"
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

__asm extern void update_MasterVolume(register __a2 struct AHIPrivAudioCtrl *);

/**
*** DSPECHO
**/

// NOTE: The mixing routine might execute while we are inside these
// functions!

__asm BOOL update_DSPEcho(
    register __a0 struct AHIDSPEcho *echo,
    register __a2 struct AHIPrivAudioCtrl *actrl,
    register __a5 struct AHIBase *AHIBase)
{
  ULONG length;
  struct Echo *es;

  free_DSPEcho(actrl, AHIBase);

  length = AHI_SampleFrameSize(actrl->ac.ahiac_BuffType) *
           (echo->ahiede_Delay + actrl->ac.ahiac_MaxBuffSamples);

  es = AllocVec(sizeof(struct Echo)+ length, MEMF_PUBLIC|MEMF_CLEAR);
  
  if(es)
  {
    ULONG mode = 0;
#define mode_stereo 1
#define mode_32bit  2
#define mode_ncnm   4       // No cross, no mix
#define mode_fast   8

    es->ahiecho_BufferSize = length;
    es->ahiecho_EndPtr     = es->ahiecho_Buffer + length;
    es->ahiecho_SrcPtr     = es->ahiecho_Buffer;
    es->ahiecho_DstPtr     = es->ahiecho_Buffer +
        (AHI_SampleFrameSize(actrl->ac.ahiac_BuffType) * echo->ahiede_Delay);

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

    update_MasterVolume(actrl);

    if(AHIBase->ahib_Flags & AHIBF_FASTECHO)
    {
      es->ahiecho_MixD = Fixed2Shift(es->ahiecho_MixD);
      es->ahiecho_MixN = Fixed2Shift(es->ahiecho_MixN);
      es->ahiecho_FeedbackDO = Fixed2Shift(es->ahiecho_FeedbackDO);
      es->ahiecho_FeedbackDS = Fixed2Shift(es->ahiecho_FeedbackDS);
      es->ahiecho_FeedbackNO = Fixed2Shift(es->ahiecho_FeedbackNO);
      es->ahiecho_FeedbackNS = Fixed2Shift(es->ahiecho_FeedbackNS);
      mode |= mode_fast;
    }

    switch(mode)
    {
      case 0:
//        KPrintF("do_DSPEchoMono16\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16;
        break;
      case 1:
//        KPrintF("do_DSPEchoStereo16\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16;
        break;
      case 2:
//        KPrintF("do_DSPEchoMono32\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;
      case 3:
//        KPrintF("do_DSPEchoStereo32\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;
      case 4:
//        KPrintF("do_DSPEchoMono16NCFM\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16NCFM;
        break;
      case 5:
//        KPrintF("do_DSPEchoStereo16NCFM\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16NCFM;
        break;
      case 6:
//        KPrintF("do_DSPEchoMono32\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;
      case 7:
//        KPrintF("do_DSPEchoStereo32\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;
      case 8:
//        KPrintF("do_DSPEchoMono16Fast\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16Fast;
        break;
      case 9:
//        KPrintF("do_DSPEchoStereo16Fast\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16Fast;
        break;
      case 10:
//        KPrintF("do_DSPEchoMono32\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono32;
        break;
      case 11:
//        KPrintF("do_DSPEchoStereo32\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo32;
        break;
      case 12:
//        KPrintF("do_DSPEchoMono16NCFMFast\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoMono16NCFMFast;
        break;
      case 13:
//        KPrintF("do_DSPEchoStereo16NCFMFast\n");
        es->ahiecho_Code   = (void (*)(void)) do_DSPEchoStereo16NCFMFast;
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
  update_MasterVolume(actrl);
}
