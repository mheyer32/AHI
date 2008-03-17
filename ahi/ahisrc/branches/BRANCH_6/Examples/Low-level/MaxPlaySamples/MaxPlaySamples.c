
#include <config.h>

#include <devices/ahi.h>
#include <proto/ahi.h>
#include <proto/exec.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "version.h"

const char version[] = "$VER: MaxPlaySamples " VERS "\n\r";

struct Library* AHIBase = NULL;

static int MaxPlaySamples(ULONG mode_id, ULONG mix_freq, ULONG player_freq);

int
main(int argc, char* argv[]) {
  int rc = RETURN_OK;

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <audio mode id> <mix_freq> <player_freq>\n", argv[0]);
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

	  rc = MaxPlaySamples(atol(argv[1]), atol(argv[2]), atol(argv[3]));

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

static int
MaxPlaySamples(ULONG mode_id, ULONG mix_freq, ULONG player_freq) {
  int rc = RETURN_OK;

  struct AHIAudioCtrl* actrl = AHI_AllocAudio(AHIA_AudioID,       mode_id,
					      AHIA_MixFreq,       mix_freq,
					      AHIA_PlayerFreq,    player_freq << 16,
					      AHIA_MinPlayerFreq, player_freq << 16,
					      AHIA_MaxPlayerFreq, player_freq << 16,
					      AHIA_Channels,      1,
					      AHIA_Sounds,        1,
					      TAG_DONE);

  if (actrl != NULL) {
    ULONG max_play_samples = 0;

    if (AHI_GetAudioAttrs(AHI_INVALID_ID, actrl,
			  AHIDB_MaxPlaySamples, &max_play_samples,
			  TAG_DONE)) {
      fprintf(stderr, 
	      "MaxPlaySamples for mode 0x%08lx (Fsample = %ld Hz, Fplayer = %ld Hz) is %ld\n",
	      mode_id, mix_freq, player_freq, max_play_samples);
    }
    else {
      fprintf(stderr, "Unable to get MaxPlaySamples.\n");
      rc = RETURN_ERROR;
    }
    AHI_FreeAudio(actrl);
  }
  else {
    fprintf(stderr, "Unable to allocate audio.\n");
    rc = RETURN_ERROR;
  }

  return rc;
}
