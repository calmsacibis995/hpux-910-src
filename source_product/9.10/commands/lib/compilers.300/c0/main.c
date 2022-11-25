/*	SCCS	REV(47.9);	DATE(90/05/16	13:31:38) */
/* KLEENIX_ID @(#)main.c	47.9 90/03/30 */

#include "mfile2"
#include "c0.h"
#include "commonb"

long 	defaultlines = MAXLINES;    /* default threshold # lines for inlining */
long 	dlines;			/* threshold # lines for inlining */

long 	maxlabels = MAXLABELS;	/* max labels in "labarray" */
long 	*labarray;		/* array for collecting labels */
long	nxtfreelab;		/* next free slot in "labarray" */

pPD	firstpd;		/* first proc descriptor in chain */
pPD	lastpd;			/* last proc descriptor in chain */
pPD	currpd;			/* current proc descriptor */

long	maxpnpbytes = MAXPNPBYTES;	/* max size of permanent name pool */
long	maxpnp;			/* offset of end of perm name pool */
char	*pnp;			/* permanent name pool */
long	freepnp;		/* offset of next free byte in pnp */

long	maxtnpbytes = MAXPNPBYTES;	/* max size of temporary name pool */
char	*maxtnp;		/* end of temp name pool */
char	*tnp;			/* temp name pool */
char	*freetnp;		/* next free byte in tnp */

long	maxipnpbytes = MAXPNPBYTES;	/* max size of ip name pool */
char	*maxipnp;		/* end of ip name pool */
char	*ipnp;			/* ip name pool */
char	*freeipnp;		/* next free byte in ipnp */

FILE * ifd;			/* input file -- all reads use this fd */
FILE * ofd;			/* output file -- all writes use this fd */
FILE * isrcfd;			/* input file -- from pass1 */
FILE * ithunkfd;		/* input file -- from pass1 */
FILE * opermfd;			/* final output file */
FILE * otmpfd;			/* tmp output file for "no emit" procs */
FILE * ipermfd;			/* fd for input from opermfd */
FILE * itmpfd;			/* fd for input from otmpfd */
FILE * oasfd;			/* fd for .s file output (append) */

char	infilename[MAXSTRBYTES];	/* name of input file */
char	outfilename[MAXSTRBYTES];	/* name of output file */
char	asfilename[MAXSTRBYTES];	/* name of .s file */

char	strbuf[MAXTEXTBYTES+1]; /* a temporary name building buffer */

long	hash[HASHSIZE];		/* hash table for pnp symbols */

long	callrectopnp;		/* 1 == call recs point to pnp, not pd */

long farg_high = 0, farg_low = 0, farg_pos = 0;

#ifdef DEBUGGING
/* DEBUGGING FLAGS */
long xdebugflag;
#endif DEBUGGING
long verboseflag;
long pdumpflag;
long hdumpflag;
long cfdumpflag;
long ctdumpflag;
long ldumpflag;
char caseflag;			/* -1 == uppercase; 0 == lowercase; */
				/*  1 == mixed case */
char noemitseen;		/* 1 == "no emit" procedure seen; 0 == not */
char assignflag;		/* 1 == ASSIGN seen */
char nowarnflag;		/* 1 == don't emit warnings */
char ftnflag;			/* YES == FORTRAN; NO == C */

char warnbuff[500];		/* buffer for warning message */

long	nforced;	/* current # forced procs in array */
long	nomitted;	/* current # omitted procs in array */
long	maxforced;	/* size of array */
long	maxomitted;	/* size of array */
long	*forceoffsets;	/* pnpoffsets of current forced procs */
long	*omitoffsets;	/* pnpoffsets of current omitted procs */

/*****************************************************************************/

/* initlab -- initialize labels array */
initlab()
{
    if ((labarray = (long *) malloc(maxlabels*sizeof(long))) == NULL)
	fatal ("Out of memory in initlab()");
    nxtfreelab = 0;	/* next free slot is the first one */
}  /* initlab */

/*****************************************************************************/

/* realloclab -- reallocate labels array */
realloclab(requiredsize)
long requiredsize;	/* minimum required # of labels in new array */
{
    maxlabels += MAXLABELS;
    if (maxlabels <= requiredsize)
	maxlabels = requiredsize + MAXLABELS;
    if ((labarray = (long *) realloc(labarray,maxlabels*sizeof(long))) == NULL)
	fatal ("Out of memory in realloclab()");
}  /* realloclab */

/*****************************************************************************/

/* initpnp -- initialize permanent name pool */
/* Only procedure names collected during pass1 are entered in the pnp */
initpnp()
{
    if ((pnp = (char *) malloc(maxpnpbytes)) == NULL)
	{
	fatal ("can't initialize permanent name pool");
	}
    else
	{
	freepnp = 4;
	*((long *) pnp) = 0;	/* first word of pnp is 0 */
	maxpnp = maxpnpbytes;
	}
}  /* initpnp */

/*****************************************************************************/

/* addtopnp -- add procedure to pnp, checking for overflow */
long addtopnp(s,nwords)
char *s;	/* name of procedure */
long nwords;	/* # 4-byte words in procedure name */
{
long offset;	/* offset of name record when all done */
register long i;
register unsigned long hashval;	 /* hash value of procedure name */
register pNameRec np;	/* pnp name record */
long recordsize = sizeof(NameRec) + nwords * 4;

    /* make sure only nulls after first null in name -- up to next 4-byte
     * boundary.  Do this because all pnp name comparisons are done 4 bytes
     * at a time.  Hash values are calculated by added 4-byte of the name
     * at a time.
     */
    {
    register char *p;
    register char *pend;

    pend = s + nwords*4;
    for (p = pend - 4; *p; p++)
	;
    for (p++; p < pend; p++)
	*p = '\0';
    }

    /* hash name */
    {
    register unsigned long *p = (unsigned long *) s;
    hashval = 0;
    for (i = nwords; i > 0; p++, i--)
	hashval += *p;
    hashval %= HASHSIZE;
    }

    /* search hash bucket chain for matching procedure name */
    offset = hash[hashval];	/* pnp offset was stored in hash table */
    while (offset)	/* search entire hash bucket chain in pnp */
	{
	np = (pNameRec) (pnp + offset);
	if ((np->nlongs == nwords) && !memcmp(np->name,s,nwords*4))
	    /* found -- return offset */
	    return (offset);
	else
	    /* check next in chain */
	    offset = np->onext;
	}

    /* not already in table -- add it */
    if ((freepnp + recordsize) >= maxpnp)  /* pnp full --
							    * expand it */
	{
	maxpnpbytes += MAXPNPBYTES;
	if ((pnp = (char *) realloc(pnp,maxpnpbytes)) == NULL)
	    fatal ("out of space -- permanent name pool");
	maxpnp = maxpnpbytes;
	}
    offset = freepnp;
    freepnp += recordsize;
    np = (pNameRec) (pnp + offset);
    memset(np,'\0',recordsize); /* zero out record */
    np->onext = hash[hashval];
    hash[hashval] = offset;	/* add this record to front of hash bucket */
    np->nlongs = nwords;
    memcpy(np->name,s,nwords*4);
    return(offset);
}  /* addtopnp */

/*****************************************************************************/


/* inittnp -- initialize temporary name pool */
/* Names collected during pass2 (caller procs) are entered into the tnp.
 * The tnp is cleared after every expression tree.
 */
inittnp()
{
    if ((tnp = (char *) malloc(maxtnpbytes)) == NULL)
	{
	fatal ("can't initialize temporary name pool");
	}
    else
	{
	freetnp = tnp;			/* free space starts at beginning */
	maxtnp = tnp + maxtnpbytes;	/* end of table pointer */
	}
}  /* inittnp */

/*****************************************************************************/

/* addtotnp -- add string to tnp, checking for overflow */
char *addtotnp(s,words)
char *s;	/* string to be added */
long words;	/* # 4-byte words in string */
{
    long length = words*4;	/* byte length of string */
    char *p;			/* pointer to added string in pnp */

    if ((freetnp + length + 1) >= maxtnp)  /* overflow -- should never happen */
	{
	fatal ("out of space -- temporary name pool");
	return (NULL);
	}
    else
	{
	strcpy(freetnp, s);
	p = freetnp;
	freetnp += length + 4;	/* advance free space pointer */
	return (p);
	}
}  /* addtotnp */

/*****************************************************************************/

/* initipnp -- initialize ip name pool */
/* The integrated-procedure name pool contains names encountered during
 * doip() -- callee procedure being integrated.  It is cleared after
 * every expression tree in the callee procedure.
 */
initipnp()
{
    if ((ipnp = (char *) malloc(maxipnpbytes)) == NULL)
	{
	fatal ("can't initialize ip name pool");
	}
    else
	{
	freeipnp = ipnp;		/* free space starts at beginning */
	maxipnp = ipnp + maxipnpbytes;  /* end of free space pointer */
	}
}  /* initipnp */

/*****************************************************************************/

/* addtoipnp -- add string to ipnp, checking for overflow */
char *addtoipnp(s,words)
char *s;	/* string to be added */
long words;	/* # 4-byte words in string */
{
    long length = words * 4;  /* byte length of string */
    char *p;		      /* pointer to added string in ipnp */

    if ((freeipnp + length + 1) >= maxipnp)  /* table full -- should never happen */
	{
	fatal ("out of space -- ip name pool");
	return (NULL);
	}
    else
	{
	strcpy(freeipnp, s);
	p = freeipnp;
	freeipnp += length + 4;	  /* advance free space pointer */
	return (p);
	}
}  /* addtoipnp */

/*****************************************************************************/

/* initpd -- initialize procedure descriptor linked list */
initpd ()
{
    firstpd = NULL;
    lastpd = NULL;
    currpd = NULL;
}

/*****************************************************************************/

/* getpd -- allocate and return a procedure descriptor entry */
pPD getpd (nparms)
long nparms;		/* number of parameters */
{
    pPD p;	/* allocated procedure descriptor to be returned */

    if (nparms > MAXARGS)	/* too many formal parameters */
	    p = (pPD) calloc(1, sizeof(PD));	/* don't allocate parm space */
    else
	    p = (pPD) calloc(1, sizeof(PD)+(nparms-1)*(sizeof(TWORD)));
    if (p == NULL)
	fatal ("can't allocate pd entry");
    return (p);
}  /* getpd */

/*****************************************************************************/

/* addpd -- add pd entry to pd chain */
addpd(p)
pPD p;  /* pd entry to be added */
{
    if (lastpd == NULL)		/* nothing in chain yet -- set start pointer */
	firstpd = p;
    else			/* chain exists -- add to end */
        lastpd->next = p;
    lastpd = p;
}  /* addpd */

/*****************************************************************************/

/* dumppd -- dump pd entry */
dumppd(p)
pPD p;
{
register long i;
printf ("\n0x%.8x\n",p);
printf ("    %-20s0x%.8x\n",	"pnpoffset",p->pnpoffset);
printf ("    %-20s%d\n",	"namewords",((pNameRec)(pnp+(p->pnpoffset)))->nlongs);
printf ("    %-20s\"%*s\"\n",    "name",((pNameRec)(pnp+(p->pnpoffset)))->nlongs,
				     ((pNameRec)(pnp+(p->pnpoffset)))->name);
printf ("    %-20s0x%.8x\n",	"inoffset",p->inoffset);
printf ("    %-20s0x%.8x\n",	"outoffset",p->outoffset);
printf ("    %-20s0x%.8x\n",	"outroffset",p->outroffset);
printf ("    %-20s%d\n",	"nlines",p->nlines);
printf ("    %-20s0x%.8x\n",	"callsto",p->callsto);
printf ("    %-20s0x%.8x\n",	"callsfrom",p->callsfrom);
printf ("    %-20s%d\n",	"stackuse",p->stackuse);
printf ("    %-20s%d\n",	"regsused",p->regsused);
printf ("    %-20s%d\n",	"nlab",p->nlab);
printf ("    %-20s0x%.8x\n",	"labels",p->labels);
printf ("    %-20s0x%.8x\n",	"retoffset",p->retoffset);
printf ("    %-20s%d\n",	"dimbytes",p->dimbytes);
printf ("    %-20s0x%.8x\n",	"next",p->next);
printf ("    %-20s0x%.8x\n",	"nextout",p->nextout);
printf ("    %-20s%d\n",	"integorder",p->integorder);
printf ("    %-20s%d\n",	"typeproc",p->typeproc);
printf ("    %-20s%d\n",	"inlineOK",p->inlineOK);
printf ("    %-20s%d\n",	"noemitflag",p->noemitflag);
printf ("    %-20s%d\n",	"hasaltrets",p->hasaltrets);
printf ("    %-20s%d\n",	"imbeddedAsm",p->imbeddedAsm);
printf ("    %-20s%d\n",	"rettype",p->rettype);
printf ("    %-20s%d\t",	"nparms",p->nparms);
if (p->nparms <= MAXARGS)
    for (i=0 ; i < p->nparms ; i++)
        printf("  %.8x", p->parmtype[i]);
printf ("\n");
}  /* dumppd */

/*****************************************************************************/

/* dumppdchain -- dump entire pd chain */
dumppdchain()
{
pPD p = firstpd;

    while (p != NULL)
	{
	dumppd(p);
	p = p->next;
	}
}  /* dumppdchain */

/*****************************************************************************/

/* dumppoutchain -- dump pd chain in file output order */
dumppoutchain()
{
pPD p = firstoutpd;

    while (p != NULL)
	{
	dumppd(p);
	p = p->nextout;
	}
}  /* dumppoutchain */

/*****************************************************************************/

/* dumplabels -- dump labels array */
dumplabels(p,max,n)
long *p;	/* labels array */
long max;	/* offset of start of "new" labels in array */
long n;		/* number of "old" labels in array */
{
    long i;
    printf("\n");
    for (i=0; i<n; i++)
	printf("oldlabel = %d\t\tnewlabel = %d\n", p[i], p[i+max]);
}  /* dumplabels */

/*****************************************************************************/

/* dumphash -- dump hash table and name records in pnp */
dumphash()
{
    register long i;
    register long offset;
    for (i = 0; i < HASHSIZE; i++)
	if (offset = hash[i])		/* there is a pnp offset here */
	    {
	    printf("\nhash[%d]:\n",i);
	    while (offset)		/* dump entire hash bucket chain */
		{
		dumpname(offset);
		offset = ((pNameRec)(pnp+offset))->onext;
		}
	    }
}  /* dumphash */

/*****************************************************************************/

/* dumpname -- dump single name record in pnp */
dumpname(offset)
long offset;	/* pnp offset of name record */
{
    register pNameRec p = (pNameRec)(pnp + offset);
    printf("    0x%.8x:  onext=0x%.8x, pd=0x%.8x, ffrom=0x%.8x,\n                      nlongs=%d, name=\"%.*s\"\n",
	offset, p->onext, p->pd, p->ffrom, p->nlongs, p->nlongs*4,  p->name);
}  /* dumpname */

/*****************************************************************************/

/* dumpct -- dump call graph ("to" ordering) */
dumpct()
{
    register pPD p = firstpd;	/* current procedure */
    register pCallRec ct;	/* current call graph node */
    register pNameRec np;	/* pnp name record of called procedure */

    while (p)	/* step through all procedures for which we have source */
	{
        if (ct = p->callsto)
	    {
	    np = (pNameRec) (pnp + p->pnpoffset);
	    printf("%.*s:\n", np->nlongs*4, np->name);
	    while (ct)
		{
		dumpcr(ct);
		ct = ct->nextto;
		}
	    }
	p = p->next;
	}
}  /* dumpct */

/*****************************************************************************/

/* dumpcf -- dump call graph ("from" ordering) */
dumpcf()
{

/* During pass1 the "from" pointers to the callgraph are from the pnp name
 * records, rather than from the pd entries.  This is changed during the
 * between-pass call graph cleanup.
 */
if (callrectopnp)  /* in pass1 -- "from" pointers in pnp name records */
    {
    register long i;	    /* hash table index */
    register long offset;   /* offset of pnp name record */
    register pNameRec np;   /* pnp name record */
    register pCallRec cf;   /* current call graph node */

    for (i = 0; i < HASHSIZE; i++)	/* step thru hash table */
	if (offset = hash[i])	    /* there is a pnp offset value here */
	    while (offset)	    /* step thru hash bucket chain */
		{
		np = (pNameRec) (pnp + offset);
		if (cf = np->ffrom)	/* there are call to this proc */
		    {
		    printf("%.*s:\n", np->nlongs*4, np->name);
		    while (cf)		/* step thru "from" chain */
			{
			dumpcr(cf);
			cf = cf->nextfrom;
			}
		    }
		offset = np->onext;
		}
    }
else		/* "from" chain head is in the pd record */
    {
    register pPD p = firstpd;	/* current proc descriptor */
    register pCallRec ct;	/* current call graph node */
    register pNameRec np;	/* pnp name record */

    while (p)		/* step thru all procedures */
	{
        if (ct = p->callsfrom)	/* there are calls to this procedure */
	    {
	    np = (pNameRec) (pnp + p->pnpoffset);
	    printf("%.*s:\n", np->nlongs*4, np->name);
	    while (ct)		/* step thru calling chain */
		{
		dumpcr(ct);
		ct = ct->nextfrom;
		}
	    }
	p = p->next;
	}
    }
}  /* dumpcf */

/*****************************************************************************/

/* dumpcr -- dump call graph node */
dumpcr(cr)
register pCallRec cr;	/* call graph node to be dumped */
{
    pPD pd = cr->calledfrom;
    pNameRec np;

    if (pd)
	{
        np = (pNameRec) (pnp + pd->pnpoffset);
        printf("    0x%.8x:  calledfrom=0x%.8x (\"%.*s\"), nextfrom=0x%.8x\n",
	       cr, pd, np->nlongs*4, np->name, cr->nextfrom);
	}
    else
	{
        printf("    0x%.8x:  calledfrom=0x00000000 (\"\"), nextfrom=0x%.8x\n",
	       cr, cr->nextfrom);
	}
    if (cr->callsto)
	{
	if (callrectopnp)
	    np = (pNameRec) (pnp + (long)(cr->callsto));
	else
	    np = (pNameRec) (pnp + (cr->callsto)->pnpoffset);
        printf("                   callsto=0x%.8x (\"%.*s\"), nextto=0x%.8x\n",
	       cr->callsto, np->nlongs*4, np->name, cr->nextto);
	}
    else
	{
        printf("                   callsto=0x00000000 (\"\"), nextto=0x%.8x\n",
	       cr->nextto);
	}
    printf("                   count=%d, forced=%d, clines=%d\n",
	   cr->count, cr->forced, cr->clines);
}  /* dumpcr */

/*****************************************************************************/

/*****************************************************************************/

/* rdlong - reads 1 long while checking for a read error */

long rdlong()
{
    static long x;
    if( fread( (char *) &x, 4, 1, ifd ) <= 0 )
	fatal( "intermediate file read error" );
    return( x );
}  /* rdlong */

/*****************************************************************************/

/* fileopen -- open file */
FILE * fileopen (s,type)
char *s;	/* file name */
char *type;	/* type of fopen call */
{
  	FILE *fd;		/* result file descriptor */
	char buff[MAXSTRBYTES];	/* error message buffer */

	fd = fopen( s, type );
	if( fd == NULL )
	    {
	    sprintf(buff,"cannot open intermediate file %s", s );
	    fatal(buff);
	    }
	return(fd);
}  /* fileopen */

/*****************************************************************************/

/* rdnlongs - reads n longs into a buffer strbuf. Bytes are later read as
	chars.
*/

rdnlongs( n )
long n;	  /* number of longs to read */
{
    register char *cp;

    if( n > 0 )
	{
	if (n > (MAXTEXTBYTES / sizeof(long)))
	    fatal("string too long in rdnlongs()" );
	if( fread( strbuf, 4, n, ifd ) != n )
	    fatal( "intermediate file read error" );
	/* until f77pass1 puts out well formed asciz names, the
	   following is needed to remove blank pads.
	*/
/*	for (cp = strbuf; *cp && *cp != ' '; cp++) ;
/*	*cp = '\0';
*/
	}
}  /* rdnlongs */

/*****************************************************************************/

#define SYMLONGS (MAXSTRBYTES+1)/sizeof(long)

/* rdstring reads a series of longs from ifd until reading a null char. Used
   primarily to read arbitrary length names.
*/

long rdstring()
{
	register short 	i, j = 1;
	register char 	*lcp;
	register long	*cp = (long *)strbuf;

	while (j++ < SYMLONGS)
		{
		lcp = (char *) cp;
		if ( fread(cp++, sizeof(long), 1, ifd) != 1 )
			fatal("intermediate file read error");
		for (i=sizeof(long); i>0; i--)
			if (! *lcp++) 
				return(--j); /* return at first null byte */
		}
	return(--j);
}  /* rdstring */

/*****************************************************************************/

/* FORCE/OMIT status is recorded in two ways:
 *    a) The pnp offsets of forced/omitted procedures are kept in a global
 *	    list (allocated in initforceomit() ).  This makes it easy to
 *	    store the global state by merely making copies of the current
 *	    arrays.  (Beats checking every proc)
 *    b) There are force/omit flags in each procedures pnp record.  This
 *	    makes it easy to check for a given procedure -- don't have to
 *	    search the global arrays.
 */

/* initforceomit -- allocate current force/omit arrays */
initforceomit()
{
    maxforced = NOFFSETS;	/* max size */
    maxomitted = NOFFSETS;	/* max size */
    if ((forceoffsets = (long *) malloc(maxforced * sizeof(long))) == NULL)
	fatal("can't initialize forced procs table");
    if ((omitoffsets = (long *) malloc(maxomitted * sizeof(long))) == NULL)
	fatal("can't initialize omitted procs table");
    nforced = 0;	/* no forced procs yet */
    nomitted = 0;	/* no omitted procs yet */
}  /* initforceomit */

/*****************************************************************************/

/* addforceoffset -- add pnpoffset of forced proc to offset array */
addforceoffset(offset)
long offset;	/* pnp offset of forced proc to be added */
{
    if (nforced >= maxforced)	/* array full -- autoexpand */
	{
	maxforced += NOFFSETS;
	if ((forceoffsets =
		(long *) realloc(forceoffsets, maxforced * sizeof(long)))
	    == NULL)
	    fatal("out of space -- force offset array");
	}
    forceoffsets[nforced++] = offset;
}  /* addforceoffset */

/*****************************************************************************/

/* addomitoffset -- add pnpoffset of omitted proc to offset array */
addomitoffset(offset)
long offset;	/* pnp offset of omitted proc to be added */
{
    if (nomitted >= maxomitted)	/* array full -- autoexpand */
	{
	maxomitted += NOFFSETS;
	if ((omitoffsets =
		(long *) realloc(omitoffsets, maxomitted * sizeof(long)))
	    == NULL)
	    fatal("out of space -- omit offset array");
	}
    omitoffsets[nomitted++] = offset;
}  /* addomitoffset */

/*****************************************************************************/

/* rmforceoffset -- remove pnp offset from force offset array */
rmforceoffset(offset)
register long offset;	/* pnp offset to be removed */
{
    register long *p = forceoffsets;			/* array start */
    register long *pend = forceoffsets + nforced;	/* array end */
    while (p < pend)	/* search entire force'd list */
	{
	if (*p == offset)	/* found it!! */
	    {
	    if (nforced > 1)	/* replace this deletion with last */
		*p = forceoffsets[--nforced];
	    else
		--nforced;	/* this was only one in last */
	    break;
	    }
	p++;
	}
}  /* rmforceoffset */

/*****************************************************************************/

/* rmomitoffset -- remove pnpoffset from omit offset array */
rmomitoffset(offset)
register long offset;	/* pnp offset to be removed */
{
    register long *p = omitoffsets;			/* list start */
    register long *pend = omitoffsets + nomitted;	/* list end */
    while (p < pend)	/* search entire list to find match */
	{
	if (*p == offset)	/* found it !!! */
	    {
	    if (nomitted > 1)	/* replace deletion with last item in list */
		*p = omitoffsets[--nomitted];
	    else		/* this was only offset in list */
		--nomitted;
	    break;
	    }
	p++;
	}
}  /* rmomitoffset */

/*****************************************************************************/

/* saveforceomitoffsets -- allocate array to save current force/omit info */
saveforceomitoffsets( pnforced, pnomitted, pforceoffsets, pomitoffsets, pdlines)
long *pnforced;		/* pointer to result nforced storage */
long *pnomitted;	/* pointer to result nomitted storage */
long **pforceoffsets;	/* pointer to result forced offsets array */
long **pomitoffsets;	/* pointer to result omitted offsets array */
long *pdlines;		/* pointer to result dlines storage */
{
    long totalitems = nforced + nomitted;	/* size of malloc'd array */

    *pnforced = nforced;	/* save counts */
    *pnomitted = nomitted;
    if (totalitems != 0)	/* don't care about pointers if count == 0 */
	{
	/* to save time, make only one malloc call to allocate force/omit
	 * vector.  Forced offsets are first, followed by omitted 
	 */
	if ((*pforceoffsets = (long *) malloc( totalitems * sizeof(long))) ==
		NULL)
	    fatal("out of space -- save force/omit procedures");
	*pomitoffsets = *pforceoffsets + nforced;
	if (nforced != 0)
	    memcpy(*pforceoffsets, forceoffsets, nforced*sizeof(long));
	if (nomitted != 0)
	    memcpy(*pomitoffsets, omitoffsets, nomitted*sizeof(long));
	}
    *pdlines = dlines;	/* save threshold # lines also */
}  /* saveforceomitoffsets */

/*****************************************************************************/

/* restoreforceomitoffsets -- restore saved info to "current" */
restoreforceomitoffsets( snforced, snomitted, sforceoffsets, somitoffsets, sdlines )
long snforced;		/* saved nforced value */
long snomitted;		/* saved nomitted value */
long *sforceoffsets;	/* saved forced offsets array */
long *somitoffsets;	/* saved omitted offsets array */
long sdlines;		/* saved dlines value */
{
    register long *p;
    register long *pend;

    /* first -- unmark all current force/omit procedures */
    for (p = forceoffsets, pend = forceoffsets + nforced; p < pend; p++)
	{
	((pNameRec)(pnp + *p))->forceflag = NO;
	}
    for (p = omitoffsets, pend = omitoffsets + nomitted; p < pend; p++)
	{
	((pNameRec)(pnp + *p))->omitflag = NO;
	}

    /* copy in new information and mark pnp entries */
    nforced = snforced;
    nomitted = snomitted;
    if (nforced != 0)
	{
	memcpy(forceoffsets, sforceoffsets, nforced*sizeof(long));
        for (p = forceoffsets, pend = forceoffsets + nforced; p < pend; p++)
	    {
	    ((pNameRec)(pnp + *p))->forceflag = YES;
	    }
	}
    if (nomitted != 0)
	{
	memcpy(omitoffsets, somitoffsets, nomitted*sizeof(long));
        for (p = omitoffsets, pend = omitoffsets + nomitted; p < pend; p++)
	    {
	    ((pNameRec)(pnp + *p))->omitflag = YES;
	    }
	}
    dlines = sdlines;	/* restore threshold # executable lines */
}  /* restoreforceomitoffsets */

/*****************************************************************************/

main( argc, argv )
long argc;
char *argv[];
{
	initpnp();		/* initialize permanent name pool */
	initforceomit();	/* initialize force/omit proc tables */
	getoptions(argc, argv); /* process all command-line options */

	/* open file descriptors for input and output */
	isrcfd = fileopen(infilename, "r");
	ithunkfd = fileopen(infilename, "r");
	ifd = isrcfd;		/* current input file is pass1 output */
	if (ftnflag)
		oasfd = fileopen(asfilename, "a");
	ofd = fileopen(outfilename,"w");
	puttriple(FMAXLAB,0,0);     /* blank template to be filled in later */
	putlong(0);                 /* (max label generated) */

	callrectopnp = 1;	/* call records point to pnp */
	mkdope();		/* initialize OP code info tables */
	pass1();		/* read entire file, collect procedure info */

	if (pdumpflag)
	    dumppdchain();	/* dump procedure info */
	if (hdumpflag)
	    dumphash();		/* dump hash table */
	if (cfdumpflag)
	    dumpcf();		/* dump call graph ("from" ordering) */
	if (ctdumpflag)
	    dumpct();		/* dump call graph ("to" ordering) */

	cleancg();		/* clean up call graph - remove "don't cares" */
	callrectopnp = 0;		/* call records now point to pd */

	if (pdumpflag)
	    dumppdchain();	/* dump procedure info */
	if (hdumpflag)
	    dumphash();		/* dump hash table */
	if (cfdumpflag)
	    dumpcf();		/* dump call graph ("from" ordering) */
	if (ctdumpflag)
	    dumpct();		/* dump call graph ("to" ordering) */

	order();		/* determine integration order */

	if (pdumpflag)
	    dumppoutchain();	/* dump procedure info */

	freecallgraph();	/* free call graph space */

	pass2();		/* do second pass, integrating calls */
	exit(0);
}

/*****************************************************************************/

/* getoptions -- process all input options */
getoptions(argc,argv)
register long argc;
register char *argv[];
{
    register char *s;
    char *endnum;
    long oargc;			/* saved value of argc for start second pass */
    register char **oargv;	/* saved value of argv for start second pass */

    /* skip program invocation name */
    oargc = --argc;
    oargv = ++argv;

    /* I make 2 passes over the arguments.  The first does everything except
     * for force/omit/nocode lists.  In particular, the case option is
     * detected.  The second pass does only the force/omit/nocode lists,
     * applying the case option appropriately.
     */

    /* first pass over arguments */
    while (*argv && (**argv == '-'))	/* process all that start with '-' */
	{
	for (s = *argv + 1; *s; s++)	/* allow 'clumped' syntax */
	    {
	    switch (*s) {
	    case 'd':			/* dump internal data structures */
		switch (*++s) {
		    case 'f':
			cfdumpflag = YES;
			break;
		    case 't':
			ctdumpflag = YES;
			break;
		    case 'h':
			hdumpflag = YES;
			break;
		    case 'l':
			ldumpflag = YES;
			break;
		    case 'p':
			pdumpflag = YES;
			break;
		    default:
			fatal("bad suboption for -d");
		}
		break;

#ifdef DEBUGGING
	    case 'D':	/* echo input tree records as they are read */
		if (*++s == 'x')
		    xdebugflag = YES;
		else
		    fatal("bad suboption for -D");
		break;
#endif DEBUGGING
		
	    case 'f':			/* force routines inline */
		if (! *++s)		/* list is in next argument */
		    {
		    argc--;
		    argv++;
		    if (!*argv)
			fatal("bad argument to -f");
		    s = *argv;
		    }
		while (*++s) ;		/* skip to end of list */
		--s;
		break;

	    case 'n':			/* don't emit as stand-alone routine */
		if (! *++s)		/* list is in next argument */
		    {
		    argc--;
		    argv++;
		    if (!*argv)
			fatal("bad argument to -n");
		    s = *argv;
		    }
		while (*++s) ;		/* skip to end of list */
		--s;
		break;

	    case 'o':			/* omit routines inline */
		if (! *++s)		/* list in next argument */
		    {
		    argc--;
		    argv++;
		    if (!*argv)
			fatal("bad argument to -o");
		    s = *argv;
		    }
		while (*++s) ;		/* skip to end of list */
		--s;
		break;

	    case 'l':			/* set max integration lines */
		if (*(s+1))		/* arg immediately follows */
		    {
		    defaultlines = strtol(s+1,&endnum,10);
		    if (endnum == (s+1))
			fatal("bad argument to -l");
		    s = endnum - 1;
		    }
		else			/* next argv is it */
		    {
		    if (! *++argv)
			fatal("bad argument to -l");
		    defaultlines = strtol(*argv,&endnum,10);
		    if (endnum == *argv)
			fatal("bad argument to -l");
		    s = endnum - 1;
		    argc--;
		    }
		break;

	    case 'u':			/* external names shifted uppercase */
		caseflag = -1;
		break;

	    case 'U':			/* mixed case external names */
		caseflag = 1;
		break;

	    case 'v':			/* tell what calls are replaced */
		verboseflag = YES;
		break;

	    case 'w':			/* don't emit warnings */
		nowarnflag = YES;
		break;

	    case 'F':
		ftnflag = YES;
		break;

	    default:
		printf("Unrecognized option \"-%c\".  Options are:\n", *s);
#ifdef DEBUGGING
		printf("\t-df\tDump call graph (from)\n");
		printf("\t-dt\tDump call graph (to)\n");
		printf("\t-dh\tDump hash table and names\n");
		printf("\t-dl\tDump statement labels\n");
		printf("\t-dp\tDump procedure descriptor table\n");
		printf("\t-Dx\tDump expression nodes\n");
#endif DEBUGGING
		printf("\t-f rtn1,...\tForce inlining of rtn1,...\n");
		printf("\t-l lines\tSet max lines of target procedure\n");
		printf("\t-n rtn1,...\tDon't emit stand-alone code for rtn1,...\n");
		printf("\t-o rtn1,...\tOmit inlining of rtn1,...\n");
		printf("\t-v\t\tVerbose mode\n");
		printf("\t-w\t\tTurn off warning messages\n");
		exit(1);
	    }  /* switch */
	    }  /* for */
	argv++;
	argc--;
	}  /* while */

    /* Collect the three file names */

    if ((ftnflag && (argc != 3)) || (!ftnflag && (argc != 2)))
	fatal("Incorrect number of file arguments");

    strcpy(infilename,*argv);
    argv++;
    argc--;
    if (ftnflag)
	{
	strcpy(asfilename,*argv);
	argv++;
	argc--;
	}
    strcpy(outfilename,*argv);

    /* process force/omit/noemit options during the second pass */
    while (*oargv && (**oargv == '-'))
	{
	s = *oargv + 1;
	while (s && *s)
	    {
	    switch (*s) {
	    case 'd':			/* ignore these options */
		s = NULL;
		break;
#ifdef DEBUGGING
	    case 'D':
#endif DEBUGGING
	    case 'u':
	    case 'U':
	    case 'v':
	    case 'w':
	    case 'F':
		++s;
		break;

	    case 'l':		/* read # lines to keep proper input pos */
		if (*(s+1))
		    strtol(s+1,&endnum,10);
		else
		    strtol(*++oargv,&endnum,10);
		s = NULL;
		break;

	    case 'f':			/* force list */
		if (*++s)		/* list follows immediately */
		    doforcelist(s,YES);
		else
		    doforcelist(*++oargv,YES);
		s = NULL;
		break;

	    case 'o':			/* omit list */
		if (*++s)		/* list follows immediately */
		    doomitlist(s,YES);
		else
		    doomitlist(*++oargv,YES);
		s = NULL;
		break;

	    case 'n':			/* nocode list */
		if (*++s)		/* list follows immediately */
		    donoemitlist(s);
		else
		    donoemitlist(*++oargv);
		s = NULL;
		break;
	    default:
		fatal("Unexpected option failure");
		break;
	    }  /* switch */
	    }  /* for */
	oargv++;
	}  /* while */	
}  /* getoptions */

/*****************************************************************************/

/* doforcelist -- process list of forced routines */
doforcelist(list,init)
register char *list;		/* list of routines */
long init;			/* 1 == this is initial value */
{
    char buff[256];		/* name collection buffer */
    register char *pbuff;	/* next available spot in buff[] */
    char *endtok;
    long pnpoffset;		/* offset of item in pnp */
    register pNameRec pname;	/* name record in pnp */

    while (*list)		/* do entire list */
	{
	pbuff = buff;

	/* all procedure names in c0 have a preceding underscore to simplify
	 * matching against expression tree input.  Add one if not already
	 * in list.
	 */
	if (*list != '_')	/* no preceding underscore */
	    *pbuff++ = '_';

	endtok = strchr(list,',');  /* find end of token */
	if (endtok == list)		/* leading comma */
	    fatal("bad list for -f/\"$INLINE FORCE\"");
	else if (endtok)
	    *endtok = '\0';	/* terminate this particular token */

	/* transfer name to buffer, shifting case as necessary */
	switch (caseflag) {
	    case -1:		/* uppercase */
		for ( ; *list ; )
		    *pbuff++ = toupper(*list++);
		break;
	    case 0:		/* lowercase */
		for ( ; *list ; )
		    *pbuff++ = tolower(*list++);
		break;
	    case 1:		/* mixed case */
		for ( ; *list ; )
		    *pbuff++ = *list++;
		break;
	    }

	/* null pad -- all pnp names are compared 4 bytes at a time. */
	*pbuff++ = '\0';
	*pbuff++ = '\0';
	*pbuff++ = '\0';
	*pbuff++ = '\0';

	/* add name to permanent name pool */
	pnpoffset = addtopnp(buff,(pbuff - buff) / 4);
	
	pname = (pNameRec) (pnp + pnpoffset);
	if (pname->forceflag == NO)	/* not already forced */
	    {
	    pname->forceflag = YES;	/* now it is */
	    pname->everforced = YES;	/* remember it was once */
	    addforceoffset( pnpoffset);	/* add offset to global force list */
	    if (pname->omitflag)	/* was omitted before */
		if (init)		/* omitted in command-line list */
	            fatal("routine in both 'force' and 'omit' lists");
		else			/* during regular usage -- turn off */
		    {
	    	    pname->omitflag = NO;	/* turn off omit flag */
		    rmomitoffset( pnpoffset);	/* rm offset from omit list */
		    }
	    else if (init)
		pname->initflag = -1;	/* save initialization status */
	    }
	list = endtok ? (endtok + 1) : NULL;
	}  /* while */
}  /* doforcelist */

/*****************************************************************************/

/* doomitlist -- process list of omitted routines */
doomitlist(list,init)
register char *list;		/* list of omitted routines */
long init;			/* 1 == this is initial value */
{
    char buff[256];		/* name collection buffer */
    register char *pbuff;	/* next available spot if buff[] */
    char *endtok;
    long pnpoffset;		/* offset of item in pnp */
    register pNameRec pname;	/* name record in pnp */

    while (*list)		/* process entire list */
	{
	pbuff = buff;

	/* all procedure names in c0 have a preceding underscore to simplify
	 * matching against expression tree input.  Add one if not already
	 * in list.
	 */
	if (*list != '_')	/* no preceding underscore */
	    *pbuff++ = '_';
	endtok = strchr(list,',');  /* find end of token */
	if (endtok == list)				/* leading comma */
	    fatal("bad list for -o/\"$INLINE OMIT\"");
	else if (endtok)
	    *endtok = '\0';	/* terminate this particular token */

	/* transfer name to buffer, shifting case as necessary */
	switch (caseflag) {
	    case -1:		/* uppercase */
		for ( ; *list ; )
		    *pbuff++ = toupper(*list++);
		break;
	    case 0:		/* lowercase */
		for ( ; *list ; )
		    *pbuff++ = tolower(*list++);
		break;
	    case 1:		/* mixed case */
		for ( ; *list ; )
		    *pbuff++ = *list++;
		break;
	    }

	/* null pad -- all pnp names are compared 4 bytes at a time */
	*pbuff++ = '\0';
	*pbuff++ = '\0';
	*pbuff++ = '\0';
	*pbuff++ = '\0';

	/* add name to permanent name pool */
	pnpoffset = addtopnp(buff,(pbuff - buff) / 4);
	
	pname = (pNameRec) (pnp + pnpoffset);
	if (pname->omitflag == NO)	/* not already omitted */
	    {
	    pname->omitflag = 1;	/* now it is */
	    addomitoffset( pnpoffset);	/* add pnp offset to omit list */
	    if (pname->forceflag)	/* previous forced */
		if (init)		/* both in command-line */
	            fatal("routine in both 'force' and 'omit' lists");
		else			/* during regular usage */
		    {
	    	    pname->forceflag = NO;	/* turn off force */
		    rmforceoffset( pnpoffset);	/* rm offset from force list */
		    }
	    else if (init)		/* remember "default" status */
		pname->initflag = 1;
	    }
	list = endtok ? (endtok + 1) : NULL;
	}  /* while */
}  /* doomitlist */

/*****************************************************************************/

/* donoemitlist -- process list of no stand-alone emission routines */
donoemitlist(list)
register char *list;		/* list of "no-code" procedures */
{
    char buff[256];		/* name collection buffer */
    register char *pbuff;	/* next available spot if buff[] */
    char *endtok;
    long pnpoffset;		/* offset of item in pnp */
    register pNameRec pname;	/* name record in pnp */

    noemitseen = 1;		/* set flag that there are no-code procedures */

    while (*list)		/* process entire list */
	{
	pbuff = buff;

	/* all procedure names in c0 have a preceding underscore to simplify
	 * matching against expression tree input.  Add one if not already
	 * in list.
	 */
	if (*list != '_')	/* no preceding underscore */
	    *pbuff++ = '_';
	endtok = strchr(list,',');  /* find end of token */
	if (endtok == list)				/* leading comma */
	    fatal("bad list for -n/\"$INLINE NOCODE\"");
	else if (endtok)
	    *endtok = '\0';	/* terminate this particular token */

	/* transfer name to buffer, shifting case as necessary */
	switch (caseflag) {
	    case -1:		/* uppercase */
		for ( ; *list ; )
		    *pbuff++ = toupper(*list++);
		break;
	    case 0:		/* lowercase */
		for ( ; *list ; )
		    *pbuff++ = tolower(*list++);
		break;
	    case 1:		/* mixed case */
		for ( ; *list ; )
		    *pbuff++ = *list++;
		break;
	    }

	/* null pad -- pnp names are compared 4 bytes at a time */
	*pbuff++ = '\0';
	*pbuff++ = '\0';
	*pbuff++ = '\0';
	*pbuff++ = '\0';

	/* add name to permanent name pool */
	pnpoffset = addtopnp(buff,(pbuff - buff) / 4);
	
	pname = (pNameRec) (pnp + pnpoffset);
	pname->noemitflag = 1;		/* record no-emit status */

	list = endtok ? (endtok + 1) : NULL;
	}  /* while */
}  /* donoemitlist */

/*****************************************************************************/

/* dodefaultlist -- process list of routines to set to default force/omit */
dodefaultlist(list)
register char *list;	/* list of routines */
{

    if (*list)		/* if there is a list */
	{
	char buff[256];		/* name collection buffer */
	register char *pbuff;	/* next available spot if buff[] */
	char *endtok;
	long pnpoffset;		/* offset of item in pnp */
	register pNameRec pname;	/* name record in pnp */

	while (*list)	/* process entire list */
	    {
	    pbuff = buff;

	    /* all procedure names in c0 have a preceding underscore to simplify
	     * matching against expression tree input.  Add one if not already
	     * in list.
	     */
	    if (*list != '_')	/* no preceding underscore */
	        *pbuff++ = '_';

	    endtok = strchr(list,',');  /* find end of token */
	    if (endtok == list)				/* preceding comma */
	        fatal("bad list for \"$INLINE DEFAULT\"");
	    else if (endtok)
	        *endtok = '\0';	/* terminate this particular token */

	    /* transfer name to buffer, shifting case as necessary */
	    switch (caseflag) {
	        case -1:		/* uppercase */
		    for ( ; *list ; )
		        *pbuff++ = toupper(*list++);
		    break;
	        case 0:		/* lowercase */
		    for ( ; *list ; )
		        *pbuff++ = tolower(*list++);
		    break;
	        case 1:		/* mixed case */
		    for ( ; *list ; )
		        *pbuff++ = *list++;
		    break;
	        }

	    /* null pad -- because pnp names are compared 4 bytes at a time */
	    *pbuff++ = '\0';
	    *pbuff++ = '\0';
	    *pbuff++ = '\0';
	    *pbuff++ = '\0';

	    /* add name to permanent name pool */
	    pnpoffset = addtopnp(buff,(pbuff - buff) / 4);
	
	    pname = (pNameRec) (pnp + pnpoffset);

	    /* undo current status */
	    if (pname->forceflag == YES)
		{
	    	pname->forceflag = NO;
		rmforceoffset( pnpoffset);
		}
	    if (pname->omitflag == YES)
		{
	    	pname->omitflag = NO;
		rmomitoffset( pnpoffset);
		}

	    /* set to default status */
	    if (pname->initflag == -1)
		{
		pname->forceflag = YES;
		addforceoffset( pnpoffset);
		}
	    else if (pname->initflag == 1)
		{
		pname->omitflag = YES;
		addomitoffset( pnpoffset);
		}

	    list = endtok ? (endtok + 1) : NULL;
	    }  /* while */
	}
    else	/* no list ==> reset every routine in the symbol table */
	{
	register long i;
	register long pnpoffset;
	register pNameRec pname;

	for (i = 0 ; i < HASHSIZE ; i++)	/* step thru hash table */
	    {
	    if (pnpoffset = hash[i])	/* there is a pnp offset here */
		{
		while (pnpoffset)	/* step thru hash bucket chain */
		    {
		    pname = (pNameRec) (pnp + pnpoffset);

		    /* undo current status */
		    if (pname->forceflag == YES)
			{
			pname->forceflag = NO;
			rmforceoffset( pnpoffset);
			}
		    if (pname->omitflag == YES)
			{
			pname->omitflag = NO;
			rmomitoffset( pnpoffset);
			}

		    /* set to default status */
		    if (pname->initflag == -1)
			{
			pname->forceflag = YES;
			addforceoffset( pnpoffset);
			}
		    else if (pname->initflag == 1)
			{
			pname->omitflag = YES;
			addomitoffset( pnpoffset);
			}
		    pnpoffset = pname->onext;
		    }
		}
	    }
	}
}  /* dodefaultlist */


/*****************************************************************************/

/* fatal -- print error message and die */
	
fatal(s)
char *s;
{
    fprintf(stderr, "c0: %s\n", s);
    exit(1);
}  /* fatal */

/*****************************************************************************/

/* warn -- print warning message */

warn(s)
char *s;
{
    if (! nowarnflag)
	fprintf(stderr,"c0: %s\n", s);
}  /* warn */


/* rstatus is duplicated in local2.c of the backend */

unsigned rstatus[] = {
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG,
	SAREG|STAREG, SAREG|STAREG,

	SBREG|STBREG, SBREG|STBREG,
	SBREG|STBREG, SBREG|STBREG,
	SBREG|STBREG, SBREG|STBREG,
	SBREG,	      SBREG,

# ifndef C1
	STFREG,		STFREG,
	STFREG,		STFREG,
	STFREG,		STFREG,
	STFREG,		STFREG,
# else /* C1 */
	SFREG|STFREG,	SFREG|STFREG,
	SFREG|STFREG,	SFREG|STFREG,
	SFREG|STFREG,	SFREG|STFREG,
	SFREG|STFREG,	SFREG|STFREG,
# endif /* C1 */
#ifndef LCD
# ifndef C1
	STDREG,		STDREG,
	STDREG,		STDREG,
	STDREG,		STDREG,
	STDREG,		STDREG,
	STDREG,		STDREG,
	STDREG,		STDREG,
	STDREG,		STDREG,
	STDREG,		STDREG,
# else /* C1 */
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
	SDREG|STDREG,		SDREG|STDREG,
# endif /* C1 */
#endif /* LCD */
	};


# ifdef DEBUGGING

/* eprint is duplicated in reader.c of the backend */

eprint( p, down, a, b ) NODE *p; int *a, *b; {

	flag fc0flag = NO;

	*a = *b = down+1;
	while( down >= 2 ){
		printf( "\t" );
		down -= 2;
		}
	if( down-- ) printf( "    " );


	printf( "0x%x) %s", p, opst[p->in.op] );
	switch( p->in.op ) { /* special cases */

	case FC0CALL:
	case FC0CALLC:
		printf( "\n" );
		return;

	case REG:
		printf( " %s", rnames[p->tn.rval] );
		break;

	case FC0OREG:
		fc0flag = YES;
		p->in.op = OREG;	/* depends on fall thru */
	case ICON:
	case NAME:
	case OREG:
		printf( " " );
		adrput( p );
		if ( fc0flag )
		  p->in.op = FC0OREG;
		break;

	case STCALL:
	case UNARY STCALL:
	case STARG:
	case STASG:
		printf( " size=0x%x", p->stn.stsize );
		printf( " align=0x%x", p->stn.stalign );
		break;
	}

	printf( ", " );
	tprint( p->in.type, 0 );
	printf("\n");
#if 0
	printf( ", " );
	if( p->in.rall == NOPREF ) printf( "NOPREF" );
	else {
		if( p->in.rall & MUSTDO ) printf( "MUSTDO " );
		else printf( "PREF " );
		printf( "%s", rnames[p->in.rall&~MUSTDO]);
		}
	printf( ", SU= 0x%x, FSU= 0x%x\n", p->in.su, p->in.fsu );
#endif
	}

/* BITMASK is duplicated in local2.c of the backend */

# define BITMASK(n) ((1L<<n)-1)

/* adrput is duplicated in local2.c of the backend */

adrput( p ) register NODE *p; {
#ifndef FLINT
	/* output an address, with offsets, from p */

	if( p->in.op == FLD )
		p = p->in.left;

	switch( p->in.op ){

	case NAME:
		acon( p );
		return;

	case ICON:
		/* addressable value of the constant */
		if( p->in.type == DOUBLE ) {
			/* print the high order value */
			CONSZ save;
			save = p->tn.lval;
			p->tn.lval = ( p->tn.lval >> SZINT ) & BITMASK(SZINT);
			printf( "&" );
			acon( p );
			p->tn.lval = save;
			return;
			}
		printf( "&" );
		acon( p );
		return;

	case REG:
		printf( "%s", rnames[p->tn.rval] );
		return;

	case OREG:
# ifdef R2REGS
		if (p->tn.rval < 0)
			{
			/* It's a (packed) indexed oreg. */
			print_indexed(p);
			return;
			}
# endif	/* 	R2REGS */
		if( p->tn.rval == A6 ){  /* in the argument region */
			if( p->in.name != NULL ) warn( "bad arg temp" );
			printf( CONFMT, p->tn.lval );
			printf( "(%%a6)" );
			return;
			}
		if( p->tn.lval != 0 || p->in.name != NULL )
		  { acon( p ); }
		printf( "(%s)", rnames[p->tn.rval] );
		break;

	case UNARY MUL:
		/* STARNM or STARREG found */
		if( tshape(p, STARNM) ) {
			adrput( p->in.left);
			}
		else {	/* STARREG - really auto inc or dec */
			/* turn into OREG so replacement node will
			   reflect the value of the expression */
			register NODE *q, *l;

			l = p->in.left;
			q = l->in.left;
			p->in.op   = OREG;
			p->in.rall = q->in.rall;
			p->tn.lval = q->tn.lval;
			p->tn.rval = q->tn.rval;
			p->in.name = q->in.name;	/* just copy the ptr */
			if( l->in.op == INCR ) {
				adrput( p );
				printf( "+" );
				p->tn.lval -= l->in.right->tn.lval;
				}
			else {	/* l->in.op == ASG MINUS */
				printf( "-" );
				adrput( p );
				}
		}
		return;

	default:
		fatal( "illegal address" );
		return;

		}
#endif FLINT
	}


/* acon is duplicated in local2.c of the backend */

LOCAL acon( p ) register NODE *p; { /* print out a constant */
#ifndef FLINT
	if( p->in.name == NULL )		/* constant only */
		{
		if (p->in.op == ICON) switch (p->tn.type)
			/* don't mask for OREG lvals */
			{
			/* for signed operands, explicitly extend */
			case CHAR:
				if (p->tn.lval & 0x00000080)
					p->tn.lval |= 0xffffff00;
					break;
			case SHORT:
				if (p->tn.lval & 0x00008000)
					p->tn.lval |= 0xffff0000;
					break;
			}
		printf( CONFMT, p->tn.lval);
		}
	else if( p->tn.lval == 0 ) 	/* name only */
		printf( "%s", p->in.name );
	else 				/* name + offset */
		{
		printf( "%s+", p->in.name );
		printf( CONFMT2, p->tn.lval );
		}
#endif FLINT
	}


/* rnames is duplicated in local2.c of the backend */

char *
rnames[]= {  /* keyed to register number tokens */

	"%d0", "%d1", "%d2", "%d3", "%d4", "%d5", "%d6", "%d7",
	"%a0", "%a1", "%a2", "%a3", "%a4", "%a5", "%a6", "%sp",
	"%fp0", "%fp1", "%fp2", "%fp3", "%fp4", "%fp5", "%fp6", "%fp7"
#ifndef LCD
       ,"%fpa0", "%fpa1", "%fpa2", "%fpa3", "%fpa4", "%fpa5", "%fpa6",
	"%fpa7", "%fpa8", "%fpa9", "%fpa10", "%fpa11", "%fpa12", "%fpa13",
	"%fpa14", "%fpa15"
#endif
	};


/* print_indexed is duplicated in local2.c of the backend */

# ifdef R2REGS
/* print_indexed() prints out an indexed-mode address */
LOCAL print_indexed(p)	register NODE *p;
{
#ifndef FLINT
	    /* NOTE: Not completed for MEMIND_POST/PRE. Current code assumes
	       that od is 0 (not always the case)!
	    */
	    union indexpacker x;
	    int bd;

	    x.rval = p->tn.rval;
	    bd = p->tn.lval;
	    printf(x.i.mode == MEMIND_PRE? "([" :  "(" );
	    if (p->tn.name)
		    {
		    printf("%s", p->tn.name);
		    if (bd) printf("+%d,", bd);
		    else printf(",");
		    }
	    else if (bd) printf("%d,", bd);
	    if (x.i.addressreg)
		    {
		    printf("%s,", rnames[x.i.addressreg]);
		    }
	    else
		    printf("%%za0,");
	    printf(x.i.mode==MEMIND_PRE? "%s.l*%d],0)" : "%s.l*%d)",
			rnames[x.i.xreg], 1<<(x.i.scale) );
	    return;
#endif FLINT
}
# endif	/* R2REGS */

/* mamask is duplicated in match.c of the backend */

static long mamask[] = { /* masks for matching dope with shapes */
	SIMPFLG,		/* OPSIMP */
	SIMPFLG|ASGFLG,		/* ASG OPSIMP */
	COMMFLG,	/* OPCOMM */
	COMMFLG|ASGFLG,	/* ASG OPCOMM */
	MULFLG,		/* OPMUL */
	MULFLG|ASGFLG,	/* ASG OPMUL */
	DIVFLG,		/* OPDIV */
	DIVFLG|ASGFLG,	/* ASG OPDIV */
	UTYPE,		/* OPUNARY */
	TYFLG,		/* ASG OPUNARY is senseless */
	LTYPE,		/* OPLEAF */
	TYFLG,		/* ASG OPLEAF is senseless */
	0,		/* OPANY */
	ASGOPFLG|ASGFLG,	/* ASG OPANY */
	LOGFLG,		/* OPLOG */
	TYFLG,		/* ASG OPLOG is senseless */
	FLOFLG,		/* OPFLOAT */
	FLOFLG|ASGFLG,	/* ASG OPFLOAT */
	SHFFLG,		/* OPSHFT */
	SHFFLG|ASGFLG,	/* ASG OPSHIFT */
	SPFLG,		/* OPLTYPE */
	TYFLG,		/* ASG OPLTYPE is senseless */
	};

/* SPECIALLIST is duplicated in match.c of the backend */

# define SPECIALLIST (SCCON|SICON|S8CON|SZERO|SONE|SMONE)

/* tshape is duplicated in match.c of the backend */

tshape( p, shape ) register NODE *p; register shape; {
	/* return true if shape is appropriate for the node p
	   side effect for SFLD is to set up fldsz,etc */

	register short o = p->in.op;
	register  mask;

	if( shape & SPECIAL )
		{
		if( o != ICON || p->in.name ) ;
		else
		switch( mask = shape & SPECIALLIST ){

		case SZERO:
		case SONE:
		case SMONE:
			if( p->tn.lval == 0 && shape == SZERO ) return(1);
			else if( p->tn.lval == 1 && shape == SONE ) return(1);
			else if( p->tn.lval == -1 && shape == SMONE ) return(1);
			else  break;

		default:
# ifdef MULTILEVEL
			if( mask & MULTILEVEL )
				return( mlmatch(p,mask,0) );
			else
				fatal ("bad special shape");
# endif

		case SCCON :
			if (p->tn.lval >= -128 && p->tn.lval <= 127)
				return(1);
			break;
		case SICON :	/* signed integer const. (16 bits) */
			if (p->tn.lval >= -32768 && p->tn.lval <= 32767)
				return (1);
			break;
		case S8CON :
			if (p->tn.lval >= 1 && p->tn.lval <= 8)
				return (1);
			break;
		}
		if (mask == shape) return (0);

		}

	if( shape & SANY ) return(1);

	if( (shape&INTEMP) && shtemp(p) ) return(1);

# ifndef mc68000
	if( (shape&SWADD) && (o==NAME||o==OREG) ){
		if( BYTEOFF(p->tn.lval) ) return(0);
		}
# endif

	switch( o ){

	case NAME:
		return( shape&SNAME );
	case ICON:
		mask = SCON;
		return( shape & mask );

	case FLD:
		if( shape & SFLD ){
			if( !flshape( p->in.left ) ) return(0);
			/* it is a FIELD shape; make side-effects */
			o = p->tn.rval;
			return(1);
			}
		return(0);

	case CCODES:
		return( shape&SCC );

	case REG:
		/* distinctions:
		SAREG	any scalar register
		STAREG	any temporary scalar register
		SBREG	any lvalue (index) register
		STBREG	any temporary lvalue register
		*/
#ifdef LCD
		mask = ISBREG( p->tn.rval ) ? SBREG : 
# ifndef C1
			ISFREG(p->tn.rval)? STFREG : SAREG;
# else /* C1 */
			ISFREG(p->tn.rval)? SFREG : SAREG;
# endif /* C1 */
		if( ISTREG( p->tn.rval ) && /* busy[p->tn.rval]<=1 */)
			mask |= (mask==SAREG)? STAREG :
# ifndef C1
				(mask==STFREG)? STFREG:STBREG;
# else /* C1 */
				(mask==SFREG)? STFREG:STBREG;
# endif /* C1 */
#else
		mask = ISBREG( p->tn.rval ) ? SBREG : 
# ifndef C1
			ISFREG(p->tn.rval)? STFREG :
			ISDREG(p->tn.rval)? STDREG : SAREG;
# else /* C1 */
			ISFREG(p->tn.rval)? SFREG :
			ISDREG(p->tn.rval)? SDREG : SAREG;
# endif /* C1 */
		if( ISTREG( p->tn.rval ) /* && busy[p->tn.rval]<=1 */)
			mask |= (mask==SAREG)? STAREG :
# ifndef C1
				(mask==STFREG)? STFREG:
				(mask==STDREG)? STDREG:STBREG;
# else /* C1 */
				(mask==SFREG)? STFREG:
				(mask==SDREG)? STDREG:STBREG;
# endif /* C1 */
#endif
		return( shape & mask );

	case OREG:
		return( shape & SOREG );

	case UNARY MUL:
		/* return STARNM or STARREG or 0 */
		return( (shumul(p->in.left) & shape) || 
			(p->in.left->in.op == OREG && (shape & STARNM)) );

		}

	return(0);
	}

/* shtemp is duplicated in local2.c of the backend */

/* shtemp() returns TRUE if p points to a NDOE that is acceptable in lieu
   of a temporary.
*/
shtemp( p ) register NODE *p; {
	if( p->in.op == UNARY MUL ) p = p->in.left;
	if( p->in.op == REG ) return( !ISTREG( p->tn.rval ) );
	return( daleafop(p->in.op) );
	}

/* spsz is duplicated in local2.c of the backend */

spsz( t, v ) TWORD t; CONSZ v; {

	/* is v the size to increment something of type t */

	if( !ISPTR(t) ) return( 0 );
	t = DECREF(t);

	if( ISPTR(t) ) return( v == 4 );

	switch( t ){

	case UCHAR:
	case CHAR:
		return( v == 1 );

	case SHORT:
	case USHORT:
		return( v == 2 );

	case INT:
	case UNSIGNED:
	case FLOAT:
		return( v == 4 );

	case DOUBLE:
		return( v == 8 );
		}

	return( 0 );
	}

/* shumul is duplicated in local2.c of the backend */

shumul( p ) register NODE *p; {

	if( INDEXREG(p) ) return( STARNM );

	if( p->in.op == INCR && INDEXREG(p->in.left) && p->in.right->in.op==ICON
		&& p->in.right->in.name == NULL
		&& spsz( p->in.left->in.type, p->in.right->tn.lval ) )
			return( STARREG );

	return( 0 );
	}

/* flshape is duplicated in local2.c of the backend */

flshape( p ) register NODE *p; {
	if ( daleafop(p->in.op) ) return(1);
	return( p->in.op==UNARY MUL && shumul(p->in.left)==STARNM );
	}



/*****************************************************************************/

/* backps is duplicated in backend.c of the backend */

struct backop {short op; char *oname;} backops[] = 
	{
	FTEXT, "FTEXT",
	FEXPR, "FEXPR",
	FSWITCH, "FSWITCH",
	FLBRAC, "FLBRAC",
	FRBRAC, "FRBRAC",
	FEOF, "FEOF",
	FARIF, "FARIF",
	LABEL, "LABEL",
	SETREGS, "SETREGS",
	FMAXLAB, "FMAXLAB",
	FCMGO, "FCMGO",
	SWTCH, "SWTCH",
#ifdef C1SUPPORT
	ARRAYREF, "ARRAYREF",
	FICONLAB, "FICONLAB",
	C1SYMTAB, "C1SYMTAB",
	VAREXPRFMTDEF, "VFEDEF",
	VAREXPRFMTEND, "VFEEND",
	VAREXPRFMTREF, "VFEREF",
	C1OPTIONS, "C1OPTION",
	C1OREG, "C1OREG",
	C1NAME, "C1NAME",
	STRUCTREF, "STRUCTREF",
	C1HIDDENVARS, "C1HIDDEN",
	C1HVOREG, "C1HVOREG",
	C1HVNAME, "C1HVNAME",
#endif C1SUPPORT
	-1,""
	};

/* xfop is duplicated in backend.c of the backend */

LOCAL char * xfop(o)	register short o;
{
	register short testop;
	register i;

	if (o >= DSIZE)
		{
		for (i = 0; (testop = backops[i].op) >= 0; i++)
			if (o == testop) return(backops[i].oname);
		}
	else
		return (opst[o]);
}
#endif DEBUGGING

/*****************************************************************************/

/* gettype is duplicated in backend.c of the backend and renamed from gettyp */

/* Unpack the compacted type information.  See trees.c for a description of
 * the packing scheme.
 */
#define TYWD1(x)        ((x)&0x80000000)
#define TYWD2(x)        ((x)&0x40000000)

gettype(x,tptr,aptr)		
int x;
long *tptr, *aptr;
{
register int tmp;
	
	*tptr = ((x)&0x0fff0000)>>16; 
	*aptr = ((x)&0x30000000)>>28; 
	if (TYWD1(x)) {  /* at least 1 extension word follows */
		tmp = rdlong(); 
		*tptr |= ((tmp)&0xfffff000); 
		*aptr |= ((tmp)&0xfff)<<6; 
		if (TYWD2(x)) {   /* a second extension word follows */
			tmp = rdlong(); 
			*aptr |= ((tmp)&0xfffc0000);
		}
	} 
}

/* puttype is duplicated in trees.c of the frontend and renamed from puttyp */

puttype(op, val, type, attr)
int op, val, type, attr;
{
register int node;

	node = (op)&0xff | ((val)&0xff)<<8 | ((type)&0xfff)<<16 |((attr)&3)<<28;
	if (!(attr&0xfffc0000)) { /* 0 or 1 extension words */
		if ((!((type)&0xfffff000)) && (!((attr)&0xffffffc0))) {
		    putlong(node);
		}
		else { /* 1 extension word */
		    putlong( 0x80000000 | node );
		    putlong(((type)&0xfffff000)|((attr)&0x3ffc0)>>6);
		}
	}
	else { /* 2 extension words */
		putlong( 0xc0000000 | node );
		putlong(((type)&0xfffff000)|((attr)&0x3ffc0)>>6);
		putlong((attr)&0xfffc0000);
	}
}
