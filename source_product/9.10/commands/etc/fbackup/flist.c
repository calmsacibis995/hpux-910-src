/* @(#) $Revision: 72.2 $ */

/***************************************************************************
****************************************************************************

	flist.c

    This file contains functions that perform operations of the flist, the
    linked list of files which is used both as the list of files to (attempt
    to) back up, and as the index.

****************************************************************************
***************************************************************************/

#include "head.h"

#ifdef NLS
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

void free();
FLISTNODE *hardflink();

				/* string between file & link in indices */
#define LINKTOSTR (catgets(nlmsg_fd,NL_SETN,701, " <LINK> "))

				/* string used when a file isn't backed up */
#define NOTONBACKUP "XXXX"	/* MUST be 4 characters long */

extern PADTYPE	*pad;

extern int idxsize,
	   nfiles,
	   nflag,
	   pipefd[2],
	   level;

extern long lastbtime, lastetime;

extern char *graphfile,
	    *indexfile,
	    *volhdrfile,
	    *restartfile;

int already_in_flist;
dtabentry *current_dirptr, *root_dirptr;
FLISTNODE *current_flistptr;

extern dtabentry *add_dentry();
extern char *flistpath();
extern char *new_malloc();
/*
 * macro which returns the length of the pathname in the given flist pointer
 */
#define mypathlen(p)	(((p)->dirptr ? (p)->dirptr->len : 0) + (p)->len)

static FLISTNODE *flisthead;
static FLISTNODE *flisttail = (FLISTNODE *) NULL;

/***************************************************************************
    This function is called to add files to the (tail end) of the linked list
    of files which comprise the file list kept in memory (flist).  Each file
    is exmined to see if has been modified or had its inode data change since
    the most recent previous backup (if one exists).  If this is the case, it
    is added to the flist.
    If the file to be added is not a directory and its link count > 1 (so it
    has multiple hard links to it).  Another list of hard links to non-
    directories is maintained, and the new file is checked against this list.
    One of two cases may occur:
    (1) This is the first one of these links that will be backed up, so it is
    added to the list of hard links to non-directories.
    (2) This is NOT the first one of these links that will be backed up, so
    the link pointer (in the flist node for this file) is set to point to the
    flist node which is the one that actually has the data associated with it.
    Add_flistnode returns a pointer to the new node in the flist if it is
    added, or NULL if it is not.
***************************************************************************/
FLISTNODE *
add_flistnode(path, statbuf)
char *path;
struct stat *statbuf;
{
    FLISTNODE *new, *linkptr;
    FLISTNODE *t1, *t2;
    int len;
    int inserted;
    int n;
    char *tmp;

    /*  If this file's modifcation time time is before the time when the
     *	last (appropriate) session BEGAN, AND the change i-node time is
     *	before the time that same session ENDED, do not add it to the list.
     *	This is a kludge which was necessitated by the fact that it isn't
     *	possible to reset st_ctime.  When this is possible, the following
     *	'if test' should be replaced by the line:
    if ((statbuf->st_mtime < lastbtime) && (statbuf->st_ctime < lastbtime))
     *								   *^*
     *	Note the difference, lastbtime vs lastetime.
    */

    inserted = 0;			/* not yet in the list */

    /* The reason why fbackup saved empty directories even if all files were up to date, commented in next line ... */
    if ((statbuf->st_mtime < lastbtime) && (statbuf->st_ctime < lastetime) /* && !(S_ISDIR(statbuf->st_mode)) */ )
	return(FLISTNODE*)NULL;

    if ((statbuf->st_mode & S_IFMT) == S_IFNWK) {
      msg((catgets(nlmsg_fd,NL_SETN,715, "fbackup(1715): Skipping obsolete Network Special File: %s\n")), path);
      return(FLISTNODE*)NULL;
    }

    if ((!nflag) && (statbuf->st_fstype == MOUNT_NFS)) {
      return(FLISTNODE*)NULL;
    }

    already_in_flist = 0;

    if (tmp = strrchr(path, '/'))
      len = strlen(++tmp);
    else
      len = strlen(tmp = path);

    new = (FLISTNODE*)new_malloc(sizeof(FLISTNODE) + len);
    if (new == (FLISTNODE*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,702, "fbackup(1702): out of virtual memory\n"));
    
    if (flisttail == (FLISTNODE *) NULL) {
	flisthead = new;
	flisttail = new;
	inserted = 1;
    }

    new->dirptr = current_dirptr;

    if (new->len = len) {
	(void)strcpy(&new->pathptr, tmp);
    } else {
	new->pathptr = '\0';
    }
    len = strlen(path)+1;
    idxsize += (FNAMEOFFSET + len);

    if (((statbuf->st_mode&S_IFMT) != S_IFDIR) && (statbuf->st_nlink > 1) &&
		    ((linkptr=hardflink(new, statbuf)) != (FLISTNODE*)NULL)) {
	new->linkptr = linkptr;
	idxsize += (strlen(LINKTOSTR) + FNAMEOFFSET + mypathlen(linkptr));
    } else
	new->linkptr = (FLISTNODE*)NULL;

    new->vol = FIRSTVOL;
    new->weirdflag = (char)FALSE;
    new->fwdlinkflag = (char)FALSE;
    new->ino = statbuf->st_ino;
    new->dev = statbuf->st_dev;
    new->dotdotptr = (FLISTNODE*)NULL;
    new->next = (FLISTNODE *) NULL;
    nfiles++;
    if(inserted)
    {
      return(new);
    }
    if((n = pathcmp(new, flisttail)) > 0) {	/* add at end */
	flisttail->next = new;
	flisttail = new;
	return(new);
    } else if(n == 0) {
	nfiles--; 
	already_in_flist = 1;
	return flisttail;
    }

/*
 * We set the current flist pointer in expanddir() and use it as the point
 * of comparison.  Since everything is sorted, the next file should always
 * come after the current one.  This is done to avoid traversing the list
 * from the head (ie. keep locality of reference near the end of the list).
 */
    if((n = pathcmp(new, current_flistptr)) < 0) {
    t1 = flisthead;
    t2 = flisthead->next;
    } else {
	t1 = current_flistptr;
	t2 = current_flistptr->next;
    }
    while(t2 != (FLISTNODE *) NULL) {
	if((n = pathcmp(new, t2)) < 0) {
	    t1->next = new;
	    new->next = t2;
	    return(new);
	} else if(n == 0) {
	    nfiles--; 
	    already_in_flist = 1;
	    return t2;
	}
	t1 = t2;
	t2 = t2->next;
    }
    nfiles--;
    return((FLISTNODE *)NULL);
}






/***************************************************************************
    This function is called when a "sibling" hard linkd to a directory is
    discovered.  (Both links from inode M to inode N.)  In such cases, the
    data is backed up with the first occurrence of one of these links, and
    any other (subsequent) links are marked as links, and not expanded.
***************************************************************************/
add_siblink(fromptr, toptr)
FLISTNODE *fromptr, *toptr;
{
    fromptr->linkptr = toptr;
    idxsize += (strlen(LINKTOSTR) + FNAMEOFFSET + mypathlen(toptr));
}






typedef struct weirdnode { 
    FLISTNODE *flistptr;
    ino_t dotdotino;
    dev_t dotdotdev;
    struct weirdnode *next;
} WEIRDNODE;

static WEIRDNODE *weirdhead;
static WEIRDNODE *weirdtail = (WEIRDNODE*)NULL;

/***************************************************************************
    This function is called to add a "weird" directory to the list of dirs
    which are processed after the first pass in making the flist.  A "weird"
    directory is one whose dotdot (..) pointer does not point to the expected
    ("normal") place.  For example, say we're in a directory, "parent" which
    has a subdirectory, "child".  If we execute, "cd child" and then "cd ..",
    and we don't wind up back in "parent".  The link from "parent" to "child"
    may or may not be the only link to "child".
***************************************************************************/
add_weirddir(statbuf, dotdotstatbuf)
struct stat *statbuf, *dotdotstatbuf;
{
    WEIRDNODE *new;

    /* Note: this test should be the same as the one in 'add_flistnode' */
    if ((statbuf->st_mtime < lastbtime) && (statbuf->st_ctime < lastetime))
	return;

    new = (WEIRDNODE*)mymalloc(sizeof(WEIRDNODE));
    if (new == (WEIRDNODE*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,704, "fbackup(1704): out of virtual memory\n"));
    if (weirdtail == (WEIRDNODE *) NULL)
	weirdhead = new;
    else
	weirdtail->next = new;
    new->flistptr = flisttail;
    flisttail->weirdflag = (char)TRUE;
    new->dotdotino = dotdotstatbuf->st_ino;
    new->dotdotdev = dotdotstatbuf->st_dev;
    new->next = (WEIRDNODE *) NULL;
    weirdtail = new;
}






/***************************************************************************
    This function sequences through all the files in the "weird" list and
    finds and then resolves any links to them.  As mentioned in the example
    in the comments of add_weirddir(), the link from "parent" to "child"
    may or may not be the only link to "child".
	If it is NOT the only link to "child", the flist is searched to find
    the one and only non-weird path (represented by an flist node) to it.
    When this path is found, "child" is set to be a link to this path.
    The (sub)tree specified by this path is NOT expanded, since it would
    have already been expanded by the non-weird link to it.
	If it IS the only link to "child", we have an excessively weird file
    system.  The flist is searched to find the path which this file's dotdot 
    pointer points to.  When it is located, it is so marked in the flist.
    The (sub)tree specified by this path IS expanded, since it would NOT
    have already been expanded by the non-weird link to it.
***************************************************************************/
doweirds()
{
    int fwdlinkflag, found;
    ino_t ino;
    dev_t dev;
    FLISTNODE *flistnode, *localflistptr;
    WEIRDNODE *weirdnode = weirdhead;

    while (weirdnode != (WEIRDNODE*)NULL) {	    /* for all weird nodes */
	localflistptr = weirdnode->flistptr;
	ino = localflistptr->ino;
	dev = localflistptr->dev;

	fwdlinkflag = FALSE;
	found = FALSE;
	flistnode = flisthead;
	while (flistnode != (FLISTNODE*)NULL) {	    /* for all flist nodes */
	    if (flistnode == localflistptr)
		fwdlinkflag = TRUE;
	    if ((flistnode->ino == ino) && (flistnode->dev == dev) &&
						    !(flistnode->weirdflag)) {
		found = TRUE;
		localflistptr->linkptr = flistnode;
		idxsize += (strlen(LINKTOSTR) + FNAMEOFFSET
						+ mypathlen(flistnode));
		if (fwdlinkflag) {
		    localflistptr->fwdlinkflag = (char)TRUE;
		}
		localflistptr->weirdflag = (char)FALSE;
		break;
	    }
	    flistnode = flistnode->next;
	}
	if (!found) {
	    ino = weirdnode->dotdotino;
	    dev = weirdnode->dotdotdev;
	    flistnode = flisthead;
	    while (flistnode != (FLISTNODE*)NULL) {    /* for all flist nodes */
		if ((flistnode->ino == ino) && (flistnode->dev == dev)) {
		    localflistptr->dotdotptr = flistnode;
		    break;
		}
		flistnode = flistnode->next;
	    }
	    stuffinflist(localflistptr);
	}
	weirdnode = weirdnode->next;
	free(weirdhead);
	weirdhead = weirdnode;
    }
}






/***************************************************************************
    This function is called to insert files into the flist when it has been
    discovered (while processing the "weird" list) that a (sub)tree needed
    to be expanded (but couldn't have been expanded before, since it wasn't
    know at that time that expanding it was the right thing to do).
***************************************************************************/
stuffinflist(insertafter)
FLISTNODE *insertafter;
{
    FLISTNODE *saveflisthead = flisthead, *saveflisttail = flisttail;
    char tmpbuf[MAXPATHLEN];

    flisttail = (FLISTNODE*)NULL;
    (void)flistpath(insertafter, tmpbuf);
    search(tmpbuf, 1);
    idxsize -= (FNAMEOFFSET + mypathlen(insertafter) + 1);
    nfiles--;
    if (flisthead != flisttail) {
	flisthead = flisthead->next;	    /* don't put this node in twice */
	flisttail->next = insertafter->next;
	insertafter->next = flisthead;
	if (insertafter != saveflisttail)
	    flisttail = saveflisttail;
    } else {
	flisttail = saveflisttail;
    }
    flisthead = saveflisthead;
}

/*
 * Save the file index of the volume change when the writer wants an
 * update.  Then we can just update the volume numbers on the fly in
 * sendidx().  This saves us from having to traverse flist twice.
 */
int updateafter = 999999;	/* initialize to a big number */

/***************************************************************************
    This function is called to update the index.  The volume number where
    each file will eventually reside (erroneous numbers, except for volumes
    which are already written) is incremented for each file (in the flist)
    that is after the one specified (by pad->afterfile).
***************************************************************************/
updateidx()
{
    updateafter = pad->updateafter;
    (void) write(pipefd[1], "\000", 1);	/* signal we're thru updating idx */
}



/*BUGFIX*/
static char idxstr[(MAXPATHLEN * 2) + FNAMEOFFSET];

/***************************************************************************
    This function reads through the flist and sends it over the pipe to the
    writer process.
***************************************************************************/
sendidx()
{
    FLISTNODE *node = flisthead;
    int fno = 1;

    while (node != (FLISTNODE *) NULL) {
	if (fno++ > updateafter) node->vol = pad->vol;
	getidxstr(node, idxstr);
	(void) write(pipefd[1], idxstr, (unsigned)strlen(idxstr));
	node = node->next;
    }
    (void) write(pipefd[1], "\000", 1);	/* signal we're thru sending idx */
}






/***************************************************************************
    This function is called to write the on-line index to disk after a
    session is completed (and if the index has been requested with the
    -I option.
***************************************************************************/
onlineidx()
{
    FLISTNODE *node = flisthead;
    int idxfd;

    if (indexfile == (char*)NULL)
	return;

    if ((idxfd = creat(indexfile, PROTECT)) < 0) {
	msg((catgets(nlmsg_fd,NL_SETN,705, "fbackup(1705): unable to create on line index file %s\n")), indexfile);
	return;
    }

    while (node != (FLISTNODE *) NULL) {
	getidxstr(node, idxstr);
	(void) write(idxfd, idxstr, (unsigned)strlen(idxstr));
	node = node->next;
    }
    (void) close(idxfd);
}



/***************************************************************************
    This function is called to write the volume header to disk after a
    session is completed (and if the volume header has been requested with the
    -V option.
***************************************************************************/
int
volheader_print()
{
    int fd;
    FILE *fs;
    struct utsname name;
    char str[64];

    if (volhdrfile == (char*)NULL)
	return;

    if ((fs = fopen(volhdrfile, "w")) == 0) {
	msg((catgets(nlmsg_fd,NL_SETN,715, "fbackup(1715): unable to create volume header file %s\n")), volhdrfile);
	return;
    }

    fprintf(fs, "Magic Field:%s\n", VOLMAGIC);

    (void) uname(&name);
    fprintf(fs, "Machine Identification:%s\n", name.machine);
    fprintf(fs, "System Identification:%s\n", name.sysname);
    fprintf(fs, "Release Identification:%s\n", name.release);
    fprintf(fs, "Node Identification:%s\n", name.nodename);

    (void) cuserid(str);
    fprintf(fs, "User Identification:%s\n", str);

    fprintf(fs, "Record Size:%d\n", pad ->recsize);
    fprintf(fs, "Time:%s\n", myctime(&((long)(pad->begtime))));
    fprintf(fs, "Media Use:%d\n", 0);
    fprintf(fs, "Volume Number:%d\n", pad->vol);
    fprintf(fs, "Checkpoint Frequency:%d\n", pad->ckptfreq);
    fprintf(fs, "Fast Search Mark Frequency:%d\n", pad->fsmfreq);
    fprintf(fs, "Index Size:%d\n", pad->idxsize);
    fprintf(fs, "Backup Identification Tag:%d %d\n", pad->pid,  pad->begtime);
    fprintf(fs, "Language:%s\n", pad->envlang);

    (void) fclose(fs);
    
    return;
    
}






/***************************************************************************
    This routine is called by both sendidx and onlineidx to form strings
    that make up indices.  If there is a hard link involved, that is
    appropriately put into the string too.
***************************************************************************/
getidxstr(node, str)
FLISTNODE *node;
char *str;
{
    char pvstr[32], lvstr[32];
    char tmpbuf[MAXPATHLEN];
    int len;

    if (node->vol < FIRSTVOL)			/* make the volume number */
	(void) sprintf(pvstr, NOTONBACKUP);
    else
	(void) sprintf(pvstr, "#%3d", node->vol);

    if (node->linkptr == (FLISTNODE*)NULL)	/* NOT linked to anything */
	(void)sprintf(str, "%s %s\n", pvstr, flistpath(node, tmpbuf));

    else {					/* it IS linked to something */
	if (node->linkptr->vol < FIRSTVOL)	/* make link's volume number */
	    (void) sprintf(lvstr, NOTONBACKUP);
	else
	    (void) sprintf(lvstr, "#%3d", node->linkptr->vol);

	len = sprintf(str, "%s %s%s", pvstr, flistpath(node, tmpbuf), LINKTOSTR);
	(void)sprintf(&str[len], "%s %s\n", lvstr, flistpath(node->linkptr, tmpbuf));
    }
}






static int lastfno = 2;
static FLISTNODE *thisnode;

/***************************************************************************
    This function is called to grab the next file in the flist.  The pointer
    thisnode, and the integer lastfno are static variables, so they retain
    their values, and hence, the normal mode of operation is to simply get
    the next item in the list.  However, it is possible to get a file which
    is not the next one.  In this case, the list is searched from the
    beginning until it gets to the file of interest.
***************************************************************************/
int
getfile(fno, fwdlinkflag, file, linkto, dotdot)
int fno, *fwdlinkflag;
char **file, **linkto, **dotdot;
{
/*
 * The routine calling this function should copy the data into its own
 * buffers if it wants them saved since this routine returns the data
 * in static buffers.
 */
    static char tmp_file[MAXPATHLEN];
    static char tmp_linkto[MAXPATHLEN];
    static char tmp_dotdot[MAXPATHLEN];
    if (fno < lastfno) {
	lastfno = 1;
	thisnode = flisthead;
    }

    while (lastfno < fno) {
	thisnode = thisnode->next;
	lastfno++;
    }
    if (lastfno <= nfiles && thisnode) {
	*file = flistpath(thisnode, tmp_file);
	*linkto = flistpath(thisnode->linkptr, tmp_linkto);
	*dotdot = flistpath(thisnode->dotdotptr, tmp_dotdot);
	*fwdlinkflag = thisnode->fwdlinkflag;
	return(TRUE);
    } else
	return(FALSE);
}






/***************************************************************************
    Whenever it is discovered that a file cannot be backed up (eg, it can't
    be stat-ed or opened, or some such) by some part of the main process
    (but not by a reader), it's volume number is set to an illegal value
    to indicate (on the indices) that it was not backed up.
***************************************************************************/
void
cantbackup()
{
    thisnode->vol = FIRSTVOL-1;
}






/***************************************************************************
    The data structure which contains the info necessary to restart a fbackup
    session later (but not the flist info, see below for that).
***************************************************************************/
typedef struct {
    long begtime;
    int pid;
    int level;
    int vol;
    int filenum;
    int graphfilelen;
} RSTHDR;

/***************************************************************************
    The data structure for used to store the flist on a disk file.
***************************************************************************/
typedef struct {
    int pathlen;
    int linknum;
    int dotdotnum;
    int vol;
    char weirdflag;
    char fwdlinkflag;
    ino_t ino;
    dev_t dev;
} RSTNODE;

RSTHDR rsthdr;
RSTNODE rstnode;


/***************************************************************************
    This function is called when it has been determined that the user wishes
    to be able to restart this session later.  It saves (on a disk file)
    all the info necessary to do this.  There are two parts to this data:
    (1) the "header" information which uniquely identifies the session,
    file at which to restart, etc.   Also (2), the flist info.  There is
    one record per file in the flist.
***************************************************************************/
int
savestate(filenum)
int filenum;
{
    char rstfile[MAXPATHLEN];
    int rstfd = -1;
    FLISTNODE *node=flisthead;

    while (rstfd < 0) {
	msg((catgets(nlmsg_fd,NL_SETN,706, "fbackup(1706): enter the name of the restart file (return to exit)\n")));
	*rstfile = '\n';
	(void) fgets(rstfile, MAXPATHLEN, stdin);
	if (*rstfile == '\n')
	    return(FALSE);
	rstfile[strlen(rstfile)-1] = '\0';
	if ((rstfd = creat(rstfile, PROTECT)) < 0)
	    msg((catgets(nlmsg_fd,NL_SETN,707, "fbackup(1707): unable to create restart file %s for writing\n")), rstfile);
    }

    rsthdr.begtime = pad->begtime;	/* save "header" info for the session */
    rsthdr.pid = pad->pid;
    rsthdr.level = level;
    rsthdr.vol = pad->vol + 1;
    rsthdr.filenum = filenum;

			/* and the graphfile, if one has been specified */
    rsthdr.graphfilelen = strlen(graphfile)+1;
    if (write(rstfd, &rsthdr, sizeof(rsthdr)) != sizeof(rsthdr))
	return(FALSE);
    if (write(rstfd, graphfile, (unsigned)rsthdr.graphfilelen) != rsthdr.graphfilelen)
	return(FALSE);
    while (node != (FLISTNODE*)NULL) {		/* for each file in the flist */
	rstnode.pathlen = mypathlen(node)+1;
	if (node->linkptr == (FLISTNODE*)NULL)		    /* save its data */
	    rstnode.linknum = 0;
	else
	    rstnode.linknum = grablinkforsavestate(node->linkptr);
	if (node->dotdotptr == (FLISTNODE*)NULL)
	    rstnode.dotdotnum = 0;
	else
	    rstnode.dotdotnum = grablinkforsavestate(node->dotdotptr);
	rstnode.vol = node->vol;
	rstnode.weirdflag = node->weirdflag;
	rstnode.fwdlinkflag = node->fwdlinkflag;
	rstnode.ino = node->ino;
	rstnode.dev = node->dev;
	if (write(rstfd, &rstnode, sizeof(rstnode)) != sizeof(rstnode))
	    return(FALSE);
	(void)flistpath(node, rstfile);
	if (write(rstfd, rstfile, (unsigned)rstnode.pathlen) != rstnode.pathlen)
	    return(FALSE);
	node = node->next;
    }
    (void) close(rstfd);
    return(TRUE);
}






/***************************************************************************
    This function is called when it is determined that a file has a hard
    link or a dotdot link to another file in the flist.  Since the pointer
    info saved in the flist would be meaningless in the disk file, we must
    get the cardinal number of the file.  This is the value which is saved
    in the 'linknum' and/or 'dotdotnum' field of each RSTNODE.
***************************************************************************/
int
grablinkforsavestate(soughtnode)
FLISTNODE *soughtnode;
{
    int fno=1;
    FLISTNODE *node=flisthead;

    while (node != (FLISTNODE*)NULL) {
	if (node == soughtnode)
	    break;
	fno++;
	node = node->next;
    }
    return(fno);
}






static int rstfd;

/***************************************************************************
    This function does the first part of loading a restart file, when using
    the "-R" option.  Restart files have session information, such as which
    file to restart on, beginning time, process id of the (original) session,
    volume number to restart on, dump level, etc.  This function recovers
    all this information.  The flist is recovered by restoreflist.
    Together, this function and the following one (restoreflist) perform the
    inverse operation that the 'savestate' function (see above).
    Originally, they were one function, but the writer process needs to
    know the starting file number very early, so this functionality had to
    be split into these two functions, one before the writer fork and one
    after.
***************************************************************************/
int
restoresession(filenum)
int *filenum;
{
    if ((rstfd = open(restartfile, 0)) < 0)
	errorcleanx((catgets(nlmsg_fd,NL_SETN,708, "fbackup(1708): unable to open restart file %s for reading\n")), restartfile);
    
    flisthead = (FLISTNODE*)NULL;
    if (read(rstfd, &rsthdr, sizeof(rsthdr)) != sizeof(rsthdr))
	return(FALSE);
    pad->begtime = rsthdr.begtime;
    pad->pid = rsthdr.pid;
    level = rsthdr.level;
    pad->vol = rsthdr.vol;
    *filenum = rsthdr.filenum;
    pad->startfno = rsthdr.filenum;

    graphfile = (char*)mymalloc((unsigned)rsthdr.graphfilelen);
    if (graphfile == (char*)NULL)
	errorcleanx(catgets(nlmsg_fd,NL_SETN,709, "fbackup(1709): out of virtual memory\n"));

    if (read(rstfd, graphfile, (unsigned)rsthdr.graphfilelen) != rsthdr.graphfilelen)
	return(FALSE);
    return(TRUE);
}






/***************************************************************************
    This function does the second part of loading a restart file, when using
    the "-R" option.  The function restoresession recovers the session
    information (see above), and this function recovers the flist.
***************************************************************************/
int
restoreflist(filenum)
int filenum;
{
    FLISTNODE *new, *putlinkforrestoreflist();
    FLISTNODE *prev = (FLISTNODE*)NULL;		/* only set for lint */
    long lseek();
    char path[MAXPATHLEN];
    char *tmp;
    int len, fno = 1;

    --filenum;

    while (read(rstfd, &rstnode, sizeof(rstnode)) == sizeof(rstnode)) {
	if (read(rstfd, path, (unsigned)rstnode.pathlen) != rstnode.pathlen)
	    return(FALSE);

	if (tmp = strrchr(path, '/'))
	  len = strlen(++tmp);
	else
	  len = strlen(tmp = path);

	new = (FLISTNODE*)new_malloc(sizeof(FLISTNODE) + len);

        if (new == (FLISTNODE*)NULL)
	    errorcleanx(catgets(nlmsg_fd,NL_SETN,710, "fbackup(1710): out of virtual memory\n"));

	if (new->len = len)
	    (void)strcpy(&new->pathptr, tmp);
	else
	    new->pathptr = '\0';

	if (tmp != path) {
	  *tmp = '\0';
	  new->dirptr = add_dentry(path);
	} else {
	  new->dirptr = (dtabentry *)NULL;
	}
/*
 * Update the volume number here instead of calling updateidx() later.
 * This saves us from having to traverse flist again.
 * We should do it in savestate() but then the file would be incompatible
 * with older fbackups since they would update the volume number again in
 * this routine.
 */
	if (fno++ > filenum)
	    new->vol = rstnode.vol + 1;
	else
	new->vol = rstnode.vol;
	new->weirdflag = rstnode.weirdflag;
	new->fwdlinkflag = rstnode.fwdlinkflag;
	new->ino = rstnode.ino;
	new->dev = rstnode.dev;
	idxsize += (FNAMEOFFSET + rstnode.pathlen);
	new->next = (FLISTNODE*)NULL;
	if (flisthead == (FLISTNODE*)NULL)
	    flisthead = new;
	else
	    prev->next = new;
	prev = new;
	nfiles++;
    }

		    /* Resolve all the links (normal or forward), and the
		       dotdot links too.
		    */
    new = flisthead;
    (void) lseek(rstfd, (long)(sizeof(rsthdr)+rsthdr.graphfilelen), 0);
    while (read(rstfd, &rstnode, sizeof(rstnode)) == sizeof(rstnode)) {
	(void) lseek(rstfd, (long)rstnode.pathlen, 1);
	if (rstnode.linknum == 0)
	    new->linkptr = (FLISTNODE*)NULL;
	else {
	    new->linkptr = putlinkforrestoreflist(rstnode.linknum);
	    idxsize += (strlen(LINKTOSTR) + FNAMEOFFSET
					    + mypathlen(new->linkptr));
	}
	if (rstnode.dotdotnum == 0)
	    new->dotdotptr = (FLISTNODE*)NULL;
	else
	    new->dotdotptr = putlinkforrestoreflist(rstnode.dotdotnum);
	new = new->next;
    }

    (void) close(rstfd);
    lastfno = nfiles + 1;
    (void) unlink(restartfile);
    msg((catgets(nlmsg_fd,NL_SETN,712, "fbackup(1712): restarting session begun on %s\n")), myctime(&pad->begtime));
    if (rsthdr.graphfilelen > 1)
	msg((catgets(nlmsg_fd,NL_SETN,713, "fbackup(1713): of graph file %s at level %d\n")), graphfile, level);
    else
	graphfile = (char*)NULL;
    msg((catgets(nlmsg_fd,NL_SETN,714, "fbackup(1714): resuming at file number %d on volume number %d\n")), filenum, pad->vol);
    return(TRUE);
}






/***************************************************************************
    This function performs the inverse operation of that described in the
    grablinkforsavestate function above.
***************************************************************************/
FLISTNODE *
putlinkforrestoreflist(filenum)
int filenum;
{
    int fno=1;
    FLISTNODE *node=flisthead;

    while (fno < filenum) {
	fno++;
	node = node->next;
    }
    return(node);
}

/*
 * This is the directory hash table
 */
#define DTABLESIZE	1021
dtabentry *dtable[DTABLESIZE];

/*
 * This routine initializes the directory hash table and initializes
 * some state variables.
 */
init_dtable()
{
  int i;
  FILE *out;
  time_t date;

  for (i = 0; i < DTABLESIZE; i++) {
    dtable[i] = (dtabentry *)NULL;
  }

  root_dirptr = (dtabentry *)NULL;	/* need to initialize before calling */
  root_dirptr = add_dentry("/");	/*  add_dentry */
  current_flistptr = flisttail;

} /* init_dtable */

/*
 * This routine adds a pathname to the directory hash table.
 */
dtabentry *add_dentry(path)
     char *path;
{
  char *dir;
  unsigned int i, len;
  dtabentry *dptr, *dentry;
  char tmp[MAXPATHLEN];

  if (path == NULL)
    return current_dirptr = (dtabentry *)NULL;

  if (root_dirptr && *path == '/' && path[1] == '\0')
    return current_dirptr = root_dirptr;
/*
 * copy the pathname, calculate the hash value, and compute the length
 */
  dir = tmp;
  for (i = len = 0; *dir++ = *path++; i += (unsigned)*dir) len++;
/*
 * make sure the path has a trailing slash
 */
  if (tmp[len - 1] != '/') {
    i += (unsigned)'/';
    tmp[len++] = '/';
    tmp[len] = '\0';
  }

  dir = tmp;
  dptr = dtable[i %= DTABLESIZE];

  if (dptr == (dtabentry *)NULL) {
    dtable[i] = dentry = (dtabentry *)new_malloc(sizeof(dtabentry) + len);
  } else {
    while (dptr->next != (dtabentry *)NULL) {
      if (len == dptr->len && !strcmp(dir, &dptr->path)) {
	return current_dirptr = dptr;	/* entry already exists */
      }

      dptr = dptr->next;
    }

    if (len == dptr->len && !strcmp(dir, &dptr->path)) {
      return current_dirptr = dptr;	/* entry already exists */
    }

    dptr->next = dentry = (dtabentry *)new_malloc(sizeof(dtabentry) + len);
  }

  (void)strcpy(&dentry->path, tmp);
  dentry->len = len;
  dentry->next = (dtabentry *)NULL;
  return current_dirptr = dentry;
} /* add_dentry */

/*
 * This routine adds the parent directory of the given pathname to the
 * directory hash table.  It assumes that the given pathname is absolute.
 */
dtabentry *add_pdentry(path)
     char *path;
{
  char *ptr;
  dtabentry *dptr;
/*
 * if "/" is the parent of path then return the root directory pointer
 */
  if (root_dirptr && (ptr = strrchr(path, '/')) && ptr == path)
    return current_dirptr = root_dirptr;

  if (ptr == NULL) {
    current_dirptr = dptr = (dtabentry *)NULL;
  } else {
    *ptr = '\0';
    dptr = add_dentry(path);
    *ptr = '/';
  }

  return dptr;
} /* add_pdentry */

/*
 * This routine returns the full pathname in the given flist pointer.
 * The data is copied into buf, and buf is returned.
 * if the flist pointer is NULL then NULL is returned.
 */
char *flistpath(f, buf)
     FLISTNODE *f;
     char *buf;
{
  if (f) {
    if (f->dirptr) {
      (void)strcpy(buf, &f->dirptr->path);

      if (f->len)
	(void)strcpy(&buf[f->dirptr->len], &f->pathptr);
/*
      else
	buf[f->dirptr->len - 1] = '\0';
*/
    } else {
      (void)strcpy(buf, &f->pathptr);
    }

    return buf;
  }

  return NULL;
} /* flistpath */

/*
 * This routine compares the two pathnames in the given flist pointers.
 */
pathcmp(p1, p2)
     FLISTNODE *p1, *p2;
{
  int ret;
/*
 * compare the directory component
 */
  if (p1->dirptr != p2->dirptr) {
    if (p1->dirptr && p2->dirptr) {
      ret = mystrncmp(&p1->dirptr->path, &p2->dirptr->path,
			min(p1->dirptr->len,p2->dirptr->len));

      if (ret)
	return ret;

      if (p1->dirptr->len == p2->dirptr->len)
	return mystrcmp(&p1->pathptr, &p2->pathptr);
      else
      if (p1->dirptr->len < p2->dirptr->len)
	return mystrcmp(&p1->pathptr, &(&p2->dirptr->path)[p1->dirptr->len]);
      else
      	return mystrcmp(&(&p1->dirptr->path)[p2->dirptr->len], &p2->pathptr);
    }
    else if (p1->dirptr)
        return mystrcmp(&p1->dirptr->path, &p2->pathptr);
    else	
        return mystrcmp(&p1->pathptr, &p2->dirptr->path);
  }
  else
  	return mystrcmp(&p1->pathptr, &p2->pathptr); /* compare the 
					              *filename component
						      */
} /* pathcmp */

#pragma OPT_LEVEL 1		/* the compiler made me do this, honest! */
/*
 * This routine malloc's a large chunk to be given out in smaller pieces.
 * Buffers obtained from this routine cannot be free'd!
 */
#define CHUNK	(128*1024)

char *new_malloc(size)
     size_t size;
{
  static size_t left = 0;
  static char *buf;
  char *ptr;

  size = ((size + (sizeof(int)-1)) & ~(sizeof(int)-1));	/* round up */

  if (left < size) {
/*
 * allocate enough for the request
 */
    for (left = CHUNK; left < size; left += CHUNK);

    while ((buf = (char *)malloc(left)) == NULL) {
      if ((left >>= 1) < size) {
	return NULL;
      }
    }

  }

  ptr = buf;
  left -= size;
  buf = &buf[size];
  return ptr;
} /* new_malloc */

