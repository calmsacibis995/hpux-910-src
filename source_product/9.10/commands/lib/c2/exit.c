/* @(#) $Revision: 70.1 $ */    
#include "o68.h"

node *insertl();

/*
 * Combine calls to _exit & __exit.
 *
 * If we see a call to _exit or __exit,
 * try to find a later call and branch to that instead.
 */
exits()
{
	node *find_last();
	register node *p, *p_exit;

	p_exit = find_last("_exit");

	/*
	 * Did we find it?
	 * If not give up right now.
	 */
	if (p_exit==NULL)
		return;

	for (p=first.forw; p!=NULL; p = p->forw) {
		if (p->op!=JSR || p->mode1!=ABS_L || p->type1!=STRING)
			continue;

		/*
		 * Is this a call to _exit ?
		 *
		 * Don't make us jump to ourselves!
		 */
		if (p_exit!=p && equstr(p->string1,"_exit")) {
			cleanse(&p->op1);
			p->op=JMP;
			p->subop=UNSIZED;
			p->mode1=ABS_L;
			p->type1=INTLAB;
			p->ref=insertl(p_exit);
			p->labno1=p->ref->labno1;
			change_occured=true;
		}
	}
}


/*
 * Find the last occurance of JSR target
 * and return a pointer to it.
 *
 * If no JSR target exists, return NULL.
 */
node *
find_last(target)
register char *target;
{
	register node *p, *hit;

	hit=NULL;

	for (p=first.forw; p!=NULL; p = p->forw) {
		if (p->op==JSR && p->mode1==ABS_L && p->type1==STRING
		&& equstr(target, p->string1))
			hit=p;
	}

	return (hit);
}
