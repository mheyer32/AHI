#ifndef DATATYPES_SOUNDCLASS_AHI_H
#define DATATYPES_SOUNDCLASS_AHI_H

#ifndef DATATYPES_SOUNDCLASS_H
#include <datatypes/soundclass.h>
#endif

#ifndef CLASSES_AHI_INTERNAL_H
#include <classes/ahi_internal.h>
#endif

/*****************************************************************************/

/* Sound attributes */

/* The supported decode formats */
#define SDTA_DecodeFormats (_AHIA_Dummy+68)

/* The supported encode formats */
#define SDTA_EncodeFormats (_AHIA_Dummy+69)

/* The number of sounds stored in the file */
#define SDTA_GetNumSounds (_AHIA_Dummy+70)

/* The index number of the sound to load */
#define SDTA_WhichSound (_AHIA_Dummy+71)


/*****************************************************************************/

/* Sound methods */

/* Used to decode sample frames from the sound stream */
#define SDTM_DECODECHUNK  (_AHIM_Dummy+28)

/* Used to encode sample frames from the sound stream */
#define SDTM_ENCODECHUNK  (_AHIM_Dummy+29)

/* Used to start playback (prepare for playback) */
#define SDTM_START        (_AHIM_Dummy+30)

/* Used to pause playback */
#define SDTM_PAUSE        (_AHIM_Dummy+31)

/* Used to stop playback */
#define SDTM_STOP         (_AHIM_Dummy+32)

/* Used to locate a sample frame in the (decoded) stream */
#define SDTM_LOCATE       (_AHIM_Dummy+33)


/*****************************************************************************/

/* SDTM_START, SDTM_LOCATE */

struct stdStart
{
    ULONG MethodID;
    ULONG Frames;
    ULONG StartFrameHigh;
    ULONG StartFrameLow;
};


/* SDTM_DECODECHUNK, SDTM_ENCODECHUNK */

struct stdChunk
{
    ULONG MethodID;
    ULONG Frames;
    ULONG StartFrameHigh;
    ULONG StartFrameLow;
    ULONG Format;
    APTR  Destination;
    ULONG DestinationSize;
};


/* When querying the number of sounds stored in a file, the following
 * value denotes "the number of sounds is unknown".
 */

#define SDTANUMSOUNDS_Unknown (0)


#endif /* DATATYPES_SOUNDCLASS_AHI_H */
