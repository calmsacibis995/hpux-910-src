/*
 *  Configuration information
 */


#define	MAXUSERS	8
#define	TIMEZONE	420
#define	DST	1
#define	NPROC	(20+8*MAXUSERS+(NGCSP))
#define	NUM_CNODES	((5*SERVER_NODE)+DSKLESS_NODE)
#define	DSKLESS_NODE	1
#define	SERVER_NODE	1
#define	NINODE	((NPROC+16+MAXUSERS)+32+(2*NPTY)+SERVER_NODE*18*NUM_CNODES)
#define	NFILE	(16*(NPROC+16+MAXUSERS)/10+32+(2*NPTY))
#define	FILE_PAD	10
#define	MAXFILES	60
#define	MAXFILES_LIM	1024
#define	DBC_CUSHION	512
#define	DBC_MIN_PCT	5
#define	DBC_MAX_PCT	50
#define	NBUF	0
#define	BUFPAGES	0
#define	FS_ASYNC	0
#define	CREATE_FASTLINKS	0
#define	DOS_MEM_BYTE	0
#define	NCALLOUT	(16+NPROC+USING_ARRAY_SIZE+SERVING_ARRAY_SIZE)
#define	UNLOCKABLE_MEM	102400
#define	NFLOCKS	200
#define	NPTY	82
#define	MAXUPRC	50
#define	MAXDSIZ	0x01000000
#define	MAXSSIZ	0x00200000
#define	MAXTSIZ	0x01000000
#define	PARITY_OPTION	2
#define	REBOOT_OPTION	1
#define	TIMESLICE	0
#define	ACCTSUSPEND	2
#define	ACCTRESUME	4
#define	NDILBUFFERS	30
#define	USING_ARRAY_SIZE	(NPROC)
#define	SERVING_ARRAY_SIZE	(SERVER_NODE*NUM_CNODES*MAXUSERS+2*MAXUSERS)
#define	DSKLESS_FSBUFS	(SERVING_ARRAY_SIZE)
#define	SELFTEST_PERIOD	120
#define	INDIRECT_PTES	1
int	indirect_ptes = INDIRECT_PTES;
#define	AES_OVERRIDE	0
#define	CHECK_ALIVE_PERIOD	4
#define	RETRY_ALIVE_PERIOD	21
#define	MAXSWAPCHUNKS	512
#define	MINSWAPCHUNKS	4
#define	NSWAPDEV	10
#define	NSWAPFS	10
#define	NUM_LAN_CARDS	2
#define	NETISR_PRIORITY	-1
#define	NETMEMMAX	0
#define	NGCSP	(8*NUM_CNODES)
#define	NNI	1
#define	SCROLL_LINES	100
#define	NUM_PDN0	-1
#define	MESG	1
#define	MSGMAP	(MSGTQL+2)
#define	MSGMAX	8192
#define	MSGMNB	16384
#define	MSGMNI	50
#define	MSGSSZ	1
#define	MSGTQL	40
#define	MSGSEG	16384
#define	SEMA	1
#define	SEMMAP	(SEMMNI+2)
#define	SEMMNI	64
#define	SEMMNS	128
#define	SEMMNU	30
#define	SEMUME	10
#define	SEMVMX	32767
#define	SEMAEM	16384
#define	SHMEM	1
#define	SHMMAX	0x00600000
#define	SHMMIN	1
#define	SHMMNI	30
#define	SHMSEG	10
#define	FPA	1
#define	MC68040	1
#define	PRB	1
#define	SWAPMEM_ON	0
#define	SWCHUNK	2048
#define	DDBBOOT	0
#define	NSTREVENT	50
#define	STRMSGSZ	4096
#define	STRCTLSZ	1024
#define	NSTRPUSH	16

#define	UIPC
#define	UIPC
#define	UIPC
#define	NIPC
#define	INET
#define	INET
#define	INET
#define	NI
#define	DSKLESS
#define	LAN01
#define	VME2

#include	"../h/param.h"
#include	"../h/systm.h"
#include	"../h/tty.h"
#include	"../h/space.h"
#include	"../h/opt.h"
#include	"../h/conf.h"

#define	ieee802_open	lan_open
#define	ieee802_close	lan_close
#define	ieee802_read	lan_read
#define	ieee802_write	lan_write
#define	ieee802_link	lan_link
#define	ieee802_select	lan_select
#define	ethernet_open	lan_open
#define	ethernet_close	lan_close
#define	ethernet_read	lan_read
#define	ethernet_write	lan_write
#define	ethernet_link	lan_link
#define	ethernet_select	lan_select
#define	hpib_link	gpio_link
#define	lla_link	lan_link
#define	lan01_link	lan_link

extern nodev(), nulldev();
extern seltrue(), notty();

extern cs80_open(), cs80_close(), cs80_read(), cs80_write(), cs80_ioctl(), cs80_size(), cs80_dump(), cs80_link(), cs80_strategy();
extern amigo_open(), amigo_close(), amigo_read(), amigo_write(), amigo_ioctl(), amigo_size(), amigo_link(), amigo_strategy();
extern swap_strategy();
extern swap1_strategy();
extern rdu_read(), rdu_write(), rdu_ioctl(), rdu_select(), rdu_strategy();
extern scsi_open(), scsi_close(), scsi_read(), scsi_write(), scsi_ioctl(), scsi_size(), scsi_dump(), scsi_link(), scsi_strategy();
extern cons_open(), cons_close(), cons_read(), cons_write(), cons_ioctl(), cons_select();
extern tty_open(), tty_close(), tty_read(), tty_write(), tty_ioctl(), tty_select();
extern sy_open(), sy_close(), sy_read(), sy_write(), sy_ioctl(), sy_select();
extern mm_read(), mm_write();
extern tp_open(), tp_close(), tp_read(), tp_write(), tp_ioctl();
extern lp_open(), lp_close(), lp_write(), lp_ioctl();
extern swap_read(), swap_write();
extern stp_open(), stp_close(), stp_read(), stp_write(), stp_ioctl();
extern iomap_open(), iomap_close(), iomap_read(), iomap_write(), iomap_ioctl();
extern graphics_open(), graphics_close(), graphics_ioctl(), graphics_link();
extern srm629_open(), srm629_close(), srm629_read(), srm629_write(), srm629_ioctl(), srm629_select(), srm629_link();
extern rje_open(), rje_close(), rje_read(), rje_write(), rje_ioctl(), rje_link();
extern ptym_open(), ptym_close(), ptym_read(), ptym_write(), ptym_ioctl(), ptym_select();
extern ptys_open(), ptys_close(), ptys_read(), ptys_write(), ptys_ioctl(), ptys_select(), ptys_link();
extern lla_open(), lla_link();
extern lla_open();
extern hpib_open(), hpib_close(), hpib_read(), hpib_write(), hpib_ioctl(), hpib_link();
extern hpib_open(), hpib_close(), hpib_read(), hpib_write(), hpib_ioctl(), hpib_link();
extern r8042_open(), r8042_close(), r8042_ioctl();
extern hil_open(), hil_close(), hil_read(), hil_ioctl(), hil_select(), hil_link();
extern nimitz_open(), nimitz_close(), nimitz_read(), nimitz_select();
extern vme_open(), vme_close(), vme_read(), vme_write(), vme_ioctl(), vme_link();
extern meas_sys_open(), meas_sys_close(), meas_sys_read(), meas_sys_write(), meas_sys_ioctl();
extern stealth_open(), stealth_close(), stealth_ioctl(), stealth_link();
extern netdiag1_open(), netdiag1_close(), netdiag1_read(), netdiag1_write(), netdiag1_ioctl();
extern ni_open(), ni_close(), ni_read(), ni_write(), ni_ioctl(), ni_select(), ni_link();
extern nm_open(), nm_close(), nm_read(), nm_ioctl(), nm_select();
extern eeprom_open(), eeprom_close(), eeprom_read(), eeprom_write(), eeprom_ioctl(), eeprom_link();
extern dconfig_open(), dconfig_close(), dconfig_read(), dconfig_ioctl();
extern strace_open(), strace_close(), strace_read(), strace_ioctl(), strace_select(), strace_link();

extern scsi_if_link();
extern parallel_link();
extern dskless_link();
extern uipc_link();
extern inet_link();
extern nfsc_link();
extern nipc_link();
extern cdfs_link();
extern ti9914_link();
extern hshpib_link();
extern sio626_link();
extern sio628_link();
extern sio642_link();
extern ap_sio_link();
extern ite200_link();


struct bdevsw bdevsw[] = {
/* 0*/	{cs80_open, cs80_close, cs80_strategy, cs80_dump, cs80_size, C_ALLCLOSES, nodev},
/* 1*/	{nodev, nodev, nodev, nodev, nodev, 0, nodev},
/* 2*/	{amigo_open, amigo_close, amigo_strategy, nodev, amigo_size, C_ALLCLOSES, nodev},
/* 3*/	{nodev, nodev, swap_strategy, nodev, 0, 0, nodev},
/* 4*/	{nodev, nodev, nodev, nodev, nodev, 0, nodev},
/* 5*/	{nodev, nodev, swap1_strategy, nodev, 0, 0, nodev},
/* 6*/	{nodev, nodev, rdu_strategy, nodev, 0, 0, nodev},
/* 7*/	{scsi_open, scsi_close, scsi_strategy, scsi_dump, scsi_size, C_ALLCLOSES, nodev},
};

struct cdevsw cdevsw[] = {
/* 0*/	{cons_open, cons_close, cons_read, cons_write, cons_ioctl, cons_select, C_ALLCLOSES},
/* 1*/	{tty_open, tty_close, tty_read, tty_write, tty_ioctl, tty_select, C_ALLCLOSES},
/* 2*/	{sy_open, sy_close, sy_read, sy_write, sy_ioctl, sy_select, C_ALLCLOSES},
/* 3*/	{nulldev,  nulldev, mm_read, mm_write, notty, seltrue, 0},
/* 4*/	{cs80_open, cs80_close, cs80_read, cs80_write, cs80_ioctl, seltrue, C_ALLCLOSES},
/* 5*/	{tp_open, tp_close, tp_read, tp_write, tp_ioctl, seltrue, 0},
/* 6*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/* 7*/	{lp_open, lp_close, nodev, lp_write, lp_ioctl, seltrue, 0},
/* 8*/	{nulldev,  nulldev, swap_read, swap_write, notty, nodev, 0},
/* 9*/	{stp_open, stp_close, stp_read, stp_write, stp_ioctl, seltrue, 0},
/*10*/	{iomap_open, iomap_close, iomap_read, iomap_write, iomap_ioctl, nodev, C_ALLCLOSES},
/*11*/	{amigo_open, amigo_close, amigo_read, amigo_write, amigo_ioctl, seltrue, C_ALLCLOSES},
/*12*/	{graphics_open, graphics_close, nodev, nodev, graphics_ioctl, nodev, C_ALLCLOSES},
/*13*/	{srm629_open, srm629_close, srm629_read, srm629_write, srm629_ioctl, srm629_select, C_ALLCLOSES},
/*14*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*15*/	{rje_open, rje_close, rje_read, rje_write, rje_ioctl, seltrue, 0},
/*16*/	{ptym_open, ptym_close, ptym_read, ptym_write, ptym_ioctl, ptym_select, 0},
/*17*/	{ptys_open, ptys_close, ptys_read, ptys_write, ptys_ioctl, ptys_select, C_ALLCLOSES},
/*18*/	{lla_open, nulldev, nodev, nodev, notty, nodev, C_ALLCLOSES},
/*19*/	{lla_open, nulldev, nodev, nodev, notty, nodev, C_ALLCLOSES},
/*20*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*21*/	{hpib_open, hpib_close, hpib_read, hpib_write, hpib_ioctl, seltrue, C_ALLCLOSES},
/*22*/	{hpib_open, hpib_close, hpib_read, hpib_write, hpib_ioctl, seltrue, C_ALLCLOSES},
/*23*/	{r8042_open, r8042_close, nodev, nodev, r8042_ioctl, nodev, 0},
/*24*/	{hil_open, hil_close, hil_read, nodev, hil_ioctl, hil_select, 0},
/*25*/	{nimitz_open, nimitz_close, nimitz_read, nodev, notty, nimitz_select, 0},
/*26*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*27*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*28*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*29*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*30*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*31*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*32*/	{vme_open, vme_close, vme_read, vme_write, vme_ioctl, nodev, 0},
/*33*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*34*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*35*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*36*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*37*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*38*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*39*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*40*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*41*/	{meas_sys_open, meas_sys_close, meas_sys_read, meas_sys_write, meas_sys_ioctl, nodev, 0},
/*42*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*43*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*44*/	{stealth_open, stealth_close, nodev, nodev, stealth_ioctl, nodev, 0},
/*45*/	{nulldev,  nulldev, rdu_read, rdu_write, rdu_ioctl, rdu_select, 0},
/*46*/	{netdiag1_open, netdiag1_close, netdiag1_read, netdiag1_write, netdiag1_ioctl, seltrue, 0},
/*47*/	{scsi_open, scsi_close, scsi_read, scsi_write, scsi_ioctl, seltrue, C_ALLCLOSES},
/*48*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*49*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*50*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*51*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*52*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*53*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*54*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*55*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*56*/	{ni_open, ni_close, ni_read, ni_write, ni_ioctl, ni_select, 0},
/*57*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*58*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*59*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*60*/	{nm_open, nm_close, nm_read, nodev, nm_ioctl, nm_select, 0},
/*61*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*62*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*63*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*64*/	{eeprom_open, eeprom_close, eeprom_read, eeprom_write, eeprom_ioctl, nodev, 0},
/*65*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*66*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*67*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*68*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*69*/	{dconfig_open, dconfig_close, dconfig_read, nodev, dconfig_ioctl, nodev, C_ALLCLOSES},
/*70*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*71*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*72*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*73*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*74*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*75*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*76*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*77*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*78*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*79*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*80*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*81*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*82*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*83*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*84*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*85*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*86*/	{nodev, nodev, nodev, nodev, nodev, nodev, 0},
/*87*/	{strace_open, strace_close, strace_read, nodev, strace_ioctl, strace_select, C_ALLCLOSES},
};

int	nblkdev = sizeof (bdevsw) / sizeof (bdevsw[0]);
int	nchrdev = sizeof (cdevsw) / sizeof (cdevsw[0]);

dev_t	rootdev = makedev(-1,0xFFFFFF);

/* The following three variables are dependent upon bdevsw and cdevsw. If
   either changes then these variables must be checked for correctness */

dev_t	swapdev1 = makedev(5, 0x000000);
int	brmtdev = 6;
int	crmtdev = 45;

struct swdevt swdevt[] = {
	{ SWDEF, 0, -1, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
	{ NODEV, 0, 0, 0 },
};

dev_t	dumpdev = makedev(-1,0xFFFFFF);

int	(*driver_link[])() = 
{
	cs80_link,
	amigo_link,
	scsi_link,
	graphics_link,
	srm629_link,
	rje_link,
	ptys_link,
	lla_link,
	hpib_link,
	hpib_link,
	hil_link,
	vme_link,
	stealth_link,
	ni_link,
	eeprom_link,
	strace_link,
	scsi_if_link,
	parallel_link,
	dskless_link,
	uipc_link,
	inet_link,
	nfsc_link,
	nipc_link,
	cdfs_link,
	ti9914_link,
	hshpib_link,
	sio626_link,
	sio628_link,
	sio642_link,
	ap_sio_link,
	ite200_link,
	(int (*)())0
};
char dfile_data[] = "\
cs80\n\
scsi\n\
amigo\n\
tape\n\
stape\n\
printer\n\
srm\n\
ptymas\n\
ptyslv\n\
hpib\n\
gpio\n\
rje\n\
strace\n\
vme\n\
vme2\n\
parallel\n\
eisa\n\
dskless\n\
nfs\n\
uipc\n\
nipc\n\
inet\n\
netman\n\
cdfs\n\
meas_sys\n\
lla\n\
lan01\n\
ni\n\
netdiag1\n\
98624\n\
98625\n\
98626\n\
98628\n\
98642\n\
98658\n\
apci\n\
98265\n\
dskless_node 1\n\
server_node 1\n\
";
