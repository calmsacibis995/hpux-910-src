/* @(#)automount:	$Revision: 1.4.109.6 $	$Date: 93/12/13 16:42:47 $
*/
#ifndef lint
static char sccsid[] = 	"(#)auto_node.c	1.5 90/07/24 4.1NFSSRC Copyr 1990 Sun Micro";
#endif

#ifdef PATCH_STRING
static char *patch_2501="@(#) PATCH_9.0: auto_node.o $Revision: 1.4.109.6 $ 93/05/17 PHNE_2501 PHNE_2967"
;
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/* HP Native Language Support Declarations */

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1       /* set number */
#include <nl_types.h>
extern nl_catd catd;
#endif NLS

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <values.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/dir.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <rpc/clnt.h>
#include <netinet/in.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include "nfs_prot.h"
#define NFSCLIENT
typedef nfs_fh fhandle_t;
/* HPNFS - pdb */
#ifdef hpux
#define _NFS_AUTOMOUNT
#include <nfs/nfs.h>
#endif
#include <sys/mount.h>
#include "automount.h"

struct internal_fh {
	int	fh_pid;
	long	fh_time;
	int	fh_num;
};

static int fh_cnt = 3;
static right_pid = -1;
static time_t right_time;

struct queue fh_q_hash[FH_HASH_SIZE];

new_fh(vnode)
	struct vnode *vnode;
{
	time_t time();
	register struct internal_fh *ifh =
		(struct internal_fh *)(&vnode->vn_fh);

	if (right_pid == -1) {
		right_pid = getpid();
		(void) time(&right_time);
	}
	ifh->fh_pid = right_pid;
	ifh->fh_time = right_time;
	ifh->fh_num = ++fh_cnt;

	INSQUE(fh_q_hash[ifh->fh_num % FH_HASH_SIZE], vnode);
}

free_fh(vnode)
	struct vnode *vnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&vnode->vn_fh);

	REMQUE(fh_q_hash[ifh->fh_num % FH_HASH_SIZE], vnode);
}

struct vnode *
fhtovn(fh)
	nfs_fh *fh;
{
	register struct internal_fh *ifh = 
		(struct internal_fh *)fh;
	int num;
	struct vnode *vnode;

	if (ifh->fh_pid != right_pid || ifh->fh_time != right_time)
		return ((struct vnode *)0);
	num = ifh->fh_num;
	vnode = HEAD(struct vnode, fh_q_hash[num % FH_HASH_SIZE]);
	while (vnode) {
		ifh = (struct internal_fh *)(&vnode->vn_fh);
		if (num == ifh->fh_num)
			return (vnode);
		vnode = NEXT(struct vnode, vnode);
	}
	return ((struct vnode *)0);
}

int
fileid(vnode)
	struct vnode *vnode;
{
	register struct internal_fh *ifh =
		(struct internal_fh *)(&vnode->vn_fh);

	return (ifh->fh_num);
}

dirinit(mntpnt, map, opts, isdirect)
	char *mntpnt, *map, *opts;
{
	struct autodir *dir;
	struct filsys *fs;
	register fattr *fa;
	struct stat stbuf;
	int mydir = 0;
	struct link *link;
	extern int verbose, yp;
	char *check_hier();
	char *opt_check();
	char *p;

	for (dir = HEAD(struct autodir, dir_q); dir;
	    dir = NEXT(struct autodir, dir))
		if (strcmp(dir->dir_name, mntpnt) == 0)
			return;
	p = mntpnt + (strlen(mntpnt) - 1);
	if (*p == '/')
		*p = '\0';	/* trim trailing / */
	if (*mntpnt != '/') {
		syslog(LOG_ERR, (catgets(catd,NL_SETN,501, "dir %s must start with '/'")), mntpnt);
		return;
	}
	if (p = check_hier(mntpnt)) {
		(void) syslog(LOG_ERR, (catgets(catd,NL_SETN,502, "hierarchical mountpoints: %s and %s")),
			p, mntpnt);
		return;
	}
	if (p = opt_check(opts)) {
		syslog(LOG_ERR,
			(catgets(catd,NL_SETN,503, "WARNING: default option \"%s\" ignored for map %s")),
			p, map);
	}

	/*
	 * If it's a direct map then call dirinit
	 * for every map entry. Try first for a local
	 * file, then a NIS map.
	 */
	if (strcmp(mntpnt, "/-") == 0) {
		if (loaddirect_file(map, opts) < 0) {
			(void) loaddirect_yp(map, map, opts);
		}
		return;
	}

	/*
	 * Check whether there's something already mounted here
	 */
	for (fs = HEAD(struct filsys, fs_q); fs;
		fs = NEXT(struct filsys, fs)) {
		if (strcmp(fs->fs_mntpnt, mntpnt) == 0) {
			(void) syslog(LOG_ERR,
				(catgets(catd,NL_SETN,504, "WARNING: %s:%s already mounted on %s")),
				fs->fs_host, fs->fs_dir, mntpnt);
			break;
		}
	}

	/*
	 * Check whether the map (local file or NIS) exists
	 */
	if (*map != '-' && access(map, R_OK) != 0 && yp) {
		char *val ; int len;

		if (yp_match(mydomain, map, "x",
			1, &val, &len) == YPERR_MAP) {
			if (verbose)
				syslog(LOG_ERR, (catgets(catd,NL_SETN,505, "%s: Not found")), map);
			return;
		}
	}

	/*
	 * Create a mount point if necessary
	 */
	if (stat(mntpnt, &stbuf) == 0) {
		if ((stbuf.st_mode & S_IFMT) != S_IFDIR) {
			syslog(LOG_ERR, (catgets(catd,NL_SETN,506, "%s: Not a directory")), mntpnt);
			return;
		}
		if (verbose && !emptydir(mntpnt))
			syslog(LOG_ERR, (catgets(catd,NL_SETN,507, "WARNING: %s not empty!")), mntpnt);
	} else {
		if (mkdir_r(mntpnt)) {
			syslog(LOG_ERR, (catgets(catd,NL_SETN,508, "Cannot create directory %s: %m")), mntpnt);
			return;
		}
		mydir = 1;
	}

	dir = (struct autodir *)malloc(sizeof *dir);
	if (dir == NULL)
		goto alloc_failed;
	bzero((char *)dir, sizeof *dir);
	dir->dir_name = strdup(mntpnt);
	if (dir->dir_name == NULL) {
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_map = strdup(map);
	if (dir->dir_map == NULL) {
		free((char *)dir->dir_name);
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_opts = strdup(opts);
	if (dir->dir_opts == NULL) {
		free((char *)dir->dir_name);
		free((char *)dir->dir_map);
		free((char *)dir);
		goto alloc_failed;
	}
	dir->dir_remove = mydir;
	INSQUE(dir_q, dir);

	new_fh(&dir->dir_vnode);
	dir->dir_vnode.vn_data = (char *)dir;
	fa = &dir->dir_vnode.vn_fattr;
	fa->nlink = 1;
	fa->uid = 0;
	fa->gid = 0;
	fa->size = 512;
	fa->blocksize = 512;
	fa->rdev = 0;
	fa->blocks = 1;
	fa->fsid = 0;
	fa->fileid = fileid(&dir->dir_vnode);
	(void) gettimeofday((struct timeval *)&fa->atime, (struct timezone *)0);
	fa->mtime = fa->atime;
	fa->ctime = fa->atime;

	if (!isdirect) {
		/* The mount point is a directory.
		 * Set up links for it's "." and ".." entries.
		 */
		dir->dir_vnode.vn_type = VN_DIR;
		fa->type = NFDIR;
		fa->mode = NFSMODE_DIR + 0555;
		link = makelink(dir, "." , NULL, "");
		if (link == NULL)
			goto alloc_failed;
		link->link_death = MAXLONG;
		link->link_vnode.vn_fattr.fileid = fileid(&link->link_vnode);
		link = makelink(dir, "..", NULL, "");
		if (link == NULL)
			goto alloc_failed;
		link->link_death = MAXLONG;
		link->link_vnode.vn_fattr.fileid = fileid(&link->link_vnode);
	} else {
		/* The mount point is direct-mapped. Set it
		 * up as a symlink to the real mount point.
		 */
		dir->dir_vnode.vn_type = VN_LINK;
		fa->type = NFLNK;
		fa->mode = NFSMODE_LNK + 0777;
		fa->size = 20;
		link = (struct link *)malloc(sizeof *link);
		if (link == NULL)
			goto alloc_failed;
		dir->dir_vnode.vn_data = (char *)link;
		link->link_dir = dir;
		link->link_name = strdup(mntpnt);
		if (link->link_name == NULL) {
			free((char *)link);
			goto alloc_failed;
		}
		link->link_fs = NULL;
		link->link_path = NULL;
		link->link_death = 0;
	}
	return;

alloc_failed:
	syslog(LOG_ERR, (catgets(catd,NL_SETN,509, "dirinit: memory allocation failed: %m")));
	return;
}

/*
 *  Check whether the mount point is a
 *  subdirectory or a parent directory
 *  of any previously mounted automount
 *  mount point.
 */
char *
check_hier(mntpnt)
	char *mntpnt;
{
	register struct autodir *dir;
	register char *p, *q;

	for (dir = TAIL(struct autodir, dir_q) ; dir ; 
		dir = PREV(struct autodir, dir)) {
		if (strcmp(dir->dir_map, "-null") == 0)
			continue;
		p = dir->dir_name;
		q = mntpnt;
		for (; *p == *q ; p++, q++)
			if (*p == '\0')
				break;
		if (*p == '/' && *q == '\0')
			return (dir->dir_name);
		if (*p == '\0' && *q == '/')
			return (dir->dir_name);
		if (*p == '\0' && *q == '\0')
			return (dir->dir_name);
	}
	return (NULL);	/* it's not a subdir or parent */
}

emptydir(name)
	char *name;
{
	DIR *dirp;
	struct direct *d;

	dirp = opendir(name);
	if (dirp == NULL) 
		return (0);
	while (d = readdir(dirp)) {
		if (strcmp(d->d_name, ".") == 0)
			continue;
		if (strcmp(d->d_name, "..") == 0)
			continue;
		break;
	}
	(void) closedir(dirp);
	if (d)
		return (0);
	return (1);
}

loaddirect_file(map, opts)
	char *map, *opts;
{
	FILE *fp;
	int done = 0;
	char *line, *p1, *p2;
	extern char *get_line();
	char linebuf[1024];

	if ((fp = fopen(map, "r")) == NULL) {
		return -1;
	}

	while ((line = get_line(fp, linebuf, sizeof linebuf)) != NULL) {
		p1 = line;
		while (*p1 && isspace(*(u_char *)p1)) p1++;
		if (*p1 == '\0')
			continue;
		p2 = p1;
		while (*p2 && !isspace(*(u_char *)p2)) p2++;
		*p2 = '\0';
		if (*p1 == '+')
                {
                    /* HPNFS 11/15/93 - hv
                     * Also Check for included flat file maps.
                     */
                    if (loaddirect_file(p1+1, opts) < 0)
                        (void) loaddirect_yp(p1+1, map, opts);
                }
		else
			dirinit(p1, map, opts, 1);
		done++;
	}
	
	(void) fclose(fp);
	return done;
}

loaddirect_yp(ypmap, localmap, opts)
	char *ypmap, *localmap, *opts;
{
	int first, err;
	char *key, *nkey, *val;
	int kl, nkl, vl;
	char dir[100];
	extern int yp;

	if (!yp)
		return;

	first = 1;
	key  = NULL; kl  = 0;
	nkey = NULL; nkl = 0;
	val  = NULL; vl  = 0;

	for (;;) {
		if (first) {
			first = 0;
			err = yp_first(mydomain, ypmap, &nkey, &nkl, &val, &vl);
		} else {
			err = yp_next(mydomain, ypmap, key, kl, &nkey, &nkl,
				&val, &vl);
		}
		if (err) {
			if (err != YPERR_NOMORE && err != YPERR_MAP)
				syslog(LOG_ERR, "%s: %s",
					ypmap, yperr_string(err));
			return;
		}
		if (key)
			free(key);
		key = nkey;
		kl = nkl;

		if (kl < 2 || kl >= 100)
			continue;
		if (isspace(*(u_char *)key) || *key == '#')
			continue;
		(void) strncpy(dir, key, kl);
		dir[kl] = '\0';

		dirinit(dir, localmap, opts, 1);

		free(val);
	}
}

struct link *
makelink(dir, name, fs, linkpath)
	struct autodir *dir;
	char *name;
	struct filsys *fs;
	char *linkpath;
{
	struct link *link;
	char *malloc();
	register fattr *fa;

	link = findlink(dir, name);
	if (link == NULL) {
		link = (struct link *)malloc(sizeof *link);
		if (link == NULL)
			goto alloc_failed;
		link->link_name = strdup(name);
		if (link->link_name == NULL) {
			free((char *)link);
			goto alloc_failed;
		}
		INSQUE(dir->dir_head, link);
		link->link_fs = NULL;
		link->link_path = NULL;
		new_fh(&link->link_vnode);
		link->link_vnode.vn_data = (char *)link;
		link->link_vnode.vn_type = VN_LINK;
	}
	link->link_dir = dir;
	link->link_fs = NULL;
	if (link->link_path) {
		free(link->link_path);
		link->link_path = NULL;
	}
	if (fs) {
		link->link_fs = fs;
		fs->fs_rootfs->fs_death = time_now + max_link_time;
	}
	if (linkpath) {
		link->link_path = strdup(linkpath);
		if (link->link_path == NULL) {
			REMQUE(link->link_dir->dir_head, link);
			if (link->link_name)
				free(link->link_name);
			free((char *)link);
			goto alloc_failed;
		}
	}
	link->link_death = time_now + max_link_time;

	fa = &link->link_vnode.vn_fattr;
	fa->type = NFLNK;
	fa->mode = NFSMODE_LNK + 0777;
	fa->nlink = 1;
	fa->uid = 0;
	fa->gid = 0;
	fa->size = strlen(linkpath);
	if (fs) 
		fa->size += strlen(fs->fs_dir) + 1;
	fa->blocksize = 512;
	fa->rdev = 0;
	fa->blocks = 1;
	fa->fsid = 0;
	fa->fileid = fileid(&link->link_vnode);
	(void) gettimeofday((struct timeval *)&fa->atime, (struct timezone *)0);
	fa->mtime = fa->atime;
	fa->ctime = fa->atime;

	return (link);

alloc_failed:
	syslog(LOG_ERR, (catgets(catd,NL_SETN,510, "Memory allocation failed: %m")));
	return (NULL);
}

zero_link(link)
	struct link *link;
{
	link->link_death = 0;
	link->link_fs = (struct filsys *)0;
}

free_link(link)
	struct link *link;
{

	/* Don't remove a direct link - just zero it */

	if (link->link_dir->dir_vnode.vn_type == VN_LINK) {
		zero_link(link);
		return;
	}

	REMQUE(link->link_dir->dir_head, link);
	free_fh(&link->link_vnode);
	if (link->link_name)
		free(link->link_name);
	if (link->link_path)
		free(link->link_path);
	free((char *)link);
}

struct link *
findlink(dir, name)
	struct autodir *dir;
	char *name;
{
	struct link *link;

	for (link = HEAD(struct link, dir->dir_head); link;
	    link = NEXT(struct link, link))
		if (strcmp(name, link->link_name) == 0)
			return (link);
	return ((struct link *)0);
}

free_filsys(fs)
	struct filsys *fs;
{
	REMQUE(fs_q, fs);
	free(fs->fs_host);
	free(fs->fs_dir);
	free(fs->fs_mntpnt);
	if (fs->fs_opts)
		free(fs->fs_opts);
#ifdef hpux
	/* Needed for nfs_args in vfsmount - pdb */
	free(fs->fs_name);
#endif
	/* SUN code doesn't do this for some reason, so every mount causes
	 * the automounter to allocate another filsys structure causing
	 * growth of the automounter process.  cwb 5/19/93
	 */
	free(fs);
}

struct filsys *
alloc_fs(host, dir, mntpnt, opts)
	char *host, *dir, *mntpnt, *opts;
{
	struct filsys *fs;

	fs = (struct filsys *)malloc(sizeof *fs);
	if (fs == NULL)
		goto alloc_failed;
	bzero((char *)fs, sizeof *fs);
	fs->fs_rootfs = fs;
	fs->fs_host = strdup(host);
	if (fs->fs_host == NULL) {
		free((char *)fs);
		goto alloc_failed;
	}
	fs->fs_dir = strdup(dir);
	if (fs->fs_dir == NULL) {
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	fs->fs_mntpnt = strdup(mntpnt);
	if (fs->fs_mntpnt == NULL) {
		free(fs->fs_dir);
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	if (opts != NULL) {
		fs->fs_opts = strdup(opts);
		if (fs->fs_opts == NULL) {
			free(fs->fs_mntpnt);
			free(fs->fs_dir);
			free(fs->fs_host);
			free((char *)fs);
			goto alloc_failed;
		}
	} else
		fs->fs_opts = NULL;
#ifdef hpux
	fs->fs_name = (char *)malloc( strlen(host) + strlen(dir) + 2 );
	if (fs->fs_name == NULL) {
		if (fs->fs_opts)
			free(fs->fs_opts);
		free(fs->fs_mntpnt);
		free(fs->fs_dir);
		free(fs->fs_host);
		free((char *)fs);
		goto alloc_failed;
	}
	(void) sprintf(fs->fs_name,"%s:%s", host, dir);
#endif
	INSQUE(fs_q, fs);
	return (fs);

alloc_failed:
	syslog(LOG_ERR, (catgets(catd,NL_SETN,511, "Memory allocation failed: %m")));
	return (NULL);
}

my_insque(head, item)
	struct queue *head, *item;
{
	item->q_next = head->q_head;
	item->q_prev = NULL;
	head->q_head = item;
	if (item->q_next)
		item->q_next->q_prev = item;
	if (head->q_tail == NULL)
		head->q_tail = item;
}

my_remque(head, item)
	struct queue *head, *item;
{
	if (item->q_prev)
		item->q_prev->q_next = item->q_next;
	else
		head->q_head = item->q_next;
	if (item->q_next)
		item->q_next->q_prev = item->q_prev;
	else
		head->q_tail = item->q_prev;
	item->q_next = item->q_prev = NULL;
}

do_timeouts()
{
	struct autodir *dir;
	struct link *link, *nextlink;
	struct filsys *fs, *nextfs;
	extern int trace, syntaxok;
	extern void check_mtab();

	if (trace > 1)
        {
              long    tv_sec;
              struct  tm      *tp, *localtime();
              (void) time(&tv_sec); /*      trace the current time ...  */
              tp = localtime(&tv_sec);
              (void) fprintf( stderr,
                              " %1$02d.%2$02d.%3$02d %4$02d:%5$02d:%6$02d  ",
                                tp->tm_year, tp->tm_mon+1, tp->tm_mday,
                                tp->tm_hour, tp->tm_min, tp->tm_sec);
		(void) fprintf(stderr, (catgets(catd,NL_SETN,512, "do_timeouts: enter\n")));
        }

	check_mtab();
	syntaxok = 1;

/* Problem statement:
 *
 * This is another case of the symlink to mount points are suddenly removed.
 *
 * The original code checked for the link->link_death before fs->fs_death.  
 * If the link_death has timeouted, zero_link() is called and the symlink to
 * the real mount point is deleted.  The code then checks for fs_death.  If
 * fs_death has timeouted, do_unmount() is called to unmount the idle
 * filesystem.  If the filesystem is busy, remount() is called to undo
 * everything.  The problem is that the symlink has already been deleted, and
 * the user ends up with a bad directory.
 *
 * Fix:
 *
 * Switch the order of checking for timeout.  Check the filesystem first.  If
 * the filesystem is successfully unmounted, fs pointer will be NULL; otherwise,
 * fs->fs_death will have a new value.  Then check the link.  If fs is NULL,
 * then call zero_link() to delete the symlink; otherwise, set link->link_death
 * to the value of fs->fs_death.
 *
 * Mod. 8/26/93 by hv
 */ 

 /* NOTE: The above is Hue Vue's original comment but its not really complete.
  * Such a bug would have been discovered long ago.  When I tried to reproduce
  * this problem I couldn't and after lots of work, I found that the conditions
  * are:
  *		1) The -hosts map is being used.
  *		2) The file system with shortest name that is exported by the
  *		   NFS server is not exported to this NFS client.
  *		3) The NFS client is not running NIS.
  *
  * You can see why it took me so long.
  * The problem is that the file system would time out and the automounter
  * would try to unmount it.  It would not succeed and then it would remount
  * everything.  Unfortunately, the automounter when not using -hosts and NIS
  * does not try to determine if it is possible to mount a files system, it
  * just attempts it.  Also it thinks the first file system is "special", but
  * in this case that file system could not be mounted.
  * After the link has timed out, it is recreated if the user trys to access it.
  * In lookup(), do_mount() is called to do the mount and it first checks to
  * see if the mount has already been done.  Unfortunately, it only checks
  * the first file system in the list, which is invalid in this case.  Even
  * worse, it then trys to mount the other file systems which of course fail
  * since they have already been mounted there.  Thus the link is not recreated
  * and the user is hosed.
  *
  * But after finally determining the real problem, it still looks like Hue
  * Vue's fix is a good one.  We can think of no reason to time out links to
  * file system we have mounted ourselves.  There are reasons to time out links
  * we have created to file systems we didn't mount, but none for the other
  * case.
  */

	for (fs = HEAD(struct filsys, fs_q); fs; fs = nextfs) {
		nextfs = NEXT(struct filsys, fs);
		if (fs->fs_mine) {
			if (fs != fs->fs_rootfs)
				continue;
			if (fs->fs_death > time_now)
				continue;
			if (!do_unmount(fs)) {
                           	if (trace)
                              		syslog(LOG_ERR,
                             		  	"do_timeouts() - cannot unmount %s, busy\n",
					  	fs->fs_name);
				fs->fs_death = time_now + max_link_time;
			}
		}
	}

	for (dir = HEAD(struct autodir, dir_q); dir;
            dir = NEXT(struct autodir, dir)) {
	      for (link = HEAD(struct link, dir->dir_head); link;
			link = nextlink) {
			nextlink = NEXT(struct link, link);
			if (link->link_death && link->link_death <= time_now) {
                          if (trace)
                            syslog(LOG_ERR,
                      "do_timeouts() - check symlink on %s\n", link->link_name);

                          if ((link->link_fs) && 
                              (link->link_fs->fs_rootfs->fs_death > link->link_death))
                          {
                              if (trace)
                                syslog(LOG_ERR,"fs is busy - keep the link\n");
                              link->link_death = link->link_fs->fs_rootfs->fs_death; 
                          }
                          else
                          {
                              if (trace)
                                syslog(LOG_ERR,"delete symlink\n");
			      zero_link(link);
                          }
                        }
		}
	}

	if (trace > 1)
		(void) fprintf(stderr, (catgets(catd,NL_SETN,513, "do_timeouts: exit\n")));
}

flush_links(fs)
	struct filsys *fs;
{
	struct link *link;
	struct vnode *vnode;
	int i;

	for (i = 0; i < FH_HASH_SIZE; i++) {
		vnode = HEAD(struct vnode, fh_q_hash[i]);
		for (; vnode; vnode = NEXT(struct vnode, vnode)) {
			if (vnode->vn_type != VN_LINK)
				continue;
			link = (struct link *)vnode->vn_data;
			if (link->link_fs == fs)
				zero_link(link);
		}
	}
}
