#ifndef CLASSES_AHI_PROCESSOR_LINEAR_RESAMPLER_H
#define CLASSES_AHI_PROCESSOR_LINEAR_RESAMPLER_H

/*
**	$VER: linear-resampler.h 7.0 (2.7.2003)
**
**	linear-resampler.ahi-processor definitions
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
#define AHIC_LinearResamplerProcessor	"linear-resampler.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace LinearResampler {
#else
# define _P _LinearResamplerProcessor
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

#endif /* CLASSES_AHI_PROCESSOR_Linear_RESAMPLER_H */
