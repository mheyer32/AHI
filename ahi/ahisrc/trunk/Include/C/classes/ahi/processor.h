#ifndef CLASSES_AHI_PROCESSOR_H
#define CLASSES_AHI_PROCESSOR_H

/*
**	$VER: processor.h 7.0 (2.7.2003)
**
**	ahi-processor.class definitions
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

/* This class inherits "ahi.class". */
#define AHI_PROCESSOR_CLASS	"ahi-processor.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
#else
# define _P _Processor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {
      
      /* The buffer object we're operating on. */
      _AHIA(_P, Buffer,		(_AHIA_Dummy+20)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

      /* Pointer to parent processor. */
      _AHIA(_P, Parent,		(_AHIA_Dummy+21)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */


      /* Disable processing (pass-through) when TRUE. */
      _AHIA(_P, Disabled,	(_AHIA_Dummy+22)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */

      /* Add a child processor object.  This special attribute can be
	 specified more than once during object creation to add
	 several child objects, as if they were added by
	 OM_ADDMEMBER. */
      _AHIA(_P, AddChild,	(_AHIA_Dummy+23)),	/* OM_NEW */

      /* When TRUE, this object is busy and no children can be added
	 or removed, nor can this object be removed from its
	 parent. When FALSE, the AHIPM_PREPARE and AHIPM_PROCESS
	 methods cannot be called. */

      _AHIA(_P, Busy,		(_AHIA_Dummy+24))	/* OM_SET,
							   OM_GET */

    };

/*****************************************************************************/

     enum {
       
       /* Prepare to process buffer. */
       _AHIM(_P, Prepare,	(_AHIM_Dummy+9)),

       /* Process buffer. */
       _AHIM(_P, Process,	(_AHIM_Dummy+10))
     };

     /* Additional supported methods, except those inherited from
	superclass: 

	OM_ADDMEMBER Add another processor object to the process
		     chain.  The object will be disposed when this
		     object is disposed.

        OM_REMMEMBER Remove another processor object from the process
                     chain.
     */

/*****************************************************************************/

     struct _AHIP(_P, Process) {
       ULONG	MethodID;
       ULONG	Flags;			/* No flags defined yet */
       UQUAD	CurrentTime;
     };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */
 
#endif /* CLASSES_AHI_PROCESSOR_H */
