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
*                                 inc/sci.h
*
*  The format of the sci data structure looks like this:
*
*       an sci_header
*
*       an sci_record
*         slot_num = 0
*         length = x
*
*       a body of a slot record of length x
*
*       an sci_record
*         slot_num = 1
*         length = y
*
*       a body of a slot record of length y
*
*       .
*       .
*       .
*       .
*
*
*       an sci_record
*         slot_num = SCI_EOF
*
*       an sci_header for the cfg file data
*
*	an sci_cfg_record
*	  boardid = system board id
*
*	the cfg file data for the system board
*
*	an sci_cfg_record
*	  boardid = next board id
*
*	the cfg file data for the next board
*
*       .
*       .
*       .
*       .
*
*       an sci_cfg_record
*         boardid = SCI_EOF
*
*
*  Each of the slot record bodies is of the following form:
*
*       compressed board id                      4 bytes
*       dupid information                        2 bytes
*       cfg extension rev level                  2 bytes
*
*       function 0 information
*       function 1 information
*
*       .
*       .
*       .
*
*       function n-1 information
*
*       checksum for the cfg file for this board  2 bytes
*
*
*  Each of the functions is of the following form:
*
*       function length (length bytes not inc)  2 bytes (LSB first!)
*	selections				selections[0] + 1 bytes
*       fib					1 byte
*	if (fib.type)
*	   type string				type[0] + 1 bytes
*	if (fib.data)
*	   freeform data string			data[0] + 1 bytes
*	else
*	   if (fib.memory)
*	      memory blocks (until more == 0)	
*	   if (fib.int)
*             interrupt blocks (until more == 0)
*	   if (fib.dma)
*	      dma blocks (until more == 0)	
*	   if (fib.port)
*	      port blocks (until more == 0)	
*	   if (fib.init)
*	      init blocks (until more == 0)	
*
*  The last function must have a function length of 0 (end marker).
*
*  Note that the slot body record (and children) is defined in the EISA spec
*  under the Write NVM section (page 368 in rev 3.10)
*
*******************************************************************************/


typedef struct {
    unsigned int	signature;	/* sci file signature */
    unsigned int	checksum;	/* sci file checksum */
    unsigned int	revision;	/* creator utility version */
    unsigned char	title[96];	/* sci file title */
    unsigned int	length;		/* of data, this section only */
} sci_header;


/* Note that I make sure this is always 4-byte aligned. */
typedef struct {
    unsigned int	slot_num;	/* slot number or SCI_EOF or boardid */
    unsigned int	length; 	/* bytes in record */
} sci_record;


/* Note that I make sure this is always 4-byte aligned. */
typedef struct {
    unsigned int	boardid;	/* boardid or SCI_EOF */
    unsigned int	length; 	/* bytes in record */
} sci_cfg_record;


#define SCI_DEFAULT_NAME	"/etc/eisa/system.sci"
#define SCI_EOF 		0xFFFFFFFF	/* End of file record */
#define SCI_SIGNATURE		0x694C		/* sci file signature */


/****************
* Utility revisions:
*
*   There have been two flavors of SCI files. Both are still supported for
*   read, but only the most recent (UTIL_REVISION_2) is currently written
*   by eisa_config.
*
*   The original version (UTIL_REVISION_1) contained only the configuration
*   information. It was made up of an sci_header followed by a set of
*   sci_record/slot record pairs. It was terminated by a EOF sci_record.
*   The size of the entire file was not saved in the file -- it can be obtained
*   by stat'ing the file.
*
*   The current version (UTIL_REVISION_2) is just an extension of the original
*   version. The big change is that cfg records are now placed after the
*   original configuration records. This was done for reliability. If a CFG
*   file that is in the current configuration is lost, it will be automatically
*   retrieved from the default system.sci file.
*
****************/

#define UTIL_REVISION_1		1	/* starting format */
#define UTIL_REVISION_2		2	/* added cfg files to end of sci file */


