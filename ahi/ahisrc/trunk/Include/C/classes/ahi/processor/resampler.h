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
	_AHIA(_P, Frequency,	(_AHIA_Dummy+108)),	/* OM_NEW,
							   OM_GET,
							   OM_SET,
							   OM_NOTIFY */

	/* The pitch in octaves on the input side. See frequency above
	   too. 0x10000 means Fin=2*Fout. (Fixed) */
	_AHIA(_P, Pitch,	(_AHIA_Dummy+109)),	/* OM_NEW,
							   OM_GET,
							   OM_SET,
							   OM_NOTIFY */

	/* The phase offset. Not all subclasses support this
	 * attribute! (Fixed, radians, keep it within 0-2*PI) */
	_AHIA(_P, PhaseOffset,	(_AHIA_Dummy+110))	/* OM_NEW,
							   OM_GET,
							   OM_SET,
							   OM_NOTIFY */
      };
      
/*****************************************************************************/

      /* No methods. */

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
