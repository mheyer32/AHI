
#include <config.h>

#include <exec/memory.h>
#include <datatypes/soundclass.h>
#include <devices/ahi.h>
#include <libraries/ahi_sub.h>

#include <proto/datatypes.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "DriverData.h"
#include "library.h"

#define dd ((struct FilesaveData*) AudioCtrl->ahiac_DriverData)


/******************************************************************************
** The record slave process ***************************************************
******************************************************************************/

#undef SysBase

void RecSlaveEntry(void)
{
  struct ExecBase*        SysBase = *SysBasePtr;
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct FilesaveBase*    FilesaveBase;

  ULONG   signals;
  BPTR    lock = NULL,cd=0,file = NULL;
  Object *o = NULL;
  BYTE   *samples = NULL;
  ULONG   length = NULL;
  ULONG   count = 0,offs = 0,i;

  struct AHIRecordMessage RecordMessage = 
  {
    AHIST_S16S,
    NULL,
    RECBUFFERSIZE
  };

  AudioCtrl    = (struct AHIAudioCtrlDrv*) FindTask( NULL )->tc_UserData;
  AHIsubBase   = (struct DriverBase*) dd->fs_AHIsubBase;
  FilesaveBase = (struct FilesaveBase*) AHIsubBase;

  RecordMessage.ahirm_Buffer = dd->fs_RecBuffer;

  if(!(lock = Lock(dd->fs_RecFileReq->fr_Drawer,ACCESS_READ)))
    goto quit;
  cd = CurrentDir(lock);

  if(DataTypesBase)
  {
    if (!(o = NewDTObject (dd->fs_RecFileReq->fr_File,
        DTA_GroupID,GID_SOUND,
        TAG_DONE)))
      goto quit;

    GetDTAttrs(o,
      SDTA_Sample,      (ULONG) &samples,
      SDTA_SampleLength,(ULONG) &length,
      TAG_DONE);
  }
  else // datatypes.library not open. Open the selected file as raw 8 bit signed instead.
  {
    if(!(file = Open(dd->fs_RecFileReq->fr_File,MODE_OLDFILE)))
      goto quit;
    Seek(file,0,OFFSET_END);
    length = Seek(file,0,OFFSET_BEGINNING);
    if(!(samples = AllocVec(length,MEMF_ANY)))
      goto quit;
    if(length != (ULONG) Read(file,samples,length))
      goto quit;
  }

  if(!samples || !length )
    goto quit;

  if((dd->fs_RecSlaveSignal = AllocSignal(-1)) == -1)
    goto quit;

// Everything set up. Tell Master we're alive and healthy.
    Signal((struct Task *)dd->fs_MasterTask,1L<<dd->fs_RecMasterSignal);

    for(;;)
    {
      signals = SetSignal(0L,0L);
      if(signals & (SIGBREAKF_CTRL_C | 1L<<dd->fs_RecSlaveSignal))
        break;

      for(;;)
      {
        if(count+RECBUFFERSIZE-offs < length)
        {
// End of sample will not be reached; just fill to the end of dd->fs_RecBuffer.
          for(i = RECBUFFERSIZE-offs;i>0;i--)
          {
            dd->fs_RecBuffer[(offs)<<1] = 
            dd->fs_RecBuffer[((offs++)<<1)+1] = 
            samples[count++]<<8;
          }
          offs = 0;
          break;
        }
        else
        {
// End of sample will be reached. Fill part of buffer, and iterate (== don't break).
          for(i = length-count;i>0;i--)
          {
            dd->fs_RecBuffer[(offs)<<1] = 
            dd->fs_RecBuffer[((offs++)<<1)+1] = 
            samples[count++]<<8;
          }
          count = 0;
        }

      }

      CallHookPkt(AudioCtrl->ahiac_SamplerFunc,AudioCtrl,&RecordMessage);
      Delay(50*RECBUFFERSIZE/AudioCtrl->ahiac_MixFreq);
    }

quit:
// Get rid of object
  if(DataTypesBase)
  {
    if(o)
      DisposeDTObject (o);
  }
  else // datatypes.library not open.
  {
    if(samples)
      FreeVec(samples);
    if(file)
      Close(file);
  }
  CurrentDir(cd);
  if(lock)
    UnLock(lock);

  Forbid();
  dd->fs_RecSlaveTask = NULL;
  FreeSignal(dd->fs_RecSlaveSignal);
  dd->fs_RecSlaveSignal = -1;
  // Tell the Master we're dying
  Signal((struct Task *)dd->fs_MasterTask,1L<<dd->fs_RecMasterSignal);
  // Multitaking will resume when we are dead.
}
