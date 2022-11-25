/*	   @(#)9.1	88/01/14	*/
/*
 *        Access sccs_file    (across the LAN if necessary)
 *
 *   USAGE: Get <filename>
 *   USAGE: Prs <filename>
 *	etc.
 *	where: filename is either an ordinary sccs file or
 *		is a 'special' sccs file
 *		(i.e. contains psuedo linking information such as
 *			machine, pathname resolution, and user info.)
 *
 *   SPECIAL FILE FORMAT: %% <machine_name> <path_name> 
 *
 *	E.G.	file: s.example.h    contains
 *
 *	%% hpfclj /hpux/shared/src/usr/head/proc.dep/s200/s.example.h
 *
 *   Limitations:
 *     All options are available over Lan (Untested: Get -e  Delta)
 *
 *
 *   Passwd_file
 *	Permissions over net are determined via file: /users/sca/naccess
 *
 *   Example of usage:
 *
 *	  Get s200io/s.tp.c
 *		--> (translate to):	/usr/bin/get  s200io/s.tp.c
 *        Get s200/s.conf.h     
 *		--> (translates to):
 *          netunam /net/hpfclj sccslook:<passwd>
 *          get /net/hpfclj/hpux/shared/supp/usr/src/head/proc.dep/s200/s.conf.h
 */

#include <stdio.h>

/* #define	DEBUG	/* Define for Debugging - Does not issue command */

#define MAXLINE 256

char   cmd[512];		/* generic command */
char  *passwd_file;
char  *def_passwd_file = {"/users/nerfs/nfsmgr/naccess"};
char   commandline[MAXLINE];
char   machine_name[80];
char   opt_list[80];

char *getenv();

main(argc, argv)
char **argv;
{
	int i;
	short magic;
	FILE *ptr, *naccess_ptr, *fopen();
	char word[MAXLINE], string[MAXLINE], mach[20], user[80], passwd[32],
		*chptr, *strrchr(), errflag, *strrchr();

	errflag=0;

	/* Obtain basename of command */
        if ((chptr = strrchr(argv[0],'/'))==NULL)
                chptr = argv[0];
        else    chptr++;
	/* Get command */
	if	(!strcmp(chptr,"Get"))      strcat(cmd,"/usr/bin/get ");
	else if (!strcmp(chptr,"Prs"))      strcat(cmd,"/usr/bin/prs ");
	else if (!strcmp(chptr,"Sccsdiff")) strcat(cmd,"/usr/bin/sccsdiff ");
	else if (!strcmp(chptr,"Delta"))    strcat(cmd,"/usr/bin/delta ");
	else if (!strcmp(chptr,"Admin"))    strcat(cmd,"/usr/bin/admin ");
	else if (!strcmp(chptr,"Rmdel"))    strcat(cmd,"/usr/bin/rmdel ");
	else if (!strcmp(chptr,"Unget"))    strcat(cmd,"/usr/bin/unget ");
	else	{ 
		fprintf(stderr, "Unknown Sccs Command: %s\n",chptr);
		exit(1);
	}

	/* Obtain options */
	/* NOTE: Have to put single quotes around options to avoid */
	/* interpretation of shell variables and to make sure that options */
	/* with blanks in are interpretted correctly. */
	/* Second NOTE: this assumes that all optional values follow the */
	/* option designated with NO blanks, i.e. -r3.5 */
	*++argv;
	while (**argv == '-')	{
		strcat(opt_list, "'");
		strcat(opt_list, *argv++);
		strcat(opt_list, "' ");
		--argc;
		}

	if (!--argc) {
		fprintf(stderr, "usage: %s sccs_file\n", cmd);
		exit(1);
	}

	/* get the name of the passwd file with appropriate default */
	if ( (passwd_file = getenv("NACCESS")) == NULL)
		passwd_file = def_passwd_file;
#ifdef DEBUG
	printf("Passwd file is: %s\n",passwd_file);
#endif DEBUG
	
	/* Initialize command line sent to /usr/bin/get */
	strcat(commandline, cmd);
	strcat(commandline,opt_list);
	chptr = strrchr(commandline, '\0');

	while (argc)	{
		if ((ptr=fopen(*argv,"r"))==NULL)	{
			fprintf(stderr,"%s: error in opening file %s\n",cmd,
				*argv);
			errflag = 1;
			goto trynextfile;
			}
		fscanf(ptr, "%2s", (char *)&magic); /* Unpleasant trick */
#ifdef DEBUG
		printf("command <%s> opts <%s>\n", commandline, opt_list);
		printf("magic %x\n", magic);
#endif
		if (magic == 0x0168){  /* Magic number for sccs files */
			strcat(commandline,*argv);
			goto sendmsg;
			}
		if (magic != 0x2525) { /* Special 2 char header: "%%"=0x2525 */
			fprintf(stderr, "Unkown sccs file type: %s\n", *argv);
			errflag = 1;
			goto trynextfile;
			}

		/* Start constructing commandline */
#ifdef DEBUG
		printf("Psuedo Sccs File\n");
#endif
		strcat(commandline," /net/");
		/* Obtain machine name */
		fscanf(ptr, "%s", mach);
		strcat(machine_name,"/net/");
		strcat(machine_name,mach);
	
		strcat(commandline,mach);
		strcat(commandline,"/");
		fscanf(ptr, "%s", word);
		strcat(commandline,word);
	
		/* Obtain LAN password - Do network stuff */
		if ((naccess_ptr=fopen(passwd_file,"r"))==NULL)	{
			fprintf(stderr,"%s: error opening passwd file %s\n",
				cmd,passwd_file);
			errflag = 1;
			goto trynextfile;
			}
		/* Lookup machine for appropriate login:password */
		while (fgets(string, MAXLINE, naccess_ptr) != NULL)	{
			sscanf(string,"%s%s%s", word, user, passwd);
			if (!strcmp(word, mach)) goto got_passwd;
			}
		/* if we're here, couldn't find machine */
		fprintf(stderr,"Unable to find machine %s in %s\n",
				mach,passwd_file);
		errflag = 1;
		goto trynextfile;

	got_passwd:
		/* Construct user:passwd entry */
		strcat(user,":");
		strcat(user,passwd);
#ifdef	DEBUG
		printf("Over the NET: user:passwd <%s>\n", user);
#endif
 		if ((i=netunam(machine_name,user))<0)	{
			fprintf(stderr,"%s: netunam to %s as %s failed\n",cmd,
				machine_name,user);
			errflag = 1;
			goto trynextfile;
			}
	sendmsg:

#ifdef	DEBUG
		printf("Commandline:  \n%s\n",commandline);
#endif
		if (system(commandline))
			errflag = 1;

	trynextfile:
		/* Explicit closes for long arg lists */
		fclose(ptr);
		fclose(naccess_ptr);

		machine_name[0] = '\0';
		*chptr = '\0';
		--argc;
		*++argv;
	}
	exit(errflag);
}
