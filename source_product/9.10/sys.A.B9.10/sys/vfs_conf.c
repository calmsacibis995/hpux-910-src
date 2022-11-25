/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vfs_conf.c,v $
 * $Revision: 1.6.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:15:23 $
 */

/* HPUX_ID: @(#)vfs_conf.c	55.1		88/12/23 */

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

/*	@(#)vfs_conf.c 1.1 86/02/03 SMI	*/
/*	@(#)vfs_conf.c	2.1 86/04/15 NFSSRC */

#include "../h/param.h"
#include "../h/sysmacros.h"
#include "../h/vfs.h"

extern	struct vfsops ufs_vfsops;	/* XXX Should be ifdefed */

#ifdef PCFS
extern	struct vfsops pcfs_vfsops;
#endif

struct vfsops *vfssw[] = {
	&ufs_vfsops,		/* 0 = MOUNT_UFS */
	(struct vfsops *)0,	/* 1 = MOUNT_NFS to be filled in by */
				/* nfs_init() or nfsc_link() */
	(struct vfsops *)0,	/* 2 = MOUNT_CDFS to be filled in by */
				/* cdfsc_link() */
#ifdef PCFS
	&pcfs_vfsops,		/* 3 = MOUNT_PC */
#else /* PCFS */
	(struct vfsops *)0,	/* 3 = MOUNT_PC */
#endif /* PCFS */
	(struct vfsops *)0,	/* 4 = Available */
	(struct vfsops *)0,	/* 5 = Available */
	(struct vfsops *)0,	/* 6 = Available */
	(struct vfsops *)0,	/* 7 = Available */
	(struct vfsops *)0,	/* 8 = Available */
	(struct vfsops *)0,	/* 9 = Available */
	(struct vfsops *)0,	/*10 = Available */
};

int nvfssw = sizeof(vfssw) / sizeof(vfssw[0]);
