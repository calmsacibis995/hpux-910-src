/* @(#) $Revision: 70.1 $ */    
/*****************************************************************
 * C Shell
 ******************************************************************/

#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#define NL_SETN 15	/* set number */
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif NLS

#include "sh.h"
/*  Called indirectly by func () when 'set' seen.
*/
/**********************************************************************/
doset(v)
	register CHAR **v;
/**********************************************************************/
{
	register CHAR *p;
	CHAR *vp, op;
	CHAR **vecp;
	CHAR *ss;
	bool hadsub;
	int subscr;

#ifdef DEBUG_SET
  {
    CHAR **traceSetArgs;

    traceSetArgs = v;

    printf ("doset (1): pid: %d, arguments:\n", getpid ());

    while (*traceSetArgs)
      {
	printf ("\t%s\n", to_char (*traceSetArgs));
	traceSetArgs ++;
      }
  }

#endif

/*  Reset the status variable.  This is done before any processing on the 
    variable to be set so that if an error occurs during this processing which
    sets the status variable, then the status at the end will reflect that 
    error.  We need to set it to 0 now so that if no errors occur then we will
    have a status of 0 at the end.
*/
	set(CH_status, CH_zero);	

	v++;
	p = *v++;
	if (p == 0) {

		prvars();
		return;
	}

	do {
		hadsub = 0;
		for (vp = p; alnum(*p); p++)
			continue;
		if (vp == p)
			goto setsyn;
		if (*p == '[') {
			hadsub++;
			p = getinx(p, &subscr);
		}
		if (op = *p) {
			*p++ = 0;
			if (*p == 0 && *v && **v == '(')
				p = *v++;
		} else if (*v && eq(*v, "=")) {
			op = '=', v++;
			if (*v)
				p = *v++;
		}
		if (op && op != '=')
setsyn:
			bferr((catgets(nlmsg_fd,NL_SETN,1, "Syntax error")));
		if (eq(p, "(")) {
			register CHAR **e = v;

			if (hadsub)
				goto setsyn;
			for (;;) {
				if (!*e)
					bferr((catgets(nlmsg_fd,NL_SETN,2, "Missing )")));
				if (**e == ')')
					break;
				e++;
			}
			p = *e;
			*e = 0;

/*  Saveblk calloc's space, so vecp needs to be freed eventually.
*/
			vecp = saveblk(v);

/*  This modifies the alias list.
*/
			set1(vp, vecp, &shvhed);
			*e = p;
			v = e + 1;
		} 


/*  Savestr calloc's space, so this needs to be freed eventually.
*/
	     else if (hadsub)
			asx(vp, subscr, savestr(p));
		else {

/*  Savestr calloc's space, so both of these need to be freed eventually.
*/
			if (eq(vp, "home"))
				ss = savestr(value(vp));
			set(vp, savestr(p));
		}

		if (eq(vp, "path")) {
			exportpath(adrof(CH_path)->vec);
			dohash();
		} else if (eq(vp, "histchars")) {
			register CHAR *p = value(CH_histchars);

			HIST = *p++;
			HISTSUB = *p;
		} else if (eq(vp, "user"))
			setenv(CH_USER, value(vp));
		else if (eq(vp, "term"))
			setenv(CH_TERM, value(vp));
		else if (eq(vp, "home")) {
			if (*(value(vp)) != '/') {
				set(vp, ss);
				bferr((catgets(nlmsg_fd,NL_SETN,11, "\"home\" must be an absolute path")));
			}
			else
				setenv(CH_HOME, value(vp));
		}
	} while (p = *v++);
}

/*  Called by:
	doset ()
	dolet ()
*/
/**********************************************************************/
CHAR *
getinx(cp, ip)
	register CHAR *cp;
	register int *ip;
/**********************************************************************/
{

	*ip = 0;
	*cp++ = 0;
	while (*cp && digit(*cp))
		*ip = *ip * 10 + *cp++ - '0';
	if (*cp++ != ']')
		bferr((catgets(nlmsg_fd,NL_SETN,3, "Subscript error")));
	return (cp);
}

/*  Called by:
	doset ()
	dolet ()
*/
/**********************************************************************/
asx(vp, subscr, p)
	CHAR *vp;
	int subscr;
	CHAR *p;
/**********************************************************************/
{
	register struct varent *v = getvx(vp, subscr);

	xfree(v->vec[subscr - 1]);
	v->vec[subscr - 1] = globone(p);
}

/*  Called by:
	asx ()
	dolet ()
*/
/**********************************************************************/
struct varent *
getvx(vp, subscr)
	CHAR *vp;
	int subscr;
/**********************************************************************/
{
	register struct varent *v = adrof(vp);

	if (v == 0)
		udvar(vp);
	if (subscr < 1 || subscr > blklen(v->vec))
		bferr((catgets(nlmsg_fd,NL_SETN,4, "Subscript out of range")));
	return (v);
}

CHAR	plusplus[2] = { '1', 0 };


/*  Called indirectly by func () when '@' seen.
*/
/**********************************************************************/
dolet(v)
	CHAR **v;
/**********************************************************************/
{
	register CHAR *p;
	CHAR *vp, c, op;
	bool hadsub;
	int subscr;

#ifdef TRACE_DEBUG
  {
    CHAR **traceV;
    traceV = v;

    printf ("dolet (1): pid: %d\n", getpid ());
    while (*traceV)
      {
        printf ("\tv: %s\n", to_char (*traceV));
	traceV ++;
      }
  }
#endif

/*  Reset the status variable.  This is done before any processing on the 
    variable to be set so that if an error occurs during this processing which
    sets the status variable, then the status at the end will reflect that 
    error.  We need to set it to 0 now so that if no errors occur then we will
    have a status of 0 at the end.
*/
	set(CH_status, CH_zero);	

/*  v is an array of strings.  For example, if the statement was '@ hours = -1'
    then v is:
      v [0]: @
      v [1]: hours
      v [2]: =
      v [3]: -1

    v is incremented to point to 'hours', p is set to point to this same string,
    and then v is incremented to point to '='.
*/
	v++;
	p = *v++;
	if (p == 0) {

		/* 
		 *  Reset the status variable.  This is done before printing
		 *  anything, in case the status needs to be set by the
		 *  printing routines.  (Doesn't look like this happens
		 *  now, but you never know about the future!
		 */
		set(CH_status, CH_zero);	

		prvars();
		return;
	}

/*  This loop runs as long as p is not NULL.  p is stepped through each of
    the strings in v.
*/
	do {

#ifdef TRACE_DEBUG
  printf ("dolet (2): pid: %d, p: %s\n", getpid (), to_char (p));
#endif

		hadsub = 0;

/*  As long as the characters in p are alpha-numerics, this is the name of
    the variable.  Loop to the end of the name.  Remember the beginning of
    the name in vp.
*/
		/* for (vp = p; letter(*p); p++) */
		for (vp = p; alnum(*p); p++)	/* changed to allow @ x1 */
			continue;

/*  If there weren't any characters in the name, this is an error.
*/
		if (vp == p)
			goto letsyn;

/*  If the name was followed by a [, then this is a subscripted variable.
*/
		if (*p == '[') {
			hadsub++;
			p = getinx(p, &subscr);
		}

#ifdef TRACE_DEBUG
  printf ("dolet (3): pid: %d, p: %s\n", getpid (), to_char (p));
  printf ("\tv: %s\n", to_char (*v));
#endif

/*  p should point to the end of the name string, the NULL character.  v should
    point to the next element of its array, in the example above, the '='.
    Make p point to this and move v to point to the next element of the string
    array.
*/
		if (*p == 0 && *v)
			p = *v++;

#ifdef TRACE_DEBUG
  printf ("dolet (4): pid: %d, p: %s\n", getpid (), to_char (p));
  printf ("\tv: %s\n", to_char (*v));
#endif

/*  Set op to point the the first character of p.  Then set this character of
    p to NULL, and increment p to point to the next character, which should
    be NULL if there was only one operator.  Since op is declared to be a 
    CHAR, changing *p does not affect it.
*/
		if (op = *p)
			*p++ = 0;

/*  If there isn't an operator, this is an error.
*/
		else
			goto letsyn;

#ifdef TRACE_DEBUG
  printf ("dolet (5): pid: %d, p: %s\n", getpid (), to_char (p));
  printf ("\top: %c\n", (op & 0377));
#endif

/*  vp still points to the name of the variable, in the original v array.
    Save the name in calloc'd space and set vp to point to it.  This pointer
    will eventually have to be freed.
*/
		vp = savestr(vp);

/*  If the operator is an =, xset is called to add the value pointed to by
    p in the string array v, then to save the whole thing and return a pointer
    to it in p.  Note that the actual value has now been stored as a string.
*/
		if (op == '=') {
			c = '=';
			p = xset(p, &v);

#ifdef TRACE_DEBUG
  printf ("dolet (6): pid: %d, p: %s\n", getpid (), to_char (p));
#endif

		} else {
			c = *p++;
			if (any(c, "+-")) {
				if (c != op || *p)
					goto letsyn;
				p = plusplus;
			} else {
				if (any(op, "<>")) {
					if (c != op)
						goto letsyn;
					c = *p++;
letsyn:
					bferr((catgets(nlmsg_fd,NL_SETN,5, "Syntax error")));
				}
				if (c != '=')
					goto letsyn;
				p = xset(p, &v);
			}
		}
		if (op == '=')
			if (hadsub)
				asx(vp, subscr, p);
			else
			  {

#ifdef TRACE_DEBUG
  printf ("dolet (7): pid: %d, variable (vp): %s\n", getpid (), to_char (vp));
  printf ("\tvalue (p): %s\n", to_char (p));
#endif

/*  Save the value pointed to by p in the variable name pointed to by vp.
*/
				set(vp, p);
			  }
		else
			if (hadsub)
#ifndef V6
				/* avoid bug in vax CC */
				{
					struct varent *gv = getvx(vp, subscr);

					asx(vp, subscr, operate(op, gv->vec[subscr - 1], p));
				}
#else
				asx(vp, subscr, operate(op, getvx(vp, subscr)->vec[subscr - 1], p));
#endif
			else
				set(vp, operate(op, value(vp), p));
		if (eq(vp, "path"))
			dohash();

/*  Free the space containing the variable name.
*/
		xfree(vp);
		if (c != '=')
			xfree(p);
	} while (p = *v++);
}

/*  Called by:
	dolet ()
*/
/**********************************************************************/
CHAR *
xset(cp, vp)
	CHAR *cp, ***vp;
/**********************************************************************/
{
	register CHAR *dp;

#ifdef XSET_DEBUG
  printf ("xset (1): pid: %d, cp: %lu, *cp: %s, vp: %lu, *vp: %lu, **vp: %lu\n",
	  getpid (), (long) cp, to_char (cp), (long) vp, (long) *vp, 
	  (long) **vp);
#endif

/*  If the first pointer isn't pointing to NULL, save what it is pointing
    at and somehow add that to the string array.
*/
	if (*cp) {
		dp = savestr(cp);
		--(*vp);

#ifdef XSET_DEBUG
  printf ("xset (2): pid: %d, vp: %lu, *vp: %lu, **vp: %lu\n", getpid (), 
	   (long) vp, (long) *vp, (long) **vp);
#endif

		xfree(**vp);
		**vp = dp;

#ifdef XSET_DEBUG
  printf ("xset (3): pid: %d, vp: %lu, *vp: %lu, **vp: %lu\n", getpid (), 
	   (long) vp, (long) *vp, (long) **vp);
#endif
	}

/*  Both exp and putn do calloc's.  The putn routine saves a number as a string.
*/
	return (putn(exp(vp)));
}

/*  Called by:
	dolet ()
*/
/**********************************************************************/
CHAR *
operate(op, vp, p)
	CHAR op, *vp, *p;
/**********************************************************************/
{
	CHAR opr[2];
	CHAR *vec[5];
	register CHAR **v = vec;
	CHAR **vecp = v;
	register int i;

	if (op != '=') {
		if (*vp)
			*v++ = vp;
		opr[0] = op;
		opr[1] = 0;
		*v++ = opr;
		if (op == '<' || op == '>')
			*v++ = opr;
	}
	*v++ = p;
	*v++ = 0;
	i = exp(&vecp);
	if (*vecp)
		bferr((catgets(nlmsg_fd,NL_SETN,6, "Expression syntax")));
	return (putn(i));
}

/*  Called by:
	set ()
*/
/*  Purpose:  Determine if the address is outside the malloc space or not.
	      If so, return 1.
*/
/**********************************************************************/
onlyread(cp)
	CHAR *cp;
/**********************************************************************/
{
#if defined (hp9000s500) || defined (hp9000s800)

/*  Apparently, the S500 and S800 malloc spaces grow up in memory, so if the
    address is below the start (allocs) or above the top (alloct) it is not
    in the malloc space.
*/
	extern CHAR *allocs, *alloct;
	return ((cp <= allocs || cp > alloct));
#else

/*  Apparently the S300 malloc space grows down in memory, so if the address
    is less than the top (end), then it is not in the malloc space.
*/
	extern CHAR end[];
	return ((int) (cp < end));
#endif
}

/*  Called by just about every routine.
*/
/*  Purpose:  Free the space used by one string.
*/
/**********************************************************************/
xfree(cp)
	CHAR *cp;
/**********************************************************************/
{
#if defined (hp9000s500) || defined (hp9000s800)

/*  Allocs is the initial arena and alloct is the arena top.  If the address
    to free is within this area, then call cfree on it.
*/
	extern CHAR *allocs,*alloct;
	if (cp > allocs && cp <= alloct)
#else

/* The first part of this check is whether the address to free is above the
   malloc end.  The second part checks the address to free against the address
   containing that address ??  If the check is OK, call cfree to free the 
   address.
*/
	extern CHAR end[];
	if (cp >=end && cp < (CHAR *) &cp)
#endif	

/*  Free the string pointed to by cp.
*/
		cfree(cp);
}

/*  Called by just about every routine.
*/
/*  Purpose:  Copy a string into the malloc area.
*/
/**********************************************************************/
CHAR *
savestr(s)
/**********************************************************************/
	register CHAR *s;
{
	register CHAR *n;

	if (s == 0)
		s = nullstr;

/*  Calloc only returns a non-zero address.  This call gets the space
    for the string s and then copies s into that space.  A pointer to the
    new space is returned.
*/
	Strcpy(n = calloc(1, (unsigned)((Strlen(s)+1)*sizeof(CHAR))), s);
	return (n);
}

static	CHAR *putp;
 
/*  Called by:
	main ()
	Dgetdol ()
	exp3, 3a, 4, 5, 6 ()
	doexit ()
	gethent ()
	pjwait ()
	xset ()
	operate ()
*/
/*  Purpose:  To convert a number into a string and save it in calloc'd
	      space.
*/
/**********************************************************************/
CHAR *
putn(n)
	register int n;
/**********************************************************************/
{
	static CHAR number[15];

/*  putp is a global.
*/
	putp = number;
	if (n < 0) {
		n = -n;
		*putp++ = '-';
	}
	if (sizeof (int) == 2 && n == -32768) {
		*putp++ = '3';
		n = 2768;
#ifdef pdp11
	}
#else
	} 

/*  Something must happen with the largest possible 32 bit negative number.
    Somehow a negative number stayed negative after negation since at the 
    beginning of the routine if the number is < 0 then it is negated.  
*/
      else if (sizeof (int) == 4 && n == -2147483648) {
		*putp++ = '2';
		n = 147483648;
	}
#endif

/*  Store numbers in the string.
*/
	putn1(n);

/*  Terminate the string with a NULL.
*/
	*putp = 0;
	return (savestr(number));
}

/*  Called by:
	putn ()
	putn1 ()
*/
/*  Purpose:  Recursively put the digits of a number into a string as
	      ascii numbers.
*/
/**********************************************************************/
putn1(n)
	register int n;
/**********************************************************************/
{
/*  This works; called with 789, it calls itself with 78, then 7, then
    adds 7 to the string, then 8, then 9.
*/
	if (n > 9)
		putn1(n / 10);
	*putp++ = n % 10 + '0';
}

/*  Called by:
	exitstat ()
	mailchk ()
	dfind ()
	egetn ()
	dorepeat ()
	savehist ()
	dohist ()
	execute ()
	donice ()
*/
/**********************************************************************/
getn(cp)
	register CHAR *cp;
/**********************************************************************/
{
	register int n;
	int sign;

	sign = 0;
	if (cp[0] == '+' && cp[1])
		cp++;
	if (*cp == '-') {
		sign++;
		cp++;
		if (!digit(*cp))
			goto badnum;
	}
	n = 0;
	while (digit(*cp))
		n = n * 10 + *cp++ - '0';
	if (*cp)
		goto badnum;
	return (sign ? -n : n);
badnum:
	bferr((catgets(nlmsg_fd,NL_SETN,7, "Badly formed number")));
	return (0);
}

/*  Called by just about every routine.
*/
/*  Purpose:  Return a string which is the value of the variable passed
	      to this routine.
*/
/**********************************************************************/
CHAR *
value(var)
	CHAR *var;
/**********************************************************************/
{

	return (value1(var, &shvhed));
}

/*  Called by:
	value ()
*/
/*  Purpose:  Return a string which is the value of the variable passed
	      to this routine.
*/
/**********************************************************************/
CHAR *
value1(var, head)
	CHAR *var;
	struct varent *head;
/**********************************************************************/
{
/*  Structure varent:  CHAR **vec;
		       CHAR *name;
		       struct varent *link;
*/
	register struct varent *vp;

/*  Get a structure associated with the variable name.  Return 0 if there
    isn't such a variable or if the value is 0.  Otherwise, return the
    value of the variable.
*/
	vp = adrof1(var, head);
	return (vp == 0 || vp->vec[0] == 0 ? nullstr : vp->vec[0]);
}

static	struct varent *shprev;

/*  Called by lots of routines.
*/
/*  Purpose:  Return the structure of a variable if it exists.
*/
/**********************************************************************/
struct varent *
adrof(var)
	CHAR *var;
/**********************************************************************/
{

	return (adrof1(var, &shvhed));
}

/*  Called by:
	unset1 ()
*/
/**********************************************************************/
struct varent *
madrof(pat, head)
	CHAR *pat;
	struct varent *head;
/**********************************************************************/
{
	register struct varent *vp;

	shprev = head;
	for (vp = shprev->link; vp != 0; vp = vp->link) {
		if (Gmatch(vp->name, pat))
			return (vp);
		shprev = vp;
	}
	return (0);
}

/*  Called by:
	texec ()
	doalias ()
	asyn3 ()
	value1 ()
	adrof ()
	setq ()
	unsetv1 ()
*/
/*  Purpose:  Return a variable structure if the variable name is in 
	      the linked list, or 0 if it is not.  A side effect is that
	      the global shprev is left pointing to the element previous
	      to where an element with the target name would be located.
*/
/**********************************************************************/
struct varent *
adrof1(var, head)
	CHAR *var;
	struct varent *head;
/**********************************************************************/
{
/*  Structure varent:  CHAR **vec;
		       CHAR *name;
		       struct varent *link;
*/
	register struct varent *vp;
	int cmp;

/*  shprev is a global pointer to a variable structure.
*/
	shprev = head;

/*  Traverse the linked list, comparing the variable name passed to this 
    routine with the one stored in the structure.  If they match, return a
    pointer to the structure.  If the name in the structure is "larger" than
    the target name, return 0.  Otherwise, step to the next structure in the
    linked list.  If the entire list is traversed with no match, return 0.

    At the end of this loop, shprev will either point to the last element
    in the list or the element just before the one whose name was too "big".
*/
	for (vp = shprev->link; vp != 0; vp = vp->link) {
		cmp = Strcmp(vp->name, var);
		if (cmp == 0)
			return (vp);
		else if (cmp > 0)
			return (0);
		shprev = vp;
	}
	return (0);
}

/*
 * The caller is responsible for putting value in a safe place
 */
/*  Called by lots of routines.
*/
/**********************************************************************/
set(var, value)
	CHAR *var, *value;
/**********************************************************************/
{
/*  Get space for an array of 2 strings, vec [0] and vec [1].  A side
    effect is that the pointers are initialized to NULL.
*/
	register CHAR **vec = (CHAR **) calloc(2, sizeof (CHAR **));

/*  If the value is not in the malloc space, then save it.  Otherwise it is
    already in the malloc space.  Set vec [0] to point to the value in the 
    malloc space.
*/
	vec[0] = onlyread(value) ? savestr(value) : value;
	set1(var, vec, &shvhed);
}

/*  Called by:
	main ()
	importpath ()
	doalias ()
	doset ()
	set ()
*/
/*  Purpose:  Glob the value and continue with setting the variable to the
	      value.
*/
/**********************************************************************/
set1(var, vec, head)
	CHAR *var, **vec;
	struct varent *head;
/**********************************************************************/
{

	register CHAR **oldv = vec;

/*  Check for globbing characters in the value.  If there are any, call 
    glob to resolve them.  Glob returns a string array of pointers which has
    been calloc'd, so it will eventually need to be freed.  Set vec to point
    to this space.  The previous space is pointed to by oldv.  This space
    is then freed.
*/
	gflag = 0; rscan(oldv, tglob);
	if (gflag) {
		vec = glob(oldv);
		if (vec == 0) {
			bferr((catgets(nlmsg_fd,NL_SETN,8, "No match")));
			blkfree(oldv);
			return;
		}
		blkfree(oldv);
		gargv = 0;
	}
	setq(var, vec, head);
}

/*  Called by:
	main ()
	error ()
	set1 ()
*/
/**********************************************************************/
setq(var, vec, head)
	CHAR *var, **vec;
	struct varent *head;
/**********************************************************************/
{
	register struct varent *vp;

/*  Structure varent:  CHAR **vec;
		       CHAR *name;
		       struct varent *link;
*/

/*  Get a structure to the variable if it exists in the linked list.
*/
	vp = adrof1(var, head);

#ifdef DEBUG_SET
  printf ("setq (2): %d, value from adrof1: %ld\n", getpid (), (long) vp);
#endif

/*  If the variable doesn't exist, get space for one.  Calloc space for the
    name (using savestr), and save it in the structure name field.  Now,
    after the call to adrof1, the global shprev is pointing to the last
    element in the list, or the one just before where the new element should
    be inserted.  So make the link in the new element point to the one that
    shprev pointed to, and set shprev to point to the new element.
*/
	if (vp == 0) {
		vp = (struct varent *) calloc(1, sizeof *vp);
		vp->name = savestr(var);
		vp->link = shprev->link;
		shprev->link = vp;
	}

/*  If the variable was in the list previously, then the value element in 
    the structure is not NULL.  Free this element.
*/
	if (vp->vec)
		blkfree(vp->vec);

/*  For each string in the value array of strings, AND the characters with
    077777.  Then set the value element of the structure to point to the
    value string array.
*/
	scan(vec, trim);
	vp->vec = vec;
}

/*  Called indirectly by func when 'unset' seen.
*/
/**********************************************************************/
unset(v)
	register CHAR *v[];
/**********************************************************************/
{

	unset1(v, &shvhed);
	if (adrof(CH_histchars) == 0) {
		HIST = '!';
		HISTSUB = '^';
	}
}

/*  Called by:
	unset ()
	unalias ()
*/
/**********************************************************************/
unset1(v, head)
	register CHAR *v[];
	struct varent *head;
/**********************************************************************/
{
	register CHAR *var;
	register struct varent *vp;
	register int cnt;

	v++;
	while (var = *v++) {
		cnt = 0;
		while (vp = madrof(var, head))
			unsetv1(vp->name, head), cnt++;
		if (cnt == 0)
			setname(to_char(var));
	}
}

/*  Called by:
	pchild ()
*/
/**********************************************************************/
unsetv(var)
	CHAR *var;
/**********************************************************************/
{

	unsetv1(var, &shvhed);
}

/*  Called by:
	unset1 ()
	unsetv ()
*/
/**********************************************************************/
unsetv1(var, head)
	CHAR *var;
	struct varent *head;
/**********************************************************************/
{
	register struct varent *vp;

	vp = adrof1(var, head);
	if (vp == 0)
		udvar(var);
	vp = shprev->link;
	shprev->link = vp->link;
	blkfree(vp->vec);
	xfree(vp->name);
	xfree((CHAR *)vp);
}

/*  Called by:
	main ()
*/
/**********************************************************************/
setNS(cp)
	CHAR *cp;
/**********************************************************************/
{

	set(cp, nullstr);
}

/*  Called indirectly by func when 'shift' seen.
*/
/**********************************************************************/
shift(v)
	register CHAR **v;
/**********************************************************************/
{
	register struct varent *argv;
	register CHAR *name;

	v++;
	name = *v;
	if (name == 0)
		name = CH_argv;
	else
		(void) strip(name);
	argv = adrof(name);
	if (argv == 0)
		udvar(name);
	if (argv->vec[0] == 0)
		bferr((catgets(nlmsg_fd,NL_SETN,9, "No more words")));
	lshift(argv->vec, 1);
}

#ifndef NONLS
CHAR CH_colon[] = {':',0};
#else
#define CH_colon	":"
#endif

#define	MAXPATH		1024

/*  Called by:
	doset ()
*/
/**********************************************************************/
exportpath(val)
CHAR **val;
/**********************************************************************/
{
	CHAR exppath[MAXPATH];

	exppath[0] = '\0';    /* used to be initialized to 0. hn */
	if (val)
		while (*val) {
			if (Strlen(*val) + Strlen(exppath) + 2 > MAXPATH) {
				printf((catgets(nlmsg_fd,NL_SETN,10, "Warning: ridiculously long PATH truncated\n")));
				break;
			}
			Strcat(exppath, *val++);
			if (*val == 0 || eq(*val, ")"))
				break;
			Strcat(exppath, CH_colon);
		}
	setenv(CH_PATH, exppath);
}
