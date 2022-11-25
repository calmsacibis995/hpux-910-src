/* @(#) $Revision: 64.1 $ */       
/* vohldr.h: volume header for HP-UX "LIF" volumes (so the media format
   field can be recongnized) */
#ifndef _VOLHDR_INCLUDED /* allows multiple inclusion */
#define _VOLHDR_INCLUDED
#ifdef __hp9000s300

struct HPUX_vol_type {
	short	HPUXid;			/* must be HPUXID */
	char	HPUXreserved1[2];
	int	HPUXowner;		/* must be OWNERID */
	int	HPUXexecution_addr;
	int	HPUXboot_start_sector;
	int	HPUXboot_byte_count;
	char	HPUXfilename[16];
};

/* load header for boot rom */
struct load {
	int address;
	int count;
};

#define HPUXID	0x3000
#define OWNERID	0xFFFFE942
#endif /* __hp9000s300 */
#endif /* _VOLHDR_INCLUDED */
