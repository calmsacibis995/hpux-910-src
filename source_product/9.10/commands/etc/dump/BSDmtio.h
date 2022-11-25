/* @(#)  $Revision: 66.1 $ */
/*	@(#)mtio.h 2.8 88/02/08 SMI; from UCB 4.10 83/01/17	*/

/* structure for MTIOCGET - mag tape get status command */
struct	BSDmtget	{
	short	mt_type;	/* type of magtape device */
/* the following two registers are grossly device dependent */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
/* end device-dependent registers */
	short	mt_resid;	/* residual count */
/* the following two are not yet implemented */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
/* end not yet implemented */
};

/*
 * Constants for mt_type byte
 */
#define	BSD_ISTS		0x01		/* vax: unibus ts-11 */
#define	BSD_ISHT		0x02		/* vax: massbus tu77, etc */
#define	BSD_ISTM		0x03		/* vax: unibus tm-11 */
#define	BSD_ISMT		0x04		/* vax: massbus tu78 */
#define	BSD_ISUT		0x05		/* vax: unibus gcr */
#define	BSD_ISCPC	0x06		/* sun: multibus cpc */
#define	BSD_ISAR		0x07		/* sun: multibus archive */
#define	BSD_ISSC		0x08		/* sun: SCSI archive */
#define	BSD_ISXY		0x09		/* sun: Xylogics 472 */
#define	BSD_ISSYSGEN11	0x10		/* sun: SCSI Sysgen, QIC-11 only */
#define	BSD_ISSYSGEN	0x11		/* sun: SCSI Sysgen QIC-24/11 */
#define	BSD_ISDEFAULT	0x12		/* sun: SCSI default CCS */
#define	BSD_ISCCS3	0x13		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISMT02	0x14		/* sun: SCSI Emulex MT02 */
#define	BSD_ISCCS5	0x15		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS6	0x16		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS7	0x17		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS8	0x18		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS9	0x19		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS11	0x1a		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS12	0x1b		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS13	0x1c		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS14	0x1d		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS15	0x1e		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS16	0x1f		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS17	0x20		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS18	0x21		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS19	0x22		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS20	0x23		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS21	0x24		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS22	0x25		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS23	0x26		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS24	0x27		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS25	0x28		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS26	0x29		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS27	0x2a		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS28	0x2b		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS29	0x2c		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS30	0x2d		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS31	0x2e		/* sun: SCSI generic (unknown) CCS */
#define	BSD_ISCCS32	0x2f		/* sun: SCSI generic (unknown) CCS */
