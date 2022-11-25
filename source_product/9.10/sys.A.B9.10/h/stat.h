/* $Header: stat.h,v 1.28.83.5 93/10/19 16:20:23 drew Exp $ */

#ifndef _SYS_STAT_INCLUDED
#define _SYS_STAT_INCLUDED

/* stat.h: definitions for use with stat(), lstat(), and fstat() */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_POSIX_SOURCE
#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#endif /* _KERNEL_BUILD */

   /*
    * The stat structure filled in by stat(), lstat(), and fstat()
    */

   struct stat
   {
	dev_t	st_dev;
	ino_t	st_ino;
	mode_t	st_mode;
	nlink_t	st_nlink;
	unsigned short st_reserved1; /* old st_uid, replaced spare positions */
	unsigned short st_reserved2; /* old st_gid, replaced spare positions */
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	int	st_spare1;
	time_t	st_mtime;
	int	st_spare2;
	time_t	st_ctime;
	int	st_spare3;
	long	st_blksize;
	long	st_blocks;
	unsigned int	st_pad:30;
	unsigned int	st_acl:1;   /* set if there are optional ACL entries */
	unsigned int    st_remote:1;	/* Set if file is remote */
	dev_t   st_netdev;  	/* ID of device containing */
				/* network special file */
	ino_t   st_netino;  	/* Inode number of network special file */
	__cnode_t	st_cnode;
	__cnode_t	st_rcnode;
	/* The site where the network device lives			*/
	__site_t	st_netsite;
	short	st_fstype;
	/* Real device number of device containing the inode for this file*/
	dev_t	st_realdev;
	/* Steal three spare for the device site number                   */
	unsigned short	st_basemode;
	unsigned short	st_spareshort;
#ifdef _CLASSIC_ID_TYPES
	unsigned short st_filler_uid;
	unsigned short st_uid;
#else
	uid_t	st_uid;
#endif
#ifdef _CLASSIC_ID_TYPES
	unsigned short st_filler_gid;
	unsigned short st_gid;
#else
	gid_t	st_gid;
#endif
#define _SPARE4_SIZE 3
	long    st_spare4[_SPARE4_SIZE];
   };

#endif /* _INCLUDE_POSIX_SOURCE */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifdef _PROTOTYPES
     extern int chmod(const char *, mode_t);
     extern int fstat(int, struct stat *);
     extern int mkdir(const char *, mode_t);
     extern int mkfifo(const char *, mode_t);
     extern int stat(const char *, struct stat *);
#  else /* not _PROTOTYPES */
     extern int chmod();
     extern int fstat();
     extern int mkdir();
     extern int mkfifo();
     extern int stat();
#  endif /* not _PROTOTYPES */

#  ifdef _CLASSIC_POSIX_TYPES
     extern int umask();
#  else
#    ifdef _PROTOTYPES
       extern mode_t umask(mode_t);
#    else /* not _PROTOTYPES */
       extern mode_t umask();
#    endif /* not _PROTOTYPES */
#  endif /* not _CLASSIC_POSIX_TYPES */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_AES_SOURCE
#  ifdef _PROTOTYPES
      extern int fchmod(int, mode_t);
      extern int lstat(const char *, struct stat *);
#  else /* not _PROTOTYPES */
      extern int fchmod();
      extern int lstat();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_AES_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef _PROTOTYPES
      extern int mknod(const char *, mode_t, dev_t);
#  else /* not _PROTOTYPES */
      extern int mknod();
#  endif /* not _PROTOTYPES */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Symbols and macros for decoding the value of st_mode */

#ifdef _INCLUDE_POSIX_SOURCE
#  ifndef S_IRWXU		/* fcntl.h might have already defined these */
#    define S_ISUID 0004000	/* set user ID on execution */
#    define S_ISGID 0002000	/* set group ID on execution */

#    define S_IRWXU 0000700	/* read, write, execute permission (owner) */
#    define S_IRUSR 0000400	/* read permission (owner) */
#    define S_IWUSR 0000200	/* write permission (owner) */
#    define S_IXUSR 0000100	/* execute permission (owner) */

#    define S_IRWXG 0000070	/* read, write, execute permission (group) */
#    define S_IRGRP 0000040	/* read permission (group) */
#    define S_IWGRP 0000020	/* write permission (group) */
#    define S_IXGRP 0000010	/* execute permission (group) */

#    define S_IRWXO 0000007	/* read, write, execute permission (other) */
#    define S_IROTH 0000004	/* read permission (other) */
#    define S_IWOTH 0000002	/* write permission (other) */
#    define S_IXOTH 0000001	/* execute permission (other) */
#  endif /* S_IRWXU */

#  define _S_IFMT   0170000	/* type of file */
#  define _S_IFREG  0100000	/* regular */
#  define _S_IFBLK  0060000	/* block special */
#  define _S_IFCHR  0020000	/* character special */
#  define _S_IFDIR  0040000	/* directory */
#  define _S_IFIFO  0010000	/* pipe or FIFO */

#  define S_ISDIR(_M)  ((_M & _S_IFMT)==_S_IFDIR) /* test for directory */
#  define S_ISCHR(_M)  ((_M & _S_IFMT)==_S_IFCHR) /* test for char special */
#  define S_ISBLK(_M)  ((_M & _S_IFMT)==_S_IFBLK) /* test for block special */
#  define S_ISREG(_M)  ((_M & _S_IFMT)==_S_IFREG) /* test for regular file */
#  define S_ISFIFO(_M) ((_M & _S_IFMT)==_S_IFIFO) /* test for pipe or FIFO */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_XOPEN_SOURCE
#  define S_IFMT	_S_IFMT		/* type of file */
#  define S_IFBLK	_S_IFBLK	/* block special */
#  define S_IFCHR	_S_IFCHR	/* character special */
#  define S_IFDIR	_S_IFDIR	/* directory */
#  define S_IFIFO	_S_IFIFO	/* pipe or FIFO */
#  define S_IFREG	_S_IFREG	/* regular */
#  ifndef _XPG4
#    define S_ISVTX	0001000		/* save swapped text even after use */
#  endif /* not _XPG4 */
#endif /* _INCLUDE_XOPEN_SOURCE */

#ifdef _INCLUDE_AES_SOURCE
#  define S_IFLNK	0120000	/* symbolic link */
#  define S_ISLNK(_M) ((_M & S_IFMT)==S_IFLNK)   /* test for symbolic link */
#endif /* _INCLUDE_AES_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  define S_CDF		0004000	/* hidden directory */
#  define S_ENFMT	0002000	/* enforced file locking (shared w/ S_ISGID) */
#  define S_IFNWK	0110000 /* network special */
#  define S_IFSOCK	0140000	/* socket */

#  ifndef S_ISVTX
#    define S_ISVTX	0001000	/* save swapped text even after use */
#  endif /* not S_ISVTX */

#  define S_ISNWK(_M) ((_M & S_IFMT)==S_IFNWK)  /* test for network special */
#  define S_ISSOCK(_M) ((_M & S_IFMT)==S_IFSOCK) /* test for socket */
#  define S_ISCDF(_M) (S_ISDIR(_M) && (_M & S_CDF)) /* test for hidden dir */


/* Some synonyms used historically in the kernel and elsewhere */

#  define S_IREAD  	S_IRUSR	/* read permission, owner */
#  define S_IWRITE 	S_IWUSR	/* write permission, owner */
#  define S_IEXEC  	S_IXUSR	/* execute/search permission, owner */

#  define st_rsite st_rcnode

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef _UNSUPPORTED

	/* 
	 * NOTE: The following header file contains information specific
	 * to the internals of the HP-UX implementation. The contents of 
	 * this header file are subject to change without notice. Such
	 * changes may affect source code, object code, or binary
	 * compatibility between releases of HP-UX. Code which uses 
	 * the symbols contained within this header file is inherently
	 * non-portable (even between HP-UX implementations).
	*/
#ifdef _KERNEL_BUILD
#	include "../h/_stat.h"
#else  /* ! _KERNEL_BUILD */
#	include <.unsupp/sys/_stat.h>
#endif /* _KERNEL_BUILD */
#endif /* _UNSUPPORTED */

#endif /* _SYS_STAT_INCLUDED */
