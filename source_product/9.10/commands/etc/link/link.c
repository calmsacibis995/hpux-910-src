static char *HPUX_ID = "@(#) $Revision: 66.6 $";
/*
 * /etc/rename - rename a file
 * /etc/link   - link a file to another file
 * /etc/unlink - unlink a file
 *
 * Originally written by Edgar Circenis
 * Hewlett Packard - March 1990
 *
 * This program is primarily intended for use in shared library
 * recovery.  It allows a moved shared library to be renamed to
 * its original name, provided it is still on the same file system.
 *
 * This program MUST be compiled with libc.a (as opposed to libc.sl)
 * to be good for anything.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LINK   0
#define UNLINK 1
#define RENAME 2

main(argc,argv)
int argc;
char *argv[];
{
    extern int link(), unlink(), rename();
    extern uid_t geteuid();
    extern char *ltoa();
    extern char *sys_errlist[];
    extern int sys_nerr;

    char *prog;
    int cmd;
    int result;
    struct stat statb;

    static char *usages[] = {
	"Usage: /etc/link from to\n",
	"Usage: /etc/unlink name\n",
	"Usage: /etc/rename from to\n"
    };
    static char *progs[] = {
	"link: ",
	"unlink: ",
	"rename: "
    };
    static char not_root[] =
	"only root can link or unlink directories\n";

    /*
     * Get the program name, this determines what function we perform.
     */
    if ((prog= strrchr(argv[0], '/')) != NULL)
	prog++;
    else
	prog = argv[0];

    /*
     * Figure out what function to perform, "rename" is the default,
     * since it is the least harmful.  The commands are unique at
     * the first character, so we key off of it.
     */
    if (*prog == 'l')
	cmd = LINK;
    else if (*prog == 'u')
	cmd = UNLINK;
    else
	cmd = RENAME;

    /*
     * Make sure arg count is correct.
     */
    if (argc != (cmd == UNLINK ? 2 : 3))
    {
	write(2, usages[cmd], strlen(usages[cmd]));
	exit(1);
    }

    /*
     * If they aren't root, make sure that they aren't trying to
     * link or unlink a directory.
     */
    if ((cmd == UNLINK || cmd == LINK) &&
	geteuid() != 0 &&
	stat(argv[1], &statb) == 0 && S_ISDIR(statb.st_mode))
    {
	write(2, progs[cmd], strlen(progs[cmd]));
	write(2, not_root, sizeof not_root - 1);
	exit(2);
    }

    /*
     * Now do the operation.
     */
    if (cmd == UNLINK)
	result = unlink(argv[1]);
    else
	result = (*(cmd == LINK ? link : rename))(argv[1], argv[2]);

    if (result != 0)
    {
	int err = errno; /* save errno in case write() fails */

	write(2, progs[cmd], strlen(progs[cmd]));
	if (err > sys_nerr)
	{
	    char *s = ltoa(err);
	    write(2, "Unknown error (", 15);
	    write(2, s, strlen(s));
	    write(2, ")\n", 2);
	}
	else
	{
	    write(2, sys_errlist[err], strlen(sys_errlist[err]));
	    write(2, "\n", 1);
	}
	exit(2);
    }
    exit(0);
}

/*
 * Define our own exit so that crt0.o calls it instead of
 * the one that brings in all of the standard i/o stuff
 */
#ifdef _NAMESPACE_CLEAN
#   pragma _HP_SECONDARY_DEF ___exit exit
#   define exit ___exit
#endif

int
exit(code)
{
    _exit(code);
}
