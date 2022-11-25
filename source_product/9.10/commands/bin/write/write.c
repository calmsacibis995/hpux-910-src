static char *HPUX_ID = "@(#) $Revision: 66.6 $";

/*	Program to communicate with other users of the system.		*/
/*	Usage:	write user [line]					*/

#include	<stdio.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<utmp.h>
#include	<pwd.h>
#include	<fcntl.h>
#ifdef NLS
#define NL_SETN 1	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_types.h>
#include <locale.h>
nl_catd nlmsg_fd;
nl_catd nlmsg_tfd;
#else
#define catgets(i, sn,mn,s) (s)
#endif NLS
#ifdef SecureWare
#include        <sys/security.h>
#include	<sys/audit.h>
#endif /* SecureWare */

#define		TRUE	1
#define		FALSE	0
#define		FAILURE	-1

FILE	*fp ;	/* File pointer for receipient's terminal */
char *rterm,*receipient ;	/* Pointer to receipient's terminal and name */

#ifdef SecureWare
char    auditbuf[80];
#endif

main(argc,argv)

int argc ;
char **argv ;

  {
	register int i ;
	register struct utmp *ubuf ;
	static struct utmp self ;
	char ownname[sizeof(self.ut_user) + 1] ;
	static char rterminal[] = "/dev/\0 2345678901";
	extern char *rterm,*receipient ;
	char *terminal,*ownterminal, *oterminal,*ttyname() ;
	short count ;
	extern FILE *fp ;
	extern int openfail(),eof() ;
	char input[134] ;
	register char *ptr ;
	long tod ;
	char *time_of_day ;
	struct passwd *passptr ;
	extern struct utmp *getutent() ;
	extern struct passwd *getpwuid() ;
#ifndef NLS
	extern char *ctime() ;
#else NLS
	char *nl_cxtime();	 	/* print date in the user's language */
#endif NLS
	char badterm[20][20];
	register int bad = 0;

/*	Set "rterm" to location where receipient's terminal will go.	*/

#ifdef NLS || NLS16			/* initialize to the current locale */
	unsigned char lctime[5+4*MAXLNAME+4], *pc;
	unsigned char savelang[5+MAXLNAME+1];

	if (!setlocale(LC_ALL, "")) {		/* setlocale fails */
		fputs(_errlocale("write"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)-1;		/* use default messages */
		nlmsg_tfd = (nl_catd)-1;
	} else {				/* setlocale succeeds */
		nlmsg_fd = catopen("write", 0);	/* use $LANG messages */
		strcpy(lctime, "LANG=");	/* $LC_TIME affects some msgs */
		strcat(lctime, getenv("LC_TIME"));
		if (lctime[5] != '\0') {	/* if $LC_TIME is set */
			strcpy(savelang, "LANG=");	/* save $LANG */
			strcat(savelang, getenv("LANG"));
			if ((pc = strchr(lctime, '@')) != NULL) /*if modifier*/
				*pc = '\0';	/* remove modifer part */
			putenv(lctime);		/* use $LC_TIME for some msgs */
			nlmsg_tfd = catopen("write", 0);
			putenv(savelang);	/* reset $LANG */
		} else				/* $LC_TIME is not set */
			nlmsg_tfd = nlmsg_fd;	/* use $LANG messages */
	}
#endif NLS || NLS16

#ifdef SecureWare
	if(ISSECURE)
            set_auth_parameters(argc, argv);
#ifdef B1
	if(ISB1){
            initprivs();

            (void) enablepriv(SEC_ALLOWMACACCESS);
            (void) enablepriv(SEC_OWNER);
            (void) enablepriv(SEC_WRITEUPCLEARANCE);
            (void) enablepriv(SEC_WRITEUPSYSHI);
            (void) enablepriv(SEC_ALLOWDACACCESS);
	}
#endif
#endif
	rterm = &rterminal[sizeof("/dev/") - 1] ;
	terminal = NULL ;

	if (--argc <= 0)
	  {
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1,"Usage: write user [terminal]\n"))) ;
	    exit(1) ;
	  }
	else
	  {
	    receipient = *++argv ;
	  }

/*	Was a terminal name supplied?  If so, save it.			*/

	if (--argc > 1)
	  {
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,2,"Usage: write user [terminal]\n"))) ;
	    exit(1) ;
	  }
	else terminal = *++argv ;

/*	One of the standard file descriptors must be attached to a	*/
/*	terminal in "/dev".						*/

	if ((ownterminal = ttyname(fileno(stdin))) == NULL &&
	    (ownterminal = ttyname(fileno(stdout))) == NULL &&
	    (ownterminal = ttyname(fileno(stderr))) == NULL)
	  {
#ifdef SecureWare
	    if (ISSECURE)
                audit_subsystem("unable to determine terminal name",
                                "no reply possible",  ET_SUBSYSTEM);
#endif
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,3,"I cannot determine your terminal name. No reply possible.\n"))) ;
	    ownterminal = "/dev/???" ;
	  }

/*	Set "ownterminal" past the "/dev/" at the beginning of		*/
/*	the device name.						*/

	oterminal = ownterminal + sizeof("/dev/")-1 ;

/*	Scan through the "utmp" file for your own entry and the		*/
/*	entry for the person we want to send to.			*/

	for (self.ut_pid=0,count=0;(ubuf = getutent()) != NULL;)
	  {
/*	Is this a USER_PROCESS entry?					*/

	    if (ubuf->ut_type == USER_PROCESS)
	      {
/*	Is it our entry?  (ie.  The line matches ours?)			*/

		if (strncmp(&ubuf->ut_line[0],oterminal,
		    sizeof(ubuf->ut_line)) == 0) self = *ubuf ;

/*	Is this the person we want to send to?				*/

		if (strncmp(receipient,&ubuf->ut_user[0],
		    sizeof(ubuf->ut_user)) == 0)
		  {
/*	If a terminal name was supplied, is this login at the correct	*/
/*	terminal?  If not, ignore.  If it is right place, copy over the	*/
/*	name.								*/

		    if (terminal != NULL)
		      {
			if (strncmp(terminal,&ubuf->ut_line[0],
			    sizeof(ubuf->ut_line)) == 0)
			  {
			    strncpy(rterm,&ubuf->ut_line[0],
				sizeof(ubuf->ut_line)+1) ;
			    if (!permit(rterminal)) {
				bad++;
				rterm[0] = '\0';
			    }
			  }
		      }

/*	If no terminal was supplied, then take this terminal if no	*/
/*	other terminal has been encountered already.			*/

		    else
		      {
/*	If this is the first encounter, copy the string into		*/
/*	"rterminal".							*/

			if (*rterm == '\0')
			  {
			    strncpy(rterm,
			    &ubuf->ut_line[0],sizeof(ubuf->ut_line)+1) ;
			    if (!permit(rterminal)) {
				strcpy(badterm[bad++],rterm);
				rterm[0] = '\0';
			    }
			    else if (bad > 0)
			      {
#ifndef NLS
				fprintf(stderr, "%s is logged on more than one place.\nYou are connected to \"%s\".\nOther locations are:\n", receipient,rterm) ;
#else NLS
				fprintmsg(stderr, (catgets(nlmsg_fd,NL_SETN,4,"%1$s is logged on more than one place.\nYou are connected to \"%2$s\".\nOther locations are:\n")),
				    receipient,rterm) ;
#endif NLS
				for (i = 0 ; i < bad ; i++)
				    fprintf(stderr,"%s\n",badterm[i]) ;
			      }
			}

/*	If this is the second terminal, print out the first.  In all	*/
/*	cases of multiple terminals, list out all the other terminals	*/
/*	so the user can restart knowing what her/his choices are.	*/

			else if (terminal == NULL)
			  {
			    if (count == 1 && bad == 0)
			      {
#ifndef NLS
				fprintf(stderr,"%s is logged on more than one place.\nYou are connected to \"%s\".\nOther locations are:\n",
				    receipient,rterm) ;
#else NLS
				fprintmsg(stderr, (catgets(nlmsg_fd,NL_SETN,5,"%1$s is logged on more than one place.\nYou are connected to \"%2$s\".\nOther locations are:\n")),
				    receipient,rterm) ;
#endif NLS
			      }
			    fwrite(&ubuf->ut_line[0],sizeof(ubuf->ut_line),
				1,stderr) ;
			    fprintf(stderr,"\n") ;
			  }

			count++ ;
		      }			/* End of "else" */
		  }			/* End of "else if (strncmp" */
	      }			/* End of "if (USER_PROCESS" */
	  }		/* End of "for(count=0" */

/*	Did we find a place to talk to?  If we were looking for a	*/
/*	specific spot and didn't find it, complain and quit.		*/

	if (terminal != NULL && *rterm == '\0')
	  {
	    if (bad > 0) {

#ifdef SecureWare
		if (ISSECURE) {
                	sprintf(auditbuf, "write to user %s at terminal %s",
                            receipient, terminal);
                	audit_subsystem(auditbuf,
			     "permission denied, write  aborted",ET_SUBSYSTEM);
		}
#endif
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,6,"Permission denied.\n"))) ;
            }
	    else
            {
#ifdef SecureWare
                sprintf(auditbuf, "write to user %s at terminal %s",
                        receipient, terminal);
                audit_subsystem(auditbuf,
                 "user not at requested terminal, write aborted", ET_SUBSYSTEM);
#endif
#ifndef NLS
		fprintf(stderr,"%s is not at \"%s\".\n",receipient,terminal) ;
#else NLS
		fprintmsg(stderr,(catgets(nlmsg_fd,NL_SETN,7,"%1$s is not at \"%2$s\".\n")),receipient,terminal) ;
#endif NLS
	    }
	    exit(1) ;
	  }

/*	If we were just looking for anyplace to talk and didn't find	*/
/*	one, complain and quit.						*/
/*	If permissions prevent us from sending to this person - exit	*/

	else if (*rterm == '\0')
	  {
	    if (bad > 0) {
#ifdef SecureWare
		if (ISSECURE) {
                	sprintf(auditbuf, "write to user %s", receipient);
                	audit_subsystem(auditbuf,
			     "permission denied, write  aborted",ET_SUBSYSTEM);
		}
#endif
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,8,"Permission denied.\n"))) ;
	    } else
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,9,"%s is not logged on.\n")),receipient) ;
	    exit(1) ;
	  }

/*	Did we find our own entry?					*/

	else if (self.ut_pid == 0)
	  {
/*	Use the user id instead of utmp name if the entry in the	*/
/*	utmp file couldn't be found.					*/

#ifdef SecureWare
            if (((ISSECURE) && 
		 ((passptr = getpwuid(getluid())) == (struct passwd *)NULL))||
                ((!ISSECURE) &&
		 ((passptr = getpwuid(getuid())) == (struct passwd *)NULL)))
#else	
            if ((passptr = getpwuid(getuid())) == (struct passwd *)NULL)
#endif
	      {
#ifdef SecureWare
                if (ISSECURE) 
		    audit_subsystem("determine user identity from /etc/passwd",
                        "unable to locate entry, write aborted", ET_SUBSYSTEM);
#endif
		fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,10,"Cannot determine who you are.\n"))) ;
		exit(1) ;
	      }
	    strncpy(&ownname[0],&passptr->pw_name[0],sizeof(ownname)) ;
	  }
	else
	  {
	    strncpy(&ownname[0],self.ut_user,sizeof(self.ut_user)) ;
	  }
	ownname[sizeof(ownname)-1] = '\0' ;

	if (!permit1(1))
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,11,"Warning: You have your terminal set to \"mesg -n\". No reply possible.\n"))) ;

/*	Try to open up the line to the receipient's terminal.		*/

	signal(SIGALRM,openfail) ;
	alarm(5) ;
#ifdef TRUX
	if(ISSECURE)
            fp = write_open_terminal(rterminal);
	else
	    fp = fopen(&rterminal[0],"w") ;
#else
	fp = fopen(&rterminal[0],"w") ;
#endif
#ifdef SecureWare
	if(ISSECURE)
            write_adjust_security();
#endif
	alarm(0) ;

/*	Catch signals SIGHUP, SIGINT, SIGQUIT, and SIGTERM, and send	*/
/*	<EOT> message to receipient before dying away.			*/

	setsignals(eof) ;

/*	Get the time of day, convert it to a string and throw away the	*/
/*	year information at the end of the string.			*/
	time(&tod) ;

#ifndef NLS
	time_of_day = ctime(&tod) ;
	*(time_of_day + 19) = '\0' ;
#else NLS
	time_of_day = nl_cxtime(&tod, (catgets(nlmsg_tfd,NL_SETN,17, "%a %h %d %H:%M:%S")));
#endif NLS
#ifndef NLS
	fprintf(fp,"\n\007\007\007\tMessage from %s (%s) [ %s ] ...\n",
	    &ownname[0],oterminal,time_of_day) ;
#else NLS
	fprintmsg(fp,(catgets(nlmsg_fd,NL_SETN,12,"\n\007\007\007\tMessage from %1$s (%2$s) [ %3$s ] ...\n")), &ownname[0],oterminal,time_of_day) ;
#endif NLS
	fflush(fp) ;
	fprintf(stderr,"\007\007") ;	

/*	Get input from user and send to receipient unless it begins	*/
/*	with a !, when it is to be a shell command.			*/

	while ((ptr = fgets(&input[0],sizeof(input),stdin)) != NULL)
	  {
/*	Is this a shell command?					*/

	    if (*ptr == '!')
	      {
		shellcmd(++ptr) ;
	      }

/*	Send line to the receipient.					*/

	    else
	      {
		if (!permit1(fileno(fp))) {
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,13,"Can no longer write to %s\n")),rterminal);
			break;
		}
#ifdef SecureWare
		if(ISSECURE)
                    write_clean_output(ptr, fp);
		else
		    fputs(ptr,fp) ;
#else
		fputs(ptr,fp) ;
#endif
		fputc('\r', fp);        /* send cr for raw ttys' benefit */
		fflush(fp) ;
	      }
	  }

/*	Since "end of file" received, send <EOT> message to receipient.	*/
#ifdef SecureWare
	if (ISSECURE) {
        	sprintf(auditbuf, "write performed by %s to %s",
                    self.ut_user, receipient);
        	audit_subsystem(auditbuf, "write complete", ET_SUBSYSTEM);
	}
#endif
	eof() ;
  }

setsignals(catch)

int (*catch)() ;

  {
	signal(SIGHUP,catch) ;
	signal(SIGINT,catch) ;
	signal(SIGQUIT,catch) ;
	signal(SIGTERM,catch) ;
  }

shellcmd(command)

char *command ;

  {
	register int child ;
	extern int eof() ;

	if ((child = fork()) == FAILURE)
	  {
	    fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,14,"Unable to fork.  Try again later.\n"))) ;
	    return ;
	  }
	else if (child == 0)
	  {
/*	Reset the signals to the default actions and exec a shell.	*/

#ifdef SecureWare
	    if(ISSECURE)
                write_prepare_for_escape(fp);
#endif
	    execl("/bin/sh","sh","-c",command,0) ;
	    exit(0) ;
	  }
	else
	  {
/*	Allow user to type <del> and <quit> without dying during	*/
/*	commands.							*/

	    signal(SIGINT,SIG_IGN) ;
	    signal(SIGQUIT,SIG_IGN) ;

/*	As parent wait around for user to finish spunoff command.	*/

	    while(wait(NULL) != child) ;

/*	Reset the signals to their normal state.			*/

	    setsignals(eof) ;
	  }
	fprintf(stdout,"!\n") ;
  }

openfail()
  {
	extern char *rterm,*receipient ;

#ifndef NLS
	fprintf(stderr,"Timeout trying to open %s's line(%s).\n",
	    receipient,rterm) ;
#else NLS
	fprintmsg(stderr,(catgets(nlmsg_fd,NL_SETN,15,"Timeout trying to open %1$s's line(%2$s).\n")),
	    receipient,rterm) ;
#endif NLS
	exit(1) ;
  }

eof()
  {
	extern FILE *fp ;

	fprintf(fp,(catgets(nlmsg_fd,NL_SETN,16,"<EOT>\n"))) ;
	exit(0) ;
  }

/* permit: check mode of terminal - if not writable by all disallow writing to
	 (even the user him/herself cannot therefore write to their own tty) */

permit (term)
char *term;
  {
	struct stat buf;
	int fildes;

	if ((fildes = open(term,O_WRONLY)) < 0)
		return(0);
#ifdef SecureWare
	memset(&buf, '\0', sizeof(buf));
#endif
	fstat(fildes,&buf);
	close(fildes);
#ifdef SecureWare
	if(ISSECURE)
            return write_secure_permit(&buf);
	else
	    return( (getuid() == 0) || (buf.st_mode & 0000002) );
#else
	return( (getuid() == 0) || (buf.st_mode & 0000002) );
#endif
  }



/* permit1: check mode of terminal - if not writable by all disallow writing to
	 (even the user him/herself cannot therefore write to their own tty) */
/* this is used with fstat (which is faster than stat) where possible */

permit1 (fildes)
int fildes;
  {
	struct stat buf;

#ifdef SecureWare
	memset(&buf, '\0', sizeof(buf));
#endif
	fstat(fildes,&buf);
#ifdef SecureWare
	if(ISSECURE)
            return write_secure_permit(&buf);
	else
	    return( (getuid() == 0) || (buf.st_mode & 0000002) );
#else
	return( (getuid() == 0) || (buf.st_mode & 0000002) );
#endif 
  }
