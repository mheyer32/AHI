* $Id$
* $Log$
* Revision 1.3  1997/01/04 20:25:02  lcs
* ...I forgot: AHIET_CHANNELINFO effect added
*
* Revision 1.2  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*


 IFD	_PHXASS_

DEBUG	EQU	0	;no debug 
ALIGN	EQU	1	;move code to even 128 bit boundary

 ELSE

DEBUG	EQU	1	;debug when using asmone!
ALIGN	EQU	0	;set to 0 when using the source level debugger, and 1 when timing
 ENDC

	incdir	include:
	include	exec/exec.i
	include	lvo/exec_lib.i
	include	devices/ahi.i
	include	utility/hooks.i
	include	lvo/utility_lib.i
	include	ahi_def.i

	XREF	_UtilityBase

	XDEF	initcode
	XDEF	logtable

	XDEF	initSignedTable
	XDEF	calcSignedTable
	XDEF	initUnsignedTable
	XDEF	calcUnsignedTable
	XDEF	SelectAddRoutine
	XDEF	_Mix
	XDEF	CalcSamples

	XDEF	UDivMod64

	XDEF	do_DSPEchoMono16
	XDEF	do_DSPEchoStereo16
	XDEF	do_DSPEchoMono32
	XDEF	do_DSPEchoStereo32

TABLEMAXVOL	EQU	32
TABLESHIFT	EQU	11	(TABLEMAXVOL<<TABLESHIFT == 0x10000)

;-----------------------------------------

 IFNE	DEBUG
SAMPLES	=30
BUFFER	=(SAMPLES*2+7)&(~8)

 	include	devices/timer.i
 	include	lvo/timer_lib.i
J:
 	base	exec
	bsr	initcode
	call	CacheClearU

	lea	audioctrl(pc),a2
	move.l	#AHIACF_STEREO|AHIACF_HIFI,ahiac_Flags(a2)
	move.w	#1,ahiac_Channels(a2)
	move.w	ahiac_Channels(a2),ahiac_Channels2(a2)
	move.l	#SAMPLES,ahiac_BuffSamples(a2)
	move.l	#BUFFER,ahiac_BuffSize(a2)
	move.l	ahiac_BuffSize(a2),ahiac_BuffSizeNow(a2)
	move.l	#$10000,ahiac_MasterVolume(a2)
	move.l	#channeldatas,ahiac_ChannelDatas(a2)
	move.l	#soundfunc,ahiac_SoundFunc(a2)
	bsr	initSignedTable
	bsr	initUnsignedTable

	move.l	ahiac_ChannelDatas(a2),a5

	move.l	cd_AddI(a5),d0
	move.w	cd_AddF(a5),d1
	move.l	cd_Type(a5),d2
	move.l	cd_LastOffsetI(a5),d3
	move.w	cd_LastOffsetF(a5),d4
	move.l	cd_OffsetI(a5),d5
	move.w	cd_OffsetF(a5),d6
	bsr	CalcSamples
	move.l	d0,cd_Samples(a5)

;	lea	buffer(pc),a1
;	bsr	Mix

;	move.l	#$10000,d0
;	move.l	#$10000,d1
;	lea	audioctrl(pc),a2
;;	bsr	FixVolWordSVPH
;;	add.w	(a0),a0
;	move.l	#AHIST_M8S,d2
;	bsr	SelectAddRoutine
;	move.l	d2,a0
;
;	move.l	d1,d2
;	move.l	d0,d1
;	move.l	#SAMPLES,d0
;	moveq	#0,d3
;	moveq	#0,d4
;	moveq	#0,d5
;	move.l	#$4000,d6
;	lea	audioctrl(pc),a2
;	lea	sample(pc),a3
;	lea	buffer(pc),a4
;
;	base	exec
;	call	Disable
;
;	pushm	d0-d1
;	bsr	ReadEClock
;	move.l	d0,eclock1
;	popm	d0-d1
;
;	jsr	(a0)
;
;	bsr	ReadEClock
;	move.l	d0,eclock2
;
;	base	exec
;	call	Enable

exit:
	base	exec

	lea	audioctrl(pc),a2
	move.l	ahiac_MultTableS(a2),a1
	clr.l	ahiac_MultTableS(a2)
	call	FreeVec

	move.l	ahiac_MultTableU(a2),a1
	clr.l	ahiac_MultTableU(a2)
	call	FreeVec
	move.l	eclock1,d0
	sub.l	eclock2,d0
	bpl.b	.ok
	add.l	#65536,d0
.ok
	lsr.l	#4,d0
	rts

audioctrl:
	blk.l	AHIPrivAudioCtrl_SIZEOF,0
eclock1	dc.l	0
eclock2	dc.l	0

ReadEClock:
	moveq	#0,d0
	move.b	$bfe701,d0
	move.b	$bfe601,d1
	cmp.b	$bfe701,d0
	bne.b	ReadEClock
	lsl.w	#8,d0
	move.b	d1,d0
	rts

soundfunc:
	dc.l	0,0
	dc.l	.func
	dc.l	0,0
.func
	rts

BACK=1
* AHIChannelData (private)
channeldatas:
 IFEQ BACK
	dc.w	0		;UWORD	cd_EOS
	dc.b	$ff		;UBYTE	cd_FreqOK
	dc.b	$ff		;UBYTE	cd_SoundOK
	dc.l	0		;ULONG	cd_OffsetI
	dc.w	0		;UWORD	cd_Pad1
	dc.w	0		;UWORD	cd_OffsetF
	dc.l	0		;ULONG	cd_AddI
	dc.w	0		;UWORD	cd_Pad2
	dc.w	$8000		;UWORD	cd_AddF
	dc.l	sample		;APTR	cd_DataStart
	dc.l	sample_len-1	;ULONG	cd_LastOffsetI
	dc.w	0		;UWORD	cd_Pad3
	dc.w	$ffff		;UWORD	cd_LastOffsetF
	dc.l	256		;ULONG	cd_ScaleLeft
	dc.l	0		;ULONG	cd_ScaleRight
	dc.l	AddByteMV	;FPTR	cd_AddRoutine
	dc.l	$10000		;LONG	cd_VolumeLeft
	dc.l	0		;LONG	cd_VolumeRight
	dc.l	AHIST_M8S	;ULONG	cd_Type

	dc.w	0		;UWORD	cd_NextEOS
	dc.b	$ff		;UBYTE	cd_NextFreqOK
	dc.b	$ff		;UBYTE	cd_NextSoundOK
	dc.l	0		;ULONG	cd_NextOffsetI
	dc.w	0		;UWORD	cd_NextPad1
	dc.w	0		;UWORD	cd_NextOffsetF
	dc.l	0		;ULONG	cd_NextAddI
	dc.w	0		;UWORD	cd_NextPad2
	dc.w	$8000		;UWORD	cd_NextAddF
	dc.l	sample		;APTR	cd_NextDataStart
	dc.l	sample_len-1	;ULONG	cd_NextLastOffsetI
	dc.w	0		;UWORD	cd_NextPad3
	dc.w	$ffff		;UWORD	cd_NextLastOffsetF
	dc.l	256		;ULONG	cd_NextScaleLeft
	dc.l	0		;ULONG	cd_NextScaleRight
	dc.l	AddByteMV	;FPTR	cd_NextAddRoutine
	dc.l	$10000		;LONG	cd_NextVolumeLeft
	dc.l	0		;LONG	cd_NextVolumeRight
	dc.l	AHIST_M8S	;ULONG	cd_NextType

	dc.l	0		;ULONG	cd_LCommand
	dc.l	0		;ULONG	cd_LAddress
	dc.l	0		;ULONG	cd_Samples
	dc.l	0		;ULONG	cd_FirstOffsetI
	dc.w	0		;WORD	cd_LastSample
 ELSE
	dc.w	0		;UWORD	cd_EOS
	dc.b	$ff		;UBYTE	cd_FreqOK
	dc.b	$ff		;UBYTE	cd_SoundOK
	dc.l	sample_len-1	;ULONG	cd_OffsetI
	dc.w	0		;UWORD	cd_Pad1
	dc.w	$ffff		;UWORD	cd_OffsetF
	dc.l	0		;ULONG	cd_AddI
	dc.w	0		;UWORD	cd_Pad2
	dc.w	$8000		;UWORD	cd_AddF
	dc.l	sample		;APTR	cd_DataStart
	dc.l	0		;ULONG	cd_LastOffsetI
	dc.w	0		;UWORD	cd_Pad3
	dc.w	0		;UWORD	cd_LastOffsetF
	dc.l	256		;ULONG	cd_ScaleLeft
	dc.l	0		;ULONG	cd_ScaleRight
	dc.l	AddByteBMV	;FPTR	cd_AddRoutine
	dc.l	$10000		;LONG	cd_VolumeLeft
	dc.l	0		;LONG	cd_VolumeRight
	dc.l	AHIST_BW|AHIST_M8S	;ULONG	cd_Type

	dc.w	0		;UWORD	cd_NextEOS
	dc.b	$ff		;UBYTE	cd_NextFreqOK
	dc.b	$ff		;UBYTE	cd_NextSoundOK
	dc.l	sample_len-1	;ULONG	cd_NextOffsetI
	dc.w	0		;UWORD	cd_NextPad1
	dc.w	$ffff		;UWORD	cd_NextOffsetF
	dc.l	0		;ULONG	cd_NextAddI
	dc.w	0		;UWORD	cd_NextPad2
	dc.w	$8000		;UWORD	cd_NextAddF
	dc.l	sample		;APTR	cd_NextDataStart
	dc.l	0		;ULONG	cd_NextLastOffsetI
	dc.w	0		;UWORD	cd_NextPad3
	dc.w	0		;UWORD	cd_NextLastOffsetF
	dc.l	256		;ULONG	cd_NextScaleLeft
	dc.l	0		;ULONG	cd_NextScaleRight
	dc.l	AddByteBMV	;FPTR	cd_NextAddRoutine
	dc.l	$10000		;LONG	cd_NextVolumeLeft
	dc.l	0		;LONG	cd_NextVolumeRight
	dc.l	AHIST_BW|AHIST_M8S	;ULONG	cd_NextType

	dc.l	0		;ULONG	cd_LCommand
	dc.l	0		;ULONG	cd_LAddress
	dc.l	0		;ULONG	cd_Samples
	dc.l	0		;ULONG	cd_FirstOffsetI
	dc.w	0		;WORD	cd_LastSample
 ENDC * BACK 
 ENDC

;-----------------------------------------

;in:
* a6	ExecBase
initcode:

* Init ²log-table
	lea	logtable(pc),a0
	clr.w	(a0)+
	moveq	#1,d0
	moveq	#1,d1
.1
	move.l	d0,d2
	subq.l	#1,d2
.2
	move.b	d1,(a0)+
	dbf	d2,.2
	lsl.b	#1,d0
	addq.b	#1,d1
	cmp.b	#9,d1
	bne.b	.1

 IFNE	ALIGN
* Align Add#? routines to even 16-byte address
	lea	AlignStart(pc),a0
	move.l	a0,d0
	and.b	#$f0,d0
	move.l	d0,a1
	move.l	#AlignEnd-AlignStart-1,d0
.11
	move.b	(a0)+,(a1)+
	dbf	d0,.11
* Update relative pointers
	lea	AlignStart(pc),a0
	move.l	a0,d0
	and.w	#$0f,d0
	lea	OffsetTable(pc),a0
.12
	tst.w	(a0)
	bmi.b	.13
	sub.w	d0,(a0)+
	bra.b	.12
.13
	call	CacheClearU
 ENDC
	rts


;in:
* a2	ptr to AHIAudioCtrl
* a5	ptr to AHIBase
initSignedTable:
	pushm	d0-a6

	move.l	ahiac_Flags(a2),d0
	btst.l	#AHIACB_MULTTAB,d0
	beq.b	.notable
	tst.l	ahiac_MultTableS(a2)
	bne.b	.notable			;there is already a table!

 IFEQ	DEBUG
	move.l	ahib_SysLib(a5),a6
 ELSE
 	base	exec
 ENDC
	move.l	#256*(TABLEMAXVOL+1)*4,d0	;include highest volume, too!
	moveq	#MEMF_PUBLIC,d1			;may be accessed from interrupts
	call	AllocVec
	move.l	d0,ahiac_MultTableS(a2)
	beq.b	.notable
	bsr.b	calcSignedTable
.notable
	popm	d0-a6
	rts

* Create multiplication table for use with signed bytes/words.
*
* WORD hints:
* // 0<=v<=32 // (h*256+l)*(v*2048)>>16 = ((h*v)<<8+(l*v))>>5
*s	=$fffffcf4
*h	=s>>8		;signed
*l	=s&255		;unsigned
*v	=16
*	PRINTV	h*v,l*v
*	PRINTV	((h*v)<<8+(l*v))>>5
*
* Usage: a0 points to correct line in table
*
*        d1 is signed byte with bits 8-15 cleared:
*        move.w  0(a0,d1.w*4),d2		;d2 is signed!
*
*        d1 is unsigned byte with bits 8-15 cleared:
*        move.w  2(a0,d1.w*4),d2		;d2 is unsigned!
calcSignedTable:
	pushm	d0-d4/a0
	move.l	ahiac_MultTableS(a2),d0
	beq.b	.notable
	move.l	d0,a0
	add.l	#256*(TABLEMAXVOL+1)*4,a0
	move.w	ahiac_Channels2(a2),d3
	lsl.w	#8,d3
	move.l	ahiac_MasterVolume(a2),d4	; Range: (0 .. 1.0 .. ??) * 65536
	lsr.l	#8,d4
	moveq.l	#(TABLEMAXVOL+1)-1,d0
.10
	move.w	#255,d1
.11
	move.w	d1,d2				; Unsigned
	mulu.w	d0,d2
	mulu.w	d4,d2				; *((Mastervolume*65536)/256)
	lsl.l	#TABLESHIFT-8,d2
	divu.w	d3,d2				; /(Channels*256)
	move.w	d2,-(a0)

	move.b	d1,d2 				; Signed
	ext.w	d2
	muls.w	d0,d2
	muls.w	d4,d2				; *((Mastervolume*65536)/256)
	asl.l	#TABLESHIFT-8,d2
	divs.w	d3,d2				; /(Channels*256)
	move.w	d2,-(a0)

	dbf	d1,.11
	dbf	d0,.10
.notable
	popm	d0-d4/a0
	rts
;in:
* a2	ptr to AHIAudioCtrl
* a5	ptr to AHIBase
initUnsignedTable:
	pushm	d0-a6
	move.l	ahiac_Flags(a2),d0
	btst.l	#AHIACB_MULTTAB,d0
	beq.b	.notable
	tst.l	ahiac_MultTableU(a2)
	bne.b	.notable			;there is already a table!

 IFEQ	DEBUG
	move.l	ahib_SysLib(a5),a6
 ELSE
 	base	exec
 ENDC
	move.l	#256*(TABLEMAXVOL+1)*2,d0	;incude highest volume, too!
	moveq	#MEMF_PUBLIC,d1			;may be accessed from interrupts
	call	AllocVec
	move.l	d0,ahiac_MultTableU(a2)
	beq.b	.notable
	bsr.b	calcUnsignedTable
.notable
	popm	d0-a6
	rts

* Create multiplication table for use with unsigned bytes
* Usage: a0 points to correct line in table
*
*        d1 is unsigned byte with bits 8-15 cleared:
*        move.w  0(a0,d1.w*2),d2		;d2 is signed!
calcUnsignedTable:
	pushm	d0-d4/a0
	move.l	ahiac_MultTableU(a2),d0
	beq.b	.notable
	move.l	d0,a0
	add.l	#256*(TABLEMAXVOL+1)*2,a0
	move.w	ahiac_Channels2(a2),d3
	lsl.w	#8,d3
	move.l	ahiac_MasterVolume(a2),d4	; Range: (0 .. 1.0 .. ??) * 65536
	lsr.l	#8,d4
	moveq.l	#(TABLEMAXVOL+1)-1,d0
.10
	move.w	#255,d1
.11
	move.b	d1,d2				; Unsigned
	sub.b	#$80,d2				; -> Signed
	ext.w	d2
	muls.w	d0,d2
	muls.w	d4,d2				; *((Mastervolume*65536)/256)
	asl.l	#TABLESHIFT-8,d2
	divs.w	d3,d2				; /(Channels*256)
	move.w	d2,-(a0)

	dbf	d1,.11
	dbf	d0,.10
.notable
	popm	d0-d4/a0
	rts


;in:
* d0	VolumeLeft (Fixed)
* d1	VolumeRight (Fixed)
* d2	SampleType with AHIST_LOOP bit set or cleared
* a2	AudioCtrl
;out:
* d0	ScaleLeft
* d1	ScaleRight
* d2	AddRoutine
SelectAddRoutine:
	pushm	d3-a6

	pea	sa_exit(pc)			;return address

	and.l	#~AHIST_LOOP,d2			;Don't care about that bit.
	move.l	d2,d3
	and.l	#~AHIST_BW,d2
	move.l	d2,d6
	moveq	#0,d2
	move.w	ahiac_Channels2(a2),d2
	lsl.w	#8,d2
	move.l	ahiac_Flags(a2),d4
	move.l	ahiac_MasterVolume(a2),d5
	asr.l	#8,d5

* Check for volume 0
	tst.l	ahiac_MasterVolume(a2)
	beq.b	.off
	tst.l	d0
	bne.b	.not_off
	tst.l	d1
	bne.b	.not_off
.off
	bra	FixVolSilence
.not_off
 IFGE	__CPU-68020
	btst.l	#AHIACB_HIFI,d4
	bne.w	sa_hifi
 ENDC

	cmp.l	#AHIST_M8U,d6
	beq	sa_unsigned

sa_signed:
	tst.l	d0
	bmi	sa_signed_notable
	tst.l	d1
	bmi	sa_signed_notable
	tst.l	ahiac_MultTableS(a2)
	bne	sa_signed_table
sa_signed_notable
 IFGE	__CPU-68020
	muls.l	d5,d0
	divs.l	d2,d0
	muls.l	d5,d1
	divs.l	d2,d1
 ELSE
	move.l	_UtilityBase(pc),a0
	
	push	d1
	move.l	d5,d1
	jsr	_LVOSMult32(a0)
	move.l	d2,d1
	jsr	_LVOSDivMod32(a0)
	pop	d1

	push	d0
	move.l	d1,d0
	move.l	d5,d1
	jsr	_LVOSMult32(a0)
	move.l	d2,d1
	jsr	_LVOSDivMod32(a0)
	move.l	d0,d1
	pop	d0
 ENDC
	btst.l	#AHIACB_STEREO,d4
	bne.b	.stereo
	cmp.l	#AHIST_M8S,d6
	bne.b	.mono16
	bsr	d0ispowerof2
	beq	FixVolByteM
	bra	FixVolByteMV
.mono16
	bsr	d0ispowerof2
	beq	FixVolWordM
	bra	FixVolWordMV
.stereo
	cmp.l	#AHIST_M8S,d6
	bne.b	.stereo16
	tst.l	d0
	beq.b	.onlyright8
	tst.l	d1
	beq.b	.onlyleft8
	bra	FixVolByteSVP
.onlyright8
	bsr	d1ispowerof2
	beq	FixVolByteSr
	bra	FixVolByteSVr
.onlyleft8
	bsr	d0ispowerof2
	beq	FixVolByteSl
	bra	FixVolByteSVl
.stereo16
	tst.l	d0
	beq.b	.onlyright16
	tst.l	d1
	beq.b	.onlyleft16
	bra	FixVolWordSVP
.onlyright16
	bsr	d1ispowerof2
	beq	FixVolWordSr
	bra	FixVolWordSVr
.onlyleft16
	bsr	d0ispowerof2
	beq	FixVolWordSl
	bra	FixVolWordSVl


sa_signed_table:
	btst.l	#AHIACB_STEREO,d4
	bne.b	.stereo
	cmp.l	#AHIST_M8S,d6
	bne	FixVolWordMVT
	bra	FixVolByteMVT
.stereo
	cmp.l	#AHIST_M8S,d6
	bne.b	.stereo16
	tst.l	d0
	beq	FixVolByteSVTr
	tst.l	d1
	beq	FixVolByteSVTl
	bra	FixVolByteSVPT	
.stereo16
	tst.l	d0
	beq	FixVolWordSVTr
	tst.l	d1
	beq	FixVolWordSVTl
	bra	FixVolWordSVPT


sa_unsigned:
	tst.l	d0
	bmi	sa_unsigned_notable
	tst.l	d1
	bmi	sa_unsigned_notable
	tst.l	ahiac_MultTableU(a2)
	bne.b	sa_unsigned_table
sa_unsigned_notable
 IFGE	__CPU-68020
	muls.l	d5,d0
	divs.l	d2,d0
	muls.l	d5,d1
	divs.l	d2,d1
 ELSE
	move.l	_UtilityBase(pc),a0
	
	push	d1
	move.l	d5,d1
	jsr	_LVOSMult32(a0)
	move.l	d2,d1
	jsr	_LVOSDivMod32(a0)
	pop	d1

	push	d0
	move.l	d1,d0
	move.l	d5,d1
	jsr	_LVOSMult32(a0)
	move.l	d2,d1
	jsr	_LVOSDivMod32(a0)
	move.l	d0,d1
	pop	d0
 ENDC
	btst.l	#AHIACB_STEREO,d4
	bne.b	.stereo
	bsr.w	d0ispowerof2
	beq	FixVolUByteM
	bra	FixVolUByteMV
.stereo
	tst.l	d0
	beq.b	.onlyright
	tst.l	d1
	beq.b	.onlyleft
	bra	FixVolUByteSVP
.onlyright
	bsr.w	d1ispowerof2
	beq	FixVolUByteSr
	bra	FixVolUByteSVr
.onlyleft
	bsr.w	d0ispowerof2
	beq	FixVolUByteSl
	bra	FixVolUByteSVl

sa_unsigned_table:
	btst.l	#AHIACB_STEREO,d4
	bne.b	.stereo
	bra	FixVolUByteMVT
.stereo
	tst.l	d0
	beq	FixVolUByteSVTr
	tst.l	d1
	beq	FixVolUByteSVTl
	bra	FixVolUByteSVPT	

 IFGE	__CPU-68020
sa_hifi:
	muls.l	d5,d0
	divs.l	d2,d0
	muls.l	d5,d1
	divs.l	d2,d1

	btst.l	#AHIACB_STEREO,d4
	bne.b	.stereo
	cmp.l	#AHIST_M8U,d6
	beq	FixVolUByteMVH
	cmp.l	#AHIST_M8S,d6
	beq	FixVolByteMVH
	bra	FixVolWordMVH
.stereo
	cmp.l	#AHIST_M8U,d6
	beq	FixVolUByteSVPH
	cmp.l	#AHIST_M8S,d6
	beq	FixVolByteSVPH
	bra	FixVolWordSVPH
 ENDC

sa_exit:
	add.w	(a0),a0
	move.l	a0,d2
sa_quit:
	popm	d3-a6
	rts

;out:
* z	set if power of 2
d1ispowerof2:
	pushm	d0-d1
	move.l	d1,d0
	bra.b	ispowerof2
d0ispowerof2:
	pushm	d0-d1
ispowerof2:
	cmp.l	#$10000,d0
	beq	.exit
	cmp.l	#$8000,d0
	beq.b	.exit
	cmp.l	#$4000,d0
	beq.b	.exit
	cmp.l	#$2000,d0
	beq.b	.exit
	cmp.l	#$1000,d0
	beq.b	.exit
	cmp.l	#$800,d0
	beq.b	.exit
	cmp.l	#$400,d0
	beq.b	.exit
	cmp.l	#$200,d0
	beq.b	.exit
	cmp.l	#$100,d0
	beq.b	.exit
	cmp.l	#$80,d0
	beq.b	.exit
	cmp.l	#$40,d0
	beq.b	.exit
	cmp.l	#$20,d0
	beq.b	.exit
	cmp.l	#$10,d0
	beq.b	.exit
	cmp.l	#$8,d0
	beq.b	.exit
	cmp.l	#$4,d0
	beq.b	.exit
	cmp.l	#$2,d0
	beq.b	.exit
	cmp.l	#$1,d0
	beq	.exit
.exit
	popm	d0-d1			;does not trash CCR! D1 är med för att assemblern inte ska få för sej att optimera!
	rts





*
* The mixing routine mixes ahiac_BuffSamples each pass, fitting in
* ahiac_BuffSizeNow bytes. ahiac_BuffSizeNow must be an even multiplier
* of 8.
*

;in:
* a0	Hook
* a1	Mixing buffer (size is 8 byte aligned)
* a2	AHIAudioCtrl
_Mix:
	pushm	d0-a6

* Clear the buffer
	move.l	ahiac_BuffSizeNow(a2),d0
	lsr.l	#3,d0
	subq.w	#1,d0
	move.l	a1,a0
.clearbuff
	clr.l	(a0)+
	clr.l	(a0)+
	dbf	d0,.clearbuff

* Mix the samples
	clr.b	ahiac_WetOrDry(a2)
	move.l	ahiac_WetList(a2),d0
	move.l	d0,a5
	beq	.do_dry

.nextchannel
	move.l	ahiac_BuffSamples(a2),d0
	move.l	a1,a4			;output buffer
.contchannel
	tst.w	cd_EOS(a5)
	beq	.notEOS

* Loop sound
	move.l	cd_Type(a5),d1
	cmp.l	#AHIST_NOTYPE,d1
	beq	.loop_exit
	and.l	#AHIST_LOOP,d1
	beq	.loop_exit
	move.l	cd_LCommand(a5),d1
	beq	.loop_exit			;LCommand=0 => Loop forever
	not.l	d1
	bne	.loop_not_off
	clr.b	cd_SoundOK(a5)			;LCommand=~0 => Turn channel off
	bra	.loop_exit
.loop_not_off
	subq.w	#1,cd_LCommand+2(a5)
	bne	.loop_exit
 * Advance to next structure
	move.l	cd_LAddress(a5),a3
	add.w	#AHIMultiLoop_SIZEOF,a3
	move.l	a3,cd_LAddress(a5)
	move.l	(a3)+,cd_LCommand(a5)
	move.l	(a3)+,d0
	move.l	d0,cd_NextOffsetI(a5)
	add.l	(a3)+,d0
	move.l	d0,cd_NextLastOffsetI(a5)
.loop_exit

* Call Sound Hook
	pushm	d0/a1
	move.l	ahiac_SoundFunc(a2),d0	;a2 ready
	beq	.noSoundFunc
	move.l	d0,a0
	lea	cd_ChannelNo(a5),a1
	move.l	h_Entry(a0),a3
	jsr	(a3)
.noSoundFunc
	popm	d0/a1
	clr.w	cd_EOS(a5)		;clear EOS flag
.notEOS


	movem.l	(a5),d1/d3/d4/d5/d6/a3	;Flags,OffsI,OffsF,AddI,AddF,DataStart
	not.w	d1			;FreqOK and SoundOK must both be $FF
	bne	.channel_done		;No sound or freq not set yet.

	cmp.l	cd_Samples(a5),d0
	blo	.wont_reach_end

* How may samples left?
	sub.l	cd_Samples(a5),d0

	push	d0
	move.l	cd_Samples(a5),d0

	movem.l	cd_ScaleLeft(a5),d1/d2/a0
	jsr	(a0)
	move.l	cd_TempLastSample(a5),cd_LastSample(a5)	;linear interpol. stuff

; d3:d4 always points OUTSIDE the sample after this call. Ie, if we read a
; sample at offset d3 now, it does not belong to the sample just played.
; This is true for both backward and forward mixing.
; Oh, btw... OffsetF is unsigned, so -0.5 is expressed as -1:$8000.

; What we do now is to calculate how much futher we have advanced.
	sub.w	cd_LastOffsetF(a5),d4
	move.l	cd_LastOffsetI(a5),d0
	subx.l	d0,d3

; d3:d4 should now be added to the NEXT OffsetI (and OffsetF which is 0 when
; every sample begin).
; d3:d4 is positive of the sample was mixed forwards, and negative if the sample
; was mixed backwards. There is one catch, however. If the direction is about
; to change now, d3:d4 should instead be SUBTRACTED. Let's check:
	move.l	cd_Type(a5),d0
	move.l	cd_NextType(a5),d1
	eor.l	d0,d1
	and.l	#AHIST_BW,d1			;filter the BW bit
	beq.b	.same_type
	neg.l	d3
	neg.w	d4
.same_type

; Ok, now we add.
	add.w	cd_NextOffsetF(a5),d4
	move.l	cd_NextOffsetI(a5),d0
	addx.l	d0,d3
	move.l	d3,cd_OffsetI(a5)
	move.w	d4,cd_OffsetF(a5)
	move.l	d3,cd_FirstOffsetI(a5)

; But what if the next sample is so short that we just passed it!?
; Here is the nice part. CalcSamples checks this,
; and set cd_Samples to 0 in that case. And the add routines doesn't
; do anything when asked to mix 0 samples.
; Assume we have passed a sample with 4 samples, and the next one
; is only 3. CalcSamples returns 0. The 'jsr (a0)' call above does
; not do anything at all, OffsetI is still 4. Now we subtract LastOffsetI,
; which is 3. Result: We have passed the sample with 1. And guess what?
; That's in range.

; Now, let's copy the rest of the cd_Next#? stuff...
	move.l	cd_NextFlags(a5),cd_Flags(a5)
	movem.l	cd_NextAddI(a5),d0-d7/a0/a3/a6	; AddI,AddF,DataStart,LastOffsetI,LastOffsetF,ScaleLeft,ScaleRight,AddRoutine, VolumeLeft,VolumeRight,Type
	movem.l	d0-d7/a0/a3/a6,cd_AddI(a5)

					;AddI/AddF,LastOffsetI/LastOffsetF ok
	move.l	a6,d2			;Type
	movem.l	cd_OffsetI(a5),d5/d6	;cd_OffsetI/cd_OffsetF
	bsr	CalcSamples
	move.l	d0,cd_Samples(a5)

	pop	d0
	move.w	#$ffff,cd_EOS(a5)	;signal End-Of-Sample
	bra.w	.contchannel		;same channel, new sound

.wont_reach_end
	sub.l	d0,cd_Samples(a5)
	movem.l	cd_ScaleLeft(a5),d1/d2/a0
	jsr	(a0)
	movem.l	d3/d4,cd_OffsetI(a5)		;update Offset

.channel_done
	move.l	cd_Succ(a5),d0		;next channel in list
	move.l	d0,a5
	bne.w	.nextchannel
	tst.b	ahiac_WetOrDry(a2)
	bne.b	.exit			;Both wet and dry finished
	addq.b	#1,ahiac_WetOrDry(a2)	;Mark dry

*** AHIET_DSPECHO
	move.l	ahiac_EffDSPEchoStruct(a2),d0
	beq	.noEffDSPEcho
	move.l	d0,a0
	move.l	ahiecho_Code(a0),a0
	jsr	(a0)
.noEffDSPEcho

.do_dry
	move.l	ahiac_DryList(a2),d0
	move.l	d0,a5
	beq	.exit
	btst.b	#AHIACB_POSTPROC-24,ahiac_Flags(a2)
	beq	.nextchannel
	move.l	a4,a1			;New block
	bra	.nextchannel

.exit

*** AHIET_OUTPUTBUFFER
	move.l	ahiac_EffOutputBufferStruct(a2),d0
	beq	.noEffOutputBuffer
	move.l	a1,a4
	move.l	d0,a1
	move.l	ahieob_Func(a1),a0
	move.l	a4,ahieob_Buffer(a1)
	move.l	ahiac_BuffSamples(a2),ahieob_Length(a1)
	move.l	ahiac_BuffType(a2),ahieob_Type(a1)
	move.l	h_Entry(a0),a3
	jsr	(a3)
.noEffOutputBuffer

*** AHIET_CHANNELINFO
	move.l	ahiac_EffChannelInfoStruct(a2),d0
	beq	.noEffChannelInfo
	move.l	d0,a1
	move.l	ahieci_Func(a1),a0
	move.w	ahieci_Channels(a1),d0
	subq.w	#1,d0
	lea	ahieci_Offset(a1),a3
	move.l	ahiac_ChannelDatas(a2),a4
.ci_loop
	move.l	cd_OffsetI(a4),(a3)+
	add.w	#AHIChannelData_SIZEOF,a4
	dbf	d0,.ci_loop
	move.l	h_Entry(a0),a3
	jsr	(a3)
.noEffChannelInfo


	popm	d0-a6
	rts


 IFGE	__CPU-68020

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
	XREF	update_DSPEcho
	XREF	free_DSPEcho

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
	subq.l	#1,d7
	bmi	.exit				;This is an error (should not happen)
.echoloop
	ENDM


DSPECHO_POST	MACRO
	dbf	d7,.echoloop
	bra	.loop
.exit
	move.l	d6,ahiecho_Offset(a0)
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

	move.w	d2,(a6)+			;store d2
	DSPECHO_POST


do_DSPEchoStereo16:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

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



do_DSPEchoMono32:
	DSPECHO_PRE
;in:
* d0-d6	Scratch
* a1	& x[n]           (Advance)
* a3	& d[n-N]         (Advance)
* a6	& d[n]           (Advance)

*** The delay buffer is only 16 bit, and only the upper word of the 32 bit
*** input data is used!

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

;in:
* d0	AddI
* d1.w	AddF
* d2	Type
* d3	LastOffsetI
* d4.w	LastOffsetF
* d5	OffsetI
* d6.w	OffsetF
;ut:
* d0	Samples
CalcSamples:
* Calc how many samples it will take when mixed (Samples=Length×Rate)

	and.l	#AHIST_BW,d2
	bne.b	.backwards
	sub.w	d6,d4
	subx.l	d5,d3
	bra.b	.1
.backwards
	sub.w	d4,d6
	subx.l	d3,d5
	move.w	d6,d4
	move.l	d5,d3
.1
	bmi.b	.error
	swap.w	d4
	move.w	d3,d4
	swap.w	d4
	clr.w	d3
	swap.w	d3
; d3:d4 is now (positive) length <<16

	swap	d0
	move.w	d1,d0
	tst.l	d0
	beq.b	.error
; d0 is now rate<<16

 IFGE	__CPU-68020
	divu.l	d0,d3:d4
	move.l	d4,d0
 ELSE
	move.l	d3,d1
	move.l	d4,d2
	bsr	UDivMod64		;d0 = (d1:d2)/d0
 ENDC
	addq.l	#1,d0
	rts
.error
	moveq	#0,d0
	rts

;UDivMod64 -- unsigned 64 by 32 bit division
;             64 bit quotient, 32 bit remainder.
; (d1:d2)/d0 = d0, d1 remainder.

UDivMod64:
	movem.l	d3-d7,-(sp)
	move.l	d0,d7
	moveq	#0,d0
	move.l	#$80000000,d3
	move.l	#$00000000,d4
	moveq	#0,d5			;result
	moveq	#0,d6			;result

.2
	lsl.l	#1,d2
	roxl.l	#1,d1
	roxl.l	#1,d0
	sub.l	d7,d0
	bmi	.3
	or.l	d3,d5
	or.l	d4,d6
	skipw
.3
	add.l	d7,d0

	lsr.l	#1,d3
	roxr.l	#1,d4
	bcc	.2

	move.l	d5,d1
	move.l	d6,d0
	movem.l	(sp)+,d3-d7
	rts


* ALL FIXVOL RUTINES:
;in:
* d0	VolumeLeft (Fixed)
* d1	VolumeRight (Fixed)
* d3	Sample type (or:ed with AHIST_BW if backward play)
* a2	AudioCtrl
;out:
* d0	ScaleLeft
* d1	ScaleRight (if needed)
* a0	Pointer to correct Offs#? label

FixVolSilence:
	lea	OffsSilence(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteM:
	moveq	#1,d2
	cmp.l	#$10000,d0
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d0,d2
.ok
	cmp.w	#256,d2
	bhi.b	FixVolSilence
	lea	logtable(pc),a0
	moveq	#8,d0
	sub.b	(a0,d2.w),d0
	lea	OffsByteM(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteM:
	moveq	#1,d2
	cmp.l	#$10000,d0
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d0,d2
.ok
	cmp.w	#256,d2
	bhi.b	FixVolSilence
	lea	logtable(pc),a0
	moveq	#8,d0
	sub.b	(a0,d2.w),d0
	lea	OffsUByteM(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteMV:
	lsr.l	#8,d0
	lea	OffsByteMV(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteMVH:
	lea	OffsByteMVH(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteMV:
	lsr.l	#8,d0
	lea	OffsUByteMV(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteMVH:
	lea	OffsUByteMVH(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteMVT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsByteMVT(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteMVT:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+1),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+1),d0
	add.l	a0,d0
	lea	OffsUByteMVT(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSl:
	moveq	#1,d2
	cmp.l	#$10000,d0
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d0,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#8,d0
	sub.b	(a0,d2.w),d0
	lea	OffsByteSl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSr:
	moveq	#1,d2
	cmp.l	#$10000,d1
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d1,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#8,d1
	sub.b	(a0,d2.w),d1
	lea	OffsByteSr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSl:
	moveq	#1,d2
	cmp.l	#$10000,d0
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d0,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#8,d0
	sub.b	(a0,d2.w),d0
	lea	OffsUByteSl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSr:
	moveq	#1,d2
	cmp.l	#$10000,d1
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d1,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#8,d1
	sub.b	(a0,d2.w),d1
	lea	OffsUByteSr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVl:
	lsr.l	#8,d0
	lea	OffsByteSVl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVr:
	lsr.l	#8,d1
	lea	OffsByteSVr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVP:
	lsr.l	#8,d0
	lsr.l	#8,d1
	lea	OffsByteSVP(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVPH:
	lea	OffsByteSVPH(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVl:
	lsr.l	#8,d0
	lea	OffsUByteSVl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVr:
	lsr.l	#8,d1
	lea	OffsUByteSVr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVP:
	lsr.l	#8,d0
	lsr.l	#8,d1
	lea	OffsUByteSVP(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVPH:
	lea	OffsUByteSVPH(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVTl:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsByteSVTl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVTr:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsByteSVTr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolByteSVPT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsByteSVPT(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVTl:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+1),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+1),d0
	add.l	a0,d0
	lea	OffsUByteSVTl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVTr:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+1),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+1),d1
	add.l	a0,d1
	lea	OffsUByteSVTr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolUByteSVPT:
	move.l	ahiac_MultTableU(a2),a0
	lsr.l	#TABLESHIFT-(8+1),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+1),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+1),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+1),d1
	add.l	a0,d1
	lea	OffsUByteSVPT(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordM:
	moveq	#1,d2
	cmp.l	#$10000,d0
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d0,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#0,d0
	move.b	(a0,d2.w),d0
	lea	OffsWordM(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordMV:
 IFGE	__CPU-68020
	lsr.l	#1,d0
 ELSE
	lsr.l	#8,d0
 ENDC
	lea	OffsWordMV(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordMVH:
	lea	OffsWordMVH(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordMVT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsWordMVT(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSl:
	moveq	#1,d2
	cmp.l	#$10000,d0
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d0,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#0,d0
	move.b	(a0,d2.w),d0
	lea	OffsWordSl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSr:
	moveq	#1,d2
	cmp.l	#$10000,d1
	beq.b	.ok
	move.l	#$10000,d2
	divu.w	d1,d2
.ok
	cmp.w	#256,d2
	bhi.w	FixVolSilence
	lea	logtable(pc),a0
	moveq	#0,d1
	move.b	(a0,d2.w),d1
	lea	OffsWordSr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVl:
 IFGE	__CPU-68020
	lsr.l	#1,d0
 ELSE
	lsr.l	#8,d0
 ENDC
	lea	OffsWordSVl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVr:
 IFGE	__CPU-68020
	lsr.l	#1,d1
 ELSE
	lsr.l	#8,d1
 ENDC
	lea	OffsWordSVr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVP:
 IFGE	__CPU-68020
 	lsr.l	#1,d0
 	lsr.l	#1,d1
 ELSE
	lsr.l	#8,d0
	lsr.l	#8,d1
 ENDC
	lea	OffsWordSVP(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVPH:
	lea	OffsWordSVPH(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVTl:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lea	OffsWordSVTl(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVTr:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsWordSVTr(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

FixVolWordSVPT:
	move.l	ahiac_MultTableS(a2),a0
	lsr.l	#TABLESHIFT-(8+2),d0
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d0
	add.l	a0,d0
	lsr.l	#TABLESHIFT-(8+2),d1
	and.l	#(TABLEMAXVOL*2-1)<<(8+2),d1
	add.l	a0,d1
	lea	OffsWordSVPT(pc),a0
	and.w	#AHIST_BW,d3
	beq.b	.fw
	add.w	#OffsetBackward,a0
.fw
	rts

;------------------------------------------------------------------------------
* Overview: - M=Mono S=Stereo V=Volume P=Panning T=Table H=HiFi
*             l=Left r=Right
*
* Routine	Speed¹	Memory	Volume	Panning
*
* AddSilence	0	None	=0	-
*
* AddByteM	278	None	2^x	-
* AddByteMV	403	None	±8 bit	-
* AddByteMVH		None	±15 bit	-
* AddByteMVT	250	²	5 bit ²	-
* AddByteSl	361	None	2^x	No (Left)
* AddByteSr	340	None	2^x	No (Right)
* AddByteSVl	455	None	±8 bit	No (Left)
* AddByteSVr	474	None	±8 bit	No (Right)
* AddByteSVP	704	None	±8 bit	Yes
* AddByteSVPH		None	±15 bit	Yes
* AddByteSVTl	327	²	5 bit ²	No (Left)
* AddByteSVTr	324	²	5 bit ²	No (Right)
* AddByteSVPT	384	²	5 bit ²	Yes
*
* AddUByteM	296	None	2^x	-
* AddUByteMV	421	None	±8 bit	-
* AddUByteMVH		None	±16 bit	-
* AddUByteMVT	250	²	5 bit ²	-
* AddUByteSl	366	None	2^x	No (Left)
* AddUByteSr	371	None	2^x	No (Right)
* AddUByteSVl	486	None	±8 bit	No (Left)
* AddUByteSVr	490	None	±8 bit	No (Right)
* AddUByteSVP	738	None	±8 bit	Yes
* AddUByteSVPH		None	±16 bit	Yes
* AddUByteSVTl	327	²	5 bit ²	No (Left)
* AddUByteSVTr	324	²	5 bit ²	No (Right)
* AddUByteSVPT	384	²	5 bit ²	Yes
*
* AddWordM	243	None	2^x	-
* AddWordMV	438	None	±15 bit	-
* AddWordMVH		None	±16 bit	Yes
* AddWordMVT	374	²	5 bit ²	-
* AddWordSl	327	None	2^x	No (Left)
* AddWordSr	318	None	2^x	No (Right)
* AddWordSVl	523	None	±15 bit	No (Left)
* AddWordSVr	523	None	±15 bit	No (Right)
* AddWordSVP	837	None	±15 bit	Yes
* AddWordSVPH		None	±16 bit	Yes
* AddWordSVTl	452	²	5 bit ²	No (Left)
* AddWordSVTr	452	²	5 bit ²	No (Right)
* AddWordSVPT	685	²	5 bit ²	Yes
*
*
* ¹) Number of CIAA timer B ticks (PAL) it takes to process
*    10000 samples, divided by 16. Lower is faster. NOTE: The samples
*    are all the same, which makes the table routines look a little
*    better than they would in real life, where the cache cannot be
*    fully used.
*    System data:
*       A4000/040 25 Mhz, all caches on.
*       Code, samples and mixing buffer in Fast RAM.
*       All interrupts off - no multitasking.
* ²) Multiplication table, using 256*(TABLEMAXVOL+1)*4 bytes 
*    (includes 0 and TABLEMAXVOL itself!).
* ³) Multiplication table, using 256*(TABLEMAXVOL+1)*2 bytes 
*    (includes 0 and TABLEMAXVOL itself!).
*
;------------------------------------------------------------------------------
* ALL ADD ROUTINES:
;in:
* d0.l	Samples (scratch)
* d3	Offset Integer
* d4.w	Offset Fraction (upper word cleared!)
* d5	Add Integer (s)
* d6.w	Add Fraction (s) (upper word cleared!)
* d7	Scratch
* a0	Scratch
* a2	AHIAudioCtrl (only used by AddSilence routines)
* a3	Sample data (s)
* a4	Output buffer
* a5	ChannelDatas (only used by HiFi routines)
* a6	Scratch
;out:
* d3	Updated Offset Integer
* d4	Updated Offset Fraction
* a4	Updated buffer pointer (where to store next sample)
;------------------------------------------------------------------------------

OffsetTable:
OffsSilence:	dc.w	AddSilence-*

OffsByteM:	dc.w	AddByteM-*
OffsByteMV:	dc.w	AddByteMV-*
OffsByteMVH:	dc.w	AddByteMVH-*
OffsByteMVT:	dc.w	AddByteMVT-*
OffsByteSl:	dc.w	AddByteSl-*
OffsByteSr:	dc.w	AddByteSr-*
OffsByteSVl:	dc.w	AddByteSVl-*
OffsByteSVr:	dc.w	AddByteSVr-*
OffsByteSVP:	dc.w	AddByteSVP-*
OffsByteSVPH:	dc.w	AddByteSVPH-*
OffsByteSVTl:	dc.w	AddByteSVTl-*
OffsByteSVTr:	dc.w	AddByteSVTr-*
OffsByteSVPT:	dc.w	AddByteSVPT-*

OffsUByteM:	dc.w	AddUByteM-*
OffsUByteMV:	dc.w	AddUByteMV-*
OffsUByteMVH:	dc.w	AddUByteMVH-*
OffsUByteMVT:	dc.w	AddUByteMVT-*
OffsUByteSl:	dc.w	AddUByteSl-*
OffsUByteSr:	dc.w	AddUByteSr-*
OffsUByteSVl:	dc.w	AddUByteSVl-*
OffsUByteSVr:	dc.w	AddUByteSVr-*
OffsUByteSVP:	dc.w	AddUByteSVP-*
OffsUByteSVPH:	dc.w	AddUByteSVPH-*
OffsUByteSVTl:	dc.w	AddUByteSVTl-*
OffsUByteSVTr:	dc.w	AddUByteSVTr-*
OffsUByteSVPT:	dc.w	AddUByteSVPT-*

OffsWordM:	dc.w	AddWordM-*
OffsWordMV:	dc.w	AddWordMV-*
OffsWordMVH:	dc.w	AddWordMVH-*
OffsWordMVT:	dc.w	AddWordMVT-*
OffsWordSl:	dc.w	AddWordSl-*
OffsWordSr:	dc.w	AddWordSr-*
OffsWordSVl:	dc.w	AddWordSVl-*
OffsWordSVr:	dc.w	AddWordSVr-*
OffsWordSVP:	dc.w	AddWordSVP-*
OffsWordSVPH:	dc.w	AddWordSVPH-*
OffsWordSVTl:	dc.w	AddWordSVTl-*
OffsWordSVTr:	dc.w	AddWordSVTr-*
OffsWordSVPT:	dc.w	AddWordSVPT-*


OffsetBackward	EQU	*-OffsetTable
OffsSilenceB:	dc.w	AddSilenceB-*

OffsByteBM:	dc.w	AddByteBM-*
OffsByteBMV:	dc.w	AddByteBMV-*
OffsByteBMVH:	dc.w	AddByteBMVH-*
OffsByteBMVT:	dc.w	AddByteBMVT-*
OffsByteBSl:	dc.w	AddByteBSl-*
OffsByteBSr:	dc.w	AddByteBSr-*
OffsByteBSVl:	dc.w	AddByteBSVl-*
OffsByteBSVr:	dc.w	AddByteBSVr-*
OffsByteBSVP:	dc.w	AddByteBSVP-*
OffsByteBSVPH:	dc.w	AddByteBSVPH-*
OffsByteBSVTl:	dc.w	AddByteBSVTl-*
OffsByteBSVTr:	dc.w	AddByteBSVTr-*
OffsByteBSVPT:	dc.w	AddByteBSVPT-*

OffsUByteBM:	dc.w	AddUByteBM-*
OffsUByteBMV:	dc.w	AddUByteBMV-*
OffsUByteBMVH:	dc.w	AddUByteBMVH-*
OffsUByteBMVT:	dc.w	AddUByteBMVT-*
OffsUByteBSl:	dc.w	AddUByteBSl-*
OffsUByteBSr:	dc.w	AddUByteBSr-*
OffsUByteBSVl:	dc.w	AddUByteBSVl-*
OffsUByteBSVr:	dc.w	AddUByteBSVr-*
OffsUByteBSVP:	dc.w	AddUByteBSVP-*
OffsUByteBSVPH:	dc.w	AddUByteBSVPH-*
OffsUByteBSVTl:	dc.w	AddUByteBSVTl-*
OffsUByteBSVTr:	dc.w	AddUByteBSVTr-*
OffsUByteBSVPT:	dc.w	AddUByteBSVPT-*

OffsWordBM:	dc.w	AddWordBM-*
OffsWordBMV:	dc.w	AddWordBMV-*
OffsWordBMVH:	dc.w	AddWordBMVH-*
OffsWordBMVT:	dc.w	AddWordBMVT-*
OffsWordBSl:	dc.w	AddWordBSl-*
OffsWordBSr:	dc.w	AddWordBSr-*
OffsWordBSVl:	dc.w	AddWordBSVl-*
OffsWordBSVr:	dc.w	AddWordBSVr-*
OffsWordBSVP:	dc.w	AddWordBSVP-*
OffsWordBSVPH:	dc.w	AddWordBSVPH-*
OffsWordBSVTl:	dc.w	AddWordBSVTl-*
OffsWordBSVTr:	dc.w	AddWordBSVTr-*
OffsWordBSVPT:	dc.w	AddWordBSVPT-*
		dc.w	-1

	cnop	0,16
	ds.b	16
AlignStart:

;------------------------------------------------------------------------------
	cnop	0,16

AddSilence:
 IFGE	__CPU-68020
	move.l	d5,d1			;AddI<65535
	swap.w	d1
	move.w	d6,d1			;d1=Add<<16
	mulu.l	d0,d2:d1
	add.w	d0,d4			;New OffsetF (X)
	move.w	d1,d0
	move.w	d2,d1
	swap.w	d1			;d1=d2:d1>>16
	addx.l	d1,d3			;New OffsetI
 ELSE
	move.l	d5,d1
	mulu.w	d0,d1			;OffsI*BuffSamples
	add.l	d1,d3			;New OffsetI (1)
	move.l	d6,d2
	mulu.w	d0,d2			;OffsF*BuffSamples...
	add.w	d2,d4			;New OffsetF (X)
	clr.w	d2
	swap.w	d2			;...>>16
	addx.l	d2,d3			;New OffsetI (2)
 ENDC
	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
	beq.b	.nostereo
	lsl.l	#1,d0
.nostereo
	btst.b	#AHIACB_HIFI,ahiac_Flags+3(a2)
	beq.b	.nohifi
	lsl.l	#1,d0
.nohifi
	lsl.l	#1,d0
	add.l	d0,a4			;New buffer pointer
	rts


;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.b	8 - ²log(channels)
AddByteM:
	lsr.w	#1,d0
	bcs.b	.1
	subq.l	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.w	-256..256
AddByteMV:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
AddByteMVT:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.b	8 - ²log(channels)
AddByteSl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4			;skip right channel
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4			;skip right channel
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.b	8 - ²log(channels)
AddByteSr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+		;skip and add
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+		;skip and add
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.w	-256..256
AddByteSVl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.w	-256..256
AddByteSVr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+		;skip and add
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+		;skip and add
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.w	-256..256
* d2.w	-256..256
AddByteSVP:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
AddByteSVTl:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts
;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.l	Pointer in multiplication table
AddByteSVTr:
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddByteSVPT:
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.b	8 - ²log(65536/volume) (2^x, 8<=x<=16)
AddUByteM:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.w	-256..256
AddUByteMV:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
AddUByteMVT:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.b	8 - ²log(65536/volume) (2^x, 8<=x<=16)
AddUByteSl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4			;skip right channel
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4			;skip right channel
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.b	8 - ²log(65536/volume) (2^x, 8<=x<=16)
AddUByteSr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.w	-256..256
AddUByteSVl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.w	-256..256
AddUByteSVr:
	move.l	#$ffff,d1
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.w	-256..256
* d2.w	-256..256
AddUByteSVP:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
AddUByteSVTl:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.l	Pointer in multiplication table
AddUByteSVTr:
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddUByteSVPT:
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*2),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*2),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.b	²log(65536/volume) (2^x, 8<=x<=16)
AddWordM:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts


;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	-32768..32767/-256..256
AddWordMV:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
AddWordMVT:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.b	²log(65536/volume) (2^x, 8<=x<=16)
AddWordSl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4			;skip right channel
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4			;skip right channel
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.b	²log(65536/volume) (2^x, 8<=x<=16)
AddWordSr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d1
	add.l	d1,d1
	move.w	(a3,d1.l),d7
 ENDC
	asr.w	d2,d7
	add.l	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d1
	add.l	d1,d1
	move.w	(a3,d1.l),d7
 ENDC
	asr.w	d2,d7
	add.l	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	-32768..32767/-256..256
AddWordSVl:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.l	-32768..32767/-256..256
AddWordSVr:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d1

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	lsr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	add.w	d6,d4
	addx.l	d5,d3
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	lsr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	add.w	d6,d4
	addx.l	d5,d3
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	-32768..32767/-256..256
* d2.l	-32768..32767/-256..256
AddWordSVP:
* 16/8 bit signed input (8 for '000 version)
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
AddWordSVTl:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d2.l	Pointer in multiplication table
AddWordSVTr:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	addq.l	#2,a4
	add.w	d2,(a4)+
	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	Pointer in multiplication table
* d2.l	Pointer in multiplication table
AddWordSVPT:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	push	a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+

	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a1,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a1,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+

	move.w	0(a1,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+

	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a1,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a1,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+

	move.w	0(a1,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+
 ENDC
	add.w	d6,d4
	addx.l	d5,d3

	dbf	d0,.nextsample
.exit
	pop	a1
	rts

*******************************************************************************
; HIFI routines (unoptimized, only 020+!)
;------------------------------------------------------------------------------
	cnop	0,16

;in:
* d1.l	-65536..65536
AddByteMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in:
* d1.l	-65536..65536
* d2.l	-65536..65536
AddByteSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in:
* d1.l	-65536..65536
* d2.l	-65536..65536
AddUByteMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in:
* d1.l	-65536..65536
* d2.l	-65536..65536
AddUByteSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	-1(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in:
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.w	-2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

;in:
* d1.l	-65536..65536
* d2.l	-65536..65536
AddWordSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.w	-2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	muls.l	d4,d7
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	add.w	d6,d4
	addx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

*******************************************************************************

;------------------------------------------------------------------------------
	cnop	0,16

AddSilenceB:
 IFGE	__CPU-68020
	move.l	d5,d1			;AddI<65535
	swap.w	d1
	move.w	d6,d1			;d1=Add<<16
	mulu.l	d0,d2:d1
	sub.w	d0,d4			;New OffsetF (X)
	move.w	d1,d0
	move.w	d2,d1
	swap.w	d1			;d1=d2:d1>>16
	subx.l	d1,d3			;New OffsetI
 ELSE
	move.l	d5,d1
	mulu.w	d0,d1			;OffsI*BuffSamples
	add.l	d1,d3			;New OffsetI (1)
	move.l	d6,d2
	mulu.w	d0,d2			;OffsF*BuffSamples...
	sub.w	d2,d4			;New OffsetF (X)
	clr.w	d2
	swap.w	d2			;...>>16
	subx.l	d2,d3			;New OffsetI (2)
 ENDC
	btst.b	#AHIACB_STEREO,ahiac_Flags+3(a2)
	beq.b	.nostereo
	lsl.l	#1,d0
.nostereo
	btst.b	#AHIACB_HIFI,ahiac_Flags+3(a2)
	beq.b	.nohifi
	lsl.l	#1,d0
.nohifi
	lsl.l	#1,d0
	add.l	d0,a4			;New buffer pointer
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBM:
	lsr.w	#1,d0
	bcs.b	.1
	subq.l	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBMV:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBMVT:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4			;skip right channel
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4			;skip right channel
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+		;skip and add
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+		;skip and add
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSVl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSVr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+		;skip and add
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+		;skip and add
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSVP:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSVTl:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts
;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSVTr:
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2		;signed multiplication
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddByteBSVPT:
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*4),d2
 ELSE
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*4),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBM:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBMV:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBMVT:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4			;skip right channel
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4			;skip right channel
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	lsl.w	d2,d7
	add.l	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSVl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSVr:
	move.l	#$ffff,d1
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	addq.l	#2,a4
	sub.b	#$80,d7
	ext.w	d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSVP:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d7
	sub.b	#$80,d7
	ext.w	d7
	move.w	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+
	move.w	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSVTl:
	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSVTr:
	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2		;unsigned multiplication -> signed result
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;unsigned multiplication -> signed result
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddUByteBSVPT:
	move.l	a1,d7			;save a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*2),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
	move.b	(a3,d3.l),d1
 IFGE	__CPU-68020
	move.w	0(a0,d1.w*2),d2
 ELSE
	add.w	d1,d1
	move.w	0(a0,d1.w),d2
 ENDC
	add.w	d2,(a4)+

 IFGE	__CPU-68020
	move.w	0(a1,d1.w*2),d2
 ELSE
	move.w	0(a1,d1.w),d2
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	move.l	d7,a1			;restore a1
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBM:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts


;------------------------------------------------------------------------------
	cnop	0,16

AddWordBMV:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBMVT:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSl:
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4			;skip right channel
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.w	(a3,d7.l),d7
 ENDC
	asr.w	d1,d7
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4			;skip right channel
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSr:
	moveq	#0,d7
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d1
	add.l	d1,d1
	move.w	(a3,d1.l),d7
 ENDC
	asr.w	d2,d7
	add.l	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
 ELSE
	move.l	d3,d1
	add.l	d1,d1
	move.w	(a3,d1.l),d7
 ENDC
	asr.w	d2,d7
	add.l	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSVl:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d1,d7
	asr.l	d2,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	muls.w	d1,d7
 ENDC
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSVr:
* 16/8 bit signed input (8 for '000 version)
	moveq	#15,d1

	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	lsr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	sub.w	d6,d4
	subx.l	d5,d3
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	muls.w	d2,d7
	lsr.l	d1,d7
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	sub.w	d6,d4
	subx.l	d5,d3
	ext.w	d7
	muls.w	d2,d7
 ENDC
	addq.l	#2,a4
	add.w	d7,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSVP:
* 16/8 bit signed input (8 for '000 version)
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.w	(a3,d3.l*2),d7
	move.l	d7,a0
	muls.w	d1,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	lsr.l	#8,d7
	lsr.l	#7,d7
	add.w	d7,(a4)+
 ELSE
	move.l	d3,d7
	add.l	d7,d7
	move.b	0(a3,d7.l),d7		;high byte
	ext.w	d7
	move.l	d7,a0
	muls.w	d1,d7
	add.w	d7,(a4)+

	move.l	a0,d7
	muls.w	d2,d7
	add.w	d7,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSVTl:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	move.l	d1,a0
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	addq.l	#2,a4

	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSVTr:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	move.l	d2,a0
	moveq	#0,d1
	moveq	#0,d2
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	add.l	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	moveq	#0,d1
 ENDC
	addq.l	#2,a4
	add.w	d2,(a4)+
	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
	rts

;------------------------------------------------------------------------------
	cnop	0,16

AddWordBSVPT:

* ([h*v]<<8+[l*v])>>8 == [h*v]+[l*v]>>8. The latter is used.

	push	a1
	move.l	d1,a0
	move.l	d2,a1
	moveq	#0,d1
	lsr.w	#1,d0
	bcs.b	.1
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+

	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a1,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a1,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+

	move.w	0(a1,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3
.1
 IFGE	__CPU-68020
	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a0,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a0,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+

	move.b	0(a3,d3.l*2),d1		;high byte
	move.w	0(a1,d1.w*4),d2		;signed multiplication
	move.b	1(a3,d3.l*2),d1		;low byte
	move.b	2(a1,d1.w*4),d1		;unsigned multiplication / 256
	add.w	d1,d2
	add.w	d2,(a4)+
 ELSE
	move.l	d3,d7
 	add.l	d7,d7
	move.b	0(a3,d7.l),d1		;high byte
	add.w	d1,d1
	add.w	d1,d1
	move.w	0(a0,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+

	move.w	0(a1,d1.w),d2		;signed multiplication
	add.w	d2,(a4)+
 ENDC
	sub.w	d6,d4
	subx.l	d5,d3

	dbf	d0,.nextsample
.exit
	pop	a1
	rts


*******************************************************************************
; HIFI routines (unoptimized, only 020+!)
;------------------------------------------------------------------------------
	cnop	0,16

AddByteBMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddByteBSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddUByteBMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddUByteBSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.b	1(a3,d3.l),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.b	0(a3,d3.l*2),d7
	sub.b	#$80,d7
	lsl.w	#8,d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddWordBMVH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.w	2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	muls.l	d1,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts

AddWordBSVPH:
 IFGE	__CPU-68020
	subq.w	#1,d0
	bmi.b	.exit
.nextsample
	cmp.l	cd_FirstOffsetI(a5),d3	;Is this the first sample?
	bne	.not_first
	move.l	cd_LastSample(a5),d7
	bra	.got_sample
.not_first
	move.w	2(a3,d3.l*2),d7
	ext.l	d7
.got_sample
	muls.l	d4,d7
	move.l	d7,a0
	move.w	0(a3,d3.l*2),d7
	ext.l	d7
	move.l	d7,cd_TempLastSample(a5)
	neg.w	d4			;(65536-offset fraction)
	beq.b	.null
	muls.l	d4,d7
	neg.w	d4
	add.l	a0,d7
	asr.l	#8,d7
	asr.l	#8,d7
.null
	move.l	d7,a0
	muls.l	d1,d7
	add.l	d7,(a4)+

	move.l	a0,d7
	muls.l	d2,d7
	add.l	d7,(a4)+

	sub.w	d6,d4
	subx.l	d5,d3
	dbf	d0,.nextsample
.exit
 ENDC
	rts


;------------------------------------------------------------------------------
AlignEnd:

logtable:
	blk.b	258,0


 IFNE	DEBUG
sample:
;	dc.b	$5,$10,$5,0,-$5,-$10,-$5,0
	dc.b	$AA,$AA,$AA,$AA,$AA,$AA,$AA,$AA
sample_len=*-sample

	blk.w	SAMPLES,$5555
buffer:	blk.b	BUFFER,'*'
end:
 ENDC
