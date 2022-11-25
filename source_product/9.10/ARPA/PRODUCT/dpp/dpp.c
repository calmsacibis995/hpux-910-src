/********************************************************************/
/*                                                                  */
/*                     Main program of dpp program                  */
/*                                                                  */
/*                                                                  */
/* Entry name :  dpp                                                */
/*                                                                  */
/*                                                                  */
/* ARGUMENTS :   argv[1]  = dpp file name.                          */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                - l <log_file> : log in file (create if doesn't   */
/*                                 exist).                          */
/*                                                                  */
/*                - c   : check only dpp_file.                      */
/*                                                                  */
/*                - k   : remove all device_files.                  */
/*                                                                  */
/*                - p   : ocd directory (default: /etc)             */
/*                                                                  */
/* PURPOSE : that main program is in charge of scanning  the        */
/*           dpp_file and starting all processes with corresponding */
/*           entry parameters.                                      */
/*                                                                  */
/*                                                                  */
/********************************************************************/


/*
*
* include system library
*
*/
#include <sys/termio.h>
#include <sys/getaccess.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <utmp.h>

/*
* Imported module
*/

#include "display_msgx.h"
#include "processx.h"
#include "decodex.h"
#include "processx.h"
#include "store_loginx.h"

#include "dpp_const.h"
#include "display_msg.h"


/*
*  Revision code and copyright written in code
*
*/
char    copyright_o[]= "Copyright (c) 1992, HEWLETT-PACKARD COMPANY";
char    iden_c[] = "@(#)$Header: dpp.c,v 1.1.109.1 92/04/02 15:11:20 gomathi Exp $";



main(argc, argv)
int	argc;
char 	*argv[];
{
/*
*
* option data storage.
*
*/
unsigned	short	int	dpp_v_option_purge_device,
				dpp_v_option_log_on_console,
				dpp_v_option_only_check_file;
char				dpp_v_file_name[200],
				dpp_v_ocd_directory[200];



/*
*  general data
*
*/
unsigned	int	 status;

/*
* parameter for process handling
*
*/
unsigned	int		O_IP_address;
char				O_port_nb[200], O_board_nb[200];
char				O_device_name[200],
				O_config_file[200];
struct		stat		buf;

/*
*
* data for file handling
*
*/
char		work_buffer[10000];
int  		fp_dpp;
char            *O_start_pointer,*v_last_new_line_pointer;
register	short 	int	length;
unsigned	int	I_buffer_size, v_global_length;


	/*
	*  Start of main program
	*
	*  First decode options 
	*
	*/
	utmpname("/etc/utmp.dfa");
	status = dec_f_decode_option (  argc,
				        argv,
					&dpp_v_option_purge_device,
					&dpp_v_option_only_check_file,
					dpp_v_file_name,
					dpp_v_ocd_directory);
	if (status != status_ok) {
		/*
		* message has been logged by procedure
		*
		*/
		exit (0);
	}

	/*
	*
	* Then open dpp_file to scan entries 
	*
	* we are using a buffer of 10Kbytes
	* That buffer is stored with the different pages of the
        * file.
	*
	*
	* open the file 
	*/
	fp_dpp = open (dpp_v_file_name, O_RDONLY);
	if (fp_dpp == -1) {
		dis_p_display_msg(dpp_c_bad_dpp_file);
		exit (0);
	}
	/*
	* then read the first 10K page
	*
	*/
	v_global_length = 0;
	sto_p_init_login_entries();
	do {
		/*
		* read one block of data
		*/
		length = read (fp_dpp,
				work_buffer,
				sizeof(work_buffer)-1);
       		O_start_pointer = work_buffer;
		/*
		* To be able to use string scanning ,
		* First replace all '\n' with '\0'
		*/
		I_buffer_size = length;
		while (length-- != 0) {
			if ( *O_start_pointer++ == End_of_line ) {
				*(O_start_pointer -1) = End_of_string;
				/*
				* remain last "new_line char"
				*
				* point on next char after End_of_line
				*/
				v_last_new_line_pointer 
						= O_start_pointer;
			}
		}
		length = I_buffer_size;
		/*
		* do not cut in the middle of a line
		* length = size until last end of line.
		*/
		if (I_buffer_size == (sizeof(work_buffer)-1)) {
			I_buffer_size =  (v_last_new_line_pointer
						         - work_buffer);
			/*
			* set file pointer to first data not
			* processed in work_buffer for next read
			*/ 
			lseek ( fp_dpp,
		       		(long) (v_global_length + I_buffer_size),
		       		SEEK_SET );	
			v_global_length = (v_global_length + I_buffer_size);
		}
		/*
		*
		* mark the end of the buffer speparatly from EOF
		* as EOF is used for each entry.
		*/
		work_buffer[I_buffer_size] = End_of_buffer;
	
       		O_start_pointer = work_buffer;
		/*
		* get all reverse entry to start process til we reach
		* the  end_of_buffer char.
		* 
		* dec_f_get_next_reverse_entry return all entries
		* that can be programmatically accessed.
		* if return status != 0 --> no more valid entry.
		*/
		while (*O_start_pointer != End_of_buffer) {

 	               switch ( dec_f_get_next_reverse_entry (   &O_start_pointer,
							&O_IP_address,
							O_port_nb,
							O_board_nb,
							O_device_name,
							O_config_file ) )
			{
			case 0:
				break;
			case 1:   /* this a login entry */
				if (dpp_v_option_only_check_file == FALSE) {
					sto_p_store_login_entry (O_IP_address,
								 O_port_nb,
						 		 O_board_nb,
								 O_device_name);
				}
				break;

			case 2:   /* this is a prog entry */
				pro_p_purge_device_file (O_device_name,
							 dpp_v_option_purge_device,
							 dpp_v_ocd_directory );

				if (    (dpp_v_option_only_check_file == FALSE)
				     && ( stat (O_device_name, &buf) == -1) ) {
			
					/*
					* create reverse telnet process
					* that procedure direct log errors 
					* if process cannot be started.
					* 
					*/
					pro_p_create_rtelnet_process (  O_IP_address,
									O_port_nb,
									O_board_nb,
									O_device_name,
									O_config_file,
									dpp_v_ocd_directory );
				}
				break;
			}
		}
	} while (length == (sizeof(work_buffer)-1));
	sto_p_save_login_file();
}



