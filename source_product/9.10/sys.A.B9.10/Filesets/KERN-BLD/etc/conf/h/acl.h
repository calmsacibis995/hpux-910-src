/* $Header: acl.h,v 1.5.83.3 93/09/17 17:01:46 root Exp $  */

/*
 * KERNEL-LEVEL ACCESS CONTROL LIST MANAGEMENT
 *
 * These definitions support the setacl() and getacl() system calls.
 */

#ifndef _SYS_ACL_INCLUDED
#define _SYS_ACL_INCLUDED


#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#  include "../h/types.h"
#else  /* ! _KERNEL_BUILD */
#  include <sys/types.h>
#endif /* _KERNEL_BUILD */


  /*
   * ACL SIZE VALUES:
   */

#  define NACLENTRIES  16	/* maximum per ACL, including base entries */
#  define NBASEENTRIES  3			   /* number of base entries */
#  define NOPTENTRIES (NACLENTRIES - NBASEENTRIES) /* max number of optional */


   /*
    * ACL ENTRY STRUCTURE:
    */

   typedef unsigned char aclmode_t;

#  ifdef _KERNEL

  /*
   * In the kernel, struct acl_tuple is the internal representation of the
   * ACL and acl_tuple_user is the external representation.
   *
   * Outside the kernel, struct acl_entry is the external representation of
   * the ACL and struct acl_entry_internal is the internal representation.  The
   * reason for this name game is that initially the internal and external
   * representations were the same.  The external representation changed to
   * conform to the POSIX definition of uid and gid.
   */

#  define NACLTUPLES	NACLENTRIES
#  define NBASETUPLES	NBASEENTRIES
#  define NOPTTUPLES	NOPTENTRIES

   typedef unsigned short aclid_t;

   struct acl_tuple {
	aclid_t	   uid;		/* user ID	*/
	aclid_t	   gid;		/* group ID	*/
	aclmode_t  mode;	/* see unistd.h	*/
   };

   struct acl_tuple_user {
	long	   uid;		/* user ID	*/
	long	   gid;		/* group ID	*/
	aclmode_t  mode;	/* see unistd.h	*/
   };

#  else /* not _KERNEL */

   /*
    * The long type used for uid and gid is required by POSIX:
    */

   typedef unsigned long aclid_t;

   struct acl_entry {
	aclid_t	   uid;		/* user ID	*/
	aclid_t	   gid;		/* group ID	*/
	aclmode_t  mode;	/* see unistd.h	*/
   };

   /*
    * Internal representation, for programs that compile using inode.h:
    */

   struct acl_entry_internal {
	unsigned short uid;	/* user ID	*/
	unsigned short gid;	/* group ID	*/
	aclmode_t  mode;	/* see unistd.h	*/
   };

#  endif /* not _KERNEL */



  /* Function prototypes */

#  ifndef _KERNEL
#  ifdef __cplusplus
       extern "C" {
#  endif /* __cplusplus */

#  ifdef _PROTOTYPES
      extern int getacl(const char *, int, struct acl_entry *);
      extern int fgetacl(int, int, struct acl_entry *);
      extern int setacl(const char *, size_t, const struct acl_entry *);
      extern int fsetacl(int, size_t, const struct acl_entry *);
#  else /* not _PROTOTYPES */
      extern int getacl();
      extern int fgetacl();
      extern int setacl();
      extern int fsetacl();
#  endif /* not _PROTOTYPES */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */
#  endif /* not _KERNEL */


  /*
   * NON-SPECIFIC USER AND GROUP ID:
   *
   * These are chosen to be greater than the greatest legal user/group ID value
   * (59999) but still fit in an unsigned short (< 65536), and to not collide
   * with values used by NFS (65535 == (unsigned) -1, 65534 == (unsigned) -2).
   */

#  define ACL_NSUSER	65500	/* non-specific user ID  */
#  define ACL_NSGROUP	65500	/* non-specific group ID */


  /*
   * DELETE OPTIONAL ENTRIES:
   *
   * This is used for the nentries argument to setacl().
   */

# define ACL_DELOPT	(-1)


#  ifdef _KERNEL

  /*
   * UNUSED ENTRY:
   *
   * This value appears in the user ID field to mark an unused entry.
   */

#  define ACLUNUSED	65501

  /*
   * CONVENIENCE MACROS:
   *
   * The following are used to manipulate base entries.
   *
   * User, group, or other specifiers; also, shift factors for base mode bits:
   */

#  define ACL_USER	6
#  define ACL_GROUP	3
#  define ACL_OTHER	0

  /*
   * Given an old basemode, a mode you wish to replace, and a specifier
   * for user, group or other, come up with the new basemode bits:
   */

#  define	setbasemode(__oldmode,__mode,__ugo) \
	      ((__oldmode & ~(7 << __ugo)) | (__mode << __ugo))

  /*
   * Given the base mode bits and a specifier for user, group, or other, return
   * the mode for that base entry:
   */

#  define getbasetuple(__basemode,__ugo) ((__basemode >> __ugo) & 7)

#  endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_ACL_INCLUDED */
