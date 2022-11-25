/* @(#) $Revision: 64.1 $ */     
/**********************************************************************


	       WARNING!!!!!!!!!!!!!!

	       This version which uses memallc for memory is limited
	       to MAXSEGSIZE for how much memory you can allocate
	       with it.
**********************************************************************/
#ifndef NLS
#define catgets(i,sn,mn, s) (s)
#else NLS
#include <nl_types.h>
nl_catd nlmsg_fd;
#define NL_SETN 1	/* set number */
#endif NLS

#ifndef NONLS
typedef unsigned short int 	CHAR;	/* create new type to hold expanded characters */
#else
typedef char CHAR;			/* remain char for 7 bit processing */
#endif

#ifdef hp9000s500
#define MAXSEGSIZE 500000
#endif

#include "sh.local.h"

#ifdef hp9000s500
#include <sys/ems.h>
#endif

#ifdef debug2
#define ASSERT(p) if(!(p))botch("p");else
botch(s)
char *s;
{
	printf((catgets(nlmsg_fd,NL_SETN,1, "assertion botched: %s\n")),s);
/*	abort(); */
	return(0);
}
#else
#define ASSERT(p)
#endif

/*	avoid break bug */
#ifdef pdp11
#define GRANULE 64
#else
#define GRANULE 0
#endif
/*	C storage allocator
 *	circular first-fit strategy
 *	works with noncontiguous, but monotonically linked, arena
 *	each block is preceded by a ptr to the (pointer of) 
 *	the next following block
 *	blocks are exact number of words long 
 *	aligned to the data type requirements of ALIGN
 *	pointers to blocks must have BUSY bit 0
 *	bit in ptr is 1 for busy, 0 for idle
 *	gaps in arena are merely noted as busy blocks
 *	last block of arena (pointed to by alloct) is empty and
 *	has a pointer to first
 *	idle blocks are coalesced during space search
 *
 *	a different implementation may need to redefine
 *	ALIGN, NALIGN, BLOCK, BUSY, INT
 *	where INT is integer type to which a pointer can be cast
*/
#define INT int                    
#define ALIGN int
#define NALIGN 1
#define WORD sizeof(union store)
#define BLOCK 1024	/* a multiple of WORD*/
#define BUSY 1
#define NULL 0
#define testbusy(p) ((INT)(p)&BUSY)
#define setbusy(p) (union store *)((INT)(p)|BUSY)
#define clearbusy(p) (union store *)((INT)(p)&~BUSY)

union store { union store *ptr;
	      ALIGN dummy[NALIGN];
	      int calloc;	/*calloc clears an array of integers*/
};

#ifdef hp9000s500
union store *allocs;	/*initial arena*/
union store *alloct;    /*arena top*/
#else
union store allocs[2];
union store *alloct;    /*arena top*/
#endif

union store *allocp;	/*search ptr*/
union store *allocx;	/*for benefit of realloc*/

#ifdef hp9000s500
static	union store *NXTADR;
static  int CURLEN;
char	*memallc();
extern errno;
#endif

#ifndef hp9000s500
extern char *sbrk();
#endif

char *
malloc(nbytes)
unsigned nbytes;
{
	register union store *p, *q;
	register nw;
	static temp;	/*coroutines assume no auto*/

#ifdef hp9000s500
	int stat,sleepcnt;
#endif

#ifdef hp9000s500
	if(allocs==0)  
	{		/*first time*/
	    	sleepcnt=0;
		/* now allocate the overall segment which is MAXSEGSIZE long but
	   	   only use the first 2 words to start things up */
	    	while((p=(union store *)memallc(-1,0,2*WORD,MAXSEGSIZE,MEM_PAGED|MEM_PRIVATE,MEM_R|MEM_W))== -1)
	    	{
			if (sleepcnt++>4)
			{
			    printf((catgets(nlmsg_fd,NL_SETN,2, "failed initial alloc err=%d\n")),errno);
			    return(NULL);
			}
			else
			    sleep(2);
	    	}
		CURLEN=2*WORD;
		NXTADR=p + 2*WORD;
		q=p+WORD;
		allocs=p;
		p->ptr = setbusy(q);
		q->ptr = setbusy(p);
		alloct = q;
		allocp = p;
	}
#else
	if (allocs[0].ptr == 0)
	{
		allocs[0].ptr = setbusy(&allocs[1]);
		allocs[1].ptr = setbusy(&allocs[0]);
		alloct = &allocs[1];
		allocp = &allocs[0];
	}
#endif

	nw = (nbytes+WORD+WORD-1)/WORD;
	ASSERT(allocp>=allocs && allocp<=alloct);
	ASSERT(allock());
	for(p=allocp; ; ) {
		for(temp=0; ; ) {
			if(!testbusy(p->ptr)) {

				while(!testbusy((q=p->ptr)->ptr)) {
					ASSERT(q>p&&q<alloct);
					p->ptr = q->ptr;
				}
				if(q>=p+nw && p+nw>=p)
					goto found;
			}
			q = p;
			p = clearbusy(p->ptr);
			if(p>q)
				ASSERT(p<=alloct);
			else if(q!=alloct || p!=allocs) {
				ASSERT(q==alloct&&p==allocs);
				return(NULL);
			} else if(++temp>1)
				break;
		}
		temp = ((nw+BLOCK/WORD)/(BLOCK/WORD))*(BLOCK/WORD);
#ifdef hp9000s500
		CURLEN += temp*WORD;
		stat = memvary(allocs,CURLEN);
		if(stat == -1) {
		printf((catgets(nlmsg_fd,NL_SETN,3, "failed alloc of temp:%d WORD:%derr=%d\n")),temp,WORD,errno);
			return(NULL);
		}
		q = NXTADR;
		NXTADR += temp;
#else
		q = (union store *)sbrk(0);
		if (q+temp+GRANULE < q) 
			return(NULL);
		q = (union store *)sbrk(temp*WORD);
		if ((INT)q == -1)
			return(NULL);
#endif
		ASSERT(q>alloct);
		alloct->ptr = q;
		if(q!=alloct+1)
			alloct->ptr = setbusy(alloct->ptr);
		alloct = q->ptr = q+temp-1;
		alloct->ptr = setbusy(allocs);
	}
found:
	allocp = p + nw;
	ASSERT(allocp<=alloct);
	if(q>allocp) {
		allocx = allocp->ptr;
		allocp->ptr = p->ptr;
	}
	p->ptr = setbusy(allocp);
	return((char *)(p+1));
}

/*	freeing strategy tuned for LIFO allocation
*/
free(ap)
register CHAR *ap;
{
	register union store *p = (union store *)ap;
	union store *q;

#ifdef hp9000s500
	ASSERT(p>allocs && p<=alloct);
	if(!(p>allocs && p<=alloct))return;
#else
	ASSERT(p>allocs[1].ptr && p <=alloct);
#endif
	ASSERT(allock());
	q = --p;
 	ASSERT(testbusy(p->ptr));
	if (!(testbusy(p->ptr))) return;
	allocp = q;
	p->ptr = clearbusy(p->ptr);
	ASSERT(p->ptr > allocp && p->ptr <= alloct);
}

/*	realloc(p, nbytes) reallocates a block obtained from malloc()
 *	and freed since last call of malloc()
 *	to have new size nbytes, and old content
 *	returns new location, or 0 on failure
*/

CHAR *
realloc(p, nbytes)
register union store *p;
unsigned nbytes;
{
	register union store *q;
	union store *s, *t;
	register unsigned nw;
	unsigned onw;

	if(testbusy(p[-1].ptr))
		free((CHAR *)p);
	onw = p[-1].ptr - p;
	q = (union store *)malloc(nbytes);
	if(q==NULL || q==p)
		return((CHAR *)q);
	s = p;
	t = q;
	nw = (nbytes+WORD-1)/WORD;
	if(nw<onw)
		onw = nw;
	while(onw--!=0)
#ifdef	V6
		copy(t++, s++, sizeof (*t));
#else
		*t++ = *s++;
#endif
	if(q<p && q+nw>=p)
		(q+(q+nw-p))->ptr = allocx;
	return((CHAR *)q);
}

#ifdef debug
allock()
{
#ifdef longdebug
	register union store *p;
	int x;
	x = 0;
	for(p= &allocs[0]; clearbusy(p->ptr) > p; p=clearbusy(p->ptr)) {
		if(p==allocp)
			x++;
	}
	ASSERT(p==alloct);
	return(x==1|p==allocp);
#else
	return(1);
#endif
}

showall(v)
	CHAR **v;
{
	register union store *p, *q;
	int used = 0, free = 0, i;

	for (p = clearbusy(allocs->ptr); p != alloct; p = q) {
		q = clearbusy(p->ptr);
		if (v[1])
		printf("%x %5d %s\n", p,
		    ((unsigned) q - (unsigned) p),
		    testbusy(p->ptr) ? "BUSY" : "FREE");
		i = ((unsigned) q - (unsigned) p);
		if (testbusy(p->ptr)) used += i; else free += i;
	}
	printf("%d used, %d free %d total, %x end\n", used, free, used+free, clearbusy(alloct));
}
#endif
