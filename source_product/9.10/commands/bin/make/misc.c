/* @(#) $Revision: 66.6.1.1 $ */     

/***************************************************************/
/* This file contains a mishmash of functions used throughout  */
/* the make code.  Eventually these will be organized by       */
/* functionality.  For now, just deciphering them will be      */
/* enough.                                                     */
/*                                                             */
/* Two of the functions, subst and straightrans, are mutually  */
/* recursive.  subst() expands macros in a given input string  */
/* and straightrans does something mysterious.                 */
/***************************************************************/

#include "defs"
#include <ctype.h>
#ifdef NLS16
#include <nl_ctype.h>
#endif

CHARSTAR do_colon();
CHARSTAR do_df();


FSTATIC CHARSTAR nextchar=0;
FSTATIC CHARSTAR lastchar=0;

FSTATIC int *nextint=0;
FSTATIC int *lastint=0;

FSTATIC NAMEBLOCK *hashtab;  /* array of doubly linked nameblocks */
int nhashed=0;
int hashsize = HASHSIZE;
int gcount = 1;

/***************** HASH TABLE FUNCTIONS ********************/

/* The following hash function was written by Jack Applin for
	8.0.  It has significantly better performance than the
	old one.
*/


hashloc(s)
CHARSTAR s;
/************************************************/
/* This function does a simple linear hash into */
/* a hashtable.   The hash function is the sum  */
/* of the characters in the string mod the hash */
/* table size.                                  */ 
/* All this function does is locate the position*/
/* in the hashtable into which the string can   */
/* or has been inserted.  It does not actually  */
/* do the insertion.  The index value of the    */
/* hashtable location is returned.              */
/************************************************/
{
    register int i;
    register int j;
    register int k;
    register int count;
    register int hashval;
    register CHARSTAR t;

    hashval = 0;

    if (!hashtab)
	if (!(hashtab = (NAMEBLOCK *)calloc(HASHSIZE, sizeof(NAMEBLOCK))))
		fatal("out of memory");

    i = 0;
    for (t=s; *t!=CNULL ; ++t)
	{
		hashval += *t << i;
		i+=19;
		if (i>=24)
			i-=24;
    }
    hashval &= 0x7fffffff;	      /* Don't mod negative numbers */
    hashval %= HASHSIZE;          /* mod part of hash function */

         /* search for spot for s - keep going until you */
         /* find an empty spot, or until a non-empty spot*/
         /* contains the name you are looking for (s)    */

    for(i=hashval, j=0, count=0; hashtab[i]!=0 && !equal(s,hashtab[i]->namep); j++, i++) 
	{
		k = HASHSIZE * count;	
		if (i == (k + HASHSIZE - 1) && hashval) { /* i has reached    */
			i = k - 1;                        /* end of table     */
			continue;			  /* reset it to 0    */
		}
			
		if ( j == HASHSIZE - 1) {		 /* Finished searching*/
			if (count < gcount - 1) {        /* a hashtable check */
				j = -1;                  /* for next table    */
				i = hashval + k + HASHSIZE - 1;
				count += 1;
			}
			else 
				return -1;               /* no more tables to */
							 /* search realloc    */
							 /* needed            */
		}
    }
          /* what keeps the above for loop from looping endlessly */
          /* is a limit imposed in makename(s).  The hashtable    */
          /* can never be absolutely full, guaranteeing a termi-  */
          /* nation to this loop                                  */ 

    return(i);
}


NAMEBLOCK
srchname(s)
register CHARSTAR s;
/****************************************/
/* Returns the NAMEBLOCK structure in   */
/* the hashtable which contains the name*/
/* s                                    */
/****************************************/
{
    int i;
    i = hashloc(s);
    if (i == -1)
	return (NAMEBLOCK)0;
    return( hashtab[i] );
}




NAMEBLOCK
makename(s)
register CHARSTAR s;
/****************************************/
/* Inserts the filename 's' into the    */
/* hashtable AND the firstname linked   */
/* list.                                */
/****************************************/
{
/* make a name entry; `s' is presumed to already to have been saved */

    register NAMEBLOCK p;

         /* check how many hashtable entries exist and then */
         /* increment the hash entry counter, which is a    */
         /* global                                          */

    if(nhashed++ >= hashsize) {
	hashtab = (NAMEBLOCK *)realloc(hashtab, (hashsize + HASHSIZE) * sizeof(NAMEBLOCK));
	gcount += 1;
	if (hashtab == '\0')
	    fatal("Cannot allocate memory");
	else {
	    memset(hashtab + hashsize, '\0', sizeof(NAMEBLOCK) * HASHSIZE);
	    hashsize += HASHSIZE;
	}
    }

         /* there is room to make another hash table entry */
 
    p = ALLOC(nameblock);       /* set up a new NAMEBLOCK for firstname */
    p->nextname = firstname;    /* global var - linked list             */
    p->backname = NULL; 

    p->namep = s;               /* fill NAMEBLOCK with info */
    p->linep = 0;
    p->done = 0;
    p->septype = 0;
    p->rundep  = 0;
#ifdef	PATH
    p->path = 0;
#endif	PATH
    p->modtime = 0;

    firstname = p;             /* tack new NAMEBLOCK on front of */
                                   /* firstname list                 */

    hashtab[hashloc(s)] = p;   /* stick pointer to new NAMEBLOCK */
                                   /* into hash table                */

    return(p);                 /* also return the new pointer    */
}







#define NOTSHORT sizeof (struct nameblock)
#define TABLESIZE 100
#define TRUE 1
#define FALSE 0

CHARSTAR
copys(s)
register CHARSTAR s;
{
    CHARSTAR t;
    static struct entry			/* structure will form linked      */
 	   {				/* list of strings in alpha-order. */
	      CHARSTAR name;		/* table is hash-table of 100 els. */
	      struct entry *next;	/* used to make searching faster.  */
	   } *table[TABLESIZE],*laste,*newe,*e;

    int found;
    int i;
    int hashval = 0;

    for(t=s; *t != CNULL; ++t)		/* Hash the string - sum of chars */
	hashval += *t;

    hashval %= TABLESIZE;		/* Mod tablesize */

    e = table[hashval];			/* initial element */
    laste = table[hashval];
    while(TRUE)
	if(e == NULL)			/* No - table[hashval] == NULL */
	{				/* or end of linked list       */
	    if((e = (struct entry *) calloc(1,sizeof(struct entry))) == NULL)
		fatal("Cannot allocate memory");
	    if((e->name = (unsigned char *) calloc(strlen(s)+1,sizeof(*s))) == NULL)
		fatal("Cannot allocate memory");

	    strcpy(e->name,s);
	    e->next = NULL;

	    if(!table[hashval])
		table[hashval] = e;
	    else
		laste->next = e;
	        break;
	}
	else 
 	    if((i=strcmp(e->name,s)) == 0)
	        break;			/* e points to entry */
	    else
                if(i>0)
		{			/* create entry  and link */
		    if((newe=(struct entry *) calloc(1,
				sizeof(struct entry))) == NULL)
			fatal("Cannot allocate memory");
 		    if((newe->name=calloc(strlen(s)+1,sizeof(*s)))==NULL)
			fatal("Cannot allocate memory");

		    strcpy(newe->name,s);

		    newe->next = e;
		    if(laste == table[hashval])
			table[hashval] = newe;
		    else
			laste->next = newe;

		    e = newe;			/* set up e for return value */
		    break;
		}
		else 
		{
		    laste = e;
		    e = e->next;
	        }

    return(e->name);
}


CHARSTAR
concat(a,b,c)  
register CHARSTAR a, b;
register CHARSTAR c;
/*********************************************/
/* c = concatenation of a and b              */
/*********************************************/
{
    register CHARSTAR t;
    t = c;

    if (a)                   /* a is not empty string */
	while(*t = *a++)     /* copy characters from a to c */
            t++;             /* this prevents the null from a being copied */
    if (b)                   /* b is not empty string */
	while(*t++ = *b++);  /* copy characters from b to c */ 

    return(c);
}

suffix(a,b,p)  
register CHARSTAR a, b, p;
/*********************************/
/* Is b the suffix of a?         */
/* If so, set p = prefix         */
/*********************************/
{
    CHARSTAR a0, b0;
    a0 = a;
    b0 = b;

    if(!a || !b)
	return(0);

    while(*a++);
    while(*b++);

    if( (a-a0) < (b-b0) )
	return(0);

    while(b>b0)
	if(*--a != *--b)
	    return(0);

    while(a0<a)
        *p++ = *a0++;

    *p = CNULL;

    return(1);
}



int * 
intalloc(n)
register int n;
{
    register INTSTAR p;

    if( p = (int *) calloc(1,n) )
	return(p);

    fprintf(stderr,"out of memory");
    fatal("out of memory");
}



extern NAMEBLOCK cur_name;

#ifndef NLS16

CHARSTAR
subst(a,b,maxchr)
register CHARSTAR a, b, maxchr;
/************************************************************/
/* copy string a into b, expanding any macros recursively.  */ 
/* This whole routine is substituted for in event of NLS16  */
/* being used.  The altered routine follows this one.       */
/*                                                          */
/* subst() is indirectly recursive.  After some boundary    */
/* condition checking (have we recursed too many times?     */
/* is the expanded string too long?  is the input string    */
/* pointer null?) characters are copied directly from a to b*/
/* until and unless a $ is read.  If a $ is detected in a,  */
/* and the $ is not an empty macro definition or a double   */
/* $$, the macro name following the $ is parsed into vname. */
/* vname is then categorized as either a straight macro     */
/* translation, a macro translation with substitution indi- */
/* cated, or a macro with a D or F specifier.               */
/* Three different routines are called to deposit the       */
/* resulting macro value into b, and a pointer to the end   */
/* of b is returned.  This allows the recursion to progress */
/* down b depositing further values, until a is empty and   */
/* all recursive calls terminate.                           */
/*                                                          */
/* The length of the expansion is limited to one of several */
/* constants, usually INMAX or OUTMAX.  The reason for      */
/* limiting the length is historical and unknown.  The      */
/* constants have been up'd several times in response to    */
/* user complaints.  A better solution needs to be applied  */
/* here.  Look into dynamically allocating space for this   */
/* expansion instead of using constants.                    */
/************************************************************/
/* There is a defect here - actually in colontrans and      */
/* dftrans.  straightrans is mutually recursive with subst  */
/* which allows for unlimited expansion of nested macros    */
/* but colontrans and dftrans are not recursive.  I am      */
/* looking into fixing these two procedures.                */
/************************************************************/
{
    register CHARSTAR s;
    static depth=0;
    unsigned char vname[100];
    unsigned char closer;

    if(++depth > 100)
	fatal("infinitely recursive macro?");
    if(a!=0)
    {
	while(*a)
	{
	    if (b >= maxchr) 
            {
				/* 
				   if the stack grows upward and so
				   do strings, in theory the string could
				   grow so rapidly that it overwrites the
				   arguments to this procedure before
				   this check can be made, and BOOM!
				   500-land please note, this means
				   you.  (210 bug number 626 - partial
				   fix sufficient for 210 only.)
				   DT
				*/
		fatal("Macro expansion too big");
	    }
	    if(*a != DOLLAR)
		*b++ = *a++;
	    else
                if(*++a==CNULL || *a==DOLLAR)
		    *b++ = *a++;
		else
		{
		    s = vname;
		    if( *a==LPAREN || *a==LCURLY )
		    {
			closer=(*a==LPAREN ? RPAREN : RCURLY);
			++a;
			while(*a == BLANK)
			    ++a;
			while(	*a!=BLANK  && *a!=closer && *a!=CNULL)
			    *s++ = *a++;
			while(*a!=closer && *a!=CNULL)
			    ++a;
			if(*a == closer)
			    ++a;
		    }
		    else
			*s++ = *a++;
		    *s = CNULL;
		    if(amatch(&vname[0], "*:*=*"))
		        b = colontrans(b, vname);
		    else
		        if(any("@*<%?", vname[0]) && vname[1])
		            b = dftrans(b, vname);
			else
			    b = straightrans(b, vname, maxchr);
		    s++;
		}
	}
    }

    *b = CNULL;
    --depth;
    return(b);
}

#else 
/* NLS: the whole routine was rewritten so as to avoid an entire set */
/* 	of ugly ifdefs.  */

CHARSTAR
subst(a,b,maxchr)
register CHARSTAR a, b, maxchr;
{
    register CHARSTAR s;
    static depth=0;
    unsigned char vname[100];
    unsigned char closer;

    register int last_status = ONEBYTE;

#define BSTATUS 	last_status = BYTE_STATUS( *a, last_status)
			/* NLS: macro used to determine byte length of char */

    if(++depth > 100)
	fatal("infinitely recursive macro?");
    if(a!=0)
    {
	BSTATUS;		/* NLS: get status of curr char */
	while(*a)
	{
	    if (b >= maxchr) 
            {
				/* 
				   if the stack grows upward and so
				   do strings, in theory the string could
				   grow so rapidly that it overwrites the
				   arguments to this procedure before
				   this check can be made, and BOOM!
				   500-land please note, this means
				   you.  (210 bug number 626 - partial
				   fix sufficient for 210 only.)
				   DT
				*/
	         fatal("Macro expansion too big");
	    }
			/* NLS: check for byte redefinition */
	    if(*a != DOLLAR || last_status == SECOF2)
	    {
		*b++ = *a++;
		BSTATUS;
	    }
	    else 
	    {
		a++;     	/* NLS: increment & check */
		BSTATUS;
		if(*a==CNULL || ((*a==DOLLAR) && (last_status == ONEBYTE)))
		{
		    *b++ = *a++;
		    BSTATUS;
		}
		else
		{
		    s = vname;
		    if((*a==LPAREN || *a==LCURLY ) && (last_status == ONEBYTE))
		    {
			closer=(*a==LPAREN ? RPAREN : RCURLY);
			++a;
			BSTATUS;
			while(*a == BLANK)
			{
			    ++a;
			    BSTATUS;
			}
			while (	*a!=BLANK  && ((*a!=closer) ||
				(last_status == SECOF2)) && *a!=CNULL)
			{
			    *s++ = *a++;
			    BSTATUS;
			}
			while(((*a!=closer) || (last_status == SECOF2))
							&& *a!=CNULL)
			{
			    ++a;
			    BSTATUS;
			}
			if((*a == closer) && (last_status == ONEBYTE))
			{
			    ++a;
			    BSTATUS;
			}

		    }
	  	    else
		    {
		        *s++ = *a++;
			BSTATUS;
		    }

		    *s = CNULL;
		    if(amatch(&vname[0], "*:*=*"))
			b = colontrans(b, vname);
		    else
			if(any("@*<%?", vname[0]) && vname[1])
			    b = dftrans(b, vname);
			else
			    b = straightrans(b, vname, maxchr);
		    s++;
		}
             }
        }
    }

    *b = CNULL;
    --depth;
    return(b);
}
#endif


CHARSTAR
colontrans(b, vname)
register CHARSTAR b;        /* to be returned with substitutions made */
unsigned char vname[];      /* full substitute string with macro name */
/*****************************************************/
/* MACRO STRING SUBSTITUTION                         */
/*                                                   */
/* vname contains the substring $(MACRO:*=*).  From  */
/* this, the macro name is extracted and its value   */
/* either computed (builtins) or retrieved from      */
/* firstvar.  The substitution sequence *=* is then  */
/* isolated and fed with the macro value to do_colon */
/* which does the substitution and returns the result*/
/* in b which is then returned by this routine.      */
/*****************************************************/
{
    register CHARSTAR p;              /* value of named macro */
    register CHARSTAR q = 0;
    unsigned char dftype = 0;
    CHARSTAR pcolon;                 /* pointer to location of colon */
    VARBLOCK vbp;                    /* macro def from firstvar      */

#ifndef NLS16
    for(p = &vname[0]; *p && *p != KOLON; p++);   /* find COLON in string */
#else
    for(p = &vname[0]; *p && *p != KOLON; ADVANCE(p));
	/* NLS: advance to start of next char (not necessarily next byte) */
#endif
    pcolon = p;                      /* save location of COLON in string */
    *pcolon = CNULL;                 /* replace the COLON with a null    */
                                     /* so the following calls to any()  */
                                     /* and srchvar will only work on    */
                                     /* macro name part of vname         */ 

    if(any("@*<%?", vname[0]))      /* handle built-in macro names       */
    {
	dftype = vname[1];          /* not sure - stripping dollar sign? */ 
	vname[1] = CNULL;         
    }
    if((vbp = srchvar(vname)) == NULL)
	return(b);                 /* return empty string if macro not found */
    p = vbp->varval;               /* get value of macro from firstvar */
    if(dftype)
    {
	if((q = calloc((strlen(p)+2),1)) == NULL)   /* NLS fix */
	    fatal("Cannot alloc mem");
	do_df(q, p, dftype);       /* bug fix, pass dftype, not vname[1] */
	p = q;
    }
    if(p && *p)                   /* p is string to be substituted into */
	b = do_colon(b, p, pcolon+1);  /* pcolon+1 points to sub. expression */
    *pcolon = KOLON;              /* put the COLON back into original string */
    if(dftype)
	vname[1] = dftype;
    if(q)
        cfree(q);
    return(b);
}

CHARSTAR
do_colon(to, from, trans)
register CHARSTAR to;         /* to is the string into which the      */
                              /* substitution is written              */
register CHARSTAR from;       /* from is the value of the macro       */
                              /* into which substitution will be made */
CHARSTAR trans;               /* string specifying substitution       */
/********************************************************************/
/* Translate the $(name:*=*) type things.  The left and right sides */
/* of the substitution sequence following a macro are isolated and  */
/* the appropriate substitution is applied to each substring,       */
/* delimited by blanks or tabs, in the macro value (from).  The     */
/* substitution is done only if the pattern to be replaced is found */
/* at the end of the substrings in the macro value.                 */
/*                                                                  */
/* The use of sindex() to determine a pattern match for substitution*/
/* reasons introduced a bug in the original AT&T code.              */
/* sindex() only returns the first occurence of a pattern in a      */
/* string, whereas this routine is interested ONLY in matches at the*/
/* end of the string.  Therefore, for those substrings in which the */
/* pattern occurred both embedded and at the end, substitution was  */
/* not made.  The call to sindex() to detect a pattern match has    */
/* been replaced with the code so noted below, which goes directly  */
/* to the end of the string to look for the pattern to be           */
/* substituted for.                                                 */
/* This fix addresses defects FSDlj02628 and FSDlj04766.            */
/********************************************************************/

{
    register int i;         /* counter */
    register CHARSTAR p;
    register int leftlen;   /* length of pattern to be substituted for */
    int len;                /* length of substring to be sub'd into    */
    unsigned char left[30], right[70]; /* holds left and right sides of */
                                       /* substitution expression */ 
    unsigned char buf[256]; /* working buffer for substitution */
    CHARSTAR pbuf;          /* working pointer to substring to be sub'd into */
    CHARSTAR findit;        /* marker for place to begin the substitution */
    int lwig = 0;           /* flag detecting sccs sub.  on left side */
    int rwig = 0;           /* flag detecting sccs sub.  on right side */
    int match;              /* flag if end of string match is found */

#ifdef NLS16
    int last_stat = ONEBYTE;		/* NLS: holds value of byte */
    int prev_stat = ONEBYTE;		/* holds val of previous byte */
#endif

/*  Break the substitution expression into */
/*  two pieces, left[] and right[], and    */
/*  throw away the = sign                  */ 

    i = 0;

#ifndef NLS16
    while(*trans != EQUALS)
        left[i++] = *trans++;      /* copy pattern to be substituted for */
#else
    while (*trans != EQUALS  || last_stat == SECOF2)
    {
	left[i++] = *trans++;
	prev_stat = last_stat;    	/* NLS: save last status */
	last_stat = BYTE_STATUS( *trans, last_stat);
			/* NLS: get value of current byte */
    }
#endif

#ifndef NLS16
    if(left[i-1] == WIGGLE)
#else
    if (left[i-1] == WIGGLE && prev_stat == ONEBYTE)
			/* NLS: check for single byte character */
#endif
    {
	lwig++;
	--i;
    }
    left[i] = CNULL;
    leftlen = i;
    i = 0;

#ifndef NLS16
    while(*++trans)
	right[i++] = *trans;   /* copy pattern to be substituted */
#else
    while (*++trans) 
    {
	last_stat = BYTE_STATUS(*trans, last_stat);
	right[i++] = *trans;
    }
#endif

#ifndef NLS16
    if(right[i-1] == WIGGLE)
#else
    if(right[i-1] == WIGGLE && last_stat == ONEBYTE)
			/* NLS: check for single byte character */
#endif
    {
	rwig++;
	--i;
    }
    right[i] = CNULL;

/*	Now, make the substitutions and copy the result to "to"  */ 

    for(; len=getword(from,buf); from += len)
    {
        pbuf = buf;

/* this code has been rewritten to not call sindex(), but to go */
/* straight to the end of each piece of the macro value and do  */
/* a pattern match with left[]                                  */

        if (len >= leftlen)  /* pattern search only if buf is long enough */
        {
            findit = &pbuf[len-leftlen]; /*locate place to compare strings*/
            if (strncmp(left,findit,leftlen) == 0) /*match at end of string  */
            {
                match = YES; 
                *findit = CNULL;       /* null out pbuf where match began */
            }
            else
                match = NO;    
        }
        else
            match = NO;
     
        if ((match == YES) && (lwig?((p=sdot(pbuf))!=(unsigned char *)0):1) )
	{
	    if(!lwig && rwig)
	        trysccs(pbuf);
	    else
		if(lwig && !rwig)
		    strshift(p, -2);
	    to = copstr(to, pbuf);
	    to = copstr(to, right);
	}
	else
	{
	    to = copstr(to, pbuf);
	}
    }
    return(to);
}


getword(from, buf)
register CHARSTAR from;
register CHARSTAR buf;
/*************************************************************/
/* This function takes a string, from, and returns a piece of*/
/* it.  The piece is either a real character string or a     */
/* string of blanks and tabs and also returns the number of  */
/* characters copied                                         */
/*************************************************************/
{
    register int i = 0;         /* count number of chars copied */

    if(*from == TAB || *from == BLANK) 
    {
	while(*from == TAB || *from == BLANK)
	{
	    *buf++ = *from++;  /* copy all blanks and tabs */
	    ++i;
	}
	goto out;
    }
    while(*from && *from != TAB && *from != BLANK)
    {
	*buf++ = *from++;    /* copy only good characters */
	++i;
    }
out:
    *buf = CNULL;           /* in either case, null terminate */
    return(i);              /* and return character count */
                            /* this will be 0 when from is empty */
}


CHARSTAR
dftrans(b, vname)
register CHARSTAR b;
CHARSTAR vname;
/*********************************************/
/* This function translates the D and F sub- */
/* stitutions in macro references            */
/*********************************************/
{
    register unsigned char c1;
    VARBLOCK vbp;

    c1 = vname[1];
    vname[1] = CNULL;
    vbp = srchvar(vname);
    vname[1] = c1;
    if(vbp != 0 && *vbp->varval != 0)
	b = do_df(b, vbp->varval, c1);
    return(b);
}

#define lastslash(a)		strrchr( (a) , SLASH )


CHARSTAR
do_df(b, str, type)
register CHARSTAR b;
unsigned char str[];
unsigned char type;
/********************************************/
/* I guess this is where the really dirty   */
/* work of the $(@D) and $(@F) translations */
/* takes place.                             */
/********************************************/
{
    register CHARSTAR p;
    register int i;
    unsigned char buf[256];

    *b = CNULL;
    for(; (i=getword(str, buf)); str += i)
    {
	if(buf[0] == BLANK || buf[0] == TAB)
	{
	    b = copstr(b, buf);
	    continue;
	}
	p = lastslash(buf);
	if(p)
	{
	    *p = CNULL;
	    b = copstr(b, type== (unsigned char) 'D'?(buf[0]==(unsigned char) CNULL? (unsigned char *) "/":buf):p+1);
	    *p = SLASH;
	}
	else
             /* POSIX bug fix - return ./, not . for empty directory part */
	    b = copstr(b, type== (unsigned char ) 'D'? (unsigned char *) "./":buf);
    }
    return(b);
}




CHARSTAR
straightrans(b, vname, maxchr)
register CHARSTAR b, maxchr;
unsigned char vname[];
/*************************************************/
/* I have saved this very helpful comment until  */
/* I can figure out what this does:              */
/*     Standard trnaslation, nothing fancy.      */
/* Great, huh?                                   */
/*************************************************/
{
    register VARBLOCK vbp;
    register CHAIN pchain;
    register NAMEBLOCK pn;

    vbp = srchvar(vname);
    if( vbp != 0)
    {
	if(vbp->v_aflg == YES && vbp->varval)
	{
	    pchain = (CHAIN)vbp->varval;
	    for(; pchain; pchain = pchain->nextchain)
	    {
		if (b >= maxchr)
		    fatal("Macro translation too big.");
		pn = (NAMEBLOCK)pchain->datap;
		if(pn->alias)
		    b=copstr(b, pn->alias);
		else
		    b=copstr(b,pn->namep);
		*b++ = BLANK;
	    }
	    vbp->used = YES;
	}
	else
	    if(vbp->varval && *vbp->varval)
	    {
		b = subst(vbp->varval, b, maxchr);
		vbp->used = YES;
	    }
    }
    return(b);
}



CHARSTAR
copstr(s, t)
register CHARSTAR s, t;
/*************************************************************/
/* copy t into s, and return the location of the end of s,   */
/* the end of string character.  If t is empty, s is         */
/* returned unchanged.                                       */
/*************************************************************/
{
    if(t == 0)
	return(s);
    while (*t)
	*s++ = *t++;
    *s = CNULL;
    return(s);
}

setvar(v,s)
register CHARSTAR v, s;
/************************************************************/
/* This often used function inserts the value s into the    */
/* global linked list firstvar, which contains macro names  */
/* and values.  v is the macro name, and s is the value.    */
/* firstvar is first searched for the macro name v, and if  */
/* found, s is inserted as the new value.  If v is not      */
/* found, a new VARBLOCK is allocated, initialized, the     */
/* values of v and s inserted, and the new block is added   */
/* to the front of the firstvar list.                       */
/************************************************************/
{
    register VARBLOCK p;

    p = srchvar(v);              /* look for v in firstvar */
    if(p == 0)                   /* if not found */
    {
	p = varptr(v);           /* allocate new block for v with NULL value */
    }
    s = s?s:Nullstr;         /* set value of s - what ? */
    if (p->noreset == NO)
    {
	if(IS_ON(EXPORT))
	    p->envflg = YES;
	p->varval = s;
	if(IS_ON(INARGS) || IS_ON(ENVOVER))
	    p->noreset = YES;
	else
	    p->noreset = NO;
        if(IS_ON(DBUG))printf("setvar: %s = %s noreset = %d envflg = %d Mflags = 0%o\n", v, p->varval, p->noreset, p->envflg, Mflags);

	if(p->used && !amatch(v, "[@*<?!%]") )
	    if(IS_ON(DBUG))
		fprintf(stderr, "Warning: %s changed after being used\n",v);
    }
}


eqsign(a)
register CHARSTAR a;
/************************************************************************/
/* Checks for certain characters (:;=$\n\t) in the input string a and   */
/* if any are found, calls callyacc on a.  Returns YES if callyacc is   */
/* called and NO otherwise.  This is a loaded call, because it can      */
/* result in the invocation of the parser and the insertion into one of */
/* the global variables another new value for a dependency line, a      */
/* shell line, a macro definition, a reference to a macro, or any input */
/* line ending in a EOL or a tab.  This function is called when         */
/* reading the argument list to the make invocation and when reading    */
/* into firstvar the environment variables and values from environ.  It */
/* IS NOT used to parse any part of any input makefile.                 */
/* Although it can feed lines with a number of characters to the parse  */
/* in the actual make code, it only gets called on = lines              */
/************************************************************************/
{
    register CHARSTAR p;

    for(p = (unsigned char *) ":;=$\n\t"; *p; p++)
	if(any(a, *p))
	{
	    callyacc(a);
	    return(YES);
	}
    return(NO);
}


VARBLOCK
varptr(v)
register CHARSTAR v;
/**********************************************************/
/* This function determines if the macro name v is in the */
/* global variable firstvar, and if it is, returns a      */
/* pointer to the VARBLOCK which contains the name.   If  */
/* v is not present in firstvar, a new VARBLOCK is allo-  */
/* cated, initialized, v inserted, and a value of the     */
/* null string inserted also.  A pointer to this new      */
/* VARBLOCK is returned.                                  */
/**********************************************************/
{
    register VARBLOCK vp;

    if((vp = srchvar(v)) != 0)
	return(vp);

    vp = ALLOC(varblock);
    vp->nextvar = firstvar;
    firstvar = vp;
    vp->varname = copys(v);
    vp->varval = Nullstr;
    return(vp);
}


VARBLOCK
srchvar(vname)
register CHARSTAR vname;
/********************************************************/
/* Looks for a macro name vname in the global linked    */
/* list firstvar.  If it is found, a pointer to that    */
/* VARBLOCK is returned.  Otherwise, a NO is returned   */
/********************************************************/
{
    register VARBLOCK vp;

    for(vp=firstvar; vp != 0 ; vp = vp->nextvar)
	if(equal(vname, vp->varname))
	    return(vp);

    return(NO);
}


/*VARARGS*/
fatal1(s,t1,t2,t3)
CHARSTAR s;
{
    unsigned char buf[100];

    sprintf(buf, s, t1,t2,t3);
    fatal(buf);
}


fatal(s)
CHARSTAR s;
/*******************************************************************/
/* the VFORK changes here make sure that fatal calls the correct   */
/* exit routine depending on if a vfork or a fork is done.         */
/* _exit will not flush the stdio buffers, preserving the parent's */
/* buffer information                                              */
/*******************************************************************/
{
#ifdef VFORK
    extern int child_of_vfork;
#endif
    if(s)
	fprintf(stderr, "Make: %s.  Stop.\n", s);
    else
	fprintf(stderr, "\nStop.\n");
#ifdef unix
#ifdef VFORK
    if (child_of_vfork)
        _exit(1);
    else
	exit(1);
#else
	exit(1);
#endif
#endif
#ifdef gcos
    exit(0);
#endif
}



yyerror(s)
CHARSTAR s;
/******************************************************/
/* Error printing routine for use by the yacc parse   */
/******************************************************/
{
    unsigned char buf[50];
    extern int yylineno;

    sprintf(buf, "line %d: %s", yylineno, s);
    fatal(buf);
}



CHAIN
appendq(head,tail)
register CHAIN head;
register CHARSTAR tail;
/*****************************************************/
/* As yet, a mystery.  Maybe has something to do     */
/* with internal macro translation for $? constructs */
/* This is a simple append to the end of a queue     */
/*****************************************************/
{
    register CHAIN p;

    p = ALLOC(chain);
    p->datap = tail;
    while(head->nextchain)
	head = head->nextchain;
    head->nextchain = p;
}


CHARSTAR
mkqlist(p)
register CHAIN p;
/*****************************************************/
/* More queue manipulations.  Don't know what the    */
/* queue is for                                      */
/*****************************************************/
{
    register CHARSTAR qbufp, s;
    static unsigned char qbuf[OUTMAX];

    qbufp = qbuf;

    for( ; p ; p = p->nextchain)
    {
	s = p->datap;
	if(qbufp != qbuf)
	    *qbufp++ = BLANK;
	if(qbufp+strlen(s) > &qbuf[OUTMAX-3])
	{
	    fprintf(stderr, "$? list too long\n");
	    break;
	}
	while (*s)
	    *qbufp++ = *s++;
    }
    *qbufp = CNULL;
    return(qbuf);
}

sindex(s1,s2)
CHARSTAR s1;
CHARSTAR s2;
/*************************************************************/
/* Search for substring s2 in s1.  If the substring is found */
/* in s1, the flag value returned will equal the location in */
/* the string where the first character of the match occured.*/
/* If the substring is not found, a -1 flag value is returned*/
/* This routine terminates after one match is found - it does*/
/* not look for more than one occurence of the substring s2  */
/* in s1.                                                    */ 
/*************************************************************/
{
    register CHARSTAR p1;   /* pointer into string being searched */
    register CHARSTAR p2;   /* pointer to string pattern being searched for */
                            /* incremented only if match is found */ 
    register int flag;
    int ii;                  /* counter for beginning position of match */

    p1 = &s1[0];             /* make local pointers to start of arg strings */
    p2 = &s2[0];
    flag = -1;              
    for(ii = 0; ; ii++)      
    {
	while(*p1 == *p2)     /* this loop is the only place where */ 
	{                     /* character matches are handled     */
	    if(flag < 0) 
		flag = ii;    /* save first spot the two strings matched */ 
	    if(*p1++ == CNULL)  /* increment first pointer */
		return(flag);   /* quit if at end of first string */
	    p2++;               /* increment second pointer and loop */
	}

      /* last match failed - check status and either quit or continue */

	if(*p2 == CNULL)     /* end of pattern is reached */ 
	    return(flag);    /* return location of match - may be no match */

	if(flag >= 0)        /* we are not at the end of the substring     */
                             /* so whatever match we thought we found      */
                             /* was not complete                           */ 
	{
	    flag = -1;     /* reset the flag to nomatch                     */
	    p2 = &s2[0];   /* reset pointer on second string to search again */
	}
	if(*s1++ == CNULL)   /* end of first string with or without match  */  
	    return(flag);    /* it is not clear to me why s1 has to be     */
                             /* incremented along with p1                  */
	p1 = s1;             /* advance string pointer of string we are    */
                             /* searching in for a match                   */ 
    }
}


#include "sys/types.h"
#include "sys/stat.h"

static unsigned char fname[256];

CHARSTAR execat();

CHARSTAR
findfl(name)
register CHARSTAR name;
/********************************************************************/
/* This function is like execvp, but does a path search and finds   */
/* files.  This function is called only by exists() in files.c      */
/********************************************************************/
{
#ifndef PATH
    register CHARSTAR p;
    register VARBLOCK cp;

#else PATH
    register LINEBLOCK pp;
    register DEPBLOCK pathp;
#endif PATH

    if(name[0] == SLASH)
	return(name);
#ifndef PATH
    cp = varptr("VPATH");
    if(*cp->varval == 0)
        p = (unsigned char *) ":";
    else
	p = cp->varval;

    do
    {
	p = execat(p, name, fname);
	if(access(fname, 4) == 0)
	    return(fname);
    } while (p);
#else PATH

    for(pp=pathlist ; pp!=0 ; pp = pp->nextline) 
	for(pathp = pp->depp; pathp!=0; pathp = pathp->nextdep) 
	{
	    if (equal(pathp->depname->namep, ".")) 
		continue;
	    concat(pathp->depname->namep, "/", fname);
	    concat(fname, name, fname);
	    if(access(fname, 4) == 0)
		return(fname);
	}
#endif PATH
    return((CHARSTAR )-1);
}

CHARSTAR
execat(s1, s2, si)
register CHARSTAR s1, s2;
CHARSTAR si;
{
    register CHARSTAR s;
#ifdef NLS16
    int last_status = ONEBYTE;  /* NLS: set preceding byte to single */
#endif

    s = si;
#ifndef NLS16
    while (*s1 && *s1 != KOLON)
	*s++ = *s1++;
#else
    while (*s1 && ((*s1 != KOLON) || last_status == SECOF2))
    {
	*s++ = *s1++;
	last_status = BYTE_STATUS(*s1, last_status);  /* update */
    }
#endif
    if (si != s)
	*s++ = SLASH;
    while (*s2)
	*s++ = *s2++;
    *s = CNULL;
    return(*s1? ++s1: 0);
}


CHARSTAR
trysccs(str)
register CHARSTAR str;
/***************************************************************/
/* Translate filenames, simple or complete pathnames, to sccs  */
/* file names, by appending an s. to the front of the filename */
/***************************************************************/
{
    register CHARSTAR sstr;
    register int i = 2;
#ifdef NLS16
    register int j = 0;
    register int last_byte = 0;
    int byte_type[128];  		/* NLS: holds status of char in str */

    for (j = 0; j < 128; j++)
	byte_type[j] = 0;	/* NLS: initialize to ONEBYTE */
#endif

    sstr = str;
#ifndef NLS16
    for(; *str; str++);
#else
    for(j=0; *str; str++)
    {
	last_byte = BYTE_STATUS(*str, last_byte);
	if (last_byte == SECOF2)
	    byte_type[j] = SECOF2;  /* if multi-byte char found parse, */
				    /* map it to array for later use   */
	j++;
    }

    j--;                                    /* decrement to match 'str' */
#endif
    str[2] = CNULL;
    str--;
    for(;str >= sstr; str--)
    {
#ifndef NLS16
	if(*str == SLASH)
#else
	if ((*str == SLASH) && !byte_type[j])	/* NLS: check array to */
			                        /* verify single byte  */
#endif
	    if(i == 2)
	    {
		i = 0;
		*(str+2) = DOT;
		*(str+1) = 's';
	    }
	*(str+i) = *str;
#ifdef NLS16
	j--;
#endif
    }
    if(i == 2)
    {
	*(str+2) = DOT;
	*(str+1) = 's';
    }
    return(sstr);
}


is_sccs(filename)
register CHARSTAR filename;
{
    register CHARSTAR p;
#ifdef NLS16
    char last_char = '\0';    /* NLS: keep track of last legit. char */

    for (p = filename; *p; ADVANCE(p)) {
#else
    for(p = filename; *p; p++)
#endif
	if(*p == 's')
	    if(p == filename && p[1] == DOT)
		return(YES);
#ifndef NLS16
	    else
		if(p[-1] == SLASH && p[1] == DOT)
#else
	    else
		if(last_char == SLASH && p[1] == DOT)
			/* NLS: guarantees single byte char if it's = '/'*/
#endif
		    return(YES);
#ifdef NLS16
    last_char = *p;      /* NLS: assign char for next round */
}
#endif
    return(NO);
}


CHARSTAR
sdot(p)
register unsigned char *p;
{
    register unsigned char *ps = p;

#ifdef NLS16
    register int last_byte = '\0';

    for(; *p; ADVANCE(p))
    {
#else
    for(; *p; p++)
#endif
	if(*p == 's')
	    if(p == ps && p[1] == (unsigned char ) DOT)
		return(p);
#ifndef NLS16
	    else
		if(p[-1] == SLASH && p[1] == DOT)
#else
	    else
		if( last_byte == SLASH && p[1] == DOT)
#endif
		    return(p);
#ifdef NLS16
    }
#endif
    return((unsigned char *)0);
}


CHARSTAR
addstars(pfx)
register CHARSTAR pfx;
/**********************************************************/
/* Change pfx to /xxx/yy/*zz.* or *zz.*                   */
/* This is called only once in doname.c                   */
/**********************************************************/
{
    register CHARSTAR p1, p2;

#ifdef NLS16
    register int j =0;
    register int last_byte =0;
    int byte_type[128];		/* NLS: holds status of chars in ptr */

    for (j=0; j < 128; j++)
	byte_type[j] = 0;	/* NLS: init to ONEBYTE before process */

    j = 0;
    for (p1 = pfx; *p1; p1++)
    {
	last_byte = BYTE_STATUS( *p1, last_byte);
	if (last_byte == SECOF2)
	    byte_type[j] = SECOF2;    /* NLS: if byte is the 2nd of two, */
                                      /* mark corresponding array element*/
                                      /* as such                         */
	j++;
    }
    j--;
#else
    for(p1 = pfx; *p1; p1++);
#endif
    p2 = p1 + 3;	/* 3 characters, '*', '.', and '*'. */
    p1--;

    *p2-- = CNULL;
    *p2-- = STAR;
    *p2-- = DOT;
    while(p1 >= pfx)
    {
#ifndef NLS16
	if(*p1 == SLASH)
#else
	if(*p1 == SLASH && byte_type[j] != SECOF2)
			/* NLS: check array for char size */
#endif
	{
	    *p2 = STAR;
	    return(pfx);
	}
	*p2-- = *p1--;
#ifdef NLS16
	j--;
#endif
    }
    *p2 = STAR;
    return(p2);
}


#define NENV	300
extern CHARSTAR *environ;


setenv()
/*************************************************/
/* This routine is called just before an exec    */
/*************************************************/
{
    register CHARSTAR *ea;
    register int nenv = 0;
    register CHARSTAR p;
    CHARSTAR *es;
    VARBLOCK vp;
    int length;

    if(firstvar == 0)
	return;

    es=ea=(CHARSTAR *)calloc(NENV, sizeof *ea);
    if(es == (CHARSTAR *) 0)
	fatal("Cannot alloc mem for envp.");

    for(vp=firstvar; vp != 0; vp=vp->nextvar)
	if(vp->envflg)
	{
	    if(++nenv >= NENV)
		fatal("Too many env parameters.");
	    length = strlen(vp->varname) + strlen(vp->varval) + 2;
	    if((*ea = (CHARSTAR )calloc(length, sizeof **ea)) == (CHARSTAR ) 0)
		fatal("Cannot alloc mem for env.");
	    p = copstr(*ea++, vp->varname);
	    p = copstr(p, "=");
	    p = copstr(p, vp->varval);
	}
    *ea = 0;
    if(nenv > 0)
	environ=es;
    if(IS_ON(DBUG))
	printf("nenv = %d\n", nenv);
}

#ifdef VFORK
/***********************************************************************/
/* This is a kludge to undo the side effects of setenv after a vfork . */
/* It is used after a child returns to the parent of a vfork,          */
/* in doexec and dosys.                                                */
/***********************************************************************/
unsetenv()
{
    register i;
    
    if (environ == (CHARSTAR *) 0)
	return;			/* let's not free twice */

    for(i=0;i<NENV;i++) {
	if (environ[i] != (CHARSTAR) 0)
	    (void) free (environ[i]);
    }
    free(environ);
    environ = (CHARSTAR *) 0;
}
#endif VFORK


readenv()
/********************************************************************/
/* Called in main.  This function reads all = lines from the        */
/* environment from which make was invoked and puts these           */
/* macro definitions into the firstvar linked list.  This function  */
/* used to ignore null environment variables (VAR=) but with the    */
/* P1003.2/D9+ draft of POSIX, this behavior was changed.  Since    */
/* present day shells all now have an unset capability, make no     */
/* longer needs to ignore null variables (that was the only way to  */
/* remove them from the environment historically).  There are       */
/* situations when a null environment variable may be desireable    */
/* in a makefile.                                                   */
/********************************************************************/
{
    register CHARSTAR *ea;
    register CHARSTAR p;

    ea=environ;
    for(;*ea; ea++)
    {
	/* If SHELL env. variable, don't process, per POSIX.2 */
	if((strncmp(*ea,"SHELL",5)) == 0)
	    continue;
#ifndef NLS16
	for(p = *ea; *p && *p != EQUALS; p++);
#else
	for(p = *ea; *p && *p != EQUALS; ADVANCE(p));
#endif
	if(*p == EQUALS)
/*	    if(*(p+1)) */   /* null variables OK now - remove this test */
		eqsign(*ea);    /* put variable values into firstvar */
    }
}


sccstrip(pstr)
register CHARSTAR pstr;
{
    register CHARSTAR p2;

    register CHARSTAR sstr;
#ifdef NLS16				    /* NLS: */
    register int last_stat = ONEBYTE;   /* holds status of char */
    register int prev_stat = ONEBYTE;   /* holds status of prev char */   
    register int last_stat2 = ONEBYTE;  /* holds status for inner loop */
#endif

    sstr = pstr;
    for(; *pstr ; pstr++)
#ifdef NLS16
    {
	prev_stat = last_stat; 		/* save status of last char */
	last_stat = BYTE_STATUS( *pstr, last_stat);
					/* get current status */
	if(*pstr == RCURLY && last_stat == ONEBYTE)
#else
	if(*pstr == RCURLY)
#endif
	{
	    if(isdigit(pstr[1]))
		if(pstr != sstr)
#ifndef NLS16
		    if(pstr[-1] != DOLLAR)
#else
		    if(pstr[-1] != DOLLAR && prev_stat == ONEBYTE)
#endif
		    {
#ifndef NLS16
			for(p2 = pstr; *p2 && (*p2 != LCURLY); p2++);
#else
			last_stat2 = last_stat; /* start inner loop */
			p2 = pstr;
			while(*p2 && (*p2 != LCURLY || last_stat2 == SECOF2)) 
			{
			    p2++;
			    last_stat2 = BYTE_STATUS( *p2, last_stat2);
			}
#endif

			if(*p2 == CNULL)
			    break;
			strshift(pstr, -(int)(p2-pstr+1) );
		    }
	}
#ifdef NLS16
    }   /*  NLS addition */
#endif
}

 
CHARSTAR
strshift(pstr, count)
register CHARSTAR pstr;
register int count;
/***********************************************************/
/* Shift a string `pstr' count places. Negative is left,   */
/* pos is right.  Negative shifts cause char's at front to */
/* be lost.  Positive shifts assume enough space!          */
/* This function is called in do_colon and sccstrip        */
/***********************************************************/
{
    register CHARSTAR sstr;

    sstr = pstr;
    if(count < 0)
    {
	for(count = -count; *pstr=pstr[count]; pstr++);
	*pstr = 0;
	return(sstr);
    }
    for(; *pstr; pstr++);
        do
	{
	    pstr[count] = *pstr;
	} while (pstr--, count--);

    return(sstr);

}


#include <errno.h>
#ifndef NULL
#define	NULL	0
#endif

CHARSTAR execat();
extern	errno;

execlp(name, argv)
CHARSTAR name, argv;
/*******************************************************************/
/*	execlp(name, arg,...,0)	(like execl, but does path search) */
/*******************************************************************/
{
    return(execvp(name, &argv));
}

execvp(name, argv)
CHARSTAR name, *argv;
/*******************************************************************/
/*	execvp(name, argv)	(like execv, but does path search) */
/*******************************************************************/
{
    register etxtbsy = 1;
    register eacces = 0;
    register CHARSTAR cp;
    CHARSTAR pathstr;
    CHARSTAR shell;
    unsigned char fname[256];

    pathstr = varptr("PATH")->varval;
    if(pathstr == 0 || *pathstr == CNULL)
	pathstr = (unsigned char *) ":/bin:/usr/bin";
    shell = varptr("SHELL")->varval;
    if(shell == 0 || *shell == CNULL)
	shell = (unsigned char *) "/bin/sh";
    cp = any(name, SLASH)? (unsigned char *) "": pathstr;

    do
    {
	cp = execat(cp, name, fname);
retry:
	execv(fname, argv);
	switch(errno)
	{
	    case ENOEXEC:
			*argv = fname;
			*--argv = (unsigned char *) "sh";
			execv(shell, argv);
			return(-1);
	    case ETXTBSY:
			if (++etxtbsy > 5)
			    return(-1);
			sleep(etxtbsy);
			goto retry;
	    case EACCES:
			eacces++;
			break;
	    case ENOMEM:
	    case E2BIG:
			return(-1);
	}
    } while (cp);

    if (eacces)
	errno = EACCES;
    return(-1);
}




/*********************************************************************/
/* The gothead and gotf structures are used to remember the names of */
/* the files `make' automatically gets so `make' can remove them     */
/* upon exit.                                                        */
/*********************************************************************/

GOTHEAD gotfiles;

get(ssfile, cdflag, rlse)
register CHARSTAR ssfile;
int cdflag;
CHARSTAR rlse;
/*********************************************************************/
/* get() does an SCCS get on the file ssfile.  For the get command,  */
/* get() uses the value of the variable "GET".  If ssfile has a      */ 
/* slash in it, get() does a "chdir" to the appropriate directory if */
/* the cdflag is set to CD.  This assures the program finds the      */
/* ssfile where it belongs when necessary.  If the rlse string       */
/* variable is set, get() uses it in the get command sequence.  Thus */
/* a possible sequence is:                                           */
/*                                                                   */
/* 	set -x;                                                      */
/*      cd ../sys/head;                                              */
/*  	get -r2.3.4.5 s.stdio.h                                      */
/*                                                                   */
/*********************************************************************/
{
    register CHARSTAR pr;
    register CHARSTAR pr1;
    unsigned char gbuf[256];
    unsigned char sfile[256];
    int retval;
    GOTF gf;

#ifdef NLS16
    register int j = 0;
    register int last_stat = 0;
    int byte_type[128];

    for (j=0; j < 128; j++)   /* NLS: init string that will hold char status */
	byte_type[j] = 0;
#endif

    copstr(sfile, ssfile);
    if(!sdot(sfile))
	trysccs(sfile);
    if(access(sfile, 4) != 0 && IS_OFF(GET))
	return(NO);

    pr = gbuf;
    if(IS_OFF(SIL))
	pr = copstr(pr, "set -x;\n");

    if(cdflag == CD)
	if(any(sfile, SLASH))
	{
	    pr = copstr(pr, "cd ");

#ifndef NLS16
	    for(pr1 = sfile; *pr1; pr1++);
#else
	    for (pr1 = sfile; *pr1; pr1++) 
	    {
		last_stat = BYTE_STATUS( *pr1, last_stat);
		if (last_stat == SECOF2)
		    byte_type[j] = SECOF2;
		j++;
	    }
#endif 

#ifndef NLS16
	    while(*pr1 != SLASH)
		pr1--;
#else
	    while (*pr1 != SLASH || byte_type[j] == SECOF2)
	    {
		pr1--;
		j--;
	    }
#endif

	    *pr1 = CNULL;
	    pr = copstr(pr, sfile);
	    pr = copstr(pr, ";\n");
	    *pr1 = SLASH;
	}

    pr = copstr(pr, varptr("GET")->varval);
    pr = copstr(pr, " ");
    pr = copstr(pr, varptr("GFLAGS")->varval);
    pr = copstr(pr, " ");

    pr1 = rlse;
    if(pr1 != NULL && pr1[0] != CNULL)
    {
#ifndef NLS16
	if(pr1[0] != MINUS)	/* RELEASE doesn't have '-r' */
#else
	last_stat = ONEBYTE;
	last_stat = BYTE_STATUS(*pr1, last_stat);
	if(pr1[0] != MINUS || last_stat == SECOF2)
                                 /* RELEASE doesn't have '-r' */
#endif
	    pr = copstr(pr, "-r");
	pr = copstr(pr, pr1);
	pr = copstr(pr, " ");
    }

    pr = copstr(pr, sfile);

/* exit codes are opposite of error codes so do the following: */

    retval = (system(gbuf) == 0) ? YES : NO ;
    if(retval == YES)
    {
	if(gotfiles == 0)
	{
	    gotfiles = ALLOC(gothead);
	    gf = (GOTF)gotfiles;
	    gotfiles->gnextp = 0;
	    gotfiles->endp = (GOTF)gotfiles;
	}
	else
	{
	    gf = gotfiles->endp;
	    gf->gnextp = ALLOC(gotf);
	    gf = gf->gnextp;
	    gf->gnextp = 0;
	}
	gf->gnamep = copys(sfile+2);	/* `+2' skips `s.' */
	gotfiles->endp = gf;
    }
    return(retval);
}

rm_gots()
/******************************************************/
/* This function actually removes the gotten files.   */
/******************************************************/
{
    register GOTF gf;

    if(IS_ON(GF_KEEP))
	return;
    for(gf = (GOTF)gotfiles; gf ; gf=gf->gnextp)
	if(gf->gnamep)
	{
	    if(IS_ON(DBUG))
		printf("rm_got: %s\n", gf->gnamep);
	    unlink(gf->gnamep);
	}
}


callyacc(str)
register CHARSTAR str;
/*******************************************************/
/* Calls the parser on lines containing = found on the */
/* command line.  Sets up a global variable linesptr   */
/* which the parser knows to look in for input when    */
/* the normal input file pointer is null.              */
/*******************************************************/
{
    CHARSTAR lines[2];
    FILE *finsave;
    CHARSTAR *lpsave;

    finsave = fin;          /* save any open input file pointer       */
    lpsave = linesptr;      /* save any leftover input in linesptr    */
    fin = 0;                /* null out input file pointer for parser */
    lines[0] = str;         /* put string to be parsed into lines     */
    lines[1] = 0;
    linesptr = lines;       /* set up linesptr for parser             */
    yyparse();              /* call the parse routine to parse line   */

    fin = finsave;          /* after successful parse, restore input file */
    linesptr = lpsave;      /* and old linesptr information           */
}

exit(arg)
/****************************************************************/
/* Exit routine for removing the files `make' automatically got */
/****************************************************************/
{
    rm_gots();
    if(IS_ON(MEMMAP))
    {
	prtmem();
    }
    _cleanup();
    _exit(arg);
}
