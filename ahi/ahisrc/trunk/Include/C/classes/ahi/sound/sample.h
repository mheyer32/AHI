#ifndef CLASSES_AHI_SOUND_SAMPLE_H
#define CLASSES_AHI_SOUND_SAMPLE_H

/*
**	$VER: sample.h 7.0 (2.7.2003)
**
**	sample.ahi-sound definitions
**
**	(C) Copyright 2002-2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef CLASSES_AHI_SOUND_H
#include <classes/ahi/sound.h>
#endif

/* This class inherits "ahi-sound.class" */
#define AHI_SAMPLE_SOUND_CLASS	"sample.ahi-sound"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Sound {
    namespace Sample {
#else
# define _P _Sound_Sample
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

	/* The sample type. */
	_AHIA(_P, SampleType,	(_AHIA_Dummy+30)),	/* OM_NEW,
							   OM_GET */

	/* Address for data. */
	_AHIA(_P, Address,	(_AHIA_Dummy+31)),	/* OM_NEW,
							   OM_GET */

	/* Length (unit is sample frames). */
	_AHIA(_P, Length,	(_AHIA_Dummy+32)),	/* OM_NEW */

	/* Modification to source buffer allowed if TRUE */
	_AHIA(_P, Dynamic,	(_AHIA_Dummy+33)),	/* OM_NEW,
							   OM_GET */

	/* Make a local copy of the sample data */
	_AHIA(_P, CopyData,	(_AHIA_Dummy+34))	/* OM_NEW */

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

#endif /* CLASSES_AHI_SOUND_SAMPLE_H */
