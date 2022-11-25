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
*+*+*+*************************************************************************
*
*                             src/nvmlowlevel.c
*
*   This module contains the low level routines for handling interactions
*   with either the eeprom or sci files.
*
*       nvm_clear()		-- open_save.c
*       nvm_initialize()  	-- open_save.c, init.c
*       nvm_read_slot()		-- init.c, nvmload.c, open_save.c
*       nvm_read_function()	-- nvmload.c
*       nvm_write_slot()	-- nvmsave.c
*	nvm_reinitialize()	-- init.c
*	nvm_chk_checksum()	-- init.c
*	nvm_board_inited()	-- init.c
*	sci_flush()		-- open_save.c
*	dump_iodc_slot()	-- main.c
*	sci_init_cfg_file()	-- main.c
*	sci_add_cfg_file()	-- add.c
*	sci_remove_cfg_file()	-- main.c
*	sci_move_cfg_file()	-- main.c
*	sci_make_cfg_file()	-- cfgload.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/errno.h>
#include <sys/eeprom.h>
#include <fcntl.h>
#include "config.h"
#include "nvm.h"
#include "sci.h"
#include "err.h"


/* functions declared in this file */
int			nvm_clear();
int			nvm_read_slot();
int			nvm_read_function();
int			nvm_write_slot();
int			nvm_initialize();
void			nvm_reinitialize();
int			nvm_chk_checksum();
int			nvm_board_inited();
int			sci_flush();
void			dump_iodc_slot();
void			sci_init_cfg_file();
void			sci_add_cfg_file();
void			sci_move_cfg_file();
void			sci_remove_cfg_file();
int			sci_make_cfg_file();
static int		sci_init();
static void		sci_write_header();
static int		sci_read_slot();
static int		sci_read_function();
static void		sci_write_slot();
static int 		sci_expand_function();
static sci_record       *sci_locate();
static int		sci_checksum();
static void 		sci_write_cfg_recs();


/* global to this file only -- pertain to the current sci file */
sci_header      	*sci_hdr;
unsigned int		sci_length;


/* global to this file only -- keeps track of what cfg files are in use */
char			*sci_cfg_file_names[MAX_NUM_SLOTS] =
					{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};


/* globals used here -- declared in globals.c */
extern char		sci_name[];
extern char		iodc_name[];
extern int		eeprom_fd;
extern int		eeprom_opened;


/* global err values (for reads & writes, etc.) */
extern int		errno;


/* functions declared elsewhere */
extern  void            *mn_trapcalloc();
extern  void            *mn_traprealloc();
extern  void            mn_trapfree();
extern int 		nvm_init_size();
extern void	        get_cfg_file_base_name();
extern int	        make_board_id();


/****+++***********************************************************************
*
* Function:     nvm_initialize()
*
* Parameters:   source			NVM_EEPROM or NVM_SCI
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		Successful
*		NVM_INVALID_EEPROM	Couldn't read eeprom or bad source
*		NVM_EEPROM_IN_USE	/dev/eeprom already open
*		NVM_NO_EEPROM_DRIVER	No eeprom driver in the kernel
*		NVM_NO_EISA_HW		System does not have eisa hardware
*               NVM_INVALID_SCI		File was read, but is not an sci file
*		NVM_SCI_CANNOT_OPEN	Could not read sci file
*
* Description:
*
*   In the NVM_EEPROM case, this function opens the eeprom device. It saves
*   the fd in the global eeprom_fd.
*
*   In the NVM_SCI case, this function calls a rtn to get the contents of
*   the sci file pointed to by the global sci_name.
*
****+++***********************************************************************/

nvm_initialize (source)
    unsigned int	source;
{
    int			err=1;


    if (source == NVM_EEPROM) {

try_open:
	eeprom_fd = open("/dev/eeprom", O_RDWR);
	if (eeprom_fd == -1) {

	    /*************
	    * Someone else has the /dev/eeprom driver open.
	    *************/
	    if (errno == EBUSY)
		return(NVM_EEPROM_IN_USE);

	    /*************
	    * The driver is in the kernel, but there is no eisa hw.
	    *************/
	    else if (errno == ENXIO)
		return(NVM_NO_EISA_HW);

	    /*************
	    * The driver is not in the kernel. When this happens, we need
	    * to find out if there is eisa hardware there, anyway.
	    *************/
	    else if (errno == ENODEV)
		/* try to discover if we have eisa hw in the system
		   or are just missing the driver
		   if so,
			return(NVM_NO_EEPROM_DRIVER);
		   else
			return(NVM_NO_EISA_HW);
		*/
		return(NVM_NO_EEPROM_DRIVER);

	    /*************
	    * The device file was not there and we have not already tried
	    * to created it.
	    *************/
	    else if ( ( (errno == EACCES) || (errno == ENOENT) )  &&
			(err == 1) )  {
		err = mknod("/dev/eeprom", S_IFCHR|S_IRUSR|S_IWUSR,
			    makedev(64,0));
		goto try_open;
	    }

	    /************
	    * The eeprom is broken.
	    ************/
	    return(NVM_INVALID_EEPROM);
	}

	eeprom_opened = 1;
	return(NVM_SUCCESSFUL);

    }

    return(sci_init());

}


/****+++***********************************************************************
*
* Function:     nvm_reinitialize()
*
* Parameters:   None
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   This function is used to re-initialize boards which have been added 
*   since the system has booted. The drivers for the new boards will be
*   attached and the boards will be usable afterwards.
*
*   This function can only be called if the NVM data for the new boards
*   will not conflict with the existing boards; that is, if none of the
*   existing boards was changed in any way.
*
*   There are no errors returned form the ioctl (or this function). The
*   kernel will print error messages (if any) to the system console.
*
****+++***********************************************************************/

void nvm_reinitialize()
{

    (void)ioctl(eeprom_fd, INIT_NEW_CARDS);

}



/****+++***********************************************************************
*
* Function:     nvm_board_inited()
*
* Parameters:   slotnum			slot number to check for
*
* Used:		external only
*
* Returns:      1		board was initialized
*               0		board was not initialized
*
* Description:
*
*	This function figures out if the board in a given slot was
*	initialized by the kernel this time around.
*
****+++***********************************************************************/

int nvm_board_inited(slotnum)
    int		slotnum;
{
    int		err;


    err = ioctl(eeprom_fd, WAS_BOARD_INITED, &slotnum);
    if (err == 0)
	return(1);
    return(0);

}



/****+++***********************************************************************
*
* Function:     nvm_chk_checksum()
*
* Parameters:   None
*
* Used:		external only
*
* Returns:      1		eeprom checksum is ok
*               0		eeprom checksum is not ok
*
* Description:
*
*	This function figures out if the eeprom checksum is valid or not.
*
****+++***********************************************************************/

int nvm_chk_checksum()
{
    int		result;


    (void)ioctl(eeprom_fd, CHECK_CHKSUM, &result);
    return(result);

}


/****+++***********************************************************************
*
* Function:     nvm_clear()
*
* Parameters:   target			NVM_EEPROM or NVM_SCI
*		version			utility version number
*		title			optional title for sci
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_WRITE_ERROR		eeprom or sci write error
*
* Description:
*
*   In the NVM_EEPROM case, this function clears the eeprom and sets up the
*   version number (the utility's version number).
*
*   In the NVM_SCI case, this function calls a rtn to set up a new sci 
*   header and establish the version number and the title. This will
*   cause old sci data to be made invalid.
*
*   In either case, this is done in preparation for writing slot
*   configuration information.
*
****+++***********************************************************************/

nvm_clear(target, version, title)
    unsigned int	target;
    unsigned int	version;
    unsigned char	*title;
{
    int			rc;


    if (target == NVM_EEPROM) {
	rc = ioctl(eeprom_fd, CLEAR_EEPROM, &version);
	if (rc != 0)
	    return(NVM_WRITE_ERROR);
	return(NVM_SUCCESSFUL);
    }

    sci_write_header(version, title);
    return(NVM_SUCCESSFUL);

}


/****+++***********************************************************************
*
* Function:     nvm_read_slot()
*
* Parameters:   slot_number		slot number to read
*		buffer			target buffer
*		source
*		   NVM_CURRENT		use the global nvm_current 
*		   NVM_EEPROM
*		   NVM_SCI
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_INVALID_SLOT	Invalid slot number
*   		NVM_INVALID_EEPROM	Nonvolatile memory corrupt
*   		NVM_EMPTY_SLOT		Slot is empty
*
* Description:
*
*   The nvm_read_slot function will return slot specific information
*   either from EISA eeprom or from an sci file.
*
****+++***********************************************************************/

nvm_read_slot(slot_number, buffer, source)
    unsigned int		slot_number;
    nvm_slotinfo		*buffer;
    unsigned int		source;
{
    int				rc;
    struct eeprom_slot_data	iobuf;


    if (source == NVM_CURRENT)
	source = nvm_current;

    if (source == NVM_EEPROM) {

	/* do the ioctl to get the data */
	iobuf.slot_info = slot_number;
	rc = ioctl(eeprom_fd, READ_SLOT_INFO, &iobuf);
	if (rc != 0) {
	    if (errno == ENXIO)
		return(NVM_INVALID_SLOT);
	    if (errno == ENOENT)
		return(NVM_EMPTY_SLOT);
	    return(NVM_INVALID_EEPROM);
	}
    
	/* fill in the target buffer from what we got from the ioctl */
	buffer->functions = iobuf.number_of_functions;
	(void)memcpy((void *)&buffer->fib, (void *)&iobuf.function_info, 1);
	buffer->checksum = iobuf.checksum;
	(void)memcpy((void *)buffer->boardid, (void *)&iobuf.slot_id, 4);
	(void)memcpy((void *)&buffer->revision, (void *)&iobuf.major_cfg_rev, 2);
	(void)memcpy((void *)&buffer->dupid, (void *)&iobuf.slot_info, 1);

	return(NVM_SUCCESSFUL);

    }

    return(sci_read_slot(slot_number, buffer));

}


/****+++***********************************************************************
*
* Function:     nvm_read_function()
*
* Parameters:   slot_number		slot number to read
*		function_number		function number within slot
*		buffer			target buffer
*		source			NVM_EEPROM/NVM_SCI/NVM_CURRENT
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_INVALID_FUNCTION    Invalid function number
*   		NVM_INVALID_EEPROM	Nonvolatile memory corrupt
*
* Description:
*
*   The nvm_read_function function will return function specific
*   information either from EISA eeprom or from an sci file.
*
****+++***********************************************************************/

nvm_read_function(slot_number, function_number, buffer, source)
    unsigned int	slot_number;
    unsigned int	function_number;
    nvm_funcinfo	*buffer;
    unsigned int	source;
{
    struct func_slot	input;
    int			rc;


    if (source == NVM_CURRENT)
	source = nvm_current;

    if (source == NVM_EEPROM)  {

	/* do the ioctl to set up for the read */
	input.function = function_number;
	input.slot = slot_number;
	rc = ioctl(eeprom_fd, READ_FUNCTION_PREP, &input);
	if (rc != 0)
	    return(NVM_INVALID_EEPROM);
    
	/* now do the actual read to get the data */
	rc = read(eeprom_fd, (char *)buffer, sizeof(struct eeprom_function_data));
	if (rc != sizeof(struct eeprom_function_data))
	    return(NVM_INVALID_EEPROM);
	return(NVM_SUCCESSFUL);

    }
	
    return(sci_read_function(slot_number, function_number, buffer));

}


/****+++***********************************************************************
*
* Function:     nvm_write_slot()
*
* Parameters:   slot_number		slot number to write
*		buffer			target buffer
*		length			size of buffer (bytes)
*		target			NVM_EEPROM or NVM_SCI
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_WRITE_ERROR		Failure to write to eeprom or sci file
*
* Description:
*
*   The nvm_write_slot function will write the slot configuration
*   information either to EISA eeprom or to an sci file.
*
****+++***********************************************************************/

nvm_write_slot(slot_number, buffer, length, target)
    unsigned int	slot_number;
    unsigned char	*buffer;
    unsigned int	length;
    unsigned int	target;
{
    int			rc;


    if (target == NVM_EEPROM)  {

	/* do the ioctl to set up for the write */
	rc = ioctl(eeprom_fd, WRITE_SLOT_PREP, &slot_number);
	if (rc != 0)
	    return(NVM_WRITE_ERROR);
    
	/* now do the actual write */
	rc = write(eeprom_fd, (void *)buffer, length);
	if (rc != length)
	    return(NVM_WRITE_ERROR);

	return(NVM_SUCCESSFUL);

    }

    sci_write_slot(slot_number, buffer, length);
    return(NVM_SUCCESSFUL);

}


/****+++***********************************************************************
*
* Function:     sci_init()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      NVM_INVALID_SCI		file was read, but is not an sci file
*		NVM_SCI_CANNOT_OPEN	could not read file
*		NVM_SUCCESSFUL		successful
*
* Description:
*
*   The sci_init function will attempt to initialize an internal 
*   buffer (sci_hdr) from the sci file (sci_name). The cfg file stuff
*   is not read and stored here.
*
****+++***********************************************************************/

static sci_init()
{
    FILE		*sci_handle;
    struct stat		buf;
    int			err;


    /***********************
    * If we already have an sci file that we've handled, release its memory
    * and forget about it.
    ***********************/
    if (sci_hdr) {
	mn_trapfree((void *)sci_hdr);
	sci_hdr = NULL;
	sci_length = 0;
    }

    /*******************
    * Try to open the sci file. Get outta here if the open fails.
    *******************/
    sci_handle = fopen(sci_name, "rb");
    if (sci_handle == 0)
	return(NVM_SCI_CANNOT_OPEN);

    /*******************
    * Prepare to read the header information from the sci file.
    * Allocate some space for the header information.
    *******************/
    sci_length = sizeof(sci_header);
    sci_hdr = (sci_header *)mn_trapcalloc(1, sci_length);

    /********************
    * Read the header from the file and stuff it into our new buffer.
    * Do same basic validity checks on the header.
    ********************/
    err = fread((void *)sci_hdr, sci_length, 1, sci_handle);
    if ( (err == 0) || (sci_hdr->signature != SCI_SIGNATURE) )  {
	(void)fclose(sci_handle);
	return(NVM_INVALID_SCI);
    }

    /*****************
    * Read the rest of the sci file into the buffer. If we have a rev1
    * sci file, the size of the file is only determinable by doing a stat.
    * Otherwise, we keep the length of the config part of the sci file in
    * the header.
    *****************/
    if (sci_hdr->revision == UTIL_REVISION_1) {
	(void)stat(sci_name, &buf);
	sci_length = buf.st_size; 
    }
    else if (sci_hdr->revision == UTIL_REVISION_2) 
	sci_length = sci_hdr->length;
    else {
	(void)fclose(sci_handle);
	return(NVM_INVALID_SCI);
    }
    sci_hdr = (sci_header * )mn_traprealloc((void *)sci_hdr, sci_length);
    err = fread((void *)(&((char *)sci_hdr)[sizeof(sci_header)]),
		 sci_length - sizeof(sci_header), 1, sci_handle);
    (void)fclose(sci_handle);
    if (err == 0)
	return(NVM_INVALID_SCI);

    /*****************
    * Calculate a checksum for the data we just read and make sure that it
    * is the same as what was saved in the header when the file was written.
    *****************/
    if (sci_checksum(sci_hdr, sci_length) == sci_hdr->checksum)
	return(NVM_SUCCESSFUL);
    return(NVM_INVALID_SCI);

}


/****+++***********************************************************************
*
* Function:     sci_flush
*
* Parameters:   None              
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL		successful
*		other			the errno from the file call that failed
*
* Description:
*
*   The sci_flush function will attempt to write out the internal eeprom
*   buffer to the sci file specified in the global sci_name.
*
*   The CFG files that are used in this configuration are also copied to
*   the end of the sci file. Note that only one copy of a given file will
*   be saved to the sci file.
*
****+++***********************************************************************/

sci_flush()
{
    FILE        	*sci_handle;
    int 		err;
    sci_record		*sci_rec;


    /************
    * Add an eof record to the end of the global sci_hdr. Stuff the length
    * into the header and calculate (and save) a checksum.
    ************/
    sci_hdr = (sci_header *)mn_traprealloc((void *)sci_hdr,
					   sci_length + sizeof(sci_record));
    sci_rec = (sci_record *)((unsigned char *)sci_hdr + sci_length);
    sci_rec->slot_num = SCI_EOF;
    sci_rec->length = 0;
    sci_length += sizeof(sci_record);
    sci_hdr->length = sci_length;
    sci_hdr->checksum = sci_checksum(sci_hdr, sci_length);

    /************
    * Open the target sci file.
    ************/
    sci_handle = fopen(sci_name, "wb");
    if (sci_handle == 0)
	return(errno);

    /************
    * Write the configuration data out.
    ************/
    if (fwrite((void *)sci_hdr, sci_length, 1, sci_handle) == 0) {
	err = errno;
	(void)fclose(sci_handle);
	return(err);
    }

    /**************
    * Build the cfg portion of the file and write it out. If there were
    * any errors, no cfg data will be written.
    **************/
    sci_write_cfg_recs(sci_handle);

    /**************
    * Close the sci file and exit normally.
    **************/
    (void)fclose(sci_handle);
    return(NVM_SUCCESSFUL);

}


/****+++***********************************************************************
*
* Function:     sci_write_header()
*
* Parameters:   version		utility version
*		title		system board title
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The sci_write_header function updates the header information for the
*   current sci file.  The information is updated in the buffer but not
*   actually written to the file. If there is already a header, it is
*   freed up before we begin.
*
****+++***********************************************************************/

static void sci_write_header(version, title)
    unsigned int	version;
    unsigned char	*title;
{

    /*********************
    * If there is already a header, free it and forget about it.
    *********************/
    if (sci_hdr)
	mn_trapfree((void *)sci_hdr);

    /********************
    * Grab the space needed for the header. Write all header info into the
    * space. Initialize all of the sci globals.
    ********************/
    sci_length = sizeof(sci_header);
    sci_hdr = (sci_header * )mn_trapcalloc(1, sci_length);
    sci_hdr->signature = SCI_SIGNATURE;
    sci_hdr->revision = version;
    (void)sprintf((char *)sci_hdr->title, "NAME=\"%.80s\"", title);

}


/****+++***********************************************************************
*
* Function:     sci_read_slot()
*
* Parameters:   slot_number		slot to get info for
*		buffer			target buffer
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_INVALID_EEPROM	sci file is invalid
*   		NVM_INVALID_SLOT	slot number is invalid
*   		NVM_EMPTY_SLOT	        slot is empty
*
* Description:
*
*   The sci_read_slot function compiles slot information from an sci
*   file for a particular slot.  This function is the counterpart to the
*   nvm_read_slot function.
*
****+++***********************************************************************/

static sci_read_slot(slot_number, buffer)
    unsigned int	slot_number;
    nvm_slotinfo	*buffer;
{
    unsigned int    	function;
    unsigned int	err;
    unsigned char   	*bptr;
    sci_record	 	*sci_rec;
    nvm_funcinfo    	fbuf;


    /**************
    * Find the sci record corresponding to this slot. If there is none,
    * exit with an error.
    **************/
    sci_rec = sci_locate(slot_number);
    if (sci_rec == (sci_record *)NULL)
	return(NVM_INVALID_SLOT);

    /**************
    * If the record we read shows that this is an empty slot, exit.
    **************/
    if (sci_rec->length == 12)
	return(NVM_EMPTY_SLOT);

    /*********************
    * Set up a ptr to the first byte of the record. Get some of the
    * basics and stuff them into the target buffer.
    *********************/
    bptr = (unsigned char *)sci_rec + sizeof(sci_record);
    (void)memcpy((void *)buffer->boardid, (void *)bptr, sizeof(buffer->boardid));
    buffer->dupid = *(nvm_dupid *)&bptr[4];
    buffer->revision = sci_hdr->revision;

    /********************
    * The checksum is the last two bytes of the record (LSB first).
    ********************/
    buffer->checksum = *(unsigned char *)&bptr[sci_rec->length - 2] +
                       (*(unsigned char *)&bptr[sci_rec->length - 1] << 8);

    /*******************
    * Construct the board's fib value by or'ing together the fib from each
    * of the functions on the board.
    *******************/
    for (function=0, err=0 ; !err ; function++) {
	err = sci_expand_function (bptr, &fbuf, (int)function);
	if (err == NVM_SUCCESSFUL)
	    *(unsigned char *) &buffer->fib |= *(unsigned char *) & fbuf.fib;
    }

    /******************
    * Fill in the remaining slotinfo fields and reset err if we just ran
    * out of functions in the previous loop.
    ******************/
    buffer->fib.disable = 0;
    buffer->functions = function - 1;
    if (err == NVM_INVALID_FUNCTION)
	err = NVM_SUCCESSFUL;

    return(err);
}


/****+++***********************************************************************
*
* Function:     sci_read_function
*
* Parameters:   slot_number		slot number of board in question
*		function_number		function number within board
*		buffer			target buffer
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL		Successful
*   		NVM_INVALID_FUNCTION	function number is invalid
*
* Description:
*
*   The sci_read_function function compiles function information from an
*   sci file for a particular function.  This function is the equivalent
*   of the nvm_read_function function. The slot number must have already
*   been validated before calling this function.
*
****+++***********************************************************************/

static sci_read_function(slot_number, function_number, buffer)
    unsigned int	slot_number;
    unsigned int	function_number;
    nvm_funcinfo	*buffer;
{
    unsigned char   	*bptr;
    sci_record	 	*sci_rec;


    sci_rec = sci_locate(slot_number);

    bptr = (unsigned char *)sci_rec + sizeof(sci_record);
    return(sci_expand_function(bptr, buffer, (int)function_number));

}


/****+++***********************************************************************
*
* Function:     sci_write_slot()
*
* Parameters:   slot_number		slot to write
*		buffer			slot data
*		length			of slot data
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The sci_write_slot function writes the configuration information for
*   a particular slot to the sci file.  This is the equivalent of the
*   nvm_write_slot function that writes the information to EISA eeprom.
*   Note that this function writes all data for a slot (including function
*   data) -- there is no sci_write_function() function.
*
****+++***********************************************************************/

static void sci_write_slot(slot_number, buffer, length)
    unsigned int	slot_number;
    unsigned int	length;
    unsigned char	*buffer;
{
    unsigned char   	*bptr;
    sci_record	 	*sci_rec;
    unsigned int	length1;


    /**************
    * Get some more space for this new slot. Update the current length
    * and get a ptr to the new record.
    **************/
    if ((length % 4) == 0)
	length1 = length;
    else
	length1 = length + 4 - (length % 4);
    sci_hdr = (sci_header *)mn_traprealloc((void *)sci_hdr,
    				sci_length + sizeof(sci_record) + length1);
    sci_rec = (sci_record *)((unsigned char *)sci_hdr + sci_length);
    sci_length += sizeof(sci_record) + length1;

    /**************
    * Set up the record proper.
    **************/
    sci_rec->slot_num = slot_number;
    sci_rec->length = length;

    /**************
    * Copy the input buffer into the sci buffer.
    **************/
    bptr = (unsigned char *)sci_rec + sizeof(sci_record);
    (void)memset((void *)bptr, 0, length1);
    if (length != 0)
	(void)memcpy((void *)bptr, (void *)buffer, length);

}


/****+++***********************************************************************
*
* Function:     sci_locate()
*
* Parameters:   slot_number		slot to locate
*
* Used:		internal only
*
* Returns:      NULL			record could not be found
*		ptr			to a slot record
*
* Description:
*
*   The sci_locate function locates the sci record belonging to the slot
*   in slot_number.
*
****+++***********************************************************************/

static sci_record *sci_locate(slot_number)
    unsigned int	slot_number;
{
    sci_record	 	*sci_rec;


    /********************
    * Set sci_rec to the first slot record (skip the header).
    ********************/
    sci_rec = (sci_record *)((unsigned char *)sci_hdr+sizeof(sci_header));

    /********************
    * Walk through each of the slot records in the sci data structure until
    * we get a slot record with a matching slot number or we hit eof.
    ********************/
    while (sci_rec->slot_num != SCI_EOF) {
	if (sci_rec->slot_num == slot_number)
	    return(sci_rec);
	sci_rec = (sci_record *) ( (unsigned char *)sci_rec +
	                              sizeof(sci_record) + sci_rec->length);
 	if ( ((unsigned)sci_rec % 4) != 0)
	    sci_rec = (sci_record *)((unsigned)sci_rec + 4 - ((unsigned)sci_rec % 4));
    }

    /*******************
    * Did not find a match so return a NULL record.
    *******************/
    return((sci_record *)NULL);

}





/****+++***********************************************************************
*
* Function:     sci_checksum()
*
* Parameters:   hdr		pointer to the sci buffer
*		length		size of hdr to calculate the checksum for
*
* Used:		internal only
*
* Returns:      the checksum for the sci data
*
* Description:
*
*    Calculates a checksum for the sci data (excluding the signature and
*    checksum fields from sci_header, of course).
*
****+++***********************************************************************/

static int sci_checksum(hdr, length)
    sci_header	   	*hdr;
    unsigned int	length;
{
    unsigned int    	index;
    unsigned int    	sum;
    unsigned char   	*bptr;


    sum = 0;
    index = sizeof(hdr->signature) + sizeof(hdr->checksum);
    bptr = (unsigned char *)hdr + index;
    while (index < length) {
	sum += *bptr++;
	index++;
    }

    return(sum);
}


/****+++***********************************************************************
*
* Function:     sci_expand_function()
*
* Parameters:   buffer		slot data
*		func_info	receiving buffer
*		func_number	function to expand
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL		Successful
*		NVM_INVALID_FUNCTION    couldn't find this function
*
* Description:
*
*   The sci_expand_function function expands the configuration record
*   for a specific function of a slot configuration block. This is called
*   to get the function data out of an sci buffer.
*
****+++***********************************************************************/

static int sci_expand_function (buffer, func_info, func_number)
    BYTE		*buffer;
    nvm_funcinfo	*func_info;
    int			func_number;
{
    unsigned char	*bptr;
    unsigned char	*bptr2;
    int			count;
    int			count1;
    int			func_len;
    int			i;
    nvm_fib		fib;
    nvm_irq		*irqp;
    nvm_dma		*dmap;
    nvm_memory		*memoryp;
    nvm_port		*portp;
    nvm_init		*initp;
    unsigned char	temp;


    /***************
    * Set up a ptr into the input buffer and write the slot stuff into the
    * target buffer.
    ***************/
    bptr = buffer;
    func_info->boardid[0] = *bptr++;
    func_info->boardid[1] = *bptr++;
    func_info->boardid[2] = *bptr++;
    func_info->boardid[3] = *bptr++;
    (void)memcpy((void *)&func_info->dupid, (void *)bptr, 2);
    bptr += 2;
    func_info->minor_cfg_ext_rev = *bptr++;
    func_info->major_cfg_ext_rev = *bptr++;

    /***************
    * Advance the ptr until we get to
    * the information for the function in question.
    * If we run out of data before we find the function in question, return
    * with an error. At the end of this clause, bptr will point to the 
    * first byte of the function data for the correct function (after the
    * function length field.
    ***************/
    count = bptr - buffer;
    for (i=0 ; i<func_number ; i++)  {
	temp = *bptr++;
	func_len = temp + (*bptr++ << 8);
	count += func_len + 2;
	bptr += func_len;
    }
    temp = *bptr++;
    func_len = temp + (*bptr++ << 8);
    if (func_len == 0)
	return(NVM_INVALID_FUNCTION);
	    
    /***************
    * Now we will fill in the target buffer with the information for the
    * function that was requested. First do the selects and the fib.
    ***************/
    (void)memcpy((void *)func_info->selects, (void *)(bptr+1), (size_t)*bptr);
    bptr += *bptr + 1;
    (void)memcpy((void *)&(func_info->fib), (void *)bptr, 1);
    bptr++;
    fib = func_info->fib;
    
    /***************
    * Now handle the type string (if one was stored).
    ***************/
    if (fib.type) {
	(void)memcpy((void *)func_info->ftype, (void *)bptr, (size_t)(*bptr+1));
	bptr += *bptr + 1;
    }

    /***************
    * If we have freeform data, copy it over.
    ***************/
    if (fib.data) {
	(void)memcpy((void *)func_info->u.freeform, (void *)bptr, (size_t)(*bptr+1));
	bptr += *bptr + 1;
    }

    /*************
    * Otherwise, handle each of the standard resource types if their fib bit
    * indicates that they were stored.
    *************/
    else {
	if (fib.memory) {
	    memoryp = (nvm_memory *)bptr;
	    count = 1;
	    while (memoryp->more) {
		count++;
		memoryp++;
	    }
	    count = NVM_MEMORY_SIZE * count;
	    (void)memcpy((void *)&func_info->u.r.memory[0], (void *)bptr, (size_t)count);
	    bptr += count;
	}
	if (fib.irq) {
	    irqp = (nvm_irq *)bptr;
	    count = 1;
	    while (irqp->more) {
		count++;
		irqp++;
	    }
	    count = NVM_IRQ_SIZE * count;
	    (void)memcpy((void *)&func_info->u.r.irq[0], (void *)bptr, (size_t)count);
	    bptr += count;
	}
	if (fib.dma) {
	    dmap = (nvm_dma *)bptr;
	    count = 1;
	    while (dmap->more) {
		count++;
		dmap++;
	    }
	    count = NVM_DMA_SIZE * count;
	    (void)memcpy((void *)&func_info->u.r.dma[0], (void *)bptr, (size_t)count);
	    bptr += count;
	}
	if (fib.port) {
	    portp = (nvm_port *)bptr;
	    count = 1;
	    while (portp->more) {
		count++;
		portp++;
	    }
	    count = NVM_PORT_SIZE * count;
	    (void)memcpy((void *)&func_info->u.r.port[0], (void *)bptr, (size_t)count);
	    bptr += count;
	}
	if (fib.init) {
	    initp = (nvm_init *)bptr;
	    bptr2 = bptr;
	    count = 0;
	    while (initp->more) {
		count1 = nvm_init_size(initp);
		count += count1;
		bptr2 += count1;
		initp = (nvm_init *)bptr2;
	    }
	    count1 = nvm_init_size(initp);
	    count += count1;
	    (void)memcpy((void *)&func_info->u.r.init[0], (void *)bptr, (size_t)count);
	}
    }
      
    return(NVM_SUCCESSFUL);

}


/****+++***********************************************************************
*
* Function:     dump_iodc_slot()
*
* Parameters:   slot_number		slot to dump to the file
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   This function takes a slot number and writes a iodc format record of
*   it to the file specified in the global iodc_name.
*
****+++***********************************************************************/

void dump_iodc_slot(slot_number)
    unsigned int			slot_number;
{
    FILE				*handle;
    int 				nbytes;
    short				func_base; 
    short				size; 
    int					nfuncs;
    unsigned char			*data;
    unsigned char			func_info;
    struct eeprom_per_slot_info		slot_info;
    sci_record          		*sci_rec;


    /****************
    * Get a pointer to the sci record for this slot. Exit with
    * an error if we have a problem.
    ****************/
    sci_rec = sci_locate(slot_number);
    if (sci_rec == (sci_record *)NULL) {
	err_handler(IODC_BAD_SLOT_ERRCODE);
	return;
    }

    /****************
    * Set up a pointer to the data past the sci record.
    * If this is an empty slot, get outta here.
    ****************/
    data = (unsigned char *) (sci_rec + 1);
    nbytes = sci_rec->length;
    if (nbytes == 12) {
	err_handler(IODC_BAD_SLOT_ERRCODE);
	return;
    }

    /****************
    * Calculate the number of functions and the combined fib value.
    * At the end, skip over the end of function indicator (a function
    * length of 0).
    ****************/
    for (nfuncs = 0, func_base = 8, func_info = 0;
	 size = data[func_base] + (data[func_base + 1] << 8);
	 nfuncs++, func_base += size + 2) {
	func_info |= data[func_base + 2 + data[func_base + 2] + 1];
    }
    func_base += 2;

    /***************
    * Build the slot info part of the record.
    ***************/
    slot_info.slot_id = (data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
    slot_info.slot_info = data[4];
    slot_info.slot_features = data[5];
    slot_info.minor_cfg_ext_rev = data[6];
    slot_info.major_cfg_ext_rev = data[7];
    slot_info.number_of_functions = nfuncs;
    slot_info.cfg_data_size = nbytes - 10;
    slot_info.function_info = func_info;
    slot_info.checksum = data[func_base++];
    slot_info.checksum += data[func_base++] << 8;
    slot_info.cfg_data_offset = sizeof(struct eeprom_per_slot_info);
    slot_info.write_count = 0;

    /*************
    * Now write the slot_info followed by the function data, and get outta here.
    *************/
    handle = fopen(iodc_name, "w");
    if (handle == 0) {
	err_handler(IODC_WRITE_PROBLEM_ERRCODE);
	return;
    }
    if (!fwrite((void *)&slot_info, sizeof(struct eeprom_per_slot_info), 1, handle)) {
	err_handler(IODC_WRITE_PROBLEM_ERRCODE);
	return;
    }
    if (!fwrite((void *)&data[8], (unsigned)(nbytes-10), 1, handle)) {
	err_handler(IODC_WRITE_PROBLEM_ERRCODE);
	return;
    }
    if (fclose(handle) != 0) {
	err_handler(IODC_WRITE_PROBLEM_ERRCODE);
	return;
    }

    /**************
    * If we got here, we were successful, so tell them so.
    **************/
    err_handler(IODC_SUCCESSFUL_ERRCODE);
    return;

}


/****+++***********************************************************************
*
* Function:     sci_init_cfg_file()
*
* Parameters:   None
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   This function initializes the cfg files currently in use (empty).
*
****+++***********************************************************************/

void sci_init_cfg_file()
{
    int		i;

    for (i=0 ; i<MAX_NUM_SLOTS ; i++)
	if (sci_cfg_file_names[i] != NULL) {
	    mn_trapfree((void *)sci_cfg_file_names[i]);
	    sci_cfg_file_names[i] = NULL;
	}
}





/****+++***********************************************************************
*
* Function:     sci_add_cfg_file()
*
* Parameters:   slotnum			slot number cfg file corresponds to
*		filename		full cfg file name
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   This function adds an entry to the cfg file list for a particular slot.
*
****+++***********************************************************************/

void sci_add_cfg_file(slotnum, filename)
    int			slotnum;
    char		*filename;
{
    char		*ptr;


    ptr = (char *)mn_trapcalloc(1, strlen(filename)+1);
    (void)strcpy(ptr, filename);
    sci_cfg_file_names[slotnum] = ptr;
}


/****+++***********************************************************************
*
* Function:     sci_remove_cfg_file()
*
* Parameters:   slotnum			slot number of cfg file to be removed
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   This function removes an entry for a particular slot number.
*
****+++***********************************************************************/

void sci_remove_cfg_file(slotnum)
    int		slotnum;
{
    mn_trapfree((void *)sci_cfg_file_names[slotnum]);
    sci_cfg_file_names[slotnum] = NULL;
}






/****+++***********************************************************************
*
* Function:     sci_move_cfg_file()
*
* Parameters:   curslotnum		current slot number
*		newslotnum		new slot number
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   This function moves the entry for one slot to another. The current slot
*   entry is subsequently removed.
*
****+++***********************************************************************/

void sci_move_cfg_file(curslotnum, newslotnum)
    int		curslotnum;
    int		newslotnum;
{
    sci_cfg_file_names[newslotnum] = sci_cfg_file_names[curslotnum];
    sci_cfg_file_names[curslotnum] = NULL;
}


/****+++***********************************************************************
*
* Function:     sci_make_cfg_file()
*
* Parameters:   boardid			boardid of board in question
*		filename		CFG filename (what to name it)
*
* Used:		external only
*
* Returns:      0			successful
*		-1			no file made
*
* Description:
*
*     This function tries to make a CFG file from data stored in the
*     /etc/eisa/system.sci file. If the board in question is not part
*     of the configuration in the SCI file, this will fail.
*
****+++***********************************************************************/

int sci_make_cfg_file(boardid, filename)
    unsigned int	boardid;
    char		*filename;
{
    sci_header		*hdr;
    sci_cfg_record 	*rec;
    FILE		*sci_handle;
    FILE		*fd;
    unsigned int	length;
    int			err;
    unsigned char	*ptr;
    struct stat		buf;


    /*******************
    * Try to open the sci file. Get outta here if the open fails.
    * Stat the file and save the file size.
    *******************/
    sci_handle = fopen(SCI_DEFAULT_NAME, "rb");
    if (sci_handle == 0)
	return(-1);
    (void)stat(SCI_DEFAULT_NAME, &buf);

    /*******************
    * Read the sci header into buffer. If the read failed, or the file is not
    * of the right format, or the sci file is of the original format (no
    * cfg files in the sci file) get outta here. We will also fall out here
    * if the length in the first header is the same as the size of the file.
    * This could happen if the fwrite for the cfg stuff fails (after the
    * config stuff has already been written).
    *******************/
    hdr = (sci_header *)mn_trapcalloc(1, sizeof(sci_header));
    err = fread((void *)hdr, sizeof(sci_header), 1, sci_handle);
    if ( (err == 0) || (hdr->signature != SCI_SIGNATURE) ||
	 (hdr->revision != UTIL_REVISION_2) ||
	 (hdr->length == buf.st_size) ) {
	mn_trapfree((void *)hdr);
	(void)fclose(sci_handle);
	return(-1);
    }

    /********************
    * Now read the second header (for the cfg files' data).
    ********************/
    err = fseek(sci_handle, (long)hdr->length, SEEK_SET);
    if (err != 0) {
	mn_trapfree((void *)hdr);
	(void)fclose(sci_handle);
	return(-1);
    }
    err = fread((void *)hdr, sizeof(sci_header), 1, sci_handle);
    if (err == 0) {
	mn_trapfree((void *)hdr);
	(void)fclose(sci_handle);
	return(-1);
    }

    /********************
    * Read in all of the cfg files' data.
    ********************/
    length = hdr->length;
    hdr = (sci_header *)mn_traprealloc((void *)hdr, length);
    err = fread((void *)(&((char *)hdr)[sizeof(sci_header)]),
		length-sizeof(sci_header), 1, sci_handle);
    (void)fclose(sci_handle);
    if ( (err == 0) || (sci_checksum(hdr, length) != hdr->checksum) )  {
	mn_trapfree((void *)hdr);
	return(-1);
    }

    /********************
    * Set rec to the first slot record (skip the header).
    ********************/
    rec = (sci_cfg_record *)((unsigned char *)hdr + sizeof(sci_header));

    /********************
    * Walk through each of the cfg records in the sci data structure until
    * we get a boardid match or we hit the second eof. If we find a match,
    * write the cfg data out to the specified file.
    ********************/
    while (rec->boardid != SCI_EOF) {
	if (rec->boardid == boardid) {
	    fd = fopen(filename, "wb");
	    if (fd == 0)
		break;
	    ptr = (unsigned char *)((unsigned char *)rec + sizeof(sci_cfg_record));
	    err = fwrite((void *)ptr, (size_t)rec->length, 1, fd);
	    (void)fclose(fd);
	    mn_trapfree((void *)hdr);
	    if (err == 0)
		return(-1);
	    return(0);
	}
	rec = (sci_cfg_record *) ( (unsigned char *)rec +
				  sizeof(sci_cfg_record) + rec->length);
 	if ( ((unsigned)rec % 4) != 0)
	    rec = (sci_cfg_record *)((unsigned)rec + 4 - ((unsigned)rec % 4));
    }

    /*******************
    * Did not find a match so return a bad status.
    *******************/
    mn_trapfree((void *)hdr);
    return(-1);

}


/****+++***********************************************************************
*
* Function:     sci_write_cfg_recs
*
* Parameters:   sci_fd			sci file descriptor
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   This function writes the cfg records to the end of the sci file.
*   It is assumed that the file has already been created and that the 
*   configuration records have already been written to it.
*   Note that only one copy of a given file will be saved to the sci file.
*
****+++***********************************************************************/

static void sci_write_cfg_recs(sci_fd)
    FILE		*sci_fd;
{
    FILE		*fd;
    int 		err;
    int			i, j;
    unsigned int	length;
    int			l, l1;
    unsigned char	dups[MAX_NUM_SLOTS];
    char		base_name[10];
    sci_cfg_record	*rec;
    sci_header		*hdr;
    struct stat		buf;


    /************
    * Figure out where the duplicate cfg file names are (if any) and save
    * them away.
    ************/
    for (i=0 ; i<MAX_NUM_SLOTS ; i++)
	dups[i] = 0;
    for (i=1 ; i<MAX_NUM_SLOTS ; i++)
	for (j=i+1 ; j<MAX_NUM_SLOTS ; j++)
	    if (strcmp(sci_cfg_file_names[i], sci_cfg_file_names[j]) == 0) {
		dups[i] = 1;
		break;
	    }

    /*****************
    * Allocate space for a header.
    *****************/
    length = sizeof(sci_header);
    hdr = (sci_header *)mn_trapcalloc(1, length);

    /*****************
    * For each of the cfg files that were used in the configuration, try to
    * copy them (along with a describing record) at the end of the header    
    * buffer we are building. If we cannot read a given cfg file, we will
    * skip it.
    *****************/
    for (i=0 ; i<MAX_NUM_SLOTS ; i++)  {

	if ( (sci_cfg_file_names[i] != NULL) && (dups[i] == 0) )  {

	    /******************
	    * Open the cfg file and get its size. If the size is not an
	    * even multiple of 4, calculate the next highest multiple of 4.
	    ******************/
	    fd = fopen(sci_cfg_file_names[i], "rb");
	    if (fd == 0)
		continue;
	    (void)stat(sci_cfg_file_names[i], &buf);
	    l = buf.st_size;
	    if ((l % 4) == 0)
		l1 = l;
	    else
		l1 = l + 4 - (l % 4);

	    /****************
	    * Expand the buffer by the size of the sci_cfg_record and the
	    * number of bytes to write for the cfg file. Update the overall
	    * length and set the rec ptr to the new record.
	    ****************/
	    hdr = (sci_header *)mn_traprealloc((void *)hdr,
	    			length + sizeof(sci_cfg_record) + l1);
	    rec = (sci_cfg_record *)((unsigned char *)hdr + length);
	    (void)memset((void *)rec, 0, sizeof(sci_cfg_record) + l1);
	    length += sizeof(sci_cfg_record) + l1;

	    /****************
	    * Update the new record and read the cfg file into the buffer.
	    * If we get an error here (low likeliehood since the open worked),
	    * we will just release the buffer and exit without writing any
	    * cfg records out.
	    ****************/
	    get_cfg_file_base_name(sci_cfg_file_names[i], base_name);
	    err = make_board_id(base_name, &rec->boardid);
	    if (err != 0)  {
		mn_trapfree((void *)hdr);
		return;
	    }
	    rec->length = l;
	    err = fread((void *)((char *)rec+sizeof(sci_cfg_record)),
	                (size_t)l, 1, fd);
	    if (err == 0) {
		mn_trapfree((void *)hdr);
		return;
	    }

	}

    }

    /************
    * Add an eof record to the end of the buffer. Stuff the length
    * into the header and calculate (and save) a checksum.
    ************/
    hdr = (sci_header *)mn_traprealloc((void *)hdr,
					   length + sizeof(sci_cfg_record));
    rec = (sci_cfg_record *)((unsigned char *)hdr + length);
    rec->boardid = SCI_EOF;
    rec->length = 0;
    length += sizeof(sci_cfg_record);
    hdr->signature = SCI_SIGNATURE;
    (void)sprintf((char *)hdr->title, "CFG data follows!!");
    hdr->length = length;
    hdr->checksum = sci_checksum(hdr, length);

    /************
    * Write the cfg data out and free the buffer.
    ************/
    (void)fwrite((void *)hdr, (size_t)length, 1, sci_fd);
    mn_trapfree((void *)hdr);
    return;

}
