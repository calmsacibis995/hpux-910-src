/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/dux/RCS/funcentry.c,v $
 * $Revision: 1.6.83.3 $	$Author: root $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 16:44:02 $
 */

/* HPUX_ID: @(#)funcentry.c	55.1		88/12/23 */

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
 * List of functions to be called for various DM messages
 */
#include "../h/param.h"
#include "../h/buf.h"
#include "../dux/dm.h"
#include "../dux/dmmsgtype.h"

int no_func(){}

int funcent_error();
/*
** IMPORTANT: This table is filled in by the dskless_link()
**		routine in dux_hooks.c at bootup time. If DUX
**		is configured in, the table is populated, else
**		it remains as is. If ANY functions are to be added
**		to the end of the table, they have to be defined in
**		 ../dux/dmmsgtype.h, and added to the list in
**		 ../dux/dux_hooks.c.
**
**			 dm_funcentry_assign(x,y,z)
**
**					daveg
*/
 

struct dm_funcentry dm_functions[] =
{
	{DM_UNUSED, NULL},		/*entry 0 is unused*/
	{0, 		no_func},	/*DMSIGNAL			01*/
	{0,		no_func},	/*DM_CLUSTER			02*/
	{0,		no_func},   	/*DM_ADD_MEMBER			03*/
	{0, 		no_func},	/*DM_READCONF			04*/
	{0, 		no_func},	/*DM_CLEANUP			05*/
	{0,		no_func},	/*DMNDR_READ			06*/
	{0,		no_func},	/*DMNDR_WRITE			07*/
	{0,		no_func},	/*DMNDR_OPEND			08*/
	{0,		no_func},	/*DMNDR_CLOSE			09*/
	{0,		no_func},	/*DMNDR_IOCTL			10*/
	{0,		no_func},	/*DMNDR_SELECT			11*/
	{0,		no_func},	/*DMNDR_STRAT			12*/
	{0, 		no_func},	/*DMNDR_BIGREAD			13*/
	{0,		no_func},	/*DMNDR_BIGWRITE		14*/
	{0,		no_func},	/*DMNDR_BIGIOFAIL		15*/
	{0,		no_func},	/*DM_LOOKUP			16*/
	{0,		no_func},	/*DMNETSTRAT_READ		17*/
	{0,		no_func},	/*DMNETSTRAT_WRITE		18*/
	{0,		no_func},	/*DMSYNCSTRAT_READ		19*/
	{0,		no_func},	/*DMSYNCSTRAT_WRITE		20*/
	{0,		no_func},	/*DM_CLOSE			21*/
	{0,		no_func},	/*DM_GETATTR			22*/
	{0,		no_func},	/*DM_SETATTR			23*/
	{0,		no_func},	/*DM_IUPDAT			24*/
	{0,		no_func},   	/*DM_SYNC			25*/
	{0,		no_func},	/*DM_REF_UPDATE			26*/
	{0, 		no_func},	/*DM_OPENPW			27*/
	{0, 		no_func},	/*DM_FIFO_FLUSH			28*/
	{0, 		no_func},	/*DM_PIPE			29*/
	{0, 		no_func},	/*DM_GETMOUNT			30*/
	{0, 		no_func},	/*DM_INITIAL_MOUNT_ENTRY	31*/
	{0,		no_func},   	/*DM_MOUNT_ENTRY		32*/
	{0, 		no_func},	/*DM_UFS_MOUNT			33*/
	{0,		no_func},   	/*DM_COMMIT_MOUNT		34*/
	{0,	 	no_func},	/*DM_ABORT_MOUNT		35*/
	{0,	 	no_func},	/*DM_UMOUNT_DEV			36*/
	{0,	 	no_func},	/*DM_UMOUNT			37*/
	{0,	 	no_func},	/*DM_SYNCDISC			38*/
	{0,	 	no_func},	/*DM_FAILURE			39*/
	{0,	 	no_func},	/*DM_SERSETTIME			40*/
	{0, 		no_func},	/*DM_SERSYNCREQ			41*/
	{0, 		no_func},	/*DM_RECSYNCREP			42*/
	{0,	 	no_func},	/*DM_GETPIDS			43*/
	{0,	 	no_func},	/*DM_RELEASEPIDS		44*/
	{0,	 	no_func},	/*DM_LSYNC			45*/
	{0,	  	no_func},	/*DM_FSYNC			46*/
	{0,  		no_func},	/*DM_CHUNKALLOC			47*/
	{0,		no_func},	/*DM_CHUNKFREE			48*/
	{0, 		no_func},	/*DM_TEXT_CHANGE		49*/
	{0,	 	no_func},	/*DM_XUMOUNT			50*/
	{0,	 	no_func},	/*DM_XRELE			51*/
	{0, 		no_func},	/*DM_USTAT			52*/
	{0, 		no_func},	/*DM_RMTCMD 			53*/
	{0, 		no_func},	/*DM_ALIVE 			54*/
	{0,	 	no_func},	/*DM_DMMAX			55*/
	{0, 		no_func},	/*DM_SYMLINK 			56*/
	{0, 		no_func},	/*DM_RENAME 			57*/
	{0, 		no_func},	/*DM_FSTATFS			58*/
	{0,	 	no_func},	/*DM_LOCKF    			59*/
	{0,	 	no_func},	/*DM_PROCLOCKF 			60*/
	{0, 		no_func},	/*DM_LOCKWAIT			61*/
	{0,	 	no_func},	/*DM_INOUPDATE 			62*/
	{0, 		no_func},	/*DM_UNLOCKF		 	63*/
	/*
	** Need these as placeholders for NFS configurability.
	*/
	{DM_KERNEL|DM_LIMITED, funcent_error},   /*DM_NFS_UMOUNT        64 */
						 /*nfs_umount_serve        */
	{DM_KERNEL|DM_LIMITED, funcent_error},   /*DM_COMMIT_NFS_UMOUNT 65 */
						 /*nfs_umount_commit       */
	{DM_KERNEL|DM_LIMITED, funcent_error}, 	 /*DM_ABORT_NFS_UMOUNT  66 */
						 /*abort_nfs_umount	   */
	{0,	 	no_func},	/*DM_MARK_FAILED		67*/
	{0,	 	no_func},	/*DM_LOCK_MOUNT			68*/
	{0,	 	no_func},	/*DM_UNLOCK_MOUNT		69*/
#ifdef ACLS
	{0,	 	no_func},	/*DM_SETACL			70*/
	{0,	 	no_func},	/*DM_GETACL			71*/
#endif
#ifdef	POSIX
	{0,	 	no_func},	/*DM_FPATHCONF			72*/
#endif	POSIX
#ifdef AUDIT
	{0,	 	no_func},	/*DM_SETEVENT			73*/
	{0,	 	no_func},	/*DM_AUDCTL			74*/
	{0,	 	no_func},	/*DM_AUDOFF			75*/
	{0,	 	no_func},	/*DM_GETAUDSTUFF		76*/
	{0,	 	no_func},	/*DM_SWAUDFILE			77*/
	{0,	 	no_func},	/*DM_CL_SWAUDFILE		78*/
#endif
	{0,	 	no_func},	/*DM_FSCTL			79*/
#ifdef QUOTA
	{0,	 	no_func},	/*DM_QUOTACTL			80*/
	{0,	 	no_func},	/*DM_QUOTAONOFF			81*/
#endif
};

int funcentrysize = sizeof(dm_functions) / sizeof(struct dm_funcentry);


int dux_nop() 
{
	return(0);
}

int nop() 
{
	return(0);
}
/*
 * For NFS configurability, this array of indirect functions is included.
 * Its entries are initially NULL.  They are filled in by nfsc_link, 
 * which configures NFS when it's included in the kernel. --gmf.
 *  They are called with the NFSCALL macro in dm.h.
 */
int (*nfsproc[])() =
{
	dux_nop,	/* rfind (NFS_RFIND) */
	dux_nop,	/* makenfsnode (NFS_MAKENFSNODE) */
	dux_nop,	/* find_mntinfo (NFS_FIND_MNT) */
	dux_nop,	/* enter_mntinfo (NFS_ENTER_MNT) */
	dux_nop,	/* delete_mntinfo (NFS_DELETE_MNT) */
	dux_nop,	/* free_lock_with_lm (NFS_FREELOCKLM) */
	dux_nop,	/* clean_up_lm	(NFS_LMEXIT) */
};


/*
** This function is necessary in case we get a remote request
** that makes it through the dm layer and NFS is not configured
** into the kernel. We simply cannot have the associated knsp 
** exec a null function and return, we have to be concerned about
** releasing the associated dux_mbuf and possible file system buffer.
** This situation should not happen, and it would not be appropriate
** to panic the system, instead we have chosen to release the 
** associated resources and blow off the request after printing
** errors on the system console. - daveg
*/

funcent_error(message)
dm_message message;
{
register struct buf *bp;
register struct dm_header *hp = DM_HEADER(message);

	printf("ERROR: Bad dm op-code, releasing associated buffers.\n");
	printf("       Possible kernel  mismatch!.\n");
	bp = hp->dm_bufp;

	if( bp != NULL )
	{
		hp->dm_flags |= ~(DM_KEEP_BUF);
		dm_release(message, 1);
	}
	else
	{
		dm_release(message, 0);
	}
}





/*
** Because of the way the rootinit code is written, it is
** necessary to return zeros when the system has DUX configured
** out. This is how the local/remote root is determined. - daveg
*/
find_root_dummy(remoteroot)
register int *remoteroot;
{
	*remoteroot = 0;
}


/*
 * For DUX configurability, this array of indirect functions is included.
 * Its entries are initially NULL.  They are filled in by dskless_link, 
 * which configures DUX when it's included in the kernel.
 * They are called with the DUXCALL macro in dux_hooks.h - daveg
 */

int (*duxproc[])() = {
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, find_root_dummy, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
	dux_nop, dux_nop, dux_nop, dux_nop, dux_nop, dux_nop,
#ifdef hp9000s800
	/*
	 * DUX_INPUT function used by LAN drivers should be schednetisr()
	 * when just networking is configured in and dux_input() when
	 * DUX is configured in as well.
	 */
	dux_nop
#endif
};




