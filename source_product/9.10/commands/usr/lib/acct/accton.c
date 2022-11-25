/* @(#) $Revision: 66.5 $ */    
/*
 *	accton [ file ]
 *
 *	Accton alone turns process accounting off. If file is given, it
 *	must be the name of an existing file, to which the kernel
 *	appends accounting records (see acct(2) and acct(5)).
 *
 */

#include	<sys/types.h>
#include	<errno.h>
#include	<sys/stat.h>
#include	<stdio.h>
#if defined(SecureWare) && defined(B1)
#include        <sys/security.h>
#include        <sys/audit.h>
#include        <prot.h>
#include 	<mandatory.h>
#endif

#define	ROOT	0
#define	ADM	4
#define	ERR	(-1)
#define	OK	0
#define	NOGOOD	1
#define	prmsg(X)	fprintf(stderr,"%s: %s\n",arg0,X);

#if defined(SecureWare) && defined(B1)
char auditbuf[80];
#endif

char		pacct[] = "/usr/adm/pacct";
char		wtmp[] = "/etc/wtmp";
char		*arg0;

main(argc,argv, environ)
int argc;
char **argv;
char **environ;
{
	register int	uid;
#ifdef TRUX
	char doit = 0;
#endif


        cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", 0 );
#if defined(SecureWare) && defined(B1)
	if(ISB1){
	    set_auth_parameters(argc, argv);
            initprivs();
	}
#endif
	arg0 = argv[0];
	uid = getuid();
#if defined(SecureWare) && defined(B1)
	if(ISB1){
            if(hassysauth(SEC_ACCT))  {
		(void) forcepriv(SEC_ALLOWMACACCESS);
		(void) forcepriv(SEC_ALLOWDACACCESS);
		(void) forcepriv(SEC_SETOWNER);
		doit++;
	    }
	}
	else{
	    if(uid == ROOT || uid == ADM) {

		if(setuid(ROOT) == ERR) {
			prmsg("Cannot setuid (check command mode and owner)");
			exit(1);
		}
		doit++;
	    }
	}
	if(doit){
#else
	if(uid == ROOT || uid == ADM) {

		if(setuid(ROOT) == ERR) {
			prmsg("Cannot setuid (check command mode and owner)");
			exit(1);
		}
#endif
		if(ckfile(pacct) == NOGOOD)
			exit(1);
		if(ckfile(wtmp) == NOGOOD)
			exit(1);

#if defined(SecureWare) && defined(B1)
                if ((ISB1) && (!enablepriv(SEC_ACCT)))  {
                        fprintf(stderr, "need `%s' potential privilege\n",
                                sys_priv[SEC_ACCT].name);
                        sprintf(auditbuf,
                                "does not have %s potential privilege",
                                sys_priv[SEC_ACCT].name);
                        audit_security_failure(OT_PROCESS, auditbuf,
                          "accton aborted", ET_INSUFF_PRIV);
                        exit(1);
                }
#endif
		if (argc > 1) {
			if(acct(argv[1]) == ERR) {
				if(errno == EBUSY)
					fprintf(stderr, "%s: %s %s\n",
						arg0,
						"Accounting is busy",
						"cannot turn accounting ON");
				else
					prmsg("Cannot turn accounting ON");
				exit(1);
			}
		}
	/*
	 * The following else branch currently never returns
	 * an ERR.  In other words, you may turn the accounting
	 * off to your heart's content.
	 */
		else if(acct((char *)0) == ERR) {
			prmsg("Cannot turn accounting OFF");
			exit(1);
		}
		exit(0);

	}
#if defined(SecureWare) && defined(B1)
	if(ISB1){
	    if (!doit) {
		fprintf(stderr, "accton: you must have the '%s' authorization\n"
                  , sys_priv[SEC_ACCT].name);
                sprintf(auditbuf, "does not have the %s authorization",
                  sys_priv[SEC_ACCT].name);
                audit_security_failure(OT_PROCESS, auditbuf,
                         "accton aborted", ET_SUBSYSTEM);
	    } else
		fprintf(stderr,
		  "%s: Check potential privileges on the command\n", argv[0]);
	}
	else
            fprintf(stderr,"%s: Permission denied - UID must be root or adm\n",
			arg0);
#else
        fprintf(stderr,"%s: Permission denied - UID must be root or adm\n", arg0
);
#endif
	exit(1);
}

ckfile(admfile)
register char	*admfile;
{
	struct stat		stbuf;
	register struct stat	*s = &stbuf;

#if defined(SecureWare) && defined(B1)
	if((ISB1) && (stat(admfile, s) == ERR))
	{
		if (create_file_securely(admfile,AUTH_SILENT) !=
		  CFS_GOOD_RETURN) {
			fprintf(stderr,
			  "%s: cannot create %s\n", arg0, admfile);
                        sprintf(auditbuf, "cannot create file %s securely",
                                admfile);
                        audit_security_failure(OT_REGULAR, auditbuf,
                                "accton aborted", ET_OBJECT_UNAV);
			return(NOGOOD);
		}
	}
#else
	if(stat(admfile, s) == ERR)
		if(creat(admfile, 0644) == ERR) {
			fprintf(stderr,"%s: Cannot create %s\n", arg0, admfile);
			return(NOGOOD);
		}

	if(s->st_uid != ADM || s->st_gid != ADM)
		if(chown(admfile, ADM, ADM) == ERR) {
			fprintf(stderr,"%s: Cannot chown %s\n", arg0, admfile);
			return(NOGOOD);
		}

	if(s->st_mode & 0777 != 0664)
		if(chmod(admfile, 0664) == ERR) {
			fprintf(stderr,"%s: Cannot chmod %s\n", arg0, admfile);
			return(NOGOOD);
		}
#endif

	return(OK);
}
