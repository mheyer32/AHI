#ifndef CLASSES_AHI_PROCESSOR_FIR_H
#define CLASSES_AHI_PROCESSOR_FIR_H

/*
**	$VER: fir.h 7.0 (2.7.2003)
**
**	fir.ahi-processor definitions
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
#define AHIC_FIRProcessor	"fir.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace FIR {
#else
# define _P _FIRProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

	/* The number of taps */
	_AHIA(_P, Taps,		(_AHIA_Dummy+37))	/* OM_NEW,
							   OM_SET,
							   OM_GET */

      };

/*****************************************************************************/

      enum {

	/* Set coefficients for x taps (the remaining are set to 0). */
	_AHIM(_P, SetCoefficients,	(_AHIM_Dummy+12)),

	/* Get coefficients for x taps */
	_AHIM(_P, GetCoefficients,	(_AHIM_Dummy+13))

      };


      struct _AHIP(_P,Coefficients) {
	ULONG		MethodID;
    	ULONG		Taps;
	Fixed28*	CoefficientPtr;
      };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_FIR_H */
