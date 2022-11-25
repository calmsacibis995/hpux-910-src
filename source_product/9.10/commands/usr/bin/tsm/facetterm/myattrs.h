/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: myattrs.h,v 66.3 90/09/20 12:14:58 kb Exp $ */
#define ATTR_ALTCHARSET	0x0100
#define ATTR_PROTECT	0x0200
#define	ATTR_INVIS	0x0400
#define	ATTR_BOLD	0x0800
#define ATTR_DIM	0x1000
#define	ATTR_BLINK	0x2000
#define ATTR_REVERSE	0x4000
#define ATTR_UNDERLINE	0x8000

#define MAX_FTATTR_ATTRIBUTES 24
#define ATTR_a		0x00010000
#define ATTR_b		0x00020000
#define ATTR_c		0x00040000
#define ATTR_d		0x00080000
#define ATTR_e		0x00100000
#define ATTR_f		0x00200000
#define ATTR_g		0x00400000
#define ATTR_h		0x00800000
#define ATTR_i		0x01000000
#define ATTR_j		0x02000000
#define ATTR_k		0x04000000
#define ATTR_l		0x08000000
#define ATTR_m		0x10000000
#define ATTR_n		0x20000000
#define ATTR_o		0x40000000
#define ATTR_p		0x80000000

#define ATTR_MAGIC	ATTR_g

#define FTCHAR_ATTR_MASK	0xFFFFFF00
#define FTCHAR_CHAR_MASK	0x000000FF
