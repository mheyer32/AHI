
#include <config.h>

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
  NewList((struct List*) &AHIClassData->buffers);

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

  // Make sure OM_REMMEMBER succeeds
  AHIClassData->busy = FALSE;
  
  while ((member = NextObject(&ostate)) != NULL) {
    DoMethod(object, OM_REMMEMBER, member);
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

	ULONG use_child_buffers = FALSE;
	ULONG history_samples = 0;

	Object* ostate = (Object*) AHIClassData->members.mlh_Head;
	Object* member;

	if (AHIClassData->busy) {
	  SetSuperAttrs(class, object,
			AHIA_Error, AHIE_Processor_ObjectBusy, TAG_DONE);
	  break;
	}
	
	GetAttr(AHIA_Processor_UseChildBuffers, object, &use_child_buffers);	
	GetAttr(AHIA_Processor_HistorySamples, object, &history_samples);

	// Propagate AHIA_Processor_Buffer to our children, if they
	// were operating on our current buffer too. Else, check if we
	// should create clone buffers.
	while ((member = NextObject(&ostate)) != NULL) {
	  Object* member_buffer = NULL;
	  
	  GetAttr(AHIA_Processor_Buffer, member, (ULONG*) &member_buffer);

	  if (member_buffer == AHIClassData->buffer) {
	    SetAttrs(member, AHIA_Processor_Buffer, tag->ti_Data, TAG_DONE);
	  }
	  else if (use_child_buffers) {
	    Object* buffer = NULL;

	    if ((Object*) tag->ti_Data != NULL) {
	      buffer = (Object*) DoMethod((Object*) tag->ti_Data,
					  AHIM_Buffer_Clone, history_samples);

	      if (buffer == NULL) {
		ULONG error = 0;
      
		GetAttr(AHIA_Error, (Object*) tag->ti_Data, &error);

		SetSuperAttrs(class, object, AHIA_Error, error, TAG_DONE);
	      }
	      else {
		// TODO: Add notification from new->cloned buffer?
		DoMethod(buffer, OM_ADDTAIL, &AHIClassData->buffers);
	      }
	    }
	    
	    // Unlink old Buffer object from our buffer list and dispose
	    DoMethod(member_buffer, OM_REMOVE);
	    DisposeObject(member_buffer);

	    // Install cloned buffer
	    SetAttrs(member, AHIA_Processor_Buffer, (ULONG) buffer, TAG_DONE);
	  }
	}

	// Remove us from the buffer's notify list
	if (AHIClassData->buffer != NULL) {
	  DoMethod(AHIClassData->buffer, AHIM_RemNotify, AHIClassData->ic);
	}

	// Install new buffer object
	AHIClassData->buffer = (Object*) tag->ti_Data;
	
	if (AHIClassData->buffer != NULL) {
	  // Add us to the new buffer's notify list
	  DoMethod(AHIClassData->buffer, AHIM_AddNotify, AHIClassData->ic);

	  // Fill in OM_UPDATE attributes
	  GetAttr(AHIA_Buffer_SampleType,      AHIClassData->buffer, &update_tags[0].ti_Data);
	  GetAttr(AHIA_Buffer_SampleFreqInt,   AHIClassData->buffer, &update_tags[1].ti_Data);
	  GetAttr(AHIA_Buffer_SampleFreqFract, AHIClassData->buffer, &update_tags[2].ti_Data);
	  GetAttr(AHIA_Buffer_Length,          AHIClassData->buffer, &update_tags[3].ti_Data);
	  GetAttr(AHIA_Buffer_Data,            AHIClassData->buffer, &update_tags[4].ti_Data);
	}

	// Send notify for AHIA_Processor_Buffer to our own notification list
	NotifySuper(class, object, msg,
		    AHIA_Processor_Buffer,       (ULONG) AHIClassData->buffer,
		    TAG_DONE);

	// Send AHIA_Buffer_* notifications to subclasses
	DoMethod(object, OM_UPDATE, (ULONG) update_tags, (ULONG) msg->opu_GInfo, 0);
	break;
      }

      case AHIA_Processor_Parent:
	if (AHIClassData->busy) {
	  SetSuperAttrs(class, object,
			AHIA_Error, AHIE_Processor_ObjectBusy, TAG_DONE);
	}
	else {
	  AHIClassData->parent = (Object*) tag->ti_Data;
	}
	break;

      case AHIA_Processor_Disabled:
	AHIClassData->disabled = tag->ti_Data;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	break;
	
      case AHIA_Processor_AddChild:
	if (msg->MethodID == OM_NEW) {
	  DoMethod(object, OM_ADDMEMBER, tag->ti_Data);
	}
	else {
	  SetSuperAttrs(class, object,
			AHIA_Error, AHIE_AttributeNotApplicable, TAG_DONE);
	}
	break;

      case AHIA_Processor_Busy:
	AHIClassData->busy = tag->ti_Data;
	break;

      case AHIA_Processor_Ready:
	AHIClassData->ready = tag->ti_Data;
	break;

      case AHIA_Processor_MaxChildren:
      case AHIA_Processor_NumChildren:
      case AHIA_Processor_UseChildBuffers:
      case AHIA_Processor_HistorySamples:
      case AHIA_Processor_ChildBuffers:
	SetSuperAttrs(class, object,
		      AHIA_Error, AHIE_AttributeNotApplicable, TAG_DONE);
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

  switch (msg->opg_AttrID) {
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
	
    case AHIA_Processor_AddChild:
      SetSuperAttrs(class, object,
		    AHIA_Error, AHIE_AttributeNotApplicable, TAG_DONE);
      return FALSE;

    case AHIA_Processor_Busy:
      *msg->opg_Storage = AHIClassData->busy;
      break;

    case AHIA_Processor_Ready:
      *msg->opg_Storage = AHIClassData->ready;
      break;

    case AHIA_Processor_MaxChildren:
      *msg->opg_Storage = 1;
      break;

    case AHIA_Processor_NumChildren:
      *msg->opg_Storage = AHIClassData->num_members;

    case AHIA_Processor_UseChildBuffers:
      *msg->opg_Storage = FALSE;
      break;

    case AHIA_Processor_HistorySamples:
      *msg->opg_Storage = 0;
      break;
      
    case AHIA_Processor_ChildBuffers:
      *msg->opg_Storage = (ULONG) &AHIClassData->buffers;
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
  ULONG   max_children      = 0;
  ULONG   parent            = 0;
  ULONG   owner             = 0;
  ULONG   my_owner          = 0;
  Object* buffer            = NULL;
  ULONG   use_child_buffers = FALSE;

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

  GetAttr(AHIA_Processor_UseChildBuffers, object, &use_child_buffers);

  if (use_child_buffers && AHIClassData->buffer != NULL) {
    ULONG history_samples = 0;

    GetAttr(AHIA_Processor_HistorySamples, object, &history_samples);

    buffer = (Object*) DoMethod(AHIClassData->buffer, AHIM_Buffer_Clone, history_samples);

    if (buffer == NULL) {
      ULONG error = 0;
      
      GetAttr(AHIA_Error, AHIClassData->buffer, &error);

      SetSuperAttrs(class, object, AHIA_Error, error, TAG_DONE);
      return FALSE;
    }
    else { 
      // TODO: Add notification from new->cloned buffer?
      DoMethod(buffer, OM_ADDTAIL, &AHIClassData->buffers);
    }
  }
  else {
    buffer = AHIClassData->buffer;
  }
  
  SetAttrs(msg->opam_Object,
	   AHIA_Processor_Parent, (ULONG) object,
	   AHIA_Processor_Buffer, (ULONG) buffer,
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
  Object* parent = NULL;
  Object* buffer = NULL;
  ULONG   use_child_buffers = FALSE;

  if (msg->opam_Object == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_NullMember, TAG_DONE);
    return FALSE;
  }

  if (AHIClassData->busy) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_ObjectBusy, TAG_DONE);
    return FALSE;
  }

  GetAttr(AHIA_Processor_Parent, msg->opam_Object, (ULONG*) &parent);

  if (parent != object) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_Processor_NotMember, TAG_DONE);
    return FALSE;
  }

  GetAttr(AHIA_Processor_Buffer, msg->opam_Object, (ULONG*) &buffer);
  
  SetAttrs(msg->opam_Object,
	   AHIA_Processor_Parent, (ULONG) NULL,
	   AHIA_Processor_Buffer, (ULONG) NULL,
	   TAG_DONE);

  --AHIClassData->num_members;
  DoMethod(msg->opam_Object, OM_REMOVE);

  GetAttr(AHIA_Processor_UseChildBuffers, object, &use_child_buffers);

  if (use_child_buffers && buffer != NULL) {
    // Unlink Buffer object from our buffer list
    DoMethod(buffer, OM_REMOVE);
    DisposeObject(buffer);
  }
  
  return TRUE;
}
