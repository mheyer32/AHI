
#include <devices/ahi.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/ahi.h>
#include <stdlib.h>

struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;

#define AHIVERSION 3

LONG __OSlibversion=37;

const static UBYTE version[]="$VER: AddAudioModes 1.1 "__AMIGADATE__"\n\r";

void cleanup(LONG rc)
{
  if(!AHIDevice)
    CloseDevice((struct IORequest *)AHIio);
  DeleteIORequest((struct IORequest *)AHIio);
  DeleteMsgPort(AHImp);
  exit(rc);
}

struct
{
  STRPTR *files;
  ULONG   quiet;
  ULONG   refresh;
  ULONG   remove;
} args = {NULL, FALSE, FALSE, FALSE};

LONG main(void)
{
  struct RDArgs *rdargs;
  ULONG  i=0,id,rc=RETURN_OK;

  if(AHImp=CreateMsgPort())
    if(AHIio=(struct AHIRequest *)CreateIORequest(AHImp,sizeof(struct AHIRequest))) {
      AHIio->ahir_Version=AHIVERSION;
      AHIDevice=OpenDevice(AHINAME,AHI_NO_UNIT,(struct IORequest *)AHIio,NULL);
      }

  if(AHIDevice) {
    Printf("Unable to open %s version %d\n",AHINAME,AHIVERSION);
    cleanup(RETURN_FAIL);
  }
  AHIBase=(struct Library *)AHIio->ahir_Std.io_Device;

  if(rdargs=ReadArgs("FILES/M,QUIET/S,REFRESH/S,REMOVE/S", (LONG *) &args, NULL))
  {
    if(args.refresh && !args.remove)
    {
      if(!AHI_LoadModeFile("DEVS:AudioModes") && !args.quiet)
      {
        PrintFault(IoErr(),"DEVS:AudioModes");
        rc=RETURN_ERROR;
      }
      
    }
    if(args.files && !args.remove)
    {
      while(args.files[i])
      {
        if(!AHI_LoadModeFile(args.files[i]) && !args.quiet)
        {
          PrintFault(IoErr(),args.files[i]);
          rc=RETURN_ERROR;
        }
        i++;
      }
    }
    if(args.remove)
    {
      if(args.files || args.refresh)
      {
        PutStr("The REMOVE switch cannot be used together with FILES or REFRESH.\n");
        rc=RETURN_FAIL;
      }
      else
      {
        while((id=AHI_NextAudioID(AHI_INVALID_ID)) != AHI_INVALID_ID)
          AHI_RemoveAudioMode(id);
      }
    }
    FreeArgs(rdargs);
  }
  cleanup(rc);
}
