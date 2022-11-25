/* $Revision: 64.1 $ */
/* routines to manipulate priority structure */

#include	"lp.h"
#include	"lpsched.h"

/* newpri() -- returns pointer to empty priority structure */

struct prilist *
newpri()
{
	struct prilist *pri;

	if((pri = (struct prilist *) malloc(sizeof(struct prilist))) == NULL){
		fatal(CORMSG, 1);
	}

	pri->priority = -1;
	pri->pl_output = NULL;
	pri->pl_next = pri->pl_prev = pri;

	return(pri);
}
	
/* newpl() -- returns pointer to empty priority list */

struct prilist *
newpl()
{
	struct	prilist	*plhead, *old, *new;
	extern	struct outlist *newol();
	short	priority;

	plhead = newpri();
	old = plhead;

	for(priority = MAXPRI ; priority >= MINPRI ; priority--){
		new = newpri();
		new->priority = priority;
		new->pl_output = newol();

		new->pl_next = old->pl_next;
		new->pl_prev = old;
		old->pl_next = new;
		(new->pl_next)->pl_prev = new;
		old = new;
	}
	return(plhead);
}

/* getpri() -- returns pointer to the priority list structure for
		destination d with priority priority.
		If this doesn't exist, then goto returns NULL.
		(fatal error)
*/

struct prilist *
getpri(d, priority)
struct dest *d;
short priority;
{
	struct prilist *headpl, *pl;

	for(headpl = d->d_priority, pl = headpl->pl_next ;
		pl != headpl ; pl = pl->pl_next){
			if(pl->priority == priority)
				return(pl);
	}

	return(NULL);
}
