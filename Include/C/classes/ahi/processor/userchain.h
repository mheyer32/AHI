#ifndef CLASSES_AHI_PROCESSOR_USERCHAIN_H
#define CLASSES_AHI_PROCESSOR_USERCHAIN_H

/*
**	$VER: userchain.h 7.0 (2.7.2003)
**
**	userchain.ahi-processor definitions
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
#define AHIC_UserchainProcessor	"userchain.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Userchain {
#else
# define _P _Processor_Userchain
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

#endif /* CLASSES_AHI_PROCESSOR_GAIN_H */
