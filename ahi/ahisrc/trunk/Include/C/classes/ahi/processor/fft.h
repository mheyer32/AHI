#ifndef CLASSES_AHI_PROCESSOR_FFT_H
#define CLASSES_AHI_PROCESSOR_FFT_H

/*
**	$VER: fft.h 7.0 (2.7.2003)
**
**	fft.ahi-processor definitions
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
#define AHIC_FFTProcessor	"fft.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace FFT {
#else
# define _P _FFTProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

 	/* The number of overlapping sample frames to use */
	_AHIA(_P, Overlap,	(_AHIA_Dummy+65))	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_UPDATE */

      };

/*****************************************************************************/

      /* No methods */

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_FFT_H */
