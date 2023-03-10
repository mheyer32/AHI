
#include <config.h>

#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/processor/my.h>

#include <clib/alib_protos.h>
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
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);
  ULONG result = 0;

  // TODO: Initialize ObjectData here!

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

  // TODO: Clean up ObjectData here
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
	check_ready = TRUE;
	break;

      case AHIA_Buffer_SampleType: {
	ULONG st = tag->ti_Data;
	
	if ((st & AHIST_TYPE_MASK) ==
	    (AHIST_T_FLOAT | AHIST_D_DISCRETE | AHIST_FE)) {
	  ObjectData->channels = AHIST_C_DECODE(st);
	}
	else {
	  SetSuperAttrs(class, object,
			AHIA_Error, AHIE_Processor_InvalidSampleType,
			TAG_DONE);
	}

	check_ready = TRUE;
	break;
      }
	
      // TODO: Add your custom attributes here
	
      default:
	break;
    }
  }

  if (check_ready) {
    SetSuperAttrs(class, object,
		  AHIA_Processor_Ready, (ObjectData->data != NULL &&
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
      *msg->opg_Storage = (ULONG) "AHI Skeleton Processor";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Performs some awesome fx";
      break;
      
    case AHIA_DescriptionURL:
      *msg->opg_Storage = (ULONG) "http://example.com/fx";
      break;
      
    case AHIA_Author:
      *msg->opg_Storage = (ULONG) "Your Name";
      break;
      
    case AHIA_Copyright:
      *msg->opg_Storage = (ULONG) "?2004 Your Name";
      break;
      
    case AHIA_Version:
      *msg->opg_Storage = (ULONG) VERS;
      break;
      
    case AHIA_Annotation:
      *msg->opg_Storage = (ULONG) "Annotation string or NULL";
      break;
      
      // TODO: Add your custom attributes here

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
	float* data = ObjectData->data;

	for (s = 0, i = 0; s < ObjectData->length; ++s) {
	  for (c = 0; c < ObjectData->channels; ++c, ++i) {
	    // No-op
	  }
	}
      }
      break;
  }

  return result;
}
