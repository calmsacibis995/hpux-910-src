/* @(#) $Revision: 70.1 $ */

/***************************************************************************
****************************************************************************

	inex.c
    
    This file contains routines to handle the include/exclude options (-i and
    -e).  This is accomplished by first gatherering all the -i and -e paths
    specified and puting them on a linked list.  The number of exlude (-e)
    paths are further split into directory and non-directory (called 'files'
    for simplicity) groups.  The number of each of these 3 types is counted,
    and tables are allocated to hold them.  The include (-i) table is then
    sorted into "alphabetical" order, and is used as the means for specifying
    the graph to be backed up.  The exlude tables are used to exlude portions
    of this graph so that they don't get included in the final flist.

****************************************************************************
***************************************************************************/

#include "head.h"

#ifdef NLS
#include <nl_ctype.h>
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

void qsort(), free();
FLISTNODE *add_flistnode();

extern dtabentry *add_dentry();

typedef struct inexnode {	/*  include/exclude path data structure */
    char *path;			/* so we can copy pointers later */
    ino_t inode;
    dev_t device;
    struct inexnode *next;
    char ie;			/* moved down here to eliminate "holes" in */
    unsigned char dirflag;	/*  the structure */
} INEXNODE;

typedef struct {		/* internal checkpoint data structure */
    char *path;
} ITAB;

static INEXNODE *node, *tail, *head = (INEXNODE *) NULL;

ETAB	*dir_excldtab,			/* exclude table for directories */
	*file_excldtab;			/* exclude table for non-directories */
int	dir_excld = 0,
	file_excld = 0;
static int incld = 0;
static ITAB	*incldtab;		/* include table */






/***************************************************************************
    This function adds either -i or -e paths to the list of include/excluded
    files.  It returns TRUE if the pathname was added to the table, otherwise
    FALSE.
***************************************************************************/
int
add_inex(path, ie)
char *path, ie;
{
    struct stat statbuf;

    if (statcall(path, &statbuf) == -1) {
	msg((catgets(nlmsg_fd,NL_SETN,601, "fbackup(1601): could not access -i/-e file %s\n")), path);
	return(FALSE); 
    }

    node = (INEXNODE *) mymalloc(sizeof(INEXNODE));
    if (node == (INEXNODE*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,602, "fbackup(1602): out of virtual memory\n"));

    if (head == (INEXNODE *) NULL)
	head = node;
    else
	tail->next = node;
    if (ie == 'i') {
	node->path = (char *)mymalloc(strlen(path) + 1);
        (void)strcpy(node->path, path);
    } else {
	node->path = NULL;		/* path is not needed for exclusion */
    }
    node->ie = ie;
    node->inode = statbuf.st_ino;
    node->device = statbuf.st_dev;
    node->next = (INEXNODE *) NULL;
    tail = node;
    switch (ie) {
	case 'i':
	    incld++;
	break;
	case 'e':
	    if ((statbuf.st_mode&S_IFMT) == S_IFDIR) {
		node->dirflag = TRUE;
		dir_excld++;
	    } else {
		node->dirflag = FALSE;
		file_excld++;
	    }
	break;
    }
    return(TRUE); 
}  /* end add_inex */






/***************************************************************************
    This function first allocates the 3 tables (include, exlude directories,
    and exclude non-directories [AKA files]), and then fills each with
    the data from the linked list.  The include table is then sorted, so
    that the files will appear in the flist in "alphabetical" order
***************************************************************************/
void
make_tables()
{
    int sortcmp();
    ITAB *incldptr;
    ETAB *dir_excldptr, *file_excldptr;

						    /* allocate the tables */
    incldtab = (ITAB *) mymalloc(incld*sizeof(ITAB));
    if (incldtab == (ITAB*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,603, "fbackup(1603): out of virtual memory\n"));

    incldptr = incldtab;
    dir_excldtab =  (ETAB *) mymalloc(dir_excld*sizeof(ETAB));
    if (dir_excldtab == (ETAB*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,604, "fbackup(1604): out of virtual memory\n"));

    dir_excldptr = dir_excldtab;
    file_excldtab = (ETAB *) mymalloc(file_excld*sizeof(ETAB));
    if (file_excldtab == (ETAB*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,605, "fbackup(1605): out of virtual memory\n"));
    file_excldptr = file_excldtab;

    node = head;				/* put the data in the tables */
    while (node != (INEXNODE *) NULL) {
	switch(node->ie) {
	case 'i':
	    incldptr->path = node->path;
	    incldptr++;
	    break;
	case 'e':
	    if (node->dirflag) {
		dir_excldptr->inode = node->inode;
		dir_excldptr->device = node->device;
		dir_excldptr++;
	    } else {
		file_excldptr->inode = node->inode;
		file_excldptr->device = node->device;
		file_excldptr++;
	    }
	    break;
	}
	node = node->next;
    }
						/* sort the include table */
    qsort ((char *)incldtab, (unsigned)incld, sizeof(ITAB), sortcmp);
}






/***************************************************************************
    This function is used to compare the strings when sorting the include file.
***************************************************************************/
int
sortcmp(e1, e2)
ITAB *e1, *e2;
{
    return(mystrcmp(e1->path, e2->path));
}






/***************************************************************************
    This function reads through the (sorted) include table and recursively
    expands these pathnames and then places the appropriate files on the
    flist (subject to the entries in the 2 exclude tables).  Before search()
    is called to expand the subtree, any leading directories are added to
    the flist.  Eg, if the pathname is /mnt/topaz/crook, the three
    directories, /, /mnt, and /mnt/topaz will be automatically added
    the the flist in the correct (sorted) order.  A liked list of these
    added leading directories is kept, so that none of them are added more
    than once.  Consider if fbackup had as part of its argument string:
    "-i /mnt/topaz/crook -i /mnt/topaz/lkc/foo".  The three leading
    directories mentioned before would be added because of the former, and
    the leading directory /mnt/topaz/lkc would be added because of the
    latter.
    After all the 'normal' cases are done, the 'weird' cases are handled.
***************************************************************************/
void
make_flist()
{
    ITAB *incldptr = incldtab;
    char *path, *pp, *lastpath = '\0', *lp;
    struct stat statbuf;
    int i;

    for (i=0; i<incld; i++, incldptr++) {

	path = incldptr->path;			/* add any leading */
	lp = lastpath;				/* directories to the flist */
	pp = path;
	while ((CHARAT(lp) == CHARAT(pp)) && CHARAT(lp)) {
	    ADVANCE(lp);
	    ADVANCE(pp);
	}
	while (CHARAT(pp) != (NLSCHAR)'\0') {
	    if ((CHARAT(pp) == (NLSCHAR)'/')&&(CHARAT(pp+1) != (NLSCHAR)'\0')) {
		PCHAR((NLSCHAR)'\0', pp);
		if (CHARAT(path)) {
		    (void) statcall(path, &statbuf);
		    (void)add_pdentry(path);
		    (void) add_flistnode(path, &statbuf);
		} else {
		    (void) statcall("/", &statbuf);
		    (void)add_dentry("/");
		    (void) add_flistnode("/", &statbuf);
		}
		PCHAR((NLSCHAR)'/', pp);
	    }
	    ADVANCE(pp);
	}

	search(path, 0);
	lastpath = path;
    }

    free(incldtab);			/* free include table space */
    node = head;			/* and in/ex list space */
    while ((head=node) != (INEXNODE*)NULL) {
	node = node->next;
	if (head->path) free(head->path);
	free(head);
    }

    doweirds();
}
