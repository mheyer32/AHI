/* $Id$
* $Log$
* Revision 1.5  1997/01/05 13:38:01  lcs
* Fixed a bug (attaching a iorequest to a silent one) in NewWriter()
*
* Revision 1.4  1997/01/04 20:19:56  lcs
* Changed the AHI_DEBUG levels
* CMD_WRITE seem to work as supposed now
*
* Revision 1.3  1997/01/04 13:26:41  lcs
* Debugged CMD_WRITE
*
* Revision 1.2  1996/12/21 23:06:35  lcs
* "Finished" the code for CMD_WRITE
*
* Revision 1.1  1996/12/21 13:05:12  lcs
* Initial revision
*
*/

#include "ahi_def.h"
#include <dos/dos.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <math.h>

#ifndef _GENPROTO
#include "devcommands_protos.h"
#endif

#include "device_protos.h"
#include "devsupp_protos.h"

static void TermIO(struct AHIRequest *, struct AHIBase *);
void PerformIO(struct AHIRequest *, struct AHIBase *);
static void Devicequery(struct AHIRequest *, struct AHIBase *);
static void ReadCmd(struct AHIRequest *, struct AHIBase *);
static void WriteCmd(struct AHIRequest *, struct AHIBase *);

void FeedReaders(struct AHIDevUnit *,struct AHIBase *);
static void FillReadBuffer(struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);

static void NewWriter(struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);
static void AddWriter(struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);
static void PlayRequest(int, struct AHIRequest *, struct AHIDevUnit *, struct AHIBase *);
void RethinkPlayers( struct AHIDevUnit *, struct AHIBase *);
static void RemPlayers( struct List *, struct AHIDevUnit *, struct AHIBase *);
void UpdateSilentPlayers( struct AHIDevUnit *, struct AHIBase *);

// Should be moved to a separate file... IMHO.
struct Node *FindNode(struct List *, struct Node *);


/******************************************************************************
** DevBeginIO *****************************************************************
******************************************************************************/

// This function is called by the system each time exec.library/DoIO()
// is called.

__asm void DevBeginIO(
    register __a1 struct AHIRequest *ioreq,
    register __a6 struct AHIBase *AHIBase)
{
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("BeginIO(0x%08lx)\n", ioreq);
  }

  ioreq->ahir_Std.io_Message.mn_Node.ln_Type = NT_MESSAGE;

  switch(ioreq->ahir_Std.io_Command)
  {

// Immediate commands
    case NSCMD_DEVICEQUERY:
      PerformIO(ioreq,AHIBase);
      break;

// Queued commands
    case CMD_READ:
    case CMD_WRITE:
      ioreq->ahir_Std.io_Flags &= ~IOF_QUICK;
      PutMsg(&ioreq->ahir_Std.io_Unit->unit_MsgPort,&ioreq->ahir_Std.io_Message);
      break;

// Unknown commands
    default:
      ioreq->ahir_Std.io_Error = IOERR_NOCMD;
      TermIO(ioreq,AHIBase);
      break;
  }
}


/******************************************************************************
** AbortIO ********************************************************************
******************************************************************************/

// This function is called by the system each time exec.library/AbortIO()
// is called.

__asm ULONG DevAbortIO(
    register __a1 struct AHIRequest *ioreq,
    register __a6 struct AHIBase *AHIBase)
{
  ULONG rc = NULL;
  struct AHIDevUnit *iounit;
  
  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("AbortIO(0x%08lx)", ioreq);
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  
  ObtainSemaphore(&iounit->ListLock);
  if(ioreq->ahir_Std.io_Message.mn_Node.ln_Type != NT_REPLYMSG)
  {
    switch(ioreq->ahir_Std.io_Command)
    {

      case CMD_READ:
        if(FindNode((struct List *) &iounit->ReadList, (struct Node *) ioreq))
        {
          Remove((struct Node *) ioreq);
          ioreq->ahir_Std.io_Error = IOERR_ABORTED;
          TermIO(ioreq,AHIBase);
        }
        break;

      case CMD_WRITE:
      case AHICMD_WRITTEN:
        if(FindNode((struct List *) &iounit->PlayingList, (struct Node *) ioreq)
        || FindNode((struct List *) &iounit->SilentList, (struct Node *) ioreq)
        || FindNode((struct List *) &iounit->WaitingList, (struct Node *) ioreq))
        {
          struct AHIRequest *nextreq;

          while(ioreq)
          {
            Remove((struct Node *) ioreq);

            if(ioreq->ahir_Channel != NOCHANNEL)
            {
              iounit->Voices[ioreq->ahir_Channel].PlayingRequest = NULL;
              iounit->Voices[ioreq->ahir_Channel].QueuedRequest = NULL;
              iounit->Voices[ioreq->ahir_Channel].NextRequest = NULL;
              iounit->Voices[ioreq->ahir_Channel].NextOffset = MUTE;
              if(iounit->AudioCtrl)
              {
                AHI_SetSound(ioreq->ahir_Channel,AHI_NOSOUND,0,0,
                    iounit->AudioCtrl,AHISF_IMM);
              }
            }

            ioreq->ahir_Std.io_Command = CMD_WRITE;
            ioreq->ahir_Std.io_Error   = IOERR_ABORTED;
            nextreq = ioreq->ahir_Link;
            TermIO(ioreq,AHIBase);
            ioreq = nextreq;
          }
        }

      default:
        rc = IOERR_NOCMD;
        break;
    }
  }
  ReleaseSemaphore(&iounit->ListLock);

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n",rc);
  }

  return rc;
}


/******************************************************************************
** TermIO *********************************************************************
******************************************************************************/

// This functions returns an IO request back to the sender.

static void TermIO(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  ULONG error = ioreq->ahir_Std.io_Error;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("Terminating IO Request 0x%08lx", ioreq);
  }

  if( ! (ioreq->ahir_Std.io_Flags & IOF_QUICK))
      ReplyMsg(&ioreq->ahir_Std.io_Message);

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_LOW)
  {
    KPrintF("=>%ld\n", error);
  }
}


/******************************************************************************
** PerformIO ******************************************************************
******************************************************************************/

void PerformIO(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  ioreq->ahir_Std.io_Error = NULL;

  switch(ioreq->ahir_Std.io_Command)
  {
    case NSCMD_DEVICEQUERY:
      Devicequery(ioreq, AHIBase);
      break;
    case CMD_READ:
      ReadCmd(ioreq, AHIBase);
      break;
    case CMD_WRITE:
      WriteCmd(ioreq, AHIBase);
      break;
    default:
      ioreq->ahir_Std.io_Error = IOERR_NOCMD;
      TermIO(ioreq, AHIBase);
      break;
  }
}


/******************************************************************************
** Findnode *******************************************************************
******************************************************************************/

// Find a node in a list

struct Node *FindNode(struct List *list, struct Node *node)

{
  struct Node *currentnode;

  for(currentnode = list->lh_Head;
      currentnode->ln_Succ;
      currentnode = currentnode->ln_Succ)
  {
    if(currentnode == node)
    {
      return currentnode;
    }
  }
  return NULL;
}


/******************************************************************************
** Devicequery ****************************************************************
******************************************************************************/

static UWORD commandlist[] =
{
  NSCMD_DEVICEQUERY,
  CMD_READ,
  CMD_WRITE,
  NULL
};

static void Devicequery (struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct NSDeviceQueryResult *dqr;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("NSCMD_DEVICEQUERY\n");
  }

  dqr = ioreq->ahir_Std.io_Data;
  if(ioreq->ahir_Std.io_Length >= 16)
  {
    dqr->SizeAvailable = 16;
    dqr->DeviceType = NSDEVTYPE_UNKNOWN;
    dqr->DeviceSubType = 0;
    dqr->SupportedCommands = commandlist;
  }

  ioreq->ahir_Std.io_Actual = dqr->SizeAvailable;
  TermIO(ioreq, AHIBase);
}



/* All the following functions are called within the unit process context */


/******************************************************************************
** ReadCmd ********************************************************************
******************************************************************************/

static void ReadCmd(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;
  ULONG error,mixfreq = 0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_READ\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  /* Start recording if neccessary */
  if( ! iounit->IsRecording)
  {
    if( (! iounit->FullDuplex) && iounit->IsPlaying)
    {
      error = AHIE_UNKNOWN;   // FIXIT!
    }
    else
    {
      error = AHI_ControlAudio(iounit->AudioCtrl,
         AHIC_Record,TRUE,
         TAG_DONE);
    }

    if( ! error)
    {
      iounit->IsRecording = TRUE;
    }
  }

  if(iounit->IsRecording)
  {
    AHI_ControlAudio(iounit->AudioCtrl,
        AHIC_MixFreq_Query,&mixfreq,
        TAG_DONE);

    ioreq->ahir_Std.io_Actual = 0;

    /* Initialize ahir_Frequency for the assembler record routines */
    if(ioreq->ahir_Frequency && mixfreq)
      ioreq->ahir_Frequency = ((mixfreq << 15) / ioreq->ahir_Frequency) << 1;
    else
      ioreq->ahir_Frequency = 0x00010000;       // Fixed 1.0

    ObtainSemaphore(&iounit->ListLock);

    /* Add the request to the list of readers */
    AddTail((struct List *) &iounit->ReadList,(struct Node *) ioreq);

    /* Copy the current buffer contents */
    FillReadBuffer(ioreq, iounit, AHIBase);

    ReleaseSemaphore(&iounit->ListLock);
  }
  else
  {
    ioreq->ahir_Std.io_Error = error;
    TermIO(ioreq, AHIBase);
  }
}


/******************************************************************************
** WriteCmd *******************************************************************
******************************************************************************/

const static UWORD type2snd[] =
{
  SND8,             // AHIST_M8S  (0)
  SND16,            // AHIST_M16S (1)
  AHI_NOSOUND,      // AHIST_S8S  (2)
  AHI_NOSOUND,      // AHIST_S16S (3)
  AHI_NOSOUND,      // AHIST_M8U  (4)
  AHI_NOSOUND,
  AHI_NOSOUND,
  AHI_NOSOUND,
  AHI_NOSOUND,      // AHIST_M32S (8)
  AHI_NOSOUND,
  AHI_NOSOUND       // AHIST_S32S (10)
};

static void WriteCmd(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;
  ULONG error = 0;

  if(AHIBase->ahib_DebugLevel >= AHI_DEBUG_HIGH)
  {
    KPrintF("CMD_WRITE\n");
  }

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  /* Start playback if neccessary */
  if( ! iounit->IsPlaying)
  {
    if( (! iounit->FullDuplex) && iounit->IsRecording)
    {
      error = AHIE_UNKNOWN;   // FIXIT!
    }
    else
    {
      error = AHI_ControlAudio(iounit->AudioCtrl,
         AHIC_Play,TRUE,
         TAG_DONE);
    }

    if( ! error)
    {
      iounit->IsPlaying = TRUE;
    }
  }

  if(iounit->IsPlaying)     // (error == 0)
  {
    ioreq->ahir_Std.io_Actual = 0;

    if(ioreq->ahir_Frequency >= 262144)
    {
      error = AHIE_UNKNOWN;
    }

    switch(ioreq->ahir_Type)
    {
      case AHIST_M8S:
        break;
      case AHIST_M16S:

        // Address to sample offset and length in bytes to length in samples

        ioreq->ahir_Std.io_Data = (APTR) ((ULONG) ioreq->ahir_Std.io_Data >> 1);
        ioreq->ahir_Std.io_Length >>= 1;
        break;
      case AHIST_S8S:
      case AHIST_S16S:
      case AHIST_M8U:
      case AHIST_M32S:
      case AHIST_S32S:
      default:
        error = AHIE_BADSAMPLETYPE;
    }

    if(! error)
    {
      NewWriter(ioreq, iounit, AHIBase);
    }
  }

  if(error)
  {
    ioreq->ahir_Std.io_Error = error;
    TermIO(ioreq, AHIBase);
  }
}


/******************************************************************************
** FeedReaders ****************************************************************
******************************************************************************/

// This function is called by DevProc or ReadCmd to scan the list of waiting
// readers, and fill their buffers. When a buffer is full, the IORequest is
// terminated.

void FeedReaders(struct AHIDevUnit *iounit,struct AHIBase *AHIBase)
{
  struct AHIRequest *ioreq;

  ObtainSemaphore(&iounit->ListLock);
  for(ioreq = (struct AHIRequest *)iounit->ReadList.mlh_Head;
      ioreq->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq = (struct AHIRequest *)ioreq->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    FillReadBuffer(ioreq, iounit, AHIBase);
  }

  // Check if Reader-list is empty. If so, stop recording.

  if( ! iounit->ReadList.mlh_Head->mln_Succ )
  {
    AHI_ControlAudio(iounit->AudioCtrl,
        AHIC_Record,FALSE,
        TAG_DONE);
    iounit->IsRecording = FALSE;
  }
  ReleaseSemaphore(&iounit->ListLock);
}


/******************************************************************************
** FillReadBuffer *************************************************************
******************************************************************************/

// Handles a read request. Note that the request MUST be in a list, and the
// list must be semaphore locked!

static void FillReadBuffer(struct AHIRequest *ioreq, struct AHIDevUnit *iounit,
    struct AHIBase *AHIBase)
{
  ULONG length,length2;
  APTR  oldaddress;
  BOOL  remove;

  if(iounit->ValidRecord) // Make sure we have a valid source buffer
  {
    oldaddress = ioreq->ahir_Std.io_Data;

    length = ioreq->ahir_Std.io_Length - ioreq->ahir_Std.io_Actual;

    switch (ioreq->ahir_Type)
    {
      case AHIST_M8S:
        break;
      case AHIST_S8S:
        length >>= 1;
        break;
      case AHIST_M16S:
        length >>= 1;
        break;
      case AHIST_S16S:
        length >>= 2;
        break;
      case AHIST_M8U:
        break;
      case AHIST_M32S:
        length >>= 2;
        break;
      case AHIST_S32S:
        length >>= 3;
        break;
    }

    length2 = (iounit->RecordSize - ioreq->ahir_Std.io_Offset) >> 2;
    length2 = MultFixed(length2, (Fixed) ioreq->ahir_Frequency);

    if(length <= length2)
    {
      remove=TRUE;
    }
    else
    {
      length = length2;
      remove = FALSE;
    }

    switch (ioreq->ahir_Type)
    {
      case AHIST_M8S:
        RecM8S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_S8S:
        RecS8S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_M16S:
        RecM16S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_S16S:
        RecS16S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_M8U:
      {
        BYTE *p = ioreq->ahir_Std.io_Data;
        int i;

        RecM8S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        for(i = length; i; i--)
          *p ^= 0x80;
        break;
      }
      case AHIST_M32S:
        RecM32S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      case AHIST_S32S:
        RecS32S(length,ioreq->ahir_Frequency,
            iounit->RecordBuffer,
            &ioreq->ahir_Std.io_Offset,
            &ioreq->ahir_Std.io_Data);
        break;
      default:
        ioreq->ahir_Std.io_Error = AHIE_BADSAMPLETYPE;
        remove = TRUE;
        break;
    }

    ioreq->ahir_Std.io_Actual += ((ULONG) ioreq->ahir_Std.io_Data - (ULONG) oldaddress);

    if(remove)
    {
      Remove((struct Node *) ioreq);
      TermIO(ioreq, AHIBase);
    }
    else
    {
      ioreq->ahir_Std.io_Offset = 0;
    }
  }
  else
  {
    ioreq->ahir_Std.io_Offset = 0;
  }
}


/******************************************************************************
** NewWriter ******************************************************************
******************************************************************************/

// This function is called by WriteCmd when a new write request comes.

static void NewWriter(struct AHIRequest *ioreq, struct AHIDevUnit *iounit,
    struct AHIBase *AHIBase)
{
  int channel;
  BOOL delay = FALSE;

  iounit=(struct AHIDevUnit *)ioreq->ahir_Std.io_Unit;
  ObtainSemaphore(&iounit->ListLock);

  if(ioreq->ahir_Link)
  {
    // See if the linked request is playing, silent or waiting...

    if(FindNode((struct List *) &iounit->PlayingList,
        (struct Node *) ioreq->ahir_Link))
    {
      delay = TRUE;
    }
    else if(FindNode((struct List *) &iounit->SilentList,
        (struct Node *) ioreq->ahir_Link))
    {
      delay = TRUE;
    }
    else if(FindNode((struct List *) &iounit->WaitingList,
        (struct Node *) ioreq->ahir_Link))
    {
      delay = TRUE;
    }
  }

// NOTE: ahir_Link changes direction here. When the user set's it, she makes a new
// request point to an old. We let the old point to the next (that's more natural,
// anyway...) It the user tries to link more than one request to another, we fail.

  if(delay)
  {
    if( ! ioreq->ahir_Link->ahir_Link)
    {
      channel = ioreq->ahir_Link->ahir_Channel;
      ioreq->ahir_Channel = channel;

      ioreq->ahir_Link->ahir_Link = ioreq;
      ioreq->ahir_Link = NULL;
      Enqueue((struct List *) &iounit->WaitingList,(struct Node *) ioreq);

      if(channel != NOCHANNEL)
      {
        // Attach the request to the currently playing one

        iounit->Voices[channel].QueuedRequest = ioreq;
        iounit->Voices[channel].NextOffset  = PLAY;
        iounit->Voices[channel].NextRequest = NULL;
        AHI_Play(iounit->AudioCtrl,
            AHIP_BeginChannel,  channel,
            AHIP_LoopFreq,      ioreq->ahir_Frequency,
            AHIP_LoopVol,       ioreq->ahir_Volume,
            AHIP_LoopPan,       ioreq->ahir_Position,
            AHIP_LoopSound,     type2snd[ioreq->ahir_Type],
            AHIP_LoopOffset,    (ULONG) ioreq->ahir_Std.io_Data + ioreq->ahir_Std.io_Actual,
            AHIP_LoopLength,    ioreq->ahir_Std.io_Length - ioreq->ahir_Std.io_Actual,
            AHIP_EndChannel,    NULL,
            TAG_DONE);
      }
    }
    else
    { // She tried to add more than one requeste to unother
      ioreq->ahir_Std.io_Error = AHIE_UNKNOWN;
      TermIO(ioreq, AHIBase);
    }
  }
  else
  {
    ioreq->ahir_Link=NULL;
    AddWriter(ioreq, iounit, AHIBase);
  }

  ReleaseSemaphore(&iounit->ListLock);

}


/******************************************************************************
** AddWriter ******************************************************************
******************************************************************************/

// This function is called by NewWriter and RethinkPlayers. It adds an
// initialized request to either the playing or waiting list, and starts
// the sound it if possible

static void AddWriter(struct AHIRequest *ioreq, struct AHIDevUnit *iounit,
    struct AHIBase *AHIBase)
{
  int channel;

  // Search for a free channel, and use if found

  for(channel = 0; channel < iounit->Channels; channel++)
  {
    if(iounit->Voices[channel].NextOffset == FREE)
    {
      Enqueue((struct List *) &iounit->PlayingList,(struct Node *) ioreq);
      PlayRequest(channel, ioreq, iounit, AHIBase);
      break;
    }
  }


  if(channel == iounit->Channels)
  {
    struct AHIRequest *ioreq2;

    // No free channel found. Check if we can kick the last one out...
    // There is at least on node in the list, and the last one has lowest priority.

    ioreq2 = (struct AHIRequest *) iounit->PlayingList.mlh_TailPred; 
    if(ioreq->ahir_Std.io_Message.mn_Node.ln_Pri
        > ioreq2->ahir_Std.io_Message.mn_Node.ln_Pri)
    {
      // Let's steal his place!
      RemTail((struct List *) &iounit->PlayingList);
      channel = ioreq2->ahir_Channel;
      ioreq2->ahir_Channel = NOCHANNEL;
      Enqueue((struct List *) &iounit->SilentList,(struct Node *) ioreq2);

      Enqueue((struct List *) &iounit->PlayingList,(struct Node *) ioreq);
      PlayRequest(channel, ioreq, iounit, AHIBase);
    }
    else
    {
      // Let's be quiet for a while.

      ioreq->ahir_Channel = NOCHANNEL;
      Enqueue((struct List *) &iounit->SilentList,(struct Node *) ioreq);
    }
  }
}


/******************************************************************************
** PlayRequest ****************************************************************
******************************************************************************/

// This begins to play an AHIRequest (starting at sample io_Actual).

static void PlayRequest(int channel, struct AHIRequest *ioreq,
    struct AHIDevUnit *iounit, struct AHIBase *AHIBase)
{
  // Start the sound

  ioreq->ahir_Channel = channel;

  if(ioreq->ahir_Link)
  {
    struct Voice        *v = & iounit->Voices[channel];
    struct AHIRequest   *r = ioreq->ahir_Link;

    v->NextSound     = type2snd[r->ahir_Type];
    v->NextVolume    = r->ahir_Volume;
    v->NextPan       = r->ahir_Position;
    v->NextFrequency = r->ahir_Frequency;
    v->NextOffset    = (ULONG) r->ahir_Std.io_Data
                     + r->ahir_Std.io_Actual;
    v->NextLength    = r->ahir_Std.io_Length
                     - r->ahir_Std.io_Actual;
    v->NextRequest   = r;
  }
  else
  {
    iounit->Voices[channel].NextOffset  = PLAY;
    iounit->Voices[channel].NextRequest = NULL;
  }

  iounit->Voices[channel].PlayingRequest = NULL;
  iounit->Voices[channel].QueuedRequest = ioreq;
  AHI_Play(iounit->AudioCtrl,
      AHIP_BeginChannel,  channel,
      AHIP_Freq,          ioreq->ahir_Frequency,
      AHIP_Vol,           ioreq->ahir_Volume,
      AHIP_Pan,           ioreq->ahir_Position,
      AHIP_Sound,         type2snd[ioreq->ahir_Type],
      AHIP_Offset,        (ULONG) ioreq->ahir_Std.io_Data+ioreq->ahir_Std.io_Actual,
      AHIP_Length,        ioreq->ahir_Std.io_Length-ioreq->ahir_Std.io_Actual,
      AHIP_EndChannel,    NULL,
      TAG_DONE);
}


/******************************************************************************
** RethinkPlayers *************************************************************
******************************************************************************/

// When a playing sample has reached it's end, this function is called.
// It finds and terminates all finished requests, and moves their 'childs'
// from the waiting list.
// Then it tries to restart all silent sounds.

void RethinkPlayers(struct AHIDevUnit *iounit, struct AHIBase *AHIBase)
{
  struct MinList templist;
  struct AHIRequest *ioreq;

  NewList((struct List *) &templist);

  ObtainSemaphore(&iounit->ListLock);

  RemPlayers((struct List *) &iounit->PlayingList, iounit, AHIBase);
  RemPlayers((struct List *) &iounit->SilentList, iounit, AHIBase);

  // Move all silent requests to our temporary list

  while(ioreq = (struct AHIRequest *) RemHead((struct List *) &iounit->SilentList))
  {
    AddTail((struct List *) &templist, (struct Node *) ioreq);
  }

  // And add them back...
  while(ioreq = (struct AHIRequest *) RemHead((struct List *) &templist))
  {
    AddWriter(ioreq, iounit, AHIBase);
  }

  ReleaseSemaphore(&iounit->ListLock);
}


/******************************************************************************
** RemPlayers *****************************************************************
******************************************************************************/

// Removes all finished play requests from a list. The lists must be locked!

static void RemPlayers( struct List *list, struct AHIDevUnit *iounit,
    struct AHIBase *AHIBase)
{
  struct AHIRequest *ioreq, *node;

  node = (struct AHIRequest *) list->lh_Head;
  while(node->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    ioreq = node;
    node = (struct AHIRequest *) node->ahir_Std.io_Message.mn_Node.ln_Succ;

    if(ioreq->ahir_Std.io_Command == AHICMD_WRITTEN)
    {
      Remove((struct Node *) ioreq);

      if(ioreq->ahir_Link)
      {
        // Move the attached one to the list
        Remove((struct Node *) ioreq->ahir_Link);
        ioreq->ahir_Link->ahir_Channel = ioreq->ahir_Channel;
        Enqueue(list, (struct Node *) ioreq->ahir_Link);
        // We have to go through the whole procedure again, in case
        // the child is finished, too.
        node = (struct AHIRequest *) list->lh_Head;
      }

      ioreq->ahir_Std.io_Error = AHIE_OK;
      ioreq->ahir_Std.io_Command = CMD_WRITE;
      ioreq->ahir_Std.io_Actual = ioreq->ahir_Std.io_Length
                                * AHI_SampleFrameSize(ioreq->ahir_Type);
      TermIO(ioreq, AHIBase);
    }
  }
}

/******************************************************************************
** UpdateSilentPlayers ********************************************************
******************************************************************************/

// Updates the io_Actual field of all silent requests. The lists must be locked.
// This function is either called from the interrupt or DevProc.

void UpdateSilentPlayers( struct AHIDevUnit *iounit, struct AHIBase *AHIBase)
{
  struct AHIRequest *ioreq;

  for(ioreq = (struct AHIRequest *)iounit->SilentList.mlh_Head;
      ioreq->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq = (struct AHIRequest *)ioreq->ahir_Std.io_Message.mn_Node.ln_Succ)

  {
    // Update io_Actual
    ioreq->ahir_Std.io_Actual += ((ioreq->ahir_Frequency << 14) / PLAYERFREQ) >> 14;

    // Check if the whole sample has been "played"
    if(ioreq->ahir_Std.io_Actual >= ioreq->ahir_Std.io_Length)
    {
      // Mark request as finished
      ioreq->ahir_Std.io_Command = AHICMD_WRITTEN;

      // Make us call Rethinkplayers later
      Signal((struct Task *) iounit->Master, (1L << iounit->SampleSignal));
    }
  }
}
