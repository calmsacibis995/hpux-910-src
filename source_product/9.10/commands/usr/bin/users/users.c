
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 70.1 $";
#endif
/* $Header: users.c,v 70.1 93/05/24 13:45:01 ssa Exp $ (Hewlett-Packard) */

/*
 *	users(UTIL)
 *	
 *	Note: The synopsis handled by the code is:
 *		users [-c] [file]
 *
 *	      The man page currently neglects to mention the optional
 *	      file parameter.
 */

char	*malloc();

#include <stdio.h>
#ifdef hpux
#include <sys/types.h>
#endif
#include <utmp.h>
#include <ndir.h>
#include <string.h>

#ifdef NLS || NLS16
#include <locale.h>
#endif NLS || NLS16

#define EQ(x,y)		(strcmp(x,y)==0)
#define NMAX sizeof(utmp.ut_name)
#define LMAX sizeof(utmp.ut_line)
#define UMAX 1024	/* amount of array space for user names */

struct utmp utmp;
char *myname;

main(argc, argv)
int argc;
char **argv;
{
	register FILE	*fi;
	int	cflag = 0;
	char	*p;
	char	s[40], t[40];
	DIR	*dirp;
	DIR	*opendir();
	struct direct *d;

#ifdef NLS || NLS16			/* initialize to the correct locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("users"), stderr);
		putenv("LANG=");
	}
#endif NLS || NLS16

	myname = *argv;

	switch(argc) {
	  case 1:
		strcpy(s, "/etc/utmp");
		break;
#if defined(DUX) || defined(DISKLESS)
	  case 2:
		if ( EQ(argv[1], "-c") ) {
			++cflag;
			strcpy(s, "/etc/utmp");
		}
		else
			strcpy(s, argv[1]);
		break;
#endif
	  default:
#if defined(DUX) || defined(DISKLESS)
		if ( EQ(argv[1], "-c") ) {
			++cflag;
			strcpy(s, argv[2]);
		}
		else
			strcpy(s, argv[1]);
#else
		strcpy(s, argv[1]);
#endif
		break;
	}

	if ( cflag ) {

		/* Remove trailing '/' from path name */
		while ( (p=strrchr(s, '/')) != NULL  && *(p+1) == '\0' )
			*p = '\0';

		/* Attempt to open the file as a hidden directory */
		strcat(s, "+");
		if ( (dirp=opendir(s)) == NULL ) { 
			perror(s);
			exit(1);
		}

		/* Gather user information for each CDF element */
		strcat(s, "/");
		while ((d=readdir(dirp)) != NULL ) {
			if ( (d->d_ino==0) || (EQ(d->d_name, ".")) || (EQ(d->d_name, "..")) )
		     		continue;
			strcpy(t, s);
			strcat(t, d->d_name);
			if ((fi = fopen(t, "r")) == NULL) {
				perror(t);
				exit(1);
			}
			users(fi);
		}
	}

	/* Here if no cluster wide information needed */
	else {
		if ((fi = fopen(s, "r")) == NULL) {
			perror(s);
			exit(1);
		}
		users(fi);
	}
	
	summary();
	return(0);
}

users(fi)
FILE *fi;
{
	static int i=0;

	while (fread((char *)&utmp, sizeof(utmp), 1, fi) == 1) {
#ifdef hpux
		if((utmp.ut_name[0] == '\0')||(utmp.ut_type != USER_PROCESS))
#else
		if(utmp.ut_name[0] == '\0')
#endif
			continue;
		else if ( ++i > UMAX ) {
		    fputs(myname, stdout);
		    fputs(": Out of table space\n", stdout);
		    exit(1);
		}
		putline();
	}
	fclose(fi);
	return(0);
}

char	*names[UMAX];
char	**namp = names;
putline()
{
	char temp[NMAX+1];
	(void) strncpy(temp, utmp.ut_name, NMAX);
	temp[NMAX] = 0;
	*namp = malloc(strlen(temp) + 1);
	(void) strcpy(*namp++, temp);
}

scmp(p, q)
char **p, **q;
{
	return(strcoll(*p, *q));
}

summary()
{
	register char **p;

	qsort(names, namp - names, sizeof names[0], scmp);
	for (p=names; p < namp; p++) {
		if (p != names)
			fputc(' ', stdout);
		fputs(*p, stdout);
	}
	fputc('\n', stdout);
}
