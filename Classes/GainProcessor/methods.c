
#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/processor/gain.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"

static inline float
fixed2float(Fixed x) {
  return x / 65536.0;
}

static inline Fixed
float2fixed(float x) {
  return (Fixed) (x * 65536.0);
}


/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  FreeVec(AHIClassData->gains);
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
	Object* buffer = (Object*) tag->ti_Data;

	if (buffer != NULL) {
	  ULONG st  = AHIST_NOTYPE;

	  GetAttr(AHIA_Buffer_Data, buffer, (ULONG*) &AHIClassData->data);
	  GetAttr(AHIA_Buffer_Length, buffer, &AHIClassData->length);
	  GetAttr(AHIA_Buffer_SampleType, buffer, &st);

	  if ((st & AHIST_TYPE_MASK) ==
	      (AHIST_T_FLOAT | AHIST_D_DISCRETE | AHIST_FE)) {
	    AHIClassData->channels = AHIST_C_DECODE(st);
	    AHIClassData->gains = AllocVec( sizeof (float) * AHIClassData->channels,
					    MEMF_PUBLIC);

	    if (AHIClassData->gains != NULL) {
	      ULONG c;

	      for (c = 0; c < AHIClassData->channels; ++c) {
		AHIClassData->gains[c] = 1.0f;
	      }
	      
	      // Leave now
	      break;
	    }
	    else {
	      SetAttrs(object, AHIA_Error, ERROR_NO_FREE_STORE,
		       TAG_DONE);
	    }
	  }
	  else {
	    SetAttrs(object, AHIA_Error, AHIE_GainProcessor_InvalidSampleType,
		     TAG_DONE);
	  }
	}

	AHIClassData->channels = 0;
	FreeVec(AHIClassData->gains);
	AHIClassData->gains = NULL;
	AHIClassData->data = NULL;
	AHIClassData->length = 0;
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  switch (msg->opg_AttrID)
  {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Gain Processor";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Controls the volume of the channels.";
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
      
    case AHIA_GainProcessor_Channels:
      *msg->opg_Storage = AHIClassData->channels;
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  ULONG result = DoSuperMethodA(class, object, (Msg) msg);
  
  switch (result) {
    case AHIV_Processor_Process_FAIL:
    case AHIV_Processor_Process_SKIP:
      break;

    case AHIV_Processor_Process_PERFORM:
      if (AHIClassData->gains != NULL &&
	  AHIClassData->data != NULL &&
	  AHIClassData->length > 0 &&
	  AHIClassData->channels > 0) {
	ULONG  c;
	ULONG  s;
	float* data = AHIClassData->data;

	for (s = 0; s < AHIClassData->length; ++s) {
	  for (c = 0; c < AHIClassData->channels; ++c) {
	    *data++ *= AHIClassData->gains[c];
	  }
	}
      }
      break;
  }

  return result;
}


/******************************************************************************
** MethodSetGain **************************************************************
******************************************************************************/

BOOL
MethodSetGain(Class* class, Object* object, struct AHIP_GainProcessor_Gain* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG c;
  
  if (msg->Channels > AHIClassData->channels || AHIClassData->gains == NULL) {
    SetAttrs(object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  msg->Channels; ++c) {
    AHIClassData->gains[c] = fixed2float(msg->Gains[c]);
  }

  return TRUE;
}


/******************************************************************************
** MethodGetGain **************************************************************
******************************************************************************/

BOOL
MethodGetGain(Class* class, Object* object, struct AHIP_GainProcessor_Gain* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG c;

  if (msg->Channels > AHIClassData->channels || AHIClassData->gains == NULL) {
    SetAttrs(object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  msg->Channels; ++c) {
    msg->Gains[c] = float2fixed(AHIClassData->gains[c]);
  }

  return TRUE;
}


/******************************************************************************
** MethodSetAllGain ***********************************************************
******************************************************************************/

BOOL
MethodSetAllGain(Class* class, Object* object, struct AHIP_GainProcessor_GainAll* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG c;
  
  if (AHIClassData->gains == NULL) {
    SetAttrs(object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  AHIClassData->channels; ++c) {
    AHIClassData->gains[c] = fixed2float(msg->Gain);
  }

  return TRUE;
}
