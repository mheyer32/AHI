/* $Id$
* $Log$
* Revision 1.6  1997/02/01 19:44:18  lcs
* *** empty log message ***
*
* Revision 1.4  1997/01/15 18:35:07  lcs
* AHIB_Dizzy has a better implementation and definition now.
* (Was BOOL, now pointer to a second tag list)
*
* Revision 1.3  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
*
* Revision 1.2  1996/12/21 23:06:35  lcs
* Replaced all EQ with ==
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

#include "ahi_def.h"


#include <exec/memory.h>
#include <exec/alerts.h>
#include <utility/utility.h>
#include <utility/tagitem.h>

#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <strings.h>

#ifndef  noprotos
#include "database_protos.h"
#ifndef _GENPROTO
#include "cfuncs_protos.h"
#endif
#endif
// Makes 'in' fit the given bounds.

#define inbounds(in,min,max) \
    ( (in > max) ? max : ( (in < min) ? min : in ) )

extern __asm ULONG RecalcBuff( register __d1 ULONG , register __a2 struct AHIPrivAudioCtrl * );
extern __asm BOOL InitMixroutine(register __a2 struct AHIPrivAudioCtrl *);
extern __asm void Mix(void);

/******************************************************************************
** CreateAudioCtrl & UpdateAudioCtrl ******************************************
******************************************************************************/

extern struct Hook DefRecordHook;
extern struct Hook DefPlayerHook;

static struct TagItem boolmap[] =
{
  { AHIDB_Volume,   AHIACF_VOL },
  { AHIDB_Panning,  AHIACF_PAN },
  { AHIDB_Stereo,   AHIACF_STEREO },
  { AHIDB_HiFi,     AHIACF_HIFI },
  { AHIDB_PingPong, AHIACF_PINGPONG },
  { AHIDB_Record,   AHIACF_RECORD },
  { AHIDB_MultTable,AHIACF_MULTTAB },
  { TAG_DONE, }
};

#define DEFPLAYERFREQ (50)

__asm struct AHIPrivAudioCtrl *CreateAudioCtrl( register __a0 struct TagItem *tags)
{
  struct AHIPrivAudioCtrl *audioctrl;
  struct AHI_AudioDatabase *audiodb;
  struct TagItem *dbtags;
  BOOL   error=TRUE;

  if(audioctrl=AllocVec(sizeof(struct AHIPrivAudioCtrl),MEMF_PUBLIC|MEMF_CLEAR))
  {
    audioctrl->ac.ahiac_AudioCtrl.ahiac_UserData=(APTR)GetTagData(AHIA_UserData,NULL,tags);
    audioctrl->ac.ahiac_MixFreq=GetTagData(AHIA_MixFreq,AHI_DEFAULT_FREQ,tags);
    audioctrl->ac.ahiac_Channels=GetTagData(AHIA_Channels,0,tags);
    audioctrl->ac.ahiac_Sounds=GetTagData(AHIA_Sounds,0,tags);
    audioctrl->ac.ahiac_SoundFunc=(struct Hook *)GetTagData(AHIA_SoundFunc,NULL,tags);
    audioctrl->ahiac_RecordFunc=(struct Hook *)GetTagData(AHIA_RecordFunc,NULL,tags);
    audioctrl->ac.ahiac_PlayerFunc=(struct Hook *)GetTagData(AHIA_PlayerFunc,NULL,tags);
    audioctrl->ac.ahiac_PlayerFreq=GetTagData(AHIA_PlayerFreq,0,tags);
    audioctrl->ac.ahiac_MinPlayerFreq=GetTagData(AHIA_MinPlayerFreq,0,tags);
    audioctrl->ac.ahiac_MaxPlayerFreq=GetTagData(AHIA_MaxPlayerFreq,0,tags);
    audioctrl->ahiac_AudioID=GetTagData(AHIA_AudioID,AHI_DEFAULT_ID,tags);

    audioctrl->ahiac_MasterVolume=0x00010000;

    if(audioctrl->ahiac_AudioID == AHI_DEFAULT_ID)
      audioctrl->ahiac_AudioID=AHIBase->ahib_AudioMode;

    if(audioctrl->ac.ahiac_MixFreq == AHI_DEFAULT_FREQ)
      audioctrl->ac.ahiac_MixFreq=AHIBase->ahib_Frequency;

    if(audioctrl->ahiac_RecordFunc == NULL)
      audioctrl->ahiac_RecordFunc=&DefRecordHook;
    if(audioctrl->ac.ahiac_PlayerFunc == NULL)
      audioctrl->ac.ahiac_PlayerFunc=&DefPlayerHook;

    if(audioctrl->ac.ahiac_PlayerFreq == 0)
      audioctrl->ac.ahiac_PlayerFreq = DEFPLAYERFREQ;
    if(audioctrl->ac.ahiac_MinPlayerFreq == 0)
      audioctrl->ac.ahiac_MinPlayerFreq = DEFPLAYERFREQ;
    if(audioctrl->ac.ahiac_MaxPlayerFreq == 0)
      audioctrl->ac.ahiac_MaxPlayerFreq = DEFPLAYERFREQ;

    if(audiodb=LockDatabase())
    {
      if(dbtags=GetDBTagList(audiodb,audioctrl->ahiac_AudioID))
      {
        audioctrl->ac.ahiac_Flags=PackBoolTags(GetTagData(AHIDB_Flags,NULL,dbtags),dbtags,boolmap);
        stpcpy(stpcpy(stpcpy(audioctrl->ahiac_DriverName,"DEVS:ahi/"),(char *)GetTagData(AHIDB_Driver,(ULONG)"",dbtags)),".audio");
        error=FALSE;
      }
      UnlockDatabase(audiodb);
    }
  }

  if(error)
  {
    FreeVec(audioctrl);
    return NULL;
  }
  else
    return audioctrl;
}

__asm  void UpdateAudioCtrl( register __a0 struct AHIPrivAudioCtrl *audioctrl)
{
  ULONG  temp;

  temp=audioctrl->ac.ahiac_MinPlayerFreq;
  if(temp>=65536)
    temp >>=16;
  if(temp)
    audioctrl->ac.ahiac_MaxBuffSamples=audioctrl->ac.ahiac_MixFreq/temp;
  else
    audioctrl->ac.ahiac_MaxBuffSamples=AHIBase->ahib_Frequency/audioctrl->ac.ahiac_PlayerFreq;

  temp=audioctrl->ac.ahiac_MaxPlayerFreq;
  if(temp>=65536)
    temp = (temp + 65535) >> 16;
  if(temp)
    audioctrl->ac.ahiac_MinBuffSamples=audioctrl->ac.ahiac_MixFreq/temp;
  else
    audioctrl->ac.ahiac_MinBuffSamples=AHIBase->ahib_Frequency/audioctrl->ac.ahiac_PlayerFreq;
}

/******************************************************************************
** TestAudioID & DizzyTestAudioID *********************************************
******************************************************************************/

// tags may be NULL

BOOL TestAudioID(ULONG id, struct TagItem *tags )
{
  if(DizzyTestAudioID(id, tags) != 0x10000)
    return FALSE;
  else
    return TRUE;
}

// tags may be NULL

Fixed DizzyTestAudioID(ULONG id, struct TagItem *tags )
{
  LONG volume=0,stereo=0,panning=0,hifi=0,pingpong=0,record=0,realtime=0,
       fullduplex=0,bits=0,channels=0,minmix=0,maxmix=0;
  ULONG total=0,hits=0;
  struct TagItem *tag;
  
  if(tags == NULL)
  {
    return (Fixed) 0x10000;
  }

// Check source mode
  if(tag=FindTagItem(AHIDB_AudioID,tags))
  {
    total++;
    if( ((tag->ti_Data)&0xffff0000) == (id & 0xffff0000) )
      hits++;
  }

  AHI_GetAudioAttrs(id, NULL,
                        AHIDB_Volume, &volume,
                        AHIDB_Stereo, &stereo,
                        AHIDB_Panning, &panning,
                        AHIDB_HiFi,&hifi,
                        AHIDB_PingPong,&pingpong,
                        AHIDB_Record,&record,
                        AHIDB_Bits,&bits,
                        AHIDB_MaxChannels,&channels,
                        AHIDB_MinMixFreq,&minmix,
                        AHIDB_MaxMixFreq,&maxmix,
                        AHIDB_Realtime,&realtime,
                        AHIDB_FullDuplex,&fullduplex,
                        TAG_DONE);

// Boolean tags
  if(tag=FindTagItem(AHIDB_Volume,tags))
  {
    total++;
    if(tag->ti_Data == volume)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_Stereo,tags))
  {
    total++;
    if(tag->ti_Data == stereo)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_Panning,tags))
  {
    total++;
    if(tag->ti_Data == panning)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_HiFi,tags))
  {
    total++;
    if(tag->ti_Data == hifi)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_PingPong,tags))
  {
    total++;
    if(tag->ti_Data == pingpong)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_Record,tags))
  {
    total++;
    if(tag->ti_Data == record)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_Realtime,tags))
  {
    total++;
    if(tag->ti_Data == realtime)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_FullDuplex,tags))
  {
    total++;
    if(tag->ti_Data != fullduplex)
      hits++;
  }

// The rest
  if(tag=FindTagItem(AHIDB_Bits,tags))
  {
    total++;
    if(tag->ti_Data <= bits)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_MaxChannels,tags))
  {
    if(tag->ti_Data <= channels )
      hits++;
  }
  if(tag=FindTagItem(AHIDB_MinMixFreq,tags))
  {
    total++;
    if(tag->ti_Data >= minmix)
      hits++;
  }
  if(tag=FindTagItem(AHIDB_MaxMixFreq,tags))
  {
    total++;
    if(tag->ti_Data <= maxmix)
      hits++;
  }

  if(total)
    return (Fixed) ((hits<<16)/total);
  else
    return (Fixed) 0x10000;
}

/******************************************************************************
** AHI_AllocAudioA ************************************************************
******************************************************************************/

__asm struct AHIAudioCtrl *AllocAudioA( register __a1 struct TagItem *tags )
{
  struct AHIPrivAudioCtrl* audioctrl;
  struct Library *AHIsubBase;
  struct AHI_AudioDatabase *audiodb;
  struct TagItem *dbtags;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_AllocAudioA(0x%08lx)",tags);
  }

  audioctrl=CreateAudioCtrl(tags);
  if(!audioctrl)
    goto error;

  AHIBase->ahib_AudioCtrl=audioctrl;                      // Save latest (for KillAudio)

  if(!audioctrl->ac.ahiac_Channels || !audioctrl->ac.ahiac_Sounds)
    goto error;

  audioctrl->ac.ahiac_SamplerFunc=audioctrl->ahiac_RecordFunc;   // FIXIT!

  audioctrl->ahiac_SubLib=
  AHIsubBase=OpenLibrary(audioctrl->ahiac_DriverName,DriverVersion);
  if(!AHIsubBase)
    goto error;

  audiodb=LockDatabase();
  if(!audiodb)
    goto error;

  dbtags=GetDBTagList(audiodb,audioctrl->ahiac_AudioID);
  if(dbtags)
    audioctrl->ahiac_SubAllocRC=AHIsub_AllocAudio(dbtags,(struct AHIAudioCtrlDrv *)audioctrl);

  UnlockDatabase(audiodb);

  if(!dbtags)
    goto error;

  UpdateAudioCtrl(audioctrl);

  if(audioctrl->ahiac_SubAllocRC & AHISF_ERROR)
    goto error;

// Mixing
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_MIXING))
    audioctrl->ac.ahiac_Flags |= AHIACF_NOMIXING;

// Timing    
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_TIMING))
    audioctrl->ac.ahiac_Flags |= AHIACF_NOTIMING;

// Stereo
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_KNOWSTEREO))
    audioctrl->ac.ahiac_Flags &= ~AHIACF_STEREO;

// HiFi
  // If plain 68k, unconditionally clear the HIFI bit
#ifdef _M68020
  // If plain 68k, unconditionally clear the HIFI bit
  if(!(audioctrl->ahiac_SubAllocRC & AHISF_KNOWHIFI))
#endif
    audioctrl->ac.ahiac_Flags &= ~AHIACF_HIFI;

// Post-processing
  if(audioctrl->ahiac_SubAllocRC & AHISF_CANPOSTPROCESS)
    audioctrl->ac.ahiac_Flags |= AHIACF_POSTPROC;


  if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOMIXING))
  {
    switch(audioctrl->ac.ahiac_Flags & (AHIACF_STEREO | AHIACF_HIFI))
    {
      case NULL:
        audioctrl->ac.ahiac_BuffType=AHIST_M16S;
        break;
      case AHIACF_STEREO:
        audioctrl->ac.ahiac_BuffType=AHIST_S16S;
        break;
      case AHIACF_HIFI:
        audioctrl->ac.ahiac_BuffType=AHIST_M32S;
        break;
      case (AHIACF_STEREO | AHIACF_HIFI):
        audioctrl->ac.ahiac_BuffType=AHIST_S32S;
        break;
      default:
        Alert(AT_Recovery|AG_BadParm);
        goto error;
    }

/* Max channels/2 channels per hardware channel if stereo w/o pan */
    if((audioctrl->ac.ahiac_Flags & (AHIACF_STEREO | AHIACF_PAN)) == AHIACF_STEREO)
      audioctrl->ahiac_Channels2=(audioctrl->ac.ahiac_Channels+1)/2;
    else
      audioctrl->ahiac_Channels2=audioctrl->ac.ahiac_Channels;

    if(!(InitMixroutine(audioctrl)))
      goto error;

    if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOTIMING))
    {
      RecalcBuff(audioctrl->ac.ahiac_MinPlayerFreq,audioctrl);
      audioctrl->ac.ahiac_BuffSize=audioctrl->ahiac_BuffSizeNow;
      RecalcBuff(audioctrl->ac.ahiac_PlayerFreq,audioctrl);
    }
    else // No timing
    {
      ULONG size;

      size=audioctrl->ac.ahiac_BuffSamples*\
           AHI_SampleFrameSize(audioctrl->ac.ahiac_BuffType)*\
           (audioctrl->ac.ahiac_Flags & AHIACF_POSTPROC ? 2 : 1);

      size +=7;
      size &= ~7;  // byte align

      audioctrl->ahiac_BuffSizeNow=size;
    }

    audioctrl->ac.ahiac_MixerFunc=AllocVec(sizeof(struct Hook),MEMF_PUBLIC|MEMF_CLEAR);
    if(!audioctrl->ac.ahiac_MixerFunc)
      goto error;


    audioctrl->ac.ahiac_MixerFunc->h_Entry=(ULONG (*)())Mix;

/* FIXIT!
    audioctrl->ac.ahiac_SamplerFunc=AllocVec(sizeof(struct Hook),MEMF_PUBLIC!MEMF_CLEAR);
    if(!audioctrl->ac.ahiac_SamplerFunc)
      goto error;

    audioctrl->ac.ahiac_SamplerFunc->h_Entry=Sampler;
*/
  }

  /* Set default hardware properties */
  {
    LONG minmon,maxmon,mingain,maxgain,minvol,maxvol,inputs,outputs;

    if(AHI_GetAudioAttrs(AHI_INVALID_ID,(struct AHIAudioCtrl *)audioctrl,
        AHIDB_MinMonitorVolume, &minmon,
        AHIDB_MaxMonitorVolume, &maxmon,
        AHIDB_MinInputGain, &mingain,
        AHIDB_MaxInputGain, &maxgain,
        AHIDB_MinOutputVolume, &minvol,
        AHIDB_MaxOutputVolume, &maxvol,
        AHIDB_Inputs, &inputs,
        AHIDB_Outputs, &outputs,
        TAG_DONE))

    {
      AHIBase->ahib_MonitorVolume = inbounds(AHIBase->ahib_MonitorVolume, minmon, maxmon);
      AHIBase->ahib_InputGain     = inbounds(AHIBase->ahib_InputGain, minmon, maxmon);
      AHIBase->ahib_OutputVolume  = inbounds(AHIBase->ahib_OutputVolume, minmon, maxmon);
      AHIBase->ahib_Input         = inbounds(AHIBase->ahib_Input,0,inputs-1);
      AHIBase->ahib_Output        = inbounds(AHIBase->ahib_Output,0,outputs-1);

      AHI_ControlAudio((struct AHIAudioCtrl *)audioctrl,
          AHIC_MonitorVolume,   AHIBase->ahib_MonitorVolume,
          AHIC_InputGain,       AHIBase->ahib_InputGain,
          AHIC_OutputVolume,    AHIBase->ahib_OutputVolume,
          AHIC_Input,           AHIBase->ahib_Input,
          AHIC_Output,          AHIBase->ahib_Output,
          TAG_DONE);
    }
  }

exit:
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>0x%08lx\n",audioctrl);
  }
  return (struct AHIAudioCtrl *) audioctrl;


error:
  AHI_FreeAudio((struct AHIAudioCtrl *)audioctrl);
  audioctrl=NULL;
  goto exit;
}


/******************************************************************************
** AHI_FreeAudio **************************************************************
******************************************************************************/

__asm ULONG FreeAudio( register __a2 struct AHIPrivAudioCtrl *audioctrl )
{
  struct Library *AHIsubBase;
  int i;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_FreeAudio(0x%08lx)\n",audioctrl);
  }

  if(audioctrl)
  {
    if(AHIsubBase=audioctrl->ahiac_SubLib)
    {
      if(!(audioctrl->ahiac_SubAllocRC & AHISF_ERROR))
      {
        AHIsub_Stop(AHISF_PLAY|AHISF_RECORD,(struct AHIAudioCtrlDrv *)audioctrl);

        for(i=audioctrl->ac.ahiac_Sounds-1;i>=0;i--)
        {
          AHI_UnloadSound(i,(struct AHIAudioCtrl *)audioctrl);
        }
      }
      AHIsub_FreeAudio((struct AHIAudioCtrlDrv *) audioctrl);
      CloseLibrary(AHIsubBase);
    }

    FreeVec(audioctrl->ahiac_MultTableS);
    FreeVec(audioctrl->ahiac_MultTableU);
/* FIXIT! FreeVec(audioctrl->ahiac_SamplerFunc); */
    FreeVec(audioctrl->ac.ahiac_MixerFunc);
    FreeVec(audioctrl->ahiac_SoundDatas);
    FreeVec(audioctrl->ahiac_ChannelDatas);

    FreeVec(audioctrl);
  }
  return NULL;
}


/******************************************************************************
** AHI_KillAudio **************************************************************
******************************************************************************/

__asm ULONG KillAudio(void)
{
  UWORD i;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_KillAudio()\n");
  }

  for(i=0xffff;i != 0; i--)
  {
    *((UWORD *) 0xdff102)=i;
  }

  AHI_FreeAudio(AHIBase->ahib_AudioCtrl);
  AHIBase->ahib_AudioCtrl=NULL;
  return NULL;
}

/******************************************************************************
** AHI_ControlAudioA **********************************************************
******************************************************************************/

__asm ULONG ControlAudioA( register __a2 struct AHIPrivAudioCtrl *audioctrl,
    register __a1 struct TagItem *tags)
{
  ULONG *ptr, playflags=NULL, stopflags=NULL, rc=AHIE_OK;
  UBYTE update=FALSE;
  struct TagItem *tag,*tstate=tags;
  struct Library *AHIsubBase=audioctrl->ahiac_SubLib;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_ControlAudioA(0x%08lx, 0x%08lx)",audioctrl,tags);
  }

  while(tag=NextTagItem(&tstate))
  {
    ptr=(ULONG *)tag->ti_Data;  // For ..._Query tags
    switch(tag->ti_Tag)
    {
    case AHIA_SoundFunc:
      audioctrl->ac.ahiac_SoundFunc=(struct Hook *) tag->ti_Data;
      update=TRUE;
      break;
    case AHIA_RecordFunc:
      audioctrl->ahiac_RecordFunc=(struct Hook *) tag->ti_Data;
      audioctrl->ac.ahiac_SamplerFunc=(struct Hook *) tag->ti_Data;  // FIXIT!
      update=TRUE;
      break;
    case AHIA_PlayerFunc:
      audioctrl->ac.ahiac_PlayerFunc=(struct Hook *) tag->ti_Data;
      update=TRUE;
      break;
    case AHIA_PlayerFreq:
      audioctrl->ac.ahiac_PlayerFreq=tag->ti_Data;
      if(!(audioctrl->ac.ahiac_Flags & AHIACF_NOTIMING)) // Dont call unless timing is used.
        RecalcBuff(audioctrl->ac.ahiac_PlayerFreq,audioctrl);
      update=TRUE;
      break;
    case AHIA_UserData:
      audioctrl->ac.ahiac_AudioCtrl.ahiac_UserData=(void *)tag->ti_Data;
      break;
    case AHIC_Play:
      if(tag->ti_Data)
      {
        playflags |= AHISF_PLAY;
        stopflags &= ~AHISF_PLAY;
      }
      else
      {
        playflags &= ~AHISF_PLAY;
        stopflags |= AHISF_PLAY;
      }
      update=FALSE;
      break;
    case AHIC_Record:
      if(tag->ti_Data)
      {
        playflags |= AHISF_RECORD;
        stopflags &= ~AHISF_RECORD;
      }
      else
      {
        playflags &= ~AHISF_RECORD;
        stopflags |= AHISF_RECORD;
      }
      update=FALSE;
      break;
    case AHIC_MixFreq_Query:
      *ptr=audioctrl->ac.ahiac_MixFreq;
      break;
    case AHIC_MonitorVolume:
    case AHIC_InputGain:
    case AHIC_OutputVolume:
    case AHIC_Input:
    case AHIC_Output:
      AHIsub_HardwareControl(tag->ti_Tag, tag->ti_Data, (struct AHIAudioCtrlDrv *)audioctrl);
      break;
    case AHIC_MonitorVolume_Query:
    case AHIC_InputGain_Query:
    case AHIC_OutputVolume_Query:
    case AHIC_Input_Query:
    case AHIC_Output_Query:
      *ptr=AHIsub_HardwareControl(tag->ti_Tag, NULL, (struct AHIAudioCtrlDrv *)audioctrl);
      break;
    }
  }
// Let's act!
  if(update)
    AHIsub_Update(NULL,(struct AHIAudioCtrlDrv *)audioctrl);
  if(playflags)
    rc=AHIsub_Start(playflags,(struct AHIAudioCtrlDrv *)audioctrl);
  if(stopflags)
    AHIsub_Stop(stopflags,(struct AHIAudioCtrlDrv *)audioctrl);

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n",rc);
  }
  return rc;
}

/******************************************************************************
** AHI_GetAudioAttrsA *********************************************************
******************************************************************************/

__asm BOOL GetAudioAttrsA( register __d0 ULONG id,
    register __a2 struct AHIAudioCtrlDrv *actrl,
    register __a1 struct TagItem *tags)
{
  struct AHI_AudioDatabase *audiodb;
  struct TagItem *dbtags,*tag1,*tag2,*tstate=tags;
  ULONG *ptr;
  ULONG stringlen;
  struct Library *AHIsubBase=NULL;
  struct AHIAudioCtrlDrv *audioctrl=NULL;
  BOOL rc=TRUE; // TRUE == _everything_ went well
  struct TagItem idtag[2] = { AHIA_AudioID, 0, TAG_DONE };

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("AHI_GetAudioAttrsA(0x%08lx, 0x%08lx, 0x%08lx)",id,actrl,tags);
  }

  if(audiodb=LockDatabase())
  {
    if(id == AHI_INVALID_ID)
    {
      if(!(audioctrl=actrl))
        rc=FALSE;
      else
        idtag[0].ti_Data=((struct AHIPrivAudioCtrl *)actrl)->ahiac_AudioID;
    }
    else
    {
      idtag[0].ti_Data=id;
      audioctrl=(struct AHIAudioCtrlDrv *)CreateAudioCtrl(idtag);
    }
    if(audioctrl && rc )
    {
      if(dbtags=GetDBTagList(audiodb, idtag[0].ti_Data))
      {
        stringlen=GetTagData(AHIDB_BufferLen,0,tags);
        if(AHIsubBase=OpenLibrary(((struct AHIPrivAudioCtrl *)audioctrl)->ahiac_DriverName,DriverVersion))
        {
          while(tag1=NextTagItem(&tstate))
          {
            ptr=(ULONG *)tag1->ti_Data;
            switch(tag1->ti_Tag)
            {
            case AHIDB_Driver:
            case AHIDB_Name:
              if(tag2=FindTagItem(tag1->ti_Tag,dbtags))
                stccpy((char *)tag1->ti_Data,(char *)tag2->ti_Data,stringlen);
              break;
// Skip these!
            case AHIDB_FrequencyArg:
            case AHIDB_IndexArg:
            case AHIDB_InputArg:
            case AHIDB_OutputArg:
              break;
// Strings
            case AHIDB_Author:
            case AHIDB_Copyright:
            case AHIDB_Version:
            case AHIDB_Annotation:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,0,(ULONG)"",dbtags,audioctrl),stringlen);
              break;
// Input & Output strings
            case AHIDB_Input:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,GetTagData(AHIDB_InputArg,0,tags),(ULONG)"Default",dbtags,audioctrl),stringlen);
              break;
            case AHIDB_Output:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,GetTagData(AHIDB_OutputArg,0,tags),(ULONG)"Default",dbtags,audioctrl),stringlen);
              break;
// Other
            case AHIDB_Bits:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0,dbtags,audioctrl);
              break;
            case AHIDB_MaxChannels:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,128,dbtags,audioctrl);
              break;
            case AHIDB_MinMixFreq:
              *ptr=AHIsub_GetAttr(AHIDB_Frequency,0,0,dbtags,audioctrl);
              break;
            case AHIDB_MaxMixFreq:
              *ptr=AHIsub_GetAttr(AHIDB_Frequency,(AHIsub_GetAttr(AHIDB_Frequencies,1,0,dbtags,audioctrl)-1),0,dbtags,audioctrl);
              break;
            case AHIDB_Frequencies:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,1,dbtags,audioctrl);
              break;
            case AHIDB_Frequency:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,GetTagData(AHIDB_FrequencyArg,0,tags),0,dbtags,audioctrl);
              break;
            case AHIDB_Index:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,GetTagData(AHIDB_IndexArg,0,tags),0,dbtags,audioctrl);
              break;
            case AHIDB_MaxPlaySamples:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,audioctrl->ahiac_MaxBuffSamples,dbtags,audioctrl);
              break;
            case AHIDB_MaxRecordSamples:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0,dbtags,audioctrl);
              break;
            case AHIDB_MinMonitorVolume:
            case AHIDB_MaxMonitorVolume:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0x00000,dbtags,audioctrl);
              break;
            case AHIDB_MinInputGain:
            case AHIDB_MaxInputGain:
            case AHIDB_MinOutputVolume:
            case AHIDB_MaxOutputVolume:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0x10000,dbtags,audioctrl);
              break;
            case AHIDB_Inputs:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,0,dbtags,audioctrl);
              break;
            case AHIDB_Outputs:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,1,dbtags,audioctrl);
              break;
// Booleans that defaults to FALSE
            case AHIDB_Realtime:
            case AHIDB_Record:
            case AHIDB_FullDuplex:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,FALSE,dbtags,audioctrl);
              break;
// Booleans that defaults to TRUE
            case AHIDB_PingPong:
              *ptr=AHIsub_GetAttr(tag1->ti_Tag,0,TRUE,dbtags,audioctrl);
              break;
// Tags from the database.
            default:
              if(tag2=FindTagItem(tag1->ti_Tag,dbtags))
                *ptr=tag2->ti_Data;
              break;
            }
          }
        }
        else // no AHIsubBase
          rc=FALSE;
      }
      else // no database taglist
        rc=FALSE;
    }
    else // no valid audioctrl
       rc=FALSE;
    if(id != AHI_INVALID_ID)
      FreeVec(audioctrl);
    if(AHIsubBase)
      CloseLibrary(AHIsubBase);
    UnlockDatabase(audiodb);
  }
  else // unable to lock database
    rc=FALSE;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("=>%s\n", rc ? "TRUE" : "FALSE" );
  }

  return rc;
}

/******************************************************************************
** AHI_BestAudioIDA ***********************************************************
******************************************************************************/

__asm ULONG BestAudioIDA( register __a1 struct TagItem *tags )
{
  ULONG id = AHI_INVALID_ID, bestid = 0;
  Fixed score, bestscore = 0;
  struct TagItem *dizzytags;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_BestAudioIDA(0x%08lx)",tags);
  }

  dizzytags = (struct TagItem *) GetTagData(AHIB_Dizzy,NULL,tags);

  while(AHI_INVALID_ID != (id=AHI_NextAudioID(id)))
  {
    if(!TestAudioID(id,tags))
    {
      continue;
    }

    // Check if this id the better than the last one
    score = DizzyTestAudioID(id,tags);
    if(score > bestscore)
    {
      bestscore = score;
      bestid = id;
    }
    else if(score == bestscore)
    {
      if(id > bestid)
      {
        bestid = id;    // Return the highest suitable audio id.
      }
    }
  }

  if(bestid == 0)
  {
    bestid = AHI_INVALID_ID;
  }

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>0x%08lx\n",bestid);
  }

  return bestid;
}

/******************************************************************************
** AHI_PlayA ******************************************************************
******************************************************************************/

__asm ULONG PlayA( register __a2 struct AHIAudioCtrl *audioctrl,
    register __a1 struct TagItem *tags)
{
  struct TagItem *tag,*tstate=tags;
  struct Library *AHIsubBase=((struct AHIPrivAudioCtrl *)audioctrl)->ahiac_SubLib;
  BOOL  setfreq,setvol,setsound,loopsetfreq,loopsetvol,loopsetsound;
  ULONG channel,freq,vol,pan,sound,offset,length;
  ULONG loopfreq,loopvol,looppan,loopsound,loopoffset,looplength;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_ALL)
  {
    KPrintF("AHI_PlayA(0x%08lx, 0x%08lx)\n",audioctrl,tags);
  }

  AHIsub_Disable((struct AHIAudioCtrlDrv *)audioctrl);

  while(tag=NextTagItem(&tstate))
  {
    switch(tag->ti_Tag)
    {
    case AHIP_BeginChannel:
      channel=tag->ti_Data;
      setfreq=setvol=setsound=loopsetfreq=loopsetvol=loopsetsound= \
      vol=pan=offset=length=loopvol=looppan=loopoffset=looplength=0;
      break;
    case AHIP_Freq:
      loopfreq=
      freq=tag->ti_Data;
      setfreq=TRUE;
      break;
    case AHIP_Vol:
      loopvol=
      vol=tag->ti_Data;
      setvol=TRUE;
      break;
    case AHIP_Pan:
      looppan=
      pan=tag->ti_Data;
      setvol=TRUE;
      break;
    case AHIP_Sound:
      loopsound=
      sound=tag->ti_Data;
      setsound=TRUE;
      break;
    case AHIP_Offset:
      loopoffset=
      offset=tag->ti_Data;
      break;
    case AHIP_Length:
      looplength=
      length=tag->ti_Data;
      break;
    case AHIP_LoopFreq:
      loopfreq=tag->ti_Data;
      loopsetfreq=TRUE;
      break;
    case AHIP_LoopVol:
      loopvol=tag->ti_Data;
      loopsetvol=TRUE;
      break;
    case AHIP_LoopPan:
      looppan=tag->ti_Data;
      loopsetvol=TRUE;
      break;
    case AHIP_LoopSound:
      loopsound=tag->ti_Data;
      loopsetsound=TRUE;
      break;
    case AHIP_LoopOffset:
      loopoffset=tag->ti_Data;
      loopsetsound=TRUE;           // AHIP_LoopSound: doesn't have to be present
      break;
    case AHIP_LoopLength:
      looplength=tag->ti_Data;
      break;
    case AHIP_EndChannel:
      if(setfreq)
        AHI_SetFreq(channel,freq,audioctrl,AHISF_IMM);
      if(loopsetfreq)
        AHI_SetFreq(channel,loopfreq,audioctrl,NULL);
      if(setvol)
        AHI_SetVol(channel,vol,pan,audioctrl,AHISF_IMM);
      if(loopsetvol)
        AHI_SetVol(channel,loopvol,looppan,audioctrl,NULL);
      if(setsound)
        AHI_SetSound(channel,sound,offset,length,audioctrl,AHISF_IMM);
      if(loopsetsound)
        AHI_SetSound(channel,loopsound,loopoffset,looplength,audioctrl,NULL);
      break;
    }
  }

  AHIsub_Enable((struct AHIAudioCtrlDrv *)audioctrl);
  return NULL;
}

/******************************************************************************
** AHI_SampleFrameSize ********************************************************
******************************************************************************/

const static UBYTE type2bytes[]=
{
  1,    // AHIST_M8S  (0)
  2,    // AHIST_M16S (1)
  2,    // AHIST_S8S  (2)
  4,    // AHIST_S16S (3)
  1,    // AHIST_M8U  (4)
  0,
  0,
  0,
  4,    // AHIST_M32S (8)
  0,
  8     // AHIST_S32S (10)
};

__asm ULONG SampleFrameSize( register __d0 ULONG sampletype )
{
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AHI_SampleFrameSize(%ld)=>%ld\n",sampletype,type2bytes[sampletype]);
  }

  return type2bytes[sampletype];
}
