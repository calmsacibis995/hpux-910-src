/*	@(#)mount.h	$Revision: 1.17.109.1 $	$Date: 91/11/19 14:42:35 $  */

/*      (c) Copyright 1987 Hewlett-Packard Company  */
/*      (c) Copyright 1985 Sun Microsystems, Inc.   */

#ifndef _RPCSVC_MOUNT_INCLUDED
#define _RPCSVC_MOUNT_INCLUDED

#define MOUNTPROG 100005
#define MOUNTPROC_MNT 1
#define MOUNTPROC_DUMP 2
#define MOUNTPROC_UMNT 3
#define MOUNTPROC_UMNTALL 4
#define MOUNTPROC_EXPORT 5
#define MOUNTPROC_EXPORTALL 6
#define MOUNTVERS_ORIG 1
#define MOUNTVERS 1

#ifndef svc_getcaller
#define svc_getcaller(x) (&(x)->xp_raddr)
#endif

bool_t xdr_path();
bool_t xdr_fhandle();
bool_t xdr_fhstatus();
bool_t xdr_mountlist();
bool_t xdr_exports();

struct mountlist {		/* what is mounted */
	char *ml_name;
	char *ml_path;
	struct mountlist *ml_nxt;
};

struct fhstatus {
	int fhs_status;
	fhandle_t fhs_fh;
};

/*
 * List of exported directories
 * An export entry with ex_groups
 * NULL indicates an entry which is exported to the world.
 */
struct exports {
	dev_t		  ex_dev;	/* dev of directory */
	char		 *ex_name;	/* name of directory */
	struct groups	 *ex_groups;	/* groups allowed to mount this entry */
	struct exports	 *ex_next;
	short		  ex_rootmap;	/* id to map root requests to */
	short		  ex_flags;	/* bits to mask off file mode */
	int               ex_opts;      /* export option flags */
};

struct groups {
	char		*g_name;
	struct groups	*g_next;
};

#endif /* _RPCSVC_MOUNT_INCLUDED */
