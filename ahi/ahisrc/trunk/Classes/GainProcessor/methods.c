
#include <config.h>

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
dbfixed2float(dBFixed x) {
  return pow(10, x / 65536.0 / 20.0);
}

static inline dBFixed
float2dbfixed(float x) {
  return (dBFixed) (20 * 65536 * log10(x));
}


static void
recalc_sweeps(struct ObjectData* ObjectData) {
  if (ObjectData->length > 0 &&
      ObjectData->balance != NULL &&
      ObjectData->gains != NULL &&
      ObjectData->sweeps != NULL) {
    ULONG c;
    float step = 1.0f / ObjectData->length;

    for (c = 0; c <  ObjectData->channels; ++c) {
      float gain = ObjectData->balance[c] * ObjectData->gain;
      ObjectData->sweeps[c] = pow(ObjectData->gains[c]/gain, step);
    }
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

  ObjectData->gain = 1.0f;
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


  FreeVec(ObjectData->balance); ObjectData->balance = NULL;
  FreeVec(ObjectData->gains);   ObjectData->gains   = NULL;
  FreeVec(ObjectData->sweeps);  ObjectData->sweeps  = NULL;
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
      case AHIA_Buffer_Data:
	ObjectData->data = (float*) tag->ti_Data;
	check_ready = TRUE;
	break;

      case AHIA_Buffer_Length:
	ObjectData->length = tag->ti_Data;
	recalc_sweeps(ObjectData);
	check_ready = TRUE;
	break;

      case AHIA_Buffer_SampleType: {
	ULONG st = tag->ti_Data;

	FreeVec(ObjectData->balance); ObjectData->balance = NULL;
	FreeVec(ObjectData->gains);   ObjectData->gains   = NULL;
	FreeVec(ObjectData->sweeps);  ObjectData->sweeps  = NULL;

	ObjectData->channels = 0;
	
	if ((st & AHIST_TYPE_MASK) ==
	    (AHIST_T_FLOAT | AHIST_D_DISCRETE | AHIST_FE)) {
	  int size = sizeof (float) * ObjectData->channels;
	  
	  ObjectData->channels = AHIST_C_DECODE(st);
	  ObjectData->balance  = AllocVec(size, MEMF_PUBLIC);
	  ObjectData->gains    = AllocVec(size, MEMF_PUBLIC);
	  ObjectData->sweeps   = AllocVec(size, MEMF_PUBLIC);

	  if (ObjectData->balance != NULL &&
	      ObjectData->gains != NULL &&
	      ObjectData->sweeps != NULL) {
	    ULONG c;

	    for (c = 0; c < ObjectData->channels; ++c) {
	      ObjectData->balance[c] = 1.0f;
	      ObjectData->gains[c]   = ObjectData->gain;
	      ObjectData->sweeps[c]  = 1.0f;
	    }
	  }
	  else {
	    SetSuperAttrs(class, object,
			  AHIA_Error, ERROR_NO_FREE_STORE,
			  TAG_DONE);
	  }
	}
	else {
	  SetSuperAttrs(class, object,
			AHIA_Error, AHIE_Processor_InvalidSampleType,
			TAG_DONE);
	}

	check_ready = TRUE;
	break;
      }

      case AHIA_GainProcessor_Gain: {
	ULONG c;

	ObjectData->gain = dbfixed2float(tag->ti_Data);
	recalc_sweeps(ObjectData);
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
      }
	
      default:
	break;
    }
  }

  if (check_ready) {
    SetSuperAttrs(class, object,
		  AHIA_Processor_Ready, (ObjectData->balance != NULL &&
					 ObjectData->gains != NULL &&
					 ObjectData->sweeps != NULL &&
					 ObjectData->data != NULL &&
					 ObjectData->length > 0 &&
					 ObjectData->channels > 0),
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
      *msg->opg_Storage = ObjectData->channels;
      break;

    case AHIA_GainProcessor_Gain:
      *msg->opg_Storage = float2dbfixed(ObjectData->gain);

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

  ULONG result = DoSuperMethodA(class, object, (Msg) msg);
  
  switch (result) {
    case AHIV_Processor_FailProc:
    case AHIV_Processor_SkipProc:
      break;

    case AHIV_Processor_PerformProc: {
	ULONG  c;
	ULONG  s;
	ULONG  i;
	float* data  = ObjectData->data;
	float* gain  = ObjectData->gains;
	float* sweep = ObjectData->sweeps;

	for (s = 0, i = 0; s < ObjectData->length; ++s) {
	  for (c = 0; c < ObjectData->channels; ++c, ++i) {
	    data[i] *= gain[c];
	    gain[c] *= sweep[c];
	  }
	}

	// Reset sweep factors
	for (c = 0; c < ObjectData->channels; ++c, ++i) {
	  sweep[c] = 1.0;
	}
      }
      break;
  }

  return result;
}


/******************************************************************************
** MethodSetBalance ***********************************************************
******************************************************************************/

BOOL
MethodSetBalance(Class* class, Object* object, struct AHIP_GainProcessor_Balance* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
  ULONG c;
  
  if (msg->Channels > ObjectData->channels ||
      ObjectData->balance == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  msg->Channels; ++c) {
    ObjectData->balance[c] = dbfixed2float(msg->Gains[c]);
  }

  recalc_sweeps(ObjectData);

  return TRUE;
}


/******************************************************************************
** MethodGetBalance ***********************************************************
******************************************************************************/

BOOL
MethodGetBalance(Class* class, Object* object, struct AHIP_GainProcessor_Balance* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
  ULONG c;

  if (msg->Channels > ObjectData->channels ||
      ObjectData->balance == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  msg->Channels; ++c) {
    msg->Gains[c] = float2dbfixed(ObjectData->balance[c]);
  }

  return TRUE;
}
