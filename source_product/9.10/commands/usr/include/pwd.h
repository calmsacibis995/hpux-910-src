/* @(#) $Revision: 70.1 $ */
#ifndef _PWD_INCLUDED /* allow multiple inclusions */
#define _PWD_INCLUDED

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

#  ifndef _UID_T
#    define _UID_T
     typedef long uid_t;
#  endif /* _UID_T */

   struct passwd {
	char	*pw_name;
	char 	*pw_passwd;
#ifdef _CLASSIC_ID_TYPES
	int	pw_uid;
	int	pw_gid;
#else
	uid_t	pw_uid;
	gid_t	pw_gid;
#endif
	char 	*pw_age;
	char	*pw_comment;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
#ifdef AUDIT
	long	pw_audid;
	int	pw_audflg;
#endif
   };

#  if defined(__STDC__) || defined(__cplusplus)
     extern struct passwd *getpwuid(uid_t);
     extern struct passwd *getpwnam(const char *);
#  else /* __STDC__ || __cplusplus */
     extern struct passwd *getpwuid();
     extern struct passwd *getpwnam();
#  endif /* __STDC__ || __cplusplus */
#endif /* _INCLUDE_POSIX_SOURCE */

#ifdef _INCLUDE_HPUX_SOURCE
#  ifdef SHADOWPWD
   struct s_passwd {
       char    *pw_name;
       char    *pw_passwd;
       char    *pw_age;
#    ifdef AUDIT
       long     pw_audid;
       int     pw_audflg;
#    endif
   };

#  endif /* SHADOWPWD */

#  if defined(__STDC__) || defined(__cplusplus)
#    ifndef _STDIO_INCLUDED
#      include <stdio.h>
#    endif /* _STDIO_INCLUDED */

     extern void setpwent(void);
     extern void endpwent(void);
     extern struct passwd *getpwent(void);
     extern struct passwd *fgetpwent(FILE *);
#    ifdef SHADOWPWD
     extern struct s_passwd *getspwent(void);
     extern struct s_passwd *getspwuid(int);
     extern struct s_passwd *getspwaid(int);
     extern struct s_passwd *getspwnam(char *);
     extern struct s_passwd *fgetspwent(FILE *);
#    endif /* SHADOWPWD */
#  else /* __STDC__ || __cplusplus */
     extern void setpwent();
     extern void endpwent();
     extern struct passwd *getpwent();
     extern struct passwd *fgetpwent();
#    ifdef SHADOWPWD
     extern struct s_passwd *getspwent();
     extern struct s_passwd *getspwuid();
     extern struct s_passwd *getspwaid();
     extern struct s_passwd *getspwnam();
     extern struct s_passwd *fgetspwent();
#    endif /* SHADOWPWD */
#  endif /* __STDC__ || __cplusplus */

#  ifndef UID_NOBODY		/* Uid of NFS "nobody". */
#     define UID_NOBODY ((unsigned short) 0xfffe)
#  endif
#endif /* _INCLUDE_HPUX_SOURCE */

#ifdef __cplusplus
}
#endif

#endif /* _PWD_INCLUDED */
