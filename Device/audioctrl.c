/* $Id$
* $Log$
* Revision 1.12  1997/02/12 15:32:45  lcs
* Moved each autodoc header to the file where the function is
*
* Revision 1.11  1997/02/10 02:23:06  lcs
* Infowindow in the requester added.
*
* Revision 1.10  1997/02/04 15:44:30  lcs
* AHIDB_MaxChannels didn't work in AHI_BestAudioID()
*
* Revision 1.9  1997/02/02 22:35:50  lcs
* Localized it
*
* Revision 1.8  1997/02/02 18:15:04  lcs
* Added protection against CPU overload
*
* Revision 1.7  1997/02/01 21:54:53  lcs
* Will never use drivers that are newer than itself anymore
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
#include "localize.h"

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
extern __asm BOOL PreTimer(void);
extern __asm void PostTimer(void);
extern __asm BOOL DummyPreTimer(void);
extern __asm void DummyPostTimer(void);


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
      audioctrl->ahiac_AudioID = AHIBase->ahib_AudioMode;

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
        stpcpy(stpcpy(stpcpy(audioctrl->ahiac_DriverName,"DEVS:ahi/"),
            (char *)GetTagData(AHIDB_Driver,(ULONG)"",dbtags)),".audio");
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
    total++;
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

/****** ahi.device/AHI_AllocAudioA ******************************************
*
*   NAME
*       AHI_AllocAudioA -- allocates and initializes the audio hardware
*       AHI_AllocAudio -- varargs stub for AHI_AllocAudioA()
*
*   SYNOPSIS
*       audioctrl = AHI_AllocAudioA( tags );
*       D0                           A1
*
*       struct AHIAudioCtrl *AHI_AllocAudioA( struct TagItem * );
*
*       audioctrl = AHI_AllocAudio( tag1, ... );
*
*       struct AHIAudioCtrl *AHI_AllocAudio( Tag, ... );
*
*   FUNCTION
*       Allocates and initializes the audio hardware, selects the best
*       mixing routine (if neccessary) according to the supplied tags.
*       To start playing you first need to call AHI_ControlAudioA().
*
*   INPUTS
*       tags - A pointer to a tag list.
*
*   TAGS
*
*       AHIA_AudioID (ULONG) - The audio mode to use (AHI_DEFAULT_ID is the
*           user's default mode. It's a good value to use the first time she
*           starts your application.
*
*       AHIA_MixFreq (ULONG) - Desired mixing frequency. The actual
*           mixing rate may or may not be exactly what you asked for.
*           AHI_DEFAULT_FREQ is the user's prefered frequency.
*
*       AHIA_Channels (UWORD) - Number of channel to use. The actual
*           number of channels used will be equal or grater than the
*           requested. If too many channels were requested, this function
*           will fail. This tag must be supplied.
*
*       AHIA_Sounds (UWORD) - Number of sounds to use. This tag must be
*           supplied.
*
*       AHIA_SoundFunc (struct Hook *) - A function to call each time
*           when a sound has been started. The function receives the
*           following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHISoundMessage *)
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply.
*           The called function should follow normal register conventions,
*           which means that d2-d7 and a2-a6 must be preserved.
*
*       AHIA_PlayerFunc (struct Hook *) - A function to be called at regular
*           intervals. By using this hook there is no need for music players
*           to use other timing, such as VBLANK or CIA timers. But the real
*           reason it's present is that it makes it possible to do non-
*           realtime mixing to disk.
*           Using this interrupt source is currently the only supported way
*           to ensure that no mixing occurs between calls to AHI_SetVol(),
*           AHI_SetFreq() or AHI_SetSound().
*           If the sound playback is done without mixing, 'realtime.library'
*           is used to provide timing. The function receives the following
*           parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - Undefined.
*           Do not assume A1 contains any particular value!
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply.
*           The called function should follow normal register conventions,
*           which means that d2-d7 and a2-a6 must be preserved.
*
*       AHIA_PlayerFreq (Fixed) - If non-zero, enables timing and specifies
*           how many times per second PlayerFunc will be called. This must
*           be specified if AHIA_PlayerFunc is! Do not use any extreme
*           frequences. The result of MixFreq/PlayerFreq must fit an UWORD,
*           ie it must be less or equal to 65535. It is also suggested that
*           you keep the result over 80. For normal use this should not be a
*           problem. Note that the data type is Fixed, not integer. 50 Hz is
*           50<<16.
*
*       AHIA_MinsPlayerFreq (Fixed) - The minimum frequency (AHIA_PlayerFreq)
*           you will use. You should always supply this if you are using the
*           device's interrupt feature!
*
*       AHIA_MaxPlayerFreq (Fixed) - The maximum frequency (AHIA_PlayerFreq)
*           you will use. You should always supply this if you are using the
*           device's interrupt feature!
*
*       AHIA_RecordFunc (struct Hook *) - This function will be called
*           regulary when sampling is turned on (see AHI_ControlAudioA())
*           with the following parameters:
*               A0 - (struct Hook *)
*               A2 - (struct AHIAudioCtrl *)
*               A1 - (struct AHIRecordMessage *)
*           The message (AHIRecordMessage) is filled as follows:
*               ahirm_Buffer - Pointer to the samples. The buffer is valid
*                   until next time the Hook is called.
*               ahirm_Length - Number of sample FRAMES in buffer.
*                   To get the size in bytes, multiply by 4 if ahiim_Type is
*                   AHIST_S16S.
*               ahirm_Type - Always AHIST_S16S at the moment, but you *must*
*                   check this, since it may change in the future!
*           The hook may be called from an interrupt, so normal interrupt
*           restrictions apply. Signal a process if you wish to save the
*           buffer to disk. The called function should follow normal register
*           conventions, which means that d2-d7 and a2-a6 must be preserved.
*       *** NOTE: The function MUST return NULL (in d0). This was previously
*           not documented. Now you know.
*
*       AHIA_UserData (APTR) - Can be used to initialise the ahiac_UserData
*           field.
*
*   RESULT
*       A pointer to an AHIAudioCtrl structure or NULL if an error occured.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_FreeAudio(), AHI_ControlAudioA()
*
****************************************************************************
*
*/

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

  audioctrl->ahiac_SubAllocRC = AHISF_ERROR;
  audioctrl->ahiac_SubLib=
  AHIsubBase = OpenLibrary(audioctrl->ahiac_DriverName,DriverVersion);
  if(!AHIsubBase)
    goto error;

  // Never allow drivers that are newer than ahi.device.
  if(AHIsubBase->lib_Version > Version)
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


    audioctrl->ac.ahiac_MixerFunc->h_Entry = (ULONG (*)()) Mix;

    if((AHIBase->ahib_MaxCPU >= 0x10000) || (AHIBase->ahib_MaxCPU <= 0x0))
    {
      audioctrl->ac.ahiac_PreTimer  = (BOOL (*)()) DummyPreTimer;
      audioctrl->ac.ahiac_PostTimer = (void (*)()) DummyPostTimer;
    }
    else
    {
      audioctrl->ahiac_MaxCPU       = AHIBase->ahib_MaxCPU >> 8;
      audioctrl->ac.ahiac_PreTimer  = (BOOL (*)()) PreTimer;
      audioctrl->ac.ahiac_PostTimer = (void (*)()) PostTimer;
    }

/* FIXIT!
    audioctrl->ac.ahiac_SamplerFunc=AllocVec(sizeof(struct Hook),MEMF_PUBLIC!MEMF_CLEAR);
    if(!audioctrl->ac.ahiac_SamplerFunc)
      goto error;

    audioctrl->ac.ahiac_SamplerFunc->h_Entry=Sampler;
*/
  }

  /* Set default hardware properties, only if AHI_DEFAULT_ID was used!*/
  if(GetTagData(AHIA_AudioID, AHI_DEFAULT_ID, tags) == AHI_DEFAULT_ID)
  {
    AHI_ControlAudio((struct AHIAudioCtrl *)audioctrl,
        AHIC_MonitorVolume,   AHIBase->ahib_MonitorVolume,
        AHIC_InputGain,       AHIBase->ahib_InputGain,
        AHIC_OutputVolume,    AHIBase->ahib_OutputVolume,
        AHIC_Input,           AHIBase->ahib_Input,
        AHIC_Output,          AHIBase->ahib_Output,
        TAG_DONE);
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

/****** ahi.device/AHI_FreeAudio *******************************************
*
*   NAME
*       AHI_FreeAudio -- deallocates the audio hardware
*
*   SYNOPSIS
*       AHI_FreeAudio( audioctrl );
*                      A2
*
*       void AHI_FreeAudio( struct AHIAudioCtrl * );
*
*   FUNCTION
*       Deallocates the AHIAudioCtrl structure and any other resources
*       allocated by AHI_AllocAudioA(). After this call it must not be used
*       by any other functions anymore. AHI_UnloadSound() is automatically
*       called for every sound.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure obtained from
*           AHI_AllocAudioA(). If NULL, this function does nothing.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_AllocAudioA(), AHI_KillAudio(), AHI_UnloadSound()
*
****************************************************************************
*
*/

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

/****i* ahi.device/AHI_KillAudio *******************************************
*
*   NAME
*      AHI_KillAudio -- clean up
*
*   SYNOPSIS
*      AHI_KillAudio();
*
*      void AHI_KillAudio( void );
*
*   FUNCTION
*      'ahi.device' keeps track of most of what the user does. This call is
*      used to clean up as much as possible. It must never, ever, be used
*      in an application. It is included for development use only, and can
*      be used to avoid rebooting the computer if your program has allocated
*      the audio hardware and crashed. This call can lead to a system crash,
*      so don't use it if you don't have to.
*
*   INPUTS
*
*   RESULT
*      This function returns nothing. In fact, it may never return.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_FreeAudio()
*
****************************************************************************
*
*/

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

/****** ahi.device/AHI_ControlAudioA ***************************************
*
*   NAME
*       AHI_ControlAudioA -- change audio attributes
*       AHI_ControlAudio -- varargs stub for AHI_ControlAudioA()
*
*   SYNOPSIS
*       error = AHI_ControlAudioA( audioctrl, tags );
*       D0                         A2         A1
*
*       ULONG AHI_ControlAudioA( struct AHIAudioCtrl *, struct TagItem * );
*
*       error = AHI_ControlAudio( AudioCtrl, tag1, ...);
*
*       ULONG AHI_ControlAudio( struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       This function should be used to change attributes for a given
*       AHIAudioCtrl structure. It is also used to start and stop playback,
*       and to control special hardware found on some sound cards.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIC_Play (BOOL) - Starts (TRUE) and stops (FALSE) playback and
*           PlayerFunc. NOTE: If the audio hardware cannot play at the same
*           time as recording samples, the recording will be stopped.
*
*       AHIC_Record (BOOL) - Starts (TRUE) and stops (FALSE) sampling and
*           RecordFunc. NOTE: If the audio hardware cannot record at the same
*           time as playing samples, the playback will be stopped.
*
*       AHIC_MonitorVolume (Fixed) - Sets the input monitor volume, i.e. how
*           much of the input signal is mixed with the output signal while
*           recording. Use AHI_GetAudioAttrsA() to find the available range.
*
*       AHIC_MonitorVolume_Query (Fixed *) - Get the current input monitor
*           volume. ti_Data is a pointer to a Fixed variable, where the result
*           will be stored.
*
*       AHIC_MixFreq_Query (ULONG *) - Get the current mixing frequency.
*           ti_Data is a pointer to an ULONG variable, where the result will
*           be stored.
*
*       AHIC_InputGain (Fixed) - Set the input gain. Use AHI_GetAudioAttrsA()
*           to find the available range. (V2)
*
*       AHIC_InputGain_Query (Fixed *) - Get current input gain. (V2)
*
*       AHIC_OutputVolume (Fixed) - Set the output volume. Use
*           AHI_GetAudioAttrsA() to find the available range. (V2)
*
*       AHIC_OutputVolume_Query (Fixed *) - Get current output volume. (V2)
*
*       AHIC_Input (ULONG) - Select input source. See AHI_GetAudioAttrsA().
*           (V2)
*
*       AHIC_Input_Query (ULONG *) - Get current input source. (V2)
*
*       AHIC_Output (ULONG) - Select destination for output. See
*           AHI_GetAudioAttrsA(). (V2)
*
*       AHIC_Output_Query (ULONG *) - Get destination for output. (V2)
*
*       The following tags are also recognized by AHI_ControlAudioA(). See
*       AHI_AllocAudioA() for what they do. They may be used from interrupts.
*
*       AHIA_SoundFunc (struct Hook *)
*       AHIA_PlayerFunc (struct Hook *)
*       AHIA_PlayerFreq (Fixed)
*       AHIA_RecordFunc (struct Hook *)
*       AHIA_UserData (APTR)
*
*       Note that AHIA_PlayerFreq must never be outside the limits specified
*       with AHIA_MinPlayerFreq and AHIA_MaxPlayerFreq!
*
*   RESULT
*       An error code, defined in <devices/ahi.h>.
*
*   EXAMPLE
*
*   NOTES
*       The AHIC_Play and AHIC_Record tags *must not* be used from
*       interrupts.
*
*   BUGS
*
*   SEE ALSO
*       AHI_AllocAudioA(), AHI_GetAudioAttrsA(), <devices/ahi.h>
*
****************************************************************************
*
*/

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

/****** ahi.device/AHI_GetAudioAttrsA ***************************************
*
*   NAME
*       AHI_GetAudioAttrsA -- examine an audio mode via a tag list
*       AHI_GetAudioAttrs -- varargs stub for AHI_GetAudioAttrsA()
*
*   SYNOPSIS
*       success = AHI_GetAudioAttrsA( ID, [audioctrl], tags );
*       D0                            D0  A2           A1
*
*       BOOL AHI_GetAudioAttrsA( ULONG, struct AHIAudioCtrl *,
*                                struct TagItem * );
*
*       success = AHI_GetAudioAttrs( ID, [audioctrl], attr1, &result1, ...);
*
*       BOOL AHI_GetAudioAttrs( ULONG, struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       Retrieve information about an audio mode specified by ID or audioctrl
*       according to the tags in the tag list. For each entry in the tag
*       list, ti_Tag identifies the attribute, and ti_Data is mostly a
*       pointer to a LONG (4 bytes) variable where you wish the result to be
*       stored.
*
*   INPUTS
*       ID - An audio mode identifier or AHI_INVALID_ID.
*       audioctrl - A pointer to an AHIAudioCtrl structure, only used if
*           ID equals AHI_INVALID_ID. Set to NULL if not used. If set to
*           NULL when used, this function returns immediately. Always set
*           ID to AHI_INVALID_ID and use audioctrl if you have allocated
*           a valid AHIAudioCtrl structure. Some of the tags return incorrect
*           values otherwise.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIDB_Volume (ULONG *) - TRUE if this mode supports volume changes.
*
*       AHIDB_Stereo (ULONG *) - TRUE if output is in stereo. Unless
*           AHIDB_Panning (see below) is TRUE, all even channels are played
*           to the left and all odd to the right.
*
*       AHIDB_Panning (ULONG *) - TRUE if this mode supports stereo panning.
*
*       AHIDB_HiFi (ULONG *) - TRUE if no shortcuts, like predivision, is
*           used by the mixing routines.
*
*       AHIDB_PingPong (ULONG *) - TRUE if this mode can play samples backwards.
*
*       AHIDB_Record (ULONG *) - TRUE if this mode can record samples.
*
*       AHIDB_FullDuplex (ULONG *) - TRUE if this mode can record and play at
*           the same time.
*
*       AHIDB_Realtime (ULONG *) - Modes which return TRUE for this fulfils
*           two criteria:
*           1) Calls to AHI_SetVol(), AHI_SetFreq() or AHI_SetSound() will be
*              preformed within (about) 10 ms if called from a PlayFunc Hook.
*           2) The PlayFunc Hook will be called at the specifed frequency.
*           If you don't use AHI's PlayFunc Hook, you must not use modes that
*           are not realtime. (Criterium 2 is not that obvious if you consider
*           a mode that renders the output to disk as a sample.)
*
*       AHIDB_Bits (ULONG *) - The number of output bits (8, 12, 14, 16 etc).
*
*       AHIDB_MaxChannels (ULONG *) - The maximum number of channels this mode
*           can handle.
*
*       AHIDB_MinMixFreq (ULONG *) - The minimum mixing frequency supported.
*
*       AHIDB_MaxMixFreq (ULONG *) - The maximum mixing frequency supported.
*
*       AHIDB_Frequencies (ULONG *) - The number of different sample rates
*           available.
*
*       AHIDB_FrequencyArg (ULONG) - Specifies which frequency
*           AHIDB_Frequency should return (see below). Range is 0 to
*           AHIDB_Frequencies-1 (including).
*           NOTE: ti_Data is NOT a pointer, but an ULONG.
*
*       AHIDB_Frequency (ULONG *) - Return the frequency associated with the
*           index number specified with AHIDB_FrequencyArg (see above).
*
*       AHIDB_IndexArg (ULONG) - AHIDB_Index will return the index which
*           gives the closest frequency to AHIDB_IndexArg
*           NOTE: ti_Data is NOT a pointer, but an ULONG.
*
*       AHIDB_Index (ULONG *) - Return the index associated with the frequency
*           specified with AHIDB_IndexArg (see above).
*
*       AHIDB_MaxPlaySamples (ULONG *) - Return the lowest number of sample
*           frames that must be present in memory when AHIST_DYNAMICSAMPLE
*           sounds are used. This number must then be scaled by Fs/Fm, where
*           Fs is the frequency of the sound and Fm is the mixing frequency.
*
*       AHIDB_MaxRecordSamples (ULONG *) - Return the number of sample frames
*           you will recieve each time the RecordFunc is called.
*
*       AHIDB_BufferLen (ULONG) - Specifies how many characters will be
*           copyed when requesting text attributes. Default is 0, which
*           means that AHIDB_Driver, AHIDB_Name, AHIDB_Author,
*           AHIDB_Copyright, AHIDB_Version and AHIDB_Annotation,
*           AHIDB_Input and AHIDB_Output will do nothing.
*
*       AHIDB_Driver (STRPTR) - Name of driver (excluding path and
*           extention). 
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Name (STRPTR) - Human readable name of this mode.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Author (STRPTR) - Name of driver author.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Copyright (STRPTR) - Driver copyright notice.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen
*
*       AHIDB_Version (STRPTR) - Driver version string.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_Annotation (STRPTR) - Annotation by driver author.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen.
*
*       AHIDB_MinMonitorVolume (Fixed *)
*       AHIDB_MaxMonitorVolume (Fixed *) - Lower/upper limit for input
*           monitor volume, see AHI_ControlAudioA(). If both are 0.0,
*           the sound hardware does not have an input monitor feature.
*           If both are same, but not 0.0, the hardware always sends the
*           recorded sound to the outputs (at the given volume). (V2)
*
*       AHIDB_MinInputGain (Fixed *)
*       AHIDB_MaxInputGain (Fixed *) - Lower/upper limit for input gain,
*           see AHI_ControlAudioA(). If both are same, there is no input
*           gain hardware. (V2)
*
*       AHIDB_MinOutputVolume (Fixed *)
*       AHIDB_MaxOutputVolume (Fixed *) - Lower/upper limit for output
*           volume, see AHI_ControlAudioA(). If both are same, the sound
*           card does not have volume control. (V2)
*
*       AHIDB_Inputs (ULONG *) - The number of inputs the sound card has.
*           (V2)
*
*       AHIDB_InputArg (ULONG) - Specifies what AHIDB_Input should return
*           (see below). Range is 0 to AHIDB_Inputs-1 (including).
*           NOTE: ti_Data is NOT a pointer, but an ULONG. (V2)
*
*       AHIDB_Input (STRPTR) - Gives a human readable string describing the
*           input associated with the index specified with AHIDB_InputArg
*           (see above). See AHI_ControlAudioA() for how to select one.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen. (V2)
*
*       AHIDB_Outputs (ULONG *) - The number of outputs the sound card
*           has. (V2)
*
*       AHIDB_OutputArg (ULONG) - Specifies what AHIDB_Output should return
*           (see below). Range is 0 to AHIDB_Outputs-1 (including)
*           NOTE: ti_Data is NOT a pointer, but an ULONG. (V2)
*
*       AHIDB_Output (STRPTR) - Gives a human readable string describing the
*           output associated with the index specified with AHIDB_OutputArg
*           (see above). See AHI_ControlAudioA() for how to select one.
*           NOTE: ti_Data is a pointer to an UBYTE array where the name
*           will be stored. See AHIDB_BufferLen. (V2)
*
*       If the requested information cannot be found, the variable will be not
*       be touched.
*
*   RESULT
*       TRUE if everything went well.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       In versions earlier than 3, the tags that filled a string buffer would
*       not NULL-terminate the string on buffer overflows.
*
*   SEE ALSO
*      AHI_NextAudioID(), AHI_BestAudioIDA()
*
****************************************************************************
*
*/

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
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,
                  GetTagData(AHIDB_InputArg,0,tags),
                  (ULONG) GetahiString(msgDefault),dbtags,audioctrl),stringlen);
              break;
            case AHIDB_Output:
              stccpy((char *)tag1->ti_Data,(char *)AHIsub_GetAttr(tag1->ti_Tag,
                  GetTagData(AHIDB_OutputArg,0,tags),
                  (ULONG) GetahiString(msgDefault),dbtags,audioctrl),stringlen);
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

/****** ahi.device/AHI_BestAudioIDA *****************************************
*
*   NAME
*       AHI_BestAudioIDA -- calculate the best ModeID with given parameters
*       AHI_BestAudioID -- varargs stub for AHI_BestAudioIDA()
*
*   SYNOPSIS
*       ID = AHI_BestAudioIDA( tags );
*       D0                     A1
*
*       ULONG AHI_BestAudioIDA( struct TagItem * );
*
*       ID = AHI_BestAudioID( tag1, ... );
*
*       ULONG AHI_BestAudioID( Tag, ... );
*
*   FUNCTION
*       Determines the best AudioID to fit the parameters set in the tag
*       list.
*
*   INPUTS
*       tags - A pointer to a tag list. Only the tags present matter.
*
*   TAGS
*       Many combinations are probably stupid to ask for, like not supporting
*       panning or recording.
*
*       AHIDB_AudioID (ULONG) - The mode must use the same audio hardware
*           as this mode does.
*
*       AHIDB_Volume (BOOL) - If TRUE: mode must support volume changes.
*           If FALSE: mode must not support volume changes.
*
*       AHIDB_Stereo (BOOL) - If TRUE: mode must have stereo output.
*           If FALSE: mode must not have stereo output (=mono).
*
*       AHIDB_Panning (BOOL) - If TRUE: mode must support volume panning.
*           If FALSE: mode must not support volume panning. 
*
*       AHIDB_HiFi (BOOL) - If TRUE: mode must have HiFi output.
*           If FALSE: mode must not have HiFi output.
*
*       AHIDB_PingPong (BOOL) - If TRUE: mode must support playing samples
*           backwards. If FALSE: mode must not support playing samples
*           backwards.
*
*       AHIDB_Record (BOOL) - If TRUE: mode must support recording. If FALSE:
*           mode must not support recording.
*
*       AHIDB_Realtime (BOOL) - If TRUE: mode must be realtime. If FALSE:
*           take a wild guess.
*
*       AHIDB_FullDuplex (BOOL) - If TRUE: mode must be able to record and
*           play at the same time.
*
*       AHIDB_Bits (UBYTE) - Mode must have greater or equal number of bits.
*
*       AHIDB_MaxChannels (UWORD) - Mode must have greater or equal number
*           of channels.
*
*       AHIDB_MinMixFreq (ULONG) - Lowest mixing frequency supported must be
*           less or equal.
*
*       AHIDB_MaxMixFreq (ULONG) - Highest mixing frequency must be greater
*           or equal.
*
*       AHIB_Dizzy (struct TagItem *) - This tag points to a second tag list.
*           After all other tags has been tested, the mode that matches these
*           tags best is returned, i.e. the one that has most of the features
*           you ask for, and least of the ones you don't want. Without this
*           second tag list, this function hardly does what its name
*           suggests. (V3)
*
*   RESULT
*       ID - The best AudioID to use or AHI_INVALID_ID if none of the modes
*           in the audio database could meet the requirements.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      AHI_NextAudioID(), AHI_GetAudioAttrsA()
*
****************************************************************************
*
*/

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

/****** ahi.device/AHI_PlayA ************************************************
*
*   NAME
*       AHI_PlayA -- Start multiple sounds in one call (V3)
*       AHI_Play -- varargs stub for AHI_PlayA()
*
*   SYNOPSIS
*       AHI_PlayA( audioctrl, tags );
*                  A2         A1
*
*       void AHI_PlayA( struct AHIAudioCtrl *, struct TagItem * );
*
*       AHI_Play( AudioCtrl, tag1, ...);
*
*       void AHI_Play( struct AHIAudioCtrl *, Tag, ... );
*
*   FUNCTION
*       This function performs the same actions as multiple calls to
*       AHI_SetFreq(), AHI_SetSound() and AHI_SetVol(). The advantages
*       of using only one call is that simple loops can be set without
*       using a SoundFunc (see AHI_AllocAudioA(), tag AHIA_SoundFunc) and
*       that sounds on different channels can be syncronized even when the
*       sounds are not started from a PlayerFunc (see AHI_AllocAudioA(), tag
*       AHIA_PlayerFunc). The disadvantage is that this call has more
*       overhead than AHI_SetFreq(), AHI_SetSound() and AHI_SetVol(). It is
*       therefore recommended that you only use this call if you are not
*       calling from a SoundFunc or PlayerFunc.
*
*       The supplied tag list works like a 'program'. This means that
*       the order of tags matter.
*
*   INPUTS
*       audioctrl - A pointer to an AHIAudioCtrl structure.
*       tags - A pointer to a tag list.
*
*   TAGS
*       AHIP_BeginChannel (UWORD) - Before you start setting attributes
*           for a sound to play, you have to use this tag to chose a
*           channel to operate on. If AHIP_BeginChannel is omitted, the
*           resullt is undefined.
*
*       AHIP_EndChannel (ULONG) - Signals the end of attributes for
*           the current channel. If AHIP_EndChannel is omitted, the result
*           is undefined. ti_Data MUST BE NULL!
*
*       AHIP_Freq (ULONG) - The playback frequency in Hertz or AHI_MIXFREQ.
*
*       AHIP_Vol (Fixed) - The desired volume. If omitted, but AHIP_Pan is
*           present, AHIP_Vol defaults to 0.
*
*       AHIP_Pan (sposition) - The desired panning. If omitted, but AHIP_Vol
*           is present, AHIP_Pan defaults to 0 (extreme left).
*
*       AHIP_Sound (UWORD) - Sound to be played, or AHI_NOSOUND.
*
*       AHIP_Offset (ULONG) - Specifies an offset (in samples) into the
*           sound. If this tag is present, AHIP_Length MUST be present too!
*
*       AHIP_Length (LONG) - Specifies how many samples that should be
*           player.
*
*       AHIP_LoopFreq (ULONG)
*       AHIP_LoopVol (Fixed)
*       AHIP_LoopPan (sposition)
*       AHIP_LoopSound (UWORD)
*       AHIP_LoopOffset (ULONG)
*       AHIP_LoopLength (LONG) - These tags can be used to set simple loop
*          attributes. They default to their sisters. These tags must be
*          after the other tags.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       AHI_SetFreq(), AHI_SetSound(), AHI_SetVol()
*
****************************************************************************
*
*/

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

/****** ahi.device/AHI_SampleFrameSize **************************************
*
*   NAME
*       AHI_SampleFrameSize -- get the size of a sample frame (V3)
*
*   SYNOPSIS
*       size = AHI_SampleFrameSize( sampletype );
*       D0                          D0
*
*       ULONG AHI_SampleFrameSize( ULONG );
*
*   FUNCTION
*       Returns the size in bytes of a sample frame for a given sample type.
*
*   INPUTS
*       sampletype - The sample type to examine. See <devices/ahi.h> for
*           possible types.
*
*   RESULT
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*      <devices/ahi.h>
*
****************************************************************************
*
*/

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
