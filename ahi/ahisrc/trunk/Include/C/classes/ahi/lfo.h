#ifndef CLASSES_AHI_LFO_H
#define CLASSES_AHI_LFO_H

/*
**	$VER: lfo.h 7.0 (16.6.2004)
**
**	ahi-lfo.class definitions
**
**	(C) Copyright 2004 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef CLASSES_AHI_H
#include <classes/ahi.h>
#endif

/* This class inherits "ahi.class". */
#define AHIC_LFO	"ahi-lfo.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace LFO {
#else
# define _P _LFO
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {

	/* The frequency (Fixed) */
	_AHIA(_P, Frequency,	(_AHIA_Dummy+84)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

	/* The phase offset (Fixed, radians, keep it within ±2*PI) */
	_AHIA(_P, PhaseOffset,	(_AHIA_Dummy+85)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */
	
	/* The amplitude */
	_AHIA(_P, Amplitude,	(_AHIA_Dummy+86)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

	/* The DC bias */
	_AHIA(_P, Bias,		(_AHIA_Dummy+87)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

	/* The LFO waveform */
	_AHIA(_P, Waveform,	(_AHIA_Dummy+88)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

	/* The phase (Fixed, radians, 0-2*PI) */
	_AHIA(_P, Phase,	(_AHIA_Dummy+89)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

	/* The in-phase (cos) signal */
	_AHIA(_P, I,		(_AHIA_Dummy+90)),	/* OM_GET,
							   OM_NOTIFY */

	/* The quadrature (sin) signal */
	_AHIA(_P, Q,		(_AHIA_Dummy+91)),	/* OM_GET,
							   OM_NOTIFY */

	/* The current time (ULONG). Setting this attribute causes the
	 * LFO to update AHIA_LFO_Phase, AHIA_LFO_I and AHIA_LFO_Q. */
	_AHIA(_P, Tick,		(_AHIA_Dummy+92)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */

	/* The tick frequency (ULONG, Hz) */
	_AHIA(_P, TickFreq,	(_AHIA_Dummy+93))	/* OM_NEW,
							   OM_SET,
							   OM_GET */
	
	/* In addition to AHIA_LFO_Tick and AHIA_LFO_TickFreq,
	   ahi-lfo.class also recognizes the AHIA_TickProcessor_Tick
	   attribute, which uses an ahi-buffer.class object as
	   ti_Data. The buffer's timestamp and frequency is used as
	   reference clock. */
      };


/*****************************************************************************/

/* No methods */

/*****************************************************************************/

      /* Values for AHIA_LFO_Waveform */
      enum {
	_AHIV(_P, Sine,		(0)),
	_AHIV(_P, Square,	(1)),
	_AHIV(_P, Triangle,	(2)),
	_AHIV(_P, Sawtooth,	(3))
      };

/*****************************************************************************/

     enum {
       _AHIE(_P, InvalidWaveform,	(_AHIE_Dummy+10))
     };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_LFO_H */
