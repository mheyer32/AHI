
#include <classes/ahi/buffer.h>
#include <classes/ahi/processor.h>
#include <intuition/icclass.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  NewList((struct List*) &AHIClassData->members);

  AHIClassData->ic = NewObject(NULL, ICCLASS,
			       ICA_TARGET, (ULONG) object,
			       TAG_DONE);

  if (AHIClassData->ic == NULL) {
    return ERROR_NO_FREE_STORE;
  }

  MethodUpdate(class, object, (struct opUpdate*) msg);

  return 0;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  Object* ostate = (Object*) AHIClassData->members.mlh_Head;
  Object* member;

  while ((member = NextObject(&ostate)) != NULL) {
    DoMethod(member, OM_REMOVE);
    DoMethodA(member, msg);
  }

  if (AHIClassData->buffer != NULL && AHIClassData->ic != NULL) {
    DoMethod(AHIClassData->buffer, AHIM_RemNotify, AHIClassData->ic);
  }
  
  DisposeObject(AHIClassData->ic);
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg)
{
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Processor_Buffer: {
	struct TagItem update_tags[6] = {
	  { AHIA_Buffer_SampleType,      AHIST_NOTYPE },
	  { AHIA_Buffer_SampleFreqInt,   0            },
	  { AHIA_Buffer_SampleFreqFract, 0            },
	  { AHIA_Buffer_Length,          0            },
	  { AHIA_Buffer_Data,            0            },
	  { TAG_DONE,                    0            }
	};
	
	if (AHIClassData->buffer != NULL) {
	  DoMethod(AHIClassData->buffer, AHIM_RemNotify, AHIClassData->ic);
	}
	
	AHIClassData->buffer = (Object*) tag->ti_Data;
	
	if (AHIClassData->buffer != NULL) {
	  DoMethod(AHIClassData->buffer, AHIM_AddNotify, AHIClassData->ic);
	  
	  GetAttr(AHIA_Buffer_SampleType,      AHIClassData->buffer, &update_tags[0].ti_Data);
	  GetAttr(AHIA_Buffer_SampleFreqInt,   AHIClassData->buffer, &update_tags[1].ti_Data);
	  GetAttr(AHIA_Buffer_SampleFreqFract, AHIClassData->buffer, &update_tags[2].ti_Data);
	  GetAttr(AHIA_Buffer_Length,          AHIClassData->buffer, &update_tags[3].ti_Data);
	  GetAttr(AHIA_Buffer_Data,            AHIClassData->buffer, &update_tags[4].ti_Data);
	}
	
	NotifySuper(class, object, msg,
		    AHIA_Processor_Buffer,       (ULONG) AHIClassData->buffer,
		    TAG_DONE);
	DoMethod(object, OM_UPDATE, (ULONG) update_tags, (ULONG) msg->opu_GInfo, 0);
	break;
      }

      case AHIA_Processor_Parent:
	AHIClassData->parent = (Object*) tag->ti_Data;
	break;

      case AHIA_Processor_Disabled:
	AHIClassData->disabled = tag->ti_Data;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	break;
	
      case AHIA_Processor_AddChild: {
	struct opMember om = {
	  OM_ADDMEMBER, (Object*) tag->ti_Data
	};

	MethodAddMember(class, object, &om);
	break;
      }

      case AHIA_Processor_Busy:
	AHIClassData->busy = tag->ti_Data;
	break;

      case AHIA_Processor_Ready:
	AHIClassData->ready = tag->ti_Data;
	break;

      default:
	break;
    }
  }

  return 0;
}


/******************************************************************************
** MethodGet ******************************************************************
******************************************************************************/

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg)
{
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  switch (msg->opg_AttrID)
  {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Processor Base Class";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "The mother of all processor classes.";
      break;
      
    case AHIA_DescriptionURL:
      *msg->opg_Storage = (ULONG) "http://www.lysator.liu.se/ahi/";
      break;
      
    case AHIA_Author:
      *msg->opg_Storage = (ULONG) "Martin Blom";
      break;
      
    case AHIA_Copyright:
      *msg->opg_Storage = (ULONG) "©2004 Martin Blom";
      break;
      
    case AHIA_Version:
      *msg->opg_Storage = (ULONG) VERS;
      break;
      
    case AHIA_Annotation:
      *msg->opg_Storage = 0;
      break;
      
    case AHIA_Processor_Buffer:
      *msg->opg_Storage = (ULONG) AHIClassData->buffer;
      break;

    case AHIA_Processor_Parent:
      *msg->opg_Storage = (ULONG) AHIClassData->parent;
      break;

    case AHIA_Processor_Disabled:
      *msg->opg_Storage = AHIClassData->disabled;
      break;
	
    case AHIA_Processor_Busy:
      *msg->opg_Storage = AHIClassData->busy;
      break;

    case AHIA_Processor_Ready:
      *msg->opg_Storage = AHIClassData->ready;
      break;

    case AHIA_Processor_MaxChildren:
      *msg->opg_Storage = 1;
      break;

    default:
      return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** MethodPrepare **************************************************************
******************************************************************************/

BOOL
MethodPrepare(Class* class, Object* object, struct AHIP_Processor_Process* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  Object* ostate = (Object*) AHIClassData->members.mlh_Head;
  Object* member;

  BOOL result = FALSE;

  if (!AHIClassData->busy || !AHIClassData->ready) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_ObjectNotReady, TAG_DONE);
    return FALSE;
  }
  
  while ((member = NextObject(&ostate)) != NULL) {
    result |= DoMethodA(member, (Msg) msg);
  }

  return result != FALSE;
}


/******************************************************************************
** MethodProcess **************************************************************
******************************************************************************/

ULONG
MethodProcess(Class* class, Object* object, struct AHIP_Processor_Process* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  Object* ostate = (Object*) AHIClassData->members.mlh_Head;
  Object* member;

  ULONG result = AHIV_Processor_PerformProc;
  
  if (!AHIClassData->busy || !AHIClassData->ready) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_ObjectNotReady, TAG_DONE);
    return AHIV_Processor_FailProc;
  }
  
  while ((member = NextObject(&ostate)) != NULL) {
    if (DoMethodA(member, (Msg) msg) == AHIV_Processor_FailProc) {
      result = AHIV_Processor_FailProc;
      break;
    }
  }

  if (result == AHIV_Processor_PerformProc && AHIClassData->disabled) {
    return AHIV_Processor_SkipProc;
  }
  else {
    return result;
  }
}


/******************************************************************************
** MethodAddMember ************************************************************
******************************************************************************/

BOOL
MethodAddMember(Class* class, Object* object, struct opMember* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG max_children = 0;
  ULONG parent       = 0;
  ULONG owner        = 0;
  ULONG my_owner     = 0;

  if (msg->opam_Object == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_NullMember, TAG_DONE);
    return FALSE;
  }

  if (AHIClassData->busy) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_ObjectBusy, TAG_DONE);
    return FALSE;
  }

  GetAttr(AHIA_Processor_MaxChildren, object, &max_children);

  if (AHIClassData->num_members >= max_children) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_TooManyChildren, TAG_DONE);
    return FALSE;
  }

  GetAttr(AHIA_Processor_Parent, msg->opam_Object, &parent);

  if (parent != 0) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_MultipleParents, TAG_DONE);
    return FALSE;
  }

  GetAttr(AHIA_Owner, msg->opam_Object, &owner);
  GetAttr(AHIA_Owner, object, &my_owner);

  if (owner != my_owner) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_DifferentOwner, TAG_DONE);
    return FALSE;
  }
  
  SetAttrs(msg->opam_Object,
	   AHIA_Processor_Parent, (ULONG) object,
	   AHIA_Processor_Buffer, (ULONG) AHIClassData->buffer,
	   TAG_DONE);
  
  ++AHIClassData->num_members;
  DoMethod(msg->opam_Object, OM_ADDTAIL, &AHIClassData->members);
  return TRUE;
}


/******************************************************************************
** MethodRemMember ************************************************************
******************************************************************************/

BOOL
MethodRemMember(Class* class, Object* object, struct opMember* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG parent = 0;

  if (msg->opam_Object == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_NullMember, TAG_DONE);
    return FALSE;
  }

  if (AHIClassData->busy) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_ObjectBusy, TAG_DONE);
    return FALSE;
  }

  GetAttr(AHIA_Processor_Parent, msg->opam_Object, &parent);

  if (parent != object) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_NotMember, TAG_DONE);
    return FALSE;
  }

  SetAttrs(msg->opam_Object,
	   AHIA_Processor_Parent, (ULONG) NULL,
	   AHIA_Processor_Buffer, (ULONG) NULL,
	   TAG_DONE);

  --AHIClassData->num_members;
  DoMethod(msg->opam_Object, OM_REMOVE);
  return TRUE;
}
