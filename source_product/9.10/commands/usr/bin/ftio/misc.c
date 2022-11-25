/* HPUX_ID: @(#) $Revision: 72.2 $  */
/*
 * misc.c
 *
 * miscelaneous routines that don't fit in elsewhere
 */

#include	"ftio.h"

/*
 * printperformance
 *
 * keeps track of how long the program has been running.
 * uses noblocks to calculate the effective transfer rate.
 * has four modes that do:
 *	mode = 0	set time for start of process.
 *	mode = 1	stop timer
 *	mode = 2	start timer
 *	mode = 3	print performance so far, using noblocks.
 */

printperformance(noblocks, mode)
int	noblocks;
int	mode;
{
	struct	timeval	timevalue;
	struct	timezone timez;
	long	time;			/* current time (from start of backup)*/
	static	long	starttime;	/* time of backup start */
	static	long	stoptime;	/* place holder */

	/*
	 * 	Get finishing time
	 */
	if (gettimeofday(&timevalue, &timez) < 0)
	{
		(void)ftio_mesg(FM_NGETT);
	}

	switch (mode)
	{
		case 0:	/* set time at start */
		{
			starttime = timevalue.tv_sec;
		}
		break;

		case 1:	/* stop timer */
		{
			stoptime = timevalue.tv_sec;
		}
		break;

		case 2:	/* restart timer */
		{
			starttime = starttime + (timevalue.tv_sec - stoptime);
		}
		break;

		case 3:	/* print performance to date */
		{
		   time = timevalue.tv_sec - starttime;

		   sprintf(Errmsg,
			"\n%s: total no of %d byte blocks transferred: %d\n",
			Myname, Blocksize, noblocks);
		   sprintf(Errmsg,
			"%s      total time was: %d secs\n", Errmsg, time);

		   if ( time > 0 )
		   {
		       sprintf(Errmsg,
			"%s      effective transfer rate: %d kbytes/sec\n\n",
			Errmsg, (Blocksize*noblocks)/(1024*time) );
		   }
		   fprintf(stderr, "%s", Errmsg);
		}
		break;

		default:
		{
			fprintf(stderr,
				"XXXX: illegal call on print_performance()!\n");
		}
		break;

	}
	return(0);
}

#ifdef hpux

short
makeshortdev (dev_no)	/* convert dev_t address to 16 bits */
dev_t dev_no;
{
	static struct conv_entry
	{
		long int	actual;
		short int	nickname;
	} Lookup[ENTRIES];

	static	nextfree = 0;
	static	nextnick = 1;
	int	candidate;

	/* search all prev. entered pairs; return if match  */
	for (candidate=0; candidate < nextfree; candidate++ )
		if ( Lookup[candidate].actual == dev_no )
			return ( Lookup[candidate].nickname );

			/* not found -- can we add it?  */

	if ( nextfree >= ENTRIES )
		return ( OUT_OF_SPACE );

			/* add this entry and return the next nickname  */

	Lookup[nextfree].actual = dev_no;
	return ( Lookup[nextfree++].nickname=nextnick++ );
}

#ifdef NOT_DEFINED
/*
 *	Don't need to use clip() anymore - it's too heavy handed anyway.
 *	See new code in filegrabber() and fileblaster().
 */
short
clip (big_inum)
long big_inum;
{
	return (big_inum >= UNREP_NO?  UNREP_NO: big_inum);
}
#endif /* NOT_DEFINED */
#endif /* hpux */

static	union
{
	long	l;
	short	s[2];
	char	c[4];
} u;

mkshort(v,lv)
short	*v;
long	lv;
{
	u.l = lv;
	*v = u.s[0];
	*(v+1) = u.s[1];
}

/*
 *	-- Convert long integer lv to two short integers v[0] and v[1],
 *	   and perform word swapping if necessary.
 */

long
mklong(v)
short v[];
{
	u.l = 1;
	if(u.c[0])
		u.s[0] = v[1], u.s[1] = v[0];
	else
		u.s[0] = v[0], u.s[1] = v[1];
	return u.l;
}

/*
 * katoi()
 *
 * same as atoi(3), but supports a 'k' as the last
 * char in a string and multiplies the total by 1024
 */
long
katoi(s)
char	*s;
{
	long	ret;

	ret = atoi(s);

	/*
	 * if there is a k in the string
	 * find atoi() of string up to that point, then
	 * multiply by 1024
	 */
	if ( strchr(s, 'k') )
		ret *= 1024;

	return(ret);
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : set_time()
 *	Purpose ............... : Sets access and modification times.
 *
 *	Description:
 *
 *
 *
 *	Returns:
 *		 0	if successful
 *		-1	if not (also prints diagnostic in this case).
 *
 *
 *
 */

set_time(namep, atime, mtime)
register char *namep;
time_t atime, mtime;
{
	struct	utimbuf	time;

	time.actime = atime;
	time.modtime = mtime;

	return utime(namep, &time);
}

ckname(name, patterns)
char	*name;
char	**patterns;
{
	/*
	 * 	If the pattern matches the file name
	 * 	return true normally, or false if restoring with '-f'
	 */
	if (nmatch(name, patterns))
		return(Except_patterns? 0: 1);
	else
		return(Except_patterns? 1: 0);
}

/* pattern matching functions */
nmatch(s, pat)
char *s, **pat;
{
	int accept = 0;
			/* To check for multiple exclude specifications. */

	if(!strcmp(*pat, "*"))
		return(1);

	while(*pat) {
		if(**pat == '!')
			if (gmatch(s, *pat+1))
				return 0;
			else accept = 1;
		else
			if (gmatch(s, *pat))
				return 1;
		++pat;
	}
	return(accept);
}

#ifdef NLS

gmatch(s, p)
register char *s, *p;
{
	if (fnmatch(p,s,0))
		return(0);
	else
		return(1);
}

#else /* NLS */

gmatch(s, p)
register char *s, *p;
{
	register int c;
	register cc, ok, lc, scc;

	scc = *s;
	lc = 077777;
	switch (c = *p) {

	case '[':
		ok = 0;
		while (cc = *++p) {
			switch (cc) {

			case ']':
				if (ok)
					return(gmatch(++s, ++p));
				else
					return(0);

			case '-':
				ok |= ((lc <= scc) && (scc <= (cc=p[1])));
			}
			if (scc==(lc=cc)) ok++;
		}
		return(0);

	case '?':
	caseq:
		if(scc) return(gmatch(++s, ++p));
		return(0);
	case '*':
		return(umatch(s, ++p));
	case 0:
		return(!scc);
	}
	if (c==scc) goto caseq;
	return(0);
}

umatch(s, p)
register char *s, *p;
{
	if(*p==0) return(1);
	while(*s)
		if (gmatch(s++,p)) return(1);
	return(0);
}

#endif /* NLS */

struct	ll_perms
{
	ushort	mask;
	char	c;
} perms[] =
{
	S_IREAD>>0,	'r',
	S_IWRITE>>0,	'w',
	S_IEXEC>>0,	'x',
	S_IREAD>>3,	'r',
	S_IWRITE>>3,	'w',
	S_IEXEC>>3,	'x',
	S_IREAD>>6,	'r',
	S_IWRITE>>6,	'w',
	S_IEXEC>>6,	'x'
};

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : llprint()
 *	Purpose ............... : Print stats like ls(1) -l.
 *
 *	Description:
 *
 *
 *
 *	Returns:
 *
 *	nothing.
 *
 */

llprint(namep, stats)
register char 	*namep;
struct	stat	*stats;
{
	static	int	last_lid = -1;
	static 	short 	last_uid = -1;
#include <pwd.h>
	static 	struct 	passwd *pw;
	struct 	passwd 	*getpwuid();
	char 	buf[32];
	char 	*nl_ctime();
	int	i;
	char	*p;

	if (last_lid == -1)
		last_lid = currlangid();

	(void)memset(buf, (int)'-', 11);

	switch(stats->st_mode & S_IFMT)
	{
	case S_IFDIR:
			*buf = 'd';
			break;

#if defined(OLD_RFA) && !defined(S_IFNWK)
#define S_IFNWK 0110000 	/* network special */
#endif
#if defined(RFA) || defined(OLD_RFA)
	case S_IFNWK:
			*buf = 'n';
			break;

#endif /* RFA || OLD_RFA */
#ifdef SYMLINKS
	case S_IFLNK:
			*buf = 'l';
			break;
#endif SYMLINKS

	case S_IFCHR:
			*buf = 'c';
			break;

	case S_IFBLK:
			*buf = 'b';
			break;

	case S_IFIFO:
			*buf = 'p';
			break;

	default:
			break;
	}


	/*
	 *	Do permissions.
	 */
	for (i = 0; i < 9; i++)
	{
		if (stats->st_mode & perms[i].mask)
			*(buf + 1 + i) = perms[i].c;
	}

	if (stats->st_mode & S_ISUID)
		*(buf + 3) = 's';

	if (stats->st_mode & S_ISGID)
		*(buf + 6) = 's';

	if (stats->st_mode & S_ISVTX)
		*(buf + 9) = 't';

	*(buf + 10) = '\0';

	(void)printf("%s %4d ", buf, stats->st_nlink);

	if (last_uid == stats->st_uid)
		(void)printf("%-8s", pw->pw_name);
	else
	{
		(void)setpwent();
		if(pw = getpwuid((int)stats->st_uid))
		{
			(void)printf("%-8s", pw->pw_name);
			last_uid = stats->st_uid;
		}
		else
		{
			(void)printf("%-8d", stats->st_uid);
			last_uid = -1;
		}
	}

	(void)strncpy(buf,
		      nl_ctime(&stats->st_mtime, "%h %d 19%y", last_lid),
		      32
	);

	if (p = strchr(buf, '\n'))
		*p = '\0';

	switch(stats->st_mode & S_IFMT)
	{
	case S_IFCHR:
	case S_IFBLK:
		(void)printf(" %2ld ", major(stats->st_size));
		(void)printf(MINOR_FORMAT, minor(stats->st_size));
		(void)printf(" %s %s\n", buf, namep);
		break;

	default:
		(void)printf(" %7ld %s %s\n", stats->st_size, buf, namep);
		break;
	}
}

static	long	pathname_size;

Pathname_init()
{
	char	*malloc();

	if ((Pathname = malloc(MAXPATHLEN)) == NULL)
		return -1;

	pathname_size = MAXPATHLEN;

	return 0;
}

Pathname_cpy(s, size)
char	*s;
int	size;
{
	char	*realloc();

	/*
	 *	If we don't have enough space,
	 *	get a new space.
	 */
	if (size >= pathname_size)
	{
		unsigned n_size;

		for(n_size = size + 1; n_size % MAXPATHLEN; n_size++)
		    ;

		if ((Pathname = realloc(Pathname, n_size)) == NULL)
		{
			ftio_mesg(FM_NMALL);
			return -1;
		}

		pathname_size = n_size;
	}

	/*
	 *	Copy over the new Pathname.
	 */
	(void)memcpy(Pathname, s, size);
	*(Pathname + size) = '\0';
	return 0;
}

Pathname_cat(s, size)
char	*s;
int	size;
{
	char	*realloc();
	int	current_l;

	current_l = strlen(Pathname);

	/*
	 *	If we don't have enough space,
	 *	get a new space.
	 */
	if (size + current_l >= pathname_size)
	{
	    unsigned n_size;

	    for(n_size = size + current_l + 1; n_size % MAXPATHLEN; n_size++)
		;

	    if ((Pathname = realloc(Pathname, n_size)) == NULL)
	    {
		ftio_mesg(FM_NMALL);
		return -1;
	    }

	    pathname_size = n_size;
	}

	/*
	 *	Copy over the new Pathname.
	 */
	(void)memcpy(Pathname + current_l, s, size);
	*(Pathname + size + current_l) = '\0';
	return 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : basename()
 *	Purpose ............... : Works like the shell (sh(1)) command.
 *
 *	Description:
 *
 *	Finds the basename of the string given to it.
 *
 *	Returns:
 *
 *	Pointer to the basename.
 *
 */
char	*
basename(p)
char	*p;
{
	char	*s;
	char    *lastcp = (char *)0;

	lastcp = &p[strlen(p)-1];
	if ( *lastcp == '/' )
	    *lastcp = '\0';

	if (s = strrchr(p, '/'))
		s++;
	else
		s = p;

	if ( *lastcp == '\0' )
	    *lastcp = '/';

	return s;
}
