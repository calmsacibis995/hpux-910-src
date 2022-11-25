/* @(#) $Revision: 70.1 $ */      
/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 *
 */


#include	"defs.h"

static struct dolnod *copyargs();
static struct dolnod *freedolh();
extern struct dolnod *freeargs();
static struct dolnod *dolh;

char	flagadr[14];

char	flagchar[] =
{
	'x',
	'n', 
	'v', 
	't', 
	STDFLG, 
	'i', 
	'e', 
	'r', 
	'k', 
	'u', 
	'h',
	'f',
	'a',
	 0
};

long	flagval[]  =
{
	execpr,	
	noexec,	
	readpr,	
	oneflg,	
	stdflg,	
	intflg,	
	errflg,	
	rshflg,	
	keyflg,	
	setflg,	
	hashflg,
	nofngflg,
	exportflg,
	  0
};
extern cheat;

/* ========	option handling	======== */
/* Modified 9/20/90 by Bill Gates to handle multiple option arguments. */
/* This entailed small mods to main.c to clarify login sh handling.    */

static  tchar sicr[] = { 's', 'i', 'c', 'r', 0 };

options(argc,argv)
tchar	**argv;
int    argc;
{
    register tchar *cp;
    register tchar **argp;
    register char *flagc;
    char    *flagp;
    int argno = 1;
    int increment = 1;

    argp = argv + cheat;
    argc -= cheat;

    if (argc > 1) {
        while(argp[argno]) {
            cp = argp[argno];

            if(*cp == '-') {
                if(*(cp+1) == '\0')
                    flags &= ~(execpr|readpr);
                else if(*(cp+1) == '-' && *(cp+2) == '\0') {
                    argp[argno] = argp[0];
                    argc--;
                    return(argc);
                } else {
                    while(*++cp) {
                        switch(*cp) {
                            case STDFLG: 
                                if(eqtc(argv[0], "set"))
                	            tfailed(argp[argno], nl_msg(601,badopt));
                	        flags |= stdflg;
                                break;
                            case 'i': 
                                if(eqtc(argv[0], "set"))
                	            tfailed(argp[argno], nl_msg(601,badopt));
                	        flags |= intflg;
                                break;
                            case 'c':
                                if(eqtc(argv[0], "set"))
                	            tfailed(argp[argno], nl_msg(601,badopt));
                                if(argp[argno+increment]) {
                                    if(comdiv == 0)
                                        comdiv = argp[argno+increment];
                                    increment++;
                                    argc--;
                                }
                                break;
                            case 'r': 
                                if(eqtc(argv[0], "set"))
                	            tfailed(argp[argno], nl_msg(601,badopt));
                	        flags |= rshflg;
                                break;
                            case 'x':
                	        flags |= execpr;
                                break;
                            case 'n': 
                	        flags |= noexec;
                                break;
                            case 'v': 
                	        flags |= readpr;
                                break;
                            case 't': 
                	        flags |= oneflg;
                                break;
                            case 'e': 
                	        flags |= errflg;
                	        eflag = errflg;
                                break;
                            case 'k': 
                	        flags |= keyflg;
                                break;
                            case 'u': 
                	        flags |= setflg;
                                break;
                            case 'h':
                	        flags |= hashflg;
                                break;
                            case 'f':
                	        flags |= nofngflg;
                                break;
                            case 'a':
                	        flags |= exportflg;
                                break;
                            default:
                                tfailed(argp[argno], nl_msg(601,badopt));
                        }
                    }
                }
                argp[argno+increment-1] = argp[0];
                argc--;

            } else if(*cp == '+') {
                while(*++cp) {
                    switch(*cp) {
                        case 'x':
                            if(flags & execpr)
                                flags &= ~execpr;
                            break;
                        case 'n': 
                            if(flags & noexec)
                                flags &= ~noexec;
                            break;
                        case 'v': 
                            if(flags & readpr)
                                flags &= ~readpr;
                            break;
                        case 't': 
                            if(flags & oneflg)
                                flags &= ~oneflg;
                            break;
                        case 'e': 
                            if(flags & errflg) {
                                flags &= ~errflg;
                                eflag = 0;
                            }
                            break;
                        case 'k': 
                            if(flags & keyflg)
                                flags &= ~keyflg;
                            break;
                        case 'u': 
                            if(flags & setflg)
                                flags &= ~setflg;
                            break;
                        case 'h':
                            if(flags & hashflg)
                                flags &= ~hashflg;
                            break;
                        case 'f':
                            if(flags & nofngflg)
                                flags &= ~nofngflg;
                            break;
                        case 'a':
                            if(flags & exportflg)
                                flags &= ~exportflg;
                            break;
                    }
                }
                argp[argno] = argp[0];
                argc--;
            } else
                break;
            argno += increment;
            increment = 1;
        }
    }
    /*
     * set up $-
     */
    flagp = flagadr;
    if (flags)
    {
        flagc = flagchar;
        while (*flagc)
        {
            if (flags & flagval[flagc-flagchar])
                *flagp++ = *flagc;
            flagc++;
        }
    }
    *flagp = 0;
    return(argc + cheat);
}

/*
 * sets up positional parameters
 */
setargs(argi)
    tchar    *argi[];
{
    register tchar **argp = argi;    /* count args */
    register int argn = 0;

    while (Rcheat(*argp++) != ENDARGS)
        argn++;
    /*
     * free old ones unless on for loop chain
     */
    freedolh();
    dolh = copyargs(argi, argn);
    dolc = argn - 1;
}


static struct dolnod *
freedolh()
{
    register tchar **argp;
    register struct dolnod *argblk;

    if (argblk = dolh)
    {
        if ((--argblk->doluse) == 0)
        {
            for (argp = argblk->dolarg; Rcheat(*argp) != ENDARGS; argp++)
                free(*argp);
            free(argblk);
        }
    }
}

struct dolnod *
freeargs(blk)
    struct dolnod *blk;
{
    register tchar **argp;
    register struct dolnod *argr = 0;
    register struct dolnod *argblk;
    int cnt;

    if (argblk = blk)
    {
        argr = argblk->dolnxt;
        cnt  = --argblk->doluse;

        if (argblk == dolh)
        {
            if (cnt == 1)
                return(argr);
            else
                return(argblk);
        }
        else
        {    		
            if (cnt == 0)
            {
                for (argp = argblk->dolarg; Rcheat(*argp) != ENDARGS; argp++)
                	free(*argp);
                free(argblk);
            }
        }
    }
    return(argr);
}

static struct dolnod *
copyargs(from, n)
    tchar    *from[];	
{
    register struct dolnod *np = (struct dolnod *)alloc(sizeof(tchar**) * n + 3 * BYTESPERWORD);
    register tchar **fp = from;
    register tchar **pp;

    np->doluse = 1;    /* use count */
    pp = np->dolarg;
    dolv = pp;
    
    while (n--)
        *pp++ = make(*fp++);
    *pp++ = ENDARGS;
    return(np);
}


struct dolnod *
clean_args(blk)
    struct dolnod *blk;
{
    register tchar **argp;    	
    register struct dolnod *argr = 0;
    register struct dolnod *argblk;

    if (argblk = blk)
    {
        argr = argblk->dolnxt;

        if (argblk == dolh)
            argblk->doluse = 1;
        else
        {
            for (argp = argblk->dolarg; Rcheat(*argp) != ENDARGS; argp++)
                free(*argp);
            free(argblk);
        }
    }
    return(argr);
}

clearup()
{
    /*
     * force `for' $* lists to go away
     */
    while (argfor = clean_args(argfor))
        ;
    /*
     * clean up io files
     */
    while (pop())
        ;

    /*
     * clean up tmp files
    */
    while (poptemp())
        ;
}

struct dolnod *
useargs()
{
    if (dolh)
    {
        if (dolh->doluse++ == 1)
        {
            dolh->dolnxt = argfor;
            argfor = dolh;
        }
    }
    return(dolh);
}
/*  commented out for possible use later
billconv(str1, str2)
char *str1, *str2;
{
    int i, index = 0;

    for(i=1; str1[i]; i+=2)
        str2[index++] = str1[i];
    str2[index] = 0;
}

printargp(p)
tchar **p;
{
    int i;
    char mystr[400];

    for(i=0; p[i]; i++) {
        billconv(p[i], mystr);
        printf("argp[%d] = %s\n", i, mystr);
    }
}*/
