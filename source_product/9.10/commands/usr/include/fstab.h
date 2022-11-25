/* @(#) $Revision: 66.6 $ */      
#ifndef _FSTAB_INCLUDED /* allow multiple inclusions */
#define _FSTAB_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_HPUX_SOURCE

/*
 * File system table, see fstab(4)
 *
 * Used by ported BSD applications
 *
 * The fs_spec field is the block special name.  Programs
 * that want to use the character special name may use the
 * fs_cspec field.
 */
#define	FSTAB		"/etc/checklist"

#define	FSTAB_RW	"rw"	/* read/write device */
#define	FSTAB_RQ	"rq"	/* read/write with quotas */
#define	FSTAB_RO	"ro"	/* read-only device */
#define	FSTAB_SW	"sw"	/* swap device */
#define	FSTAB_XX	"xx"	/* ignore totally */

struct	fstab{
	char	*fs_cspec;		/* char special device name */
	char	*fs_spec;		/* block special device name */
	char	*fs_file;		/* file system path prefix */
	char	*fs_type;		/* FSTAB_* */
	int	fs_passno;		/* pass number */
	int	fs_freq;		/* dump frequency, in days */
};

#if ((defined(__STDC__) || defined(__cplusplus)) && !defined(__lint))  
extern struct fstab *getfsent(void);
extern struct fstab *getfsspec(char *);
extern struct fstab *getfsfile(char *);
extern struct fstab *getfstype(char *);
extern int setfsent(void);
extern int endfsent(void);
#else
#ifndef __lint
extern struct fstab *getfsent();
extern struct fstab *getfsspec();
extern struct fstab *getfsfile();
extern struct fstab *getfstype();
extern int setfsent();
extern int endfsent();
#endif /* __lint */
#endif /* __STDC, __cplusplus */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _FSTAB_INCLUDED */
