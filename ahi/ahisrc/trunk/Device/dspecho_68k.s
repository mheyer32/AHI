* $Id$
* $Log$
* Revision 1.4  1997/03/24 12:41:51  lcs
* Echo rewritten
*
*


	include	exec/types.i

	include	ahi_def.i
	include dsp.i

	XDEF	_do_DSPEchoMono16
	XDEF	_do_DSPEchoMono16Fast
	XDEF	_do_DSPEchoStereo16
	XDEF	_do_DSPEchoStereo16Fast
	XDEF	_do_DSPEchoMono32
	XDEF	_do_DSPEchoStereo32
	XDEF	_do_DSPEchoMono16NCFM
	XDEF	_do_DSPEchoStereo16NCFM
	XDEF	_do_DSPEchoMono16NCFMFast
	XDEF	_do_DSPEchoStereo16NCFMFast

 IFGE	__CPU-68020

***
*** DSPECHO
***

**************
* Inputs: ahiede_Delay, ahiede_Feedback, ahiede_Mix, ahiede_Cross
*
* Delay      = ahide_Delay
* MixN       = 1-ahide_Mix
* MixD       = ahide_Mix
* FeedbackDO = ahide_Feedback*ahide_Cross
* FeedbackDS = ahide_Feedback*(1-ahide_Cross)
* FeedbackNO = (1-ahide_Feedback)*ahide_Cross
* FeedbackNS = (1-ahide_Feedback)*(1-ahide_Cross)
*
*                                               |\
* left in ->---+----------+---------------------| >---->(+)----> left out
*              |          |                MixN |/       ^
*  FeedbackNO \¯/        \¯/ FeedbackNS                  |
*              v          v                              |
*              |          |                              |
*              |          v    |¯¯¯|            |\       |
*              |    +--->(+)-->| T |----+-------| >------+
*              |    |     ^    |___|    |  MixD |/
*              |    |     |    Delay    |
*              |    |     |             |
*              |    |     |      /|     |
*              |    |     +-----< |-----+
*              |    | FeedbackDS \|     |
*              |    |                   |
*              |    |            /|     |
*             (+)<--(-----------< |-----+
*              |    |            \| FeedbackDO
*              |    |
*              |    |
*              |    |
*              |    |            /| FeedbackDO
*              |   (+)<---------< |-----+
*              |    |            \|     |
*              |    |                   |
*              |    | FeedbackDS /|     |
*              |    |     +-----< |-----+
*              |    |     |      \|     |
*              |    |     |             |
*              |    |     v    |¯¯¯|    |       |\
*              +----(--->(+)-->| T |----+-------| >------+
*                   |     ^    |___|       MixD |/       |
*                   |     |    Delay                     |
*                   ^     ^                              |
*       FeedbackNO /_\   /_\ FeedbackNS                  |
*                   |     |                     |\       v
* right in ->-------+-----+---------------------| >---->(+)----> right out
*                                          MixN |/
*
*
**************
*
* The delay buffer: (BuffSamples = 5, Delay = 8 Total size = 13
*
*  1) Delay times
*
*  +---------+
*  |         |
*  v         ^
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* *---------*
*
*  2) BuffSamples times
*
*  +-Mix-----------+
*  |               |
*  ^               v
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* *---------*
*
* Or optimized using a circular buffer:
*
*
* Offset<BuffSamples => BuffSamples-Offset times:
*
*  +-Mix-----------+
*  |               |
*  ^               v
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* *---------*
*
* BuffSamples<=Offset<=Delay => BuffSamples times:
*
*  +-Mix-----+
*  |         |
*  v         ^
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
*           *---------*
*
* Offset>Delay => BuffSamples+Delay-Offset times:
*
*          +-Mix-----+
*          |         |
*          v         ^
* |_|_|_|_|_|_|_|_|_|_|_|_|_|
* --*               *--------
*
* The delay buffer: (BuffSamples = 5, Delay = 3 Total size = 8
*
* Offset<BuffSamples => BuffSamples-Offset times:
*
*  +-Mix-+
*  |     |
*  ^     v
* |_|_|_|_|_|_|_|_|
* *---------*
*
* Offset>=BuffSamples => BuffSamples+Delay-Offset times:
*
*  +-----Mix-+
*  |         |
*  v         ^
* |_|_|_|_|_|_|_|_|
* ----*     *------
*
*
*
* Algoritm:
*
*   LoopsLeft=BuffSamples
*   Offset=0
*   Src=E
*   Dst=E+Delay
* Loop:
*   If LoopsLeft <= 0 GOTO Exit
*   IF Src >= (E + BuffSamples + Delay) THEN Src = Src - (BuffSamples + Delay)
*   IF Dst >= (E + BuffSamples + Delay) THEN Dst = Dst - (BuffSamples + Delay)
*   IF Offset >= (BuffSamples + Delay) THEN Offset = Offset - (BuffSamples + Delay)
*
*   IF Offset < BuffSamples THEN LoopTimes = BuffSamples-Offset : GOTO Echo
*   IF Offset <= Delay THEN LoopTimes = BuffSamples : GOTO Echo 
*   LoopTimes = BuffSamples+Delay-Offset
* Echo:
*   LoopTimes = min(LoopTimes,LoopsLeft)
*   Echo LoopTimes samples
*
*   Src = Src + LoopTimes
*   Dst = Dst + LoopTimes
*   Offset = Offset + LoopTimes
*   LoopsLeft = LoopsLeft - LoopTimes
*   GOTO Loop
* Exit:
*

;in:
* a1	Buffer pointer
* a2	audioctrl
;Don't trash a2, a4 or a5

*******************************************************************************

DSPECHO_PRE	MACRO
LOCALSIZE	SET	4
	subq.l	#LOCALSIZE,sp			;Local variable

	move.l	ahiac_EffDSPEchoStruct(a2),a0

	move.l	ahiac_BuffSamples(a2),(sp)	;Looped
	move.l	ahiecho_Offset(a0),d6
	move.l	ahiecho_SrcPtr(a0),a3
	move.l	ahiecho_DstPtr(a0),a6
.loop
	tst.l	(sp)
	ble	.exit
	cmp.l	ahiecho_EndPtr(a0),a3
	blo	.src_ok
	sub.l	ahiecho_BufferSize(a0),a3
.src_ok
	cmp.l	ahiecho_EndPtr(a0),a6
	blo	.dst_ok
	sub.l	ahiecho_BufferSize(a0),a6
.dst_ok
	move.l	ahiac_BuffSamples(a2),d0
	add.l	ahiecho_Delay(a0),d0
	cmp.l	d0,d6
	blo	.offs_ok
	sub.l	d0,d6
.offs_ok
	cmp.l	ahiac_BuffSamples(a2),d6
	bhs	.hi_buffsamples
	move.l	ahiac_BuffSamples(a2),d7
	sub.l	d6,d7
	bra	.echo
.hi_buffsamples
	cmp.l	ahiecho_Delay(a0),d6
	bhi	.hi_delay
	move.l	ahiac_BuffSamples(a2),d7
	bra	.echo
.hi_delay
	move.l	ahiac_BuffSamples(a2),d7
	add.l	ahiecho_Delay(a0),d7
	sub.l	d6,d7

.echo
	sub.l	d7,(sp)
	bpl	.loopsleft_ok
	add.l	(sp),d7				;Gives Max((sp),d7)
	clr.l	(sp)
.loopsleft_ok
	add.l	d7,d6
	move.l	d6,ahiecho_Offset(a0)
	subq.l	#1,d7
	bmi	.exit				;This is an error (should not happen)
	ENDM


DSPECHO_POST	MACRO
	dbf	d7,.echoloop
	bra	.loop
.exit
	move.l	a3,ahiecho_SrcPtr(a0)
	move.l	a6,ahiecho_DstPtr(a0)

	addq.l	#LOCALSIZE,sp
	rts
	ENDM



_do_DSPEchoMono16:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get sample x[n]
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get delayed sample d[n-N]
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	swap.w	d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.l	#1,d2				;Fix for -1
	muls.l	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N]

	muls.l	ahiecho_FeedbackNS(a0),d1
	add.l	d1,d2				;d2=...+FeedbackNS*x[n]
	swap.w	d2

	move.w	d2,(a6)+			;store d2
	DSPECHO_POST


_do_DSPEchoMono16Fast:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)


	move.w	ahiecho_MixN(a0),d4
	move.w	ahiecho_MixD(a0),d5
	move.w	ahiecho_FeedbackDS(a0),d6
.echoloop
	move.w	(a1),d0				;Get sample x[n]
	move.w	d0,d1
	asr.w	d4,d0	
	move.w	(a3)+,d3			;Get delayed sample d[n-N]
	move.w	d3,d2
	asr.w	d5,d3
	add.w	d0,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	asr.w	d6,d2				;d2=FeedbackDS*d[n-N]

	move.w	ahiecho_FeedbackNS(a0),d0
	asr.w	d0,d1
	add.w	d1,d2				;d2=...+FeedbackNS*x[n]

	move.w	d2,(a6)+			;store d2
	DSPECHO_POST


_do_DSPEchoStereo16:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get left sample x[n]
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get left delayed sample d[n-N]
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	swap.w	d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	(a1),d0				;Get right sample x[n]
	ext.l	d0
	move.l	d0,d4
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get right delayed sample d[n-N]
	ext.l	d3
	move.l	d3,d5
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	swap.w	d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.l	#1,d2				;Fix for -1
	move.l	d2,d0
	muls.l	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N] (left)
	muls.l	ahiecho_FeedbackDO(a0),d0

	addq.l	#1,d5				;Fix for -1
	move.l	d5,d3
	muls.l	ahiecho_FeedbackDS(a0),d5	;d5=FeedbackDS*d[n-N] (right)
	muls.l	ahiecho_FeedbackDO(a0),d3

	add.l	d3,d2				;d2=...+FeedbackDO*d[n-N]
	add.l	d0,d5				;d5=...+FeedbackDO*d[n-N]

	move.l	d1,d0
	muls.l	ahiecho_FeedbackNS(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNS*x[n]
	muls.l	ahiecho_FeedbackNO(a0),d1
	add.l	d1,d5				;d5=...+FeedbackNO*x[n]

	move.l	d4,d0
	muls.l	ahiecho_FeedbackNO(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNO*x[n]
	muls.l	ahiecho_FeedbackNS(a0),d4
	add.l	d4,d5				;d5=...+FeedbackNS*x[n]

	swap.w	d5
	move.w	d5,d2
	move.l	d2,(a6)+			;store d2 and d5
	DSPECHO_POST


_do_DSPEchoStereo16Fast:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get left sample x[n]
	move.w	d0,d1
	move.l	ahiecho_MixN(a0),d6
	asr.w	d6,d0
	move.w	(a3)+,d3			;Get left delayed sample d[n-N]
	move.w	d3,d2
	move.l	ahiecho_MixD(a0),d6
	asr.w	d6,d3
	add.w	d0,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	(a1),d0				;Get right sample x[n]
	move.w	d0,d4
	move.l	ahiecho_MixN(a0),d6
	asr.w	d6,d0
	move.w	(a3)+,d3			;Get right delayed sample d[n-N]
	move.w	d3,d5
	move.l	ahiecho_MixD(a0),d6
	asr.w	d6,d3
	add.w	d0,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.w	#1,d2				;Fix for -1
	move.w	d2,d0
	move.l	ahiecho_FeedbackDS(a0),d6
	asr.w	d6,d2				;d2=FeedbackDS*d[n-N] (left)
	move.l	ahiecho_FeedbackDO(a0),d6
	asr.w	d6,d0

	addq.w	#1,d5				;Fix for -1
	move.w	d5,d3
	move.l	ahiecho_FeedbackDS(a0),d6
	asr.w	d6,d5				;d5=FeedbackDS*d[n-N] (right)
	move.l	ahiecho_FeedbackDO(a0),d6
	asr.w	d6,d3

	add.w	d3,d2				;d2=...+FeedbackDO*d[n-N]
	add.w	d0,d5				;d5=...+FeedbackDO*d[n-N]

	move.w	d1,d0
	move.l	ahiecho_FeedbackNS(a0),d6
	asr.w	d6,d0
	add.w	d0,d2				;d2=...+FeedbackNS*x[n]
	move.l	ahiecho_FeedbackNO(a0),d6
	asr.w	d6,d1
	add.w	d1,d5				;d5=...+FeedbackNO*x[n]

	move.w	d4,d0
	move.l	ahiecho_FeedbackNO(a0),d6
	asr.w	d6,d0
	add.w	d0,d2				;d2=...+FeedbackNO*x[n]
	move.l	ahiecho_FeedbackNS(a0),d6
	asr.w	d4,d6
	add.w	d4,d5				;d5=...+FeedbackNS*x[n]

	swap.w	d5
	move.w	d2,d5
	move.l	d5,(a6)+			;store d2 and d5
	DSPECHO_POST


_do_DSPEchoMono32:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

*** The delay buffer is only 16 bit, and only the upper word of the 32 bit
*** input data is used!

.echoloop
	move.w	(a1),d0				;Get sample x[n] (high word)
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get delayed sample d[n-N]
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.l	#1,d2				;Fix for -1
	muls.l	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N]

	muls.l	ahiecho_FeedbackNS(a0),d1
	add.l	d1,d2				;d2=...+FeedbackNS*x[n]

	swap.w	d2
	move.w	d2,(a6)+			;store d2
	DSPECHO_POST

_do_DSPEchoStereo32:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

*** The delay buffer is only 16 bit, and only the upper word of the 32 bit
*** input data is used!

.echoloop
	move.l	(a1),d0				;Get left sample x[n] (high word)
	ext.l	d0
	move.l	d0,d1
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get left delayed sample d[n-N]
	ext.l	d3
	move.l	d3,d2
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	(a1),d0				;Get right sample x[n] (high word)
	ext.l	d0
	move.l	d0,d4
	muls.l	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get right delayed sample d[n-N]
	ext.l	d3
	move.l	d3,d5
	muls.l	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.l	#1,d2				;Fix for -1
	move.l	d2,d0
	muls.l	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N] (left)
	muls.l	ahiecho_FeedbackDO(a0),d0

	addq.l	#1,d5				;Fix for -1
	move.l	d5,d3
	muls.l	ahiecho_FeedbackDS(a0),d5	;d5=FeedbackDS*d[n-N] (right)
	muls.l	ahiecho_FeedbackDO(a0),d3

	add.l	d3,d2				;d2=...+FeedbackDO*d[n-N]
	add.l	d0,d5				;d5=...+FeedbackDO*d[n-N]

	move.l	d1,d0
	muls.l	ahiecho_FeedbackNS(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNS*x[n]
	muls.l	ahiecho_FeedbackNO(a0),d1
	add.l	d1,d5				;d5=...+FeedbackNO*x[n]

	move.l	d4,d0
	muls.l	ahiecho_FeedbackNO(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNO*x[n]
	muls.l	ahiecho_FeedbackNS(a0),d4
	add.l	d4,d5				;d5=...+FeedbackNS*x[n]

	swap.w	d5
	move.w	d5,d2
	move.l	d2,(a6)+			;store d1 and d4
	DSPECHO_POST


 ENDC



* Delay      = ahide_Delay
* FeedbackDS = ahide_Feedback
* FeedbackNS = (1-ahide_Feedback)
*
*
* left in ->--------------+                              +-----> left out
*                         |                              |
*                        \¯/ (FeedbackNS)                |
*                         v                              |
*                         |                              |
*                         v    |¯¯¯|                     |
*                        (+)-->| T |----+----------------+
*                         ^    |___|    |
*                         |    Delay    |
*                         |             |
*                         |      /|     |
*                         +-----< |-----+
*                     FeedbackDS \|
*
*
*
*
*
* NOTE!! We tweak the MasterVolume, so we can ignore FeedbackNS! Clever, eh? ;-)
*
*
*
*
*
*                     FeedbackDS /|
*                         +-----< |-----+
*                         |      \|     |
*                         |             |
*                         v    |¯¯¯|    |
*                        (+)-->| T |----+----------------+
*                         ^    |___|                     |
*                         |    Delay                     |
*                         ^                              |
*                        /_\ (FeedbackNS)                |
*                         |                              |
* right in ->-------------+                              +-----> right out
*


_do_DSPEchoMono16NCFM:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get sample x[n]
	ext.l	d0
	move.w	(a3)+,d1			;Get delayed sample d[n-N]
	move.w	d1,(a1)+
	ext.l	d1
	muls.l	ahiecho_FeedbackDS(a0),d1	;d3=FeedbackDS*d[n-N]
	add.l	d0,d1
	swap.w	d1
	move.w	d1,(a6)+
	DSPECHO_POST

_do_DSPEchoMono16NCFMFast:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

	move.l	ahiecho_FeedbackDS(a0),d2
.echoloop
	move.w	(a1),d0				;Get sample x[n]
	move.w	(a3)+,d1			;Get delayed sample d[n-N]
	move.w	d1,(a1)+
	asr.w	d2,d1				;d3=FeedbackDS*d[n-N]
	add.w	d0,d1
	move.w	d1,(a6)+
	DSPECHO_POST


_do_DSPEchoStereo16NCFM:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get sample x[n] (left)
	ext.l	d0
	move.w	(a3)+,d1			;Get delayed sample d[n-N]
	move.w	d1,(a1)+
	ext.l	d1
	muls.l	ahiecho_FeedbackDS(a0),d1	;d3=FeedbackDS*d[n-N]
	add.l	d0,d1
	swap.w	d1
	move.w	d1,(a6)+

	move.w	(a1),d0				;Get sample x[n] (right)
	ext.l	d0
	move.w	(a3)+,d1			;Get delayed sample d[n-N]
	move.w	d1,(a1)+
	ext.l	d1
	muls.l	ahiecho_FeedbackDS(a0),d1	;d3=FeedbackDS*d[n-N]
	add.l	d0,d1
	swap.w	d1
	move.w	d1,(a6)+
	DSPECHO_POST


_do_DSPEchoStereo16NCFMFast:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

	move.l	ahiecho_FeedbackDS(a0),d2
	move.l	ahiecho_FeedbackDS(a0),d3
.echoloop
	move.w	(a1),d0				;Get sample x[n]
	move.w	(a3)+,d1			;Get delayed sample d[n-N]
	move.w	d1,(a1)+
	asr.w	d2,d1				;d3=FeedbackDS*d[n-N]
	add.w	d0,d1
	move.w	d1,(a6)+

	move.w	(a1),d0				;Get sample x[n]
	move.w	(a3)+,d1			;Get delayed sample d[n-N]
	move.w	d1,(a1)+
	asr.w	d3,d1				;d3=FeedbackDS*d[n-N]
	add.w	d0,d1
	move.w	d1,(a6)+
	DSPECHO_POST
