#ifndef CLASSES_AHI_PROCESSOR_SOUND_H
#define CLASSES_AHI_PROCESSOR_SOUND_H

/*
**	$VER: sound.h 7.0 (2.7.2003)
**
**	sound.ahi-processor definitions
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
#define AHI_SOUND_PROCESSOR_CLASS	"sound.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Sound {
#else
# define _P _Processor_Sound
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* The sound object */
	_AHIA(_P, Sound,	(_AHIA_Dummy+39))	/* OM_NEW,
							   OM_GET */

      };

/*****************************************************************************/

/* Overridden members:

   OM_ADDMEMBER	Always returns FALSE and does nothing.

   OM_REMMEMBER	Always returns FALSE and does nothing.
*/

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_SOUND_H */
