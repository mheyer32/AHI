#ifndef CLASSES_AHI_PROCESSOR_V6SYNTH_H
#define CLASSES_AHI_PROCESSOR_V6SYNTH_H

/*
**	$VER: v6synth.h 7.0 (2.7.2003)
**
**	v6synth.ahi-processor definitions
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
#define AHIC_V6SynthProcessor	"v6synth.ahi-processor"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
    namespace V6Synth {
#else
# define _P _V6SynthProcessor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* The number of channels */
	_AHIA(_P, Channels,	(_AHIA_Dummy+40))	/* OM_NEW,
							   OM_GET */

      };

/*****************************************************************************/

//      enum {
	
	/* *Lots* of methods here :-) I want at least what AHI 5 has,
	 * plus volume and frequency envelopes and LFOs. If
	 * possible/easy to do, filter as well. */

//      };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
    } 
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_PROCESSOR_V6SYNTH_H */
