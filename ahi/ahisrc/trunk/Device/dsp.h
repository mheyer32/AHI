/* $Id$ */

#ifndef _DSP_H_
#define _DSP_H_

#include <exec/types.h>
#include "ahi_def.h"


#ifdef VERSION68K

typedef void ASMCALL (*ECHOFUNC)( REG(a0, struct Echo *),
                                  REG(a1, void *),
                                  REG(a2, struct AHIPrivAudioCtrl *) );

#else

typedef void (*ECHOFUNC)( struct Echo *,
                          void *,
                          struct AHIPrivAudioCtrl * );

#endif

struct Echo
{
	ULONG	ahiecho_Delay;
	ECHOFUNC ahiecho_Code;		// The echo routine
	Fixed	ahiecho_FeedbackDS;	// Delayed signal to same channel
	Fixed	ahiecho_FeedbackDO;	// Delayed signal to other channel
	Fixed	ahiecho_FeedbackNS;	// Normal signal to same channel
	Fixed	ahiecho_FeedbackNO;	// Normal signal to other channel
	Fixed	ahiecho_MixN;		// Normal signal
	Fixed	ahiecho_MixD;		// Delayed signal
	ULONG	ahiecho_Offset;		// (&Buffer-&SrcPtr)/sizeof(ahiecho_Buffer[0])
	APTR	ahiecho_SrcPtr;		// Pointer to &Buffer
	APTR	ahiecho_DstPtr;		// Pointer to &(Buffer[Delay])
	APTR	ahiecho_EndPtr;		// Pointer to address after buffer
	ULONG	ahiecho_BufferLength;	// Delay buffer length in samples
	ULONG	ahiecho_BufferSize;	// Delay buffer size in bytes
	BYTE	ahiecho_Buffer[0];	// Delay buffer
};


#endif /* _DSP_H_ */
