/*
 * $Id$
 * $Log$
 * Revision 1.1  1997/06/29 03:04:02  lcs
 * Initial revision
 *
 */

#ifndef TOCCATAEMUL_H
#define TOCCATAEMUL_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/ports.h>
#include <libraries/toccata.h>

/* Force 32 bit results */

#define BOOL LONG


/* Arguments in registers */

#define ASM     __asm
#define REG(x)  register __ ## x


/* Definitions */

#define PLAYERFREQ 50

/* Structures */

struct toccataprefs {
  UBYTE ID[8];

  LONG  MixAux1Left;
  LONG  MixAux1Right;
  LONG  MixAux2Left;
  LONG  MixAux2Right;
  LONG  InputVolumeLeft;
  LONG  InputVolumeRight;
  LONG  OutputVolumeLeft;
  LONG  OutputVolumeRight;
  LONG  LoopbackVolume;
  ULONG Mode;
  ULONG Frequency;
  ULONG Input;
  ULONG MicGain;
  LONG  CaptureIoPri;
  LONG  CaptureBufferPri;
  ULONG CaptureBlockSize;
  ULONG MaxCaptureBlocks;
  LONG  PlaybackIoPri;
  LONG  PlaybackBufferPri;
  ULONG PlaybackBlockSize;
  ULONG PlaybackStartBlocks;
  ULONG PlaybackBlocks;

  ULONG MonoMode;
  ULONG StereoMode;
  ULONG LineInput;
  ULONG Aux1Input;
  ULONG MicInput;
  ULONG MicGainInput;
  ULONG MixInput;
};


struct slavemessage {
	struct Message Msg;
  ULONG          ID;   
  APTR           Data;
};


/* ID codes */

#define MSG_MODE      1
#define MSG_HWPROP    2
#define MSG_RAWPLAY   3
#define MSG_PLAY      4
#define MSG_RECORD    5
#define MSG_STOP      6
#define MSG_PAUSE     7
#define MSG_LEVELON   8
#define MSG_LEVELOFF  9


/* Externals */

extern char __far _LibID[];
extern char __far _LibName[];

extern struct ToccataBase *ToccataBase;
extern struct Process *SlaveProcess;
extern BOOL SlaveInitialized;
extern struct AHIAudioCtrl *audioctrl;
extern struct toccataprefs tprefs;
extern const Fixed negboundaries[];
extern const Fixed posboundaries[];

extern ULONG error;

void kprintf(char *, ...);

ASM void SlaveTaskEntry(void);
ASM void HookLoad(void);
ASM ULONG GetRawReply(REG(a0) struct ToccataBase *);

void fillhardinfo(void);

#endif /* TOCCATAEMUL_H */
