/* HPUX_ID: @(#) $Revision: 70.3 $ */
/*

 *      Copyright (c) 1984, 1985, 1986, 1987, 
 *                  1988, 1989   AT&T
 *      All Rights Reserved

 *      THIS IS UNPUBLISHED PROPRIETARY SOURCE 
 *      CODE OF AT&T.
 *      The copyright notice above does not 
 *      evidence any actual or intended
 *      publication of such source code.

 */
/*
 * UNIX shell
 *
 * S. R. Bourne
 * Rewritten by David Korn
 * AT&T Bell Laboratories
 *
 */

#include	"defs.h"
#ifdef DEVFD
#   include	"jobs.h"
#endif	/* DEVFD */
#include	"sym.h"
#include	"builtins.h"

#ifdef HP_DEBUG
#include	"debug.h"
#endif

#ifdef DEVFD
    void	close_pipes();
#endif	/* DEVFD */

extern void	gsort();
#ifdef NLS
extern int	strcoll();
#endif /* NLS */
extern int	strcmp();

static int		arg_expand();
static struct dolnod*	copyargs();
static void		print_opts();
static int		split();

static char		*null;
static struct dolnod	*argfor; /* linked list of blocks to be cleaned up */
static struct dolnod	*dolh;
static char flagadr[12];
#ifndef POSIX2
static const char flagchar[] =
{
	'i',	'n',	'v',	't',	's',	'x',	'e',	'r',	'k',
	'u',	'f',	'a',	'm',	'h',	'p',	'c', 0
};
static const optflag flagval[]  =
{
	INTFLG,	NOEXEC,	READPR,	ONEFLG, STDFLG,	EXECPR,	ERRFLG,	RSHFLG,	KEYFLG,
	NOSET,	NOGLOB,	ALLEXP,	MONITOR, HASHALL, PRIVM, CFLAG, 0
};
#else
/* Added the -C (noclobber) option for POSIX.2 */
/* Added the -Q flag for skipping the execution of the ENV file */
static const char flagchar[] =
{
	'i',	'n',	'v',	't',	's',	'x',	'e',	'r',	'k',
	'u',	'f',	'a',	'm',	'h',	'p',	'c',    'C',	'Q', 0
};
static const optflag flagval[]  =
{
	INTFLG,	NOEXEC,	READPR,	ONEFLG, STDFLG,	EXECPR,	ERRFLG,	RSHFLG,	KEYFLG,
	NOSET,	NOGLOB,	ALLEXP,	MONITOR, HASHALL, PRIVM, CFLAG, NOCLOB, EVFLAG, 0
};
#endif /* POSIX2 */

/* ======== option handling	======== */

/*
 *  This routine turns options on and off
 *  The options "sicr" are illegal from set command (only allowed 
 *  from the command line).
 *  The -o option is used to set option by name
 *  This routine returns the number of non-option arguments
 */

int arg_opts(argc,argv)
register char **argv;
int  argc;
{
	register char *cp;
	register int c;
	register char *flagc;
	register optflag newflags=opt_flags;
	register optflag opt=0;
	char minus;   /* set if '-' option, unset if '+' option */
	struct namnod *np = (struct namnod*)0;
	char sort = 0;
	char minmin = 0;	/* set if we see '--' */
	/*
	 *  setflag is on if we are processing stuff for
	 *  the set built-in.  setflag is off if we are 
	 *  processing options specified on the command line
	 *  (e.g., /bin/ksh -c ....).
	 */
	int setflag = (st.states&BUILTIN);
	while((cp= *++argv) && ((c= *cp) == '-' || c=='+'))
	{
		minus = (c == '-');
		argc--;
		if((c= *++cp)==0)
		{
			newflags &= ~(EXECPR|READPR);
			argv++;
			break;
		}
		else if(c == '-')
		{
			/*
			 * supposed to detect '--' here
			 * but really detects 'c-' where
			 * c is any character.
			 */
			minmin = 1;
			argv++;
			break;
		}

		/*
		 *   Process all the option letters behind one
		 *   '+' or '-' (e.g., set +xs).
		 */
		while(c= *cp++)
		{
#ifdef HP_DEBUG
			/*
			 *   Process a set +/-D option here
			 *
			 *   There are several DEBUG options and they
			 *   can all be turned on/off independently.
			 *   A 'set -D' shows the current state of all
			 *   the debug options.   set -D option turns it
			 *   on.  set +D option turns it off.
			 */
			extern SYSTAB debug_options;

			if(c=='D')
			{
			    argv++;	/* skip over '-D' or '+D' */
			    if (*argv != NIL)
				opt = sh_lookup(*argv, debug_options);	

			    if(minus)
			    {
			    	/* -D: turn on debugging */
				if (*argv == NIL)	
					/* List Debug options */
					systab_print(debug_options,1);
				else
					/*  Turn on debug option */
					hp_debug |= opt;
			    }
			    else 
			    	/* +D: turn off debugging */
				hp_debug &= ~opt;

			    continue;
			}
#endif /* HP_DEBUG */
			if(setflag)
			{
			/*
			 *  Process stuff for set built-in
			 */
				if(c=='s')
				/*
				 * -s from set built-in means
				 * sort positional parameters
				 */
				{
					sort = 1;
					continue;
				}
				else if(c=='A')
				/*  Array assignment */
				{
					if(argv[1]==0)
						sh_fail(*argv, e_argexp);
					np = env_namset(*++argv,sh.var_tree,P_FLAG|V_FLAG);
					argc--;

					/* -A unsets name before assignment */
					if(minus)
						nam_free(np);
					continue;
				}
#ifndef POSIX2
				/*
				 * i, c, and r are only good from 
				 * from the command line.  Here we 
				 * are doing stuff for set.
				 */
				else if(strchr("icr",c))
#else
				/* Q flag is not good in set */
				else if(strchr("icrQ",c))
#endif POSIX2
					sh_fail(*argv, e_option);
			} /* end of setflag */
			if(c=='c' && minus && argc>=2 && sh.comdiv==0)
			/*
			 *  '-c string' from command line means read
			 *  commands from string.
			 */
			{
				sh.comdiv= *++argv;
				argc--;
				newflags |= CFLAG;
				continue;
			}
#ifdef POSIX2
			if(c=='Q' && minus)
			{
				newflags |= EVFLAG;
				continue;
			}
#endif
			/*
			 *  Check that the option letter is legal
			 *  and set opt to the value associated with
			 *  the option letter.
			 */
			if(flagc=strchr(flagchar,c))
				opt = flagval[flagc-flagchar];
			else if(c != 'o')
				sh_fail(*argv,e_option);
			else
			/*
			 *   got a '-o option' or a '+o option'
			 *   that we need to parse
			 */
			{
				/* skip over the '+o' or '-o' */
				argv++;
				if(*argv==NIL)
				/* '-o' and '+o' w/o an option */
				{
					print_opts(newflags);
					argv--;
					continue;
				}
				else
				/* '-o' and '+o' with an option */
				{
					argc--;
					c=sh_lookup(*argv,tab_options);
					if(c)
					{
					   opt = 1L<<c;
					   if(opt&(1|INTFLG|RSHFLG))
						sh_fail(*argv,e_option);
					}
					else
					{
#ifdef HP_BUILTIN			
					   if(eq(*argv,"hp_builtin"))
					      hp_builtin=minus?1:0;
					   else
#endif /* HP_BUILTIN */					    
					    sh_fail(*argv,e_option);
					}
				}
			}
			if(minus)
			{
#if ESH || VSH
				/*
				 *  Deal with all editing modes since
				 *  one edit option can affect several
				 *  flags
				 */
				if(opt&(EDITVI|EMACS|GMACS))
					newflags &= ~ (EDITVI|EMACS|GMACS);
#endif
#ifdef HPBRACE
				/*
				 *  If we turn on Hpbrace, then '{' is a
				 *  character that needs to be expanded.
				 */
				if (opt==HPBRACES)
				{
					_ctype1['{'] = T_EXP;
				}
#endif /* HPBRACE */
				newflags |= opt;
			}
			else
		        {
#ifdef HPBRACE
				/* reset '{' to a normal character */
				if (opt==HPBRACES)
				{
					_ctype1['{'] = 0;
				}
#endif /* HPBRACE */
				/* turn off the option */
				newflags &= ~opt;
			}

			/*
			 *  Mark that we have turned the monitor
			 *  option on/off, so that we don't override
			 *  a set +m in $ENV from job_init. See 
			 *  main.c:main() and jobs.c:job_init().
			 */
			if (opt&MONITOR)
				env_monitor = minus?ON:OFF;
		}
	}
	/* cannot set -n for interactive shells since there is no way out */
	if(is_option(INTFLG))
		newflags &= ~NOEXEC;
#ifdef RAWONLY
	if(is_option(EDITVI))
		newflags |= VIRAW;
#endif	/* RAWONLY */
	if(!setflag)
		goto skip;
	if(sort)
	{
		if(argc>1)
#ifdef NLS
			gsort(argv,argc-1,strcoll);
		else
			gsort(st.dolv+1,st.dolc,strcoll);
#else
			gsort(argv,argc-1,strcmp);
		else
			gsort(st.dolv+1,st.dolc,strcmp);
#endif /* NLS */
	}
	if((newflags&PRIVM) && !is_option(PRIVM))
	{
		if((sh.userid!=sh.euserid && setuid(sh.euserid)<0) ||
			(sh.groupid!=sh.egroupid && setgid(sh.egroupid)<0) ||
			(sh.userid==sh.euserid && sh.groupid==sh.egroupid))
			newflags &= ~PRIVM;
	}
	else if(!(newflags&PRIVM) && is_option(PRIVM))
	{
		setuid(sh.userid);
		setgid(sh.groupid);
		if(sh.euserid==0)
		{
			sh.euserid = sh.userid;
			sh.egroupid = sh.groupid;
		}
	}
skip:
	opt_flags = newflags;
	if(setflag)
	{
		argv--;
		if(np)
			env_arrayset(np,argc,argv);
		else if(argc>1 || minmin)
			arg_set(argv);
	}
	return(argc);
}

/*
 * returns the value of $-
 */

char *arg_dolminus()
{
	register const char *flagc=flagchar;
	register char *flagp=flagadr;
	while(*flagc)
	{
		if(opt_flags&flagval[flagc-flagchar])
			*flagp++ = *flagc;
		flagc++;
	}
	*flagp = 0;
	return(flagadr);
}

/*
 * set up positional parameters 
 */

void arg_set(argi)
char *argi[];
{
	register char **argp=argi;
	register int size = 0; /* count number of bytes needed for strings */
	register char *cp;
	register int 	argn;
	/* count args and number of bytes of arglist */
	while((cp=(char*)*argp++) != ENDARGS)
	{
		size += strlen(cp);
	}
	/* free old ones unless on for loop chain */
	argn = argp - argi;
	arg_free(dolh,0);
	dolh=copyargs(argi, --argn, size);
	st.dolc=argn-1;
}

/*
 * free the argument list if the use count is 1
 * If count is greater than 1 decrement count and return same blk
 * Free the argument list if the use count is 1 and return next blk
 * Delete the blk from the argfor chain
 * If flag is set, then the block dolh is not freed
 */

struct dolnod *arg_free(blk,flag)
struct dolnod *	blk;
{
	register struct dolnod*	argr=blk;
	register struct dolnod*	argblk;
	if(argblk=argr)
	{
		if((--argblk->doluse)==0)
		{
			if(flag && argblk==dolh)
				dolh->doluse = 1;
			else
			{
				/* delete from chain */
				if(argfor == argblk)
					argfor = argblk->dolnxt;
				else
				{
					for(argr=argfor;argr;argr=argr->dolnxt)
						if(argr->dolnxt==argblk)
							break;
					if(argr==0)
					{
						return(NULL);
					}
					argr->dolnxt = argblk->dolnxt;
				}
				free((char*)argblk);
			}
			argr = argblk->dolnxt;
		}
	}
	return(argr);
}

/*
 * grab space for arglist and link argblock for cleanup
 * The strings are copied after the argment vector
 */

static struct dolnod *copyargs(from, n, size)
char *from[];
{
	register struct dolnod *dp=new_of(struct dolnod,n*sizeof(char*)+size+n);
	register char **pp;
	register char *sp;
	dp->doluse=1;	/* use count */
	/* link into chain */
	dp->dolnxt = argfor;
	argfor = dp;
	pp= dp->dolarg;
	st.dolv=pp;
	sp = (char*)dp + sizeof(struct dolnod) + n*sizeof(char*);
	while(n--)
	{
		*pp++ = sp;
		sp = sh_copy(*from++,sp) + 1;
	}
	*pp = ENDARGS;
	return(dp);
}

/*
 *  used to set new argument chain for functions
 */

struct dolnod *arg_new(argi,savargfor)
char *argi[];
struct dolnod **savargfor;
{
	register struct dolnod *olddolh = dolh;
	*savargfor = argfor;
	dolh = NULL;
	argfor = NULL;
	arg_set(argi);
	return(olddolh);
}

/*
 * reset arguments as they were before function
 */

void arg_reset(blk,afor)
struct dolnod *blk;
struct dolnod *afor;
{
	while(argfor=arg_free(argfor,0));
	dolh = blk;
	argfor = afor;
}

void arg_clear()
{
	/* force `for' $* lists to go away */
	while(argfor=arg_free(argfor,1));
	argfor = dolh;
#ifdef DEVFD
	close_pipes();
#endif	/* DEVFD */
}

/*
 * increase the use count so that an arg_set will not make it go away
 */

struct dolnod *arg_use()
{
	register struct dolnod *dh;
	if(dh=dolh)
		dh->doluse++;
	return(dh);
}

/*
 *  Print option settings on standard output
 */

static void print_opts(oflags)
#ifndef pdp11
register
#endif	/* pdp11 */
optflag oflags;
{
	register const struct sysnod *syscan = tab_options;
#ifndef pdp11
	register
#endif	/* pdp11 */
	optflag value;
	p_setout(st.standout);
	p_str(e_heading,NL);
	while(value=syscan->sysval)
	{
		value = 1<<value;
		p_str(syscan->sysnam,SP);
		p_nchr(SP,16-strlen(syscan->sysnam));
		if(oflags&value)
			p_str(e_on,NL);
		else
			p_str(e_off,NL);
		syscan++;
	}
}

#ifdef VPIX
#   define EXTRA 2
#else
#   define EXTRA 1
#endif /* VPIX */

/*
 * build an argument list
 */

char **arg_build(nargs,comptr)
int 	*nargs;
struct comnod	*comptr;
{
	register struct argnod	*argp;
	{
		register struct comnod	*ac = comptr;
		register struct argnod	*schain;
		/* see if the arguments have already been expanded */
		if(ac->comarg==NULL)
		{
			*nargs = 0;
			return(&null);
		}
		else if((ac->comtyp&COMSCAN)==0)
		{
			*nargs = ((struct dolnod*)ac->comarg)->doluse;
			return(((struct dolnod*)ac->comarg)->dolarg+EXTRA);
		}
		schain = st.gchain;
		st.gchain = NULL;
		*nargs = arg_expand(ac);
		argp = st.gchain;
		st.gchain = schain;
	}
	{
		register char	**comargn;
		register int	argn;
		register char	**comargm;
		argn = *nargs;
		argn += EXTRA;	/* allow room to prepend args */
		comargn=(char**)stak_alloc((unsigned)(argn+1)*sizeof(char*));
		comargm = comargn += argn;
		*comargn = ENDARGS;
		while(argp)
		{
			*--comargn = argp->argval;
			if((argp->argflag&A_RAW)==0)
				sh_trim(*comargn);
			if((argp=argp->argchn)==0 || (argp->argflag&A_MAKE))
			{
				if((argn=comargm-comargn)>1)
#ifdef NLS
					gsort(comargn,argn,strcoll);
#else
					gsort(comargn,argn,strcmp);
#endif /* NLS */
				comargm = comargn;
			}
		}
		return(comargn);
	}
}

#ifdef DEVFD
static int to_close[15];

void close_pipes()
{
	register int *fd = to_close;
	while(*fd)
	{
		close(*fd);
		*fd++ = -1;
	}
}
#endif	/* DEVFD */

/* Argument list generation */

static int arg_expand(ac)
struct comnod		*ac;
{
	register struct argnod	*argp;
	register int 	count=0;
#ifdef DEVFD
	int indx = 0;
	close_pipes();
#endif	/* DEVFD */
	if(ac)
	{
		argp = ac->comarg;
		while(argp)
		{
			argp->argflag &= ~A_MAKE;
#ifdef DEVFD
			if(*argp->argval==0 && (argp->argflag&A_EXP))
			{
				/* argument of the form (cmd) */
				register struct argnod *ap;
				char *cp;
				int pv[2];
				int fd;
				ap = (struct argnod*)stak_begin();
				ap->argflag |= A_MAKE;
				ap->argflag &= ~A_RAW;
				ap->argchn= st.gchain;
				st.gchain = ap;
				count++;
				cp = sh_copy(e_devfd,ap->argval);
				io_popen(pv);
				fd = argp->argflag&A_RAW;
				stak_end(sh_copy(sh_itos(pv[fd]),cp));
				sh.inpipe = sh.outpipe = 0;
				if(fd)
				{
					sh.inpipe = pv;
					sh_exec((union anynode*)argp->argchn,(int)(st.states&ERRFLG));
				}
				else
				{
					sh.outpipe = pv;
					sh_exec((union anynode*)argp->argchn,(int)(st.states&ERRFLG));
				}
#ifdef JOBS
				job.pipeflag++;
#endif	/* JOBS */
				close(pv[1-fd]);
				to_close[indx++] = pv[fd];
			}
			else
#endif	/* DEVFD */
			if((argp->argflag&A_RAW)==0)
			{
				register char *ap; ap = argp->argval;
				if(argp->argflag&A_MAC)
					ap = mac_expand(ap);
				count += split(ap,argp->argflag&(A_SPLIT|A_EXP));
			}
			else
			{
				argp->argchn= st.gchain;
				st.gchain = argp;
				argp->argflag |= A_MAKE;
				count++;
			}
			argp = argp->argnxt.ap;
		}
	}
	return(count);
}

/*
 *  Perform word and file-name generation on the
 *  string s.  The results are a linked list of 
 *  struct argnods.  st.gchain will point to the
 *  last argnod created.
 */
static int split(s,macflg)
char *s;
{
	register char *argp;
	register int 	c;
	register struct argnod *ap;
	int 	count=0;
	int expflag = (!is_option(NOGLOB) && (macflg&A_EXP));
	const char *seps;
#ifdef POSIX2
	int	n_sep = 0;
	int 	coalesce_seps = 0; /* if true, then we coalesce adjacent seps */
#endif /* POSIX2 */	

	/*
	 *  Get the list of delimiters (separators)
	 *  and put it in seps
	 */
#ifdef POSIX2
	/*
	 *  POSIX.2 requires that if the value of IFS is
	 *  exactly <space><tab><newline>, or if it is
	 *  unset, then any each occurrence of IFS that is not
	 *  IFS white space (\n\t and blank), along with any
	 *  adjacent IFS white space shall delimit a field.
	 *
	 *  If coalesce_seps is true, then sequential seps
	 *  will count as one.
	 */
	/*
	 *  if IFS is unset  or IFS == sptbnl
	 *  then we coalesce
	 */
	seps = nam_fstrval(IFSNOD);
	if(!seps) coalesce_seps = 1;
	else if((c = strlen(e_sptbnl)) == strlen(seps)) {
		int i;
		for(i=0; i<c; i++)
			if(!strchr(seps, e_sptbnl[i])) break;
		if(i == c) coalesce_seps = 1;
	}
#endif /* POSIX2 */	
	if(macflg &= A_SPLIT)
		seps = nam_fstrval(IFSNOD);
	else
		seps = e_nullstr;
	if(seps==NULL)
		seps = e_sptbnl;

	while(1)
	{
		/*
		 *  There are still characters in s, so
		 *  get another struct argnod and make the
		 *  next word in the list
		 */
		if(sh.trapnote&SIGSET)
			sh_exit(SIGFAIL);
		ap = (struct argnod*)stak_begin();
		argp = ap->argval;
		while(c= *s++)
		{

			/*
			 *  Copy the string up to the first character in 
			 *  seps into argp (ap->argval).  If we run into 
			 *  an escape character, we copy it into the string
			 *  unless the next character is '/', in which case
			 *  we just copy the '/'.
			 */
			if(c == ESCAPE)
			{
				c = *s++;
				if(c!='/')
					*argp++ = ESCAPE;
			}
			else if(strchr(seps,c)) {
#ifdef POSIX2
				if(!strchr(e_sptbnl,c))
				   n_sep++;	/* non-white-space char */
#endif /* POSIX2 */
				break;
			}
			if(argp >= sh.brkend)
				sh_addmem(BRKINCR);
			*argp++ = c;
#ifdef POSIX2
			n_sep = 0;		/* initialize */
#endif
		}
		/*
		 *  At this point, s points to the character after the 
		 *  separator we just found; c = the separator we found;
		 *  argp points to the first space after the last non-separator
		 *  character
		 */

	        /* 
		 *  This allows contiguous visible delimiters to count 
		 *  as delimiters.
		 *
		 *  We have generated an argnod with a null string.  Check 
		 *  if this is because we are done, or we found a zero length
		 *  argument.
		 */
		if(argp==ap->argval)
		{
		        /*
			 *  If we have run off the end of s
			 *  return the number of words we found
			 */
			if(c==0)
				return(count);
			
			/*
			 *  If we do the continue, then we throw away
			 *  the current argnod and start a new one.  This
			 *  means that contiguous separators count as one.
			 *  If we don't do the continue, then we save the
			 *  null argument, make it a node, link it in, and
			 *  go on (i.e., contiguous delimeters delimit null
			 *  arguments.
			 */
#ifdef POSIX2
			/*
			 *  For POSIX shell, whether or not we coalesce is 
			 *  determined by the flag.  See above.
			 */
			if(macflg==0 || coalesce_seps==1)
				continue;
			/*
			 * let n be IFS non-white space, and s be IFS white
			 * space. We have to check for (s*ns*)
			 */
			if(coalesce_seps==0) {
				/* if it is a white space, coalesce */
				if(strchr(e_sptbnl,c)) continue;
				/* not a white space: find (ns* and s*ns*)
				 * check if we had already met with one non-
				 * white space character!
				 */
				if(n_sep == 0) {
				   /* this is first non-white space char */
				   n_sep++;
				   continue;
				}
			}
#else			
			if(macflg==0 || strchr(e_sptbnl,c))
				continue;
#endif /* POSIX2 */			
		}
		else if(c==0)
		{
			s--;
		}
		/*
		 *  We have found the end of one word.
		 *  We now end the stak element and do
		 *  file name generation on the new word.
		 */
		stak_end(argp);
		ap->argflag &= ~(A_RAW|A_MAKE);
#ifdef HPBRACE
		if(expflag && is_option(HPBRACES))
			count += expbrace(ap);
		else if(expflag && (c=path_expand(ap->argval)))
			count +=c;
#else
		if(expflag && (c=path_expand(ap->argval)))
			count += c;
#endif /* HPBRACE */
		else
		{
		/*
		 *  We do not do file name generation, so
		 *  increase the count of words we have found
		 *  and link this word in the word chain
		 */
			count++;
			ap->argchn= st.gchain;
			st.gchain = ap;
		}
		/* mark the list for sorting */
		st.gchain->argflag |= A_MAKE;
	}
}


#ifdef HP_DEBUG
/*
 *  Print out the first field of each record in a SYSTAB
 *  If flag==1 then print out debug info as well.
 */

systab_print(table, flag)
SYSTAB table;
int    flag;
{
	register struct sysnod *entry;

	entry = table;

	if (flag)
	/*  print out for debug info */
	{
		while(*entry->sysnam)
		{
			if (eq("all", entry->sysnam))
			{
				entry++;
				continue;
			}
			p_str(entry->sysnam,'\t');
			if (hp_debug & entry->sysval)
				p_str("On",NL);
			else
				p_str("Off",NL);
			entry++;
		}
	}
	else
		while(*entry->sysnam)
			p_str(entry++->sysnam, NL);
}
#endif /* HP_DEBUG */
