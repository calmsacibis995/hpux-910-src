/* @(#)  $Revision: 66.3 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)dumprestore.h	5.1 (Berkeley) 6/5/85
 */

/*
 * TP_BSIZE is the size of file blocks on the dump tapes.
 * Note that TP_BSIZE must be a multiple of DEV_BSIZE.
 *
 * NTREC is the number of TP_BSIZE blocks that are written
 * in each tape record. HIGHDENSITYTREC is the number of
 * TP_BSIZE blocks that are written in each tape record on
 * 6250 BPI or higher density tapes.
 *
 * TP_NINDIR is the number of indirect pointers in a TS_INODE
 * or TS_ADDR record. Note that it must be a power of two.
 */
#define TP_BSIZE	1024
#define NTREC   	10
#define HIGHDENSITYTREC	32
#define TP_NINDIR	(TP_BSIZE/2)

#define TS_TAPE 	1
#define TS_INODE	2
#define TS_BITS 	3
#define TS_ADDR 	4
#define TS_END  	5
#define TS_CLRI 	6
#if defined(TRUX) && defined(B1)
#define TS_SEC_INODE    7
#define BLS_B1_MAGIC    (int)60014      /* When it_is_b1fs is true=tagged fs */
#define BLS_C2_MAGIC    (int)60015      /* When it_is_b1fs is false=reg. fs  */
#endif  /* TRUX & B1 */
#define OFS_MAGIC   	(int)60011
#define NFS_MAGIC   	(int)60012
#ifdef CNODE_DEV
#define CSD_MAGIC       (int)60013
#endif CNODE_DEV
#define CHECKSUM	(int)84446

union u_spcl {
	char dummy[TP_BSIZE];
	struct	s_spcl {
		int	c_type;
		time_t	c_date;
		time_t	c_ddate;
		int	c_volume;
		daddr_t	c_tapea;
		ino_t	c_inumber;
		int	c_magic;
		int	c_checksum;
#if defined(TRUX) && defined(B1)
                union {
                struct dinode   c_dinode;
                struct sec_dinode s_dinode;
                } udino;
#else /* TRUX & B1 */
                struct  dinode  c_dinode;
#endif  /* TRUX & B1 */
		int	c_count;
		char	c_addr[TP_NINDIR];
	} s_spcl;
} u_spcl;

#define spcl u_spcl.s_spcl

#define	DUMPOUTFMT	"%-s %c %s"		/* for printf */
						/* name, incno, ctime(date) */
#define	DUMPINFMT	"%s %c %[^\n]\n"	/* inverse for scanf */

#if defined(TRUX) && defined(B1)

/* copy of  parts of 
 * @(#)export_sec.h:  with modified msgs
 * Error message definition table. NOTE msg#s are also differ from export_sec.h
 * The constants define message triplets
 * "User message", "Audit op", "Audit result".
 * Null entries are skipped.
 */

#define NOACTION		0
#define TERMINATE		1

#define MSG_NOAUTH		0
#define MSG_DEVLAB		1
#define MSG_DEVDB		2
#define MSG_DEVASSIGN		3
#define MSG_DEVIMPEXP		4
#define MSG_DEVSENSLEV		5
#define MSG_DEVNOLEV		6
#define MSG_DEVMAXLEV		7
#define MSG_DEVMINLEV		8
#define MSG_CHGLEV		9
#define MSG_SETPPRIVS		10
#define MSG_SETGPRIVS		11
#define MSG_AUTHDEV		12


typedef struct {
		char *msg;
		char *aud_op;
		char *aud_res;
		int  action;
	} audmsg_t;
static audmsg_t audmsg[] = {		/* corresponding #s in export_sec.h */
    	{	"No authorization for attempted operation", /* 2 MSG_NOAUTH */
		"Tape authorization",
		"Authorization not granted",
		TERMINATE
	}, 
	{ 	"Cannot determine device label",	/* 6 MSG_DEVLAB */
		"Determine device label",
		"Failed",
		TERMINATE
	},
	{	"Device not in database",		/* 7 MSG_DEVDB */
		"Lookup device in device database",
		"Entry missing",
		TERMINATE
	},

	{	"Incorrect import/export level",	/* 8 MSG_DEVASSIGN */
		"Check single/multi level assignment",
		"Failed",
		TERMINATE
	},
	{	"Device not enabled for import/export",	/* 9 MSG_DEVIMPEXP */
		"Check import/export enabled",
		"Failed",
		TERMINATE
	},
	{	"Unrecognized device level",		/* 10 MSG_DEVSENSLEV */
		"Check assigned device level",
		"Unrecognized level",
		TERMINATE
	},
	{	"Device has no assigned level",		/* 11 MSG_DEVNOLEV */
		"Check assigned device level",
		"No level set",
		TERMINATE
	},
							/* 12 MSG_DEVMAXLEV */
	{	"Missing or unrecognized maximum device level",
		"Check device maximum level",
		"Missing or unrecognized",
		TERMINATE
	},
							/* 13 MSG_DEVMINLEV */
	{	"Missing or unrecognized minimum device level",
		"Check device minimum level",
		"Missing or unrecognized",
		TERMINATE
	},
	{       "Cannot change process level",             /* 33 MSG_CHGLEV */
		"Change process level",
		"Denied, terminated",
	 	NOACTION
	},
	{	"Cannot set potential priv set",	  /* 36 MSG_SETPPRIVS */
		"Set potential set",
		"Denied, ignored",
		NOACTION
	},
	{	"Cannot set granted priv set",	   	  /* 37 MSG_SETGPRIVS */
		"Set granted set",
		"Denied, ignored",
		NOACTION
	},
	{	"Not authorized for this device",	/* 50 MSG_AUTHDEV */
		"Authorized for device",
		"Not authorized for device, terminated",
		TERMINATE
	}
	};

#endif /* TRUX & B1 */
