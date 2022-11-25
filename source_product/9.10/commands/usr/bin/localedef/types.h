/* @(#) $Revision: 70.2 $ */    
/*
** This file contains derived types.
*/

#include	<limits.h>
#include	<stdio.h>
#include	"define.h"



/*
 * The following is added for NLS/localedef code
 */
typedef struct {
  unsigned char *sym;
  unsigned char *str;
} coll_elem;

struct coll_sym{
  unsigned char *sym;
  int seq_pos;
  int *backfill;
};

typedef struct {
  int token;
  union {
    unsigned int number;
    unsigned char string[MAX_COLL_ELEMENT+1];
	} value;
} col_ent;


typedef union {
	unsigned int number;
	unsigned char string[_POSIX_NAME_MAX+1];
} col_part;

/*
typedef struct{		*/	/* type for collating entry raw form */
/*	int part_type[COLL_WEIGHTS_MAX+1];*/     /* token number of entry */
/*	col_part part_val[COLL_WEIGHTS_MAX+1];*/  /* value of token */
/*
} col_ent;*/

typedef struct {		/* type for sequence table entry */
	short	seq_no;		/* seqence number: changed from
				   char so that it can have value -1 */
	unsigned char	type_info;	/* character type */
} seq_ent;

typedef struct {		/* type for 2-1 table entry */
	unsigned char	reserved;	/* seqence number */
	unsigned char	legal;		/* character type */
	seq_ent		seq2;		/* seq table entry for 2-1 */
} two1;

typedef struct info {	/* type for temporary store of 2-1 & 1-2 info */
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


