
#include <config.h>

#include <math.h>

#include <classes/ahi/output.h>

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
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->ops_AttrList;
  struct TagItem* tag;
  ULONG result = 0;

  AHIClassData->mode = AHIV_Output_AnyMode;

  while ((tag = NextTagItem(&tstate))) {
    switch (tag->ti_Tag) {

      case AHIA_Output_Mode:
	switch (tag->ti_Tag) {
	  case AHIV_Output_AnyMode:
	  case AHIV_Output_SharedMode:
	  case AHIV_Output_ExclusiveMode:
	    AHIClassData->mode = tag->ti_Data;
	    break;

	  default:
	    result = AHIE_Output_InvalidMode;
	    break;
	}
	break;
    }
  }

  if (result == 0) {
    MethodUpdate(class, object, (struct opUpdate*) msg);
    GetAttr(AHIA_Error, object, &result);
  }

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
MethodUpdate(Class* class, Object* object, struct opUpdate* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
    
    switch (tag->ti_Tag) {
      // No writable attributes
      
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
MethodGet(Class* class, Object* object, struct opGet* msg) {
  struct AHIClassBase* AHIClassBase = (struct AHIClassBase*) class->cl_UserData;
  struct AHIClassData* AHIClassData = (struct AHIClassData*) INST_DATA(class, object);

  switch (msg->opg_AttrID)
  {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI Sound card";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Sound card base class";
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

      // Subclasses are expected to override these:

    case AHIA_Output_Mode:
      *msg->opg_Storage = AHIClassData->mode;
      
    case AHIA_Board_OutputClass:
    case AHIA_Board_Inputs:
    case AHIA_Board_InputClass:
    case AHIA_Board_Mixers:
    case AHIA_Board_MixerClass:
    case AHIA_Board_MIDIOutputs:
    case AHIA_Board_MIDIOutputClass:
    case AHIA_Board_MIDIInputs:
    case AHIA_Board_MIDIInputClass:
      *msg->opg_Storage = 0;
      break;

    default:
      return FALSE;
  }

  return TRUE;
}
