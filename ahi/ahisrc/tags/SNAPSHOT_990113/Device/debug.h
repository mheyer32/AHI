/* $Id$ */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <devices/ahi.h>

void
Debug_AllocAudioA( struct TagItem *tags );

void
Debug_FreeAudio( struct AHIPrivAudioCtrl *audioctrl );

void
Debug_KillAudio( void );

void
Debug_ControlAudioA( struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags );


void
Debug_SetVol( UWORD chan, Fixed vol, sposition pan, struct AHIPrivAudioCtrl *audioctrl, ULONG flags);

void
Debug_SetFreq( UWORD chan, ULONG freq, struct AHIPrivAudioCtrl *audioctrl, ULONG flags);

void
Debug_SetSound( UWORD chan, UWORD sound, ULONG offset, LONG length, struct AHIPrivAudioCtrl *audioctrl, ULONG flags);

void
Debug_SetEffect( ULONG *effect, struct AHIPrivAudioCtrl *audioctrl );

void
Debug_LoadSound( UWORD sound, ULONG type, APTR info, struct AHIPrivAudioCtrl *audioctrl );

void
Debug_UnloadSound( UWORD sound, struct AHIPrivAudioCtrl *audioctrl );

void
Debug_NextAudioID( ULONG id);

void
Debug_GetAudioAttrsA( ULONG id, struct AHIAudioCtrlDrv *audioctrl, struct TagItem *tags );

void
Debug_BestAudioIDA( struct TagItem *tags );

void
Debug_AllocAudioRequestA( struct TagItem *tags );

void
Debug_AudioRequestA( struct AHIAudioModeRequester *req, struct TagItem *tags );

void
Debug_FreeAudioRequest( struct AHIAudioModeRequester *req );

void
Debug_PlayA( struct AHIAudioCtrl *audioctrl, struct TagItem *tags );

void
Debug_SampleFrameSize( ULONG sampletype);

void
Debug_AddAudioMode(struct TagItem *tags );

void
Debug_RemoveAudioMode( ULONG id);

void
Debug_LoadModeFile( STRPTR name);

#endif /* _DEBUG_H_ */
