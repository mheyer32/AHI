#ifndef CLASSES_AHI_PROCESSOR_DITHER_H
#define CLASSES_AHI_PROCESSOR_DITHER_H

/*
**	$VER: dither.h 7.0 (2.7.2003)
**
**	dither.ahi-processor definitions
**
**	(C) Copyright 2002-2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/* Interesting link: http://audio.rightmark.org/lukin/dither/ */

/*****************************************************************************/

#ifndef CLASSES_AHI_PROCESSOR_H
#include <classes/ahi/processor.h>
#endif

/* This class inherits "ahi-processor.class". */
#define AHI_DITHER_PROCESSOR_CLASS	"dither.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Dither {
#else
# define _P _Processor_Dither
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

	/* The kind of dither to add */
	_AHIA(_P, Type,		(_AHIA_Dummy+35)),	/* OM_NEW,
							   OM_GET */

	/* The bit level we're adding dither to */
	_AHIA(_P, Bit,		(_AHIA_Dummy+36))	/* OM_NEW,
							   OM_GET */

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

#endif /* CLASSES_AHI_PROCESSOR_DITHER_H */
