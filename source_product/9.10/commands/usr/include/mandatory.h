/* @(#) $Revision: 66.8 $ */
#ifndef _MANDATORY_INCLUDED /* allows multiple inclusion */
#define _MANDATORY_INCLUDED

/* Copyright (c) 1988 SecureWare, Inc.
 *  All rights reserved
 *
 * This Module contains Proprietary Information of SecureWare, Inc.
 * and should be treated as Confidential.
 *
 * mandatory.h	2.3 10:45:35 9/22/89 SecureWare
 */

/*
 * Include file for mandatory access control
 *
 * This file contains definitions for all major mandatory access control
 * data structures and routine declarations. 
 * The data structures are for classifications, categories, synonyms,
 * and mandatory policy parameters.
 */

#include <sys/security.h>
#include <dirent.h>

#ifdef SEC_ILB
#include <std_labels.h>
#endif

/* Types for classifications and categories and sensitivity labels */

typedef unsigned long	class_ir_t;	/* 32 bit classification */

typedef struct mand_ir {
	class_ir_t	class;
	mask_t		cat[1];
}  mand_ir_t;

#ifdef SEC_ILB
typedef mand_ir_t       ilb_ir_t;
#endif

/* structure of the synonym database file:
 *    putw (number of synonyms)
 *    putw (number of category masks)	each of these is CATWORDS words
 *    putw (number of string characters)
 *    syn_file structures
 *    category masks
 *    string table
 *
 * The index of category mask (starting at zero) is stored in syn.cat_set.
 * The index of the synonym name in the string table (starting at zero) is
 * stored in syn.name.
 * syn.type is CLASS_SYN, CATEGORY_SYN, or SENS_LABEL_SYN.
 * syn.class is an integer between 1 and mand_max_class.
 */

struct syn_file {
	int	name;
	int	type;
	int	cat_set;
	int	class;
};

#define	CLASS_SYN	1	/* synonym for a classification */
#define CATEGORY_SYN	2	/* synonym for a category set */
#define SENS_LABEL_SYN	3	/* synonym for a sensitivity label */

/* following definition tells how many words required to represent the
 * categories on this system.
 */

#define CATWORDS	(WORD_OF_BIT(mand_max_cat) + 1)
#ifdef SEC_ILB
#define MARKWORDS       (WORD_OF_BIT(mand_max_mark) + 1)
#endif

#define MAND_INVALID_CLASS	(-1)
#define MAND_INVALID_CAT	(-1)

/* definition of parameters from the mandatory policy file */

struct mand_config {
	char	dbase[50];
	u_long	cache_size;
	u_long	buffers;
	u_short	subj_tags;
	u_short	obj_tags;
	u_short	first_subj_tag;
	u_short	first_obj_tag;
	dev_t	minor_device;
	u_short	policy;
};

extern 	struct mand_config mand_config;	/* configuration parms */

/* Parameters of classifications and categories: */
extern	unsigned	mand_max_class;	/* maximum numerical class */
extern	unsigned	mand_max_cat;	/* maximum numerical category */
#ifdef SEC_ILB
extern  unsigned        mand_max_mark;  /* maximum numerical marking */
extern  ilb_ir_t        *mand_syshi;    /* system high info label */
extern  ilb_ir_t        *mand_syslo;    /* system low  info label */
#else
extern	mand_ir_t	*mand_syshi;	/* system high sens. label */
extern	mand_ir_t	*mand_syslo;	/* system low  sens. label */
#endif

#ifdef SEC_ENCODINGS

extern  mand_ir_t       *mand_clrnce;           /* user's clearance */
extern  mand_ir_t       *mand_minclrnce;        /* system minimum clearance */
extern  mand_ir_t       *mand_minsl;            /* minimum useful SL */

#define ILB_PARAM_FILE	"/etc/policy/macilb/config"
#define ASCII_ENCODINGS_FILE	"/etc/policy/macilb/Encodings"
#define BINARY_ENCODINGS_FILE	"/etc/policy/macilb/Encodings.db"


#else

#define	MAND_CLASS_FILE	"/etc/policy/mand/b1/classes"
#define MAND_CAT_FILE	"/etc/policy/mand/b1/categories"
#define MAND_SYN_FILE	"/etc/policy/mand/b1/synonyms"
#define MAND_SYN_DB	"/etc/policy/mand/b1/synonyms.db"
#define MAND_PARAM_FILE	"/etc/policy/mand/b1/mand_policy"

#endif

#define MAND_EXTENSION	":t"		/* when re-writing new files */

/* decision values upon comparing the relationship of two labels */

#define	MAND_SDOM	1
#define	MAND_ODOM	2
#define	MAND_EQUAL	4
#define	MAND_INCOMP	8
#if defined(SEC_ILB) && !defined(ILB_SDOM)
#define ILB_SDOM        0x10
#define ILB_ODOM        0x20
#define ILB_SAME        0x40
#endif

/* different ways to traverse a multilevel directory */

#define	MAND_MLD_ALLDIRS	0
#define	MAND_MLD_MANDLEVEL	1

/* definition to use for traversal of multilevel directories (see mld(3)) */

struct multdir  {
	char *name;
	char sdirname[MAXNAMLEN+1];
	DIR *mdir;
	DIR *sdir;
	int technique;
	mand_ir_t *ir;
};
typedef struct multdir MDIR;


/* functions for mandatory access control representations */

char	  *mand_cltoname();		/* return a name given a number */
int	  mand_nametocl();		/* return a number given a name */
char	  *mand_cattoname();		/* return a name given a number */
int	  mand_nametocat();		/* return a number given a name */
mand_ir_t *mand_er_to_ir();		/* map external to internal */
char	  *mand_ir_to_er();		/* map internal to external */
void	  mand_lookup_and_print_syn();	/* lookup and print a synonym */
mand_ir_t *mand_alloc_ir();		/* allocate a mandatory structure */
void	  mand_free_ir();		/* free memory for structure */
void	  mand_end();			/* free storage used by databases */
int	  mand_init();			/* init parms but not databases */
int	  mand_ir_to_tag();		/* convert IR to tag */
int	  mand_tag_to_ir();		/* convert tag to IR */
int	  mand_ir_relationship();	/* compare two IRs */
int	  mand_tag_relationship();	/* compare two tags */
MDIR	  *openmultdir();		/* open a MLD for traversal */
void	  closemultdir();		/* close a MLD */
void	  rewindmultdir();		/* reset MLD to beginning */
void	  readmultdir();		/* read a name from a MLD */
char	  *ir_to_subdir();		/* find subdir in a MLD with given IR */
char      *mand_convert();		/* generic ir to er conversion */
#ifdef SEC_ILB
ilb_ir_t   *mand_parse();		/* generic er to ir conversion */
#else
mand_ir_t  *mand_parse();		/* generic er to ir conversion */
#endif

#ifdef SEC_ILB
ilb_ir_t        *ilb_er_to_ir();        /* map external to internal */
char            *ilb_ir_to_er();        /* map internal to external */
ilb_ir_t        *ilb_alloc_ir();        /* allocate ILB structure */
void            ilb_free_ir();          /* deallocate ILB structure */

/* definitions for the irtype argument to mand_convert() */

#define MAND_IL 0
#endif

/* the number of bytes in a sensitivity label */

#define mand_bytes() (sizeof(class_ir_t) + (CATWORDS * sizeof(mask_t)))
#ifdef SEC_ILB
#define ilb_bytes() (mand_bytes() + (MARKWORDS * sizeof(mask_t)))
#endif

/* copy a sensitivity label given a source and destination pointer */

#define mand_copy_ir(from,to)	memcpy(to,from,mand_bytes())
#ifdef SEC_ILB
#define ilb_copy_ir(from,to)    memcpy(to,from,ilb_bytes())
#endif

#endif  /* _MANDATORY_INCLUDED */
