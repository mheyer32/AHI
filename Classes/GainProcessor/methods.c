
#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/processor/gain.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"

#if 0

static inline float
fixed2float(Fixed x) {
  return x / 65536.0;
}

static inline Fixed
float2fixed(float x) {
  return (Fixed) (x * 65536.0);
}

#else

static inline float
dbfixed2float(dBFixed x) {
  return pow(10, x / 65536.0 / 20.0);
}

static inline dBFixed
float2dbfixed(float x) {
  return (dBFixed) (20 * 65536 * log10(x));
}

#endif

/******************************************************************************
** MethodNew ******************************************************************
******************************************************************************/

LONG
MethodNew(Class* class, Object* object, struct opSet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG result = 0;

  AHIClassData->gain = 1.0f;
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

  FreeVec(AHIClassData->balance);
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

  BOOL check_ready = FALSE;
  
  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {
      case AHIA_Buffer_Data:
	AHIClassData->data = (float*) tag->ti_Data;
	check_ready = TRUE;
	break;

      case AHIA_Buffer_Length:
	AHIClassData->length = tag->ti_Data;
	check_ready = TRUE;
	break;

      case AHIA_Buffer_SampleType: {
	ULONG st = tag->ti_Data;

	FreeVec(AHIClassData->gains);
	AHIClassData->gains = NULL;
	FreeVec(AHIClassData->balance);
	AHIClassData->balance = NULL;
	AHIClassData->channels = 0;
	
	if ((st & AHIST_TYPE_MASK) ==
	    (AHIST_T_FLOAT | AHIST_D_DISCRETE | AHIST_FE)) {
	  AHIClassData->channels = AHIST_C_DECODE(st);
	  AHIClassData->gains = AllocVec( sizeof (float) * AHIClassData->channels,
					  MEMF_PUBLIC);
	  AHIClassData->balance = AllocVec( sizeof (float) * AHIClassData->channels,
					    MEMF_PUBLIC);

	  if (AHIClassData->gains != NULL &&
	      AHIClassData->balance != NULL) {
	    ULONG c;

	    for (c = 0; c < AHIClassData->channels; ++c) {
	      AHIClassData->gains[c] = AHIClassData->gain;
	      AHIClassData->balance[c] = 1.0f;
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
			AHIA_Error, AHIE_GainProcessor_InvalidSampleType,
			TAG_DONE);
	}

	check_ready = TRUE;
	break;
      }

      case AHIA_GainProcessor_Gain: {
	ULONG c;

	AHIClassData->gain = dbfixed2float(tag->ti_Data);

	if (AHIClassData->gains != NULL && AHIClassData->balance != NULL) {
	  for (c = 0; c <  AHIClassData->channels; ++c) {
	    AHIClassData->gains[c] = AHIClassData->balance[c] * AHIClassData->gain;
	  }
	}

	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
      }
	
      default:
	break;
    }
  }

  if (check_ready) {
    SetSuperAttrs(class, object,
		  AHIA_Processor_Ready, (AHIClassData->gains != NULL &&
					 AHIClassData->balance != NULL &&
					 AHIClassData->data != NULL &&
					 AHIClassData->length > 0 &&
					 AHIClassData->channels > 0),
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

    case AHIA_GainProcessor_Gain:
      *msg->opg_Storage = float2dbfixed(AHIClassData->gain);

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
    case AHIV_Processor_FailProc:
    case AHIV_Processor_SkipProc:
      break;

    case AHIV_Processor_PerformProc: {
	ULONG  c;
	ULONG  s;
	ULONG  i;
	float* data = AHIClassData->data;
	float* gain = AHIClassData->gains;

	for (s = 0, i = 0; s < AHIClassData->length; ++s) {
	  for (c = 0; c < AHIClassData->channels; ++c, ++i) {
	    data[i] *= gain[c];
	  }
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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG c;
  
  if (msg->Channels > AHIClassData->channels ||
      AHIClassData->balance == NULL ||
      AHIClassData->gains == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  msg->Channels; ++c) {
    AHIClassData->balance[c] = dbfixed2float(msg->Gains[c]);
    AHIClassData->gains[c]   = AHIClassData->balance[c] * AHIClassData->gain;
  }

  return TRUE;
}


/******************************************************************************
** MethodGetBalance ***********************************************************
******************************************************************************/

BOOL
MethodGetBalance(Class* class, Object* object, struct AHIP_GainProcessor_Balance* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  ULONG c;

  if (msg->Channels > AHIClassData->channels || AHIClassData->balance == NULL) {
    SetSuperAttrs(class, object, AHIA_Error, AHIE_GainProcessor_TooManyChannels, TAG_DONE);
    return FALSE;
  }

  for (c = 0; c <  msg->Channels; ++c) {
    msg->Gains[c] = float2dbfixed(AHIClassData->balance[c]);
  }

  return TRUE;
}
