/*
 * @(#) $Revision: 70.14.1.5 $
 */

#define APOLLOPCI 37
#define HP986PP   36
#define NULL_STR '\0'
#define NULL_IDX -1
#define NULL_ENTRY 0
#define TRUE 1
#define FALSE 0
#define IOSCAN_MAP "./ioscan_map"

#define NUM_COLS 5          /* number of columns in output listing */


#define IOSCAN_DEV_FILE "/dev/rdsk/ioscan_"
#define NO_SCSI_DEV 0x1f

#define FIRST_LU 0
#define SCSI_START_ADDR 0

/*
 * Wide fast SCSI on the 700 supports
 * 16 bus addresses 
 * single ended and differential only
 * supports 7.
 */
#ifdef __hp9000s700

#define MAX_SCSI_LU 7
#define MAX_SCSI_BA 7
#define MAX_FW_SCSI_BA 16

#else

#define MAX_SCSI_LU 1      /* luns are not supported on the 300/400 series */
#define MAX_SCSI_ADDR 8

#endif

#define HPIB_START_ADDR 0
#define MAX_HPIB_ADDR 8

#define FLOPPY_START_ADDR 0
#define MAX_FLOPPY_ADDR 2

#define SW_STATUS_OK "ok"

#define NAME_LEN 20
typedef char NameStr[NAME_LEN];


#define DEF_MASK(x)    ((0x00000001) << (x))

/**************************************************************
 *             DECLARATIONS FOR LIST OF CLASS TYPES
 *
 *  The list of classes represent the different catgories of
 *  interfaces and peripheral devices recognized by ioscan.
 *  The only peripheral devices that are currently recognized
 *  are disk and tapes.
 *
 *  IMPORTANT !!!!
 *       To add a new class type to the list of classes (ClassList)
 *  the following steps must be done.
 *
 *    	 1)  -  The new entry to ClassList must be added to the end
 *  		of the list, before the last entry which is "unknown".
 *	 2)  -  The constant NUM_CLASS must be incremented to resize
 *		the array for the new entry.
 *  	 3)  -  Index and mask defines should also be added below for
 *		for the new entry (see mask defines and Index defines).
 *************************************************************/

#define NUM_CLASS 18

NameStr ClassList[NUM_CLASS+1] = { "hpib",
	 	                   "disk",
	                	   "tape_drive",
	                	   "lan",
	                	   "scsi",
				   "parallel",
				   "serial",
				   "eeprom",
				   "graphics",
				   "hil",
				   "mux",
				   "autoch",
				   "scanner",
				   "audio",
				   "psi",
				   "floppy",
				   "ps2",
				   "ISDN",
				   "unknown"
		      	         };

/*
 *  Index defines for ClassList
 *
 *  These defines are used to access entries in the
 *  in the ClassList.
 */
#define HPIB_C_ENT	0
#define DISK_C_ENT	1
#define TAPE_C_ENT	2
#define LAN_C_ENT	3
#define SCSI_C_ENT	4
#define PARALLEL_C_ENT	5
#define SERIAL_C_ENT	6
#define EEPROM_C_ENT	7
#define GRAPH_C_ENT	8
#define HIL_C_ENT	9
#define MUX_C_ENT	10
#define AUTOCH_C_ENT	11
#define SCANNER_C_ENT	12
#define AUDIO_C_ENT	13
#define PSI_C_ENT	14
#define PC_FDC_C_ENT	15
#define PS2_C_ENT	16
#define ISDN_C_ENT	17
#define UNKNW_C_ENT  NUM_CLASS
/*
 *  Mask defines for ClassList
 *
 *  The following defines are used to test the bits in the
 *  ClassMask.  The index of an entry in the ClassList maps
 *  to a bit position in ClassMask, with the rightmost bit
 *  of the mask corresponding to index 0 and the leftmost bit
 *  corresponding to bit 31.
 */

#define HPIB_CLASS      DEF_MASK(HPIB_C_ENT)
#define DISK_CLASS      DEF_MASK(DISK_C_ENT)
#define TAPE_CLASS      DEF_MASK(TAPE_C_ENT)
#define LAN_CLASS       DEF_MASK(LAN_C_ENT)
#define SCSI_CLASS      DEF_MASK(SCSI_C_ENT)
#define PARALLEL_CLASS  DEF_MASK(PARALLEL_C_ENT)
#define SERIAL_CLASS    DEF_MASK(SERIAL_C_ENT)
#define EEPROM_CLASS    DEF_MASK(EEPROM_C_ENT)
#define  GRAPH_CLASS    DEF_MASK(GRAPH_C_ENT)
#define  HIL_CLASS      DEF_MASK(HIL_C_ENT)
#define MUX_CLASS       DEF_MASK(MUX_C_ENT)
#define AUTOCH_CLASS    DEF_MASK(AUTOCH_C_ENT)
#define SCANNER_CLASS   DEF_MASK(SCANNER_C_ENT)
#define AUDIO_CLASS     DEF_MASK(AUDIO_C_ENT)
#define PSI_CLASS       DEF_MASK(PSI_C_ENT)
#define PC_FDC_CLASS    DEF_MASK(PC_FDC_C_ENT)
#define PS2_CLASS       DEF_MASK(PS2_C_ENT)
#define ISDN_CLASS      DEF_MASK(ISDN_C_ENT)
#define UNKNOWN_CLASS   DEF_MASK(UNKNW_C_ENT)

#define DISK_TAPE_CLASS DISK_CLASS | TAPE_CLASS

/*
 ********************************************************************
 *	       SELECTION MASK DECLARATIONS
 *
 *
 *  The mask variables are declared local in the mainline of ioscan.
 *  This allows each invocation of ioscan to have its own copy of
 *  the masks.
 *	Currently the masks are declared as type int (32 bits).
 *  Later they can be expanded to an array of 2 or more integers
 *  if more bits are required.  Macros are used to access the masks
 *  therefore any modifications required to change the type declaration
 *  of the masks will be localized to this file.
 *
 *	MaskType drv_mask;       Drivers to be searched
 *	MaskType class_mask;     Classes to be searched for 
 *
 ********************************************************************
 */
#define ALL_MASK 0xFFFFFFFF
#define NULL_MASK 0x00000000
#define MASK_BIT_SIZE 32
#define MASK_BYTE_SIZE 4

typedef unsigned MaskType;
/*
 * The following macros are used to set bits in the masks
 *
 */

#define SET_MASK( mask1, mask2 )	((mask1) = (mask2))

#define SET_BITS( mask, bit_pat )	((mask) |= (bit_pat))

#define CLEAR_BITS( mask, bit_pat )	((mask) &= ~(bit_pat))

#define SET_RIGHTMOST_BIT( mask )	((mask) = (0x00000001))

#define SHIFT_LEFT_MASK( mask, incr )	((mask) <<= (incr))

/*
 *  The following macros are used to test bits in the masks
 *
 */
#define TEST_BITS( mask, bit )  ((mask) & (bit))

#define DIFF_MASKS( mask1, mask2 )  ((mask1) ^ (mask2))



/********************************************************************
  		       DRIVER TABLE DECLARATIONS

 *      The driver table consists of an array of records where each
 *  record (row entry) represents a driver.
 *
 *	typedef struct{
 *			DrvNameStr drv_name;
 *	 		short	   drv_type;
 *			MaskType   drv_class;
 *			MaskType   drv_mask;
 *			MaskType   req_drv;
 *			int        if_type;
 *			int	   drv_major;
 *		} DrvRecType;
 *
 *  The fields of the record are described below.
 *
 *                 drv_name    - Is a string which has the name of the driver.
 *
 *		   drv_type    - Is an integer whose value indicate
 *				the type of driver (device, interface).
 *
 *		  drv_class    - Is a bit mask which indicates what class
 *			        of device or devices the driver controls.
 *				The bits map to the entries in the
 *				ClassList array, with the rightmost bit
 *				corresponding to ClassList[0], the next bit
 *				ClassList[1] etc.
 *
 *	 	  req_drv    - Is a bit mask which indicates if any other
 *			       drivers are required by this driver.  If no
 *			       drivers are required, it is set to the
 *			       NULL_MASK otherwise it is set to the bit
 *			       patterns of the required drivers.
 *
 *		  if_type    - Is an integer indicating the interface type for
 *			       a given driver.  The defines for the different
 *			       interfaces are declared below.
 *
 *		  major      - The major number of the character device driver.
 *
 *	   There will be two versions of the table, one for the 300/400
 *	   series and one for the 700 series.
 *
 *	   Currently the driver table is declared staticly in this file.
 *	   Ideally in the future it would be nice if the table could be
 *	   generated dynamically, as part of the config process.  To do
 *	   this a database of driver names and hardware could be added
 *	   to the file "/etc/master".
 *
 *	Adding a new driver
 *     
 *	   To add a new driver to the table the constant DRV_TBL_SIZE
 *	   must be incremented and the new entry inserted at the end
 *	   of the table, before the "unknown" entry.  Defines should
 *	   be declared for the driver bit mask and the driver index
 *	   (See defines below).  If the driver is a scsi device driver
 *	   then its mask should be or'd into the "scsi_drv_mask" and
 *	   and its index added to the array "scsi_dev_drvs". When
 *	   adding an entry to the "scsi_dev_drvs" array the constant
 *	   MAX_SCSI_DEV_TYPES must be incremented to make room.
 *	   If the driver is an HPIB type device driver (cs80, amigo etc)
 *         then its mask should be or'd into the "hpib_drv_mask" mask
 *	   and the index of the driver added to the "hpib_dev_drvs" array.
 *
 ***************************************************************************
 */

/*****************************
 * constants for driver types
 *****************************
 */
#define DEV_DRV		1
#define IF_DRV		2
#define DEV_IF_DRV	3

/***************************************
 * These defines are the interface types
 * for the drivers.
 **************************************
 */
#define UNKNOWN_IF_TYPE    0
#define HPIB_IF_TYPE	   1
#define SCSI_IF_TYPE	   2
#define PARALLEL_IF_TYPE   3
#define SERIAL_IF_TYPE 	   4
#define EEPROM_IF_TYPE 	   5
#define LAN_IF_TYPE        6
#define HIL_IF_TYPE        6
#define GRAPH_IF_TYPE 	   7
#define AUDIO_IF_TYPE	   8
#define PSI_IF_TYPE	   9
#define PC_FDC_IF_TYPE     10
#define PS2_IF_TYPE        11
#define ISDN_IF_TYPE       12


/**************************************
* general declarations for driver table
***************************************
*/
#define DRV_NAME_LEN 15
typedef char DrvNameStr[DRV_NAME_LEN];

typedef struct{
		DrvNameStr drv_name;
		short	   drv_type;
		MaskType   drv_class;
		MaskType   drv_mask;
		MaskType   req_drv;
		int        if_type;
		int	   drv_major;
} DrvRecType;

/***************************************
****************************************
*   DECLARATION FOR 700 DRIVER TABLE   *
****************************************
****************************************
*/
#ifdef __hp9000s700

#define DRV_TBL_SIZE 24

#define CS80_D_ENT	0 
#define HSHPIB_D_ENT	1
#define SCSITAPE_D_ENT	2
#define SCSI_D_ENT	3
#define C700_D_ENT	4
#define AUTOX_D_ENT	5
#define PARALLEL_D_ENT	6
#define ASIO_D_ENT	7
#define EISA_D_ENT	8
#define LAN01_D_ENT	9
#define GRAPH3_D_ENT	10
#define SCTL_D_ENT	11
#define HIL_D_ENT	12
#define FDDI_D_ENT	13
#define FDDI2_D_ENT	14
#define TOKEN_D_ENT	15
#define AUDIO_D_ENT	16
#define FLOPPY_D_ENT	17
#define PSI_D_ENT	18
#define PC_FDC_D_ENT	19
#define PC_FLOPPY_D_ENT	20
#define PS2_D_ENT	21
#define ISDN_D_ENT	22
#define TOKEN3_D_ENT	23
#define UNKNW_D_ENT DRV_TBL_SIZE

/*
   These defines indicate the bit position of a driver in the drv_mask
   The bit set corresponds to the entry for the driver in the driver
   table.  For example the cs80 driver is the first driver in the
   driver table therefore its bit mask is 0x00000001.
*/
#define CS80	  DEF_MASK(CS80_D_ENT)
#define HSHPIB	  DEF_MASK(HSHPIB_D_ENT)
#define SCSITAPE  DEF_MASK(SCSITAPE_D_ENT)
#define SCSI	  DEF_MASK(SCSI_D_ENT)

#define C700   	  DEF_MASK(C700_D_ENT)
#define AUTOX	  DEF_MASK(AUTOX_D_ENT)
#define PARALLEL  DEF_MASK(PARALLEL_D_ENT)
#define ASIO_0 	  DEF_MASK(ASIO_D_ENT)

#define EISA	  DEF_MASK(EISA_D_ENT)
#define LAN_01    DEF_MASK(LAN01_D_ENT)
#define GRAPH3    DEF_MASK(GRAPH3_D_ENT)
#define SCTL      DEF_MASK(SCTL_D_ENT)
#define HIL       DEF_MASK(HIL_D_ENT)
#define FDDI      DEF_MASK(FDDI_D_ENT)
#define FDDI2     DEF_MASK(FDDI2_D_ENT)
#define TOKEN  	  DEF_MASK(TOKEN_D_ENT)
#define AUDIO  	  DEF_MASK(AUDIO_D_ENT)
#define FLOPPY 	  DEF_MASK(FLOPPY_D_ENT)
#define PSI 	  DEF_MASK(PSI_D_ENT)
#define PC_FDC    DEF_MASK(PC_FDC_D_ENT)
#define PC_FLOPPY DEF_MASK(PC_FLOPPY_D_ENT)
#define PS2       DEF_MASK(PS2_D_ENT)
#define ISDN      DEF_MASK(ISDN_D_ENT)
#define TOKEN3 	  DEF_MASK(TOKEN3_D_ENT)
#define UNKNW_DRV DEF_MASK(UNKNW_D_ENT)


DrvRecType drv_tbl[DRV_TBL_SIZE+1] = {

"cs80",     DEV_DRV, (DISK_CLASS | TAPE_CLASS), CS80,     HSHPIB,    HPIB_IF_TYPE,      4,
"hshpib",   IF_DRV,  HPIB_CLASS,      HSHPIB,   NULL_MASK, HPIB_IF_TYPE,     -1,
"scsitape", DEV_DRV, TAPE_CLASS,      SCSITAPE, C700,      SCSI_IF_TYPE,    105,
"scsi",     DEV_DRV, DISK_CLASS|AUTOCH_CLASS,      SCSI,     C700,      SCSI_IF_TYPE,    105,
"c700",     IF_DRV,  SCSI_CLASS,      C700,     NULL_MASK, SCSI_IF_TYPE,     -1,
"autox",    DEV_DRV, AUTOCH_CLASS,    AUTOX,    C700,      SCSI_IF_TYPE,     33,
"parallel", IF_DRV,  PARALLEL_CLASS,  PARALLEL, NULL_MASK, PARALLEL_IF_TYPE, -1,
"asio0",    IF_DRV,  SERIAL_CLASS,    ASIO_0,   NULL_MASK, SERIAL_IF_TYPE,   -1,
"eisa",     IF_DRV,  EEPROM_CLASS,    EISA,     NULL_MASK, EEPROM_IF_TYPE,   64,
"lan01",    IF_DRV,  LAN_CLASS,       LAN_01,   NULL_MASK, LAN_IF_TYPE,	     52,
"graph3",   IF_DRV,  GRAPH_CLASS,     GRAPH3,   NULL_MASK, GRAPH_IF_TYPE,    -1,
"sctl",     DEV_DRV, SCANNER_CLASS,   SCTL,     C700,      SCSI_IF_TYPE,    105,
"hil",      DEV_DRV, HIL_CLASS,       HIL,      NULL_MASK, HIL_IF_TYPE,      24,
"fddi",     IF_DRV,  LAN_CLASS,       FDDI,     NULL_MASK, LAN_IF_TYPE,      49,
"fddi2",    IF_DRV,  LAN_CLASS,       FDDI2,    NULL_MASK, LAN_IF_TYPE,     111,
"token",    IF_DRV,  LAN_CLASS,	      TOKEN,	NULL_MASK, LAN_IF_TYPE,	    102,
"audio",    IF_DRV,  AUDIO_CLASS,     AUDIO,    NULL_MASK, AUDIO_IF_TYPE,    57,
"scsifloppy", DEV_DRV, DISK_CLASS,    FLOPPY,   C700,	   SCSI_IF_TYPE,    106,
"psi",	    IF_DRV,  PSI_CLASS,	      PSI,	NULL_MASK, PSI_IF_TYPE,	    59,
"pcfdc",    IF_DRV,  PC_FDC_CLASS,    PC_FDC,   NULL_MASK, PC_FDC_IF_TYPE,   -1,
"pcfloppy", DEV_DRV, DISK_CLASS,      PC_FLOPPY,PC_FDC,	   PC_FDC_IF_TYPE,  112,
"ps2",      IF_DRV,  PS2_CLASS,       PS2,      NULL_MASK, PS2_IF_TYPE,     159,
"ISDN",     IF_DRV,  ISDN_CLASS,      ISDN,     NULL_MASK, ISDN_IF_TYPE,     -1,
"token3",   IF_DRV,  LAN_CLASS,	      TOKEN3,	NULL_MASK, LAN_IF_TYPE,	    160,
"unknown",  NULL_ENTRY, UNKNOWN_CLASS, UNKNW_DRV , NULL_MASK, 0,             -1
};


MaskType scsi_drv_mask = SCSI | SCSITAPE | AUTOX | SCTL | FLOPPY;

/*
 * The entries in this array map the possible integer values
 * in the dev_type field of the record returned by the SIOC_INQUIRY 
 * ioctl to the SCSI drivers in the kernel that control them.
 *
 */
#define MAX_SCSI_DEV_TYPES 10
int scsi_dev_types[MAX_SCSI_DEV_TYPES] = { SCSI_D_ENT,	  /* direct access */
				 	   SCSITAPE_D_ENT,/* sequential access*/
					   UNKNW_DRV,     /* printer device */
				 	   SCTL_D_ENT,    /* processor device */
					   SCSI_D_ENT,    /* write-once dev */
					   SCSI_D_ENT,    /* CD-ROM dev     */
				           SCTL_D_ENT,    /* Scanner        */
			 		   SCSI_D_ENT,    /* Optical mem    */
					   AUTOX_D_ENT,   /* Medium changer */
				 	   UNKNW_DRV      /* Communications */
				       };

/*
 * This array lists the SCSI device drivers that
 * can be used for probing the bus for SCSI devices. 
 */
#define MAX_SCSI_DEV_DRVS 1
int scsi_dev_drvs[MAX_SCSI_DEV_DRVS] = {
					SCTL_D_ENT
				       };
MaskType hpib_drv_mask = CS80;
/*
 * This array lists the HPIB device drivers that
 * can be used for probing the bus for HPIB devices. 
 */
#define MAX_HPIB_DRVS 1
int hpib_dev_drvs[ MAX_HPIB_DRVS ] = { CS80_D_ENT };

MaskType floppy_drv_mask = PC_FDC;
/*
 * This array lists the PC Floppy device drivers that
 * can be used for probing the bus for floppy devices. 
 */
#define MAX_FLOPPY_DEV_DRVS 1
int floppy_dev_drvs[ MAX_FLOPPY_DEV_DRVS ] = { PC_FLOPPY_D_ENT };


#else
/***************************************
****************************************
*   DECLARATION FOR 300 DRIVER TABLE   *
****************************************
****************************************
*/

#define DRV_TBL_SIZE 18

#define CS80_D_ENT	0 
#define H98625_D_ENT	1
#define H98624_D_ENT	2
#define SCSITAPE_D_ENT	3
#define SCSI_D_ENT	4
#define H98265_D_ENT	5
#define H98658_D_ENT	6
#define AUTOX_D_ENT	7
#define TAPE_D_ENT	8
#define STAPE_D_ENT	9
#define LLA_D_ENT	10
#define GRAPHICS_D_ENT	11
#define AMIGO_D_ENT	12
#define PARALLEL_D_ENT  13
#define H98626_D_ENT	14
#define H98628_D_ENT	15
#define MUX_D_ENT	16
#define APCI_D_ENT	17
#define UNKNW_D_ENT DRV_TBL_SIZE

#define CS80	   DEF_MASK(CS80_D_ENT)
#define H98625	   DEF_MASK(H98625_D_ENT)
#define H98624	   DEF_MASK(H98624_D_ENT)
#define SCSITAPE   DEF_MASK(SCSITAPE_D_ENT)
#define SCSI	   DEF_MASK(SCSI_D_ENT)
#define H98265	   DEF_MASK(H98265_D_ENT)
#define H98658	   DEF_MASK(H98658_D_ENT)
#define AUTOX	   DEF_MASK(AUTOX_D_ENT)
#define TAPE	   DEF_MASK(TAPE_D_ENT)
#define STAPE	   DEF_MASK(STAPE_D_ENT)
#define LLA	   DEF_MASK(LLA_D_ENT)
#define GRAPHICS   DEF_MASK(GRAPHICS_D_ENT)
#define AMIGO	   DEF_MASK(AMIGO_D_ENT)
#define PARALLEL   DEF_MASK(PARALLEL_D_ENT)
#define H98626	   DEF_MASK(H98626_D_ENT)
#define H98628	   DEF_MASK(H98628_D_ENT)
#define MUX	   DEF_MASK(MUX_D_ENT)
#define APCI	   DEF_MASK(APCI_D_ENT)
#define UNKNW_DRV  DEF_MASK(UNKNW_D_ENT)

#define SCSI_IF  H98265 | H98658
#define HPIB_IF  H98625 | H98624

DrvRecType drv_tbl[DRV_TBL_SIZE+1] = {

"cs80",     DEV_DRV, DISK_TAPE_CLASS, CS80,     HPIB_IF,   HPIB_IF_TYPE,   4,
"98625",    IF_DRV,  HPIB_CLASS,      H98625,   NULL_MASK, HPIB_IF_TYPE,   -1,
"98624",    IF_DRV,  HPIB_CLASS,      H98624,   NULL_MASK, HPIB_IF_TYPE,   -1,
"scsitape", DEV_DRV, TAPE_CLASS,      SCSITAPE, SCSI_IF,   SCSI_IF_TYPE,   54,
"scsi",	    DEV_DRV, DISK_CLASS|AUTOCH_CLASS,      SCSI,     SCSI_IF,   SCSI_IF_TYPE,   47,
"98265",    IF_DRV,  SCSI_CLASS,      H98265,   NULL_MASK, SCSI_IF_TYPE,   -1,
"98658",    IF_DRV,  SCSI_CLASS,      H98658,   NULL_MASK, SCSI_IF_TYPE,   -1,
"autox",    DEV_DRV, AUTOCH_CLASS,    AUTOX,    SCSI_IF,   SCSI_IF_TYPE,   33,
"tape",	    DEV_DRV, TAPE_CLASS,      TAPE,     HPIB_IF,   HPIB_IF_TYPE,    5,
"stape",    DEV_DRV, TAPE_CLASS,      STAPE,    HPIB_IF,   HPIB_IF_TYPE,    9,
"lla",	    IF_DRV,  LAN_CLASS,	      LLA,      NULL_MASK, LAN_IF_TYPE,    18,
"graphics", IF_DRV,  GRAPH_CLASS,     GRAPHICS, NULL_MASK, GRAPH_IF_TYPE,  12,
"amigo",    DEV_DRV, DISK_CLASS,      AMIGO,    HPIB_IF,   HPIB_IF_TYPE,   11,
"parallel", IF_DRV,  PARALLEL_CLASS,  PARALLEL, NULL_MASK, PARALLEL_IF_TYPE, -1,
"98626",    IF_DRV,  SERIAL_CLASS,    H98626,   NULL_MASK, SERIAL_IF_TYPE, -1,
"98628",    IF_DRV,  SERIAL_CLASS,    H98628,   NULL_MASK, SERIAL_IF_TYPE, -1,
"mux",	    IF_DRV,  SERIAL_CLASS,    MUX,      NULL_MASK, SERIAL_IF_TYPE, -1,
"apci",	    IF_DRV,  SERIAL_CLASS,    APCI,     NULL_MASK, SERIAL_IF_TYPE, -1,
"unknown",  NULL_ENTRY, UNKNOWN_CLASS, UNKNW_DRV , NULL_MASK, 0,           -1
};

MaskType scsi_drv_mask = SCSITAPE | SCSI | AUTOX;

/*
 * The entries in this array map the possible integer values
 * in the dev_type field of the record returned by the SIOC_INQUIRY 
 * ioctl to the SCSI drivers in the kernel that control them.
 *
 */
#define MAX_SCSI_DEV_TYPES 10
int scsi_dev_types[MAX_SCSI_DEV_TYPES] = {  SCSI_D_ENT,	   /* Direct access   */
				 	    SCSITAPE_D_ENT,/* Sequent-access  */
				   	    UNKNW_DRV,	   /* printer device  */
				   	    UNKNW_DRV,     /* processor device*/
					    SCSI_D_ENT,	   /* Write-once      */
					    SCSI_D_ENT,	   /* CD-ROM device   */
				   	    UNKNW_DRV,	   /* Scanner device  */
			 		    SCSI_D_ENT,	   /* Optical memory  */
					    AUTOX_D_ENT,   /* Medium changer  */
				            UNKNW_DRV      /* Communications  */
				          };
/*
 * This array lists the SCSI device drivers that
 * can be used for probing the bus for SCSI devices. 
 */
#define MAX_SCSI_DEV_DRVS 2
int scsi_dev_drvs[MAX_SCSI_DEV_DRVS] = {
					SCSI_D_ENT,
					SCSITAPE_D_ENT
				       };


MaskType hpib_drv_mask = CS80 | TAPE | STAPE | AMIGO;

/*
 * This array lists the HPIB device drivers that
 * can be used for probing the bus for HPIB devices. 
 */
#define MAX_HPIB_DRVS 4
int hpib_dev_drvs[ MAX_HPIB_DRVS ] = {	CS80_D_ENT,
					TAPE_D_ENT,
					STAPE_D_ENT,
					AMIGO_D_ENT
				     };
#endif

/***********************************************
 * Definitions for hardware table
 *
 * The hardware table is an array of records
 * which contains entries that describe the
 * different types of supported interfaces.
 * Each record contains three fields:
 *
 *	hdw_id -  An integer which is the interface
 *		  or card id of the interface.
 *		 
 *	hdw_drv - An integer which is an index
 *	          into the driver table for the
 *		  corresponding interface driver.
 *		 
 *	hdw_class - An integer which is an index into
 *		    the ClassList array for the
 *		    corresponding interface class.
 *
 *	There are two different table declarations, one
 *	for the series 700 and another for the series
 *	300/400.
 *	Currently this table is declared staticly within
 *	this file.  Later it would be nice if the table
 *	could be generated dynamically as part of the
 *	config process.  This would require a database of
 *	supported interface cards in the "/etc/master" file.
 *
 *	Adding an entry
 *
 *	Any new entries should be added to the end of
 *	the table, before the UNKNW_HDW entry.  The constant
 *	SIZE_OF_HDW_TBL should be incremented to make room.
 *
 ***********************************************
 */
typedef struct{
		int hdw_id;    /* id code for hardware */
		int hdw_drv;   /* index into drv_tbl for controlling driver */
		int hdw_class; /* index into ClassList for type of hardware */
} HdwRecType;

#ifdef __hp9000s700

#define UNKNW_HDW 0xFFFF

#define SIZE_OF_HDW_TBL 26
HdwRecType hdw_tbl[SIZE_OF_HDW_TBL+1] = {

SGC_SV_ID,   		GRAPH3_D_ENT,	GRAPH_C_ENT,
SCSI_SV_ID,		C700_D_ENT,	SCSI_C_ENT,
LAN_SV_ID,		LAN01_D_ENT, 	LAN_C_ENT,
CENT_SV_ID,		PARALLEL_D_ENT,	PARALLEL_C_ENT,
SERIAL_SV_ID,		ASIO_D_ENT,	SERIAL_C_ENT,
0x22f00c70,		HSHPIB_D_ENT,	HPIB_C_ENT,
0x22f01850,		LAN01_D_ENT,	LAN_C_ENT,
HIL_SV_ID,		HIL_D_ENT, 	HIL_C_ENT,
SCSI_FW_SV_ID,		C700_D_ENT,	SCSI_C_ENT,
SCSI_F_EISA_SV_ID,	C700_D_ENT,	SCSI_C_ENT,
SCSI_EISA_SV_ID,	C700_D_ENT,	SCSI_C_ENT,
0x22f01860,		FDDI_D_ENT,	LAN_C_ENT,
0x7D,			FDDI2_D_ENT,	LAN_C_ENT,
0x34870002,		TOKEN_D_ENT,	LAN_C_ENT,
0x7A,			AUDIO_D_ENT,	AUDIO_C_ENT,
0x7B,			AUDIO_D_ENT,	AUDIO_C_ENT,
0x7E,			AUDIO_D_ENT,	AUDIO_C_ENT,
0x7F,			AUDIO_D_ENT,	AUDIO_C_ENT,
0x22f01870,		PSI_D_ENT,	PSI_C_ENT,
0x82,			C700_D_ENT,	SCSI_C_ENT,
0x83,			PC_FDC_D_ENT,   PC_FDC_C_ENT,
0x84,			PS2_D_ENT,	PS2_C_ENT,
0x85,			GRAPH3_D_ENT,	GRAPH_C_ENT,
0x87,			ISDN_D_ENT,	ISDN_C_ENT,
0x8A,			LAN01_D_ENT, 	LAN_C_ENT,
0x00005e80,		TOKEN3_D_ENT,	LAN_C_ENT,
UNKNW_HDW,		UNKNW_D_ENT, 	UNKNW_C_ENT
};

#else

#define SIZE_OF_HDW_TBL 11
#define UNKNW_HDW 0xFFFF

HdwRecType hdw_tbl[SIZE_OF_HDW_TBL+1] = {
        
INTERNAL_HPIB,	H98624_D_ENT,	HPIB_C_ENT,
HP98624,	H98624_D_ENT,	HPIB_C_ENT,
HP98625,	H98625_D_ENT,	HPIB_C_ENT,
HP98626,	H98626_D_ENT,	SERIAL_C_ENT,
HP98628,	H98628_D_ENT,	SERIAL_C_ENT,
SCUZZY,		H98265_D_ENT,	SCSI_C_ENT,
HP98642,        MUX_D_ENT,      MUX_C_ENT,
HP98643,	LLA_D_ENT,	LAN_C_ENT,
HP98644,	H98626_D_ENT,	SERIAL_C_ENT,
APOLLOPCI,	APCI_D_ENT,	SERIAL_C_ENT,
HP986PP,	PARALLEL_D_ENT, PARALLEL_C_ENT,
UNKNW_HDW,      UNKNW_D_ENT,      UNKNW_C_ENT
};

#endif


/**************************************************************************
 *  The structure OptRecType is used by ioscan to save the options passed in
 *  on the command line.  The variable of this type structure MUST be
 *  declared local in the mainline so that each call to ioscan has
 *  its own unique copy.  If it were declared global then the code would
 *  not be reentrant and therefore the command could not support multiple
 *  threads.
 **************************************************************************
 */

#define OPT_LEN 20
#define MAX_HW_STR 30
#define MAX_SW_STR 30
#define MAX_STATUS_STR 23

typedef char HwPathType[MAX_HW_STR];
typedef char StatusType[MAX_STATUS_STR];

typedef char OptStrType[OPT_LEN];

typedef struct {
		OptStrType d_option;      /* -d option        		    */
		OptStrType C_option;      /* -C options       		    */
		HwPathType H_option;      /* -H options       		    */
		int        f_list;	  /* long list option specified     */
		int	   b_opt;
		int	   h_opt;
	} OptRecType;


#define CLASS_HDR       "Class"
#define LU_HDR	        "LU"
#define HW_PATH_HDR     "H/W Path"
#define SW_PATH_HDR     "Driver"
#define HW_STATUS_HDR   "H/W Status"
#define SW_STATUS_HDR   "S/W Status"
#define DESC_HDR	"Description"
#define STATUS_HDR      "Status"
#define HVERS_HDR       "hversion"

/*
 * defines for hdw_type field
 */
#define IF_CARD 0
#define DEVICE  1
#define MAX_DESC 30

typedef struct {
		short	   print_node;
		short	   class;
		HwPathType hw_path;
		int	   sw_path;
		StatusType hw_status;
		StatusType hw_hversion;
		StatusType sw_status;
		char	   desc[MAX_DESC];
		short	   hdw_type    /* 0 interface card : 1 = device */
} ModuleRecType;

typedef struct {
		int max_class_len;
		int max_hw_path_len;
		int max_sw_path_len;
		int max_hw_status_len;	   /* max string length for field    */
		int max_hw_hversion_len;   /* max string length for field    */
		int max_sw_status_len;
		int max_desc_len;	   /* max string length for field    */
      	        int nxt_avail; 		   /* index of next available record */
		int num_print;
} HdrRecType;

typedef union {
		HdrRecType    hdr_rec;
		ModuleRecType mod_rec;
} IoMapType;

