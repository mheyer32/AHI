
#include <config.h>

#include <classes/ahi.h>
#include <intuition/icclass.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "errors.h"
#include "util.h"
#include "version.h"


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  struct TagItem model_tags[] = {
    { ICA_TARGET,     0 },
    { ICA_MAP,        0 },
    { ICSPECIAL_CODE, 0 },
    { TAG_DONE,       0 }
  };
  
  struct TagItem* tag;
  struct TagItem* tstate = msg->ops_AttrList;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Locale:
	ObjectData->locale = (struct Locale*) tag->ti_Data;
	break;

      case AHIA_Owner:
	ObjectData->owner = (Object*) tag->ti_Data;
	break;

      case AHIA_InterruptSafe:
	ObjectData->interrupt_safe = tag->ti_Data;
	break;

      case AHIA_UserData:
	ObjectData->user_data = tag->ti_Data;
	break;

      case ICA_TARGET:
	model_tags[0].ti_Data = tag->ti_Data;
	break;

      case ICA_MAP:
	model_tags[1].ti_Data = tag->ti_Data;
	break;

      case ICSPECIAL_CODE:
	model_tags[2].ti_Data = tag->ti_Data;
	break;

      default:
	break;
    }
  }

  // The owner dictates the InterruptSafe flag
  if (ObjectData->owner != NULL) {
    GetAttr(AHIA_InterruptSafe, ObjectData->owner,
	    &ObjectData->interrupt_safe);
  }
    
  ObjectData->model_class = NewObjectA(NULL, MODELCLASS, model_tags);

  tstate = msg->ops_AttrList;

  while ((tag = NextTagItem(&tstate))) {
    if (tag->ti_Tag == AHIA_AddNotify) {
      if (ObjectData->model_class == NULL) {
	DoMethod((Object*) tag->ti_Data, OM_DISPOSE);
      }
      else {
	DoMethod(ObjectData->model_class, OM_ADDMEMBER, tag->ti_Data);
      }
    }
  }

  if (ObjectData->model_class == NULL) {
    return ERROR_NO_FREE_STORE;
  }
  
  InitSemaphore(&ObjectData->semaphore);

  return 0;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  DisposeObject(ObjectData->model_class);
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/

static void
notify(Object* object, struct opUpdate* msg, ULONG tag, ULONG data) {
  struct TagItem tl[] = {
    { tag,      data },
    { TAG_DONE, 0    }
  };

  DoMethod(object, OM_NOTIFY, (ULONG) tl, (ULONG) msg->opu_GInfo,
	   msg->MethodID == OM_UPDATE ? msg->opu_Flags : 0 );
}

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg)
{
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Locale:
	ObjectData->locale = (struct Locale*) tag->ti_Data;
	break;

      case AHIA_Owner:
	ObjectData->owner = (Object*) tag->ti_Data;

	// The owner dictates the InterruptSafe flag
	if (ObjectData->owner != NULL) {
	  GetAttr(AHIA_InterruptSafe, ObjectData->owner,
		  &ObjectData->interrupt_safe);
	}
	
	break;

      case AHIA_InterruptSafe:
	if (ObjectData->owner == NULL) {
	  ObjectData->interrupt_safe = tag->ti_Data;
	}
	break;
 
      case AHIA_Error:
	ObjectData->error = tag->ti_Data;
	notify(object, msg, tag->ti_Tag, tag->ti_Data);
	break;

      case AHIA_UserData:
	ObjectData->user_data = tag->ti_Data;
	notify(object, msg, tag->ti_Tag, tag->ti_Data);
	break;

      case ICA_TARGET:
      case ICA_MAP:
      case ICSPECIAL_CODE: {
	struct TagItem tl[] = {
	  { tag->ti_Tag, tag->ti_Data },
	  { TAG_DONE,    0            }
	};
	
	DoMethod(ObjectData->model_class, msg->MethodID,
		 (ULONG) tl, (ULONG) msg->opu_GInfo,
		 msg->MethodID == OM_UPDATE ? msg->opu_Flags : 0 );
	break;
      }

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
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  switch (msg->opg_AttrID) {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Root Class";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "The mother of all AHI classes.";
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

    case AHIA_Locale:
      *msg->opg_Storage = (ULONG) ObjectData->locale;
      break;

    case AHIA_Owner:
      *msg->opg_Storage = (ULONG) ObjectData->owner;
      break;

    case AHIA_InterruptSafe:
      *msg->opg_Storage = ObjectData->interrupt_safe;
      break;

    case AHIA_Error:
      *msg->opg_Storage = ObjectData->error;
      break;

    case AHIA_ErrorMessage:
      *msg->opg_Storage = (ULONG) get_error_message(ObjectData);
      break;
      
    case AHIA_UserData:
      *msg->opg_Storage = ObjectData->user_data;
      break;

    case AHIA_ParameterTags:
    case AHIA_Parameters:
      *msg->opg_Storage = 0;
      break;
      
    case ICA_TARGET:
    case ICA_MAP:
    case ICSPECIAL_CODE:
      GetAttr(msg->opg_AttrID, ObjectData->model_class, msg->opg_Storage);
      break;
	
    default:
      return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** MethodLock *****************************************************************
******************************************************************************/

void
MethodLock(Class* class, Object* object, Msg msg)
{
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  if (ObjectData->interrupt_safe) {
    Disable();
  }
  else if (ObjectData->owner != NULL) {
    DoMethodA(ObjectData->owner, msg);
  }
  else {
    ObtainSemaphore(&ObjectData->semaphore);
  }
}


/******************************************************************************
** MethodUnlock ***************************************************************
******************************************************************************/

void
MethodUnlock(Class* class, Object* object, Msg msg)
{
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  if (ObjectData->interrupt_safe) {
    Enable();
  }
  else if (ObjectData->owner != NULL) {
    DoMethodA(ObjectData->owner, msg);
  }
  else {
    ReleaseSemaphore(&ObjectData->semaphore);
  }
}
