#ifndef CLASSES_AHI_INTERNAL_H
#define CLASSES_AHI_INTERNAL_H

/*
**	$VER: ahi_internal.h 7.0 (2.7.2003)
**
**	Private AHI macro definitions
**
**	(C) Copyright 2003 Martin Blom
**	All Rights Reserved.
**
** (TAB SIZE: 8)
*/

#define _AHI_INCLUDE_VERSION		7

/*****************************************************************************/

#if defined(__cplusplus) && !defined(AHI_NO_NAMESPACES)
# define _AHIA(prefix,name,value)	a ## name = value
# define _AHIM(prefix,name,value)	m ## name = value
# define _AHIV(prefix,name,value)	v ## name = value
# define _AHIB(prefix,name,value)	b ## name = value, \
					f ## name = (1<<value)
# define _AHIP(prefix,name)		name
# define _AHIST(name,value)		st ## name = value
# define _AHIE(prefix,name,value)	e ## name = value
#else
# define _AHIA(prefix,name,value)	_AHIA2(prefix,name ## ,value)
# define _AHIM(prefix,name,value)	_AHIM2(prefix,name ## ,value)
# define _AHIV(prefix,name,value)	_AHIV2(prefix,name ## ,value)
# define _AHIB(prefix,name,value)	_AHIB2(prefix,name ## ,value)
# define _AHIP(prefix,name)		_AHIP2(prefix,name ## )
# define _AHIE(prefix,name,value)	_AHIE2(prefix,name ## ,value)
# define _AHIA2(prefix,name,value)	AHIA ## prefix ## _ ## name = value
# define _AHIM2(prefix,name,value)	AHIM ## prefix ## _ ## name = value
# define _AHIV2(prefix,name,value)	AHIV ## prefix ## _ ## name = value
# define _AHIB2(prefix,name,value)	AHIB ## prefix ## _ ## name = value, \
					AHIF ## prefix ## _ ## name = (1<<value)
# define _AHIP2(prefix,name)		AHIP ## prefix ## _ ## name
# define _AHIST(name,value)		AHIST_ ## name = value
# define _AHIE2(prefix,name,value)	AHIE ## prefix ## _ ## name = value
#endif /* __cplusplus && !AHI_NO_NAMESPACES */
  
/*****************************************************************************/

#define _AHI_LITTLE_ENDIAN	1234
#define _AHI_BIG_ENDIAN		4321

#if defined(__PPC__) || defined(__mc68000__)
# define _AHI_BYTE_ORDER	_AHI_BIG_ENDIAN
# define _AHI_FLOAT_WORD_ORDER	_AHI_BIG_ENDIAN
#else
# if defined(__i386__) && defined(__BIG_ENDIAN__)
#  define _AHI_BYTE_ORDER	_AHI_BIG_ENDIAN
# else
#  define _AHI_BYTE_ORDER	_AHI_LITTLE_ENDIAN
# endif
# define _AHI_FLOAT_WORD_ORDER	_AHI_LITTLE_ENDIAN
#endif

/*****************************************************************************/

/* FIXME: Register attribute and method space! */

#define _AHIM_Dummy		0x4D420000
#define _AHIA_Dummy		-851312640 /* 0xCD420000 */
#define _AHIE_Dummy		0x4D420000
    
/* Attribute and tag numbers are allocated sequentially in order to
   make it easier to get good hashing and to pollute the BOOPSI
   method/attribute space less. The following GNU commands are handy
   for finding the last offset used:

   grep '(_AHI[AME]_Dummy' `find . -name '*.h'` |
        sed 's,.*(_AHI\([AM]\)_Dummy.*+\([0-9]*\)).*,\2 \1,' |
        sort -n -r | sort -s -k2 -u

   The following commands are handy to check that there are no
   conflicts.

   grep '(_AHI[AME]_Dummy' `find . -name '*.h'` |
        sed 's,.*(_AHI\([AM]\)_Dummy.*+\([0-9]*\)).*,\2 \1,' |
        sort -n | uniq -d
*/

#endif /* CLASSES_AHI_INTERNAL_H */
