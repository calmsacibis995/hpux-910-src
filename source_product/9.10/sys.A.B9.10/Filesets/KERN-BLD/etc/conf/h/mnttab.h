/*
 * @(#)mnttab.h: $Revision: 1.11.83.4 $ $Date: 93/09/17 18:29:58 $
 * $Locker:  $
 * 
 */

#ifndef _SYS_MNTTAB_INCLUDED
#define _SYS_MNTTAB_INCLUDED

#define MNTLEN  32

/* Format of the /etc/mnttab file which is set by the mount(1m)
 * command
 */
struct mnttab {
	char	mt_dev[MNTLEN],
		mt_filsys[MNTLEN];
		short	mt_ro_flg;
	time_t	mt_time;
};

#endif /* not _SYS_MNTTAB_INCLUDED */
