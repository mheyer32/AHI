
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

  struct TagItem* tstate = msg->ops_AttrList;
  struct TagItem* tag;
  ULONG result = 0;

  AHIClassData->sample_type = AHIST_NOTYPE;

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

      default:
	break;
    }
  }

  MethodUpdate(class, object, (struct opUpdate*) msg);

  GetAttr(AHIA_Error, object, &result);

  if (result != 0) {
    return result;
  }

  if (AHIClassData->sample_freq_int == 0 &&
      AHIClassData->sample_freq_fract == 0) {
    return AHIE_Buffer_InvalidSampleFreq;
  }
  
  if (AHIClassData->length == 0) {
    AHIClassData->length = AHIClassData->capacity;
  }

  if (AHIClassData->capacity == 0) {
    return AHIE_Buffer_InvalidCapacity;
  }
  else {
    ULONG size = DoMethod(object, AHIM_Buffer_SampleFrameSize,
			  AHIClassData->sample_type, AHIClassData->capacity, NULL);

    if (size == 0) {
      return AHIE_Buffer_InvalidSampleType;
    }
    
    AHIClassData->data = AllocVec(size, MEMF_PUBLIC | MEMF_CLEAR);
  }

  if (AHIClassData->data == NULL) {
    return ERROR_NO_FREE_STORE;
  }
  
  return AHIE_OK;
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
      case AHIA_Buffer_Length:
	if (tag->ti_Data < AHIClassData->capacity) {
	  AHIClassData->length = tag->ti_Data;
	  NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	}
	else {
//	  SetAttrs(object, AHIA_Error, AHIE_Buffer_InvalidLength, TAG_DONE);
	  SetSuperAttrs(class, object, AHIA_Error, AHIE_Buffer_InvalidLength, TAG_DONE);
	}
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

/******************************************************************************
** MethodSampleFrameSize ******************************************************
******************************************************************************/

ULONG
MethodSampleFrameSize(Class* class, Object* object,
		      struct AHIP_Buffer_SampleFrameSize* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  ULONG cnt = 0;
  
  switch (msg->SampleType & AHIST_T_MASK) {
    case AHIST_T_BYTE:
      cnt = 1;
      break;

    case AHIST_T_WORD:
      cnt = 2;
      break;

    case AHIST_T_LONG:
      cnt = 4;
      break;

    case AHIST_T_QUAD:
      cnt = 8;
      break;

    case AHIST_T_UBYTE:
      cnt = 1;
      break;

    case AHIST_T_UWORD:
      cnt = 2;
      break;

    case AHIST_T_ULONG:
      cnt = 4;
      break;

    case AHIST_T_UQUAD:
      cnt = 8;
      break;

    case AHIST_T_FLOAT:
      cnt = 4;
      break;

    case AHIST_T_DOUBLE:
      cnt = 8;
      break;
  }

  switch (msg->SampleType & AHIST_D_MASK) {
    case AHIST_D_DISCRETE:
    case AHIST_D_AMBISONIC:
    case AHIST_D_FOURIER:
      cnt *= AHIST_C_DECODE(msg->SampleType);
      cnt *= msg->Samples;
      break;

    case AHIST_D_COMPRESSED:
      // TODO: Fix later
      cnt = 0;
      break;

    default:
      cnt = 0;
      break;
  }

  return cnt;
}


/******************************************************************************
** MethodLoad *****************************************************************
******************************************************************************/

BOOL
MethodLoad(Class* class, Object* object, struct AHIP_Buffer_Load* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

#warning TODO: Implement AHIM_Buffer_Load
  
  return FALSE;
}


/******************************************************************************
** MethodClone ****************************************************************
******************************************************************************/

Object*
MethodClone(Class* class, Object* object, Msg msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  Object*  r;
  
  SetIoErr(0); // Make sure OM_NEW calls SetIoErr().

  r = NewObject(AHIClassBase->common.cl.cl_Class, NULL,
		AHIA_Buffer_SampleType,      AHIClassData->sample_type,
		AHIA_Buffer_SampleFreqInt,   AHIClassData->sample_freq_int,
		AHIA_Buffer_SampleFreqFract, AHIClassData->sample_freq_fract,
		AHIA_Buffer_Capacity,        AHIClassData->capacity,
		AHIA_Buffer_Length,          AHIClassData->length,
		AHIA_Buffer_TimestampHigh,   AHIClassData->timestamp_hi,
		AHIA_Buffer_TimestampLow,    AHIClassData->timestamp_lo,
		AHIA_Buffer_AgeHigh,         AHIClassData->age_hi,
		AHIA_Buffer_AgeLow,          AHIClassData->age_lo,
		TAG_DONE);

  if (r == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, IoErr(), TAG_DONE);
  }

  return r;
}
