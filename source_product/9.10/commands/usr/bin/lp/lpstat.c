/* @(#) $Revisionac.cv.hp.com 444: 66.8 $ */
/* lpstat -- display line printer status */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 11	/* message set number */
#define MAXLNAME 14	/* maximum length of a language name */
#include <nl_types.h>
#include <locale.h>
nl_catd	nlmsg_fd;
nl_catd	nlmsg_tfd;
#endif NLS

#include	"lp.h"
#include	"lpsched.h"
#ifdef TRUX
#include <sys/security.h>
#endif

char errmsg[200];
#ifdef REMOTE
int	iflag	= FALSE;	/* inhibit remote status */
int	users	= 0;		/* number of users specified */
char	*user_names[MAXUSERS];	/* pointers to user names */
char	syscommand[TMPLEN], option[TMPLEN * 4];
char	destprinter[TMPLEN];
char	*requ[MAXREQUESTS];	/* remote request id array */
int	nargs;			/* # of args in remote request id array */
#ifdef DELAYED
int	delayed_status_enabled;	/* 0 = do not display delayed status */
				/*     info. */
				/* 1 = display delayed status info. */
#endif DELAYED

#endif REMOTE

main(argc, argv)
int argc;
char *argv[];
{
	int i;

	int user(), output(), accept_stat(), printer(), device(), class();
	char *arg, letter;
	char buf[BUFSIZ];	/* buffer used for line buffering output */
#ifdef REMOTE
	char	*targ;
#endif REMOTE

#ifdef NLS || NLS16			/* initialize to the current locale */
	unsigned char lctime[5+4*MAXLNAME+4], *pc;
	unsigned char savelang[5+MAXLNAME+1];
#endif NLS || NLS16

	/* Set line buffering mode so file output matches terminal output */
	setvbuf(stdout,buf,_IOLBF,BUFSIZ);

#ifdef NLS || NLS16			/* initialize to the current locale */
	if (!setlocale(LC_ALL, "")) {
		fputs(_errlocale("lpstat"), stderr);
		putenv("LANG=");
		nlmsg_fd = (nl_catd)-1;
		nlmsg_tfd = (nl_catd)-1;
	} else {				/* setlocale succeeds */
		nlmsg_fd = catopen("lp", 0);	/* use $LANG messages */
		strcpy(lctime, "LANG=");	/* $LC_TIME affects some msgs */
		strcat(lctime, getenv("LC_TIME"));
		if (lctime[5] != '\0') {	/* if $LC_TIME is set */
			strcpy(savelang, "LANG=");	/* save $LANG */
			strcat(savelang, getenv("LANG"));
			if ((pc = strchr(lctime, '@')) != NULL) /*if modifier*/
				*pc = '\0';	/* remove modifer part */
			putenv(lctime);		/* use $LC_TIME for some msgs */
			nlmsg_tfd = catopen("lp", 0);
			putenv(savelang);	/* reset $LANG */
		} else				/* $LC_TIME is not set */
			nlmsg_tfd = nlmsg_fd;	/* use $LANG messages */
	}
#endif NLS || NLS16
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
	startup(argv[0]);

#ifdef REMOTE
        for(i = 1; i < argc; i++) {
                arg = argv[i];
                if(*arg == '-') {
                        letter = tolower(*(arg + 1));
                        if (letter == 'i'){     /* inhibit remote */
                                iflag = TRUE;   /* status */
                                break;
                        }
                }
        }
#endif REMOTE


	if((argc == 1) || ((argc == 2) && iflag)) {
#ifdef REMOTE
#ifdef SecureWare
		if(ISSECURE)
                    user_names[0] = lp_getlname();
		else{
		    user_names[0] = (char *) malloc(LOGMAX+1);
		    strncpy(user_names[0], getname(), LOGMAX);
		}
#else
		user_names[0] = (char *) malloc(LOGMAX+1);
		strncpy(user_names[0], getname(), LOGMAX);
#endif		
		user_names[1] = NULL;
		users = 1;
		douserlist();
#else
#ifdef SecureWare
		if(ISSECURE)
                    dolist(lp_getlname(), user);
		else
		    dolist(getname(), user);
#else 
		dolist(getname(), user);
#endif
#endif REMOTE
		exit(0);
	}


	for(i = 1; i < argc; i++) {
		arg = argv[i];
		if(*arg == '-') {
			letter = tolower(*(arg + 1));
			if(! islower(letter)) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,1, "unknown option \"%s\"")), arg);
				fatal(errmsg, 0);
				continue;
			}
			switch(letter) {
			case 'a':	/* acceptance status */
				dolist(arg + 2, accept_stat);
				break;
			case 'c':	/* class to printer mapping */
				dolist(arg + 2, class);
				break;
			case 'd':	/* default destination */
				def();
				break;
			case 'i':	/* ignore remote status */
				break;
			case 'o':	/* output for destinations */
				dolist(arg + 2, output);
				break;
			case 'p':	/* printer status */
				dolist(arg + 2, printer);
				break;
			case 'r':	/* is scheduler running? */
				running();
				break;
			case 's':	/* configuration summary */
				config();
				break;
			case 't':	/* print all info */
				all();
				break;
			case 'u':	/* output for user list */
#ifdef REMOTE
				doqueueuser(arg + 2);
				targ = argv[i + 1];
				if((*targ++ != '-') || (*targ != 'u')) {
					douserlist();
				}
#else
				dolist(arg + 2, user);
#endif REMOTE
				break;
			case 'v':	/* printers to devices mapping */
				dolist(arg + 2, device);
				break;
			default:
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,1, "unknown option \"%s\"")), arg);
				fatal(errmsg, 0);
			}
		}
		else
			dolist(arg, output);
	}
#ifdef REMOTE
	if (nargs != 0){
		remoteoutput();
	}
#endif REMOTE

	exit(0);
/* NOTREACHED */
}


static char *
basename(str)
char	*str;
{
	char	*ptr;
	extern char *strrchr();

	if ( ( ptr = strrchr(str, '/') ) == NULL )
	    return(str);
	else
	    return(ptr+1);
}


config()
{
	int class(), device();

	def();
	dolist((char *) NULL, class);
	dolist((char *) NULL, device);
}

all()
{
	int accept_stat(), printer(), output();

	running();
	config();
	dolist((char *) NULL, accept_stat);
	dolist((char *) NULL, printer);
	dolist((char *) NULL, output);
}

def()
{
	char d[DESTMAX + 1];
	FILE *fopen(), *f;

	if(GET_ACCESS(DEFAULT, ACC_R) == 0) {
		if((f = fopen(DEFAULT, "r")) != NULL) {
			if(fscanf(f, "%s\n", d) == 1)
				printf((catgets(nlmsg_fd,NL_SETN,3, "system default destination: %s\n")), d);
			else
				printf((catgets(nlmsg_fd,NL_SETN,4, "no system default destination\n")));
			fclose(f);
		}
	}
	else
		printf((catgets(nlmsg_fd,NL_SETN,4, "no system default destination\n")));
}

output(argc, argv)
int argc;
char *argv[];
{
	int	i, seqno;
	struct	outq o;
	extern	int errno;
	char	 *arg;
	char	 dest[DESTMAX + 1];
#ifdef REMOTE
	int	pid, status;
	struct	pstat p;
#endif REMOTE

	if(argc == 0){
		while(getoent(&o) != EOF){
			putoline(&o);
		}

		endoent();
#ifdef REMOTE
		if (iflag)
			return(0);
		while (getmpent(&p) != EOF){
			if ((p.p_rflags & (P_OCI|P_OCM)) != 0) {
				sprintf(syscommand , "%s/%s/%s",SPOOL,SINTERFACE,p.p_dest);
				sprintf(destprinter, "-d%s",p.p_dest);
				if( (pid = fork()) == 0) {
					(void) execlp(syscommand, p.p_dest, destprinter, 0);
					sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, "execlp failed")));
					fatal(errmsg, 1);
				}
				else if (pid == -1){
					printf((catgets(nlmsg_fd,NL_SETN,6, "Unable to execute the remote status command %s.\n")), syscommand);
					printf((catgets(nlmsg_fd,NL_SETN,7, "Error %d occured.\n")), errno);
				}else{
					while((wait(&status)) != pid)
						;
				}
			}
		}
		endmpent();
#endif REMOTE
	}else {
		for(i = 0; i < argc; i++) {
			arg = argv[i];
			if(isrequest(arg, dest, &seqno)) {
				if(getoid(&o, dest, seqno) != EOF){
					putoline(&o);
				}
#ifdef REMOTE
				else{
					if (getmpdest(&p, dest) != EOF){
						if ((p.p_rflags & (P_OCI|P_OCM)) != 0) {
							if (enter(arg, requ, &nargs, MAXREQUESTS) == -1){
								printf( (catgets(nlmsg_fd,NL_SETN,8, "unable to malloc memory to hold more requests.\n")));
							}
						}
					}
				}
#endif REMOTE
				*arg = '\0';
			}
			else if(!isdest(arg)) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,9, "\"%s\" not a request id or a destination")), arg);
				fatal(errmsg, 0);
				*arg = '\0';
			}
		}
		setoent();
		while(getoent(&o) != EOF)
			for(i = 0; i < argc; i++)
				if(strcmp(o.o_dest, argv[i]) == 0) {
					putoline(&o);
					break;
				}
		endoent();
#ifdef REMOTE
		if (iflag)
			return(0);
		for(i = 0; i < argc; i++){
			if(!isdest(argv[i])) {
				break;
			}else{
				if(getmpdest(&p,argv[i]) == EOF){
					break;
				}else{
					if ((p.p_rflags & (P_OCI|P_OCM)) == 0){
						break;
					}
				}
			}
			sprintf(syscommand , "%s/%s/%s",SPOOL,SINTERFACE,p.p_dest);
			sprintf(destprinter, "-d%s",p.p_dest);
			if( (pid = fork()) == 0) {
				(void) execlp(syscommand, p.p_dest, destprinter, 0);
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, "execlp failed")));
				fatal(errmsg, 1);
			}
			else if (pid == -1){
				printf((catgets(nlmsg_fd,NL_SETN,6, "Unable to execute the remote status command %s.\n")), syscommand);
				printf((catgets(nlmsg_fd,NL_SETN,7, "Error %d occured.\n")), errno);
			}else{
				while((wait(&status)) != pid)
					;
			}
		}
		endmpent();
#endif REMOTE
	}
	return(0);
}

user(argc, argv)
int argc;
char *argv[];
{
	int i;
	struct outq o;

	if(argc == 0)
		while(getoent(&o) != EOF)
			putoline(&o);
	else
		while(getoent(&o) != EOF)
			for(i = 0; i < argc; i++)
				if(strcmp(o.o_logname, argv[i]) == 0) {
					putoline(&o);
					break;
				}
	endoent();
}

putoline(o)
struct outq *o;
{
	char file[FILEMAX];
	char rname[FILEMAX];
	char type;
	char arg[FILEMAX];
	int  copies;
	FILE *cfp;
	extern FILE *lockf_open();
	int	j=0;

	char tmp1[TMPLEN];
	char tmp2[TMPLEN*2];
	char reqid[IDSIZE + 1], *dt;
#ifdef SecureWare
#ifdef REMOTE
        if ((ISSECURE) && (!lp_can_view(SPOOL, REQUEST, o->o_logname, o->o_dest,
                         o->o_seqno, o->o_host, o->o_rflags&O_OB3)))
#else
        if ((ISSECURE) && (!lp_can_view(SPOOL, REQUEST, o->o_logname, o->o_dest,
			o->o_seqno)))
#endif
                return;
#endif	
	sprintf(reqid, "%s-%d", o->o_dest, o->o_seqno);
	dt = date(o->o_date);
	if(o->o_flags & O_PRINT)
		sprintf(tmp1, (catgets(nlmsg_fd,NL_SETN,13, " on %s")), o->o_dev);
	else
		sprintf(tmp1, (catgets(nlmsg_fd,NL_SETN,14, ""))); 
#ifdef REMOTE
	if((o->o_rflags & O_REM) == 0){
		sprintf(tmp2, (catgets(nlmsg_fd,NL_SETN,14, "")));
	}else{
		sprintf(tmp2, (catgets(nlmsg_fd,NL_SETN,16, " from %s")), o->o_host);
	}
#endif REMOTE

#ifdef REMOTE
	if(( o->o_rflags & O_OB3 ) != 0){
		sprintf(rname, "%s/%s/cfA%03d%s", REQUEST, o->o_dest, o->o_seqno, o->o_host);
	}else{
		sprintf(rname, "%s/%s/cA%04d%s", REQUEST, o->o_dest, o->o_seqno, o->o_host);
	}
#endif REMOTE
#ifdef SecureWare
	if( ISSECURE )
	    lp_change_mode(SPOOL, rname, 0640, ADMIN, "Adding to request file");
	else
	    chmod(rname, 0640);
#else
	chmod(rname, 0640);
#endif

	if((cfp=lockf_open(rname, "r", FALSE)) == NULL){
		return(0);
	}

	printf((catgets(nlmsg_fd,NL_SETN,17, "%-*s %-*s priority %d  %s%s%s\n")), IDSIZE, reqid, LOGMAX-1, o->o_logname, o->o_priority, dt, tmp2, tmp1);

	while(getrent(&type, arg, cfp) != EOF){
		switch(type){
		    case 'K':
			copies = atoi(arg);
			break;
		    default:
			if(type < 'a' ||  type > 'z')
			    continue;
			if(j == 0 || strcmp(file, arg) != 0)
			    strcpy(file, arg);
			j++;
			break;
		    case 'N':
			show(arg, o->o_dest, file, copies);
			j = 0;
			break;
		}
	}

	fclose(cfp);	/*	unlock the control file		*/
}

int
show(nfile, dest, file, copies)
char	*nfile, *file;
char	*dest;
int	copies;
{
	char file_path[FILEMAX];
	char tmp1[4+1+6+1];
	struct stat lbuf;
	int	len_name;

	nfile = basename(nfile);

	if((len_name = strlen(nfile)) > DESTMAX){
		strcpy(nfile+DESTMAX, "...");
	}else{
		if(strcmp(nfile, " ") == 0){
			strcpy(nfile, (catgets(nlmsg_fd,NL_SETN,18, "(standard input)")) );
		}
	
		for(;strlen(nfile) < DESTMAX+strlen("...");)
			strcat(nfile, " ");
	}

	sprintf(file_path, "%s/%s/%s", REQUEST, dest, file);

	if(copies < 2){
		strcpy(tmp1, (catgets(nlmsg_fd,NL_SETN,19, "           ")) );
	}else{
		sprintf(tmp1, (catgets(nlmsg_fd,NL_SETN,20, "%-4d copies")), copies);
	}
	printf((catgets(nlmsg_fd,NL_SETN,21, "\t%-*s          %s")), DESTMAX, nfile, tmp1);

	if(*file && !stat(file_path, &lbuf)){
		printf((catgets(nlmsg_fd,NL_SETN,22, " %*ld bytes\n")), OSIZE, lbuf.st_size);
	}else{
		printf((catgets(nlmsg_fd,NL_SETN,23, " ??? bytes\n")));
	}
}


accept_stat(argc, argv)
int argc;
char *argv[];
{
	int i, bad = 0;
	struct qstat q;
	char *arg;

	if(argc == 0)
		while(getqent(&q) != EOF)
			putqline(&q);
	else {
		for(i = 0; i < argc; i++) {
			arg = argv[i];
			if(! isdest(arg)) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,24, "destination \"%s\" non-existent")), arg);
				fatal(errmsg, 0);
				bad++;
				*arg = '\0';
			}
		}
		if(bad == argc)
			return(0);
		while(getqent(&q) != EOF)
			for(i = 0; i < argc; i++)
				if(strcmp(q.q_dest, argv[i]) == 0) {
					putqline(&q);
					break;
				}
	}
	endqent();
	return(0);
}

putqline(q)
struct qstat *q;
{
	char *dt;

	dt = date(q->q_date);
	if(q->q_accept)
#ifndef NLS
		printf("%s accepting requests since %s\n", q->q_dest, dt);
#else NLS
	        printmsg((catgets(nlmsg_fd,NL_SETN,25, "%1$s accepting requests since %2$s\n")), q->q_dest, dt);
#endif NLS
	else
#ifndef NLS
		printf("%s not accepting requests since %s -\n\t%s\n", q->q_dest, dt, q->q_reason);
#else NLS
	        printmsg((catgets(nlmsg_fd,NL_SETN,26, "%1$s not accepting requests since %2$s -\n\t%3$s\n")), q->q_dest, dt, q->q_reason);
#endif NLS
}

printer(argc, argv)
int argc;
char *argv[];
{
	int i, bad = 0;
	struct pstat p;
	char *arg;

	if(argc == 0)
		while(getpent(&p) != EOF)
			putpline(&p);
	else {
		for(i = 0; i < argc; i++) {
			arg = argv[i];
			if(! isprinter(arg)) {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,27, "printer \"%s\" non-existent")), arg);
				fatal(errmsg, 0);
				bad++;
				*arg = '\0';
			}
		}
		if(bad == argc)
			return(0);
		while(getpent(&p) != EOF)
			for(i = 0; i < argc; i++)
				if(strcmp(p.p_dest, argv[i]) == 0) {
					putpline(&p);
					break;
				}
	}
	endpent();
	return(0);
}

putpline(p)
struct pstat *p;
{
	char tmp1[TMPLEN];
	char tmp2[TMPLEN + TMPLEN];
	char *dt;
#ifdef DELAYED
	int	delayed_message = 0;
#endif DELAYED

	tmp1[0] = '\0';
	tmp2[0] = '\0';

	dt = date(p->p_date);

	if(p->p_flags & P_AUTO)
		sprintf(tmp1, (catgets(nlmsg_fd,NL_SETN,28, "(login terminal) ")));
	if(p->p_flags & P_ENAB) {
		if(p->p_flags & P_BUSY) {
#ifdef REMOTE
			if ((p->p_rflags & (P_OMA|P_OPR)) != 0){
				sprintf(tmp2, (catgets(nlmsg_fd,NL_SETN,29, "now sending %s-%d to %s.  enabled since %s")), p->p_rdest, p->p_seqno, p->p_remotedest, dt);
			}else{
#ifndef NLS
				sprintf(tmp2, "now printing %s-%d.  enabled since %s", p->p_rdest, p->p_seqno, dt);
#else NLS
				sprintmsg(tmp2, (catgets(nlmsg_fd,NL_SETN,30, "now printing %1$s-%2$d.  enabled since %3$s")), p->p_rdest, p->p_seqno, dt);
#endif NLS

#ifdef DELAYED
				delayed_message = 1;
#endif DELAYED

			}
#else
#ifndef NLS
			sprintf(tmp2, "now printing %s-%d.  enabled since %s", p->p_rdest, p->p_seqno, dt);
#else NLS
			sprintmsg(tmp2, (catgets(nlmsg_fd,NL_SETN,30, "now printing %1$s-%2$d.  enabled since %3$s")), p->p_rdest, p->p_seqno, dt);
#endif NLS
#endif REMOTE
		}
		else {
#ifndef NLS
			sprintf(tmp2, "is idle.  enabled since %s", dt);
#else NLS
			sprintmsg(tmp2, (catgets(nlmsg_fd,NL_SETN,32, "is idle.  enabled since %1$s")), dt);
#endif NLS
		}
	}
	else {
#ifndef NLS
		sprintf(tmp2, "disabled since %s -\n\t%s", dt, p->p_reason);
#else NLS
		sprintmsg(tmp2, (catgets(nlmsg_fd,NL_SETN,33, "disabled since %1$s -\n\t%2$s")), dt, p->p_reason);
#endif NLS
	}
#ifndef NLS
	printf("printer %s %s%s\n", p->p_dest, tmp1, tmp2);
	printf("\tfence priority : %d\n", p->p_fence);
#else NLS
	printmsg((catgets(nlmsg_fd,NL_SETN,34, "printer %1$s %2$s%3$s\n")), p->p_dest, tmp1, tmp2);
	printf((catgets(nlmsg_fd,NL_SETN,35, "\tfence priority : %d\n")), p->p_fence);
#endif NLS
#ifdef REMOTE

#ifdef DELAYED
	if ((delayed_message == 1) && (delayed_status_enabled == 1)){
		delayed_status(p); /* display delayed status info. */
	}
#endif DELAYED

#endif REMOTE
}

device(argc, argv)
int argc;
char *argv[];
{
	int i;
	struct dirent *dir;
	DIR  *d;
	char dest[DESTMAX + 1];

	if(argc == 0) {
		if((d = opendir(MEMBER)) == (DIR *) NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,36, "MEMBER directory has disappeared!")), 1);
		while((dir = readdir(d)) != (struct dirent *) NULL)
			if(dir->d_ino != 0 && dir->d_name[0] != '.') {
				strncpy(dest, dir->d_name, DESTMAX + 1);
				dest[DESTMAX] = '\0';
				putdline(dest,argc,argv);
			}
		closedir(d);
	}
	else
		for(i = 0; i < argc; i++)
			if(isprinter(argv[i]))
				putdline(argv[i],argc,argv);
			else {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,27, "printer \"%s\" non-existent")), argv[i]);
				fatal(errmsg, 0);
			}
}

putdline(p,argc,argv)
int argc;
char *argv[];
char *p;
        {
        int i;
        struct pstat ps;
	char mfile[sizeof(MEMBER) + DESTMAX + 1];
	char dev[FILEMAX], *fgets();
	FILE *f = NULL, *fopen();

	sprintf(mfile, "%s/%s", MEMBER, p);
	if(GET_ACCESS(mfile, ACC_R) == 0 &&
	   (f = fopen(mfile, "r")) != NULL &&
	   fgets(dev, FILEMAX, f) != NULL) {
                 while(getpent(&ps) != EOF)
                                if(strcmp(ps.p_dest, p) == 0) 
                                        break;
                 endpent();
#ifndef NLS
		printf("device for %s: %s  remote to: %s %s \n", p, dev, ps.p_remoteprinter, ps.p_remotedest);
#else NLS
	        printmsg((catgets(nlmsg_fd,NL_SETN,38, "device for %1$s: %2$s")), p, dev); 
         if((ps.p_rflags & (P_OMA|P_OPR)) != 0)
    printmsg((catgets(nlmsg_fd,NL_SETN,60,"    remote to: %1$s on %2$s \n")),ps.p_remoteprinter, ps.p_remotedest);
#endif NLS
	}  else {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,39, "printer \"%s\" has disappeared!")), p);
		fatal(errmsg, 0);
	}
	if(f != NULL)
		fclose(f);
}

class(argc, argv)
int argc;
char *argv[];
{
	DIR  *d;
	char cl[DESTMAX + 1];
	struct dirent *dir;
	int i;

	if(argc == 0) {
		if((d = opendir(CLASS, "r")) == (DIR *) NULL)
			fatal((catgets(nlmsg_fd,NL_SETN,40, "CLASS directory has disappeared!")), 1);
		while((dir = readdir(d)) != (struct dirent *) NULL)
			if(dir->d_ino != 0 && dir->d_name[0] != '.') {
				strncpy(cl, dir->d_name, DESTMAX + 1);
				cl[DESTMAX] = '\0';
				putcline(cl);
			}
		closedir(d);
	}
	else
		for(i = 0; i < argc; i++)
			if(isclass(argv[i]))
				putcline(argv[i]);
			else {
				sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,41, "class \"%s\" non-existent")), argv[i]);
				fatal(errmsg, 0);
			}
}

putcline(c)
char *c;
{
	char cfile[sizeof(CLASS) + DESTMAX + 1];
	char member[DESTMAX + 2], *fgets();
	FILE *f = NULL, *fopen();

	sprintf(cfile, "%s/%s", CLASS, c);
	if(GET_ACCESS(cfile, ACC_R) == 0 &&
	   (f = fopen(cfile, "r")) != NULL) {
		printf((catgets(nlmsg_fd,NL_SETN,42, "members of class %s:\n")), c);
		while(fgets(member, DESTMAX + 1, f) != NULL)
			printf("\t%s", member);
	}
	else {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,43, "class \"%s\" has disappeared!")), c);
		fatal(errmsg, 0);
	}
	if(f != NULL)
		fclose(f);
}

running()
{
	if(enqueue(F_NOOP, "") == 0)
		printf((catgets(nlmsg_fd,NL_SETN,44, "scheduler is running\n")));
	else
	        printf((catgets(nlmsg_fd,NL_SETN,45, "scheduler is not running\n")));
}

/* dolist(func, list) -- apply function "func" to a "list" of arguments
	func is called as follows:
		func(argc, argv)
	where
		argc is number of elements in list
		argv is an array of pointers to the elements in list
*/

dolist(list, func)
char *list;
int (*func)();
{
	int argc = 0;
	char *argv[ARGMAX];
	int i;
	char *value, *argp, c, *malloc();

	if(list == NULL || *list == '\0') {
		(*func)(0, argv);
		return(0);
	}

	if((argp = malloc((unsigned)(strlen(list)+1))) == NULL)
		fatal(CORMSG, 1);
	strcpy(argp, list);
	while(*argp != '\0') {
		value = argp;
		while((c = *value) == ' ' || c == ',')
			value++;
		if(c == '\0')
			break;
		argp = value + 1;
		while((c = *argp) != '\0' && c != ' ' && c != ',')
			argp++;
		if(c != '\0')
			*(argp++) = '\0';
		if(enter(value, argv, &argc, ARGMAX) == -1)
			fatal(CORMSG, 1);
	}
	if(argc > 0) {
		(*func)(argc, argv);
		for(i = 0; i < argc; i++)
			free(argv[i]);
	}
	else
		(*func)(0, NULL);
	return(0);
}

startup(name)
char *name;
{
	int catch(), cleanup();
	extern char *f_name;
	extern int (*f_clean)();

	if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, catch);
	if(signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, catch);
	if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		signal(SIGQUIT, catch);
	if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, catch);

#ifdef REMOTE

#ifdef DELAYED
/*	This is a experiment to see if displaying delayed status */
/*	information would be useful.  This feature is enabled */
/*	when this command is named "statlp" instead of "lpstat" */

	if (strcmp(name,"statlp") == 0){
		delayed_status_enabled = 1;
	}else{
		delayed_status_enabled = 0;
	}
#endif DELAYED

#endif REMOTE
	f_name = name;
	f_clean = cleanup;
	if(chdir(SPOOL) == -1)
		fatal((catgets(nlmsg_fd,NL_SETN,46, "spool directory non-existent")), 1);
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
	endqent();
	endpent();
	endoent();
}

#ifdef REMOTE
doqueueuser(list)
char *list;
{
	char *value, *argp, c, *malloc();

	if(list == NULL || *list == '\0') {
		return(0);
	}

	if((argp = malloc((unsigned)(strlen(list)+1))) == NULL)
		fatal(CORMSG, 1);
	strcpy(argp, list);
	while(*argp != '\0') {
		value = argp;
		while((c = *value) == ' ' || c == ',')
			value++;
		if(c == '\0')
			break;
		argp = value + 1;
		while((c = *argp) != '\0' && c != ' ' && c != ',')
			argp++;
		if(c != '\0')
			*(argp++) = '\0';
		if(enter(value, user_names, &users, MAXUSERS) == -1)
			fatal(CORMSG, 1);
	}
	return(0);
}
douserlist()
{
	int	i;
	struct	pstat	p;
	int	pid, status;

	if(users > 0) {
		user(users, user_names);
	} else{
		user(0, NULL);
	}
	if (iflag){
		for(i = 0; i < users; i++)
			free(user_names[i]);
		return(0);
	}
	if (users == 0){
		if((user_names[users] = malloc((unsigned)(strlen("-all")+1))) == NULL)
			fatal(CORMSG, 1);
		user_names[users] = "-all";
	}
	while(getmpent(&p) != EOF){
		if  ((p.p_rflags & (P_OCI|P_OCM)) == 0) {
			continue;
		}
		sprintf(syscommand, "%s/%s/%s",SPOOL,SINTERFACE,p.p_dest);
		sprintf(destprinter, "-d%s",p.p_dest);
		option[0] = 0;

		for(i = 0; i < users; i++){
			if ((strlen(option) + strlen(user_names[i] + 3)) > TMPLEN){
				printf((catgets(nlmsg_fd,NL_SETN,47, "Not enough room for all the user names\n")));
				printf((catgets(nlmsg_fd,NL_SETN,48, "User name not added is %s\n")),user_names[i]);
				break;
			}
			strcat(option,"-u");
			strcat(option,user_names[i]);
			strcat(option," ");
		}
		if( (pid = fork()) == 0) {
			(void) execlp(syscommand, p.p_dest, destprinter, option, 0);
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, "execlp failed")));
			fatal(errmsg, 1);
		}
		else if (pid == -1){
			printf((catgets(nlmsg_fd,NL_SETN,6, "Unable to execute the remote status command %s.\n")),syscommand);
			printf((catgets(nlmsg_fd,NL_SETN,7, "Error %d occured.\n")), errno);
		}else{
			while((wait(&status)) != pid)
				;
		}
	}
	endmpent();
	for(i = 0; i < users; i++){
		free(user_names[i]);
	}
	return(0);
}
remoteoutput()
{
	int	active;
	int	index;
	int	index1;
	int	pid;
	int	remids_index;
	int	seqno;
	int	status;

	char	dest[DESTMAX + 1];
	char	head_dest[DESTMAX + 1];

	char	*head_ptr;
	char	*remids[MAXREQUESTS + 1];	/* remote request id array */

	active = nargs;
	while (active > 0){
		remids_index = 1;
		head_ptr = NULL;
		for (index = 0; index < nargs; index++){
			if (head_ptr == NULL){
				if (requ[index] != (char *) -1){
					if(isrequest(requ[index], head_dest, &seqno)) {
						head_ptr = requ[index];
						remids[remids_index] = requ[index];
						remids_index++;
						active--;
					}
				}
			}else{
				if (requ[index] != (char *) -1){
					if(isrequest(requ[index], dest, &seqno)) {
						if (!strcmp(head_dest,dest)){
							remids[remids_index] = requ[index];
							remids_index++;
							active--;
						}
					}
				}
			}
		}
		sprintf(syscommand , "%s/%s/%s",SPOOL,SINTERFACE,head_dest);
		remids[0] = syscommand;
		remids[remids_index] = NULL;
		if( (pid = fork()) == 0) {
			(void) execvp(syscommand, remids);
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,57, "execlp failed")));
			fatal(errmsg, 1);
		}
		else if (pid == -1){
			printf((catgets(nlmsg_fd,NL_SETN,6, "Unable to execute the remote status command %s.\n")),syscommand);
			printf((catgets(nlmsg_fd,NL_SETN,7, "Error %d occured.\n")), errno);
		}else{
			while((wait(&status)) != pid)
				;
			for (index1=0; index1 < remids_index; index1++){
				*remids[index1] = (char) -1;
			}
		}
	}
	for(index = 0; index < nargs; index++)
			free(requ[index]);
}

#ifdef DELAYED
int
delayed_status(p)
struct	pstat	*p;
{
	FILE	*m;
	FILE	*fopen();
	char	*fgets();
	char	device_name[FILEMAX];
	char	memfile[sizeof(MEMBER) + DESTMAX + 1];
	char	*c;
	struct	stat	buffer;
	long	current_time, delayed_time, temp, time();
	int	delayed_secs;
	int	delayed_mins;
	int	delayed_hours;
	int	delayed_days;

	sprintf(memfile, "%s/%s", MEMBER, p->p_dest);
	if((m = fopen(memfile, "r")) == NULL) {
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,53,"can't open %s file in MEMBER directory")), p->p_dest);
		fatal(errmsg, 0);
		return(1);
	} else {
		if(fgets(device_name, FILEMAX, m) == NULL){
			sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,54, "No device specified in MEMBER directory")), p->p_dest);
			fatal(errmsg, 0);
			return(1);
		}else{
		   if(*(c=device_name+strlen(device_name)-1) == '\n')
			*c = '\0';
		}
	fclose(m);
	}
	if (stat(device_name, &buffer) != 0){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,55, "Unable to stat the device  %s")), device_name);
		fatal(errmsg, 0);
		return(1);
	}
	if ((current_time = time(device_name, &buffer)) == -1){
		sprintf(errmsg, (catgets(nlmsg_fd,NL_SETN,56, "Unable to get the current time")));
		fatal(errmsg, 0);
		return(1);
	}
	if ((delayed_time = (current_time - (long) buffer.st_mtime)) > 0){
		if (delayed_time > (long) 60){
			delayed_secs	= delayed_time % (long) 60;
			temp		= delayed_time / (long) 60;
			delayed_mins	= temp % (long) 60;
			temp		= temp / (long) 60;
			delayed_hours	= temp % (long) 24;
			delayed_days	= temp / (long) 24;
			printf("\tprinting delayed %d:%d:%d:%d\n", delayed_days, delayed_hours, delayed_mins, delayed_secs);
		}
	}
	return(0);
}
#endif DELAYED
#endif REMOTE
