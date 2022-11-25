/* @(#) $Revision: 66.4 $ */
#ifndef _GRP_INCLUDED
#define _GRP_INCLUDED

#ifndef _SYS_STDSYMS_INCLUDED
#  include <sys/stdsyms.h>
#endif /* _SYS_STDSYMS_INCLUDED */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _INCLUDE_POSIX_SOURCE

#  ifndef _GID_T
#    define _GID_T
     typedef long gid_t;
#  endif /* _GID_T */

   struct	group {	/* see getgrent(3) */
	char	*gr_name;
	char	*gr_passwd;
#ifdef _CLASSIC_ID_TYPES
	int	gr_gid;
#else
	gid_t	gr_gid;
#endif
	char	**gr_mem;
   };

#  if defined(__STDC__) || defined(__cplusplus)
     extern struct group *getgrgid(gid_t);
     extern struct group *getgrnam(const char *);
#  else /* __STDC__ || __cplusplus */
     extern struct group *getgrgid();
     extern struct group *getgrnam();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  if defined(__STDC__) || defined(__cplusplus)
#    ifndef _STDIO_INCLUDED
#    include <stdio.h>
#    endif
     extern struct group *getgrent(void);
     extern void endgrent(void);
     extern void setgrent(void);
     extern struct group *fgetgrent (FILE *);
#  else /* __STDC__ || __cplusplus */
     extern struct group *getgrent();
     extern void endgrent();
     extern void setgrent();
     extern struct group *fgetgrent ();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _GRP_INCLUDED */
