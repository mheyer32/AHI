#ifndef CLASSES_AHI_PROCESSOR_RESAMPLER_H
#define CLASSES_AHI_PROCESSOR_RESAMPLER_H

/*
**	$VER: resampler.h 7.0 (2.7.2003)
**
**	resampler.ahi-processor definitions
**
**	(C) Copyright 2002-2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef CLASSES_AHI_PROCESSOR_H
#include <classes/ahi/processor.h>
#endif

/* This class inherits "ahi-processor.class". */
#define AHIC_ResamplerProcessor	"resampler.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Resampler {
#else
# define _P _ResamplerProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

	/* The sample frequency in Hz on the input side. See pitch
	   below too. (ULONG) */
	_AHIA(_P, Frequency,	(_AHIA_Dummy+109)),	/* OM_NEW,
							   OM_GET,
							   OM_SET,
							   OM_NOTIFY */

	/* The pitch in octaves on the input side. See frequency above
	   too. 0x10000 means Fin=2*Fout. (Fixed) */
	_AHIA(_P, Pitch,	(_AHIA_Dummy+110)),	/* OM_NEW,
							   OM_GET,
							   OM_SET,
							   OM_NOTIFY */

	/* The phase offset. Not all subclasses support this
	 * attribute! (Fixed, radians, keep it within 0-2*PI) */
	_AHIA(_P, PhaseOffset,	(_AHIA_Dummy+111)),	/* OM_NEW,
							   OM_GET,
							   OM_SET,
							   OM_NOTIFY */

	/* The maximum downsampling factor expressed as maximum
	 * pitch. (Fixed) */
	_AHIA(_P, MaxPitch,	(_AHIA_Dummy+112))	/* OM_NEW,
							   OM_GET */
      };
      
/*****************************************************************************/

      enum {
	/* Process with extra return parameters. */
	_AHIM(_P, Process,	(_AHIM_Dummy+51))
      };

      /* AHIM_ResamplerProcessor_Process */
      struct _AHIP(_P, Process) {
	ULONG	MethodID;
	ULONG	Flags;
	UQUAD	CurrentTime;

	/* These will be filled in by the AHIM_ResamplerProcessor_Process */
	UQUAD   Rate;
	ULONG   Phase;
	FLOAT*  Source;
	FLOAT*  Destination;
	ULONG   Length;
      };


      /* Return value */

      enum {
	_AHIV(_P, FailProc,		(0)),
	_AHIV(_P, SkipProc,		(1)),
	_AHIV(_P, PerformProc,		(2)),
	_AHIV(_P, PerformAndRepeatProc,	(3))
      };
      
/*****************************************************************************/

      /* No errors. */
      
/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_RESAMPLER_H */
