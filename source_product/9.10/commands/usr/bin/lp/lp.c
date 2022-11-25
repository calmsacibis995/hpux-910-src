/* $Revision: 70.2.1.1 $ */
/* lp -- print files on a line printer */

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 6                                       /* set number */
#include <nl_ctype.h>
#include <msgbuf.h>
#endif NLS

#include        "lp.h"
#ifdef TRUX
#include <sys/security.h>
#endif

char work[FILEMAX];     /* Global error message string */
struct stat stbuf;      /* Global stat buffer */
char title[TITLEMAX + 1] = NULL;        /* User-supplied title for output */
int copies = 1;         /* number of copies of output */
short priority = -1;    /* priority of request */
int optlen = 0;         /* size of option string for interface  program */
char opts[OPTMAX] = NULL;       /* options for interface program */
int silent = 0;         /* don't run off at the mouth */
short mail = FALSE;     /* TRUE ==> user wants mail, FALSE ==> no mail */
short wrt = FALSE;      /* TRUE ==> user wants notification on tty via write,
                           FALSE ==> don't write */
short copy = FALSE;     /* TRUE ==> copy files, FALSE ==> don't */
char curdir[FILEMAX+1]; /* working directory at time of request */

int nfiles = 0;         /* number of files on cmd line (excluding "-") */
int fileargs = 0;       /* total number of file args */
int stdinp = 0;         /* indicates how many times to print std input
                           -1 ==> standard input empty          */
 
int uid;                /* real user id -- for accessing protected */
                        /* files */
int euid;               /* effective user id -- as set by setuid bit */
int gid;                /* real group id -- for accessing protected */
                        /* files */
int egid;               /* effective group id -- as set by setgid bit */
int saved_id = -1;              /* saved id (is not being changed) */
int currentuid;         /* holds the current read id */
char tname[RNAMEMAX] = NULL;    /* name of temp request file */
FILE *tfile = NULL;             /* stream for temp request file */
char rname[RNAMEMAX] = NULL;    /* name of actual request file */
char stdbase[NAMEMAX];  /* basename of copy of standard input */
char reqid[IDSIZE + 1]; /* request id to be supplied to user */
#ifdef REMOTE
char datafilename[NAMEMAX];
int     finchar;        /* holds index into first  changing character */
int     sinchar;        /* holds index into second changing character */
#endif REMOTE


/*
        The structure outq is to be aligned for sharing on both
        the 300 and 800.  Before you make any changes, make sure
        the alignment and size of the structure match on both
        machines.  This is to support DUX on teh 300 and 800.
*/
#ifdef REMOTE
struct outq o = {       /* output request to be appended to output queue */
        0L,     /* size of request */
        0L,     /* date of entry into output queue */
        0L,     /* date of printing start */
        0,      /* sequence # */
        0,      /* not printing, not deleted */
        -1,     /* priority */
        0,      /* Use three digit sequence numbers */
        NULL,   /* host name */
        NULL,   /* logname */
        NULL,   /* destination */
        NULL    /* device where printing */
};
#else REMOTE
struct outq o = {       /* output request to be appended to output queue */
        NULL,   /* destination */
        NULL,   /* logname */
        0,      /* sequence # */
        0L,     /* size of request */
        NULL,   /* device where printing */
        0L,     /* date of entry into output queue */
        0L,     /* date of printing start */
        0,      /* not printing, not deleted */
        -1      /* priority */
};
#endif REMOTE

struct qstat q;         /* acceptance status */
struct pstat p;

char clienthost[SP_MAXHOSTNAMELEN];

extern char **environ;	/* for cleanenv() */

main(argc, argv)
int argc;
char *argv[];
{

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

        options(argc, argv);    /* process command line options */

        if(chdir(SPOOL) == -1)
                fatal((nl_msg(1, "spool directory non-existent")), 1);

        defaults();             /* establish default parameters */
        setpriority();

        if(getqdest(&q, o.o_dest) == EOF) {     /* get acceptance status */
                sprintf(work, (nl_msg(2, "acceptance status of destination \"%s\" unknown")), o.o_dest);
                fatal(work, 1);
        }
        endqent();
        if(! q.q_accept) {      /* accepting requests ? */
#ifndef NLS
                sprintf(work, "can't accept requests for destination \"%s\" -\n\t%s", o.o_dest, q.q_reason);
#else NLS
                sprintmsg(work, (nl_msg(3, "can't accept requests for destination \"%1$s\" -\n\t%2$s")), o.o_dest, q.q_reason);
#endif NLS
                fatal(work, 1);
        }

        openreq();              /* create and init request file */

#ifdef REMOTE
/*
        For remote operation: put the name of the host,
        the user logname, the jobname (printer-sequence),
        classification (hostname) and the user logname 
        in the request file in addition to the usual
        HP-UX request commands (NOTE:  The commands have
        been redefined for remote operation.
*/

        sprintf(jobprinter,"%s-%d",o.o_dest,o.o_seqno);

        putrent(R_HOSTNAME      , o.o_host     , tfile);
        putrent(R_PERSON        , o.o_logname  , tfile);
        putrent(R_JOBNAME       , jobprinter   , tfile);
        putrent(R_CLASSIFICATION, clienthost   , tfile);
        putrent(R_LITERAL       , o.o_logname  , tfile);
#endif REMOTE

#ifdef REMOTE
        putrent(R_HEADERTITLE, title, tfile);
#else
        putrent(R_TITLE, title, tfile);
#endif REMOTE
        sprintf(work, "%d", copies);
        putrent(R_COPIES, work, tfile);
        putrent(R_OPTIONS, opts, tfile);

        if(stdinp > 0)
                savestd();      /* save standard input */

        files(argc, argv);      /* process command line file arguments */

        closereq();             /* complete and then close request file */

        time(&o.o_date);
        o.o_size *= copies;
        signal(SIGHUP, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, SIG_IGN);
        addoent(&o);            /* enter request in output queue */
        endoent();

#ifdef REMOTE
        sprintf(work, "%s %d %s %s %d", o.o_dest, o.o_seqno, o.o_logname, o.o_host, o.o_rflags);
#else
        sprintf(work, "%s %d %s", o.o_dest, o.o_seqno, o.o_logname);
#endif REMOTE


        /* enqueue can return 0,-1, or -2. We are only interested in -1 */
        /* since that indicates a fifo write error. We still print the  */
        /* request id message for a -2 return, since the most likely    */
        /* cause of that problem is that the scheduler is not running.  */

        if(enqueue(F_REQUEST, work) != -1)  /* inform scheduler of request */
                qmesg();                /* issue request id message */
        else
                qmesg2();

        exit(0);
}

/* setpriority -- set the value of priority in the outputq file */

setpriority()
{
        short   class_default();

        if (priority >= MINPRI && priority <= MAXPRI){
                o.o_priority = priority;
        }else{
                if (priority != -1){
                        sprintf(work, (nl_msg(4, "bad priority %d")), priority);
                        fatal(work, 1);
                }else{
                        if(getpdest(&p, o.o_dest) == EOF){
                                if(isclass(o.o_dest)){
                                        p.p_default = class_default(o.o_dest);
                                }else{
                                        sprintf(work, (nl_msg(5, "no such printer or class \"%s\"")), o.o_dest);
                                        fatal(work, 1);
                                }
                        }

		/* verify that printer pstat structure is not corrupt */
		/* If the default priority is out of range, refuse    */
		/* the request with a fatal error                     */

        		if (p.p_default < MINPRI || p.p_default > MAXPRI){
                            sprintf(work, (nl_msg(19, "bad default priority in %s/%s for printer: %s")), SPOOL,PSTATUS,p.p_dest);
                            fatal(work, 1);
			}

                        o.o_priority = p.p_default;
                        endpent();
                }
        }
}

/* class_default - get default priority when class destination was specified */

short
class_default(dest)
char    *dest;
{
        char    *c, printer[DESTMAX+1];
        char    class[FILEMAX+1];
        FILE    *fp;
        struct  pstat p;
        short   default_max = 0;

        sprintf(class, "%s/%s/%s", SPOOL, CLASS, dest);
        if((fp = fopen(class, "r")) == NULL) {
                sprintf(work, (nl_msg(6, "can't open %s file in CLASS directory")), o.o_dest);
                fatal(work, 1);
        }else{
                while(fgets(printer, DESTMAX, fp) != NULL){
                        if(*(c=printer+strlen(printer)-1) == '\n')
                                *c = '\0';
                        if(getpdest(&p, printer) == EOF)
                                continue;
                        else
                                if(p.p_default > default_max)
                                        default_max = p.p_default;
                }
                return(default_max);
        }
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

/* cleanup -- called by catch() after interrupts or by fatal() after errors */

cleanup()
{
        /* Unlink request and data files (if any) */

#ifdef REMOTE
        rmreq(o.o_dest, o.o_seqno, o.o_host, (o.o_rflags & O_OB3));
#else
        rmreq(o.o_dest, o.o_seqno);
#endif REMOTE

        endoent();
        endqent();
}

/* closereq -- complete and close request file */

closereq()
{
        char    string[2];

        if(fileargs == 0)
#ifdef REMOTE
                putitall(stdbase," ");
#else
                putrent(R_FILE, stdbase, tfile);
#endif REMOTE

        if(mail)
                putrent(R_MAIL, o.o_logname, tfile);
        if(wrt)
                putrent(R_WRITE, o.o_logname, tfile);

#ifdef REMOTE
        sprintf(string, "%d", (int)o.o_priority);
        putrent(R_PRIORITY, string, tfile);
#endif REMOTE

        /* Rename temporary request file to real request file name */

        fclose(tfile);
        if(link(tname, rname) == -1) {
                sprintf(work, (nl_msg(7, "can't create request file %s")), rname);
                fatal(work, 1);
        }
        unlink(tname);
}

/*
 * copyfile(stream, name) -- copy stream to file "name"
 *      name is actually just the basename of a file
 *      the file is created under REQUEST/o.o_dest/
 */

copyfile(stream, name)
FILE *stream;
char *name;
{
        FILE *ostream;
        int i;
        char buf[BUFSIZ];

        sprintf(work, "%s/%s/%s", REQUEST, o.o_dest, name);
        if((ostream = fopen(work, "w")) == NULL) {
                sprintf(work, (nl_msg(8, "cannot create temp file %s")), name);
                fatal(work, 1);
        }

#ifdef SecureWare
	if(ISSECURE)
            lp_change_mode(SPOOL, work, 0440, ADMIN,
                       "data file for printer request");
	else
            chmod(work, 0440);
#else
        chmod(work, 0440);
#endif
        while((i = fread(buf, sizeof(char), BUFSIZ, stream)) > 0){
                fwrite(buf, sizeof(char), (unsigned) i, ostream);
                if(feof(stream)) break;
        }

        fclose(ostream);
#ifdef SecureWare
	if(ISB1)
            lp_save_label(stream, work);
#endif
}

/* defaults -- establish default destination if not set on command line */

defaults()
{
	char rootname[SP_MAXHOSTNAMELEN];
        char *d, *getenv(), *c, *strcpy(), *strncpy();
        FILE *dflt;     /* stream for reading default destination file */

        if(o.o_dest[0] == '\0') {       /* avoid optimiser bug */
                if((d = getenv(LPDEST)) != NULL && *d != '\0') {
                        if(strlen(d) > DESTMAX) {
                                sprintf(work, (nl_msg(9, "%s destination \"%s\" illegal")), LPDEST, d);
                                fatal(work, 1);
                        }
                        strncpy(o.o_dest, d, sizeof(o.o_dest));
                }
                else  {
                        if((d = getenv(PRINTER)) != NULL && *d != '\0') {
                           if(strlen(d) > DESTMAX) {
                                   sprintf(work, (nl_msg(9, "%s destination \"%s\"illegal")), LPDEST, d);
                                   fatal(work, 1);
                           }
                           strncpy(o.o_dest, d, sizeof(o.o_dest));
                        } 
                        else {
                           if((dflt = fopen(DEFAULT, "r")) == NULL)
                                fatal((nl_msg(10, "can't open default destination file")),1);
                           if(fgets(o.o_dest, DESTMAX+1, dflt) == NULL) {
                                fatal((nl_msg(11, "no system default destination")), 1);
                           } 
                           fclose(dflt);
                           if(*(c = o.o_dest + strlen(o.o_dest) -1) == '\n')
                                *c = '\0';
                        }
                } 

                if(! isdest(o.o_dest)) {
                        sprintf(work, (nl_msg(12, "default destination \"%s\" non-existent")), o.o_dest);
                        fatal(work, 1);
               } 
       } 
#ifdef REMOTE
        gethostname(clienthost, SP_MAXHOSTNAMELEN);
	if ((getrootname(clienthost,rootname)) == -1)
	{
	    strncpy(o.o_host, clienthost, SP_MAXHOSTNAMELEN);
	    fatal((nl_msg(17, "Warning: clusterserver name not found")),0);
		/* this error is indicative of an improper cluster */
		/* configuration--check /etc/clusterconf	   */
	}
	else
	    strncpy(o.o_host, rootname, SP_MAXHOSTNAMELEN);
#endif REMOTE
}

/* files -- process command line file arguments */

files(argc, argv)
int argc;
char *argv[];
{
        int i;
        char *fullpath(), *file, *dname, *newname(), *full;
        FILE *f;

        for(i = 1; i < argc; i++) {
                file = argv[i];
                if(file == NULL)
                        continue;
                if(strcmp(file, "-") == 0) {
                        if(stdinp > 0)
#ifdef REMOTE
                                putitall(stdbase," ");
#else
                                putrent(R_FILE, stdbase, tfile);
#endif REMOTE
                }
                else {
                        setugid(uid);
                        if((full = fullpath(file, curdir)) == NULL) {
                                setugid(euid);
                                fatal((nl_msg(13, "can't read current directory")), 1);
                        }
                        setugid(euid);
                        dname = newname();
#ifdef SecureWare
			if(((ISSECURE)&&(!lp_file_source(copy, (short) TRUE)))||
                           ((!ISSECURE) && (!copy)))
#else
                        if(!copy)
#endif
			{
                                sprintf(work, "%s/%s/%s", REQUEST, o.o_dest, dname);
                                if(GET_ACCESS(full, ACC_R) < 0
                                        || link(full, work) < 0)
                                        copy = TRUE;
                                else
#ifdef REMOTE
                                        putitall(dname,file);
#else
                                        putrent(R_FILE, dname, tfile);
#endif REMOTE
                        }
#ifdef SecureWare
			if(((ISSECURE) &&
                        	(lp_file_source(copy, (short) TRUE))) ||
			   ((!ISSECURE) && (copy))) 
#else
			if(copy) 
#endif			
			{
                                setugid(uid);
                                if((f=fopen(full,"r")) != NULL) {
                                        setugid(euid);
                                        copyfile(f, dname);
                                        fclose(f);
                                }
                                else {
                                        setugid(euid);
                                        sprintf(work, (nl_msg(14, "can't open file %s")), full);
                                        fatal(work, 1);
                                }
/*
                                setugid(euid);
*/
#ifdef REMOTE
                                putitall(dname,file);
#else
                                putrent(R_FILE, dname, tfile);
#endif REMOTE
                        }
                }
        }
}

/* getseq(snum) -- get next sequence number */

getseq(snum)
int *snum;
{
        FILE *fp;
        char seqfilename[sizeof(REQUEST) + sizeof(o.o_dest) +7];
        extern FILE *lockf_open();

#ifdef REMOTE
        if (q.q_ob3){
                sprintf(seqfilename,"%s/%s/.seq", REQUEST, o.o_dest);
        }else{
                sprintf(seqfilename,"%s", SEQFILE );
        }
#else
        sprintf(seqfilename,"%s", SEQFILE );
#endif REMOTE

        if ((fp = lockf_open(seqfilename, "r+", TRUE)) == NULL) {
                fatal(nl_msg(15, "can't open new sequence number file %s"), seqfilename);
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
            lp_change_mode(SPOOL, SEQFILE, 0644, ADMIN,
                       "holds ID for unique printer file generation");
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

/* newname() -- create new name for data file
        returns a pointer to the new name.
        The file will ultimately be created under SPOOL/REQUEST/o.o_dest/
*/

char *
newname()
{
#ifdef REMOTE
        if (++datafilename[sinchar] > 'z') {
                if (++datafilename[finchar] == 't') {
                        printf((nl_msg(34, "too many files - break up the job\n")));
                        cleanup();
                }
                datafilename[sinchar] = 'A';
        } else if (datafilename[sinchar] == '[')
                datafilename[sinchar] = 'a';
        return(datafilename);
#else
        static int n = 0;
        static char name[NAMEMAX];

        sprintf(name, "d%d-%d", n++, o.o_seqno);
        return(name);
#endif REMOTE
}

/* openreq -- open and initialize request file */

openreq()
{
    char dname[RNAMEMAX];
	strcpy (dname, "");

        while(tfile == NULL) {
                getseq(&o.o_seqno);
#ifdef REMOTE
                if (q.q_ob3){
                        sprintf(datafilename,"cfz%03d%s",o.o_seqno,o.o_host);
                        sprintf(tname       ,"%s/%s/tfA%03d%s",REQUEST, o.o_dest, o.o_seqno,o.o_host);
                        sprintf(rname       ,"%s/%s/cfA%03d%s",REQUEST, o.o_dest, o.o_seqno,o.o_host);
                        sprintf(dname       ,"%s/%s/dfA%03d%s",REQUEST, o.o_dest, o.o_seqno,o.o_host);
                        sinchar = 2;
                        finchar = 0;
                        o.o_rflags = O_OB3;
                }else{
                        sprintf(datafilename,"cz%04d%s",o.o_seqno,o.o_host);
                        sprintf(tname       ,"%s/%s/tA%04d%s",REQUEST, o.o_dest, o.o_seqno,o.o_host);
                        sprintf(rname       ,"%s/%s/cA%04d%s",REQUEST, o.o_dest, o.o_seqno,o.o_host);
                        sprintf(dname       ,"%s/%s/dA%04d%s",REQUEST, o.o_dest, o.o_seqno,o.o_host);
                        sinchar = 1;
                        finchar = 0;
                        o.o_rflags = 0;
                }
#else
                sprintf(tname, "%s/%s/t-%d", REQUEST, o.o_dest, o.o_seqno);
                sprintf(rname, "%s/%s/r-%d", REQUEST, o.o_dest, o.o_seqno);
#endif REMOTE

                if(GET_ACCESS(tname, 0) == -1 && GET_ACCESS(rname, 0) == -1 &&
                  (GET_ACCESS (dname, 0) == -1) && (tfile = fopen(tname, "w")) != NULL)
#ifdef SecureWare
                {
		    if(ISSECURE){
                        lp_change_mode(SPOOL, tname, 0440, ADMIN,
                                       "file that holds data to print");
#ifdef B1
			if(ISB1)
                            lp_banner_label(tname);
#endif
		    }
		    else
                        chmod(tname, 0440);
                }
#else
                        chmod(tname, 0440);
#endif 	
        }

        sprintf(reqid, "%s-%d", o.o_dest, o.o_seqno);
}

/* options -- process command line options */

#define OPTMSG  (nl_msg(16, "too many options for interface program"))

options(argc, argv)
int argc;
char *argv[];
{
        extern char *optarg;
        extern int optind, opterr;
        int i;
        char letter;            /* current keyletter */
        char *value;            /* value of current keyletter (or NULL) */
        char *strcat(), *strncpy(), *file;

        opterr = 1;             /* turns on getopt error reporting */
#if defined(SecureWare) && defined(B1)
        while ((letter = getopt(argc, argv, "cd:mn:o:p:st:wlf")) != EOF) 
#else
        while ((letter = getopt(argc, argv, "cd:mn:o:p:st:w")) != EOF) 
#endif	
	{

                        letter = tolower(letter);
                        switch(letter) {

                        case 'c':       /* copy files */
                                copy = TRUE;
                                break;
                        case 'd':       /* destination */
                                if(! isdest(optarg)) {
                                        sprintf(work, (nl_msg(18, "destination \"%s\" non-existent")), optarg);
                                        fatal(work, 1);
                                }
                                strncpy(o.o_dest, optarg, DESTMAX);
                                o.o_dest[DESTMAX] = NULL;
                                break;

                        case 'm':       /* mail */
                                mail = TRUE;
                                break;
                        case 'n':       /* # of copies */
                                if ((copies=atoi(optarg)) <= 0)
                                        copies = 1;
                                break;
                        case 'p':       /* priority of request */
                                if((priority=(short)atoi(optarg)) < 0
					|| priority > 7)
                                        priority = -1;
                                break;
                        case 'o':       /* option for interface program */
                                if(optlen == 0)
                                        optlen = strlen(optarg);
                                else
                                        optlen = strlen(optarg) + 1;
                                if(optlen >= OPTMAX)
                                        fatal(OPTMSG, 1);
                                if(*opts != '\0')
                                        strcat(opts, " ");
                                strcat(opts, optarg);
                                break;
                        case 's':       /* silent */
                                silent = 1;
                                break;
                        case 't':       /* title */
                                strncpy(title, optarg, TITLEMAX);
                                title[TITLEMAX] = '\0';
                                break;
                        case 'w':       /* write */
                                wrt = TRUE;
                                break;
#if defined(SecureWare) && defined(B1)
			case 'l':
			case 'f':
				if(ISB1)
                                    lp_special_options(letter, argv, i);
                                        break;
#endif	
                        default:
                        case '?':
                                exit(1);
                        }
        }

	for ( i=1; i < optind; argv[i++] = NULL );

                /* file name or - */
        /* file names should be left */
        while (optind < argc) {
                        fileargs++;
                        file = argv[optind++];
                        if(strcmp(file, "-") == 0)
                                stdinp++;
                        else {
                                setugid(uid);
                                if(stat(file, &stbuf) == -1) {
                                        sprintf(work, (nl_msg(20, "can't access file \"%s\"")), file);
                                        goto badf;
                                }
                                else if((stbuf.st_mode & S_IFMT) == S_IFDIR) {
                                        sprintf(work, (nl_msg(21, "\"%s\" is a directory")), file);
                                        goto badf;
                                }
                                else if(stbuf.st_size == 0) {
                                        sprintf(work, (nl_msg(22, "file \"%s\" is empty")), file);
                                        goto badf;
                                }
                                else{   setugid(euid);
                                        if(access(file, ACC_R) == -1) {
                                                sprintf(work, (nl_msg(20, "can't access file \"%s\"")), file);
                                                goto badf;
                                        }
                                        else {
                                                nfiles++;
                                                o.o_size += stbuf.st_size;
                                        }
                                }
                        }
                setugid(euid);
                continue;

badf:
                setugid(euid);
                fatal(work, 0); 
                file = NULL;

        }

        if(fileargs == 0)
                stdinp = 1;
        else if(nfiles == 0 && stdinp == 0)
                fatal((nl_msg(24, "request not accepted")), 1);
}

/* qmesg -- issue request id message */

qmesg()

{
        char tmp1[TMPLEN];
        char tmp2[TMPLEN];
        char tmp3[TMPLEN];

        tmp1[0] = '\0';
        tmp2[0] = '\0';
        tmp3[0] = '\0';
        
        if(silent == 1) return;
        if(nfiles > 0) {
                if(nfiles > 1)
                        sprintf(tmp1, (nl_msg(25, "%d files")), nfiles);
                else
                        sprintf(tmp1, (nl_msg(26, "%d file")), nfiles);
        }
        if(stdinp > 0) {
                if(nfiles > 0)
                        sprintf(tmp2, (nl_msg(27, " and ")));
                sprintf(tmp3, (nl_msg(28, "standard input")));
        }
#ifndef NLS
        printf("request id is %s (%s%s%s)\n", reqid, tmp1, tmp2, tmp3);
#else NLS
        printmsg((nl_msg(29, "request id is %1$s (%2$s%3$s%4$s)\n")), reqid, tmp1, tmp2, tmp3);
#endif /* NLS */
}

/* qmesg2 -- request did not enqueue */

qmesg2()

{
        if(silent == 1) return;
        printf((nl_msg(30, "request id %s failed to enqueue\n")), reqid);
}

/* savestd -- save standard input */

savestd()
{
        char *newname();

        strcpy(stdbase, newname());
        copyfile(stdin, stdbase);
        sprintf(work, "%s/%s/%s", REQUEST, o.o_dest, stdbase);
        stat(work, &stbuf);
        if(stbuf.st_size == 0) {
                fatal((nl_msg(31, "standard input is empty")), 0);
                unlink(work);
                if(nfiles == 0)         /* no files to queue */
                        fatal((nl_msg(24, "request not accepted")), 1);
                else    /* inhibit printing of std input */
                        stdinp = -1;
        }
        else
                o.o_size += (stdinp * stbuf.st_size);
}

/* startup -- initialization routine */

startup(name)
char *name;
{
        int catch(), cleanup();
        struct passwd   *adm, *getpwnam();
        extern char *f_name;
        extern int (*f_clean)();

	cleanenv(&environ, "LANG", "LANGOPTS", "NLSPATH", LPDEST, PRINTER, 0);
#ifdef NLS
        nl_catopen("lp");
#endif NLS

        if(signal(SIGHUP, SIG_IGN) != SIG_IGN)
                signal(SIGHUP, catch);
        if(signal(SIGINT, SIG_IGN) != SIG_IGN)
                signal(SIGINT, catch);
        if(signal(SIGQUIT, SIG_IGN) != SIG_IGN)
                signal(SIGQUIT, catch);
        if(signal(SIGTERM, SIG_IGN) != SIG_IGN)
                signal(SIGTERM, catch);

#ifdef SecureWare
	if(ISSECURE)
            lp_set_mask(0000);
	else
	    umask(0000);
#else	
	umask(0000);
#endif
        f_name = name;
        f_clean = cleanup;

#ifdef SecureWare
	if(ISSECURE){
	    uid =  (int)getuid();
	    euid = (int)geteuid();
	    gid =  (int)getgid();
	    egid = (int)getegid();
	}
	else{
            if((adm = getpwnam(ADMIN)) == NULL)
                fatal("LP Administrator not in password file\n", 1);

            uid  = (int)getuid();
            euid = (int)adm->pw_uid;
            gid  = (int)getgid();
            egid = (int)adm->pw_gid;
	}
#else
        if((adm = getpwnam(ADMIN)) == NULL)
                fatal("LP Administrator not in password file\n", 1);

        uid  = (int)getuid();
        euid = (int)adm->pw_uid;
        gid  = (int)getgid();
        egid = (int)adm->pw_gid;
#endif
        currentuid = (int)geteuid();	/* set the current user id
						to the current effective uid */
        setugid(uid); gwd(curdir);	/* get current directory */
        setugid(euid);

#ifdef SecureWare
	if(ISSECURE)
            strcpy(o.o_logname, lp_getlname());
	else
            strcpy(o.o_logname, getname());
#else
        strcpy(o.o_logname, getname());
#endif
        strcpy(o.o_dev, "-");
}

/*
   putitall will put the R_FILE, R_FORMATTEDFILE, R_UNLINKFILE and
   the R_FILENAME entries in the control file.

   The R_FILE          entry is for the HP-UX spool system.
   The R_FORMATTEDFILE entry is for the BSD spool system.
   The R_UNLINEFILE    entry is for the BSD spool system.
   The R_FILENAME      entry is for the BSD spool system.
*/
#ifdef REMOTE
putitall(datafile,realname)
char    *datafile;
char    *realname;
{
        int     index;

/*      make an entry in the control file using the original name
        of the file (or " " for stdin) for the title to be used
        by the pr command (BSD) */

        putrent(R_TITLE        , realname, tfile);

/*      make an entry in the control file for the file that is
        to be printed (HP-UX) */

        putrent(R_FILE         , datafile, tfile);

/*      make an entry in the control file for the file that is
        to be printed (BSD) */

        for (index = 0; index < copies; index++){
                putrent(R_FORMATTEDFILE, datafile, tfile);
        }

/*      make an entry in the control file for the file to unlink when
        the operation is complete (BSD) */

        putrent(R_UNLINKFILE   , datafile, tfile);

/*      make an entry in the control file using the original name
        of the file (or " " for stdin) for status requests (BSD) */

        putrent(R_FILENAME     , realname, tfile);
}
#endif /* REMOTE */
/*
        setugid = uid,  set the real user id 
        setugid = euid, set the effective user id 
*/
int
setugid(idtype)
int     idtype;
{
        int     lruid;          /* holds real      user  id */
        int     leuid;          /* holds effective user  id */
        int     lrgid;          /* holds real      group id */
        int     legid;          /* holds effective group id */

        if (idtype == uid){
                lruid = euid;   /* set the effective user and group */
                leuid = uid;    /* ids to the original real user and */
                lrgid = egid;   /* real group ids */
                legid = gid;
        }else{
                lruid = uid;    /* set the effective user and group */
                leuid = euid;   /* ids to the original effective user */
                lrgid = gid;    /* and effective group ids */
                legid = egid;
        }
/*
        Set the effective id.
        if the current effective id is already set, do not reset it.
*/
        if (currentuid != leuid){
                currentuid = leuid;
                if (setresuid(lruid, leuid, saved_id) == -1){
                        sprintf(work,"Unable to set the effective user id to %d\n", leuid);
                        fatal(work,1);
                }
                if (setresgid(lrgid, legid, saved_id) == -1){
                        sprintf(work,"Unable to set the effective group id to %d\n", legid);
                        fatal(work,1);
                }
        }
}
