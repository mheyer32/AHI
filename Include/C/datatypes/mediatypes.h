#ifndef DATATYPES_MEDIATYPES_H
#define DATATYPES_MEDIATYPES_H

#ifndef DATATYPES_SOUNDCLASS_AHI_H
#include <datatypes/soundclass_ahi.h>
#endif

/*****************************************************************************/

/*** "Embedded mode" attributes ***/

/* Fake data header magic. This tag is guaranteed to be the first tag
   in the data header */
#define MTA_MAGIC		(_AHIA_Dummy+72)

/* Total length of fake data header (i.e., offset of real data in stream) */
#define MTA_Length		(_AHIA_Dummy+73)

/* The datatype group */
#define MTA_GroupID		(DTA_GroupID)

/* AHI ID (for sounds) */
#define MTA_ID_AHI		(_AHIA_Dummy+74)

/* FourCC (Windows) ID (for video) */
#define MTA_ID_FourCC		(_AHIA_Dummy+75)

/* A raw, little endian, WaveFormat structure (Windows, for sound) */
#define MTA_WaveFormat		(_AHIA_Dummy+76)

/* The sample frequency (for sound) */
#define MTA_SamplesPerSec	(_AHIA_Dummy+77)

/* The datatype that embedded us */
#define MTA_Parent		(_AHIA_Dummy+78)

/*** Contants ***/

/* Magic ("MTv1") */
#define MT_MAGIC		0x4D445430

/*****************************************************************************/

/*** "Embedded mode" methods ***/

#define MTM_INDEX		(_AHIM_Dummy+34)

#define MTM_STREAM_OPEN		(_AHIM_Dummy+35)
#define MTM_STREAM_CLOSE	(_AHIM_Dummy+36)
#define MTM_STREAM_READ		(_AHIM_Dummy+37)
#define MTM_STREAM_WRITE	(_AHIM_Dummy+38)
#define MTM_STREAM_SEEK		(_AHIM_Dummy+39)
#define MTM_STREAM_ENTRY	(_AHIM_Dummy+40)
#define MTM_STREAM_EXIT		(_AHIM_Dummy+41)
#define MTM_STREAM_GETSIZE	(_AHIM_Dummy+42)

/*****************************************************************************/

/* MTM_INDEX */

struct mtIndex
{
    ULONG MethodID;
    ULONG TimeStamp;		/* 1/fps resolution (will be updated) */

    /* The rest is filled by MTM_INDEX */
    ULONG Frame;		/* Frame number or ~0 */
    ULONG Duration;		/* 1/fps resolution or ~0 */
    ULONG Zero;			/* Always 0 for now */
    ULONG FileOffset;
    ULONG Size;
};


/* MTM_STREAM_OPEN */

struct mtStreamOpen
{
    ULONG MethodID;
    ULONG StreamID;
    ULONG Mode;			/* MODE_OLDFILE/MODE_NEWFILE/MODE_READWRITE */
};


/* MTM_STREAM_CLOSE */

struct mtStreamClose
{
    ULONG MethodID;
    APTR  Stream;
};


/* MTM_STREAM_READ, MTM_STREAM_WRITE */

struct mtStreamRW
{
    ULONG MethodID;
    APTR  Stream;
    APTR  Buffer;
    LONG  Size;
};


/* MTM_STREAM_SEEK */

struct mtStreamSeek
{
    ULONG MethodID;
    APTR  Stream;
    LONG  Position;
    LONG  Mode;                 /* OFFSET_BEGINNING/OFFSET_CURRENT/OFFSET_END */
};


/* MTM_STREAM_ENTRY */

struct mtStreamEntry
{
    ULONG MethodID;
    ULONG TimeStamp;		/* 1/fps resolution */
    ULONG Flags;		/* Set to 0 */
};


/* MTM_STREAM_EXIT */

struct mtStreamExit
{
    ULONG MethodID;
    APTR  Stream;
};


/* MTM_STREAM_GETSIZE */

struct mtStreamGetSize
{
    ULONG MethodID;
    ULONG StreamID;
};


#endif /* DATATYPES_MEDIATYPES_H */
