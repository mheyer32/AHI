#ifndef DEVICES_AHI_H
#define DEVICES_AHI_H

/*
**	$VER: ahi.h 7.0 (2.7.2003)
**
**	ahi.device definitions
**
**	(C) Copyright 1994-2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

/*****************************************************************************/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifndef LIBRARIES_IFFPARSE_H
#include <libraries/iffparse.h>
#endif

#ifndef DEVICES_AHI_TYPES_H
#include <classes/ahi_types.h>
#endif

#if !defined(DEVICES_AHI_OBSOLETE_H) && !defined(AHI_V7_NAMES_ONLY)
#include <devices/ahi_obsolete.h>
#endif

/*****************************************************************************/

/*** DEFS */

#define AHINAME			"ahi.device"
#define AHI_INVALID_ID		(~0UL)			/* Invalid Audio ID */

 /* Error codes */
#define AHIE_OK			(0UL)			/* No error */
#define AHIE_NOMEM		(1UL)			/* Out of memory */
#define AHIE_BADSOUNDTYPE	(2UL)			/* Unknown sound type */
#define AHIE_BADSAMPLETYPE	(3UL)			/* Unknown/unsupported sample type */
#define AHIE_ABORTED		(4UL)			/* User-triggered abortion */
#define AHIE_UNKNOWN		(5UL)			/* Error, but unknown */
#define AHIE_HALFDUPLEX		(6UL)			/* CMD_WRITE/CMD_READ failure */


/* DEVICE INTERFACE DEFINITIONS FOLLOWS ************************************/

 /* Device units */

#define AHI_DEFAULT_UNIT	(0U)
#define AHI_NO_UNIT		(255U)


 /* The preference file */

#define ID_AHIU MAKE_ID('A','H','I','U')
#define ID_AHIG MAKE_ID('A','H','I','G')

struct AHIUnitPrefs
{
	UBYTE	ahiup_Unit;
        UBYTE	ahiup_Pad;
        UWORD	ahiup_Channels;
        ULONG	ahiup_AudioMode;
        ULONG	ahiup_Frequency;
        Fixed	ahiup_MonitorVolume;
        Fixed	ahiup_InputGain;
        Fixed	ahiup_OutputVolume;
        ULONG	ahiup_Input;
        ULONG	ahiup_Output;
};

struct AHIGlobalPrefs
{
	UWORD	ahigp_DebugLevel;			/* Range: 0-3 (for None, Low,
							   High and All) */
	BOOL	ahigp_DisableSurround;
	BOOL	ahigp_DisableEcho;
	BOOL	ahigp_FastEcho;
	Fixed	ahigp_MaxCPU;
	BOOL	ahigp_ClipMasterVolume;
	UWORD	ahigp_Pad;
	Fixed	ahigp_AntiClickTime;			/* In seconds (V5) */
};

 /* Debug levels */
#define AHI_DEBUG_NONE		(0U)
#define AHI_DEBUG_LOW		(1U)
#define AHI_DEBUG_HIGH		(2U)
#define AHI_DEBUG_ALL		(3U)

 /* AHIRequest */

struct AHIRequest
{
	struct	IOStdReq	 ahir_Std;		/* Standard IO request */
	UWORD			 ahir_Version;		/* Needed version */
/* --- New for V4, they will be ignored by V2 and earlier --- */
	UWORD			 ahir_Pad1;
	ULONG			 ahir_Private[2];	/* Hands off! */
	ULONG			 ahir_Type;		/* Sample format */
	ULONG			 ahir_Frequency;	/* Sample/Record frequency */
	Fixed			 ahir_Volume;		/* Sample volume */
	Fixed			 ahir_Position;		/* Stereo position */
	struct AHIRequest 	*ahir_Link;		/* For double buffering */
};

 /* Flags for OpenDevice() */

#define	AHIDF_NOMODESCAN	(1UL<<0)
#define	AHIDB_NOMODESCAN	(0UL)

#endif /* DEVICES_AHI_H */
