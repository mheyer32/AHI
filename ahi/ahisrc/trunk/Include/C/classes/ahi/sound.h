#ifndef CLASSES_AHI_SOUND_H
#define CLASSES_AHI_SOUND_H

/*
**	$VER: sound.h 7.0 (2.7.2003)
**
**	ahi-sound.class definitions
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
#define AHIC_Sound		"ahi-sound.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Sound {
#else
# define _P _Sound
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {

      /* High part of length (unit is sample frames << 32). */
      _AHIA(_P, LengthHigh,	(_AHIA_Dummy+25)),		/* OM_GET */

      /* Low part of length (unit is sample frames). */
      _AHIA(_P, LengthLow,	(_AHIA_Dummy+26)),		/* OM_GET */

      /* The sample frequency of this sound. */
      _AHIA(_P, SampleFreq	,(_AHIA_Dummy+27))		/* OM_GET */

    };
    
/*****************************************************************************/

    enum {

      /* Decode to a buffer. Returns TRUE if successful. */
      _AHIM(_P, Decode,		(_AHIM_Dummy+11))

    };

/*****************************************************************************/

    /* AHIM_Sound_Decode, returns TRUE on success */
    struct _AHIP(_P, Decode) {
	ULONG	MethodID;
	Object*	Buffer;
	UQUAD	Offset;
    };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_SOUND_H */
