/* $Header: suffix.c,v 66.1 90/01/15 14:46:03 pfm Exp $ */

/*
 * Author: Peter J. Nicklin
 */
#include <stdio.h>
#include "Mkmf.h"
#include "hash.h"
#include "macro.h"
#include "null.h"
#include "suffix.h"
#include "system.h"
#include "yesno.h"

static int SFX1[SFXTABSIZE];		/* single character suffixes */
static int INC1[SFXTABSIZE];		/* include file types for 1 char sfx */
static char *SPEC1[SFXTABSIZE];		/* include file specs for 1 char sfx */
static SFXBLK *SFX2[SFXTABSIZE];	/* 2+ character suffixes */

/*
 * buildsfxtable() converts a suffix list into a hash table for fast lookup.
 * Returns YES if successful, otherwise NO.
 */
buildsfxtable()
{
	extern HASH *MDEFTABLE;		/* macro definition table */
	extern SUFFIX DEFSFX[];		/* default suffix list */
	HASHBLK *htb;			/* hash table block */
	HASHBLK *htlookup();		/* find hash table entry */
	int i;				/* suffix list counter */
	int installsfx();		/* install suffix in hash table */
	int sfxbuftotable();		/* feed suffixes to installsfx() */

	/* default suffix list */
	for (i = 0; DEFSFX[i].suffix != NULL; i++)
		if (installsfx(DEFSFX[i].suffix, DEFSFX[i].sfxtyp,
		    DEFSFX[i].incspec) == NO)
			return(NO);
	
	/* supplementary suffix definitions */
	if ((htb = htlookup(MSUFFIX, MDEFTABLE)) != NULL)
		{
		if (sfxbuftotable(htb->h_def) == NO)
			return(NO);
		}
	return(YES);
}



/*
 * installsfx() installs a suffix in one of two suffix tables: SFX1 for
 * one character suffixes, and SFX2 for two or more character suffixes.
 * For a suffix that already exists, only its type and corresponding
 * included file type is updated. Returns integer YES if successful,
 * otherwise NO.
 */
installsfx(suffix, sfxtyp, incspec)
	char *suffix;			/* suffix string */
	int sfxtyp;			/* suffix type */
	char *incspec;			/* user spec for include file */
{
	char *malloc();			/* memory allocator */
	char *strsav();			/* save a string somewhere */
	int sfxindex;			/* index into suffix tables */
	int mapsuffixkey();		/* convert suffix spec to index key */ 
	SFXBLK *sfxblk;			/* suffix list block */

	if (*suffix == '.')
		suffix++;
	sfxindex = suffix[0];
	if (suffix[0] == '\0' || suffix[1] == '\0')
		{
		SFX1[sfxindex] = sfxtyp;	/* 0 or 1 character suffix */
		INC1[sfxindex] = mapsuffixtokey(incspec);
		}
	else	{				/* 2+ character suffix */
		if ((sfxblk = (SFXBLK *) malloc(sizeof(SFXBLK))) == NULL)
			return(NO);
		if ((sfxblk->sfx.suffix = strsav(suffix)) == NULL)
			return(NO);
		sfxblk->sfx.sfxtyp = sfxtyp;
		sfxblk->sfx.inctyp = mapsuffixtokey(incspec);
		sfxblk->next = SFX2[sfxindex];
		SFX2[sfxindex] = sfxblk;
		}
	return(YES);
}



/*
 * mapsuffixtokey() translates a user-specified include file type string
 * into an internal integer definition. Returns the integer definition.
 */
mapsuffixtokey(spec)
	char *spec;			/* user spec for include file */
{
	extern MAPINCLUDE INCKEY[];	/* default suffix list */

	int i;				/* INCKEY index */

	if (spec == NULL)
		return(INCLUDE_NONE);

	for (i = 0; INCKEY[i].incspec != NULL; i++)
		if (EQUAL(INCKEY[i].incspec, spec))
			break;
	return(INCKEY[i].inctyp);
}



/*
 * lookuptypeofinclude() returns the include file type for suffix, or 0 if
 * unknown suffix.
 */
lookuptypeofinclude(suffix)
	char *suffix;			/* suffix string */
{
	SFXBLK *sfxblk;			/* suffix block pointer */

	if (suffix[0] == '\0' || suffix[1] == '\0')
		return(INC1[*suffix]);		/* 0 or 1 char suffix */
						/* 2+ character suffix */
	for (sfxblk = SFX2[*suffix]; sfxblk != NULL; sfxblk = sfxblk->next)
		if (EQUAL(suffix, sfxblk->sfx.suffix))
			return(sfxblk->sfx.inctyp);
	return(0);
}



/*
 * lookupsfx() returns the suffix type, or 0 if unknown suffix.
 */
lookupsfx(suffix)
	char *suffix;			/* suffix string */
{
	SFXBLK *sfxblk;			/* suffix block pointer */

	if (suffix[0] == '\0' || suffix[1] == '\0')
		return(SFX1[*suffix]);		/* 0 or 1 char suffix */
						/* 2+ character suffix */
	for (sfxblk = SFX2[*suffix]; sfxblk != NULL; sfxblk = sfxblk->next)
		if (EQUAL(suffix, sfxblk->sfx.suffix))
			return(sfxblk->sfx.sfxtyp);
	return(0);
}



/*
 * sfxbuftotable() parses a buffer containing suffixes and presents them
 * to installsfx() for installation into the appropriate hash table.
 * The suffix type may be altered by attaching a modifier :suffixtype.
 *	:h	--> header file type
 *	:o	--> object file type
 *	:s	--> source file type (default)
 *	:x	--> executable file type
 *	:	--> unknown file type
 * The include file type may be altered by attaching an additional
 * modifier includetype.
 *	C	--> C source code
 *	C++	--> C++ source code
 *	F	--> Fortran, Ratfor, Efl source code
 *	P	--> Pascal source code
 * If the suffix is object file type, the OBJSFX default object suffix
 * is modified accordingly. Returns YES if successful, otherwise NO.
 */
sfxbuftotable(sfxbuf)
	char *sfxbuf;			/* buffer containing suffixes */
{
	extern char OBJSFX[];		/* object file name suffix */
	char *gettoken();		/* get next token */
	char *strrchr();		/* find last occurrence of character */
	char *sfxtyp;			/* suffix type */
	char *strcpy();			/* string copy */
	char suffix[SUFFIXSIZE+2];	/* suffix + modifier */
	int installsfx();		/* install suffix in hash table */

	while ((sfxbuf = gettoken(suffix, sfxbuf)) != NULL)
		if ((sfxtyp = strrchr(suffix, ':')) == NULL)
			{
			if (installsfx(suffix, SFXSRC, NULL) == NO)
				return(NO);
			}
		else	{
			*sfxtyp = '\0';
			if (sfxtyp[1] == '\0')
				{
				if (installsfx(suffix, sfxtyp[1], NULL) == NO)
					return(NO);
				}
			else
				{
				if (installsfx(suffix, sfxtyp[1], sfxtyp+2) == NO)
					return(NO);
				}
			if (sfxtyp[1] == SFXOBJ)
				strcpy(OBJSFX, suffix);
			}
	return(YES);
}
