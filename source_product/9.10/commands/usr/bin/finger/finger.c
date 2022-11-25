/* HPUX_ID: @(#) $Revision: 70.1 $ */

/*  Modified: 11 December 1986 - lkc - if pri=n is in the GCOS field,
	      its printing is suppressed.
*/

/*  This is a finger program.  It prints out useful information about users
 *  by digging it up from various system files.  It is not very portable
 *  because the most useful parts of the information (the full user name,
 *  office, and phone numbers) are all stored in the VAX-unused gecos field
 *  of /etc/passwd, which, unfortunately, other UNIXes use for other things.
 *
 *  There are three output formats, all of which give login name, teletype
 *  line number, and login time.  The short output format is reminiscent
 *  of finger on ITS, and gives one line of information per user containing
 *  in addition to the minimum basic requirements (MBR), the full name of
 *  the user, his idle time and office location and phone number.  The
 *  quick style output is UNIX who-like, giving only name, teletype and
 *  login time.  Finally, the long style output give the same information
 *  as the short (in more legible format), the home directory and shell
 *  of the user, and, if it exits, a copy of the file .plan in the users
 *  home directory.  Finger may be called with or without a list of people
 *  to finger -- if no list is given, all the people currently logged in
 *  are fingered.
 *
 *  The program is validly called by one of the following:
 *
 *	finger			{short form list of users}
 *	finger -l		{long form list of users}
 *	finger -b		{briefer long form list of users}
 *	finger -q		{quick list of users}
 *	finger -i		{quick list of users with idle times}
 *      finger -R               {print user's host name}
 *	finger namelist		{long format list of specified users}
 *	finger -s namelist	{short format list of specified users}
 *	finger -w namelist	{narrow short format list of specified users}
 *
 *  where 'namelist' is a list of users login names.
 *  The other options can all be given after one '-', or each can have its
 *  own '-'.  The -f option disables the printing of headers for short and
 *  quick outputs.  The -b option briefens long format outputs.  The -p
 *  option turns off plans for long format outputs.
 */

#include	<sys/types.h>
#include	<sys/file.h>
#include	<sys/stat.h>
#include	<sgtty.h>
#include	<utmp.h>
#include	<signal.h>
#include	<pwd.h>
#include	<stdio.h>
#include	<sys/time.h>
#ifdef TRUX
#include	<sys/security.h>
#endif TRUX

struct	utmp	utmp;	/* for sizeof */
#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)
#define HMAX sizeof(utmp.ut_host)

#ifdef HP
#include <memory.h>
#define	bzero(s, c)	memset(s, 0, c)
#endif HP

#define		ASTERISK	'*'	/* ignore this in real name */
#define		BLANK		' '	/* blank character (i.e. space) */
#define		CAPITALIZE	0137&	/* capitalize character macro */
#define		COMMA		','	/* separator in pw_gecos field */
#define		COMMAND		'-'	/* command line flag char */
#define		CORY		'C'	/* cory hall office */
#define		EVANS		'E'	/* evans hall office */
#define		LINEBREAK	012	/* line feed */
#define		NULLSTR		""	/* the null string, opposed to NULL */
#define		SAMENAME	'&'	/* repeat login name in real name */
#define		TALKABLE	0222	/* tty is writeable if 222 mode */

struct  person  {			/* one for each person fingered */
	char		name[NMAX+1];	/* login name */
	char		tty[LMAX+1];	/* NULL terminated tty line */
	char		host[HMAX+1];	/* NULL terminated host name */
	long		loginat;	/* time of login (possibly last) */
	long		idletime;	/* how long idle (if logged in) */
	short int	loggedin;	/* flag for being logged in */
	short int	writeable;	/* flag for tty being writeable */
	char		*realname;	/* pointer to full name */
	char		*office;	/* pointer to office name */
	char		*officephone;	/* pointer to office phone no. */
	char		*homephone;	/* pointer to home phone no. */
	char		*random;	/* for any random stuff in pw_gecos */
	struct  passwd	*pwd;		/* structure of /etc/passwd stuff */
	struct  person	*link;		/* link to next person */
};

struct  passwd			*NILPWD = 0;
struct  person			*NILPERS = 0;

int		persize		= sizeof( struct person );
int		pwdsize		= sizeof( struct passwd );

extern int netfinger();

#ifdef TRUX
extern void putc_filter();
#endif TRUX

char		USERLOG[]	= "/etc/utmp";		/* who is logged in */
char		outbuf[BUFSIZ];				/* output buffer */
char		*ctime();

int		unbrief		= 1;		/* -b option default */
int		header		= 1;		/* -f option default */
int		hack		= 1;		/* -h option default */
int		print_host	= 0;		/* -R option default */
int		idle		= 0;		/* -i option default */
int		large		= 0;		/* -l option default */
int		match		= 1;		/* -m option default */
int		plan		= 1;		/* -p option default */
int		unquick		= 1;		/* -q option default */
int		small		= 0;		/* -s option default */
int		wide		= 1;		/* -w option default */

long		tloc;				/* current time */

char		domainname[256] = "";	/* for YP version of lookup */



main( argc, argv )

    int		argc;
    char	*argv[];

{
	FILE			*fp,  *fopen();		/* for plans */
	struct  passwd		*getpwent();		/* read /etc/passwd */
	struct  person		*person1,  *p,  *pend;	/* people */
	struct  passwd		*pw;			/* temporary */
	struct  utmp		user;			/*   ditto   */
	char			*malloc();
	char			*s,  *pn,  *ln;
	char			c;
	char			*PLAN = "/.plan";	/* what plan file is */
	char			*PROJ = "/.project";	/* what project file */
	int			PLANLEN = strlen( PLAN );
	int			PROJLEN = strlen( PROJ );
	int			numnames = 0;
	int			orgnumnames;
	int			uf;
	int			usize = sizeof user;
	int			unshort;
	int			i, j;
	int			fngrlogin;

    /*  parse command line for (optional) arguments */

	i = 1;
	if(  strcmp( *argv, "sh" )  )  {
	    fngrlogin = 0;
	    while( i++ < argc  &&  (*++argv)[0] == COMMAND )  {
		for( s = argv[0] + 1; *s != NULL; s++ )  {
			switch  (*s)  {

			    case 'b':
				    unbrief = 0;
				    break;

			    case 'f':
				    header = 0;
				    break;

			    case 'h':
				    hack = 0;
				    break;

			    case 'R':
				    print_host = 1;
				    break;

			    case 'i':
				    idle = 1;
				    unquick = 0;
				    break;

			    case 'l':
				    large = 1;
				    break;

			    case 'm':
				    match = 0;
				    break;

			    case 'p':
				    plan = 0;
				    break;

			    case 'q':
				    unquick = 0;
				    break;

			    case 's':
				    small = 1;
				    break;

			    case 'w':
				    wide = 0;
				    break;

			    default:
				fprintf( stderr, "finger: Usage -- 'finger [-bfhilmpqswR] [login1 [login2 ...] ]'\n" );
				exit( 1 );
			}
		}
	    }
	}
	else  {
	    fngrlogin = 1;
	}
	if( unquick )  {
	    time( &tloc );
	}
	else  {
	    if( idle )  {
		time( &tloc );
	    }
	}

    /*  i > argc means no login names given so get them by reading USERLOG */

	if(  (i > argc)  ||  fngrlogin  )  {
	    unshort = large;
	    if(  ( uf = open(USERLOG, 0) ) >= 0  )  {
Retry:		user.ut_name[0] = NULL;
		while( user.ut_name[0] == NULL )  {
		    if( read( uf, (char *) &user, usize ) != usize )  {
			printf( "\nNo one logged on\n" );
			exit( 0 );
		    }
		}
		if( user.ut_type != USER_PROCESS )	goto Retry;
		person1 = (struct person  *) calloc(1, persize );
		strncpy(person1->name, user.ut_name, NMAX);
		strncpy(person1->tty, user.ut_line, LMAX);
		strncpy(person1->host, user.ut_host, HMAX);
		person1->name[NMAX] = NULL;
		person1->tty[LMAX] = NULL;
		person1->host[HMAX] = NULL;
		person1->loginat = user.ut_time;
		person1->pwd = NILPWD;
		person1->loggedin = 1;
		numnames++;
		p = person1;
		while( read( uf, (char *) &user, usize ) == usize )  {
		    if( user.ut_name[0] == NULL )	continue;
		    if( user.ut_type != USER_PROCESS )	continue;
		    p->link = (struct person  *) calloc(1, persize );
		    p = p->link;
		    strncpy(p->name, user.ut_name, NMAX);
		    strncpy(p->tty, user.ut_line, LMAX);
		    strncpy(p->host, user.ut_host, HMAX);
		    p->name[NMAX] = NULL;
		    p->tty[LMAX] = NULL;
		    p->host[HMAX] = NULL;
		    p->loginat = user.ut_time;
		    p->pwd = NILPWD;
		    p->loggedin = 1;
		    numnames++;
		}
		p->link = NILPERS;
		close( uf );
	    }
	    else  {
		fprintf( stderr, "finger: error opening %s\n", USERLOG );
		exit( 2 );
	    }

	    /*  If the domainname is set, we use getpwnam(), since it will */
	    /*  be more efficient with Yellow Pages; otherwise use the old */
	    /*  algorithm which will work better in the local non-YP case */
	    /*  Mark Notess and Darren Smith, 3-7-89                       */

	    if( unquick )  {
						   /* if we are running YP */
		(void) getdomainname( domainname, sizeof(domainname) );
		if ( domainname[0] != '\0' ) {
		    p = person1;
		    do  {
			if( p->pwd == NILPWD )  {
			    if(  (pw = getpwnam( p->name )) != NULL  )  {
				p->pwd = (struct passwd  *) calloc(1, pwdsize );
				pwdcopy( p->pwd, pw );
				decode( p );
				i--;
			    }
			}
			p = p->link;
		    }  while( p != NILPERS );
		}
 	        else {
					/* non-YP case */
		    setpwent();
		    i = numnames;
		    while(  ( (pw = getpwent()) != NILPWD )  &&  ( i > 0 )  )  {
		        p = person1;
		        do  {
			    if( p->pwd == NILPWD )  {
			        if(  strcmp( p->name, pw->pw_name ) == 0  )  {
				    p->pwd = (struct passwd  *) calloc(1, pwdsize );
				    pwdcopy( p->pwd, pw );
				    decode( p );
				    i--;
			        }
			    }
			    p = p->link;
		        }  while( p != NILPERS );
		    }
		    endpwent();
		}
	    }
	}

    /* get names from command line and check to see if they're  logged in */

	else  {
	    unshort = ( small == 1 ? 0 : 1 );
	    p = person1 = NILPERS;
	    while (i++ <= argc) {
		if (netfinger(argv[0])) {
		    argv++;
		    continue;
		}
		if (person1 == NILPERS) {
	    	    person1 = (struct person  *) calloc(1, persize );
	    	    strcpy(  person1->name, (argv++)[ 0 ]  );
	    	    person1->loggedin = 0;
	    	    person1->pwd = NILPWD;
		    p = person1;
		} else {
		    p->link = (struct person  *) calloc(1, persize );
		    p = p->link;
		    strcpy(  p->name, (argv++)[ 0 ]  );
		    p->loggedin = 0;
		    p->pwd = NILPWD;
		}
		numnames++;
	    }
	    if (person1 == NILPERS)
		exit(0);
	    p->link = NILPERS;
	    pend = p;

	    /*  If we are returning the details from /etc/passwd, read     */
	    /*  /etc/passwd for the useful info.			   */

	    orgnumnames = numnames;
	    if( unquick )  {
		  setpwent();
		  while(  ( pw = getpwent() ) != NILPWD  )  {
		    p = person1;
		    i = 0;
		    do  {
			if( strcmp( p->name, pw->pw_name ) == 0    ||
			matchcmp( pw->pw_gecos, pw->pw_name, p->name ) )  {
			    if( p->pwd == NILPWD )  {
				p->pwd = (struct passwd  *) calloc(1, pwdsize );
				pwdcopy( p->pwd, pw );
			    }
			    else  {	/* handle multiple logins -- append new
					   "duplicate" entry to end of list */
				/* DTS: DSDe406894
				 * Changed to use calloc instead of malloc to
				 * avoid uninitialized fields.
				 */
				pend->link = (struct person  *) calloc(1, persize);
				pend = pend->link;
				pend->link = NILPERS;
				strcpy( pend->name, p->name );
				pend->pwd = (struct passwd  *) calloc(1, pwdsize);
				pwdcopy( pend->pwd, pw );
				numnames++;
			    }
			}
			p = p->link;
		    }  while( ++i < orgnumnames );
		  }
		endpwent();
	    }

		/*  Now get login information */

	    if(  ( uf = open(USERLOG, 0) ) >= 0  )  {
		while( read( uf, (char *) &user, usize ) == usize )  {
		    if( user.ut_name[0] == NULL )	continue;
		    if( user.ut_type != USER_PROCESS )	continue;
		    p = person1;
		    do  {
			pw = p->pwd;
			if( pw == NILPWD )  {
			    i = ( strcmp( p->name, user.ut_name ) ? 0 : NMAX );
			}
			else  {
			    i = 0;
			    while(  (i < NMAX)  &&
				    ( pw->pw_name[i] == user.ut_name[i])  )  {
				if( pw->pw_name[i] == NULL )  {
				    i = NMAX;
				    break;
				}
				i++;
			    }
			}
			if( i == NMAX )  {
			    if( p->loggedin == 1 )  {
				pend->link = (struct person  *) calloc(1, persize);
				pend = pend->link;
				pend->link = NILPERS;
				strcpy( pend->name, p->name );
				strncpy( pend->tty, user.ut_line, LMAX);
				strncpy( pend->host, user.ut_host, HMAX);
				pend->tty[ LMAX ] = NULL;
				pend->host[ HMAX ] = NULL;
				pend->loginat = user.ut_time;
				pend->loggedin = 2;
				if(  pw == NILPWD  )  {
				    pend ->pwd = NILPWD;
				}
				else  {
				    pend->pwd = (struct passwd  *) calloc(1, pwdsize);
				    pwdcopy( pend->pwd, pw );
				}
				numnames++;
			    }
			    else  {
				if( p->loggedin != 2 )  {
				    strncpy( p->tty, user.ut_line, LMAX );
				    strncpy( p->host, user.ut_host, HMAX );
				    p->tty[ LMAX ] = NULL;
				    p->host[ HMAX ] = NULL;
				    p->loginat = user.ut_time;
				    p->loggedin = 1;
				}
			    }
			}
			p = p->link;
		    }  while( p != NILPERS );
		}
		p = person1;
		while( p != NILPERS )  {
		    if( p->loggedin == 2 )  {
			p->loggedin = 1;
		    }
		    decode( p );
		    p = p->link;
		}
		close( uf );
	    }
	    else  {
		fprintf( stderr, "finger: error opening %s\n", USERLOG );
		exit( 2 );
	    }
	}

    /* print out what we got */

	if( header )  {
	    if( unquick )  {
		if( !unshort )  {
		    if( wide && print_host )  {
			printf(
 "Login       Name               TTY Idle    When     Where\n" );
		    }
		    else if ( wide && !print_host ) {
			printf(
#ifdef HP
"Login       Name               TTY Idle    When    Bldg.         Phone\n" );
#else
"Login       Name              TTY Idle    When            Office\n" );
#endif HP
		    }
		    else if ( !wide && print_host ) {
			printf(
"Login    TTY Idle    When      Host                    Bldg.\n" );
		    }
		    else  {
			printf(
#ifdef HP
"Login    TTY Idle    When            Bldg.\n" );
#else
"Login    TTY Idle    When            Office\n" );
#endif HP
		    }
		}
	    }
	    else  {
		printf( "Login      TTY            When" );
		if ( print_host ) {
		    printf( "            Where" );
		}
		if( idle )  {
		    if ( print_host ) {
		        printf( "            Idle" );
		    }
		    else {
		        printf( "             Idle" );
		    }
		}
		printf( "\n" );
	    }
	}
	p = person1;
	do  {
	    if( unquick )  {
		if( unshort )  {
		    personprint( p );
		    if( p->pwd != NILPWD )  {
			if( hack )  {
			    s = malloc(strlen((p->pwd)->pw_dir) + PROJLEN + 1 );
			    strcpy(  s, (p->pwd)->pw_dir  );
			    strcat( s, PROJ );
			    if(  ( fp = fopen( s, "r") )  != NULL  )  {
				printf( "Project: " );
				while(  ( c = getc(fp) )  !=  EOF  )  {
				    if( c == LINEBREAK )  {
					break;
				    }
#ifdef TRUX
				    if (ISB1)
				        putc_filter( c, stdout );
				    else
				        putc( c, stdout );
#else
				    putc( c, stdout );
#endif TRUX
				}
				fclose( fp );
				printf( "\n" );
			    }
			}
			if( plan )  {
			    s = malloc( strlen( (p->pwd)->pw_dir ) + PLANLEN + 1 );
			    strcpy(  s, (p->pwd)->pw_dir  );
			    strcat( s, PLAN );
			    if(  ( fp = fopen( s, "r") )  == NULL  )  {
				printf( "No Plan.\n" );
			    }
			    else  {
				printf( "Plan:\n" );
				while(  ( c = getc(fp) )  !=  EOF  )  {
#ifdef TRUX
				    if (ISB1)
				        putc_filter( c, stdout );
				    else
				        putc( c, stdout );
#else
				    putc( c, stdout );
#endif TRUX
				}
				fclose( fp );
			    }
			}
		    }
		    if( p->link != NILPERS )  {
			printf( "\n" );
		    }
		}
		else  {
		    shortprint( p );
		}
	    }
	    else  {
		quickprint( p );
	    }
	    p = p->link;
	}  while( p != NILPERS );
	exit(0);
}


/*  given a pointer to a pwd (pfrom) copy it to another one, allocating
 *  space for all the stuff in it.  Note: Only the useful (what the
 *  program currently uses) things are copied.
 */

pwdcopy( pto, pfrom )		/* copy relevant fields only */

    struct  passwd		*pto,  *pfrom;
{
	pto->pw_name = malloc(  strlen( pfrom->pw_name ) + 1  );
	strcpy( pto->pw_name, pfrom->pw_name );
	pto->pw_uid = pfrom->pw_uid;
	pto->pw_gecos = malloc(  strlen( pfrom->pw_gecos ) + 1  );
	strcpy( pto->pw_gecos, pfrom->pw_gecos );
	pto->pw_dir = malloc(  strlen( pfrom->pw_dir ) + 1  );
	strcpy( pto->pw_dir, pfrom->pw_dir );
	pto->pw_shell = malloc(  strlen( pfrom->pw_shell ) + 1  );
	strcpy( pto->pw_shell, pfrom->pw_shell );
}


/*  print out information on quick format giving just name, tty, login time
 *  and idle time if idle is set.
 */

quickprint( pers )

    struct  person		*pers;
{
	int			idleprinted;

	printf( "%-*.*s", NMAX, NMAX, pers->name );
	printf( "  " );
	if( pers->loggedin )  {
	    if( idle )  {
		findidle( pers );
		if( pers->writeable )  {
		    printf(  " %-*.*s %-16.16s", LMAX, LMAX, 
			pers->tty, ctime( &pers->loginat )  );
		}
		else  {
		    printf(  "*%-*.*s %-16.16s", LMAX, LMAX, 
			pers->tty, ctime( &pers->loginat )  );
		}
		if ( print_host ) {
		    printf( "  %-*.*s ", HMAX, HMAX, pers->host);
		}
		else {
		    printf( "   " );
		}
		idleprinted = ltimeprint( &pers->idletime );
	    }
	    else  {
		if ( print_host ) {
                    printf(  " %-*.*s %-16.16s  %-*.*s", LMAX, LMAX,
                        pers->tty, ctime( &pers->loginat ), HMAX,
                        HMAX, pers->host );
		}
		else {
		    printf(  " %-*.*s %-16.16s", LMAX, LMAX, 
		        pers->tty, ctime( &pers->loginat )  );
		}
	    }
	}
	else  {
	    printf( "          Not Logged In" );
	}
	printf( "\n" );
}


/*  print out information in short format, giving login name, full name,
 *  tty, idle time, login time, office location and phone.
 */

shortprint( pers )

    struct  person	*pers;

{
	struct  passwd		*pwdt = pers->pwd;
	char			buf[ 26 ];
	int			i,  len,  offset,  dialup;

	if( pwdt == NILPWD )  {
	    printf( "%-*.*s", NMAX, NMAX,  pers->name );
	    printf( "       ???\n" );
	    return;
	}
	printf( "%-*.*s", NMAX, NMAX,  pwdt->pw_name );
	dialup = 0;
	if( wide )  {
	    if(  strlen( pers->realname ) > 0  )  {
		printf( " %-20.20s", pers->realname );
	    }
	    else  {
		printf( "        ???          " );
	    }
	}
	if( pers->loggedin )  {
	    if( pers->writeable )  {
		printf( "  " );
	    }
	    else  {
		printf( " *" );
	    }
	}
	else  {
	    printf( "  " );
	}
	if(  strlen( pers->tty ) > 0  )  {
            register char *p = strrchr(pers->tty, '/');
            strcpy( buf, p ? ++p : pers->tty );
	    if(  (buf[0] == 't')  &&  (buf[1] == 't')  &&  (buf[2] == 'y')  )  {
		offset = 3;
		for( i = 0; i < 3; i++ )  {
		    buf[i] = buf[i + offset];
		}
	    }
	    if(  (buf[0] == 'd')  &&  pers->loggedin  )  {
		dialup = 1;
	    }
	    printf( "%-3.3s ", buf );
	}
	else  {
	    printf( "   " );
	}
	strcpy(buf, ctime(&pers->loginat));
	if( pers->loggedin )  {
	    stimeprint( &pers->idletime );
	    offset = 7;
	    for( i = 4; i < 19; i++ )  {
		buf[i] = buf[i + offset];
	    }
	    printf( " %-9.9s ", buf );
	}
	else if (pers->loginat == 0)
	    printf(" < .  .  .  . >");
	else if (tloc - pers->loginat >= 180 * 24 * 60 * 60)
	    printf( " <%-6.6s, %-4.4s>", buf+4, buf+20 );
	else
	    printf(" <%-12.12s>", buf+4);
	if ( print_host ) {
	    printf( "  %-*.*s", HMAX, HMAX, pers->host );
	}
	if ( !print_host || !wide ) {
#ifdef HP
	    len = strlen( pers->officephone );
#else
	    len = strlen( pers->homephone );
#endif HP
	    if(  dialup  &&  (len > 0)  )  {
	        if( len == 8 )  {
		    printf( "             " );
	        }
	        else  {
		    if( len == 12 )  {
		        printf( "         " );
		    }
		    else {
		        for( i = 1; i <= 21 - len; i++ )  {
			    printf( " " );
		        }
		    }
	        }
	        printf( "%s", pers->homephone );
	    }
	    else  {
	        if(  strlen( pers->office ) > 0  )  {
		    printf( " %-11.11s", pers->office );
		    if(  strlen( pers->officephone ) > 0  )  {
		        printf( " %8.8s", pers->officephone );
		    }
		    else  {
		        if( len == 8 )  {
			    printf( " %8.8s", pers->homephone );
		        }
		    }
	        }
	        else  {
		    if(  strlen( pers->officephone ) > 0  )  {
		        printf( "             %8.8s", pers->officephone );
		    }
		    else  {
		        if( len == 8 )  {
			    printf( "             %8.8s", pers->homephone );
		        }
		        else  {
			    if( len == 12 )  {
			        printf( "         %12.12s", pers->homephone );
			    }
		        }
		    }
	        }
	    }
	}
	printf( "\n" );
}


/*  print out a person in long format giving all possible information.
 *  directory and shell are inhibited if unbrief is clear.
 */

personprint( pers )

    struct  person	*pers;
{
	struct  passwd		*pwdt = pers->pwd;
	int			idleprinted;

	if( pwdt == NILPWD )  {
	    printf( "Login name: %-10s", pers->name );
	    printf( "			" );
	    printf( "In real life: ???\n");
	    return;
	}
	printf( "Login name: %-10s", pwdt->pw_name );
	if( pers->loggedin )  {
	    if( pers->writeable )  {
		printf( "			" );
	    }
	    else  {
		printf( "	(messages off)	" );
	    }
	}
	else  {
	    printf( "			" );
	}
	if(  strlen( pers->realname ) > 0  )  {
	    printf( "In real life: %-s", pers->realname );
	}
	if(  strlen( pers->office ) > 0  )  {
#ifdef HP
	    printf( "\nBldg: %-.11s", pers->office );
#else
  	    printf( "\nOffice: %-.11s", pers->office );
#endif HP
	    if(  strlen( pers->officephone ) > 0  )  {
#ifndef HP
		printf( ", %s", pers->officephone );
#else 
		printf(", Work phone: %s", pers->officephone);
#endif HP
		if(  strlen( pers->homephone ) > 0  )  {
#ifndef HP
		    printf( "		Home phone: %s", pers->homephone );
#else
			printf(", Home phone: %s", pers->homephone);
#endif HP
		}
		else  {
		    if(  strlen( pers->random ) > 0  )  {
			printf( "	%s", pers->random );
		    }
		}
	    }
	    else  {
		if(  strlen( pers->homephone ) > 0  )  {
		    printf("			Home phone: %s",pers->homephone);
		}
		if(  strlen( pers->random ) > 0  )  {
		    printf( "			%s", pers->random );
		}
	    }
	}
	else  {
	    if(  strlen( pers->officephone ) > 0  )  {
#ifndef HP
		printf( "\nPhone: %s", pers->officephone );
#else
		printf( "\nWork phone: %s", pers->officephone );
#endif HP
		if(  strlen( pers->homephone ) > 0  )  {
#ifndef HP
		    printf( "\n, %s", pers->homephone );
#else
			printf("\nHome phone: %s", pers->homephone);
#endif HP
		    if(  strlen( pers->random ) > 0  )  {
			printf( ", %s", pers->random );
		    }
		}
		else  {
		    if(  strlen( pers->random ) > 0  )  {
			printf( "\n, %s", pers->random );
		    }
		}
	    }
	    else  {
		if(  strlen( pers->homephone ) > 0  )  {
		    printf( "\nPhone: %s", pers->homephone );
		    if(  strlen( pers->random ) > 0  )  {
			printf( ", %s", pers->random );
		    }
		}
		else  {
		    if(  strlen( pers->random ) > 0  )  {
			printf( "\n%s", pers->random );
		    }
		}
	    }
	}
	if( unbrief )  {
	    printf( "\n" );
	    printf( "Directory: %-25s", pwdt->pw_dir );
	    if(  strlen( pwdt->pw_shell ) > 0  )  {
		printf( "	Shell: %-s", pwdt->pw_shell );
	    }
	}
	if( pers->loggedin )  {
	    register char *ep = ctime( &pers->loginat );
	    if ( *pers->host != '\0' ) {
	        printf("\nOn since %15.15s on %s from %s\n", &ep[4],
				 pers->tty, pers->host );
	    }
	    else {
	        printf("\nOn since %15.15s on %-*.*s\t", &ep[4], LMAX, LMAX, pers->tty );
	    }
	    idleprinted = ltimeprint( &pers->idletime );
	    if( idleprinted )  {
		printf( " Idle Time" );
	    }
	}
	else if (pers->loginat == 0)
	    printf("\nNever logged in.");
	else if (tloc - pers->loginat > 180 * 24 * 60 * 60) {
	    register char *ep = ctime( &pers->loginat );
	    printf("\nLast login %10.10s, %4.4s on %.*s", ep, ep+20, LMAX, pers->tty);
	}
	else  {
	    register char *ep = ctime( &pers->loginat );
	    printf("\nLast login %16.16s on %.*s", ep, LMAX, pers->tty );
	}

	printf( "\n" );
}


/*
 *  very hacky section of code to format phone numbers.  filled with
 *  magic constants like 4, 7 and 10.
 */

char  *phone( s, len )

    char		*s;
    int			len;
{
	char		*strsave();
	char		fonebuf[ 15 ];
	int		i;

	switch(  len  )  {

	    case  4:
	    case  5:
#ifndef HP
		fonebuf[ 0 ] = ' ';
		fonebuf[ 1 ] = 'x';
		fonebuf[ 2 ] = '2';
		fonebuf[ 3 ] = '-';
		for( i = 0; i <= 3; i++ )  {
		    fonebuf[ 4 + i ] = *s++;
		}
		fonebuf[ 8 ] = NULL;
#else
		for ( i = 0 ; i <= len-1; i++) 
			fonebuf[i] = *s++;
		fonebuf[ i ] = NULL;
#endif HP
		return( strsave( &fonebuf[0] ) );
		break;

	    case  7:
		for( i = 0; i <= 2; i++ )  {
		    fonebuf[ i ] = *s++;
		}
		fonebuf[ 3 ] = '-';
		for( i = 0; i <= 3; i++ )  {
		    fonebuf[ 4 + i ] = *s++;
		}
		fonebuf[ 8 ] = NULL;
		return( strsave( &fonebuf[0] ) );
		break;
#ifdef HP
	    case 8:
		fonebuf[0] = *s++;
		fonebuf[1] = '-';
		for( i = 2; i <= 4; i++ ) 
			fonebuf[i] = *s++;
		fonebuf[i++] = '-';
		while ( i <= 9 )
			fonebuf[i++] = *s++;
		fonebuf[i] = NULL;
		return ( strsave( &fonebuf[0] ) );
		break;
#endif HP
	    case 10:
		for( i = 0; i <= 2; i++ )  {
		    fonebuf[ i ] = *s++;
		}
		fonebuf[ 3 ] = '-';
		for( i = 0; i <= 2; i++ )  {
		    fonebuf[ 4 + i ] = *s++;
		}
		fonebuf[ 7 ] = '-';
		for( i = 0; i <= 3; i++ )  {
		    fonebuf[ 8 + i ] = *s++;
		}
		fonebuf[ 12 ] = NULL;
		return( strsave( &fonebuf[0] ) );
		break;

	    default:
		fprintf( stderr, "finger: error in phone numbering\n" );
		return( strsave(s) );
		break;
	}
}


/*  decode the information in the gecos field of /etc/passwd
 *  another hacky section of code, but given the format the stuff is in...

	BUG FIX:

	buffer[ 40 ] changed to *buffer to prevent core dump when
	a gecos sub-field is a really long continuous string.

	5-4-89 Mark Notess

 */

decode( pers )

    struct  person	*pers;

{
	struct  passwd		*pwdt = pers->pwd;
	char			*buffer,  *bp,  *gp,  *lp;
	char			*phone();
	int			alldigits;
	int			len;
	int			i;

	pers->realname = NULLSTR;
	pers->office = NULLSTR;
	pers->officephone = NULLSTR;
	pers->homephone = NULLSTR;
	pers->random = NULLSTR;
	if(  pwdt != NILPWD )  {
							/* 5-4-89 mhn */
            buffer = (char *) malloc (strlen (pwdt->pw_gecos) + 2);

	    gp = pwdt->pw_gecos;
	    if(strncmp("pri=", gp, 4) == 0) {
		int i;
		
		i = 4;
		if(pwdt->pw_gecos[i] == '-')
		    i++;
		while(pwdt->pw_gecos[i] >= '0' && pwdt->pw_gecos[i] <= '9')
		    i++;
		gp += i;			/* skip pri=n if it's there */
	    }
	    bp = &buffer[ 0 ];
	    if( *gp == ASTERISK )  {
		gp++;
	    }
	    while(  (*gp != NULL)  &&  (*gp != COMMA)  )  {	/* name */
		if( *gp == SAMENAME )  {
		    lp = pwdt->pw_name;
		    *bp++ = CAPITALIZE(*lp++);
		    while( *lp != NULL )  {
			*bp++ = *lp++;
		    }
		}
		else  {
		    *bp++ = *gp;
		}
		gp++;
	    }
	    *bp = NULL;
	    pers->realname = malloc( strlen( &buffer[0] ) + 1 );
	    strcpy( pers->realname, &buffer[0] );
	    if( *gp++ == COMMA )  {			/* office, supposedly */
		alldigits = 1;
		bp = &buffer[ 0 ];
		while(  (*gp != NULL)  &&  (*gp != COMMA)  )  {
		    *bp = *gp++;
		    alldigits = alldigits && ('0' <= *bp) && (*bp <= '9');
		    bp++;
		}
		*bp = NULL;
		len = strlen( &buffer[0] );
#ifndef HP
		if( buffer[ len - 1 ]  ==  CORY )  {
		    strcpy( &buffer[ len - 1 ], " Cory" );
		    pers->office = malloc( len + 5 );
		    strcpy( pers->office, &buffer[0] );
		}
		else  {
		    if( buffer[ len - 1 ] == EVANS )  {
			strcpy( &buffer[ len - 1 ], " Evans" );
			pers->office = malloc( len + 6 );
			strcpy( pers->office, &buffer[0] );
		    }
		    else  {
			if( buffer[ len - 1 ] == 'L' )  {
			    strcpy( &buffer[ len - 1 ], " LBL" );
			    pers->office = malloc( len + 4 );
			    strcpy( pers->office, &buffer[0] );
			}
			else  {
			    if( alldigits )  {
				if( len == 4 )  {
				    pers->officephone = phone(&buffer[0], len);
				}
				else  {
				    if(  (len == 7) || (len == 10)  )  {
					pers->homephone = phone(&buffer[0],len);
				    }
				}
			    }
			    else  {
				pers->random = malloc( len + 1 );
				strcpy( pers->random, &buffer[0] );
			    }
			}
		    }
		}
#else 
		pers->office = malloc( strlen(buffer) + 1 );
		strcpy ( pers->office, &buffer[0] );
#endif HP
		if( *gp++ == COMMA )  {	    /* office phone, theoretically */
		    bp = &buffer[ 0 ];
		    alldigits = 1;
		    while(  (*gp != NULL)  &&  (*gp != COMMA)  )  {
			*bp = *gp++;
			alldigits = alldigits && ('0' <= *bp) && (*bp <= '9');
			bp++;
		    }
		    *bp = NULL;
		    len = strlen( &buffer[0] );
		    if( alldigits )  {
#ifdef HP
			pers->officephone = phone( &buffer[0], len );
#else
			if(  len != 4  )  {
			    if(  (len == 7) || (len == 10)  )  {
				pers->homephone = phone( &buffer[0], len );
			    }
			    else  {
				pers->random = malloc( len + 1 );
				strcpy( pers->random, &buffer[0] );
			    }
			}
			else  {
				pers->officephone = phone( &buffer[0], len );
			}
#endif HP
		    }
		    else  {
			pers->random = malloc( len + 1 );
			strcpy( pers->random, &buffer[0] );
		    }
		    if( *gp++ == COMMA )  {		/* home phone?? */
			bp = &buffer[ 0 ];
			alldigits = 1;
			    while(  (*gp != NULL)  &&  (*gp != COMMA)  )  {
				*bp = *gp++;
				alldigits = alldigits && ('0' <= *bp) &&
							(*bp <= '9');
				bp++;
			    }
			*bp = NULL;
			len = strlen( &buffer[0] );
#ifndef HP
			if( alldigits  &&  ( (len == 7) || (len == 10) )  )  {
#else
			if( alldigits &&
			   ( ( len == 7) || ( len == 8) || ( len == 10) ) ) {
#endif HP
#ifndef HP
			    if( *pers->homephone != NULL )  {
				pers->officephone = pers->homephone;
			    }
#endif HP
			    pers->homephone = phone( &buffer[0], len );
			}
			else  {
			    pers->random = malloc( strlen( &buffer[0] ) + 1 );
			    strcpy( pers->random, &buffer[0] );
			}
		    }
		}
	    }
	    if( pers->loggedin == 0 )  {
		findwhen( pers );
	    }
	    else  {
		findidle( pers );
	    }
	free (buffer);  /* 5-4-89 mhn */
	}
}


findwhen( pers )

    struct  person	*pers;
{
	struct  passwd		*pwdt = pers->pwd;
	struct  utmp		*bp, *lasttime();
	int			i;

	if (pwdt)
		bp=lasttime(pwdt->pw_name, 0);
	else
		bp=lasttime(pers->name, 0);
	if( bp != (struct utmp *)NULL )  {
	    for( i = 0; i < LMAX; i++ )  {
		pers->tty[ i ] = bp->ut_line[ i ];
	    }
	    pers->tty[ LMAX ] = NULL;
	    pers->loginat = bp->ut_time;
	}
        else  {
#ifndef HP
	    fprintf(stderr, "finger: lastlog read error\n");
#endif HP
	    pers->tty[ 0 ] = NULL;
	    pers->loginat = 0L;
	}
}


/*  find the idle time of a user by doing a stat on /dev/histty,
 *  where histty has been gotten from USERLOG, supposedly.
 */

findidle( pers )

    struct  person	*pers;
{
	struct  stat		ttystatus;
	struct  passwd		*pwdt = pers->pwd;
	char			buffer[ 20 ];
	char			*TTY = "/dev/";
	int			TTYLEN = strlen( TTY );
	int			i;

	strcpy( &buffer[0], TTY );
	i = 0;
	do  {
	    buffer[ TTYLEN + i ] = pers->tty[ i ];
	}  while( ++i <= LMAX );
	if(  stat( &buffer[0], &ttystatus ) >= 0  )  {
	    time( &tloc );
	    if( tloc < ttystatus.st_atime )  {
		pers->idletime = 0L;
	    }
	    else  {
		pers->idletime = tloc - ttystatus.st_atime;
	    }
	    if(  (ttystatus.st_mode & TALKABLE) == TALKABLE  )  {
		pers->writeable = 1;
	    }
	    else  {
		pers->writeable = 0;
	    }
	}
	else  {
	    fprintf( stderr, "finger: error STATing %s\n", &buffer[0] );
	    exit( 4 );
	}
}


/*  print idle time in short format; this program always prints 4 characters;
 *  if the idle time is zero, it prints 4 blanks.
 */

stimeprint( dt )

    long	*dt;
{
	struct  tm		*gmtime();
	struct  tm		*delta;

	delta = gmtime( dt );
	if( delta->tm_yday == 0 )  {
	    if( delta->tm_hour == 0 )  {
		if( delta->tm_min >= 10 )  {
		    printf( " %2.2d ", delta->tm_min );
		}
		else  {
		    if( delta->tm_min == 0 )  {
			printf( "    " );
		    }
		    else  {
			printf( "  %1.1d ", delta->tm_min );
		    }
		}
	    }
	    else  {
		if( delta->tm_hour >= 10 )  {
		    printf( "%3.3d:", delta->tm_hour );
		}
		else  {
		    printf( "%1.1d:%02.2d", delta->tm_hour, delta->tm_min );
		}
	    }
	}
	else  {
	    printf( "%3dd", delta->tm_yday );
	}
}


/*  print idle time in long format with care being taken not to pluralize
 *  1 minutes or 1 hours or 1 days.
 */

ltimeprint( dt )

    long	*dt;
{
	struct  tm		*gmtime();
	struct  tm		*delta;
	int			printed = 1;

	delta = gmtime( dt );
	if( delta->tm_yday == 0 )  {
	    if( delta->tm_hour == 0 )  {
		if( delta->tm_min >= 10 )  {
		    printf( "%2d minutes", delta->tm_min );
		}
		else  {
		    if( delta->tm_min == 0 )  {
			if( delta->tm_sec > 10 )  {
			    printf( "%2d seconds", delta->tm_sec );
			}
			else  {
			    printed = 0;
			}
		    }
		    else  {
			if( delta->tm_min == 1 )  {
			    if( delta->tm_sec == 1 )  {
				printf( "%1d minute %1d second",
				    delta->tm_min, delta->tm_sec );
			    }
			    else  {
				printf( "%1d minute %d seconds",
				    delta->tm_min, delta->tm_sec );
			    }
			}
			else  {
			    if( delta->tm_sec == 1 )  {
				printf( "%1d minutes %1d second",
				    delta->tm_min, delta->tm_sec );
			    }
			    else  {
				printf( "%1d minutes %d seconds",
				    delta->tm_min, delta->tm_sec );
			    }
			}
		    }
		}
	    }
	    else  {
		if( delta->tm_hour >= 10 )  {
		    printf( "%2d hours", delta->tm_hour );
		}
		else  {
		    if( delta->tm_hour == 1 )  {
			if( delta->tm_min == 1 )  {
			    printf( "%1d hour %1d minute",
				delta->tm_hour, delta->tm_min );
			}
			else  {
			    printf( "%1d hour %2d minutes",
				delta->tm_hour, delta->tm_min );
			}
		    }
		    else  {
			if( delta->tm_min == 1 )  {
			    printf( "%1d hours %1d minute",
				delta->tm_hour, delta->tm_min );
			}
			else  {
			    printf( "%1d hours %2d minutes",
				delta->tm_hour, delta->tm_min );
			}
		    }
		}
	    }
	}
	else  {
		if( delta->tm_yday >= 10 )  {
		    printf( "%2d days", delta->tm_yday );
		}
		else  {
		    if( delta->tm_yday == 1 )  {
			if( delta->tm_hour == 1 )  {
			    printf( "%1d day %1d hour",
				delta->tm_yday, delta->tm_hour );
			}
			else  {
			    printf( "%1d day %2d hours",
				delta->tm_yday, delta->tm_hour );
			}
		    }
		    else  {
			if( delta->tm_hour == 1 )  {
			    printf( "%1d days %1d hour",
				delta->tm_yday, delta->tm_hour );
			}
			else  {
			    printf( "%1d days %2d hours",
				delta->tm_yday, delta->tm_hour );
			}
		    }
		}
	}
	return( printed );
}


/*****************************************************************************
	BUG FIX:

	buffer[ 20 ] changed to *buffer to prevent core dump when
	a gecos sub-field is a really long continuous string.

	5-4-89 Mark Notess
*****************************************************************************/

matchcmp( gname, login, given )

    char		*gname;
    char		*login;
    char		*given;
{
	char		*buffer;
	char		c;
	int		flag,  i,  unfound;

/*
printf("(MATCHCMP) gecos:%s  pw_name:%s  given:%s\n",gname, login, given );
*/
	if( !match )  {
	    return( 0 );
	}
	else  {
	    if(  namecmp( login, given )  )  {
		return( 1 );
	    }
	    else if (*gname == '\0')
		return (0);
	    else  {
		if( *gname == ASTERISK )  {
		    gname++;
		}
		flag = 1;
		i = 0;
		unfound = 1;
							/* 5-4-89 mhn */
		buffer = (char *) malloc (strlen (gname) + 2);

		while( unfound && *gname )  {		/* 4-17-90 ec */
		    if( flag )  {
			c = *gname++;
			if( c == SAMENAME )  {
			    flag = 0;
			    c = *login++;
			}
			else  {
			    unfound = (*gname != COMMA)  &&  (*gname != NULL);
			}
		    }
		    else {
			c = *login++;
			if( c == NULL )  {
			    if(  (*gname == COMMA)  ||  (*gname == NULL)  )  {
				break;
			    }
			    else  {
				flag = 1;
				continue;
			    }
			}
		    }
		    if( c == BLANK )  {
			buffer[i++] = NULL;
			if(  namecmp( buffer, given )  )  {
			    free (buffer);  /* 5-4-89 mhn*/
			    return( 1 );
			}
			i = 0;
			flag = 1;
		    }
		    else  {
			buffer[ i++ ] = c;
		    }
		}
		buffer[i++] = NULL;
		if(  namecmp( buffer, given )  )  {
		    free (buffer);  /* 5-4-89 mhn*/
		    return( 1 );
		}
		else  {
		    free (buffer);  /* 5-4-89 mhn*/
		    return( 0 );
		}
	    }
	}
}


namecmp( name1, name2 )

    char		*name1;
    char		*name2;
{
	char		c1,  c2;

/*
printf("(NAMECMP)  name1:%s  name2:%s\n",name1,name2);
*/
	c1 = *name1;
	if( (('A' <= c1) && (c1 <= 'Z')) || (('a' <= c1) && (c1 <= 'z')) )  {
	    c1 = CAPITALIZE( c1 );
	}
	c2 = *name2;
	if( (('A' <= c2) && (c2 <= 'Z')) || (('a' <= c2) && (c2 <= 'z')) )  {
	    c2 = CAPITALIZE( c2 );
	}
	while( c1 == c2 )  {
	    if( c1 == NULL )  {
		return( 1 );
	    }
	    c1 = *++name1;
	    if( (('A'<=c1) && (c1<='Z')) || (('a'<=c1) && (c1<='z')) )  {
		c1 = CAPITALIZE( c1 );
	    }
	    c2 = *++name2;
	    if( (('A'<=c2) && (c2<='Z')) || (('a'<=c2) && (c2<='z')) )  {
		c2 = CAPITALIZE( c2 );
	    }
	}
	if( *name1 == NULL )  {
	    while(  ('0' <= *name2)  &&  (*name2 <= '9')  )  {
		name2++;
	    }
	    if( *name2 == NULL )  {
		return( 1 );
	    }
	}
	else  {
	    if( *name2 == NULL )  {
		while(  ('0' <= *name1)  &&  (*name1 <= '9')  )  {
		    name1++;
		}
		if( *name1 == NULL )  {
		    return( 1 );
		}
	    }
	}
	return( 0 );
}


char  *strsave( s )

    char		*s;
{
	char		*malloc();
	char		*p;

	p = malloc( strlen( s ) + 1 );
	return strcpy( p, s );
}

#ifdef TRUX
/*
 * This routine is a modified version of SecureWare's write_clean_output()
 * routine which is used by write(1M).
 *
 * This routine insures that no escape sequences or control characters
 * are output. The output looks like:
 *	^[	the escape character
 *	^?	the delete character
 *	M-x	the x character has the 0200 bit on
 *	^x	the character x&037
 *
 * This routine protects against someone putting control characters in
 * their .plan or .project file which could do things like transmit a
 * "send" character to the "finger" user's screen, effectively causing
 * an arbitrary character sequence to be transmitted from the user's
 * terminal.
 */
void
putc_filter(c, fp)
char c;
FILE *fp;
{
	if (!isascii((int) c))  {
		(void) fputs("M-", fp);
		c = (char) toascii(c);
	}
	switch (c)  {
		case '\n':
		case '\t':
		case '\007': /* bell char */
			(void) fputc((int) c, fp);
			break;
		case '\033': /* escape char */
			(void) fputs("^[", fp);
			break;
		case '\177': /* delete char */
			(void) fputs("^?", fp);
			break;
		default:
			if (iscntrl((int) c))  {
				(void) fputc((int) '^', fp);
				c |= 0100;
			}
			(void) fputc((int) c, fp);
			break;
	}
}
#endif TRUX
