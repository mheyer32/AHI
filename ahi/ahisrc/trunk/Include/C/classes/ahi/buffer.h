#ifndef CLASSES_AHI_BUFFER_H
#define CLASSES_AHI_BUFFER_H

/*
**	$VER: buffer.h 7.0 (2.7.2003)
**
**	ahi-buffer.class definitions
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
#define AHI_BUFFER_CLASS	"ahi-buffer.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Buffer {
#else
# define _P _Buffer
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {

      /* The sample type. */
      _AHIA(_P, SampleType,	(_AHIA_Dummy+10)),		/* OM_NEW,
								   OM_GET */

      /* Sample rate (integer part). */
      _AHIA(_P, SampleFreqInt,(_AHIA_Dummy+11)),		/* OM_NEW,
								   OM_GET */

      /* Sample rate (fraction part). */
      _AHIA(_P, SampleFreqFract,(_AHIA_Dummy+12)),		/* OM_NEW,
								   OM_GET */

      /* Maximum length (unit is sample frames). */
      _AHIA(_P, Capacity,	(_AHIA_Dummy+13)),		/* OM_NEW,
								   OM_GET */

      /* Current length (unit is sample frames). */
      _AHIA(_P, Length,	(_AHIA_Dummy+14)),			/* OM_NEW,
								   OM_SET,
								   OM_GET,
								   OM_NOTIFY */

      /* Buffer data pointer. */
	_AHIA(_P, Data,		(_AHIA_Dummy+15)),		/* OM_GET */

      /* High part of buffer timestamp (unit is sample frames <<
       * 32). */
      _AHIA(_P, TimestampHigh,(_AHIA_Dummy+16)),		/* OM_NEW,
								   OM_SET,
								   OM_GET */

      /* Low part of buffer timestamp (unit is sample frames). */
      _AHIA(_P, TimestampLow,	(_AHIA_Dummy+17)),		/* OM_NEW,
								   OM_SET,
								   OM_GET */

      /* High part of buffer age (unit is sample frames << 32). */
      _AHIA(_P, AgeHigh,	(_AHIA_Dummy+18)),		/* OM_NEW,
								   OM_SET,
								   OM_GET */

      /* Low part of buffer age (unit is sample frames). */
      _AHIA(_P, AgeLow,	(_AHIA_Dummy+19))			/* OM_NEW,
								   OM_SET,
								   OM_GET */
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

#endif /* CLASSES_AHI_BUFFER_H */