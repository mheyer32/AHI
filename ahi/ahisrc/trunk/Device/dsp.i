* $Id$

	IFND    _AHI_DSP_I_
_AHI_DSP_I_	SET	1

	include	exec/types.i

	STRUCTURE Echo,0
	ULONG	ahiecho_Delay
	FPTR	ahiecho_Code		;The echo routine
	Fixed	ahiecho_FeedbackDS	;Delayed signal to same channel
	Fixed	ahiecho_FeedbackDO	;Delayed signal to other channel
	Fixed	ahiecho_FeedbackNS	;Normal signal to same channel
	Fixed	ahiecho_FeedbackNO	;Normal signal to other channel
	Fixed	ahiecho_MixN		;Normal signal
	Fixed	ahiecho_MixD		;Delayed signal
	ULONG	ahiecho_Offset		;(&Buffer-&SrcPtr)/sizeof(ahiecho_Buffer[0])
	APTR	ahiecho_SrcPtr		;Pointer to &Buffer
	APTR	ahiecho_DstPtr		;Pointer to &(Buffer[Delay])
	APTR	ahiecho_EndPtr		;Pointer to address after buffer
	ULONG	ahiecho_BufferLength	;Delay buffer length in samples
	ULONG	ahiecho_BufferSize	;Delay buffer size in bytes
	LABEL	ahiecho_Buffer		;Delay buffer
	LABEL	Echo_SIZEOF

	ENDC
