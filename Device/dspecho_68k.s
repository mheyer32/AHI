* $Id$
* $Log$
* Revision 1.3  1997/03/22 18:58:07  lcs
* --background-- updated + some work on dspecho
*
* Revision 1.2  1997/03/20 02:07:02  lcs
* Weiß nicht?
*
* Revision 1.1  1997/01/31 19:12:25  lcs
* Initial revision
*

 IFGE	__CPU-68020

	incdir	include:
	include	exec/exec.i
	include	lvo/exec_lib.i
	include	devices/ahi.i
	include	utility/hooks.i
	include	lvo/utility_lib.i
	include	ahi_def.i

	XDEF	update_DSPEcho
	XDEF	free_DSPEcho

	XREF	_Fixed2Shift

***
*** DSPECHO
***

;in:
* a0	dpsecho struct
* a2	audioctrl
* a5	AHIBase

	XDEF	update_DSPEcho
update_DSPEcho:
	pushm	d2-d7/a3-a6
	move.l	a0,a3
	bsr	free_DSPEcho
	move.l	ahiac_BuffType(a2),d0
	moveq	#0,d1
	move.b	.type2shift(pc,d0),d1
	move.l	ahiede_Delay(a3),d0
	move.l	d0,d3
	lsl.l	d1,d3
	add.l	ahiac_MaxBuffSamples(a2),d0
	lsl.l	d1,d0
	move.l	d0,d2
	add.l	#AHIEcho_SIZEOF,d0
	move.l	#MEMF_PUBLIC|MEMF_CLEAR,d1
	move.l	ahib_SysLib(a5),a6
	call	AllocVec
	move.l	d0,a1
	tst.l	d0
	beq	.exit

	lea	ahiecho_Buffer(a1),a0
	move.l	d2,ahiecho_BufferSize(a1)
	add.l	a0,d2
	move.l	d2,ahiecho_EndPtr(a1)
	move.l	a0,ahiecho_SrcPtr(a1)
	add.l	a0,d3
	move.l	d3,ahiecho_DstPtr(a1)


	lea	do_DSPEchoMono32(pc),a0
	cmp.l	#AHIST_M32S,d0
	beq	.save_mono
	lea	do_DSPEchoStereo32(pc),a0
	cmp.l	#AHIST_S32S,d0
	beq	.save

	move.b	ahib_Flags(a5),d0
	and.b	#AHIBF_FASTECHO,d0
	bne	.fastecho

	move.l	ahiac_BuffType(a2),d0
	lea	do_DSPEchoMono16(pc),a0
	cmp.l	#AHIST_M16S,d0
	beq	.save_mono
	lea	do_DSPEchoStereo16(pc),a0
	cmp.l	#AHIST_S16S,d0
	beq	.save
	bra	.exit					;ERROR, unknown buffer!
.fastecho
	move.l	ahiac_BuffType(a2),d0
	lea	do_DSPEchoMono16fast(pc),a0
	cmp.l	#AHIST_M16S,d0
	beq	.save_mono

	cmp.l	#AHIST_S16S,d0
	bne	.exit					;ERROR, unknown buffer!
; Either full cross echo or none!
	lea	do_DSPEchoStereo16fast(pc),a0
;	cmp.l	#$8000,ahiede_Cross(a3)
;	blo	.save_mono
;	lea	do_DSPEchoStereo16fastCross(pc),a0
;	move.l	#$10000,ahiede_Cross(a3)
;	bra	.save
.save_mono
	clr.l	ahiede_Cross(a3)
.save
	move.l	a0,ahiecho_Code(a1)

* Delay      = ahiede_Delay
	move.l	ahiede_Delay(a3),ahiecho_Delay(a1)
* MixD       = ahiede_Mix
	move.l	ahiede_Mix(a3),d0
	lsr.w	#1,d0
	bpl.b	.0a
	subq.w	#1,d0
.0a
	move.w	d0,ahiecho_MixD(a1)
* MixN       = $10000-ahiede_Mix
	move.l	#$10000,d0
	sub.l	ahiede_Mix(a3),d0
	lsr.w	#1,d0
	bpl.b	.0b
	subq.w	#1,d0
.0b
	move.w	d0,ahiecho_MixN(a1)


* FeedbackDS = ahide_Feedback*($10000-ahide_Cross)
	move.l	ahiede_Feedback(a3),d0
	move.l	#$10000,d1
	sub.l	ahiede_Cross(a3),d1
	mulu.l	d1,d0
	bvc	.1
	moveq	#$ffffffff,d0
.1
	clr.w	d0
	swap.w	d0
	lsr.w	#1,d0
	move.w	d0,ahiecho_FeedbackDS(a1)

* FeedbackDO = ahide_Feedback*ahide_Cross
	move.l	ahiede_Feedback(a3),d0
	mulu.l	ahiede_Cross(a3),d0
	bvc	.2
	moveq	#$ffffffff,d0
.2
	clr.w	d0
	swap.w	d0
	lsr.l	#1,d0
	move.w	d0,ahiecho_FeedbackDO(a1)

* FeedbackNS = ($10000-ahide_Feedback)*($10000-ahide_Cross)
	move.l	#$10000,d0
	sub.l	ahiede_Cross(a3),d0
	move.l	#$10000,d1
	sub.l	ahiede_Feedback(a3),d1
	mulu.l	d1,d0
	bvc	.3
	moveq	#$ffffffff,d0
.3
	clr.w	d0
	swap.w	d0
	lsr.w	#1,d0
	move.w	d0,ahiecho_FeedbackNS(a1)

* FeedbackNO = ($10000-ahide_Feedback)*ahide_Cross
	move.l	ahiede_Cross(a3),d0
	mulu.l	d1,d0
	bvc	.4
	moveq	#$ffffffff,d0
.4
	clr.w	d0
	swap.w	d0
	lsr.l	#1,d0
	move.w	d0,ahiecho_FeedbackNO(a1)

	move.b	ahib_Flags(a5),d0
	and.b	#AHIBF_FASTECHO,d0
	beq	.no_fastecho
* Now get the shift-left value for each parameter
	
	moveq	#0,d0
	move.w	ahiecho_FeedbackDS(a1),d0
	bsr	_Fixed2Shift
	move.w	d0,ahiecho_FeedbackDS(a1)

	moveq	#0,d0
	move.w	ahiecho_FeedbackDO(a1),d0
	bsr	_Fixed2Shift
	move.w	d0,ahiecho_FeedbackDO(a1)

	moveq	#0,d0
	move.w	ahiecho_FeedbackNS(a1),d0
	bsr	_Fixed2Shift
	move.w	d0,ahiecho_FeedbackNS(a1)

	moveq	#0,d0
	move.w	ahiecho_FeedbackNO(a1),d0
	bsr	_Fixed2Shift
	move.w	d0,ahiecho_FeedbackNO(a1)

	moveq	#0,d0
	move.w	ahiecho_MixN(a1),d0
	bsr	_Fixed2Shift
	move.w	d0,ahiecho_MixN(a1)

	moveq	#0,d0
	move.w	ahiecho_MixD(a1),d0
	bsr	_Fixed2Shift
	move.w	d0,ahiecho_MixD(a1)
.no_fastecho

	move.l	a1,ahiac_EffDSPEchoStruct(a2)
.exit
	popm	d2-d7/a3-a6
	rts
.type2shift
	dc.b	0	;AHIST_M8S  (0)
	dc.b	1	;AHIST_M16S (1)
	dc.b	1	;AHIST_S8S  (2)
	dc.b	2	;AHIST_S16S (3)
	dc.b	0	;AHIST_M8U  (4)
	dc.b	0
	dc.b	0
	dc.b	0
	dc.b	2	;AHIST_M32S (8)
	dc.b	0
	dc.b	3	;AHIST_S32S (10)

	even

	XDEF	free_DSPEcho
free_DSPEcho:
	push	a6
	move.l	ahib_SysLib(a5),a6
	move.l	ahiac_EffDSPEchoStruct(a2),a1
	clr.l	ahiac_EffDSPEchoStruct(a2)
	call	FreeVec
	pop	a6
	rts


**************
* Inputs: ahiede_Delay, ahiede_Feedback, ahiede_Mix, ahiede_Cross
*
* Delay      = ahide_Delay
* MixN       = $10000-ahide_Mix
* MixD       = ahide_Mix
* FeedbackDS = (ahide_Feedback*($10000-ahide_Cross))/2
* FeedbackDO = (ahide_Feedback*ahide_Cross)/2
* FeedbackNS = (($10000-ahide_Feedback)*($10000-ahide_Cross))/2
* FeedbackNO = (($10000-ahide_Feedback)*ahide_Cross)/2
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

test:
	lea	.ahibase,a6
	move.l	4.w,ahib_SysLib(a6)
	lea	.actrl,a2
	move.l	#AHIST_S16S,ahiac_BuffType(a2)
	move.l	#5,ahiac_MaxBuffSamples(a2)
	move.l	#5,ahiac_BuffSamples(a2)

	lea	.echostruct(pc),a0
	bsr	update_DSPEcho

	lea	.buffer,a1
	move.l	#$40000000,(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16

	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16
	lea	.buffer,a1
	clr.l	4*0(a1)
	clr.l	4*1(a1)
	clr.l	4*2(a1)
	clr.l	4*3(a1)
	clr.l	4*4(a1)
	clr.l	4*5(a1)
	bsr	do_DSPEchoStereo16



	bsr	free_DSPEcho
	rts

.ahibase
	blk.b	AHIBase_SIZEOF
.actrl
	blk.b	AHIPrivAudioCtrl_SIZEOF,0
.echostruct
	dc.l	AHIET_DSPECHO			; ahie_Effect
	dc.l	3				; ahiede_Delay
	dc.l	$8000				; ahiede_Feedback
	dc.l	$10000				; ahiede_Mix
	dc.l	$0				; ahiede_Cross
.buffer
	dc.w	$0000,$0000
	blk.l	5


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



do_DSPEchoMono16:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get sample x[n]
	move.w	d0,d1
	muls.w	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get delayed sample d[n-N]
	move.w	d3,d2
	muls.w	ahiecho_MixD(a0),d3
	add.l	d0,d3
	asr.l	#8,d3
	asr.l	#8-1,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.w	#1,d2				;Fix for -1
	muls.w	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N]

	muls.w	ahiecho_FeedbackNS(a0),d1
	add.l	d1,d2				;d2=...+FeedbackNS*x[n]
	swap.w	d2

	move.w	d2,(a6)+			;store d2
	DSPECHO_POST

do_DSPEchoMono16fast:
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

do_DSPEchoStereo16:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get left sample x[n]
	move.w	d0,d1
	muls.w	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get left delayed sample d[n-N]
	move.w	d3,d2
	muls.w	ahiecho_MixD(a0),d3
	add.l	d0,d3
	asr.l	#8,d3
	asr.l	#8-1,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	(a1),d0				;Get right sample x[n]
	move.w	d0,d4
	muls.w	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get right delayed sample d[n-N]
	move.w	d3,d5
	muls.w	ahiecho_MixD(a0),d3
	add.l	d0,d3
	asr.l	#8,d3
	asr.l	#8-1,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.w	#1,d2				;Fix for -1
	move.w	d2,d0
	muls.w	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N] (left)
	muls.w	ahiecho_FeedbackDO(a0),d0

	addq.w	#1,d5				;Fix for -1
	move.w	d5,d3
	muls.w	ahiecho_FeedbackDS(a0),d5	;d5=FeedbackDS*d[n-N] (right)
	muls.w	ahiecho_FeedbackDO(a0),d3

	add.l	d3,d2				;d2=...+FeedbackDO*d[n-N]
	add.l	d0,d5				;d5=...+FeedbackDO*d[n-N]

	move.w	d1,d0
	muls.w	ahiecho_FeedbackNS(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNS*x[n]
	muls.w	ahiecho_FeedbackNO(a0),d1
	add.l	d1,d5				;d5=...+FeedbackNO*x[n]

	move.w	d4,d0
	muls.w	ahiecho_FeedbackNO(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNO*x[n]
	muls.w	ahiecho_FeedbackNS(a0),d4
	add.l	d4,d5				;d5=...+FeedbackNS*x[n]

;	asl.l	#1,d2
;	asr.l	#8,d5
;	asr.l	#8-1,d5
	swap.w	d5
	move.w	d5,d2
	move.l	d2,(a6)+			;store d1 and d4
	DSPECHO_POST

do_DSPEchoStereo16fast:				;No cross echo!
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

.echoloop
	move.w	(a1),d0				;Get left sample x[n]
	move.w	d0,d1
	move.w	ahiecho_MixN(a0),d6
	asr.w	d6,d0
	move.w	(a3)+,d3			;Get left delayed sample d[n-N]
	move.w	d3,d2
	move.w	ahiecho_MixD(a0),d6
	asr.w	d6,d3
	add.w	d0,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	(a1),d0				;Get right sample x[n]
	move.w	d0,d4
	move.w	ahiecho_MixN(a0),d6
	asr.w	d6,d0
	move.w	(a3)+,d3			;Get right delayed sample d[n-N]
	move.w	d3,d5
	move.w	ahiecho_MixD(a0),d6
	asr.w	d6,d3
	add.w	d0,d3
	move.w	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	ahiecho_FeedbackDS(a0),d6
	asr.w	d6,d2				;d2=FeedbackDS*d[n-N] (left)
	asr.w	d6,d5				;d5=FeedbackDS*d[n-N] (right)

	move.w	ahiecho_FeedbackNS(a0),d6
	asr.w	d6,d1
	add.w	d1,d2				;d2=...+FeedbackNS*x[n]

	asr.w	d6,d4
	add.w	d4,d5				;d5=...+FeedbackNS*x[n]

	move.w	d2,(a6)+			;store d2 (left)
	move.w	d5,(a6)+			;store d1 (right)
	DSPECHO_POST


do_DSPEchoMono32:
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
	move.w	d0,d1
	muls.w	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get delayed sample d[n-N]
	move.w	d3,d2
	muls.w	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.w	#1,d2				;Fix for -1
	muls.w	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N]

	muls.w	ahiecho_FeedbackNS(a0),d1
	add.l	d1,d2				;d2=...+FeedbackNS*x[n]

	move.w	d2,(a6)+			;store d2
	DSPECHO_POST

do_DSPEchoStereo32:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

*** The delay buffer is only 16 bit, and only the upper word of the 32 bit
*** input data is used!

.echoloop
	move.w	(a1),d0				;Get left sample x[n] (high word)
	move.w	d0,d1
	muls.w	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get left delayed sample d[n-N]
	move.w	d3,d2
	muls.w	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	move.w	(a1),d0				;Get right sample x[n] (high word)
	move.w	d0,d4
	muls.w	ahiecho_MixN(a0),d0
	move.w	(a3)+,d3			;Get right delayed sample d[n-N]
	move.w	d3,d5
	muls.w	ahiecho_MixD(a0),d3
	add.l	d0,d3
	move.l	d3,(a1)+			;x[n]=MixN*x[n]+MixD*d[n-N]

	addq.w	#1,d2				;Fix for -1
	move.w	d2,d0
	muls.w	ahiecho_FeedbackDS(a0),d2	;d2=FeedbackDS*d[n-N] (left)
	muls.w	ahiecho_FeedbackDO(a0),d0

	addq.w	#1,d5				;Fix for -1
	move.w	d5,d3
	muls.w	ahiecho_FeedbackDS(a0),d5	;d5=FeedbackDS*d[n-N] (right)
	muls.w	ahiecho_FeedbackDO(a0),d3

	add.l	d3,d2				;d2=...+FeedbackDO*d[n-N]
	add.l	d0,d5				;d5=...+FeedbackDO*d[n-N]

	move.w	d1,d0
	muls.w	ahiecho_FeedbackNS(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNS*x[n]
	muls.w	ahiecho_FeedbackNO(a0),d1
	add.l	d1,d5				;d5=...+FeedbackNO*x[n]

	move.w	d4,d0
	muls.w	ahiecho_FeedbackNO(a0),d0
	add.l	d0,d2				;d2=...+FeedbackNO*x[n]
	muls.w	ahiecho_FeedbackNS(a0),d4
	add.l	d4,d5				;d5=...+FeedbackNS*x[n]

	swap.w	d5
	move.w	d5,d2
	move.l	d2,(a6)+			;store d1 and d4
	DSPECHO_POST


 ENDC
