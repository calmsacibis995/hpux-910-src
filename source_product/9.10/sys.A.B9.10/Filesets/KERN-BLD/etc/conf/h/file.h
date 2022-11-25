/* $Header: file.h,v 1.34.83.4 93/09/17 18:26:10 kcs Exp $ */

#ifndef _SYS_FILE_INCLUDED /* allows multiple inclusion */
#define _SYS_FILE_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#    include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */
#endif   /* _SYS_STDSYMS_INCLUDED  */

#ifdef _KERNEL_BUILD
#include "../h/types.h"
#include "../h/fcntl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>
#include <fcntl.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

/*
 * Descriptor table entry.
 * One for each kernel object.
 */
struct	file {
	int	f_flag;		/* see below */
	short	f_type;		/* descriptor type */
	short	f_count;	/* reference count */
	short	f_msgcount;	/* references from message queue */
#ifdef __cplusplus
	struct	fileops {
		int	(*fo_rw)(...);
		int	(*fo_ioctl)(...);
		int	(*fo_select)(...);
		int	(*fo_close)(...);
	} *f_ops;
#else /* not __cplusplus */
	struct	fileops {
		int	(*fo_rw)();
		int	(*fo_ioctl)();
		int	(*fo_select)();
		int	(*fo_close)();
	} *f_ops;
#endif /* not __cplusplus */
	caddr_t	f_data;		/* ptr to file specific struct (vnode/socket) */
	off_t	f_offset;
#ifdef _WSIO /* DIL */
	caddr_t	f_buf;
#endif /* _WSIO */
	struct	ucred *f_cred;	/* credentials of user who opened file */
};

#ifdef _KERNEL
extern	struct	file *file, *fileNFILE, *file_reserve;
extern	int	nfile;
extern  int     file_pad;
extern  struct  file *getf();
extern	struct	file **getfp();
extern	char	getp();
extern	char 	*getpp();
extern  struct  file *falloc();
#endif /* _KERNEL */

/* 
 * flags- also for fcntl call.
 */
#define	FOPEN		(-1)
#define	FREAD		000001		/* descriptor read/receive'able */
#define	FWRITE		000002		/* descriptor write/send'able */
/*
  FNDELAY and FAPPEND have equivalents in fcntl.h.  The numbers should 
  be the same!  HP replicates them because Bell replicates them.
 */
#define	FNDELAY		000004		/* no delay */
#define	FAPPEND		000010		/* append on each write */
#define	FMARK		000020		/* mark during gc() */
#define	FDEFER		000040		/* defer for next gc pass */
#define FNBLOCK	       0200000		/* POSIX */

/* bits to save after open */
/* FMASK defines the bits to save after open */
/* FCNTLCANT defines the bits that can't be modified with fcntl() */
/* FNOCTTY has equivalent in fcntl.h, O_NOCTTY. */
#define FNOCTTY		0400000 /* do not affiliate terminal device on open */
#define FMASK		(FREAD|FWRITE|FNDELAY|FNBLOCK|FAPPEND|FSYNCIO|FNOCTTY)
#define FCNTLCANT	(FREAD|FWRITE|FMARK|FDEFER|FNOCTTY)

/*
  FCREAT, FTRUNC and FEXCL have equivalents in fcntl.h.  The numbers must
  be the same!  HP replicates them because Bell replicates them.
 */
/* open only modes */
#define	FCREAT		00400		/* create if nonexistant */
#define	FTRUNC		01000		/* truncate to zero length */
#define	FEXCL		02000		/* error if already created */

/*
 * Access call.
 */
/*
  These have equivalents in unistd.h.  The numbers must
  be the same!  HP replicates them because Bell replicates them.
 */
#ifndef R_OK    
#define	F_OK		0	/* does file exist */
#define	X_OK		1	/* is it executable by caller */
#define	W_OK		2	/* writable by caller */
#define	R_OK		4	/* readable by caller */
#endif

/*
 * Lseek call.
 */
/*
  These have equivalents in unistd.h (SEEK_*).  The numbers must
  be the same!  HP replicates them because Bell replicates them.
 */
#define	L_SET		0	/* absolute offset */
#define	L_INCR		1	/* relative to current offset */
#define	L_XTND		2	/* relative to end of file */

#ifdef _KERNEL
/*
 * Convert a user supplied file descriptor into a pointer, with side effect
 * of returning if there is no file pointer for this file descriptor.
 */
#define GETF(fp, fd) { \
	if (((fp) = getf((fd))) == NULL) \
		return; \
}
/*
 * GETFP returns a pointer to location u.u_ofile.ofile[fd].  Should be
 * used when you need to modify u.u_ofile to make it point to something else.
 */
#define GETFP(fpp, fd) { \
	(fpp) = getfp((fd)); \
}
/*
 * GETP returns the contents of u.u_ofile.pofile[fd].  Should be used when
 * you need to examine u.u_ofile.pofile[fd].  
 */
#define GETP(c, fd) { \
	(c) = getp((fd)); \
}
/*
 * GETPP returns a pointer to location u.u_ofile.pofile[fd].  Should be used
 * when you need to modify u.u_ofile.pofile[fd].  
 */
#define GETPP(cp, fd) { \
	(cp) = getpp((fd)); \
}
#define FREEFP(fd, fp) { \
        if ((fp) != NULL) { \
                uffree(fd); \
                crfree((fp)->f_cred); \
		FPENTRYFREE(fp); \
                } \
}
/*
 * FPENTRYFREE frees an active (in-use) file table entry and decrements the 
 * number of active file table entries (for sar(1), etc. support).
 */
#define FPENTRYFREE(fp) { \
	(fp)->f_count = 0; \
	/* When the mpcntrs struct is added to mp.h -> UN-comment next line. \
	MPCNTRS.activefiles--; \
	*/ \
}

#endif /* _KERNEL */

#define	DTYPE_VNODE	1	/* file */
#define	DTYPE_SOCKET	2	/* Berkeley IPC Socket */
#define DTYPE_UNSP      5       /* user nsp control */
#define DTYPE_LLA	6	/* link-level lan access */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_FILE_INCLUDED */
