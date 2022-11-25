/* @(#) $Revision: 66.5 $ */
/*	lpmove dest1 dest2 - move all requests from dest1 to dest2
 *	lpmove request ... dest - move requests to destination dest
 *
 *	This command may be used only by an LP Administrator
 */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 8					/* set number */
#include <msgbuf.h>
#endif NLS

#include	"lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

int interrupt = FALSE;
char errmsg[200];
char usage[] = "usage:\t%s dest1 dest2\n\t\t-or-\n\t%s request ... dest\n";	/* nl_msg 1 */
char corrupt[] = "LP system corrupted";						/* nl_msg 2 */
int moved = 0;		/* number of requests moved */
char *strncpy();

main(argc, argv)
int argc;
char *argv[];
{
	int i, seqno;
	char d1[DESTMAX + 1], d2[DESTMAX + 1], *strcpy();

#ifdef SecureWare
	if(ISSECURE){
            set_auth_parameters(argc, argv);
#ifdef B1
	    if(ISB1){
		initprivs();
        	(void) forcepriv(SEC_ALLOWMACACCESS);
	    }
#endif
	}
#endif
#ifdef NLS
	nl_catopen("lp");
#endif NLS

	startup(argv[0]);

	if(argc < 3) {
		printf((nl_msg(1, usage)), argv[0], argv[0]);
		exit(0);
	}
	if(! ISADMIN)
		fatal(ADMINMSG, 1);
	if(enqueue(F_NOOP, "") == 0)
		fatal((nl_msg(3, "scheduler is still running - can't proceed")), 1);

	if(isdest(argv[1])) {
		if(argc != 3) {
			printf((nl_msg(1, usage)), argv[0], argv[0]);
			exit(0);
		}
		if(! isdest(argv[2])) {
			sprintf(errmsg, (nl_msg(4, "destination \"%s\" non-existent")),
			    argv[2]);
			fatal(errmsg, 1);
		}

		strncpy(d1, argv[1], DESTMAX);
		d1[DESTMAX] = '\0';
		strncpy(d2, argv[2], DESTMAX);
		d2[DESTMAX] = '\0';

		if(strcmp(d1, d2) == 0)
			fatal((nl_msg(5, "destinations are identical")), 1);

		moveall(d1, d2);
#ifndef NLS
		printf("total of %d requests moved from %s to %s\n",
#else NLS
		printmsg((nl_msg(6, "total of %1$d requests moved from %2$s to %3$s\n")),
#endif NLS
		    moved, d1, d2);
	}
	else if(isdest(argv[argc - 1])) {
		strncpy(d2, argv[argc - 1], DESTMAX);
		d2[DESTMAX] = '\0';

		for(i = 1; i < argc - 1; i++) {
			if(! isrequest(argv[i], d1, &seqno)) {
				sprintf(errmsg, (nl_msg(7, "\"%s\" is not a request id")), argv[i]);
				fatal(errmsg, 0);
			}
			else if(strcmp(d1, d2) != 0)
				movit(d1, seqno, d2);
		}
#ifndef NLS
		printf("total of %d requests moved to %s\n", moved, d2);
#else NLS
		printmsg((nl_msg(8, "total of %1$d requests moved to %2$s\n")), moved, d2);
#endif NLS
		endoent();
	}
	else
		printf((nl_msg(1, usage)), argv[0], argv[0]);
	exit(0);
/* NOTREACHED */
}

/* moveall(d1, d2) - move all requests from d1 to d2 */

moveall(d1, d2)
char *d1;
char *d2;
{
	struct outq o;
	DIR *dir1;
	struct dirent *d;
	char r1[FILEMAX], r2[FILEMAX], *file1, *file2, *strncpy();
	int i, saveint();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, saveint);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, saveint);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, saveint);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, saveint);

	reject(d1, d2);

	while(getodest(&o, d1) != EOF) {
		strcpy(o.o_dest, d2);
		putoent(&o);
		moved++;
	}

	i = sprintf(r1, "%s/%s/", REQUEST, d1);
	file1 = &r1[i];
	i = sprintf(r2, "%s/%s/", REQUEST, d2);
	file2 = &r2[i];
	if((dir1 = opendir(r1)) == (DIR *) NULL) {
		sprintf(errmsg, (nl_msg(9, "can't open request directory %s")), r1);
		fatal(errmsg, 0);
		fatal((nl_msg(2, corrupt)), 1);
	}
	while((d = readdir(dir1)) != (struct dirent *) NULL){
		if(d->d_ino != 0 && d->d_name[0] != '.') {
			strncpy(file1, d->d_name, MAXNAMLEN + 1);
			*(file1 + DIRSIZ) = '\0';
			strncpy(file2, d->d_name, MAXNAMLEN + 1);
			*(file2 + DIRSIZ) = '\0';
			if(link(r1, r2) == -1 || unlink(r1) == -1) {
				sprintf(errmsg, (nl_msg(10, "can't move request %s")), r1);
				fatal(errmsg, 0);
				fatal((nl_msg(2, corrupt)), 1);
			}
		}
	}
	closedir(dir1);
	endoent();
}

/*
 *	movit(dest, seqno, newdest) - move request dest-seqno to newdest
 */

movit(dest, seqno, newdest)
char *dest;
int seqno;
char *newdest;
{
	struct outq o;
	int saveint(), catch(), i;
	char r1[FILEMAX], r2[FILEMAX], *file1, *file2;
	char rname[RNAMEMAX], arg[FILEMAX], type;
	FILE *rfile;

#if defined(SecureWare) && defined(B1)
        register int label_moved;
#endif
	
	if(getoid(&o, dest, seqno) == EOF) {
		sprintf(errmsg, (nl_msg(11, "request \"%s-%d\" non-existent")), dest, seqno);
		fatal(errmsg, 0);
		return;
	}

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, saveint);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, saveint);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, saveint);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, saveint);

	strcpy(o.o_dest, newdest);
	putoent(&o);

	i = sprintf(r1, "%s/%s/", REQUEST, dest);
	file1 = &r1[i];
	i = sprintf(r2, "%s/%s/", REQUEST, newdest);
	file2 = &r2[i];

#ifdef REMOTE
	if ((o.o_rflags & O_OB3) == O_OB3){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, dest, seqno, o.o_host);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, dest, seqno, o.o_host);
	}
#else
	sprintf(rname, "%s/%s/r-%d", REQUEST, dest, seqno);
#endif REMOTE
	if((rfile = fopen(rname, "r")) == NULL) {
		sprintf(errmsg, (nl_msg(12, "can't open request file %s")), rname);
		fatal(errmsg, 0);
		fatal((nl_msg(2, corrupt)), 1);
	}

	while(getrent(&type, arg, rfile) != EOF) {
		switch(type) {
		case R_FILE:
			if(*arg != '/') {
				strcpy(file1, arg);
				strcpy(file2, arg);
				if(link(r1, r2) == -1 || unlink(r1) == -1) {
					sprintf(errmsg, (nl_msg(13, "can't move request %s-%d")), dest, seqno);
					fatal(errmsg, 0);
					fatal((nl_msg(2, corrupt)), 1);
				}
			}
			break;
		default:
			break;
		}
	}
	fclose(rfile);
#ifdef REMOTE
	if ((o.o_rflags & O_OB3) == O_OB3){
		sprintf(file2, "cfA%03d%s", seqno, o.o_host);
	}else{
		sprintf(file2, "cA%04d%s", seqno, o.o_host);
	}
#else
	sprintf(file2, "r-%d", seqno);
#endif REMOTE
	if(link(rname, r2) == -1 || unlink(rname) == -1) {
		sprintf(errmsg, (nl_msg(14, "can't move request %s-%d")), dest, seqno);
		fatal(errmsg, 0);
		fatal((nl_msg(2, corrupt)), 1);
	}
	moved++;
	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);
	if(interrupt) {
		cleanup();
		exit(1);
	}
}

/*
 *	reject(d1, d2) - reject requests for destination d1 and
 *	supply message that requests were routed to destination d2
 */

reject(d1, d2)
char *d1;
char *d2;
{
#ifndef NLS
	sprintf(errmsg, "%s/%s -r'all requests moved to %s' %s >/dev/null 2>&1", ADMDIR, REJECT, d2, d1);
#else NLS
	sprintf(errmsg, "%s/%s -r\"%s%s\" %s >/dev/null 2>&1", ADMDIR, REJECT,
		(nl_msg(15, "all requests moved to ")), d2, d1);
#endif NLS
	system(errmsg);
	printf((nl_msg(16, "destination %s is not accepting requests\n")), d1);
	printf((nl_msg(17, "move in progress ...\n")));
	fflush(stdout);
}

startup(name)
char *name;
{
	int catch(), cleanup();
	extern char * f_name;
	extern int (*f_clean)();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

	f_name = name;
	f_clean = cleanup;
	if(chdir(SPOOL) == -1)
		fatal((nl_msg(18, "spool directory non-existent")), 1);
}

/* catch -- catch signals */

catch()
{
	int cleanup();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}

cleanup()
{
	endoent();
}

saveint()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	interrupt = TRUE;
}
