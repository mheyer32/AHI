#ifndef CLASSES_AHI_PROCESSOR_ADDER_H
#define CLASSES_AHI_PROCESSOR_ADDER_H

/*
**	$VER: adder.h 7.0 (2.7.2003)
**
**	adder.ahi-processor definitions
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
#define AHIC_AdderProcessor	"adder.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Adder {
#else
# define _P _Processor_Adder
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

/* No attributes */

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

#endif /* CLASSES_AHI_PROCESSOR_ADDER_H */
