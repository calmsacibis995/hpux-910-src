/* @(#)  $Revision: 66.5 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifdef __hpux
#  include <sys/types.h>
#  include <sys/vm.h>
#  include <sys/mtio.h>
#  include <sys/errno.h>
#  include <fcntl.h>
#  include <unistd.h>	/* For the _SC_PAGE_SIZE macro */
#endif /* __hpux */

#include <sys/file.h>
#include "dump.h"

char	(*tblock)[TP_BSIZE];	/* Pointer to malloc()ed buffer for tape */
int	writesize;		/* Size of malloc()ed buffer for tape */
int	trecno = 0;
extern int ntrec;		/* blocking factor on tape */
extern int cartridge;
extern int read(), write();

#if defined(TRUX) && defined(B1)
short   Lflag, Rflag, Cflag; /* In order to compile */
short   Aflag, Pflag ;
#endif /* (TRUX) && defined(B1) */


#ifdef __hpux
extern char *host;
#endif /* __hpux */
#ifdef RDUMP
extern char *host;
#endif /* RDUMP */

/*
 * Concurrent dump mods (Caltech) - disk block reading and tape writing
 * are exported to several slave processes.  While one slave writes the
 * tape, the others read disk blocks; they pass control of the tape in
 * a ring via flock().	The parent process traverses the filesystem and
 * sends spclrec()'s and lists of daddr's to the slaves via pipes.
 */
struct req {			/* instruction packets sent to slaves */
	daddr_t dblk;
	int count;
} *req;
int reqsiz, slaves;

/* More than one on HP-UX 6.0 seems risky yet !! */
#define MAXSLAVES 1		/* 1 slave writing, 1 reading, 1 for slack */

int slavefd[MAXSLAVES];		/* pipes from master to each slave */
int slavepid[MAXSLAVES];	/* used by killall() */
int rotor;			/* next slave to be instructed */
int master;			/* pid of master, for sending error signals */
int tenths;			/* length of tape used per block written */

alloctape()
{
#ifdef __hpux
	int pgoff = sysconf(_SC_PAGE_SIZE) - 1;
#else /* __hpux */
	int pgoff = getpagesize() - 1;
#endif /* __hpux */

	writesize = ntrec * TP_BSIZE;
	reqsiz = ntrec * sizeof(struct req);
	/*
	 * CDC 92181's and 92185's make 0.8" gaps in 1600-bpi start/stop mode
	 * (see DEC TU80 User's Guide).  The shorter gaps of 6250-bpi require
	 * repositioning after stopping, i.e, streaming mode, where the gap is
	 * variable, 0.30" to 0.45".  The gap is maximal when the tape stops.
	 */
	tenths = writesize/density + (cartridge ? 16 : density == 625 ? 5 : 8);
	/*
	 * Allocate tape buffer contiguous with the array of instruction
	 * packets, so flusht() can write them together with one write().
	 * Align tape buffer on page boundary to speed up tape write().
	 */
	req = (struct req *)malloc(reqsiz + writesize + pgoff);
	if (req == NULL)
		return(0);
	tblock = (char (*)[TP_BSIZE]) (((long)&req[ntrec] + pgoff) &~ pgoff);
	req = (struct req *)tblock - ntrec;
	return(1);
}


taprec(dp)
	char *dp;
{
	req[trecno].dblk = (daddr_t)0;
	req[trecno].count = 1;
	*(union u_spcl *)(*tblock++) = *(union u_spcl *)dp;	/* movc3 */
	trecno++;
	spcl.c_tapea++;
	if(trecno >= ntrec)
		flusht();
}

dmpblk(blkno, size)
	daddr_t blkno;
	int size;
{
	int avail, tpblks, dblkno;

	dblkno = fsbtodb(sblock, blkno);
	tpblks = size / TP_BSIZE;
	while ((avail = MIN(tpblks, ntrec - trecno)) > 0) {
		req[trecno].dblk = dblkno;
		req[trecno].count = avail;
		trecno += avail;
		spcl.c_tapea += avail;
		if (trecno >= ntrec)
			flusht();
		dblkno += avail * (TP_BSIZE / DEV_BSIZE);
		tpblks -= avail;
	}
}

int	nogripe = 0;

tperror() {
	if (pipeout) {
		msg("Tape write error on %s\n", tape);
		msg("Cannot recover\n");
		dumpabort();
		/* NOTREACHED */
	}
	msg("Tape write error %d feet into tape %d\n", asize/120L, tapeno);
	broadcast("TAPE ERROR!\n");
	if (!query("Do you want to restart?"))
		dumpabort();
	msg("This tape will rewind.  After it is rewound,\n");
	msg("replace the faulty tape with a new one;\n");
	msg("this dump volume will be rewritten.\n");
	killall();
	nogripe = 1;
	close_rewind();
	Exit(X_REWRITE);
}

sigpipe()
{

	msg("Broken pipe\n");
	dumpabort();
}

#ifdef __hpux
/*
 * compatibility routine
 */
tflush(i)
	int i;
{

	for (i = 0; i < ntrec; i++)
		spclrec();
}
#else /* __hpux */
#ifdef RDUMP
/*
 * compatibility routine
 */
tflush(i)
	int i;
{

	for (i = 0; i < ntrec; i++)
		spclrec();
}
#endif /* RDUMP */
#endif /* __hpux */

flusht()
{
	int siz = (char *)tblock - (char *)req;

	if (atomic(write, slavefd[rotor], req, siz) != siz) {
		perror("  DUMP: error writing command pipe");
		dumpabort();
	}
	if (++rotor >= slaves) rotor = 0;
	tblock = (char (*)[TP_BSIZE]) &req[ntrec];
	trecno = 0;
	asize += tenths;
	blockswritten += ntrec;
	if (!pipeout && asize > tsize) {
		close_rewind();
		otape();
	}
	timeest();
}

#ifdef __hpux
drewind()
#else /* __hpux */
rewind()
#endif /* __hpux */
{
	int f;
#ifdef __hpux
	int rmterrno = 0;
#endif /* __hpux */

	if (pipeout)
		return;
	for (f = 0; f < slaves; f++)
		close(slavefd[f]);
	while (wait(NULL) >= 0)    ;	/* wait for any signals from slaves */
	msg("Tape rewinding\n");
#ifdef __hpux
	if (host) {
		rmtclose();
		while (rmtopen(tape, 0, rmterrno) < 0)
			sleep(10);
		rmtclose();
		return;
	}
#endif /* __hpux */
#ifdef RDUMP
	if (host) {
		rmtclose();
		while (rmtopen(tape, 0) < 0)
			sleep(10);
		rmtclose();
		return;
	}
#endif /* RDUMP */
	close(to);
	while ((f = open(tape, 0)) < 0)
		sleep (10);
	close(f);
}

close_rewind()
{

#ifdef __hpux
	drewind();
#else /* __hpux */
	rewind();
#endif /* __hpux */

	if (!nogripe) {
		msg("Change Tapes: Mount tape #%d\n", tapeno+1);
		broadcast("CHANGE TAPES!\7\7\n");
	}
	while (!query("Is the new tape mounted and ready to go?"))
		if (query("Do you want to abort?")) {
			dumpabort();
			/*NOTREACHED*/
		}

}

/*
 *	We implement taking and restoring checkpoints on the tape level.
 *	When each tape is opened, a new process is created by forking; this
 *	saves all of the necessary context in the parent.  The child
 *	continues the dump; the parent waits around, saving the context.
 *	If the child returns X_REWRITE, then it had problems writing that tape;
 *	this causes the parent to fork again, duplicating the context, and
 *	everything continues as if nothing had happened.
 */

otape()
{
	int	parentpid;
	int	childpid;
	int	status;
	int	waitpid;
	int	interrupt();
	char	new_tape[256];
#ifdef __hpux
	int	rmterrno = 0;
#endif /* __hpux */

	parentpid = getpid();

    restore_check_point:
	signal(SIGINT, interrupt);
	/*
	 *	All signals are inherited...
	 */
	childpid = fork();

#if defined(TRUX) && defined(B1)
	msg_b1("childpid is: %d\n", childpid);
#endif 

	if (childpid < 0) {
		msg("Context save fork fails in parent %d\n", parentpid);
		Exit(X_ABORT);
	}
	if (childpid != 0) {
		/*
		 *	PARENT:
		 *	save the context by waiting
		 *	until the child doing all of the work returns.
		 *	don't catch the interrupt
		 */
		signal(SIGINT, SIG_IGN);
#ifdef TDEBUG
		msg("Tape: %d; parent process: %d child process %d\n",
			tapeno+1, parentpid, childpid);
#endif /* TDEBUG */
		while ((waitpid = wait(&status)) != childpid)
			msg("Parent %d waiting for child %d has another child %d return\n",
				parentpid, childpid, waitpid);
		if (status & 0xFF) {
			msg("Child %d returns LOB status %o\n",
				childpid, status&0xFF);
		}
		status = (status >> 8) & 0xFF;
#ifdef TDEBUG
		switch(status) {
			case X_FINOK:
				msg("Child %d finishes X_FINOK\n", childpid);
				break;
			case X_ABORT:
				msg("Child %d finishes X_ABORT\n", childpid);
				break;
			case X_REWRITE:
				msg("Child %d finishes X_REWRITE\n", childpid);
				break;
			default:
				msg("Child %d finishes unknown %d\n",
					childpid, status);
				break;
		}
#endif /* TDEBUG */
		switch(status) {
			case X_FINOK:
				Exit(X_FINOK);
			case X_ABORT:
				Exit(X_ABORT);
			case X_REWRITE:
				goto restore_check_point;
			default:
				msg("Bad return code from dump: %d\n", status);
				Exit(X_ABORT);
		}
		/*NOTREACHED*/
	} else {	/* we are the child; just continue */

#ifdef TDEBUG
		sleep(4);	/* allow time for parent's message to get out */
		msg("Child on Tape %d has parent %d, my pid = %d\n",
			tapeno+1, parentpid, getpid());
#endif /* TDEBUG */
		strcpy(new_tape, tape);

#ifdef __hpux
		if ( host ) {
		    while ((to = rmtopen(tape, 2, &rmterrno)) < 0)
			switch(rmterrno) {
			case ENOENT:
		    		if ((to = rmtfilecreate(tape, 0666)) < 0)
			    	   if (!query("Cannot open tape.  Do you want to retry the open?"))
			               dumpabort();
				break;
			default:
				if (!query("Cannot open tape.  Do you want to retry the open?"))
			        dumpabort();
			}
#if defined(TRUX) && defined(B1)
			rmtgetlevel(tape, AUTH_DEV_MULTI, AUTH_DEV_EXPORT, to);
#endif /* (TRUX) && (B1) */
		} else {
		    while ((to = (pipeout ? 1 : creat(new_tape, 0666))) < 0)
			if (!query("Cannot open tape.  Do you want to retry the open?"))
			    dumpabort();
#if defined(TRUX) && defined(B1)
			if (!pipeout) { 
				msg("Checking the input device security... \n");
        			check_device(new_tape, AUTH_DEV_MULTI, AUTH_DEV_EXPORT, to); 
				msg("Completed device Checking\n\n");
			}
			/* else it's stdout which needs no chking */
#endif /*(TRUX) && (B1) */
		}
#else /* __hpux */
#ifdef RDUMP
		while ((to = (host ? rmtopen(tape, 2) :
			pipeout ? 1 : creat(new_tape, 0666))) < 0)
#else /* RDUMP */
		while ((to = pipeout ? 1 : creat(new_tape, 0666)) < 0)
#endif /* RDUMP */
			if (!query("Cannot open tape.  Do you want to retry the open?"))
				dumpabort();
#endif /* __hpux */

		enslave();  /* Share open tape file descriptor with slaves */
		asize = 0;
		tapeno++;		/* current tape sequence */
		newtape++;		/* new tape signal */
		spcl.c_volume++;
		spcl.c_type = TS_TAPE;
		spclrec();
		if (tapeno > 1)
			msg("Tape %d begins with blocks from ino %d\n",
				tapeno, ino);
	}
}

dumpabort()
{
	if (master != 0 && master != getpid())
		kill(master, SIGTERM);	/* Signals master to call dumpabort */
	else {
		killall();
		msg("The ENTIRE dump is aborted.\n");
	}
	Exit(X_ABORT);
}

Exit(status)
{
#ifdef	TDEBUG
	msg("pid = %d exits with status %d\n", getpid(), status);
#endif /* TDEBUG */
	exit(status);
}

/*
 * could use pipe() for this if flock() worked on pipes
 */
lockfile(fd)
	int fd[2];
{
	char tmpname[20];

	strcpy(tmpname, "/tmp/dumplockXXXXXX");
	mktemp(tmpname);
	if ((fd[1] = open(tmpname, (O_CREAT | O_WRONLY), 0777)) < 0) {
	        msg("Could not create first lockfile ");
		perror(tmpname);
		dumpabort();
	}
	if ((fd[0] = open(tmpname, (O_CREAT | O_WRONLY), 0777)) < 0) {
	        msg("Could not create second lockfile ");
		perror(tmpname);
		dumpabort();
	}
	unlink(tmpname);
}

enslave()
{
	int first[2], prev[2], next[2], cmd[2];     /* file descriptors */
	register int i, j;

	slaves = MAXSLAVES;

	master = getpid();
	signal(SIGTERM, dumpabort); /* Slave sends SIGTERM on dumpabort() */
	signal(SIGPIPE, sigpipe);
	signal(SIGUSR1, tperror);    /* Slave sends SIGUSR1 on tape errors */
	lockfile(first);
	for (i = 0; i < slaves; i++) {
		if (i == 0) {
			prev[0] = first[1];
			prev[1] = first[0];
		} else {
			prev[0] = next[0];
			prev[1] = next[1];
#ifdef __hpux
			if (   (lseek(prev[1], 0L, 0) != 0L)
			    || lockf(prev[1], F_LOCK, 0L)) {
				perror("");
				msg("lseek or lockf failed ... ");
				dumpabort();
			}
#else /* __hpux */
			flock(prev[1], LOCK_EX);
#endif /* __hpux */
		}
		if (i < slaves - 1) {
			lockfile(next);
		} else {
			next[0] = first[0];
			next[1] = first[1];	    /* Last slave loops back */
		}
		if (pipe(cmd) < 0 || (slavepid[i] = fork()) < 0) {
			msg("too many slaves, %d (recompile smaller) ", i);
			perror("");
			dumpabort();
		}
		slavefd[i] = cmd[1];
		if (slavepid[i] == 0) { 	    /* Slave starts up here */
			for (j = 0; j <= i; j++)
				close(slavefd[j]);
			signal(SIGINT, SIG_IGN);    /* Master handles this */
			doslave(cmd[0], prev, next);
			Exit(X_FINOK);
		}
		close(cmd[0]);
		if (i > 0) {
			close(prev[0]);
			close(prev[1]);
		}
	}
	close(first[0]);
	close(first[1]);
	master = 0; rotor = 0;
}

killall()
{
	register int i;

	for (i = 0; i < slaves; i++)
		if (slavepid[i] > 0)
			kill(slavepid[i], SIGKILL);
}

/*
 * Synchronization - each process has a lockfile, and shares file
 * descriptors to the following process's lockfile.  When our write
 * completes, we release our lock on the following process's lock-
 * file, allowing the following process to lock it and proceed. We
 * get the lock back for the next cycle by swapping descriptors.
 */
doslave(cmd, prev, next)
	register int cmd, prev[2], next[2];
{
	register int nread, toggle = 0;

	close(fi);
	if ((fi = open(disk, 0)) < 0) { 	/* Need our own seek pointer */
		perror("  DUMP: slave couldn't reopen disk");
		dumpabort();
	}
	/*
	 * Get list of blocks to dump, read the blocks into tape buffer
	 */
	while ((nread = atomic(read, cmd, req, reqsiz)) == reqsiz) {
		register struct req *p = req;
		for (trecno = 0; trecno < ntrec; trecno += p->count, p += p->count) {
			if (p->dblk) {
				bread(p->dblk, tblock[trecno],
					p->count * TP_BSIZE);
			} else {
				if (p->count != 1 || atomic(read, cmd,
				    tblock[trecno], TP_BSIZE) != TP_BSIZE) {
					msg("Master/slave protocol botched.\n");
					dumpabort();
				}
			}
		}
#ifdef __hpux
		if ((lseek(prev[1], 0L, 0) != 0L)
		    || lockf(prev[1], F_LOCK, 0L)) {
			perror("");
			msg("lseek or lockf failed ... ");
			dumpabort();
		}
#else /* __hpux */
		flock(prev[toggle], LOCK_EX);	/* Wait our turn */
#endif /* __hpux */

#ifdef __hpux
		if ((host ? rmtwrite(tblock[0], writesize)
			: write(to, tblock[0], writesize)) != writesize) {
#else /* __hpux */
#ifdef RDUMP
		if ((host ? rmtwrite(tblock[0], writesize)
			: write(to, tblock[0], writesize)) != writesize) {
#else /* RDUMP */
		if (write(to, tblock[0], writesize) != writesize) {
#endif /* RDUMP */
#endif /* __hpux */
			kill(master, SIGUSR1);
			for (;;)
				sigpause(0);
		}
		toggle ^= 1;

#ifdef __hpux
		if (   (lseek(prev[1], 0L, 0) != 0L)
		    || lockf(prev[1], F_ULOCK, 0L)) {
			perror("");
			msg("lseek or lockf failed ... ");
			dumpabort();
		}
#else /* __hpux */
		flock(next[toggle], LOCK_UN);	/* Next slave's turn */
#endif /* __hpux */
	}					/* Also jolts him awake */
	if (nread != 0) {
		perror("  DUMP: error reading command pipe");
		dumpabort();
	}
}

/*
 * Since a read from a pipe may not return all we asked for,
 * or a write may not write all we ask if we get a signal,
 * loop until the count is satisfied (or error).
 */
atomic(func, fd, buf, count)
	int (*func)(), fd, count;
	char *buf;
{
	int got, need = count;

	while ((got = (*func)(fd, buf, need)) > 0 && (need -= got) > 0)
		buf += got;
	return (got < 0 ? got : count - need);
}

#if defined(TRUX) && defined(B1)
/*
 * This routine is called to note an exception, error or auditable condition.
 * The code value selects an element of the list[] array contained in the  file
 * restore.h.  Each element contains a string to be printed to stderr,
 * a string that if not NULL is used in creating an audit record, a string
 * representing the audit result, and action code, which can be either
 * TERMINATE or NOACTION.
 */

audit_rec(code)
	int code;
{
	if(audmsg[code].msg)
		fprintf(stderr,"%s\n",audmsg[code].msg);
	if(audmsg[code].aud_op != NULL)
		audit_subsystem(audmsg[code].aud_op,audmsg[code].aud_res,ET_SUBSYSTEM);
	if(audmsg[code].action == TERMINATE)
		exit(1);
	return(-1);
}

/* check_device(                 *** this is almost a copy of ie_check_device 
 *      device pathname,         *** in libscmd. ie_check_device calls a bread
 *      single or multi level,   *** defined onlyin tar&cpio and checks mlmagic
 *      import or export,        *** I don't use that magic So Ihave to have 
 *      device file descriptor)  *** my own routine.
 *
 * This procedure checks the device to see if it can be used for the requested
 * operation.  It also fills out the needed IR's and flags so the software can
 * handle single/multi levels for the sensitivity label, information label and
 * national caveat set.
 *
 * The routine does not return anything.  It exits upon failure.
 */ 

#include <errno.h>
#include <pwd.h>

/* WATCH OUT this is slightly diff from the check_device in restore directory*/

#define SL_SLABEL       1
#define SL_ILABEL       2
#define SL_NCAVLABEL    4
#define SL_NOT_MULTI    0x100
check_device(dev,lev,direction,fd)
	char *dev;	
	int lev;	/* AUTH_DEV_SINGLE or AUTH_DEV_MULTI */
	int direction;	/* AUTH_DEV_IMPORT,AUTH_DEV_EXPORT,AUTH_DEV_PASS */
	int fd;		/* device file descriptor	      */
{
	struct dev_asg *d;
	char **alias;
	int comp1,comp2;
	int st;
	int i,x;
	struct passwd *pw;
	int asg_impexp;    	/*Remember transfer direction.*/
	struct stat sb;          /* Holds file status structure. */
	char *lab = NULL;
	char *WildCard           = "WILDCARD";
	/*
	 * Pointers for irs. describing the process.
	 */
	mand_ir_t        *proc_ir;      /* process level        */
	mand_ir_t        *proc_clr;     /* process clearance    */
	  /*
	   * Pointers for irs. describing the device.
	   */
	mand_ir_t        *tape_ir;    /* dev default level    */
	mand_ir_t        *tape_max_ir;/* dev max level        */
	mand_ir_t        *tape_min_ir;/* dev min level        */
	int 	level_flags = 0 ;

	if(((proc_ir     = mand_alloc_ir()) == NULL) ||
	   ((proc_clr    = mand_alloc_ir()) == NULL)){
		msg("check_device(): Not enough memory for malloc\n");
		dumpabort();
	}

	tape_ir = fir; /* fir is allocated */
	asg_impexp  = direction;
	/********************
	 *  PASS MODE 
	 ********************/
	/* In pass mode there is no device to check. */
	if (direction == AUTH_DEV_PASS) {
		/* In pass mode we consider the destination directory to be
		 * a device with a range set from the user's clearance
		 * down to syslo.
		 */
		tape_max_ir = proc_clr;
		tape_min_ir = mand_syslo;
		level_flags |= SL_ILABEL;
		level_flags |= SL_NCAVLABEL;

		return;
	}


	/*********************************
	 * IMPORT OR EXPORT
 	 *********************************/

	setgid(starting_rgid());

	/* We do not allow the user to specify a device through indirection */
	if (dev == NULL) {
	   if (direction == AUTH_DEV_IMPORT)
		fprintf(stderr,"ERROR: specify input device with -I option\n");
	   else fprintf(stderr,"ERROR: specify output device with -O option\n");
	   exit(1);
	}

	/* Make sure the device has a proper label. */
	if((statslabel(dev,tape_ir)!=0) && (errno != EINVAL)) 
		   	audit_rec(MSG_DEVLAB);

	msg_b1("*ie_check_device():\n");
	msg_b1(" tape: %s\n",dev);

	/* Check the device assignment data base to see if the requested device
	 * has been configured by the SSO.
	 */

	/* forcepriv(SEC_ALLOWDACACCESS); */

	while( (d = getdvagent()) != NULL) {	/* get a device record. */
   		if(d->uflg.fg_devs) {
			/* Get pointer to the list of valid device pathnames	
			 * for this device.
			 */
      			alias = d->ufld.fd_devs;
	
			/* Search the list for a match for our device. */
      			while(*alias != NULL) {
         			if(strcmp(dev,*alias) == 0) break;
	 			alias++; 
      			}
       		}
		/* break out of loop if we found a match. */
   		if(*alias != NULL) break;
	}

	/* njh if(!hassysauth(SEC_ALLOWDACACCESS))
		disablepriv(SEC_ALLOWDACACCESS); */

	/* If we didn't find the device in the data base, then only accept if
	 * it is a regular file or the file does not exist.
	 */
	if (d == NULL) {
		st = stat(dev,&sb);

	   	if (((st ==  0) && ((sb.st_mode & S_IFMT) == S_IFREG)) ||
		    ((st == -1) && (errno == ENOENT))) {
			/* Input/Output to disk file.  Consider the disk file 
			 * to be a multilevel device ranging from the 
			 * process clearance down to syslo.
		 	 */
	      		tape_ir     = proc_ir;
	      		tape_min_ir = mand_syslo;
	      		tape_max_ir = proc_clr;


			/* If ILB's are not configured, we will consider the 
			 * file to be a single-level.
	 		 */
			level_flags |= SL_ILABEL;

			/* If NCAV's are not configured, we will consider the
			 * file to be single-level device.
			 */
			level_flags |= SL_NCAVLABEL;

	      		msg_b1("ie_check_device(): disk save set specified\n");
	      		return;
		}
		/* Trying to redirect to device, note error and exit. */
		else audit_rec(MSG_DEVDB);
	}

	/* 
	 * Is user authorized to use this device?
	 */
	if (d->uflg.fg_users) {
		pw = getpwuid(getluid());    /* Get user name. */
		alias = d->ufld.fd_users;    /* Get list of allowed users */
		while(*alias != NULL)  {     /* Search for match. */
			if(strcmp(pw->pw_name,*alias) == 0) break;
				alias++; 
		}
		if(*alias == NULL) audit_rec(MSG_AUTHDEV);
	}

	/* 
	 * Make sure that the device is authorized for the requested operation,
	 * i.e. import or export.
	 */
	if (!d->uflg.fg_assign) audit_rec(MSG_DEVIMPEXP);
	if(!ISBITSET(d->ufld.fd_assign,asg_impexp)) 
           audit_rec(MSG_DEVIMPEXP);

	/*********************************************
	 * Check the device sensitivity label range.
	 ********************************************/
	/* Is it a multi-level device? */
	if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_MULTI)) {
		/* Can only assign it one way. */
		if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_SINGLE)) 
			audit_rec(MSG_DEVASSIGN);

		if (d->uflg.fg_max_sl) 
			tape_max_ir = d->ufld.fd_max_sl;
		else if (d->sflg.fg_max_sl) {
			tape_max_ir = d->sfld.fd_max_sl;
		}
		else audit_rec(MSG_DEVSENSLEV);

		if (d->uflg.fg_min_sl) 
			tape_min_ir = d->ufld.fd_min_sl;
		else if (d->sflg.fg_min_sl) {
			tape_min_ir = d->sfld.fd_min_sl;
		}
		else audit_rec(MSG_DEVSENSLEV);
	}
	/* Is it a single level device? */
	else if (ISBITSET(d->ufld.fd_assign,AUTH_DEV_SINGLE)) {
		if (d->uflg.fg_cur_sl) {
			tape_ir = d->ufld.fd_cur_sl;
			level_flags |= SL_SLABEL;
		}
		/* Is it a system default single level? */
		else if (d->sflg.fg_cur_sl) {
			tape_ir = d->sfld.fd_cur_sl;
			level_flags |= SL_SLABEL;
		}
		else audit_rec(MSG_DEVNOLEV);
	}

	/* The SSO must specify the device in one of the options above, else
	 * we have an error, so notify the user and exit. 
	 */
	else audit_rec(MSG_DEVNOLEV);

	/* If single-level, only files that match the designated level
	 * exactly can be exported.  Restrict the min/max_ir to the 
	 * designated device level.
	 */
	if (level_flags & SL_SLABEL) {
		tape_max_ir = tape_ir;
		tape_min_ir = tape_ir;
#ifdef B1DEBUG
		if (tape_ir) lab = mand_ir_to_er(tape_ir);
		else lab = WildCard;
		msg_b1("Device single-level for sensitivity labels: %s\n",lab);
#endif
	}
	else {	/* If the device is multi-level, make sure that we are using
		 * one of the multilevel programs.
		 */
	   if (lev != AUTH_DEV_MULTI) {
		fprintf(stderr,
		       "ERROR: the device is configured for multi-level sls\n");
		exit(1);
	   }
#ifdef B1DEBUG
	   if (tape_max_ir) lab = mand_ir_to_er(tape_max_ir);
	   else lab = WildCard;
	   msg_b1("Device multi-level for sens. labels, max: %s\n",lab);
	   if (tape_min_ir) lab = mand_ir_to_er(tape_min_ir);
	   else lab = WildCard;
	   msg_b1("                                     min: %s\n",lab);
#endif B1DEBUG
	}

	/*
	 * Now make sure that the specified sensitivity range is valid.
	 */
	/* No wildcards allowed. */
	if(tape_max_ir == (mand_ir_t *)0) audit_rec(MSG_DEVMAXLEV);
	if(tape_min_ir == (mand_ir_t *)0) audit_rec(MSG_DEVMINLEV);

  	/* Determine the relationship between the process's clearance and the 
	 * device. 
	 */

	/* njh forcepriv(SEC_ALLOWMACACCESS); */

   	comp1 = mand_ir_relationship(proc_clr,tape_max_ir);
   	comp2 = mand_ir_relationship(proc_clr,tape_min_ir);

	/* njh
	if(!hassysauth(SEC_ALLOWMACACCESS))
		disablepriv(SEC_ALLOWMACACCESS);
	*/

	/* For single-level devices, set the process level equal to 
	 * the device label.
 	 */

	/* njh forcepriv(SEC_ALLOWMACACCESS); */

	if (level_flags & SL_SLABEL) {
		proc_ir     = tape_ir;
	 	if(setslabel(proc_ir) < 0) {
			/* njh
			if(!hassysauth(SEC_ALLOWMACACCESS))
				disablepriv(SEC_ALLOWMACACCESS);
			*/
			audit_rec(MSG_CHGLEV);
		}
	}

	/* njh
	if(!hassysauth(SEC_ALLOWMACACCESS))
		disablepriv(SEC_ALLOWMACACCESS);
	*/

	/*******************************************
	 * Check the device information label range.
	 *******************************************/
	/* If ILB's are not configured for this system, then we will ignore
	 * them by considering the device to be single level.
	 */
	level_flags |= SL_ILABEL;
	msg_b1("Information labels not configured.\n");

	/* There are no checks here on the value for the information label, for
	 * we allow the wildcard on single-level devices.
	 */


	/*****************************************
	 * Check the device national caveat range.
	 *****************************************/
	/* If NCAV's are not configured for this system, then we will ignore
	 * them by considering the device to be single level.
	 */
	level_flags |= SL_NCAVLABEL;
	msg_b1("National Caveats not configured.\n");

	/* If we have made it this far, we have passed all tests and import/
	 * export will proceed.  
	 */
}
void
writehdr()
{
}
void
bwrite()
{
}
#endif  /* TRUX && B1 */

