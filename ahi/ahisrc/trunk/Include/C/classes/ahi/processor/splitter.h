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

/* No attributes */

/*****************************************************************************/

      enum {
	
	/* Create a "clone" of this object */
	_AHIM(_P, CloneObject,	(_AHIM_Dummy+16))

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
