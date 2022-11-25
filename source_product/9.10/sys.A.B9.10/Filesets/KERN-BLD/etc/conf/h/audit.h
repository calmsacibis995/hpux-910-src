/* $Header: audit.h,v 1.8.83.3 93/09/17 17:01:58 root Exp $ */

#ifndef _SYS_AUDIT_INCLUDED
#define _SYS_AUDIT_INCLUDED

/*
 *  audit.h: definitions for auditing functions
 */

#ifdef _KERNEL_BUILD
#include "../h/stdsyms.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/stdsyms.h>
#endif /* _KERNEL_BUILD */

#ifdef _INCLUDE_HPUX_SOURCE

#ifdef _KERNEL_BUILD
#include "../h/types.h"		/* types.h includes stdsyms.h */
#include "../h/param.h"
#include "../h/socket.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/types.h>		/* types.h includes stdsyms.h */
#include <sys/param.h>
#include <sys/socket.h>
#endif /* _KERNEL_BUILD */

/*  added for C++ compatibility */
#ifdef __cplusplus
#define delete c_delete
#endif /* __cplusplus */

#ifndef _AID_T
#  define _AID_T
   typedef long aid_t;
#endif /* _AID_T */


/* User interface structures */

struct aud_event_tbl {
	unsigned create : 2;
	unsigned delete : 2;
	unsigned moddac : 2;
	unsigned modaccess : 2;
	unsigned open : 2;
	unsigned close : 2;
	unsigned process : 2;
	unsigned removable : 2;
	unsigned login : 2;
	unsigned admin : 2;
	unsigned ipccreat : 2;
	unsigned ipcopen : 2;
	unsigned ipcclose : 2;
	unsigned ipcdgram :2;
	unsigned uevent1 : 2;
	unsigned uevent2 : 2;
	unsigned uevent3 : 2;
};


struct aud_type {
	unsigned char logit : 2;
};


struct audit_hdr {
	u_long ah_time;			/* date/time (tv_sec of timeeval) */
	pid_t ah_pid;			/* process ID */
	u_short ah_error;		/* success/failure */
	u_short ah_event;		/* event being audited - event number */
	u_short ah_len;			/* length of variant part */
};


struct audit_str_hdr {			/* Streams audit header */
	u_short	pid;			/* pid of process that made the call */
	u_short ppid;			/* parent process ID */
	aid_t aid;			/* audit ID */
	u_short uid;			/* user ID */
	u_short gid;			/* group ID */
	u_short euid;			/* effective user ID */
	u_short egid;			/* effective group ID */
	u_short tty;			/* tty number */
};


#define MAX_AUD_DATA	60	/* Max numeric audit data */

struct audit_bdy_data {
	union ab_data {				 /*  audit body */
		int ival;
		u_long ulval; 
		long	lval;
		u_short usval;
		char cval;
		u_char ucval;
	} data [MAX_AUD_DATA];
};


struct audit_str_data {
	struct audit_str_hdr str_hdr;
	struct audit_bdy_data bdy_data;
};


#define MAX_AUD_TEXT	2048	/* Max len of the text in audit record body */

struct self_audit_rec {		/* self-auditing record */
	struct audit_hdr aud_head;	/* header */
	union {
		char text[MAX_AUD_TEXT];         /* self explanatory text */
		struct audit_str_data str_data;
	} aud_body;
};

#ifdef __cplusplus
#undef delete
#endif /* __cplusplus */

/* Values for use with aud_type */

#define PASS		01	/* audit successful calls */
#define FAIL		02	/* audit failed calls */
#define BOTH		03	/* audit both success and failures */
#define NONE		00	/* don't audit it */


/* Function prototypes */

#ifndef _KERNEL
#ifdef __cplusplus
   extern "C" {
#endif /* __cplusplus */

#ifdef _PROTOTYPES
    extern int audctl(int, char *, char *, mode_t);
    extern int audswitch(int);
    extern int audwrite(const struct self_audit_rec *);
    extern int getaudid(void);
    extern int getaudproc(void);
    extern int getevent(struct aud_type *, struct aud_event_tbl *);
    extern int setaudid(aid_t);
    extern int setaudproc(int);
    extern int setevent(const struct aud_type [],
			const struct aud_event_tbl []);
#else /* not _PROTOTYPES */
    extern int audctl();
    extern int audswitch();
    extern int audwrite();
    extern int getaudid();
    extern int getaudproc();
    extern int getevent();
    extern int setaudid();
    extern int setaudproc();
    extern int setevent();
#endif /* not _PROTOTYPES */

#ifdef __cplusplus
   }
#endif /* __cplusplus */
#endif /* not _KERNEL */


/* Various constants */

#define MAX_EVENT	64
#define MAX_VALID_SYSCALL	63
#define MAX_SYSCALL	315	/* Max sys call # + 1 */
#define MAXAUDSTRING	70	/* the largest string is currently 68 */
#define MAX_AUD_ARGS	4	/* number of sections the audit rec is 
					broken into */

#define MAXAUDID 	60000	/* max audit ID, same as max uid	*/

#define AUD_PROC	1	/* set the process for auditing		*/
#define AUD_CLEAR	0	/* clear the process from auditiing	*/

#define AUD_SUSPEND     1
#define AUD_RESUME      0


#define AUD_SET 	0x0
#define AUD_GET   	0x1
#define AUD_SETNEXT	0x2
#define AUD_SETCURR	0x3
#define AUD_ON    	0x4	
#define AUD_OFF  	0x5
#define AUD_SWITCH  	0x6


/* Event number ranges for self-auditing event types */

#define EN_CREATE	02000	/* starting event number for each range */
#define EN_DELETE	04000
#define EN_MODDAC	06000
#define EN_MODACCESS	010000
#define EN_OPEN		012000
#define EN_CLOSE	014000
#define EN_PROCESS	016000
#define EN_REMOVABLE	020000
#define EN_LOGIN	022000
#define EN_ADMIN	024000
#define EN_IPCCREAT	026000
#define EN_IPCOPEN	030000
#define EN_IPCCLOSE	032000
#define EN_UEVENT1	034000
#define EN_UEVENT2	036000
#define EN_UEVENT3	040000
#define EN_IPCDGRAM	042000

/* Event numbers for self-auditing events */
/**********************/
/* EN_CREATE	02xxx */
/**********************/
/* EN_DELETE	04xxx */
/**********************/
/* EN_MODDAC	06xxx */
/**********************/
/* EN_MODACCESS	010xxx */
#define EN_NEWGRP	010001	/* event number for newgrp */
/***********************/
/* EN_OPEN	012xxx */
/* Reserve open event subtypes 1-10 for streams specific implicit opens */
#define EN_OPEN_SENDFD  012001  /* event number for streams sending a file
				   descriptor */
#define EN_OPEN_RECVFD  012002  /* event number for streams sending a file
				   descriptor */
#define EN_LP	012013		/* event number for lpsched */
/***********************/
/* EN_CLOSE	014xxx */
/***********************/
/* EN_PROCESS	016xxx */
/***********************/
/* EN_REMOVABLE	020xxx */
/***********************/
/* EN_LOGIN	022xxx */
#define EN_LOGOUT	022001	/* event number for logout (in init) */
#define EN_LOGINS	022002	/* event number for login */
#define EN_PIDWRITE	022003	/* event number for pid identification records*/
/***********************/
/* EN_ADMIN	024xxx */
#define EN_RUNSTATE	024001	/* event number for changing run state (init) */
#define EN_AUDISP	024002	/* event number for audisp */
#define EN_AUDEVENT	024003	/* event number for audevent */
#define EN_AUDSYS	024004	/* event number for audsys */
#define EN_AUDUSR	024005	/* event number for audusr */
#define EN_SAMADDUSR	024006  /* event number for sam */
#define EN_SAMDELUSR	024007  /* event number for sam */
#define EN_SAMREUSR	024010  /* event number for sam */
#define EN_SAMMODUSR	024011  /* event number for sam */
#define EN_SAMNEWGRP	024012  /* event number for sam */
#define EN_SAMDELGRP	024013  /* event number for sam */
#define EN_SAMMODGRP	024014  /* event number for sam */
#define EN_SAMNEWFS	024015  /* event number for sam */
#define EN_SAMMNTFS	024016  /* event number for sam */
#define EN_SAMUNMNTFS	024017  /* event number for sam */
#define EN_SAMSWAP	024020  /* event number for sam */
#define EN_SAMADDFSEN	024021  /* event number for sam */
#define EN_SAMRMFSEN	024022  /* event number for sam */
#define EN_SAMCHGFSEN	024023  /* event number for sam */
#define EN_SAMFBACKUP	024024  /* event number for sam */
#define EN_SAMIBACKUP	024025  /* event number for sam */
#define EN_SAMBUSCHED	024026  /* event number for sam */
#define EN_SAMRECINDEX	024027  /* event number for sam */
#define EN_SAMRECFILE	024030  /* event number for sam */
#define EN_SAMBUFILE	024031  /* event number for sam */
#define EN_SAMUUCP1	024032  /* event number for sam */
#define EN_SAMUUCP2	024033  /* event number for sam */
#define EN_SAMUUCP3	024034  /* event number for sam */
#define EN_SAMUUCP4	024035  /* event number for sam */
#define EN_SAMUUCP5	024036  /* event number for sam */
#define EN_SAMADDTERM	024037  /* event number for sam */
#define EN_SAMGENMIN	024040  /* event number for sam */
#define EN_SAMGENFULL	024041  /* event number for sam */
#define EN_SAMGENCUST	024042  /* event number for sam */
#define EN_SAMLONGFN	024043  /* event number for sam */
#define EN_SAMADDLP	024044  /* event number for sam */
#define EN_SAMRMLP	024045  /* event number for sam */
#define EN_SAMLP	024046  /* event number for sam */
#define EN_CHFN		024050	/* event number for chfn */
#define EN_CHSH		024051	/* event number for chsh */
#define EN_PASSWD	024052	/* event number for passwd */
#define EN_AUDOMON	024053	/* event number for audomon */
/***********************/
/* EN_IPCCREAT	026xxx */
/***********************/
/* EN_IPCOPEN	030xxx */
/* subtypes 1-20 reserved for streams related ipcpen calls */
#define EN_IPC_GETMSG   030001  /* Getmsg streams call for datagrams */
#define EN_IPC_PUTMSG   030002  /* Putmsg streams call for datagrams */
/***********************/
/* EN_IPCCLOSE	032xxx */
/***********************/
/* EN_UEVENT1	034xxx */
/***********************/
/* EN_UEVENT2	036xxx */
/***********************/
/* EN_UEVENT3	040xxx */
/***********************/
/* EN_IPCDGRAM 	042xxx */
/***********************/

struct aud_flag {
	unsigned rtn_val1 : 1;		/* return value 1 */
	unsigned rtn_val2 : 2;		/* return value 2 */
	unsigned param_count : 4;	/* number of param in syscall */
	unsigned char param[10];	/* flags which specify: */
					/*   0 = don't save 	*/
					/*   1 = integer	*/
					/*   2 = unsigned long	*/
					/*   3 = long		*/
					/*   4 = char*		*/
					/*   5 = int*		*/
					/*   6 = int array	*/
					/*   7 = sockaddr - u_short and char* */
					/*   8 = pathname   	*/
					/*   9 = file descriptor */
					/*   10 = timeval - u_long, long */
					/*   11 = timezone - int, int */
					/*   12 = addr of char* */
					/*   13 = char* cast to a long */
					/*   14 = acls		*/
					/*   15 = ret values are file desc. */
};

struct pir_body {			/* pir related info */
	pid_t ppid;			/* parent process ID */
	aid_t aid;			/* audit ID */
	uid_t ruid;			/* user ID */
	gid_t rgid;			/* group ID */
	uid_t euid;			/* effective user ID */
	gid_t egid;			/* effective group ID */
	dev_t tty;			/* tty device number */
};

struct pir_t {				/* pid identification record */
	struct audit_hdr aud_head;	/* standard header */
	struct pir_body pid_info;	/* record body */
};

struct audit_filename {
	u_long apn_cnode;		/* audit pathname cnode */
	u_long apn_dev;			/* audit pathname device */
	u_long apn_inode;		/* audit pathname inode */
	u_short apn_len;		/* audit pathname length */
	char apn_path[MAXPATHLEN];	/* audit pathname in characters */
	struct audit_filename *next;	/* ptr to next filename */
};

struct audit_string {
	unsigned a_strlen;		/* audit string length */
	char a_str[MAXAUDSTRING];	/* audit string */
	struct audit_string *next;	/* ptr to next string */
};

struct audit_sock {
	unsigned a_socklen;		/* audit sockaddr length */
	struct sockaddr a_sock;		/* sockaddr information */
};

struct audit_arg {	/* audit arguments passed to kern_aud_wr */
	char *data;
	short len;
};

#ifdef _KERNEL
int	audit_state_flag;
int	audit_ever_on;
char	curr_file[MAXPATHLEN];
int	audit_mode;
int	currlen;
struct	vnode *curr_vp;
char	next_file[MAXPATHLEN];
int	nextlen;
struct	vnode *next_vp;
int	next_sz;
int	next_dz;

/*
 *  auditon() returns true if the audit_state_flag is on, u.u_audproc is on,
 *            and u.u_audsusp is off;
 *	      returns false if the above 3 checks do not hold;
 */
#define AUDITON()	((audit_state_flag) && (u.u_audproc) && (!u.u_audsusp))

/*
 *  AUDITEVERON() returns true if audit_ever_on is on;
 *	          returns false if the above is off. 
 */
#define AUDITEVERON()	(audit_ever_on)
#endif /* _KERNEL */

#endif /* _INCLUDE_HPUX_SOURCE */

#endif /* _SYS_AUDIT_INCLUDED */
