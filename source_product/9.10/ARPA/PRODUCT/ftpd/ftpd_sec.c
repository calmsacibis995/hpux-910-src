#ifdef SecureWare
/*
 * Copyright (c) 1989 SecureWare, Inc.
 *   All rights reserved
 */

#ident "$Header: ftpd_sec.c	2.1 10:06:37 9/21/89 SecureWare"

#ifndef lint
static char rcsid[] = "$Header: ftpd_sec.c,v 1.21.109.1 91/11/21 11:50:40 kcs Exp $";
static char sccsid[] = "ftpd_sec.c  based on 5.28   (Berkeley) 4/20/89";
#endif /* not lint */


/*
 * This Module contains Proprietary Information of SecureWare, Inc. and
 * should be treated as Confidential.
 */

/*
 * This library module contains routines called from ftpd.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <pwd.h>
#include <sys/security.h>
#include <prot.h>
#include <syslog.h>
#include <grp.h>

#ifdef B1
#include <mandatory.h>
#endif


#include <audnetd.h>
extern  u_short	ruid, aud_luid; 
extern  u_long	raddr, laddr; 
extern  int     errno;


extern char *rusr, *lusr, *rhost;

#ifdef B1
/*
 * Don't have to worry about restoring privileges here since
 * AUDIT_FAILURE is only called when it doesn't matter if the
 * privileges are disabled. 
 */
#define ENABLE_PRIV_FOR_AUDIT() \
		if (ISB1) { \
			enablepriv(SEC_ALLOWDACACCESS); \
			enablepriv(SEC_ALLOWMACACCESS); \
			enablepriv(SEC_WRITE_AUDIT); \
		}
#define DISABLE_PRIV_FOR_AUDIT() \
		if (ISB1) { \
			disablepriv(SEC_ALLOWDACACCESS); \
			disablepriv(SEC_ALLOWMACACCESS); \
			disablepriv(SEC_WRITE_AUDIT); \
		}
#else /* not B1 */
#define ENABLE_PRIV_FOR_AUDIT() 
#define DISABLE_PRIV_FOR_AUDIT()        
#endif /* B1 */

#define AUDIT_FAILURE(s)        \
        if (ISSECURE) { \
		ENABLE_PRIV_FOR_AUDIT();     \
                audit_daemon(NA_RSLT_FAILURE, NA_VALI_PASSWD, \
                        raddr, rhost, ruid, rusr, aud_luid, lusr, NA_EVNT_START, \
                        NA_MASK_RUID|NA_MASK_LUID|\
                        (raddr == (u_long)-1 ? NA_MASK_RADDR : 0)|\
                        (rhost == (char *)0 ? NA_MASK_RHOST : 0)|\
                        (rusr == (char *)0 ? NA_MASK_RUSRNAME : 0)|\
                        (lusr == (char *)0 ? NA_MASK_LUSR : 0), (s)); \
		DISABLE_PRIV_FOR_AUDIT();    ; \
         }


#ifdef B1
#define ENABLEPRIV(priv)        if (ISB1) enablepriv(priv);
#define DISABLEPRIV(priv)       if (ISB1) disablepriv(priv);
#else
#define ENABLEPRIV(priv)
#define DISABLEPRIV(priv)
#endif /* B1 */

/*
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 *
 * This is here because the audit subsystem is hosed.
 * This cannot be here for released code or there will be NO auditing.
 *
 * WARNING WARNING WARNING WARNING WARNING WARNING WARNING
 */
#ifdef NO_AUDIT
#undef  AUDIT_FAILURE
#define AUDIT_FAILURE(s)   {}
#endif


static struct pr_passwd *ppr = (struct pr_passwd *) 0;
static mask_t   nosprivs[SEC_SPRIVVEC_SIZE];  
       mask_t   effprivs[SEC_SPRIVVEC_SIZE];
static mask_t   orig_user_effprivs[SEC_SPRIVVEC_SIZE];


/*
 * Check the protected password entry for a user. 
 * Set the user's user id and check the user's locked conditions.  
 * Returns the user's uid if successful, otherwise -1.
 * If anonymous == 1, then just look for the existence of the user
 * name as this will be 'ftp' for ANONYMOUS FTP.
 */

ftpd_check_prpw(pwd, password, anonymous)
struct passwd *pwd;
char *password;
int  anonymous;
{
	char *ciphertext;
	int new_nice;

	ENABLEPRIV(SEC_ALLOWDACACCESS);


	if ( pwd ) {
		ppr = getprpwnam(pwd->pw_name);
	} else {
		ppr = (struct pr_passwd *) 0;
	}
	DISABLEPRIV(SEC_ALLOWDACACCESS);

	/*
	 * Check fg_name and fg_uid flags to make sure that the 
	 * fd_name and fd_uid fields are valid
	 */


	if (ppr == (struct pr_passwd *) 0 ||
	    ppr->uflg.fg_name == 0 || ppr->uflg.fg_uid == 0 ||
	    ppr->ufld.fd_uid != pwd->pw_uid ||
	    strcmp(ppr->ufld.fd_name, pwd->pw_name) != 0) {
		AUDIT_FAILURE("No valid protected password entry.");
		reply(530, "Login incorrect.");
		return(-1);
	}
	if (anonymous != 1) {
	        /*
  	 	 * check that the password sent over is correct. 
 	 	 * Compare with the one in the protected password database.
 	 	 * return 0 on success, else nonzero.
 	 	 */
		if (!ppr->uflg.fg_encrypt) {
			AUDIT_FAILURE("Password verification failed.");
			reply(530, "Login incorrect.");
			return(-1);
		}
		ciphertext = bigcrypt(password, ppr->ufld.fd_encrypt);
		if (strcmp(ciphertext, ppr->ufld.fd_encrypt) != 0) {
				AUDIT_FAILURE("Password verification failed.");
				reply(530, "Login incorrect.");
				return(-1);
			}
	}

	if (locked_out(ppr)) {
		AUDIT_FAILURE("Account is disabled.");
		reply(530, "Account is disabled.");
		return(-1);
	}
	if (ppr->uflg.fg_type &&
	    ISBITSET(ppr->ufld.fd_type, AUTH_RETIRED_TYPE)) {
		AUDIT_FAILURE("Account has been retired.");
		reply(530, "Account has been retired.");
		return(-1);
	}
#ifdef B1
	if (ISB1)
		if (time_lock(ppr)) {
			AUDIT_FAILURE("Account is unavailable at this time.");
			reply(530, "Account is unavailable at this time.");
			return(-1);
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
	 * Note we must do this before the setuid(2) below.
	 */
	errno = 0;
	if (ppr->uflg.fg_nice)
		new_nice = ppr->ufld.fd_nice;
	else if (ppr->sflg.fg_nice)
		new_nice = ppr->sfld.fd_nice;

	ENABLEPRIV(SEC_IDENTITY);
	if (ppr->uflg.fg_nice || ppr->sflg.fg_nice)  {
		(void) nice(new_nice);
		if (errno != 0)  {
			AUDIT_FAILURE("Bad priority setting");
			DISABLEPRIV(SEC_IDENTITY);
			return(-1);
		}
	DISABLEPRIV(SEC_IDENTITY);
	}

	return(pwd->pw_uid);
}

/*
 * set clearance to user's maximum clearance, from password database.
 * The clearance has been set to syshi by inetd when the request for
 * service came in.  Then the sensitivity level is set from the
 * level on the socket.
 * Now the clearance will be set down to the level appropriate to
 * the user.  If this clearance exceeds that of the current sensitivity
 * level, this will fail.
 * This is only called if ISB1 is TRUE.
 */

ftpd_setclrnce()
{
#ifdef B1
	mand_ir_t *clearance;

	if (ppr == (struct pr_passwd *) 0) {
		reply(530, "Cannot set clearance.");
		return(-1);
	}
	if ( ppr->uflg.fg_clearance) {
		clearance = &ppr->ufld.fd_clearance;
	} else if (ppr->sflg.fg_clearance) {
		clearance = &ppr->sfld.fd_clearance;
	} else {
		mand_init();
		clearance = mand_syslo;
	}

	if (setclrnce(clearance) < 0) {
		reply(530, "Cannot set clearance.");
		AUDIT_FAILURE("Cannot set clearance.");
		return(-1);
	}
#endif /* B1 */
	return(0);
}



/*
 * audit the login condition and set up the process audit mask
 */

ftpd_audit(pwd)
struct passwd *pwd;
{
	audit_adjust_mask(ppr);
	audit_login(ppr, pwd, "ftpd", ES_LOGIN);
}




/*
 * Set up the user's privileges according to the protected password db.
 */

ftpd_setup_privs()
{
	mask_t          *sysprivs;
#ifdef B1
	mask_t          *baseprivs;
#endif

        if (ppr->uflg.fg_sprivs)
                sysprivs = ppr->ufld.fd_sprivs;
        else if (ppr->sflg.fg_sprivs)
                sysprivs = ppr->sfld.fd_sprivs;
        else {
		memset(nosprivs, 0, SEC_SPRIVVEC_SIZE*sizeof(mask_t));
                sysprivs = nosprivs;
	}
#ifdef B1
	if (ISB1) {
        	if (ppr->uflg.fg_bprivs) {
                	baseprivs = ppr->ufld.fd_bprivs;
        	} else {
			if (ppr->sflg.fg_bprivs) {
                		baseprivs = ppr->sfld.fd_bprivs;
        	        } else {
				memset(nosprivs, 0, SEC_SPRIVVEC_SIZE*sizeof(mask_t));
                		baseprivs = nosprivs;
			}
		}
		
        	if (setpriv(SEC_MAXIMUM_PRIV, sysprivs)) {
		 	syslog(LOG_ERR, "Cannot set kernel authorizations.");
			AUDIT_FAILURE("Cannot set kernel authorizations.");
		}
        	if (setpriv(SEC_BASE_PRIV, baseprivs)) {
		 	syslog(LOG_ERR, "Cannot set base authorizations.");
			AUDIT_FAILURE("Cannot set base authorizations.");
		}
		/*
		 * The effective privileges need to be set here because
		 * they have been reduced to the bare minimum needed to
		 * get this far in ftpd.  Now ftpd is going to act on
		 * behalf of the "logged in" user so it needs to have that
		 * user's privileges.  The baseprivs were selected because
		 * of SMP+ rule for setting effective privileges at the
		 * time of an exec().  Effective privileges are set to be
		 * the UNION of the program's granted and the user's base
		 * privileges.  Even though there is no new program to
		 * exec, I will still act as if there were to make the
		 * effective privileges reflect the capabilities of the
		 * "logged in" user.
		 */
        	if (setpriv(SEC_EFFECTIVE_PRIV, baseprivs)) {
		 	syslog(LOG_ERR, "Cannot set effective authorizations.");
			AUDIT_FAILURE("Cannot set effective authorizations.");
		}
		ftpd_initpriv();
	} else {
        	if (setpriv(SEC_EFFECTIVE_PRIV, sysprivs)) {
		 	syslog(LOG_ERR, "Cannot set effective authorizations.");
			AUDIT_FAILURE("Cannot set effective authorizations.");
		}
	} /* ISB1 */
#else
        if (setpriv(SEC_EFFECTIVE_PRIV, sysprivs)) {
	 	syslog(LOG_ERR, "Cannot set effective authorizations.");
		AUDIT_FAILURE("Cannot set effective authorizations.");
	}
#endif

        endprpwent();
        endprdfent();
}

/*
 * These routines were added since the routines that are provided by 
 * SecureWare really are for privileged processes and these functions
 * will be called only by the ftpd process after it has become the
 * "logged-in" user.
 * The private privilege mask is set to what the "logged-in" user
 * actually is granted by the system.  This may seem messy to have
 * privileges manipulated with two different scheme's, but is is safer.
 * The real problem is that the "cache" effective privileges in the
 * library routines cannot be accessed outside of the library.  This
 * causes problems if you ever need to set the entire effective
 * privilege mask.  Only if you use forcepriv() will you stay in
 * sync between the actual privileges and the library's cache.
 *
 * Also make a copy of the original effective privileges for the user
 * to be used to restore privileges quickly.
 */

ftpd_initpriv()
{
	getpriv(SEC_EFFECTIVE_PRIV, effprivs);
	memcpy(orig_user_effprivs, effprivs, SEC_SPRIVVEC_SIZE*sizeof(mask_t));
}

/*
 * Turn on a privilege even if there is no authorization for it (if possible).
 * The routine will fail if the privilege is not in either the base
 * privilege set or the program's potential set.
 */
int
ftpd_forcepriv(priv)
	int priv;
{
	int ret = 1;

	if (!ISBITSET(effprivs, priv))  {
		ADDBIT(effprivs, priv);
		ret = setpriv(SEC_EFFECTIVE_PRIV, effprivs);
		if (ret < 0)
			RMBIT(effprivs, priv);
	}

	return(ret >= 0) ;
}



/*
 * Restore the original user's privileges since they were bumped up
 * to do a privileged operation
 */
int ftpd_restorepriv()
{
	int ret;

	ret = setpriv(SEC_EFFECTIVE_PRIV, orig_user_effprivs);
	memcpy(effprivs, orig_user_effprivs, SEC_SPRIVVEC_SIZE*sizeof(mask_t));
	return(ret);
}



/*
 * Turn off a privilege.
 */
int
ftpd_disablepriv(priv)
	int priv;
{
	int ret = 1;

	if (ISBITSET(effprivs, priv))  {
		RMBIT(effprivs, priv);
		ret = setpriv(SEC_EFFECTIVE_PRIV, effprivs);
	}

	return(ret >= 0) ;
}




#define FTPD_NUM_DIR   2
#define FTPD_NUM_FILES 4

/* 
 * Contains the names of the directories and files to check. 
 */
struct dirs_files_st {
	char	*dirs;
	char    *files[FTPD_NUM_FILES];
} dirs_files[FTPD_NUM_DIR] = {
	{ "/bin",
	  { "/ls", "/pwd", 0 }
	},

	{ "/etc", 
	  { "/passwd", "/group", "/logingroup", 0 }
	}
};

 
#define FTP_LOGIN_NAME  "ftp"
#define ROOT "/"

#define WRITE_MASK   (S_IWUSR | S_IWGRP | S_IWOTH) 



/*
 * This function will check on the validity of the anonymous ftp environment.
 * It will enforce the following state:
 *  1. User ftp is a valid entry in /etc/passwd (This is not actually done here)
 *  2. Its home directory is not "/" and is not writable
 *  3. If "~ftp/bin" exists, that it is not writable
 *     a.  If "~ftp/bin/ls" exists, that it is not writable
 *     b.  If "~ftp/bin/pwd" exists, that it is not writable
 *  4. If "~ftp/etc" exists, that it is not writable
 *     a.  If "~ftp/etc/passwd" exists, that it is not writable
 *     b.  If "~ftp/etc/group" exists, that it is not writable
 *     c.  If "~ftp/etc/logingroup" exists, that it is not writable
 *     
 */
check_ftp_env(ftp_pwd)
struct  passwd  *ftp_pwd;
{

	struct  group   *grp, *getgrent();
	struct  stat    stats, rootstats;
	char    temp_path[MAXPATHLEN+20];
	int	root_uid, ftp_uid, dir_marker, file_marker, file_cnt, dir_cnt;


        /*
	 * Check on the existence of the home directory and proper mode bits
	 */

	if (strcmp(ftp_pwd->pw_dir, ROOT) == 0) {
		syslog(LOG_NOTICE, "Unable to provide ANONYMOUS ftp.  Home directory for user '%s' is not permitted to be '%s'", FTP_LOGIN_NAME, ROOT);
		return(-1);
	}
	if (stat(ftp_pwd->pw_dir, &stats) < 0) {
		syslog(LOG_NOTICE, "Unable to provide ANONYMOUS ftp.  The home directory '%s' for user %s is non-existent",
			 ftp_pwd->pw_dir, FTP_LOGIN_NAME);
		return(-1);
	} 
	/*
	 * Check to see that the home directory is not some kind of link
	 * to root
	 */
	if (stat(ROOT, &rootstats) < 0) {
		/* 
		 * If you cannot stat "/", but you could stat the home
		 * directory, then the home directory can't be "/", just
		 * don't worry about things.
		 */
	} else {
	 	 if ( (stats.st_dev == rootstats.st_dev) &&
		     ( stats.st_ino == rootstats.st_ino) ) {
			syslog(LOG_NOTICE, "Unable to provide ANONYMOUS ftp.  Home directory for user '%s' is not permitted to be linked to '%s'", FTP_LOGIN_NAME, ROOT);
                	return(-1);
		 }
		
	}
	
	if (stats.st_mode & WRITE_MASK ) {
		syslog(LOG_NOTICE, "Unable to provide ANONYMOUS ftp.  The home directory '%s' is writable and should not be.",
			ftp_pwd->pw_dir);
		return(-1);
	} 

	(void) strcpy(temp_path, ftp_pwd->pw_dir);
	dir_marker = strlen(temp_path);  /* Remember where the end of the */
					 /* ~ftp is                       */
	/*
	 * Check for the bin and etc directories and for the proper mode bits
	 */
	for (dir_cnt=0; dir_cnt<FTPD_NUM_DIR; dir_cnt++) {
		(void) strcat(temp_path, dirs_files[dir_cnt].dirs);
		if (stat(temp_path, &stats) == 0) {
		   if (stats.st_mode & WRITE_MASK ) {
			syslog(LOG_NOTICE, "Unable to provide ANONYMOUS ftp.  The '%s' directory is writable and should not be.",
				temp_path);
			return(-1);
		   } 
		   /*
		    * Check for the files in the directories to make
		    * sure they have the proper mode bits.
		    */
               	   file_marker = strlen(temp_path);
               	   for (file_cnt=0; dirs_files[dir_cnt].files[file_cnt]; file_cnt++) {
                       (void) strcat(temp_path, dirs_files[dir_cnt].files[file_cnt]);
		       if (stat(temp_path, &stats) == 0) {
			  if (stats.st_mode & WRITE_MASK ) {
				syslog(LOG_NOTICE, "Unable to provide ANONYMOUS ftp.  The '%s' file is writable and should not be.",
					temp_path);
				return(-1);
			  } 
		      }
                      temp_path[file_marker] = '\0';
                   }  /* for file_cnt... */

		}
		temp_path[dir_marker] = '\0';

	} /* for dir_cnt... */
     
	return(0);

}

#endif /* SecureWare */

