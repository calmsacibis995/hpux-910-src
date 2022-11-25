static char    *HPUX_ID = "@(#) $Revision: 70.2 $";
/*  man.c
    
    Changed 10/10/90 to recognize PAGER.  

    Changed 07/13/90 to recognize MANPATH.  MANPATH replaces the default
    paths.  If $LANG is used, each path in MANPATH has $LANG added to
    it and these are searched first (same as now).
    Randy Campbell

    Changed 4/24/89 to try full filename for a match before truncating
    to 11 characters.  should now work OK with LFN systems.
    Randy_Campbell  */

#include <stdio.h>
#include <ctype.h>
#include <sgtty.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>		/* for getenv() */
#include <limits.h>		/* for PATH_MAX, NL_LANGMAX */
/*  tmp kludge */
#ifndef PATH_MAX
# define PATH_MAX 1023
#endif

#ifdef NLS16
#include <nl_ctype.h>
#else
#define CHARAT(p)   *(p)&0377
#define ADVANCE(p)   (p)++
#define CHARADV(p)   (*(p)++)&0377
#endif

#ifndef NLS
#define nl_msg(i, s) (s)
#else NLS
#define NL_SETN 1		/* set number */
#include <msgbuf.h>
#include <locale.h>
#endif NLS

/*
 *  Defines and globals added to support MANPATH
 */
#define DEF_PATH  (unsigned char *)"/usr/man:/usr/contrib/man:/usr/local/man"
#ifdef NLS
#  define NLS_DEF_PATH  (unsigned char *)"/usr/man/%L:/usr/contrib/man/%L:/usr/local/man/%L:/usr/man:/usr/contrib/man:/usr/local/man"
#endif /* NLS */
unsigned char Dot[PATH_MAX + 1];
extern unsigned char *next_dir();
unsigned char *manpath;

/*
 *  Default to use for paging output if PAGER not defined in 
 *  environment.
 */
unsigned char  *pager = "more -s";

/*
 *  If nroff produces a file smaller than this many bytes, it's no good.
 *  Used in printit().
 */
#define MIN_FILESIZE	5

/*
 * man
 */
unsigned char loc_buf[40];
int     nomore;
int     zflag;			/* signifies compression */
int     bflag;			/* signifies noncompression */
char   *strcpy();
char   *strcat();
void	*calloc();
char   *trim();
int     remove();
char   *trunc();
char   *section = NULL;
int     troffit;
struct stat     stbuf, stbuf2;

#define   eq(a,b)   (strcmp(a,b) == 0)
#define wracc 02

#ifdef NLS
extern int  __nl_langid[];
#endif

/*                        Mon Jun 10, 1985
**   Added code to check /usr/man, /usr/contrib/man, /usr/local/man
**   No user interface change -- still needs work.          --  jad
*/
unsigned char    sbMsg[128];		/* error message buffer */

unsigned char   *sections[] = {		/* sections to check in man */
    	"1",
	"2",
	"3",
	"1m",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	NULL
};

main(argc, argv)
int     argc;
char   *argv[];
{
    unsigned char  *lang = (unsigned char *)"";
    unsigned char  *loc;
    short	   non_default_lang = 0;  /* set if not C or n-computer */
    short	   default_manpath = 0;   /* set if MANPATH default paths */
    unsigned char  *save_manpath;
    unsigned char  *pgr;

#ifdef NLS || NLS16
    if (!setlocale(LC_ALL, "")) {
	fputs(_errlocale("man"), stderr);
	/* this is to have default messages and to avoid duplicate */
	/* printing of _errlocale messages by the commands exec'ed */
	/* in this program.                  */
	putenv("LANG=");
	putenv("LC_COLLATE=");
	putenv("LC_CTYPE=");
	putenv("LC_MONETARY=");
	putenv("LC_NUMERIC=");
	putenv("LC_TIME=");
    }
    nl_catopen("man");
#endif NLS || NLS16

    if (signal(SIGINT, SIG_IGN) == SIG_DFL) {
	signal(SIGINT, remove);
	signal(SIGQUIT, remove);
	signal(SIGTERM, remove);
    }
    umask(0);
    if (argc <= 1) {
    usage:
	fprintf(stderr, (nl_msg(1, "Usage: man [ section ] name ...\n")));
	fprintf(stderr, (nl_msg(2, "or: man -k keyword ...\n")));
	fprintf(stderr, (nl_msg(3, "or: man -f file ...\n")));
	exit(1);
    }
    argc--, argv++;
    while (argc > 0 && argv[0][0] == '-') {
	switch (argv[0][1]) {

	case 0:
	    nomore++;
	    break;

	case 't':
	    troffit++;
	    break;

	case 'k':
	    apropos(argc - 1, argv + 1);
	    exit(0);

	case 'f':
	    whatis(argc - 1, argv + 1);
	    exit(0);
	default:
	    goto usage;
	}
	argc--, argv++;
    }
    if (troffit == 0 && nomore == 0 && !isatty(1))
	nomore++;
    else
	/*
	 *  If output is to be piped, get pager to use either from
	 *  PAGER in environment or from default.
	 */
        if((pgr = getenv("PAGER")) != NULL)
	    pager = pgr;

#ifdef NLS
    non_default_lang = __nl_langid[LC_ALL] != 0 && __nl_langid[LC_ALL] != 99;
#endif /* NLS */

    /*
     *  Find out if MANPATH is set;  if not, give it default value
     */
    if((manpath = (unsigned char *)getenv("MANPATH")) == NULL){
	/*
	 *  If locale other than C or n-computer, use path with %L 
	 *  components in it.
	 */
	if(non_default_lang)
	    manpath = NLS_DEF_PATH;
	else{
            manpath = DEF_PATH;
	    default_manpath = 1;
	}
    }
    else{
	/*
	 *  Get value of current directory to use in case of '.' in MANPATH
	 */
	if(getcwd(Dot,PATH_MAX + 1) == NULL){
	   fprintf(stderr,"Can't get current directory.\n");
	   exit(1);
	}

	/*
	 *  figure out if MANPATH is really the default set of paths.
	 */
	default_manpath = (strcmp(manpath,DEF_PATH) == 0);

	/*
	 *  If really default, but LANG is non-default, need NLS paths.
	 */
	if(default_manpath && non_default_lang)
	    manpath = NLS_DEF_PATH;
    }

    /*
     *  Save in case of repeated searches
     */
    save_manpath = manpath;

    do {
	if (argv[0][0] >= '0' && argv[0][0] <= '9' &&
		(argv[0][1] == 0 || argv[0][2] == 0)) {
	    argv[0][1] = tolower(argv[0][1]);
	    section = argv[0];
	    argc--, argv++;
	    if (argc == 0) {
		fprintf(stderr, (nl_msg(4, "But what do you want from section %s?\n")), argv[-1]);
		exit(1);
	    }
	    continue;
	}

	sbMsg[0] = NULL;
	/*
	 *  next_dir() which is in the file manlib.c, iteratively returns
	 *  pathnames from MANPATH until it is all used up (or we find our
	 *  match).  manpath is passed by reference, so that as a side 
	 *  effect, after each call, manpath points to the character after
	 *  the ':' that terminated the path returned by next_dir().
	 */
	while((loc = next_dir(&manpath)) != NULL){
	    if (chdir(loc) < 0)
		continue;	/*   silently   */
	    else {
		if (manual(section, argv[0]) == 0)
		    break;	/*   found it   */
	    }
	}

	/*
	 *  If we got here with loc == NULL, the entry hasn't been
	 *  found.  Report that fact before exiting.
	 */
	if (loc == NULL){ 
	    /*
	     *  The message could have been filled in by manual().  
	     *  If it wasn't, put something in.
	     */
	    if(sbMsg[0] == NULL)
		sprintf(sbMsg,"No manual entry for %s.\n",argv[0]);
	    fprintf(stderr, sbMsg);
	}

	argc--, argv++;
	/*
	 *  Restore manpath and flags in case we need to search for 
	 *  more entries.
	 */
	manpath = save_manpath;
	zflag = bflag = 0;
    } while (argc > 0);
    exit(0);
}

manual(section, name)
char   *section;
char   *name;
{
    char    work[100];
    int     sec;
    int     group;
    char   *dir;
    char   *suffix;
    char    subsec;
    char    sname[12];		/* used if name needs to be truncated */

    work[0] = sname[0] = 0;	/* cannot count on these arrays being zeroes*/
    trunc(name, sname);

    for (group = 0; group < 4; group++) {
	switch (group){
	case 0:
	    dir = "cat";
	    suffix = ".Z";
	    break;
	case 1:
	    dir = "man";
	    suffix = ".Z";
	    break;
	case 2:
	    dir = "man";
	    suffix = "";
	    break;
	case 3:
	    dir = "cat";
	    suffix = "";
	    break;
	}
	if (section){		/* section # specified by user */
	    if (eq(section, "1m")) {
		sprintf(work, "%s%s%s", dir, section, suffix);
		if (stat(work, &stbuf) < 0) /* dir exists? */
		    continue;
		sprintf(work, "%s%s%s/%s.%c", dir, section,
			suffix, name, *section);
		if (subsec = search(work, section[1])) {
		    resolve(group, name, section, subsec, work);
		    return (0);
		}
		else {		/* research with shortened name, if over 11 */
		    if (strlen(name) > 11) {
			sprintf(work, "%s%s%s/%s.%c", dir, section,
				suffix, sname, *section);
			if (subsec = search(work, section[1])) {
			    resolve(group, name, section, subsec, work);
			    return (0);
			}
		    }
		}
	    }
	    else {
		sprintf(work, "%s%c%s", dir, *section, suffix);
		if (stat(work, &stbuf) < 0) /* dir exists */
		    continue;
		sprintf(work, "%s%c%s/%s.%c", dir, *section,
			suffix, name, *section);
		if (subsec = search(work, section[1])) {
		    section[1] = '\0';
		    resolve(group, name, section, subsec, work);
		    return (0);
		}
		else {
		    if (strlen(name) > 11) {
			sprintf(work, "%s%s%s/%s.%c", dir, section,
				suffix, sname, *section);
			if (subsec = search(work, section[1])) {
			    section[1] = '\0';
			    resolve(group, name, section, subsec, work);
			    return (0);
			}
		    }
		}
	    }
	}
	else {			/* try all sections */
	    for (sec = 0; sections[sec]; sec++) {
		sprintf(work, "%s%s%s", dir, sections[sec], suffix);
		if (stat(work, &stbuf) < 0) /* dir exists */
		    continue;
		sprintf(work, "%s%s%s/%s.%c", dir, sections[sec],
			suffix, name, sections[sec][0]);
		if (subsec = search(work, sections[sec][1])) {
		    resolve(group, name, sections[sec], subsec, work);
		    return (0);
		}
		else {
		    if (strlen(name) > 11) {
			sprintf(work, "%s%s%s/%s.%c", dir, sections[sec],
				suffix, sname, sections[sec][0]);
			if (subsec = search(work, sections[sec][1])) {
			    resolve(group, name, sections[sec], subsec, work);
			    return (0);
			}
		    }
		}
	    }
	}
    }
    if (group == 4)
	if (section == 0)
	    return sprintf(sbMsg,
		    (nl_msg(6, "No manual entry for %s.\n")),
		    name);
	else
#ifndef NLS
	    return sprintf(sbMsg,
		    "No entry for %s in section %s of the manual.\n",
		    name, section);
#else
    return sprintmsg(sbMsg,
	    (nl_msg(7, "No entry for %1$s in section %2$s of the manual.\n")),
	    name, section);
#endif
}

search(path, subsec)
char   *path,
        subsec;
{
    char   *cp;

    int     last = strlen(path);
    path[last + 1] = '\0';
    if (subsec){
	path[last] = subsec;
	return stat(path, &stbuf) ? 0 : subsec;	/* file found */
    }
    else
	if (stat(path, &stbuf) == 0) {	/* file found */
	    return '1';		/* no subsection */
	}
	else {
	    switch (path[last - 1]) {
	    case '1':
		cp = "hmcgp";
		break;
	    case '2':
		cp = "hjpv";
		break;
	    case '3':
		cp = "csxmfdghjwin";
		break;
	    case '4':
		cp = "fhp";
		break;
	    case '5':
		cp = "h";
		break;
	    case '6':
		cp = "h";
		break;
	    case '7':
		cp = "7fhp";
		break;
	    case '8':
		cp = "h";
		break;
	    case '9':
		cp = "n";
		break;
	    }
	    while (*cp) {
		path[last] = *cp++;
		if (stat(path, &stbuf) == 0)	/* file found */
		    return path[last];
	    }
	}
    return 0;
}

resolve(group, name, section, subsec, work)
int     group;
char   *name,
       *section,
        subsec;
char   *work;
{
    char    work2[100], catdirz[100], catdir[100],
            catfile[100], temp[100], cmdbuf[BUFSIZ];

    if (subsec == '1')
	subsec = 0;		/* return 1 = no subsection */
    switch (group){
    case 0:			/* found
				   cat<section>.Z/<name>.<section><subsec> */
	sprintf(work2, "man%s.Z/%s.%c%c", section, name, *section, subsec);
	if ((stat(work2, &stbuf2) == 0) &&
		(stbuf2.st_mtime > stbuf.st_mtime)) {	/* man.Z is newer */
	    sprintf(temp, "/tmp/man%d", getpid());
	    sprintf(cmdbuf, "uncompress -f < %s > %s", work2, temp);
	    system(cmdbuf);
	    sprintf(catdirz, "cat%s.Z", section);
	    sprintf(catdir, "cat%s", section);
	    sprintf(catfile, "/%s.%c%c", name, *section, subsec);
	    printit(temp, catdirz, catdir, catfile);
	    return;
	}
	sprintf(work2, "man%s/%s.%c%c", section, name, *section, subsec);
	if ((stat(work2, &stbuf2) == 0) &&
		(stbuf2.st_mtime > stbuf.st_mtime)) {	/* man is newer */
	    sprintf(catdirz, "cat%s.Z", section);
	    sprintf(catdir, "cat%s", section);
	    sprintf(catfile, "/%s.%c%c", name, *section, subsec);
	    printit(work2, catdirz, catdir, catfile);
	    return;
	}
	sprintf(work2, "cat%s/%s.%c%c", section, name, *section, subsec);
	if ((stat(work2, &stbuf2) == 0) &&
		(stbuf2.st_mtime > stbuf.st_mtime)) {
	    nroff(work2);
	    return;
	}
	zflag++;
	nroff(work);
	return;
    case 1:			/* found
				   man<section>.Z/<name>.<section><subsec> */
	sprintf(work2, "man%s/%s.%c%c", section, name, *section, subsec);
	if ((stat(work2, &stbuf2) == 0) &&
		(stbuf2.st_mtime > stbuf.st_mtime)) {
	    sprintf(catdirz, "cat%s.Z", section);
	    sprintf(catdir, "cat%s", section);
	    sprintf(catfile, "/%s.%c%c", name, *section, subsec);
	    printit(work2, catdirz, catdir, catfile);
	    return;
	}
	sprintf(work2, "cat%s/%s.%c%c", section, name, *section, subsec);
	if ((stat(work2, &stbuf2) == 0) &&
		(stbuf2.st_mtime > stbuf.st_mtime)) {
	    nroff(work2);
	    return;
	}
	sprintf(temp, "/tmp/man%d", getpid());
	sprintf(cmdbuf, "uncompress -f < %s > %s", work, temp);
	system(cmdbuf);
	sprintf(catdirz, "cat%s.Z", section);
	sprintf(catdir, "cat%s", section);
	sprintf(catfile, "/%s.%c%c", name, *section, subsec);
	printit(temp, catdirz, catdir, catfile);
	return;
    case 2:			/* found man<section>/<name>.<section><subsec> 
				*/
	sprintf(work2, "cat%s/%s.%c%c", section, name, *section, subsec);
	if ((stat(work2, &stbuf2) == 0) &&
		(stbuf2.st_mtime > stbuf.st_mtime)) {	/* cat is newer */
	    nroff(work2);
	    return;
	}
	else {
	    sprintf(catdirz, "cat%s.Z", section);
	    sprintf(catdir, "cat%s", section);
	    sprintf(catfile, "/%s.%c%c", name, *section, subsec);
	    printit(work, catdirz, catdir, catfile);
	    return;
	}
    case 3:			/* found cat<section>/<name>.<section><subsec> 
				*/
	nroff(work);
	return;
    }
}

/* filenames will be truncated to 11 characters and retried if no match is
   found for the entire filename.  */

char   *trunc(filename, sfilename)
char   *filename;
char   *sfilename;
{

    register i     , b_status = ONEBYTE;

    strncpy(sfilename, filename, 11);

    for (i = 0; i < 11; i++)
	b_status = BYTE_STATUS(filename[i] & 0377, b_status);
    if (b_status == FIRSTOF2)
	sfilename[10] = '\0';
    else
	sfilename[11] = '\0';

    return (sfilename);
}

printit(manfile, catdirz, catdir, catfile)
char   *manfile;
char   *catdir;
char   *catdirz;
char   *catfile;
{
    char    cmdbuf[BUFSIZ];
    char    catprint[100];
    char    catprintz[100];
    char    tmpfile[PATH_MAX+1];

    struct stat sbuf;

    if (troffit)
	troff(manfile);
    else {
	FILE   *it;
	char    abuf[BUFSIZ];

	if (!nomore) {

/* this next part checks to see if the man file is going to source */
/* another man file.  If it is, man will not create a cat file so  */
/* that the current source will be printed if a change is made to  */
/* the file being sourced.                                         */
/*                            */
/* This code will not work with a file in man.1m or if a file from */
/* /usr/local/man or /usr/contrib/man is being sourced or if nLS   */
/* is being used.  I left it here as to not break anything.        */
/*                            */
/*                                     9-25-86  Gayle Guidry (gmg) */

	    if ((it = fopen(manfile, "r")) == NULL) {
		perror(manfile);
		exit(1);
	    }
	    if (fgets(abuf, BUFSIZ - 1, it) &&
		    abuf[0] == '.' && abuf[1] == 's' &&
		    abuf[2] == 'o' && abuf[3] == ' ') {
		register char  *cp = abuf + strlen(".so ");
		char   *dp;

		while (*cp && *cp != '\n')
		    cp++;
		*cp = 0;
		while (cp > abuf && *--cp != '/');
		dp = ".so /usr/man/man";
		if (cp != abuf + strlen(dp) + 1) {
		tohard:
		    nomore = 1;
		    strcpy(manfile, abuf + 4);
		    goto hardway;
		}
		for (cp = abuf; *cp == *dp && *cp; cp++, dp++);
		if (*dp)
		    goto tohard;
		strcpy(manfile, cp - 3);
	    }
	    fclose(it);
/* end of code for source  of another file */
	    zflag = 0;
	    bflag = 0;
	    if (access(catdirz, wracc) == 0) {
		strcpy(catprintz, catdirz);
		strcat(catprintz, catfile);
		zflag++;
	    }
	    if (access(catdir, wracc) == 0) {
		strcpy(catprint, catdir);
		strcat(catprint, catfile);
		bflag++;
	    }
	    else
		if (!zflag)
		    goto hardway;

	    fputs((nl_msg(9, "Reformatting entry.  Wait...")), stderr);
	    fflush(stdout);
	    if (bflag)
		unlink(catprint);
	    if (zflag)
		unlink(catprintz);

	    /*
	     *  Create tmp file name
	     */
	    sprintf(tmpfile,"/tmp/cat%d",getpid());

	    /*
	     *  Format the manpage into the tmpfile
	     *  If this step fails, abort gracefully.
	     */
	    sprintf(cmdbuf,
		    "tbl -TX %s | neqn | nroff -h -man | col -x > %s",
		    manfile, tmpfile);
	    if (system(cmdbuf)) {
		printf((nl_msg(10, " aborted (sorry)\n")));
		remove(1);
		/* NOTREACHED */
	    }

	    /*
	     * Check to see that a file of reasonable length got created;
	     * if not, abort gracefully.  (This is to fix a bug about
	     * leaving null files in cat* directories if nroff step
	     * fails.)
	     * If the file is OK, go ahead and create a copy of it
	     * in cat*.
	     */
	    if(stat(tmpfile,&sbuf) != 0 || sbuf.st_size < MIN_FILESIZE){
		printf(nl_msg(10, " aborted (sorry)\n"));
		remove(1);
	    }
	    else{
		sprintf(cmdbuf,"trap '' 1 15; %s %s %s %s",
	    	    bflag ? "mv" : "compress < ",
		    tmpfile, bflag ? "" : " > ", 
		    bflag ? catprint : catprintz);
		if(system(cmdbuf)){
		    printf((nl_msg(10, " aborted (sorry)\n")));
		    remove(1);
		}
	    }

	    /*
	     * If a cat*.Z directory was found as well, copy the
	     * file to it, too.
	     */
	    if (bflag && zflag) {
		sprintf(cmdbuf, "compress < %s > %s",
			catprint, catprintz);
		if (system(cmdbuf)) {
		    printf(nl_msg(10, " aborted (sorry)\n"));
		    remove(1);
		    /* NOTREACHED */
		}
	    }
	    fputs((nl_msg(11, " done\n")), stderr);
	    if (bflag)
		strcpy(manfile, catprint);
	    else
		strcpy(manfile, catprintz);
	}
    hardway:
	nroff(manfile);
    }
    return 0;
}

/*
 *  Figures out where manpage is coming from, so know what treatment 
 *  it needs:
 *
 *	(1) cat*.Z -- uncompress with zcat
 *	(2) cat*   -- just cat it
 *	(3) man*   -- run it through nroff, et.al.
 *
 *  In each case, nomore is tested.  If not set, output is piped
 *  through pager, otherwise is, in effect, cat'd.
 */
nroff(cp)
char   *cp;
{
    char    cmd[BUFSIZ];

    if (cp[0] == 'c')		/* if catfile exists */
	if (zflag && !bflag)	/* if its compressed */
	    sprintf(cmd,
		    nomore ? "exec zcat < %s" : "exec zcat < %s | %s",
		    cp,pager);
	else			/* not compressed */
	    sprintf(cmd,
		    "exec %s %s",(nomore ? (unsigned char *)"cat -s" : pager),cp);
    else {			/* we have to nroff it */
	fputs((nl_msg(9, "Reformatting entry.  Wait...")), stderr);
	fflush(stdout);
	sprintf(cmd,
		"tbl -TX %s | neqn | nroff -man | col -x | %s",
		cp, (nomore ? (unsigned char *)"cat -s" : pager));
    }
    if (system(cmd))
	remove(1);
    remove(0);
}

troff(cp)
char   *cp;
{
    char    cmdbuf[BUFSIZ];

    sprintf(cmdbuf, "troff -man %s", cp);
    system(cmdbuf);
}

any(c, sp)
register int    c;
register char  *sp;
{
    register int    d;

    while (d = CHARADV(sp));
    if (c == d)
	return (1);
    return (0);
}

remove(excode)
int     excode;
{
    char    name[15];

    sprintf(name, "/tmp/man%d", getpid());
    unlink(name);
    sprintf(name, "/tmp/cat%d", getpid());
    unlink(name);
    if (excode)
	exit(excode);
}

apropos(argc, argv)
int     argc;
char  **argv;
{
    char    buf[BUFSIZ];
    char   *gotit;
    register char **vp;

    if (argc == 0) {
	fprintf(stderr, (nl_msg(12, "man: -k what?\n")));
	exit(1);
    }

#ifdef NLS
    if (__nl_langid[LC_ALL] && __nl_langid[LC_ALL] != 99) {
	sprintf(loc_buf, "/usr/lib/nls/%s/whatis", getenv("LANG"));
	if (freopen(loc_buf, "r", stdin) != NULL)
	    goto opened;
    }
#endif
    if (freopen("/usr/lib/whatis", "r", stdin) == NULL) {
	perror("/usr/lib/whatis");
	exit(1);
    }
opened:
    gotit = calloc(1, blklen(argv));
    while (fgets(buf, sizeof buf, stdin) != NULL)
	for (vp = argv; *vp; vp++)
	    if (match(buf, *vp)) {
		printf("%s", buf);
		gotit[vp - argv] = 1;
		for (vp++; *vp; vp++)
		    if (match(buf, *vp))
			gotit[vp - argv] = 1;
		break;
	    }
    for (vp = argv; *vp; vp++)
	if (gotit[vp - argv] == 0)
	    printf((nl_msg(13, "%s: nothing appropriate\n")), *vp);
}

match(buf, str)
char   *buf, *str;
{
    register char  *bp;

    bp = buf;
    for (;;) {
	if (*bp == 0)
	    return (0);
	if (amatch(bp, str))
	    return (1);
	ADVANCE(bp);
    }
}

amatch(cp, dp)
register char  *cp, *dp;
{

    while (*cp && *dp && lmatch(CHARAT(cp), CHARAT(dp)))
	ADVANCE(cp), ADVANCE(dp);
    if (*dp == 0)
	return (1);
    return (0);
}

lmatch(c, d)
int     c, d;
{

    if (c == d)
	return (1);
#ifdef NLS16
    if (c > 0377 || d > 0377 || !isalpha(c) || !isalpha(d))
	return (0);
#else
    if (!isalpha(c) || !isalpha(d))
	return (0);
#endif
    if (islower(c))
	c = toupper(c);
    if (islower(d))
	d = toupper(d);
    return (c == d);
}

blklen(ip)
register int   *ip;
{
    register int    i = 0;

    while (*ip++)
	i++;
    return (i);
}

whatis(argc, argv)
int     argc;
char  **argv;
{
    register char **avp;

    if (argc == 0) {
	fprintf(stderr, (nl_msg(14, "man: -f what?\n")));
	exit(1);
    }
#ifdef NLS
    if (__nl_langid[LC_ALL] && __nl_langid[LC_ALL] != 99) {
	sprintf(loc_buf, "/usr/lib/nls/%s/whatis", getenv("LANG"));
	if (freopen(loc_buf, "r", stdin) != NULL)
	    goto opened;
    }
#endif
    if (freopen("/usr/lib/whatis", "r", stdin) == NULL) {
	perror("/usr/lib/whatis");
	exit(1);
    }
opened:
    for (avp = argv; *avp; avp++)
	*avp = trim(*avp);
    whatisit(argv);
    exit(0);
}

whatisit(argv)
char  **argv;
{
    char    buf[BUFSIZ];
    register char  *gotit;
    register char **vp;

    gotit = calloc(1, blklen(argv));
    while (fgets(buf, sizeof buf, stdin) != NULL)
	for (vp = argv; *vp; vp++)
	    if (wmatch(buf, *vp)) {
		printf("%s", buf);
		gotit[vp - argv] = 1;
		for (vp++; *vp; vp++)
		    if (wmatch(buf, *vp))
			gotit[vp - argv] = 1;
		break;
	    }
    for (vp = argv; *vp; vp++)
	if (gotit[vp - argv] == 0)
	    printf((nl_msg(15, "%s: not found\n")), *vp);
}

wmatch(buf, str)
char   *buf, *str;
{
    register char  *bp, *cp;

    bp = buf;
again:
    cp = str;
    while (*bp && *cp && lmatch(CHARAT(bp), CHARAT(cp)))
	ADVANCE(bp), ADVANCE(cp);
    if (*cp == 0 && (*bp == '(' || *bp == ',' || *bp == '\t' || *bp == ' '))
	return (1);
    while (isalpha(*bp) || isdigit(*bp))
	bp++;
    if (*bp != ',')
	return (0);
    bp++;
    while (isspace(*bp))
	bp++;
    goto again;
}

char   *
trim(cp)
register char  *cp;
{
    register char  *dp;

    for (dp = cp; *dp; ADVANCE(dp))
	if (*dp == '/')
	    cp = dp + 1;
    if (cp[0] != '.') {
	if (cp + 3 <= dp && dp[-2] == '.' && any(dp[-1], "cosa12345678npP"))
	    dp[-2] = 0;
	if (cp + 4 <= dp && dp[-3] == '.' && any(dp[-2], "13") && isalpha(dp[-1]))
	    dp[-3] = 0;
    }
    return (cp);
}

