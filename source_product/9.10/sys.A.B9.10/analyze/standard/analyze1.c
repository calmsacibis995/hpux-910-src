/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/analyze/standard/RCS/analyze1.c,v $
 * $Revision: 1.38.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:30:07 $
 */


/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/



#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"
#include "h/utsname.h"
#ifdef NETWORK
extern int nflg;
#endif

#ifdef MP
#undef longjmp
#endif /* MP */

int 	dirtytable = 1;
int 	realstatus;


#define NUMP0VFDS	ctopt(0x10000 + 0x10000 + 0x10000)
#define NUMTEXTVFDS	ctopt(0x10000)


/* Saved options go here */
int	saved_vflg, saved_Eflg, saved_Uflg, saved_semaflg;
int	saved_Pflg, saved_Nflg;
extern	int	eof;
extern	jmp_buf jumpbuf;

scan(redir,path)
int redir;
char *path;
{
int i;
	extern unsigned int boottime;

	/**************************************************************/
	/*                                                            */
	/* Scan kernel data structures and do some consistency checks */
	/*                                                            */
	/**************************************************************/


	if (redir)
		if ((outf = fopen(path, ((redir == 2)?"a+":"w+"))) == NULL){
			fprintf(stderr,"Can't open file %s, errno = %d\n",
				path, errno);
			goto out;
		}

	/* clear logging tables, and mark as dirty */
	clearlogtables();

	/* allow sigints here */
	allow_sigint = 1;

	/*
	 * Print out utsname
	 */
	{
	    struct utsname uts;
	    long addr;

	    if ((addr = lookup("utsname")) == 0)
	    {
		fprintf(outf, "scan: can't find symbol \"utsname\"\n");
	    }
	    else if (getchunk(KERNELSPACE, addr, &uts, sizeof uts, "scan") == 0)
	    {
		fprintf(outf, "\n%s %s %s %s %s %s\n",
		    uts.sysname, uts.nodename, uts.release,
		    uts.version, uts.machine, uts.__idnumber);
	    }
	}

	/*
	 * Print out the stored message buffer
	 */
	dumpmsgbuf();

	if (Aflg){
		dumpaddress();
#ifdef iostuff
		dumpio(outf);
#endif

/*======================================================================*/
#ifdef OLD_REGISTER_DUMP
#ifdef hp9000s800
		/* Dump monarch rpb */
		getrpb(nl[X_RPB].n_value);
		dumprpb();
#endif hp9000s800
#ifdef MP
		/* Dump slave rpbs */
		for (i = 1; i < MAX_PROCS; i++){
			if (mp[i].prochpa != 0) {
				getrpb(mpiva[i].rpb_offset);
				dumprpb();
			}
		}
#endif MP

#ifdef hp9000s800
		gethpmc(nl[X_RPB].n_value);
		dumphpmc();
#ifdef MP
		/* Dump slave hpmcs */
		for (i = 1; i < MAX_PROCS; i++){
			if (mp[i].prochpa != 0) {
				gethpmc(mpiva[i].rpb_offset);
				dumphpmc();
			}
		}

		dumpmp();
#endif MP

#endif hp9000s800
#endif OLD_REGISTER_DUMP
/*======================================================================*/
#ifdef hp9000s800

		boottime = Reference (nl[X_BOOTTIME].n_value);
		/* Dump out crash processor information. */
		dump_crash_processor_table ();

		/* Dump HPMC/TOC/Panic event table */
		dump_event_table ();

		/* Dump out all system RPBs. */
		dump_system_rpbs ();


#ifdef MP
		dumpmp();
#endif MP

#endif hp9000s800
/*======================================================================*/

		if (my_site_status & CCT_CLUSTERED)
			selftest_status();

	}

	dumpvmaddr();

	dumpvmstats();

#ifdef iostuff
	/* -o flag only scan io subsystem */
	if (oflg) {
		dirtytable = 1;
		allow_sigint = 1;
		iocheck(Oflg,outf);
		allow_sigint = 0;
		goto reset;
	}
	if (zflg) {
		allow_sigint = 1;
		an_mgr_basic_analysis(outf);
		allow_sigint = 0;
		goto reset;
	}
	if (Zflg) {
		allow_sigint = 1;
		an_mgr_detail_analysis(outf);
		allow_sigint = 0;
		goto reset;
	}
#endif

#ifdef NETWORK
	/* -n flag only scans networking, nothing else. */
	if (nflg) {
	    netscan();
	    goto reset;
	}
#endif NETWORK

	dumpuptr();


	/* Because sproc_text mucks with pointers we must be
	 * more careful breaking in on it. It must break out
	 * at select times. We will also mark our log tables
	 * as dirty, as sproc_table will write into them.
	 */
	allow_sigint = 0;

	dirtytable = 1;

	sproc_text();

	dumpcallout();

	if (got_sigint)
		goto reset;

	/* allow sigints again */
	allow_sigint = 1;

	shmem_table();

	runqueues();

#ifdef hp9000s800
	uidhashchains();
#endif
	dmcheck();

	sysmapcheck();
	scan_hashchains();

	vfs_list();

	file_table();

	dnlc_check();

	bufcheck();

	icheck();

#ifdef QFS
	qfscheck();
#endif /* QFS */

	rcheck();
	if (Mflg)
		dev_table();

	dqcheck ();

	if (DUXFLAG && my_site_status & CCT_CLUSTERED) {
		cluster_table();

		nsp_table();
		scan_limited_queue();
		scan_general_queue();

		dm_info();

		/* DUX protocol */
		/* dux_proto_info(); */
		scan_using_array();

		scan_serving_array();

		scan_dux_mbufs();
#ifdef hp9000s200
		/*
		 * this is unimplemented for now--S800 does not use netbuf pool
		 */
#ifdef NEED_TO_IMPLEMENT
		scan_netbufs();
#endif NEED_TO_IMPLEMENT
#endif
	}

	/* There will be too many inconsistencies to check these
	 * on an active system.
	 */
#ifdef hp9000s800
	if (activeflg == 0){
		pdircheck();
	}
#endif
	freelist();

	if (activeflg == 0) {
		isrfreepool();
		coresummary();
	}
#ifdef MP
	dumpsemaphores();
#endif


reset:
	if (redir){
		fclose(outf);
	}

out:
	outf = stdout;


}

/* zero out options */
clearoptions()
{
	Cflg = 0; vflg = 0; sflg = 0; Fflg = 0; Dflg = 0; dflg = 0; Iflg = 0;
	iflg = 0; Uflg = 0; Mflg = 0; Qflg = 0; Bflg = 0; bflg = 0;
	Aflg = 0, Vflg = 0, Pflg = 0, Eflg = 0, Xflg = 0;
	Sflg = 0; Rflg = 0; Hflg = 0;
	Jflg = 0; qflg = 0;
#ifdef iostuff
	oflg = 0; Oflg = 0; Zflg = 0; zflg = 0;
#endif
	lflg = 0;
	Tflg = 1;
	Nflg = 0;
	DUXFLAG = 0;
#ifdef NETWORK
	nflg = 0;
#endif NETWORK
	semaflg = 0;
	suppress_default = 0;
}


/* Save verbose options */
saveoptions()
{
	saved_vflg = vflg;
	saved_Eflg = Eflg;
	saved_Pflg = Pflg;
	saved_Uflg = Uflg;
	saved_semaflg = semaflg;
	saved_Nflg = Nflg;
}

restoreoptions()
{
	vflg = saved_vflg;
	Eflg = saved_Eflg;
	Pflg = saved_Pflg;
	Uflg = saved_Uflg;
	semaflg = saved_semaflg;
	Nflg = saved_Nflg;
}

clearlogtables()
{
	register int i;
	register int j;
	register struct dblks *dblog;

	if (!dirtytable)
		return;

#ifdef DEBUG
	fprintf(outf," Clearing log tables..\n");
#endif

#ifdef iostuff
	cleariotables();
#endif

#ifdef hp9000s800
	bzero(paginfo, npdir * sizeof(struct paginfo));
#else  hp9000s300
	bzero(paginfo, (maxfree - firstfree +1) * sizeof(struct paginfo));
#endif

	bzero(bufblk, BUFBLKSZ * sizeof(struct bufblk));

	bzero(swbufblk, nswbuf * sizeof(struct swbufblk));

	bzero(inodeblk, ninode * sizeof(struct inodeblk));

	bzero(dblks, 4 * nswapmap * sizeof(struct dblks));


	/*
	for (i = 0; i < MAXSWAPCHUNKS; i++){
		if (swaptab[i].st_devvp != 0){
			if (dusagetable[i] != 0) {
				dblog = dusagetable[i];
				for (j = 0; j < swaptab[i].st_npgs; j++) {
					dblog[j].d_type = RLOST;
					dblog[j].d_count = 0;
				}

			}

		}
	}
	*/



	dirtytable = 0;
}

/* quick little clear routine */
bzero(addr,count)
register char *addr;
register int count;
{
	register int i;
	for(i = 0; i < count; i++,addr++){
		*addr = 0;
	}
}

/* reset options */
processoptions(cp)
char *cp;
{
		while (*cp) switch (*cp++) {

		case '-':
			break;

		case 'C':
			Cflg++;
			break;

		case 'V':
			Vflg++;
			break;

		case 'P':
			Pflg++;
			break;

		case 'E':
			Eflg++;
			break;

		case 'v':
			vflg++;
			break;

		 /* we don't support this, mainly because we overwrite
		  * it with the core image
		  */

		/*
		case 's':
			sflg++;
			break;
		*/

		case 'F':
			Fflg++;
			break;

		case 'D':
			dflg++;
			Dflg++;
			break;

		case 'd':
			dflg++;
			break;

		case 'I':
			iflg++;
			Iflg++;
			break;

		case 'i':
			iflg++;
			break;

		case 'U':
			Uflg++;
			break;

		case 'M':
			Mflg++;
			break;

		case 'Q':
			Qflg++;
			break;

		case 'J':
			Jflg++;
			break;

		case 'X':
			Xflg++;
			break;

		case 'T':
			Tflg++;
			break;

		case 'B':
			Bflg++;
			bflg++;
			break;

		case 'b':
			bflg++;
			break;

		case 'a':
			suppress_default++;
			break;

		case 'A':
			Aflg++;
			break;

		case 'S':
			Sflg++;
			break;

#ifdef iostuff
		case 'o':
			oflg++;
			break;

		case 'O':
			Oflg++;
			oflg++;
			break;

		case 'z':
			zflg++;
			break;

		case 'Z':
			Zflg++;
			break;
#endif

		case 'R':
			Rflg++;
			break;

		case 'H':
			Hflg++;
			break;

		case DUXCHAR:
			DUXFLAG++;
			break;
#ifdef NETWORK
		case 'n':
			nflg++;
			break;

		case 'N':
			Nflg++;
			break;
#endif NETWORK
		case 'l':
			lflg++;
			break;

		case 'q':
			qflg++;
			break;

		default:
			printusage();
			return(1);
		}
		return(0);
}

printusage()
{

		fprintf(stderr, "usage: analyze [ -dDbBCMFiIUSRoOzZn%c ]  corefile [ system ]\n", DUXCHAR);
		fprintf(stderr," C coremap, F freelist, d swap map checks, D discmaps,\n");
		fprintf(stderr," E vfd & dbd  consistency check\n");
		fprintf(stderr," P pregions and regions\n");
		fprintf(stderr," b buffer checks, B buffers , U uarea\n");
		fprintf(stderr," i inode checks, I inodes, M mount, file, and device tables, vfs list\n A interesting misc addresses\n");
		fprintf(stderr," S shared memory tables, R make analyze realtime\n");
		fprintf(stderr," o io message checks, O io messages, n networking\n");
		fprintf(stderr," z io manager basics, Z io manager details\n");
		fprintf(stderr," %c discLess information\n", DUXCHAR);
}

togglerealtime()
{
	if (!realstatus){
		/* make realtime */
		realstatus = 1;
		oldpri = rtprio(0,127);
		if (oldpri != -1)
			fprintf(outf, "realtime pri = 127\n");
	} else {
		/* turn off realtime */
		realstatus = 0;
		fprintf(outf," realtime disabled\n");
		if (oldpri != -1){
				oldpri = rtprio(0,oldpri);
				fprintf(outf," realtime disabled\n");
		}
	}
}


#ifdef INTERACTIVE
yywrap()
{

	eof = 1;
	return(1);

}
#endif

/* say goodbye */
quit()
{

	/* reset priority back to normal mortal we've got all we need */
	/* Actually we could just exit, but this is ok too. */
	if (Rflg){
		if (oldpri != -1)
			oldpri = rtprio(0,oldpri);
	}
	fprintf(outf,"\n Goodbye, have a nice day!!\n");
	exit(0);
}


sigint_handler(sig, code, scp)
int sig, code;
struct sigcontext *scp;

{
	if (!allow_sigint){
		got_sigint = 1;
#ifdef DEBUG
		fprintf(stderr,"\n received an unwanted sigint, continuing..\n");
#endif
		return;
	}

#ifdef DEBUG
	fprintf(stderr,"\n received sigint, breaking out of scan..\n");
#endif
	allow_sigint = 0;
	got_sigint = 0;
	longjmp(jumpbuf);
}

sigquit_handler(sig, code, scp)
int sig, code;
struct sigcontext *scp;

{
	fprintf(stderr,"\n received sigquit, goodbye..\n");
	exit(1);
}
