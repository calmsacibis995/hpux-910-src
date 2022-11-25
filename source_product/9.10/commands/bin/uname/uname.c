static char HPUX_ID[] = "@(#) $Revision: 70.6 $";

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#define catclose(x)	/* nothing */;
#else
#define NL_SETN 1
#include <nl_types.h>
nl_catd nlmsg_fd;
#endif /* NLS */

#include	<stdio.h>
#include	<sys/utsname.h>


extern char *ltoa();

struct utsname	unstr, *un;

static char *license();

main(argc, argv)
int argc;
char **argv;
{
	register i;
	int	lflg=0; 
	int     sflg=1, nflg=0, mflg=0, rflg=0, vflg=0, iflg=0, errflg=0;
	int	optlet;

#ifdef NLS
	nlmsg_fd = catopen("uname",0);
#endif

	if((argv[1][0] == '-') && (argv[1][1] == 'S'))
	{
		if(getuid() != 0)
		{
			fputs(catgets(nlmsg_fd,NL_SETN,100, "You must be superuser to set nodename\n"), stderr);
	        	catclose(nlmsg_fd);
			exit(1);
		}
		if(strlen(argv[2]) > UTSLEN-1)
		{
			fputs(catgets(nlmsg_fd,NL_SETN,101, "Nodename must be less than\n"), stderr);
			fputs(ltoa(UTSLEN), stderr);
			fputs(catgets(nlmsg_fd,NL_SETN,102, " characters\n"), stderr);
	        	catclose(nlmsg_fd);
			exit(1);
		}
		else if(argc < 3)
		{
			fputs(catgets(nlmsg_fd,NL_SETN,103, "usage: uname [-amnrsvil] [-S nodename]\n"), stderr);
	        	catclose(nlmsg_fd);
			exit(1);
		}
		setuname(argv[2], strlen(argv[2]));
	        catclose(nlmsg_fd);
		exit(0);
			
	}
	un = &unstr;
	uname(un);

	while((optlet=getopt(argc, argv, "amnrsvil")) != EOF) switch(optlet) {
	case 'a':
	       /*  
		*  behave as though all of the options (except l) 
		*  were specified 
		*/
		sflg++; nflg++; rflg++; vflg++; iflg++; mflg++; lflg++;
		break;
	case 'm':
		/*
		 *  write the name of the hardware type on which
		 *  the system is running
		 */
		mflg++;
		break;
	case 'n':
	        /*  
		 *  write the name of this node within an 
		 *  implementation-specified communications network 
		 */
		nflg++;
		break;
	case 'r':
		/*
		 *  r: write the current release level of the OS
		 */
		rflg++;
		break;
	case 's':
	       /*  
		*  write the name of the implementation of the 
		*  operating system 
		*/
		sflg++;
		break;
	case 'v':
		/*
		 *  v: write the current verion of this release of the OS
		 */
		vflg++;
		break;
        case 'l':
		/*
		 *  write the license level on this node
		 */
		lflg++;
		break;
	case 'i':
		/*
		 *  Print the nodename if the machine identification 
		 *  number cannot be ascertained.
		 */
		iflg++;
		break;
	case '?':
		errflg++;
	}
	if(errflg) {
		fputs(catgets(nlmsg_fd,NL_SETN,103, "usage: uname [-amnrsvil] [-S nodename]\n"), stderr);
	        catclose(nlmsg_fd);
		exit(1);
	}
	if(mflg | nflg | rflg | vflg | lflg | iflg) sflg--;
	if(sflg)
		fputs(un->sysname, stdout);
	if(nflg) {
		if(sflg) putchar(' ');
		fputs(un->nodename, stdout);
	}
	if(rflg) {
		if(sflg | nflg) putchar(' ');
		fputs(un->release, stdout);
	}
	if(vflg) {
		if(sflg | nflg | rflg) putchar(' ');
		   fputs(un->version, stdout);
	}
	if(mflg) {
		if(sflg | nflg | rflg | vflg) putchar(' ');
		fputs(un->machine, stdout);
	}
	if(iflg) {
		if(sflg | nflg | rflg | vflg | mflg) putchar(' ');
		if ( un->idnumber[0] != '\0' )
		    fputs(un->idnumber, stdout);
		else
		    fputs(un->nodename, stdout);
	}
	if(lflg) {
		if(sflg | nflg | rflg | vflg | mflg | iflg) putchar(' ');
		   fputs(license(un->version,un->machine), stdout);
	}
	putchar('\n');
	catclose(nlmsg_fd);
	exit(0);
}

static char*
license(level,machine)
char *level, *machine;
{
    switch(level[0])
    {
       case 'A':
	      return catgets(nlmsg_fd,NL_SETN,104, "two-user license");
       case 'B':
	      if (strncmp(machine,"9000/3", 6) == 0 ||
		  strncmp(machine, "9000/4", 6) == 0)
		 return catgets(nlmsg_fd,NL_SETN,105, "unlimited-user license");
	      else
		 return catgets(nlmsg_fd,NL_SETN,106, "16-user license");
       case 'C':
	      return catgets(nlmsg_fd,NL_SETN,107, "32-user license");
       case 'D':
	      return catgets(nlmsg_fd,NL_SETN,108, "64-user license");
       case 'E':
	      return catgets(nlmsg_fd,NL_SETN,109, "8-user license");
       case 'U':
	      return catgets(nlmsg_fd,NL_SETN,105, "unlimited-user license");
       default:
	      return level;
    }
}
