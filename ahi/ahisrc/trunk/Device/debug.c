/* $Id$ */

/*
     AHI - Hardware independent audio subsystem
     Copyright (C) 1997-1999 Martin Blom <martin@blom.org>
     
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Library General Public
     License as published by the Free Software Foundation; either
     version 2 of the License, or (at your option) any later version.
     
     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Library General Public License for more details.
     
     You should have received a copy of the GNU Library General Public
     License along with this library; if not, write to the
     Free Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge,
     MA 02139, USA.
*/

#include <config.h>
#include <CompilerSpecific.h>

#include <utility/tagitem.h>
#include <proto/utility.h>

#include "ahi_def.h"
#include "debug.h"


/******************************************************************************
** Support code ***************************************************************
******************************************************************************/

static const char*
GetTagName( Tag tag )
{
  switch( tag )
  {
    case AHIA_AudioID: return "AHIA_AudioID";
    case AHIA_MixFreq: return "AHIA_MixFreq";
    case AHIA_Channels: return "AHIA_Channels";
    case AHIA_Sounds: return "AHIA_Sounds";
    case AHIA_SoundFunc: return "AHIA_SoundFunc";
    case AHIA_PlayerFunc: return "AHIA_PlayerFunc";
    case AHIA_PlayerFreq: return "AHIA_PlayerFreq";
    case AHIA_MinPlayerFreq: return "AHIA_MinPlayerFreq";
    case AHIA_MaxPlayerFreq: return "AHIA_MaxPlayerFreq";
    case AHIA_PlayerFreqUnit: return "AHIA_PlayerFreqUnit";
    case AHIA_RecordFunc: return "AHIA_RecordFunc";
    case AHIA_UserData: return "AHIA_UserData";
    case AHIA_ErrorFunc: return "AHIA_ErrorFunc";
    case AHIA_AntiClickSamples: return "AHIA_AntiClickSamples";
    case AHIP_BeginChannel: return "AHIP_BeginChannel";
    case AHIP_EndChannel: return "AHIP_EndChannel";
    case AHIP_Freq: return "AHIP_Freq";
    case AHIP_Vol: return "AHIP_Vol";
    case AHIP_Pan: return "AHIP_Pan";
    case AHIP_Sound: return "AHIP_Sound";
    case AHIP_Offset: return "AHIP_Offset";
    case AHIP_Length: return "AHIP_Length";
    case AHIP_LoopFreq: return "AHIP_LoopFreq";
    case AHIP_LoopVol: return "AHIP_LoopVol";
    case AHIP_LoopPan: return "AHIP_LoopPan";
    case AHIP_LoopSound: return "AHIP_LoopSound";
    case AHIP_LoopOffset: return "AHIP_LoopOffset";
    case AHIP_LoopLength: return "AHIP_LoopLength";
    case AHIC_Play: return "AHIC_Play";
    case AHIC_Record: return "AHIC_Record";
    case AHIC_PausePlay: return "AHIC_PausePlay";
    case AHIC_PauseRecord: return "AHIC_PauseRecord";
    case AHIC_MixFreq_Query: return "AHIC_MixFreq_Query";
    case AHIC_Input: return "AHIC_Input";
    case AHIC_Input_Query: return "AHIC_Input_Query";
    case AHIC_Output: return "AHIC_Output";
    case AHIC_Output_Query: return "AHIC_Output_Query";
    case AHIC_MonitorVolumeLeft: return "AHIC_MonitorVolumeLeft";
    case AHIC_MonitorVolumeLeft_Query: return "AHIC_MonitorVolumeLeft_Query";
    case AHIC_MonitorVolumeRight: return "AHIC_MonitorVolumeRight";
    case AHIC_MonitorVolumeRight_Query: return "AHIC_MonitorVolumeRight_Query";
    case AHIC_OutputVolumeLeft: return "AHIC_OutputVolumeLeft";
    case AHIC_OutputVolumeLeft_Query: return "AHIC_OutputVolumeLeft_Query";
    case AHIC_OutputVolumeRight: return "AHIC_OutputVolumeRight";
    case AHIC_OutputVolumeRight_Query: return "AHIC_OutputVolumeRight_Query";
    case AHIC_InputGainLeft: return "AHIC_InputGainLeft";
    case AHIC_InputGainLeft_Query: return "AHIC_InputGainLeft_Query";
    case AHIC_InputGainRight: return "AHIC_InputGainRight";
    case AHIC_InputGainRight_Query: return "AHIC_InputGainRight_Query";
    case AHIC_PlaySampleFormat: return "AHIC_PlaySampleFormat";
    case AHIC_PlaySampleFormat_Query: return "AHIC_PlaySampleFormat_Query";
    case AHIC_RecordSampleFormat: return "AHIC_RecordSampleFormat";
    case AHIC_RecordSampleFormat_Query: return "AHIC_RecordSampleFormat_Query";
    case AHIDB_AudioID: return "AHIDB_AudioID";
    case AHIDB_BufferLen: return "AHIDB_BufferLen";
    case AHIDB_Driver: return "AHIDB_Driver";
    case AHIDB_Author: return "AHIDB_Author";
    case AHIDB_Copyright: return "AHIDB_Copyright";
    case AHIDB_Version: return "AHIDB_Version";
    case AHIDB_Annotation: return "AHIDB_Annotation";
    case AHIDB_Name: return "AHIDB_Name";
    case AHIDB_Data: return "AHIDB_Data";
    case AHIDB_Flags: return "AHIDB_Flags";
    case AHIDB_Play: return "AHIDB_Play";
    case AHIDB_Record: return "AHIDB_Record";
    case AHIDB_Direct: return "AHIDB_Direct";
    case AHIDB_Volume: return "AHIDB_Volume";
    case AHIDB_Stereo: return "AHIDB_Stereo";
    case AHIDB_Panning: return "AHIDB_Panning";
    case AHIDB_Surround: return "AHIDB_Surround";
    case AHIDB_PingPong: return "AHIDB_PingPong";
    case AHIDB_MultTable: return "AHIDB_MultTable";
    case AHIDB_MaxChannels: return "AHIDB_MaxChannels";
    case AHIDB_MaxPlaySamples: return "AHIDB_MaxPlaySamples";
    case AHIDB_MaxRecordSamples: return "AHIDB_MaxRecordSamples";
    case AHIDB_Bits: return "AHIDB_Bits";
    case AHIDB_HiFi: return "AHIDB_HiFi";
    case AHIDB_Realtime: return "AHIDB_Realtime";
    case AHIDB_FullDuplex: return "AHIDB_FullDuplex";
    case AHIDB_Accelerated: return "AHIDB_Accelerated";
    case AHIDB_Available: return "AHIDB_Available";
    case AHIDB_Hidden: return "AHIDB_Hidden";
    case AHIDB_Frequencies: return "AHIDB_Frequencies";
    case AHIDB_FrequencyArg: return "AHIDB_FrequencyArg";
    case AHIDB_Frequency: return "AHIDB_Frequency";
    case AHIDB_FrequencyArray: return "AHIDB_FrequencyArray";
    case AHIDB_IndexArg: return "AHIDB_IndexArg";
    case AHIDB_Index: return "AHIDB_Index";
    case AHIDB_Inputs: return "AHIDB_Inputs";
    case AHIDB_InputArg: return "AHIDB_InputArg";
    case AHIDB_Input: return "AHIDB_Input";
    case AHIDB_InputArray: return "AHIDB_InputArray";
    case AHIDB_Outputs: return "AHIDB_Outputs";
    case AHIDB_OutputArg: return "AHIDB_OutputArg";
    case AHIDB_Output: return "AHIDB_Output";
    case AHIDB_OutputArray: return "AHIDB_OutputArray";
    case AHIDB_MonitorVolumesLeft: return "AHIDB_MonitorVolumesLeft";
    case AHIDB_MonitorVolumeLeftArg: return "AHIDB_MonitorVolumeLeftArg";
    case AHIDB_MonitorVolumeLeft: return "AHIDB_MonitorVolumeLeft";
    case AHIDB_MonitorVolumeLeftArray: return "AHIDB_MonitorVolumeLeftArray";
    case AHIDB_MonitorVolumesRight: return "AHIDB_MonitorVolumesRight";
    case AHIDB_MonitorVolumeRightArg: return "AHIDB_MonitorVolumeRightArg";
    case AHIDB_MonitorVolumeRight: return "AHIDB_MonitorVolumeRight";
    case AHIDB_MonitorVolumeRightArray: return "AHIDB_MonitorVolumeRightArray";
    case AHIDB_OutputVolumesLeft: return "AHIDB_OutputVolumesLeft";
    case AHIDB_OutputVolumeLeftArg: return "AHIDB_OutputVolumeLeftArg";
    case AHIDB_OutputVolumeLeft: return "AHIDB_OutputVolumeLeft";
    case AHIDB_OutputVolumeLeftArray: return "AHIDB_OutputVolumeLeftArray";
    case AHIDB_OutputVolumesRight: return "AHIDB_OutputVolumesRight";
    case AHIDB_OutputVolumeRightArg: return "AHIDB_OutputVolumeRightArg";
    case AHIDB_OutputVolumeRight: return "AHIDB_OutputVolumeRight";
    case AHIDB_OutputVolumeRightArray: return "AHIDB_OutputVolumeRightArray";
    case AHIDB_InputGainsLeft: return "AHIDB_InputGainsLeft";
    case AHIDB_InputGainLeftArg: return "AHIDB_InputGainLeftArg";
    case AHIDB_InputGainLeft: return "AHIDB_InputGainLeft";
    case AHIDB_InputGainLeftArray: return "AHIDB_InputGainLeftArray";
    case AHIDB_InputGainsRight: return "AHIDB_InputGainsRight";
    case AHIDB_InputGainRightArg: return "AHIDB_InputGainRightArg";
    case AHIDB_InputGainRight: return "AHIDB_InputGainRight";
    case AHIDB_InputGainRightArray: return "AHIDB_InputGainRightArray";
    case AHIDB_PlaySampleFormats: return "AHIDB_PlaySampleFormats";
    case AHIDB_PlaySampleFormatArg: return "AHIDB_PlaySampleFormatArg";
    case AHIDB_PlaySampleFormat: return "AHIDB_PlaySampleFormat";
    case AHIDB_PlaySampleFormatArray: return "AHIDB_PlaySampleFormatArray";
    case AHIDB_RecordSampleFormats: return "AHIDB_RecordSampleFormats";
    case AHIDB_RecordSampleFormatArg: return "AHIDB_RecordSampleFormatArg";
    case AHIDB_RecordSampleFormat: return "AHIDB_RecordSampleFormat";
    case AHIDB_RecordSampleFormatArray: return "AHIDB_RecordSampleFormatArray";
    case AHIB_Dizzy: return "AHIB_Dizzy";
    case AHIR_Window: return "AHIR_Window";
    case AHIR_Screen: return "AHIR_Screen";
    case AHIR_PubScreenName: return "AHIR_PubScreenName";
    case AHIR_PrivateIDCMP: return "AHIR_PrivateIDCMP";
    case AHIR_IntuiMsgFunc: return "AHIR_IntuiMsgFunc";
    case AHIR_SleepWindow: return "AHIR_SleepWindow";
    case AHIR_UserData: return "AHIR_UserData";
    case AHIR_TextAttr: return "AHIR_TextAttr";
    case AHIR_Locale: return "AHIR_Locale";
    case AHIR_TitleText: return "AHIR_TitleText";
    case AHIR_PositiveText: return "AHIR_PositiveText";
    case AHIR_NegativeText: return "AHIR_NegativeText";
    case AHIR_InitialLeftEdge: return "AHIR_InitialLeftEdge";
    case AHIR_InitialTopEdge: return "AHIR_InitialTopEdge";
    case AHIR_InitialWidth: return "AHIR_InitialWidth";
    case AHIR_InitialHeight: return "AHIR_InitialHeight";
    case AHIR_InitialAudioID: return "AHIR_InitialAudioID";
    case AHIR_InitialMixFreq: return "AHIR_InitialMixFreq";
    case AHIR_InitialInfoOpened: return "AHIR_InitialInfoOpened";
    case AHIR_InitialInfoLeftEdge: return "AHIR_InitialInfoLeftEdge";
    case AHIR_InitialInfoTopEdge: return "AHIR_InitialInfoTopEdge";
    case AHIR_InitialInfoWidth: return "AHIR_InitialInfoWidth";
    case AHIR_InitialInfoHeight: return "AHIR_InitialInfoHeight";
    case AHIR_DoMixFreq: return "AHIR_DoMixFreq";
    case AHIR_DoDefaultMode: return "AHIR_DoDefaultMode";
    case AHIR_DoChannels: return "AHIR_DoChannels";
    case AHIR_DoHidden: return "AHIR_DoHidden";
    case AHIR_DoDirectModes: return "AHIR_DoDirectModes";
    case AHIR_FilterTags: return "AHIR_FilterTags";
    case AHIR_FilterFunc: return "AHIR_FilterFunc";
    case AHIC_MonitorVolume: return "AHIC_MonitorVolume";
    case AHIC_MonitorVolume_Query: return "AHIC_MonitorVolume_Query";
    case AHIC_InputGain: return "AHIC_InputGain";
    case AHIC_InputGain_Query: return "AHIC_InputGain_Query";
    case AHIC_OutputVolume: return "AHIC_OutputVolume";
    case AHIC_OutputVolume_Query: return "AHIC_OutputVolume_Query";
    case AHIDB_MinMixFreq: return "AHIDB_MinMixFreq";
    case AHIDB_MaxMixFreq: return "AHIDB_MaxMixFreq";
    case AHIDB_MinMonitorVolume: return "AHIDB_MinMonitorVolume";
    case AHIDB_MaxMonitorVolume: return "AHIDB_MaxMonitorVolume";
    case AHIDB_MinInputGain: return "AHIDB_MinInputGain";
    case AHIDB_MaxInputGain: return "AHIDB_MaxInputGain";
    case AHIDB_MinOutputVolume: return "AHIDB_MinOutputVolume";
    case AHIDB_MaxOutputVolume: return "AHIDB_MaxOutputVolume";

    default:
      return "Unknown";
  }
}


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
      KPrintF("\n  %30s, 0x%08lx,", GetTagName( tag->ti_Tag ), tag->ti_Data);
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
