/* Header: $Header: getmount.c,v 70.5 92/08/24 22:25:00 ssa Exp $ */

/*
** update_mnttab(3x) -- Undocumented library routine to update 
**  /etc/mnttab from the kernel mount table preserving the correct
**  root device, or finding it if run on the root server cnode.
*/

#ifdef _NAMESPACE_CLEAN
#define close		_close
#define fileno		_fileno
#define open		_open
#define getccent	_getccent
#define endccent	_endccent
#define sleep		_sleep
#define gettimeofday	_gettimeofday
#define strncmp		_strncmp
#define ltoa		_ltoa
#define strcpy		_strcpy
#define strlen		_strlen
#define strncat 	_strncat
#define strcat		_strcat
#define strcmp		_strcmp
#define strstr		_strstr
#define strncpy		_strncpy
#define strtok		_strtok
#define getcdf		_getcdf
#define chmod		_chmod
#define cnodeid		_cnodeid
#define getcontext	_getcontext
#define lockf		_lockf
#define mkrnod		_mkrnod
#define stat		_stat
#define utime		_utime
#define addmntent	_addmntent
#define endmntent	_endmntent
#define getmntent	_getmntent
#define setmntent	_setmntent
#define hasmntopt	_hasmntopt
#define getmount_entry	_getmount_entry
#define getmount_cnt	_getmount_cnt
#endif /* _NAMESPACE_CLEAN */

#include <stdio.h>
#include <fcntl.h>
#include <mntent.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <nfs/nfs.h>
#include <string.h>
#include <utime.h>
#include <cluster.h>
#include <unistd.h>
#ifdef QUOTA
#include <sys/quota.h>
#endif

/* Macro to expand CDFs into a new string */

#define CDFEXPAND_NAME(target, source, strlength) \
      {	\
	if ((getcdf(source, target, strlength)) == NULL) {\
	    strncpy(target, source, strlength); \
	} target[strlength] = '\0'; \
      }

/* Macro to add a comma to the end of a mnttab string */

#define ADD_COMMA(string) (void)strncat(string,",",MNTMAXSTR-strlen(string))

#define NOT_ROOT	0	/* Return values of is_root_device() */
#define IS_ROOT		1
#define CANNOT_STAT_DEV	2
#define CANNOT_STAT_FS	3

#define LOCALROOT 	"localroot"   /* context req'd for root server */
#define LOCALROOTCDF 	"+/localroot" /* context req'd for root server */
#define UNKNOWN 	"unknown"     /* name for unknown root device */
#define ROOT_DEVICE 	"/dev/root"   /* name of generic root device */
				      /* mode for block device at 0640 */
#define DEV_MODE	S_IFBLK|S_IRUSR|S_IWUSR|S_IRGRP

/*
 * mnttab_info --
 *	A structure to hold /etc/mnttab information which is not
 *	maintained in the kernel.  Specifically, this includes
 *	the passno and freq fields and the noauto, bg and retry 
 * 	option information.
 *
 *	The dir[] field contains the mounted on directory and is
 *	used for searching lists of these structures.
 *
 *	If the is_root_fs field is TRUE, then this represents the
 *	root file system's entry, and in this case the dir[] field
 *	will contain the name of a valid special device file for 
 *	the root file system.
 */
struct mnttab_info {
	struct mnttab_info *next;
	char options[MNTMAXSTR];	/* may contain noauto, bg, retry */
	int passno;
	int freq;
	char dir[MNTMAXSTR];		/* mount point (unless is_root_fs) */
	int is_root_fs;			/* flag indicating root fs entry */
};

static int have_good_root;		/* found valid root dev flag */

/*
 * The get_root_server_context() routine will accept as an argument a path 
 * name (assumed to be a directory name).  It will find the context string 
 * associated with the root server and return this string.  It uses the rules 
 * maintained by the other commands; that is, it will first check for the root 
 * servers cluster name, then it will check for "localroot".  If both exist, 
 * then "localroot" will be ignored.  If neither is found, it will return
 * the LOCALROOT string.  If we couldn't malloc the memory for the new
 * string, then just return NULL.
 */
static char *
get_root_server_context(root_path, sub_path)
char *root_path;
char *sub_path;
{
struct stat stat_buf;
struct cct_entry *cct_ent;
char *new_path;

    /*
     * We're not a localroot, but we need to get at the
     * localroot's /root_pat/sub_path directory; so figure out appropriate
     * CDF for /root_path.  We want to check the root servers cnode name
     * first.  If this doesn't work, then just default to "localroot".
     * If for some reason, the malloc() for the new string fails, then
     * return NULL.
     */
    while (cct_ent = getccent())
    {
        /*
         * find the rootserver's entry in /etc/clusterconf
         */
        if (cct_ent->cnode_type == 'r')
            break;  /* found rootserver */
    }
    endccent();

    /*
     * If it was found, then check to see if it is a directory
     */
    if (cct_ent)
    {
	new_path = (char *) malloc(strlen(root_path) +
				   strlen(sub_path) +
				   strlen(cct_ent->cnode_name) + 3);
	if (new_path == (char *)NULL)
	    return (char *) NULL;
	(void) strcpy(new_path, root_path);
	(void) strcat(new_path, "+/");
	(void) strcat(new_path, cct_ent->cnode_name);
	if (stat(new_path, &stat_buf) == 0)
	{
	    /*
	     * Check to see if its a directory.
	     */
	    if ((stat_buf.st_mode & S_IFMT) == S_IFDIR)
	    {
		/* Add the subpath to the path and then return
		 *
		 * NOTE: We could check to see if the file actually exists
		 * under this directory;  if not, then default to
		 * localroot, but this isn't how the kernel and other
		 * commands will treat it, so we shouldn't either.
		 */
		(void) strcat(new_path, sub_path);
		return new_path;
	    }
	    else if (sub_path == NULL || sub_path[0] == '\0')
		return new_path;
	}
	/* 
	 * Free the new_path memory.  We didn't find it.
	 */
	(void) free(new_path);
    }

    if (new_path = (char *) malloc (strlen(root_path) + strlen(sub_path) +
				    strlen(LOCALROOTCDF) + 1))
    {
        /*
	 * Copy in best guess at CDF expansion
	 */
	(void) strcpy(new_path, root_path);
	(void) strcat(new_path, LOCALROOTCDF);
	(void) strcat(new_path, sub_path);
	/*
	 * return newly expanded name in new_path 
	 */
	return new_path;
    }
    return (char *) NULL;
}

/*
 * localroot() -- function call to determine that we've got a local
 *		root disk. (i.e., we're standalone or a root server)
 *		Returns TRUE if localroot, FALSE otherwise.
 */
static int
localroot()
{
    static int  is_localroot = -1;
    int     length;
    char   *contextbuf;
    char   *s;

    if (is_localroot != -1)
	return is_localroot;

    length = getcontext(NULL, 0);   /* get length of context str */
    contextbuf = (char *)malloc(length);    /* get space to put it in */
    (void)getcontext(contextbuf, length);   /* get the context */

    /* look for localroot in context -- (in standalone context also) */
    s = strstr(contextbuf, LOCALROOT);
    free(contextbuf);
    return is_localroot = (s != NULL);
}

/*
 * is_root_device(dev_name) -- stats the root file system (/) and the
 *			   device file named by dev_name, and compares
 *			   the device on which the root fs resides to
 *			   dev_name.  Returns the following values
 *
 * 	NOT_ROOT	0:	if dev_name is not the root device
 * 	IS_ROOT		1:	if dev_name is the root device 
 * 	CANNOT_STAT_DEV	2:	if we couldn't stat dev_name
 * 	CANNOT_STAT_FS	3:	if we couldn't stat /
 *
 *	When this routine is run on a non-rootserver cnode, an attempt
 *	is made to expand "/dev/" to "/dev+/localroot", so that the 
 *	device can be found even if the CDF is not expanded.  This 
 *	should be sufficient for about 98% of possible cases.
 *
 *	Note that in the case of a non-rootserver cnode, the dev_name
 *	string may be changed by this routine.
 */
static int
is_root_device(dev_name)
char   **dev_name;
{
    struct stat	root_statbuf;
    struct stat	dev_statbuf;

    if ((stat("/", &root_statbuf)) != 0)
    {
	/* couldn't stat / -- this should never happen */
	return CANNOT_STAT_FS;
    }

    /*
     * in case this is run on a non-root server cnode, check for unexpanded
     * cdf and try to expand /dev/ into /dev+/rootservername/ (first) or 
     * /dev+/localroot/ (second) and put new string into dev_name pointer
     */
    if (!localroot() && (strncmp(*dev_name, "/dev/", 5) == 0)) 
    {
        char *new_name;
       /*
        * We're not a localroot, but we need to get at the
        * localroot's /dev directory; so figure out appropriate CDF
        * for /dev (should be "localroot", but may be cnodename type cdf).
	* We want to check the cnodename case first since we never know.
        */
        new_name = get_root_server_context("/dev", *dev_name + 4);
	if (new_name != NULL)
	{
	    /*
	     * return newly expanded name in new_name
	     */
	    *dev_name = new_name;
	}
	/*
	 * if new_name == NULL, then just leave *dev_name alone.
	 */
    }
    if ((stat(*dev_name, &dev_statbuf)) != 0)
    {
	/* couldn't stat device */
	return CANNOT_STAT_DEV;
    }

    if ((dev_statbuf.st_rdev == root_statbuf.st_realdev) &&
	((dev_statbuf.st_rcnode == root_statbuf.st_cnode) ||
	    (dev_statbuf.st_rcnode == 0)))
	return IS_ROOT;

    return NOT_ROOT;
}

/*
 * get_mnttab_info() -- Reads the /etc/mnttab file and, for each entry,
 * 			stores certain mount information into mnttab_info
 *			structure.  These structues are linked together
 *			to form a circular linked list.  The head of this 
 *			cirular list is returned to the caller.  NULL is
 *			returned if mnttab cannot be opened.  
 *
 *			This routine also looks for and flags the first
 *			valid root device found in /etc/mnttab.
 *
 *			May set global have_good_root flag.
 */
static struct mnttab_info*
get_mnttab_info()
{

    FILE *fp;			/* file pointer for mnttab */
    struct mnttab_info *head = NULL;
    struct mnttab_info *m_info = NULL;
    struct mnttab_info *last = NULL;
    struct mntent *mnt;		/* actual data from mnttab */

    char *c;

    if ((fp = setmntent(MNT_MNTTAB, "r")) == NULL) 
	return NULL;	/* couldn't open /etc/mnttab */

    while ((mnt = getmntent(fp)) != NULL) 	/* for each mnttab entry */
    {
	/* allocate space and initialize it */
	m_info = (struct mnttab_info *) malloc(sizeof(struct mnttab_info));
	m_info->options[0] = '\0';

	/* check for root */
	if (mnt->mnt_dir[0] == '/' && mnt->mnt_dir[1] == '\0')
	{
	    /* is it valid? */
	    if (!have_good_root && is_root_device(&mnt->mnt_fsname)==IS_ROOT) 
	    {
		/* set flags */
		have_good_root = TRUE;
		m_info->is_root_fs = TRUE;

		/*
		 * For the root device ONLY, the dir field will contain
		 * the special device file instead of the mount point
		 */
		CDFEXPAND_NAME(m_info->dir, mnt->mnt_fsname, MNTMAXSTR - 1);
	    }
	    else
	    {
		/* root entry is INVALID -- bail */
		(void)free(m_info);
		continue;
	    }
	}
	else
	{
	    /* 
	     * Not the root entry, save the fs mount point name.
	     * Do NOT expand CDF if type is NFS or IGNORE (for automount)
	     * as they might hang an update_mnttab which occurs on a client 
	     * cnode before networking is up
	     */
	    if (strcmp(mnt->mnt_type, MNTTYPE_NFS) == 0 ||
	       strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) 
		strcpy(m_info->dir, mnt->mnt_dir);
	    else
		CDFEXPAND_NAME(m_info->dir, mnt->mnt_dir, MNTMAXSTR - 1);
	    m_info->is_root_fs = FALSE;
	}

	m_info->passno = mnt->mnt_passno;
	m_info->freq = mnt->mnt_freq;

	/* check for noauto option */
	if ((strstr(mnt->mnt_opts, MNTOPT_NOAUTO)) != NULL)
	{
	    /* add option */
	    (void)strcat(m_info->options, MNTOPT_NOAUTO);
	    ADD_COMMA(m_info->options);
	}

	/* check for nfs options */
	if ((strstr(mnt->mnt_type, MNTTYPE_NFS)) != NULL) 
	{
	    /* check for bg and retry options */
	    if ((strstr(mnt->mnt_opts, MNTOPT_BG)) != NULL)
	    {
		/* add option */
		(void)strcat(m_info->options, MNTOPT_BG);
		ADD_COMMA(m_info->options);
	    }
	    if ((c = strstr(mnt->mnt_opts, MNTOPT_RETRY)) != NULL)
	    {
		/* retry carries a value, so copy directly from mnttab */
		if (strtok(c,","))
		{
		    (void)strcat(m_info->options,c); 
		    ADD_COMMA(m_info->options);
		}
	    }
	}

	/* set up head if this is first entry */
	if (head == NULL) head = m_info;

	/* set up the last entry to point to the current entry */
	if (last) last->next = m_info;
	last = m_info;			/* make current last for next time */

    }   /* end of while getmntent */

    /* set the last to point to the head */
    if (head) last->next = head;

    endmntent(fp);

    return head;
}

static struct mnttab_info *new_head;
/*
 * find_in_mnttab(dir) -- scans the linked list of mnttab_info 
 *			and returns mnttab_info ptr of match.  Returns
 * 			null if no match.  Use's head as head
 *			of list for first pass.
 */
static struct mnttab_info *
find_in_mnttab(dir,head)
char *dir;
struct mnttab_info *head;
{
    /*
     * for the 1st searching pass, use global head of list, after 1st pass,
     * always pick up where we left off.  Since mnttab order should
     * approximate mount order, this should be most efficient.
     */
    struct mnttab_info *m_info;

    if (new_head == NULL) new_head = head;	/* otherwise, use static */

    if ((m_info = new_head) == NULL)
	return NULL;	/* list must be empty */
    
    do
    {
	/* is this the entry we're looking for? */
	if ((strcmp(m_info->dir, dir)) == NULL) 
	{
	    /* yes, save new list head */
	    new_head = m_info->next;

	    /* return matching entry */
	    return m_info;
	}

	/* keep looking */
	m_info = m_info->next;

    } while (m_info != new_head);

    /* never found a match */
    return NULL;
}

/*
 * free_mnttab_list(head) -- frees mnttab_info storage 
 */
static void
free_mnttab_list(head)
struct mnttab_info *head;
{
    struct mnttab_info *m_info, *next;

    if (m_info = head)		/* if there is a list */
    {
	do
	{
	    next = m_info->next;
	    (void)free(m_info);
	    m_info = next;
	} while ((m_info != head) && m_info);
    }
}

static void
AddOpt(optstring,option)
char *optstring,*option;
{
	/* Put option on the end */
	(void)strncat(optstring, option, (MNTMAXSTR - strlen(optstring)));
	(void)strncat(optstring, ",", (MNTMAXSTR - strlen(optstring)));
}


static void
AddOptVal(optstring,option,value)
char *optstring,*option;
long value;
{
	/* Put option AND it's value on the end */
	(void)strncat(optstring, option, (MNTMAXSTR - strlen(optstring)));
	(void)strncat(optstring, "=", (MNTMAXSTR - strlen(optstring)));
	(void)strncat(optstring, ltoa(value), (MNTMAXSTR - strlen(optstring)));
	(void)strncat(optstring, ",", (MNTMAXSTR - strlen(optstring)));
}


/*  Clean up ANSI/POSIX namespace */
#ifdef _NAMESPACE_CLEAN
#undef update_mnttab
#pragma _HP_SECONDARY_DEF _update_mnttab update_mnttab
#define update_mnttab _update_mnttab
#endif /* _NAMESPACE_CLEAN */

#include <time.h>
/*
 * update_mnttab() -- This routine rewrites the userland mount table using
 *			the kernel mount table, the checklist file and
 *			the existing /etc/mnttab.
 */
int
update_mnttab()
{
    int     cnt, i, defaults;
    time_t  timebuf;
    struct  utimbuf mnt_times;
    struct  stat statbuf;
    unsigned counter = 1; 	/* loop counter for waiting on lockf */
    unsigned tries = 0; 	/* loop counter for waiting on lockf */
    int fd;
    struct timeval tp;
    struct timezone tzp;
    unsigned long endtime;

    /* character buffers for pathnames, etc */
    char    mnt_fsname[MAXPATHLEN];
    char    mnt_dir[MAXPATHLEN];
    char    mnttab_fsname[MNTMAXSTR];
    char    mnttab_dir[MNTMAXSTR];
    char    mnttab_opts[MNTMAXSTR];
    char    *checklist_name;
    char    *tmp_fsname;		/* temp pointer for is_root_device */

    FILE   *read_fp;		/* file ptr for reading mnttab & cklist */
    FILE   *write_fp;		/* file ptr for writing mnttab */

    struct mntent   mnttab;		/* mnttab entry to write out */
    struct mntent  *rootmnt = NULL; 	/* mount entry for root device */
    struct mount_data   mnt_data;	/* mount data from kernel */

    struct mnttab_info *mnttab_head;	/* static structure points to
					   head of mnttab list */
    struct mnttab_info *m_info;

    have_good_root = FALSE;		/* haven't found one yet */
    new_head = (struct mnttab_info *)NULL; 
					/* initialize here since 
					   update_mnttab() can be called more
					   than once from the same source */
    mnttab_head = get_mnttab_info();	/* read info from exising mnttab */

    gettimeofday(&tp, &tzp);
    endtime = tp.tv_sec + 900;
    /* 
     * create mnttab or rewrite existing one 
     */
    while ((write_fp=setmntent(MNT_MNTTAB,"w")) == NULL) {
        if (errno!=EACCES && errno!=EAGAIN) /* real failure, return -1 */
            return -1;
	sleep(1);
	gettimeofday(&tp, &tzp);
	if (tp.tv_sec > endtime) /* give it 15 minutes to complete */
	        return -1;
    }

    if (write_fp == NULL)
	return -1;

    (void)chmod(MNT_MNTTAB, MNTTAB_MODE);

    /* set mount entry pointers to point at real space */
    mnttab.mnt_opts = mnttab_opts;
    mnttab.mnt_dir = mnttab_dir;
    mnttab.mnt_fsname = mnttab_fsname;

    /* For each entry in kernel mount table, make an entry in mnttab */
    cnt = getmount_cnt(&timebuf);
    for (i = 0; i < cnt; i++)
    {
	mnttab_opts[0] = '\0';	/* initialize for string append */

	/* Get the kernel mount entry */
	if (getmount_entry(i, mnt_fsname, mnt_dir, &mnt_data) < 0)
	    continue;

	/* check for root device */
	if (mnt_dir[0] == '/' && mnt_dir[1] == '\0')
	{
	    /* copy in mount dir */
	    mnttab_dir[0] = '/'; 
	    mnttab_dir[1] = '\0';

	    /* root entry -- Check for valid root entry in mnttab */
	    if (have_good_root)
	    {
		/* find it in mnttab info */
		if (m_info = mnttab_head)
		{
		    do
		    {
			if (m_info->is_root_fs) 
			    break;
			m_info = m_info->next;
		    } while (m_info != mnttab_head);
		}

		if (m_info->is_root_fs) 	/* just in case :-) */
		{
		    /* Use old mnttab info only where BETTER than kernel */
		    CDFEXPAND_NAME(mnttab.mnt_fsname,m_info->dir,MNTMAXSTR-1);
		    mnttab.mnt_freq = m_info->freq;
		    mnttab.mnt_passno = m_info->passno;
		    if (m_info->options[0] != '\0')
			(void)strcat(mnttab_opts, m_info->options);
		}
	    }
	    else	/* don't have a good root device from mnttab */
	    {
		/*
		 * Get root device from root server's /etc/checklist file
		 *   First determine the name of the rootserver's checklist
		 */
		if (localroot())
		{
		    /*
		     * If we're standalone||rootserver, use "/etc/checklist"
		     */
		    checklist_name = MNT_CHECKLIST;
		}
		else
		{
		    /*
		     * We're not a localroot, but we need to look at the
		     * localroot's checklist, so figure out appropriate CDF
		     * for checklist (should be cnodename type cdf)
		     */
		    checklist_name = 
			get_root_server_context("/etc/checklist", "");
		}
			
		/*
		** Now we have the name of checklist, 
		** look for the root device entry!
		*/
		if (checklist_name && 
			(read_fp = setmntent(checklist_name, "r")) != NULL) 
		{
		    while ((rootmnt = getmntent(read_fp)) != NULL) 
		    {
			/* is this a legit root device? */
			if ((rootmnt->mnt_dir[0] == '/' && 
			    rootmnt->mnt_dir[1] == '\0') &&
			    (is_root_device(&rootmnt->mnt_fsname) 
			    == IS_ROOT))
			{
			    /* Found legit entry, use it */
			    CDFEXPAND_NAME(mnttab.mnt_fsname,
			      rootmnt->mnt_fsname, MNTMAXSTR-1);
			    mnttab.mnt_freq = rootmnt->mnt_freq;
			    mnttab.mnt_passno = rootmnt->mnt_passno;
			    have_good_root = TRUE;
			    break;  /* quit looking */
			}   
		    }	
		    endmntent(read_fp);
		}

		if (!have_good_root)
		{
		    if (localroot())
		    {

			/*
			** Have_good_root is still FALSE; both search 
			** of checklist and mnttab failed, so now try 
			** ROOT_DEVICE, creating it if necessary. If that
			** doesn't work, punt & use UNKNOWN as fsname.
			** 	ONLY DO THIS ON THE ROOT SERVER !
			*/

			/* Try and use the generic root device */
			CDFEXPAND_NAME(mnttab_fsname, ROOT_DEVICE, 
					MNTMAXSTR - 1);
			/*
			 * use a dummy pointer in call to is_root_device
			 * to handle char ** parameter which is needed
			 * for !localroot cases only.
			 */
			tmp_fsname = mnttab_fsname;
			switch(is_root_device(&tmp_fsname))
			{
			case IS_ROOT:
			    break;	/* all is fine */

			case CANNOT_STAT_FS:
			case NOT_ROOT:
			    (void)strcpy(mnttab_fsname, UNKNOWN);
			    break;

			case CANNOT_STAT_DEV:
			    /* Use UNKNOWN if any error but ENOENT */
			    if (errno != ENOENT)
			    {
				/* can't stat the file */
				(void)strcpy(mnttab_fsname, UNKNOWN);
			    }
			    else
			    {
				/* No root device, try and create one */
				if ((mkrnod(ROOT_DEVICE, DEV_MODE, 
				    makedev(-1, -1), cnodeid())) < 0)
				    (void)strcpy(mnttab_fsname, UNKNOWN);
				/* rewrite dev name, so the cdf is expanded */
				else
				    CDFEXPAND_NAME(mnttab_fsname, ROOT_DEVICE, 
					MNTMAXSTR - 1);
			    }
			    break;
			}
		    }  
		    else  /* not localroot, but no good root device found */
		    {
			/*  Something's screwy, use unknown */
			(void)strcpy(mnttab_fsname, UNKNOWN);
		    }

		}   /* end of if ! have_good_root */
	    }  /* end of else no good root in mnttab, try elsewhere */
	}   /* end of if root mount entry */

	else	/* it's not the root's mount entry */
	{
	    /*
	    ** This isn't the root's mount entry, just use the
	    ** device/fs information from the kernel  -- Don't
	    ** try to CDF expand any NFS mount points or fsname,
	    ** as this may hang a client cnode doing an update_mnttab
	    ** before networking is enabled if the cnode's cluster
	    ** has nfs file systems mounted.
	    */
	    if (mnt_data.md_fstype != MOUNT_NFS)
	    {
		CDFEXPAND_NAME(mnttab_dir, mnt_dir, MNTMAXSTR - 1);
		CDFEXPAND_NAME(mnttab_fsname, mnt_fsname, MNTMAXSTR - 1);
	    }
	    else 
	    {
		/* don't try to CDF expand an NFS pathname */
		(void)strncpy(mnttab_fsname, mnt_fsname, MNTMAXSTR-1);
	        mnttab_fsname[MNTMAXSTR-1] = '\0';
		(void)strncpy(mnttab_dir, mnt_dir, MNTMAXSTR-1);
	        mnttab_dir[MNTMAXSTR-1] = '\0';
	    }

	    /* also look for option info from mnttab */
	    if (mnttab_head && 
		(m_info = find_in_mnttab(mnttab_dir,mnttab_head))) 
	    {
		/* if there's anything there, copy it in */
		if (m_info->options != '\0')
		    (void)strcat(mnttab_opts, m_info->options);
		mnttab.mnt_freq = m_info->freq;
		mnttab.mnt_passno = m_info->passno;
	    }
	    else 
	    {
		/* set up freq and passno to be empty */
		mnttab.mnt_freq = 0;
		mnttab.mnt_passno = 0;
	    }

	} /* end else not root mount entry */

	/* write the remaining fields of each entry into mnttab */

	switch (mnt_data.md_fstype)
	{
	case MOUNT_UFS:
	    mnttab.mnt_type = MNTTYPE_HFS;
	    break;
	case MOUNT_NFS:
	    if (mnt_data.md_nfsopts & NFSMNT_IGNORE)
	        mnttab.mnt_type = MNTTYPE_IGNORE;
	    else
	        mnttab.mnt_type = MNTTYPE_NFS;
	    break;
#ifdef CDROM
	case MOUNT_CDFS:
	    mnttab.mnt_type = MNTTYPE_CDFS;
	    break;
#endif
#ifdef PCFS
	case MOUNT_PC:
	    mnttab.mnt_type = MNTTYPE_PC;
	    break;
#endif
	default:
	    mnttab.mnt_type = "unknown";
	}

	/* get mount options info from flag bitmap */


	if (mnt_data.md_fsopts & M_RDONLY) /* RW is default */
		AddOpt(mnttab_opts, MNTOPT_RO);

	if (mnt_data.md_fsopts & M_NOSUID) /* SUID is default */
		AddOpt(mnttab_opts, MNTOPT_NOSUID);

#ifdef QUOTA
	if (mnt_data.md_ufsopts & MQ_ENABLED) /* NOQUOTA is default */
		AddOpt(mnttab_opts, MNTOPT_QUOTA);
#endif

	if (mnt_data.md_fstype == MOUNT_NFS)
	{
	    /* only check for these options if its NFS */

	    if (mnt_data.md_nfsopts&NFSMNT_WSIZE && mnt_data.md_wsize != DEFAULT_WSIZE)
		AddOptVal(mnttab_opts, MNTOPT_WSIZE, mnt_data.md_wsize);

	    if (mnt_data.md_nfsopts & NFSMNT_RSIZE &&
		mnt_data.md_rsize != DEFAULT_RSIZE)
		AddOptVal(mnttab_opts, MNTOPT_RSIZE, mnt_data.md_rsize);

	    if (mnt_data.md_nfsopts & NFSMNT_TIMEO &&
		mnt_data.md_timeo != DEFAULT_TIMEO)
		AddOptVal(mnttab_opts, MNTOPT_TIMEO, mnt_data.md_timeo);

	    if (mnt_data.md_nfsopts & NFSMNT_RETRANS &&
		mnt_data.md_retrans != DEFAULT_RETRANS)
		AddOptVal(mnttab_opts, MNTOPT_RETRANS, mnt_data.md_retrans);

	    if (mnt_data.md_port != DEFAULT_PORT)
		AddOptVal(mnttab_opts, MNTOPT_PORT, mnt_data.md_port);

	    if (mnt_data.md_nfsopts & NFSMNT_SOFT) /* HARD is default */
		AddOpt(mnttab_opts, MNTOPT_SOFT);

	    if (!(mnt_data.md_nfsopts & NFSMNT_INT)) /* INTR is default */
		AddOpt(mnttab_opts, MNTOPT_NOINTR);

	    if (mnt_data.md_nfsopts & NFSMNT_NODEVS) /* DEVS is default */
		AddOpt(mnttab_opts, MNTOPT_NODEVS);

	    /* new mount options in 9.0 */
	    if (mnt_data.md_nfsopts & NFSMNT_NOAC) /* AttrCacheing is default */
		AddOpt(mnttab_opts, MNTOPT_NOAC);

	    if (mnt_data.md_nfsopts & NFSMNT_NOCTO) /* AttrCacheing is default*/
		AddOpt(mnttab_opts, MNTOPT_NOCTO);

            if (mnt_data.md_nfsopts & NFSMNT_ACREGMIN &&
                mnt_data.md_acregmin != DEFAULT_ACREGMIN)
                AddOptVal(mnttab_opts, MNTOPT_ACREGMIN, mnt_data.md_acregmin);

            if (mnt_data.md_nfsopts & NFSMNT_ACREGMAX &&
                mnt_data.md_acregmax != DEFAULT_ACREGMAX)
                AddOptVal(mnttab_opts, MNTOPT_ACREGMAX, mnt_data.md_acregmax);

            if (mnt_data.md_nfsopts & NFSMNT_ACDIRMIN &&
                mnt_data.md_acdirmin != DEFAULT_ACDIRMIN)
                AddOptVal(mnttab_opts, MNTOPT_ACDIRMIN, mnt_data.md_acdirmin);

            if (mnt_data.md_nfsopts & NFSMNT_ACDIRMAX &&
                mnt_data.md_acdirmax != DEFAULT_ACDIRMAX)
                AddOptVal(mnttab_opts, MNTOPT_ACDIRMAX, mnt_data.md_acdirmax);

	}			/* end of if NFS options */

	/* if all default options, use default string */
	if (mnttab_opts[0] == '\0')
	    strcpy(mnttab_opts, MNTOPT_DEFAULTS);
	else 
	{
	    /* remove the last comma */
	    mnttab_opts[strlen(mnttab_opts)-1] = '\0';
	}

	/* fill in mount time and mounting cnode id */
	mnttab.mnt_time = (long)(mnt_data.md_mnttime);
	mnttab.mnt_cnode = mnt_data.md_msite;

	/* write this entry to userland mount table */
	(void)addmntent(write_fp, &mnttab);
    } /* end for each entry in kernel mnttab */

    free_mnttab_list(mnttab_head);

    (void)endmntent(write_fp);	/* close the mount table */

    /* reset the access time to match the kernel time */
    mnt_times.actime = timebuf;
    mnt_times.modtime = timebuf;
    (void)utime(MNT_MNTTAB,&mnt_times);

    return 0;
}				/* update_mnttab */
