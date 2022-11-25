
#ifndef lint
static char *HPUX_ID = "@(#) $Revision: 66.4 $";
#endif
/*
 * mesg -- set current tty to accept or
 *	forbid write permission.
 *
 *	mesg [-y] [-n]
 *		y allow messages
 *		n forbid messages
 *	return codes
 *		0 if messages are ON or turned ON
 *		1 if messages are OFF or turned OFF
 *		2 if usage error
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef TRUX
#include <sys/security.h>
#endif

struct stat sbuf;

char *tty;
char *ttyname();

main(argc, argv)
char *argv[];
{
	int i, c, r=0, errflag=0;
	extern int optind;

#ifdef SecureWare
	if(ISSECURE)
        	set_auth_parameters(argc, argv);
#endif
	for(i = 0; i <= 2; i++) {
		if ((tty = ttyname(i)) != NULL)
			break;
	}
#ifdef SecureWare
	if(ISSECURE)
        	mesg_check_tty(tty);
#endif
	if (tty == NULL)
		error("not a tty");
	if (stat(tty, &sbuf) < 0)
		error("cannot stat");
	if (argc < 2) {
#ifdef SecureWare
            if ((!ISSECURE) || (!mesg_setup_secure(tty, &sbuf)))
#endif
		if (sbuf.st_mode & 02)
			fputs("is y\n", stdout);
		else  {
			r = 1;
			fputs("is n\n", stdout);
		}
	}
	while ((c = getopt(argc, argv, "yn")) != EOF) {
		switch (c){
		case 'y':
			newmode(sbuf.st_mode | 022);
			break;
		case 'n':
			newmode(sbuf.st_mode & ~022);
			r = 1;
			break;
		case '?':
			errflag++;
		}
	}

	if (errflag /*  || (argc > optind) */ )
		error("usage: mesg [-y] [-n]");

/* added for temporary compat. */
	if(argc > optind) switch(*argv[optind]) {
		case 'y':
			newmode(sbuf.st_mode | 022);
			break;
		case 'n':
			newmode(sbuf.st_mode & ~022);
			r = 1;
			break;
		default:
			errflag++;
		}

	if (errflag)
		error("usage: mesg [-y] [-n]");
/* added to here */
	exit(r);
}

error(s)
char *s;
{
	fputs("mesg: ", stderr);
	fputs(s, stderr);
	fputc('\n', stderr);
	exit(2);
}

newmode(m)
{
#ifdef SecureWare
        if (ISSECURE ? (mesg_bad_tty_setting(tty, m)) : (chmod(tty, m) < 0))
#else
	if (chmod(tty, m) < 0)
#endif
		error("cannot change mode");
}
