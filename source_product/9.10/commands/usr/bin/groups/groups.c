static char *HPUX_ID = "@(#) $Revision: 64.4 $";

/*
 * groups
 */

/* groups command based on Berkeley modified for HPUX */ 

#include <sys/param.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>

#ifdef NLS || NLS16
#include <locale.h>
#endif NLS || NLS16

int	groups[NGROUPS];
int 	lflg,pflg,gflg = 0;

#define LINES 100
#define MAXLEN 1000
#define NULLSTRING ""

char *lineptr[LINES];
int nlines = 0;
struct group *getlgrent();
extern char *ltoa();

/*
 * user_name() --
 *    return a string representation of the user's name.
 *    Uses getlogin(), but if that fails we use getpwuid(real_user_id).
 */
char *
user_name()
{
    extern char *getlogin();
    extern struct passwd *getpwuid();
    extern char *strcpy();

    char *s;
    uid_t id;
    struct passwd *pw;
    static char buf[L_cuserid];

    if ((s = getlogin()) != NULL)
	return strcpy(buf, s);
    
    if ((pw = getpwuid(id = getuid())) != NULL)
	return strcpy(buf, pw->pw_name);

    buf[0] = '\0';
    return buf;
}

main(argc, argv)
	int argc;
	char *argv[];
{
	int c = 0;
	int opterr = 0;
	extern int optind;
	int ngroups, i;
	char *sep = "";
	struct group *gr;

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("groups"), stderr);
		putenv("LANG=");
	}
#endif NLS || NLS16

	if (argc > 1) {
		while (( c=getopt(argc, argv, "lgp")) != EOF) switch(c) {
			case 'l':
				lflg++;
				continue;
			case 'g':
				gflg++;
				continue;
			case 'p':
				pflg++;
				continue;
			case '?':
				opterr++;
				continue;
			}
		if(opterr) {
			fputs("usage: groups [-l] [-g] [-p] [user]\n", stderr);
			exit(1);
		} 

		if (strcmp(argv[optind], NULLSTRING))
		        showgroups(argv[optind]);
		else
			showgroups(user_name());
	}
	ngroups = getgroups(NGROUPS, groups);
	for (i = 0; i < ngroups; i++) {
		gr = getgrgid(groups[i]);
		if (gr == NULL) {
			fputs(sep, stdout);
			fputs(ltoa(groups[i]), stdout);
		}
		else {
			fputs(sep, stdout);
			fputs(gr->gr_name, stdout);
		}
		sep = " ";
	}
	fputc('\n', stdout);
	exit(0);
}

showgroups(user)
	register char *user;
{
	register struct group *gr;
	register struct passwd *pw;
	struct passwd *getpwnam(); /* added this line for spectrum*/
	register char **cp;
	char *sep = "";
	int maxlines = 100 ;

	if ((pw = getpwnam(user)) == NULL) {
		fputs("No such user\n", stderr);
		exit(1);
	}
	while (gr = getgrent()) {
		if (pw->pw_gid == gr->gr_gid) {
			if( (pflg == 1 ) ||
			    (pflg == 0 && gflg == 0 && lflg == 0) )  {
			    savegroupname(gr->gr_name,lineptr,maxlines);
			}
		}	
		for (cp = gr->gr_mem; cp && *cp; cp++)
			if (strcmp(*cp, user) == 0) {
				if( (gflg == 1 ) ||  
				    (gflg == 0 && pflg == 0 && lflg == 0) ) {
				    savegroupname(gr->gr_name,lineptr,maxlines);
				    break;
				}
			}
	}
	while (gr = getlgrent()) {
		for (cp = gr->gr_mem; cp && *cp; cp++)
			if (strcmp(*cp, user) == 0) {
				if( (lflg == 1 ) ||  
				    (gflg == 0 && pflg == 0 && lflg == 0) ) {
				    savegroupname(gr->gr_name,lineptr,maxlines);
				    break;
				}
			}
	}
	sort(lineptr,nlines); 
	writelines(lineptr,nlines);
	fputc('\n', stdout);
	exit(0);
}

savegroupname(gr,lineptr,maxlines)
char *gr;
char *lineptr[];
int maxlines;
{
	int len;
	int same,j;
	char *p, *malloc(), line[MAXLEN];

	strcpy(line,gr);
	len = strlen( line);
		if (nlines >= maxlines)
			return(-1);
		else if (( p = malloc(len + 1 )) == NULL)
			return(-1);
		else {
			line[len+1] = '\0';
			same = 0;
			for ( j = 0; j < nlines;  j++ ) {
				if( strcmp( lineptr[j] , line) == 0 ) {
					same = 1;
					break;
				}
			}
			if ( !same ) {
				strcpy(p, line);
				lineptr[nlines++] = p;
			}
		}
	return(nlines);
}

writelines(lineptr, nlines)
char *lineptr[];
int nlines;
{
	int i;
	char *sep = "";

	for (i = 0; i < nlines; i++) {
		fputs(sep, stdout);
		fputs(lineptr[i], stdout);
		sep = " ";
	}

}
sort(v, n)
char *v[];
{
	int gap, i, j;
	char *temp;

	for (gap = n/2; gap > 0; gap /=2)
		for (i = gap; i < n; i++)
			for (j = i - gap; j >= 0; j -= gap) {
				if (strcoll(v[j], v[j+gap]) <= 0)
					break;
				temp = v[j];
				v[j] = v[j+gap];
				v[j+gap] = temp;
			}
}

#define	CL	':'
#define	CM	','
#define	NL	'\n'
#define	MAXGRP	200

extern int atoi(), fclose();
extern char *fgets();
extern FILE *fopen();
extern void rewind();

static char GROUP[] = "/etc/logingroup";
static FILE *grf = NULL;
static char line[BUFSIZ+1];
static struct group grp;
static char *gr_mem[MAXGRP];

void
setlgrent()
{
	if(grf == NULL)
		grf = fopen(GROUP, "r");
	else
		rewind(grf);
}

void
endlgrent()
{
	if(grf != NULL) {
		(void) fclose(grf);
		grf = NULL;
	}
}

static char *
grskip(p, c)
register char *p;
register int c;
{
	while(*p != '\0' && *p != c)
		++p;
	if(*p != '\0')
	 	*p++ = '\0';
	return(p);
}

struct group *
getlgrent()
{
	extern struct group *fgetlgrent();

	if(grf == NULL && (grf = fopen(GROUP, "r")) == NULL)
		return(NULL);
	return (fgetlgrent(grf));
}

struct group *
fgetlgrent(f)
FILE *f;
{
	register char *p, **q;

	if((p = fgets(line, BUFSIZ, f)) == NULL) {
		f->_flag |= _IOEOF;
		return(NULL);
	}
	grp.gr_name = p;
	grp.gr_passwd = p = grskip(p, CL);
	grp.gr_gid = atoi(p = grskip(p, CL));
	grp.gr_mem = gr_mem;
	p = grskip(p, CL);
	(void) grskip(p, NL);
	q = gr_mem;
	while(*p != '\0') {
		if( q <= &gr_mem[MAXGRP - 1] )   
			*q++ = p;
		p = grskip(p, CM);
	}
	*q = NULL;
	return(&grp);
}
