
#include <config.h>

#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/processor/adder.h>

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
      case AHIA_Processor_Buffer: {
	Object* buffer = (Object*) tag->ti_Data;
	int i;

	for (i = 0; i < MAX_CHILDREN; ++i) {
	  if (AHIClassData->children[i] != NULL) {
	    Object* old_buffer = NULL;
	    Object* new_buffer = NULL;
	    
	    if (buffer != NULL) {
	      new_buffer = (Object*) DoMethod(buffer, AHIM_Buffer_Clone, 0);

	      if (new_buffer == NULL) {
		ULONG error = 0;
      
		GetAttr(AHIA_Error, buffer, &error);
		SetSuperAttrs(class, object, AHIA_Error, error, TAG_DONE);
	      }
	    }

	    GetAttr(AHIA_Processor_Buffer, AHIClassData->children[i], (ULONG*) &old_buffer);
	    SetAttrs(AHIClassData->children[i],
		     AHIA_Processor_Buffer, (ULONG) new_buffer,
		     TAG_DONE);
	    AHIClassData->buffers[i] = new_buffer;
	    if (new_buffer != NULL) {
	      GetAttr(AHIA_Buffer_Data, new_buffer, (ULONG*) &AHIClassData->datas[i]);
	    }
	    else {
	      AHIClassData->datas[i] = NULL;
	    }
	    
	    if (old_buffer != NULL) {
	      DisposeObject(old_buffer);
	    }
	  }
	}
	break;
      }
	
      case AHIA_Buffer_Data:
	AHIClassData->data = (float*) tag->ti_Data;
	check_ready = TRUE;
	break;

      case AHIA_Buffer_Length: {
	int i;
	
	AHIClassData->length = tag->ti_Data;
	
	for (i = 0; i < MAX_CHILDREN; ++i) {
	  if (AHIClassData->buffers[i] != NULL) {
	    SetAttrs(AHIClassData->buffers[i], AHIA_Buffer_Length, AHIClassData->length, TAG_DONE);
	  }
	}
	
	check_ready = TRUE;
	break;
      }

      case AHIA_Buffer_SampleType: {
	ULONG st = tag->ti_Data;
	
	if ((st & AHIST_TYPE_MASK) ==
	    (AHIST_T_FLOAT | AHIST_D_DISCRETE | AHIST_FE)) {
	  AHIClassData->channels = AHIST_C_DECODE(st);
	}
	else {
	  SetSuperAttrs(class, object,
			AHIA_Error, AHIE_Processor_InvalidSampleType,
			TAG_DONE);
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
		  AHIA_Processor_Ready, (AHIClassData->data != NULL &&
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

  switch (msg->opg_AttrID) {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Adder Processor";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Joins several processor chains to one.";
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
      *msg->opg_Storage = MAX_CHILDREN;
      break;

    default:
      return FALSE;
  }

  return TRUE;
}


/******************************************************************************
** MethodAddMember ************************************************************
******************************************************************************/

BOOL
MethodAddMember(Class* class, Object* object, struct opMember* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  BOOL rc;
  Object* buffer = NULL;

  rc = DoSuperMethodA(class, object, (Msg) msg);

  GetAttr(AHIA_Processor_Buffer, object, (ULONG*) &buffer);

  if (rc && buffer != NULL) {
    Object* new_buffer = (Object*) DoMethod(buffer, AHIM_Buffer_Clone, 0);

    if (new_buffer == NULL) {
      ULONG error = 0;
      
      GetAttr(AHIA_Error, buffer, &error);
      SetSuperAttrs(class, object, AHIA_Error, error, TAG_DONE);

      // Use DoSuperMethod in order to prevent our MethodRemMember
      // from being called
      DoSuperMethod(class, object, OM_REMMEMBER, msg->opam_Object);
      rc = FALSE;
    }
    else {
      int i;

      // Attach the cloned buffer to the child processor. The buffer
      // will be freed by MethodRemMember below when the child is
      // detached.
      SetAttrs(msg->opam_Object, AHIA_Processor_Buffer, (ULONG) new_buffer, TAG_DONE);

      for (i = 0; i < MAX_CHILDREN; ++i) {
	if (AHIClassData->children[i] == NULL) {
	  AHIClassData->children[i] = msg->opam_Object;
	  AHIClassData->buffers[i]  = new_buffer;
	  GetAttr(AHIA_Buffer_Data, new_buffer, (ULONG*) &AHIClassData->datas[i]);
	  break;
	}
      }
    }
  }
  
  return rc;
}


/******************************************************************************
** MethodRemMember ************************************************************
******************************************************************************/

BOOL
MethodRemMember(Class* class, Object* object, struct opMember* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);
  int i;
  BOOL rc;
  Object* buffer = NULL;

  GetAttr(AHIA_Processor_Buffer, object, (ULONG*) &buffer);

  rc = DoSuperMethodA(class, object, (Msg) msg);

  if (rc) {
    for (i = 0; i < MAX_CHILDREN; ++i) {
      if (AHIClassData->children[i] == msg->opam_Object) {
	AHIClassData->children[i] = NULL;
	AHIClassData->buffers[i]  = NULL;
	AHIClassData->datas[i]    = NULL;
	break;
      }
    }

    if (buffer != NULL) {
      DisposeObject(buffer);
    }
  }    

  return rc;
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
      ULONG  p;
      ULONG  c;
      ULONG  s;
      ULONG  i;
      float* data = AHIClassData->data;

      bzero(data, sizeof (float) * AHIClassData->length * AHIClassData->channels);

      for (p = 0; p < MAX_CHILDREN; ++p) {
	if (AHIClassData->datas[p] != NULL) {
	  float* src = AHIClassData->datas[p];
	  
	  for (s = 0, i = 0; s < AHIClassData->length; ++s) {
	    for (c = 0; c < AHIClassData->channels; ++c, ++i) {
	      data[i] += src[i];
	    }
	  }
	}
      }
      break;
    }
  }

  return result;
}
