#ifndef CLASSES_AHI_H
#define CLASSES_AHI_H

/*
**	$VER: ahi.h 7.0 (2.7.2003)
**
**	ahi.class definitions
**
**	(C) Copyright 2002 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef INTUITION_CLASSES_H
#include <intuition/classes.h>
#endif

#ifndef INTUITION_CLASSUSR_H
#include <intuition/classusr.h>
#endif

#ifndef CLASSES_AHI_TYPES_H
#include <classes/ahi_internal.h>
#endif

#ifndef DEVICES_AHI_TYPES_H
#include <classes/ahi_types.h>
#endif

struct IFFHandle;

/* This class inherits "rootclass". */
#define AHIC_AHI		"ahi.class"

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
#else
# define _P
#endif

/*****************************************************************************/

  enum {
    
    /* Attributes that describe the class/object. */
    _AHIA(_P, Title,		(_AHIA_Dummy+0)),		/* OM_GET */
    _AHIA(_P, Description,	(_AHIA_Dummy+1)),		/* OM_GET */
    _AHIA(_P, DescriptionURL,	(_AHIA_Dummy+2)),		/* OM_GET */
    _AHIA(_P, Author,		(_AHIA_Dummy+3)),		/* OM_GET */
    _AHIA(_P, Copyright,	(_AHIA_Dummy+4)),		/* OM_GET */
    _AHIA(_P, Version,		(_AHIA_Dummy+5)),		/* OM_GET */
    _AHIA(_P, Annotation,	(_AHIA_Dummy+6)),		/* OM_GET */

    /* Locale to use for strings. */
    _AHIA(_P, Locale,		(_AHIA_Dummy+7)),		/* OM_NEW,
								   OM_SET,
								   OM_GET */

    /* The owner of this object, if any. Interrupt unsafe locking will
     * use this objects lock instead of a global lock. The owner also
     * determines if interrupt safe locking is in use by default. */
    _AHIA(_P, Owner,		(_AHIA_Dummy+8)),		/* OM_NEW,
								   OM_SET,
								   OM_GET */
   
    /* Use interrupt-safe locking (see AHIM_Lock/AHIM_Unlock). */
    _AHIA(_P, InterruptSafe,	(_AHIA_Dummy+9)),		/* OM_NEW,
								   OM_SET,
								   OM_GET */

    /* Whenever an error occurs, this attribute is updated */
    _AHIA(_P, Error,		(_AHIA_Dummy+41)),		/* OM_SET,
								   OM_GET,
								   OM_NOTIFY */


    /* Whenever AHIA_Error is changed, this attribute is updated
     * too */
    _AHIA(_P, ErrorMessage,	(_AHIA_Dummy+42)),		/* OM_GET */

    /* The user is free to store whatever (s)he likes here */
    _AHIA(_P, UserData,		(_AHIA_Dummy+67)),		/* OM_NEW,
								   OM_SET,
								   OM_GET,
								   OM_NOTIFY */

    /* All parameters for this class */
    _AHIA(_P, ParameterArray,	(_AHIA_Dummy+79)),		/* OM_GET */

    /* The number of parametes for this class */
    _AHIA(_P, Parameters,	(_AHIA_Dummy+80))		/* OM_GET */
    
    /* Other supported attributes: 

    ICA_TARGET	Forwarded to the internal modelclass object.

    ICA_MAP	Forwarded to the internal modelclass object.

    */

  };
    
/*****************************************************************************/

  enum {

    /* Lock this object -- mostly used by subclasses. */
    _AHIM(_P, Lock,		(_AHIM_Dummy+0)),

    /* Unlock this object -- mostly used by subclasses. */
    _AHIM(_P, Unlock,		(_AHIM_Dummy+1)),

    /* Create a settings gadget. */
    _AHIM(_P, NewSettingsGadget,(_AHIM_Dummy+2)),

    /* Open a settings requester */
    _AHIM(_P, OpenSettingsReq,	(_AHIM_Dummy+3)),

    /* Close (cancel) a settings requester */
    _AHIM(_P, CloseSettingsReq,	(_AHIM_Dummy+4)),

    /* Serialize object to an IFF stream */
    _AHIM(_P, Serialize,		(_AHIM_Dummy+5)),

    /* Unserialize object from an IFF stream */
    _AHIM(_P, Unserialize,	(_AHIM_Dummy+6)),
    
/*     /\* Create a dispatcher hook (no valid object required) *\/ */
/*     _AHIM(_P, NewDispatcher,	(_AHIM_Dummy+7)), */
    
/*     /\* Dispose a dispatcher hook *\/ */
/*     _AHIM(_P, DisposeDispatcher,(_AHIM_Dummy+8)), */

    /* Add notification: Will be forwarded as OM_ADDMEMBER to the
     * internal modelclass object */
    _AHIM(_P, AddNotify,	(_AHIM_Dummy+18)),

    /* Remove notification: Will be forwarded as OM_REMMEMBER to the
     * internal modelclass object */
    _AHIM(_P, RemNotify,	(_AHIM_Dummy+19)),

    /* Retrieve a custom parameter value */
    _AHIM(_P, GetParamValue,	(_AHIM_Dummy+43))
  };

/*****************************************************************************/

  /* AHIM_Serialize */
  struct _AHIP(_P, Serialize) {
    ULONG		MethodID;
    ULONG		Flags;
    struct IFFHandle*	IFFHandle;
  };

  enum {
    _AHIB(_P, SerializeRecursive,	(0))
  };


/*   /\* AHIM_NewDispatcher *\/ */
/*   struct _AHIP(_P, DispatcherEntry) { */
/*     ULONG	ID; */
/*     ULONG	(*FunctionPtr)(Class*  cl, */
/* 			       Object* obj, */
/* 			       Msg     msg); */
/*   }; */

/*   struct _AHIP(_P, NewDispatcher) { */
/*     ULONG				MethodID; */
/*     Class*                              Class; */
/*     APTR                                HookEntry; */
/*     struct _AHIP(_P, DispatcherEntry)*	Methods; */
/*     struct _AHIP(_P, DispatcherEntry)*	SetAttributes; */
/*     struct _AHIP(_P, DispatcherEntry)*	GetAttributes; */
/*   }; */

/*   /\* AHIM_DisposeDispatcher *\/ */
/*   struct _AHIP(_P, DisposeDispatcher) { */
/*       ULONG		MethodID; */
/*       struct Hook*	Dispatcher; */
/*   }; */

  /* AHIM_AddNotify/AHIM_RemNotify */
  struct _AHIP(_P, Notify) {
      ULONG		MethodID;
      Object*		Obj;
  };
  
  /* AHIM_GetParamValue */
  struct _AHIP(_P, ParamValue) {
      ULONG		MethodID;
      ULONG		Attrib;
      ULONG		Value;
  };
  
  /* AHIA_ParameterArray */
  struct _AHIP(_P, Param) {
    CONST_STRPTR	Name;
    CONST_STRPTR	Unit;
    UBYTE		Type;
    UBYTE		Format;
    UWORD		Flags;
    ULONG		Attrib;
    LONG		Min;
    LONG		Max;
    LONG		Step;
  };

  enum {
    _AHIV(_P, Param_Type_TEXT,		(0)),
    _AHIV(_P, Param_Type_INTEGER,	(1)),
    _AHIV(_P, Param_Type_FLOAT,		(2)),
    _AHIV(_P, Param_Type_SLIDER,	(3)),
    _AHIV(_P, Param_Type_CHOICE,	(4))
  };

 enum {
   _AHIV(_P, Param_Format_STRING,	(0)),
   _AHIV(_P, Param_Format_INTEGER,	(1)),
   _AHIV(_P, Param_Format_FIXED,	(2)),
   _AHIV(_P, Param_Format_FIXED28,	(3)),
   _AHIV(_P, Param_Format_DBFIXED,	(4))
};

/*****************************************************************************/

 enum {
   _AHIE(_P, OK,			(0))
 };

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
}
#else
# undef _P
#endif /* __cplusplus && !AHI_NO_NAMESPACES */

#endif /* CLASSES_AHI_H */
