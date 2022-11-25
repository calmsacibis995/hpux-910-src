/* @(#) $Revision: 70.3 $ */

/* 
 * _winlist.c:	routines to insert and delete entries from a link
 *		list that keep track of all windows in a screen.
 *
 *   Note: _fixScreen.c, _fixWindow.c, _winchTrap.c and _winlist.c
 *         are used only when SIGWINCH is active.
 */

#include "curses.ext"

/*
 *  create and add a new node to the beginning of the list
 *  _winlist, and initialized it's values.  		  
 */
_ins_winlist(winptr)
WINDOW *winptr;
{
	register struct _winlink *tmp;

	if (winptr == NULL) return;
	
	if ((tmp = (struct _winlink *) calloc(1, sizeof(struct _winlink))) == NULL)
		return ERR;

	tmp->win  = winptr;
	tmp->next = _winlist;
	_winlist  = tmp;
}


/*
 *  delete a node from winlist that has win valued = winptr 
 */
_del_winlist(winptr)
WINDOW *winptr;
{
	register struct _winlink *i, *j;

	i = _winlist;
	if (i->win == winptr) {
		_winlist = i->next;
		cfree((char *)i);
		return;
	}

	while ((j = i->next) != NULL) {
		if (j->win == winptr) {
			i->next = j->next;
			cfree((char *)j);
			return;
		} else 
			i = j;
	}
}


/* 
 *  free all memory allocated for the list 
 */
_free_wlist(list)
struct _winlink *list;
{
	register struct _winlink *j, *i = list;

	while ((j=i) != NULL) {
		cfree((char *)j);
		i = i->next;
	}
}


/*
 *  dump the contents of the list 
 */
_dsp_wlist(list)
struct _winlink *list;
{
	register struct _winlink *i;

	if (list == NULL) {
		printf("empty list\n");
		return;
	}

	i = list;
	while (i != NULL) {
		printf("%x\n", i->win);
		i = i->next;
	}
}

