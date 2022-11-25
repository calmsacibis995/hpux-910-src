/* @(#) $Revision: 66.1 $ */
/*
 * audnetd.h supplies constant values usable as parameters for
 * library function audit_daemon() in libsec.  The function represents
 * a common interface from the networking daemons to the
 * auditing subsystem.
 */

#ifdef SecureWare
#ifndef _AUDNETD_INCLUDED /* allow multiple inclusions */
#define _AUDNETD_INCLUDED

/* network audit result - success or failure	*/
#define NA_RSLT_SUCCESS		0
#define NA_RSLT_FAILURE		1


/* network audit validation mechanisms	*/
#define NA_VALI_OTHER		0
#define NA_VALI_PASSWD		1
#define	NA_VALI_RUSEROK		2
#define	NA_VALI_UID		3

#define	NA_VALI_LAST		NA_VALI_UID


/* network audit event	*/
#define NA_EVNT_OTHER		0
#define NA_EVNT_START		1
#define NA_EVNT_STOP		2
#define	NA_EVNT_PASSWD		3
#define	NA_EVNT_IDENT		4
#define	NA_EVNT_CONN		5

#define	NA_EVNT_LAST		NA_EVNT_CONN

/* network audit ignore mask	*/
#define	NA_MASK_NONE		0x0000
#define	NA_MASK_SERV		0x0001	/* obsolete */
#define	NA_MASK_RSLT		0x0002
#define	NA_MASK_VALI		0x0004
#define	NA_MASK_RSYS		0x0008	/* obsolete; use NA_MASK_RADDR */
#define	NA_MASK_RADDR		0x0008
#define	NA_MASK_RHOST		0x0400
#define	NA_MASK_RUSR		0x0010	/* obsolete; use NA_MASK_RUID */
#define	NA_MASK_RUID		0x0010
#define	NA_MASK_RUSRNAME	0x0800
#define	NA_MASK_LSYS		0x0020  /* obsolete */
#define	NA_MASK_LUID		0x0040
#define	NA_MASK_LUSR		0x0040	/* obsolete; use NA_MASK_LUID */
#define	NA_MASK_LUSRNAME	0x1000
#define	NA_MASK_EVNT		0x0080
#define	NA_MASK_MASK		0x0100
#define	NA_MASK_FREE		0x0200

#endif /* _AUDNETD_INCLUDED */

#else   /* Not SecureWare */

#ifndef _AUDNETD_INCLUDED /* allow multiple inclusions */
#define _AUDNETD_INCLUDED

/* network audit services    */
#define NA_SERV_OTHER	        "other   "
#define NA_SERV_RFA	        "rfa     "
#define	NA_SERV_NFT		"nft     "
#define NA_SERV_TELNET	        "telnet  "
#define NA_SERV_FTP	        "ftp     "
#define NA_SERV_SENDMAIL	"mail    "
#define NA_SERV_NETISR		"netisr  "
#define NA_SERV_INETD		"inetd   "
#define	NA_SERV_PTY		"pty     "
#define	NA_SERV_SOCKREGD	"sockreg "
#define	NA_SERV_PORTMAP		"portmap "
#define	NA_SERV_REXECD		"rexec   "
#define NA_SERV_RLOGIN	        "rlogin  "
#define NA_SERV_REMSH	        "remsh   "
#define NA_SERV_RWHOD		"rwho    "
#define NA_SERV_NINSTALL        "install "
#define NA_SERV_TIMED		"timed   "
#define NA_SERV_NAMED		"named   "
#define	NA_SERV_REXD		"rex     "
#define	NA_SERV_MOUNTD		"mount   "
#define	NA_SERV_YPPASSWDD	"yppass  "
#define	NA_SERV_LMX		"lan mgr "

/* network audit result - success or failure	*/
#define NA_RSLT_SUCCESS		0
#define NA_RSLT_FAILURE		1

/* network audit validation mechanisms	*/
#define NA_VALI_OTHER		0
#define NA_VALI_PASSWD		1
#define	NA_VALI_RUSEROK		2
#define	NA_VALI_UID		3

#define	NA_VALI_LAST		NA_VALI_UID

/* network audit event	*/
#define NA_EVNT_OTHER		0
#define NA_EVNT_START		1
#define NA_EVNT_STOP		2
#define	NA_EVNT_PASSWD		3

#define	NA_EVNT_LAST		NA_EVNT_PASSWD

/* old STAT definitions preserved for backwards compatibility */
#define NA_STAT_OTHER		NA_EVNT_OTHER
#define NA_STAT_START		NA_EVNT_START
#define NA_STAT_STOP		NA_EVNT_STOP

/* network audit ignore mask	*/
#define	NA_MASK_NONE		0x000
#define	NA_MASK_SERV		0x001
#define	NA_MASK_RSLT		0x002
#define	NA_MASK_VALI		0x004
#define	NA_MASK_RSYS		0x008
#define	NA_MASK_RUSR		0x010
#define	NA_MASK_LSYS		0x020
#define	NA_MASK_LUSR		0x040
#define	NA_MASK_EVNT		0x080
#define	NA_MASK_MASK		0x100
#define	NA_MASK_FREE		0x200

#endif /* _AUDNETD_INCLUDED */

#endif /* SecureWare */
