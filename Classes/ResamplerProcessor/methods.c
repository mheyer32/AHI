
#include <config.h>

#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/processor/resampler.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"

static void
calc_pitch(struct ClassData* ClassData,
	   struct ObjectData* ObjectData) {
  if (ObjectData->frequency != -1 && ObjectData->buffer != NULL) {
    ULONG freq = 0;
	
    GetAttr(AHIA_Buffer_SampleFreqInt, ObjectData->buffer, &freq);

    if (freq != 0) {
      ObjectData->pitch =  (double) freq / ObjectData->frequency;
    }
  }
  else {
    ObjectData->pitch = -1;
  }
}


static void
calc_freq(struct ClassData* ClassData,
	  struct ObjectData* ObjectData) {
  if (ObjectData->pitch != -1 && ObjectData->buffer != NULL) {
    ULONG freq = 0;
	
    GetAttr(AHIA_Buffer_SampleFreqInt, ObjectData->buffer, &freq);

    if (freq != 0) {
      ObjectData->frequency = freq * ObjectData->pitch;
    }
  }
  else {
    ObjectData->frequency = -1;
  }
}


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
  ULONG result = 0;

  ObjectData->frequency    = -1;
  ObjectData->pitch        = 1.0;
  ObjectData->phase_offset = 0;
  
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
      case AHIA_Processor_Buffer:
	ObjectData->buffer = (Object*) tag->ti_Data;

	if (ObjectData->frequency == -1) {
	  calc_freq(ClassData, ObjectData);
	}

	if (ObjectData->pitch == -1) {
	  calc_pitch(ClassData, ObjectData);
	}
	
	check_ready = TRUE;
	break;

      case AHIA_ResamplerProcessor_Frequency: {
	ObjectData->frequency = tag->ti_Data;
	calc_pitch(ClassData, ObjectData);
	break;
      }

      case AHIA_ResamplerProcessor_Pitch: {
	ObjectData->pitch = tag->ti_Data / 65536.0;
	calc_freq(ClassData, ObjectData);
	break;
      }

      case AHIA_ResamplerProcessor_PhaseOffset: {
	ObjectData->phase_offset = tag->ti_Data / 65536.0;
	break;
      }
	
      default:
	break;
    }
  }

  if (check_ready) {
    SetSuperAttrs(class, object,
		  AHIA_Processor_Ready, (ObjectData->buffer != 0),
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
      *msg->opg_Storage = (ULONG) "AHI Resampler Base Class";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "The base class for all resamplers";
      break;
      
    case AHIA_DescriptionURL:
      *msg->opg_Storage = (ULONG) "http://www.lysator.liu.se/ahi/";
      break;
      
    case AHIA_Author:
      *msg->opg_Storage = (ULONG) "Martin Blom";
      break;
      
    case AHIA_Copyright:
      *msg->opg_Storage = (ULONG) "?2004 Martin Blom";
      break;
      
    case AHIA_Version:
      *msg->opg_Storage = (ULONG) VERS;
      break;
      
    case AHIA_Annotation:
      *msg->opg_Storage = 0;
      break;
      
    case AHIA_ResamplerProcessor_Frequency:
      *msg->opg_Storage = (ULONG) ObjectData->frequency;
      break;

    case AHIA_ResamplerProcessor_Pitch:
      *msg->opg_Storage = (ULONG) (ObjectData->pitch * 65536);
      break;

    case AHIA_ResamplerProcessor_PhaseOffset:
      *msg->opg_Storage = (ULONG) (ObjectData->phase_offset * 65536);
      break;
	
    default:
      return FALSE;
  }

  return TRUE;
}
