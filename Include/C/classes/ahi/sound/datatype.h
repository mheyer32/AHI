#ifndef CLASSES_AHI_SOUND_DATATYPE_H
#define CLASSES_AHI_SOUND_DATATYPE_H

/*
**	$VER: datatype.h 7.0 (2.7.2003)
**
**	datatype.ahi-sound definitions
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
#define AHIC_DatatypeSound	"datatype.ahi-sound"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Sound {
    namespace Datatype {
#else
# define _P _Sound_Datatype
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

      enum {
	
	/* The datatype object to read sample data from. */
	_AHIA(_P, Datatype,	(_AHIA_Dummy+28)),	/* OM_NEW,
							   OM_GET */

	/* Make a local copy of the sample data */
	_AHIA(_P, CopyData,	(_AHIA_Dummy+29))	/* OM_NEW */

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

#endif /* CLASSES_AHI_SOUND_DATATYPE_H */
