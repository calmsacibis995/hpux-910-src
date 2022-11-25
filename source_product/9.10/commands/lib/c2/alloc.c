/* @(#) $Revision: 70.1 $ */   
#include "o68.h"

node *getnode();
char *strcpy();
char *malloc();

static node *freenodes=NULL;

/*
 * Insert a (defaultized) node before the given node.
 * The node will be given 'opcode' as its opcode.
 * Return the address of the new node.
 */
node *
insert(where, opcode)
register node *where;
int opcode;
{
	register node *new;

	new = getnode(opcode);		/* get a fresh node */

	/* set up forward links */
	new->forw = where;		/* after us is where */
	where->back->forw = new;	/* we are after who was before where */

	/* set up backward links */
	new->back = where->back;	/* before us is who was before where */
	where->back = new;		/* we are before where */

	return (new);
}



/*
 * If the operator 'where' is a label or follows a label, this function
 * will increment the reference count of the label and return a
 * pointer to it; if neither of the above is true, the function will
 * create a label node with reference count of 1, hook it in, and
 * return a pointer to it.
 */
node *
insertl(where)
register node *where;
{
	static long isn = first_c2_label;/* where new label numbers come from */
	register node *new;

	/* Are we pointing to a label? */
	if (where->op == LABEL) {
		where->refc++;
		return(where);
	}

	/* Is perchance the previous line a label? */
	if (where->back->op == LABEL) {
		where = where->back;
		where->refc++;
		return(where);
	}

	/* Heck with it, create a label for ourselves. */
	new = insert(where, LABEL);
	new->mode1=ABS_L;
	new->type1=INTLAB;
	new->labno1 = isn++;
	new->refc = 1;
	return (new);
}

#ifdef DEBUG
int nodes_allocated = 0;
int nodes_freed = 0;
int strings_allocated = 0;
int strings_freed = 0;
#endif DEBUG

/*
 * Get a node from the free list; if the list is empty, allocate one
 * node.  Set the "op" field of the new node to the input parameter
 * "newop"; set all other fields to zero. We don't have to defaultize 
 * the node if it comes off the free list, since it was defaultized 
 * when it got put on.
 */
node *
getnode(newop)
register int newop;
{
	register node *p;

	if (freenodes==NULL) {
		p = (node *) malloc((unsigned) sizeof(struct node));
		if (p==NULL)
			internal_error("getnode: no more space");
		defaultize(&p->op1);
		defaultize(&p->op2);
	} else {
		p = freenodes;
		freenodes = p->ref;
	}

	p->op = newop;
	p->subop = UNSIZED;
	p->info = 0;
	p->forw = p->back = p->ref = NULL;
	p->refc = 0;
#ifdef M68020
	p->offset = 0;
	p->width = 0;
#endif

#ifdef DEBUG
	nodes_allocated++;
	bugout("6: getnode: node #%d with op %d allocated to %x",
		nodes_allocated, newop, p);
#endif DEBUG

	return(p);
}


/* Place a node on the free list; return pointer to previous node */
node *
release(p)
register node *p;
{
	register node *before, *after;


#ifdef DEBUG
	nodes_freed++;
	bugout("6: release(%x) node #%d with op %d", p, nodes_freed, p->op);
#endif DEBUG

	p->op = 0;
	p->subop = UNSIZED;
	p->info = 0;
	p->refc = 0;
#ifdef M68020
	p->offset = 0;
	p->width = 0;
#endif
	cleanse(&p->op1);
	cleanse(&p->op2);

	/* link the previous node to the next node */
	before = p->back;			/* pointer to previous node */
	after = p->forw;			/* pointer to next node */

	if (before!=NULL)			/* if someone is before us */
		before->forw = after;		/* he points to after us */
	if (after!=NULL)			/* if someone is after us */
		after->back = before;		/* he points to before us */

	p->back = p->forw = NULL;		/* we don't point any more */

	/* link this node into the free list */
	p->ref = freenodes;
	freenodes = p;

	/* return the previous node */
	return(before);
}


/*
 * Release a whole list of nodes
 */
release_list(p)
register node *p;
{
	register node *next;

	while (p!=NULL) {
		next = p->forw;		/* save it now, release will kill it */
		release(p);
		p=next;
	}
}


/*
 * make an existing argument clean as the day it was allocated.
 */
cleanse(arg)
register argument *arg;
{
	/* tuning: check for type == STRING regardless of mode */

#ifdef PREPOST
		if (arg->od_type==STRING)
			free_string(arg->odstring);
#endif PREPOST

		if (arg->type==STRING)
			free_string(arg->string);
#ifdef DEBUG
	defaultize(arg);		/* set default values */
#else DEBUG
	/* in-line defaultize */
	arg->mode = UNKNOWN;
	arg->type = UNKNOWN;
	arg->reg = -1;
	arg->index = -1;
	arg->labno = 0;
	arg->string = NULL;
#ifdef PREPOST
	arg->od_type = UNKNOWN;
	arg->odlabno = 0;
	arg->odstring = NULL;
#endif PREPOST
#ifdef VOLATILE
	arg->attributes = 0;
#endif
#endif DEBUG
}


/*
 * Copy away the string pointed to by p to a safe place.
 */
char *copy(p)
register char *p;
{
	register char *new;
	register unsigned int len;	/* malloc takes unsigned size */

	if (p==NULL)
		internal_error("copy: can't handle null pointer");
	len = strlen(p);			/* get length of string */

	new = malloc(len+1);		/* extra spot for the terminator */
	if (new==NULL)
		internal_error("copy: ran out of memory");
	strcpy(new,p);			/* copy stuff to new safe place */

#ifdef DEBUG
	strings_allocated++;
	bugout("6: copy(\"%s\"): allocated string #%d at %x", p, strings_allocated, new);
#endif DEBUG

	return(new);			/* return pointer to copied data */
}

free_string(s)
register char *s;
{
	if (s==NULL)
		internal_error("free_string: attempt to free null string");

#ifdef DEBUG
	strings_freed++;
	bugout("6: free_string(\"%s\"[%x]) #%d", s, s, strings_freed);
#endif DEBUG

	free(s);
}


/*
 * set up 'arg' so it has default values
 */
defaultize(arg)
register argument *arg;
{
	arg->mode = UNKNOWN;
	arg->type = UNKNOWN;
	arg->reg = -1;
	arg->index = -1;
	arg->labno = 0;
	arg->string = NULL;
#ifdef PREPOST
	arg->od_type = UNKNOWN;
	arg->odlabno = 0;
	arg->odstring = NULL;
#endif PREPOST
#ifdef VOLATILE
	arg->attributes = 0;
#endif
}

#ifdef DEBUG
memory_check()
{
	if (nodes_allocated != nodes_freed
	|| strings_allocated != strings_freed)
		internal_error("memory allocation error\n nodes_allocated=%d nodes_freed=%d\n strings_allocated=%d strings_freed=%d\n",
			nodes_allocated, nodes_freed,
			strings_allocated, strings_freed);
}
#endif DEBUG
