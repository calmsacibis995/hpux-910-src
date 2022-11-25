#ifdef SecureWare

static char RCS_ID[] = "@(#)$Header: inetd_sec.c,v 1.1.109.1 91/11/21 12:00:47 kcs Exp $";

#include "inetd.h"

static struct pr_passwd *ppr = (struct pr_passwd *) 0;
static mask_t   nosprivs[SEC_SPRIVVEC_SIZE];
static mand_ir_t *slabel = (mand_ir_t *) NULL;


#ifdef B1
/*
******************************************************************************
******************************************************************************

   Warning!  Instead of requiring the caller to have the REMOTE and KILL
	     privileges, this routine should verify that the caller is
	     authorized in the network protected subsystem, and should
	     later raise the KILL potential privilege before sending the
	     signal.

******************************************************************************
******************************************************************************
*/

/*
**  REMOTE and KILL privileges required for -c, -L, or -k options.
**
**  Returns 0 if successful, otherwise -1.
*/

inetd_check_opt_privs(opt)
char opt;
{
    if ((ISB1)) {
	switch (opt) {
	    case 'c':
	    case 'k':
	    case 'L':
		initprivs();
		if (!enablepriv(SEC_REMOTE)) {
		    fprintf(stderr,
			"inetd -%c: requires remote privilege\n", opt);
		    return(-1);
		}
		disablepriv(SEC_REMOTE);
		if (!enablepriv(SEC_KILL)) {
		    fprintf(stderr,
			"inetd -%c: requires kill privilege\n", opt);
		    return(-1);
		}
		disablepriv(SEC_KILL);
		break;
	}
    }
    return(0);
}
#endif /* B1 */


/*
**  For normal inetd operation, must verify the following at startup:
**    - luid not set;
#ifdef B1
**    - clearance and sensitivity not set.
#endif
**
**  Returns 0 if successful, otherwise -1.
*/

inetd_check_init_conds()
{
    if (!(getluid() < 0 && errno == EPERM)) {
	fprintf(stderr,
		"inetd: Login uid already set.  Must be run by init\n");
	return(-1);
    }
#ifdef B1
    if (ISB1) {
	if ((slabel = mand_alloc_ir()) == NULL) {
	    perror("inetd: mand_alloc_ir");
	    return(-1);
	}
	if (!(getclrnce(slabel) < 0 && errno == EINVAL)) {
	    fprintf(stderr,
		    "inetd: Clearance already set.  Must be run by init\n");
	    return(-1);
	}
	if (!(getslabel(slabel) < 0 && errno == EINVAL)) {
	    fprintf(stderr,
		    "inetd: Sensitivity already set.  Must be run by init\n");
	    return(-1);
	}
    }
#endif /* B1 */
    return(0);
}


#ifdef B1
/*
******************************************************************************
******************************************************************************

   Warning!  How to set sensitivity labels on the sockets for non-STREAM
	     servers remains an issue!

	     The following handles PRIMORDIAL DGRAM servers only.  It
	     assumes that data with sensitivity dominated by the label
	     on the socket will be appended to the socket, and thus that
	     an Unconstrained DGRAM socket is simply one at syshi.

******************************************************************************
******************************************************************************
*/

/*
**  For PRIMORDIAL non-STREAM server only,
**  initialize socket label to mand_syshi.
**  STREAM sockets, and not-previously labeled non-STREAM sockets
**  are labeled by the transport based on the incoming data.
**
**  Returns 0 if successful, otherwise -1.
*/

int
inetd_init_label(sep)
    struct servtab *sep;
{
#ifdef notdef
    if (sep->se_socktype != SOCK_STREAM &&
	    !strcmp(sep->se_user, "PRIMORDIAL")) {
	if (setsockopt(sep->se_fd, SOL_SOCKET, SO_HPSECLABEL,
		mand_syshi, sizeof (mand_ir_t *))) {
	    syslog(LOG_ERR, "%s: setsockopt(SO_HPSECLABEL): %m",
		    servicename(sep));
	    return(-1);
	}
    }
#endif /* notdef */
    return(0);
}


/*
**  Child's clearance must be set before sensitivity.  Set clearance to
**  syshi; this will later be dropped to the appropriate user's clearance,
**  either by inetd or by a PRIMORDIAL server.
**
**  Returns 0 if successful, otherwise -1.
*/

int
inetd_init_clrnce()
{
    if (setclrnce(mand_syshi) < 0) {
	syslog(LOG_ERR, "setclrnce(mand_syshi): %m");
	return(-1);
    }
    return(0);
}


/*
************************************************************************
************************************************************************
**
**  Warning!  How to set process sensitivity based on the label on the
**            socket has not been specified.  This routine currently
**            just fakes it.
**
**  STREAM servers:  Process sensitivity must be set to match the label
**                   on the accepted socket.  Exactly how to query the
**                   socket's label has not been specified.
**
**  DGRAM servers:  Exactly how process sensitivity must be set, the
**                  relation between the label on the incoming datagram
**                  and process sensitivity, and the semantics of trusted
**		    and untrusted DGRAM servers has not been specified.
**
************************************************************************
************************************************************************
*/

/*
**  Set process sensitivity appropriately based on server configuration
**  and label on incoming request.
**
**  Returns 0 if successful, otherwise -1.
*/
int
inetd_set_sensitivity(s)
    int s;
{
#ifdef notdef
    if (sep->se_socktype == SOCK_STREAM) {
	/*
	**  For STREAM server, set process sensitivity to match
	**  the label on the accepted socket.
	*/
	if (fstatslabel(s, slabel) < 0) {
	    syslog(LOG_ERR, "fstatslabel: %m");
	    return(-1);
	}
	if (setslabel(slabel) < 0) {
	    syslog(LOG_ERR, "setslabel: %m");
	    return(-1);
	}
    } else {
	/*
	**  For DGRAM server, ?
	*/  
    }
#else /* not notdef */
    /*
    **  Fake sensitivity setting here.
    */
    if (setslabel(mand_syslo) < 0) {
	syslog(LOG_ERR, "setslabel(mand_syslo): %m");
	return(-1);
    }
    return(0);
#endif /* not notdef */
}
#endif /* B1 */


/*
**  Called by inetd_set_identity() for non-trusted server.
**
**  Check the protected password entry and locked conditions
**  for the designated user.
**
**  If all is OK, set audit mask, priority, and luid.  
**
**  Returns 0 if successful, otherwise -1.
*/

int
inetd_check_prpw(pwd)
    struct passwd *pwd;
{
    int new_nice;

    errno = 0;
    ppr = getprpwnam(pwd->pw_name);

    if (ppr == (struct pr_passwd *) 0 ||
	    ppr->uflg.fg_name == 0 || ppr->uflg.fg_uid == 0 ||
	    ppr->ufld.fd_uid != pwd->pw_uid ||
	    strcmp(ppr->ufld.fd_name, pwd->pw_name) != 0) {
	syslog(LOG_ERR, "Unable to get protected password entry for user %s",
		pwd->pw_name);
	return(-1);
    }

    if (locked_out(ppr)) {
	syslog(LOG_ERR, "Account %s is disabled", pwd->pw_name);
	return(-1);
    }
    if (ppr->uflg.fg_type && ISBITSET(ppr->ufld.fd_type, AUTH_RETIRED_TYPE)) {
	syslog(LOG_ERR, "Account %s has been retired", pwd->pw_name);
	return(-1);
    }
#ifdef B1
    if (ISB1) {
	if (time_lock(ppr)) {
	    syslog(LOG_ERR, "Account %s is unavailable at this time",
		    pwd->pw_name);
	    return(-1);
	}
    }
#endif
    /*
     * Set the user's special audit parameters.
     */
    audit_adjust_mask(ppr);

    /*
     * Set the priority if necessary.  Since the return value
     * of nice(2) can normally be -1 from the documentation, and
     * -1 is the error condition, we key off of errno, not the
     * return value to find if the change were successful.
     * Note we must do this before the setuid(2).
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
    return(setluid(pwd->pw_uid));
}


#ifdef B1
/*
**  Called by inetd_set_identity() for non-trusted server.
**
**  Set clearance to user's maximum clearance, from password database.
**  This assumes that current sensitivity has already been set (from
**  network) and will fail if current sensitivity exceeds the user's
**  maximum clearance.
**
**  If neither user-specific nor system-default clearance are valid,
**  set clearance to syslo.
**
**  Returns 0 if successful, otherwise -1.
*/

inetd_setclrnce(pwd)
{
    mand_ir_t *clearance;

    if (ppr == (struct pr_passwd *) 0) {
	return(-1);
    }

    if (ppr->uflg.fg_clearance) {
	clearance = &ppr->ufld.fd_clearance;
    } else if (ppr->sflg.fg_clearance) {
	clearance = &ppr->sfld.fd_clearance;
    } else {
	clearance = mand_syslo;
    }

    if (setclrnce(clearance) < 0) {
	    return(-1);
    }
    return(0);
}
#endif /* B1 */

/*
**  Called by inetd_set_identity() for non-trusted server.
**
**  Initialize privileges according to user's protected password
**  entry.
**
**  Returns 0 if successful, otherwise -1.
*/

int
inetd_setup_privs(pwd)
{
    mask_t *sysprivs;
#ifdef B1
    mask_t *baseprivs;
#endif /* B1 */

    memset(nosprivs, 0, SEC_SPRIVVEC_SIZE*sizeof(mask_t));

    if (ppr == (struct pr_passwd *) 0) {
	return(-1);
    }
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
	if ((setpriv(SEC_MAXIMUM_PRIV, sysprivs) < 0) ||
	       (setpriv(SEC_BASE_PRIV, baseprivs) < 0)) {
	    endprpwent();
	    endprdfent();
	    return(-1);
	}
    } else {
	if (setpriv(SEC_EFFECTIVE_PRIV, sysprivs) < 0) {
	    endprpwent();
	    endprdfent();
	    return(-1);
	}
    }
#else /* not B1 */
    if (setpriv(SEC_EFFECTIVE_PRIV, sysprivs)) {
	endprpwent();
	endprdfent();
	return(-1);
    }
#endif /* not B1 */
    endprpwent();
    endprdfent();
    return(0);
}

/* 
**  If not PRIMORDIAL, set identity of server user and audit;
**  if PRIMORDIAL, just audit.
*/

int
inetd_set_identity(sep)
    struct servtab *sep;
{
    struct passwd *pwd;
    int primordial = !strcmp(sep->se_user, "PRIMORDIAL");

    /*
    **  PRIMORDIAL servers are assumed to be trusted.
    **
    **  - they inherit all privileges from inetd.
    **  - setuid, setgid, and setluid are either done
    **    by the server, or later, by login.
    **  - clearance has already been set to system high.
    **    This will be dropped by the server (or login)
    **    to the authenticated user's maximum clearance.
    **  - sensitivity has already been set to match the
    **    sensitivity of the incoming request.
    */

    /*
    **  non-PRIMORDIAL servers are not trusted.
    **
    **  - setuid, setgid, and setluid are done here.
    **  - they inherit only the privileges appropriate to the user
    **    specified in inetd.conf.
#ifdef B1
    **  - clearance has already been set to system high.
    **  - sensitivity has already been set to match the sensitivity
    **    of the incoming connection request or datagram.
    **  - the socket label has been set by the system to match the
    **    sensitivity of the incoming connection request or datagram.
    **  - clearance is dropped to the specified user's maximum clearance.
#endif
    */

    if (!primordial) {
	pwd = getpwnam(sep->se_user);
	if (pwd == (struct passwd *) NULL) {
	    syslog(LOG_ERR, "getpwnam: %s: No such user", sep->se_user);
	    return(-1);
	}
	if (inetd_check_prpw(pwd) < 0) {
	    return(-1);
	}
#ifdef B1
	if ((ISB1)) {
	    if (inetd_setclrnce(pwd) < 0) {
		syslog(LOG_ERR, "Unable to set clearance for user %s: %m",
			sep->se_user);
		return(-1);
	    }
	}
#endif /* B1 */
	/*
	**  set up user and group permissions
	*/
	initgroups(pwd->pw_name, pwd->pw_gid);
	setuid(pwd->pw_uid);
	setgid(pwd->pw_gid);
	lusr = pwd->pw_name;
    }
    /*
    ** audit connection establishment before
    ** dropping privileges
    */
    if (sep->se_bi == NULL) {
	    sprintf(audit_buf, "%s: exec \"%s \"",
		    servicename(sep), sep->se_server);
	    AUDIT_SUCCESS(NA_EVNT_CONN, audit_buf);
    }
    else {
	    sprintf(audit_buf, "%s: internal",
		    servicename(sep));
	    AUDIT_SUCCESS(NA_EVNT_CONN, audit_buf);
    }

    if (!primordial) {
	if (inetd_setup_privs(pwd) < 0) {
	    syslog(LOG_ERR, "Unable to setup privileges for user %s: %m",
		    sep->se_user);
	    sprintf(audit_buf,
		    "%s: Unable to set up privileges for user %s",
		    servicename(sep), sep->se_user);
	    AUDIT_FAILURE(audit_buf);
	    return(-1);
	}
    }
    return(0);
}

#endif /* SecureWare */
