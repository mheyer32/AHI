#ifndef CLASSES_AHI_PROCESSOR_GAIN_H
#define CLASSES_AHI_PROCESSOR_GAIN_H

/*
**	$VER: gain.h 7.0 (2.7.2003)
**
**	gain.ahi-processor definitions
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
#define AHIC_GainProcessor	"gain.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace Gain {
#else
# define _P _GainProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* The number of discrete channels that can be manipulated */
	_AHIA(_P, Channels,	(_AHIA_Dummy+38))	/* OM_GET */

      };
      
/*****************************************************************************/

      enum {

	/* Set gain for x channels. Returns TRUE on success. */
	_AHIM(_P, Set,		(_AHIM_Dummy+14)),

	/* Get gain for x channels. Returns TRUE on success. */
	_AHIM(_P, Get,		(_AHIM_Dummy+15)),


	/* Set gain for all channels. Returns TRUE on success. */
	_AHIM(_P, SetAll,	(_AHIM_Dummy+27))

      };


      /* AHIM_GainProcessor_Set and AHIM_GainProcessor_Get */
      struct _AHIP(_P, Gain) {
	ULONG		MethodID;
	ULONG		Channels;
	dBFixed*	Gains;
      };

      /* AHIM_GainProcessor_SetAll */
      struct _AHIP(_P, GainAll ) {
	ULONG		MethodID;
	dBFixed		Gain;
      };

/*****************************************************************************/

      enum {
	_AHIE(_P, TooManyChannels,	(_AHIE_Dummy+7)),
	_AHIE(_P, InvalidSampleType,	(_AHIE_Dummy+8))
      };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_GAIN_H */
