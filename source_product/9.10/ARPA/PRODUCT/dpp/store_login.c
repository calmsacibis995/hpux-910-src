/********************************************************************/
/*  MODULE : store login entries                                    */
/*                                                                  */
/*                                                                  */
/*  PURPOSE : prepare login file that will be used dynamically      */
/*                                                                  */
/*  routines :                                                      */
/*                                                                  */
/*        sto_p_init_login_entries : prepare work to login_file     */
/*                                                                  */
/*                                                                  */
/*        sto_p_store_login_entry  : store basic entry.             */
/*                                                                  */
/*                                                                  */
/*        sto_p_save_login_file    : save all login  entries.       */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/********************************************************************/
/*                                                                  */
/*                                                                  */
/*  AUTHOR : Jean-philippe CARADEC               DATE : 12/91       */
/********************************************************************/
#include <sys/termio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>

#include "store_login.h"
/*
* imported module
*/
#include "display_msg.h"
#include "display_msgx.h"
#include "dpp_const.h"

#define         sto_s_max_login_entries         "2000"
#define		sto_s_dpp_login_file_name      "/etc/dpp_login.bin"

/*
* a buffer is created composed of the h_coding table plus extra 
* bytes for version number, statistics, and length
* Following are the blocks of data that are linked to
* the H-coding table. For a same H-coding key , the block of data
* composed of IP address, port, board and device name are linked together
*/

static 	struct 		sto_t_login_file  	sto_v_login_file_buffer;
static  unsigned	short	int		sto_v_next_index;

/********************************************************************/
/*                                                                  */
/*  Procedure :    sto_p_init_login_entries()                       */
/*                                                                  */
/*                                                                  */
/*  parameters : none                                               */
/*                                                                  */
/*                                                                  */
/*  purpose : init buffer with version number and init first index  */ 
/*            set all index of H_coding table to zero.              */
/*                                                                  */
/*                                                                  */
/********************************************************************/
extern
/*-------------------------------------------------------------
  * sto_p_init_login_entries
  *-------------------------------------------------------------*/
sto_p_init_login_entries() 

{
unsigned	short	int	index;	
	/*
	*
	* Init login_file_buffer
	*
	*/
	for (index=0; index < (sto_c_max_h_coding_entries); index++) {
		/*
		* init all index with zero
		*/
		sto_v_login_file_buffer.h_coding_first_index[index] = 0;
	}
	sto_v_next_index = 1;
        sto_v_login_file_buffer.version_number = sto_c_current_version_number;
}

/********************************************************************/
/*  Procedure : sto_p_store_login_entry                             */ 
/*                                                                  */
/*                                                                  */
/*  Purpose :  get all parameters.                                  */
/*             - build the H_coding key                             */
/*             - get  the next index available                      */
/*             - store info in the block of data		    */
/*             - link the block to the last block having the same   */
/*               H_coding key.                                      */
/*                                                                  */
/*                                                                  */
/*   Parameters:                                                    */
/*                                                                  */
/*        INPUT :  I_IP_address  : integer.                         */
/*                                                                  */
/*                 I_port_number : string of chars		    */
/*                                                                  */
/*                 I_board_number : string of chars		    */
/*                                                                  */
/*                 I_device_name : string of chars                  */
/*                                                                  */
/*        OUTPUT : none                                             */
/********************************************************************/
extern
/*-------------------------------------------------------------
  * sto_p_store_login_entry
  *-------------------------------------------------------------*/
sto_p_store_login_entry (I_IP_address,
			 I_port_string,
                         I_board_string,
			 I_device_name )

unsigned		int	I_IP_address;
char				*I_port_string, *I_board_string,
				*I_device_name;

{
unsigned	short	int	H_coding_key, next_index, prev_index;
unsigned		int	depth;
	/*
	*
	* display error message if max of entries reached.
	*
	*/

	if (sto_v_next_index > sto_c_max_login_entries) {
		dis_p_display_msg (dpp_c_too_many_login_entries,
				   sto_s_max_login_entries,
				   I_device_name );
		return(0);
	}
	/*
	* store info in the block of data
	*
	*/
	sto_v_login_file_buffer.entry_ptr[sto_v_next_index].IP_address 
						= I_IP_address;
	/*
	* if the port is not defined , add info.
	*/
	if (sscanf(I_port_string,"%d",
		&(sto_v_login_file_buffer.entry_ptr[sto_v_next_index].port_number)) != 1)
		{
		sto_v_login_file_buffer.entry_ptr[sto_v_next_index].status.port_defined = FALSE;
		sto_v_login_file_buffer.entry_ptr[sto_v_next_index].port_number = 0;
	}
	else    {
		sto_v_login_file_buffer.entry_ptr[sto_v_next_index].status.port_defined = TRUE;
	}
	/*
	* if the board is not defined , add info.
	*/
	if (sscanf(I_board_string,"%d",
		&(sto_v_login_file_buffer.entry_ptr[sto_v_next_index].board_number)) != 1)
		{
		sto_v_login_file_buffer.entry_ptr[sto_v_next_index].status.board_defined = FALSE;
		sto_v_login_file_buffer.entry_ptr[sto_v_next_index].board_number = 0;
	}
	else 	{
		sto_v_login_file_buffer.entry_ptr[sto_v_next_index].status.board_defined = TRUE;
	}
	/*
	*  add device name limited to 35 chars.
	*/

	strncpy (sto_v_login_file_buffer.entry_ptr[sto_v_next_index].device_name,
		I_device_name,
		(sto_c_max_device_name_length) );
	sto_v_login_file_buffer.entry_ptr[sto_v_next_index]
            .device_name[sto_c_max_device_name_length] = End_of_string;
	/*
	* Now link the block to the ;last block having the same H_coding key
	*
	* first determine h_coding_key 
	*
	*/
	H_coding_key =  (  (I_IP_address % 256) % 32 )
			* ( (sto_v_login_file_buffer.entry_ptr[sto_v_next_index].board_number % 3)* 8
			 + (( sto_v_login_file_buffer.entry_ptr[sto_v_next_index].port_number % 8) + 1));

	/*
	* then get last block index to fill its link_index field
	* with the current block index.
	*/	
        next_index = sto_v_login_file_buffer
				.h_coding_first_index[H_coding_key];
	if (next_index == 0) {
		depth = 1;
		sto_v_login_file_buffer
				.h_coding_first_index[H_coding_key]= sto_v_next_index;
	}
	else	{
		depth = 0;
		prev_index = 0;
		while (next_index != 0)
			{
			depth++;
                        prev_index = next_index;
			next_index = sto_v_login_file_buffer.entry_ptr[next_index].link_index;
        	}
        	sto_v_login_file_buffer.entry_ptr[prev_index].link_index = sto_v_next_index;
  	}
	/*
	* some statistics on the max number of links.
	*/
	if (depth > sto_v_login_file_buffer.max_depth) {
		sto_v_login_file_buffer.max_depth = depth;
	}
	/*
	* prepare index for next entry
	*/
	sto_v_next_index++;
}

/********************************************************************/
/*                                                                  */
/*  Procedure :     sto_p_save_login_file()                         */
/*                                                                  */
/*  Purpose : if login entries then create the file that will       */
/*            be accessed  on line by telnetd.                      */
/*                                                                  */
/*                                                                  */
/*  PARAMETERS:  none.                                              */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/********************************************************************/
extern
/*-------------------------------------------------------------
  * sto_p_save_login_file
  *-------------------------------------------------------------*/
sto_p_save_login_file() 

{
int 	fdc;
unsigned 	int	length;
	if  (sto_v_next_index != 1) {
	
	        if ( (fdc = open (sto_s_dpp_login_file_name,
				  O_WRONLY | O_CREAT ,
				  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )) == -1) {
			dis_p_display_msg (dpp_c_cannot_access_dpp_login_file,
					    sto_s_dpp_login_file_name );
			return(0);
	        }
		sto_v_login_file_buffer.length = (unsigned  int)
		&(sto_v_login_file_buffer.entry_ptr[sto_v_next_index])
		 - (unsigned  int) &(sto_v_login_file_buffer); 
		/*
		* lock all file when file is modified to prevent simultaneous
		* file access from TELNETD.
		*
		*/
		lockf (fdc, F_LOCK, 0);
		write (fdc,
			&sto_v_login_file_buffer, 
		        sto_v_login_file_buffer.length);
		/*
		* Once the write is done
		*/
		lockf (fdc, F_ULOCK, 0);
		close (fdc);
	}
} 

