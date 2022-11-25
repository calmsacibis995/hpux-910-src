/* $Header: rule.c,v 66.1 90/01/15 13:23:52 pfm Exp $ */

/*
 * Author: Peter J. Nicklin
 */
#include "Mkmf.h"
#include "null.h"
#include "rule.h"
#include "slist.h"
#include "suffix.h"
#include "system.h"
#include "yesno.h"

static RULEBLK *Ruletab[RULETABSIZE];	/* rules table */
static SLIST *Rulelist = NULL;		/* transformation rule list */

/*
 * applyrule() applies successive transformation rules to filename, and
 * checks to see if the file exists. Returns YES if filename exists,
 * otherwise NO.
 */
applyrule(target, source)
	char *target;			/* name of (transformed) file */
	char *source;			/* name of source file */
{
	register char *r;		/* rule pointer */
	register char *s;		/* source buffer pointer */
	char *sourcesuffix;		/* source file suffix */
	char *strcpy();			/* string copy */
	char *strrchr();		/* find last occurrence of character */
	char *rulesuffix;		/* target suffix in each rule */
	char *targetsuffix;		/* transformed file suffix string */
	int ruleindex;			/* index into rule table */
	int strcmp();			/* string comparison */
	RULEBLK *rblk;			/* rule list block */

	if ((targetsuffix = strrchr(target, '.')) == NULL)
		return(NO);
	ruleindex = targetsuffix[1];
	if (Ruletab[ruleindex] != NULL)
		{
		strcpy(source, target);
		sourcesuffix = strrchr(source, '.');
		for (rblk=Ruletab[ruleindex]; rblk != NULL; rblk=rblk->r_next)
			{
			rulesuffix = strrchr(rblk->r_rule, '.');
			if (strcmp(rulesuffix, targetsuffix) == 0)
				{
				r = rblk->r_rule;
				s = sourcesuffix;
				while (*++s = *++r)
					if (*s == '.')
						{
						*s = '\0';
						break;
						}
				if (FILEXIST(source))
					return(YES);
				}
			}
		}
	return(NO);
}



/*
 * buildruletable() converts a list of transformation rules into a hash table
 * for fast lookup. Returns YES if successful, otherwise NO.
 */
buildruletable()
{
	extern char *DEFRULE[];		/* default preprocessor rules */
	int i;				/* default rule list counter */
	int instalrule();		/* install rule in hash table */
	SLBLK *rblk;			/* singly-linked rulename block */

	/* process default rules */
	for (i = 0; DEFRULE[i] != NULL; i++)
		{
		if (instalrule(DEFRULE[i]) == NO)
			{
			nocore();
			return(NO);
			}
		}

	/* process rules found in makefile */
	if (Rulelist != NULL)
		{
		for (rblk = Rulelist->head; rblk != NULL; rblk = rblk->next)
			{
			if (instalrule(rblk->key) == NO)
				{
				nocore();
				return(NO);
				}
			}
		}
	return(YES);
}



/*
 * findrule() searchs a line for a transformation rule. Returns the
 * name of the transformation rule, or NULL if not found.
 */
char *
findrule(rulename, bp)
	char *rulename;			/* transformation rule buffer */
	register char *bp;		/* I/O buffer pointer */
{ 
	register char *rp;		/* rule name pointer */
	int dotcount = 0;		/* number of '.'s in rule */

	for (rp = rulename; *bp != ':' && *bp != ' ' && *bp != '\t'; rp++, bp++)
		{
		if ((*rp = *bp) == '.')
			dotcount++;
		}
	*rp = '\0';

	/* eat up white space between rule and ':' */
	if (*bp != ':')
		{
		while (*bp == ' ' || *bp == '\t')
			bp++;
		if (*bp != ':')
			return(NULL);
		}

	return((dotcount == 2) ? rulename : NULL);
}



/*
 * instalrule() installs a source transformation rule in the rule lookup
 * table. The rule table consists of a set of singly-linked lists, indexed
 * by the first character of the suffix of the target file. The index of
 * the target file is used by applyrule() to find out the name of the file
 * from which it was derived. Returns YES if successful, otherwise NO.
 */
instalrule(rule)
	char *rule;			/* rule to be installed in Rule table */
{
	char *malloc();			/* memory allocator */
	char *strrchr();		/* find last occurrence of character */
	char *strsav();			/* save a string somewhere */
	char *target;			/* target suffix */
	int lookupsfx();		/* get suffix type */
	int ruleindex;			/* index into rule table */
	RULEBLK *rblk;			/* rule list block */

	target = strrchr(rule, '.') + 1;
	if (lookupsfx(target) == SFXSRC)
		{
		ruleindex = target[0];
		if ((rblk = (RULEBLK *) malloc(sizeof(RULEBLK))) == NULL)
			return(NO);
		if ((rblk->r_rule = strsav(rule)) == NULL)
			return(NO);
		rblk->r_next = Ruletab[ruleindex];
		Ruletab[ruleindex] = rblk;
		}
	return(YES);
}



/*
 * lookuprule() returns YES if rule exists, otherwise NO.
 */
lookuprule(rule)
	char *rule;			/* .x.y rule to find */
{
	char *strrchr();		/* find last occurrence of character */
	char *targetsuffix;		/* transformed file suffix string */
	int ruleindex;			/* index into rule table */
	int strcmp();			/* string comparison */
	RULEBLK *rblk;			/* rule list block */

	if ((targetsuffix = strrchr(rule, '.')) == NULL)
		return(NO);
	ruleindex = targetsuffix[1];
	if (Ruletab[ruleindex] != NULL)
		{
		for (rblk=Ruletab[ruleindex]; rblk != NULL; rblk=rblk->r_next)
			{
			if (strcmp(rule, rblk->r_rule) == 0)
				return(YES);
			}
		}
	return(NO);
}



/*
 * makerule() creates a rule from the suffixes of two file names.
 */
void
makerule(rule, source, target)
	char *rule;			/* buffer to hold rule */
	char *source;			/* source file name */
	char *target;			/* target file name */
{
	char *strcpy();			/* string copy */
	char *strrchr();		/* find last occurrence of character */

	strcpy(rule, strrchr(source, '.'));
	strcat(rule, strrchr(target, '.'));
}



/*
 * storerule() appends a transformation rule to the end of a singly-linked
 * list. Returns integer NO if out of memory, otherwise YES.
 */
storerule(rulename)
	char *rulename;			/* transformation rule name */
{
	char *slappend();		/* append rule to list */
	SLIST *slinit();		/* initialize transformation list */

	if (Rulelist == NULL)
		Rulelist = slinit();
	if (slappend(rulename, Rulelist) == NULL)
		return(NO);
	return(YES);
}
