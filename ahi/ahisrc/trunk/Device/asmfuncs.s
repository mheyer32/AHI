; $Id$

;    AHI - Hardware independent audio subsystem
;    Copyright (C) 1996-2001 Martin Blom <martin@blom.org>
;     
;    This library is free software; you can redistribute it and/or
;    modify it under the terms of the GNU Library General Public
;    License as published by the Free Software Foundation; either
;    version 2 of the License, or (at your option) any later version.
;     
;    This library is distributed in the hope that it will be useful,
;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;    Library General Public License for more details.
;     
;    You should have received a copy of the GNU Library General Public
;    License along with this library; if not, write to the
;    Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
;    MA 02139, USA.

	include	lvo/exec_lib.i

	section	.text,code

** Debug functions ************************************************************


	XDEF	_KPrintF
	XDEF	_kprintf
	XDEF	_VKPrintF
	XDEF	_vkprintf
	XDEF	_KPutFmt
	XDEF	KPrintF
	XDEF	kprintf
	XDEF	KPutFmt
	XDEF	kprint_macro

kprint_macro:
	movem.l	d0-d1/a0-a1,-(sp)
	bsr	KPrintF
	movem.l	(sp)+,d0-d1/a0-a1
	rts


_KPrintF:
_kprintf:
	move.l	1*4(sp),a0
	lea	2*4(sp),a1
	bra	KPrintF

_VKPrintF:
_vkprintf:
_KPutFmt:
	move.l	1*4(sp),a0
	move.l	2*4(sp),a1

KPrintF:
kprintf:
KPutFmt:
	movem.l	a2-a3/a6,-(sp)
	move.l	4.w,a6
	lea.l	RawPutChar(pc),a2
	move.l	a6,a3
	jsr	_LVORawDoFmt(a6)
	movem.l	(sp)+,a2-a3/a6
	rts

RawPutChar:
	move.l	a3,a6
	jsr	-516(a6)	;_LVORawPutChar
	rts
