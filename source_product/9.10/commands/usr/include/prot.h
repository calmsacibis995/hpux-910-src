/* @(#) $Revision: 66.9 $ */
#ifndef _PROT_INCLUDED /* allows multiple inclusion */
#define _PROT_INCLUDED

/* Copyright (c) 1988, 1989 SecureWare, Inc.
 *   All rights reserved.
 *
 * Header file for Security Databases
 *
 * @(#)prot.h	2.15 14:23:22 11/1/89 SecureWare, Inc.
 */

/*
 * This Module contains Proprietary Information of SecureWare, Inc.
 * and should be treated as Confidential.
 */


#include <mandatory.h>

#define	AUTH_CAPBUFSIZ	10240

/* file template used to prove subsystem identity to audit_dlvr */

#define AUTH_SUBSYS_TICKET	"/tcb/files/subsys/XXXXXXXXXXXXXX"

/* Location of program support for Authentication and Subsystem auditing */

#define	AUTH_AUDITSUB		"/etc/auth/audit_dlvr"


/* Limits for seeds */

#define	AUTH_SEED_LOWER_LIM	1L
#define	AUTH_SEED_UPPER_LIM	10000000L


/*  Authentication database locking */

#define	AUTH_LOCK_ATTEMPTS	8		/* before giving up lock try */
#define	AUTH_RETRY_DELAY	1		/* seconds */
#define	AUTH_CHKENT		"chkent"	/* sanity check when reading */
#define	AUTH_SILENT		0		/* do actions silently */
#define	AUTH_LIMITED		1		/* tell something of action */
#define	AUTH_VERBOSE		2		/* full disclosure of action */


/*  Database system default entry name */

#define	AUTH_DEFAULT		"default"


/*  Support for large passwords and pass-phrases. */

/*
 * The size of the ciphertext is the salt at the
 * beginning, a series of 11-character segments produced by
 * each call to crypt(), and the trailing end-of-string character.
 * Each 11-character segment uses the a newly computed salt based
 * on the previous encrypted value.
 */
#define	AUTH_MAX_PASSWD_LENGTH		80
#define	AUTH_SALT_SIZE			2
#define	AUTH_CLEARTEXT_SEG_CHARS	8
#define	AUTH_CIPHERTEXT_SEG_CHARS	11
#define	AUTH_SEGMENTS(len)		(((len)-1)/AUTH_CLEARTEXT_SEG_CHARS+1)
#define	AUTH_CIPHERTEXT_SIZE(segments)	(AUTH_SALT_SIZE+(segments)*AUTH_CIPHERTEXT_SEG_CHARS+1)


/*  Time-of-day session login constraints */

#define	AUTH_TOD_SIZE		50	/* length of time-of-day constraints */

/*
 *
 *	Length of the Trusted Path Sequence (not including the \0)
 *
 */

#define	AUTH_TRUSTED_PATH_LENGTH	12

/*
 *
 *	Values returned from create_file_securely()
 *
 */

#define	CFS_GOOD_RETURN			0
#define	CFS_CAN_NOT_OPEN_FILE		1
#define	CFS_NO_FILE_CONTROL_ENTRY	2
#define	CFS_CAN_NOT_CHG_MODE		3
#define	CFS_CAN_NOT_CHG_OWNER_GROUP	4

#define	CFS_CAN_NOT_CHG_ACL		5
#define	CFS_CAN_NOT_CHG_SL		6



/*  Database access parameters */

#define	AUTH_REWIND		0	/* look for entry from file beginning */
#define	AUTH_CURPOS		1	/* look for entry from current pos'n */


/*  File updating extensions -- must use 1 char no user can have */

#define	AUTH_OLD_EXT		"-o"	/* previous version of file */
#define	AUTH_TEMP_EXT		"-t"	/* going to be new version of file */


/*  Time values can be set to current time on initial installation */

#define AUTH_TIME_NOW		"now"

/*  Location of Subsystem database.  */

#define	AUTH_SUBSYSDIR		"/etc/auth/subsystems"


/*  for fd_secclass  */

#define	AUTH_CLASSES			"security classes"
#define AUTH_D			0	/* TCSEC Class D */
#define AUTH_C1			1	/* TCSEC Class C1 */
#define AUTH_C2			2	/* TCSEC Class C2 */
#define AUTH_B1			3	/* TCSEC Class B1 */
#define AUTH_B2			4	/* TCSEC Class B2 */
#define AUTH_B3			5	/* TCSEC Class B3 */
#define AUTH_A1			6	/* TCSEC Class A1 */
#define	AUTH_MAX_SECCLASS	6


/*  for fd_cprivs and subsystem audit recording  */

#define	AUTH_CMDPRIV			"command privileges"
/* The actual command privileges are defined in <sys/audit.h> . */


/*  for fd_sprivs  */

#define	AUTH_SYSPRIV			"system privileges"
/* The actual system privileges are defined in <sys/security.h> . */


#define	AUTH_BASEPRIV			"base privileges"
#define	AUTH_PPRIVS			"potential privileges"
#define	AUTH_GPRIVS			"granted privileges"


/*  for fd_type  */

#define	AUTH_USERTYPE			"user type"
#define	AUTH_ROOT_TYPE		0	/* UID = 0, will drop in future */
#define	AUTH_OPER_TYPE		1	/* System operator */
#define	AUTH_SSO_TYPE		2	/* System security officer */
#define	AUTH_ADMIN_TYPE		3	/* System administrator */
#define	AUTH_PSEUDO_TYPE	4	/* Pseudo-user */
#define	AUTH_GENERAL_TYPE	5	/* General user */
#define AUTH_RETIRED_TYPE	6	/* User's account has been retired */
#define	AUTH_MAX_TYPE		6


/*  for fd_auditmask  */

#define	AUTH_AUDITMASK	"audit mask"
/* The actual system privileges are defined in <sys/audit.h> . */
#define	AUTH_MAX_AUDITMASK	AUDIT_MAX_EVENT


/*  for fd_seed  */

#define	AUTH_SEED		"seed"
#define AUTH_SEED_AUTO		0	/* Seed computed automatically */
#define AUTH_SEED_MANUAL	1	/* User prompted for seed value */
#define AUTH_SEED_ADMIN		2	/* Administrator sets seed value */
#define	AUTH_MAX_SEED		2


#define	AUTH_TYPEVEC_SIZE	(WORD_OF_BIT(AUTH_MAX_TYPE) + 1)
/* NOTE: if we ever decide to go over 32 command privileges, change this */
#define	AUTH_CPRIVVEC_SIZE	1
#define	AUTH_AUDITMASKVEC_SIZE	(WORD_OF_BIT(AUTH_MAX_AUDITMASK) + 1)
#define	AUTH_SECCLASSVEC_SIZE	(WORD_OF_BIT(AUTH_MAX_SECCLASS) + 1)
#define	AUTH_SEEDVEC_SIZE	(WORD_OF_BIT(AUTH_MAX_SEED) + 1)


/*  Protected password database entry  */

struct pr_field  {
	/* Identity: */
#define	AUTH_U_NAME		"u_name"
	char	fd_name[9];	/* uses 8 character maximum(and \0) from utmp */
#define	AUTH_U_ID		"u_id"
	u_short	fd_uid;	 	/* uid associated with name above */
#define	AUTH_U_PWD		"u_pwd"
	char	fd_encrypt[AUTH_CIPHERTEXT_SIZE(AUTH_SEGMENTS(AUTH_MAX_PASSWD_LENGTH)) + 1];	/* Encrypted password */
#define	AUTH_U_TYPE		"u_type"
	mask_t	fd_type[AUTH_TYPEVEC_SIZE];	/* user type */
#define	AUTH_U_OWNER		"u_owner"
	char	fd_owner[9];	/* if a pseudo -user, the user behind it */
#define	AUTH_U_PRIORITY		"u_priority"
	int	fd_nice;	/* nice value with which to login */
#define	AUTH_U_CMDPRIV		"u_cmdpriv"
	mask_t	fd_cprivs[AUTH_CPRIVVEC_SIZE];	/* command privilege vector */
#define	AUTH_U_SYSPRIV		"u_syspriv"
	mask_t	fd_sprivs[SEC_SPRIVVEC_SIZE];	/* system privilege vector */
#define	AUTH_U_BASEPRIV		"u_basepriv"
	mask_t	fd_bprivs[SEC_SPRIVVEC_SIZE];	/* base privilege vector */
#define	AUTH_U_AUDITCNTL	"u_auditcntl"
	mask_t	fd_auditcntl[AUTH_AUDITMASKVEC_SIZE];	/* audit control */
#define	AUTH_U_AUDITDISP	"u_auditdisp"
	mask_t	fd_auditdisp[AUTH_AUDITMASKVEC_SIZE];	/* audit disposition */

	/* Password maintenance parameters: */
#define	AUTH_U_MINCHG		"u_minchg"
	time_t	fd_min;		/* minimum time between password changes */
#define	AUTH_U_MAXLEN		"u_maxlen"
	int	fd_maxlen;	/* maximum length of password */
#define	AUTH_U_EXP		"u_exp"
	time_t	fd_expire;	/* expiration time duration in secs */
#define	AUTH_U_LIFE		"u_life"
	time_t	fd_lifetime;	/* account death duration in seconds */
#define	AUTH_U_SUCCHG		"u_succhg"
	time_t	fd_schange;	/* last successful change in secs past 1/1/70 */
#define	AUTH_U_UNSUCCHG		"u_unsucchg"
	time_t	fd_uchange;	/* last unsuccessful change */
#define	AUTH_U_PSWDUSER		"u_pswduser"
	u_short	fd_pswduser;	/* who can change this user's password */
#define	AUTH_U_PICKPWD		"u_pickpw"
	char	fd_pick_pwd;	/* can user pick his own passwords? */
#define	AUTH_U_GENPWD		"u_genpwd"
	char	fd_gen_pwd;	/* can user get passwords generated for him? */
#define	AUTH_U_RESTRICT		"u_restrict"
	char	fd_restrict;	/* should generated passwords be restricted? */
#define	AUTH_U_NULLPW		"u_nullpw"
	char	fd_nullpw;	/* is user allowed to have a null password? */
#define	AUTH_U_PWCHANGER	"u_pwchanger"
	u_short	fd_pwchanger;	/* who last changed this user's password */
#ifdef TMAC
#define	AUTH_U_PW_ADMIN_NUM	"u_pw_admin_num"
	long	fd_pw_admin_num;/* password generation verifier */
#endif
#define	AUTH_U_GENCHARS		"u_genchars"
	char	fd_gen_chars;	/* can have password of random ASCII? */
#define	AUTH_U_GENLETTERS	"u_genletters"
	char	fd_gen_letters;	/* can have password of random letters? */
#define	AUTH_U_SEED		"u_seed"
	mask_t	fd_seed[AUTH_SEEDVEC_SIZE];	/* seed selection parameters */
#define	AUTH_U_TOD		"u_tod"
	char	fd_tod[AUTH_TOD_SIZE];		/* times when user may login */

	/* Mandatory policy parameters: */
#define	AUTH_U_CLEARANCE	"u_clearance"
	mand_ir_t fd_clearance;	/* internal representation of clearance */
	char fd_clearance_filler[200]; /* MUST follow fd_clearance */

	/* Login parameters: */
#define	AUTH_U_SUCLOG		"u_suclog"
	time_t	fd_slogin;	/* last successful login */
#define	AUTH_U_UNSUCLOG		"u_unsuclog"
	time_t	fd_ulogin;	/* last unsuccessful login */
#define AUTH_U_SUCTTY		"u_suctty"
	char	fd_suctty[14];	/* tty of last successful login */
#define	AUTH_U_NUMUNSUCLOG	"u_numunsuclog"
	short	fd_nlogins;	/* consecutive unsuccessful logins */
#define AUTH_U_UNSUCTTY		"u_unsuctty"
	char	fd_unsuctty[14];/* tty of last unsuccessful login */
#define	AUTH_U_MAXTRIES		"u_maxtries"
	u_short	fd_max_tries;	/* maximum unsuc login tries allowed */
#define	AUTH_U_LOCK		"u_lock"
	char	fd_lock;	/* Unconditionally lock account? */


};


struct pr_flag  {
	unsigned short
		/* Identity: */
		fg_name:1,		/* Is fd_name set? */
		fg_uid:1,		/* Is fd_uid set? */
		fg_encrypt:1,		/* Is fd_encrypt set? */
		fg_type:1,		/* Is fd_type set? */
		fg_owner:1,		/* Is fd_owner set? */
		fg_nice:1,		/* Is fd_nice set? */
		fg_cprivs:1,		/* Is fd_sprivs set? */
		fg_sprivs:1,		/* Is fd_sprivs set? */
		fg_bprivs:1,		/* Is fd_bprivs set? */
		fg_auditcntl:1,		/* Is fd_auditcntl set? */
		fg_auditdisp:1,		/* Is fd_auditdisp set? */

		/* Password maintenance parameters: */
		fg_min:1,		/* Is fd_min set? */
		fg_maxlen:1,		/* Is fd_maxlen set? */
		fg_expire:1,		/* Is fd_expire set? */
		fg_lifetime:1,		/* Is fd_lifetime set? */
		fg_schange:1,		/* Is fd_schange set? */
		fg_uchange:1,		/* Is fd_fchange set? */
		fg_pswduser:1,		/* Is fd_pswduser set? */
		fg_pick_pwd:1,		/* Is fd_pick_pwd set? */
		fg_gen_pwd:1,		/* Is fd_gen_pwd set? */
		fg_restrict:1,		/* Is fd_restrict set? */
		fg_nullpw:1,		/* Is fd_nullpw set? */
		fg_pwchanger:1,		/* Is fd_pwchanger set? */
#ifdef TMAC
		fg_pw_admin_num:1,	/* Is fd_pw_admin_num set? */
#endif
		fg_gen_chars:1,		/* Is fd_gen_chars set? */
		fg_gen_letters:1,	/* Is fd_gen_letters set? */
		fg_seed:1,		/* Is fd_seed set? */
		fg_tod:1,		/* Is fd_tod set? */

		/* Mandatory policy parameters: */
		fg_clearance:1,		/* Is fd_clearance set? */

		/* Login parameters: */
		fg_slogin:1,		/* Is fd_slogin set? */
		fg_suctty: 1,		/* is fd_suctty set ? */
		fg_unsuctty: 1,		/* is fd_unsuctty set ? */
		fg_ulogin:1,		/* Is fd_ulogin set? */
		fg_nlogins:1,		/* Is fd_nlogins set? */
		fg_max_tries:1,		/* Is fd_max_tries set? */
		fg_lock:1,		/* Is fd_lock set? */

		/* System parameters: */
		fg_standpswd:1,		/* Is fd_standpswd set? */
		fg_secclass:1		/* Is fd_secclass set? */
		;
};


struct pr_passwd  {
	struct pr_field ufld;	/* Fields assoc specifically with this user */
	struct pr_flag uflg;	/* Flags assoc specifically with this user */
	struct pr_field sfld;	/* Fields assoc with system */
	struct pr_flag sflg;	/* Flags assoc with system */
};


#define	AUTH_TLSIZ		100


/*  Terminal Control Database Entry  */

struct	t_field  {
#define	AUTH_T_DEVNAME		"t_devname"
	char	fd_devname[14];	/* Device/host name */
#define	AUTH_T_UID		"t_uid"
	u_short	fd_uid;		/* uid of last successful login */
#define	AUTH_T_LOGTIME		"t_logtime"
	time_t	fd_slogin;	/* time stamp of   "        "   */
#define	AUTH_T_UNSUCUID		"t_unsucuid"
	u_short	fd_uuid;	/* uid of last unsuccessful login */
#define	AUTH_T_UNSUCTIME	"t_unsuctime"
	time_t	fd_ulogin;	/* time stamp of  "           "   */
#define	AUTH_T_PREVUID		"t_prevuid"
	u_short	fd_loutuid;	/* uid of last logout */
#define	AUTH_T_PREVTIME		"t_prevtime"
	time_t	fd_louttime;	/* time stamp of   "    */
#define	AUTH_T_FAILURES		"t_failures"
	u_short	fd_nlogins;	/* consecutive failed attempts */
#define	AUTH_T_MAXTRIES		"t_maxtries"
	u_short	fd_max_tries;	/* maximum unsuc login tries allowed */
#define	AUTH_T_LOGDELAY		"t_logdelay"
	time_t	fd_logdelay;	/* delay between login tries */
#define	AUTH_T_LABEL		"t_label"
	char	fd_label[AUTH_TLSIZ];/* terminal label */
#define	AUTH_T_LOCK		"t_lock"
	char	fd_lock;	/* terminal locked? */
#define AUTH_T_LOGIN_TIMEOUT	"t_login_timeout"
	u_short	fd_login_timeout ;	/* login timeout in seconds */
};


struct	t_flag  {
	unsigned short
		fg_devname:1,		/* Is fd_devname set? */
		fg_uid:1,		/* Is fd_uid set? */
		fg_slogin:1,		/* Is fd_stime set? */
		fg_uuid:1,		/* Is fd_uuid set? */
		fg_ulogin:1,		/* Is fd_ftime set? */
		fg_loutuid:1,		/* Is fd_loutuid set? */
		fg_louttime:1,		/* Is fd_louttime set? */
		fg_nlogins:1,		/* Is fd_nlogins set? */
		fg_max_tries:1,		/* Is fd_max_tries set? */
		fg_logdelay:1,		/* Is fd_logdelay set? */
		fg_label:1,		/* Is fd_label set? */
		fg_lock:1,		/* Is fd_lock set? */
		fg_login_timeout : 1	/* is fd_login_timeout valid? */
		;
};


struct	pr_term  {
	struct t_field ufld;
	struct t_flag uflg;
	struct t_field sfld;
	struct t_flag sflg;
};


/*  File Control Database Entry  */

struct	f_field  {
	char	*fd_name;	/* Holds full path name */
#define	AUTH_F_OWNER		"f_owner"
	u_short	fd_uid;		/* uid of owner */
#define	AUTH_F_GROUP		"f_group"
	u_short	fd_gid;		/* gid of group */
#define	AUTH_F_MODE		"f_mode"
	u_short	fd_mode;		/* permissions */
#define	AUTH_F_TYPE		"f_type"
	char 	fd_type[2];	/* file type (one of r,b,c,d,f,s) */
#define	AUTH_F_SIZE		"f_size"
	off_t	fd_size;	/* size in bytes */
#define	AUTH_F_MTIME		"f_mtime"
	time_t	fd_mtime;	/* last time file noted as touched */
#define	AUTH_F_CTIME		"f_ctime"
	time_t	fd_ctime;	/* last time inode noted as touched */
#define	AUTH_F_CKSUM		"f_cksum"
	long	fd_cksum;	/* checksum file should have (see sum(1)) */
#define	AUTH_F_SLEVEL		"f_slevel"
#define	AUTH_F_SYSLO		"syslo"
#define	AUTH_F_SYSHI		"syshi"
#define	AUTH_F_WILD		"WILDCARD"
	mand_ir_t *fd_slevel;	/* sensitivity level for file */
#define	AUTH_F_ACL		"f_acl"
	acle_t	*fd_acl;	/* access control list for file */
	int	fd_acllen;	/* number of entries in fd_acl */
#define	AUTH_F_PPRIVS		"f_pprivs"
	mask_t	fd_pprivs[SEC_SPRIVVEC_SIZE];	/* potential privileges */
#define	AUTH_F_GPRIVS		"f_gprivs"
	mask_t	fd_gprivs[SEC_SPRIVVEC_SIZE];	/* granted privileges */
};

struct	f_flag  {
	unsigned short
		fg_name:1,	/* Is fd_name set? */
		fg_uid:1,	/* Is fd_uid set? */
		fg_gid:1,	/* Is fd_gid set? */
		fg_mode:1,	/* Is fd_mode set? */
		fg_type:1,	/* Is fd_type set? */
		fg_size:1,	/* Is fd_size set? */
		fg_mtime:1,	/* Is fd_mtime set? */
		fg_ctime:1,	/* Is fd_ctime set? */
		fg_cksum:1,	/* Is fd_cksum set? */
		fg_slevel:1,	/* Is fd_slevel set? */
		fg_acl:1,	/* Is fd_acl set? */
		fg_pprivs:1,	/* Is fd_pprivs set? */
		fg_gprivs:1;	/* Is fd_gprivs set? */
};

struct	pr_file  {
	struct f_field ufld;
	struct f_flag uflg;
};



/*  Printer Control Database Entry  */

struct	l_field  {
#define	AUTH_L_LPNAME		"l_name"
	char	fd_name[15];	/* holds printer name */
#define	AUTH_L_INITSEQ		"l_initseq"
	char	fd_initseq[256];/* initial sequence */
#define	AUTH_L_TERMSEQ		"l_termseq"
	char	fd_termseq[256];/* termination sequence */
#define	AUTH_L_EMPH		"l_emph"
	char	fd_emph[256];	/* emphasize sequence */
#define	AUTH_L_DEEMPH		"l_deemph"
	char	fd_deemph[256];	/* de-emphasize sequence */
#define	AUTH_L_CHRS		"l_chrs"
	char	fd_chrs[130];	/* characters to filter */
#define	AUTH_L_CHRSLEN		"l_chrslen"
	u_short	fd_chrslen;	/* length of string of illegal chars */
#define	AUTH_L_ESCS		"l_escs"
	char	fd_escs[256];	/* escape sequences */
#define	AUTH_L_ESCSLEN		"l_esclens"
	u_short	fd_escslen;	/* length of string of illegal escape codes */
#define	AUTH_L_LINELEN		"l_linelen"
	int	fd_linelen;	/* length of a line in characters */
#define	AUTH_L_PAGELEN		"l_pagelen"
	int	fd_pagelen;	/* length of a page in lines */
#define	AUTH_L_TRUNCLINE	"l_truncline"
	char	fd_truncline;	/* does printer truncate long lines? */
};

struct	l_flag  {
	unsigned short
		fg_name:1,	/* Is fd_name set? */
		fg_initseq:1,	/* Is fd_initseq set? */
		fg_termseq:1,	/* Is fd_termseq set? */
		fg_emph:1,	/* Is fd_emph set? */
		fg_deemph:1,	/* Is fd_deemph set? */
		fg_chrs:1,	/* Is fd_chrs set? */
		fg_chrslen:1,	/* Is fd_chrslen set? */
		fg_escs:1,	/* Is fd_escs set? */
		fg_escslen:1,	/* Is fd_escslen set? */
		fg_linelen:1,	/* Is fd_linelen set? */
		fg_pagelen:1,	/* Is fd_pagelen set? */
		fg_truncline:1	/* Is fd_truncline set? */
		;
};

struct	pr_lp  {
	struct l_field ufld;
	struct l_flag uflg;
	struct l_field sfld;
	struct l_flag sflg;
};


/* Device Assignment Database entry */

#define AUTH_DEV_TYPE "device type"
#define AUTH_DEV_PRINTER	0
#define AUTH_DEV_TERMINAL	1
#define AUTH_DEV_TAPE		2
#define AUTH_DEV_REMOTE		3
#define AUTH_MAX_DEV_TYPE	3

#define AUTH_DEV_TYPE_SIZE	(WORD_OF_BIT (AUTH_MAX_DEV_TYPE) + 1)

#define AUTH_DEV_ASSIGN	"device assignment"
#define AUTH_DEV_SINGLE  0	/* single-level */
#define AUTH_DEV_MULTI   1	/* multilevel */
#define AUTH_DEV_LABEL   2	/* labeled import/export enabled */
#define AUTH_DEV_NOLABEL 3	/* unlabeled import/export enabled */
#define AUTH_DEV_IMPORT  4	/* enabled for import */
#define AUTH_DEV_EXPORT  5	/* enabled for export */
#define AUTH_DEV_PASS	 6	/* internal to mltape */
#if SEC_ILB
#define AUTH_DEV_ILSINGLE       7       /* single-level info. labels. */
#define AUTH_DEV_ILMULTI        8       /* multilevel info. labels. */
#define AUTH_MAX_DEV_ASSIGN 	8
#else
#define AUTH_MAX_DEV_ASSIGN 6
#endif
#define AUTH_DEV_ASSIGN_SIZE	(WORD_OF_BIT (AUTH_MAX_DEV_ASSIGN) + 1)

struct dev_field {
	char	*fd_name;	/* external name */
#define AUTH_V_DEVICES	"v_devs"
	char	**fd_devs;	/* device list */
#define AUTH_V_TYPE	"v_type"
	mask_t	fd_type[AUTH_DEV_TYPE_SIZE];	/* tape, printer, terminal */
#define AUTH_V_MAX_SL	"v_maxsl"
	mand_ir_t	*fd_max_sl;	/* maximum sensitivity level */
#define AUTH_V_MIN_SL	"v_minsl"
	mand_ir_t	*fd_min_sl ;	/* minimum sensitivity level */
#define AUTH_V_CUR_SL	"v_cursl"
	mand_ir_t	*fd_cur_sl ;	/* currently assigned s.l. */
#if SEC_ILB
#define AUTH_V_CUR_IL   "v_curil"
	ilb_ir_t        *fd_cur_il ;    /* currently assigned info l. */
#endif
#define AUTH_V_ASSIGN	"v_assign"
	mask_t	fd_assign[AUTH_DEV_ASSIGN_SIZE];/* single-, multilevel, etc. */


#define	AUTH_V_USERS	"v_users"
	char	**fd_users ; 	/* list of users */
};

struct dev_flag {
	unsigned short
			fg_name : 1,
			fg_devs : 1,
			fg_type : 1,
			fg_max_sl : 1,
			fg_min_sl : 1,
			fg_cur_sl : 1,
#if SEC_ILB
			fg_cur_il : 1,
#endif
			fg_assign : 1,
			fg_users  : 1;
};

struct dev_asg {
	struct dev_field ufld;
	struct dev_flag  uflg;
	struct dev_field sfld;
	struct dev_flag  sflg;
};


/*
 *
 *	Structure definitions for the System Default global values.
 *
 */

#define	AUTH_D_INACTIVITY_TIMEOUT	"d_inactivity_timeout"
#define	AUTH_D_PW_EXPIRE_WARNING	"d_pw_expire_warning"
#define	AUTH_D_BOOT_AUTHENTICATE	"d_boot_authenticate"
#define	AUTH_D_AUDIT_ENABLE		"d_audit_enable"
#define	AUTH_D_SECCLASS			"d_secclass"

/*
 * Administrator's password seed value (previously in /tcb/files/seed)
 */

#define AUTH_D_PW_SEED_VALUE		"d_pw_seed_value"

#define	AUTH_D_LOGIN_SESSION_TIMEOUT	"d_login_session_timeout"
#define	AUTH_D_LOGIN_SESSION_WARNING	"d_login_session_warning"
#define AUTH_D_TRUSTED_PATH_SEQ		"d_trusted_path_seq"



struct	system_default_fields
  {
    time_t	fd_inactivity_timeout;
    time_t	fd_pw_expire_warning;

    u_long	fd_pw_seed_value;

    mask_t	fd_secclass[AUTH_SECCLASSVEC_SIZE];/* System security class */
    char	fd_boot_authenticate;
    char	fd_audit_enable;

    u_short	fd_session_timeout ;
    u_short	fd_session_warning ;
    char	fd_trusted_path_seq[AUTH_TRUSTED_PATH_LENGTH+1] ;
  } ;

struct	system_default_flags
  {
    unsigned short
		fg_inactivity_timeout  : 1,
		fg_pw_expire_warning   : 1,
		fg_pw_seed_value       : 1,	/* set for fd_pw_seed_value */
		fg_boot_authenticate   : 1,
		fg_audit_enable        : 1,
		fg_session_timeout     : 1,	/* set if fd_session valid */
		fg_session_warning     : 1,	/* set if fd_session valid */
		fg_trusted_path_seq    : 1,	/* set if fd_trusted valid */
		fg_secclass            : 1 ;
  } ;

struct	pr_default  {
#define	AUTH_D_NAME			"d_name"
	char				dd_name[20] ;
	char				dg_name ;
	struct pr_field			prd ;
	struct pr_flag			prg ;
	struct t_field			tcd ;
	struct t_flag			tcg ;
	struct dev_field		devd ;
	struct dev_flag			devg ;
	struct system_default_fields	sfld ;
	struct system_default_flags	sflg ;
} ;


struct namepair  {
	char *name;
	u_long value;
};


extern char *command_name;
extern char *command_line;

extern struct namepair user_type[];
extern struct namepair *cmd_priv;
extern struct namepair sys_priv[];
extern struct namepair secclass[];
extern struct namepair audit_mask[];
extern struct namepair seed_choice[];
extern struct namepair auth_dev_assign[];
extern struct namepair auth_dev_type[];


/* Functions from accept_pw.c */
extern int	acceptable_password();

/* Functions from authaudit.c */
extern void	audit_lax_file(), audit_security_failure(),
		sa_audit_security_failure(), audit_no_resource(),
		sa_audit_no_resource(), audit_auth_entry(), audit_subsystem(),
		sa_audit_subsystem(), audit_login(), audit_passwd(),
		audit_lock(), audit_adjust_mask(), sa_audit_lock(),
		sa_audit_audit();

/* Functions from authcap.c */
extern void	asetdefaults(), open_auth_file(), end_authcap(), endprdfent(),
		setprdfent();
extern int	agetuser(), agettty(), agetdefault(), agetnum(),
		agetuid(), agetgid(), agetflag();
extern char	*agetfile(), *agetdvag(), *agetstr(), **agetstrlist(),
		*find_auth_file(), *agetdefault_buf();
extern long	adecodenum();
extern int	agetlp();

/* Functions from checkdata.c */
extern void	check_basic_data_structures();

/* Functions from discr.c */
extern void	setuid_least_privilege();
extern int	make_transition_files(), replace_file(),
		create_file_securely() ;

/* Functions from disk.c */
extern void	disk_set_file_system(), disk_inode_incr();
extern daddr_t	disk_itod(), disk_itoo();
extern int	disk_secure_file_system(), disk_inodes_per_block(),
		disk_dinode_size();

/* Functions from fields.c */
extern void	loadnamepair();
extern char	*storenamepair(), *storebool();
extern int	locked_out(), pr_newline(), auth_for_terminal();

/* Fucntions from getprpwent.c */
extern struct pr_passwd
		*getprpwent(), *getprpwuid(), *getprpwnam();
extern void	setprpwent(), endprpwent();
extern int	putprpwnam(), store_pw_fields(), read_pw_fields();

/* Functions from getprtcent.c */
extern struct pr_term
		*getprtcent(), *getprtcnam();
extern void	setprtcent(), endprtcent(), read_tc_fields();
extern int	putprtcnam(), store_tc_fields();

/* Functions from getprfient.c */
extern struct pr_file
		*getprfient(), *getprfinam();
extern void	setprfient(), endprfient();
extern int	putprfinam();

/* Functions from getprdfent.c */
extern struct pr_default
		*getprdfent(), *getprdfnam();
extern int	putprdfnam();

/* Functions from getdvagent.c */
extern struct dev_asg
		*getdvagent(), *getdvagnam(), *copydvagent();
extern void	setdvagent(), enddvagent();
extern int	putdvagnam();

/* Functions from getprlpent.d */
extern struct pr_lp
		*getprlpent(), *getprlpnam();
extern void	setprlpent(), endprlpent();
extern int	putprlpnam();

/* Functions from identity.c */
extern void	set_auth_parameters(), check_auth_parameters(),
		enter_quiet_zone(), exit_quiet_zone();
extern int	is_starting_luid(), is_starting_ruid(), is_starting_euid(),
		is_starting_rgid(), is_starting_egid();
extern u_short	starting_luid(), starting_ruid(), starting_euid(),
		starting_rgid(), starting_egid();

/* Functions from passlen.c */
extern int	passlen();

/* Functions from getpasswd.c */
extern char	*getpasswd(), *bigcrypt();

/* Functions from subsystems.c */
extern int	privileged_user(), authorized_user(), total_auths(),
		widest_auth(), primary_auth(), secondary_auth(),
		write_authorizations(), build_cmd_priv();
extern char	*primary_of_secondary_auth();

/* Functions from randomword.c */
extern int	randomword();
extern int	randomchars(), randomletters();

/* Functions from security.c */
extern uid_t	getluid();
extern int	stopio(), setluid(), getpriv(), setpriv(),
		statpriv(), chpriv();
extern int	eaccess(), mkmultdir(), rmmultdir(), getlabel(), setlabel(),
		lmount();

/* Functions from seed.c */
extern void	set_seed();
extern long	get_seed();

/* Functions from tod.c */
extern int	time_lock();

/* Functions from map_ids.c */
extern char	*pw_idtoname(), *gr_idtoname();
extern int	pw_nametoid(), gr_nametoid();

/* Functions from scandir.c */
extern int	alphasort(), scandir();

/* Functions from printbuf.c */
extern void	printbuf();

/* Functions from privileges.c */
extern int	hassysauth(), disablesysauth(), enablepriv(),
		disablepriv();
extern void	initprivs() ;

#endif  /* _PROT_INCLUDED */
