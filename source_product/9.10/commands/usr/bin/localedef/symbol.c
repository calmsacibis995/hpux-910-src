/* @(#) $Revision: 70.6 $ */      
/* LINTLIBRARY */

#include <stdio.h>
#include "global.h"

static int symbol;
static int gotstr;
extern unsigned char buf_str[MAX_COLL_ELEMENT];
extern unsigned char buf_sym[MAX_COLL_ELEMENT];
static unsigned char buf[MAX_COLL_ELEMENT];
/*
 * Functions to read in and store collating-elements and collating-symbols, 
 * and look them up in LC_COLLATE
 */

void
elem_init()
{
extern void elem_str();
extern void elem_finish();
extern void elem_sym();

	if (collate)
		error();

	if (op) {
	  /* if we are not the first, finish up the last keyword */
	  if (finish  == NULL) error (STATE);
	  (*finish)();
	}

	string = elem_str;
	collate_sym = elem_sym; /* changes from coll_sym of design doc */
	finish = elem_finish;
	symbol = 0; /* ctr for 1st, 2nd part of clause */
	gotstr = FALSE;
}

void
elem_str()
{
extern void getstr();

	if (!symbol)
		error(SYM_DUPLICATED);
	else
	{
		(void) getstr(buf);
		strcpy(buf_str, buf);
	}
	putelem();			/* save it for later use */
	gotstr = TRUE;
}

void
elem_sym()
{

	(void) getstr(buf);
	strcpy(buf_sym, buf);
	++symbol;
}


void
elem_finish()
{

	if (!gotstr) {
	  lineno--;
	  error(STATE);
	}
}

/* collation-symbol is really not supported,
 * just read and ignore it.
 */

void
sym_init()
{
extern void sym_finish();
extern void sym_str();

	/* Correctness check */
	if (collate) /* has to come before order start */
		error();
	if (op) {
	  /* if we are not the first, finish up the last keyword */
	  if (finish  == NULL) error (STATE);
	  (*finish)();
	}

	gotstr =FALSE;
	string = error; 
	collate_sym = sym_str; /* coll_sym in design Revisit */
	finish = sym_finish;
}

void
sym_str()
{
	if (gotstr) error(STATE);
	(void) getstr(buf);
	gotstr =TRUE;
}

void
sym_finish()
{
	if (!gotstr) {
	  lineno--;
	  error(STATE);
	}

}

cmp(ptr1, ptr2)
coll_elem *ptr1, *ptr2;
{

	return(strcmp(ptr1->sym, ptr2->sym));
}

coll_elem *colle_root = NULL;
putelem()
{
coll_elem *cptr, **ret;

	cptr = (coll_elem *)malloc(sizeof(coll_elem));
	if(cptr == NULL) error(NOMEM);
	cptr->sym = (char *)malloc(strlen(buf_sym)+1);
	if(cptr->sym == NULL) error(NOMEM);
	cptr->str = (char *)malloc(strlen(buf_str)+1);
	if(cptr->str == NULL) error(NOMEM);
	strcpy(cptr->sym, buf_sym);
	strcpy(cptr->str, buf_str);
	ret = (coll_elem **)tsearch((void *)cptr, (void **)&colle_root, cmp);
}

char *
getelem(sym)
char *sym;
{
coll_elem c_temp, **element;

	c_temp.sym = sym;	/* search for this symbol */
	if((element = (coll_elem **)tfind((void *)&c_temp,
		     (void **)&colle_root, cmp)) != NULL)
		/* found match */
		return((*element)->str);
	error(SYM_NOT_FOUND);

}
