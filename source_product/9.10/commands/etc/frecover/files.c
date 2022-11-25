/* $Header: files.c,v 72.4 93/03/30 14:38:30 ssa Exp $Revision: 66.8 $ */
/****************************************************************

 The name of this file is files.c.

 +--------------------------------------------------------------+
 | (c)  Copyright  Hewlett-Packard  Company  1986.  All  rights |
 | reserved.   No part  of  this  program  may be  photocopied, |
 | reproduced or translated to another program language without |
 | the  prior  written   consent  of  Hewlett-Packard  Company. |
 +--------------------------------------------------------------+

 Changes:
	$Log:	files.c,v $
 * Revision 72.4  93/03/30  14:38:30  14:38:30  ssa
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fixed DTS # DSDe407469. Earlier fix to files.c in 66.18 assumes
 * that this condition is an error.
 * 
 * Revision 72.3  93/03/04  14:46:17  14:46:17  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fix for extra slash in file name. changes made so that
 * ".//" is simply "./". Related to DSDe407825.
 * 
 * Revision 72.2  93/03/03  10:56:02  10:56:02  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Fixed DTS # DSDe409144. frecover should unlink file only
 * if it is a link when recovering links.
 * 
 * Revision 72.1  92/10/08  16:57:50  16:57:50  ssa (RCS Manager)
 * Author: kumaran@hpucsb2.cup.hp.com
 * Added message after successful recovery of
 * active files. Related to DTS #  DSDe407937
 * in fbackup.
 * 
 * Revision 70.2  92/09/03  12:55:44  12:55:44  ssa (RCS Manager)
 * Author: kumaran@15.0.56.21
 * Fixes defect in which the argument type to stat()
 * call was incorrect. This defect made -X option
 * Validations for 300 systems to fail.
 * 
 * Revision 70.1  92/01/29  13:40:52  13:40:52  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Merged changes made for the second frecover patch for 8.0
 * 
 * Revision 66.18  92/01/25  16:50:53  16:50:53  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * -Change to onmedia() to prevent asking for volume 0
 * -Changes to CDF-creation code to handle the case where the file already
 *  exists, but not as a CDF -- will now move it to the standalone context
 * -Now stores stat info for '.' for -F (flat) recoveries also --
 *  prevents setting the current time for '.' to Dec 31, 1969
 * 
 * Revision 66.17  91/12/11  20:23:15  20:23:15  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Changed the backup id checking to change the id mid-recovery if a
 *  file's header suggets a different one (handy for backups that have
 *  encountered problems).  Earlier change for this copied incorrect id string.
 * 
 * Revision 66.15  91/11/14  17:30:27  17:30:27  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Cleaned up catgets calls so findmsg could find them.
 * 
 * Revision 66.14  91/11/13  18:35:08  18:35:08  ssa (RCS Manager)
 * Author: dickermn@hpucsb3.cup.hp.com
 * Change for fastsearch return value on workstations (EIO for w/s,
 * but ioctl returns okay status on 800s).
 * 
 * Revision 66.13  91/11/12  19:05:30  19:05:30  ssa
 * Author: dickermn@hpucsb2.cup.hp.com
 * Further correction to allow uids to be preserved for logins not in 
 * /etc/passwd and group names not in /etc/group
 * 
 * Revision 66.12  91/11/12  16:05:06  16:05:06  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Corrected fastsearch tape driver error message to not print usage()
 * 
 * Revision 66.11  91/11/11  18:09:43  18:09:43  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Recoded the logic for writing-out sparse files so that files will contain
 * all of their intended null bytes, even those at the end of the file.
 * 
 * Revision 66.16  91/11/04  10:06:56  10:06:56  root ()
 * Fixed problem with looping DDS fastsearches (off-by-one error) and 
 * Corrected exclude code so that partial recovery will end as opposed to asking
 * for the next tape.
 * 
 * Revision 66.15  91/10/30  15:00:08  15:00:08  root ()
 * Corrected problems with fastsearch on multiple tape backups, including
 * tapes which are not in the correct initial order.
 * Changed the types for uid and gid from ushort to uid_t and gid_t, so that
 * they are correctly read-in, and can be used.
 * Also corrected the way in which uids and gids are treated when the login/group
 * name is not found in the passwd/group file.  Now frecover behaves more 
 * like other recovery utilities, in that it will restore a file with an
 * unknown login name by the uid stored in the header (as opposed to restoring
 * it to the uid who is running frecover).  
 * 
 * Revision 66.14  91/10/23  18:40:05  18:40:05  root ()
 * Added tolerance to the checksum-checking routine to accept character-based
 * checksums as well as integer-based sums.  Also cleaned-up error messages
 * 
 * Revision 66.13  91/10/18  10:13:41  10:13:41  root ()
 * corrected onmedia and wantfile routines to recover entire directories
 * correctly (see last revision), and still recover other files on the tape
 * 
 * Revision 66.12  91/10/17  10:57:16  10:57:16  root ()
 * Can now recover files within a subdirectory, even when the directory's
 * name is a subset of a regular filename in the same directory.
 * 
 * Revision 66.11  91/10/11  11:37:44  11:37:44  root ()
 * Added support for new volume changes to previous volumes
 * 
 * Revision 66.10  91/10/10  18:12:02  18:12:02  root ()
 * Added check to flag to prevent prealloc-ing before writing
 * to a possibly sparse file.  
 * Added support to -n "no recover" verify option: writes to /dev/null
 * If an included parent directory has not been recovered, but we are at
 * a file which should be, were the parent to have been, prints a warning
 * then continues recovering.
 * 
 * Revision 66.9  91/10/08  10:08:33  10:08:33  root ()
 * Set fsm_move and noread flags correctly in somefiles when a fastsearch
 * is done.  Also changed preen_index algorithm to just mark files as
 * desired in the main index rather than allocating a seperate short index.
 * Added various routines to scan this marked index rather than the seperate 
 * short_index.
 * 
 * Revision 66.8  91/10/01  17:45:12  17:45:12  root ()
 * preen_index changed to re-use index list paths from the main index
 * if memory allocation fails, it will attempt to continue without fastsearch.
 * Also changed match() so that it will accept matches with paths ending
 * in slashes, such as '/' or '/etc/'
 * 
 * Revision 66.7  91/09/13  02:03:02  02:03:02  hmgr (History Manager)
 * Author: danm@hpbblc.bbn.hp.com
 * patch to allow recovery of 8.01 DAT tapes on other 8.0X versions
 * 
 * Revision 66.6  91/03/01  08:29:26  08:29:26  ssa (#shared source login)
 * Author: danm@hpbblc.bbn.hp.com
 * fixes to MO and DAT extensions
 * 
 * Revision 66.5  91/01/17  15:11:19  15:11:19  danm
 *  changes for DAT and MO support as well as other 8.0 enhancements
 * 
 * Revision 66.4  90/09/18  09:54:58  09:54:58  danm (#Dan Matheson)
 *  fix to prevent restore of more than asked for when -i options used, eg
 *  -i a should bring back only directory a and not directory ab.
 * 
 * Revision 66.3  90/02/27  15:37:18  15:37:18  danm (#Dan Matheson)
 *  clean up of some include files
 * 
 * Revision 66.2  90/02/26  19:28:40  19:28:40  danm (#Dan Matheson)
 *  moved #includes to frecover.h too make changes easier, and fixed things
 *  to use malloc(3x)
 * 
 * Revision 64.9  90/01/11  14:24:13  14:24:13  danm (#Dan Matheson)
 *  fixes for alphabetic problem, 3.0, 3.1 & 6.5 fbackup defect
 * 
 * Revision 64.7  89/07/24  01:03:20  01:03:20  kamei
 * Modified not to change mode and utime of files by symlink.
 * 
 * Revision 64.6  89/07/19  20:49:20  20:49:20  takiya (Makoto Takiya)
 * This code was modified such that unknow field in header or trailer is
 * ignored.
 * 
 * Revision 64.5  89/05/11  20:27:48  20:27:48  kazu (Kazuhisa Yokota)
 * bug fix FSDlj04517
 * fbackup silently corrupts output (on disk) when active file encountered
 * change frecover skip files with file length instead of searching of BOH
 * 
 * Revision 64.3  89/02/18  15:36:25  15:36:25  jh
 * NLS initialization and collation.
 * Replaced nl_init() with setlocale(); replaced nl_strcmp() with strcoll().
 * 
 * Revision 64.2  89/02/02  23:30:49  23:30:49  kazu
 * add remote device access feature
 * 
 * Revision 64.1  89/01/16  08:24:18  08:24:18  lkc (Lee Casuto)
 * Changed DOTDOT to FBDOTDOT to avoid define collisions
 * 
 * Revision 63.5  88/11/09  11:16:49  11:16:49  lkc (Lee Casuto)
 * made changes to reflect tuple to entry change
 * 
 * Revision 63.4  88/10/14  16:05:57  16:05:57  lkc ()
 * Modified access checking routine to use iget_access from the kernel.
 * 
 * Revision 63.3  88/09/29  09:10:25  09:10:25  lkc (Lee Casuto)
 * Modified behavior of CSD to be consistent with ftio, cpio, dump/restore.
 * 
 * Revision 63.2  88/09/22  16:09:27  16:09:27  lkc (Lee Casuto)
 * Added code to handle ACLS
 * 
 * Revision 63.1  88/09/16  15:50:07  15:50:07  lkc (Lee Casuto)
 * Added support for cnode specific devices.
 * 
 * Revision 62.6  88/08/30  13:19:39  13:19:39  lkc (Lee Casuto)
 * modified to handle foreign device files correctly
 * 
 * Revision 62.5  88/08/24  15:01:27  15:01:27  lkc (Lee Casuto)
 * repaired recovery of net files
 * 
 * Revision 62.4  88/07/26  17:44:21  17:44:21  lkc (Lee Casuto)
 * modified to handle CDF's correctly
 * 
 * Revision 62.3  88/07/21  17:29:52  17:29:52  lkc (Lee Casuto)
 * modified to recover network special files and fifos correctly
 * 
 * Revision 62.2  88/07/20  12:07:24  12:07:24  lkc (Lee Casuto)
 * Modified so that all non recovered paths are listed. Also part of frecover
 * termination on files that can't be on media.
 * 
 * Revision 62.1  88/04/06  11:26:54  11:26:54  carolyn
 * added error numbers to messages for SAM to key off of
 * 
 * Revision 56.3  88/03/25  13:29:41  13:29:41  pvs
 * Added code to recover CDFs properly.
 * 
 * Revision 56.2  87/11/13  10:12:48  10:12:48  crook
 * Fixed files.c so that directories with descendents retain their correct
 * atime and mtime values (they used to get set to the current time whenever
 * the directory had descendents).
 * 
 * Revision 56.1  87/11/04  10:07:54  10:07:54  runyan (Mark Runyan)
 * Complete for first release (by lkc)
 * 
 * Revision 51.2  87/11/03  16:56:45  16:56:45  lkc (Lee Casuto)
 * completed for first release
 * 

 This file:
	handles all files to be recovered

 Description of this [command, test]:
	<Specify here using as many lines as necessary.>

 Input Parameters:


 Expected results:

 Supporting files and Relationship:


****************************************************************/
#ifdef CNODE_DEV
#define CSD 300
#endif /* CNODE_DEV */

#if defined NLS || defined NLS16
#define NL_SETN 1	/* set number */
#endif

#include "frecover.h"

extern char *strcat();
extern char *strcpy();
extern long lseek();
extern void free();

/********************************************************************/
/*	    VARIABLES FROM MAIN					    */
extern int  volnum;	    	    /* current volume number	    */
extern int xflag;		    /* recover to local dir.	    */
extern int vflag;		    /* verbose flag		    */
extern int oflag;		    /* don't change ownership	    */
extern int flatflag;		    /* flat filesys recovery	    */
extern int fflag;		    /* full recovery flag	    */
extern int hflag;		    /* recover only dir structure   */
extern int sflag;		    /* sparse file flag		    */
extern int nflag;		    /* No recover (verify) flag     */
extern int aflag;		    /* residual file write flag	    */
extern int residfd;		    /* residual file fd		    */
extern int overwrite;		    /* overwrite existing files	    */
extern char home[MAXPATHLEN+3];	    /* home directory		    */
extern struct passwd user;	    /* user data		    */
extern struct group  guser;	    /* group data		    */
extern int recovertype;		    /* type of recovery		    */
extern LISTNODE *ilist;		    /* include path list	    */
extern LISTNODE *elist;		    /* exclude path list	    */
extern LISTNODE *temp;		    /* temporary list pointer	    */
extern OBSCURE *o_head;		    /* start of obscure list	    */
extern int blocksize;
extern int do_fs;
extern int outfiletype;
extern int fd;
extern struct index_list *index_head;
extern struct index_list *index_tail;

#if defined NLS || defined NLS16
extern nl_catd nlmsg_fd;	    /* used by catgets		    */
#endif
#ifdef ACLS
extern int aclflag;		    /* flag for optional entry rec. */
#endif /* ACLS */
/********************************************************************/

/********************************************************************/
/*  	    VARIABLES FROM IO	    	    	    	    	    */
extern char buf[];  	    	    /* holds file data	    	    */
extern char *gbp;   	    	    /* pointer to buf	    	    */
extern int ckptcount;		    /* checkpoint count		    */
extern int noread;		    /* used by UNGETBLK 	    */
/********************************************************************/

/********************************************************************/
/*	    PUBLIC PART OF FILES				    */
char 	link_to[MAXPATHLEN+1];	    /* used to set link names  	    */
char	dotdot_to[MAXPATHLEN+1];    /* .. pointer of a dir	    */
char	loginname[MAXPATHLEN];	    /* used to check user    	    */
char	groupname[MAXPATHLEN];	    /* used to check group    	    */
char	filename[MAXPATHLEN+1];	    /* filename from backup    	    */
char	recovername[MAXPATHLEN];    /* result of tmpnam		    */
char	curdir[MAXPATHLEN+3];	    /* current directory (tempnam)  */
char	firstname[MAXPATHLEN+1];    /* firstname on the tape	    */
char	lastname[MAXPATHLEN+1];	    /* last name looked at	    */
char	filestatus;	    	    /* result of the backup	    */
char    msg[MAXS];		    /* used for message handling    */
int	blockstate;		    /* used to determine action	    */
int	filenum;    	    	    /* used for sanity check	    */
int	filedone;		    /* used to determine end	    */
int	obscure_file;		    /* flag for dofile behavior     */
struct	stat statbuf;	    	    /* used to set modes etc.	    */
struct	stat dotbuf;	    	    /* used to set times on "."     */
int	accessfile;		    /* check user access to file    */
int	domodes;		    /* check to perform modes	    */
int	linkflag;		    /* set if file is a link	    */
int	dircount;		    /* number of entries in a dir   */
int	newvol;			    /* flag to require a new vol    */
int 	newfd;			    /* also used by onintr for close*/
int	dotdot;			    /* for .. not pointing to parent*/
int	fwdlink;		    /* for link to unrecovered data */
long	byteoffset = 0L;	    /* used for sparse files	    */
int     fsm_move;                   /* did a FSS, nuke buf in io.c  */
#ifdef ACLS
char	*acluid;		    /* owner name for this entry    */
char	*aclgid;		    /* group name for this entry    */
int	aclmode;		    /* mode for this entry	    */
int	nacl_entries;		    /* number of entries	    */
int	curacl_entry;		    /* current entry		    */
int	goodacl_entries;	    /* flag for entry recovery	    */
struct acl_entry aclbuf[NACLENTRIES];/* holds real data for chownacl */
#endif /* ACLS */
/********************************************************************/

/********************************************************************/
/*	    VARIABLES FROM VOLHEADERS				    */
extern BKUPID certify;		    /* used to verify backup	    */
extern int    firstfile;	    /* flag for setting firstname   */
extern int firstfile_on_vol;
extern VHDRTYPE vol;		    /* volume header		    */
/********************************************************************/

extern int fsmfreq;

/*  The recognition of fields in a file header or file trailer is
 *  table driven to make addition of new fields easy to implement.
 *  The fields exist in a structure, tbl, and addresses are set up
 *  at runtime through a union, UTYPE name in initname().
 *  tbl contains entries that are either in the file header or the  
 *  file trailer. If new fields are to be added to the file header
 *  (it is strongly recommended to not add fields to the trailer since
 *  there is a restriction that a trailer cannot exceed 512 bytes),
 *  the following steps must be performed. 
 *
 *	1. add the new entry to the table tbl which is a string 
 *	   identifier and a type (note that a NULL element is 
 *	   required to be the last element of tbl).
 *	2. initname() initializes addresses for setting values.
 *	   the new field must be added to initname(). Please note
 *	   that the index for setting addresses is very important.
 *	   This is counting that the implementor must do once, is
 *	   then handled correctly by frecover.
 *	3. frecover.h must be modified to include any new types,
 *	   eg. LONG or USHORT.
 *	4. Also in frecover.h, the UTYPE union must be modified
 *	   to include any new types. This modification should be
 *	   rare since most types are already handled.
 *
 *  These instruction are sufficient for the implementor, however,
 *  if someone besides myself adds a field to the file header and
 *  finds these instructions insufficient, please feel free to add
 *  or correct them.
 */
char	activefile[MAXPATHLEN+1];

LOOKUP tbl[] = {		    /* initialized in initname	    */
    "st_size", 	    	OFF_T,
    "st_ino",   	INO_T,
    "st_mode",  	USHORT,
    "st_nlink",	    	SHORT,
    "link_to",  	LINKNAME,
    "st_dev",   	DEV_T,
    "st_rdev",  	DEV_T,
    "loginname",	CHAR,
    "groupname",	CHAR,
    "st_uid",   	UID_T,
    "st_gid",   	GID_T,
    "st_atime",	    	TIME_T,
    "st_mtime",	    	TIME_T,
    "st_ctime",	    	TIME_T,
    "st_remote",	UINT,
    "st_netdev",	DEV_T,
    "st_netino",	INO_T,
    "st_blksize",   	LONG,
    "st_blocks",    	LONG,
    "dotdot_to",	FBDOTDOT,
#ifdef CNODE_DEV
    "st_rcnode",	USHORT,
#endif /* CNODE_DEV */
#ifdef ACLS
    "nacl_entries",		INT,
    "acl_uid",		PTRCHAR,
    "acl_gid",		PTRCHAR,
    "acl_mode",		INT,
    "st_acl",		UINT,
#endif /* ACLS */
    "fwdlink",		FWDLINK,
    "",	    	    	UNKNOWN
};

#if defined(DUX) || defined(DISKLESS)
static  int     component_perm[MAXCOMPONENTS];
static  int     component_cnt;
#endif DUX || DISKLESS

struct index_list *index_short;

void freebuf();					/* forward reference */

LISTNODE *
getfield(blocktype)
int blocktype;
{
    int i = 0;
    int lastfield();
    static int first = 1;
    static int blockindex;
    static int more = 1;
    LISTNODE *head, *cur;

    if (first) {
      blockindex = blocksize;
      first = 0;
    } /* if */
    if(lastfield(blocktype, blockindex))
    	return((LISTNODE *) NULL);
    head = (LISTNODE *)fmalloc(sizeof(LISTNODE));
    head->ptr = (LISTNODE *)NULL;
    head->val = fmalloc(blocksize);
    cur = head;
    while(more) {
    	if(blockindex == blocksize) {
	    gbp = getblk();
	    if(gbp == (char *)IOERROR) {
		(void)freebuf(head);
		return((LISTNODE *)IOERROR);
	    }
	    blockindex = verblock(blocktype);
	    if(blockindex == (int)IOERROR) {
		(void)freebuf(head);
		return((LISTNODE *)IOERROR);
	    }
	}
	if(i == blocksize) {
	    cur->ptr = (LISTNODE *)fmalloc(sizeof(LISTNODE));
	    cur = cur->ptr;
	    cur->val = fmalloc(blocksize);
	    cur->ptr = (LISTNODE *)NULL;
	    i = 0;
	}
	cpbytes(cur->val, gbp, &i, &blockindex, &more);
    }
    more = 1;
    return(head);
}  /* end getfield */


setfield(start, blocktype)
LISTNODE *start;
int blocktype;
{
    int nameoffset;
    int bufindex;
    int more;
    int fileindex;
    LISTNODE *next;
    
    
    filedone = FALSE;
    switch(blockstate) {
	case FILEID:
	    switch(blocktype) {
	    	case HED:
		    filenum = atoi(start->val);
		    blockstate = FILENAME;
		    break;
		case TRL:
		    if(filenum != atoi(start->val)) {
			(void) freebuf(start);
			warn((catgets(nlmsg_fd,NL_SETN,1, "(1001): file trailer does not match file header for")));
			(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,2, "(1002): file %s")),filename);
			warn(msg);
			resync();
			return((int)IOERROR);
		    }
		    blockstate = FILESTATUS;
		    break;
	    }
	    break;
	case FILESTATUS:
	    filestatus = start->val[0];
	    blockstate = OTHER;
	    break;
	case BACKUPID:
	    if(strcmp(start->val, certify.ppid) != 0) {
		warn((catgets(nlmsg_fd,NL_SETN,3, "(1003): backup id does not match expected value")));
		sprintf (msg, catgets(nlmsg_fd,NL_SETN,61, "(1061): attempt to continue recovery with new backup id"));
		if (reply (msg) == GOOD) {
		   strncpy (certify.ppid, start->val, sizeof(certify.ppid));
		} else {
		   (void) freebuf(start);
		   resync();
		   return((int)IOERROR);
		}
	    }
	    blockstate = BACKUPTIME;
	    break;
	case BACKUPTIME:
#ifdef DEBUG_1004
fprintf(stderr, "SETFIELD - BACKUPTIME : val = %s, certify time = %s\n", start->val, certify.time);
#endif
	    if(strcmp(start->val, certify.time) != 0) {
		warn((catgets(nlmsg_fd,NL_SETN,4, "(1004): backup time does not match expected value")));
		sprintf (msg, catgets(nlmsg_fd,NL_SETN,62, "(1062): attempt to continue recovery with new backup time"));
		if (reply (msg) == GOOD) {
		   strncpy (certify.time, start->val, sizeof(certify.time));
		} else {
		  (void) freebuf(start);
		  resync();
		  return((int)IOERROR);
		}
	    }
	    blockstate = FILEID;
	    break;
	case FILENAME:
	    next = start;
	    nameoffset = 0;
	    more = 0;
	    while(next != (LISTNODE *)NULL) {
	    	bufindex = 0;
		fileindex = 0;
	    	cpbytes(&filename[nameoffset], next->val, 
    	    	    	&fileindex, &bufindex, &more);
		next = next->ptr;
		nameoffset += blocksize;
	    }
	    blockstate = OTHER;
	    break;
	case OTHER:
	    if(setval(start) == (int)IOERROR) {
		(void) freebuf(start);
		return((int)IOERROR);
	    }
    	    break;
    }
    (void)freebuf(start);
    return(0);
}



lastfield(blocktype, blockindex)
int blocktype, blockindex;
{
  static int first = 1;
    
  if(first) {
    first = 0;
    return(0);
  }
    
  if(noread)
    return(0);			/* block pushed back */


  switch(blocktype) {
  case HED:
    if( ((strcmp(&gbp[ENDOFFSET], "EOH")) == 0) &&
       (blockindex == blocksize)) {
      return(1);
    }
    break;
  case TRL:
    if( ((strcmp(&gbp[ENDOFFSET], "EOT")) == 0) &&
       (blockindex == blocksize)) {
      return(1);
    }
    break;
  }
    
  return(0);
}  /* end lastfield */

/* This routine assumes that no field is over blocksize bytes in length,
 * including the id tag. Any field longer than blocksize bytes is handeled
 * as a spearate case (see filename in setfield() and linkname() in
 * setval()for example)
 */
 
setval(start)
LISTNODE *start;
{
#ifdef ACLS
    struct passwd *pw;
    struct group *gr;
#endif /* ACLS */
    char    	id[BLOCKSIZE]; 	    /* id of field to set */
    char    	*vptr; 	    	    /* pointer to value portion */
    int	    	index; 	    	    /* location variable */
    char    	*ptr;		    /* points to value */
    LISTNODE 	*next;		    /* used to span link name */
    int		nameoffset;	    /* for copying the link name */
    int		more;		    /* used by cpbytes */
    int		bufindex;	    /* used by cpbytes */
    int		fileindex;	    /* used by cpbytes */
    
    ptr = start->val;		    /* beginning of field */
    for(index = 0; ptr[index] != ':'; index++) {
    	id[index] = ptr[index];
	if(ptr[index] == '\0')
	    break;		    /* error handeled later */
    }
    id[index] = '\0';
    vptr = ptr + index + 1;
    
    if((index = findptr(id)) < 0) {
	return(TRUE);              /* Unknow field in header or trailer is */
				   /* ignored.  Return value is positive   */
				   /* integer(TRUE) so that there is no    */
				   /* effective to other routines.         */
				   /* warnning msg only disturbs users     */
    }

    switch(tbl[index].type) {
    	case LONG:
   	    (void) sscanf(vptr, LONGFMT, name[index].longtype);
	    break;
    	case UID_T:
   	    (void) sscanf(vptr, UID_TFMT, name[index].uid_ttype);
	    break;
    	case GID_T:
   	    (void) sscanf(vptr, GID_TFMT, name[index].gid_ttype);
	    break;
	case OFF_T:
	    (void) sscanf(vptr, OFF_TFMT, name[index].off_ttype);
	    break;
	case DEV_T:
	    (void) sscanf(vptr, DEV_TFMT, name[index].dev_ttype);
	    break;
	case INO_T:
	    (void) sscanf(vptr, INO_TFMT, name[index].ino_ttype);
	    break;
	case TIME_T:
	    (void) sscanf(vptr, TIME_TFMT, name[index].time_ttype);
	    break;
	case USHORT:
    	    *(name[index].ushorttype) = atoi(vptr);
	    break;
	case SHORT:
    	    *(name[index].shorttype) = atoi(vptr);
	    break;
#ifdef ACLS
	case INT:
	    *(name[index].inttype) = atoi(vptr);
	    if(strcmp(tbl[index].field, "acl_mode") == 0) {
		aclbuf[curacl_entry].mode = aclmode;
	        curacl_entry++;
	    }
	    break;
#endif /* ACLS */
	case UINT:
    	    if(strcmp(tbl[index].field, "st_remote") == 0) {
	    	switch(vptr[0]) {
		    case '0':
		    	statbuf.st_remote = 0;
			break;
		    case '1':
		        statbuf.st_remote = 1;
			break;
		    default:
			warn((catgets(nlmsg_fd,NL_SETN,6, "(1006): illegal value for remote; bit field must be ")));
			warn((catgets(nlmsg_fd,NL_SETN,7, "(1007): zero or one")));
			break;
		}
	    }
#ifdef ACLS
    	    if(strcmp(tbl[index].field, "st_acl") == 0) {
	    	switch(vptr[0]) {
		    case '0':
		    	statbuf.st_acl = 0;
			break;
		    case '1':
		        statbuf.st_acl = 1;
			break;
		    default:
			warn((catgets(nlmsg_fd,NL_SETN,14, "(1014): illegal value for remote; bit field must be ")));
			warn((catgets(nlmsg_fd,NL_SETN,16, "(1016): zero or one")));
			break;
		}
	    }
#endif /* ACLS */
	    break;
	case CHAR:
	    (void) strcpy(name[index].chartype, vptr);
	    break;

	case PTRCHAR:
	    *(name[index].pchartype) = fmalloc(strlen(vptr)+1);
	    strcpy(*(name[index].pchartype), vptr);
#ifdef ACLS
	    if(strcmp(tbl[index].field, "acl_uid") == 0) {
		if(strcmp(acluid, (char *)NULL) == 0) {
		    aclbuf[curacl_entry].uid = ACL_NSUSER;
		    free(*(name[index].pchartype));
		    break;
		}
		if((pw = getpwnam(acluid)) == NULL) {
		    goodacl_entries = FALSE;
                    sprintf (msg, catgets(nlmsg_fd,NL_SETN,63, "(1063): Unable to create ACL: unknown user id for user %s"), acluid);
		    warn(msg);
		    free(*(name[index].pchartype));
		    break;
		}
		aclbuf[curacl_entry].uid = pw->pw_uid;
	    }
	    if(strcmp(tbl[index].field, "acl_gid") == 0) {
		if(strcmp(aclgid, (char *)NULL) == 0) {
		    aclbuf[curacl_entry].gid = ACL_NSGROUP;
		    free(*(name[index].pchartype));
		    break;
		}
		if((gr = getgrnam(aclgid)) == NULL) {
		    goodacl_entries = FALSE;
                    sprintf (msg, catgets(nlmsg_fd,NL_SETN,64, "(1064): Unable to create ACL: unknown group id for group %s"), acluid);
		    warn(msg);
		    free(*(name[index].pchartype));
		    break;
		}
		aclbuf[curacl_entry].gid = gr->gr_gid;
	    }
	    free(*(name[index].pchartype));
#endif /* ACLS */
	    break;
	case FWDLINK:
	    fwdlink = TRUE;
	    break;
	case FBDOTDOT:
	    dotdot = TRUE;
	    next = start;
	    nameoffset = 0;
	    more = 0;
	    index = strlen(id) + 1;
	    while(next != (LISTNODE *)NULL) {
		bufindex = index;
		fileindex = 0;
		cpbytes(&dotdot_to[nameoffset], next->val,
			&fileindex, &bufindex, &more);
		index = 0;
		nameoffset += fileindex;
		next = next->ptr;
	    }
	    break;
	case LINKNAME:
	    linkflag = TRUE;
	    next = start;
	    nameoffset = 0;
	    more = 0;
	    index = strlen(id) + 1;
	    while(next != (LISTNODE *)NULL) {
		bufindex = index;
		fileindex = 0;
		cpbytes(&link_to[nameoffset], next->val,
			&fileindex, &bufindex, &more);
		index = 0;
		nameoffset += fileindex;
		next = next->ptr;
	    }
	    break;
	case UNKNOWN:
	    warn((catgets(nlmsg_fd,NL_SETN,8, "(1008): unknown type in header or trailer")));
	    warn((catgets(nlmsg_fd,NL_SETN,9, "(1009): attempting to continue")));
	    break;
	default:
	    warn((catgets(nlmsg_fd,NL_SETN,10, "(1010): unknown id in header or trailer")));
	    warn((catgets(nlmsg_fd,NL_SETN,11, "(1011): attempting to continue")));
	    break;
    }
	return(0);
}

findptr(id)
char *id;
{
    int i;
    extern char *dirname();
    
    for(i = 0; strcmp(tbl[i].field, "") != 0; i++) {
    	if(strcmp(id, tbl[i].field) == 0)
	    break;
    }
    
    if(strcmp(tbl[i].field, "") == 0)
    	i = -1;

    return(i);
}

dofile()
{
    int i;
    int writeflag = TRUE;
    int special = FALSE;
    static int save;
    OBSCURE *element, *e1;
    void mkparents();

    accessfile = checkaccess();
    if(!accessfile) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,12, "(1012): unauthorized access to file %s")),filename);
	warn(msg);
	writeflag = FALSE;
	domodes = FALSE;
    }

    if(dotdot || fwdlink) {
	element = (OBSCURE *)fmalloc(sizeof(OBSCURE));
	element->o_dotdot = dotdot;
	element->o_fwdlink = fwdlink;
	(void) strcpy(element->o_cpath, filename);
	(void) strcpy(element->o_lpath, link_to);
	(void) strcpy(element->o_dpath, dotdot_to);
	e1 = o_head->o_ptr;
	o_head->o_ptr = element;
	element->o_ptr = e1;
    }

    
#if defined(DUX) || defined(DISKLESS)
#ifdef DEBUG
fprintf(stderr, "frecover: pre restore: <%s>\n", filename);
fflush(stderr);
#endif DEBUG
    (void) restore(filename,component_perm,&component_cnt);
#ifdef DEBUG
fprintf(stderr, "frecover: post restore: <%s>\n", filename);
fflush(stderr);
#endif DEBUG
#endif DUX || DISKLESS


    save = recovertype;
    if((recovertype == ABSOLUTE) && (filename[0] != '/'))
        recovertype = RELATIVE;
    switch(recovertype) {
        case NONE:
	    strcpy (curdir, dirname (filename));
	    newfd = mkfile(&writeflag, &special);
	    break;
        case FLAT:
	    if(accessfile) {
		(void) strcpy(curdir, home);
		stat(".", &dotbuf);
		newfd = mkfile(&writeflag, &special);
	    }
	    break;
        case RELATIVE:
	    if(accessfile) {
		i = chdir(home);			/* start from home */
		if(i != 0) {
		    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,13, "(1013): cannot chdir to directory %s")),home);
		    warn(msg);
		    writeflag = FALSE;
		    domodes = FALSE;
		    break;
		}
		/* anchor chdir */
		if (*filename == '/')
			(void) strcpy(curdir, ".");
		else
			(void) strcpy(curdir, "./");
		(void) strcat(curdir, dirname(filename));
		i = chdir(curdir);			/* parent directory */
		if(i != 0) {
                    /*cd failed; attempt to mkdir parents*/
                    mkparents (curdir);
                    if ((i = chdir(curdir)) != 0) { /* still can't get there*/
		      (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,15, "(1015): cannot chdir to directory %s")),curdir);
		      warn(msg);
		      writeflag = FALSE;
		      domodes = FALSE;
		      break;
                    }
		}
		stat(".", &dotbuf);
		newfd = mkfile(&writeflag, &special);
	    }
	    break;
        case ABSOLUTE:
	    if(accessfile) {
		(void) strcpy(curdir, dirname(filename));
		i = chdir(curdir);
		if(i != 0) {
		    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,17, "(1017): cannot chdir to directory %s")),curdir);
		    warn(msg);
		    writeflag = FALSE;
		    domodes = FALSE;
		    break;
		}
		stat(".", &dotbuf);
		newfd = mkfile(&writeflag, &special);
	    }
	    break;
    }

    recovertype = save;
    if((!special) && (!obscure_file))
        (void) cpfile(newfd, writeflag);

    if(writeflag)
        (void) close(newfd);
    return(TRUE);
}

cpfile(newfd, writeflag)
int newfd, writeflag;
{
    off_t cursize = 0;
    int writebytes = blocksize;
    LISTNODE *dirhead, *cur;
    int ftype;
    int nomore;

    ftype = statbuf.st_mode & S_IFMT;
    if(ftype == S_IFDIR) {
	nomore = FALSE;
	dirhead = (LISTNODE *)fmalloc(sizeof(LISTNODE));
	dirhead->ptr = (LISTNODE *)NULL;
	dirhead->val = fmalloc(sizeof(int));
	(void) strcpy(dirhead->val, BOH);
	cur = (LISTNODE *)NULL;
    }

    while(cursize < statbuf.st_size) {
    	gbp = getblk();
	if(gbp == (char *)IOERROR) {
	    (void) close(newfd);
	    (void) unlink(recovername);
	    return((int)IOERROR);
	}
	cursize += blocksize;
    	if(cursize > statbuf.st_size)
    	    writebytes = statbuf.st_size % blocksize;
	if(writeflag) {
    	    if(writefile(newfd, gbp, writebytes,
                                           cursize >= statbuf.st_size) < 0) {
		(void) close(newfd);
		if (!nflag)          /*Don't unlink DEVNULL */
		    (void) unlink(recovername);
		return((int)IOERROR);
	    }
	}
	if(ftype == S_IFDIR) {
	    dircount += dirlist(gbp, writebytes, dirhead, cur, &nomore);
	}
    }
    if(ftype == S_IFDIR) {
	if(fflag && !nflag)
	    cleandir(dirhead, cur, writeflag);
	freebuf(dirhead);
	dirhead = cur = (LISTNODE *)NULL;
    }
    return(0);
}


modes()
{
    struct utimbuf times;
    char *basename();
    char filemodes[12];
    struct passwd *pw;
    struct group *gr;
    int changeit;

    filedone = TRUE;
    changeit = TRUE;
    byteoffset = 0L;

    if(dotdot || fwdlink)
        o_head->o_ptr->o_statbuf = statbuf;

    if(!domodes)
        return;

    if(filestatus == 'B') {           /*File is flagged as bad (active)*/
        if (vflag) {
	  sprintf (msg, catgets(nlmsg_fd,NL_SETN,59, "(1059): File %s found, but active/unrecoverable during this retry"), filename);
           warn(msg);
	   strcpy(activefile, filename);
	}

	if (nflag) {
            return;
	} else {
	    (void) unlink(recovername);
	    return;
	}
    } else if ((filestatus == 'G') && vflag
				   && (strcmp(activefile, filename) == 0))
{
	  sprintf (msg, catgets(nlmsg_fd,NL_SETN,66, "(1066): File %s recovered successfully during this retry"), filename);
           warn(msg);
}
	

    if(!accessfile)
	return;

    if((pw = getpwnam(loginname)) == NULL)
	changeit = FALSE;

    if((gr = getgrnam(groupname)) == NULL)
	changeit = FALSE;

    if(vflag) {
	fillmodes(filemodes);
        (void) fprintf(stderr, "%s\t",filemodes);
	if(oflag)
	    (void) fprintf(stderr, "%s\t%s\t", user.pw_name, guser.gr_name);
	else if (changeit) {
	        (void) fprintf(stderr, "%s\t%s\t", loginname, groupname);
             } else {
	        (void) fprintf(stderr, "%d\t%d\t", statbuf.st_uid,
                                                   statbuf.st_gid);
             }
         
	(void) fprintf(stderr, "%s", filename);
	if((statbuf.st_mode & S_IFMT) == S_IFLNK)
	    (void) fprintf(stderr, " -> %s", link_to);
	(void) fprintf(stderr, "\n");
	(void) fflush(stderr);
    }

    if (nflag)   /*If we're not recovering, bail here */
      return;

    (void) strcpy(filename, basename(filename));

    if(((statbuf.st_mode & S_IFMT) == S_IFREG) && (!linkflag))
	rename(recovername, filename);

    times.actime = statbuf.st_atime;
    times.modtime = statbuf.st_mtime;

    if((statbuf.st_mode & S_IFMT) != S_IFLNK) {
        (void) utime(filename, &times);
        (void) chmod(filename, statbuf.st_mode);
    }

    /* When it comes to changing the ownership of the files (the oflag
     * is not active), frecover tries to get the current uid and gid from
     * the /etc/passwd and group files (above), but failing this will 
     * restore the files with their original ownership uid/gid.
     */

    if(changeit) {
        statbuf.st_uid = pw->pw_uid;
	statbuf.st_gid = gr->gr_gid;
    }

#ifdef ACLS
    if(statbuf.st_acl && (nacl_entries == 0))
        goodacl_entries = FALSE;

    if(goodacl_entries && statbuf.st_acl && aclflag) {
	chownacl(nacl_entries, aclbuf, user.pw_uid, guser.gr_gid,
		                       statbuf.st_uid, statbuf.st_gid);
	setacl(filename, nacl_entries, aclbuf);
    }

    if(!goodacl_entries && aclflag) {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,56, "(1056): invalid entries for file %s; ACLs not recovered.")), filename);
	warn(msg);
    }
#endif /* ACLS */

    if(!oflag) {
	(void) chown(filename, statbuf.st_uid, statbuf.st_gid);
    }

    times.actime = dotbuf.st_atime;
    times.modtime = dotbuf.st_mtime;
    (void) utime(".", &times);
}

files()
{
    while(1) {
	linkflag = FALSE;		/* assume no link */
	dotdot = FALSE;			/* assume not obscure */
	fwdlink = FALSE;		/* also not obscure */
#ifdef ACLS
	nacl_entries = curacl_entry = 0;
	goodacl_entries = TRUE;
#endif /* ACLS */

#ifdef CNODE_DEV
	statbuf.st_rcnode = CSD;	/* flag as not used */
#endif /* CNODE_DEV */
    
	if(hedtrl(HED)) {     /* expect either HED or EOB */
    		if((dofile() == TRUE) && hedtrl(TRL))
    			modes();
		else
	    		continue;
	}
    }
}  /* end files() */

initname()
{
    name[0].off_ttype = 	    &statbuf.st_size;
    name[1].ino_ttype =	    	    &statbuf.st_ino;
    name[2].ushorttype =	    &statbuf.st_mode;
    name[3].shorttype = 	    &statbuf.st_nlink;
    name[4].chartype =		    link_to;
    name[5].dev_ttype =	    	    &statbuf.st_dev;
    name[6].dev_ttype =	    	    &statbuf.st_rdev;
    name[7].chartype =		    loginname;
    name[8].chartype = 	    	    groupname;
    name[9].uid_ttype =		    &statbuf.st_uid;
    name[10].gid_ttype = 	    &statbuf.st_gid;
    name[11].time_ttype =   	    &statbuf.st_atime;
    name[12].time_ttype = 	    &statbuf.st_mtime;
    name[13].time_ttype = 	    &statbuf.st_ctime;
/* Currently, this is the only bit field. The compiler will not
 * allow pointers to a bit filed. So st_remote is hard wired at
 * conversion time.
 */
/*  name[14].uinttype = 	    &statbuf.st_remote; */
    name[15].dev_ttype = 	    &statbuf.st_netdev;
    name[16].ino_ttype = 	    &statbuf.st_netino;
    name[17].longtype =     	    &statbuf.st_blksize;
    name[18].longtype =	    	    &statbuf.st_blocks;
    name[19].chartype =		    dotdot_to;
#if defined(CNODE_DEV) && defined(ACLS)
    name[20].ushorttype =	    &statbuf.st_rcnode;
    name[21].inttype = 		    &nacl_entries;
    name[22].pchartype =	    &acluid;
    name[23].pchartype =	    &aclgid;
    name[24].inttype = 		    &aclmode;
    name[25].chartype = 	    NULL;
#endif	/* CNODE_DEV && ACLS */
#if defined(CNODE_DEV) && !defined(ACLS)
    name[20].ushorttype = 	    &statbuf.st_rcnode;
    name[21].chartype =		    NULL;
#endif	/* CNODE_DEV && !ACLS */
#if !defined(CNODE_DEV) && defined(ACLS)
    name[20].inttype = 		    &nacl_entries;
    name[21].pchartype =	    &acluid;
    name[22].pchartype =	    &aclgid;
    name[23].inttype = 		    &aclmode;
    name[24].chartype = 	    NULL;
#endif	/* !CNODE_DEV && ACLS */
#if !defined(CNODE_DEV) && !defined(ACLS)
    name[20].chartype = 	    NULL;
#endif /* !CNODE_DEV && !ACLS */
}  /* end initname () */


hedtrl(trltype)
int trltype;
{
    int setfield();
    LISTNODE *getfield(), *start;
    OBSCURE *e;
    
    while((start = getfield(trltype)) != (LISTNODE *)NULL) {
	if((start == (LISTNODE *)IOERROR) || 
	   (setfield(start, trltype) == (int)IOERROR)) {
	    if(fwdlink || dotdot) {		/* remove the node */
		e = o_head->o_ptr;
		o_head->o_ptr = o_head->o_ptr->o_ptr;
		free((char *)e);
	    }
	    return(0);
	}
    }
    return(1);
}  /* end hedtrl */

int
verblock(blocktype)
int blocktype;
{
    BLKID verify;

#ifdef DEBUG
fprintf(stderr, "block is a %s\n", gbp);
#endif

    if(blocktype == HED) {
	if(strcmp(gbp, BOH) == 0) {
            blockstate = BACKUPID;
	    if(blkcksum() == (int)IOERROR) {
		warn((catgets(nlmsg_fd,NL_SETN,20, "(1020): block checksum mismatch")));
		resync();
		return((int)IOERROR);
	    }
	}
	else if (strcmp(gbp, EOB) == 0) {
	  done(0);
	}
    }
    else {
	if(strcmp(gbp, BOT) == 0) {
            blockstate = FILEID;
	    if(blkcksum() == (int)IOERROR) {
		warn((catgets(nlmsg_fd,NL_SETN,21, "(1021): block checksum mismatch")));
		resync();
		return((int)IOERROR);
	    }
	}
    }
    return(sizeof(verify));
}



void
freebuf(start)
LISTNODE *start;
{
    LISTNODE *next;

    while(start != (LISTNODE *)NULL) {
   	next = start->ptr;
    	free(start->val);
    	free(start);
    	start = next;
    }
}



/*  This function calculates a checksum for a block. 
    The checksum is done on an integer (32 bit) basis. 
    for future revisions, a checksum is should also be computed
    on a character (8-bit) basis, since adding a block of 32-bit numbers, 
    storing the result in a 32-bit number is a bit risky (note that this
    second bit of checksumming has been left out until the time when it may 
    be needed, so as not to incur additional additions).  
    This number is then compared to the original value.
    If the checksum matches, the routine will return true (value of 0).
*/

blkcksum()
{
    int lim1, checkval, sum1 = 0;
    int n;
    int *intptr;
    char *chptr;
    char tmp[100];
    PBLOCKTYPE blk;

    blk.ch = gbp;
    chptr = blk.id->checksum;
    checkval = atoi(chptr); 

    (void) sscanf(chptr, "%d", &n); 
    lim1 = sizeof(blk.id->checksum);

    while (lim1--)
	*chptr++ = '\0';

    intptr = blk.integ;
    chptr  = blk.ch;

    lim1= blocksize/sizeof(int);

    while (lim1 > 0) {
      sum1 += *intptr++;
      lim1--;
    }

    if (sum1 == checkval) {         /*Checksum is okay*/
        chptr = blk.id->checksum;
        (void) sprintf(chptr, "%d", checkval);
        return(0);
    }
    
    /* If we reach this point, then the checksum didn't match */
    return((int)IOERROR);
}


#define ACCESS 0777
#define READOTHER 0004
#define READGROUP 0040
#define READOWNER 0400
checkaccess()
{
    if(user.pw_uid == ROOT || nflag)	/* if root or not recovering then ok */
        return(TRUE);

#ifdef ACLS
    if(statbuf.st_acl && goodacl_entries)
        return(getaccess());		/* same algorithm as iget_access */
#endif /* ACLS */

    if(statbuf.st_mode & READOTHER)	/* if all read then ok */
        return(TRUE);

    if(statbuf.st_mode & READGROUP) {	/* check group access */
        if(strcmp(groupname, guser.gr_name) == 0)
	        return(TRUE);		/* groups match and read access */
    }

    if(strcmp(user.pw_name, loginname) == 0)
        return(TRUE);			/* owners match */

    return(FALSE);
}

mkfile(writeflag, special)
int *writeflag, *special;
{
    char name[MAXPATHLEN];		/* ./basename of filename */
    int specres;			/* result of makespecial */

    domodes = TRUE;			/* assume success */
    (void) strcpy(name, "./");			/* anchor name */
    (void) strcat(name, basename(filename));   /* get file name */

    if(linkflag) {			/* links to existing file */
	*writeflag = FALSE;		/* none to write */
	linkfiles(link_to, name);	/* link the files, if possible */
	return(NOWRITE);
    }

    switch(statbuf.st_mode & S_IFMT) {
        case S_IFDIR:
	    *writeflag = FALSE;
	    return(makedir(name, statbuf.st_mode));
	case S_IFNWK:
	    *writeflag = FALSE;
	    *special = TRUE;
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,28, "(1028): Skipping obsolete network special file")));
	    warn(msg);
	    return(NOWRITE);
	case S_IFCHR:
	case S_IFBLK:
	case S_IFIFO:
	case S_IFSOCK:
	    *writeflag = FALSE;
	    *special = TRUE;
	    if(hflag) {
		domodes = FALSE;
		return(NOWRITE);
	    }
#ifdef CNODE_DEV
	    specres = makespecial(name, (int)statbuf.st_mode, statbuf.st_rdev,
				  statbuf.st_rcnode);
#else
	    specres = makespecial(name, (int)statbuf.st_mode, statbuf.st_rdev);
#endif
	    return(specres);
	case S_IFREG:
	case S_IFLNK:
	    if(hflag) {
		*writeflag = FALSE;
		domodes = FALSE;
		return(NOWRITE);
	    }
	    *writeflag = TRUE;
	    return(makefile(name, (int)statbuf.st_mode,
			    statbuf.st_size, writeflag));
    }
    return(NOWRITE);			/* never executed. shut up lint */
}


/* mkparents is called in the extreme case where a file under a non-existant
 * parent path needs to be recovered.  It returns nothing, and relies on the
 * calling function to follow-up with a chdir to see if the directory is
 * really usable at that point.
 */
void
mkparents (name) 
  char *name;
{
  struct stat stbuf;
  char part[MAXPATHLEN+1];
  int i=0;

  while (*name != '\0') {

    /*copy the next part of the path into the temp string*/
    while (*name != '/' && *name != '\0')
      part[i++] = *name++;

    if (i == 0)  /*just in case an unaccounted-for string is passed*/
      continue; 

    part[i]='\0';  /*null-terminate the temp string */

    if (stat(part, &stbuf)) {  /*if stat == 0, the dir already exists   */
      makedir (part, 0777);   /*if stat != 0, create the new directory */
    }

    part [i++] = *name++;
  }
}

makedir(name, mode)
char *name;
int mode;
{
    int		mkdir();
#if defined(DUX) || defined(DISKLESS)
    char        *p;
#endif DUX || DISKLESS

    dircount = 0;		    /* number of entries on media */
    if(recovertype == FLAT) {
	domodes = FALSE;
	return(NOWRITE);
    }

    if(nflag)                       /*Not actually recovering: don't mkdir*/
	return(NOWRITE);

#if defined(DUX) || defined(DISKLESS)
    p = &name[strlen(name)-1];
    if ( component_perm[component_cnt] && *p == '+' )
	*p = '\0';
#endif DUX || DISKLESS


    if ( mkdir(name, mode) == -1 ) {  
#if defined(DUX) || defined(DISKLESS)

	/* If we're making a CDF, but the filename already exists, try
	   to create the directory, then move the current file to the
	   standalone context under the new directory.
	 */
	if ( component_perm[component_cnt] && errno == EEXIST ) {
	    char linkname[MAXPATHLEN+1];

	    strcat(name,"+");

	    if ( mkdir(name, mode) == -1 ) 
		return mkdirfail(name);

	    (void) sprintf (linkname, "%s/standalone", name);

	    name [ strlen(name) -1] = '\0';   /* Get rid of the trailing + */

            /* Move old file to it's new home, and rename it without the + */
            if (link (name, linkname) != 0 || unlink (name) != 0) {
	      (void) sprintf(msg, catgets(nlmsg_fd,NL_SETN,29, "(1029): cannot change existing file %s to a CDF\n"),name);
	      warn (msg);
	    }

	    (void) sprintf (linkname, "%s+", name);
            if (rename (linkname, name) != 0) {
	      (void) sprintf(msg, catgets(nlmsg_fd,NL_SETN,30, "(1030): error (%s) renaming directory to %s\n"), strerror(errno), name);
	      warn (msg);
	    }
	} else 
#endif DUX || DISKLESS
	return mkdirfail(name);
    }

#if defined(DUX) || defined(DISKLESS)
    if ( component_perm[component_cnt] ) {
	struct      stat    stbuf;

	if ( statcall(name,&stbuf) == -1 ) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,22, "(1022): unable to stat file %s")),name);
	    warn(msg);
	}
	if ( chmod(name,stbuf.st_mode|04000) == -1 ) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,53, "(1053): unable to chmod file %s")),name);
	    warn(msg);
	}
	strcat(name,"+");
    }
#endif DUX || DISKLESS

    return(NOWRITE);		/* dir created, but don't write */
}


mkdirfail(name)
    char *name;
{
    struct	stat	stbuf;

    if (errno == EEXIST) {      /* file exists */
	if (statcall(name, &stbuf) == -1) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,18, "(1018): unable to stat file %s")),name);
	    warn(msg);
	    domodes = FALSE;
	    return(NOWRITE);
	}
	if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,23, "(1023): file %s exists but is not a directory")), name);
	    warn(msg);
	    domodes = FALSE;
	    return(NOWRITE);
	}
	domodes = FALSE;        /* don't change modes on dirs */
    }
    else {
	(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,24, "(1024): can't create directory %s")), name);
	warn(msg);
	domodes = FALSE;
	return(NOWRITE);
    }
    return(NOWRITE);		/* dir created, but don't write */
}


#ifdef CNODE_DEV
makespecial(name, mode, majmin, rcnode)
char	*name;
int	mode;
dev_t	majmin;
site_t	rcnode;
#else
makespecial(name, mode, majmin)
char	*name;
int	mode;
dev_t	majmin;
#endif /* CNODE_DEV */
{
    struct stat sbuf;
    int mknodres;

    if (nflag)                          /*Not recovering: bail at start */
	return(NOWRITE);

    if(statcall(name, &sbuf) == 0) {
	if((statbuf.st_mtime <= sbuf.st_mtime) && (!overwrite)) {
	    domodes = FALSE;
	    return(NOWRITE);
	}
	if (unlink(name) == -1) {	/* other than file doesn't exist */
	    if (errno != ENOENT) {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,25, "(1025): can't unlink file %s")),name);
		warn(msg);
		domodes = FALSE;
		return(NOWRITE);
	    }
	}
    }

	/*
	 *	Finally do the mknod(2).
	 */
    switch(mode & S_IFMT) {
        case S_IFIFO:
	    if (mknod(name, ((int)mode & S_IFMT), 0) == -1) {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,26, "(1026): can't mknod file %s")),name);
		warn(msg);
		domodes = FALSE;
		return(NOWRITE);
	    }
	    break;
	case S_IFSOCK:
	    break;
	case S_IFBLK:
	case S_IFCHR:
	default:
#ifdef CNODE_DEV
	    if(statbuf.st_rcnode == CSD) 	/* rcnode not set use CSD */
	        mknodres = mknod(name, (int)mode, (int)majmin);
	    else
	        mknodres = mkrnod(name, (int)mode, (int)majmin, (int)rcnode);
#else
	    mknodres = mknod(name, (int)mode, (int)majmin);
#endif /* CNODE_DEV */
	    if(mknodres == -1) {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,27, "(1027): can't mknod file %s")),name);
		warn(msg);
		domodes = FALSE;
		return(NOWRITE);
	    }
	    break;
    }
    return(NOWRITE);
}


makefile(name, mode, size, writeflag)
char	*name;
int	mode;
off_t size;
int	*writeflag;
{
    int i;
    int mkname();
    int fd;
    struct stat sbuf;
    int unlinkfile = FALSE;

    if (nflag) {      /*If we're not recovering, write the file to /dev/null*/
	if ((fd = open (DEVNULL, O_WRONLY)) == -1) {
	    (void) sprintf(msg, catgets(nlmsg_fd,NL_SETN,58, "(1058): unable to write to %s"), DEVNULL);
            warn(msg);
	    *writeflag = FALSE;
	    return(NOWRITE);
	}

	return (fd);
    }
	
    if(statcall(name, &sbuf) == 0) {
	if(overwrite) {
	    (void) unlink(name);
	    unlinkfile = TRUE;
	    i = 0;
	}
	else {
	    i = open(name,O_RDWR,sbuf.st_mode & 07777);
	}
	if(i < 0) {
	    domodes = FALSE;
	    *writeflag = FALSE;
	    if((sbuf.st_mtime < statbuf.st_mtime) || (overwrite)) {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,32, "(1032): %s - can't open")),filename);
		warn(msg);
	    }
	    return(NOWRITE);
	}
	if(!unlinkfile)
	    (void) close(i);
	if((statbuf.st_mtime <= sbuf.st_mtime) && (!overwrite)) {
	    domodes = FALSE;
	    *writeflag = FALSE;
	    return(NOWRITE);
	}
    }

    if(mkname(recovername) < 0) {
	domodes = FALSE;
	*writeflag = FALSE;
	return(NOWRITE);
    }

    /*
     * 	Create or truncate file.
     * 	NOTE: creat truncates an existing file
     */

    if ((fd = creat(recovername, mode)) == -1) {
        (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,33, "(1033): can't create file %s")), filename);
	warn(msg);
	domodes = FALSE;
	*writeflag = FALSE;
	return(NOWRITE);
    }
	
    return(fd);
}

char savename[MAXPATHLEN]; 		/* used by linkname and mkname */

mkname(recovername)
char *recovername;
{
    char *tempnam(), *p;

    if((p = tempnam("./", (char *)NULL)) == NULL) {
	(void) sprintf(msg, "%s%s%s",(catgets(nlmsg_fd,NL_SETN,35, "(1035): cannot creat temporary file name,")),
		(catgets(nlmsg_fd,NL_SETN,36, "(1036):  skipping file ")),filename);
	warn(msg);
	return(-1);
    }

    (void) strcpy(recovername, "./");
    (void) strcpy(savename, p);
    (void) strcat(recovername, basename(savename));
    (void)free(p);
    return(0);
}


/* link p2 to p1 */

linkfiles(p1, p2)
char *p1, *p2;
{
    struct stat sbuf;
    int res;

    if(fwdlink) {
	domodes = FALSE;
        return;
    }
    
    if(nflag) {
        return;
    }

    switch(recovertype) {
        case FLAT:
	    (void) strcpy(savename, basename(p1));
	    break;
	case RELATIVE:
	    (void) strcpy(savename, home);
	    (void) strcat(savename, "/");
	    (void) strcat(savename, p1);
	    break;
	case ABSOLUTE:
	    (void) strcpy(savename, p1);
	    break;
    }

    if(lstat(p2, &sbuf) == 0) {  	/* link already exists */
        if (((sbuf.st_mode & S_IFMT == S_IFLNK)
	     && (statbuf.st_mtime > sbuf.st_mtime)) || (overwrite)) {
	    res = unlink(p2);			/* get newer version */
	    if(res != 0) {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,52, "(1052): cannot unlink %s")),p2);
		warn(msg);
		domodes = FALSE;
		return;
	    }
	}
	else {
	    domodes = FALSE;		/* don't want this copy */
	    return;
	}
    }
    switch(statbuf.st_mode & S_IFMT) {
        case S_IFLNK:
#ifdef SYMLINKS
	    if(symlink(p1, p2) != 0) {	/* don't change pathname of p1 */
	        domodes = FALSE;	/* for symbolic links */
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,37, "(1037): cannot create link between %s and %s: %s")),p1, p2, strerror(errno));
		warn(msg);
	    }
#else
	    warn((catgets(nlmsg_fd,NL_SETN,38, "(1038): system does not support symbolic links")));
	    domodes = FALSE;
#endif /* SYMLINKS */
	    break;
	default:
	    if(statcall(savename, &sbuf) == 0) {
		if((statbuf.st_mtime > sbuf.st_mtime) &&
		   (!overwrite)) {
		    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,39, "(1039): backed up version of %s is newer")),p2);
		    warn(msg);
		    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,40, "(1040): recover the link %s first")),p1);
		    warn(msg);
		    domodes = FALSE;
		}
		else {
		    if(link(savename, p2) != 0) {
			domodes = FALSE;
			(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,41, "(1041): cannot create link between %s and %s")),p1, p2);
			warn(msg);
		    }
		}
	    }
	    else {
		(void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,42, "(1042): cannot stat link file %s")), savename);
		warn(msg);
		domodes = FALSE;
	    }
	    break;
	}
}

/* skip to end of this file.  point to trailer.
 */
void skipfile()
{
    int n = (statbuf.st_size + blocksize - 1) / blocksize;
#ifdef DEBUG_SKIP
fprintf(stderr, "files.c:skipfile: size = %d, n = %d\n", statbuf.st_size, n);
fflush(stderr);
#endif

    filedone = TRUE;
    while (n--) {
	gbp = getblk();	/* skip contents of file */
#ifdef DEBUG_SKIP
fprintf(stderr, "files.c:skipfile: n = %d\n", n);
fflush(stderr);
#endif
    }
}  /* end skipfile() */



/* Reads the current header block(s) from the input device,
 * and parses the header, updating global variables.
 * It returns 0 for a normal header read, and 1 to indicate there
 * was a problem reading the header */
int
parse_header()
{
  getnexthdr(!RESYNC);        /* position to next file header */
  linkflag = FALSE;   /* assume no link */
  dotdot = FALSE;             /* assume .. points to parent */
  fwdlink = FALSE;    /* assume the data is here */
#ifdef ACLS
  nacl_entries = curacl_entry = 0;
  goodacl_entries = TRUE;
#endif /* ACLS */
#ifdef CNODE_DEV
  statbuf.st_rcnode = CSD;/* flag as not set */
#endif /* CNODE_DEV */

  if(!hedtrl(HED)) {
    if (hedtrl(EOBU)) {
      done(0);
    }
    warn((catgets(nlmsg_fd,NL_SETN,43, "(1043): error in header on recover")));
      return (1);
  }

  return(0);
}



/* Return values for fs_position (see below)*/
#define NOTSEARCHED 0
#define SEARCHED    1
#define DONTSEARCH  2

/*Given the current position, this routine decides whether or not to
 *do a fastsearch (based on how close the file is to our current position),
 *performs the fastsearch, and returns SEARCHED if a fastsearch was performed,
 *and NOTSEARCHED if no fastsearch was performed, and DONTSEARCH if further
 *fastsearches should not be performed until after the tape is changed*/
int
fs_position (at, want, offset) 
  int at,      /*Sequence number of file where we are*/
      want,    /*Sequence number of file where we want to be*/
      offset;  /*Current calculation of how far off the setmarks are, such
                *that a file following a setmark is actually offset files
                *from an even multiple of fsmfreq*/
{
  struct vdi_gatt gatt_buf; /*Status buffer for tape information*/
  int fs_distance;          /*The number of setmarks seperating at from want*/
  static int fs_lower_bound = 0; /*Do not fast search to a file with a lower
      			          *sequence number.  This prevents fs looping
				  *by preventing fs forward after a fs back */
  

  /* First off, if we are looking at the first file on a tape, stop
   * right there:  I don't care where you want to be, don't fastsearch
   * until you've read that first file, so that the "at" (current_filenum)
   * can be correctly calibrated.
   */

  if (firstfile)
    return (NOTSEARCHED);

  /*Calculate the number of setmarks to travel, extra parens
   *to force integer rounding, in case of haphazard optimization */

  fs_distance = ((want - offset)/fsmfreq) - ((at - offset)/fsmfreq);

  /* If we need to move backwards, we need to back up one more mark than
   * calculated in fs_distance, since we want to go to the beginning of that
   * set, not the end of it.
   */
  if (want < at)
    fs_distance--;

  if (fs_distance == 0 || (want <= fs_lower_bound && fs_distance > 0)) {
    /*We are already where we want to be, don't fastsearch*/
    return (NOTSEARCHED);
  } 

  /* Note that when fastsearching, we ignore EIO, because some
   * workstation drivers report an error when fastsearching past the tape
   * limits, but the 800 and 300 drivers don't
   */
  if (fs_distance < 0) {   /*We've passed the desired file, go back*/

    fs_lower_bound = at;          /*Don't fastsearch forward to here again*/

    if (vdi_ops (outfiletype, fd, VDIOP_BSS, -fs_distance) < 0
					 && vdi_errno != EIO) {  /*error*/
      warn (catgets(nlmsg_fd,NL_SETN,57, "(1057): tape drive error during fastsearch mark positioning"));
    } 

    /* Successful backward search */
    return (SEARCHED);

  } else {  /*(fs_distance > 0): should fastsearch forward */ 
    /*fs forward or to last set on tape*/

    if (vdi_ops (outfiletype, fd, VDIOP_FSS, fs_distance) < 0 
					 && vdi_errno != EIO) {   /*error*/
      warn (catgets(nlmsg_fd,NL_SETN,57, "(1057): tape drive error during fastsearch mark positioning"));
    }

    /*check to see if we're at the end of media (searched too far)*/
    if (vdi_get_att(outfiletype, fd, VDIGAT_ISEOD | VDIGAT_ISEOM,
                                               &gatt_buf) == VDIERR_ACC){
      panic (catgets(nlmsg_fd,NL_SETN,57, "(1057): tape drive error during fastsearch mark positioning"), !USAGE);
    }
    
    if (gatt_buf.eod || gatt_buf.eom) {
    /*Strategy: backup one set mark, and don't fs forward until next tape*/

      if (vdi_ops (outfiletype, fd, VDIOP_BSS, 1) < 0 && vdi_errno != EIO) {
        warn (catgets(nlmsg_fd,NL_SETN,57, "(1057): tape drive error during fastsearch mark positioning"), !USAGE);
      }

      /*Either a successful backspace or we're at BOT: don't fs forward
       *again until the next tape point*/
      return (DONTSEARCH);

    } else { /* Successful simple forward search */
      return (SEARCHED);
    }
  }
}


/*Given a pathname to search for and our current position in the list,
 *return the list node that contains the given pathname in the main index.
 *It is assumed that the file's node is close to the currentp's node.
 *If the path is not found in the index, NULL is returned*/

struct index_list *
index_find (path, currentp)
  char *path;
  struct index_list *currentp;
{

  /*If it's actually before our current spot, go to the start of the index*/
  if (mystrcmp(path, currentp->path) < 0  ||  currentp == NULL)
    currentp = index_head;

  while (currentp != NULL) {
    if (strcmp(path, currentp->path) == 0)  /*Found it*/
      return (currentp);

    currentp = currentp->next;
  }

  return (NULL);
}


void
somefiles()
{
  int current_filenum = 0;
  int cal_filenum = 0;
  int fs_offset = 0,   /*The number of files the set marks are currently off*/
      fs_done=SEARCHED,/*A flag to indicate if we fs'd to the current file  */
      old_fs_done;
  struct index_list *tape_in; /*Pointer to the file now being read from tape */
  struct index_list *want_in; /*Pointer to the wanted file in the short indx */
  struct index_list *first_wanted; /*Pointer to the first file wanted        */
  int ret;        /*Temp storage for return values*/

  struct index_list *next_possible();  

  removedup();			        /* remove duplicate list entries */
  want_in = next_possible(index_head);  /* reudce possible index matches */
  tape_in = index_head;
  
  first_wanted = want_in;

  while((ilist->ptr != (LISTNODE *)NULL) || (strcmp(ilist->val, "NULL") == 0)) {
    current_filenum++;

    if (do_fs   &&   fs_done != DONTSEARCH   &&  !firstfile  &&
	want_in != (struct index_list *) NULL) {
      do {
        old_fs_done = fs_done;

        /* Move to where we think we should be on the tape. 
         * Note that we only allow the possibility of a backward search
         * in the event we had done a search the last time through.
         * If we had not searched to find the previous file, then
         * there should be no need to search backward.
         */
        if (current_filenum < want_in->num || 
                           fs_done == SEARCHED || fs_done == DONTSEARCH) {
          fs_done = fs_position(current_filenum, want_in->num, fs_offset);   
        } else {
          fs_done = NOTSEARCHED;
        }

        if (fs_done == SEARCHED  ||  fs_done == DONTSEARCH) {
          fsm_move = TRUE;    /* Tape was fastsearched: alert getblk() to */
          noread   = FALSE;   /* discard buffer and read from input */
        }

        /*Read-in the next header, and update global variables*/
	
	if (parse_header()) {
          current_filenum++;
          continue;                /*In the event of an error, skip the file*/
        }

	/*Find where we really are in index*/
	tape_in = index_find(filename, tape_in);

        /* If we find our current file in the index, then keep current_filenum
         * correctly synchronized.  If a file exists on the tape that is not
	 * in the index, then ether the data or the index is not kosher, so
	 * disable fastsearch so that we don't get into tape-positioning prob's
	 */
        if (tape_in != NULL) { 
          current_filenum = tape_in->num;
	} else {
          do_fs = FALSE;
	  break;        /*Break out of the while loop to stop fs checking   */
        }

	if (fs_done == SEARCHED ||        /*  If fs was used to reposition, */
	    fs_done == DONTSEARCH) {      /*  we need to adjust the offset  */
          fs_offset = current_filenum % fsmfreq;
        }

        /* If we've started searching backward, the SEARCHED flag will be 
         * set, and will cause the while loop to continue until the backward
         * fastsearch is complete.  After that, fs_done will be set to 
         * NOTSEARCHED, but we need to reset it to DONTSEARCH which will
         * prevent further fastsearching until the next volume is mounted.
         */
	if (fs_done == NOTSEARCHED) {
	  if (old_fs_done == DONTSEARCH)  /*Retain don't search state*/
            fs_done = DONTSEARCH;

	  break;        /*Break out of the while loop to stop fastsearching*/
	}

	if (old_fs_done == DONTSEARCH)  /*Retain don't search state*/
	  fs_done = DONTSEARCH;

      /*Keep looping while we're more than fsfreq files between here and 
       *the file (i.e. there are more fastsearch marks between here and the
       *desired file), or if we've fs'd past the desired file. */

      } while (!firstfile && want_in->num < current_filenum ||
          (fs_done != DONTSEARCH && want_in->num - current_filenum > fsmfreq));

    } else {

      /*Read-in the next header, and update global variables*/
      if (parse_header())
        continue;         /*In the event of a header error, skip the file*/

      /* Find where we really are in index.  This function call may seem
       * redundant, but is necessary in the corner cases where we are not
       * currently fastsearching (but were/will be).  This will keep the 
       * current_filenum pointer in sync at all times.
       */
      if (do_fs) {
        tape_in = index_find(filename, tape_in);

        if (tape_in != NULL) {        /*Update / correct the current_filenum*/
          current_filenum = tape_in->num;
        } else {
          do_fs = FALSE;
        }
      }
    }

    /* When we change media/reels, we need to reset some of the pointers
     * for fastsearching, as there is the possibility that the user has
     * mounted an earlier tape from the sequence.  Also, if we had temporarily
     * stopped fastsearching, due to reading at the end of the tape, we 
     * need to clear the DONTSEARCH flag, so that fastsearch can begin again.
     */
    if (firstfile) {
      fs_done=NOTSEARCHED;       /* clear DONTSEARCH flag */
      want_in = first_wanted;    /* Also reset the "wanted" list pointer */ 
      (void) strcpy(firstname, filename);
      firstfile = FALSE;
    }

    if (do_fs) { 
    /* If we're keeping track, increment want_in to point to the next desired
     * file.  Note that the following works only because the "want" and main
     * indices are in fact the same list, and the backup is always
     * searched in one direction, except for fastsearch error correction
     * and the case where a new tape may be loaded out of sequence,
     * which update current_filenum correctly before ever getting here.
     */

      while (want_in != NULL  &&  want_in->num <= current_filenum) {
        want_in = next_possible (want_in->next);
      }
    }

    (void) strcpy(lastname, filename);

    if(wantfile())
      recoverfile();
    else if (ilist->ptr != (LISTNODE *)NULL && !newvol)
      skipfile();
  }
}  /* end somefiles() */


wantfile()
{
    LISTNODE *t1, *t2, *t3;
    int res;				/* result of match */

    marklists();			/* mark exact matches */
 
    /* Search the exclude list.  Let res = TRUE mean that we don't
     * want this file recovered.
     */
    res = FALSE;  /*Assume we want it*/
    for(t1 = elist->ptr; t1 != (LISTNODE *)NULL; t1 = t1->ptr) {
      if (match(filename, t1->val) == RECOVER) {
       /* At this point, we seem to want this file excluded from
	* the recovery, but we need to make sure that there is not
	* some included file with a longer name which will want this
	* file recovered.  
	*/
	res = TRUE;   /*Assume we don't want it*/

	for (t2 = ilist->ptr; t2 != NULL; t2 = t2->ptr) {
	  /* If there exists an entry in the include lists which is
	   * "recovered" under the current excluded entry, then
	   * see if this file would need to be recovered under that include
	   * entry.  If so, mark it, and don't exclude the file here.
	   */
	  if ( match (t2->val,  t1->val) == RECOVER  &&
	       match (filename, t2->val) == RECOVER ) {
	    res = FALSE;
	  } 
	}

	if (res == TRUE)  /* We still don't want it after checking the ilist */
	  return (FALSE); /* Return that we don't want the file              */
      }
    }

    t3 = ilist;				/* ckeck include list */
    t1 = ilist->ptr;
    t2 = t1->ptr;
    if(strcmp(ilist->val, "NULL") == 0)	/* get all files except -e */
        return(TRUE);
    while(t1 != (LISTNODE *)NULL) {
	res = match(filename, t1->val);
	switch(res) {
	    case SUBSET:
	        if((statbuf.st_mode & S_IFMT) == S_IFDIR) {
		    markmatches(t1);
		    return(TRUE);	/* recover leading path */
		}
		break;
	    case RECOVER:
		if(((t1->ftype & S_IFMT) == S_IFDIR) ||
		   (strcmp(filename, t1->val) == 0)) {
		    markmatches(t1);
		    return(TRUE);	/* recover subsequent files */
		} else if (mystrcmp(filename, t1->val) > 0) {
                  /* This code is to handle the special case where we've
                   * found a file that should be recovered, but its included
		   * directory has not been read.  We need to warn about the
		   * problem, mark the ilist node (so as not to deal with
		   * this again), and allow the file to be recovered
		   */

                   sprintf (msg, catgets(nlmsg_fd,NL_SETN,60, "(1060): %s itself not on media;\n\tcontinuing with %s and following files"),
			t1->val, filename);
                   warn(msg);
		   t1->aflag  = RECOVER;
		   t1->ftype  = S_IFDIR;
		   return(TRUE);	/* recover subsequent files */
		}
		break;
	    default:
		break;
	}
		
	if(t1->aflag == RECOVER &&
           (strlen(filename) < strlen(t1->val) ||
            strncmp (t1->val, filename, strlen(t1->val)) != 0)) {
                                        /* remove element from list */
	    t3->ptr = t2;		/* since we're past this */
	    free(t1->val);		/* element alphabetically */
	    free((char *)t1);
	    t1 = t3;
	}
	t3 = t1;			/* check the next entry */
	t1 = t2;
	if(t2 != (LISTNODE *)NULL)
	    t2 = t2->ptr;
    }
    /* if we are dealing with tapes made on 3.0 or 3.1 or 6.5 do not
       do the onmedia.  The reason is that tapes from these releases
       can be wrong, by wrong we mean that the files are not in the
       assumed alphabetical order.  An onmedia call could result in 
       skipping to a new media and missing some files.  This is a 
       patch for 7.0 frecover to be fixed better in 8.0 release.
    */
    res = (mystrcmp(vol.release, RELEASE30) && 
	   mystrcmp(vol.release, RELEASE31) &&
	   mystrcmp(vol.release, RELEASE65));
    if(res != 0) {
      onmedia();		/* check to see if we need a new vol */
  }
					       
    return(FALSE);			/* skip this file (no match) */
}

recoverfile()
{
    	if(dofile() != TRUE) {
	    warn((catgets(nlmsg_fd,NL_SETN,44, "(1044): error in file recovery")));
	    return;
	}
	if(!hedtrl(TRL)) {
	    warn((catgets(nlmsg_fd,NL_SETN,45, "(1045): error in trailer recovery")));
	    {
			struct stat sb;

			if (stat(recovername, &sb) == 0)
				unlink(recovername); /* cleanup */
	    }
	    return;
	}
    	modes();
}


/* Remove entries that are on both the ilist and elist. The entries are
 * removed only from the ilist.
 */
removedup()
{
    LISTNODE *t1, *t2;				/* temporary pointers */

    temp = elist->ptr;
    while(temp != (LISTNODE *)NULL) {
	t1 = ilist->ptr;
	t2 = ilist;
	while(t1 != (LISTNODE *)NULL) {
	    if(strcmp(temp->val, t1->val) == 0) {   /* remove duplicates */
		t2->ptr = t1->ptr;
		free(t1->val);
		free((char *)t1);
		break;
	    }
	    t2 = t1;
	    t1 = t1->ptr;
        }
	temp = temp->ptr;
    }
}  /* end removedup() */


getnexthdr(syncflag)
int syncflag;
{
  filedone = TRUE;			/* skipped trailer */
  
  while(TRUE) {
    if(syncflag && aflag) {
      if(write(residfd, gbp, blocksize) != blocksize) {
	warn((catgets(nlmsg_fd,NL_SETN,46, "(1046): write error to residual file")));
      }
    }

    if ((gbp = getblk()) == (char *)IOERROR) {
      panic(catgets(nlmsg_fd,NL_SETN,65, "(1065): read error on file header"), !USAGE);
    }
    
    if(((strcmp(gbp, BOH) == 0) && (blkcksum() == 0)) || (strcmp(gbp, EOB)  == 0) ) {
      UNGETBLK();
      return;
    }
  }
}  /* end getnexthdr() */


/* match returns FALSE if there is no match between s1 and s2.
 * if s1 is a subset of s2, match returns SUBSET.
 * if s1 is a superset of, or equal to, s2 match returns RECOVER
 *
 * extended by danm & kawo to fix defect where more than was
 * asked for was restored.  The problem was that partial substring
 * matches without checking for directory components were done.
 * see DTS report YCOla00140. 
 */
match(s1, s2)
char *s1, *s2;
{
    int len1, len2;

    if(strcmp(s1, s2) == 0) {		/* exact match */
      return(RECOVER);
    }
    
    len1 = strlen(s1);
    len2 = strlen(s2);

    /* For the special case of including the root directory, consider
     * all paths which start with '/' supersets.  This if statement avoids
     * the following checks to verify that the substring of s2 in s1 really
     * ends with a directory name.  This special case is necessary, as root
     * is the only possible included directory which is allowed to end in a /.
     * For the code which preens-out the trailing slashes, see addnode (main.c)
     */
    if (*s2 == '/' && len2 == 1 && *s1 == '/') 
      return(RECOVER);

    if  (len2 < len1) {
        if( (strncmp(s1, s2, len2) == 0)    &&
	    (strncmp(s1+len2, "/", 1) == 0)   ) /* is s2 a parent dir of s1? */
            return(RECOVER);			/* recover leading path      */
    }
    else {
	if( (strncmp(s1, s2, len1) == 0)    &&
	    (S_ISDIR(statbuf.st_mode))      &&
	    (strncmp(s2+len1, "/", 1) == 0) &&  /* is it a directory subset? */
	    (strcmp(s1, "/") != 0)            )	/* don't count "/" as subset */
	    return(SUBSET);
    }

    return(FALSE);				/* name doesn't match        */
}


/* onmedia checks the current filename with each name on the include list.
 * If it is > each name specified in the list, then change volumes.
 */
onmedia()
{
    LISTNODE *t1;

    newvol = -TRUE;			/* assume we want the previous vol */
    t1 = ilist->ptr;
    while(t1 != (LISTNODE *)NULL) {
	if(mystrcmp(filename, t1->val) <= 0 ||
          (strlen(filename) > strlen(t1->val) &&
           strncmp(filename, t1->val, strlen(t1->val)) == 0))
	    newvol = FALSE;
	t1 = t1->ptr;
    }

    /* If there is possibly another volume to go to, return normally */
    if (volnum > 1 || newvol != -TRUE)  
      return;                           
    else {       /*We need to go back, but there's nothing to go back to*/
      done(0);
    }
}

#define U_READP 0000400
#define U_WRITEP 0000200
#define U_EXECP 0000100
#define G_READP 0000040
#define G_WRITEP 0000020
#define G_EXECP 0000010
#define O_READP 0000004
#define O_WRITEP 0000002
#define O_EXECP 0000001

fillmodes(s)
char *s;
{
    static char *rwx = "rwx";
    int i;
    static int perms[] = {
	U_READP, U_WRITEP, U_EXECP,
	G_READP, G_WRITEP, G_EXECP,
	O_READP, O_WRITEP, O_EXECP
    };

    switch(statbuf.st_mode & S_IFMT) {
        case S_IFDIR:
	    s[0] = 'd';
	    break;
        case S_IFCHR:
	    s[0] = 'c';
	    break;
        case S_IFBLK:
	    s[0] = 'b';
	    break;
        case S_IFIFO:
	    s[0] = 'p';
	    break;
        case S_IFLNK:
	    s[0] = 'l';
	    break;
        case S_IFSOCK:
	    s[0] = 's';
	    break;
	case S_IFNWK:
	    s[0] = 'n';
	    break;
	default:
	    s[0] = '-';
	    break;
    }

    for(i = 1; i < 10; i++) {
	if(statbuf.st_mode & perms[i-1])
	    s[i] = rwx[(i-1)%3];
	else
	    s[i] = '-';
    }

    if(statbuf.st_mode & S_ISUID) {
	if(s[3] == 'x')
	    s[3] = 's';
	else
	    s[3] = 'S';
    }

    if(statbuf.st_mode & S_ISGID) {
	if(s[6] == 'x')
	    s[6] = 's';
	else
	    s[6] = 'S';
    }
    if(statbuf.st_mode & S_ISVTX) {
	if(s[9] == 'x')
	    s[9] = 't';
	else
	    s[9] = 'T';
    }

    s[10] = '\0';
#ifdef ACLS
    if(statbuf.st_acl && goodacl_entries && aclflag) {
	s[10] = '+';
	s[11] = '\0';
    }
#endif /* ACLS */
}
	

#define MINDISKBLOCK 512  /*The smallest amount we will consider skipping
                           *when writing out a sparse file (512 bytes)
                           */

int
writefile(fd, buf, nbytes, last_write)
int fd, nbytes, last_write;
char *buf;
{
    int res, i, k;

    if(sflag) {               /* Try to make sparse files */
        i = 0;
        k = nbytes;           /* Only has an effect on the last_write */

        while(i < nbytes) {
	    if(last_write || (k = collectbytes(buf, i, nbytes)) > 0) { 
                                                        /*Need to do a write*/
	        (void) lseek(fd, byteoffset, SEEK_SET);
	        res = write(fd, &buf[i], (unsigned)k);
		if(res != k) {
		    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,47, "(1047): I/O error in write of file %s")),filename);
		    warn(msg);
		    return(-1);
		}
	    } else {
                k = -k;          /*Bytes to skip are returned as negative*/
            }

	    i          += k;
	    byteoffset += k;
	}
    }
    else {
	res = write(fd, buf, (unsigned)nbytes);
	if(res != nbytes) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,48, "(1048): I/O error in write of file %s")),filename);
	    warn(msg);
	    return(-1);
	}
    }

    return(0);
}


int
dirlist(buf, nbytes, head, cur, nomore)
char *buf;
int nbytes;
LISTNODE *head, *cur;
int *nomore;
{
    static char savechar = '\0';
    static int index = 0;
    int cnt = 0;
    int i;

    if(*nomore)
        return(cnt);
    for(i = 0; i < nbytes; i++) {
	if((buf[i] == '\0') && (savechar == '\0')) {
	    *nomore = TRUE;
	    break;
	}
	if(cur == (LISTNODE *)NULL) {
	    cur = (LISTNODE *)fmalloc(sizeof(LISTNODE));
	    cur->val = fmalloc(MAXNAMLEN);
	    cur->ptr = head->ptr;
	    head->ptr = cur;
	    index = 0;
	    cur->val[index] = '\0';
	    temp = head;
	}
	savechar = buf[i];
	cur->val[index++] = savechar;
	if(savechar == '\0') {
	    cnt++;
	    cur = (LISTNODE *)NULL;
	}
    }
    return(cnt);
}



int
cleandir(head, cur, writeflag)
LISTNODE *head, *cur;
int writeflag;
{
    int       	found;			/* flag for rm -rf */
    DIR	      	*dirp;			/* directory pointer */
    struct    	dirent *dp;		/* directory pointer */

    if((domodes == FALSE) && (writeflag == FALSE))
        return;				/* couldn't cd to dir */
    dirp = opendir(basename(filename));
    for(dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	if((strcmp(dp->d_name, ".") == 0) ||
	   (strcmp(dp->d_name, "..") == 0))
	    continue;		/* skip . and .. */
	cur = head->ptr;		/* start after head */
	found = FALSE;		/* assume failure */
	while(cur != (LISTNODE *)NULL) {
	    if(strcmp(cur->val, dp->d_name) == 0) {
		found = TRUE;	/* keep this file */
		break;		/* don't look further */
	    }
	    cur = cur->ptr;		/* check next name */
	}
	if(!found) {
	    (void) sprintf(msg, "rm -rf %s/%s",basename(filename),dp->d_name);
	    (void) system(msg);
	}
    }
    closedir(dirp);
}


posfile()
{
    int firstnum, save, offset = 0, fs_done;

    save = filenum;
    filenum = 1;

    for(;;) {
	getnexthdr(!RESYNC);
	if(!hedtrl(HED))
	    panic((catgets(nlmsg_fd,NL_SETN,50, "(1050): can't position to restart file")), !USAGE);

        if(do_fs && fs_done == SEARCHED)
          offset = filenum % fsmfreq;

        if(firstfile) {
           (void) strcpy(firstname, filename);
	   firstfile = FALSE;
	   firstnum = filenum;
        }

	if(save == filenum) {
	    recoverfile();
	    return;
	}

	if(save < filenum && filenum == firstnum) {
	    panic((catgets(nlmsg_fd,NL_SETN,51, "(1051): incorrect volume mounted")), !USAGE);
	}

        if (do_fs) {
          fs_done = fs_position(filenum, save, offset);

          if (fs_done == SEARCHED  ||  fs_done == DONTSEARCH) {
            fsm_move = TRUE;    /* Tape was fastsearched: alert getblk() to */
            noread   = FALSE;   /* discard buffer and read from input */
          }
        }

    }
}



markmatches(t1)
LISTNODE *t1;
{
    LISTNODE *t2;

    t2 = t1;
    while(t2 != (LISTNODE *)NULL) {
	if(strcmp(filename, t2->val) == 0) {
	    t2->aflag = RECOVER;
	    break;
	}
	t2 = t2->ptr;
    }
}


marklists()
{
    LISTNODE *t1;

    t1 = elist->ptr;
    while(t1 != (LISTNODE *)NULL) {
	if(strcmp(filename, t1->val) == 0)
	    t1->ftype = statbuf.st_mode;
	t1 = t1->ptr;
    }

    t1 = ilist->ptr;
    while(t1 != (LISTNODE *)NULL) {
	if(strcmp(filename, t1->val) == 0)
	    t1->ftype = statbuf.st_mode;
	t1 = t1->ptr;
    }
}



 /* Returns the number of bytes including start and up to end that should be 
  * written out explicitly, or lseeked-past as zeros.  The magnitude of
  * the number returned + byteoffset is guaranteed to end just before 
  * a MINDISKBLOCK boundary, or a value such that start+value=end.  
  * If a positive value is returned, this indicates that that many bytes
  * should be explicitly be written out.  If a negative value is returned,
  * this indicates that that many bytes are all zeros, and can be skipped.
  */
int
collectbytes(buf, start, end)
char *buf;
int start, end;
{
  int i, write = 0; /* write starts at 0, and is set to +/- for written/non*/
  int bytes = 0, blkstart, blkend, offset;

  /* Checks are made in blocks that are MINDISKBLOCK aligned, except 
   * for the first block which may begin on a non-block boundary.
   */
  blkstart = start;
  blkend   = start + MINDISKBLOCK - ((byteoffset + start) % MINDISKBLOCK);

  while (blkend < end + MINDISKBLOCK) {

  if (blkend > end)
    blkend = end;

    for (i = blkstart; i < blkend; i++) 
      if (buf[i] != '\0') 
	break;                    
	
    if (i == blkend) {           /*all bytes in the block are null */
      
      if (write == 0) {          /*This is the first blcok checked*/
	write = -1;              /*Set the null-block flag*/

      } else if (write == 1) {   /*Previous blocks were non-null*/
	return (bytes);
      }

    } else {                     /*This block is non-null*/

      if (write == 0) {          /*This is the first block checked*/
        write = 1;               /*Set the write flag*/

      } else if (write == -1) {  /*Previous blocks were null*/
        return (-bytes);
      } 
    } 

    bytes += blkend - blkstart; /*Add up the blocks of continguous bytes*/

    blkstart = blkend;
    blkend  += MINDISKBLOCK;
  }

  return ((write < 0) ? -bytes : bytes);
}



void
doobscure()
{
    OBSCURE *e;

    obscure_file = TRUE;		/* deal with obscurities */
    fwdlink = FALSE;			/* make sure we're clean */
    dotdot = FALSE;			/* the information is saved */
    overwrite = TRUE;			/* force these links! */
    linkflag = TRUE;			/* these are all links */

    e = o_head->o_ptr;
    while(e != (OBSCURE *)NULL) {
	(void) strcpy(filename, e->o_cpath);
	(void) strcpy(link_to, e->o_lpath);
	(void) strcpy(dotdot_to, e->o_dpath);
	statbuf = e->o_statbuf;
	if(e->o_fwdlink) {
	    (void) dofile();
	    (void) modes();
	}
	if(e->o_dotdot) {
	    (void) strcat(filename, "/..");
	    (void) strcpy(link_to, dotdot_to);
	    (void) dofile();
	    (void) modes();
	}
	e = e->o_ptr;
    }
}


#if defined(DUX) || defined(DISKLESS)
/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : restore()
 *      Purpose ............... : scan path for CDF components
 *
 *	Description:
 *
 *
 *      Given a path (possibly an expanded cdf with "+//" in it)
 *      return a compressed path, a bitstring which contains
 *      the permissions of each component, and the number of components.
 *
 *
 *      Returns:
 *
 *          compressed path ("str")
 *          permissions bit string ("bitstr")
 *          number of components in path ("cntc")
 */

restore(str,bitstr,cntc)
    char *str;
    int bitstr[];
    int *cntc;
{
    int i;
    char *p, *cp;

    if((str[strlen(str)-1] == '+') &&
       (statbuf.st_mode & S_ISUID) &&
       (statbuf.st_mode & S_IFDIR))
        strcat(str, "/");		/* put back the / we took out */
    *cntc = 0;
    for (i=0; i < MAXCOMPONENTS;i++)
	bitstr[i] = 0;

    p = str;
    if ( *p == '/' )
	p++;

    for (; *p && (p-str) < MAXPATHLEN; p++) {
	if (*p == '/')
	    (*cntc)++;

	if ( *cntc >= MAXCOMPONENTS ) {
	    (void) sprintf(msg, (catgets(nlmsg_fd,NL_SETN,54, "(1054): too many components in file name %s")),filename);
	    warn(msg);
	    break;
	}

	if ( *p == '+' && *(p+1) == '/' && (*(p+2) == '/' || *(p+2) == 0 ) ) {
	    bitstr[*cntc] = 1;
	    overlapcpy(p+1,p+2);
	}
    }

#ifdef DEBUG_B
    for(i=0; i < MAXCOMPONENTS; i++) {
	printf("bitstr[%d],%d ",i,bitstr[i]);
	printf("(%s/)\n",str);
	fflush(stdout);
    }
#endif /* DEBUG_B */

    return;
}

overlapcpy(s1,s2)
    char *s1;
    char *s2;
{

    /*
     * Copy overlapping areas.
     * Don't forget '\0' character at end.
     *
     * Note: this copy will only work if "s1" precedes "s2" in memory.
     */

    for (; *s2; s1++,s2++)
	*s1 = *s2;
    *s1 = '\0';
}
#endif DUX || DISKLESS


#ifdef ACLS
#define getbaseaclentry(basemode,ugo) ((basemode >> ugo) & 7)
#define ACL_USER 6
#define ACL_GROUP 3
#define ACL_OTHER 0

/* I G E T _ A C C E S S
 *
 * The access check routine. Check access for given file and
 * current user. Check acl in specificity order ((u,g), (u.*),
 * (*.g) and (*.*)). The user can match more than one (u.g) and 
 * (*.g) entries. For these it is necessary to check the entire
 * specificity level. For (u.*) return as soon as match is found.
 * Note there is only one (*.*) entry (the 'other' base mode bits)
 *
 * The variable names have been retained as much as possible for 
 * maintenance
 *
 * Please note that this is essentially the iget_access routine from
 * the kernel. If the algorithm needs to change here, it should also 
 * be considered there as well
 */
struct u_tag {
    int u_groups[NGROUPS];
} u;
getaccess()
{
	register int uid = user.pw_uid,
		     gid = guser.gr_gid;
	int fuid, fgid;
	struct passwd *pw;
	struct group *gr;
	int *firstgp = u.u_groups, 
	    *maxgp = &u.u_groups[NGROUPS];
	register int entry_count;
	register int match = FALSE;
	register int mode = 0;
	register struct acl_entry *iacl;
	register int *gp;
	int ngrps, ngroups = NGROUPS;

	/* set up u.u_groups  and file uid and gid */

	
	ngrps = getgroups(ngroups, u.u_groups);
	for(ngroups = ngrps; ngroups < NGROUPS; ngroups++)
	    u.u_groups[ngroups] = NOGROUP;
	if((pw = getpwnam(loginname)) == NULL)
	    return(FALSE);
	if((gr = getgrnam(groupname)) == NULL)
	    return(FALSE);
	fuid = pw->pw_uid;
	fgid = gr->gr_gid;

	if (statbuf.st_acl)
	{
		iacl = aclbuf;
		entry_count = 0;
	}
	else
		entry_count = nacl_entries;

	/* check all the (u,g) entries first */

	for (;  (entry_count < nacl_entries) &&
		(iacl->uid < ACL_NSUSER)  &&
		(iacl->gid < ACL_NSGROUP); iacl++, entry_count++ )
	{
		if (iacl->uid == uid)
		{
			if (iacl->gid == gid)
			{
				match = TRUE;
				mode |= iacl->mode;
				continue;
			}
			for (gp = firstgp; *gp != NOGROUP && gp < maxgp; gp++)
			{
				if (iacl->gid == *gp)
				{
					match = TRUE;
					mode |= iacl->mode;
					break;
				}
			}
		}
	}

	if (match) return(mode & READOTHER);

	/* next check all the (u,*) entries. Start with the base entry */

	if (fuid == uid)
		return (TRUE);

	for (;  (entry_count < nacl_entries) &&
		(iacl->gid == ACL_NSGROUP); iacl++, entry_count++ )
	{
		if (iacl->uid == uid)
			return(iacl->mode & READOTHER);
	}

	/* and now we check the (*,g) entries. Start with the base entry */

	if (fgid == gid)
	{
		match = TRUE;
		mode = getbaseaclentry(statbuf.st_mode,ACL_GROUP);
	}
	else
	{
		for (gp = firstgp; gp < maxgp && *gp != NOGROUP; gp++)
		{
			if (statbuf.st_gid == *gp)
			{
				match = TRUE;
				mode = getbaseaclentry(statbuf.st_mode,ACL_GROUP);
				break;
			}
		}
	}
	for (;  (entry_count < nacl_entries) &&
		(iacl->uid == ACL_NSUSER); iacl++, entry_count++ )
	{
		if (iacl->gid == gid)
		{
			match = TRUE;
			mode |= iacl->mode;
			continue;
		}
		for (gp = firstgp; *gp != NOGROUP && gp < maxgp ; gp++)
		{
			if (iacl->gid == *gp)
			{
				match = TRUE;
				mode |= iacl->mode;
				break;
			}
		}
	}

	if (match) return(mode & READOTHER);

	/* return the mode of the (*,*) base entry */

	return(getbaseaclentry(statbuf.st_mode,ACL_OTHER) & READOTHER);
}
#endif

/*
 * Given a pointer to an index_list of filenames, return the next possibly-
 * desired file in the list or NULL if there are none.  The checking done here
 * is cursory, banking on the idea that users will be specifying files to
 * include in the recovery, and not exclude many (the exclude list is not 
 * checked), relying on the wantfile() routine to do the final determination
 * of whether or not the file needs to be recovered.
 *
 * The routine is passed the current possible match into the list, and will
 * return either that same list node (if it should be checked for inclusion
 * in the recovery), a following node in the list (if the current filename
 * is not included, but a subsequent file is), or NULL (if no further files
 * on the list are included).
 */
struct index_list *
next_possible(current) 
  struct index_list *current;
{
  int len1;
  int len2;
  int match;
  LISTNODE *p;   /*Used to interate through the ilist*/
  char *s1;
  char *s2;
  
  for ( ; current != (struct index_list *)NULL; current=current->next) {
    match = 0;
    s1 = current->path;

    for (p=ilist->ptr; p != (LISTNODE *)NULL && match == 0; p=p->ptr) {
      s2 = p->val;
    
      len1 = strlen(s1);
      len2 = strlen(s2);

      if  (len2 <= len1) {
        if(strncmp(s1, s2, len2) == 0)
	    match++;
      } else if (!flatflag) {   /*Don't consider parent dir's if flat recover*/
	if(strncmp(s1, s2, len1) == 0)
	    match++;
      }
    } /* for over ilist  */

    if (match) {         /*The current filename matches, return the node*/
      return (current);
    } 

  } /* for over index_list */

  return (NULL);         /*Nothing found*/
}  /* end next_possible() */
