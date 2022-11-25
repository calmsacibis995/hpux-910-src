/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/bootrom.h,v $
 * $Revision: 1.3.84.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 21:10:26 $
 */
/* @(#) $Revision: 1.3.84.3 $ */    
#ifndef _MACHINE_BOOTROM_INCLUDED /* allows multiple inclusion */
#define _MACHINE_BOOTROM_INCLUDED

#ifdef __hp9000s300
/* information needed to fish stuff out of the boot rom */

/* configuration address and masks for sysflags */
#define	ALPHA50		0x01
#define	BIGGRAPH	0x02
#define	HIGHLIGHTS	0x04
#define	NOKBD		0x08
#define	CRTCONFREG	0x10
#define	NOHPIB		0x20
#define	BIT_MAP_PRESENT	0xc0

/* configuration address and masks for sysflags2 */
#define	PROM		0x01

/* interesting fields in the id prom */
#define PROMLOC		0x5f0001
#define PROMPROD	(PROMLOC+0x1c)
#define PRODSIZE	7		/* with trailing blanks if < 7 */
#define PROMSER		(PROMLOC+0x6)
#define SERSIZE		10

/* structure for MSUS from the bootrom */

struct msus {
	unsigned dir_format  :3; /* 0 = LIF Sector Oriented */
	unsigned device_type :5; /* 7-13=amigo, 16=256byte sect CS80, 17=CS80 */
	unsigned vol         :4; 
	unsigned unit        :4; /* linus = 1 */
	unsigned sc          :8; /* 0 - 31 where 7 = internal HPIB */
	unsigned ba          :8; /* HPIB bussaddress */
};


struct crtid {
	unsigned	selfinit : 1;
	unsigned	reserved : 1;
	unsigned	char_mapping : 1;
	unsigned	sub_graphics_top : 2;
	unsigned	highlite : 1;
	unsigned	graph : 1;
	unsigned	alpha : 1;
	unsigned	crt_number : 4;
	unsigned	hertz : 1;
	unsigned	model_number : 3;
};

struct sysflag {
	unsigned	reserved : 1;
	unsigned	highres  : 1;
	unsigned	hpib : 1;
	unsigned	crt_config : 1;
	unsigned	kbd : 1;
	unsigned	high : 1;
	unsigned	big_graph : 1;
	unsigned	alpha50 : 1;
};

struct sysflag2 {
	unsigned	reserved : 5;
	unsigned	timer    : 1;
	unsigned	processor: 1;
	unsigned	id_prom  : 1;
};

/* miscellaneous bootrom variables */
extern char sysflags, sysflags2, ndrives;
extern struct msus msus;
#endif /* __hp9000s300 */
#endif /* _MACHINE_BOOTROM_INCLUDED */
