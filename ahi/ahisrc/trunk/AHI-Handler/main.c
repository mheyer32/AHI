/* $Id$
 * $Log$
 * Revision 1.4  1997/01/24 23:20:47  lcs
 * Writing seem to work too...
 *
 * Revision 1.3  1997/01/23  19:55:50  lcs
 * Added AIFF and AIFC saving.
 *
 * Revision 1.2  1997/01/21  23:56:21  lcs
 * Reading seem to work okay now.
 *
 * Revision 1.1  1997/01/17  23:34:28  lcs
 * Initial revision
 *
 */

/*
 * This code is written using DICE, and is based on the DosHan example
 * source code that came with the compiler. Not all comments are mine,
 * by the way... 
 *
 * Done by Martin Blom 1997. Public Domain.
 */


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

#include "main.h"


/*
 *  Prototypes
 */

void ulong2extended (ULONG, extended *);
long AllocAudio(void);
void FreeAudio(void);
long InitHData(struct HandlerData *, char *);
void FreeHData(struct HandlerData *);
void returnpacket (struct DosPacket *);
void Initialize (void);
void UnInitialize (void);


/*
 *  Some macros
 */

#define min(a,b) ((a)<=(b)?(a):(b))
#define DOS_TRUE    -1
#define DOS_FALSE   0
#define BTOC(bptr)  ((void *)((long)(bptr) << 2))
#define CTOB(cptr)  ((BPTR)(((long)cptr) >> 2))


/*
 *  My debug stuff....
 */

#define HIT(x) {char *a=NULL; *a=x;}
void kprintf(char *, ...);


/*
 *  Global variables
 */

struct List        HanList;
struct DeviceNode *DevNode;
struct MsgPort    *PktPort;
int                AllocCnt;
BOOL               Running;

struct MsgPort    *AHImp     = NULL;
struct AHIRequest *AHIio     = NULL;
BYTE               AHIDevice = -1;
struct Library    *AHIBase;

struct AIFCHeader AIFCHeader = {
  ID_FORM, NULL, ID_AIFC,
  ID_FVER, sizeof(FormatVersionHeader), {
    AIFCVersion1
  },
  ID_COMM, sizeof(ExtCommonChunk), {
    0,
    0,
    0,
    {0},
    NO_COMPRESSION,
    sizeof("not compressed")-1,
    'n','o','t',' ','c','o','m','p','r','e','s','s','e','d'
  },
  ID_SSND, NULL, {0,0}
};

struct AIFFHeader AIFFHeader = {
  ID_FORM, NULL, ID_AIFF,
  ID_COMM, sizeof(CommonChunk),{
    0,
    0,
    0,
    {0}
  },
  ID_SSND, NULL, {0,0}
};


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
        ++base;

        {
          // Convert /'s to blanks

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
         *  the buffer, we wait until the other is filled, and switch
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
          data->readreq    = AllocVec(sizeof (struct AHIRequest), MEMF_PUBLIC);

          if((data->buffer1 == NULL)
          || (data->buffer2 == NULL)
          || (data->readreq    == NULL)) {
            packet->dp_Res2 = ERROR_NO_FREE_STORE;
            break;
          }

          CopyMem(AHIio, data->readreq, sizeof (struct AHIRequest));

          // Fill buffer 2
          // Note that io_Offset is always 0 the first time

          data->readreq->ahir_Std.io_Command  = CMD_READ;
          data->readreq->ahir_Std.io_Data     = data->buffer2;
          data->readreq->ahir_Std.io_Length   = data->buffersize;
          data->readreq->ahir_Std.io_Offset   = 0;
          data->readreq->ahir_Type            = data->type;
          data->readreq->ahir_Frequency       = data->freq;
          data->readreq->ahir_Volume          = data->vol;
          data->readreq->ahir_Position        = data->pos;
          SendIO((struct IORequest *) data->readreq);
  
          // Force buffer switch filling of the other buffer

          data->length = data->offset = 0;

          // Check if we should write a header first

          if(data->format == AIFF) {
            if(length < sizeof(struct AIFFHeader)) {
              packet->dp_Res1 = -1;
              break;
            }

            AIFFHeader.FORMsize = sizeof(AIFFHeader) + data->totallength - 8;
            AIFFHeader.COMMchunk.numChannels = data->channels;
            AIFFHeader.COMMchunk.numSampleFrames = 
                data->totallength / AHI_SampleFrameSize(data->type);
            AIFFHeader.COMMchunk.sampleSize = data->bits;
            ulong2extended(data->freq, &AIFFHeader.COMMchunk.sampleRate);
            AIFFHeader.SSNDsize =
                sizeof(SampledSoundHeader) + data->totallength;

            CopyMem(&AIFFHeader, dest, sizeof(struct AIFFHeader)); 
            dest              += sizeof(struct AIFFHeader);
            length            -= sizeof(struct AIFFHeader);
          }

          else if(data->format == AIFC) {
            if(length < sizeof(struct AIFCHeader)) {
              packet->dp_Res1 = -1;
              break;
            }

            AIFCHeader.FORMsize = sizeof(AIFCHeader) + data->totallength - 8;
            AIFCHeader.COMMchunk.numChannels = data->channels;
            AIFCHeader.COMMchunk.numSampleFrames = 
                data->totallength / AHI_SampleFrameSize(data->type);
            AIFCHeader.COMMchunk.sampleSize = data->bits;
            ulong2extended(data->freq, &AIFCHeader.COMMchunk.sampleRate);
            AIFCHeader.SSNDsize =
                sizeof(SampledSoundHeader) + data->totallength;

            CopyMem(&AIFCHeader, dest, sizeof(struct AIFCHeader)); 
            dest              += sizeof(struct AIFCHeader);
            length            -= sizeof(struct AIFCHeader);
          }
        }


        while(length > 0) {
          LONG thislength;
        
          if(data->offset >= data->length) {
            void *temp;

            temp          = data->buffer1;
            data->buffer1 = data->buffer2;
            data->buffer2 = temp;

            WaitIO((struct IORequest *) data->readreq);
            data->length = data->readreq->ahir_Std.io_Actual;
            data->offset = 0;

            if(data->readreq->ahir_Std.io_Error) {
              packet->dp_Res1 = -1;
              length = 0;
              break;
            }

            data->readreq->ahir_Std.io_Command = CMD_READ;
            data->readreq->ahir_Std.io_Data    = data->buffer2;
            data->readreq->ahir_Std.io_Length  = data->buffersize;
            data->readreq->ahir_Type           = data->type;
            data->readreq->ahir_Frequency      = data->freq;
            data->readreq->ahir_Volume         = data->vol;
            data->readreq->ahir_Position       = data->pos;
            SendIO((struct IORequest *) data->readreq);
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
        UBYTE *src    = (void *) packet->dp_Arg2;
        long   length, filled;


        kprintf("ACTION_WRITE: 0x%08lx, %ld\n", packet->dp_Arg2, packet->dp_Arg3);

        length = filled = min(data->totallength, packet->dp_Arg3);

        if(data->buffer1 == NULL) {

          data->buffer1 = AllocVec(data->buffersize, MEMF_PUBLIC);
          data->buffer2 = AllocVec(data->buffersize, MEMF_PUBLIC);

          if((data->buffer1 == NULL) || (data->buffer2 == NULL)) {
            packet->dp_Res2 = ERROR_NO_FREE_STORE;
            break;
          }

          data->offset = 0;
          data->length = (data->buffersize / AHI_SampleFrameSize(data->type))
                          * AHI_SampleFrameSize(data->type);
          kprintf("length: %ld\n", data->length);
        }


        while(length > 0) {
          LONG thislength;
        
          if(data->offset >= data->length) {
            void *temp;

            temp          = data->buffer1;
            data->buffer1 = data->buffer2;
            data->buffer2 = temp;


            temp            = data->writereq1;
            data->writereq1 = data->writereq2;
            data->writereq2 = temp;


            if(data->writereq1) {
  kprintf("Väntar på 0x%08lx..\n",data->writereq1);
              WaitIO((struct IORequest *) data->writereq1);

              if(data->writereq1->ahir_Std.io_Error) {
                packet->dp_Res1 = -1;
                length = 0;
                break;
              }
            }
            else {
              data->writereq1 = AllocVec(sizeof (struct AHIRequest), MEMF_PUBLIC);

              if(data->writereq1 == NULL) {
                packet->dp_Res1 = -1;
                length = 0;
                break;
              }
              CopyMem(AHIio, data->writereq1, sizeof (struct AHIRequest));
            }

            data->offset = 0;

  kprintf("Spelar 0x%08lx, länkad till 0x%08lx..\n", data->writereq1, data->writereq2);
            data->writereq1->ahir_Std.io_Message.mn_Node.ln_Pri = 0;
            data->writereq1->ahir_Std.io_Command = CMD_WRITE;
            data->writereq1->ahir_Std.io_Data    = data->buffer2;
            data->writereq1->ahir_Std.io_Length  = data->buffersize;
            data->writereq1->ahir_Std.io_Offset  = 0;
            data->writereq1->ahir_Type           = data->type;
            data->writereq1->ahir_Frequency      = data->freq;
            data->writereq1->ahir_Volume         = data->vol;
            data->writereq1->ahir_Position       = data->pos;
            data->writereq1->ahir_Link           = data->writereq2;
            SendIO((struct IORequest *) data->writereq1);
          } /* if */

          thislength = min(data->length - data->offset, length);
kprintf("Kopierar %ld bytes..\n", thislength);
          CopyMem(src, data->buffer1 + data->offset, thislength); 
          src               += thislength;
          length            -= thislength;
          data->offset      += thislength;
          data->totallength -= thislength;
        } /* while */

        packet->dp_Res1 = filled;
        break;
      }

      case ACTION_END:        /*  FHArg1                      Bool:TRUE   */
      {
        struct HandlerData *data = (struct HandlerData *) packet->dp_Arg1;

        kprintf("ACTION_END\n");
        if(data->readreq) {
          AbortIO((struct IORequest *) data->readreq);
          WaitIO((struct IORequest *) data->readreq);
        }
        if(data->writereq1) {
          AbortIO((struct IORequest *) data->writereq1);
          WaitIO((struct IORequest *) data->writereq1);
        }
        if(data->writereq2) {
          AbortIO((struct IORequest *) data->writereq2);
          WaitIO((struct IORequest *) data->writereq2);
        }

        FreeHData(data);
        FreeAudio();

        if(AllocCnt == 0)
          Running = FALSE;

        break;
      }

      default:
      {
        if(AllocCnt == 0)
          Running = FALSE;
        kprintf("Unknow packet!\n");
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
  UnInitialize();
  _exit (0);
}


/*
 *  This function translates an ULONG to Apples SANE Extended
 *  used in AIFF/AIFC files.
 */

void ulong2extended (ULONG in, extended *ex)
{
  ex->exponent=31+16383;
  ex->mantissa[1]=0;
  while(!(in & 0x80000000))
  {
    ex->exponent--;
    in<<=1;
  }
  ex->mantissa[0]=in;
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
    else {
      AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;
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

    if(rdargs2 != NULL) {
      kprintf("%ld bits, %ld channels, %ld Hz, %s, %ld%% volume, position %ld%%, "
              "%ld bytes total, %ld seconds, %ld bytes buffer\n",
          *args.bits, *args.channels, *args.rate, args.type, *args.volume, *args.position,
          *args.length, (args.seconds ? *args.seconds : 0), *args.buffersize);

      FreeArgs(rdargs2);    
    }
    else
      rc = ERROR_BAD_TEMPLATE;

    FreeDosObject(DOS_RDARGS, rdargs);
  }
  else
    rc = ERROR_OBJECT_WRONG_TYPE;


  if(rc == 0) {

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
  
    // Signed, unsigned, aiff, aifc...
  
    if(Stricmp("SIGNED", args.type) == 0) {
      data->format = RAW;
      kprintf("Signed\n");
    }
    else if(Stricmp("UNSIGNED", args.type) == 0) {
      data->type |= Sunsignedflag;
      data->format = RAW;
      kprintf("Unsigned\n");
    }
    else if(Stricmp("AIFF", args.type) == 0) {
      data->format = AIFF;
      kprintf("AIFF\n");
    }
    else if(Stricmp("AIFC", args.type) == 0) {
      data->format = AIFC;
      kprintf("AIFC\n");
    }
    else {
      rc = ERROR_OBJECT_WRONG_TYPE;
    }
  
    data->bits     = *args.bits;
    data->channels = *args.channels;
    data->freq     = *args.rate;
    data->vol      = *args.volume * 0x10000 / 100;
    data->pos      = *args.position * 0x8000 / 100 + 0x8000;
    if(args.seconds)
      data->totallength = *args.seconds * data->freq 
                          * AHI_SampleFrameSize(data->type);
    else
      data->totallength = (*args.length / AHI_SampleFrameSize(data->type))
                          * AHI_SampleFrameSize(data->type);
  
    switch(data->format) {
      case AIFF:
      case AIFC:
        data->totallength = (data->totallength + 1) & ~1;    // Make even
        break;
      default:
        break;
    }
  
    // User doesn't know about double buffering!
  
    data->buffersize = *args.buffersize >> 1;
  }

  return rc;
}


/*
 *   Deallocate the HandlerData structure
 */

void FreeHData(struct HandlerData *data)
{
  FreeVec(data->buffer1);
  FreeVec(data->buffer2);
  FreeVec(data->readreq);
  FreeVec(data->writereq1);
  FreeVec(data->writereq2);
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
/*
  dn->dn_Task = PktPort;
*/
  dn->dn_Task = NULL;

  packet->dp_Res1 = DOS_TRUE;
  packet->dp_Res2 = 0;
  returnpacket (packet);
}

void UnInitialize (void) {
  struct DeviceNode *dn = DevNode;

  dn->dn_Task = NULL;
  /* dn->dn_SegList = NULL; */
}
