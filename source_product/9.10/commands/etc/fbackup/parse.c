/* @(#) $Revision: 72.4 $ */

/***************************************************************************
****************************************************************************

	parse.c

    This file contains functions which parse the input string, graph file(s),
    and a confguration file.

****************************************************************************
***************************************************************************/

#include "head.h"
#include <fcntl.h>
#include <unistd.h>

#ifdef NLS
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#define FIRST "fbackup(1"
#endif NLS

/*char firstpart[10];sprintf(firstpart, "fbackup(%d", NL_SETN);*/

extern char	chgvol_file[],
		err_file[];

extern struct fn_list *temp;
extern struct fn_list *outfile_head;

int level = LEVEL_DFLT,
	    lastlev;

#ifdef AUDIT
int glob_level = LEVEL_DFLT;
#endif

#ifdef ACLS
int aclflag = TRUE;			/* assume ACLS will be backed up */
#endif

long	lastbtime = 0,
	lastetime = 0;

extern int	n_outfiles,
		nrdrs,
		blksperrec,
		nrecs,
		maxretries,
		retrylim,
		ckptfreq,
                fsmfreq,
		maxvoluses,
		yflag,
		uflag,
		vflag,
		Rflag,
		Zflag,
		Hflag,
                sflag,
                nflag;

char	*configfile,
	*restartfile,
	*graphfile = (char*)NULL,
	*volhdrfile = (char*)NULL,
	*dbfile = (char*)NULL,
	*tmpdbfile = (char*)NULL,
	*indexfile = (char*)NULL;







/***************************************************************************
    This function parses the command line.
***************************************************************************/
void
parse_cmdline(argc, argv)
int argc;
char **argv;
{
    extern char	*optarg;
    extern int optind;
    int	c, levelcnt=0, icnt=0, ecnt=0, gcnt=0, errflg=FALSE;
    struct fn_list *t;
    struct fn_list *tail;
    struct fn_list *head;
    int n;

    t = (struct fn_list *)NULL;
    head = (struct fn_list *)NULL;
    tail = (struct fn_list *)NULL;
    

#ifdef ACLS
    while ((c = getopt(argc, argv, "0123456789ui:e:g:d:f:I:V:c:snvyR:AHZ")) != EOF) {
#else	
    while ((c = getopt(argc, argv, "0123456789ui:e:g:d:f:I:V:c:snvyR:HZ")) != EOF) {
#endif /* ACLS */
	switch ((char)c) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	    level = (char)c - '0';
#ifdef AUDIT
	    glob_level = level;
#endif
	    levelcnt++;
	    break;
	case 'u': uflag = TRUE;
	    break;
	case 'i':
	    if (add_inex(optarg, 'i'))
		icnt++;
	    break;
	case 'e':
	    if (add_inex(optarg, 'e'))
		ecnt++;
	    break;
	case 'g':
	    graphfile = optarg;
	    if (parse_graphfile()) {
	      gcnt++;
	    }
	    else {  /* defective graph file exit rather than proceed */
	      errflg = TRUE;
	    }
	    break;
	case 'f':
	    t = (struct fn_list *)malloc(sizeof(struct fn_list));
	    strcpy(t->name, optarg);
	    t->next = (struct fn_list *)NULL;
	    if (head == NULL) {
	      head = t;
	      tail = t;
	    }
	    else {
	      tail->next = t;
	      tail = t;
	    }
	    n_outfiles++;
	    break;
	case 'd':
	    dbfile = optarg;
	    break;
	case 'I':
	    indexfile = optarg;
	    break;
	case 'V':
	    volhdrfile = optarg;
	    break;
	case 'c':
	    configfile = optarg;
	    break;
	case 's': sflag = TRUE;
	    break;
	case 'n': nflag = TRUE;
	    break;
	case 'v': vflag = TRUE;
	    break;
	case 'y': yflag = TRUE;
	    break;
	case 'R': Rflag = TRUE;
	    restartfile = optarg;
	    break;
	case 'H': Hflag = TRUE;
	    break;
	case 'Z': Zflag = TRUE;
		break;
#ifdef ACLS
	case 'A': aclflag = FALSE;
	        break;
#endif /* ACLS */
	case '?':
	    errflg = TRUE;
	    break;
	}
    }
    for	(; optind < argc; optind++)
	msg((catgets(nlmsg_fd,NL_SETN,401, "fbackup(1401): extra argument: %s ignored\n")), argv[optind]);

    if (levelcnt > 1) {
	msg((catgets(nlmsg_fd,NL_SETN,402, "fbackup(1402): please do not specify more than one fbackup level\n")));
	errflg = TRUE;
    }

/* if no -d option use the default names for dates and tmp dates files */

    if ( strlen(dbfile) != 0 ) {
      tmpdbfile = "/tmp/";
      strcat(tmpdbfile, mktemp("fbackdbXXXXXX"));
    }
    else {
      dbfile = DATESFILE;
      tmpdbfile = DATESTMP;
    }

    if (n_outfiles == 0) {
#ifdef	NLS
	msg((catgets(nlmsg_fd,NL_SETN,403, strcat(FIRST, "403): at least one output file must be specified\n"))));
#else	NLS
	msg((catgets(nlmsg_fd,NL_SETN,403, "fbackup(1403): at least one output file must be specified\n")));
#endif	NLS
	errflg = TRUE;
    }
    else {
      outfile_head = (struct fn_list *)NULL;
      if ((n = expand_fn(head)) < 0) {
#ifdef DEBUG
fprintf(stderr, "parse(): expand_fn error n = %d\n", n);
#endif
	errflg = TRUE;
	if (n == -3) {
	  msg((catgets(nlmsg_fd,NL_SETN,429, "fbackup(1429): number of output files cannot be more than %d\n"), MAXOUTFILES));
	}
	else {
	  msg((catgets(nlmsg_fd,NL_SETN,428, "fbackup(1428): error in expanding output file pattern\n")));
	}
      }
#ifdef DEBUG
fprintf(stderr, "parse(): expand_fn outfile_head name = %s\n", outfile_head->name);
#endif
    }

    if ((gcnt == 0) && (icnt == 0) && !Rflag) {
	msg((catgets(nlmsg_fd,NL_SETN,404, "fbackup(1404): no files have been specified; (use at least one -i and/or -g option)\n")));
	errflg = TRUE;
    }

    if (Rflag && (levelcnt || gcnt || icnt || ecnt)) {
	msg((catgets(nlmsg_fd,NL_SETN,405, "fbackup(1405): none of the options [0-9ieg] may be used with -R\n")));
	errflg = TRUE;
    }

    if (errflg)	{

#ifdef V4FS
	msg((catgets(nlmsg_fd,NL_SETN,430, "usage: /usr/sbin/fbackup -f device [-f device] ... [-0-9] [-nsuvyH]\n")));
#else /* V4FS */
	msg((catgets(nlmsg_fd,NL_SETN,406, "usage: /etc/fbackup -f device [-f device] ... [-0-9] [-nsuvyH]\n")));
#endif /* V4FS */

	msg((catgets(nlmsg_fd,NL_SETN,407, "	[-i path] ... [-e path] ... [-g graph] ... [-d path] [-I path] [-V path] [-c config]\n")));


#ifdef V4FS
	msg((catgets(nlmsg_fd,NL_SETN,408, "\nusage: /usr/sbin/fbackup -f device [-f device] ... -R restart [-nsuvyH]\n")));
#else	/* V4FS */
	msg((catgets(nlmsg_fd,NL_SETN,408, "\nusage: /etc/fbackup -f device [-f device] ... -R restart [-nsuvyH]\n")));
#endif /* V4FS */

	msg((catgets(nlmsg_fd,NL_SETN,409, "	[-d path] [-I path] [-V path] [-c config]\n\n")));
	(void) exit(ERROR_EXIT);
    }

    if ((gcnt == 1) && !Rflag)
	getdates();

    if (uflag && ((gcnt != 1) || (icnt != 0) || (ecnt != 0)) && !Rflag) {
	msg((catgets(nlmsg_fd,NL_SETN,410, "fbackup(1410): for the update (-u) to take place, exactly one graph (-g) file\n")));
	msg((catgets(nlmsg_fd,NL_SETN,411, "	and no include or exclude files (-i, -e) may be specified\n")));
	uflag = FALSE;
    }
}






#define MAXGRAPHLINE (MAXPATHLEN+32)

/***************************************************************************
    This function reads graph files, extracts legal lines (beginning with
    "i" or "e" followed by a path name), and adds them to the in/exclude
    table.  This is done in exactly the same manner as if these path names
    had been supplied on the command line with -i or -e options.  If the graph
    file cannot be opened, this function returns FALSE, otherwise TRUE.
***************************************************************************/
int
parse_graphfile()
{
    FILE *graphfp;
    char line[MAXGRAPHLINE], ch, path[MAXPATHLEN];
    int n;

    if ((graphfp = fopen(graphfile, "r")) == NULL) {
	msg((catgets(nlmsg_fd,NL_SETN,412, "fbackup(1412): unable to open graph file %s\n")), graphfile);
	return(FALSE);
    }
    while (fgets(line, MAXGRAPHLINE, graphfp) != NULL) {
	/* let's skip empty lines before handling */
	if( strspn(line," \t\n") == strlen(line) ) continue;
	n = sscanf(line, "%c%s\n", &ch, path);
	if ((n == 2) && ((ch == 'i') || (ch == 'e')))
	    (void) add_inex(path, ch);
	else {
	  msg((catgets(nlmsg_fd,NL_SETN,427, "fbackup(1427): illegal entry in graph file %s\n")), graphfile);
	  return(FALSE);
	}
      }
    (void) fclose(graphfp);
    return(TRUE);
}






#define MAXCONFLINE (MAXPATHLEN+32)

/***************************************************************************
    This function parses configuration files.  (There is presently no code
    to prevent the user from specifying multiple configuration files, however,
    there is normally no particular advantage in doing this.)
    It looks for lines of the form:
			label:value
    When such lines are found, they are incorporated into the session.
***************************************************************************/
parse_config()
{
    FILE *configfp;
    char line[MAXCONFLINE], label[32], value[MAXPATHLEN];
    int n, cnt = 0;

    if (!*configfile)
	return;
    if ((configfp = fopen(configfile, "r")) == NULL) {
	msg((catgets(nlmsg_fd,NL_SETN,413, "fbackup(1413): unable to open configuration file %s\n")), configfile);
	return;
    }
    while (fgets(line, MAXCONFLINE, configfp) != NULL) {
	cnt++;
	n = sscanf(line, "%s%s\n", label, value);
	switch (n) {
	case -1:
	    break;
	case 2: if (!strcmp(label, "readerprocesses"))
		nrdrs = atoi(value);
	    else if (!strcmp(label, "blocksperrecord"))
		blksperrec = atoi(value);
	    else if (!strcmp(label, "records"))
		nrecs = atoi(value);
	    else if (!strcmp(label, "maxretries"))
		maxretries = atoi(value);
	    else if (!strcmp(label, "retrylimit"))
		retrylim = atoi(value);
	    else if (!strcmp(label, "checkpointfreq"))
		ckptfreq = atoi(value);
	    else if (!strcmp(label, "filesperfsm"))
		fsmfreq = atoi(value);
	    else if (!strcmp(label, "maxvoluses"))
		maxvoluses = atoi(value);
	    else if (!strcmp(label, "chgvol"))
		(void) strcpy(chgvol_file, value);
	    else if (!strcmp(label, "error"))
		(void) strcpy(err_file, value);
	    else msg((catgets(nlmsg_fd,NL_SETN,414, "fbackup(1414): illegal configuration file label (%s) ignored\n")), label);
	    break;
	default: msg((catgets(nlmsg_fd,NL_SETN,415, "fbackup(1415): configuration file error at line %d\n")), cnt);
	    msg((catgets(nlmsg_fd,NL_SETN,416, "bad line:\n%s\n")), line);
	    break;
	}
    }
    (void) fclose(configfp);
}






#define DATESFMT "%s %d %d %d\n"
#define LINELEN 132

/***************************************************************************

  Extention for 8.0 add -d option so that user can specify dates database
  file.  If no -d use default hard coded names. The new files are in
  dbfile and tmpdbfile.  (Dan Matheson, CSB R&D).

    This function is called when exactly one -g argument is specified, and
    no -i and no -e arguments.  It determines when the last appropriate backup
    was started, and when it ended (lastbtime and lastetime).  If the current
    backup is a full one (level = 0), getdates assigns the beginning of time
    (0) to both of these values.  If this one is an incremental backup
    dbfile is searched to find the most recent backup of the same graph
    file ("-g"'s argument), which was done at a lower backup levels.  If no
    appropriate entry is found, again, the beginning of time is assumed.
    (Eg, if the current level is 4, the dbfile is searched for the most
    recent backup of this graph with levels in the range of 0 to 3.)
    dbfile is locked during while it is open for reading to prevent it
    from changing while we're reading it.
***************************************************************************/
getdates()
{
    FILE *datesfp;
    char graph[80], line[LINELEN];
    int datesfd, lev;
    long btime, etime;

    if (((datesfd=open(dbfile, O_RDONLY|O_CREAT, PROTECT)) < 0) ||
				    ((datesfp=fdopen(datesfd, "r")) == NULL)) {
	msg((catgets(nlmsg_fd,NL_SETN,417, "fbackup(1417): cannot open the dates file %s for reading\n")), dbfile);
	lastbtime = lastetime = 0;
	return;
    }

    (void) lockf(datesfd, F_LOCK, 0);

    while (fgets(line, LINELEN, datesfp) != NULL) {
	if ((sscanf(line, DATESFMT, graph, &lev, &btime, &etime) == 4) &&
				    (!strcmp(graph, graphfile) &&
				    (lev < level) && (btime > lastbtime))) {
	    lastbtime = btime;
	    lastetime = etime;
	    lastlev = lev;
	}
    }

    if (vflag) {
	if (lastbtime != 0) {
	    msg((catgets(nlmsg_fd,NL_SETN,418, "fbackup(1418): the last level %d session for graph %s was\n")), lastlev, graphfile);
	    msg((catgets(nlmsg_fd,NL_SETN,419, "	started  : %s\n")), myctime(&lastbtime));
	    msg((catgets(nlmsg_fd,NL_SETN,420, "	finished : %s\n")), myctime(&lastetime));
	} else {
	    msg((catgets(nlmsg_fd,NL_SETN,421, "fbackup(1421): no history is available for graph file %s ")), graphfile);
	    msg((catgets(nlmsg_fd,NL_SETN,422, "(below level %d)\n")), level);
	}
    }
    if (level == 0)
	lastbtime = lastetime = 0;

    (void) fclose(datesfp);
    (void) close(datesfd);
}






extern PADTYPE *pad;

/***************************************************************************
    This function updates dbfile, where the record of past sessions is
    kept.  (This function is called if the -u option has been set, and it
    has not been subsequently un-set by an illegal combination of other
    options, see the man page.)
	It first opens dbfile and locks it.  It then creates a temporary
    file, tmpdbfile.  It reads through dbfile and copies every entry which
    does NOT corresponds to the current graph file and level, and copies it
    to tmpdbfile.  If there is no such corresponding entry, it is added to
    tmpdbfile; if there is such an entry, it is replaced with the new
    data (ie, the start and end times) for this session.
	After all this, tmpdbfile contains the information to build a new
    dates file, so it the new data is copied back to dbfile.  tmpdbfile
    is then closed and unlinked, and dbfile is also closed (unlocking it).
***************************************************************************/
putdates(endtime)
long endtime;
{
    FILE *datesfp, *tmpfp;
    char graph[80], line[LINELEN];
    long btime, etime;
    int datesfd, lev, found=FALSE;

    if (!uflag)
	return;

    if (((datesfd = open(dbfile, O_RDWR|O_CREAT, PROTECT)) < 0) ||
				    ((datesfp=fdopen(datesfd, "r+")) == NULL)) {
	msg((catgets(nlmsg_fd,NL_SETN,423, "fbackup(1423): could not open the dates file %s for writing\n")), dbfile);
	return;
    }

    (void) lockf(datesfd, F_LOCK, 0);

    if ((tmpfp=fopen(tmpdbfile, "w+")) == NULL) {
	msg((catgets(nlmsg_fd,NL_SETN,424, "fbackup(1424): unable to open the temporary dates file %s\n")), tmpdbfile);
	return;
    }

			    /* update the dates file in the temporary file */
    while (fgets(line, LINELEN, datesfp) != NULL) {
	if (sscanf(line, DATESFMT, graph, &lev, &btime, &etime) == 4)
	    if (!strcmp(graph, graphfile) && (lev == level)) {
		(void) fprintf(tmpfp, DATESFMT, graphfile, level, pad->begtime, endtime);
		found = TRUE;
	    } else
		(void) fprintf(tmpfp, DATESFMT, graph, lev, btime, etime);
    }
    if (!found)
	(void) fprintf(tmpfp, DATESFMT, graphfile, level, pad->begtime, endtime);

    rewind(datesfp);			/* copy the temporary file back */
    (void) ftruncate(datesfd, 0);
    rewind(tmpfp);
    while (fgets(line, LINELEN, tmpfp) != NULL) {
	if (sscanf(line, DATESFMT, graph, &lev, &btime, &etime) == 4) {
	    (void) fprintf(datesfp, DATESFMT, graph, lev, btime, etime);
	    (void) fprintf(datesfp, (catgets(nlmsg_fd,NL_SETN,425, "    STARTED: %s")), myctime(&btime));
	    (void) fprintf(datesfp, (catgets(nlmsg_fd,NL_SETN,426, "    ENDED: %s\n")), myctime(&etime));
	}
    }
    (void) fclose(tmpfp);
    (void) unlink(tmpdbfile);
    (void) fclose(datesfp);
    (void) close(datesfd);
}  /* end putdates() */


int expand_fn(tmp)
     struct fn_list *tmp;
{
  struct fn_list *t;
  struct fn_list *r;
  struct fn_list *tail;  
  char dirname[MAXPATHLEN+1];
  char fn[MAXPATHLEN+1];
  struct fn_list *res;
  int match;

  DIR *dirp;
  struct dirent *dp;
  

  /* loop through the tmp list expanding each name and
     puuting the result in the res list.
  */
  
  t = tmp;
  res = (struct fn_list *)NULL;
  tail = (struct fn_list *)NULL;
  
  while (t != (struct fn_list *)NULL) {

#ifdef DEBUG
fprintf(stderr, "expand_fn pattern: %s\n", t->name);
#endif
    if (((strcmp(t->name, "-")) == 0) || ((strchr(t->name, ':')) != NULL)) {  
      /*  stdin or stdout, entry done add to end of res list */
      /* or have a remote name, entry done add to end of res list */
      if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	return(-1);
      }
      strcpy(r->name, t->name);
      r->next = (struct fn_list *)NULL;
      if (res == (struct fn_list *)NULL) {
	res = r;
	tail = r;
	tail->next = res;
      }
      else {
	tail->next = r;
	tail = r;
	tail->next = res;
      }
      t = t->next;
      continue;
    }  /* end if (- || :) */
    
    if (((strchr(t->name, '/')) == NULL)) {
      /* check cwd for match */
      if ((dirp = opendir(".")) == NULL) {
	return(-2);
      }
      while ((dp = readdir(dirp)) != NULL) {
	match = 0;
	if ((fnmatch(t->name, dp->d_name, 0)) == 0) {
	  if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	    return(-1);
	  }
	  match++;
	  strcpy(r->name, dp->d_name);
	  r->next = (struct fn_list *)NULL;
	  if (res == (struct fn_list *)NULL) {
	    res = r;
	    tail = r;
	    tail->next = res;
	  }
	  else {
	    tail->next = r;
	    tail = r;
	    tail->next = res;
	  }
	}
      }
      if (match == 0) { 
	/* probable output to file that does not yet exist */
	if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	  return(-1);
	}
#ifdef DEBUG
fprintf(stderr, "expand_fn: zero match: %s\n", t->name);
#endif
	strcpy(r->name, t->name);
	r->next = (struct fn_list *)NULL;
	if (res == (struct fn_list *)NULL) {
	  res = r;
	  tail = r;
	  tail->next = res;
	}
	else {
	  tail->next = r;
	  tail = r;
	  tail->next = res;
	}
      }
      if ((closedir(dirp)) == -1) {
	return(-2);
      }
      t = t->next;
    } /* end if cwd */
    else { 
      /* get directory prefix and look */
      strcpy(dirname, t->name);
      strcpy(fn, strrchr(t->name,'/')+1);
      dirname[strlen(dirname) - strlen(fn) -1] = '\0';

#ifdef DEBUG
fprintf(stderr, "expand_fn: dirname: %s\n", dirname);
fprintf(stderr, "expand_fn: fn: %s\n", fn);
#endif
      
      /* But it doesn't work if t->name looks like /xxxxx , so */
      if ((dirp = opendir((dirname[0]=='\0') ? "/" : dirname)) == NULL) {
#ifdef DEBUG
perror("expand_fn: opendir error");
#endif
	return(-2);
      }
      match = 0;
      while ((dp = readdir(dirp)) != NULL) {
	if ((fnmatch(fn, dp->d_name, 0)) == 0) {
	  if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	    return(-1);
	  }
	  match++;
	  strcpy(r->name, dirname);
	  strcat(r->name, "/");
	  strcat(r->name, dp->d_name);
	  r->next = (struct fn_list *)NULL;
	  if (res == (struct fn_list *)NULL) {
	    res = r;
	    tail = r;
	    tail->next = res;
	  }
	  else {
	    tail->next = r;
	    tail = r;
	    tail->next = res;
	  }
	}
      }
      if (match == 0) { 
	/* probable output to file that does not yet exist */
	if ((r = malloc((unsigned) sizeof(struct fn_list))) == NULL) {
	  return(-1);
	}
	strcpy(r->name, t->name);
	r->next = (struct fn_list *)NULL;
	if (res == (struct fn_list *)NULL) {
	  res = r;
	  tail = r;
	  tail->next = res;
	}
	else {
	  tail->next = r;
	  tail = r;
	  tail->next = res;
	}
      }
      if ((closedir(dirp)) == -1) {
	return(-2);
      }
      t = t->next;
    }  /* end else directory prefix */
  }  /* end while (t) */

  n_outfiles = 1;
  t = res;
#ifdef DEBUG
fprintf(stderr, "expand_fn: res output file: %s\n", res->name);
#endif
  while (t->next !=res) {
    n_outfiles++;
    t = t->next;
#ifdef DEBUG
fprintf(stderr, "expand_fn: output file: %s\n", t->name);
#endif
  }
  if (n_outfiles > MAXOUTFILES) {
    return(-3);
  }
  outfile_head = res;
  
  return(0);
  
}  /* end expand_fn() */

