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
#define AHIC_SoundProcessor	"sound.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Sound {
#else
# define _P _SoundProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* Add a sound object. This special attribute can be used
	 * several times to add more than one sound, as if
	 * AHIM_SoundProcessor_AddSound had been called. */
	_AHIA(_P, AddSound,	(_AHIA_Dummy+39)),	/* OM_NEW */

	/* The sound object currently playing */
	_AHIA(_P, Sound,	(_AHIA_Dummy+103)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */

	/* The offset inside the sound, in sample frames. */
	_AHIA(_P, Offset,	(_AHIA_Dummy+104)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */

	/* The length of the sound segment being played, in sample
	 * frames. */
	_AHIA(_P, Length,	(_AHIA_Dummy+105)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */

	/* The number of sample frames left. */
	_AHIA(_P, Available,	(_AHIA_Dummy+106)),	/* OM_GET */

	/* This sound has reached the end. (Object*) */
	_AHIA(_P, EndOfSample,	(_AHIA_Dummy+107)),	/* OM_NOTIFY */
	
	/* Private */
	_AHIA(_P, Master,	(_AHIA_Dummy+108))	/* OM_NEW */

      };

/*****************************************************************************/

      enum {
	
	/* Add a sound. And sound still added when this object is
	 * disposed will be disposed as well. (BOOL) */
	_AHIM(_P, AddSound,	(_AHIM_Dummy+48))

	/* Remove a sound. (BOOL) */
	_AHIM(_P, RemSound,	(_AHIM_Dummy+49))
	
	/* Create a "clone" of this object */
	_AHIM(_P, Clone,	(_AHIM_Dummy+50))

      };
      
/*****************************************************************************/

      /* AHIM_SoundProcessor_AddSound, AHIM_SoundProcessor_RemSound */
      struct _AHIP(_P, Sound) {
	ULONG	MethodID;
	Object*	Sound;
      };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_SOUND_H */
