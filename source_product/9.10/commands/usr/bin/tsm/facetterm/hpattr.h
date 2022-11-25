/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: hpattr.h,v 66.3 90/09/20 12:12:25 kb Exp $ */
				/* attribute character is stored here */
#define	ATTR_HP_ATTRIBUTE	0x0000FF00
				/* number of bits to shift ATTR_HP_ATTRIBUTE
				   to turn into a character.
				*/
#define ATTR_HP_ATTRIBUTE_SHIFT 8

				/* this bit is set on for all characters
				   to distinguish a space from a cleared
				   character position.  A space has attributes.
				   A cleared character position does not.
				*/
#define ATTR_HP_CHAR_PRESENT	0x00800000

#define	ATTR_HP_CHARSET		0x003F0000
#define ATTR_HP_CHARSET_SHIFT	16
#define	ATTR_HP_CHARSET_SET	0x00000020
#define	ATTR_HP_CHARSET_ALT	0x00000010
#define	ATTR_HP_CHARSET_SELECT	0x0000000F

#define ATTR_HP_COLOR		0x0F000000
#define ATTR_HP_COLOR_SHIFT	24
#define ATTR_HP_COLOR_PRESENT	0x00000008
#define ATTR_HP_COLOR_VALUE	0x00000007

#define ATTR_HP_ALL  (ATTR_HP_ATTRIBUTE | ATTR_HP_CHARSET | ATTR_HP_COLOR)
