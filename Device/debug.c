/* $Id$ */

#include <config.h>
#include <CompilerSpecific.h>

#include <utility/tagitem.h>
#include <proto/utility.h>

#include "ahi_def.h"
#include "debug.h"


/******************************************************************************
** Support code ***************************************************************
******************************************************************************/

// tags may be NULL

static void
PrintTagList(struct TagItem *tags)
{
  struct TagItem *tstate;
  struct TagItem *tag;

  if(tags == NULL)
  {
    KPrintF("No taglist\n");
  }
  else
  {
    tstate = tags;
    while((tag = NextTagItem(&tstate)))
    {
      KPrintF("\n  0x%08lx, 0x%08lx,", tag->ti_Tag, tag->ti_Data);
    }
    KPrintF("\n  TAG_DONE)");
  }
}


/******************************************************************************
** All functions **************************************************************
******************************************************************************/


void
Debug_AllocAudioA( struct TagItem *tags )
{
  KPrintF("AHI_AllocAudioA(");
  PrintTagList(tags);
}

void
Debug_FreeAudio( struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_FreeAudio(0x%08lx)\n",audioctrl);
}

void
Debug_KillAudio( void )
{
  KPrintF("AHI_KillAudio()\n");
}

void
Debug_ControlAudioA( struct AHIPrivAudioCtrl *audioctrl, struct TagItem *tags )
{
  KPrintF("AHI_ControlAudioA(0x%08lx,",audioctrl);
  PrintTagList(tags);
}


void
Debug_SetVol( UWORD chan, Fixed vol, sposition pan, struct AHIPrivAudioCtrl *audioctrl, ULONG flags)
{
  KPrintF("AHI_SetVol(%ld, 0x%08lx, 0x%08lx, 0x%08lx, %ld)\n",
      chan & 0xffff, vol, pan, audioctrl, flags);
}

void
Debug_SetFreq( UWORD chan, ULONG freq, struct AHIPrivAudioCtrl *audioctrl, ULONG flags)
{
  KPrintF("AHI_SetFreq(%ld, %ld, 0x%08lx, %ld)\n",
      chan & 0xffff, freq, audioctrl, flags);
}

void
Debug_SetSound( UWORD chan, UWORD sound, ULONG offset, LONG length, struct AHIPrivAudioCtrl *audioctrl, ULONG flags)
{
  KPrintF("AHI_SetSound(%ld, %ld, 0x%08lx, 0x%08lx, 0x%08lx, %ld)\n",
      chan & 0xffff, sound & 0xffff, offset, length, audioctrl, flags);
}

void
Debug_SetEffect( ULONG *effect, struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_SetEffect(0x%08lx (Effect 0x%08lx), 0x%08lx)\n",
      effect, *effect, audioctrl);
}

void
Debug_LoadSound( UWORD sound, ULONG type, APTR info, struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_LoadSound(%ld, %ld, 0x%08lx, 0x%08lx) ", sound, type, info, audioctrl);

  if(type == AHIST_SAMPLE || type == AHIST_DYNAMICSAMPLE)
  {
    struct AHISampleInfo *si = (struct AHISampleInfo *) info;

    KPrintF("[T:0x%08lx A:0x%08lx L:%ld]", si->ahisi_Type, si->ahisi_Address, si->ahisi_Length);
  }
}

void
Debug_UnloadSound( UWORD sound, struct AHIPrivAudioCtrl *audioctrl )
{
  KPrintF("AHI_UnloadSound(%ld, 0x%08lx)\n", sound, audioctrl);
}

void
Debug_NextAudioID( ULONG id)
{
  KPrintF("AHI_NextAudioID(0x%08lx)",id);
}

void
Debug_GetAudioAttrsA( ULONG id, struct AHIAudioCtrlDrv *audioctrl, struct TagItem *tags )
{
  KPrintF("AHI_GetAudioAttrsA(0x%08lx, 0x%08lx,",id,audioctrl);
  PrintTagList(tags);
}

void
Debug_BestAudioIDA( struct TagItem *tags )
{
  KPrintF("AHI_BestAudioIDA(");
  PrintTagList(tags);
}

void
Debug_AllocAudioRequestA( struct TagItem *tags )
{
  KPrintF("AHI_AllocAudioRequestA(");
  PrintTagList(tags);
}

void
Debug_AudioRequestA( struct AHIAudioModeRequester *req, struct TagItem *tags )
{
  KPrintF("AHI_AudioRequestA(0x%08lx,",req);
  PrintTagList(tags);
}

void
Debug_FreeAudioRequest( struct AHIAudioModeRequester *req )
{
  KPrintF("AHI_FreeAudioRequest(0x%08lx)\n",req);
}

void
Debug_PlayA( struct AHIAudioCtrl *audioctrl, struct TagItem *tags )
{
  KPrintF("AHI_PlayA(0x%08lx,",audioctrl);
  PrintTagList(tags);
  KPrintF("\n");
}

void
Debug_SampleFrameSize( ULONG sampletype)
{
  KPrintF("AHI_SampleFrameSize(%ld)",sampletype);
}

void
Debug_AddAudioMode(struct TagItem *tags )
{
  KPrintF("AHI_AddAudioMode(");
  PrintTagList(tags);
}

void
Debug_RemoveAudioMode( ULONG id)
{
  KPrintF("AHI_RemoveAudioMode(0x%08lx)",id);
}

void
Debug_LoadModeFile( STRPTR name)
{
  KPrintF("AHI_LoadModeFile(%s)",name);
}


/******************************************************************************
** The Prayer *****************************************************************
******************************************************************************/

static const char prayer[] =
{
  "Oh Lord, most wonderful God, I pray for every one that uses this software; "
  "Let the Holy Ghost speak, call him or her to salvation, reveal your Love. "
  "Lord, bring the revival to this country, let the Holy Ghost fall over all "
  "flesh, as You have promised a long time ago. Thank you for everything, thank "
  "you for what is about to happen. Amen."
};
