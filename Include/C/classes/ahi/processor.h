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
#define AHIC_Processor	"ahi-processor.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
  namespace Processor {
#else
# define _P _Processor
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

/*****************************************************************************/

    enum {
      
      /* The buffer object we're operating on. Note that subclasses of
	 ahi-processor.class will receive OM_UPDATE messages for
	 AHIA_Buffer_SampleType, AHIA_Buffer_SampleFreqInt,
	 AHIA_Buffer_SampleFreqFract, AHIA_Buffer_Length and
	 AHIA_Buffer_Data when this attribute is changed. The
	 ahi-processor will also be added to the buffers' notification
	 list automatically.
      */
      _AHIA(_P, Buffer,		(_AHIA_Dummy+20)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

      /* Pointer to parent processor. */
      _AHIA(_P, Parent,		(_AHIA_Dummy+21)),	/* OM_NEW,
							   OM_SET,
							   OM_GET */

      /* The maximum number of children. */
      _AHIA(_P, MaxChildren,	(_AHIA_Dummy+82)),	/* OM_GET */

      
      /* Disable processing (pass-through) when TRUE. */
      _AHIA(_P, Disabled,	(_AHIA_Dummy+22)),	/* OM_NEW,
							   OM_SET,
							   OM_GET,
							   OM_NOTIFY */

      /* Add a child processor object.  This special attribute can be
	 specified more than once during object creation to add
	 several child objects, as if they were added by
	 OM_ADDMEMBER. */
      _AHIA(_P, AddChild,	(_AHIA_Dummy+23)),	/* OM_NEW */

      /* When TRUE, this object is busy and no children can be added
	 or removed, nor can this object be removed from its
	 parent. When FALSE, the AHIM_Processor_Prepare and
	 AHIM_Processor_Process methods cannot be called
	 (ahi-processor will return AHIV_Processor_FailProc). This
	 attribute should not be set by the user or subclasses. */
      _AHIA(_P, Busy,		(_AHIA_Dummy+24)),	/* OM_SET,
							   OM_GET */

      /* This attribute should be set to TRUE by the subclass when
	 it's ready to handle AHIM_Processor_Prepare/
	 AHIM_Processor_Process calls. If FALSE, ahi-processor will
	 return AHIV_Processor_FailProc. */
      _AHIA(_P, Ready,		(_AHIA_Dummy+81))	/* OM_SET,
							   OM_GET */
    };

/*****************************************************************************/

     enum {
       
       /* Prepare to process buffer. Returns TRUE if something happened. */
       _AHIM(_P, Prepare,	(_AHIM_Dummy+9)),

       /* Process buffer. See below for return values. */
       _AHIM(_P, Process,	(_AHIM_Dummy+10))
     };

     /* Additional supported methods, except those inherited from
	superclass: 

	OM_ADDMEMBER Add another processor object to the process
		     chain.  The object will be disposed when this
		     object is disposed. Returns TRUE on success.

        OM_REMMEMBER Remove another processor object from the process
                     chain. Returns TRUE on success.
     */

/*****************************************************************************/

     struct _AHIP(_P, Process) {
       ULONG	MethodID;
       ULONG	Flags;			/* No flags defined yet */
       UQUAD	CurrentTime;
     };

     /* Return value */

     enum {
       _AHIV(_P, FailProc,		(0)),
       _AHIV(_P, SkipProc,		(1)),
       _AHIV(_P, PerformProc,		(2)),
     };
     
/*****************************************************************************/

     enum {
       _AHIE(_P, ObjectNotReady,		(_AHIE_Dummy+5)),
       _AHIE(_P, ObjectBusy,			(_AHIE_Dummy+6)),
       _AHIE(_P, TooManyChildren,		(_AHIE_Dummy+9))
     };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
  }
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */
 
#endif /* CLASSES_AHI_PROCESSOR_H */
