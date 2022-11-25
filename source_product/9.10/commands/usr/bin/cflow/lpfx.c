static char *HPUX_ID = "@(#) $Revision: 70.2 $";
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "lerror.h"
#include "manifest"
#include "lmanifest"
#include "lpass2.h"
#ifdef APEX
#include "apex.h"
#endif

typedef struct {
	union rec r;
	char *fname;
	} funct;

typedef struct LI {
	struct LI *next;
	funct fun;
	} li;

/*
 * lpfx - read lint1 output, sort and format for dag
 *
 *	options -i_ -ix (inclusion)
 *
 *	while (read lint1 output into "funct" structures)
 *		if (this is a filename record)
 *			save filename
 *		else
 *			read arg records and throw on floor
 *			if (this is to be included)
 *				copy filename into "funct"
 *				insert into list
 *	format and print
 */


#ifndef APEX

main(argc, argv)
	int argc;
	char **argv;
	{
	extern int optind;
	extern char *optarg;
	funct fu;
	int uscore, fmask, c;
	void rdargs(), insert(), putout();
	char *filename, *funcname, *getstr();

	fmask = LDS | LDX | LRV;
	uscore = 0;
	while ((c = getopt(argc, argv, "i:")) != EOF)
		if (c == 'i')
			if (*optarg == '_')
				uscore = 1;
			else if (*optarg == 'x')
				fmask &= ~(LDS | LDX);
			else
				goto argerr;
		else
		argerr:
			(void)fprintf(stderr, "lpfx: bad option %c ignored\n", c);

	while ((0 < fread((char *)&fu.r.l.decflag, sizeof(fu.r.l.decflag), 1, stdin)) &&
	       (0 < fread((char *)&fu.r.l.name, sizeof(fu.r.l.name), 1, stdin)) &&
	       (0 < fread((char *)&fu.r.l.nargs, sizeof(fu.r.l.nargs), 1, stdin)) &&
	       (0 < fread((char *)&fu.r.l.fline, sizeof(fu.r.l.fline), 1, stdin)) &&
	       (0 < fread((char *)&fu.r.l.type.aty, sizeof(fu.r.l.type.aty), 1, stdin)) &&
	       (0 < fread((char *)&fu.r.l.type.extra, sizeof(fu.r.l.type.extra), 1, stdin)))
	       {

		if (fu.r.l.decflag & LFN)
			{
			filename = fu.r.f.fn = getstr();
			}
		else
			{
			funcname = fu.r.l.name = getstr();
			rdargs(&fu);
			if (((fmask & LDS) ? ISFTN(fu.r.l.type.aty) :
			      !(fu.r.l.decflag & fmask)) &&
			    ((uscore) ? 1 : (*fu.r.l.name != '_')))
				{
				fu.fname = filename;
				insert(&fu);
				}
			}
		}
	putout();
	}



/* getstr - get strings from intermediate file
 *
 * simple while loop reading into static buffer
 * transfer into malloc'ed buffer
 * panic and die if format or malloc error
 *
 */

char *getstr()
	{
	static char buf[BUFSIZ];
	char *malloc(), *strcat(), *strcpy();
	register int c;
	register char *p = buf;

	while ((c = getchar()) != EOF)
		{
		*p++ = c;
		if (c == '\0' || !isascii(c))
			break;
		}
	if (c != '\0')
		{
		fputs("lpfx: PANIC! Intermediate file string format error\n",
		    stderr);
		exit(1);
		/*NOTREACHED*/
		}
	if (!(p = malloc(strlen(buf) + 1)))
		{
		fputs("lpfx: out of heap space\n", stderr);
		exit(1);
		/*NOTREACHED*/
		}
	return (strcpy(p, buf));
	}

/*
 * rdargs - read arg records and throw on floor
 *
 *	if ("funct" has args)
 *		get absolute value of nargs
 *		if (too many args)
 *			panic and die
 *		read args into temp array
 */

void rdargs(pf)
	register funct *pf;
	{
	struct ty atype[50];
	int i;


	if (pf->r.l.nargs)
		{
		if (pf->r.l.nargs < 0)
			pf->r.l.nargs = -pf->r.l.nargs - 1;
		if (pf->r.l.nargs > 50)
			{
			(void) fprintf(stderr, "lpfx: PANIC! nargs=%d\n",
			    pf->r.l.nargs);
			exit(1);
			}
		for (i=0;i<pf->r.l.nargs;i++){
			if ((fread((char *)&atype[i].aty,sizeof(atype[i].aty),1,stdin) < 0)||
			    (fread((char *)&atype[i].extra,sizeof(atype[i].extra),1,stdin) < 0))
			{
			(void)perror("lpfx.rdargs");
			exit(1);
			}

			}
		}
	}

#else	/* APEX */

void rdargs(), insert(), putout();
funct fu;	/* record into which all input is put */
char *savestr();



main(argc, argv)
int argc;
char **argv;
{
	extern char *optarg;
	int uscore, fmask, c;
	char *filename, *funcname;

	fmask = LDS | LDX | LRV;
	uscore = 0;
	while ((c = getopt(argc, argv, "i:")) != EOF)
		if (c == 'i')
			if (*optarg == '_')
				uscore = 1;
			else if (*optarg == 'x')
				fmask &= ~(LDS | LDX);
			else
				goto argerr;
		else
		argerr:
			(void)fprintf(stderr, "lpfx: bad option %c ignored\n", c);


	while ( apex_read() ) {
		if (fu.r.l.decflag & LFN)
			{
			filename = fu.r.f.fn;
			}
		else
			{
			funcname = fu.r.l.name;
			if (((fmask & LDS) ? ISFTN(fu.r.l.type.aty) :
			      !(fu.r.l.decflag & fmask)) &&
			    ((uscore) ? 1 : (*fu.r.l.name != '_')))
				{
				fu.fname = filename;
				insert(&fu);
				}
			}
	}
	putout();
}


/* reader input buffer variables */
#define INBUF_MIN	1024
int inbuf_sz = 0;
char *inbuf;


int apex_read()
{
    char tag;
    short len;
    char *inptr;
    register n;
    long tempdim[MAXDIMS];
    ATYPE temptype;


	for (;;) {	/* loop over non-applicable records */
	    if ( fread( (void *)&tag, 1, 1, stdin) <= 0 ) 
		return FALSE;
	    if (fread( (void *)&len, 2, 1, stdin) <= 0)
		return FALSE;
	    if (len) {
		if (len > inbuf_sz) {
		    inbuf_sz = (len>INBUF_MIN ? len : INBUF_MIN);
		    if ( (inbuf = (char *) malloc(inbuf_sz * sizeof(char)))==NULL) {
			    fprintf(stderr, "out of memory (inbuf)\n");
			    exit(-1);
		    }
		}
		if (fread( (void *)inbuf, len, 1, stdin) <= 0) {
		    return FALSE;	/* stop processing */
		}
	    }
	    inptr = inbuf;
	    
	    switch (tag) {
		case REC_CFILE :
		case REC_F77FILE :
		case REC_CPLUSFILE:
		    /* new filename and module number */
		    inptr += 1;	/* skip version number */
		    fu.r.f.fn = inptr;
		    inptr += unshroud(inptr);
		    fu.r.f.fn = savestr(fu.r.f.fn);
		    fu.r.l.decflag = LFN;
		    return TRUE;

		case REC_LDI:
		    fu.r.l.decflag = LDI;
		    break;
		case REC_LDI_NEW:
		    fu.r.l.decflag = LDI|LPR;
		    break;
		case REC_LFM:
		    fu.r.l.decflag = LDI|LFM;
		    break;
		case REC_LIB:
		    fu.r.l.decflag = LIB;
		    break;
		case REC_LIB_NEW:
		    fu.r.l.decflag = LIB|LPR;
		    break;
		case REC_LDC:
		    fu.r.l.decflag = LDC;
		    break;
		case REC_LDX:
		    fu.r.l.decflag = LDX;
		    break;
		case REC_LDX_NEW:
		    fu.r.l.decflag = LDX|LPR;
		    break;
		case REC_LRV:
		    fu.r.l.decflag = LRV;
		    break;
		case REC_LUV:
		    fu.r.l.decflag = LUV;
		    break;
		case REC_LUE:
		    fu.r.l.decflag = LUE;
		    break;
		case REC_LUE_LUV:
		    fu.r.l.decflag = LUE|LUV;
		    break;
		case REC_LUM:
		    fu.r.l.decflag = LUM;
		    break;
		case REC_LDS:
		    fu.r.l.decflag = LDS;
		    break;
		case REC_LDS_NEW:
		    fu.r.l.decflag = LDS|LPR;
		    break;
		case REC_COM:
		case REC_F77SET:
		case REC_F77PASS:
		case REC_F77BBLOCK:
		case REC_F77GOTO:
		case REC_PROCEND:
		case REC_STD:
		case REC_HINT:
		case REC_HDR_ERR:
		case REC_HDR_HINT:
		case REC_HDR_DETAIL:
		case REC_NOTUSED:
		case REC_NOTDEFINED:
		case REC_FILE_FMT:
		default:
		    continue;

	    }
	    
	    
	    /* common form: [line#] 
			    [name] 
			    [type] 
			    [# args]
			    [# altrets]
				    [type] for each arg
	    */
	    memmove(&fu.r.l.fline, inptr, 4);
	    inptr += 4;
	    fu.r.l.name = inptr;
	    inptr += unshroud(inptr);
	    fu.r.l.name = savestr(fu.r.l.name);
	    inptr += readtype(&fu.r.l.type, fu.r.l.dims, inptr);
	    if ( ISFTN(fu.r.l.type.aty) ) {
		memmove(&fu.r.l.nargs, inptr, 2);
		inptr += 4;			/* skip over nargs + altret */
	    } else
		fu.r.l.nargs = 0;
	    /* number of arguments is negative for VARARGS (plus one) */
	    n = fu.r.l.nargs;
	    if( n<0 ) n = -n - 1;
	    /* collect type info for all args */
	    if( n ) {
		int i;
		for (i=0;i<n;i++){
		    inptr += readtype(&temptype, tempdim, inptr);
		    /* read format string */
		    inptr += unshroud( inptr );
		}
	    }
	return TRUE;	/* return flag => continue processing */

	}
}

int readtype(t, dims, in)
ATYPE *t;
int dims[];
char *in;
{
char *start;
char numdims;
int i;

	start = in;
	memmove(&t->aty, in, 4);
	in += 4;
	memmove(&t->extra, in, 4);
	in += 4;
	if (BTYPE(t->aty) == STRTY || BTYPE(t->aty) == UNIONTY) {
	    memmove(&t->stcheck, in, 4);
	    in += 4;
	    t->stname = in;
	    in += unshroud(in);
	}
	memmove(&numdims, in, 1);
	t->numdim = numdims;
	in += 1;
	for (i=0; i<numdims; i++)  {
	    memmove(&dims[i], in, 4);
	    in += 4;
	}
	t->typename = in;
	in += unshroud(in);
	return (in-start);
}



/* decrypt the string p in place.
 *   Input:  [len][crypted string]
 *   Output: [decrypted, null-terminated string]
 *   Return: #bytes in input
 */
int unshroud(p)
char *p;
{
short len;
char *in, *out;
int i;

	memmove(&len, p, 2);
	out = p;
	in = p+2;
	for (i=0; i<len; i++)  {
#ifdef CLEAR_TEXT
	    *out++ = *in++;		
#else
	    *out++ = (*in++ + 96) % 256;
#endif
	}
	*out = '\0';

	return len + 2;		/* string + length field */
}


#define NSAVETAB	4096
char	*savetab;
unsigned saveleft = 0;

/* copy string into permanent string storage */
char *
savestr(cp)	
    char *cp;
{
	int len;

	len = strlen( cp ) + 1;
	if ( len > saveleft )
	{
		saveleft = NSAVETAB;
		if ( len > saveleft )
			saveleft = len;
		savetab = (char *) malloc( saveleft );
		if ( savetab == NULL ) {
			fprintf(stderr, "out of memory (lpfx:savestr)\n");
			exit (-1);
		}
	}
	(void) strncpy( savetab, cp, len );
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return ( cp );
}


#endif 	/* APEX */

/*
 * insert - insertion sort into (singly) linked list
 *
 *	stupid linear list insertion
 */

static li *head = NULL;

void insert(pfin)
	register funct *pfin;
	{
	register li *list_item, *newf;

	if ((newf = (li *)malloc(sizeof(li))) == NULL)
		{
		(void)fprintf(stderr, "lpfx: out of heap space\n");
		exit(1);
		}
	newf->fun = *pfin;
	if (list_item = head)
		if (newf->fun.r.l.fline < list_item->fun.r.l.fline)
			{
			newf->next = head;
			head = newf;
			}
		else
			{
			while (list_item->next &&
			  list_item->next->fun.r.l.fline < newf->fun.r.l.fline)
				list_item = list_item->next;
			while (list_item->next &&
			  list_item->next->fun.r.l.fline == newf->fun.r.l.fline &&
			  list_item->next->fun.r.l.decflag < newf->fun.r.l.decflag)
					list_item = list_item->next;
			newf->next = list_item->next;
			list_item->next = newf;
			}
	else	/* first insertion */
		{
		head = newf;
		newf->next = NULL;
		}
	}

/*
 * putout - format and print sorted records
 *
 *	while (there are records left)
 *		copy name and null terminate
 *		if (this is a definition)
 *			if (this is a function**)
 *				save name for reference formatting
 *			print definition format
 *		else if (this is a reference)
 *			print reference format
 *
 *	** as opposed to external/static variable
 */

void putout()
	{
	register li *pli;
	char lname[BUFSIZ], name[BUFSIZ];
	char *prtype();
	
	pli = head;
	name[0] = lname[0] = '\0';
	while (pli != NULL)
		{
		(void) strcpy(name, pli->fun.r.l.name);
		if (pli->fun.r.l.decflag & (LDI | LDC | LDS))
			{
			if (ISFTN(pli->fun.r.l.type.aty))
				(void)strcpy(lname, name);
			(void)printf("%s = %s, <%s %d>\n", name, prtype(pli),
			    pli->fun.fname, pli->fun.r.l.fline);
			}
		else if (pli->fun.r.l.decflag & (LUV | LUE | LUM))
			(void)printf("%s : %s\n", lname, name);
		pli = pli->next;
		}
	}

static char *types[] = {
	"???", "???", "char", "short", "int", "long", "float",
	"double", "struct", "union", "enum", "???", "unsigned char",
	"unsigned short", "unsigned int", "unsigned long", "void", 
	"long double", "signed char"};

/*
 * prtype - decode type fields
 *
 *	strictly arcana
 */

char *prtype(pli)
register li *pli;
{
	static char bigbuf[64];
	char buf[32], *shift(), *strcpy(), *strcat();
	register char *bp;
	register int typ;

	typ = pli->fun.r.l.type.aty;
	(void)strcpy(bigbuf, types[BTYPE(typ)]);
	*(bp = buf) = '\0';
	while (typ&TMASK) {
		if (ISPTR(typ)) {
			bp = shift(buf);
			buf[0] = '*';
		} else if (ISFTN(typ)) {
			*bp++ = '(';
			*bp++ = ')';
			*bp = '\0';
		} else if (ISARY(typ)) {
			*bp++ = '[';
			*bp++ = ']';
			*bp = '\0';
		}

		typ = DECREF(typ);
	}

	(void)strcat(bigbuf, buf);
	return(bigbuf);
}

char *shift(s)
	register char *s;
	{
	register char *p1, *p2;
	char *rp;

	for (p1 = s; *p1; ++p1)
		;
	rp = p2 = p1++;
	while (p2 >= s)
		*p1-- = *p2--;
	return(++rp);
	}


