
#include <classes/ahi.h>
#include <classes/ahi/buffer.h>
#include <classes/ahi/processor.h>
#include <intuition/classusr.h>

#include <math.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

static int buffer_type = AHIST_F2;
static int buffer_len  = 1024;

static inline check_err(Object* o) {
  ULONG  error = 0;
  STRPTR error_msg = NULL;

  GetAttr(AHIA_Error, o, &error);
  GetAttr(AHIA_ErrorMessage, o, &error_msg);

  SetAttrs(o, AHIA_Error, AHIE_OK, TAG_DONE);
  
  if (error) {
    Printf("Error %08lx: %s\n", error, error_msg);
  }
}

int main(void) {
  struct Library* buffer    = OpenLibrary("AHI/" AHIC_Buffer, 7);
  struct Library* processor = OpenLibrary("AHI/" AHIC_Processor, 7);

  Object* b = NewObject(NULL, AHIC_Buffer,
			AHIA_Buffer_SampleType,    buffer_type,
			AHIA_Buffer_SampleFreqInt, 48000,
			AHIA_Buffer_Capacity,      buffer_len,
			TAG_DONE);
  Object* p = NewObject(NULL, AHIC_Processor,
			AHIA_Processor_Buffer, b,
			TAG_DONE);

  if (IoErr()) {
    Printf("Error %lx\n", IoErr());
  }
  
  if (b && p) {
    float* buffer = NULL;

    check_err(b);
    check_err(p);
    
    Printf("Buffer object %08lx, Processor object %08lx\n", b, p);
    
    if (GetAttr(AHIA_Buffer_Data, b, &buffer)) {
      ULONG length;
      
      Printf("Data buffer %08lx\n", buffer);

      length = DoMethod(b, AHIM_Buffer_SampleFrameSize,
			buffer_type, buffer_len, NULL);

      check_err(b);
      
      if (length > 0) {
	int i;
	Printf("Length %ld bytes.\n", length);

	for (i = 0; i < buffer_len; ++i) {
	  buffer[2 * i + 0] = +1.0 * sin((double) i / 1024 * 2 * M_PI);
	  buffer[2 * i + 1] = -0.5 * sin((double) i / 1024 * 2 * M_PI);
	}
      }

      Printf("Prepare: %ld\n", DoMethod(p, AHIM_Processor_Prepare, 0, 0, 0));
      check_err(p);
      Printf("Process: %ld\n", DoMethod(p, AHIM_Processor_Process, 0, 0, 0));
      check_err(p);

      SetAttrs(p, AHIA_Processor_Busy, TRUE, TAG_DONE);
      check_err(p);

      Printf("Prepare: %ld\n", DoMethod(p, AHIM_Processor_Prepare, 0, 0, 0));
      check_err(p);
      Printf("Process: %ld\n", DoMethod(p, AHIM_Processor_Process, 0, 0, 0));
      check_err(p);
    }
    
  }
  
  DisposeObject(p);
  DisposeObject(b);

  CloseLibrary(buffer);
  CloseLibrary(processor);
  
  return 0;
}
