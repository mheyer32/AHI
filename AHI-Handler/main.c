/* $Id$
 * $Log$
 * Revision 1.2  1997/01/21 23:56:21  lcs
 * Reading seem to work okay now.
 *
 * Revision 1.1  1997/01/17  23:34:28  lcs
 * Initial revision
 *
 */

/*
** This code is written using DICE, just for testing, and is based on the
** DosHan example source code.
*/

#define min(a,b) ((a)<=(b)?(a):(b))

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <dos/rdargs.h>

#include <devices/ahi.h>
#include <proto/ahi.h>

#include <clib/exec_protos.h>
#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/utility_protos.h>


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define DOS_TRUE    -1
#define DOS_FALSE   0

#define HIT(x) {char *a=NULL; *a=x;}
void kprintf(char *, ...);

#define BTOC(bptr)  ((void *)((long)(bptr) << 2))
#define CTOB(cptr)  ((BPTR)(((long)cptr) >> 2))


struct HandlerData {
  UBYTE             *buffer1;        // Address of read buffer
  UBYTE             *buffer2;
  LONG               length;         // Offset to first invalid sample frame
  LONG               offset;         // Current pointer
  struct AHIRequest *req;
  ULONG              type;
  ULONG              freq;
  Fixed              vol;
  sposition          pos;
  LONG               totallength;     // Total number of bytes to play/record
  LONG               buffersize;      // Play/record buffer size
};

long AllocAudio(void);
void FreeAudio(void);
long InitHData(struct HandlerData *, char *);
void FreeHData(struct HandlerData *);

void returnpacket (struct DosPacket *);
void Initialize (void);
void UnInitialize (void);

struct List        HanList;
struct DeviceNode *DevNode;
struct MsgPort    *PktPort;
int                AllocCnt;
BOOL               Running;

struct MsgPort    *AHImp     = NULL;
struct AHIRequest *AHIio     = NULL;
BYTE               AHIDevice = -1;

/*
 *  Note that we use the _main entry point.  Also notice that we do not
 *  need to open any libraries.. they are openned for us via DICE's
 *  unique auto-library-open ability.
 */

void _main ()
{
  struct DosPacket *packet;
  struct Process *proc = (struct Process *) FindTask (NULL);


  PktPort = &proc->pr_MsgPort;
  NewList (&HanList);
  Initialize ();

  Running = TRUE;
  AllocCnt = 0;

  kprintf("Init\n");

  /*
   *        Main Loop
   */

  while(Running) {
    struct Message *msg;

    while ((msg = GetMsg (PktPort)) == NULL)
      Wait (1 << PktPort->mp_SigBit);
    packet = (struct DosPacket *) msg->mn_Node.ln_Name;

    /*
     *  default return value
     */

    packet->dp_Res1 = DOS_TRUE;
    packet->dp_Res2 = 0;

    /*
     *  switch on packet
     */

    switch (packet->dp_Type) {

      case ACTION_DIE:        /*  ??? */
      {
        kprintf("Die!\n");
        if(AllocCnt == 0)
          Running = FALSE;
        break;
      }

      case ACTION_FINDUPDATE: /*  FileHandle,Lock,Name        Bool        */
      case ACTION_FINDINPUT:  /*  FileHandle,Lock,Name        Bool        */
      case ACTION_FINDOUTPUT: /*  FileHandle,Lock,Name        Bool        */
      {
        struct FileHandle *fh = BTOC (packet->dp_Arg1);
        unsigned char *base = BTOC (packet->dp_Arg3);
        int len = *base;
        char buf[128];
        struct HandlerData *data;

        // Skip volume name and ':'
        while(*++base != ':')
          --len;
        base++;

        { // Convert /'s to blanks
          char *p = base;
          while(*++p)
            if(*p == '/')
              *p = ' ';
        }

        if (len >= sizeof (buf))
          len = sizeof (buf) - 1;

        strncpy (buf, base, len - 1);
        buf[len - 1] = '\n';
        buf[len] = 0;

        kprintf("ACTION_FIND#?: %s\n", (char *) buf);

        if(packet->dp_Res2 = AllocAudio()) {
          FreeAudio();
          break;
        }
        

        data = AllocVec(sizeof(struct HandlerData), MEMF_PUBLIC | MEMF_CLEAR);

        if(packet->dp_Res2 = InitHData(data, (char *) buf)) {
          FreeHData(data);
          data = NULL;
          FreeAudio();
        }
        else {
          fh->fh_Arg1 = (ULONG) data;
          fh->fh_Port = (struct MsgPort *) DOS_TRUE;
        }
        break;
      }

      case ACTION_READ:        /*  FHArg1,CPTRBuffer,Length    ActLength   */
      {

        /*
         *   Reading is straightforward except for handling EOF... We
         *  must guarentee a return value of 0 (no bytes left) before
         *  beginning to return EOFs (-1's).  If we return a negative
         *  number right off programs like COPY will assume a failure
         *  (if AUDIO: is the source) and delete the destination file.
         *
         *  The basic idea is to feed the packets from one buffer while
         *  recording asyncroniously to the other. When we have read
         *  the buffer, we wait until the other is filed, and switch
         *  buffer pointers.
         */

        struct HandlerData *data = (struct HandlerData *) packet->dp_Arg1;
        UBYTE *dest   = (void *) packet->dp_Arg2;
        LONG   length, filled;

        kprintf("ACTION_READ: 0x%08lx, %ld\n", packet->dp_Arg2, packet->dp_Arg3);

        length = filled = min(data->totallength, packet->dp_Arg3);

        if(length <= 0) {
          packet->dp_Res1 = length;
          data->totallength = -1;
          break;
        }

        if(data->buffer1 == NULL) {
          data->buffer1 = AllocVec(data->buffersize, MEMF_PUBLIC);
          data->buffer2 = AllocVec(data->buffersize, MEMF_PUBLIC);

          if((data->buffer1 == NULL) || (data->buffer2 == NULL)) {
            packet->dp_Res1 = -1;
            break;
          }

          // Fill buffer 2
          // Note that io_Offset is always 0 the first time
          data->req->ahir_Std.io_Command                 = CMD_READ;
          data->req->ahir_Std.io_Data                    = data->buffer2;
          data->req->ahir_Std.io_Length                  = data->buffersize;
          data->req->ahir_Std.io_Offset                  = 0;
          data->req->ahir_Type                           = data->type;
          data->req->ahir_Frequency                      = data->freq;
          data->req->ahir_Volume                         = data->vol;
          data->req->ahir_Position                       = data->pos;
          SendIO((struct IORequest *) data->req);
  
          // Force buffer switch filling of the other buffer
          data->length = data->offset = 0;
        }


        while(length > 0) {
          LONG thislength;
        
          if(data->offset >= data->length) {
            void *temp;

            temp          = data->buffer1;
            data->buffer1 = data->buffer2;
            data->buffer2 = temp;

            WaitIO((struct IORequest *) data->req);
            data->length = data->req->ahir_Std.io_Actual;
            data->offset = 0;

            if(data->req->ahir_Std.io_Error) {
              packet->dp_Res1 = -1;
              length = 0;
              break;
            }

            data->req->ahir_Std.io_Command                 = CMD_READ;
            data->req->ahir_Std.io_Data                    = data->buffer2;
            data->req->ahir_Std.io_Length                  = data->buffersize;
            data->req->ahir_Type                           = data->type;
            data->req->ahir_Frequency                      = data->freq;
            data->req->ahir_Volume                         = data->vol;
            data->req->ahir_Position                       = data->pos;
            SendIO((struct IORequest *) data->req);
          } /* if */

          thislength = min(data->length - data->offset, length);
          CopyMem(data->buffer1 + data->offset, dest, thislength); 
          dest              += thislength;
          length            -= thislength;
          data->offset      += thislength;
          data->totallength -= thislength;
        } /* while */

        packet->dp_Res1 = filled;
        break;

      } /* ACTION_READ */

      case ACTION_WRITE:        /*  FHArg1,CPTRBuffer,Length    ActLength   */
      {
        struct HandlerData *data  = (struct HandlerData *) packet->dp_Arg1;
        long bytes = packet->dp_Arg3;

        packet->dp_Res1 = -1;

        kprintf("ACTION_WRITE: 0x%08lx, %ld\n", packet->dp_Arg2, bytes);
#if 0
        if (xn = AllocMem (sizeof (XNode), MEMF_PUBLIC | MEMF_CLEAR)) {
          if (xn->xn_Buf = AllocMem (bytes, MEMF_PUBLIC)) {
            movmem ((char *) packet->dp_Arg2, xn->xn_Buf, bytes);
            xn->xn_Length = bytes;
            packet->dp_Res1 = bytes;
            AddTail (&xh->xh_List, &xn->xn_Node);
            xh->xh_Flags &= ~XHF_EOF;
          }
          else {
            FreeMem (xn, sizeof (XNode));
            packet->dp_Res2 = ERROR_NO_FREE_STORE;
          }
        }
        else {
          packet->dp_Res2 = ERROR_NO_FREE_STORE;
        }
#endif
        break;
      }

      case ACTION_END:        /*  FHArg1                      Bool:TRUE   */
      {
        struct HandlerData *data = (struct HandlerData *) packet->dp_Arg1;

        kprintf("ACTION_END\n");
        AbortIO((struct IORequest *) data->req);
        WaitIO((struct IORequest *) data->req);

        FreeHData(data);
        FreeAudio();
        break;
      }

      default:
      {
        packet->dp_Res2 = ERROR_ACTION_NOT_KNOWN;
        break;
      }

    } /* switch */

    if (packet) {
      if (packet->dp_Res2)
        packet->dp_Res1 = DOS_FALSE;
      returnpacket (packet);
    }

  } /* for */

  kprintf("Dying..!\n");
  UnInitialize ();
  _exit (0);
}


/*
 *  If the device isn't already open, open it now
 */

long AllocAudio(void) {
  long rc = 0;

  if(++AllocCnt == 1) {
    if(AHImp=CreateMsgPort()) {
      if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
        AHIio->ahir_Version=3;
        AHIDevice=OpenDevice(AHINAME,0,(struct IORequest *)AHIio,NULL);
      }
    }

    if(AHIDevice) {
      rc = ERROR_OBJECT_NOT_FOUND;
    }
  }
  return rc;
}


/*
 *  If we're the last user, close the device now
 */

void FreeAudio(void)
{
  if(--AllocCnt == 0) {
    if(!AHIDevice)
      CloseDevice((struct IORequest *)AHIio);
    AHIDevice = -1;
    DeleteIORequest((struct IORequest *)AHIio);
    AHIio = NULL;
    DeleteMsgPort(AHImp);
    AHImp = NULL;
  }
}


/*
 *  Initialize the HandlerData data structure, parse the arguments
 */

long InitHData(struct HandlerData *data, char *initstring)

// Returns 0 on success, else an error code

{
  struct RDArgs *rdargs, *rdargs2;
  ULONG bits = 8, channels = 1, rate = 8000, volume = 100, position = 0;
  LONG length = MAXINT, buffersize = 32768;
  STRPTR type = "SIGNED";
  BOOL rc = 0;

  struct {
    ULONG     *bits;
    ULONG     *channels;
    ULONG     *rate;
    STRPTR     type;
    Fixed     *volume;
    sposition *position;
    ULONG     *length;
    ULONG     *seconds;
    ULONG     *buffersize;
  } args = {
      &bits, &channels, &rate, type, &volume, &position, &length, NULL, &buffersize
    };

  if(data == NULL)
    return ERROR_NO_FREE_STORE;


  rdargs = (struct RDArgs *) AllocDosObjectTags(DOS_RDARGS, TAG_DONE);
  if(rdargs)
  {
    rdargs->RDA_Source.CS_Buffer = initstring;
    rdargs->RDA_Source.CS_Length = strlen(initstring);
    rdargs->RDA_Source.CS_CurChr = 0;
    rdargs->RDA_Flags |= RDAF_NOPROMPT;

    rdargs2 = ReadArgs(
      "B=BITS/K/N,C=CHANNELS/K/N,R=RATE/K/N,T=TYPE/K,V=VOLUME/K/N,P=POSITION/K/N,"
      "L=LENGTH/K/N,S=SECONDS/K/N,BUF=BUFFER/K/N",
      (LONG *) &args, rdargs);

    kprintf("%ld bits, %ld channels, %ld Hz, %s, %ld%% volume, position %ld%%, "
            "%ld bytes total, %ld seconds, %ld bytes buffer\n",
        *args.bits, *args.channels, *args.rate, args.type, *args.volume, *args.position,
        *args.length, (args.seconds ? *args.seconds : 0), *args.buffersize);

    if(data->req = AllocVec(sizeof (struct AHIRequest), MEMF_PUBLIC) ) {
      CopyMem(AHIio, data->req, sizeof (struct AHIRequest));
    }
    else
      rc = ERROR_NO_FREE_STORE;

    FreeArgs(rdargs2);    
    FreeDosObject(DOS_RDARGS, rdargs);
  }
  else
    rc = ERROR_OBJECT_WRONG_TYPE;

#define S8bitmode      0
#define S16bitmode     1
#define S32bitmode     8

#define Sstereoflag    2
#define Sunsignedflag  4

  // 8, 16 or 32 bit
  if(*args.bits <= 8)
    data->type = S8bitmode;
  else if(*args.bits <= 16)
    data->type = S16bitmode;
  else if(*args.bits <= 32)
    data->type = S32bitmode;
  else
    rc = ERROR_OBJECT_WRONG_TYPE;

  // Mono or stereo
  if(*args.channels == 2)
    data->type |= Sstereoflag;
  else if(*args.channels != 1)
    rc = ERROR_OBJECT_WRONG_TYPE;

  // Signed or unsigned
  if(Stricmp("UNSIGNED", args.type) == 0)
    data->type |= Sunsignedflag;
  else if(Stricmp("SIGNED", args.type) != 0)
    rc = ERROR_OBJECT_WRONG_TYPE;

  data->freq = *args.rate;
  data->vol  = *args.volume * 0x10000 / 100;
  data->pos  = *args.position * 0x8000 / 100 + 0x8000;

  if(data->vol > 0x10000) {
    data->vol = 0x10000;
  }
  if(data->pos > 0x10000) {
    data->pos = 0x10000;
  }

  if(args.seconds)
    data->totallength = *args.seconds * data->freq;
  else
    data->totallength = *args.length;

  // User doesn't know about double buffering!
  data->buffersize = *args.buffersize >> 1;
  return rc;
}


/*
 *   Deallocate the HandlerData structure
 */

void FreeHData(struct HandlerData *data)
{
  FreeVec(data->buffer1);
  FreeVec(data->buffer2);
  FreeVec(data->req);
  FreeVec(data);
}


/*
 *  PACKET ROUTINES.  Dos Packets are in a rather strange format as you
 *  can see by this and how the PACKET structure is extracted in the
 *  GetMsg() of the main routine.
 */

void
returnpacket (struct DosPacket *packet)
{
  struct Message *mess;
  struct MsgPort *replyPort;

  replyPort = packet->dp_Port;
  mess = packet->dp_Link;
  packet->dp_Port = PktPort;
  mess->mn_Node.ln_Name = (char *) packet;
  PutMsg (replyPort, mess);
}

/*
 *  During initialization DOS sends us a packet and sets our dn_SegList
 *  pointer.  If we set our dn_Task pointer than every Open's go to the
 *  same handler (this one).  If we set dn_Task to NULL, every Open()
 *  will create a NEW instance of this process via the seglist, meaning
 *  our process must be reentrant (i.e. -r option).
 *
 *  note: dn_Task points to the MESSAGE PORT portion of the process
 *  (or your own custom message port).
 *
 *  If we clear the SegList then we also force DOS to reload our process
 *  from disk, but we also need some way of then UnLoadSeg()ing it ourselves,
 *  which we CANNOT do from this process since it rips our code out from
 *  under us.
 */

void Initialize () {
  struct DeviceNode *dn;
  struct Process *proc = (struct Process *) FindTask (NULL);
  struct DosPacket *packet;

  /*
   *        Handle initial message.
   */
  
  struct Message *msg;

  WaitPort (PktPort);
  msg = GetMsg (PktPort);
  packet = (struct DosPacket *) msg->mn_Node.ln_Name;

  DevNode = dn = BTOC (packet->dp_Arg3);
  dn->dn_Task = PktPort;

  packet->dp_Res1 = DOS_TRUE;
  packet->dp_Res2 = 0;
  returnpacket (packet);
}

void UnInitialize (void) {
  struct DeviceNode *dn = DevNode;

  dn->dn_Task = NULL;
  /* dn->dn_SegList = NULL; */
}
