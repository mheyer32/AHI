
#include <config.h>

#include <libraries/ahi_sub.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include "DriverData.h"
#include "FileFormats.h"
#include "library.h"

#define dd ((struct FilesaveData*) AudioCtrl->ahiac_DriverData)

void ulong2extended (ULONG in, extended *ex);

/******************************************************************************
** Endian conversion **********************************************************
******************************************************************************/

#ifdef WORDS_BIGENDIAN

#define __htole_short(x) \
            ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))

#define __htole_long(x) \
            ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
             (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned short
htole_short( unsigned short x )
{
  return (unsigned short) __htole_short( x );
}

unsigned long
htole_long( unsigned long x )
{
  return __htole_long( x );
}

#define __htobe_short(x) (x)
#define __htobe_long(x)  (x)
#define htobe_short(x)   (x)
#define htobe_long(x)    (x)

#else

#define __htole_short(x) (x)
#define __htole_long(x)  (x)
#define htole_short(x)   (x)
#define htole_long(x)    (x)

#define __htobe_short(x) \
            ((((x) & 0xff00) >>  8) | (((x) & 0x00ff) << 8))

#define __htobe_long(x) \
            ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
             (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

unsigned short
htobe_short( unsigned short x )
{
  return (unsigned short) __htobe_short( x );
}

unsigned long
htobe_long( unsigned long x )
{
  return __htobe_long( x );
}

#endif


/******************************************************************************
** The slave process **********************************************************
******************************************************************************/

#undef SysBase

void SlaveEntry(void)
{
  struct ExecBase*        SysBase = *SysBasePtr;
  struct AHIAudioCtrlDrv* AudioCtrl;
  struct DriverBase*      AHIsubBase;
  struct FilesaveBase*    FilesaveBase;

  struct EIGHTSVXheader EIGHTSVXheader = // All NULLs will be filled later.
  { 
    ID_FORM, NULL, ID_8SVX,
    ID_VHDR, sizeof(Voice8Header),
    {
      NULL,
      0,
      0,
      NULL,
      1,
      sCmpNone,
      0x10000
    },
    ID_BODY, NULL
  };

  struct AIFFheader AIFFheader = // All NULLs will be filled later.
  { 
    ID_FORM, NULL, ID_AIFF,
    ID_COMM, sizeof(CommonChunk),
    {
      NULL,
      NULL,
      16,
      {
        0, { 0, 0 }
      }
    },
    ID_SSND, NULL,
    {
      0,
      0
    }
  };

  struct AIFCheader AIFCheader = // All NULLs will be filled later.
  { 
    ID_FORM, NULL, ID_AIFC,
    ID_FVER, sizeof(FormatVersionHeader), 
    {
      AIFCVersion1
    },
    ID_COMM, sizeof(ExtCommonChunk),
    {
      NULL,
      NULL,
      16,
      {
        0, { 0, 0 }
      },
      NO_COMPRESSION,
      { sizeof("not compressed") - 1,
	'n','o','t',' ','c','o','m','p','r','e','s','s','e','d' }
    },
    ID_SSND, NULL,
    {
      0,
      0
    }
  };

  struct STUDIO16FILE S16header = // All NULLs will be filled later.
  {
    S16FID,
    NULL,
    S16FINIT,
    S16_VOL_0,
    0,
    0,
    NULL,
    0,
    0,
    NULL,
    NULL,
    0,
    NULL,
    0,
    {
      0
    }
  };

  struct WAVEheader WAVEheader = // All NULLs will be filled later.
  {
    ID_RIFF, NULL, ID_WAVE,
    ID_fmt, __htole_long( sizeof(FormatChunk) ),
    {
      __htole_short( WAVE_PCM ),
      0,
      0,
      0,
      0,
      __htole_short( 16 )
    },
    ID_data, NULL
  };

  struct EasyStruct req =
  {
    sizeof (struct EasyStruct),
    0,
    (STRPTR) LibName,
    "Rendering finished.\nTo futher improve the quality of the sample,\n"
    "you can raise the volume to %ld%% and render again.",
    "OK",
  };

  BPTR lock = NULL,cd = NULL,file = NULL, file2 = NULL;
  LONG maxVolume = 0;
  ULONG signals, i, samplesAdd =0, samples = 0, length = 0;
  ULONG offset = 0, bytesInBuffer = 0, samplesWritten = 0, bytesWritten = 0;

  AudioCtrl    = (struct AHIAudioCtrlDrv*) FindTask( NULL )->tc_UserData;
  AHIsubBase   = (struct DriverBase*) dd->fs_AHIsubBase;
  FilesaveBase = (struct FilesaveBase*) AHIsubBase;

// We cannot handle stereo 8SVXs!
  if( (dd->fs_Format == FORMAT_8SVX) &&
      (AudioCtrl->ahiac_Flags & AHIACF_STEREO) )
  {
    goto quit;
  }

  if((dd->fs_SlaveSignal = AllocSignal(-1)) == -1)
  {
    goto quit;
  }

  if(!(lock = Lock(dd->fs_FileReq->fr_Drawer, ACCESS_READ)))
  {
    goto quit;
  }

  cd = CurrentDir(lock);

  switch(dd->fs_Format)
  {
    case FORMAT_8SVX:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &EIGHTSVXheader, sizeof EIGHTSVXheader);
      break;

    case FORMAT_AIFF:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &AIFFheader, sizeof AIFFheader);
      break;

    case FORMAT_AIFC:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &AIFCheader, sizeof AIFCheader);
      break;

    case FORMAT_S16:
      if (AudioCtrl->ahiac_Flags & AHIACF_STEREO)
      {
        char filename[256];
        int len;

        strncpy (filename, dd->fs_FileReq->fr_File, sizeof(filename) - 3);
        len = strlen(filename);

        if(len >= 2 && filename[len - 2] == '_'
           && (filename[len - 1] == 'L' || filename[len - 1] == 'R'))
        {
          filename[len - 1] = 'L';
        }
        else
        {
          strcat (filename, "_L");
        }

        if(!(file = Open(filename, MODE_NEWFILE))) goto quit;

        filename[strlen(filename) - 1] = 'R';
        if(!(file2 = Open(filename, MODE_NEWFILE))) goto quit;

        Write(file, &S16header, sizeof S16header);
        Write(file2, &S16header, sizeof S16header);
      }
      else
      {
        if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
        Write(file, &S16header, sizeof S16header);
      }
      break;

    case FORMAT_WAVE:
      if(!(file = Open(dd->fs_FileReq->fr_File, MODE_NEWFILE))) goto quit;
      Write(file, &WAVEheader, sizeof WAVEheader);
      break;
  }

  // Everything set up. Tell Master we're alive and healthy.
  Signal((struct Task *)dd->fs_MasterTask,1L<<dd->fs_MasterSignal);

  for(;;)
  {
    signals = SetSignal(0L,0L);
    if(signals & (SIGBREAKF_CTRL_C | 1L<<dd->fs_SlaveSignal))
    {
      break;
    }

    CallHookPkt(AudioCtrl->ahiac_PlayerFunc, AudioCtrl, NULL);
    CallHookPkt(AudioCtrl->ahiac_MixerFunc, AudioCtrl, dd->fs_MixBuffer);

    samplesAdd = AudioCtrl->ahiac_BuffSamples;
    samples    = samplesAdd;

    if(AudioCtrl->ahiac_Flags & AHIACF_STEREO)
    {
      samples <<= 1;
    }

// Search for loudest part in sample
    if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
    {
      for(i = 0; i < samples; i++)
        if(abs(((LONG *)dd->fs_MixBuffer)[i]) > maxVolume)
          maxVolume = abs(((LONG *)dd->fs_MixBuffer)[i]);
    }
    else
    {
      for(i = 0; i< samples; i++)
        if(abs(((WORD *)dd->fs_MixBuffer)[i]) > maxVolume)
          maxVolume = abs(((WORD *)dd->fs_MixBuffer)[i]);
    }

    if((AudioCtrl->ahiac_Flags & AHIACF_STEREO) && dd->fs_Format == FORMAT_S16)
    {
      samples >>= 1;  // Two buffers instead
    }

    if(offset+samples >= dd->fs_SaveBufferSize)
    {
      if((ULONG) Write(file, dd->fs_SaveBuffer, bytesInBuffer) != bytesInBuffer)
      {
        break;
      }
      if(file2 != NULL) {
        if((ULONG) Write(file2, dd->fs_SaveBuffer2, bytesInBuffer) != bytesInBuffer)
        {
          break;
        }
      }
      offset = 0;
      bytesInBuffer = 0;
    }

    switch(dd->fs_Format)
    {
      case FORMAT_8SVX:
        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          BYTE *dest = &((BYTE *) dd->fs_SaveBuffer)[offset];
          LONG *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
            *dest++ = *source++ >> 24;
        }
        else
        {
          BYTE *dest = &((BYTE *) dd->fs_SaveBuffer)[offset];
          WORD *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
            *dest++ = *source++ >> 8;
        }
        length = samples;
        break;

      case FORMAT_AIFF:
      case FORMAT_AIFC:
        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          LONG *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = *source++ >> 16;
          }
        }
        else
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          WORD *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = *source++;
          }
        }
        length = samples*2;
        break;

      case FORMAT_S16:
        switch(AudioCtrl->ahiac_Flags & (AHIACF_HIFI | AHIACF_STEREO))
        {
          case 0:
          {
            WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
            WORD *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest++ = *source++;
            }

            break;
          }

          case AHIACF_STEREO:
          {
            WORD *dest1 = &((WORD *) dd->fs_SaveBuffer)[offset];
            WORD *dest2 = &((WORD *) dd->fs_SaveBuffer2)[offset];
            WORD *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest1++ = *source++;
              *dest2++ = *source++;
            }

            break;
          }

          case AHIACF_HIFI:
          {
            WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
            LONG *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest++ = *source++ >> 16;
            }

            break;
          }

          case (AHIACF_HIFI | AHIACF_STEREO):
          {
            WORD *dest1 = &((WORD *) dd->fs_SaveBuffer)[offset];
            WORD *dest2 = &((WORD *) dd->fs_SaveBuffer2)[offset];
            LONG *source = dd->fs_MixBuffer;

            for(i = 0; i < samples; i++)
            {
              *dest1++ = *source++ >> 16;
              *dest2++ = *source++ >> 16;
            }

            break;
          }
        }

        length = samples*2;
        break;

      case FORMAT_WAVE:
        if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          LONG *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = htole_short( *source++ >> 16 );
          }
        }
        else
        {
          WORD *dest = &((WORD *) dd->fs_SaveBuffer)[offset];
          WORD *source = dd->fs_MixBuffer;

          for(i = 0; i < samples; i++)
          {
            *dest++ = htole_short( *source++ );
          }
        }
        length = samples*2;
        break;
    }

    offset          += samples;
    samplesWritten  += samplesAdd;
    bytesWritten    += length;
    bytesInBuffer   += length;
  }

  Write(file, dd->fs_SaveBuffer, bytesInBuffer);
  if(file2 != NULL)
  {
    Write(file2, dd->fs_SaveBuffer2, bytesInBuffer);
  }

  switch(dd->fs_Format)
  {
    case FORMAT_8SVX:
      EIGHTSVXheader.FORMsize = sizeof(EIGHTSVXheader)-8+bytesWritten;
      EIGHTSVXheader.VHDRchunk.oneShotHiSamples = samplesWritten;
      EIGHTSVXheader.VHDRchunk.samplesPerSec = AudioCtrl->ahiac_MixFreq;
      EIGHTSVXheader.BODYsize = bytesWritten;
      if(bytesWritten & 1)
        FPutC(file,'\0');   // Pad to even
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&EIGHTSVXheader,sizeof EIGHTSVXheader);
      break;

    case FORMAT_AIFF:
      AIFFheader.FORMsize = sizeof(AIFFheader)-8+bytesWritten;
      AIFFheader.COMMchunk.numChannels = (AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1);
      AIFFheader.COMMchunk.numSampleFrames = samplesWritten;
      ulong2extended(AudioCtrl->ahiac_MixFreq,&AIFFheader.COMMchunk.sampleRate);
      AIFFheader.SSNDsize = sizeof(SampledSoundHeader)+bytesWritten;
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&AIFFheader,sizeof AIFFheader);
      break;

    case FORMAT_AIFC:
      AIFCheader.FORMsize = sizeof(AIFCheader)-8+bytesWritten;
      AIFCheader.COMMchunk.numChannels = (AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1);
      AIFCheader.COMMchunk.numSampleFrames = samplesWritten;
      ulong2extended(AudioCtrl->ahiac_MixFreq,&AIFCheader.COMMchunk.sampleRate);
      AIFCheader.SSNDsize = sizeof(SampledSoundHeader)+bytesWritten;
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&AIFCheader,sizeof AIFCheader);
      break;

    case FORMAT_S16:
      S16header.S16F_RATE = AudioCtrl->ahiac_MixFreq;
      S16header.S16F_SAMPLES0 =
      S16header.S16F_SAMPLES1 = samplesWritten;
      S16header.S16F_SAMPLES2 = samplesWritten - 1;
      if (file2 == NULL)
      {
        S16header.S16F_PAN = S16_PAN_MID;
      }
      else
      {
        S16header.S16F_PAN = S16_PAN_LEFT;
      }

      Seek(file, 0, OFFSET_BEGINNING);
      Write(file, &S16header, sizeof S16header);
      if(file2 != NULL)
      {
        S16header.S16F_PAN = S16_PAN_RIGHT;
        Seek(file2,0,OFFSET_BEGINNING);
        Write(file2, &S16header, sizeof S16header);
      }
      break;   

    case FORMAT_WAVE:
    {
      short num_channels;
      short block_align;
      
      num_channels = AudioCtrl->ahiac_Flags & AHIACF_STEREO ? 2 : 1;
      block_align  = num_channels * 16 / 8;
      
      WAVEheader.FORMsize                   = htole_long( sizeof(WAVEheader)-8+bytesWritten );
      WAVEheader.FORMATchunk.numChannels    = htole_short( num_channels );
      WAVEheader.FORMATchunk.samplesPerSec  = htole_long( AudioCtrl->ahiac_MixFreq );
      WAVEheader.FORMATchunk.avgBytesPerSec = htole_long( AudioCtrl->ahiac_MixFreq * block_align );
      WAVEheader.FORMATchunk.blockAlign     = htole_short( block_align );
      WAVEheader.DATAsize                   = htole_long( bytesWritten );
      Seek(file,0,OFFSET_BEGINNING);
      Write(file,&WAVEheader,sizeof WAVEheader);
      break;
    }
  }

  if(AudioCtrl->ahiac_Flags & AHIACF_HIFI)
    maxVolume >>=16;

  if(maxVolume != 0)
  {
    EasyRequest(NULL, &req, NULL, 3276800/maxVolume );
  }

quit:
  if(file)
  {
    Close(file);
  }
  if(file2)
  {
    Close(file2);
  }
  if(lock)
  {
    CurrentDir(cd);
    UnLock(lock);
  }

  Forbid();
  dd->fs_SlaveTask = NULL;
  FreeSignal(dd->fs_SlaveSignal);
  dd->fs_SlaveSignal    = -1;
  // Tell the Master we're dying
  Signal((struct Task *)dd->fs_MasterTask,1L<<dd->fs_MasterSignal);
  // Multitaking will resume when we are dead.
}

/*
** Apple's 80-bit SANE extended has the following format:

 1       15      1            63
+-+-------------+-+-----------------------------+
|s|       e     |i|            f                |
+-+-------------+-+-----------------------------+
  msb        lsb   msb                       lsb

The value v of the number is determined by these fields as follows:
If 0 <= e < 32767,              then v = (-1)^s * 2^(e-16383) * (i.f).
If e == 32767 and f == 0,       then v = (-1)^s * (infinity), regardless of i.
If e == 32767 and f != 0,       then v is a NaN, regardless of i.
*/

void ulong2extended (ULONG in, extended *ex)
{
  ex->exponent = 31+16383;
  ex->mantissa[1] = 0;
  while(!(in & 0x80000000))
  {
    ex->exponent--;
    in <<= 1;
  }
  ex->mantissa[0] = in;
}
