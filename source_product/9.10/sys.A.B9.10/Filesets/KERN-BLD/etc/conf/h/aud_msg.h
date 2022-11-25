/* @(#) $Revision: 1.3.84.3 $ */

/***********************************************************
 *
 *		aud_msg.h
 *
 * this file contains all the special printf messages that are
 * used in the auditing system.
 *
 ***********************************************************/


/* audusr messages */

#define ALLUSR "All users "
#define A_MSG "will be audited\n"
#define D_MSG "will not be audited\n"
#define PWD_LK "audusr: password file is locked, cannot do any update\n"
#define UNCHANGED "audusr: the system is unchanged, all specified users are not updated\n"

/* audsys messages */

#define CFILE_MSG "audsys: current audit file is %s, max size is %d kbytes, warning zone starts at %dth kbytes, danger zone starts at %dth kbytes\n"
#define CNTLF_NE "audsys: file /.secure/etc/audnames does not exist\n"
#define C_MSG "audsys: current audit file is changed to\n"
#define F_CREAT "audsys: file %s is created\n"
#define FILE_ERR "audsys: input audit file is not an empty regular file\n"
#define IGNORED "audsys: other input arguments are ignored\n"
#define NXFILE_MSG "audsys: no available backup audit file\n"
#define OFF_MSG "audsys: auditing system is currently off\n"
#define ON_MSG "audsys: auditing system is currently on\n"
#define RESET "audsys: it is reset to \* in /.secure/etc/adnames, no backup audit file is available in the system\n"
#define ROLLOVER "audsys: the system has rolled over to use the backup file in /.secure/etc/audnames\n"
#define TOFF_MSG "audsys: auditing system is turned off\n"
#define TON_MSG "audsys: auditing system is turned on\n"
#define UNKNOWN "audsys: unknown audit file "
#define XFILE_MSG "audsys: backup audit file is %s, max size is %d kbytes, warning zone starts at %dth kbytes, danger zone starts at %dth kbytes\n"
#define X_MSG "audsys: backup audit file is changed to\n"
#define E_INTNAL "audsys: internal error\n"
