/* $Id$
* $Log$
* Revision 1.14  1997/03/13 00:19:43  lcs
* Up to 4 device units are now available.
*
* Revision 1.13  1997/02/18 22:26:49  lcs
* Fixed a bug in CMD_READ?
*
* Revision 1.12  1997/02/12 15:43:30  lcs
* Added autodocs for CMD_START and CMD_STOP
*
* Revision 1.11  1997/02/12 15:32:45  lcs
* Moved each autodoc header to the file where the function is
*
* Revision 1.10  1997/02/01 19:44:18  lcs
* Added stereo samples
*
* Revision 1.9  1997/01/31 20:22:24  lcs
* Enabled stereo samples
*
* Revision 1.8  1997/01/30 19:51:20  lcs
* Removed code for unsigned samples
*
* Revision 1.7  1997/01/29 13:44:33  lcs
* Fixed a race condition in PlayRequest()
*
* Revision 1.6  1997/01/15 14:59:50  lcs
* Added CMD_FLUSH, CMD_START, CMD_STOP and SMD_RESET
*
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

//#define DEBUG

#include "ahi_def.h"
#include <dos/dos.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/memory.h>
#include <proto/exec.h>

#include <math.h>

#ifndef  noprotos
#ifndef _GENPROTO
#include "devcommands_protos.h"
#endif
#endif

#include "device_protos.h"
#include "devsupp_protos.h"

static void TermIO(struct AHIRequest *, struct AHIBase *);
void PerformIO(struct AHIRequest *, struct AHIBase *);
static void Devicequery(struct AHIRequest *, struct AHIBase *);
static void ResetCmd(struct AHIRequest *, struct AHIBase *);
static void ReadCmd(struct AHIRequest *, struct AHIBase *);
static void WriteCmd(struct AHIRequest *, struct AHIBase *);
static void StopCmd(struct AHIRequest *, struct AHIBase *);
static void StartCmd(struct AHIRequest *, struct AHIBase *);
static void FlushCmd(struct AHIRequest *, struct AHIBase *);

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
    case CMD_STOP:
    case CMD_FLUSH:
      PerformIO(ioreq,AHIBase);
      break;

// Queued commands
    case CMD_RESET:
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

            if(GetExtras(ioreq)->Channel != NOCHANNEL)
            {
              iounit->Voices[GetExtras(ioreq)->Channel].PlayingRequest = NULL;
              iounit->Voices[GetExtras(ioreq)->Channel].QueuedRequest = NULL;
              iounit->Voices[GetExtras(ioreq)->Channel].NextRequest = NULL;
              iounit->Voices[GetExtras(ioreq)->Channel].NextOffset = MUTE;
              if(iounit->AudioCtrl)
              {
                AHI_SetSound(GetExtras(ioreq)->Channel,AHI_NOSOUND,0,0,
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

  if(ioreq->ahir_Extras != NULL)
  {
    FreeVec((APTR *)ioreq->ahir_Extras);
    ioreq->ahir_Extras = NULL;
  }

  if( ! (ioreq->ahir_Std.io_Flags & IOF_QUICK))
  {
      ReplyMsg(&ioreq->ahir_Std.io_Message);
  }

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
  ioreq->ahir_Std.io_Error = 0;

  // Just to make sure TermIO won't free a bad address
  ioreq->ahir_Extras = NULL;

  switch(ioreq->ahir_Std.io_Command)
  {
    case NSCMD_DEVICEQUERY:
      Devicequery(ioreq, AHIBase);
      break;
    case CMD_RESET:
      ResetCmd(ioreq, AHIBase);
      break;
    case CMD_READ:
      ReadCmd(ioreq, AHIBase);
      break;
    case CMD_WRITE:
      if(iounit->StopCnt)
      {
        AddTail((struct List *) &iounit->RequestQueue,(struct Node *) ioreq);
      }
      else
      {
        WriteCmd(ioreq, AHIBase);
      }
      break;
    case CMD_STOP:
      StopCmd(ioreq, AHIBase);
      break;
    case CMD_START:
      StartCmd(ioreq, AHIBase);
      break;
    case CMD_FLUSH:
      FlushCmd(ioreq, AHIBase);
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

/****** ahi.device/NSCMD_DEVICEQUERY  ***************************************
*
*   NAME
*       NSCMD_DEVICEQUERY -- Query the device for its capabilities (V3)
*
*   FUNCTION
*       Fills an initialized NSDeviceQueryResult structure with
*       information about the device.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      NSCMD_DEVICEQUERY
*       io_Data         Pointer to the NSDeviceQueryResult structure,
*                       initialized as follows:
*                           DevQueryFormat - Set to 0
*                           SizeAvailable  - Must be cleared.
*                       It is probably good manners to clear all other
*                       fields as well.
*       io_Length       Size of the NSDeviceQueryResult structure.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, the value in
*                       NSDeviceQueryResult.SizeAvailable.
*
*       The NSDeviceQueryResult structure now contains valid information.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static UWORD commandlist[] =
{
  NSCMD_DEVICEQUERY,
  CMD_RESET,
  CMD_READ,
  CMD_WRITE,
  CMD_FLUSH,
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


/******************************************************************************
** StopCmd ********************************************************************
******************************************************************************/

/****** ahi.device/CMD_STOP ************************************************
*
*   NAME
*       CMD_STOP -- stop device processing (like ^S) (V3)
*
*   FUNCTION
*       Stops all CMD_WRITE processing. All writes will be queued, and
*       are not processed until CMD_START. This is useful for syncronizing
*       two or more CMD_WRITE's.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_STOP
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of requests actually
*                       flushed.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command affects ALL writes, even those sent by other
*       applications. Make sure the code between CMD_STOP and CMD_START
*       runs as fast as possible!
*
*   BUGS
*
*   SEE ALSO
*       CMD_START, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void StopCmd(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  iounit->StopCnt++;
  TermIO(ioreq,AHIBase);
}


/******************************************************************************
** FlushCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_FLUSH ************************************************
*
*   NAME
*       CMD_FLUSH -- Cancel all I/O requests (V3)
*
*   FUNCTION
*       Aborts ALL current requestes, both active and waiting, even
*       other programs requests!
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_FLUSH
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of requests actually
*                       flushed.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command should only be used in very rare cases, like AHI
*       system utilities. Never use this command in an application.
*
*   BUGS
*
*   SEE ALSO
*       CMD_RESET, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void FlushCmd(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;
  struct AHIRequest *ior;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  ioreq->ahir_Std.io_Actual = 0;

  // Abort all current IO-requests
  while(ior = (struct AHIRequest *) iounit->ReadList.mlh_Head)
  {
    DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  while(ior = (struct AHIRequest *) iounit->PlayingList.mlh_Head)
  {
    DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  while(ior = (struct AHIRequest *) iounit->SilentList.mlh_Head)
  {
    DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  while(ior = (struct AHIRequest *) iounit->WaitingList.mlh_Head)
  {
    DevAbortIO(ior, AHIBase);
    ioreq->ahir_Std.io_Actual++;
  }
  TermIO(ioreq,AHIBase);
}



/* All the following functions are called within the unit process context */


/******************************************************************************
** ResetCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_RESET ************************************************
*
*   NAME
*       CMD_RESET -- Restore device to a known state (V3)
*
*   FUNCTION
*       Aborts all current requestes, even other programs requests
*       (CMD_FLUSH), rereads the configuration file and resets the hardware
*       to its initial state
*       
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_RESET
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*       This command should only be used in very rare cases, like AHI
*       system utilities. Never use this command in an application.
*
*   BUGS
*
*   SEE ALSO
*       CMD_FLUSH, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void ResetCmd(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;

  // Remove all requests (beware, invalid IORequest to FlushCmd!)
  FlushCmd(ioreq, AHIBase);

  // Reset the hardware
  ReadConfig(iounit, AHIBase);
  FreeHardware(iounit, AHIBase);
  AllocHardware(iounit, AHIBase);
  TermIO(ioreq,AHIBase);
}


/******************************************************************************
** ReadCmd ********************************************************************
******************************************************************************/

/****** ahi.device/CMD_READ *************************************************
*
*   NAME
*       CMD_READ -- Read raw samples from audio input (V3)
*
*   FUNCTION
*       Reads samples from the users prefered input to memory. The sample
*       format and frequency will be converted on the fly. 
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_READ
*       io_Data         Pointer to the buffer where the data should be put.
*       io_Length       Number of bytes to read, must be a multiple of the
*                       sample frame size (see ahir_Type).
*       io_Offset       Set to 0 when you use for the first time or after
*                       a delay.
*       ahir_Type       The desired sample format, see <ahi/devices.h>.
*       ahir_Frequency  The desired sample frequency in Hertz.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of bytes actually
*                       transferred.
*       io_Offset       Updated to be used as input next time.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

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
#ifdef DEBUG
    KPrintF("Copy old!\n");
#endif
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

/****** ahi.device/CMD_WRITE ************************************************
*
*   NAME
*       CMD_WRITE -- Write raw samples to audio output (V3)
*
*   FUNCTION
*       Plays the samples to the users prefered audio output.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_WRITE
*       io_Data         Pointer to the buffer of samples to be played.
*       io_Length       Number of bytes to play, must be a multiple of the
*                       sample frame size (see ahir_Type).
*       io_Offset       Must be 0.
*       ahir_Type       The desired sample format, see <ahi/devices.h>.
*       ahir_Frequency  The desired sample frequency in Hertz.
*       ahir_Volume     The desired volume. The range is 0 to 0x10000, where
*                       0 means muted and 0x10000 (== 1.0) means full volume.
*       ahir_Position   Defines the stereo balance. 0 is far left, 0x8000 is
*                       center and 0x10000 is far right.
*       ahir_Link       If non-zero, pointer to a previously sent AHIRequest
*                       which this AHIRequest will be linked to. This
*                       request will be delayed until the old one is
*                       finished (used for double buffering). Must be set
*                       to NULL if not used.
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of bytes actually
*                       played.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*       io_Data must be an even multiple of the sample frame size.
*       32 bit samples are not allowed yet.
*
*   SEE ALSO
*       <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

const static UWORD type2snd[] =
{
  SND8,             // AHIST_M8S  (0)
  SND16,            // AHIST_M16S (1)
  SND8S,            // AHIST_S8S  (2)
  SND16S,           // AHIST_S16S (3)
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

#ifdef DEBUG
    KPrintF("Not playing\n");
#endif

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

#ifdef DEBUG
    KPrintF("Playing\n");
#endif

    if( ! error)
    {
      iounit->IsPlaying = TRUE;
    }
  }

  ioreq->ahir_Extras = (ULONG) AllocVec(sizeof(struct Extras), MEMF_PUBLIC|MEMF_CLEAR);

  if(ioreq->ahir_Extras == NULL)
  {
    error = AHIE_NOMEM;
  }

  if(iounit->IsPlaying && !error)
  {
    ioreq->ahir_Std.io_Actual = 0;

#ifdef DEBUG
    KPrintF("Testing freq\n");
#endif

    // Address to sample offset and length in bytes to length in samples

    ioreq->ahir_Std.io_Data = (APTR) ((ULONG) ioreq->ahir_Std.io_Data
                              / AHI_SampleFrameSize(ioreq->ahir_Type));
    ioreq->ahir_Std.io_Length /= AHI_SampleFrameSize(ioreq->ahir_Type);

    switch(ioreq->ahir_Type)
    {
      case AHIST_M8S:
      case AHIST_S8S:
      case AHIST_M16S:
      case AHIST_S16S:
        break;
      case AHIST_M32S:
      case AHIST_S32S:
      default:
        error = AHIE_BADSAMPLETYPE;
    }

#ifdef DEBUG
    KPrintF("Error code is now: %ld\n", error);
#endif

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
** StartCmd *******************************************************************
******************************************************************************/

/****** ahi.device/CMD_START ************************************************
*
*   NAME
*       CMD_START -- start device processing (like ^Q) (V3)
*
*   FUNCTION
*       All CMD_WRITE's that has been sent to the device since CMD_STOP
*       will be started at once, syncronized.
*
*   IO REQUEST INPUT
*       io_Device       Preset by the call to OpenDevice().
*       io_Unit         Preset by the call to OpenDevice().
*       io_Command      CMD_START
*
*   IO REQUEST RESULT
*       io_Error        0 for success, or an error code as defined in
*                       <ahi/devices.h> and <exec/errors.h>.
*       io_Actual       If io_Error is 0, number of requests actually
*                       flushed.
*
*       The other fields, except io_Device, io_Unit and io_Command, are
*       trashed.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       CMD_STOP, <ahi/devices.h>, <exec/errors.h>
*
****************************************************************************
*
*/

static void StartCmd(struct AHIRequest *ioreq, struct AHIBase *AHIBase)
{
  struct AHIDevUnit *iounit;
  struct AHIPrivAudioCtrl *audioctrl;
  struct Library *AHIsubBase;

  iounit = (struct AHIDevUnit *) ioreq->ahir_Std.io_Unit;
  audioctrl = (struct AHIPrivAudioCtrl *) iounit->AudioCtrl;

  if(iounit->StopCnt)
  {
    iounit->StopCnt--;
    if(AHIsubBase = audioctrl->ahiac_SubLib)
    {
      if(iounit->StopCnt == 0)
      {
        struct AHIRequest *ior;

        AHIsub_Disable((struct AHIAudioCtrlDrv *) audioctrl);

        while(ior = (struct AHIRequest *) RemHead(
            (struct List *) &iounit->RequestQueue))
        {
          WriteCmd(ior, AHIBase);
        }

        AHIsub_Enable((struct AHIAudioCtrlDrv *) audioctrl);
      }
    }
  }
  else
  {
    ioreq->ahir_Std.io_Error = AHIE_UNKNOWN;
  }
  TermIO(ioreq,AHIBase);

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

#ifdef DEBUG
  KPrintF("FillReaders\n");
#endif

  ObtainSemaphore(&iounit->ListLock);
  for(ioreq = (struct AHIRequest *)iounit->ReadList.mlh_Head;
      ioreq->ahir_Std.io_Message.mn_Node.ln_Succ;
      ioreq = (struct AHIRequest *)ioreq->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    FillReadBuffer(ioreq, iounit, AHIBase);
  }

  // Check if Reader-list is empty. If so, stop recording (after a small delay).

  if( ! iounit->ReadList.mlh_Head->mln_Succ )
  {
#ifdef DEBUG
    KPrintF("Empty list\n");
#endif
    if(--iounit->RecordOffDelay == 0)
    {
#ifdef DEBUG
      KPrintF("Removing\n");
#endif
      AHI_ControlAudio(iounit->AudioCtrl,
          AHIC_Record,FALSE,
          TAG_DONE);
      iounit->IsRecording = FALSE;
    }
  }
  else
  {
    iounit->RecordOffDelay = 2;
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

#ifdef DEBUG
  KPrintF("FillReadBuffer\n");
#endif
  if(iounit->ValidRecord) // Make sure we have a valid source buffer
  {
    oldaddress = ioreq->ahir_Std.io_Data;

    length = (ioreq->ahir_Std.io_Length - ioreq->ahir_Std.io_Actual)
             / AHI_SampleFrameSize(ioreq->ahir_Type);

    length2 = (iounit->RecordSize - ioreq->ahir_Std.io_Offset)
              / AHI_SampleFrameSize(AHIST_S16S);
#ifdef DEBUG
    KPrintF("Samples left in buffer: %ld\n", length2);
#endif
    length2 = MultFixed(length2, (Fixed) ioreq->ahir_Frequency);

#ifdef DEBUG
    KPrintF("Left to store: %ld  Left to read: %ld\n",length, length2);
#endif


    if(length <= length2)
    {
      remove=TRUE;
    }
    else
    {
      length = length2;
      remove = FALSE;
    }

#ifdef DEBUG
    KPrintF("Copying %ld bytes from 0x%08lx + 0x%08lx to 0x%08lx, add 0x%08lx\n",
      length, iounit->RecordBuffer, ioreq->ahir_Std.io_Offset,
      ioreq->ahir_Std.io_Data, ioreq->ahir_Frequency);
#endif

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
#ifdef DEBUG
      KPrintF("Finished.\n");
#endif
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

#ifdef DEBUG
  KPrintF("New writer\n");
#endif

  iounit=(struct AHIDevUnit *)ioreq->ahir_Std.io_Unit;
  ObtainSemaphore(&iounit->ListLock);

#ifdef DEBUG
  KPrintF("Got lock\n");
#endif

  if(ioreq->ahir_Link)
  {

#ifdef DEBUG
    KPrintF("Linked\n");
#endif

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

#ifdef DEBUG
    KPrintF("Delayed\n");
#endif

    if( ! ioreq->ahir_Link->ahir_Link)
    {
      channel = GetExtras(ioreq->ahir_Link)->Channel;
      GetExtras(ioreq)->Channel = channel;

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

#ifdef DEBUG
  KPrintF("Addwriter\n");
#endif

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
      channel = GetExtras(ioreq2)->Channel;
      GetExtras(ioreq2)->Channel = NOCHANNEL;
      Enqueue((struct List *) &iounit->SilentList,(struct Node *) ioreq2);

      Enqueue((struct List *) &iounit->PlayingList,(struct Node *) ioreq);
      PlayRequest(channel, ioreq, iounit, AHIBase);
    }
    else
    {
      // Let's be quiet for a while.

      GetExtras(ioreq)->Channel = NOCHANNEL;
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

#ifdef DEBUG
  KPrintF("PlayRequest\n");
#endif

  // Start the sound

  GetExtras(ioreq)->Channel = channel;

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

#ifdef DEBUG
  KPrintF("Starting it\n");
#endif

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

  // This is a workaround for a race condition.
  // The problem can occur if a delayed request follows immediately after
  // this one, before the sample interrupt routine has been called, and
  // overwrites QueuedRequest. The result is that this sound is never
  // marked as finished, and the application will wait forever on the
  // IO Request. Quite ugly, no?

  while(iounit->Voices[channel].PlayingRequest == NULL);
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

#ifdef DEBUG
  KPrintF("RethinkPlayers\n");
#endif

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

#ifdef DEBUG
  KPrintF("RemPlayers\n");
#endif

  node = (struct AHIRequest *) list->lh_Head;
  while(node->ahir_Std.io_Message.mn_Node.ln_Succ)
  {
    ioreq = node;
    node = (struct AHIRequest *) node->ahir_Std.io_Message.mn_Node.ln_Succ;

    if(ioreq->ahir_Std.io_Command == AHICMD_WRITTEN)
    {

#ifdef DEBUG
      KPrintF("removing %08lx\n", ioreq);
#endif

      Remove((struct Node *) ioreq);

      if(ioreq->ahir_Link)
      {
        // Move the attached one to the list
        Remove((struct Node *) ioreq->ahir_Link);
        GetExtras(ioreq->ahir_Link)->Channel = GetExtras(ioreq)->Channel;
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
