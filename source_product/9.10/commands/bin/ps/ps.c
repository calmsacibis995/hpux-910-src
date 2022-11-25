#ifndef lint
static char *HPUX_ID = "@(#) ps.c  $Revision: 72.2 $ $Date: 92/12/08 15:32:21 $";
#endif

/*
 * PS based on 8.0 pstat()
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/pstat.h>
#include <pwd.h>
#include <search.h>  
#include <sys/sysmacros.h> /* XXX for major(), minor(). do it here? */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#if defined(SecureWare) && defined(B1)
#include <sys/security.h>
#endif

#ifdef FSS
#include <fsg.h>
#include <sys/fss.h>
#include <errno.h>
#endif /* FSS */

extern ENTRY *hsearch();

extern struct passwd *getpwuid(), *getpwnam();
extern char *malloc(), *strchr(), *strrchr();
extern char *ttyname(), *ctime(), *optarg;
extern time_t time();

#ifdef SecureWare 
static char psfile[] = "/etc/ps/ps_data"; /* multilevel dir for ps info */
#else
#ifdef V4FS
static char psfile[] = "/var/adm/ps_data"; /* data cache for 'static' info */
#else /* V4FS */
static char psfile[] = "/etc/ps_data"; /* data cache for 'static' info */
#endif /* V4FS */
#endif

#define PSBURST 10   /* # of pstat entries to read at a time */

#define MAXENT 30 /* # of pids, grps in a cmdline list */
int pidlist[MAXENT], grplist[MAXENT];
int npid = 0, ngrp = 0;

#ifdef FSS
int fssincluded = 0;
int fsidlist[MAXENT];
int nfsid = 0;
int Fflg = 0, Gflg = 0;
#endif /* FSS */

int eflg = 0, dflg = 0, aflg = 0, fflg = 0, lflg = 0;
int tflg = 0, uflg = 0, pflg = 0, gflg = 0, errflg = 0;

char buf[40];  /* general purpose */

int   numdev;
struct devinfo {
	char  d_name[14];
	int   d_major;
	int   d_minor;
	int   d_want;
};

struct devinfo *devtab;

/*
 * Define a list of directories to be searched for tty names.  This
 * list also defines some directories which are required and some
 * which are not.  If required directories do not exist in the system,
 * the program aborts.  /dev is the only current required directory.
 */

struct ttydirs {
	char *name;
	int  required;
};

struct ttydirs ttydir_list[] = {
	"/dev", 1,
	"/dev/pty", 0,
	NULL, 0
};

struct psdev console_device;


long maxuid;  /* max no. of users now retrieved via pstat call */
	      /* instead of being hardcoded as a constant      */
	      /* MAXUID. This is for 3000 user at 9.0 release  */
	      /* AL 11/13/91   */

struct uidinfo {
	char  u_name[14];
	int   u_uid;
	int   u_want;
};

/* flag indicating that information about processes not associated
   with a terminal should be printed */
int	w_noterm;


/*
 * Output related routines 
 *
 */


#ifdef FSS
char *
fsgname(fsid)
int fsid;
{
	struct fsg *g;
	static char buf[9];

	if ((g = getfsgid(fsid)) == NULL) {
		sprintf(buf, "%d", fsid);
		return buf;
	} else
		return g->fs_grp;
}

printfsid(pfsid)
int pfsid;
{
	if (fflg)
		printf(" %7s", fsgname(pfsid));
	else if (lflg)
		printf(" %7d", pfsid);
	else if (Fflg)
		printf(" %7s", fsgname(pfsid));
}

#define  fss_fprintf(fp, fmt) if (fssincluded) fprintf(fp, fmt)
#else
#define  fss_fprintf(fp, fmt) /* nothing */
#endif /* FSS */

/* Turn state value into printing string */
char 
state_name(state)
int state;
{
	switch (state) {
	case PS_SLEEP:	return('S');
	case PS_RUN:	return('R');
	case PS_STOP:	return('T');
	case PS_ZOMBIE:	return('Z');
	default:	return('I');
	}
}

/* Output banner line */

void banner()
{
	if (fflg && lflg) {
		printf("  F S     UID");
		fss_fprintf(stdout, "    FSID");
		printf("   PID  PPID  C PRI NI     ADDR   SZ    WCHAN    STIME TTY      TIME COMD\n");
	} else if (fflg) {
		printf("     UID");
		fss_fprintf(stdout, "    FSID");
		printf("   PID  PPID  C    STIME TTY      TIME COMMAND\n");
	} else if (lflg) {
		printf("  F S   UID");
		fss_fprintf(stdout, "    FSID");
		printf("   PID  PPID  C PRI NI     ADDR   SZ    WCHAN TTY      TIME COMD\n");
	} else {
#ifdef FSS
		if (Fflg)
			fss_fprintf(stdout, "    FSID");
#endif /* FSS */
		printf("   PID TTY      TIME COMMAND\n");
	}
}


/*
 * Copy command c into cmd mapping non-printable characters and trimming
 * The mapping algorithm isn't really very good, and should be examined
 * closely in the future. In particular, the impact to NLS users is unknown.
 * It was modified from a previous version of ps.
 */
void
getcmd(cmd, c)
char cmd[PST_CLEN];
register char *c;
{
	register char *dptr;
	char  newc;

	/* Map chars 0 - 037 to ^C where C = char + 0100 */
	/* Map chars over 0x7F to ^@ */
	dptr = cmd;
/* 
  The following loop was modified to fix DSDe407917.  The for loop used to be
  	for (; *c || ((dptr - cmd) >= PST_CLEN); c++) {
  which is incorrect because for as long as *c (character pointed to in c) is
  not NULL, then the loop is executed without regard for boundary condition!!
  This was causing a core dump on 300s (8.3 release).   AL 12/8/92
*/
	for (; *c && ((dptr - cmd) < PST_CLEN); c++) {
		if (((*c) < ' ') || ((*c) > 0x7F)) {
			*dptr++ = '^';
			if (dptr - cmd >= PST_CLEN)
				break;
			newc = (((*c) & 0x7F) + '@');
			if ((newc < ' ') || (newc > '~'))
				newc = '@';
			*dptr++ = newc;
			if ((dptr - cmd) >= PST_CLEN)
				break;
		} else
			*dptr++ = *c;
	}
	if ((dptr - cmd) >= PST_CLEN)
		cmd[PST_CLEN - 1] = '\0';
	else
		*dptr = '\0';

	/* Trim command name to fit screen & options */
	if (!fflg) {
		if (dptr = strchr(cmd, ' '))
			*dptr = '\0';
		if (dptr = strrchr(cmd, '/'))
			strcpy(cmd, dptr+1);
	}
}

/* print starting time of process unless process started more */
/* than 24 hours ago in which case date is printed   */
/* sttim is start time and it is compared to curtim (current time ) */

void prtim(curtim,sttim)
char *curtim;
char *sttim;
{
	char *p1, *p2;
	char dayst[3], daycur[3];
	if ( strncmp(curtim, sttim, 11) == 0) {
		p1 = sttim + 11;
		p2 = p1 + 8;
	} else {
		p1 = sttim + 4;
		p2 = p1 + 7;
		/* if time is < 24 hours different, then print time */
		if (strncmp(curtim+4, sttim+4, 3) == 0) {
			strncpy(dayst,sttim+8, 2);
			strcat(dayst,"");
			strncpy(daycur,curtim+8,2);
			strcat(daycur,"");
			if ((atoi(dayst) +1 == atoi(daycur)) &&
			   (strncmp(curtim+11,sttim+11,8)<=0)) {
				p1 = sttim + 11;
				p2 = p1 + 8;
			}
		}
	}
	*p2 = '\0';
	printf(" %8.8s",p1);
}



/* Output an info line for one process */
display(ps)
register struct pst_status *ps;
{
	char *uname;
	int uid;
	char *term;
	long size;
	ENTRY item, *itemptr;
	struct   passwd *pass;
	int runtime;
	char curtim[30];
	char *sttim;
	time_t   tim;
	char cmd[PST_CLEN];

	/* Look up UID */
	uid = ps->pst_uid;
	sprintf(buf,"U%d",uid);
	item.key = buf;
	if (fflg)  /* Added the check for -f option to avoid unnecessary uname
                      lookup. This improves ps performance on 8_0 and 9_0
                      considerably by not using the -f option. This is
                      effectively a workaround until we fix the getpw* routines
                     and make them more efficient. See DSDe408291.  */
	   if (itemptr = hsearch(item,FIND)) {
		uname = ((struct uidinfo *)(itemptr->data))->u_name;
	   } else {
		pass = getpwuid(uid);
		if (pass == NULL)
			uname = NULL;
		else {
			uname = pass->pw_name;
			add_uid_entry(uid,uname,0);
		}
   	   }

	/* Look up TTY */
	sprintf(buf,"D%d,%d",ps->pst_major,ps->pst_minor);
	item.key = buf;
	if (itemptr = hsearch(item,FIND)) {
		term = ((struct devinfo *)(itemptr->data))->d_name;
	} else {
		term = "?";
	}

#ifdef SecureWare
	if((ISSECURE) && (!ps_print_entry(ps->pst_uid, term)))
		return(0);
#endif

	/* Calculate TIME info */
	/* get runtime for all */
	runtime = ps->pst_utime + ps->pst_stime;
 
	if (fflg) { /* STIME calcs for '-f' only */
		tim = time((time_t *) 0);
		strcpy(curtim,ctime(&tim));
		sttim = ctime((time_t *)(&ps->pst_start));
	}

	/* Calculate SIZE in K of program (-l only) */
	if (lflg) {
		size = ps->pst_dsize + ps->pst_tsize + ps->pst_ssize;
	}

	/* Get command name with non-printing char mapping */
	if (!lflg && ps->pst_stat == PS_ZOMBIE)
		strcpy(cmd, "<defunct>");
/* The following 4 lines were added/modified to fix DTS# FSDlj09238  */
	else if (fflg)                     
		getcmd(cmd, ps->pst_cmd); 
	else
		getcmd(cmd, ps->pst_ucomm);

	/* Print it out */
	if (fflg && lflg) {
		if (uname == NULL)
			printf("%3o %c %7u",
				ps->pst_flag,state_name(ps->pst_stat),uid);
		else
			printf("%3o %c %7.7s",
				ps->pst_flag,state_name(ps->pst_stat),uname);
#ifdef FSS
		if (fssincluded)
			printfsid(ps->pst_fss); /* fair share group ID */
#endif /* FSS */
		printf(" %5u %5u %2d %3d %2d %8x %4d",
			ps->pst_pid, ps->pst_ppid, ps->pst_cpu, ps->pst_pri,
			ps->pst_nice, ps->pst_addr, size);
		if (ps->pst_wchan) {
			printf(" %8x",ps->pst_wchan);
		} else {
			printf("         ");
		}
		prtim(curtim,sttim);
		printf(" %-7.7s %2ld:%.2ld %.35s\n",
			term,runtime/60,runtime%60,cmd);
	} else if (fflg) {
		if (uname == NULL)
			printf("%8u",uid);
		else
			printf("%8.8s",uname);
#ifdef FSS
		if (fssincluded)
			printfsid(ps->pst_fss); /* fair share group ID */
#endif /* FSS */
		printf(" %5u %5u %2d", ps->pst_pid,ps->pst_ppid,ps->pst_cpu);
		prtim(curtim,sttim);
		printf(" %-7.7s %2ld:%.2ld %.80s\n",
			term,runtime/60,runtime%60,cmd);
	} else if (lflg) {
		printf("%3o %c %5u", ps->pst_flag,state_name(ps->pst_stat),uid);
#ifdef FSS
		if (fssincluded)
			printfsid(ps->pst_fss); /* fair share group ID */
#endif /* FSS */
		printf(" %5u %5u %2d %3d %2d %8x %4d",
			ps->pst_pid, ps->pst_ppid, ps->pst_cpu, ps->pst_pri,
			ps->pst_nice, ps->pst_addr, size);
		if (ps->pst_wchan) {
			printf(" %8x",ps->pst_wchan);
		} else {
			printf("         ");
		}
		printf(" %-7.7s %2ld:%.2ld %.14s\n",
			term,runtime/60,runtime%60,cmd);
	} else {
#ifdef FSS
		if (fssincluded) {
			printfsid(ps->pst_fss); /* fair share group ID */
			printf(" ");
		}
#endif /* FSS */
		printf("%6u %-7.7s %2ld:%.2ld %.14s\n",
			ps->pst_pid,term,runtime/60,runtime%60,cmd);
	}
	return(1);
}



/*
 * Selection routines
 *
 */


in_tlist(termptr)
struct psdev   *termptr;
{
	ENTRY item, *itemptr;

	sprintf(buf,"D%d,%d",termptr->psd_major,termptr->psd_minor);
	item.key = buf;
	if (itemptr = hsearch(item,FIND)) { /* find the device */
		return(((struct devinfo *)(itemptr->data))->d_want);
	}
	if (w_noterm)
		return(1);
	else
		return(0);
}

in_ulist(uid)
int uid;
{
	ENTRY item, *itemptr;
	struct uidinfo *uidinf;
	struct passwd *pass;
	char *uname=NULL;

	sprintf(buf,"U%d",uid);
	item.key = buf;
	if (itemptr = hsearch(item,FIND)) {
		return(((struct uidinfo *)(itemptr->data))->u_want);
	}
	if (fflg){ /* Added the check for -f option to avoid unnecessary uname
                      lookup. This improves ps performance on 8_0 and 9_0
                      considerably by not using the -f option. This is
                      effectively a workaround until we fix the getpw* routines
                     and make them more efficient. See DSDe408291.  */
                pass = getpwuid(uid);
                if (pass != NULL)
                        uname = pass->pw_name;
        }
	add_uid_entry(uid,uname,0);
	return(0);
}


in_plist(pid)
int pid;
{
	int   i;

	for (i = 0; i < npid; i++) {
		if (pid == pidlist[i])
			return(1);
	}
	return(0);
}


in_glist(pgrp)
int pgrp;
{
	int   i;

	for (i = 0;i < ngrp; i++) {
		if (pgrp == grplist[i])
			return(1);
	}
	return(0);
}

#ifdef FSS
in_Glist(fsid)
int fsid;
{
	int   i;

	for (i = 0;i < nfsid; i++) {
		if (fsid == fsidlist[i])
			return(1);
	}
	return(0);
}
#endif /* FSS */

static
want(ps)
register struct pst_status *ps;
{
	ENTRY item;

	/* Convert true console device to '/dev/console' alias */
	if ((ps->pst_major == console_device.psd_major) &&
	    (ps->pst_minor == console_device.psd_minor)) {
		ps->pst_major = 0;
		ps->pst_minor = 0;
	}

	/* Under "all" functions, always want to see */
	if (eflg) {
		return(1);
	}
	if (dflg) { /* but is it a pgrp leader ? */
		if (ps->pst_pid == ps->pst_pgrp) {
			return(0);
		}
		return(1);
	}
	if (aflg) {
		if (ps->pst_pid != ps->pst_pgrp) {
			sprintf(buf, "D%d,%d", ps->pst_major, ps->pst_minor);
			item.key = buf;
			if (hsearch(item, FIND))
				return(1);
		}
		return(0);
	}
	if (tflg) {
		if (in_tlist(&ps->pst_term)) {
			return(1);
		}
	}
	if (uflg) {
		if (in_ulist(ps->pst_uid)) {
			return(1);
		}
	}
	if (pflg) {
		if (in_plist(ps->pst_pid)) {
			return(1);
		}
	}
	if (gflg) {
		if (in_glist(ps->pst_pgrp)) {
			return(1);
		}
	}
#ifdef FSS
	if (fssincluded && Gflg) {
		if (in_Glist(ps->pst_fss)) {
			return(1);
		}
	}
#endif /* FSS */
	return(0);
}

/*
 * Get the console device alias
 */
getcons()
{
	struct pst_static statbuf;
	int ret;


#if defined(TRUX) && defined(B1)
	if (ISB1)
		(void) forcepriv(SEC_OWNER);
#endif

	ret = pstat(PSTAT_STATIC, &statbuf, sizeof(statbuf), 1, 0);

#if defined(TRUX) && defined(B1)
	if(ISB1)
		(void) disablepriv(SEC_OWNER);
#endif

	if (ret < 0) {
		fprintf(stderr, "ps: couldn't get console device alias\n");
		return;
	}
	console_device = statbuf.console_device;
	maxuid = statbuf.max_proc;    /*  Added for 3000 user supp */
}

/*
 * 'Static' data gathering routines
 *
 */

getdata()
{
	register struct devinfo *devptr;
	int i;
	ENTRY item;
	char *keyptr;

	if (!readdata()) {	/* did we get devs OK from cache ? */
		getdevs();	/* No, read in from /dev. */
		writedata();	/* and cache. */
	}

	/* now have devs from some source.  Load up the hash table. */

	if (hcreate(maxuid+numdev) == NULL) {  
		fprintf(stderr,"ps: not enough memory for hash table\n");
		exit(1);
	}

	for (i = 0, devptr = devtab; i < numdev; i++, devptr++) {
		devptr->d_want = 0;  /* initially none selected. */
		sprintf(buf,"D%d,%d",devptr->d_major,devptr->d_minor);
		if ((keyptr = malloc(strlen(buf)+1)) == NULL) {
			fprintf(stderr,"ps: not enough memory for tables\n");
			exit(1);
		}
		strcpy(keyptr, buf);
		item.key = keyptr;
		item.data = (char *) devptr;
		hsearch(item,ENTER);
	}
}


/*
 * readdata() - try to read in cached device information from disk 
 * Success - 1; Failure - 0.
 */

readdata()
{
	int   fd;
	struct stat sbuf1, sbuf2, sbuf3;
	struct ttydirs *tptr;

	devtab = 0; /* pessimists */

#if defined(SecureWare) && defined(B1)
	if (ISB1) {
		(void) forcepriv(SEC_ALLOWMACACCESS);
		(void) forcepriv(SEC_ALLOWDACACCESS);
		(void) forcepriv(SEC_OWNER);
		(void) forcepriv(SEC_SETOWNER);
		(void) forcepriv(SEC_SUSPEND_AUDIT);
	}
#endif

	/* check for out-of-dateness */

	if (stat(psfile, &sbuf1) < 0) {  /* did it exist? */
		goto abort;
	}

	/* see if ps_data file is older than any of the directories containing
	   ttys */
	for(tptr = ttydir_list; tptr->name != NULL; tptr++) {
		if (stat(tptr->name, &sbuf2) < 0) {
			if (tptr->required)
				goto abort;
			else
				continue;
		}
		if (sbuf1.st_mtime <= sbuf2.st_mtime)
			goto abort;
	}

	/* was ps_data file created by an old version of ps? */
#ifdef V4FS
	if ((stat("/usr/bin/ps", &sbuf3) < 0) || (sbuf1.st_mtime <= sbuf3.st_mtime))
#else

	if ((stat("/bin/ps", &sbuf3) < 0) || (sbuf1.st_mtime <= sbuf3.st_mtime))
#endif /* V4FS */
		goto abort;
	if ((fd = open(psfile,O_RDONLY)) < 0)
		goto abort;

#if defined(SecureWare) && defined(B1)
	if(ISB1){
		(void) disablepriv(SEC_ALLOWMACACCESS);
		(void) disablepriv(SEC_ALLOWDACACCESS);
		(void) disablepriv(SEC_OWNER);
		(void) disablepriv(SEC_SETOWNER);
		(void) disablepriv(SEC_SUSPEND_AUDIT);
	}
#endif

	if (!psread(fd,&numdev,sizeof(numdev)))
		return(0);
	if (numdev <= 0)
		return(0);
	if ((devtab =(struct devinfo *)
	     malloc(numdev*sizeof(struct devinfo))) == NULL) {
		fprintf(stderr,"ps: not enough memory for tables\n");
		exit(1);
	}
	if (!psread(fd,devtab,numdev*sizeof(struct devinfo))) {
		free(devtab);
		devtab = 0;
		numdev = 0;
		return(0);
	}
	close(fd);
	return(1);

abort:
#if defined(SecureWare) && defined(B1)
	if(ISB1){
		(void) disablepriv(SEC_ALLOWMACACCESS);
		(void) disablepriv(SEC_ALLOWDACACCESS);
		(void) disablepriv(SEC_OWNER);
		(void) disablepriv(SEC_SETOWNER);
		(void) disablepriv(SEC_SUSPEND_AUDIT);
	}
#endif
	return(0);
}

writedata()
{
	int   fd;
	int   flags;
	mode_t sv_mask;

	sv_mask = umask(S_IWOTH);
#ifdef SecureWare
	if(ISSECURE)
		fd = ps_open_file_securely(psfile);
	else {

		/* If we're root, we want the CREAT flag, so that the file will
		 * be created.  Otherwise, we just want to over-write the file.
		 */
		if ((getuid() == 0) || (geteuid() == 0))
			flags = O_CREAT;
		else
			flags = 0;
		fd = open(psfile, O_WRONLY | O_TRUNC | flags, 0664);
	}
	if (fd > -1)
#else

	/* If we're root, we want the CREAT flag, so that the file will be
	 * created.  Otherwise, we just want to over-write the file.
	 */
	if ((getuid() == 0) || (geteuid() == 0))
		flags = O_CREAT;
	else
		flags = 0;
	if((fd = open(psfile, O_WRONLY | O_TRUNC | flags, 0664)) > -1)
#endif
	{
		pswrite(fd, &numdev, sizeof(numdev));
		pswrite(fd, devtab, numdev * sizeof(struct devinfo));

		close(fd);
#ifdef SecureWare
		if(ISSECURE)
			ps_cleanup_new_file();
#endif
	}
	umask(sv_mask);
}

/* 
 * getdevs() 
 * read from directories containing ttys, filling in devinfo
 * structs.  numdev is set to the number of devices found.
 * skip the console aliases systty and syscon.
 * Change for 3000 user support: devtab allocation changed to
 * dynamically grow as the number of devices increase beyond
 * the old hard-coded value of MAXDEV (4096). So now if we have
 * more than 4096 dev's, we simply realloc devtab in bursts of
 * 100 entries at a time.
 */

#define INIT_DEVTAB_SIZE  4096   /*Initial size of devtab which
				  *should work on most systems  */

#define INCR_DEVTAB_ENTRIES 100  /*Incremental increase for devtab
				  *to accommodate more devices on
				  *systems like Emerald         */
getdevs()
{
	struct stat stbuf;
	struct direct *dp;
	DIR *dirp;
	struct devinfo *devptr;
	struct ttydirs *tptr;
	int end_offset, burst_count = 0;

	numdev = 0;
	if ((devtab = (struct devinfo *) 
	     malloc(INIT_DEVTAB_SIZE * sizeof(struct devinfo))) == NULL) {
		fprintf(stderr,"ps: out of memory\n");
		exit(1);
	}
	devptr = devtab;
	for(tptr = ttydir_list; tptr->name != NULL; tptr++) {
		if ((dirp = opendir(tptr->name)) == NULL) {
			if (tptr->required) {
				fprintf(stderr, "ps: cannot open %s\n", tptr->name);
				exit(1);
			} else
				continue;
		}
		if (chdir(tptr->name) < 0) {
			if (tptr->required) {
				fprintf(stderr, "ps: cannot change to %s\n", tptr->name);
				exit(1);
			} else {
				closedir(dirp);
				continue;
			}
		}
		for(dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
			if (stat(dp->d_name, &stbuf) < 0)
				continue;
			if (((stbuf.st_mode&S_IFMT) != S_IFCHR) 
			      || (!strcmp(dp->d_name,"syscon")) 
			      || (!strcmp(dp->d_name, "systty")))
				continue;
			strcpy(devptr->d_name, dp->d_name);
			devptr->d_major = major(stbuf.st_rdev);
			devptr->d_minor = minor(stbuf.st_rdev);
			numdev++;
			devptr++;
			if (numdev >= INIT_DEVTAB_SIZE + burst_count*INCR_DEVTAB_ENTRIES) {
/* Since realloc may actually change the location of devtab, 
 * remember the offset to the end of the table and then add 
 * it back after the realloc call.  */
			   end_offset = devptr - devtab;
			   if ((devtab = (struct devinfo *) 
	   		    realloc(devtab, (numdev+INCR_DEVTAB_ENTRIES) * sizeof(struct devinfo))) == NULL) {
			     fprintf(stderr,"ps: out of memory\n");
			     exit(1);
		  	   }
			   devptr = devtab + end_offset;
			   burst_count++;
	  		}
		}
		closedir(dirp);
	}
}


/* special read unlinks psfile on read error */
psread(fd, bp, bs)
int fd;
char *bp;
unsigned bs;
{
	if(read(fd, bp, bs) != bs) {
		fprintf(stderr, "ps: error on read\n");
		unlink(psfile);
		return(0);
	}
	return(1);
}

/* special write unlinks psfile on write error */
pswrite(fd, bp, bs)
int fd;
char *bp;
unsigned bs;
{
	if(write(fd, bp, bs) != bs) {
		fprintf(stderr, "ps: error on write\n");
		unlink(psfile);
		return(0);
	}
	return(1);
}


/*
 * Cmd line parsing
 * and
 * hash table misc
 *
 */

add_uid_entry(uid,uname,want)
int   uid;
char  *uname;
int   want;
{
	struct uidinfo *uidinf;
	ENTRY  item;
	char   *keyptr;

	if ((uidinf = (struct uidinfo *)
	     malloc(sizeof(struct uidinfo))) == NULL) {
		fprintf(stderr,"ps: out of memory\n");
		exit(1);
	}
	sprintf(buf,"U%d",uid);
	if ((keyptr = malloc(strlen(buf)+1)) == NULL) {
		 fprintf(stderr,"ps: not enough memory for tables\n");
		 exit(1);
	}
	strcpy(keyptr, buf);
	item.key = keyptr;
	strcpy(uidinf->u_name,uname);
	uidinf->u_uid = uid;
	uidinf->u_want = want;
	item.data = (char *) uidinf;
	hsearch(item,ENTER);
}


/*
 * getarg(arglistptr): given
 * 'foo <comma|space> bar <comma|space> ...'
 * return 'foo', and advances arglist ptr to 'bar...'
 */
char *getarg(arglistptr)
char **arglistptr;
{
	register char *arg = *arglistptr;
	register char *s = arg;
	register char c;

	while ((c = *s) && (c != ' ') && (c != ','))
		s++;  /* eat till eol or delim */
	if (c) { /* if not eol, eat delims */
		*s++ = '\0';
		while ( ((c = *s) == ' ') || (c == ',') )
			s++;
	}
	*arglistptr = s;
	return(arg);
}


add_term()
{
	char *termlist = optarg;
	char *term;
	char *fullname;
	int size;
	int i;
	struct stat stbuf;
	ENTRY item, *itemptr;
	short found;
	int max_length;
	struct ttydirs *tptr;

	/* calculate space required to assemble full path name */
	max_length = 0;
	for(tptr = ttydir_list; tptr->name != NULL; tptr++)
		if (strlen(tptr->name) > max_length)
			max_length = strlen(tptr->name);
	if ((fullname = malloc(max_length + 11)) == NULL) {
		fprintf(stderr,"ps: out of memory\n");
		exit(1);
	}

	do {
		term = getarg(&termlist);
		if (!strcmp("?", term)) {
			/* special case to allow user to request processes
			   not associated with a terminal */
			w_noterm = 1;
			continue;
		}
		found = 0;
		for(tptr = ttydir_list; tptr->name != NULL; tptr++) {
			strcpy(fullname, tptr->name);
			strcat(fullname, "/");
			if ((strncmp("tty",term,3)) &&
			    (strcmp("console",term)) &&
			    (strcmp("syscon",term)) &&
			    (strcmp("systty",term))) {
				strncat(fullname,"tty",3);
				strncat(fullname,term,6);
			} else {
				strncat(fullname,term,9);
			}
			if (stat(fullname, &stbuf) < 0)
				continue;   /* bum device */
			if ((stbuf.st_mode&S_IFMT) == S_IFCHR) {
				found = 1;
				break;
			}
		}
		if (!found)
			continue;
		sprintf(buf,"D%d,%d",major(stbuf.st_rdev),minor(stbuf.st_rdev));
		item.key = buf;
		itemptr = hsearch(item,FIND);
		if (itemptr == NULL) {
			printf("error: could not find terminal %s\n", term);
			exit(1);
		}
		((struct devinfo *)(itemptr->data))->d_want = 1;
	} while (*termlist);
}



add_uid()
{
	char *uidlist = optarg;
	char *user;
	int  uid;
	ENTRY item, *itemptr;
	struct passwd *pass;
	struct uidinfo *uidinf;

	do {
		user = getarg(&uidlist);
		if ((user[0] >= '0') && (user[0] <= '9')) {
			/* a UID number */
			uid = atoi(user);
			item.key = user;
			if (itemptr = hsearch(item,FIND)) {
				((struct uidinfo *)(itemptr->data))->u_want = 1;
				continue;
			}
			pass = getpwuid(uid);
			if (pass == NULL)
				fprintf(stderr, "ps: unknown user %s\n", user);
			else
				add_uid_entry(uid,pass->pw_name,1);
			continue;
		} else { /* a username */
			pass = getpwnam(user);
			if (pass == NULL)
				fprintf(stderr, "ps: unknown user %s\n", user);
			else {
				uid = pass->pw_uid;
				sprintf(buf,"U%d",uid);
				item.key = buf;
				if (itemptr = hsearch(item,FIND)) {
					((struct uidinfo *)(itemptr->data))->u_want = 1;
					continue;
				}
				add_uid_entry(uid,user,1);
			}
		}
	} while(*uidlist);
}


add_pid()
{
	char *pidset = optarg;
	do {
		if (npid >= MAXENT)
			return;
		pidlist[npid++] = atoi(getarg(&pidset));
	} while (*pidset);
}


add_group()
{
	char *groupset = optarg;
	do {
		if (ngrp >= MAXENT)
			return;
		grplist[ngrp++] = atoi(getarg(&groupset));
	} while (*groupset);
}

#ifdef FSS
add_fsid()
{
	char *fsidset = optarg;
	char *cp;	
	struct fsg *fsgp;

	do {
		if (nfsid >= MAXENT)
			return;
		cp = getarg(&fsidset);
		if (cp[0] >= '0' && cp[0] <= '9')
			fsidlist[nfsid++] = atoi(cp);
		else {
			fsgp = getfsgnam(cp);
			if (fsgp != NULL)
				fsidlist[nfsid++] = fsgp->fs_id;
		}
	} while (*fsidset);
}
#endif /* FSS */


main(argc, argv)
int argc;
char **argv;
{
	int i, count, idx = 0;
	char c;
	struct pst_status ps[PSBURST];
	struct stat stbuf;
	ENTRY item, *itemptr;
	char *progname = argv[0];
	char *tname;
	int no_output;

#ifdef FSS
	static char cmdargs[] = "edaflt:p:u:g:FG:";
#else
	static char cmdargs[] = "edaflt:p:u:g:";
#endif /* FSS */

#ifdef FSS
	fssincluded = (strcmp(getenv("FSS_PSBEHAVIOR"), "standard") != 0 &&
				fss(FS_STATE) != -1 && errno != EOPNOTSUPP);
#endif /* FSS */

#ifdef SecureWare
	if(ISSECURE)
		set_auth_parameters(argc, argv);
#ifdef B1
	if(ISB1)
		initprivs();
#endif
#endif

	/* get console device */
	getcons();

	/* get device info */
	getdata();

	/* Process args */
	w_noterm = 0;
	while ((c = getopt(argc, argv, cmdargs)) != EOF) {
		switch (c) {
		case 'e': eflg++; break;
		case 'd': dflg++; break;
		case 'a': aflg++; break;
		case 'f': fflg++; break;
		case 'l': lflg++; break;
		case 't': tflg++; add_term(); break;
		case 'p': pflg++; add_pid(); break;
		case 'u': uflg++; add_uid(); break;
		case 'g': gflg++; add_group(); break;

#ifdef FSS
		case 'F':
			if (fssincluded)
				Fflg++;
			break;
		case 'G':
			if (fssincluded) {
				Gflg++;
				add_fsid();
			}
			break;
#endif /* FSS */

		case '?':
		default:
			errflg++;
			break;
		}
	}
	if (errflg) {
#ifdef FSS
		if (fssincluded)
			fprintf(stderr,"usage: ps [-edaflF] [-u ulist] [-g glist] [-p plist] [-t tlist] [-G fsglist]\n");
		else
			fprintf(stderr,"usage: ps [-edafl] [-u ulist] [-g glist] [-p plist] [-t tlist]\n");
#else
		fprintf(stderr,"usage: ps [-edafl] [-u ulist] [-g glist] [-p plist] [-t tlist]\n");
#endif /* FSS */
		exit(1);
	}
	/* if specifying options not used, current terminal is default */
#ifdef FSS
	if ( !(aflg || eflg || dflg || uflg || tflg || pflg || gflg || Gflg )) {
#else
	if ( !(aflg || eflg || dflg || uflg || tflg || pflg || gflg )) {
#endif /* FSS */
		for (i = 2; i >= 0; i--)
			if (tname = ttyname(i))
				break;
		if (tname == NULL) {
			fprintf(stderr,"ps: don't know which terminal to select\n");
			exit(1);
		}
		if (stat(tname,&stbuf) < 0) {
			fprintf(stderr,"ps: ttyname() returns bad device\n");
			exit(1);
		}
		if ((stbuf.st_mode&S_IFMT) != S_IFCHR) {
			fprintf(stderr,"ps: ttyname() returns bad device\n");
			exit(1);
		}
		sprintf(buf,"D%d,%d",major(stbuf.st_rdev),minor(stbuf.st_rdev));
		item.key = buf;
		if ((itemptr = hsearch(item,FIND)) == NULL) {
			fprintf(stderr, "ps: could not find controlling terminal\n");
			exit(1);
		}
		((struct devinfo *)(itemptr->data))->d_want = 1;
		tflg++;
	}

#if defined(SecureWare)
	if(ISSECURE)
		ps_adjust_security();
#endif

	/* Print opening banner */
	banner();

	no_output = 1;
	/* Read in proc entries, print them */
#if defined(TRUX) && defined(B1)
	if (ISB1)
		(void) forcepriv(SEC_OWNER);
#endif
	while ((count = pstat(PSTAT_PROC,ps,sizeof(struct pst_status),
		PSBURST,idx)) > 0) {
		for (i = 0; i < count; ++i) {
			if (want(ps+i))   {
#if defined(SecureWare) && defined(B1)
				if(ISB1)
					ps_set_slot((ps+i)->pst_pid);
#endif
				if (display(ps+i))
					no_output = 0;
			}
		}
		idx = ps[count-1].pst_idx+1;
	}
#if defined(TRUX) && defined(B1)
	if(ISB1)
		(void) disablepriv(SEC_OWNER);
#endif
	return(no_output);
}

