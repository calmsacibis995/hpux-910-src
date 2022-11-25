# ifdef SecureWare

# 	ifndef lint
static char rcsid[] = "$Header: sendmail_sec.c,v 1.2.109.3 95/02/21 16:08:43 mike Exp $";
# 	endif /* not lint */

# 	include "sendmail.h"
# 	include <signal.h>
# 	include <pwd.h>
# 	include <errno.h>

char actionbuf[BUFSIZ];
char resultbuf[BUFSIZ];

# 	ifdef B1
/*
**  Dull-witted privilege manipulation routine for non-PRIMORDIAL
**  trusted programs with potential privileges.  Doesn't truncate calling
**  process' base/effective privileges, and doesn't cache privilege
**  vectors (raisepriv always calls getpriv first).
**
**  Returns old effective privs, unless a null pointer is passed in, so
**  that the caller can restore former effective privileges with setpriv.
**
**      priv_t old_privs[SEC_SPRIVVEC_SIZE];
**
**      #if defined(SecureWare) && defined(B1)
**	    if (ISB1) {
**		    raisepriv(SEC_SOME_PRIV, old_privs);
**		    do_something_requiring_privilege();
**		    setpriv(SEC_EFFECTIVE_PRIV, old_privs);
**	    }
**      #endif
**
**  The caller can raise multiple privileges by saving the old effective
**  privs on only the first call to raisepriv:
**
**		    raisepriv(SEC_SOME_PRIV, old_privs);
**		    raisepriv(SEC_SOME_OTHER_PRIV, NULL);
**		    raisepriv(SEC_YET_ANOTHER_PRIV, NULL);
**		    do_something_requiring_privilege();
**		    setpriv(SEC_EFFECTIVE_PRIV, old_privs);
*/


/*
**  Raise a privilege in process' effective set.  This will succeed if
**  the program has the potential privilege (usual case), or, if not, if
**  the process happens to have it in its base set. 
**
**  Returns 1 if setpriv succeeds or if process already had the privilege.
**  Copies old effective privilege vector to vect, which must be big enough,
**  unless vect is NULL.
*/

bool
raisepriv(priv, vect)
	int priv;
	priv_t *vect;
{
	priv_t eff_privs[SEC_SPRIVVEC_SIZE];

	getpriv(SEC_EFFECTIVE_PRIV, eff_privs);
	if (vect != NULL)
		bcopy(eff_privs, vect, sizeof(eff_privs));
	if (ISBITSET(eff_privs, priv))
		return(1);
	ADDBIT(eff_privs, priv);
	return(setpriv(SEC_EFFECTIVE_PRIV, eff_privs) == 0);
}

/*
**  PRIV_AUDIT_SUBSYS -- bracket raising privileges needed to self-audit
**	efficiently around call to audit_subsystem
**
**	Parameters:
**		action, result, event_type -- parameters to audit_subsystem
**
**	Returns:
**		none.
**
**	Side Effects:
**		calls audit_subsystem
*/

static void
priv_audit_subsys(action, result, event_type)
	char *action, *result;
	int event_type;
{
	priv_t eff_privs[SEC_SPRIVVEC_SIZE];
	
	raisepriv(SEC_ALLOWDACACCESS, eff_privs);
	raisepriv(SEC_WRITE_AUDIT, NULL);
	audit_subsystem(action, result, event_type);
	setpriv(SEC_EFFECTIVE_PRIV, eff_privs);
}

/*
**  SENDMAIL_INIT_SEC -- Establish mail subsystem authorization,
**	    save initial sensitivity level, and if Primordial,
**	    or running as an SMTP or queue processing daemon,
**	    truncate all but default privileges.
**
**	Parameters:
**	    	none.
**
**	Returns:
**	    	none.
**
**	Side Effects:
**		sets globals InitialLevel and Primordial;
**		if Primordial, drops kernel auths to default.
*/

void
sendmail_init_sec()
{
	if (ISB1)
	{
		/*
		**  This setclrnce call will only succeed if:
		**	Clearance has not already been set.
		**  and
		**	The process has ALLOWMACACCESS.
		**  These will only be true if this is the sendmail
		**  daemon, and is being started primordially.
		**  Clearance is checked later in the daemon and
		**  queue code.
		*/
		InitialLevel = mand_alloc_ir();
		Primordial = (getclrnce(InitialLevel) < 0);
		if (Primordial && setclrnce(mand_syshi) < 0)
		{
			syserr("Cannot set clearance");
			exit(ExitStat);
		}

		/*
		**  Get initial sensitivity level;  If not set,
		**  must have been started Primordial.
		**  Set sensitivity to syslo, if possible.
		*/
		if (getslabel(InitialLevel) < 0)
		{
			if (setslabel(mand_syslo) < 0)
			{
				syserr("Cannot set sensitivity");
				exit(ExitStat);
			}
			if (getslabel(InitialLevel) < 0)
			{
				syserr("Cannot get initial sensitivity");
				exit(ExitStat);
			}
		}
	}
}
# 	endif /* B1 */
/*
**  SENDMAIL_IS_AUTHORIZED -- check whether calling user is authorized
**	for the mail protected subsystem.
**  
**	Parameters:
**		none.
**
**	Returns:
**	    	TRUE if authorized, else FALSE.
**
**	Side Effects:
**		sets static Authorized if not set;
**		authorized_user audits success or failure.
*/

bool
sendmail_is_authorized()
{
	static bool Authorized = -1;

	if ((ISSECURE) && (Authorized == -1))
	{
# 	ifdef B1
		if (ISB1)
		{
			/*
			**  This instance of sendmail is "Authorized" in
			**  the mail protected subsystem if the running
			**  user is authorized, or if it is being run by
			**  the *rc scripts at boot time.
			*/
			Authorized = (authorized_user("mail")) ||
				(Primordial);
		}
		else 
# 	endif /* B1 */
		{
			Authorized = ((authorized_user("mail")) ||
				(getuid() == 0));
		}
	}
	return(Authorized);
}
/*
**  SENDMAIL_CHECK_AUTH -- implement mail subsystem security policy
**  
**	If the user is authorized for the mail protected subsystem,
**	audit the action and return;  otherwise audit the refusal
**	and exit.
**
**	Parameters:
**	    	action -- requested action, for auditing
**	    	quiet -- if true, don't usrerr and exit on failure
**
**	Returns:
**	    	TRUE, if authorized;
**		FALSE, if not authorized, and quiet is set;
**		otherwise none (exits).
**
**	Side Effects:
**		sets static Authorized if not set;
**		audits success or failure;
**		if unauthorized and not quiet, exits.
*/

bool
sendmail_check_auth(action, quiet)
	char *action;
	bool quiet;
{
	bool r = 0;

	if (ISSECURE)
	{
		strcpy(actionbuf, action);
		actionbuf[0] = toupper(actionbuf[0]);

		if (!(r = sendmail_is_authorized())) {
			sprintf(resultbuf, "user %s NOT authorized",
				pw_idtoname(starting_luid()));
			priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
			if (!quiet)
			{
				errno = EACCES;
				usrerr("Cannot %s", action);
				exit(EX_NOPERM);
			}
		} else {
			sprintf(resultbuf, "user %s authorized",
				pw_idtoname(starting_luid()));
			priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
		}
	}
	return(r);
}

# 	ifdef B1
/*
**  DROP_PRIVS -- release all but default privileges
**
**	This routine is called by sendmail_pre_chdir if running as the
**	SMTP or queue processing daemon, or if started primordially.
**
**	Sendmail must not have the SEC_MULTILEVELDIR effective privilege
**	except when actually needed, so drop it explicitly.
**
**	Parameters:
**	    	none.
**
**	Returns:
**	    	none.
**
**	Side Effects:
**	    	possibly truncates kernel auths and base and effective
**		privileges.
*/

static void
drop_privs()
{
	struct pr_default *pprdf;
	priv_t privs[SEC_SPRIVVEC_SIZE];

	bzero(privs, sizeof(privs));

	if ((pprdf = getprdfnam(AUTH_DEFAULT)) ==
		(struct pr_default *) 0)
	{
		/*
		**  If default privileges cannot
		**  be accessed, give up everything.
		*/
		setpriv(SEC_MAXIMUM_PRIV, privs);
		setpriv(SEC_BASE_PRIV, privs);
		setpriv(SEC_EFFECTIVE_PRIV, privs);
	}
	else
	{
		if (pprdf->prg.fg_sprivs)
		{
			setpriv(SEC_MAXIMUM_PRIV,
			    pprdf->prd.fd_sprivs);
		}
		else
		{
			setpriv(SEC_MAXIMUM_PRIV, privs);
		}

		if (pprdf->prg.fg_bprivs)
		{
			setpriv(SEC_BASE_PRIV, pprdf->prd.fd_bprivs);
			/*
			**  The chdir to the queue directory won't work if
			**  the process happens to have the MULTILEVELDIR
			**  effective privilege by default.
			*/
			bcopy(pprdf->prd.fd_bprivs, privs, sizeof(privs));
			RMBIT(privs, SEC_MULTILEVELDIR);
			setpriv(SEC_EFFECTIVE_PRIV, privs);
		}
		else
		{
			setpriv(SEC_BASE_PRIV, privs);
			setpriv(SEC_EFFECTIVE_PRIV, privs);
		}
	}
}

/*
**  SENDMAIL_PRE_CHDIR -- initialization required after thaw but before 
**	freezing configuration or chdir'ing to queue directory.
**
**	The chdir following this function call will be to the
**	subdirectory of the multi-level queue directory
**	corresponding to the process' sensitivity.
**
**	If started primordially or as a long-lived daemon, drop
**	all but default privileges.
**
**	If running as the SMTP or queue processing daemon,
**	verify that sendmail was started with syshi clearance.
**
**	If running as the SMTP or queue processing  daemon,
**	freezing the configuration, or rebuilding the alias database,
**	set sensitivity to syslo first.
**
**	If queuemode, later in runqueue sendmail will loop around
**	setslabel and chdir to each of the single-level queue
**	subdirectories.
**
**	If running the SMTP daemon, sendmail will setslabel and chdir
**	as necessary to handle incoming connections at the appropriate
**	level.
**
**	Parameters:
**	    	queuemode -- true if running the queue
**
**	Returns:
**	    	none.
**
**	Side Effects:
**		exits in some cases.
**	    	Sets sensitivity to syslo in some cases.
*/

void
sendmail_pre_chdir(queuemode)
	bool queuemode;
{
	int rel;
	mand_ir_t *daemon_clrnce;

	/*
	**  If started primordially or running as a long-lived daemon,
	**  must give up all but default privileges.  
	*/
	if (Primordial || (OpMode == MD_DAEMON)
	    || queuemode && !(OpMode == MD_TEST || OpMode == MD_KILL))
	{
		drop_privs();
	}

	/*
	**  Verify that the queue directory really is multi-level
	*/
	raisepriv(SEC_MULTILEVELDIR, old_privs);
	if (ismultdir(QueueDir) != 1)
	{
		errno = 0;
		syserr("%s is not a multi-level directory", QueueDir);
		exit(EX_SOFTWARE);
	}
	setpriv(SEC_EFFECTIVE_PRIV, old_privs);

	/*
	**  Enforce requirement that daemon run cleared to syshi
	**  (queuemode is ignored if also MD_TEST or MD_KILL).
	*/
	daemon_clrnce = mand_alloc_ir();
	getclrnce(daemon_clrnce);

	if ((OpMode == MD_DAEMON)
	    || queuemode && !(OpMode == MD_TEST || OpMode == MD_KILL))
	{
		raisepriv(SEC_ALLOWMACACCESS, old_privs);
		rel = mand_ir_relationship(daemon_clrnce, mand_syshi);
		setpriv(SEC_EFFECTIVE_PRIV, old_privs);

		strcpy(actionbuf,
		    "Check clearance to run daemon and/or process mail queue");
		if (!(rel & MAND_EQUAL))
		{
			/* audit failure */
			sprintf(resultbuf, "user %s NOT cleared to syshi",
			    pw_idtoname(starting_luid()));
			priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
			errno = EACCES;
			usrerr("not cleared to run daemon or process mail queue");
			exit(EX_NOPERM);
		}
		else
		{
			/* audit success */
			sprintf(resultbuf, "user %s cleared to syshi",
			    pw_idtoname(starting_luid()));
			priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
		}
	}

	mand_free_ir(daemon_clrnce);

	/*
	**  If daemon, freeze, or newaliases, set sensitivity to syslo
	**  (queuemode is ignored if also MD_TEST or MD_KILL).
	*/
	if ((OpMode == MD_DAEMON)
	    || (OpMode == MD_FREEZE)
	    || (OpMode == MD_INITALIAS)
	    || queuemode && !(OpMode == MD_TEST || OpMode == MD_KILL))
	{
		raisepriv(SEC_ALLOWMACACCESS, old_privs);
		if (setslabel(mand_syslo) < 0)
		{
/*
**  KERNEL BUG!
**
**  The setslabel call above currently fails if the label has already been
**  set.  For now, syslog and go on.
**
**  When this has been fixed, uncomment the syserr and exit calls below,
**  and remove the syslog call.
**
			syserr("Cannot set sensitivity");
			exit(ExitStat);
**
*/
			syslog(LOG_DEBUG,
			    "KERNEL BUG: Cannot set sensitivity: %m");
		}
		setpriv(SEC_EFFECTIVE_PRIV, old_privs);
	}
	return;
}

/*
**  SENDMAIL_MLD_PRINTQUEUE -- print a representation of the mail queue
**			      for all levels to which I have access
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Calls printqueue for each accessible level of the multi-level
**		queue directory
*/

void
sendmail_mld_printqueue()
{
	mand_ir_t *queue_level;
	MDIR *mldp;
	char relname[MAXNAMLEN + 1],	/* relative subdir name as returned */
					/* by readmultdir        	    */
	     savename[MAXNAMLEN + 1],	/* saved relative name		    */
	     fullname[MAXNAMLEN + 1];	/* absolute pathname of subdir	    */
	struct dirent *sdp;
	char *er;

	/*
	**  The queue directory is a multi-level directory.
	**  For each single-level subdirectory, setslabel
	**  to process that level, chdir there, and call printqueue.
	*/

	bzero(relname, sizeof(relname));
	bzero(savename, sizeof(savename));
	bzero(fullname, sizeof(fullname));

	queue_level = mand_alloc_ir();
	raisepriv(SEC_MULTILEVELDIR, old_privs);

	/*
	**  If the process has ALLOWMACACCESS, the following will
	**  provide all the subdirectories.  Otherwise it will only
	**  provide those dominated by the current sensitivity.
	*/
	mldp = openmultdir(QueueDir, MAND_MLD_ALLDIRS,
	    (mand_ir_t *) NULL);
	if (mldp == (MDIR *) NULL)
	{
		syserr("printqueue: Cannot open %s as multi-level directory",
		    QueueDir);
		exit(EX_OSERR);
	}
	setpriv(SEC_EFFECTIVE_PRIV, old_privs);

	for (;;)
	{
# 		ifdef notdef
		raisepriv(SEC_MULTILEVELDIR, old_privs);
		/*
		**  readmultdir returns the subdir name for every
		**  file in the subdir.  I don't know why.
		**
		**  We are only want to chdir to each subdirectory once.
		*/
		while (strcmp(savename, relname) == 0)
		{
			readmultdir(mldp, relname, &sdp);
			if (sdp == (struct dirent *) NULL)
			{
				setpriv(SEC_EFFECTIVE_PRIV, old_privs);
				return;
			}
		}
		strcpy(savename, relname);

		sprintf(fullname, "%s/%s", QueueDir, relname);
		if (statslabel(fullname, queue_level) < 0)
		{
			syserr("printqueue: Cannot get subdirectory label");
			exit(ExitStat);
		}

		raisepriv(SEC_ALLOWMACACCESS, NULL);

		if (setslabel(queue_level) < 0)
		{
			syserr("printqueue: Cannot set sensitivity");
			exit(ExitStat);
		}

		/*
		**  Drop ALLOWMACACCESS and MULTILEVELDIR here.
		**  The chdir is automatically to the appropriate
		**  subdirectory for the current process sensitivity.
		*/
		setpriv(SEC_EFFECTIVE_PRIV, old_privs);

		if (chdir(QueueDir) < 0)
		{
			syserr("printqueue: Cannot chdir to subdirectory");
			exit(ExitStat);
		}

		sprintf(resultbuf, "sensitivity level: %s",
		    (er = mand_ir_to_er(queue_level)));

		priv_audit_subsys("Print mail queue", resultbuf, ET_SUBSYSTEM);

		/* output external representation of sensitivity level */
		printf("\n===============================================================================\n");
		printf("Sensitivity: %s\n", er);
		printf("===============================================================================\n\n");
		printqueue();
# 		else /* not notdef */
/*
**  KERNEL BUG!
**
**  Because setslabel does not currently work if the sensitivity has
**  already been set, we raise ALLOWMACACCESS in order to print the queue
**  for every sensitivity level for which readmultdir returns a name.
**
**  Although the code below works correctly and demonstrates that
**  readmultdir works as documented, doing it this way is a hack.
**  Sendmail should setslabel as in the code above.
**
**  When setslabel is fixed, remove the "#ifdef notdef line above and this
**  whole "#else not notdef" clause.
*/
		raisepriv(SEC_MULTILEVELDIR, old_privs);
		/*
		**  readmultdir returns the subdir name for every
		**  file in the subdir.  I don't know why.
		**
		**  We are only want to chdir to each subdirectory once.
		*/
		while (strcmp(savename, relname) == 0)
		{
			readmultdir(mldp, relname, &sdp);
			if (sdp == (struct dirent *) NULL)
			{
				setpriv(SEC_EFFECTIVE_PRIV, old_privs);
				return;
			}
		}
		strcpy(savename, relname);

		sprintf(fullname, "%s/%s", QueueDir, relname);
		if (statslabel(fullname, queue_level) < 0)
		{
			syserr("printqueue: Cannot get subdirectory label");
			exit(ExitStat);
		}

		/*
		**  odious hack: raise ALLOWMACACCESS so that we can 
		**  chdir to and read files in subdirectories
		**  not at our own sensitivity.
		*/
		raisepriv(SEC_ALLOWMACACCESS, NULL);

		if (setslabel(queue_level) < 0)
		{
			syslog(LOG_DEBUG,
			    "KERNEL BUG: Cannot set sensitivity: %m");
		}
		/*
		**  chdir to fullname with ALLOWMACACCESS
		*/
		if (chdir(fullname) < 0)
		{
			syserr("printqueue: Cannot chdir to %s", fullname);
			exit(ExitStat);
		}

		sprintf(resultbuf, "sensitivity level: %s",
		    (er = mand_ir_to_er(queue_level)));

		priv_audit_subsys("Print mail queue", resultbuf, ET_SUBSYSTEM);

		/* output external representation of sensitivity level */
		printf("\n===============================================================================\n");
		printf("Sensitivity: %s\n", er);
		printf("===============================================================================\n\n");
		printqueue();
		/*
		**  drop ALLOWMACACCESS and MULTILEVELDIR
		*/
		setpriv(SEC_EFFECTIVE_PRIV, old_privs);
/*
**  End of KERNEL BUG!
*/
# 		endif /* not notdef */
	}
}

/*
**  SENDMAIL_MLD_RUNQUEUE -- process the mail queue for all levels.
**
**	Parameters:
**		none.
**
**	Returns:
**		none (in a separate child for each subdirectory).
**
**	Side Effects:
**  		For each single-level subdirectory, fork a child
**  		to process that level, chdir there, and call runqueue 
**		to do the work.
**		
**		Exits if the mail queue directory is not a multi-level
**		directory.
*/

void
sendmail_mld_runqueue()
{
	mand_ir_t *queue_level;
	MDIR *mldp;
	char relname[MAXNAMLEN + 1],	/* relative subdir name as returned */
					/* by readmultdir()        	    */
	     savename[MAXNAMLEN + 1],	/* saved relative name		    */
	     fullname[MAXNAMLEN + 1];	/* absolute pathname of subdir	    */
	struct dirent *sdp;
	int pid;

	/* no zombies here */
	signal(SIGCLD, SIG_IGN);

	bzero(relname, sizeof(relname));
	bzero(savename, sizeof(savename));
	bzero(fullname, sizeof(fullname));
	queue_level = mand_alloc_ir();

	/*
	**  Raise MULTILEVELDIR and ALLOWMACACCESS privileges here
	*/
	raisepriv(SEC_MULTILEVELDIR, old_privs);
	raisepriv(SEC_ALLOWMACACCESS, NULL);
	mldp = openmultdir(QueueDir, MAND_MLD_ALLDIRS,
	    (mand_ir_t *) NULL);
	if (mldp == (MDIR *) NULL) {
		syserr("runqueue: Cannot open %s as multi-level directory",
		    QueueDir);
		exit(EX_OSERR);
	}

	for (;;)
	{
		/*
		**  readmultdir returns the subdir name for every
		**  file in the subdir.  I don't know why.
		**
		**  We are only want to chdir to each subdirectory once.
		*/
		while (strcmp(savename, relname) == 0)
		{
			readmultdir(mldp, relname, &sdp);
			if (sdp == (struct dirent *) NULL)
				exit(EX_OK);
		}
		strcpy(savename, relname);

		pid = dofork();
		if (pid < 0)
		{
			syserr("runqueue: Cannot fork");
			exit(ExitStat);
		}
		else if (pid > 0)
		{
			/*
			**  Parent:
			**
			**  Process the subdirectories sequentially
			**  so that output is not jumbled together.
			**  Let each child do its own error reporting.
			*/
			while ((errno = 0, wait((int *)0) >= 0) ||
				errno != ECHILD)
			{
				continue;
			}
			continue;
		}

		/*
		**  Child:
		*/

		signal(SIGCLD, SIG_DFL);
		closemultdir(mldp);

		sprintf(fullname, "%s/%s", QueueDir, relname);
		if (statslabel(fullname, queue_level) < 0)
		{
			syserr("runqueue: Cannot get subdirectory label");
			exit(ExitStat);
		}

/*
**  KERNEL BUG!
**
**  The following is a workaround for the inability to setslabel.
**  It has the effect of running the queue for the level at which
**  sendmail is run, and complaining about all the other levels.
**
**  Remove the line below (and this comment) when setslabel is fixed.
*/
if (!(mand_ir_relationship(InitialLevel, queue_level) & MAND_EQUAL))
		if (setslabel(queue_level) < 0)
		{
			syserr("runqueue: Cannot set sensitivity");
			exit(ExitStat);
		}

		/*
		**  Drop privileges here.  The chdir is automatically to
		**  the appropriate subdirectory for the current process
		**  sensitivity.
		*/
		setpriv(SEC_EFFECTIVE_PRIV, old_privs);

		if (chdir(QueueDir) < 0)
		{
			syserr("runqueue: Cannot chdir to queue subdirectory");
			exit(ExitStat);
		}

		/*
		**  If the child is running at a sensitivity level
		**  not dominated by the process' initial sensitivity,
		**  turn off Verbose mode, to preserve MAC,
		**  unless the process also happens to have ALLOWMACACCESS.
		*/
		if (!(ISBITSET(old_privs, SEC_ALLOWMACACCESS)) &&
		    !(mand_ir_relationship(InitialLevel, queue_level) &
			MAND_SDOM))
		{
			Verbose = FALSE;
		}

		sprintf(resultbuf, "sensitivity level: %s",
		    mand_ir_to_er(queue_level));
		priv_audit_subsys("Process mail queue", resultbuf, ET_SUBSYSTEM);
		return;
	}
}

/*
**  SENDMAIL_IS_CLEARED -- return true if local recipient is cleared to
**	receive a message at the sensitivity level at which sendmail is
**	currently running.
**
**	Parameters:
**		pw -- the recipient user whose clearance is in question.
**
**	Returns:
**		1 if the user's clearance dominates current process sensitivity;
**		0 if not.
**
**	Side Effects:
**		audits success or failure.
*/

bool
sendmail_is_cleared(pw)
	struct passwd *pw;
{
	struct pr_passwd *prpw;
	mand_ir_t *usr_clrnce, *proc_level;
	int rel;

	strcpy(actionbuf, "Check recipient clearance for local mail delivery");

	/*
	**  obtain current process sensitivity
	*/
	proc_level = mand_alloc_ir();
	if (getslabel(proc_level) < 0)
	{
		/* audit failure */
		priv_audit_subsys(actionbuf,
		    "Cannot determine current process sensitivity",
		    ET_SUBSYSTEM);
		return(0);
	}

	/*
	**  obtain recipient user's protected password entry
	*/
	raisepriv(SEC_ALLOWDACACCESS, old_privs);
	prpw = getprpwnam(pw->pw_name);
	setpriv(SEC_EFFECTIVE_PRIV, old_privs);
	if (prpw == (struct pr_passwd *) NULL)
	{
		/* audit failure */
		sprintf(resultbuf,
		    "Cannot access protected passwd entry for user %s",
		    pw->pw_name);
		priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
		return(0);
	}

	/*
	**  determine recipient user's clearance
	*/
	if (prpw->uflg.fg_clearance) {
		usr_clrnce = &prpw->ufld.fd_clearance;
	} else if (prpw->sflg.fg_clearance) {
		usr_clrnce = &prpw->sfld.fd_clearance;
	} else {
		/* audit failure */
		sprintf(resultbuf,
		    "Cannot determine clearance for user %s", pw->pw_name);
		priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
		return(0);
	}

	/*
	**  compare recipient user's clearance with current sensitivity
	*/
	raisepriv(SEC_ALLOWMACACCESS, old_privs);
	rel = mand_ir_relationship(usr_clrnce, proc_level);
	setpriv(SEC_EFFECTIVE_PRIV, old_privs);

	if (!(rel & MAND_SDOM) && !(rel & MAND_EQUAL))
	{
		/* audit failure */
		sprintf(resultbuf,
		    "recipient user %s NOT cleared for sensitivity level %s",
		    pw->pw_name, mand_ir_to_er(proc_level));
		priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
		return(0);
	}
	else
	{
		/* audit success */
		sprintf(resultbuf,
		    "recipient user %s cleared for sensitivity level %s",
		    pw->pw_name, mand_ir_to_er(proc_level));
		priv_audit_subsys(actionbuf, resultbuf, ET_SUBSYSTEM);
		return(1);
	}
}

/*
**  SENDMAIL_SET_ACCEPT_LABEL -- get security attributes from the accepted
**	connection, and initialize the daemon's child process accordingly
**
**	Parameters:
**		peer -- remote hostname and [internet address]
**
**	    (Add any other parameters that are needed, for example
**	    the accepted socket descriptor.)
**
**	Returns:
**		none.
**
**	Side Effects:
**		audits success.
**		exits on failure.
**
*/

void
sendmail_set_accept_label(peer)
	char *peer;
{
	mand_ir_t *child_level;

	/*
	**  1.  Determine from the accepted socket the security
	**	attributes that the process must inherit from the
	**	client:  certainly the sensitivity level, but possibly
	**	other attributes (such as luid) in the future.
	**
	*/

/*
**	(We fake it here.  In real life, we would fill in the
**	label somehow from information in the socket.)
**
	child_level = mand_alloc_ir();
	if (magically_fill_in(child_level) < 0)
	{
		syserr("Cannot get socket sensitivity");
		exit(ExitStat);
	}
**
*/
	child_level = mand_syslo;

	/*
	**  2.  Set the process attributes.  Currently this is just setslabel.
	*/
	raisepriv(SEC_ALLOWMACACCESS, old_privs);
	if (setslabel(child_level) < 0) {
/*
**  KERNEL BUG!
**
**  Currently, setslabel doesn't work if the label is already set.
**  For now, just syslog and go on.
**
**  When this is fixed, uncomment the syserr and exit following,
**  and remove this comment and the syslog call below.
**
		syserr("Cannot set sensitivity");
		exit(ExitStat);
*/
		syslog(LOG_DEBUG,
		    "KERNEL BUG: Cannot set sensitivity: %m");
	}
	setpriv(SEC_EFFECTIVE_PRIV, old_privs);

	/*
	** 3.  The daemon initially set sensitivity to syslo and
	**     chdir'ed to the syslo subdirectory of the multi-level
	**     queue directory.  Now chdir to the appropriate
	**     single-level subdirectory.
	*/
	if (chdir(QueueDir) < 0)
	{
		syserr("Cannot chdir(%s)", QueueDir);
		exit(EX_SOFTWARE);
	}

	/* audit success */
	sprintf(resultbuf, "from %s, sensitivity %s",
		peer, mand_ir_to_er(child_level));
	priv_audit_subsys("Accept SMTP connection", resultbuf, ET_SUBSYSTEM);

	/*
	**  Once all these things have been done, the message should be
	**  processed normally, as if the whole transaction had occurred
	**  at the assigned sensitivity level.
	*/
}

# 	endif /* B1 */
# endif /* SecureWare */
