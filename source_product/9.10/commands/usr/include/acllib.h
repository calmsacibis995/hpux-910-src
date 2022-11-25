/* @(#) $Revision: 66.4 $ */    
/*
 * LIBRARY-LEVEL ACCESS CONTROL LIST AND ACL PATTERN MANAGEMENT
 *
 * These definitions support the acltostr(), strtoacl(), strtoaclpatt(),
 * setaclentry(), and fsetaclentry() library calls.
 */

#ifndef _ACLLIB_INCLUDED /* allow multiple inclusions */
#define _ACLLIB_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef _INCLUDE_HPUX_SOURCE

#include <sys/acl.h>		/* harmless convenience for the user */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * SYMBOLIC FORMS OF ACLS FOR acltostr():
 */

#define FORM_SHORT 0
#define FORM_LONG  1


/*
 * MAGIC VALUES FOR strtoacl(), strtoaclpatt(), setaclentry(), and
 * fsetaclentry():
 *
 * The ACL_ values are chosen to not collide with values in <sys/acl.h>
 * or <sys/getaccess.h>.
 * 
 * MODE_DEL is chosen to be greater than any legal mode value, allowing
 * for future definitions of up to five new (higher-significance) mode
 * bits.
 */
#define ACL_FILEOWNER	65510	/* stands for file's owner ID */
#define ACL_FILEGROUP	65510	/* stands for file's group ID */

#define ACL_ANYUSER	65511	/* stands for wildcard user  ID */
#define ACL_ANYGROUP	65511	/* stands for wildcard group ID */

#define MODE_DEL	0400	/* delete one ACL entry */

/*
 * MASK FOR VALID MODE BITS IN ACL ENTRIES:
 *
 * This is for use with onmode and offmode, e.g.:
 *
 *	if (((   mode  & MODEMASK & onmask ) == onmask)
 *	 && (((~ mode) & MODEMASK & offmask) == offmask))
 *	{
 *	    <entry matches pattern>...
 */
#define	MODEMASK (R_OK | W_OK | X_OK)

/*
 * ACL PATTERN STRUCTURE:
 *
 * Uses acl*_t types from acl.h.
 */
struct acl_entry_patt {
	aclid_t	   uid;		/* user ID  */
	aclid_t	   gid;		/* group ID */
	aclmode_t  onmode;	/* mode bits which must be on  */
	aclmode_t  offmode;	/* mode bits which must be off */
};

/*
 * PROCEDURE TYPES:
 */
#if defined(__STDC__) || defined(__cplusplus)
   extern char *acltostr(int, struct acl_entry *, int);
   extern int strtoacl(const char *, int, int, struct acl_entry *, int, int);
   extern int strtoaclpatt(const char *, int, struct acl_entry_patt *);
   extern int setaclentry(const char *, int, int, int);
   extern int fsetaclentry(int, int, int, int);
   extern int cpacl(const char *, const char *, int, int, int, int, int);
   extern int fcpacl(int, int, int, int, int, int, int);
   extern void chownacl(int, struct acl_entry *, int, int, int, int);
#else /* not __STDC__ || __cplusplus */
   extern char *acltostr();
   extern int strtoacl();
   extern int strtoaclpatt();
   extern int setaclentry();
   extern int fsetaclentry();
   extern int cpacl();
   extern int fcpacl();
   extern void chownacl();
#endif /* else not __STDC__ */

#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _ACLLIB_INCLUDED */
