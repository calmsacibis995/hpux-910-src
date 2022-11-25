/*
 * @(#)eeprom.h: $Revision: 1.4.83.4 $ $Date: 93/12/15 12:01:09 $
 * $Locker:  $
 */

#ifndef _SYS_EEPROM_INCLUDED
#define _SYS_EEPROM_INCLUDED

#ifdef	_KERNEL_BUILD
#include "../h/ioctl.h"
#else	/* ! _KERNEL_BUILD */
#include <sys/ioctl.h>
#endif	/* _KERNEL_BUILD */

#define EEPROM_BASE_ADDR_68K 	0x408000
#define EEPROM_REG_BASE_68K 	0x400000
#define MAX_EISA_SLOTS_68K      8 
#define ESB_SLOT_BASE_68K       0            /* this value ignored, value calculated at runtime */
#define MAX_SLOT_ZERO_SIZE      70           /* we can only store this many bytes */

#define EEPROM_BASE_ADDR_PA     0xF0810400
#define EEPROM_BOOT_ADDR_PA     0xF0811E00   /* PDC hardcodes this value */
#define EEPROM_ENABLE_ADDR      0xF0818000
#define EEPROM_REG_BASE_PA      0xF0818000 
#define ESB_SLOT_BASE_PA        0x1b62       /* PA is hardcoded */
#define MAX_EISA_SLOTS_PA       8

#ifdef __hp9000s300
#define EEPROM_BASE_ADDR	EEPROM_BASE_ADDR_68K
#define EEPROM_REG_BASE		EEPROM_REG_BASE_68K
#define MAX_EISA_SLOTS          MAX_EISA_SLOTS_68K
#define ESB_SLOT_BASE           ESB_SLOT_BASE_68K
#else
#define EEPROM_BASE_ADDR	EEPROM_BASE_ADDR_PA
#define EEPROM_REG_BASE		EEPROM_REG_BASE_PA
#define EEPROM_BOOT_OFFSET      (EEPROM_BOOT_ADDR_PA-EEPROM_BASE_ADDR_PA)
#define ESB_SLOT_BASE           ESB_SLOT_BASE_PA
#define MAX_EISA_SLOTS          MAX_EISA_SLOTS_PA
#endif

#define EEPROM_MAJ 	   64
#define MAX_EEPROM_WRITES  10000
#define EEPROM_ENABLE      0xff
#define EEPROM_DISABLE     0

#ifdef __hp9000s300
struct eeprom_section_header {
	u_char section_id;
	u_char version;
	u_short size;
	u_short checksum;
	u_short unused_bytes;
	short write_count;
}; /* 10 bytes */

#define CFG_ID 	20
#define EISA_NO_ID	((u_char)(0xff))

#define EEPROM_ID	0
#define EEPROM_SEC_ID	8
#define EEPROM_WRITE_ENABLE_1	0x53
#define EEPROM_WRITE_ENABLE_2	0xcc
#define EEPROM_ENABLED	0x10

#define EEPROM_ID_OFFSET	1
#define EEPROM_CONTROL_OFFSET	3
#define EEPROM_SEC_ID_OFFSET	5
#endif

/* 
 * ioctls to eeprom driver 
 */
#define	CLEAR_EEPROM	          _IOW('e', 1, int)
#define READ_SLOT_INFO	          _IOWR('e', 2, struct eeprom_slot_data)
#define READ_FUNCTION_PREP        _IOW('e', 3, struct func_slot)
#define WRITE_SLOT_PREP	          _IOW('e', 4, int)
#define READ_SLOT_ID	          _IOWR('e', 5, int)
#define INIT_NEW_CARDS            _IO('e', 6)
#define CHECK_CHKSUM              _IOR('e', 7, int)
#define WAS_BOARD_INITED          _IOW('e', 8, int)
#define CHG_NUM_SLOTS             _IOW('e', 9, int)
#define GET_NUM_SLOTS             _IOR('e',10, int)

#ifdef _KERNEL
#define CREATE_BOOT               _IOW('e',11, int)
#define READ_BOOT                 _IOR('e',12, struct eisa_boot_info)
#define READ_BOOT_FUNC            _IO('e',13)
#define BREAK_CHKSUM              _IO('e',14)
#define FIX_CHKSUM                _IO('e',15)
#endif

struct func_slot {
	int function;
	int slot;
};

struct eeprom_gen_info {
	u_int write_count;
	u_char flags;	/* valid bit, booted default, ?? */
	u_char major_cfg_rev;
	u_char minor_cfg_rev;
	u_char number_of_slots;
	u_short checksum;
	u_char spares[10];
}; /* 20 bytes */

/* flags bit definitions */
#define SECTION_NOT_INITIALIZED	0x80		/* eeprom initial value */
#define	SLOT_IS_EMPTY		0xFFFFFFFF	/* slot_id for a slot with no config data */
#define	SLOT_IS_ISA		0x1		/* minor_cfg_ext_rev bit meaning ISA card in slot */

/*
 * This is the  definition  of the per  slot  structure  of  information
 * stored  following the general  information area of the  configuration
 * portion of the  eeprom.  There is one of these fixed size  structures
 * for each slot of the EISA  bus(ses)  in the  system.  This  holds the
 * information  needed for the  read_eisa_slot_data  driver call and for
 * locating  the per slot area  defining  the  card's  functions  in the
 * eeprom.  Spare  fields  have been  added to allow for future  uses of
 * this area without invalidating the format of the eeprom.
 */

struct eeprom_per_slot_info {
	u_int slot_id;
	u_int cfg_data_offset;
	u_int write_count;
	u_short checksum;
	u_short number_of_functions;
	u_short cfg_data_size;
	u_char slot_info;
	u_char slot_features;
	u_char minor_cfg_ext_rev;
	u_char major_cfg_ext_rev;
	u_char function_info;
	u_char flags;
	u_char spares[24];
};  /* 48 bytes */


/*
 * This is the structure for handling eisa boot information 
 */

struct eisa_boot_info {
	int  slot_num;         /* which slot was the boot device found */
	struct eeprom_per_slot_info b_slot_info; /* info about the slot */
};


/*
 * These are the declarations associated with the slot
 * data structure returned by read_eisa_slot_data:
 */

/* slot_info fields */
#define DUPLICATE_ID		0xf
#define SLOT_TYPE_MASK		0x30
#define EXPANSION_SLOT		0x0
#define EMBEDDED_DEV		0x10
#define VIRTUAL_DEV		0x20
#define NON_READABLE_PID 	0x40
#define DUPLICATES		0x80

/* function_info fields */
#define HAS_FUNCTION_ENTRY	0x1
#define HAS_MEMORY_ENTRY	0x2
#define HAS_IRQ_ENTRY		0x4
#define HAS_DMA_ENTRY		0x8
#define HAS_PORT_ENTRY		0x10
#define HAS_PORT_INIT		0x20
	
struct eeprom_slot_data {
	u_char slot_info;	/* see AL, p359 */
	u_char major_cfg_rev;	/* major revision # of cfg util */
	u_char minor_cfg_rev;	/* minor revision # of cfg util */
	u_char flags;
	u_short checksum;	/* checksum of cfg file */
	u_char number_of_functions; /* # of functions in slot */
	u_char function_info;	/* see DL, p359 */
	u_int slot_id;		/* EISA ID of board in slot */
};
		

/*
 * These are the declarations associated with the function data structure
 * returned by read_eisa_function_data:
 */

/* slot_features fields */
#define EISA_ENABLE	  0x1  	/* card has enable bit feature */
#define EISA_IOCHKERR	  0x2  	/* card has IOCHK feature of EISA */
#define CFG_NOT_COMPLETE  0x80

/* function_info fields - bits 0x2-0x20 same as for slot_info */
#define FUNCTION_DISABLED	0x80
#define CFG_FREEFORM		0x40
#define HAS_TYPE_ENTRY		0x1

/* Spec states freeform data is 205 bytes max */
#define MAX_FREEFORM_SIZE 205

/* memory configuration byte fields */
#define MEM_RAM		 0x1	/* bit = 1, RAM; bit = 0, ROM */
#define MEM_CACHED	 0x2
#define MEM_TYPE_MASK	 0x18
#define MEM_SYS		 0x0	/* system memory, base or extended */
#define MEM_EXP		 0x8	/* expanded memory */
#define MEM_VIR		 0x10	/* virtual memory */
#define MEM_OTH		 0x18	/* other memory type */
#define	MEM_SHARED	 0x20
#define MEM_MORE_ENTRIES 0x80	/* more memory configs defined in array */

/* memory data size byte fields */
#define MEM_BYTE	 0x0  	/* !MEM_WORD & !MEM_DWORD */
#define MEM_WORD	 0x1
#define MEM_DWORD	 0x2
#define MEM_DECODE_20	 0x0  	/* !MEM_DECODE_24 & !MEM_DECODE_32 */
#define MEM_DECODE_24	 0x4
#define MEM_DECODE_32	 0x8

/* interrupt_info fields */
#define INT_IRQ_MASK 	 0xf	/* which interrupt, 0 - f */
#define INT_LEVEL_TRIG	 0x20   /* bit = 1, level; bit = 0, edge */
#define INT_SHARED	 0x40	/* int can be shared */
#define INT_MORE_ENTRIES 0x80   /* more interrupts defined in array */

/* dma_info fields */
#define DMA_CHANNEL_MASK 0x7	/* which channel, 0 - 7 */
#define DMA_SHARED	 0x40
#define DMA_MORE_ENTRIES 0x80	/* more dma channels defined in array */

/* more_dma_info fields */
#define DMA_SIZE_MASK	0xc 
#define DMA_SIZE_BYTE	0x0	/* !DMA_SIZE_WORD & !DMA_SIZE_DWORD */
#define DMA_SIZE_WORD	0x4	/* 16-bit transfers */
#define DMA_SIZE_DWORD	0x8	/* 32-bit transfers */
#define DMA_TIMING_MASK 0x30	/* mask for dma timing bits */
#define DMA_TIMING_ISA	0x0	/* ISA timing, default */
#define DMA_TIMING_A	0x10	/* Type A timing */
#define DMA_TIMING_B	0x20	/* Type B timing */
#define DMA_TIMING_C 	0x30	/* Burst (type C) timing */
#define DMA_TIMING_BURST DMA_TIMING_C
	
/* port info fields */
#define PORT_NUMBER_MASK  0x1f	/* number of ports - 1 */
#define PORT_SHARED	  0x40	
#define PORT_MORE_ENTRIES 0x80	/* more I/O ports defined in array */

/* defines for I/O port initialization fields */
#define INIT_ACCESS_BYTE  0x0	/* !INIT_ACCESS_WORD & !INIT_ACCESS_DWORD */
#define INIT_ACCESS_WORD  0x1	/* word access */
#define INIT_ACCESS_DWORD 0x2	/* dword access */
#define INIT_MASK	  0x4
#define INIT_MORE_ENTRIES 0x80	/* more I/O port inits defined */

#define MAX_SEL_ENTRIES		26
#define MAX_TYPE_ENTRIES	80
#define MAX_INT_ENTRIES		7
#define MAX_DMA_ENTRIES		4
#define MAX_MEM_ENTRIES		9
#define MAX_PORT_ENTRIES	20

struct eeprom_function_data {
	u_int  slot_id;			/* EISA id of board in slot */
	u_char slot_info;		/* same bits as in slot_data struct */
	u_char slot_features;		/* EISA features supported by this slot */
	u_char minor_cfg_ext_rev;	/* minor rev # of CFG file extension */
	u_char major_cfg_ext_rev;	/* major rev # of CFG file extension */
	u_char selection[26];		/* selections - used by CFG util only */
	u_char function_info;		/* info about function data to follow */
	u_char type[80];		/* type/subtype of function, zero filled */
	u_char mem_cfg[9][7];
	u_char intr_cfg[7][2];
	u_char dma_cfg[4][2];
	u_char port_cfg[20][3];
	u_char init_data[60];
};
#endif /* SYS_EEPROM_INCLUDED */

