* $Id$
* $Log$
* Revision 1.1  1997/03/24 12:43:09  lcs
* Initial revision
*
*

	IFND    AHI_DSP_I
AHI_DSP_I	SET	1

	include	exec/types.i

	STRUCTURE Echo,0
	ULONG	ahiecho_Delay
	FPTR	ahiecho_Code		;The echo routine
	LONG	ahiecho_FeedbackDS	;Delayed signal to same channel
	LONG	ahiecho_FeedbackDO	;Delayed signal to other channel
	LONG	ahiecho_FeedbackNS	;Normal signal to same channel
	LONG	ahiecho_FeedbackNO	;Normal signal to other channel
	LONG	ahiecho_MixN		;Normal signal
	LONG	ahiecho_MixD		;Delayed signal
	ULONG	ahiecho_Offset		;(&Buffer-&SrcPtr)/sizeof(ahiecho_Buffer[0])
	APTR	ahiecho_SrcPtr		;Pointer to &Buffer
	APTR	ahiecho_DstPtr		;Pointer to &(Buffer[Delay])
	APTR	ahiecho_EndPtr		;Pointer to address after buffer
	ULONG	ahiecho_BufferSize	;Delay buffer size in bytes
	LABEL	ahiecho_Buffer		;Delay buffer
	LABEL	Echo_SIZEOF

	ENDC
