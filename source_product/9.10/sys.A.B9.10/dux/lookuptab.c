/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/lookuptab.c,v $
 * $Revision: 1.5.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:44:30 $
 */

/* HPUX_ID: @(#)lookuptab.c	55.1		88/12/23 */

/* 
(c) Copyright 1983, 1984, 1985, 1986, 1987, 1988 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate 
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984, 1986 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.

                    RESTRICTED RIGHTS LEGEND

          Use,  duplication,  or disclosure by the Government  is
          subject to restrictions as set forth in subdivision (b)
          (3)  (ii)  of the Rights in Technical Data and Computer
          Software clause at 52.227-7013.

                     HEWLETT-PACKARD COMPANY
                        3000 Hanover St.
                      Palo Alto, CA  94304
*/

/*
 *Table of DUX lookup entries
 */

#include "../h/param.h"
#include "../h/user.h"
#include "../h/stat.h"
#include "../h/vnode.h"
#include "../h/vfs.h"
#include "../ufs/inode.h"
#include "../dux/dux_lookup.h"
#include "../dux/lookup_def.h"
#include "../dux/lkup_dep.h"
#include "../dux/lookupmsgs.h"

/*The following structure includes the sizes of the request and response
 *for all lookup messages.  Every request includes AT LEAST a
 *struct dux_lookup_request.  Depending on the opcode, the actual
 *structure may be larger.  Every reply includes EITHER a struct
 *dux_lookup_reply OR some larger reponse structure.  The following
 *defines take these into account.
 */

#define REQ_SIZE(structure)	sizeof(structure)
#define EMPTY_REQ 		sizeof (struct dux_lookup_request)
#define RESP_SIZE(structure) \
	MAX (sizeof(structure), sizeof (struct dux_lookup_reply))
#define	EMPTY_RESP		sizeof (struct dux_lookup_reply)

extern int dux_pure_lookup_serve();
extern int dux_pure_lookup_unpack();
extern int dux_open_pack();
extern int dux_copen_unpack();
extern int dux_open_serve();
extern int dux_create_pack();
extern int dux_create_serve();
extern int dux_stat_unpack();
extern int dux_stat_serve();
extern int dux_access_pack();
extern int dux_access_serve();
extern int dux_namesetattr_pack();
extern int dux_namesetattr_serve();
extern int dux_remove_pack();
extern int dux_remove_serve();
extern int dux_mknod_serve();
extern int dux_link_pack();
extern int dux_link_serve();
extern int dux_getmdev_serve();
extern int dux_getmdev_unpack();
extern int dux_umount_serve();
extern int dux_norecord_serve();
extern int dux_norecord_unpack();
extern int dux_exec_serve();
extern int dux_exec_unpack();
extern int dux_statfs_unpack();
extern int dux_statfs_serve();
#ifdef ACLS
extern int lkup_setacl_pack();
extern int lkup_setacl_serve();
extern int lkup_getacl_unpack();
extern int lkup_getacl_pack();
extern int lkup_getacl_serve();
extern int lkup_getaccess_unpack();
extern int lkup_getaccess_serve();
#endif
#ifdef	POSIX
extern int lkup_pathconf_pack();
extern int lkup_pathconf_unpack();
extern int lkup_pathconf_serve();
#endif	POSIX


lookup_ops_t lookup_ops[] =
{
	{ /*LKUP_LOOKUP*/
		EMPTY_REQ,
		RESP_SIZE(struct pure_lookup_reply),
		NULL,
		dux_pure_lookup_unpack,
		dux_pure_lookup_serve
	},
	{ /*LKUP_OPEN*/
		REQ_SIZE(struct open_request),
		RESP_SIZE(struct copen_reply),
		dux_open_pack,
		dux_copen_unpack,
		dux_open_serve
	},
	{ /*LKUP_CREATE*/
		REQ_SIZE(struct create_request),
		RESP_SIZE(struct copen_reply),
		dux_create_pack,
		dux_copen_unpack,
		dux_create_serve
	},
	{ /*LKUP_STAT*/
		EMPTY_REQ,
		RESP_SIZE(struct stat_reply),
		NULL,
		dux_stat_unpack,
		dux_stat_serve
	},
	{ /*LKUP_ACCESS*/
		REQ_SIZE(struct access_request),
		EMPTY_RESP,
		dux_access_pack,
		NULL,
		dux_access_serve
	},
	{ /*LKUP_NAMESETATTR*/
		REQ_SIZE(struct namesetattr_request),
		EMPTY_RESP,
		dux_namesetattr_pack,
		NULL,
		dux_namesetattr_serve
	},
	{ /*LKUP_REMOVE*/
		REQ_SIZE(struct remove_request),
		EMPTY_RESP,
		dux_remove_pack,
		NULL,
		dux_remove_serve
	},
	{ /*LKUP_MKNOD*/
		REQ_SIZE(struct create_request),
		EMPTY_RESP,
		dux_create_pack,
		NULL,
		dux_mknod_serve
	},
	{ /*LKUP_LINK*/
		REQ_SIZE(struct link_request),
		EMPTY_RESP,
		dux_link_pack,
		NULL,
		dux_link_serve
	},
	{ /*LKUP_GETMDEV*/
		EMPTY_REQ,
		RESP_SIZE(struct getmdev_reply),
		NULL,
		dux_getmdev_unpack,
		dux_getmdev_serve
	},
	{ /*LKUP_UMOUNT*/
		EMPTY_REQ,
		EMPTY_RESP,
		NULL,
		NULL,
		dux_umount_serve
	},
	{ /*LKUP_NORECORD*/
		EMPTY_REQ,
		RESP_SIZE(struct pure_lookup_reply),
		NULL,
		dux_norecord_unpack,
		dux_norecord_serve
	},
	{ /*LKUP_EXEC*/
		EMPTY_REQ,
		RESP_SIZE(struct pure_lookup_reply),
		NULL,
		dux_exec_unpack,
		dux_exec_serve
	},
	{ /*LKUP_STATFS*/
		EMPTY_REQ,
		RESP_SIZE(struct statfs_reply),
		NULL,
		dux_statfs_unpack,
		dux_statfs_serve
	},
#ifdef ACLS
	{ /* LKUP_SETACL */
		REQ_SIZE(struct setacl_request),
		EMPTY_RESP,
		lkup_setacl_pack,
		NULL,
		lkup_setacl_serve
	},
	{ /* LKUP_GETACL */
		REQ_SIZE(struct getacl_request),
		RESP_SIZE(struct getacl_reply),
		lkup_getacl_pack,
		lkup_getacl_unpack,
		lkup_getacl_serve
	},
	{ /*LKUP_GETACCESS*/
		EMPTY_REQ,
		RESP_SIZE(struct getaccess_reply),
		NULL,
		lkup_getaccess_unpack,
		lkup_getaccess_serve
	},
#endif
#ifdef	POSIX
	{ /* LKUP_PATHCONF */
		REQ_SIZE(struct pathconf_request),
		RESP_SIZE(struct pathconf_reply),
		lkup_pathconf_pack,
		lkup_pathconf_unpack,
		lkup_pathconf_serve
	},
#endif	POSIX
};

