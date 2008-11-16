
#include <config.h>

#include <devices/ahi.h>
#include <proto/ahi.h>
#include <proto/exec.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "version.h"

const char version[] = "$VER: BetterAudioID " VERS "\n\r";

struct Library* AHIBase = NULL;

ULONG BetterAudioID = AHI_DEFAULT_ID;

#define _LVOAHI_BestAudioIDA -114

typedef ULONG
BestAudioIDA_proto(struct TagItem* tags __asm("a1"),
		   struct Library* base __asm("a6"));

BestAudioIDA_proto MyBestAudioIDA;
BestAudioIDA_proto* OldBestAudioIDA;

int
main(int argc, char* argv[]) {
  int rc = RETURN_OK;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <audio mode id>\n", argv[0]);
    rc = RETURN_ERROR;
  }
  else {
    struct MsgPort* mp = CreateMsgPort();

    if (mp != NULL) {
      struct AHIRequest* io = (struct AHIRequest *)
	CreateIORequest(mp, sizeof(struct AHIRequest));

      if (io != NULL) {
	io->ahir_Version = 4;

	if (OpenDevice(AHINAME, AHI_NO_UNIT, (struct IORequest *) io, 0) == 0) {
	  AHIBase = (struct Library *) io->ahir_Std.io_Device;

	  BetterAudioID = atol(argv[1]);

	  Forbid();
	  OldBestAudioIDA = (BestAudioIDA_proto*) 
	    SetFunction(AHIBase, _LVOAHI_BestAudioIDA,
			(ULONG (*)(void)) MyBestAudioIDA );

	  Wait(SIGBREAKF_CTRL_C);

	  SetFunction(AHIBase, _LVOAHI_BestAudioIDA,
		      (ULONG (*)(void)) OldBestAudioIDA );
	  rc = 0;

	  Permit();

	  CloseDevice((struct IORequest *) io);
	}
	else {
	  fprintf(stderr, "Unable to open '" AHINAME "' version 4.\n");
	  rc = RETURN_FAIL;
	}
	
	DeleteIORequest((struct IORequest *) io);
      }
      else {
	fprintf(stderr, "Unable to create IO request.\n");
	rc = RETURN_FAIL;
      }

      DeleteMsgPort(mp);
    }
    else {
      fprintf(stderr, "Unable to create message port.\n");
      rc = RETURN_FAIL;
    }
  }
  
  return rc;
}

ULONG
MyBestAudioIDA(struct TagItem* tags __asm("a1"),
	       struct Library* base __asm("a6")) {
  return BetterAudioID;
}
