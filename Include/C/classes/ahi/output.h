#ifndef CLASSES_AHI_OUTPUT_H
#define CLASSES_AHI_OUTPUT_H

/*
**	$VER: output.h 7.0 (2.7.2003)
**
**	ahi-output.class definitions
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
#define AHIC_Output		"ahi-output.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Output {
#else
# define _P _Output
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {
      
      /* The output number */
      _AHIA(_P, Number,		(_AHIA_Dummy+57)),		/* OM_NEW */

      /* The mode (shared/exclusive/any) to use */
      _AHIA(_P, Mode,		(_AHIA_Dummy+58)),		/* OM_NEW,
								   OM_GET */
      /* The different buffer sizes supported returned as an ULONG
       * array (~0-terminated). The unit is bytes. */
      _AHIA(_P, BufferSizes,	(_AHIA_Dummy+59)),		/* OM_GET */

      /* The current buffer size (unit is bytes) */
      _AHIA(_P, BufferSize,	(_AHIA_Dummy+60)),		/* OM_NEW,
								   OM_SET,
								   OM_GET,
								   OM_NOTIFY */

      /* The different sample frequencies supported returned as an
       * ULONG array (~0-terminated). The unit is Hz. */
      _AHIA(_P, SampleFreqs,	(_AHIA_Dummy+61)),		/* OM_GET */

      /* The current sample frequency (unit is Hz) */
      _AHIA(_P, SampleFreq,	(_AHIA_Dummy+62)),		/* OM_NEW,
								   OM_SET,
								   OM_GET,
								   OM_NOTIFY */

      /* The current sample frequency's fractional part (unit is Hz >>32) */
      _AHIA(_P, SampleFreqFract, (_AHIA_Dummy+63)),		/* OM_GET */

      /* TRUE if playback is realtime */
      _AHIA(_P, Realtime,	(_AHIA_Dummy+64))		/* OM_GET */

    };

    enum {
      _AHIV(_P, AnyMode,		(0)),
      _AHIV(_P, SharedMode,		(1)),
      _AHIV(_P, ExclusiveMode,		(2))
    };
    
/*****************************************************************************/

    enum {

      /* Allocate (and perhaps initialize) hardware */
      _AHIM(_P, Allocate,	(_AHIM_Dummy+20)),

      /* Reset and free hardware */
      _AHIM(_P, Free,		(_AHIM_Dummy+21)),

      /* Start playback */
      _AHIM(_P, Start,		(_AHIM_Dummy+22)),

      /* Pause playback */
      _AHIM(_P, Pause,		(_AHIM_Dummy+23)),

      /* Stop playback */
      _AHIM(_P, Stop,		(_AHIM_Dummy+24)),


/* Helper methods follows. These are only used by subclasses. */

      /* Fill a buffer (a)synchronously and invoke
       * AHIM_Output_BufferFilled when done */
      _AHIM(_P, FillBuffer,	(_AHIM_Dummy+25)),

      /* Invokend by AHI when AHIM_Output_FillBuffer is finished */
      _AHIM(_P, BufferFilled,	(_AHIM_Dummy+26))

    };

/*****************************************************************************/
	     
    struct _AHIP(_P, FillBuffer) {
      ULONG	MethodID;
      ULONG	Flags;
      UQUAD	PlaybackTime;
    };

    /* If this flag is set, AHIM_Output_BufferFilled will be called
     * before AHIM_Output_FillBuffer returns. In that case,
     * AHIM_Output_FillBuffer MUST NOT be called from a hardware
     * interrupt. */

    enum {
      _AHIB(_P, SyncedFill,		(0))
    };

    struct _AHIP(_P, BufferFilled) {
      ULONG	MethodID;
      Object*	Buffer;
    };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */


#endif /* CLASSES_AHI_OUTPUT_H */
