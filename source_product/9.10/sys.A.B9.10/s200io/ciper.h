/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/ciper.h,v $
 * $Revision: 1.5.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:10:39 $
 */
/* @(#) $Revision: 1.5.84.3 $ */    
/* @(#) $Revision: 1.5.84.3 $ */    
#ifndef _MACHINE_CIPER_INCLUDED /* allows multiple inclusion */
#define _MACHINE_CIPER_INCLUDED

#ifdef __hp9000s300
#ifdef _KERNEL_BUILD
#include "../h/buf.h"
#include "../wsio/iobuf.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/buf.h>
#include <sys/iobuf.h>
#endif /* _KERNEL_BUILD */

/* posible values for code field of record header */

#define REC_RDY		0	/* reports the peripheral's readiness */
#define DEV_CLR		1	/* always executed by the peripheral */
#define CLR_RSP		1	/* indicates a device clear is complete */
#define DEV_STAT	2	/* send/receive device status report */
#define ENV_STAT	3	/* send/receive environment status report */
#define CIP_CONF	8	/* configures the peripheral */
#define CIP_START	9	/* start of job */
#define CIP_END		10	/* end of job */
#define JOB_STAT	11	/* send/receive job status report */
#define SIL_RUN		12	/* begins silent run recovery process */
#define CIP_WR		16	/* write */
#define CIP_RD		17	/* (enable) transfer to the host */

/* secondary commands */
/* write */
#define		CIP_SEC_RWT 	SCG_BASE+1	/* request to write */
#define		CIP_SEC_ERD 	SCG_BASE+2	/* enable read */
#define		CIP_SEC_SQC 	SCG_BASE+3	/* sequence complete */
#define		CIP_SEC_WRD 	SCG_BASE+4	/* write data */
/* read */
#define		CIP_SEC_SAB 	SCG_BASE+3	/* sequence abort */
#define		CIP_SEC_RDD 	SCG_BASE+4	/* read data */
#define		CIP_SEC_DSJ 	SCG_BASE+16	/* device specified jump */

/* misc. constants */
#define RAW     0x1	/* bit position in dev_t */
#define NOCR    0x2	/* bit position in dev_t */
#define CAP     0x4	/* bit position in dev_t */
#define NO_EJT  0x8	/* bit position in dev_t */

#define CIP_OPEN	0x1	/* regular open flag */
#define CIP_ERROR	0x2	/* error flag */
#define CIP_PWF		0x4	/* cannot tolerate powerfail flag */
#define CIP_RESET	0x8	/* cols set to 0--reset on next open */
#define CIP_LOCK	0x10	/* lock to do get buf */
#define CIP_ROPEN	0x20	/* raw open flag */

#define CIP_ON_LINE	0x0080	/* on line flag in device status report */
#define CIP_ST_PWF	3	/* power fail and data loss status bit */

#define	CIP_PSIZE	2048	/* size of output buffer */
#define CIP_INBUF	1024	/* largest buffer from user request */
#define CIPMAX		8	/* number of possible ciper printers */
#define	P_CIPER 	1
#define	CIP_CIPERSIZE	256

/* uses of spare fields in iob and buf structures. note double use of b_s1 */
#define	BP_RW		b_s1	/* used to initialize iob field */
#define DSJ_BYTE	b_s1	/* used to return DSJ byte */
#define	COUNT		b_s0	/* save count of bytes transferred */
#define	CIP_RW		io_s1	/* indicates read or write */
#define DSJ_SAVE	io_s2	/* save area for DSJ byte */

#define	SHORTTIME	HZ	/* for status: should be quick */
#define LONGTIME	HZ*25	/* for ppoll, etc. could be long */
#define	XFRTIME		HZ*10	/* transfer: must be less than LONGTIME */

struct packet_header {
	unsigned char header_len;
	unsigned char eom;
	unsigned short packet_num;
};

struct record_header {
	unsigned char header_len;
	unsigned char record_num;
	unsigned char code;
	unsigned hp:1;
	unsigned sob:1;
	unsigned eob:1;
	unsigned data_type:5;
};

struct block_header {
	unsigned char label_len;
	unsigned char reserved;
	unsigned long block_num;
};

struct cip_generic {
	struct packet_header ph;
	struct record_header rh;
};

struct dev_clear {
	struct packet_header ph;
	struct record_header rh;
	unsigned char data;
};

struct cip_config {
	struct packet_header ph;
	struct record_header rh;
	unsigned char data;
};

struct clear_resp {
	struct packet_header ph;
	struct record_header rh;
	unsigned char dc_rec_num;
	char prod_num[7];
	unsigned short rec_buf_size;
	unsigned short max_bytes;
};

#define SIZEOF_REC_RDY 9		/* the sizeof operator will return
					   ten as the size of this structure
					   but the printer will return 9 bytes
					*/
struct rec_rdy {
	struct packet_header ph;
	struct record_header rh;
	unsigned char num_recs;
};

struct dev_stat {
	struct packet_header ph;
	struct record_header rh;
	unsigned char dstat[6];
};

struct write_header {
	struct packet_header ph;
	struct record_header rh;
	struct block_header  bh;
};

union cip_recs {
	struct cip_generic	gen;
	struct dev_clear	device_clear;
	struct clear_resp	clear_response;
	struct rec_rdy		receive_ready;
	struct dev_stat		device_status;
};

/* storage local to driver */
struct CIP_bufs {
	struct buf	*r_getebuf;	/* pointer for brelse */
	struct buf	r_bpbuf;	/* buf for io request */
	struct iobuf    r_iobuf;	/* iobuf for hpib stuff */
	char	r_inbuf[CIP_INBUF];	/* raw input buffer from user land */
	char	r_outbuf[CIP_PSIZE];	/* output from canon routine */
	union cip_recs	r_cipbuf;	/* input from printer */
};
#endif /* __hp9000s300 */
#endif /* _MACHINE_CIPER_INCLUDED */
