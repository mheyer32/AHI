#ifndef CLASSES_AHI_PROCESSOR_TICK_H
#define CLASSES_AHI_PROCESSOR_TICK_H

/*
**	$VER: tick.h 7.0 (2.7.2003)
**
**	tick.ahi-processor definitions
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
#define AHIC_TickProcessor	"tick.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Tick {
#else
# define _P _TickProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* The notification attribute */
	_AHIA(_P, Tick,		(_AHIA_Dummy+94))	/* OM_NOTIFY */

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

#endif /* CLASSES_AHI_PROCESSOR_TICK_H */
