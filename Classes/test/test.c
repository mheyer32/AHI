
#include <config.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/lfo.h>
#include <classes/ahi/processor/gain.h>
#include <classes/ahi/processor/tick.h>
#include <devices/ahi.h>
#include <intuition/classes.h>
#include <intuition/icclass.h>

#include <math.h>
#include <stdio.h>
#include <sys/time.h>

#include <clib/alib_protos.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>

STRPTR ClassName = "test";
#include "../Common/util.h"

struct Library* AHIBase;

static int buffer_type = AHIST_F2;
static int buffer_len  = 48000/100;

static struct ClassLibrary* libs[32];
static int lib_count = 0;

#define AHI_NewObject(name, ...) ({			\
  ULONG _tags[] = { __VA_ARGS__ };			\
  AHI_NewObjectA((name), (struct TagItem*) _tags);	\
})

static Object*
AHI_NewObjectA(STRPTR name, struct TagItem* tags) {
  struct ClassLibrary* cl = NULL;
  Object* o;
  int i;

  for (i = 0; i < lib_count; ++i) {
    if (strcmp(name, libs[i]->cl_Lib.lib_Node.ln_Name) == 0) {
      cl = libs[i];
      break;
    }
  }

  if (i == lib_count) {
    char path[32];

    snprintf(path, sizeof (path), "AHI/%s", name);

    libs[i] = (struct ClassLibrary*) OpenLibrary(path,0);

    if (libs[i] != NULL) {
      ++lib_count;
    }
  }

  return NewObjectA(libs[i]->cl_Class, NULL, tags);
}

static void
CleanUpAHI(void) {
  int i;

  for (i = 0; i < lib_count; ++i) {
    CloseLibrary((struct Library*) libs[i]);
  }
}


static inline void
check_err(Object* o) {
  ULONG  error = 0;

  GetAttr(AHIA_Error, o, &error);
  
  if (error) {
    STRPTR error_msg = NULL;
    
    GetAttr(AHIA_ErrorMessage, o, (ULONG*) &error_msg);
    Printf("Error %08lx: %s\n", error, (ULONG) error_msg);

    SetAttrs(o, AHIA_Error, AHIE_OK, TAG_DONE);
  }
}

#if 0
    float* buffer = NULL;
    if (GetAttr(AHIA_Buffer_Data, b, &buffer)) {
      int i;
	
      for (i = 0; i < buffer_len; ++i) {
	buffer[2 * i + 0] = +1.0 * sin((double) i / buffer_len * 2 * M_PI);
	buffer[2 * i + 1] = -0.5 * sin((double) i / buffer_len * 2 * M_PI);
      }
      for (i = 0; i < buffer_len; ++i) {
	Printf( "%f, %f\n", buffer[2 * i + 0], buffer[2 * i + 1]);
      }
    }
#endif

static int
execute(Object* proc, int seconds) {
  int rc = 0;
  static struct AHIRequest ahirequest;

  ahirequest.ahir_Std.io_Message.mn_Length = sizeof (ahirequest);
  ahirequest.ahir_Version = 5;

  if (OpenDevice( AHINAME, AHI_NO_UNIT, (struct IORequest*) &ahirequest, 0) == 0) {
    AHIBase = (struct Library*) ahirequest.ahir_Std.io_Device;

    {
      struct timeval tv_start;
      struct timeval tv_end;
      UQUAD l_start;
      UQUAD l_end;
      ULONG msec;
      ULONG hz;
      int i;
      int j;
      Object* b = NULL;

      GetAttr(AHIA_Processor_Buffer, proc, (ULONG*) &b);

      Printf("Start!\n");
      gettimeofday(&tv_start, NULL);

      SetAttrs(proc, AHIA_Processor_Busy, TRUE, TAG_DONE);
      check_err(proc);

      for (i = 0; i < 1000; ++i) {
	// Set Timestamp
	SetAttrs(b, AHIA_Buffer_TimestampLow, j * buffer_len, TAG_DONE);
	
	for (j = 0; j < 100; ++j) {
	  DoMethod(proc, AHIM_Processor_Prepare, 0, 0, j * buffer_len);
	  DoMethod(proc, AHIM_Processor_Process, 0, 0, j * buffer_len);
	}
      }

      check_err(proc);
      SetAttrs(proc, AHIA_Processor_Busy, FALSE, TAG_DONE);

      gettimeofday(&tv_end, NULL);
      check_err(proc);

      l_start = tv_start.tv_sec * 1e6 + tv_start.tv_usec;
      l_end = tv_end.tv_sec * 1e6 + tv_end.tv_usec;
      msec = (l_end - l_start) / 1000 / i;
      hz = 1e6 * i / (l_end - l_start);
      Printf("i=%ld; j=%ld\n", i, j);
      Printf("%ld iterations took %ld ms (%ld Hz)\n", j, msec, hz);
	     
    }
    
    CloseDevice((struct IORequest*) &ahirequest);
  }

  return rc;
}


int main(void) {
  Object* b = AHI_NewObject(AHIC_Buffer,
			    AHIA_Buffer_SampleType,	buffer_type,
			    AHIA_Buffer_SampleFreqInt,	48000,
			    AHIA_Buffer_Capacity,	buffer_len,
			    TAG_DONE);

  Object* gain = AHI_NewObject(AHIC_GainProcessor,
			       TAG_DONE);

  struct TagItem gain_map[] = {
      { AHIA_LFO_I, AHIA_GainProcessor_Gain },
      { TAG_DONE,   0                       }
  };
  
  Object* lfo1 = AHI_NewObject(AHIC_LFO,
			       AHIA_LFO_Frequency,	0x20000,
			       AHIA_LFO_Amplitude,	0x60000,
			       AHIA_LFO_Bias,		0,
			       AHIA_LFO_Waveform,	AHIV_LFO_Sine,
			       ICA_TARGET,              (ULONG) gain,
			       ICA_MAP,                 (ULONG) gain_map,
			       TAG_DONE);

/*   Object* lfo2 = AHI_NewObject(AHIC_LFO, */
/* 			       AHIA_LFO_Frequency,	0x8000, */
/* 			       AHIA_LFO_Amplitude,	0x30000, */
/* 			       AHIA_LFO_Bias,		0, */
/* 			       AHIA_LFO_Waveform,	AHIV_LFO_Triangle, */
/* 			       TAG_DONE); */
  
  Object* tick = AHI_NewObject(AHIC_TickProcessor,
			       AHIA_Processor_Buffer,	(ULONG) b,
			       AHIA_AddNotify,		(ULONG) lfo1,
/* 			       AHIA_AddNotify,		(ULONG) lfo2, */
			       AHIA_Processor_AddChild,	(ULONG) gain,
			       TAG_DONE);

  if (IoErr()) {
    Printf("Error %lx\n", IoErr());
  }
  
  check_err(tick);
    
  if (b != NULL && tick != NULL) {
    dBFixed gains[] = { 0x60000, 0xc0000 };
    
    DoMethod(tick, AHIM_GainProcessor_SetBalance, 2, gains);
    check_err(tick);

    execute(tick, 1);
  }
    
  DisposeObject(tick);
  DisposeObject(b);
  CleanUpAHI();
}
