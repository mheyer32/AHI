
#include <config.h>

#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/processor/splitter.h>

#include <clib/alib_protos.h>
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
  ULONG result = 0;

  MethodUpdate(class, object, (struct opUpdate*) msg);
  GetAttr(AHIA_Error, object, &result);
  return result;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  FreeVec(ObjectData->cached_data);
  ObjectData->cached_data = NULL;
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg)
{
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  BOOL check_ready = FALSE;
  
  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Processor_Buffer: {
	ObjectData->buffer = (Object*) tag->ti_Data;

	if (ObjectData->buffer == NULL) {
	  ObjectData->data = NULL;
	  ObjectData->length = 0;
	  ObjectData->sample_type = AHIST_NOTYPE;
	  ObjectData->size = 0;

	  FreeVec(ObjectData->cached_data);
	  ObjectData->cached_data = NULL;
	}
	else {
	  ULONG capacity   = 0;
	  ULONG local_size = 0;

	  GetAttr(AHIA_Buffer_SampleType, ObjectData->buffer, &ObjectData->sample_type);
	  GetAttr(AHIA_Buffer_Capacity, ObjectData->buffer, &capacity);
	  local_size = DoMethod(ObjectData->buffer, AHIM_Buffer_SampleFrameSize,
				ObjectData->sample_type, capacity, NULL);

	  ObjectData->cached_data = AllocVec(local_size, MEMF_PUBLIC);

	  if (ObjectData->cached_data == NULL) {
	    SetSuperAttrs(class, object, AHIA_Error, ERROR_NO_FREE_STORE, TAG_DONE);
	  }
	}

	check_ready = TRUE;
	break;
      }

      case AHIA_Buffer_Data:
	ObjectData->data = (float*) tag->ti_Data;
	break;
	
      case AHIA_Buffer_Length: {
	ObjectData->length = tag->ti_Data;
	if (ObjectData->buffer != NULL) {
	  ObjectData->size = DoMethod(ObjectData->buffer, AHIM_Buffer_SampleFrameSize,
					ObjectData->sample_type, ObjectData->length, NULL);
	}
	check_ready = TRUE;
	break;
      }
	  
      default:
	break;
    }
  }

  if (check_ready) {
    SetSuperAttrs(class, object,
		  AHIA_Processor_Ready, (ObjectData->buffer != NULL &&
					 ObjectData->cached_data != NULL &&
					 ObjectData->data != NULL &&
					 ObjectData->length > 0),
		  TAG_DONE);
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
      *msg->opg_Storage = (ULONG) "AHI Splitter Processor";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Splits a processor chain.";
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

    case AHIA_Processor_MaxChildren:
      *msg->opg_Storage = ObjectData->master == NULL ? 1 : 0;
      break;

    default:
      return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** MethodProcess **************************************************************
******************************************************************************/

ULONG
MethodProcess(Class* class, Object* object, struct AHIP_Processor_Process* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  ULONG result;

  if (ObjectData->master == NULL) {
    ULONG time_hi = 0;
    ULONG time_lo = 0;
    UQUAD time = 0;

    GetAttr(AHIA_Buffer_TimestampHigh, ObjectData->buffer, &time_hi);
    GetAttr(AHIA_Buffer_TimestampLow, ObjectData->buffer, &time_lo);
    time = ((UQUAD) time_hi << 32) | time_lo;

    if (time < ObjectData->last_time) {
      SetSuperAttrs(class, object, AHIA_Error, AHIE_SplitterProcessor_InvalidBufferTime, TAG_DONE);
      result = AHIV_Processor_FailProc;
    }
    else if (time == ObjectData->last_time) {
      // We have already processed this timestamp, so our local copy
      // of the buffer data is valid.
      result = AHIV_Processor_SkipProc;
    }
    else {
      // Remember timestamp and process the buffer
      ObjectData->last_time = time;
      result = DoSuperMethodA(class, object, (Msg) msg);
    }

    switch (result) {
      case AHIV_Processor_FailProc:
      case AHIV_Processor_SkipProc:
	break;

      case AHIV_Processor_PerformProc: {
	bcopy(ObjectData->data, ObjectData->cached_data, ObjectData->size);
	break;
      }
    }
  }
  else {
    result = DoSuperMethodA(class, object, (Msg) msg);
  
    switch (result) {
      case AHIV_Processor_FailProc:
      case AHIV_Processor_SkipProc:
	break;

      case AHIV_Processor_PerformProc: {
	struct ObjectData* md = (struct ObjectData*) INST_DATA(class, ObjectData->master);

	if (DoMethodA(ObjectData->master, (Msg) msg) == AHIV_Processor_FailProc) {
	  ULONG error = 0;

	  GetAttr(AHIA_Error, ObjectData->master, &error);	
	  SetSuperAttrs(class, object, AHIA_Error, error, TAG_DONE);
	  result = AHIV_Processor_FailProc;
	}
	else if (md->size != ObjectData->size || md->sample_type != ObjectData->sample_type) {
	  SetSuperAttrs(class, object, AHIA_Error, AHIE_SplitterProcessor_InvalidBuffer, TAG_DONE);
	  result = AHIV_Processor_FailProc;
	}
	else {
	  bcopy(md->cached_data, ObjectData->data, ObjectData->size);
	}
	break;
      }
    }
  }

  return result;
}
