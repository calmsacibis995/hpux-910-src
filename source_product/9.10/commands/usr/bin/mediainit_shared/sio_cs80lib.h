/* HPUX_ID: @(#) $Revision: 66.2 $ */

typedef struct {
    short		unit;		/* -1=none */
    short		volume;		/* -1=none */
    short		address_mode;	/* -1=none, 0=block, 1=3 vector */
    unsigned char	address[6];
    short		disp_flag;	/* -1=none, 1=new displacement  */
    unsigned char	disp[6];	/* RTE disc lib. used 4 bytes */
    short		len_flag;	/* -1=none */
    unsigned char	len[4];
    short		burst;		/* -1=none, x=size - bit15: EOI tag */
    short		rps_time1;	/* -1=none */
    short		rps_time2;
    short		retry_time;	/* -1=none, x=retry time */
    short		status_flag;	/* -1=none */
    unsigned char	status_mask[8];
    short		release;	/* -1=none, bit0: 'Z', bit1 'T' */
    short		rtn_addr_mode;	/* -1=none, 0=block, 1=3 vector */
    short		options;	/* -1=none, x=options */
} comp_type;

#define unpk32(a,b,c) for (ii=0;ii<4;ii++) b[c+ii] = (a>>((3-ii)*8)) & 0xff;

/* CS80 commands */
#define LOCATENREAD     0x00	/* locate and read */
#define LOCATENWRITE    0x02	/* locate and write */
#define LOCATENVERIFY   0x04	/* locate and verify */
#define SPAREBLOCK	0x06	/* spare block */
#define COPYDATA	0x08	/* copy data */
#define COLDLOAD	0x0a	/* cold load read */
#define EXT_DESCRIBE	0x0c	/* extended describe */
#define REQSTATUS	0x0d	/* request status */
#define RELEASE         0x0e    /* release device */
#define RELEASEDENIED	0x0f    /* release denied */
#define SETADDRESS	0x10	/* set address */
#define SETDISPLACEMENT	0x12	/* set address */
#define SETLENGTH	0x18	/* set length */
#define SETUNIT		0x20	/* set unit 0 */
#define INITUTILITY	0x30	/* initiate utility */
#define INITDIAGNOSTIC	0x33	/* initiate diagnostic */
#define NOOP     	0x34	/* no op */
#define DESCRIBE 	0x35	/* describe */
#define SETRELEASE	0x3b	/* set release */
#define INITMEDIA	0x37	/* initialize media */
#define SETOPTION	0x38	/* set options */
#define SETRPS		0x39	/* set rps */
#define SETRETRYTIME	0x3a	/* set retry time */
#define SETBURST	0x3c	/* set burst size */
#define SETMASK		0x3e	/* set status mask */
#define SETVOLUME	0x40	/* set volume 0 */
#define SETRTNADDRMODE	0x48	/* set return addressing mode */
#define WRITEMARK	0x49	/* write file mark */
#define UNLOAD          0x4a    /* unload */
#define READLOOPBACK    0x02    /* read loopback */
#define WRITELOOPBACK	0x03    /* write loopback */
#define SELDEVICECLEAR	0x04    /* selected device clear - parity??? */
#define CHANNELCLEAR    0x08    /* channel independent clear */
#define CANCEL		0x09    /* cancel */

/* Eagle specific CS80 commands */
#define RHRESET		0x10	/* remote host reset */
#define TRANS_STAT	0x40	/* read transparent status */
#define DEVICE_LOCK	0x63	/* lock device */
#define DEVICE_UNLOCK	0x6b	/* unlock device */
#define NRH_TO		0x67	/* non responding host timeout */

/* CS80 Transaction Types */

#define CS80_WR		1
#define CS80_WWR	2
#define CS80_WRR	3
#define TRANS_WR	5
#define TRANS_WWR	6
#define TRANS_WRR	7
#define DEV_E_LB	8
#define CIO_LB		9
#define INT_LB	 	10
#define WKVC_LB	 	11

#define NON_DESTRUCTIVE 1
#define DESTRUCTIVE     2

/* Eagle loopback types */
#define INT_LOOPBACK	1
#define WKVC_LOOPBACK	2
#define DEV_LOOPBACK	3
#define EXTRN_LOOPBACK	4
#define CIO_LOOPBACK	5
