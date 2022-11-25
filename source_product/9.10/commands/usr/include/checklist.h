/* @(#) $Revision: 66.4 $ */       
/*
 * File system table, see checklist(4)
 *
 * Used by dump, mount, umount, swapon, fsck, df, ...
 *
 * The fs_spec field is the block special name.  Programs
 * that want to use the character special name must create
 * that name by prepending a 'r' after the right most slash.
 * Quota files are always named "quotas", so if type is "rq",
 * then use concatenation of fs_file and "quotas" to locate
 * quota file.
 */

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifndef _CHECKLIST_INCLUDED /* allow multiple inclusions */
#define _CHECKLIST_INCLUDED
#include <mntent.h>

#ifdef __cplusplus
extern "C" {
#endif

#define	CHECKLIST	"/etc/checklist"

#define	CHECKLIST_RW	"rw"	/* read/write device */
#define	CHECKLIST_RO	"ro"	/* read-only device */
#define	CHECKLIST_SW	"sw"	/* swap device */
#define	CHECKLIST_XX	"xx"	/* ignore totally */

struct	checklist{
	char	*fs_spec;	/* special device name */
	char    *fs_bspec;	/* block special file name */
	char	*fs_dir;	/* file system path prefix */
	char	*fs_type;	/* file system type */
	int	fs_passno;	/* pass number on parallel fsck */
	int	fs_freq;	/* dump frequency, in days */
};

#if defined(__STDC__) || defined(__cplusplus)
  extern struct checklist *getfsent(void);
  extern struct checklist *getfsspec(const char *);
  extern struct checklist *getfsfile(const char *);
  extern struct checklist *getfstype(const char *);
  extern int setfsent(void);
  extern int endfsent(void);
#else /* not __STDC__ || __cplusplus */
  extern struct checklist *getfsent();
  extern struct checklist *getfsspec();
  extern struct checklist *getfsfile();
  extern struct checklist *getfstype();
  extern int setfsent();
  extern int endfsent();
#endif /* __STDC__ || __cplusplus */

#ifdef __cplusplus
}
#endif

#endif /* _CHECKLIST_INCLUDED */
