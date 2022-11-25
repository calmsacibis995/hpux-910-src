static char *HPUX_ID = "@(#) $Revision: 70.1 $";
/*
 * Print working (current) directory
 */

#ifdef NLS
#   define NL_SETN 1	/* set number */
#   include <nl_types.h>
    extern char *catgets();
#else
#   define catgets(i, sn,mn,s) (s)
#endif

#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>

/*
 * Define our own exit routine to avoid bringing in stdio stuff
 */
#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF ___exit exit
#   define exit ___exit
#endif

int
exit(code)
int code;
{
    _exit(code);
}

main(argc, argv)
int argc;
char *argv[];
{
    extern char *getcwd();
/*     extern int ___getcwd_noexec; */

    char *cwd;
    int len;
    char *(*fn)() = getcwd;
    char buf[MAXPATHLEN];

    /*
     * check for usage
     */
    if (argc > 1)
#if defined(DUX) || defined(DISKLESS)
    {
	extern char *gethcwd();

	if ((argv[1][1] != 'H') || (argv[1][0] != '-') || argc > 2)
	{
	    char *msg;
#   ifdef NLS
	    nl_catd nlmsg_fd = catopen("pwd", 0);
#   endif

	    msg = catgets(nlmsg_fd, NL_SETN, 1, "usage: pwd [-H]\n");
	    write(2, msg, strlen(msg));
	    return 1;
	}
	fn = gethcwd;
    }
#else
    {
	char *msg;
#   ifdef NLS
	nl_catd nlmsg_fd = catopen("pwd", 0);
#   endif
	msg = catgets(nlmsg_fd, NL_SETN, 2, "usage: pwd\n");
	write(2, msg, strlen(msg));
	return 1;
    }
#endif

/*    ___getcwd_noexec = 1; */
    if ((cwd = (*fn)(buf, MAXPATHLEN)) == NULL && errno == EACCES)
    {
	extern uid_t getuid();
	extern gid_t getuid();

	uid_t ruid = getuid();
	gid_t rgid = getgid();

	if (geteuid() != ruid)
	    setuid(ruid);
	if (getegid() != rgid)
	    setgid(rgid);

	cwd = (*fn)(buf, MAXPATHLEN);
    }

    if (cwd == (char *)0)
    {
	perror("pwd");
	return errno;
    }

    len = strlen(cwd);
    cwd[len = strlen(cwd)] = '\n';
    write(1, cwd, len+1);
    return 0;
}
