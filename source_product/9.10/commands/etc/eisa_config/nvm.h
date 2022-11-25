/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/******************************************************************************
*   (C) Copyright COMPAQ Computer Corporation 1985, 1989
*******************************************************************************
*
*                            inc/nvm.h
*
*   All definitions related to reading/writing nvm. This includes both sci
*   and eeprom. (There is still some sci-specific stuff in sci.h.)
*  
******************************************************************************/


/* maximum data structure sizes */
#define NVM_MAX_SELECTIONS	26
#define NVM_MAX_TYPE		80
#define NVM_MAX_MEMORY		9
#define NVM_MAX_IRQ		7
#define NVM_MAX_DMA		4
#define NVM_MAX_PORT		20
#define NVM_MAX_INIT		60
#define NVM_MAX_DATA		204


/* These are the sizes of the structures that get copied down when we
   are saving a board. We cannot use sizeof() for these guys because
   structures are always at least 2-byte aligned, at least on the 300's.
   The structures are always made up of chars, so there are no alignment
   problems within the structures.
*/
#define NVM_MEMORY_SIZE		7
#define NVM_IRQ_SIZE		2
#define NVM_DMA_SIZE		2
#define NVM_PORT_SIZE		3


/* duplicate id information byte format */
/*         2 bytes long                 */
typedef struct {
    unsigned char	dups	:1;	/* duplicates exist */
    unsigned char	noreadid:1;	/* readable id (0=readable!!)  */
    unsigned char	type	:2;	/* slot type */
    unsigned char	dup_id	:4;	/* duplicate id number */       
    unsigned char	partial :1;	/* configuration incomplete */
    unsigned char	resvd	:5;	/* reserved */
    unsigned char	iocheck :1;	/* eisa io check supported */
    unsigned char	disable :1;	/* board disable is supported */
} nvm_dupid;


/* function information byte	*/
/*         1 byte long          */
typedef struct {
    unsigned char	disable :1;	/* function is disabled 	*/
    unsigned char	data	:1;	/* free form data		*/
    unsigned char	init	:1;	/* port initialization present	*/
    unsigned char	port	:1;	/* port configuration present	*/
    unsigned char	dma	:1;	/* dma configuration present	*/
    unsigned char	irq	:1;	/* irq configuration present	*/
    unsigned char	memory	:1;	/* memory configuration present */
    unsigned char	type	:1;	/* type string present		*/
} nvm_fib;


/* returned information from a "Read Slot" */
/*         12 bytes long                   */
typedef struct {
    unsigned char	boardid[4];	/* compressed board id */
    unsigned short	revision;	/* utility version */           
    unsigned char	functions;	/* number of functions */
    nvm_fib 		fib;		/* function information byte*/
    unsigned short	checksum;	/* cfg checksum */              
    nvm_dupid		dupid;		/* duplicate id information */
} nvm_slotinfo;


/* specification of a memory element */
/*          7 bytes long             */
typedef struct {
    unsigned char	more	:1;	/* more entries follow */
    unsigned char	rsvd1	:1;	/* reserved */
    unsigned char	share	:1;	/* shared Memory */
    unsigned char	type	:2;	/* memory type */
    unsigned char	rsvd2	:1;	/* reserved */
    unsigned char	cache	:1;	/* memory is cached */
    unsigned char	write	:1;	/* memory is writable */
    unsigned char	decode	:2;	/* address decode */
    unsigned char	width	:2;	/* data path size */
    unsigned char	rsvd 	:4;	/* reserved  */
    unsigned char	start_lsbyte;   /* start address div 100h   */
    unsigned char	start_middlebyte;
    unsigned char	start_msbyte;
    unsigned char       size_lsbyte;    /* memory size in 1K bytes  */   
    unsigned char       size_msbyte;
} nvm_memory;


/* specification of an irq element */
/*             2 bytes long        */
typedef struct {
    unsigned char	more	:1;	/* more follow */
    unsigned char	share	:1;	/* sharable */
    unsigned char	trigger :1;	/* trigger (edge=0, level=1) */
    unsigned char	rsvd1	:1;	/* reserved */
    unsigned char	line	:4;	/* irq line */                  
    unsigned char	rsvd2;		/* reserved */
} nvm_irq;


/* specification of a dma element */
/*             2 bytes long       */
typedef struct {
    unsigned char	more	:1;	/* more entries follow */
    unsigned char	share	:1;	/* shareable */
    unsigned char	rsvd1	:3;	/* reserved */
    unsigned char	channel :3;	/* dma channel number */        
    unsigned char	rsvd2	:2;	/* reserved */
    unsigned char	timing	:2;	/* transfer timing */
    unsigned char	width	:2;	/* transfer size */
    unsigned char	rsvd3	:2;	/* reserved */
} nvm_dma;


/* specification of a port element */
/*             3 bytes long        */
typedef struct {
    unsigned char	more	:1;	/* more entries follow */
    unsigned char	share	:1;	/* shareable */
    unsigned char	rsvd	:1;	/* reserved */
    unsigned char	count	:5;	/* number of sequential ports - 1 */
    unsigned char	address_lsb;	/* io port address */           
    unsigned char	address_msb;	/* io port address */           
} nvm_port;


/* specification of an init element */
/*            11 bytes long         */              
typedef struct {
    unsigned char	more	:1;	/* more entries follow */
    unsigned char	rsvd	:4;	/* reserved */
    unsigned char	mask	:1;	/* apply mask */
    unsigned char	type	:2;	/* port type */
    unsigned char	port_lsb;	/* port address */              
    unsigned char	port_msb;

    /** The following 8 bytes have different meanings depending on the 
	data size and whether a mask is involved:

	    byte-sized with no mask:
		valmask_1:  byte value to write

	    byte-sized with mask:
		valmask_1:  byte value to write
		valmask_2:  mask to apply

	    word-sized with no mask:
		valmask_1:  lsb of word value to write
		valmask_2:  msb of word value to write

	    word-sized with mask:
		valmask_1:  lsb of word value to write
		valmask_2:  msb of word value to write
		valmask_3:  lsb of word mask to apply
		valmask_4:  msb of word mask to apply

	    dword-sized with no mask:
		valmask_1:  lsb of dword value to write
		valmask_2:  byte2 of dword value to write
		valmask_3:  byte3 of dword value to write
		valmask_4:  msb of dword value to write

	    dword-sized with mask:
		valmask_1:  lsb of dword value to write
		valmask_2:  byte2 of dword value to write
		valmask_3:  byte3 of dword value to write
		valmask_4:  msb of dword value to write
		valmask_5:  lsb of dword mask to apply
		valmask_6:  byte2 of dword mask to apply
		valmask_7:  byte3 of dword mask to apply
		valmask_8:  msb of dword mask to apply

    Do not change this to union of various-sized structs. There
    are alignment problems when doing so.  ****/

    unsigned char	valmask_1;	
    unsigned char	valmask_2;
    unsigned char	valmask_3;
    unsigned char	valmask_4;
    unsigned char	valmask_5;
    unsigned char	valmask_6;
    unsigned char	valmask_7;
    unsigned char	valmask_8;
} nvm_init;


/* what is returned by a read function */
/*            320 bytes                */
/* this is the same stuff as eeprom_function_data in eeprom.h */
typedef struct {
    unsigned char	boardid[4];		/* compressed board id */
    nvm_dupid		dupid;			/* duplicate id information */
    unsigned char	minor_cfg_ext_rev;	/* extension rev (0 if none) */
    unsigned char	major_cfg_ext_rev;	/* extension rev (0 if none) */
    unsigned char	selects[NVM_MAX_SELECTIONS];	/* current selections */
	 /* selects are index numbers of the current selections. They are
	    used to connect the cfg file possibilities with what is currently
	    chosen. The selects are done in the following order:
		o  the index of the currently selected subfunction
		o  the index of the currently selected subchoice
		   (choice->current)
		o  the indexes of the currently selected resource groups */

    nvm_fib 		fib;			/* function information byte */
    unsigned char	ftype[NVM_MAX_TYPE];	/* function type/subtype */
						/* don't think this is needed */
    union  {
	struct	{
	    nvm_memory		memory[NVM_MAX_MEMORY]; /* memory config */
	    nvm_irq 		irq[NVM_MAX_IRQ];	/* irq configuration */
	    nvm_dma 		dma[NVM_MAX_DMA];	/* dma configuration */
	    nvm_port		port[NVM_MAX_PORT];	/* port configuration */
	    unsigned char	init[NVM_MAX_INIT];     /* initialization */
	} r;
	unsigned char	freeform[NVM_MAX_DATA + 1];
						/* don't think this is needed */
    } u;
} nvm_funcinfo;


/* which nvm source/target should be used? */
#define NVM_CURRENT		0	/* use current sci or eeprom */
#define NVM_EEPROM		1	/* use eeprom (and default sci file) */
#define NVM_SCI 		2	/* use sci file */
#define NVM_NONE		-1	/* not established yet */


/* nvm error codes */
#define NVM_SUCCESSFUL		0   	/* no errors */
#define NVM_INVALID_SLOT	1  	/* invalid slot number */
#define NVM_INVALID_FUNCTION	2  	/* invalid function number */
#define NVM_INVALID_EEPROM	3  	/* eeprom corrupt */
#define NVM_EMPTY_SLOT		4  	/* slot is empty */
#define NVM_WRITE_ERROR 	5  	/* failure to write to nvm */
#define NVM_SCI_CANNOT_OPEN	7	/* could not open sci file */
#define NVM_INVALID_SCI		8	/* sci file is corrupt */
#define NVM_EEPROM_IN_USE	9	/* /dev/eeprom already open */
#define NVM_NO_EEPROM_DRIVER	10      /* no eeprom driver in kernel */
#define NVM_NO_EISA_HW		11	/* no eisa hardware in system */


/* various resource types */
#define NVM_MEMORY_SYS		0
#define NVM_MEMORY_EXP		1
#define NVM_MEMORY_VIR		2
#define NVM_MEMORY_OTH		3

#define NVM_MEMORY_BYTE 	0
#define NVM_MEMORY_WORD 	1
#define NVM_MEMORY_DWORD	2

#define NVM_DMA_BYTE		0
#define NVM_DMA_WORD		1
#define NVM_DMA_DWORD		2

#define NVM_DMA_DEFAULT 	0
#define NVM_DMA_TYPEA		1
#define NVM_DMA_TYPEB		2
#define NVM_DMA_TYPEC		3

#define NVM_MEMORY_20BITS	0
#define NVM_MEMORY_24BITS	1
#define NVM_MEMORY_32BITS	2

#define NVM_IOPORT_BYTE 	0
#define NVM_IOPORT_WORD 	1
#define NVM_IOPORT_DWORD	2

#define NVM_IRQ_EDGE		0
#define NVM_IRQ_LEVEL		1


/* current source/target */
extern  unsigned short		nvm_current;
