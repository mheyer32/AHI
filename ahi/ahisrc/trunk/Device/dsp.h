/* $Id$
* $Log$
* Revision 4.1  1997/04/02 22:29:53  lcs
* Bumped to version 4
*
* Revision 1.1  1997/03/24 12:43:09  lcs
* Initial revision
*
*/

#ifndef DSP_H
#define DSP_H

#include <exec/types.h>

struct Echo
{
	ULONG	ahiecho_Delay;
	void	(*ahiecho_Code)(void);	// The echo routine
	LONG	ahiecho_FeedbackDS;	// Delayed signal to same channel
	LONG	ahiecho_FeedbackDO;	// Delayed signal to other channel
	LONG	ahiecho_FeedbackNS;	// Normal signal to same channel
	LONG	ahiecho_FeedbackNO;	// Normal signal to other channel
	LONG	ahiecho_MixN;		// Normal signal
	LONG	ahiecho_MixD;		// Delayed signal
	ULONG	ahiecho_Offset;		// (&Buffer-&SrcPtr)/sizeof(ahiecho_Buffer[0])
	APTR	ahiecho_SrcPtr;		// Pointer to &Buffer
	APTR	ahiecho_DstPtr;		// Pointer to &(Buffer[Delay])
	APTR	ahiecho_EndPtr;		// Pointer to address after buffer
	ULONG	ahiecho_BufferSize;	// Delay buffer size in bytes
	BYTE	ahiecho_Buffer[0];	// Delay buffer
};

#endif