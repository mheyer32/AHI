#ifndef CLASSES_AHI_PROCESSOR_NN_RESAMPLER_H
#define CLASSES_AHI_PROCESSOR_NN_RESAMPLER_H

/*
**	$VER: nn-resampler.h 7.0 (2.7.2003)
**
**	nn-resampler.ahi-processor definitions
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
#define AHIC_NNResamplerProcessor	"nn-resampler.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace NNResampler {
#else
# define _P _NNResamplerProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      /* No attributes */
      
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

#endif /* CLASSES_AHI_PROCESSOR_NN_RESAMPLER_H */
