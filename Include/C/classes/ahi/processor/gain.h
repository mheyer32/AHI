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
	_AHIA(_P, Channels,	(_AHIA_Dummy+38)),	/* OM_GET */

	/* The "master" gain (dbFixed) */
	_AHIA(_P, Gain,		(_AHIA_Dummy+83))	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */
      };
      
/*****************************************************************************/

      enum {

	/* Set relative gain for x channels. Returns TRUE on success. */
	_AHIM(_P, SetBalance,	(_AHIM_Dummy+14)),

	/* Get relative gain for x channels. Returns TRUE on success. */
	_AHIM(_P, GetBalance,	(_AHIM_Dummy+15))

      };


      /* AHIM_GainProcessor_SetBalance and AHIM_GainProcessor_GetBalance */
      struct _AHIP(_P, Balance) {
	ULONG		MethodID;
	ULONG		Channels;
	dBFixed*	Gains;
      };

      /* Just some utility constants for dBFixed */
      enum {
	_AHIV(_P, Tenth_dBAmplitude,		(-1310720)),
	_AHIV(_P, Tenth_dBPower,		(-655360)),
	_AHIV(_P, Half_dBAmplitude,		(-394566)),
	_AHIV(_P, Half_dBPower,			(-197273)),
	_AHIV(_P, Unity_dB,			(0)),
	_AHIV(_P, Double_dBPower,		(+197273)),
	_AHIV(_P, Double_dBAmplitude,		(+394566)),
	_AHIV(_P, Tenfold_dBPower,		(+655360)),
	_AHIV(_P, Tenfold_dBAmplitude,		(+1310720))
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
