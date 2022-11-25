/* @(#) $Revision: 70.3 $ */
/****************************************************************

 The name of this file is frecover.h.

 +--------------------------------------------------------------+
 | (c)  Copyright  Hewlett-Packard  Company  1986.  All  rights |
 | reserved.   No part  of  this  program  may be  photocopied, |
 | reproduced or translated to another program language without |
 | the  prior  written   consent  of  Hewlett-Packard  Company. |
 +--------------------------------------------------------------+

 Changes:
	$Log:	frecover.h,v $
 * Revision 70.3  92/07/06  12:17:32  12:17:32  ssa
 * Author: jlee@hpucsb2.cup.hp.com
 * correct unistd.h vs fnmatch.h include statements.  
 * Can now be used to compile on 8.xx or 9.xx
 * 
 * Revision 70.2  92/06/09  11:27:16  11:27:16  ssa
 * Author: venky@hpucsb2.cup.hp.com
 * Included fnmatch.h for POSIX/XPG4 compatibility.
 * Removed unistd.h since there are duplicate declarations
 * in fnmatch.h
 * 
 * Revision 70.1  92/01/29  11:31:25  11:31:25  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Merged changes made for the second frecover patch for 8.0
 * 
 * Revision 66.8  91/12/06  17:45:00  17:45:00  ssa (RCS Manager)
 * Author: dickermn@hpucsb2.cup.hp.com
 * Removed unused fields from the index_list structure
 * 
 * Revision 66.7  91/10/30  17:34:50  17:34:50  ssa
 * Author: dickermn@hpisoe4.cup.hp.com
 * -Changed the structure of index_list to include a pointer to a string
 *  rather than a fixed array of MAXPATHLEN+1 to save space
 * -Added wantfile field to index structure to flag a desired file
 * -Added flag NONE for nflag "no recovery" option
 * -Added uid_t and gid_t to the UTYPE and corresponding scanf format strings
 * 
 * Revision 66.11  91/10/30  15:12:26  15:12:26  root ()
 * added the uid_t and gid_t to the UTYPE and corresponding scanf format strings
 * 
 * Revision 66.10  91/10/23  18:40:51  18:40:51  root ()
 * Added tolerance to the checksum-checking routine to accept character-based
 * checksums as well as integer-based sums.  Also cleaned-up error messages
 * 
 * Revision 66.9  91/10/10  18:15:40  18:15:40  root ()
 * Added flag NONE to for nflag/-n "no recovery" option
 * 
 * Revision 66.8  91/10/08  10:11:37  10:11:37  root ()
 * Added wantfile field to the index structure to flag a desired file
 * 
 * Revision 66.7  91/10/01  17:43:07  17:43:07  root ()
 * changed the structure of index_list to include a pointer to a string
 * rather than an array of MAXPATHLEN+1, to save space.
 * 
 * Revision 66.6  91/09/13  02:03:30  02:03:30  hmgr (History Manager)
 * Author: danm@hpbblc.bbn.hp.com
 * patch to allow recovery of 8.01 DAT tapes on other 8.0X versions
 * 
 * Revision 66.5  91/03/01  08:29:48  08:29:48  ssa (#shared source login)
 * Author: danm@hpbblc.bbn.hp.com
 * fixes to MO and DAT extensions
 * 
 * Revision 66.4  91/01/17  15:14:13  15:14:13  danm
 *  changes for DAT and MO support as well as other 8.0 enhancements
 * 
 * Revision 66.3  90/07/16  13:15:44  13:15:44  danm (#Dan Matheson)
 *  change of rmt commands s & S to f & m, ascii exchange of stat and mtget
 *  structures.
 * 
 * Revision 66.2  90/02/27  15:47:07  15:47:07  danm (#Dan Matheson)
 *  clean up of #includes and switch to new directory(3c) routines.
 * 
 * Revision 66.1  90/02/26  19:58:27  19:58:27  danm (#Dan Matheson)
 *  moved #includes to frecover.h too make changes easier, and fixed things
 *  to use malloc(3x)
 * 
 * Revision 64.7  90/01/11  16:23:42  16:23:42  danm (#Dan Matheson)
 *  defines for alphabetic problem for files.c
 * 
 * Revision 64.5  89/06/25  19:32:56  19:32:56  takiya
 * Put the statement "#define mystrcmp strcmp" to compile without the
 * compiler flag "NLS".
 * 
 * Revision 64.4  89/02/18  15:38:24  15:38:24  jh
 * NLS initialization and collation.
 * Replaced nl_init() with setlocale(); replaced nl_strcmp() with strcoll().
 * 
 * Revision 64.3  89/01/16  08:23:24  08:23:24  lkc (Lee Casuto[Ft.Collins])
 * Changed DOTDOT definition to FBDOTDOT so as not to conflict with 
 * new standards define.
 * 
 * Revision 64.2  88/12/05  13:36:41  13:36:41  lkc (Lee Casuto)
 * Moved pchartype definition in union UTYPE out of #ifdef ACLS.
 * 
 * Revision 64.1  88/12/05  13:28:59  13:28:59  lkc (Lee Casuto)
 * Fixed union definition, ushorttype was defined twice.
 * Fixed #ifdef ACLS around PTRCHAR, it needs to be defined with or without ACLS
 * 
 * Revision 63.2  88/09/22  16:09:03  16:09:03  lkc (Lee Casuto)
 * Added code to handle ACLS
 * 
 * Revision 63.1  88/09/16  15:53:24  15:53:24  lkc (Lee Casuto)
 * Added support for cnode specific devices
 * 
 * Revision 62.2  88/07/28  10:35:53  10:35:53  lkc (Lee Casuto)
 * removed RECSIZE. It's now malloc'd.
 * 
 * Revision 62.1  88/04/06  11:27:24  11:27:24  carolyn
 * increased RECSIZE
 * 
 * Revision 56.2  88/03/23  10:46:10  10:46:10  pvs
 * The symbol "SYMLINK" should really be "SYMLINKS". Made appropriate changes.
 * 
 * Revision 56.1  87/11/04  10:08:06  10:08:06  runyan (Mark Runyan TN-447-6676)
 * Complete for first release (by lkc)
 * 
 * Revision 51.2  87/11/03  16:57:13  16:57:13  lkc (Lee Casuto)
 * completed for first release
 * 

 This file:
	include for frecover command.

****************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/stdsyms.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fbackup.h>
#include <fcntl.h>
#include <grp.h>
#include <malloc.h>
#include <pwd.h>
#include <signal.h>
#include <string.h>
/* Replacing unistd.h with fnmatch.h for POSIX/XPG4 
   compatibility - June 1992 
#include <unistd.h>
*/
#include <unistd.h>
#ifndef FNM_PATHNAME
#include <fnmatch.h>
#endif

#include "vdi.h"

#if defined NLS || defined NLS16
#include <nl_ctype.h>
#include <nl_types.h>
#include <langinfo.h>
#include <stdlib.h>
#include <locale.h>
#endif

#ifdef ACLS
/* Removed unistd.h since fnmatch.h has been included
*/
#include <sys/acl.h>
#endif /* ACLS */

#define BLOCKSIZE_PRE_8_0 512
#define MAXOUTFILES 256
#define MAXIDS	    50
#define OFF_T 	    0
#define DEV_T 	    1
#define INO_T 	    2
#define USHORT 	    3
#define SHORT 	    4
#define TIME_T 	    5
#define UINT 	    6
#define	CHAR	    7
#define LONG	    8
#define LINKNAME    9
#define FBDOTDOT    10
#define FWDLINK	    11
#ifdef CNODE_DEV
#define SITE_T	    12
#endif /* CNODE_DEV */
#ifdef ACLS
#define INT	    13
#endif /* ACLS */
#define PTRCHAR	    14
#define UID_T       15
#define GID_T       16
#define UNKNOWN	    -1
#define MAXS	    4096
#ifdef hpux
#define OFF_TFMT    "%ld"
#define TIME_TFMT   "%ld"
#else
#define OFF_TFMT    "%d"
#define TIME_TFMT   "%d"
#endif hpux
#define DEV_TFMT    "%ld"
#define LONGFMT	    "%ld"
#define UID_TFMT    "%ld"
#define GID_TFMT    "%ld"
#define INO_TFMT    "%lu"
#define UINTFMT	    "%u"
#define HED 0
#define TRL 1
#define EOBU 2
#define ENDOFFSET 4
#define OTHER 2
#define FILEID 7
#define FILESTATUS 8
#define BACKUPID 9
#define FILENAME 10
#define BACKUPTIME 11
#define IOERROR (char *)-3
#define DEFAULTINPUT	"/dev/rmt/0m"
#define DEVNULL "/dev/null"
#define GOOD 1
#define FAIL 0
#define ANSILABSIZE 1024
#define MAXTRYS 10
#define READWRITE 0600
#define USAGE 1
#define RESYNC 1
#define TRUE 1
#define FALSE 0
#define ROOT 0
#define FLAT 0
#define RELATIVE 1
#define ABSOLUTE 2
#define NONE 3
#define NOWRITE -1
#define SYNCDEF -1
#ifdef SYMLINKS
#define statcall lstat
#else /* SYMLINKS */
#define statcall stat
#endif /* SYMLINKS */
#ifdef hp9000s500
#define LOCALMAGIC 0xaaaa	/* as of 6/29/82 */
#endif
#ifdef hp9000s200
#define LOCALMAGIC 0xdddd	/* as of 4/17/85 (Manx file system)*/
#endif
#ifdef hp9000s800
#define LOCALMAGIC 0xcccc	/* as of 4/1/85 */
#endif
#define RECOVER 2
#define SUBSET 1
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#if defined NLS || defined NLS16
#define mystrcmp strcoll
extern char *catgets();
#else
#define mystrcmp strcmp
#define catgets(i, sn,mn,s) (s)
#endif NLS

#define RELEASE31 "A.B3.10"
#define RELEASE30 "A.B3.00"
#define RELEASE65 "6.5"
#define RELEASE_800_7 "A.B7"
#define RELEASE_300_7 "7."
#define RELEASE_700_801 "A.B8.01"
#define RELEASE_800_802 "A.08.02"

typedef union {
    off_t 	*off_ttype;
    dev_t 	*dev_ttype;
    ino_t 	*ino_ttype;
    ushort	*ushorttype;
    short	*shorttype;
    time_t	*time_ttype;
    uint	*uinttype;
    char	*chartype;
    long    	*longtype;
    uid_t    	*uid_ttype;
    gid_t    	*gid_ttype;
#ifdef ACLS
    int		*inttype;
#endif /* ACLS */
    char 	**pchartype;
} UTYPE;
UTYPE name[MAXIDS];

typedef struct {
    char 	*field;
    int 	type;
} LOOKUP;


typedef struct lnode {
    char *val;
    short aflag;		/* used to determine recoverability */
    int ftype;
    struct lnode *ptr;
} LISTNODE;

#define UNGETBLK() { \
    noread = TRUE; \
}

typedef struct {
    int filenum;
    int hflag;
    int yflag;
    int aflag;
    int vflag;
    int sflag;
    int cflag;
    int xflag;
    int fflag;
    int pflag;
#ifdef ACLS
    int aclflag;
#endif ACLS
    int flatflag;
    int oflag;
    int volnum;
    int overwrite;
    int recovertype;
    int doerror;
    int dochgvol;
    int prevvol;
    int synclimit;
    char residual[MAXPATHLEN];
    char home[MAXPATHLEN+3];
    char errfile[MAXPATHLEN];
    char chgvolfile[MAXPATHLEN];
    VHDRTYPE vol;
} RESTART;

typedef struct onode {
    char	o_cpath[MAXPATHLEN+1];	/* saved filename */
    char	o_lpath[MAXPATHLEN+1];	/* saved link_to */
    char	o_dpath[MAXPATHLEN+1];	/* saved dotdot_to */
    int		o_fwdlink;		/* saved fwdlink */
    int		o_dotdot;		/* saved dotdot */
    struct stat	o_statbuf;		/* saved statbuf */
    struct onode *o_ptr;		/* pointer to next element */
} OBSCURE;

struct index_list {
  char *path;              /* path part of string */
  int  num;                /* file number in backup  */
  struct index_list *next; /* pointer to next element */
};

struct fn_list {
  char name[MAXPATHLEN+1];
  struct fn_list *next;
};

char *fmalloc();			/* forward reference */
char *getblk();				/* forward reference */

/* the following is for remote tape device support on S300 */

/* use mt_dsreg1 for the following */
#define GMT_300_EOF(x)              ((x) & 0x00000080)
#define GMT_300_BOT(x)              ((x) & 0x00000040)
#define GMT_300_EOT(x)              ((x) & 0x00000020)
#define GMT_300_WR_PROT(x)          ((x) & 0x00000004)
#define GMT_300_ONLINE(x)           ((x) & 0x00000001)

/* use mt_dsreg2 for the following */
#define GMT_300_D_6250(x)           ((x) & 0x00000080)
#define GMT_300_DR_OPEN(x)          ((x) & 0x00000004)
#define GMT_300_IM_REP_EN(x)        ((x) & 0x00000001)
