
#include <classes/ahi.h>
#include <classes/ahi/buffer.h>
#include <classes/ahi/processor.h>
#include <intuition/icclass.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "util.h"
#include "version.h"

static char*
get_error_message(struct AHIClassData* AHIClassData) {
  switch (AHIClassData->error) {
    case AHIE_OK:
      return "No error";

    case AHIE_Buffer_InvalidSampleType:
      return "Invalid sample type";

    case AHIE_Buffer_InvalidSampleFreq:
      return "Invalid sample frequency";

    case AHIE_Buffer_InvalidCapacity:
      return "Invalid buffer capacity";

    case AHIE_Buffer_InvalidLength:
      return "Invalid buffer length";

    case AHIE_Processor_ObjectNotReady:
      return "Object is not ready";

    case AHIE_Processor_ObjectBusy:
      return "Object is busy";
	  
    default:
      if (Fault(AHIClassData->error,
		NULL,
		AHIClassData->error_message,
		sizeof (AHIClassData->error_message)) > 0) {
	return  AHIClassData->error_message;
      }
  }

  return "Unknown error";
}


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

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
	AHIClassData->locale = (struct Locale*) tag->ti_Data;
	break;

      case AHIA_Owner:
	AHIClassData->owner = (Object*) tag->ti_Data;
	break;

      case AHIA_InterruptSafe:
	AHIClassData->interrupt_safe = tag->ti_Data;
	break;

      case AHIA_UserData:
	AHIClassData->user_data = tag->ti_Data;
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
	KPrintF("Unknown NEW attribute in " _AHI_CLASS_NAME ": %08lx, %08lx\n",
		tag->ti_Tag, tag->ti_Data);
	break;
    }
  }

  // The owner dictates the InterruptSafe flag
  if (AHIClassData->owner != NULL) {
    GetAttr(AHIA_InterruptSafe, AHIClassData->owner,
	    &AHIClassData->interrupt_safe);
  }
    
  AHIClassData->model_class = NewObjectA(NULL, MODELCLASS, model_tags);

  if (AHIClassData->model_class == NULL) {
    return ERROR_NO_FREE_STORE;
  }

  InitSemaphore(&AHIClassData->semaphore);

  return 0;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  DisposeObject(AHIClassData->model_class);
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Locale:
	AHIClassData->locale = (struct Locale*) tag->ti_Data;
	break;

      case AHIA_Owner:
	AHIClassData->owner = (Object*) tag->ti_Data;

	// The owner dictates the InterruptSafe flag
	if (AHIClassData->owner != NULL) {
	  GetAttr(AHIA_InterruptSafe, AHIClassData->owner,
		  &AHIClassData->interrupt_safe);
	}
	
	break;

      case AHIA_InterruptSafe:
	if (AHIClassData->owner == NULL) {
	  AHIClassData->interrupt_safe = tag->ti_Data;
	}
	break;
 
      case AHIA_Error:
	AHIClassData->error = tag->ti_Data;
	notify(object, msg, tag->ti_Tag, tag->ti_Data);
	break;

      case AHIA_UserData:
	AHIClassData->user_data = tag->ti_Data;
	notify(object, msg, tag->ti_Tag, tag->ti_Data);
	break;

      case ICA_TARGET:
      case ICA_MAP:
      case ICSPECIAL_CODE: {
	struct TagItem tl[] = {
	  { tag->ti_Tag, tag->ti_Data },
	  { TAG_DONE,    0            }
	};
	
	DoMethod(AHIClassData->model_class, msg->MethodID,
		 (ULONG) tl, (ULONG) msg->opu_GInfo,
		 msg->MethodID == OM_UPDATE ? msg->opu_Flags : 0 );
	break;
      }

      default:
	KPrintF("Unknown SET attribute in " _AHI_CLASS_NAME ": %08lx, %08lx\n",
		tag->ti_Tag, tag->ti_Data);
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
      *msg->opg_Storage = (ULONG) AHIClassData->locale;
      break;

    case AHIA_Owner:
      *msg->opg_Storage = (ULONG) AHIClassData->owner;
      break;

    case AHIA_InterruptSafe:
      *msg->opg_Storage = AHIClassData->interrupt_safe;
      break;

    case AHIA_Error:
      *msg->opg_Storage = AHIClassData->error;
      break;

    case AHIA_ErrorMessage:
      *msg->opg_Storage = get_error_message(AHIClassData);
      break;
      
    case AHIA_UserData:
      *msg->opg_Storage = AHIClassData->user_data;
      break;

    case AHIA_ParameterArray:
    case AHIA_Parameters:
      *msg->opg_Storage = 0;
      break;
      
    case ICA_TARGET:
    case ICA_MAP:
    case ICSPECIAL_CODE:
      GetAttr(msg->opg_AttrID, AHIClassData->model_class, msg->opg_Storage);
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  if (AHIClassData->interrupt_safe) {
    Disable();
  }
  else if (AHIClassData->owner != NULL) {
    DoMethodA(AHIClassData->owner, msg);
  }
  else {
    ObtainSemaphore(&AHIClassData->semaphore);
  }
}


/******************************************************************************
** MethodUnlock ***************************************************************
******************************************************************************/

void
MethodUnlock(Class* class, Object* object, Msg msg)
{
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  if (AHIClassData->interrupt_safe) {
    Enable();
  }
  else if (AHIClassData->owner != NULL) {
    DoMethodA(AHIClassData->owner, msg);
  }
  else {
    ReleaseSemaphore(&AHIClassData->semaphore);
  }
}
