/* @(#)  $Revision: 72.8 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "dump.h"

#ifdef	PURDUE_EE
int	filepass;
#endif	PURDUE_EE
#ifdef  hpux
#include <sys/mtio.h>
#endif  hpux



#include	<sys/diskio.h>
#include 	<string.h>




int	notify = 0;	/* notify operator flag */
int	blockswritten = 0;	/* number of blocks written on current tape */

int	tapeno = 0;	/* current tape number */
int	density = 0;	/* density in bytes/0.1" */
int	ntrec = NTREC;	/* # tape blocks in each tape record */
int	cartridge = 0;	/* Assume non-cartridge tape */
int     Cascade_disk = 0; /* == 1 if logical block size > 1K */
int     lgblk = 0; /* It is number of DEV_BSIZE per logical disk block*/

#ifdef ACLS
int aclcount = 0;
#endif ACLS

#ifdef  hpux
char    *host;
#endif  hpux
#ifdef	RDUMP
char	*host;
#endif	RDUMP
int	anydskipped;	/* set true in mark() if any directories are skipped */
			/* this lets us avoid map pass 2 in some cases */


main(argc, argv)
	int	argc;
	char	*argv[];
{


	char		*arg;
	int		bflag = 0, i;
	float		fetapes;
#ifdef	hpux
	register	struct	mntent	*dt;
#else	hpux
	register	struct	fstab	*dt;
#endif	hpux

#if defined(TRUX) && defined(B1)

	msg("*************************************************\n");
	msg("This is an HP BLS (B Level security System)\n");
	msg("*************************************************\n\n");
	it_is_b1fs = 1; /* initialize to a b1fs unless super block says not */
	found_in_chklst = remote_tape = 0; /* i.e. local tape */
	b1debug = 0;     	/* no debug msgs unless option v given */
	tapedev = TAPE;  	/* initialize and place holder */

/* ie_init does a initprivs; 1 means ismltape */
/* ismltape will force SEC_ALLOWMACACCESS otherwise it's enabled */

        ie_init(argc, argv, 1);
/* ie_init routine does some enablepriv, but I want forcepriv instead So here:*/
	forcepriv(SEC_ALLOWDACACCESS);
	forcepriv(SEC_CHPRIV);
	forcepriv(SEC_MKNOD);
	forcepriv(SEC_LOCK);
	forcepriv(SEC_CHMODSUGID);
	forcepriv(SEC_WRITEUPSYSHI);
	forcepriv(SEC_WRITEUPCLEARANCE);
	forcepriv(SEC_DOWNGRADE);

	forcepriv(SEC_NETPRIVADDR);
	forcepriv(SEC_REMOTE);
	forcepriv(SEC_NETNOAUTH);
	forcepriv(SEC_NETBROADCAST);
	forcepriv(SEC_NETSETID);
	forcepriv(SEC_NETRAWACCESS);

	if((fir = (char *) mand_alloc_ir()) == (char *) 0)
                dumpabort();
#endif /*(TRUX) && (B1) */

	time(&(spcl.c_date));

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	tape = TAPE;
	disk = DISK;
	increm = NINCREM;
	temp = TEMP;
	if (TP_BSIZE / DEV_BSIZE == 0 || TP_BSIZE % DEV_BSIZE != 0) {
		msg("TP_BSIZE must be a multiple of DEV_BSIZE\n");
		dumpabort();
	}
	incno = '9';
	uflag = 0;
	arg = "u";
	if(argc > 1) {
		argv++;
		argc--;
		arg = *argv;
		if (*arg == '-')
			argc++;
	}
	while(*arg)
	switch (*arg++) {
	case 'w':
		lastdump('w');		/* tell us only what has to be done */
		exit(0);
		break;
	case 'W':			/* what to do */
		lastdump('W');		/* tell us the current state of what has been done */
		exit(0);		/* do nothing else */
		break;

	case 'f':			/* output file */
		if(argc > 1) {
			argv++;
			argc--;
			tape = *argv;
#if defined(TRUX) && defined(B1)
			tapedev = tape;
#endif /*(TRUX) && (B1) */
		}
		break;
#if defined(TRUX) && defined(B1)
	case 'v':			/* verbose(B1debug mode) */
		b1debug++ ;
		break;
#endif /*(TRUX) && (B1) */
	case 'd':			/* density, in bits per inch */
		if (argc > 1) {
			argv++;
			argc--;
			density = atoi(*argv) / 10;
			if (density >= 625 && !bflag)
				ntrec = HIGHDENSITYTREC;
		}
		break;

	case 's':			/* tape size, feet */
		if(argc > 1) {
			argv++;
			argc--;
			tsize = atol(*argv);
			tsize *= 12L*10L;
		}
		break;

	case 'b':			/* blocks per tape write */
		if(argc > 1) {
			argv++;
			argc--;
			bflag++;
			ntrec = atol(*argv);
		}
		break;

/*	case 'c': */			/* Tape is cart. not 9-track */
/*		cartridge++; */
/*		break;*/

	case '0':			/* dump level */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		incno = arg[-1];
		break;

	case 'u':			/* update /etc/dumpdates */
		uflag++;
		break;

	case 'n':			/* notify operators */
		notify++;
		break;

	default:
		fprintf(stderr, "bad key '%c'\n", arg[-1]);
		Exit(X_ABORT);
	}
	if(argc > 1) {
		argv++;
		argc--;
		disk = *argv;
	}
	if (strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}

#if defined(TRUX) && defined(B1)
	if(!authorized_user("backup")){ 	
		msg("No authorization for attempted operation\n");
		return(-1);
	}
/* read the policy file and find out which tag in the tag pool is used for 
 * Sensitivity label. This info will be used in dumptraverse later.
 * I could hard code it in dumptraverse, but I rather not. 
 */  
	if ((b1buff =(char *) malloc(sizeof(struct mand_config))) == (char *) 0)
		dumpabort();
	
	if ((b1fd = open(MAND_PARAM_FILE, O_RDONLY)) < 0){
	   	msg("DUMP: can't open the policy file: %s\n", MAND_PARAM_FILE);
		dumpabort();	
	}
	if(read(b1fd, b1buff, sizeof(struct mand_config)) 
						!= sizeof(struct mand_config)) {
	   	msg("DUMP: can't read the policy file: %s\n", MAND_PARAM_FILE);
	   	dumpabort();
	}
#endif /* (TRUX) && (B1) */
	/*
	 * Determine how to default tape size and density
	 *
	 *         	density				tape size
	 * 9-track	1600 bpi (160 bytes/.1")	2300 ft.
	 * 9-track	6250 bpi (625 bytes/.1")	2300 ft.
 	 * cartridge	8000 bpi (100 bytes/.1")	1700 ft. (450*4 - slop)
	 */
	if (density == 0)
		density = cartridge ? 100 : 160;
	if (tsize == 0)
 		tsize = cartridge ? 1700L*120L : 2300L*120L;

#ifdef  hpux
	if (strchr(tape, ':') != NULL) {
	    host = tape;
	    if ((host = strtok(tape, ":")) != NULL) {
                 tape = strtok((char *) NULL, ":");
		 if (tape == 0) {
		     msg("need keyletter ``f'' and device ``host:tape''\n");
		     dumpabort();
		 }
#if defined(TRUX) && defined(B1)
		 remote_tape = 1; /* remember it's remote */
		/* Broke in IC3
		 rem_host = m6getrhnam(host);
		 if (rem_host->host_type == M6CON_UNLABELED ){
			msg("Error: remote host %s is not a trusted host\n\n", host);
			if(query("Do you want to abort?"))
				dumpabort();
		 } */
#endif /*(TRUX) && (B1) */
		 if (rmthost(host, 'W') == 0)
		      exit(X_ABORT);
		 setuid(getuid());   /* rmthost() is the only reason to be setuid */
	    }
	}

#else  hpux
#ifdef	RDUMP
	{ host = tape;
	  tape = index(host, ':');
	  if (tape == 0) {
		msg("need keyletter ``f'' and device ``host:tape''\n");
		exit(1);
	  }
	  *tape++ = 0;
	  if (rmthost(host, 'W') == 0)
		exit(X_ABORT);
	}
	setuid(getuid());	/* rmthost() is the only reason to be setuid */
#endif	RDUMP
#endif  hpux

	if (signal(SIGHUP, sighup) == SIG_IGN)
		signal(SIGHUP, SIG_IGN);
	if (signal(SIGTRAP, sigtrap) == SIG_IGN)
		signal(SIGTRAP, SIG_IGN);
	if (signal(SIGFPE, sigfpe) == SIG_IGN)
		signal(SIGFPE, SIG_IGN);
	if (signal(SIGBUS, sigbus) == SIG_IGN)
		signal(SIGBUS, SIG_IGN);
	if (signal(SIGSEGV, sigsegv) == SIG_IGN)
		signal(SIGSEGV, SIG_IGN);
	if (signal(SIGTERM, sigterm) == SIG_IGN)
		signal(SIGTERM, SIG_IGN);
	

	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	set_operators();	/* /etc/group snarfed */
	getfstab();		/* /etc/fstab snarfed */
	/*
	 *	disk can be either the full special file name,
	 *	the suffix of the special file name,
	 *	the special name missing the leading '/',
	 *	the file system name with or without the leading '/'.
	 */
	dt = fstabsearch(disk);
	if (dt != 0)
#ifdef	hpux
		disk = rawname(dt->mnt_fsname);
#else	hpux
		disk = rawname(dt->fs_spec);
#endif	hpux
	getitime();		/* /etc/dumpdates snarfed */

	msg("Date of this level %c dump: %s\n", incno, prdate(spcl.c_date));
 	msg("Date of last level %c dump: %s\n",
		lastincno, prdate(spcl.c_ddate));
	msg("Dumping %s ", disk);
	if (dt != 0)
#ifdef	hpux
		msgtail("(%s) ", dt->mnt_dir);
#else	hpux
		msgtail("(%s) ", dt->fs_file);
#endif	hpux

#ifdef  hpux
	if ( host )
	    msgtail("to %s on host %s\n", tape, host);
	else
	    msgtail("to %s\n", tape);
#else   hpux
#ifdef	RDUMP
	msgtail("to %s on host %s\n", tape, host);
#else	RDUMP
	msgtail("to %s\n", tape);
#endif	RDUMP
#endif  hpux

	fi = open(disk, 0);
	if (fi < 0) {
		msg("Cannot open %s\n", disk);
		Exit(X_ABORT);
	}
	if ((lgblk = (int) find_min_fsize(fi)) > DEV_BSIZE) {
		Cascade_disk = 1;
		lgblk = lgblk/DEV_BSIZE;
	}

	esize = 0;
	sblock = (struct fs *)buf;
	sync();
#if defined(TRUX) && defined(B1)
	if (!found_in_chklst && !dt)
	  if (strstr(disk, "dev/") == NULL )
 	    msg("WARNING: %s ,not in checklist and not a special file name. Dump may abort.\n", disk);
#endif /*(TRUX) && (B1) */

	bread(SBLOCK, sblock, SBSIZE);

#ifdef	hpux
#if defined(FD_FSMAGIC)
	if ((sblock->fs_magic != FS_MAGIC) && (sblock->fs_magic != FS_MAGIC_LFN)
		   && (sblock->fs_magic != FD_FSMAGIC)) {
		msg("bad sblock magic number\n");
		dumpabort();
	} else if ((sblock->fs_magic == FS_MAGIC_LFN) || (sblock->fs_featurebits & FSF_LFN))
		msg("This is an HP long file name filesystem\n");
#else   /* not NEW_MAGIC */
#if defined(FS_MAGIC_LFN)
	if (sblock->fs_magic != FS_MAGIC && sblock->fs_magic != FS_MAGIC_LFN) {
		msg("bad sblock magic number\n");
		dumpabort();
	} else if (sblock->fs_magic == FS_MAGIC_LFN)
		msg("This is an HP long file name filesystem\n");
#else	/* no long filenames */
	if (sblock->fs_magic != FS_MAGIC) {
		msg("bad sblock magic number\n");
		dumpabort();
	}
#endif	/* no long filenames */
#endif /* NEW_MAGIC */
#else	hpux
	if (sblock->fs_magic != FS_MAGIC) {
		msg("bad sblock magic number\n");
		dumpabort();
	}
#endif	hpux

#if defined(TRUX) && defined(B1)
	if (FsSEC(sblock)){
		it_is_b1fs = 1; 	/* Tagged */
		msg("*************************************************\n");
		msg("This is an HP BLS ** tagged ** filesystem \n");
		msg("*************************************************\n");
	}
	else{
		it_is_b1fs = 0; 	/* Untagged */
		msg("*************************************************\n");
		msg("This is an HP BLS ** untagged ** filesystem\n");
		msg("*************************************************\n\n");
	}
/* Now check the tape device. Checking the Local, non stdout tape device */
	if (!pipeout && it_is_b1fs && !remote_tape)
		if (check_ltape(tapedev) == 1) {
			msg("error when checking the device\n");
			dumpabort();
		}

#endif /*(TRUX) && (B1) */

	msiz = roundup(howmany(sblock->fs_ipg * sblock->fs_ncg, NBBY),
		TP_BSIZE);
	clrmap = (char *)calloc(msiz, sizeof(char));
	dirmap = (char *)calloc(msiz, sizeof(char));
	nodmap = (char *)calloc(msiz, sizeof(char));

	anydskipped = 0;
	msg("mapping (Pass I) [regular files]\n");
	pass(mark, (char *)NULL);		/* mark updates esize */

	if (anydskipped) {
		do {
			msg("mapping (Pass II) [directories]\n");
			nadded = 0;
			pass(add, dirmap);
		} while(nadded);
	} else				/* keep the operators happy */
		msg("mapping (Pass II) [directories]\n");

	bmapest(clrmap);
	bmapest(nodmap);

	if (cartridge) {
		/* Estimate number of tapes, assuming streaming stops at
		   the end of each block written, and not in mid-block.
		   Assume no erroneous blocks; this can be compensated for
		   with an artificially low tape size. */
		fetapes = 
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes/block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* streaming-stops per block */
			* 15.48		/* 0.1" / streaming-stop */
		) * (1.0 / tsize );	/* tape / 0.1" */
	} else {
		/* Estimate number of tapes, for old fashioned 9-track tape */
		int tenthsperirg = (density == 625) ? 3 : 7;
		fetapes =
		(	  esize		/* blocks */
			* TP_BSIZE	/* bytes / block */
			* (1.0/density)	/* 0.1" / byte */
		  +
			  esize		/* blocks */
			* (1.0/ntrec)	/* IRG's / block */
			* tenthsperirg	/* 0.1" / IRG */
		) * (1.0 / tsize );	/* tape / 0.1" */
	}
	etapes = fetapes;		/* truncating assignment */
	etapes++;
	/* count the nodemap on each additional tape */
	for (i = 1; i < etapes; i++)
		bmapest(nodmap);
	esize += i + 10;	/* headers + 10 trailer blocks */
	msg("estimated %ld tape blocks on %3.2f tape(s).\n", esize, fetapes);

	alloctape();		/* Allocate tape buffer. Returns 0 or 1 */

	otape();			/* bitmap is the first to tape write */
	time(&(tstart_writing));
	bitmap(clrmap, TS_CLRI);

	msg("dumping (Pass III) [directories]\n");
 	pass(dirdump, dirmap);

	msg("dumping (Pass IV) [regular files]\n");
#ifdef	PURDUE_EE
	filepass = 1;
#endif	PURDUE_EE
	pass(dump, nodmap);
#ifdef	PURDUE_EE
	filepass = 0;
#endif	PURDUE_EE

	spcl.c_type = TS_END;
#ifdef  hpux
	if ( host == NULL )
	    for(i=0; i<ntrec; i++)
	        spclrec();
#else   hpux
#ifndef	RDUMP
	for(i=0; i<ntrec; i++)
		spclrec();
#endif	RDUMP
#endif  hpux
	msg("DUMP: %ld tape blocks on %d tape(s)\n\n",spcl.c_tapea,spcl.c_volume);
#ifdef ACLS
        msg("DUMP: %s has %d inodes with unsaved optional acl entries\n",disk,aclcount);
#endif ACLS
	msg("DUMP IS DONE\n");

	putitime();
#ifdef  hpux
	if ( host == NULL ) {
	    if (!pipeout) {
		close(to);
		drewind();
	    }
	} else {
	    tflush(1);
	    drewind();
	}
#else  hpux
#ifndef	RDUMP
	if (!pipeout) {
		close(to);
		rewind();
	}
#else	RDUMP
	tflush(1);
	rewind();
#endif	RDUMP
#endif  hpux
	broadcast("DUMP IS DONE!\7\7\n");

	Exit(X_FINOK);

}

int	sighup(){	msg("SIGHUP()  try rewriting\n"); sigAbort();}
int	sigtrap(){	msg("SIGTRAP()  try rewriting\n"); sigAbort();}
int	sigfpe(){	msg("SIGFPE()  try rewriting\n"); sigAbort();}
int	sigbus(){	msg("SIGBUS()  try rewriting\n"); sigAbort();}
int	sigsegv(){	msg("SIGSEGV()  ABORTING!\n"); dumpabort();}
int	sigalrm(){	msg("SIGALRM()  try rewriting\n"); sigAbort();}
int	sigterm(){	msg("SIGTERM()  try rewriting\n"); sigAbort();}

sigAbort()
{
	if (pipeout) {
		msg("Unknown signal, cannot recover\n");
		dumpabort();
	}
	msg("Rewriting attempted as response to unknown signal.\n");
	fflush(stderr);
	fflush(stdout);
	close_rewind();
	exit(X_REWRITE);
}

char *rawname(cp)
	char *cp;
{
	static char rawbuf[32];
#ifdef	hpux
	char *dp;

	/* SYSV has structured /dev */
	if (cp[0] == '/')
		dp = index((cp + 1), '/');
	else
		dp = index(cp, '/');
#else	hpux
	char *rindex();
	char *dp = rindex(cp, '/');
#endif	hpux

	if (dp == 0)
		return (0);
	*dp = 0;
	strcpy(rawbuf, cp);
	*dp = '/';
	strcat(rawbuf, "/r");
	strcat(rawbuf, dp+1);
	return (rawbuf);
}


static long 
find_min_fsize(fd)
int fd;
{
        disk_describe_type dsk_data;
        if (ioctl(fd, DIOC_DESCRIBE, &dsk_data) != -1 &&
            dsk_data.lgblksz > DEV_BSIZE)
                return((long) dsk_data.lgblksz);
        return DEV_BSIZE;
}

#if defined(TRUX) && defined(B1)

/* Most of this checking is also done in the ie_check_device. But I need to 
 * have the dev_asg structure to get other info later on.
 */
int
check_ltape(dev)
char  *dev;
{
	char **alias;

	setdvagent(); /* to reset to the first record */
	while( (td = getdvagent()) != NULL) {  /* get a device record. */
		if(td->uflg.fg_devs) {
                        /* Get pointer to the list of valid device pathnames
                         * for this device.
                         */
			alias = td->ufld.fd_devs;

                        /* Search the list for a match for our device. */
			while(*alias != NULL) {
				if(strcmp(dev,*alias) == 0) break;
					alias++;
			}
		}
                /* break out of loop if we found a match. */
                if(*alias != NULL) break;
	}
	setdvagent(); /* to reset to the first record */
	if (td == (struct dev_asg *) 0)
		return (1);

          /*
           * Make sure that the device is a tape.
           */
	if (!td->uflg.fg_type)
		return (1);
	if(!ISBITSET(td->ufld.fd_type,AUTH_DEV_TAPE))
		return (1);
}
#endif /*(TRUX) && (B1) */



