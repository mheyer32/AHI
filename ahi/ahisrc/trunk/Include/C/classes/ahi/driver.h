#ifndef CLASSES_AHI_DRIVER_H
#define CLASSES_AHI_DRIVER_H

/*
**	$VER: driver.h 7.0 (2.7.2003)
**
**	ahi-driver.class definitions
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
#define AHIC_Driver		"ahi-driver.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Driver {
#else
# define _P _Driver
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {
      
      /* The number of boards found */
      _AHIA(_P, Boards,		(_AHIA_Dummy+43)),		/* OM_GET,
								   OM_NOTIFY */

      /* A pointer to the board IClass */
      _AHIA(_P, BoardClass,	(_AHIA_Dummy+44)),		/* OM_GET */

      /* The audio mode ID base number (Martin Blom <martin@blom.org>
       * allocates these) */
      _AHIA(_P, ID,		(_AHIA_Dummy+45))		/* OM_GET */

    };

/*****************************************************************************/

    enum {
      
      /* Like NewObjectA() -- allows the driver to insert its own
       * hardware-accelerated objects */
      _AHIM(_P, NewObject,	(_AHIM_Dummy+17))

    };

/*****************************************************************************/

    struct _AHIP(_P, NewObject) {
      ULONG           MethodID;
      struct IClass*  Class;
      struct TagItem* TagList;
    };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_DRIVER_H */
