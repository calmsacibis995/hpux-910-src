/*	@(#) $Revision: 64.6 $		 */

/************************************************************
 *
 *	aud_err.h
 *
 *	This file contains the error messages and responses
 *      printed by the auditing system.
 *
 ************************************************************/

/* audusr */

#define E_aDFLG "-a and -D cannot both be specified\n"
#define E_ADFLG "-A and -D cannot both be specified\n"
#define E_AdFLG "-A and -d cannot both be specified\n"
#define E_AUDUSR_USE "usage: audusr [[-a user]...] [[- d user]...] [-AD]\n"
#define E_PWDLK "password file is locked, cannot do any update\n"
#define E_PWDTMP "cannot create the temp password file, no update\n"
#define E_PWD_OPEN "cannot open the password file /etc/passwd\n"
#define E_PWDT_OPEN "cannot open the temp password file /etc/ptmp\n"
#define E_PWDT_WR "cannot write to the temp password file /etc/ptmp\n"
#define E_STAT "cannot stat the password file\n"
#define E_U_NEX "one or more input user(s) do not exist, no update\n"
#define E_USR_SZ "number of input users is more than the maximum allowed\n"
#define UNCHANGED "auditing system is unchanged, no users updated\n"

/* shared by both audsys and audusr */

#define E_SUSP "cannot suspend auditing on this process, exiting\n"
#define E_SUSER "you do not have access to the auditing system\n"

/* audsys */

#define E_AUDSYS_USE "usage: audsys [-nf] [-c file -s kbytes] [-x file -z kbytes]\n"
#define E_ILLEGAL "illegal input"
#define E_CNTLF_EXIST "warning: /.secure/etc/audnames does not exist\n"
#define E_CNTLF_LK "cannot open and lock /.secure/etc/audnames;\nverify that the auditing system has been installed properly,\nand the /.secure/etc directory exists, prior to running this command\n" 
#define E_CNTLF_FMT "warning: cannot read badly formatted /.secure/etc/audnames\n"
#define E_NEWCNTL "created/repaired /.secure/etc/audnames\n"
#define E_CREAT "cannot create audit file: %s\n"
#define E_NEMPT "audit file %s, is not an empty file\n"
#define E_FTYPE "audit file %s, is not a regular file (or CDF)\n" 
#define E_INTNAL "internal error\n"
#define E_NFFLG  "-n and -f cannot be both specified\n"
#define E_OFFOPT "-n option must be specified to turn auditing on\n"
#define E_ON_MSG "auditing system is already on, input arguments ignored\n"
#define E_SWDFLG "-c and -s options must be specified together\n"
#define E_NOACT "auditing system unchanged\n"
#define E_CURRENT "current"
#define E_NEXT "next"
#define E_AFTBG   "%s audit file %s\nexceeds its AFS, select a different one\n"
#define E_AFSTBG  "%s audit file %s\ninsufficient space available on audit file filesystem,\nspecify a different audit file or select a smaller AFS\n"
#define E_AFPFULL "%s audit file %s\nfilesystem exceeds minfree,\nspecify an audit file on a different filesystem\n"
#define E_UNKNOWN_CUR  "unknown current audit file in use:"
#define E_UNKNOWN_NXT  "unknown next    audit file in use:"
#define E_UKCFIX  "-c must be specified to select a current file\n"
#define E_UKNFIX  "-x must be specified to select a next file\n"
#define E_UNKNOWN_ONXT "unknown next audit file was in use:"
#define E_ZGRFLG "-x and -z options must be specified together\n"
#define E_DIRNEXIST "audit file directory %s doesn't exist\n" 
#define E_WCREAT "warning: audit file %s doesn't exist\n"
#define DFILES_HDR_OUT "Audit files:\n"
#define DCFILE_OUT     "current file:"
#define DBFILE_OUT     "next    file:"
#define DUNKNOWN       "** unknown **"
#define DNXFILE_MSG    "next    file: none\n"
#define DBDAU_OUT      "next    file:        ** no data available **\n"
#define DCDAU_OUT      "current file:        ** no data available **\n"
#define DSTAT_HDR_OUT  "statistics-     afs Kb  used Kb  avail %%    fs Kb  used Kb  avail %%\n"
#define CEQX_OUT "current and next audit files set to same target\n"
#define TON_OUT "auditing system started\n"
#define TOFF_OUT "auditing system halted\n"
#define F_CREAT "created audit file:"
#define IGNORED "other input arguments are ignored\n"
#define NXFILE_MSG "no available next audit file\n"
#define OFF_MSG "auditing system is currently off\n"
#define ON_MSG "auditing system is currently on\n"
#define RESET "next file has been reset to <null> \nno next audit file is available to the system\n"
