#ifndef CLASSES_AHI_BOARD_H
#define CLASSES_AHI_BOARD_H

/*
**	$VER: board.h 7.0 (2.7.2003)
**
**	ahi-board.class definitions
**
**	(C) Copyright 2002-2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef CLASSES_AHI_H
#include <classes/ahi.h>
#endif

/* This class inherits "ahi.class" */
#define AHIC_Board		"ahi-board.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Board {
#else
# define _P _Board
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {

      /* The board number */
      _AHIA(_P, Number,		(_AHIA_Dummy+46)),		/* OM_NEW */

      /* The number of discrete outputs (i.e., D/A converters, S/PDIF
       * outputs etc) */
      _AHIA(_P, Outputs,	(_AHIA_Dummy+47)),		/* OM_GET,
								   OM_NOTIFY */

      /* A pointer to the output IClass */
      _AHIA(_P, OutputClass,	(_AHIA_Dummy+48)),		/* OM_GET */

      /* The number of discrete inputs (i.e., A/D converters, S/PDIF
       * inputs etc) */
      _AHIA(_P, Inputs,		(_AHIA_Dummy+49)),		/* OM_GET,
								   OM_NOTIFY */

      /* A pointer to the input IClass */
      _AHIA(_P, InputClass,	(_AHIA_Dummy+50)),		/* OM_GET */

      /* The number of discrete, independent analog mixers */
      _AHIA(_P, Mixer,		(_AHIA_Dummy+51)),		/* OM_GET,
								   OM_NOTIFY */

      /* A pointer to the mixer IClass */
      _AHIA(_P, MixerClass,	(_AHIA_Dummy+52)),		/* OM_GET */

      /* The number of discrete, independent MIDI outputs */
      _AHIA(_P, MIDIOutputs,	(_AHIA_Dummy+53)),		/* OM_GET,
								   OM_NOTIFY */

      /* A pointer to the MIDI output IClass */
      _AHIA(_P, MIDIOutputClass, (_AHIA_Dummy+54)),		/* OM_GET */

      /* The number of discrete, independent MIDI inputs */
      _AHIA(_P, MIDIInputs,	(_AHIA_Dummy+55)),		/* OM_GET,
								   OM_NOTIFY */

      /* A pointer to the MIDI input IClass */
      _AHIA(_P, MIDIInputClass, (_AHIA_Dummy+56))		/* OM_GET */

    };

/*****************************************************************************/

/* No methods */

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_BOARD_H */
