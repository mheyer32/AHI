
#include <config.h>

#include <classes/ahi/buffer.h>
#include <intuition/icclass.h>

#include <clib/alib_protos.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <string.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"

/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->ops_AttrList;
  struct TagItem* tag;
  ULONG result = 0;

  ObjectData->sample_type = AHIST_NOTYPE;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Buffer_SampleType:
	ObjectData->sample_type = tag->ti_Data;
	break;

      case AHIA_Buffer_SampleFreqInt:
	ObjectData->sample_freq_int = tag->ti_Data;
	break;

      case AHIA_Buffer_SampleFreqFract:
	ObjectData->sample_freq_fract = tag->ti_Data;
	break;
	
      case AHIA_Buffer_Capacity:
	ObjectData->capacity = tag->ti_Data;
	break;
 
      case AHIA_Buffer_PreLength:
	ObjectData->pre_length = tag->ti_Data;
	break;
	
     default:
	break;
    }
  }

  ObjectData->prelen_size = DoMethod(object, AHIM_Buffer_SampleFrameSize,
				       ObjectData->sample_type,
				       ObjectData->pre_length, NULL);

  MethodUpdate(class, object, (struct opUpdate*) msg);

  GetAttr(AHIA_Error, object, &result);

  if (result != 0) {
    return result;
  }

  if (ObjectData->sample_freq_int == 0 &&
      ObjectData->sample_freq_fract == 0) {
    return AHIE_Buffer_InvalidSampleFreq;
  }
  
  if (ObjectData->length == 0) {
    ObjectData->length = ObjectData->capacity;
  }

  if (ObjectData->capacity == 0) {
    return AHIE_Buffer_InvalidCapacity;
  }
  else {
    ULONG size = DoMethod(object, AHIM_Buffer_SampleFrameSize,
			  ObjectData->sample_type, ObjectData->capacity, NULL);

    if (size == 0) {

#warning TODO: Should AHIM_Buffer_SampleFrameSize set error?
      return AHIE_Buffer_InvalidSampleType;
    }
    
    ObjectData->data = AllocVec(size + ObjectData->prelen_size,
				  MEMF_PUBLIC | MEMF_CLEAR);
  }

  if (ObjectData->data == NULL) {
    return ERROR_NO_FREE_STORE;
  }
  
  return AHIE_OK;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  FreeVec(ObjectData->data);
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg)
{
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  ULONG length = ObjectData->length;
  
  BOOL check_length = FALSE;

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Buffer_Length:
	length = tag->ti_Data;
	check_length = TRUE;
	break;

      case AHIA_Buffer_TimestampHigh:
	ObjectData->timestamp_hi = tag->ti_Data;
	break;

      case AHIA_Buffer_TimestampLow:
	ObjectData->timestamp_lo = tag->ti_Data;
	break;
	
      case AHIA_Buffer_AgeHigh:
	ObjectData->age_hi = tag->ti_Data;
	break;

      case AHIA_Buffer_AgeLow:
	ObjectData->age_lo = tag->ti_Data;
	break;

      default:
	break;
    }
  }

  if (check_length) {
    if (length < ObjectData->capacity) {
      ObjectData->length = length;

      ObjectData->buffer_size = DoMethod(object, AHIM_Buffer_SampleFrameSize,
					   ObjectData->sample_type, length, NULL);
      NotifySuper(class, object, msg, AHIA_Buffer_Length, length, TAG_DONE);
    }
    else {
      SetSuperAttrs(class, object, AHIA_Error, AHIE_Buffer_InvalidLength, TAG_DONE);
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
      *msg->opg_Storage = ObjectData->sample_type;
      break;

    case AHIA_Buffer_SampleFreqInt:
      *msg->opg_Storage = ObjectData->sample_freq_int;
      break;

    case AHIA_Buffer_SampleFreqFract:
      *msg->opg_Storage = ObjectData->sample_freq_fract;
      break;
	
    case AHIA_Buffer_Capacity:
      *msg->opg_Storage = ObjectData->capacity;
      break;

    case AHIA_Buffer_Length:
      *msg->opg_Storage = ObjectData->length;
      break;

    case AHIA_Buffer_Data:
      *msg->opg_Storage = (ULONG) ObjectData->data + ObjectData->prelen_size;
      break;
      
    case AHIA_Buffer_RealData:
      *msg->opg_Storage = (ULONG) ObjectData->data;
      break;
      
    case AHIA_Buffer_PreLength:
      *msg->opg_Storage = (ULONG) ObjectData->pre_length;
      break;

    case AHIA_Buffer_TimestampHigh:
      *msg->opg_Storage = ObjectData->timestamp_hi;
      break;

    case AHIA_Buffer_TimestampLow:
      *msg->opg_Storage = ObjectData->timestamp_lo;
      break;
	
    case AHIA_Buffer_AgeHigh:
      *msg->opg_Storage = ObjectData->age_hi;
      break;

    case AHIA_Buffer_AgeLow:
      *msg->opg_Storage = ObjectData->age_lo;
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
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  ULONG cnt = 0;

  if (msg->SampleType == AHIST_NOTYPE) {
    return 0;
  }
  
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
#warning TODO: Fix later
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
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

#warning TODO: Implement AHIM_Buffer_Load
  
  return FALSE;
}


/******************************************************************************
** MethodClone ****************************************************************
******************************************************************************/

Object*
MethodClone(Class* class, Object* object, struct AHIP_Buffer_Clone* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
  Object*  r;
  
  SetIoErr(0); // Make sure OM_NEW calls SetIoErr().

  r = NewObject(ClassData->common.cl.cl_Class, NULL,
		AHIA_Buffer_SampleType,      ObjectData->sample_type,
		AHIA_Buffer_SampleFreqInt,   ObjectData->sample_freq_int,
		AHIA_Buffer_SampleFreqFract, ObjectData->sample_freq_fract,
		AHIA_Buffer_Capacity,        ObjectData->capacity,
		AHIA_Buffer_Length,          ObjectData->length,
		AHIA_Buffer_PreLength,       msg->PreLength,
		AHIA_Buffer_TimestampHigh,   ObjectData->timestamp_hi,
		AHIA_Buffer_TimestampLow,    ObjectData->timestamp_lo,
		AHIA_Buffer_AgeHigh,         ObjectData->age_hi,
		AHIA_Buffer_AgeLow,          ObjectData->age_lo,
		TAG_DONE);

  if (r == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, IoErr(), TAG_DONE);
  }

  return r;
}


/******************************************************************************
** MethodShift ****************************************************************
******************************************************************************/

BOOL
MethodShift(Class* class, Object* object, Msg msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
  char* addr = ObjectData->data;

  if (addr == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, ERROR_NO_FREE_STORE, TAG_DONE);
    return FALSE;
  }
  
  if (ObjectData->prelen_size > ObjectData->buffer_size) {
    // Old: ooooOOOOOOBBBB
    // New: OOOOOOBBBBxxxx
    bcopy(addr + ObjectData->buffer_size, addr, ObjectData->prelen_size);
  }
  else {
    // Old: oooobbbbbbBBBB
    // New: BBBBxxxxxxxxxx
    bcopy(addr + ObjectData->buffer_size, addr, ObjectData->prelen_size); // Deja vu!
  }

  return TRUE;
}
