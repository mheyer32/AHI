
#include <classes/ahi/buffer.h>
#include <intuition/icclass.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
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
  
  MethodUpdate(class, object, (struct opUpdate*) msg);

  if (AHIClassData->capacity > 0) {
    AHIClassData->data = AllocVec(AHIClassData->capacity,
				  MEMF_PUBLIC | MEMF_CLEAR);
  }

  if (AHIClassData->data == NULL) {
    return ERROR_NO_FREE_STORE;
  }
  
  return 0;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  FreeVec(AHIClassData->data);
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
      case AHIA_Buffer_SampleType:
	AHIClassData->sample_type = tag->ti_Data;
	break;

      case AHIA_Buffer_SampleFreqInt:
	AHIClassData->sample_freq_int = tag->ti_Data;
	break;

      case AHIA_Buffer_SampleFreqFract:
	AHIClassData->sample_freq_fract = tag->ti_Data;
	break;
	
      case AHIA_Buffer_Capacity:
	AHIClassData->capacity = tag->ti_Data;
	break;

      case AHIA_Buffer_Length:
	AHIClassData->length = tag->ti_Data;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data);
	break;

      case AHIA_Buffer_TimestampHigh:
	AHIClassData->timestamp_hi = tag->ti_Data;
	break;

      case AHIA_Buffer_TimestampLow:
	AHIClassData->timestamp_lo = tag->ti_Data;
	break;
	
      case AHIA_Buffer_AgeHigh:
	AHIClassData->age_hi = tag->ti_Data;
	break;

      case AHIA_Buffer_AgeLow:
	AHIClassData->age_lo = tag->ti_Data;
	break;

      default:
	KPrintF("Unknown NEW/SET attribute in " _AHI_CLASS_NAME ": %08lx, %08lx\n",
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
      *msg->opg_Storage = (ULONG) "AHI Audio Buffer Class";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "An audio buffer.";
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
      
    case AHIA_Buffer_SampleType:
      *msg->opg_Storage = AHIClassData->sample_type;
      break;

    case AHIA_Buffer_SampleFreqInt:
      *msg->opg_Storage = AHIClassData->sample_freq_int;
      break;

    case AHIA_Buffer_SampleFreqFract:
      *msg->opg_Storage = AHIClassData->sample_freq_fract;
      break;
	
    case AHIA_Buffer_Capacity:
      *msg->opg_Storage = AHIClassData->capacity;
      break;

    case AHIA_Buffer_Length:
      *msg->opg_Storage = AHIClassData->length;
      break;

    case AHIA_Buffer_Data:
      *msg->opg_Storage = (ULONG) AHIClassData->data;
      break;
      
    case AHIA_Buffer_TimestampHigh:
      *msg->opg_Storage = AHIClassData->timestamp_hi;
      break;

    case AHIA_Buffer_TimestampLow:
      *msg->opg_Storage = AHIClassData->timestamp_lo;
      break;
	
    case AHIA_Buffer_AgeHigh:
      *msg->opg_Storage = AHIClassData->age_hi;
      break;

    case AHIA_Buffer_AgeLow:
      *msg->opg_Storage = AHIClassData->age_lo;
      break;
	
    default:
      return FALSE;
  }

  return TRUE;
}
