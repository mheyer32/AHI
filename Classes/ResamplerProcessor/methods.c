
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
calc_pitch(struct AHIClassBase* AHIClassBase,
	   struct AHIClassData* AHIClassData) {
  if (AHIClassData->frequency != -1 && AHIClassData->buffer != NULL) {
    ULONG freq = 0;
	
    GetAttr(AHIA_Buffer_SampleFreqInt, AHIClassData->buffer, &freq);

    if (freq != 0) {
      AHIClassData->pitch =  (double) freq / AHIClassData->frequency;
    }
  }
  else {
    AHIClassData->pitch = -1;
  }
}


static void
calc_freq(struct AHIClassBase* AHIClassBase,
	  struct AHIClassData* AHIClassData) {
  if (AHIClassData->pitch != -1 && AHIClassData->buffer != NULL) {
    ULONG freq = 0;
	
    GetAttr(AHIA_Buffer_SampleFreqInt, AHIClassData->buffer, &freq);

    if (freq != 0) {
      AHIClassData->frequency = freq * AHIClassData->pitch;
    }
  }
  else {
    AHIClassData->frequency = -1;
  }
}


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG result = 0;

  AHIClassData->frequency    = -1;
  AHIClassData->pitch        = 1.0;
  AHIClassData->phase_offset = 0;
  
  MethodUpdate(class, object, (struct opUpdate*) msg);

  GetAttr(AHIA_Error, object, &result);

  return result;
}


/******************************************************************************
** MethodDispose **************************************************************
******************************************************************************/

void
MethodDispose(Class* class, Object* object, Msg msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/

ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg)
{
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  BOOL check_ready = FALSE;
  
  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Processor_Buffer:
	AHIClassData->buffer = (Object*) tag->ti_Data;

	if (AHIClassData->frequency == -1) {
	  calc_freq(AHIClassBase, AHIClassData);
	}

	if (AHIClassData->pitch == -1) {
	  calc_pitch(AHIClassBase, AHIClassData);
	}
	
	check_ready = TRUE;
	break;

      case AHIA_ResamplerProcessor_Frequency: {
	AHIClassData->frequency = tag->ti_Data;
	calc_pitch(AHIClassBase, AHIClassData);
	break;
      }

      case AHIA_ResamplerProcessor_Pitch: {
	AHIClassData->pitch = tag->ti_Data / 65536.0;
	calc_freq(AHIClassBase, AHIClassData);
	break;
      }

      case AHIA_ResamplerProcessor_PhaseOffset: {
	AHIClassData->phase_offset = tag->ti_Data / 65536.0;
	break;
      }
	
      default:
	break;
    }
  }

  if (check_ready) {
    SetSuperAttrs(class, object,
		  AHIA_Processor_Ready, (AHIClassData->buffer != 0),
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

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
      *msg->opg_Storage = (ULONG) "©2004 Martin Blom";
      break;
      
    case AHIA_Version:
      *msg->opg_Storage = (ULONG) VERS;
      break;
      
    case AHIA_Annotation:
      *msg->opg_Storage = 0;
      break;
      
    case AHIA_ResamplerProcessor_Frequency:
      *msg->opg_Storage = (ULONG) AHIClassData->frequency;
      break;

    case AHIA_ResamplerProcessor_Pitch:
      *msg->opg_Storage = (ULONG) (AHIClassData->pitch * 65536);
      break;

    case AHIA_ResamplerProcessor_PhaseOffset:
      *msg->opg_Storage = (ULONG) (AHIClassData->phase_offset * 65536);
      break;
	
    default:
      return FALSE;
  }

  return TRUE;
}
