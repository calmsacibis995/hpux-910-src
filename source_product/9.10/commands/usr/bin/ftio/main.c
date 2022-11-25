/* HPUX_ID: @(#) $Revision: 72.1 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : main.c 
 *	Purpose ............... : Main for ftio. 
 *	Author ................ : David Williams. 
 *
 *  	Copyright (C) 1986 David Williams & Hewlett Packard Australia.
 *
 *	Description:
 *
 * 	This is the main function for ftio. It handles checking the file
 *	type, allocating SHM and SEM resources, and finally invokes the 
 *	two processes for file copy.
 *
 *	Contents:
 *		main()
 *
 *-----------------------------------------------------------------------------
 */

/* Define _MAIN_ so ftio.h knows we're in the main module. */
#define	_MAIN_

#include	"ftio.h"
#include	"define.h"
#include	<pwd.h>

#ifdef NLS
#include	<locale.h>
#endif NLS


/*
 * 	And herrrrrrrrrrres main:
 */
main(argc, argv)
int 	argc;
char 	*argv[];
{
	int	device_fd;
	int	buffersize;
	int     ret, retwait;
	int	options = 0;
	char	*s;
	int	i;
	int     retcode = 0;    /* Return code for parent process. */

	char	*getcwd();
	extern uid_t geteuid();	
	int     sig_handler();
	char	*basename();
	char	*shmalloc();
	long	katoi();
	int     init_findpath();
	extern char *malloc();


#ifdef NLS			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("ftio"), stderr);
		putenv("LANG=");
	}
#endif NLS

#ifdef PKT_DEBUG
setvbuf(stdout,NULL,_IONBF,0);  /* Don't buffer stdout. */
#endif PKT_DEBUG

#ifdef ACLS
	Aclflag = 0;			/* assume warning messages */
#endif
	/*
	 *	Set Myname to basename of $0.
	 */
	Myname = basename(argv[0]);

	/*
	 *	Initialise Pathname area.
	 */
	if (Pathname_init() == -1)
	{
		(void)ftio_mesg(FM_NMALL);
		(void)exit(1);
	}

	/*
	 * 	Get user id.
	 */
	User_id = geteuid();

	/*
	 * 	Get the username.
	 *
	 * 	just in case we get a junk user name from cuserid
	 */
	if ( ! (User_name = cuserid(NULL)) )
		User_name = "user??";

	/*
	 *	Make the Restart list name..
	 */
	sprintf(Restart_name, "/tmp/ftio.%d", getpid());

	/*
	 *	Process options and check the alias file.
	 */
	if ((options = process_options(argc, argv)) < 1 || argc < options + 2)
	{
		(void)ftio_mesg(FM_ARGC);
		(void)exit(1);
	}

	/*
	 * 	Use getcwd(3) to find the current working directory
	 */
	if ((Home = getcwd((char *)NULL, PATHLENGTH+2)) == NULL)
	{
		(void)ftio_mesg(FM_NCWD);
		(void)exit(1);
	}


	/*
	 * Initialize findpath() routine (used for CDFs).
	 */

#if defined(DUX) || defined(DISKLESS)
	init_findpath();
#endif DUX || DISKLESS


	/* 
	 * 	Attempt to open the output device
	 */
	Output_dev_name = argv[1 + options];

	if ((device_fd = rmt_open(Output_dev_name, Mymode? 2: 0)) == -1)
	{
		(void)ftio_mesg(FM_NOPEN, Output_dev_name);
		(void)exit (1);
	}

	/*
	 * 	Check the device file for type of device.
	 */
	if (test_dev_type(device_fd) == -1)
	{
		(void)ftio_mesg(FM_NSTAT, Output_dev_name);
		(void)exit(1);
	}

	/*
	 *	If we are using filelists..
	 *	Make a list of files to backup in /tmp/ftio.PID.
	 *	else
	 *	open the restart_list
	 *	
	 */
	if (Mymode == OUTPUT && Filelist)
	{
		if (make_filelist(&argv[options + 2]) == -1)
		{
			(void)ftio_mesg(FM_NOPEN, Filelist);
			(void)exit (1);
		}
	}


	/*
	 *	Trim Blocksize to 512 byte multiples.
	 */
	if (Mymode == OUTPUT && Blocksize & (CPIOSIZE - 1))
	{
		Blocksize = Blocksize & (~(CPIOSIZE - 1));
	}

	/* 
	 *	Check we can write, and all that stuff.
	 */
	if (Dev_type == REGULAR)
	{
		if (Mymode)
		{
			/* 
			 * 	Regular file, close, then trunc it.
			 */
			rmt_close(device_fd);

			if ((device_fd = rmt_creat(Output_dev_name, 0644))
			    == -1) {
				(void)ftio_mesg(FM_NCREAT, Output_dev_name);
				(void)exit (1);
			}
		}
	}
	else	/* special file */
	{

		/*
		 *	Try to read/write the tape header to see if 
		 *	the device is writing ok.
		 *
		 *	NOTE: chgreel() is now called. This means when
		 *	it comes back it will always have worked.
		 *	The only other way out of chgreel is to kill
		 *	ftio.
		 */
		device_fd = chgreel(device_fd, Mymode, 0);
	}

	/* 
	 * 	Calculate the buffer space needed.
	 */
	buffersize = Blocksize * Nobuffers;
	
	/*
	 * 	Check the "Blocksize" specified
	 * 	check the number of buffers specified
	 * 	this is pretty genorous at the moment
	 */
	if ( (Blocksize > MAXBLOCKSIZE)
	  || (Nobuffers > MAXNOBUFFERS)
	  || (Blocksize < CPIOSIZE) 
	  || (Nobuffers < 1) 
	)
	{
		(void)ftio_mesg(FM_TOOBIG, "blksize | nobufs");
		(void)exit(1);
	}

	/*
	 *	Trap signals, so we can clean up the shm & sem stuff.
	 */
	trap_signals();


	/*
	 *  It is necessary to increase the break value before
	 *  attaching the shared memory segment. This increases
	 *  the amount of malloc'able memory available to ftio.
	 */

	moveup_breakvalue(EXTRA_MEMORY);


	/*	
	 *	Get a slab of shared memory.
	 *	We need N buffers + 1 for keeping the checkpoint 
	 *	information in.
	 */


	buffersize += (Nobuffers + 1) * sizeof(struct ftio_packet);
	if ((s = shmalloc(Output_dev_name, buffersize)) == NULL)
	{
		(void)ftio_mesg(FM_SHMALLOC);
		(void)exit(1);
	}


	/*
	 *	Initialise packet pointers.
	 */
	Packets = (struct ftio_packet *)(s);
	Packets[0].block = s + ((Nobuffers + 1) * sizeof(struct ftio_packet));
	Packets[0].status = PKT_GO;
	for (i = 1; i < Nobuffers; i++)
		Packets[i].block = Packets[i-1].block + Blocksize;

	/*
	 *	Initialise semaphores.
	 */
	if (init_sems() == -1)
	{
		(void)ftio_mesg(FM_NSEMOP);
		(void)myexit(1);
	}

	/*
	 *	Split the processes.
	 */
	if ((Pid = fork()) == -1)
	{
		(void)ftio_mesg(FM_NFORK);
		(void)myexit(1);
	}
	
	if (Mymode == OUTPUT)
		if (Pid) 
		{
#ifdef DEBUG
			fprintf(stderr,"Parent PID=%d \n",getpid());
			fprintf(stderr,"Child's PID=%d\n",Pid);
#endif
			if (!host)
				close(device_fd);
			fileloader(&argv[options + 2]); 
		}
		else
		{
			/*
			 *	Start timer.
			 */
			(void)printperformance(0, 0);
			
			/*
			 *	Call the tapewriting process.
			 */
		 	i = tapewriter(device_fd);

			/*
			 *	Print no of blocks shipped.
			 */
			if (Performance)
				(void)printperformance(i, 3);
			else
				if (i == 1)
					(void)fprintf(stderr, "%d block\n", i);
			        else
					(void)fprintf(stderr, "%d blocks\n",i);
		}
	else
		if (Pid) 
		{
			/*
			 *	Start timer.
			 */
			(void)printperformance(0, 0);
			
			/*
			 *	Call the tapereading process.
			 */
			i = tapereader(device_fd);
			
			/*
			 *	Print no of blocks shipped.
			 */
			if (Performance)
				(void)printperformance(i, 3);
			else
				if (i == 1)
					(void)fprintf(stderr, "%d block\n", i);
			        else
					(void)fprintf(stderr, "%d blocks\n", i);					
		}
		else
		{
			if (!host)
				close(device_fd);
			filewriter(&argv[options + 2]);
		}

	if (Pid) 
	{
#ifdef	GPROF
		if (chdir(Home) == -1)
			perror("could not chdir to starting directory");
#endif	GPROF
		/*
		 *	We have to now ignore the death of child signal.
		 */
		(void)sigcld_off();


		/* Wait on child (if it still exists). */

		while ( kill(Pid,0) == 0 ) {
		    ret = wait(&retwait);

		    if ( ret == Pid ) {
			/* Ftio co-process died. Exit loop. */
			retcode = ((retwait>>8)&0377);
			break;
		    } else if ( ret == -1 ) {
			/* Wait on child process failed.
			 * Issue message and set return code.
			 */
			(void)ftio_mesg(FM_NWAIT);
			retcode = 1;
			continue;
		    } else {
			/* This was not ftio(1) co-process. It
			 * was some other child, probably forked by
			 * the shell in a pipeline. Ignore its death.
			 * Continue waiting.
			 */
			continue;
		    }
		}
		rmt_close(device_fd);
	}
	else
	{
#ifdef	GPROF
		if (chdir("/tmp") == -1)
			perror("could not chdir to /tmp");
#endif	GPROF
		(void)exit(0);		/*NOTREACHED*/
	}

	(void)myexit(retcode);          /* go clean up */
	/*NOTREACHED*/
}


/*
 * moveup_breakvalue -
 *
 *  When a shared memory segment is attached to the data
 *  segment, the break value cannot be moved beyond the
 *  shared memory segment. This limits the amount of memory
 *  the user can allocate. The only way to increase the amount
 *  of memory available is to increase the break value before
 *  the shared memory segment is attached.
 *
 *  This routine (in effect) increases the break value by
 *  doing a malloc and then freeing the malloc'ed memory.
 */

moveup_breakvalue(amount)
    int amount;
{
    char *mp;

    mp = malloc(amount);
    if ( mp == (char *)0 ) {
	(void)ftio_mesg(FM_NMALL);
	(void)exit(1);
    }
    free(mp);
}


myexit(exitcode)
int	exitcode;
{
	/* 
	 *	If the process id is that of the child 
	 *	exit immediately.
	 */
	if (!Pid)
		(void)exit(exitcode);	/*NOTREACHED*/

	(void)unlink(Restart_name);	/* remove Checkpoint file */

	/*
	 *	Clean up ipc use.
	 */
	if (release_ipc() == -1)
		(void)exit(5);
	else
		(void)exit(exitcode);	/*NOTREACHED*/
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : process_options()
 *	Purpose ............... : Process the options.
 *
 *	Description:
 *
 *	Process the options on the command line, attempt to open alias
 *	file if -O or -I have been specified.
 *
 *	Returns:
 *	
 *	No of options on command line.
 */
process_options(argc, argv)
int	argc;
char	**argv;
{
	char	*s;
	int	options = 0;

	if (argc > 1 && *argv[1] == '-')
	{
		options = 1;
	
		switch (*(s = argv[1]+1))
		{
		case 'O':			/* normal out 	 */
			Mymode = OUTPUT;	/* write to tape */
			if (!alias_file())	/* check aliases */	
			{
				Htype = 1;	/* c headers     */
				Listfiles = 1;	/* verbose 	 */
				Doacctime = 1;	/* reset access times */
			}
			break;

		case 'I':			/* normal in 	 */
			Mymode = INPUT;		/* write to tape */
			if (!alias_file())	/* check aliases */	
			{
				Htype = 1;	/* c headers 	 */
				Makedir = 1;	/* make directories */
				Domodtime = 1;	/* restore mod times */
				Listfiles = 1;	/* verbose 	 */
			}
			break;

		case 'o':
			Mymode = OUTPUT;	/* write to tape */
			break;

		case 'i':
			Mymode = INPUT;		/* read from tape */
			break;

		case 'g':			/* print the tape filelist */
			(void)ftio_grep(argc, argv);
			break;

		case 'p':
		case 't':
			(void)ftio_mesg(FM_NOTIMP, *s);
			(void)exit(1);
		
		default:
			ftio_mesg(FM_INVOPT, *s);
			(void)exit(1);
		}
		options = option_check(argc, argv);

	}
	
	/*
	 * 	If args does not include a filename to use as start 
	 *	recursion then use stdin as list of files to backup.
	 */
	if (argc < 3 + options) 
	{
	    	Read_stdin = 1;
	}
	else
	{
	    	Read_stdin = 0;
		if (!Mymode)
			Matchpatterns = 1;
	}

	return options;
}


option_check(argc, argv)
int	argc;
char	**argv;
{
	char	*s;
	int	options = 1;

	long	katoi();

	/*
	 *	Skip over -I, -O, -i, or -o.
	 */
	s = argv[1] + 1;

	/* 
	 * 	Now for other options........
	 */
	while(*(++s))
		switch(*s)  
		{
		case 'R':	/* Resync to headers? */
			Resync = 1;
			break;

		case 'L': 	/* use default backup list */
			Filelist = "ftio.list";
			Checkpoint = 1;
			break;
			
		case 'M':	/* Cpio compatibility */
			Tape_headers = 0;
			Blocksize = 5120;
			break;
			
		case 'f':	/* copy in except files in patterns */
			Except_patterns = 1;
			break;
			
		case 'p':	/* print extra performance data */
			Performance++;
			break;
			
		case 'E':	/* force paths to be relative */
			Makerelative = 1;
			break;
			
		case 'u':	/* bring up all files, forget age */
			Neweronly = 0;
			break;
			
#ifdef	DEBUG					
		case 'X':	/* diagnostics */
			Diagnostic++;
			break;
#endif	DEBUG
			
		case 'x':	/* allow copy of special files */
			if (User_id)
			{
				(void)ftio_mesg(FM_REQSU, *s);
				(void)exit(1);
			}
			else
				Dospecial = 1;
			break;
			
		case 'm':	/* restore mod times */ 
			Domodtime = 1;
			break;
			
		case 'a':	/* reset access times on copy out */
			Doacctime = 1;
			break;
			
		case 'P':	/* use prealloc on recovery */
			Prealloc = 1;
			break;
			
		/* list ALL file names, not just matches to patterns */
#ifdef ACLS
		case 'A':
			if(Mymode == INPUT)
			    Listall = 1;
			if(Mymode == OUTPUT)
			    Aclflag = 1;
			break;
#else
		case 'A': 
			Listall = 1;
			break;
#endif
			
		case 'v':	/* verbose (print file names) */
			Listfiles = 1;
			break;
			
		/* 
		 *	Use locked shared memory segment 
		case 'L':	
			Lockshmem = 1;
			break;
		*/	

		case 'c':	/* character headers */
			Htype = 1;
			break;
			
		case 'd':	/* allow make directories on restore */
			Makedir = 1;
			break;
			
		case 't':	/* list files only, dont load files */
			Dontload = 1;
			break;
			
		case 'h':       /* Archive files pointed to by symbolic links. */
			Realfile = 1;
			break;

		case 'H':       /* Search hidden subdirectories (CDFs). */
			Hidden = 1;
			break;

		default:
			ftio_mesg(FM_INVOPT, *s);
			(void)exit(1);
		}		

	/*
	 * 	Now for options requiring details to be passed
	 */
	while(argc > options + 1 && *(s = argv[options + 1]) == '-')
		switch( *(++s) )
		{
		case 'L': 	/* use next option as backup list */
			if (argc <= ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}
			Filelist = argv[options + 1];
			Checkpoint = 1;
			if (strlen(basename(Filelist)) > MAXNAMLEN - 2)
			{
				(void)ftio_mesg(
						FM_BIGNAME, 
						argv[options+1],
						MAXNAMLEN - 2
				);
				(void)exit(1);
			}
			options++;
			break;
			
		case 'T': 	/* specify alternate to /dev/tty */
			if (argc < ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}
			Tty = argv[options + 1];
			options++;
			break;
			
		case 'K': 	/* specify backup Comment */
			if (argc < ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}
			Komment = argv[options + 1];
			options++;
			break;
			
		case 'Z': 	/* specify no of buffers to use */
			if (argc < ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}
			Nobuffers = atoi(argv[options + 1]);
			options++;
			break;
			
		case 'B': 	/* specify Blocksize to use */
			if (argc < ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}
			Blocksize = katoi(argv[options + 1]);
			options++;
			break;
			
		case 'S': 	/* specify Changereel script */
			if (argc < ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}
			Change_name = argv[options + 1];
			options++;
			break;
			
		case 'N': 	/* specify Incremental backup date */
			if (argc < ++options + 1)
			{
				(void)ftio_mesg(FM_ARGC);
				(void)exit(1);
			}

			if (stat(argv[options + 1], &Stats) == -1)
			{
				(void)ftio_mesg(FM_NSTAT, argv[options + 1]);
				(void)exit(1);
			}
			Inc_date = Stats.st_mtime;
			Incremental = 1;
			options++;
				break;

		case 'D':       /* specify file system type to descend */
			if (argc < ++options + 1)
			{
			    (void)ftio_mesg(FM_ARGC);
			    (void)exit(1);
			}
			Fstype = argv[options + 1];
			if ( strcmp(Fstype,"hfs") != 0 &&
			     strcmp(Fstype,"nfs") != 0
			   )
			{
			    (void)ftio_mesg(FM_INVARG,Fstype);
			    (void)exit(1);
			}
			options++;
			break;
			
		default:
			(void)ftio_mesg(FM_INVOPT, *s);
			(void)exit(1);
		}		

	return options;	
}

alias_file()
{
	int	mode = Mymode;
	FILE	*fp;
	struct	passwd	*pw,
		*getpwuid();
#define	MAX_ARGS 14
	char	*args_array[MAX_ARGS];
	char	*s;
	int	argc;
	
	char	*white_space(),
		*black_space();

	/*
	 *	Create the path to the alias file.
	 */
	if ((pw = getpwuid(User_id)) == NULL)
	{
		(void)ftio_mesg(FM_NHOME);
		return 0;
	}

	(void)strcpy(Errmsg, pw->pw_dir);
	(void)strcat(Errmsg, "/.ftiorc");

	if ((fp = fopen(Errmsg, "r")) == NULL)
	{
		(void)ftio_mesg(FM_NALIAS);
		return 0;
	}

	while(fgets(Errmsg, sizeof(Errmsg), fp) != NULL)
	{
		if ((mode == INPUT && !strncmp(Errmsg, "I=", 2))
			||
		    (mode == OUTPUT && !strncmp(Errmsg, "O=", 2))
		)
		{
			s = Errmsg + 2;
			argc = 1;
			while(1)
			{
				/*
				 *	Skip white space
				 */
				if ((s = white_space(s)) == NULL)
					break;
				
				/*
				 *	For the first arg..
				 *
				 *	If the first arg starts with a '-'
				 *	then it must be passed to
				 *	other_options() as the second arg
				 *	and the first arg must be made 
				 *	to point to a null char.
				 *
				 *	If the first arg is not a '-'
				 *	then set argv[1] back two bytes
				 *	for other_options().
				 */
				if (argc < 2)
				{
					if (*s == '-')
					{
						args_array[argc++] = "\0\0\0";
						args_array[argc] = s;
					}
					else
						args_array[argc] = s - 2;
				}
				
				/*
				 *	For all but the first argument
				 *	just set the argv to s.
				 */
				else
					args_array[argc] = s;

				argc++;

				if ((s = black_space(s)) == NULL)
					break;

				*s++ = '\0';
			}
			break;
		}
	}

	(void)fclose(fp);
	if (argc > 1) {
		(void)option_check(argc, args_array);
		return 1;
	} else
		return 0;
}

char *
white_space(s)
char	*s;
{
	register char	*p;

	for (p = s; *p; p++)
	{
		switch(*p)
		{
		case '\n':
			return NULL;

		case ' ':
		case '\t':
			break;

		default:
			return p;
		}
	}
	return NULL;
}

char *
black_space(s)
char	*s;
{
	register char	*p;

	for (p = s; *p; p++)
	{
		switch(*p)
		{
		case '\n':
			*p = '\0';
			return NULL;

		case ' ':
		case '\t':
			return p;

		default:
			break;
		}
	}
	return NULL;
}

trap_signals()
{
	/*
	 * 	Before we start grabbing shared memory,
	 * 	set up an interrupt trap so that the program will
	 * 	always clean up it's shared memory before existing.
	 */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, sig_handler);

	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, sig_handler);
	
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, sig_handler);
	
	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, sig_handler);
	
	(void)sigcld_on();
}

sigcld_on()
{
	if (signal(SIGCLD, SIG_IGN) != SIG_IGN)
		signal(SIGCLD, sig_handler);
}

sigcld_off()
{
	if (signal(SIGCLD, SIG_IGN) != SIG_IGN)
		signal(SIGCLD, SIG_DFL);
}

/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : sig_handler()
 *	Purpose ............... : All signals are vectored through here. 
 *
 *	Description:
 *		Manages the handling of signals trapped by ftio.
 *	
 *	Returns:
 *
 *	exits. 
 *
 *	History:
 *		920604	J. Lee	Added the processing for cld_flag.  To be used
 *				by other functions as a guide as to whether
 *				or not a SIGCLD signal was valid.
 */

sig_handler(sigval)
int     sigval;
{
	int     code = 0, ret, status;

	/* 
	 *	Reset for next interrupt 
	 */
	(void)signal(SIGINT, sig_handler);

	/* 
	 *	Parent kills child, lots of blood and guts
	 */
	if (Pid) 	
	{
		/*
		 *	If the signal is not death of child
		 *	kill the child.
		 */
		if (sigval == SIGCLD)
		{
			cld_flag = TRUE; 	/* reset Death of child flag */
			/* Wait on child. */
			ret = wait(&status);
#ifdef DEBUG
fprintf(stderr, "Caught SIGCLD, ret=%d, status=%o\n", ret, status);
#endif DEBUG
			if ( ret == Pid ) {
			    if ( ((status>>8)&0377) == 0 ) {
				/* Child had a successful exit.
				 * Return and let parent continue
				 * normally.
				 */
				return;
			    } else {
				/* Child exited unsuccessfully and
				 * unexpectedly. Set return code to
				 * 1 and die below ("myexit(code)").
				 */
				(void)ftio_mesg(FM_SIGCLD);
				code = 1;
			    }
			} else if ( ret == -1 ) {
			    /* Wait on child process failed. Set SIGCLD
			     * handler again.
			     */
			    (void)ftio_mesg(FM_NWAIT);
			    (void)sigcld_on();
			} else {
			    /* This was not ftio(1) co-process. It
			     * was some other child, probably forked by
			     * the shell in a pipeline. Ignore its death
			     * and set the signal handler for SIGCLD
			     * again.
			     */
#ifdef DEBUG
fprintf(stderr, "Process with pid=%d died.\n", ret);
fprintf(stderr, "This was not the co-process of ftio(1). Ignore it.\n");
#endif DEBUG
			    cld_flag=FALSE;
			    (void)sigcld_on();
			    return;
			}
		}
		else
		{
			(void)sigcld_off();

			if (kill(Pid, SIGTERM) == -1 && errno != ESRCH)
			{
				(void)ftio_mesg(FM_NKILL);
				code = 1;
			}
		}

		(void)myexit(code);
	}
	else 	
	/* 
	 *	Child just goes away!! leaving parent to clean up mess 
	 */
	{
		(void)exit(0);	/*NOTREACHED*/
	}
}


