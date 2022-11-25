/* $Header: suffix.h,v 66.1 90/01/15 14:47:16 pfm Exp $ */

/*
 * Suffix/include definitions
 *
 * Author: Peter J. Nicklin
 */
#ifndef SUFFIX_H
#define SUFFIX_H

/*
 * Suffix types 
 */
#define SFXHEAD			'h'	/* header file name suffix */
#define SFXOBJ			'o'	/* object file name suffix */
#define SFXOUT			'x'	/* executable file name suffix */
#define SFXSRC			's'	/* source file name suffix */
#define SFXNULL			0	/* null suffix */

/*
 * Suffix/include table structs
 */
typedef struct _mapinclude
	{
	char *incspec;			/* user spec for include files */
	int inctyp;			/* type of included file */
	} MAPINCLUDE;

typedef struct _suffix
	{
	char *suffix;			/* points to a suffix */
	int sfxtyp;			/* type of file name suffix */
	int inctyp;			/* type of included file */
	char *incspec;			/* default included file user spec */
	} SUFFIX;

typedef struct _sfxblk
	{
	SUFFIX sfx;			/* suffix struct */
	struct _sfxblk *next;		/* ptr to next suffix list block */
	} SFXBLK;

#endif /* SUFFIX_H */
