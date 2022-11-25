/* @(#) $Revision: 70.1 $ */

/***************************************************************************
****************************************************************************

	pwgr.c

    This file contains the routines which build the hash lookup tables for
    user names and group names (one table each).  

****************************************************************************
***************************************************************************/

char *strcpy();
long *mymalloc();

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include "head.h"

#define PWTABSIZ 67
#define GRTABSIZ 23
#define PWGRNAMLEN 32		/* maximum len of login or group name */

#ifdef NLS
#define NL_SETN 1
extern nl_catd nlmsg_fd;
#endif NLS

typedef struct pwgrnode {
    int	 number;
    char name[PWGRNAMLEN];
    struct pwgrnode *next;
} PWGRNODE;

static PWGRNODE *pwtable[PWTABSIZ];
static PWGRNODE *grtable[GRTABSIZ];
static PWGRNODE **table;
static int modval;






/***************************************************************************
    This function sets up the empty hash lookup tables according to the
    parameters defined above.  I then searches through all the password
    (group) file and extracts all the login (group) names and hashes
    them into the username (groupname) hash table.  It does this by calling
    the function 'hash'.
	The tables consist of an array (of size modval) of linked lists
    of structures containing the number, username pairs.
	Note that for the case where there is more than one login (group)
    name associated with a particular user-id (group-id), only the first
    name in the password (group) file for that particular user-id (group-id)
    will appear in the hash table.
***************************************************************************/
void
initpwgr()
{
    struct passwd *pwbuf, *getpwent();
    struct group  *grbuf, *getgrent();
    int i;

    for (i=0; i< PWTABSIZ; i++)			/* do passwd stuff */
	pwtable[i] = (PWGRNODE *) NULL;
    modval = PWTABSIZ;
    table = pwtable;
    while ((pwbuf=getpwent()) != (struct passwd *)NULL)
	hash(pwbuf->pw_uid, pwbuf->pw_name);

    for (i=0; i< GRTABSIZ; i++)			/* do group stuff */
	grtable[i] = (PWGRNODE *) NULL;
    modval = GRTABSIZ;
    table = grtable;

    while ((grbuf=getgrent()) != (struct group *)NULL)
	hash(grbuf->gr_gid, grbuf->gr_name);
}






/***************************************************************************
    This function is called to place the [number, character string] pairs
    into the appropriate hash table.  This funtion is used to build both
    the username and groupname tables.
***************************************************************************/
hash(number, name)
int number;
char *name;
{
    int index;
    PWGRNODE *prev, *node, *new;

    number = number & 0xffff;
    index = number%modval;
    node = table[index];
    prev = (PWGRNODE *) NULL;
    while (node != (PWGRNODE *) NULL) {
	if (number == node->number)
	    return;
	if (number < node->number) {
	    new = (PWGRNODE *) mymalloc(sizeof(PWGRNODE));
    	    if (new == (PWGRNODE*)NULL)
		errorcleanx(catgets(nlmsg_fd,NL_SETN,801, "fbackup(1801): out of virtual memory\n"));
	    new->number = number;
	    (void) strcpy(new->name, name);
	    new->next = node;
	    if (prev == (PWGRNODE *) NULL) {	/* add to the beginning */
		table[index] = new;
	    } else {				/* add to the middle */
		prev->next = new;
	    }
	    return;
	}
	prev = node;
	node = node->next;
    }
		    				/* add to the end */
    new = (PWGRNODE *) mymalloc(sizeof(PWGRNODE));
    if (new == (PWGRNODE*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,802, "fbackup(1802): out of virtual memory\n"));
    new->number = number;
    (void) strcpy(new->name, name);
    new->next = (PWGRNODE *) NULL;
    if (prev == (PWGRNODE *) NULL) {		/* of a null list */
	table[index] = new;
    } else {					/* of a non-null list */
	prev->next = new;
    }
    return;
}






/***************************************************************************
    This function is called to lookup the login name for a given user-id.
    It does a hash and then a linear search through the entries in that list.
***************************************************************************/
char *
getapw(number)
int number;
{
    PWGRNODE *node;

    node = pwtable[number%PWTABSIZ];
    while (node != (PWGRNODE *) NULL) {
	if (number == node->number)
	    return(node->name);
	node = node->next;
    }
    return((char *) NULL);
}






/***************************************************************************
    This function is called to lookup the group name for a given group-id.
    It does a hash and then a linear search through the entries in that list.
***************************************************************************/
char *
getagr(number)
int number;
{
    PWGRNODE *node;

    node = grtable[number%GRTABSIZ];
    while (node != (PWGRNODE *) NULL) {
	if (number == node->number)
	    return(node->name);
	node = node->next;
    }
    return((char *) NULL);
}
