#ifndef CLASSES_AHI_PROCESSOR_SPLITTER_H
#define CLASSES_AHI_PROCESSOR_SPLITTER_H

/*
**	$VER: splitter.h 7.0 (2.7.2003)
**
**	splitter.ahi-processor definitions
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
#define AHIC_SplitterProcessor	"splitter.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Splitter {
#else
# define _P _SplitterProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* Private */
	_AHIA(_P, Master,	(_AHIA_Dummy+96))	/* OM_NEW */

      };

/*****************************************************************************/

      enum {
	
	/* Create a "clone" of this object */
	_AHIM(_P, Clone,	(_AHIM_Dummy+16))

      };

/*****************************************************************************/

      enum {

	/* Raised in case one of the clones askes the splitter to
	 * process a buffer with a timestamp in the past. */
	_AHIE(_P, InvalidBufferTime,	(_AHIE_Dummy+15)),

	/* Raised in case one of the clones askes the splitter to
	 * process a buffer that does not match the original
	 * buffer. */
	_AHIE(_P, InvalidBuffer,	(_AHIE_Dummy+16))
      };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_SPLITTER_H */
