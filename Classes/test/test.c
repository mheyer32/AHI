
#include <classes/ahi.h>
#include <classes/ahi/buffer.h>
#include <classes/ahi/processor.h>
#include <classes/ahi/processor/gain.h>
#include <devices/ahi.h>

#include <math.h>
#include <stdio.h>

#include <proto/ahi.h>
#include <proto/exec.h>
#include <proto/intuition.h>

struct Library* AHIBase;

static int buffer_type = AHIST_F2;
static int buffer_len  = 16;

static inline void check_err(Object* o) {
  ULONG  error = 0;
  STRPTR error_msg = NULL;

  GetAttr(AHIA_Error, o, &error);
  GetAttr(AHIA_ErrorMessage, o, &error_msg);

  SetAttrs(o, AHIA_Error, AHIE_OK, TAG_DONE);
  
  if (error) {
    printf("Error %08x: %s\n", error, error_msg);
  }
}

int main(void) {
  struct Library* buffer    = OpenLibrary("AHI/" AHIC_Buffer, 7);
  struct Library* processor = OpenLibrary("AHI/" AHIC_GainProcessor, 7);

  Object* b = NewObject(NULL, AHIC_Buffer,
			AHIA_Buffer_SampleType,    buffer_type,
			AHIA_Buffer_SampleFreqInt, 48000,
			AHIA_Buffer_Capacity,      buffer_len,
			TAG_DONE);
  Object* p = NewObject(NULL, AHIC_GainProcessor,
			AHIA_Processor_Buffer, b,
			TAG_DONE);

  if (IoErr()) {
    printf("Error %x\n", IoErr());
  }

  printf("AHIST_C_DECODE(AHIST_F1): %d\n", AHIST_C_DECODE(AHIST_F1));
  printf("AHIST_C_DECODE(AHIST_NOTYPE): %d\n", AHIST_C_DECODE(AHIST_NOTYPE));
  
  if (b && p) {
    float* buffer = NULL;

    check_err(b);
    check_err(p);
    
    printf("Buffer object %08x, Processor object %08x\n", b, p);
    
    if (GetAttr(AHIA_Buffer_Data, b, &buffer)) {
      ULONG length;
      
      printf("Data buffer %08x\n", buffer);

      length = DoMethod(b, AHIM_Buffer_SampleFrameSize,
			buffer_type, buffer_len, NULL);

      check_err(b);
      
      if (length > 0) {
	dBFixed gains[] = { 0x60000, 0xc0000 };
	int i;
	
	printf("Length %d bytes.\n", length);

	for (i = 0; i < buffer_len; ++i) {
	  buffer[2 * i + 0] = +1.0 * sin((double) i / buffer_len * 2 * M_PI);
	  buffer[2 * i + 1] = -0.5 * sin((double) i / buffer_len * 2 * M_PI);
	}

	printf("Prepare: %d\n", DoMethod(p, AHIM_Processor_Prepare, 0, 0, 0));
	check_err(p);
	printf("Process: %d\n", DoMethod(p, AHIM_Processor_Process, 0, 0, 0));
	check_err(p);

	SetAttrs(p, AHIA_Processor_Busy, TRUE, TAG_DONE);
	check_err(p);

	DoMethod(p, AHIM_GainProcessor_SetBalance, 2, gains);
	check_err(p);

	printf("Prepare: %d\n", DoMethod(p, AHIM_Processor_Prepare, 0, 0, 0));
	check_err(p);
	printf("Process: %d\n", DoMethod(p, AHIM_Processor_Process, 0, 0, 0));
	check_err(p);

	for (i = 0; i < buffer_len; ++i) {
	  printf( "%f, %f\n", buffer[2 * i + 0], buffer[2 * i + 1]);
	}

	SetAttrs(b, AHIA_Buffer_Length, 8, TAG_DONE);
      }

    }
    
  }
  
  DisposeObject(p);
  DisposeObject(b);

  CloseLibrary(buffer);
  CloseLibrary(processor);
  
  return 0;
}


static int
test(Object* proc) {
  int rc = 0;
  static struct AHIRequest ahirequest;

  ahirequest.ahir_Std.io_Message.mn_Length = sizeof (ahirequest);
  ahirequest.ahir_Version = 7;

  if (OpenDevice( AHINAME, AHI_NO_UNIT, (struct IORequest*) &ahirequest, 0) == 0) {
    AHIBase = (struct Library*) ahirequest.ahir_Std.io_Device;

    CloseDevice((struct IORequest*) &ahirequest);
  }

  return rc;
}
