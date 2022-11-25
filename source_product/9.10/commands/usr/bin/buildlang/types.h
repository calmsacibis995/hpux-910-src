/* @(#) $Revision: 66.1 $ */    
/*
** This file contains derived types.
*/

#include	<limits.h>
#include	<stdio.h>
#include	"define.h"

typedef struct {		/* type for sequence table entry */
	unsigned char	seq_no;		/* seqence number */
	unsigned char	type_info;	/* character type */
} seq_ent;

typedef struct {		/* type for 2-1 table entry */
	unsigned char	reserved;	/* seqence number */
	unsigned char	legal;		/* character type */
	seq_ent		seq2;		/* seq table entry for 2-1 */
} two1;

typedef struct info {		/* type for temporary store of 2-1 & 1-2 info */
	int first;			/* 1st character code constant */
	int second;			/* 2nd character code constant */
	int seq_no;			/* sequence number */
	int pri_no;			/* priority number */
	struct info *next;		/* pointer to the next element */
} list;

typedef struct {		/* type for modifier table entry */
	char mod_name[_POSIX_NAME_MAX+1];	/* modifier name if specified */
	FILE *tmp;			/* fp of tmp file stores the data */
	int size;			/* size of the category/modifier */
	short pad;			/* number of bytes padded to the size
					   of category/modifier (for word
					   boundary alignment) */
} store;

typedef struct {		/* type for category/modifier table entry */
	unsigned short n_mod;		/* number of modifiers */
	store cm[MAXCMNUM];		/* info on tmp storage of cat/mod */
} cat_mod;

typedef struct {		/* type for queue item */
	int num1;			/* character code constant number */
	int num2;			/* character code constant number */
	int type;			/* constant, 2-1 or 1-2 */
} queue;
