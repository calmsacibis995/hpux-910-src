#ifdef SecureWare
/*
 * Copyright (c) 1989 SecureWare, Inc.
 *   All rights reserved
 */

#ident "@(#)remshd_sec.c	2.1 10:06:37 9/21/89 SecureWare"

/*
 * This Module contains Proprietary Information of SecureWare, Inc. and
 * should be treated as Confidential.
 */

/*
 * This library module contains routines called from remshd and rexecd.
 */

#include <sys/types.h>
#include <pwd.h>
#include <sys/security.h>
#include <prot.h>
#ifdef B1
#include <mandatory.h>
#endif
#include <audnetd.h>

extern int errno;

extern u_short validation;
extern uid_t ruid, luid;
extern u_long raddr, laddr;
extern char *rusr, *lusr, *rhost;

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
	{ \
		enable_priv_for_audit(); \
		audit_daemon(NA_RSLT_FAILURE, validation, \
			raddr, rhost, ruid, rusr, luid, lusr, NA_EVNT_START, \
			NA_MASK_RUID|NA_MASK_LUID|\
			(raddr == (u_long)-1 ? NA_MASK_RADDR : 0)|\
			(rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
			(rusr == (char *)0 ? NA_MASK_RUSRNAME : 0)|\
			(lusr == (char *)0 ? NA_MASK_LUSR : 0), (s)); \
		disable_priv_for_audit(); \
	}
#else /* not notdef */

#define AUDIT_SUCCESS(event,s)
#define AUDIT_FAILURE(s)

#endif /* notdef */

static struct pr_passwd *ppr = (struct pr_passwd *) NULL;
static mask_t   nosprivs[SEC_SPRIVVEC_SIZE];

/*
 * Check the protected password entry for a user. 
 * Set the user id and check the user's locked conditions.  
 * Set the user's individual audit mask and process priority.
 * If all is OK, set the user's LUID.
 * Returns 0 if successful, otherwise -1.
 */

remshd_check_prpw(pwd)
struct passwd *pwd;
{
	int Luid;
	int new_nice;
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
		error("Login incorrect.\n");
		AUDIT_FAILURE("No valid protected password entry.");
		return(-1);
	}
	if (locked_out(ppr)) {
		error("Account is disabled.\n");
		AUDIT_FAILURE("Account is disabled.");
		return(-1);
	}
	if (ppr->uflg.fg_type &&
	    ISBITSET(ppr->ufld.fd_type, AUTH_RETIRED_TYPE)) {
		error("Account has been retired.\n");
		AUDIT_FAILURE("Account has been retired.");
		return(-1);
	}
#ifdef B1
	if (ISB1)
		if (time_lock(ppr)) {
			error("Account is unavailable at this time.\n");
			AUDIT_FAILURE("Account is unavailable at this time.");
			return(-1);
		}
#endif /* B1 */

	/*
	 * Set the user's special audit parameters.
	 */
	audit_adjust_mask(ppr);

	/*
	 * Set the priority if necessary.  Since the return value
	 * of nice(2) can normally be -1 from the documentation, and
	 * -1 is the error condition, we key off of errno, not the
	 * return value to find if the change were successful.
	 * Note we must do this before the setuid(2) below.
	 */
	errno = 0;
	if (ppr->uflg.fg_nice)
		new_nice = ppr->ufld.fd_nice;
	else if (ppr->sflg.fg_nice)
		new_nice = ppr->sfld.fd_nice;

#ifdef B1
	if (ISB1)
		enablepriv(SEC_IDENTITY);
#endif /* B1 */
	if (ppr->uflg.fg_nice || ppr->sflg.fg_nice)  {
		(void) nice(new_nice);
		if (errno != 0)  {
			AUDIT_FAILURE("Bad priority setting");
#ifdef B1
			if (ISB1)
				disablepriv(SEC_IDENTITY);
#endif /* B1 */
			return(-1);
		}
	}

	Luid = setluid(pwd->pw_uid);
#ifdef B1
	if (ISB1)
		disablepriv(SEC_IDENTITY);
#endif /* B1 */
	return(Luid);
}

#ifdef B1
/*
 * set clearance to user's maximum clearance, from password database.
 * This assumes that current sensitivity has already been set (from
 * network) and will fail if current sensitivity exceeds the user's
 * maximum clearance.
 */

remshd_setclrnce(pwd)
{
	mand_ir_t *clearance;

	if (ppr->uflg.fg_clearance) {
		clearance = &ppr->ufld.fd_clearance;
	} else if (ppr->sflg.fg_clearance) {
		clearance = &ppr->sfld.fd_clearance;
	} else {
		mand_init();
		clearance = mand_syslo;
	}

	if (setclrnce(clearance) < 0) {
		error("Cannot set clearance.\n");
		AUDIT_FAILURE("Cannot set clearance.");
		return(-1);
	}
	return(0);
}
#endif /* B1 */

/*
 * return non-zero if the user is a privileged user.
 * In non-B1 case, simply return(pwd->pw_uid == 0).
 * In B1 case, a user is privileged if he/she has any of the
 * superuser-equivalent kernel authorizations that are not
 * also in the system default set.
 */

remshd_is_priv(pwd)
struct passwd *pwd;
{
#ifdef B1
	mask_t *defprivs; 	/* system default kernel authorizations */
	mask_t *userprivs;  	/* kernel authorizations specific to user */
	mask_t suprivs[SEC_SPRIVVEC_SIZE]; /* superuser equivalent privileges */
	int i;

	if (ISB1) {
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
 * check that the password sent over is correct. 
 * Compare with the one in the protected password database.
 * Return 0 on success, else nonzero.
 */

rexecd_check_passwd(password)
char *password;
{
	char *ciphertext;

	/*
	 * no password on account causes failure
	 */

	if (!ppr->uflg.fg_encrypt)
		return(1);
	ciphertext = bigcrypt(password, ppr->ufld.fd_encrypt);
	return strcmp(ciphertext, ppr->ufld.fd_encrypt);
}


/*
 * Set up the user's privileges according to the protected password db.
 * Return 0 on success, else -1.
 */

remshd_setup_privs()
{
	mask_t          *sysprivs;
#ifdef B1
	mask_t          *baseprivs;
#endif

        if (ppr->uflg.fg_sprivs)
                sysprivs = ppr->ufld.fd_sprivs;
        else if (ppr->sflg.fg_sprivs)
                sysprivs = ppr->sfld.fd_sprivs;
        else
                sysprivs = nosprivs;
#ifdef B1
	if (ISB1) {
        	if (ppr->uflg.fg_bprivs)
                	baseprivs = ppr->ufld.fd_bprivs;
        	else if (ppr->sflg.fg_bprivs)
                	baseprivs = ppr->sfld.fd_bprivs;
        	else
                	baseprivs = nosprivs;
        	if (setpriv(SEC_MAXIMUM_PRIV, sysprivs) < 0) {
			error("Cannot set kernel authorizations.\n");
			AUDIT_FAILURE("Cannot set kernel authorizations.");
			return(-1);
		}
        	if (setpriv(SEC_BASE_PRIV, baseprivs) < 0) {
			error("Cannot set base privileges.\n");
			AUDIT_FAILURE("Cannot set base privileges.");
			return(-1);
		}
	} else
#endif /* B1 */
	       {
        	if (setpriv(SEC_EFFECTIVE_PRIV, sysprivs)) {
			error("Cannot set effective privileges.\n");
			AUDIT_FAILURE("Cannot set effective privileges.");
			return(-1);
		}
	}
        endprpwent();
        endprdfent();
	return(0);
}

/*
 * after forking, remshd or rexecd parent process needs no privilege.
 */

remshd_drop_privs()
{
#ifdef B1
	if (ISB1)
		setpriv(SEC_MAXIMUM_PRIV, nosprivs);
	else
#endif /* not B1 */
		setpriv(SEC_EFFECTIVE_PRIV, nosprivs);
}

#endif /* SecureWare */
