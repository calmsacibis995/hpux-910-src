/* @(#) $Revision: 64.5 $ */

/***********************************************************
 *
 *		aud_msg.h
 *
 * this file contains all the self auditing records written by
 * audusr and audsys 
 *
 ***********************************************************/


/* audusr messages */

#define ALLUSR "All users "
#define A_MSG "will be audited\n"
#define D_MSG "will not be audited\n"

/* audsys messages */

#define CNTSUSP_MSG "audsys: cannot suspend auditing - cannot continue"
#define C_MSG "audsys: current audit file is changed to"
#define X_MSG "audsys: next audit file is changed to"
#define FILE_ERR "audsys: input audit file is not an empty regular file\n"
#define CREAT_ERR "audsys: unable to create audit file"
#define TOFF_MSG "audsys: auditing system shut-down\n"
#define TON_MSG "audsys: auditing system started\n"
#define UNKNOWNC_MSG "audsys: unknown current audit file in use "
#define UNKNOWNN_MSG "audsys: unknown next    audit file in use "
#define RESET_MSG "audsys: reset next file to NULL\n"
#define CEQX_MSG "audsys: current and next file set to same file (no-next)\n"
#define E_CNTLF_MSG "audsys: unable to open/lock /.secure/etc/audnames\n"
#define E_INTNAL_MSG "audsys: unknown internal error\n"
#define TOFF_INTN_MSG "audsys: internal error: shut-down failed\n"
#define CNT_WRCTL_MSG "audsys: internal error: /.secure/etc/audnames update failed\n"
#define SET_INTN_MSG "audsys: internal error: could not update kernel\n"
#define E_CNTL_EXMSG "audsys: could not find /.secure/etc/audnames\n"
#define E_CNTL_FMTMSG "audsys: detected bad /.secure/etc/audnames\n"
#define FIXCNTL_MSG "audsys: corrected /.secure/etc/audnames\n"
#define NOT_FIXCNTL_MSG "audsys: could not repair /.secure/etc/audnames\n"
#define NOACT_MSG "audsys: auditing system unchanged\n"
#define CURR_MSG   "audsys: current file is "
#define BACKUP_MSG "audsys: next    file is "
#define F_WCREAT "audsys: expected audit file not found (deleted?)" 
