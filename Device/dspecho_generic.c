/* $Id$
* $Log$
* Revision 4.2  1998/01/12 20:07:28  lcs
* Generic echo code.
*
*/


#include <CompilerSpecific.h>
#include "ahi_def.h"
#include "dsp.h"


/* See dspecho_68k.a för more information */



static void
EchoMono32 ( LONG loops,
             struct Echo *es,
             void **buffer,
             void **srcptr,
             void **dstptr)
{
  LONG *buf;
  WORD *src, *dst;
  LONG sample, delaysample;
  
  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sample      = *buf >> 16;
    delaysample = *src++;

    *buf++ = es->ahiecho_MixN * sample + es->ahiecho_MixD * delaysample;

    sample = es->ahiecho_FeedbackNS * sample + 
             es->ahiecho_FeedbackDS * (delaysample + 1);
    
    *dst++ = sample >> 16;

    loops--;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}

static void
EchoStereo32 ( LONG loops,
               struct Echo *es,
               void **buffer,
               void **srcptr,
               void **dstptr)
{
  LONG *buf;
  WORD *src, *dst;
  LONG sampleL, sampleR, delaysampleL, delaysampleR;
  
  buf = *buffer;
  src = *srcptr;
  dst = *dstptr;

  while(loops > 0)
  {
    sampleL      = *buf >> 16;
    delaysampleL = *src++;

    *buf++ = es->ahiecho_MixN * sampleL + es->ahiecho_MixD * delaysampleL;

    sampleR      = *buf >> 16;
    delaysampleR = *src++;

    *buf++ = es->ahiecho_MixN * sampleR + es->ahiecho_MixD * delaysampleR;

    sampleL = es->ahiecho_FeedbackDS * (delaysampleL + 1) +
              es->ahiecho_FeedbackDO * (delaysampleR + 1) +
              es->ahiecho_FeedbackNS * sampleL +
              es->ahiecho_FeedbackNO * sampleR;
    
    *dst++ = sampleL >> 16;

    sampleR = es->ahiecho_FeedbackDO * (delaysampleL + 1) +
              es->ahiecho_FeedbackDS * (delaysampleR + 1) +
              es->ahiecho_FeedbackNO * sampleL +
              es->ahiecho_FeedbackNS * sampleR;

    *dst++ = sampleR >> 16;
  }

  *buffer = buf;
  *srcptr = src;
  *dstptr = dst;
}



static void
do_DSPEcho ( struct Echo *es,
             void *buf,
             struct AHIPrivAudioCtrl *audioctrl,
             void (*echofunc)(LONG, struct Echo *, void **, void **, void **) )
{
  LONG  samples, offset, loops;
  void *srcptr, *dstptr;

  samples = audioctrl->ac.ahiac_BuffSamples;
  offset  = es->ahiecho_Offset;
  srcptr  = es->ahiecho_SrcPtr;
  dstptr  = es->ahiecho_DstPtr;

  while(samples > 0)
  {
    /* Circular buffer stuff */  
    
    if(srcptr >= es->ahiecho_EndPtr)
    {
      srcptr = (char *) srcptr - es->ahiecho_BufferSize;
    }

    if(dstptr >= es->ahiecho_EndPtr)
    {
      dstptr = (char *) dstptr - es->ahiecho_BufferSize;
    }

    if(offset >= es->ahiecho_BufferLength)
    {
      offset -= es->ahiecho_BufferLength;
    }



    if(offset < audioctrl->ac.ahiac_MaxBuffSamples)
    {
      loops = audioctrl->ac.ahiac_MaxBuffSamples - offset;
    }
    else if(offset <= es->ahiecho_Delay)
    {
      loops = audioctrl->ac.ahiac_MaxBuffSamples;
    }
    else
    {
      loops = audioctrl->ac.ahiac_MaxBuffSamples + es->ahiecho_Delay - offset;
    }

    loops = min(loops, samples);
    
    samples -= loops;
    offset  += loops;

    while(loops > 0)
    {
      /* Call echo function */

      echofunc(loops, es, &buf, &srcptr, &dstptr);

      loops--;
    }

  } // while(samples > 0)

  es->ahiecho_Offset = offset;
  es->ahiecho_SrcPtr = srcptr;
  es->ahiecho_DstPtr = dstptr;
}




/* Entry points **************************************************************/

void
do_DSPEchoMono32 ( struct Echo *es,
                   void *buf,
                   struct AHIPrivAudioCtrl *audioctrl )
{
  do_DSPEcho(es, buf, audioctrl, EchoMono32);
}

void
do_DSPEchoStereo32 ( struct Echo *es,
                     void *buf,
                     struct AHIPrivAudioCtrl *audioctrl )
{ 
  do_DSPEcho(es, buf, audioctrl, EchoStereo32);
}

