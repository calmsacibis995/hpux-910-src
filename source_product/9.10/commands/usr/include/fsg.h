/* @(#) $Revision: 66.3 $ */

#ifndef _FSG_INCLUDED
#define _FSG_INCLUDED
#ifdef __hp9000s800

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fair share group information
 */

struct fsg {
	char	*fs_grp;
	int	fs_id;
	int	fs_shares;
	char	*fs_passwd;
	char	**fs_mem;
};

#if defined(__cplusplus)
struct fsg *getfsgent(...);
struct fsg *getfsgnam(...);
struct fsg *getfsgid(...);
char *getfsgdef(...);
int getfsguser(...);
int setfsgfil(...);
int setfsgent(...);
int endfsgent(...);
#else
struct fsg *getfsgent();
struct fsg *getfsgnam();
struct fsg *getfsgid();
char *getfsgdef();
int getfsguser();
int setfsgfil();
int setfsgent();
int endfsgent();
#endif

#endif /* __hp9000s800 */

#ifdef __cplusplus
}
#endif

#endif /* _FSG_INCLUDED */
