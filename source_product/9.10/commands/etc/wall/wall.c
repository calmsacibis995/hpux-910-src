static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#define	FAILURE	(-1)
#define	TRUE	1
#define	FALSE	0

#include	<signal.h>

char	mesg[3000];

#include <sys/types.h>
#include <utmp.h>
#include <grp.h>
#include <sys/stat.h>
#include <ndir.h>
#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#ifdef SecureWare
#include <sys/security.h>
#endif

struct utmp *utmp;
char	*line = "???";
char	who[9]	= "???";
char	*infile;
int	group;
struct	group *pgrp;
extern struct group *getgrnam();
extern char *ttyname();
char *grpname;
char	*timstr;
long	tloc, time();
extern int errno;

struct utmp *getutline();
struct utmp *getutent();
void setutent();

#define equal(a,b)		(!strcmp( (a), (b) ))

main(argc, argv)
int argc;
char *argv[];
{
	int i;
	FILE *f;
	struct utmp utmp;
	struct utmp *utmpp;
	struct stat statbuf;
	register char *ptr;
	struct passwd *pwd;
	extern struct passwd *getpwuid();
	char *malloc();

#ifdef SecureWare
	if (ISSECURE)  {
		set_auth_parameters(argc, argv);

#ifdef B1
		if (ISB1) {
			initprivs();
			(void) forcepriv(SEC_ALLOWDACACCESS);
			(void) forcepriv(SEC_ALLOWMACACCESS);
			(void) forcepriv(SEC_LIMIT);
#if SEC_ILB
			(void) forcepriv(SEC_ILNOFLOAT);
#endif
		} 
#endif

		if (!authorized_user("terminal"))  {
			fprintf(stderr, "wall: no authorization to use wall\n");
			exit (1);
		}
   }
#endif
	readargs(argc, argv);

/*	Get the name of the terminal wall is running from and search	*/
/*	for self so that we can identify ourselves when we send.	*/

	if ( (line=ttyname(fileno(stderr))) != (char *)0 ) {

	    /* Skip "/dev/" (i.e. first 5 characters) */
	    line = &line[5];

	    /* Search for "line" entry in utmp: */
	    strcpy(utmp.ut_line, line);
	    if ( (utmpp=getutline(&utmp)) != (struct utmp *)0 )
		strncpy(who, utmpp->ut_user, sizeof(utmpp->ut_user));
	    else {
		/* If we didn't find the utmp entry, reset line to "???" */
		strcpy(&who[0],"???");
		line = "???";
	    }
	} else if (pwd = getpwuid(getuid()))
		strncpy(&who[0],pwd->pw_name,sizeof(who));

	f = stdin;
	if(infile) {
		f = fopen(infile, "r");
		if(f == NULL) {
			printf("%s??\n", infile);
			exit(1);
		}
	}
	for(ptr= &mesg[0]; fgets(ptr,&mesg[sizeof(mesg)]-ptr, f) != NULL
		; ptr += strlen(ptr))
	    strcat(mesg, "\r");
	fclose(f);
	time(&tloc);
	timstr = ctime(&tloc);
	for ((void)setutent(); (utmpp=getutent()) != (struct utmp *)0;) {
	    if ( utmpp->ut_type != USER_PROCESS )
		continue;
	    sendmes(utmpp);
	}
	alarm(60);
	do {
		i = wait((int *)0);
	} while(i != -1 || errno != ECHILD);
	exit(0);
}

sendmes(p)
struct utmp *p;
{
	register i;
	register char *s;
	static char device[] = "/dev/123456789012";
	FILE *f;

	if(group)
		if(!chkgrp(p->ut_user,sizeof(p->ut_user)))
			return;
	while((i=fork()) == -1) {
		perror("forkfailed");
		alarm(60);
		wait((int *)0);
		alarm(0);
	}

	if(i)
		return;

	signal(SIGHUP, SIG_IGN);
	alarm(60);
	s = &device[0];
	sprintf(s,"/dev/%s",&p->ut_line[0]);
#ifdef DEBUG
	f = fopen("wall.debug", "a");
#else
	f = fopen(s, "w");
#endif
	if(f == NULL) {
		printf("Cannot send to %.-8s\n", &p->ut_user[0]);
		perror("open");
		exit(1);
	}

	fprintf(f, "\r\n\07\07\07Broadcast Message from %s (%s) %19.19s",
		 who, line, timstr);
	if(group)
		fprintf(f, " to group %s", grpname);
	fprintf(f, "...\r\n");
#ifdef DEBUG
	fprintf(f,"DEBUG: To %.8s on %s\n", p->ut_user, s);
#endif
	fprintf(f, "%s\r\n", mesg);
	fclose(f);
	exit(0);
}

readargs(ac, av)
int ac;
char **av;
{
	register int i, j;

	for(i = 1; i < ac; i++) {
		if(equal(av[i], "-g")) {
			if(group) {
				fprintf(stderr, "Only one group allowed\n");
				exit(1);
			}
			i++;
			if (i == ac) {
				fprintf(stderr, "Group name missing\n");
				exit(1);
			}
			if((pgrp=getgrnam(grpname= av[i])) == NULL) {
				fprintf(stderr, "Unknown group %s\n", grpname);
				exit(1);
			}
			endgrent();
			group++;
		}
		else
			infile = av[i];
	}
}
#define BLANK		' '
chkgrp(name, len)
register char *name;
int len;
{
	register int i;
	register char *p;

	for(i = 0; pgrp->gr_mem[i] && pgrp->gr_mem[i][0]; i++) {
		for(p=name; *p && *p != BLANK; p++);
		*p = 0;
/*
                if(equal(name, pgrp->gr_mem[i]))
   Changed the above check to a strncmp() call since ut_user field is
   only 8 characters (see utmp.h), and if user name is 8 or more characters,
   then ut_user is not null terminated (since it runs into the next field
   of the struct, namely ut_id). This results in defect DSDe408346.
*/
		if( !strncmp(name, pgrp->gr_mem[i], len) )
			return(1);
	}

	return(0);
}
