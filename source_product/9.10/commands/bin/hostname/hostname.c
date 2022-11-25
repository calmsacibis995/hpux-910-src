static char *HPUX_ID = "@(#) $Revision: 66.2 $";
/* SecureWare ID: @(#)hostname.c 1.2 10:48:46 10/20/89 */
#include <stdio.h>
#include <sys/param.h>
#if defined(SecureWare) && defined(B1)
#include <sys/types.h>
#include <sys/security.h>
#include <sys/audit.h>
#endif

/******************************************************************
 *    hostname
 *    this command calls the sethostname intrinsic to  set the node
 *    name in the  utsname.h system file. The syntax of the command
 *    is:
 *         hostname string
 *    the length of the string must be greater than 0 but less than
 *    or equal to MAXLEN characters.
 *    
 * HISTORY OF THIS MODULE
 *   
 *  19 Nov 82 - this module was created.
 *  19 Nov 82 - modified to return node name if null string specified.
 *  18 Apr 83 - modified to check for uid=0 if trying to set nodename
 *		and issue diagnostics if you are not the superuser.
 *
 *****************************************************************/

main(argc,argv)
int argc;                       /* argument count */
char *argv[];                   /* argument vector */
{
    char hostn[MAXHOSTNAMELEN + 1];

#ifdef TRUX
    if( ISSECURE ){
        set_auth_parameters(argc, argv);
#ifdef B1
        if (ISB1) initprivs();
#endif B1
    }
#endif TRUX

    if (argc >= 3)
    {
	fputs(stderr, "usage: \n", stderr);
	fputs(argv[0], stderr);
	fputs(" [string]\n", stderr);
	exit(1);
    }
    if (strlen(argv[1]) > MAXHOSTNAMELEN - 1)
    {
	fputs("hostname must be less than ", stderr);
	fputs(ltoa(MAXHOSTNAMELEN), stderr);
	fputs(" characters\n", stderr);
	exit(1);
    }
    if (argc == 1)
    {
	gethostname(hostn, MAXHOSTNAMELEN);
	fputs(hostn, stdout);
	fputc('\n', stdout);
    }
    else
    {
#if defined(SecureWare) && defined(B1)
	if (ISB1) {
        	if (!enablepriv(SEC_SYSATTR))
        	{
            		audit_security_failure(OT_PROCESS,
                        	"verify user has sysattr authorization",
                        	"does not have authorization, hostname not set",
                        	ET_INSUFF_PRIV);
            		fputs("You must have the sysattr authorization.\n",
                    		stderr);
            		exit(1);
        	}
	} else {
        	if (getuid() != 0)
        	{
            		fputs("You must be superuser to set the hostname\n",
                    		stderr);
            	exit(1);

        	}
	}

#else
	if (getuid() != 0)
	{
	    fputs("You must be superuser to set the hostname\n",
		    stderr);
	    exit(1);

	}
#endif
	sethostname(argv[1], strlen(argv[1])+1);
    }
    exit(0);
}
