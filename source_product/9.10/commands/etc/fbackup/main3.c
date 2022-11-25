/* @(#) $Revision: 70.1 $ */

/***************************************************************************
****************************************************************************

	main3.c

    This file contains functions which build the header and trailer blocks.
    The two functions buildhdr and buildtrl are called to perform these
    tasks, while the other functions is this file are called by buildhdr
    and buildtrl to allocate shared, memory blocks, fill them with data,
    calculate the blocks checksum values, etc.

****************************************************************************
***************************************************************************/

#include "head.h"
#include <unistd.h>

#ifdef NLS
#define NL_SETN 1	/* message set number */
extern nl_catd nlmsg_fd;
#endif NLS

void blk_cksum(), markempty(), markfull();

extern int segsize,
	   segindex;
extern char	*segment;
extern PADTYPE	*pad;
extern RDRTYPE	*rdr;
extern RECTYPE	*rec;
extern PENDFTYPE *pend_ptr;

#ifdef ACLS
extern int aclflag;			/* from parse_cmd */
#endif /* ACLS */

static int blkidx, blkcount, oldsegidx, hdrflag;
static char	pidstr[INTSTRSIZE+1], timestr[INTSTRSIZE+1],
		fnumstr[INTSTRSIZE+1], blktype;
static off_t	datasize;
static PBLOCKTYPE blk;






/***************************************************************************
    This function is called to get a block for either a header or a
    trailer.  It waits until a least BLOCKSIZE bytes are available, then
    fills in the fields of the block which are common to both header and
    trailer blocks.  It returns a (char *) pointer to the block.
***************************************************************************/
char *
getblock()
{
    PBLOCKTYPE ptr;
    long savemask;

    savemask = sigblock(WRTRSIG);
    while (pad->avail < BLOCKSIZE) {
	if (pad->wrtrstatus != READY) {
	    (void) sigsetmask(savemask);
	    blkidx = 0;
	    return(&segment[0]);
	}
	(void) sigpause(savemask);
    }
    (void) sigsetmask(savemask);

    ptr.ch = &segment[segindex];
    blkcount++;
    if (hdrflag) {			/* Header block */
	if (blkcount == 1) {			/* first block */
	    (void) strcpy(ptr.id->type, BOH);
	    (void) strcpy(ptr.hdr->filenum, fnumstr);
	    (void) strcpy(ptr.hdr->id.ppid, pidstr);
	    (void) strcpy(ptr.hdr->id.time, timestr);
	    blktype = HDRFIRST;
	    blkidx = sizeof(FHDRTYPE);
	} else {				/* non-first block */
	    (void) strcpy(ptr.id->type, COH);
	    blktype = HDRCONT;
	    blkidx = sizeof(BLKID);
	}
	(void) strcpy(ptr.id->last, MOR);
    } else {				/* Trailer block */
	(void) strcpy(ptr.id->type, BOT);	/* note: trailers MUST be */
	(void) strcpy(ptr.id->last, EOT);	/* only one block in length */
	(void) strcpy(ptr.trl->filenum, fnumstr);
	(void) strcpy(ptr.trl->status, ASCIIGOOD);
	pend_ptr->statusidx = (ptr.trl->status);
	if (pend_ptr->modflag)
	    (void) strcpy(pend_ptr->statusidx, ASCIIBAD);
	pend_ptr->status = TRLINMEM;
	blktype = TRAILER;
	blkidx = sizeof(FTRLTYPE);
    }
    (void) strcpy(ptr.id->magic, MAGICSTR);
    atomicadd(-BLOCKSIZE);
    segindex = (segindex+BLOCKSIZE) % segsize;
    markempty(oldsegidx, BLOCKSIZE, blktype, datasize);
    return(ptr.ch);
}






/***************************************************************************
    This function is called to place two strings into block(s) of data.
    Whenever any part either string will not fit in the present block,
    a new block is acquired, and the filling process continues.
***************************************************************************/
void
fill(str)
char *str;
{
    int len, minn, n_copied;

    n_copied = 0;
    len = strlen(str)+1;
    while (n_copied < len) {
	if (blkidx == BLOCKSIZE) {
	    if (blkcount > 0) {
		blk_cksum(blk);
		markfull(oldsegidx, BLOCKSIZE);
	    }
	    oldsegidx = segindex;
	    blk.ch = getblock();
	}
	minn = min(len-n_copied, BLOCKSIZE-blkidx);
	bcopy(blk.ch+blkidx, &str[n_copied], minn);
	n_copied += minn;
	blkidx += minn;
    }
}






static char str[MAXSTRLEN];   /* This is used for both buildhdr and buildtrl. */


/***************************************************************************
    This function places all relevant information in the header block(s).
    It repeatedly calls fill with a string argument.  Any numeric values
    are first converted to strings.  Most strings have the form:
		    label:value
***************************************************************************/
void
buildhdr(file, linkto, dotdot, filenum, fwdlinkflag, statbuf)
char *file, *linkto, *dotdot;
int filenum, fwdlinkflag;
struct stat *statbuf;
{
    char *ptr, *getapw(), *getagr();
    int lim;
    blkcount = 0;
    blkidx = BLOCKSIZE;
    hdrflag = TRUE;
    datasize = statbuf->st_size;
    (void) sprintf(fnumstr, INTSTRFMT, filenum);

    fill(file);
    (void) sprintf(str, "st_size:%d", statbuf->st_size);
    fill(str);
    (void) sprintf(str, "st_ino:%d", statbuf->st_ino);
    fill(str);
    (void) sprintf(str, "st_mode:%d", statbuf->st_mode);
    fill(str);
    (void) sprintf(str, "st_nlink:%d", statbuf->st_nlink);
    fill(str);
    if (linkto != (char*)NULL) {	/* this is a link to another node */
	(void) sprintf(str, "link_to:%s", linkto);
	fill(str);
    }
    if (dotdot != (char*)NULL) {	/* .. pointer goes to a weird place */
	(void) sprintf(str, "dotdot_to:%s", dotdot);
	fill(str);
    }
    if (fwdlinkflag) {			/* a "forward link" */
	fill("fwdlink:");
    }
    (void) sprintf(str, "st_atime:%d", statbuf->st_atime);
    fill(str);
    (void) sprintf(str, "st_mtime:%d", statbuf->st_mtime);
    fill(str);
    (void) sprintf(str, "st_ctime:%d", statbuf->st_ctime);
    fill(str);
    (void) sprintf(str, "st_dev:%d", statbuf->st_dev);
    fill(str);
    (void) sprintf(str, "st_rdev:%d", statbuf->st_rdev);
    fill(str);
    (void) sprintf(str, "st_blocks:%d", statbuf->st_blocks);
    fill(str);
    (void) sprintf(str, "st_blksize:%d", statbuf->st_blksize);
    fill(str);

#ifdef CNODE_DEV			/* cnode specific devices */
    (void) sprintf(str, "st_rcnode:%d", statbuf->st_rcnode);
    fill(str);
#endif /* CNODE_DEV */

#ifdef ACLS
    if(aclflag) {			/* backup entries */
	(void) sprintf(str, "st_acl:%d", statbuf->st_acl);
	fill(str);
	if(statbuf->st_acl) {		/* file has optional entries */
	    int nacl_entries, i, write_acls;
	    struct acl_entry acl[NACLENTRIES];

	    write_acls = TRUE;			/* assume all is known */
	    nacl_entries = getacl(file, 0, acl);/* get number of entries */
	    i = getacl(file, nacl_entries, acl);/* get the entries */
	    for(i = 0; i < nacl_entries; i++) {
		if((((strcmp(getapw(acl[i].uid),(char *)NULL) == 0)) &&
		    (acl[i].uid != ACL_NSUSER)) ||
		    ((strcmp(getagr(acl[i].gid), (char *)NULL) == 0) &&
		    (acl[i].gid != ACL_NSGROUP))) {
		    write_acls = FALSE;
		    msg((catgets(nlmsg_fd,NL_SETN,202, "fbackup(1202): unknown user or group specified in ACL for file: %s\n; optional entries are not backed up.\n")), file);
		    break;
		}
	    }
	    if(write_acls) {
		(void) sprintf(str, "nacl_entries:%d", nacl_entries);	/* 1st for frecover */
		fill(str);
		for(i = 0; i < nacl_entries; i++) {
		    (void) sprintf(str, "acl_uid:%s", getapw(acl[i].uid));
		    fill(str);
		    (void) sprintf(str, "acl_gid:%s", getagr(acl[i].gid));
		    fill(str);
		    (void) sprintf(str, "acl_mode:%d", acl[i].mode);
		    fill(str);
		}
	    }
	    else {
		(void) sprintf(str, "nacl_entries:0");	/* handle bad entries*/
		fill(str);
	    }
	}
    }
#endif /* ACLS */

    if ((ptr=getapw((int)statbuf->st_uid))) {
	(void) sprintf(str, "loginname:%s", ptr);
	fill(str);
    }
    if ((ptr=getagr((int)statbuf->st_gid))) {
	(void) sprintf(str, "groupname:%s", ptr);
	fill(str);
    }

		    /* Fill any unused part of the last block with fill
		       characters, and mark that this block is full.
		    */
    (void) strcpy(blk.id->last, EOH);
    if ((lim=BLOCKSIZE-blkidx) > 0) {
	ptr = blk.ch+blkidx;
	while (lim--)
	    *ptr++ = FILLCHAR;
    }
    blk_cksum(blk);
    markfull(oldsegidx, BLOCKSIZE);
}


/***************************************************************************
    This function places all relevant information in the trailer block, in
    a manner similar to buildhdr, above.

    NOTE: Trailers MUST be only one block in length!  This is to prevent
    the situation of a write error in the middle of a trailer, ie, the
    first trailer block indicates that the file is a good copy ("G") but
    the rest of the trailer may not be good.
***************************************************************************/
void
buildtrl(filenum, statbuf)
int filenum;
struct stat *statbuf;
{
    char *ptr;
    int lim;

    blkcount = 0;
    blkidx = BLOCKSIZE;
    hdrflag = FALSE;
    (void) sprintf(fnumstr, INTSTRFMT, filenum);

    (void) sprintf(str, "st_uid:%d", statbuf->st_uid);
    fill(str);
    (void) sprintf(str, "st_gid:%d", statbuf->st_gid);
    fill(str);
    (void) sprintf(str, "st_remote:%d", statbuf->st_remote);
    fill(str);
    (void) sprintf(str, "st_netdev:%d", statbuf->st_netdev);
    fill(str);
    (void) sprintf(str, "st_netino:%d", statbuf->st_netino);
    fill(str);

    if ((lim=BLOCKSIZE-blkidx) > 0) {
	ptr = blk.ch+blkidx;
	while (lim--)			/* fill with null chars */
	    *ptr++ = FILLCHAR;
    }
    blk_cksum(blk);
    markfull(oldsegidx, BLOCKSIZE);
}






/***************************************************************************
    This function calculates a checksum for a block.  It is passed
    a pointer and the checksum in done on an integer (32 bit)
    basis.  This number is then converted to a string (in INTSTR
    format) and put in the place within the block allocated for the
    checksum information.
***************************************************************************/
void
blk_cksum(localblk)
PBLOCKTYPE localblk;			/* Note: this is a pointer */
{
    int lim, sum = 0;
    int *intptr;
    char *chptr;

    chptr = localblk.id->checksum;
    lim = sizeof(localblk.id->checksum);
    while (lim--)
	*chptr++ = '\0';

    intptr = localblk.integ;
    lim = BLOCKSIZE/sizeof(int); 
    while (lim--) {
	sum += *intptr++;
    }
    (void) sprintf(localblk.id->checksum, INTSTRFMT, sum);
}






/***************************************************************************
    This function is called to initialize the session identifcation info.
    This consists of the process id number of fbackup (but none of its
    child processes) and the time at which it was invoked.
***************************************************************************/
void
inithdrtrl()
{
    (void) sprintf(pidstr, INTSTRFMT, pad->pid);
    (void) sprintf(timestr, INTSTRFMT, pad->begtime);
}






/***************************************************************************
    This function is called to reset the access and modification times of
    a file after fbackup is finished backing it up.  At the present time,
    it is NOT possible to reset the change inode time (st_ctime).  Hopefully,
    a new system call will be added which will allow this to be done too.
    When this happens, this function should be modified.
***************************************************************************/
resettimes(p_ptr)
PENDFTYPE *p_ptr;
{
    struct utimbuf times;
    long savemask;

    times.actime  = p_ptr->atime;
    times.modtime = p_ptr->mtime;
    savemask = sigblock(RDRSIG|WRTRSIG);
    if (utime(p_ptr->file, &times)) {
      perror("fbackup(9999)");
      msg((catgets(nlmsg_fd,NL_SETN,201, "fbackup(1201): unable to reset access and modification times for file %s\n")), p_ptr->file);
    }
    (void) sigsetmask(savemask);
}






/***************************************************************************
    Simple block copy function.
***************************************************************************/
bcopy(to, from, size)
char *to, *from;
int size;
{
    int i;
    for (i=0; i<size; i++, to++, from++)
	*to = *from;
}


