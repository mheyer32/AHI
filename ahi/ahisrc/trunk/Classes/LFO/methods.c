
#include <config.h>

#include <math.h>

#include <classes/ahi/buffer.h>
#include <classes/ahi/lfo.h>
#include <classes/ahi/processor/tick.h>

#include <clib/alib_protos.h>
#include <proto/intuition.h>

#include "ahiclass.h"
#include "methods.h"
#include "util.h"
#include "version.h"

static const double FIXED2PI = 65536 * 2 * M_PI;

static inline double
fract(double x) {
  return x - floor(x);
}

static void
calc_iq(Class* class, Object* object, struct opUpdate* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  double phase;
  double i = 0;
  double q = 0;

  phase  = fmod(ObjectData->ticktime * ObjectData->frequency +
		ObjectData->phase_offset, 1.0);

  ObjectData->phase = phase;

  switch  (ObjectData->waveform) {
    case AHIV_LFO_Sine:
      i = cos(phase * 2 * M_PI);
      q = sin(phase * 2 * M_PI);
      break;

    case AHIV_LFO_Square: {
      static const double sqr[5] = { 1, 1, -1, -1, 1 };

      i = sqr[(int) (phase * 4) + 1];
      q = sqr[(int) (phase * 4) + 0];
      break;
    }
	    
    case AHIV_LFO_Triangle: {
      static const double tri[6] = { 0, 1, 0, -1, 0, 1 };
      double f  = fmod( phase * 4, 1.0);
      int i_idx = (int) (phase * 4) + 1;
      int q_idx = (int) (phase * 4) + 0;

      i = tri[i_idx] + (tri[i_idx + 1] - tri[i_idx]) * f;
      q = tri[q_idx] + (tri[q_idx + 1] - tri[q_idx]) * f;
      break;
    }

    case AHIV_LFO_Sawtooth:
      i = 1.0 - 2.0 * phase;
      q = 1.0 - 2.0 * fmod(phase + 0.75, 1.0);
      break;
  }

  ObjectData->i = ObjectData->bias + ObjectData->amplitude * i;
  ObjectData->q = ObjectData->bias + ObjectData->amplitude * q;

/*   KPrintF("Calculated phase to %08lx, I = %08lx, Q = %08lx\n", */
/* 	  (ULONG) (ObjectData->phase * FIXED2PI), */
/* 	  (ULONG) ObjectData->i, (ULONG) ObjectData->q); */
  
  NotifySuper(class, object, msg,
	      AHIA_LFO_Phase, (ULONG) (ObjectData->phase * FIXED2PI),
	      AHIA_LFO_I,     (ULONG) ObjectData->i,
	      AHIA_LFO_Q,     (ULONG) ObjectData->q,
	      TAG_DONE);
}


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
}


/******************************************************************************
** MethodSet ******************************************************************
******************************************************************************/
		    
ULONG
MethodUpdate(Class* class, Object* object, struct opUpdate* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  BOOL update_iq = FALSE;
  
  struct TagItem* tstate = msg->opu_AttrList;
  struct TagItem* tag;

  while ((tag = NextTagItem(&tstate))) {
//    KPrintF("LFO %08lx attrib %08lx is %08lx\n", object, tag->ti_Tag, tag->ti_Data);
    
    switch (tag->ti_Tag) {
      case AHIA_LFO_Frequency:
	ObjectData->frequency = tag->ti_Data / 65536.0;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	update_iq = TRUE;
	break;

      case AHIA_LFO_Phase: {
	double offset = (ObjectData->phase_offset + tag->ti_Data / FIXED2PI -
			 ObjectData->phase);

	ObjectData->phase_offset = fmod(offset, 1.0);
	NotifySuper(class, object, msg,
		    AHIA_LFO_PhaseOffset, (ULONG) (ObjectData->phase_offset *
						   FIXED2PI),
		    TAG_DONE);
	update_iq = TRUE;
	break;
      }

      case AHIA_LFO_PhaseOffset:
	ObjectData->phase_offset = tag->ti_Data / FIXED2PI;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	update_iq = TRUE;
	break;
	
      case AHIA_LFO_Amplitude:
	ObjectData->amplitude = tag->ti_Data;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	update_iq = TRUE;
	break;

      case AHIA_LFO_Bias:
	ObjectData->bias = tag->ti_Data;
	NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	update_iq = TRUE;
	break;
	
      case AHIA_LFO_Waveform:
	switch (tag->ti_Data) {
	  case AHIV_LFO_Sine:
	  case AHIV_LFO_Square:
	  case AHIV_LFO_Triangle:
	  case AHIV_LFO_Sawtooth:
	    ObjectData->waveform = tag->ti_Data;
	    NotifySuper(class, object, msg, tag->ti_Tag, tag->ti_Data, TAG_DONE);
	    update_iq = TRUE;
	    break;

	  default:
	    SetSuperAttrs(class, object,
			  AHIA_Error, AHIE_LFO_InvalidWaveform,
			  TAG_DONE);
	    break;
	    
	}
	break;

      case AHIA_TickProcessor_Tick: {
	Object* buffer  = (Object*) tag->ti_Data;
	ULONG   freq_hi = 0;
	ULONG   freq_lo = 0;
	ULONG   time_hi = 0;
	ULONG   time_lo = 0;
	double  time;
	double  freq;

	GetAttr(AHIA_Buffer_SampleFreqInt,   buffer, &freq_hi);
	GetAttr(AHIA_Buffer_SampleFreqFract, buffer, &freq_lo);
	GetAttr(AHIA_Buffer_TimestampHigh,   buffer, &time_hi);
	GetAttr(AHIA_Buffer_TimestampLow,    buffer, &time_lo);

	freq = freq_hi + ldexp(freq_lo, -32);
	time = ldexp(time_hi, 32) + time_lo;

	ObjectData->ticktime = time / freq;
	ObjectData->tickfreq = freq;
	update_iq = TRUE;
	break;
      }
	
      case AHIA_LFO_Tick:
	ObjectData->ticktime = tag->ti_Data / ObjectData->tickfreq;
	update_iq = TRUE;
	break;

      case AHIA_LFO_TickFreq:
	ObjectData->tickfreq = tag->ti_Data;
	update_iq = TRUE;
	break;
	
      default:
	break;
    }
  }

  if (update_iq) {
    calc_iq(class, object, msg);
  }
  
  return 0;
}


/******************************************************************************
** MethodGet ******************************************************************
******************************************************************************/

BOOL
MethodGet(Class* class, Object* object, struct opGet* msg) {
  struct ClassData* ClassData = (struct ClassData*) class->cl_UserData;
  struct ObjectData* ObjectData = (struct ObjectData*) INST_DATA(class, object);

  switch (msg->opg_AttrID) {
    case AHIA_Title:
      *msg->opg_Storage = (ULONG) "AHI LFO";
      break;

    case AHIA_Description:
      *msg->opg_Storage = (ULONG) "Low-frequency oscillator";
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
      
    case AHIA_LFO_Frequency:
      *msg->opg_Storage = (ULONG) (ObjectData->frequency * 65536);
      break;

    case AHIA_LFO_Phase:
      *msg->opg_Storage = (ULONG) (ObjectData->phase * FIXED2PI);
      break;

    case AHIA_LFO_PhaseOffset:
      *msg->opg_Storage = (ULONG) (ObjectData->phase_offset * FIXED2PI);
      break;
      
    case AHIA_LFO_Amplitude:
      *msg->opg_Storage = ObjectData->amplitude;
      break;

    case AHIA_LFO_Bias:
      *msg->opg_Storage = ObjectData->bias;
      break;
	
    case AHIA_LFO_Waveform:
      *msg->opg_Storage = ObjectData->waveform;
      break;

    case AHIA_LFO_I:
      *msg->opg_Storage = (LONG) ObjectData->i;
      break;

    case AHIA_LFO_Q:
      *msg->opg_Storage = (LONG) ObjectData->q;
      break;
      
    case AHIA_LFO_TickFreq:
      *msg->opg_Storage = (ULONG) ObjectData->tickfreq;
      break;

    default:
      return FALSE;
  }

  return TRUE;
}
