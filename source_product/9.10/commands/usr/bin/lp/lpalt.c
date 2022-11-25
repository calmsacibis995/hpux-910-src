/* $Revision: 70.2 $ */
/* lpalt -- alteration of request for line printer */

#ifndef NLS
#define nl_msg(i,s) (s)
#else NLS
#define NL_SETN 21
#include <msgbuf.h>
#endif NLS

#include "lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

char errmsg[FILEMAX];
char work[BUFSIZ];

char dst[DESTMAX+1] = NULL;		/* target destination */
int  seqno;

char newdst[DESTMAX+1] = NULL;		/* alter destination */
char t_file[RNAMEMAX] = NULL;
char d_file[RNAMEMAX] = NULL;
char c_file[RNAMEMAX] = NULL;

char title[TITLEMAX+1] = NULL;		/* alter title of request */
char opts[OPTMAX] = NULL;		/* options for interface program */
int optlen = 0;				/* size of option string for */
					/* interface program */
int copies = -1;			/* alter number of copies */
short priority = -1;			/* alter priority of request */
short mail = FALSE;			/* mail back when request is done */
short wrt = FALSE;			/* write back when request is done */

short   silent = FALSE;			/* silent */

short others = FALSE;			/* flag for request of alteration */
					/* parameters except for destination */
					/* and priority */
char user_name[LOGMAX+1];

struct qstat q;

#ifdef REMOTE
int	iflag=FALSE;			/* inhibit remote alter */
char	*RM;
char	*RP;
char	*printer;
char	*from;
char	host[SP_MAXHOSTNAMELEN];
#endif REMOTE

extern char **environ;			/* for cleanenv() */

main(argc,argv)
int	argc;
char	*argv[];
{
	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH",0);

	startup(argv[0]);
	
	options(argc,argv);		/* process command line options */

	if(chdir(SPOOL) == -1)
	    fatal((nl_msg(1, "spool directory non-existent")),1);

#ifdef REMOTE
	gethostname(host, sizeof(host));
#endif REMOTE

	if(getqdest(&q, dst) == EOF){		/* get acceptance status */
		sprintf(errmsg,(nl_msg(2, "acceptance status of destination \"%s\" unknown")), dst);
		fatal(errmsg, 1);
	}
	endqent();

	if(others){
		altother(iflag);	/* alter the attributes of request */
				   	/* but for destination and priority */
		set_user_id(1);		/* reset user id to LP administrator */
				   	/* Because when remotealt() is called */
				   	/* euid is set to 0 */
	}

	if(newdst[0] != NULL)
		altdest();		/* alter the destination of request */
	else if(priority != -1)
		altpri();		/* alter the priority of request */

	exit(0);
/* NOTREACHED */
}


/* altdest -- alteration of destination for the request */

altdest()
{
	int newseqno;
	struct qstat q;
	struct outq o;
	char	*strncpy();
        int nm_taken = 0;
        short UNIQNAME = FALSE;
 

	if(! ISADMIN)
	    chkuser();			/* check the owner of the request */

	if(getqdest(&q, newdst) == EOF){	/* get acceptance status */
		sprintf(work, (nl_msg(2, "acceptance status of destination \"%s\" unknown")), newdst);
		fatal(work, 1);
	}
	endqent();

	if(! q.q_accept) {			/* accepting requests ? */
		sprintf(work, (nl_msg(3, "can't accept requests for destination \"%s\" -\n\t%s")),newdst, q.q_reason);
		fatal(work, 1);
	}

	if(getoid(&o, dst, seqno) == EOF){	/* get output structure */
		sprintf(errmsg, (nl_msg(8, "no such request \"%s-%d\"")), dst, seqno);
	    	fatal(errmsg, 1);
	}
	endoent();

	if(enqueue(F_NOOP, "") != 0){		/* scheduler is running ? */
		sprintf(work, (nl_msg(4, "scheduler is not running. alteration failed.")));
		fatal(work, 1);
	}
	
        while((UNIQNAME == FALSE) && (nm_taken < SEQMAX)) {
           getseq(&newseqno);                   /* get new sequence # */
           sprintf(t_file, "%s/%s/tA%04d%s", REQUEST, newdst, newseqno, o.o_host);
           sprintf(d_file,"%s/%s/dA%04d%s", REQUEST, newdst, newseqno, o.o_host);
           sprintf(c_file,"%s/%s/cA%04d%s", REQUEST, newdst, newseqno, o.o_host);
           if(GET_ACCESS(t_file, 0) == -1 && GET_ACCESS(d_file, 0) == -1 && GET_ACCESS(c_file, 0))
              UNIQNAME=TRUE;  /*file has unique name */
           nm_taken++;
        }

	/* building command line which is written to the FIFO */

	if( priority != -1){
#ifdef REMOTE
		sprintf(work, "%s %d %s %d %s %d",
			dst, seqno, newdst, newseqno, o.o_host, priority);
#else REMOTE
		sprintf(work, "%s %d %s %d %d",
			dst, seqno, newdst, newseqno, priority);
#endif REMOTE
	}else{
#ifdef REMOTE
		sprintf(work, "%s %d %s %d %s",
			dst, seqno, newdst, newseqno, o.o_host);
#else REMOTE
		sprintf(work, "%s %d %s %d",
			dst, seqno, newdst, newseqno);
#endif REMOTE
	}

	if(enqueue(F_DEST, work) != 0){		/* enqueue to the FIFO */
		if(!silent)
		    printf((nl_msg(5, "new request id %s-%d failed to enqueue\n")), newdst, newseqno);
	}else{
		if(!silent)
		    printf((nl_msg(6, "new request id is %s-%d\n")), newdst, newseqno);
	}
}


/* altpri -- alteration of priority for the request */

altpri()
{
	struct outq o;
	char	*strncpy();

	if(! ISADMIN)
	    chkuser();			/* check the owner of the request */

	if(getoid(&o, dst, seqno) == EOF){	/* get output structure */	
		sprintf(errmsg, (nl_msg(8, "no such request \"%s-%d\"")), dst, seqno);
		fatal(errmsg, 1);
	}
	endoent();

	if(enqueue(F_NOOP, "") != 0){		/* scheduler is running ? */
		sprintf(work, (nl_msg(4, "scheduler is not running. alteration failed.")));
		fatal(work, 1);
	}

	/* building command line which is written to the FIFO */
#ifdef REMOTE
	sprintf(work, "%s %d %d %s", dst, seqno, priority, o.o_host);
#else REMOTE
	sprintf(work, "%s %d %d", dst, seqno, priority);
#endif REMOTE

	if(enqueue(F_PRIORITY, work) != 0){	/* enqueue to the FIFO */
		if(!silent)
		    printf((nl_msg(7, "failed to enqueue\n")));
	}
}


/* chkuser -- check if user is identified for the owner of the request */

chkuser()
{
	struct outq o;

	if(getoid(&o, dst, seqno) == EOF){	/* get output structure */
		sprintf(errmsg, (nl_msg(8, "no such request \"%s-%d\"")), dst, seqno);
		fatal(errmsg, 1);
	}
	endoent();

	if(strcmp(getname(), o.o_logname) != 0){
		sprintf(errmsg,(nl_msg(9, "request \"%s-%d\" not altered: not owner")), o.o_dest, o.o_seqno);
		fatal(errmsg, 1);
	}
}


/* getseq(snum) -- get next sequence number */

getseq(snum)
     int *snum;
{
	FILE *fp;
	char seqfilename[sizeof(REQUEST) + DESTMAX+1 +5];
	extern FILE *lockf_open();
	
#ifdef REMOTE
	if (q.q_ob3){
		sprintf(seqfilename,"%s/%s/.seq", REQUEST, dst);
	}else{
		sprintf(seqfilename,"%s", SEQFILE );
	}
#else
	sprintf(seqfilename,"%s", SEQFILE );
#endif REMOTE
	
	if ((fp = lockf_open(seqfilename, "r+", TRUE)) == NULL) {
		fatal(nl_msg(10, "can't open new sequence number file \"%s\""), seqfilename);
	}
	/* read sequence number file */
	if (fscanf(fp, "%d\n", snum) < 1){
#ifdef REMOTE
		*snum = -1;
#else
		*snum = 0;
#endif REMOTE
	}
	
#ifdef SecureWare
	if(ISSECURE)
		lp_change_mode(SPOOL, seqfilename, 0644, ADMIN,
			"new sequence file");
	else
		chmod(seqfilename, 0644);
#else
	chmod(seqfilename, 0644);
#endif
	++(*snum);
#ifdef REMOTE
	if (q.q_ob3){
		if((*snum) == SEQMAXREMOTE)
		    *snum = 0;
	}else{
		if((*snum) == SEQMAX)
		    *snum = 0;
	}
#else
	if((*snum) == SEQMAX)
	    *snum = 1;
#endif REMOTE
	
	ftruncate(fileno(fp),0);
	rewind(fp);
	fprintf(fp, "%d\n", *snum);
	fclose(fp);
}


/* options -- process command line options */

options(argc,argv)
int	argc;
char	*argv[];
{
	short	reqargs = 0;
	int	j;
	char letter;
	char *value;
	char *strcat(), *strncpy();

	for(j=1; j<argc; j++){
		if(argv[j][0] == '-' && (letter = argv[j][1]) != '\0'){
			if(! isalpha(letter)){
				sprintf(errmsg, (nl_msg(11, "illegal keyletter \"%c\"")),letter);
				fatal(errmsg, 1);
			}
			letter = tolower(letter);

			value  = &argv[j][2];

			switch(letter){

			    case 'd':		/* destination */
				if(*value != '\0'){
					if( !isdest(value) ){
						sprintf(errmsg, (nl_msg(12, "destination \"%s\" non-existent")), value);
						fatal(errmsg, 1);
					}
					strncpy(newdst, value, DESTMAX);
					newdst[DESTMAX] = NULL;
				}else{
					sprintf(errmsg, (nl_msg(13, "keyletter not expecting argument \"-%c%s\"")), letter, value);
					fatal(errmsg, 1);
				}
					if(! strncmp(dst,newdst,DESTMAX) ){
						sprintf(errmsg, (nl_msg(18, "new destination must be different than the old destination")));
						fatal(errmsg, 1);
					}
				break;

			    case 'm':		/* mail */
				if(*value == '\0'){
					mail = TRUE;
					others = TRUE;
				}else{
					sprintf(errmsg, (nl_msg(13, "keyletter not expecting argument \"-%c%s\"")), letter, value);
					fatal(errmsg, 1);
				}
				break;

			    case 'n':		/* copy number */
				if(*value == '\0' || (copies = atoi(value)) <= 0){
					sprintf(errmsg, (nl_msg(13, "keyletter not expecting argument \"-%c%s\"")), letter, value);
					fatal(errmsg, 1);
				}else{
					others = TRUE;
				}
				break;

			    case 'o':		/* options for interface */
				if(*value != '\0'){
					if(optlen == 0)
					    optlen = strlen(value);
					else
					    optlen += (strlen(value) + 1);
					if(optlen >= OPTMAX)
					    fatal((nl_msg(14, "too many options for interface program")), 1);
					if(*opts != '\0')
					    strcat(opts, " ");
					strcat(opts, value);
					others = TRUE;
				}
				break;

			    case 'p':		/* priority */
				if(*value == '\0' 
				   || (priority = (short)atoi(value)) < MINPRI
				   || priority > MAXPRI){
				   	sprintf(errmsg, (nl_msg(15, "bad priority \"-%c%s\"")), letter, value);
				   	fatal(errmsg, 1);
				}
				break;

			    case 's':		/* silent */
				if(*value == '\0'){
					silent = TRUE;
				}else{
					sprintf(errmsg, (nl_msg(13, "keyletter not expecting argument \"-%c%s\"")), letter, value);
					fatal(errmsg, 1);
				}
				break;

			    case 't':		/* title */
				strncpy(title, value, TITLEMAX);
				title[TITLEMAX] == '\0';
				others = TRUE;
				break;

			    case 'w':		/* write to user's tty */
				if(*value == '\0'){
					wrt = TRUE;
					others = TRUE;
				}else{
					sprintf(errmsg, (nl_msg(13, "keyletter not expecting argument \"-%c%s\"")), letter, value);
					fatal(errmsg, 1);
				}
				break;

			    case 'i':		/* inhibit remote alter */
				if(*value == '\0'){
					iflag=TRUE;
				}else{
					sprintf(errmsg, (nl_msg(13, "keyletter not expecting argument \"-%c%s\"")), letter, value);
					fatal(errmsg, 1);
				}
				break;

			    default:
				sprintf(errmsg, (nl_msg(16, "unknown keyletter \"-%c\"")),letter);
				fatal(errmsg,1);
				break;
			}
			
			argv[j] = NULL;
		}else{
			if (reqargs || !isrequest(argv[j], dst, &seqno))
				usage();

			reqargs++;
		}
	}
	if(dst[0] == NULL)
	    usage();
}


/* startup -- initialization routine */

startup(name)
char	*name;
{
	int catch(), cleanup();
	extern char *f_name;
	extern int (*f_clean)();

	f_name = name;
	f_clean = cleanup;

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

	set_user_id(1);	/* set user id to LP administrator */

	/* save the real-user-name */
	strncpy(user_name, getname(), LOGMAX);
	user_name[LOGMAX] = '\0';
}


/* usage -- Bad usage error */

usage()
{
	fprintf(stderr,(nl_msg(17, "usage : lpalt id [-ddest] [-m] [-nnumber] [-ooption]\n\t[-ppriority] [-s] [-ttitle] [-w]\n")) );

	exit(1);
}


/* catch -- catch signals */

catch()
{
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	cleanup();
	exit(1);
}


/* cleanup -- called by catch() after interrupts or by fatal() after errors */

cleanup()
{
	endoent();
	endpent();
	endqent();
}
