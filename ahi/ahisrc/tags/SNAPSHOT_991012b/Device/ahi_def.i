; $Id$

;    AHI - Hardware independent audio subsystem
;    Copyright (C) 1996-1999 Martin Blom <martin@blom.org>
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

	IFND    _AHI_DEF_I_
_AHI_DEF_I_	SET	1

DEBUG_DETAIL	SET	2

	include exec/semaphores.i
	include	exec/devices.i
	include	devices/ahi.i
	include devices/timer.i
	include	libraries/ahi_sub.i
	include	macros.i


*** Processor identification ****

 IFGE	__CPU-68020

HAVE_HIFI	EQU	1
HAVE_CLIPPING	EQU	1

 ENDC


*** Definitions ***

	STRUCTURE Timer,0
	STRUCT	EntryTime,EV_SIZE
	STRUCT	ExitTime,EV_SIZE
	LABEL	Timer_SIZEOF

* Private AudioCtrl structure
	STRUCTURE AHIPrivAudioCtrl,AHIAudioCtrlDrv_SIZEOF
	APTR	ahiac_SubLib
	ULONG	ahiac_SubAllocRC
	APTR	ahiac_ChannelDatas
	APTR	ahiac_SoundDatas
	ULONG	ahiac_BuffSizeNow		* Now many bytes of the buffer are used?
	
	APTR	ahiac_MasterVolumeTable
	APTR	ahiac_MultTableS
	APTR	ahiac_MultTableU
	APTR	ahiac_RecordFunc		* AHIA_RecordFunc
	ULONG	ahiac_AudioID
	Fixed	ahiac_MasterVolume;		* Real
	Fixed	ahiac_SetMasterVolume;		* Set by user
	Fixed	ahiac_EchoMasterVolume;		* Set by dspecho
	APTR	ahiac_EffOutputBufferStruct	* struct AHIEffOutputBuffer *
	APTR	ahiac_EffDSPEchoStruct		* struct Echo *
	APTR	ahiac_EffChannelInfoStruct	* struct AHIChannelInfo *
	APTR	ahiac_WetList
	APTR	ahiac_DryList
	UBYTE	ahiac_WetOrDry
	UBYTE	ahiac_MaxCPU
	UWORD	ahiac_Channels2
	STRUCT	ahiac_Timer,Timer_SIZEOF
	UWORD	ahiac_UsedCPU
	UWORD	ahiac_PPCCommand;
	LONG	ahiac_PPCArgument;
	APTR	ahiac_PPCMixBuffer;
	APTR	ahiac_PPCMixInterrupt;
	APTR	ahiac_PPCWarpUpContext;
	APTR	ahiac_M68KPort;
	APTR	ahiac_PPCStartupMsg;
	APTR	ahiac_PPCTask;
	STRUCT	ahiac_DriverName,256
	LABEL	AHIPrivAudioCtrl_SIZEOF

	ENDC