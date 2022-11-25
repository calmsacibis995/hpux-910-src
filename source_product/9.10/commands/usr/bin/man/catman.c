/* @(#) $Revision: 70.4 $ */
#ifndef lint
    static char    *HPUX_ID = "@(#) $Revision: 70.4 $";
#endif

# include    <stdio.h>
# include    <sys/types.h>
# include    <dirent.h>
# include    <sys/stat.h>
# include    <ctype.h>
# include    <errno.h>
# include    <stdlib.h>		/* for getenv() */
# include    <unistd.h>		/* for getcwd() and getopt() */
# include    <string.h>
# include    <strings.h>

# define    reg    register
# define    bool    unsigned char
# define    MAX_DIRS    1000

# define    SYSTEM(str)    (pflag ? printf("%s\n\n", str) : system(str))


/*  
 *  dirList and dup_list point to dynamically allocated lists so
 *  that catman can handle any number of entries in a directory 
 */
static unsigned char  **dirList;
static int *dup_list;		/*  list of inodes with links  */
static int  num_dups = 0;

extern char    *strrchr();
unsigned char   buf[BUFSIZ],
                pflag = 0,
		mflag = 0,
                nflag = 0,
                wflag = 0,
                zflag = 0;
unsigned char **ReadDir();
unsigned char  *loc;

/*
**  Added defines and globals to support MANPATH
*/
#ifndef PATH_MAX
#  define PATH_MAX	1023
#endif
#define DEF_PATH (unsigned char *)"/usr/man:/usr/contrib/man:/usr/local/man"

#ifdef NLS
#  define NLS_DEF_PATH (unsigned char *)"/usr/man/%L:/usr/contrib/man/%L:/usr/local/man/%L:/usr/man:/usr/contrib/man:/usr/local/man"
#endif				/* NLS */
extern unsigned char   *next_dir();
unsigned char   Dot[PATH_MAX + 1];

int     exstat = 0;
bool    changed = 0;
static struct stat  sbuf, Zsbuf, csbuf;

main(argc, argv)
int     argc;
unsigned char  *argv[];
{

    reg unsigned char  *msp, *Zmsp, *csp, *sp;
    unsigned char   man[MAXNAMLEN], Zman[MAXNAMLEN], cat[MAXNAMLEN];
    reg unsigned char  *sections;
    int     i;
    int     findex, Zfindex;
    short   order;
    unsigned char **entries=NULL,
                  **Zentries=NULL;
    unsigned char   Zflag = 0;
    unsigned char  *manpath, *lang;
    short   non_default_lang, default_manpath;
    char c;		/* for getopt */

    while ((c = getopt(argc,(char **)argv,"mnpwz")) != EOF)
	switch(c){
	case 'm':
	    mflag++;
	    break;
	case 'p':
	    pflag++;
	    break;
	case 'n':
	    nflag++;
	    wflag = 0;
	    break;
	case 'w':
	    wflag++;
	    nflag = 0;
	case 'z':
	    zflag++;
	    break;
	case '-':
	case '?':
	    goto usage;
	default:
	    break;
        }

    argc -= optind;
    argv = &argv[optind];

    if (argc == 1)
	sections = *argv;
    else if (argc < 1)
	sections = (unsigned char *)"11m23456789";
    else {
    usage:
	fprintf(stderr,"usage: catman [ -p ] [ -m ] [ -n ] [ -w ] [ -z ] [ sections ]\n");
	exit(-1);
    }


#ifdef NLS
    lang = (unsigned char *)getenv("LANG");
    non_default_lang = (strcmp((char *)lang, "C") == 0 ||
			strcmp((char *)lang, "n-computer") == 0);
#endif				/* NLS */

    /* 
     *  Find out if MANPATH is set;  if not, give it default value
     */
    if ((manpath = (unsigned char  *)getenv("MANPATH")) == NULL) {
	/* 
	 *  If locale other than C or n-computer, use path with %L 
	 *  components in it.
	 */
	if (non_default_lang)
	    manpath = NLS_DEF_PATH;
	else {
	    manpath = DEF_PATH;
	    default_manpath = 1;
	}
    }
    else {
	/* 
	 *  Get value of current directory to use in case of '.' in MANPATH
	 */
	if (getcwd((char *)Dot, PATH_MAX + 1) == NULL) {
	    fprintf(stderr, "Can't get current directory.\n");
	    exit(1);
	}

	/* 
	 *  figure out if MANPATH is really the default set of paths.
	 */
	default_manpath = (strcmp((char *)manpath, (char *)DEF_PATH) == 0);

	/* 
	 *  If really default, but LANG is non-default, need NLS paths.
	 */
	if (default_manpath && non_default_lang)
	    manpath = NLS_DEF_PATH;
    }


    if (wflag)
	goto whatis;

    /* 
     *  for each of the directories, whether default or other specified
     *  by MANPATH, chdir() into it and start finding man?, man?.Z, cat?,
     *  and cat?.Z directories.
     */
    while ((loc = next_dir(&manpath)) != NULL) {
	if (chdir((char *)loc) < 0) {
	    perror((char *)loc);
	    continue;
	}

	/* 
	 *  assume section number followed by possibly one alpha character 
	 *  for subdirectory name         
	 */
	(void) strcpy((char *)man, "man");
	(void) strcpy((char *)Zman, "man");
	(void) strcpy((char *)cat, "cat");
	umask(0);

#ifdef DEBUG
	setbuf(stdout, NULL);
#endif

	for (sp = sections, i = 3; *sp; sp++, i++) {
#ifdef    TRACE
	    printf("sp is %s\n", sp);
#endif    TRACE

	    /*
	     * A list of duplicates is built up for each section.  This can
	     * be realloc'd in the function format().
	     */
    	    if ((dup_list = (int *)calloc(MAX_DIRS,sizeof(int))) == NULL)
	        perror("calloc");

#ifdef DEBUG
	    printf("+ 0x%x, dup_list(main)\n",(unsigned int)dup_list);
#endif
	    num_dups = 0;
	    Zman[i] = man[i] = cat[i] = *sp;
	    if (isalpha(*++sp)) {
		--sp;
		continue;
	    }
	    --sp;
	    ++i;
	    if (zflag)
		csp = &cat[i];
	    else {
		cat[i] = '\0';
		(void) strcat((char *)cat, ".Z");
		i += 2;
		csp = &cat[i];
		i -= 2;
	    }
	    man[i] = '/';
	    Zman[i] = '\0';
	    (void) strcat((char *)Zman, ".Z/");
	    msp = &man[++i];
	    i += 2;
	    Zmsp = &Zman[i];
	    i = 2;
	    *Zmsp = *msp = *csp = '\0';
	    if (stat((char *)cat, &sbuf) < 0) {
		sprintf((char *)buf, "mkdir %s", cat);
		SYSTEM((char *)buf);
		stat((char *)cat, &sbuf);
	    }
	    if ((sbuf.st_mode & 0777) != 0777)
		chmod((char *)cat, 0777);
	    *csp = '/';
	    *++csp = '\0';
	    entries = ReadDir(man);	/* returns a pointer to */
					/* a sorted list of */
	    Zentries = ReadDir(Zman);	/* the dir. contents */

	    findex = Zfindex = 0;
	    while ( (entries && entries[findex] != NULL) ||
		    (Zentries && (Zentries[Zfindex] != NULL)) ) {
#ifdef    TRACE
		printf("  order = %d, findex = %d, Zfindex = %d\n", order, findex, Zfindex);
		printf("\tentries[%d] = %s\n\tZentries[%d] = %s\n",
			findex, entries[findex], Zfindex, Zentries[Zfindex]);
#endif    TRACE

		/*  
		 *  the stat calls et.al. down the way need pathname 
		 *  relative to current directory 
		 */
		(void) strcat((char *)man, (char *)entries[findex]);
		(void) strcat((char *)Zman, (char *)Zentries[Zfindex]);

#ifdef TRACE
		printf("\tman = %s\tZman = %s\n", man, Zman);
#endif
		order = strcmp((char *)entries[findex],
			       (char *)Zentries[Zfindex]);
		if (order == 0) {
		    /* 
		     ** entries exist in both dirs
		     */
		    stat((char *)entries[findex], &sbuf);
		    stat((char *)Zentries[Zfindex], &Zsbuf);
		    if (sbuf.st_mtime >= Zsbuf.st_mtime) {
#ifdef    TRACE
			printf("    using findex\n");
#endif    TRACE
			(void) strcat((char *)cat, (char *)entries[findex]);
			Zflag = 0;
			format(man, cat, Zflag);
			*msp = *csp = *Zmsp = '\0';
		    }
		    else {
#ifdef    TRACE
			printf("    using Zfindex\n");
#endif    TRACE
			(void) strcat((char *)cat, (char *)Zentries[Zfindex]);
			Zflag = 1;
			format(Zman, cat, Zflag);
			*Zmsp = *csp = *msp = '\0';
		    }
		    findex++;
		    Zfindex++;
		    continue;
		}
		if (((order < 0) || (Zentries[Zfindex] == NULL)) &&
			(entries[findex] != NULL)) {
		    /* 
		     ** non-compressed name is lexicographically first
		     ** or compressed files are all processed 
		     */
#ifdef    TRACE
		    printf("using findex\n");
#endif    TRACE
		    (void) strcat((char *)cat, (char *)entries[findex]);
		    Zflag = 0;
		    format(man, cat, Zflag);
		    *msp = *csp = *Zmsp = '\0';
		    findex++;
		    continue;
		}
		if (((order > 0) || (entries[findex] == NULL)) &&
			(Zentries[Zfindex] != NULL)) {
		    /* 
		     ** compressed name is lexicographically first
		     ** or non-compressed files have all been processed
		     */
#ifdef    TRACE
		    printf("using Zfindex\n");
		    printf("******  cat = %s\n", cat);
#endif    TRACE
		    (void) strcat((char *)cat, (char *)Zentries[Zfindex]);
		    Zflag = 1;
		    format(Zman, cat, Zflag);
		    *Zmsp = *csp = *msp = '\0';
		    Zfindex++;
		    continue;
		}
	    }

#ifdef OLDDEBUG
	    list_arena();
#endif

#ifdef DEBUG
	    printf("- 0x%x, Zentries(main)\n",(unsigned int)Zentries);
#endif
	    free((void *)Zentries);
#ifdef DEBUG
	    printf("- 0x%x, entries(main)\n",(unsigned int)entries);
#endif
	    free((void *)entries);
	}	/* end for each section */

	/*
	 * Let space go, start over for next section number.
	 */
	free((void *)dup_list);
    }		/* end while loc = next_dir (directories in MANPATH) */

    if (changed && !nflag) {
    whatis:
	if (pflag)
	    printf("/bin/sh /usr/lib/mkwhatis%s\n",mflag ? " -m" : "");
	else {
	    /*
	     *  mflag set means merge with old /usr/lib/whatis
	     */
	    if(mflag)
	        execl("/bin/sh", "/bin/sh", "/usr/lib/mkwhatis", "-m", 0);
	    else
	        execl("/bin/sh", "/bin/sh", "/usr/lib/mkwhatis", 0);
	    perror("/bin/sh /usr/lib/mkwhatis");
	    exstat = 1;
	}
    }

    exit(exstat);
#ifdef lint
    return(exstat);
#endif
}

/*
**    compar()    --    quicksort comparison routine
*/
compar(e1, e2)
unsigned char **e1, **e2;
{
    return strcmp((char *)*e1, (char *)*e2);
}

/*
**    ReadDir()    --    read and format a directory entry
**
** Parameters
**    input    --    directory to read
**    output    --    list of file names
**    effects    --    returns pointer to malloc()ed data!
*/
unsigned char **
ReadDir(dir)
unsigned char *dir;		/* absolute path to directory ... */
{
    DIR    *dirp;
    struct stat     stb;
    struct dirent  *dp;
    reg unsigned char  *tsp;
    unsigned char  *dsp;
    int     nDirs = 0;
    int     times = 1;		/* how many times dirList has been realloc'd */

    /* 
     **    returns list of files in "dir" sorted by name ...
     */
    dsp = dir + (int)strlen((char *)dir);
#ifdef    TRACE
    printf("  ReadDir(%s)\n", dir);
#endif    TRACE

    /* 
     *  Get space for the list of entries 
     */
    if ((dirList = (unsigned char **)calloc(MAX_DIRS, sizeof(unsigned char *))) == NULL)
	perror("calloc");

#ifdef DEBUG
    printf("+ 0x%x, dirList(ReadDir1)\n",(unsigned int)dirList);
#endif
    

    /*
     *  Can't get into directory, return pointer to NULL.
     */
    if ((dirp = opendir((char *)dir)) == NULL) 
	return((unsigned char **)NULL);

    for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
	/* 
	 * make sure this is a man file, i.e., that it
	 * ends in .[0-9] or .[0-9][a-z]
	 * .[0-9lnp] [a-z] for s500
	 * .[0-9] {a-z} for spectrum
	 */
	tsp = (unsigned char *)strrchr(dp->d_name, '.');
#ifdef    TRACE
	printf("      name = %s (0x%x), tsp = 0x%x\n", dp->d_name, dp->d_name, tsp);
#endif    TRACE
	if (tsp == NULL)
	    continue;
	if (!isdigit(*++tsp) || ((*++tsp && !isalpha(*tsp)) || *++tsp))
	    continue;
	(void) strcat((char *)dir, dp->d_name);

	if (stat((char *)dir, &stb)) {
	    perror((char *)dir);
	    *dsp = '\0';
	    continue;		/* keep trying ...    */
	}
	*dsp = '\0';
#ifdef    TRACE
	printf("\t  adding name as %d\n", nDirs);
#endif    TRACE
	if ((dirList[nDirs] = (unsigned char *)malloc((size_t)dp->d_namlen + 1)) ==
	    (unsigned char *)NULL) {
	    perror("malloc");
	}
	else {
#	    ifdef DEBUG
		printf("+ 0x%x, dirList[%d](ReadDir)\n",
		       (unsigned int)dirList[nDirs], nDirs);
#	    endif
	    (void) strcpy((char *)dirList[nDirs++], dp->d_name);
	}


	if (nDirs >= (MAX_DIRS * times) - 1) {
	    times++;
	    if ((dirList = (unsigned char **)
			   realloc((void *)dirList, MAX_DIRS * times * sizeof (unsigned char *))) == NULL)
		perror("realloc");
	}

#ifdef DEBUG
    printf("+ 0x%x, dirList(ReadDir2)\n", (unsigned int)dirList);
#endif
    
    }
    closedir(dirp);
    /* 
     **    now either return a NULL list, or malloc space for what we
     **    just read into dirList, trasnfer into av, and then qsort.
     */
    if (nDirs == 0)
	return((unsigned char **)NULL);

    dirList[nDirs] = NULL;

    /* 
     **    return entries in sorted order, to determine test order ...
     */
    (void)qsort((void *)dirList, (unsigned)nDirs, sizeof (*dirList), compar);
#ifdef    TRACE
    for (i = 0; i < nDirs; i++)
	printf("\t  %d\t%s\n", i, dirList[i]);
#endif    TRACE

    return dirList;
}
/*  
**    format()     --     formats pages and installs them into cat
*/
format(manfile, catfile, Flag)
unsigned char  *manfile, *catfile;
unsigned char   Flag;
{
    reg     FILE *inf;
    extern int  errno;
    int     i, num_found = 0;
    int     times = 1;		/* number of times space is alloc'd for dup_list */
    unsigned char  *cutoff;
    unsigned char   catdir[MAXNAMLEN], catlink[MAXNAMLEN], mandir[MAXNAMLEN];
    DIR    *dirp;
    struct dirent  *entry;

#ifdef    TRACE
    printf("\n  format(%s, %s, %d)\n", manfile, catfile, Flag);
#endif    TRACE

    if ((inf = fopen((char *)manfile, "r")) == NULL) {
	if (errno != ENOENT) {
	    perror((char *)manfile);
	}
	exstat = 1;
	return;
    }

    /* These lines for backward compatibilty */
    if (getc(inf) == '.' && getc(inf) == 's'
	    && getc(inf) == 'o') {
	fclose(inf);
	return;
    }
    fclose(inf);

    if (stat((char *)manfile, &sbuf) < 0) { /* can't get to source file */
	perror((char *)manfile);
	return;
    }


    /* 
     *  first, see if source file has multiple links to it 
     */
    if (sbuf.st_nlink > 1) {
	/*  
	 * then, check to see if this one has been found already 
	 */
	for (i = 0; i < num_dups; i++) {
	    if (dup_list[i] == sbuf.st_ino) {
#ifdef TRACE
		printf("\tInode found, returning from format()\n");
#endif
		return;
	    }
	}

	/* Not found, add to list */
	dup_list[num_dups++] = sbuf.st_ino; /*  add this inode # to list */

	if (num_dups >= MAX_DIRS * times) {
	    times++;
	    if ((dup_list = (int *)realloc((void *)dup_list, MAX_DIRS * times * sizeof (int))) == NULL)
		perror("realloc");
#	    ifdef DEBUG
		printf("+ 0x%x, dup_list(format)\n", (unsigned int)dup_list);
#	    endif
	}

    

	/* 
	 *  see if cat entry already exists & how old it is 
	 */
	if ((stat((char *)catfile, &csbuf) < 0) || (csbuf.st_mtime < sbuf.st_mtime)) {
#ifdef TRACE
	    printf("\tmtime of manfile = %ld\n", sbuf.st_mtime);
	    printf("\tmtime of catfile = %ld\n", csbuf.st_mtime);
#endif
	    /* Create initial entry in cat directory */
	    create_entry(manfile, catfile, Flag);
	}

	/*  We'll go ahead from here and create or recreate links 
	 *  without checking whether they're present; they might 
	 *  have changed anyway.  If user has created links, they 
	 *  will be unaffected. 
	 */

	/* 
	 * Make some temp strings to work with. Must use pathname 
	 * relative to current directory 
	 */
	(void) strcpy((char *)catdir, (char *)catfile);/*  Will hold dir, i.e. "cat1.Z/" */
	(void) strcpy((char *)mandir, (char *)manfile);
	cutoff = (unsigned char *)strrchr((char *)catdir, '/');
	*(++cutoff) = '\0';
	cutoff = (unsigned char *)strrchr((char *)mandir, '/');
	*(++cutoff) = '\0';

#ifdef TRACE
	printf("\tStarting to search for other links\n");
	printf("\t\tcatdir = %s\t\n", catdir);
#endif
	/* Find all other files in source dir with same inode no. */
	if ((dirp = opendir((char *)mandir)) == (DIR *)NULL) {
	    perror((char *)mandir);
	    return;
	}
	while (((entry = readdir(dirp)) != (struct dirent  *)NULL) &&
		(num_found < sbuf.st_nlink)) {
	    if (entry->d_ino == sbuf.st_ino) {
		++num_found;
		(void) strcpy((char *)catlink, (char *)catdir);
		(void) strcat((char *)catlink,
			      (char *)(entry->d_name));/* create name to link */
#ifdef   TRACE
		printf("\tCreating new entry, %s linked to %s\n", catlink, catfile);
#endif   TRACE
		if(pflag)
		    printf("link %s %s\n",catfile, catlink);
		else
		    link((char *)catfile, (char *)catlink);
	    }
	}
	closedir(dirp);
    }
    else {			/* Source does not have links */
#ifdef TRACE
	printf("\tSource %s had no links, creating %s\n", manfile, catfile);
#endif

	/* If catfile exists and is as new as manfile, quit */
	if ((stat((char *)catfile, &csbuf) >= 0) && (csbuf.st_mtime > sbuf.st_mtime)) {
#ifdef TRACE
	    printf("\tmtime of manfile = %ld\n", sbuf.st_mtime);
	    printf("\tmtime of catfile = %ld\n", csbuf.st_mtime);
#endif
	    return;
	}

	/* Otherwise remove catfile & recreate it */
	unlink((char *)catfile);
	create_entry(manfile, catfile, Flag);
    }
}

/*  
**  Hand off to this routine to do actual formatting according to
**  flags
*/
int     create_entry(manfile, catfile, Flag)
unsigned char *manfile, *catfile;
unsigned char   Flag;
{
#ifdef TRACE
    printf("\n\tIn create_flag()\n\tmanfile = %s, catfile = %s, Flag = %d\n",
	    manfile, catfile, Flag);
#endif
    if (zflag)
	if (!Flag)
	    sprintf((char *)buf, "tbl -TX %s | nroff -man | col > %s",
		    manfile, catfile);
	else
	    sprintf((char *)buf, "uncompress < %s | tbl -TX | nroff -man | col > %s",
		    manfile, catfile);
    else
    if (!Flag)
	sprintf((char *)buf, "tbl -TX %s | nroff -man | col | compress > %s",
		manfile, catfile);
    else
	sprintf((char *)buf, "uncompress < %s | tbl -TX | nroff -man | col | compress > %s", manfile, catfile);
#ifdef    TRACE
    printf("\tbuf = %s\n", buf);
#endif    TRACE
    SYSTEM((char *)buf);
    changed = 1;
}
