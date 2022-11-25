#ifndef lint
static  char rcsid[] = "@(#)makedbm:	$Revision: 1.33.109.4 $	$Date: 94/12/16 09:19:12 $  ";
#endif
/* makedbm.c	2.1 86/04/16 NFSSRC */
/*static  char sccsid[] = "makedbm.c 1.1M 86/02/05  Copyr 1985 Sun Micro";*/

/* 
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* RB added clean exit code to remove temp files and semaphonre file
   on error exit. made exit(1) calls go here.
   also added a line to remove sem file on regular exit. 7/23/91

   Also removed the umask statement and added chmod lines at the end to
   set only read permissions on files.  This allows no root users
   to run this program - because dbminit requires write access. rb 7/29/31
*/

#ifdef PATCH_STRING
static char *patch_5081="@(#) PATCH_9.0: makedbm.o $Revision: 1.33.109.4 $ 94/06/01 PHNE_5081";
#endif

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include <dbm.h>
#undef NULL
#include <stdio.h>
#include <sys/types.h>		/*  Needed inclusion after 5.22 install  */
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stat.h>
/* added the following for file locking - prabha 02/09/89 */
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#define LASCII_DATE 11
#define MAXLINE 4096		/* max length of input line */
#define MAXHOST 255		/*  max length of host name */
static char *get_date();
static char *any();
FILE *fopen();
int fcntl();
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

/* added for file locking - prabha */

int	sem_fd = -1; /* fd on lock semaphore */
char	sem_file[MAXPATHLEN];

/* NEXT 2 varibales moved here  from main to allow clean exit rb 7/23/91 */
char tmppagbuf[MAXPATHLEN];
char tmpdirbuf[MAXPATHLEN];

int catch_it(sig)
int sig;
{
	if (sem_fd > -1)
		lockf (sem_fd, F_ULOCK, 0);
	exit (sig);
}


main(argc, argv)
	char **argv;
{
	FILE *outfp, *infp;
	datum key, content, tmp;
	char buf[MAXLINE];
	char pagbuf[MAXPATHLEN];
	char dirbuf[MAXPATHLEN];
	char *p,ic;	
	char *infile, *outfile;
#ifdef DBINTERDOMAIN
	char *infilename, *outfilename, *mastername, *domainname, *interdomain_bind, *lower_case_keys;
#else
	char *infilename, *outfilename, *mastername, *domainname, *lower_case_keys;
#endif /* DBINTERDOMAIN */
	char local_host[MAXHOST];
	int cnt,i;
        /* buffers used by fstat call.  mjk (1/13/87) */
        struct stat dbuf;
        struct stat pbuf;

	signal(SIGTERM, catch_it);
	signal(SIGINT,  catch_it);
	signal(SIGQUIT, catch_it);
	signal(SIGHUP,  catch_it);
	signal(SIGSEGV,  catch_it);

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("makedbm",0);
#endif NLS
	infile = outfile = NULL; /* where to get files */
	/* name to imbed in database */
#ifdef DBINTERDOMAIN
	infilename = outfilename = mastername = domainname = interdomain_bind = lower_case_keys = NULL; 
#else
	infilename = outfilename = mastername = domainname = lower_case_keys = NULL; 
#endif /* DBINTERDOMAIN */
	argv++;
	argc--;
	while (argc > 0) {
		if (argv[0][0] == '-' && argv[0][1]) {
			switch(argv[0][1]) {
				case 'i':
					infilename = argv[1];
					argv++;
					argc--;
					break;
				case 'o':
					outfilename = argv[1];
					argv++;
					argc--;
					break;
				case 'm':
					mastername = argv[1];
					argv++;
					argc--;
					break;
				case 'd':
					domainname = argv[1];
					argv++;
					argc--;
					break;
#ifdef DBINTERDOMAIN
				case 'b':
				   	interdomain_bind = argv[0];
					break;
#endif /* DBINTERDOMAIN */
				case 'l':
                                        lower_case_keys = argv[0];
                                        break;
				case 'u':
					unmake(argv[1]);
					return;
				default:
					usage();
			}
		}
		else if (infile == NULL)
			infile = argv[0];
		else if (outfile == NULL)
			outfile = argv[0];
		else
			usage();
		argv++;
		argc--;
	}
	if (infile == NULL || outfile == NULL)
		usage();
	if (strcmp(infile, "-") != 0)
		infp = fopen(infile, "r");
	else
		infp = stdin;
	if (infp == NULL) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,1, "makedbm: can't open %s\n")), infile);
		exit(1);
	}
/*
 * Now get a write lock on sem_fd so that multiple ypmakes will not
 * mess up the databases. Note that all the options that need to open
 * the output file (i.e, all except -u) will get here. Wait until we
 * get the lock so the access is serialised - prabha
 */
	strncpy(sem_file, outfile, strlen(outfile)); /*make the name*/
	strcat (sem_file, "~"); /* of the semaphore file from the db file */
	sem_fd = open (sem_file, O_RDWR | O_CREAT, 0644);/* get the fd
						      for sem_file */

	if (sem_fd < 0) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,23, "makedbm: can't open semaphore file %s\n")), sem_file);
		exit(1);
	}

	if (lockf(sem_fd, F_LOCK, 0) == -1) 
	{
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,24, "makedbm: can't lock semaphore file %s\n")), sem_file);
		fclose (infp);		/* close the input */
                unlink (sem_file);      /* rb 7/23/91 */
	        exit (1);		/* somthin is wrong, just get out */
	}
/*
 *  HPNFS
 *
 *  To create a temporary filename, instead of appending ".tmp" to the
 *  filename like Sun does, change the last character to '~'.
 *  Dave Erickson, 2-22-87.
 *
 *  HPNFS
 */
	strncpy(tmppagbuf, outfile, strlen(outfile)-1);
	strcat(tmppagbuf, "~");
	strcpy(tmpdirbuf, tmppagbuf);
	strcat(tmpdirbuf, ".dir");
	strcat(tmppagbuf, ".pag");

	if ((outfp = fopen(tmpdirbuf, "w")) == NULL) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,2, "makedbm: can't create %s\n")), tmpdirbuf);
		cln_sem_exit();
	}
        if ( fstat(fileno(outfp), &dbuf) == -1) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,19, "makedbm: can't stat %s\n")), tmpdirbuf);
                  cln_sem_exit();
         }
	if ((outfp = fopen(tmppagbuf, "w")) == NULL) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,3, "makedbm: can't create %s\n")), tmppagbuf);
                 cln_sem_exit();
	}
        if ( fstat(fileno(outfp), &pbuf) == -1) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,20, "makedbm: can't stat %s\n")), tmppagbuf);
                cln_sem_exit();
         }

/* fstat was used above to determine the ID of the device containing
 * the file and the inode number of the file. If these are the same for
 * both files, then the user has specified an outfile name which is too
 * long (no room for the .pag and .dir extensions). This fix still
 * allows a user to create databases that can not append the full
 * extension. This fix prevents makedbm from core dumping when tmppagbuf
 * and tmpdirbuf are the same file due to truncation of the file name
 * in a short file name system.
 * MARK KEAN (mjk 1/13/87) 
 */
        if ( (dbuf.st_dev == pbuf.st_dev) && (dbuf.st_ino == pbuf.st_ino) ) {
	    	fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,21, "makedbm: the outfile name, %s is too long\n")), outfile);
                if ( unlink(tmpdirbuf) == -1) 
        	    fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,22, "makedbm: can't unlink temporary file %s\n")), tmpdirbuf);

	}
/*  HPNFS
 *
 *  To create a temporary filename, instead of appending ".tmp" to the
 *  filename like Sun does, change the last character to '~'.
 *  Dave Erickson, 2-22-87.
 *
 *  HPNFS  */
	strncpy(dirbuf, outfile, strlen(outfile)-1);
	strcat(dirbuf, "~");

	/* dbminit will create two files by appending .dir and .pag to the argu-
	 * ment. That will make those names same as tmppagbuf and tmpdirbuf */

	if (dbminit(dirbuf) != 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,4, "makedbm: can't init %s\n")), dirbuf);
		    cln_sem_exit();	
	}
	strcpy(dirbuf, outfile);
	strcpy(pagbuf, outfile);
	strcat(dirbuf, ".dir");
	strcat(pagbuf, ".pag");
	while (fgets(buf, sizeof(buf), infp) != NULL) {
		p = buf;
		cnt = strlen(buf) - 1; /* erase trailing newline */
		while (p[cnt-1] == '\\') {
			p+=cnt-1;
			if (fgets(p, sizeof(buf)-(p-buf), infp) == NULL)
				goto breakout;
			cnt = strlen(p) - 1;
		}
		p = any(buf, " \t\n");
		key.dptr = buf;
		key.dsize = p - buf;
		while (1) {
			if (p == NULL || *p == NULL) {
				fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,5, "makedbm: yikes!\n")));
				    cln_tmp_sem_exit();
			}
			if (*p != ' ' && *p != '\t')
				break;
			p++;
		}
		content.dptr = p;
		content.dsize = strlen(p) - 1; /* erase trailing newline */
		if (lower_case_keys)
		  for (i=0; i<key.dsize;i++) {
		    ic = *(key.dptr+i);
		    if (isascii(ic) && isupper(ic))
		      *(key.dptr+i) = tolower(ic);
			}
		tmp = fetch(key);
		if (tmp.dptr == NULL) {
			if (store(key, content) != 0) {
				nl_printf((catgets(nlmsg_fd,NL_SETN,6, "problem storing %1$.*s %2$.*s\n")),
				    key.dsize, key.dptr,
				    content.dsize, content.dptr);
				    cln_tmp_sem_exit();
			}
		}
#ifdef DEBUG
		else {
			nl_printf((catgets(nlmsg_fd,NL_SETN,7, "duplicate: %1$.*s %2$.*s\n")),
			    key.dsize, key.dptr,
			    content.dsize, content.dptr);
		}
#endif
	}
   breakout:
	addpair((catgets(nlmsg_fd,NL_SETN,14, "YP_LAST_MODIFIED")), get_date(infile));
	if (infilename)
		addpair((catgets(nlmsg_fd,NL_SETN,15, "YP_INPUT_FILE")), infilename);
	if (outfilename)
		addpair((catgets(nlmsg_fd,NL_SETN,16, "YP_OUTPUT_NAME")), outfilename);
	if (domainname)
		addpair((catgets(nlmsg_fd,NL_SETN,17, "YP_DOMAIN_NAME")), domainname);
#ifdef DBINTERDOMAIN
	if (interdomain_bind)
	   	addpair("YP_INTERDOMAIN", "");
#endif /* DBINTERDOMAIN */
	if (!mastername) {
		gethostname(local_host, sizeof (local_host) - 1);
		mastername = local_host;
	}
	addpair((catgets(nlmsg_fd,NL_SETN,18, "YP_MASTER_NAME")), mastername);

	dbmclose();

	if (rename(tmppagbuf, pagbuf) < 0)
		perror((catgets(nlmsg_fd,NL_SETN,8, "makedbm: rename")));
	if (rename(tmpdirbuf, dirbuf) < 0)
		perror((catgets(nlmsg_fd,NL_SETN,9, "makedbm: rename")));

        chmod(pagbuf, S_IRUSR | S_IRGRP | S_IROTH); /* rb 7/29/91 */
        chmod(dirbuf, S_IRUSR | S_IRGRP | S_IROTH);

	if (lockf (sem_fd, F_ULOCK, 0) == -1)
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,25, "makedbm: can't unlock semaphore file %s\n")), sem_file);
        unlink(sem_file); /* rb 7/23/91 */
	exit(0);


}



/* 
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}

static char *
get_date(name)
	char *name;
{
	struct stat filestat;
	static char ans[LASCII_DATE];/* ASCII numeric string */

	if (strcmp(name, "-") == 0)
		sprintf(ans, "%010d", time(0));
	else {
		if (stat(name, &filestat) < 0) {
			fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,10, "makedbm: can't stat %s\n")), name);

		    cln_tmp_sem_exit();
		}
		sprintf(ans, "%010d", filestat.st_mtime);
	}
	return ans;
}

usage()
{
#ifdef DBINTERDOMAIN
	fprintf(stderr,
		"usage: makedbm -u file\n       makedbm [-b] [-i NIS_INPUT_FILE] [-o NIS_OUTPUT_NAME] [-d NIS_DOMAIN_NAME] [-m NIS_MASTER_NAME] infile outfile\n");
#else
	fprintf(stderr,
(catgets(nlmsg_fd,NL_SETN,11, "usage: makedbm -u file\n       makedbm [-i NIS_INPUT_FILE] [-o NIS_OUTPUT_NAME] [-d NIS_DOMAIN_NAME] [-m NIS_MASTER_NAME] infile outfile\n")));
#endif /* DBINTERDOMAIN */
	exit(1);
}

addpair(str1, str2)
	char *str1, *str2;
{
	datum key;
	datum content;
	
	key.dptr	= str1;
	key.dsize	= strlen(str1);
	content.dptr	= str2;
	content.dsize	= strlen(str2);
	if (store(key, content) != 0) {
		nl_printf((catgets(nlmsg_fd,NL_SETN,12, "makedbm: problem storing %1$.*s %2$.*s\n")),
		    key.dsize, key.dptr, content.dsize, content.dptr);
			 cln_tmp_sem_exit();
	}
}

unmake(file)
	char *file;
{
	datum key, content;

	if (file == NULL)
		usage();
	
	if (dbminit(file) != 0) {
		fprintf(stderr, (catgets(nlmsg_fd,NL_SETN,13, "makedbm: couldn't init %s\n")), file);
		exit(1);
	}
	for (key = firstkey(); key.dptr != NULL; key = nextkey(key)) {
		content = fetch(key);
		printf("%.*s %.*s\n", key.dsize, key.dptr,
		    content.dsize, content.dptr);
	}
}

 /* next few lines added by RB to do cleanup on error exit */
cln_tmp_sem_exit()
{
        unlink(tmppagbuf);
        unlink(tmpdirbuf);
        lockf(sem_fd, F_ULOCK, 0);
        unlink(sem_file);
        exit(1);
}

cln_sem_exit()
{
        lockf(sem_fd, F_ULOCK, 0);
        unlink(sem_file);
        exit(1);
}
