#ifndef DEVICES_AHI_TYPES_H
#define DEVICES_AHI_TYPES_H

/*
**	$VER: ahi_types.h 7.0 (2.7.2003)
**
**	AHI type definitions
**
**	(C) Copyright 2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef DEVICES_AHI_INTERNAL_H
#include <classes/ahi_internal.h>
#endif

/*****************************************************************************/

#if !defined(__AROS__) && INCLUDE_VERSION < 50

/* A 64 bit signed integer */
typedef long long QUAD;

/* A 64 bit unsigned integer */
typedef unsigned long long UQUAD;

#endif

#ifndef EIGHTSVX_H 				/* Do not define Fixed twice */

/* A 32 bit fixed-point value. 16 bits to the left of the point and 16
 * bits to the right */
typedef LONG	Fixed;

#endif /* EIGHTSVX_H */

/* A 32 bit fixed-point value. 4 bits to the left of the point and 28
 * bits to the right */
typedef LONG	Fixed28;

/* A 64 bit fixed-point value. 32 bits to the left of the point and 32
 * bits to the right */
typedef QUAD	Fixed64;

/* A 32 bit fixed point value (16.16 bits) representing a dB value */
typedef Fixed	dBFixed;

/* A stereo position type. 0 is far left and 0x10000 is far right */
typedef Fixed	sposition;

/*****************************************************************************/

#define AHIST_TYPE_MASK		(0x00000ffdUL) /* All bits that defines type */
#define AHIST_T_MASK		(0x000000fdUL) /* Basic type mask (B/W/L ...) */
#define AHIST_D_MASK		(0x00000700UL) /* Domain mask */
#define AHIST_E_MASK		(0x00000800UL) /* Endian mask */
#define AHIST_C_MASK		(0x001f0002UL) /* # of chanels (encoded) */
#define AHIST_CF_MASK		(0x00e00000UL) /* Channel flags mask */

/* Channels */
#define AHIST_C_ENCODE(x)	(((((x)-1)&62)<<15)|(((x-1))&1)<<1)
#define AHIST_C_DECODE(x)	((((((x)>>15)&62)|(((x)>>1)&1))+1)&63)

/* Channel flags */
#define AHIST_CF_SURROUND1	(0x00200000UL) /* Mono surround at the end */
#define AHIST_CF_SURROUND2	(0x00400000UL) /* Left/right surround at end */
#define AHIST_CF_LFE		(0x00800000UL) /* LFE at end */

/* Primitive data types */
#define AHIST_T_BYTE		(0x00000000UL)
#define AHIST_T_WORD		(0x00000001UL)
#define AHIST_T_LONG		(0x00000008UL)
#define AHIST_T_QUAD		(0x00000009UL)
#define AHIST_T_UBYTE		(0x00000004UL)
#define AHIST_T_UWORD		(0x00000005UL)
#define AHIST_T_ULONG		(0x0000000cUL)
#define AHIST_T_UQUAD		(0x0000000dUL)
#define AHIST_T_FLOAT		(0x00000010UL)
#define AHIST_T_DOUBLE		(0x00000011UL)

/* Domain */
#define AHIST_D_DISCRETE	(0x00000000UL)
#define AHIST_D_AMBISONIC	(0x00000100UL)
#define AHIST_D_FOURIER		(0x00000200UL)
#define AHIST_D_COMPRESSED	(0x00000300UL)

#define AHIST_E_BIG		(0x00000000UL)
#define AHIST_E_LITTLE		(0x00000800UL)

/* Endian definitions */

#if _AHI_BYTE_ORDER == _AHI_BIG_ENDIAN
# define AHIST_IE		AHIST_E_BIG
#else
# define AHIST_IE		AHIST_E_LITTLE
#endif

#if _AHI_FLOAT_WORD_ORDER == _AHI_BIG_ENDIAN
# define AHIST_FE		AHIST_E_BIG
#else
# define AHIST_FE		AHIST_E_LITTLE
#endif

/* A few channel combinations. We make a distinction between movie and
 * music modes in some cases, since it makes a difference when
 * decoding Ambisonic formats */

/* Mono (C) */
#define _AHIST_CH_1		(AHIST_D_DISCRETE|AHIST_C_ENCODE(1))

/* Stereo (L, R) */
#define _AHIST_CH_2		(AHIST_D_DISCRETE|AHIST_C_ENCODE(2))

/* 2.1 channels (L, R, LFE) */
#define _AHIST_CH_2_1		(AHIST_D_DISCRETE|AHIST_C_ENCODE(3)|\
				 AHIST_CF_LFE)

/* 3 channels (L, R, C) */
#define _AHIST_CH_3		(AHIST_D_DISCRETE|AHIST_C_ENCODE(3))

/* 3.1 channels (L, R, C, LFE) */
#define _AHIST_CH_3_1		(AHIST_D_DISCRETE|AHIST_C_ENCODE(4)|\
				 AHIST_CF_LFE)

/* 4 channels, music mode (L, R, RL, RR) */
#define _AHIST_CH_4		(AHIST_D_DISCRETE|AHIST_C_ENCODE(4))

/* 4 channels, pro-logic mode (L, R, C, SL/SR) */
#define _AHIST_CH_4_PROLOGIC	(AHIST_D_DISCRETE|AHIST_C_ENCODE(4)|\
				 AHIST_CF_SURROUND1)

/* 4 channels, movie mode (L, R, SL, SR) */
#define _AHIST_CH_4_MOVIE	(AHIST_D_DISCRETE|AHIST_C_ENCODE(4)|\
				 AHIST_CF_SURROUND2)

/* 4.1 channels, music mode (L, R, RL, RR, LFE) */
#define _AHIST_CH_4_1		(AHIST_D_DISCRETE|AHIST_C_ENCODE(5)|\
				 AHIST_CF_LFE)

/* 4.1 channels, movie mode (L, R, SL, SR, LFE) */
#define _AHIST_CH_4_1_MOVIE	(AHIST_D_DISCRETE|AHIST_C_ENCODE(5)|\
				 AHIST_CF_LFE|AHIST_CF_SURROUND2)

/* 5.1 channels, music mode (L, R, RL, RR, C, LFE) */
#define _AHIST_CH_5_1		(AHIST_D_DISCRETE|AHIST_C_ENCODE(6)|\
				 AHIST_CF_LFE)

/* 5.1 channels, movie mode (L, R, SL, SR, C, LFE) */
#define _AHIST_CH_5_1_MOVIE	(AHIST_D_DISCRETE|AHIST_C_ENCODE(6)|\
				 AHIST_CF_LFE|AHIST_CF_SURROUND2)

/* 7.1 channels (L, R, RL, RR, SL, SR, C, LFE) */
#define _AHIST_CH_7_1		(AHIST_D_DISCRETE|AHIST_C_ENCODE(8)|\
				 AHIST_CF_LFE|AHIST_CF_SURROUND2)


/* First order ambisonic (B-Format) */
#define _AHIST_CH_3D1		(AHIST_D_AMBISONIC|AHIST_C_ENCODE(4))

/* Second order ambisonic (Furse-Malham Higher Order Format) */
#define _AHIST_CH_3D2		(AHIST_D_AMBISONIC|AHIST_C_ENCODE(9))


/* Frequency domain: Mono (C) */
#define _AHIST_CH_FT_1		(AHIST_D_FOURIER|AHIST_C_ENCODE(1))

/* Frequency domain: Stereo (L, R) */
#define _AHIST_CH_FT_2		(AHIST_D_FOURIER|AHIST_C_ENCODE(2))

/* Frequency domain: 2.1 channels (L, R, LFE) */
#define _AHIST_CH_FT_2_1	(AHIST_D_FOURIER|AHIST_C_ENCODE(3)|\
				 AHIST_CF_LFE)

/* Frequency domain: 3 channels (L, R, C) */
#define _AHIST_CH_FT_3		(AHIST_D_FOURIER|AHIST_C_ENCODE(3))

/* Frequency domain: 3.1 channels (L, R, C, LFE) */
#define _AHIST_CH_FT_3_1	(AHIST_D_FOURIER|AHIST_C_ENCODE(4)|\
				 AHIST_CF_LFE)

/* Frequency domain: 4 channels, music mode (L, R, RL, RR) */
#define _AHIST_CH_FT_4		(AHIST_D_FOURIER|AHIST_C_ENCODE(4))

/* Frequency domain: 4 channels, pro-logic mode (L, R, C, SL/SR) */
#define _AHIST_CH_FT_4_PROLOGIC	(AHIST_D_FOURIER|AHIST_C_ENCODE(4)|\
				 AHIST_CF_SURROUND1)

/* Frequency domain: 4 channels, movie mode (L, R, SL, SR) */
#define _AHIST_CH_FT_4_MOVIE	(AHIST_D_FOURIER|AHIST_C_ENCODE(4)|\
				 AHIST_CF_SURROUND2)

/* Frequency domain: 4.1 channels, music mode (L, R, RL, RR, LFE) */
#define _AHIST_CH_FT_4_1	(AHIST_D_FOURIER|AHIST_C_ENCODE(5)|\
				 AHIST_CF_LFE)

/* Frequency domain: 4.1 channels, movie mode (L, R, SL, SR, LFE) */
#define _AHIST_CH_FT_4_1_MOVIE	(AHIST_D_FOURIER|AHIST_C_ENCODE(5)|\
				 AHIST_CF_LFE|AHIST_CF_SURROUND2)

/* Frequency domain: 5.1 channels, music mode (L, R, RL, RR, C, LFE) */
#define _AHIST_CH_FT_5_1	(AHIST_D_FOURIER|AHIST_C_ENCODE(6)|\
				 AHIST_CF_LFE)

/* Frequency domain: 5.1 channels, movie mode (L, R, SL, SR, C, LFE) */
#define _AHIST_CH_FT_5_1_MOVIE	(AHIST_D_FOURIER|AHIST_C_ENCODE(6)|\
				 AHIST_CF_LFE|AHIST_CF_SURROUND2)

/* Frequency domain: 7.1 channels (L, R, RL, RR, SL, SR, C, LFE) */
#define _AHIST_CH_FT_7_1	(AHIST_D_FOURIER|AHIST_C_ENCODE(8)|\
				 AHIST_CF_LFE|AHIST_CF_SURROUND2)

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
namespace AHI {
#endif

  enum {

    /* Some common types follows ...  First, the old types: */

    /* No type at all, actually */
    _AHIST(NOTYPE,		(~0UL)),
    
    /* Mono, 8 bit signed (BYTE) */
    _AHIST(M8S,			(AHIST_IE|AHIST_T_BYTE|_AHIST_CH_1)),

    /* Mono, 16 bit signed (WORD) */
    _AHIST(M16S,		(AHIST_IE|AHIST_T_WORD|_AHIST_CH_1)),

    /* Stereo, 8 bit signed (2×BYTE) */
    _AHIST(S8S,			(AHIST_IE|AHIST_T_BYTE|_AHIST_CH_2)),

    /* Stereo, 16 bit signed (2×WORD) */
    _AHIST(S16S,		(AHIST_IE|AHIST_T_WORD|_AHIST_CH_2)),

    /* Mono, 32 bit signed (LONG) */
    _AHIST(M32S,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_1)),

    /* Stereo, 32 bit signed (2×LONG) */
    _AHIST(S32S,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_2)),

    /* Mono, 8 bit unsigned (UBYTE) -- obsolete in V4, removed in V5,
     * resurrected in V7 */
    _AHIST(M8U,			(AHIST_IE|AHIST_T_UBYTE|_AHIST_CH_1)),
  };

  enum {

    /* Now, all common channel configurations (see above) for
     * floats. These formats are used internally in the audio
     * processing pipeline. (V7) */

    _AHIST(F1,			(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_1)),
    _AHIST(F2,			(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_2)),
    _AHIST(F2_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_2_1)),
    _AHIST(F3,			(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_3)),
    _AHIST(F3_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_3_1)),
    _AHIST(F4,			(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_4)),
    _AHIST(F4_PROLOGIC,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_4_PROLOGIC)),
    _AHIST(F4_MOVIE,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_4_MOVIE)),
    _AHIST(F4_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_4_1)),
    _AHIST(F4_1_MOVIE,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_4_1_MOVIE)),
    _AHIST(F5_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_5_1)),
    _AHIST(F5_1_MOVIE,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_5_1_MOVIE)),
    _AHIST(F7_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_7_1)),
    _AHIST(F3D1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_3D1)),
    _AHIST(F3D2,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_3D2)),
    _AHIST(FFT1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_1)),
    _AHIST(FFT2,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_2)),
    _AHIST(FFT2_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_2_1)),
    _AHIST(FFT3,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_3)),
    _AHIST(FFT3_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_3_1)),
    _AHIST(FFT4,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_4)),
    _AHIST(FFT4_PROLOGIC,	(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_4_PROLOGIC)),
    _AHIST(FFT4_MOVIE,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_4_MOVIE)),
    _AHIST(FFT4_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_4_1)),
    _AHIST(FFT4_1_MOVIE,	(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_4_1_MOVIE)),
    _AHIST(FFT5_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_5_1)),
    _AHIST(FFT5_1_MOVIE,	(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_5_1_MOVIE)),
    _AHIST(FFT7_1,		(AHIST_FE|AHIST_T_FLOAT|_AHIST_CH_FT_7_1)),


    /* And some common channel configurations again, this time for 32
     * bit integers.  These are used as output formats for drivers, in
     * case they don't support floats. (AHIST_L1 == AHIST_M32S and
     * AHIST_L2 == AHIST_S32S) (V7) */

    _AHIST(L1,			(AHIST_IE|AHIST_T_LONG|_AHIST_CH_1)),
    _AHIST(L2,			(AHIST_IE|AHIST_T_LONG|_AHIST_CH_2)),
    _AHIST(L2_1,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_2_1)),
    _AHIST(L3,			(AHIST_IE|AHIST_T_LONG|_AHIST_CH_3)),
    _AHIST(L3_1,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_3_1)),
    _AHIST(L4,			(AHIST_IE|AHIST_T_LONG|_AHIST_CH_4)),
    _AHIST(L4_PROLOGIC,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_4_PROLOGIC)),
    _AHIST(L4_MOVIE,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_4_MOVIE)),
    _AHIST(L4_1,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_4_1)),
    _AHIST(L4_1_MOVIE,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_4_1_MOVIE)),
    _AHIST(L5_1,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_5_1)),
    _AHIST(L5_1_MOVIE,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_5_1_MOVIE)),
    _AHIST(L7_1,		(AHIST_IE|AHIST_T_LONG|_AHIST_CH_7_1)),

    /* Drivers may also be able to output a few compressed
     * formats. One could write ahi-sound.class subclasses that can
     * decode these formats as well, even though that is perhaps
     * better left to the Datatypes subsystem (V7) */

    _AHIST(MPEG1_L1,		(AHIST_D_COMPRESSED|0x00)),
    _AHIST(MPEG1_L2,		(AHIST_D_COMPRESSED|0x01)),
    _AHIST(MPEG1_L3,		(AHIST_D_COMPRESSED|0x04)),
    _AHIST(MPEG2_AAC,		(AHIST_D_COMPRESSED|0x05)),
    _AHIST(MPEG4_AAC,		(AHIST_D_COMPRESSED|0x08)),
    _AHIST(AC3,			(AHIST_D_COMPRESSED|0x09)),
    _AHIST(DTS,			(AHIST_D_COMPRESSED|0x0c)),
    _AHIST(WMA_V1,		(AHIST_D_COMPRESSED|0x0d)),
    _AHIST(WMA_V2,		(AHIST_D_COMPRESSED|0x10)),
    _AHIST(WMA_V3,		(AHIST_D_COMPRESSED|0x11)),
    _AHIST(VORBIS,		(AHIST_D_COMPRESSED|0x14))
  };

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
}
#endif


#endif /* DEVICES_AHI_TYPES_H */
