#ifndef CLASSES_AHI_PROCESSOR_FIR_RESAMPLER_H
#define CLASSES_AHI_PROCESSOR_FIR_RESAMPLER_H

/*
**	$VER: fir-resampler.h 7.0 (2.7.2003)
**
**	fir-resampler.ahi-processor definitions
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

/* This class inherits "resampler.ahi-processor". */
#define AHIC_FIRResamplerProcessor	"fir-resampler.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace FIRResampler {
#else
# define _P _FIRResamplerProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

	/* Enable low-pass filtering when upsampling. (BOOL) */
	_AHIA(_P, LowPass,	(_AHIA_Dummy+111)),	/* OM_NEW,
							   OM_GET */

	/* Enable linear interpolation of FIR tables. (BOOL) */
	_AHIA(_P, Interpolate,	(_AHIA_Dummy+112)),	/* OM_NEW,
							   OM_GET */

	/* The number of taps in the FIR filter (equals the number of
	 * multiplications per sample if not interpolating, else half
	 * the number of multiplications per sample). (ULONG) */
	_AHIA(_P, Taps,		(_AHIA_Dummy+113))	/* OM_NEW,
							   OM_GET */

	/* The number of bits used when calculating fractional
	 * offsets. The FIR table size equals Taps*(2^Accuracy);
	 * double it if interpolation is used. (ULONG, suggester
	 * range: 8-12)
	_AHIA(_P, Accuracy,	(_AHIA_Dummy+114))	/* OM_NEW,
							   OM_GET */

	/* The quality of the resampling. (ULONG) */
	_AHIA(_P, Quality,	(_AHIA_Dummy+115))	/* OM_NEW,
							   OM_GET */
      };

      enum {
	_AHIV(_P, CustomQuality,	(0)),
	_AHIV(_P, LowQuality,		(1)), /* 8 taps, 8 bits */
	_AHIV(_P, MediumQuality,	(2)), /* 12 taps, 12 bits, LP */
	_AHIV(_P, HighQuality,		(3))  /* 32 taps, 12 bits, LP, LI */
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

#endif /* CLASSES_AHI_PROCESSOR_FIR_RESAMPLER_H */
