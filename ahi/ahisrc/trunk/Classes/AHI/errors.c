
#include <classes/ahi/buffer.h>
#include <classes/ahi/lfo.h>
#include <classes/ahi/processor.h>
#include <classes/ahi/processor/gain.h>
#include <classes/ahi/processor/splitter.h>

#include <proto/dos.h>

#include "errors.h"
#include "util.h"

char*
get_error_message(struct AHIClassData* AHIClassData) {
  switch (AHIClassData->error) {


    // ahi.class errors

    case AHIE_OK:
      return "No error";

    case AHIE_AttributeNotApplicable:
      return AHIC_AHI ": Attribute is not applicable";


    // ahi-buffer.class errors

    case AHIE_Buffer_InvalidSampleType:
      return AHIC_Buffer ": Invalid sample type";

    case AHIE_Buffer_InvalidSampleFreq:
      return AHIC_Buffer ": Invalid sample frequency";

    case AHIE_Buffer_InvalidCapacity:
      return AHIC_Buffer ": Invalid buffer capacity";

    case AHIE_Buffer_InvalidLength:
      return AHIC_Buffer ": Invalid buffer length";


    // ahi-processor.class errors
      
    case AHIE_Processor_ObjectNotReady:
      return AHIC_Processor ": Object is not ready";

    case AHIE_Processor_ObjectBusy:
      return AHIC_Processor ": Object is busy";

    case AHIE_Processor_InvalidSampleType:
      return AHIC_Processor ": Invalid buffer sample type";
      
    case AHIE_Processor_TooManyChildren:
      return AHIC_Processor ": Too many children";

    case AHIE_Processor_MultipleParents:
      return AHIC_Processor ": Multiple parents detected";

    case AHIE_Processor_DifferentOwner:
      return AHIC_Processor ": Different owner detected";

    case AHIE_Processor_NotMember:
      return AHIC_Processor ": Not a member";

    case AHIE_Processor_NullMember:
      return AHIC_Processor ": Null member";


    // gain.ahi-processor errors

    case AHIE_GainProcessor_TooManyChannels:
      return AHIC_GainProcessor ": Too many channels";


    // splitter.ahi-processor errors

    case AHIE_SplitterProcessor_InvalidBufferTime:
      return AHIC_SplitterProcessor ": Invalid buffer time";

    case AHIE_SplitterProcessor_InvalidBuffer:
      return AHIC_SplitterProcessor ": Invalid buffer";


    // ahi-lfo.class errors
      
    case AHIE_LFO_InvalidWaveform:
      return AHIC_LFO ": Invalid waveform";


    // dos.library or unknown errors

    default:
      if (Fault(AHIClassData->error,
		DOSNAME,
		AHIClassData->error_message,
		sizeof (AHIClassData->error_message)) > 0) {
	return  AHIClassData->error_message;
      }
  }

  // I don't think this will ever be reached, since Fault() should
  // handle unknown error codes too ...?

  SPrintF(AHIClassData->error_message,
	  "Unknown error 0x%08lx (%ld)",
	  AHIClassData->error, AHIClassData->error);

  return AHIClassData->error_message;
}


