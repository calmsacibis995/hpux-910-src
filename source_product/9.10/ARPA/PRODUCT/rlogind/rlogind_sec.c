#ifdef SecureWare
/*
 * Copyright (c) 1989 SecureWare, Inc.
 *   All rights reserved
 */

#ifndef lint
static char rcsid[] = "$Header: rlogind_sec.c,v 1.1.109.1 91/11/21 12:04:32 kcs Exp $"
;
#endif


/*
 * This Module contains Proprietary Information of SecureWare, Inc. and
 * should be treated as Confidential.
 */

/*
 * This library module contains routines called from rlogind.
 */

#include <sys/types.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>

#include <sys/security.h>
#include <prot.h>
#include <protcmd.h>
#ifdef B1
#include <mandatory.h>
#endif

#include <audnetd.h>

extern uid_t ruid, luid;
extern u_long raddr, laddr;
extern char *rusr, *lusr, *rhost;

/*
**    Auditing disabled because audit_daemon routine not yet available.
**    To re-enable auditing, remove the #ifdef notdef and the code
**    from the #else to the #endif.
*/

#ifdef notdef
#ifdef B1
#define enable_priv_for_audit()	\
		if (ISB1) { \
			enablepriv(SEC_ALLOWDACACCESS); \
			enablepriv(SEC_ALLOWMACACCESS); \
			enablepriv(SEC_WRITE_AUDIT); \
		}
#define disable_priv_for_audit()	\
		if (ISB1) { \
			disablepriv(SEC_ALLOWDACACCESS); \
			disablepriv(SEC_ALLOWMACACCESS); \
			disablepriv(SEC_WRITE_AUDIT); \
		}
#else /* not B1 */
#define enable_priv_for_audit()	
#define disable_priv_for_audit()	
#endif /* not B1 */

#define AUDIT_FAILURE(s)	\
	if (ISSECURE) { \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_FAILURE, NA_VALI_RUSEROK, \
			raddr, rhost, ruid, rusr, luid, lusr, NA_EVNT_START, \
			NA_MASK_RUID|NA_MASK_LUID|\
			(raddr == (u_long)-1 ? NA_MASK_RADDR : 0)|\
			(rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
			(rusr == (char *)0 ? NA_MASK_RUSRNAME : 0)|\
			(lusr == (char *)0 ? NA_MASK_LUSR : 0), (s)); \
		disable_priv_for_audit(); \
	}
#else /* not notdef */
#define AUDIT_FAILURE(s)
#endif /* not notdef */

static mask_t   nosprivs[SEC_SPRIVVEC_SIZE];
static struct pr_passwd *ppr = (struct pr_passwd *) NULL;

/*
 * Check the protected password entry for a user. 
 * Set the user's user id and check the user's locked conditions.  
 * Returns 0 if successful, otherwise -1.
 */

rlogind_check_prpw(pwd)
struct passwd *pwd;
{
#ifdef B1
	if (ISB1)
		enablepriv(SEC_ALLOWDACACCESS);
#endif /* B1 */
	ppr = getprpwnam(pwd->pw_name);
#ifdef B1
	if (ISB1)
		disablepriv(SEC_ALLOWDACACCESS);
#endif /* B1 */

	if (ppr == (struct pr_passwd *) 0 ||
	    ppr->uflg.fg_name == 0 || ppr->uflg.fg_uid == 0 ||
	    ppr->ufld.fd_uid != pwd->pw_uid ||
	    strcmp(ppr->ufld.fd_name, pwd->pw_name) != 0) {
		AUDIT_FAILURE("No valid protected password entry.");
		return(-1);
	}
	if (locked_out(ppr)) {
		AUDIT_FAILURE("Account is disabled.");
		return(-1);
	}
	if (ppr->uflg.fg_type &&
	    ISBITSET(ppr->ufld.fd_type, AUTH_RETIRED_TYPE)) {
		AUDIT_FAILURE("Account has been retired.");
		return(-1);
	}
#ifdef B1
	if ((ISB1) && time_lock(ppr)) {
		AUDIT_FAILURE("Account is unavailable at this time.");
		return(-1);
	}
#endif /* B1 */
	return(0);
}

/*
 * return non-zero if the user is a privileged user.
 * In non-B1 case, simply return(pwd->pw_uid == 0).
 * In B1 case, a user is privileged if he/she has any of the
 * superuser-equivalent kernel authorizations that are not
 * also in the system default set.
 */

rlogind_is_priv(pwd)
struct passwd *pwd;
{
#ifdef B1
	mask_t *defprivs; 	/* system default kernel authorizations */
	mask_t *userprivs;  	/* kernel authorizations specific to user */
	mask_t suprivs[SEC_SPRIVVEC_SIZE]; /* superuser equivalent privileges */
	int i;

	if (ISB1) {
		if (ppr == (struct pr_passwd *) NULL)
			fatal("protected password entry not set");

		ADDBIT(suprivs, SEC_SUSPEND_AUDIT);
		ADDBIT(suprivs, SEC_CONFIG_AUDIT);
		ADDBIT(suprivs, SEC_WRITE_AUDIT);
		ADDBIT(suprivs, SEC_ACCT);
		ADDBIT(suprivs, SEC_LIMIT);
		ADDBIT(suprivs, SEC_LOCK);
		ADDBIT(suprivs, SEC_LINKDIR);
		ADDBIT(suprivs, SEC_MKNOD);
		ADDBIT(suprivs, SEC_MOUNT);
		ADDBIT(suprivs, SEC_SYSATTR);
		ADDBIT(suprivs, SEC_SETPROCIDENT);
		ADDBIT(suprivs, SEC_CHROOT);
		ADDBIT(suprivs, SEC_DEBUG);
		ADDBIT(suprivs, SEC_SHUTDOWN);
		ADDBIT(suprivs, SEC_FILESYS);
		ADDBIT(suprivs, SEC_REMOTE);
		ADDBIT(suprivs, SEC_KILL);
		ADDBIT(suprivs, SEC_OWNER);
		ADDBIT(suprivs, SEC_DOWNGRADE);
		ADDBIT(suprivs, SEC_WRITEUPCLEARANCE);
		ADDBIT(suprivs, SEC_WRITEUPSYSHI);
		ADDBIT(suprivs, SEC_CHPRIV);
		ADDBIT(suprivs, SEC_MULTILEVELDIR);
		ADDBIT(suprivs, SEC_ALLOWMACACCESS);
		ADDBIT(suprivs, SEC_ALLOWDACACCESS);

		if (ppr->sflg.fg_sprivs)
			defprivs = ppr->sfld.fd_sprivs;
		else
			defprivs = nosprivs;

		if (ppr->uflg.fg_sprivs)
			userprivs = ppr->ufld.fd_sprivs;
		else
			userprivs = nosprivs;

		/*
		 * return true if user has any of the
		 * superuser-equivalent kernel authorizations
		 * that are not in the system default set.
		 */

		for (i = 0; i < SEC_SPRIVVEC_SIZE; i++) {
			if ((userprivs[i] & ~(defprivs[i])) & suprivs[i]) {
				return(1);
			}
		}
		return(0);
	} else
#endif
		return(pwd->pw_uid == 0);
}

/*
** restore pty device files to their unused state
*/

void
rlogind_condition_line(line)
	register char *line;
{
	register int uid_to_set, gid_to_set;
	register struct group *g;
	register struct passwd *p;

	p = getpwnam("bin");
	if (p != (struct passwd *) 0)
		uid_to_set = p->pw_uid;
	else
		uid_to_set = 0;

	g = getgrnam("terminal");
	if (g != (struct group *) 0)
		gid_to_set = g->gr_gid;
	else
		gid_to_set = GID_NO_CHANGE;

	endgrent();

#ifdef B1
	if (ISB1) {
		enablepriv(SEC_OWNER);
	}
#endif /* B1 */
	if (chmod(line, 0600) < 0)
		syslog(LOG_ERR, "chmod(%s, 0600): %m", line);
	if (chown(line, uid_to_set, gid_to_set) < 0)
		syslog(LOG_ERR, "chown(%s, %d, %d): %m", line,
			uid_to_set, gid_to_set);

#ifdef B1
	if (ISB1) {
		/*
	 	 * Reset the sensitivity label of the pty to WILDCARD.
	 	 */
		enablepriv(SEC_ALLOWMACACCESS);
		if (chslabel(line, mand_er_to_ir(AUTH_F_WILD)) < 0)
			syslog(LOG_ERR, "chslabel(%s, %s): %m",
				line, AUTH_F_WILD);
		disablepriv(SEC_ALLOWMACACCESS);
	}
#endif /* B1 */

	/*
	 * Disable I/O on line.
	 * All future attempts at I/O on file descriptors referring
	 * to line will cause SIGSYS (?) to be sent to the process 
	 * attempting the I/O.
	 */
	if (stopio(line) < 0)
		syslog(LOG_ERR, "stopio(%s): %m", line);
#ifdef B1
	if (ISB1) {
		disablepriv(SEC_OWNER);
	}
#endif /* B1 */
}
#endif /* SecureWare */
