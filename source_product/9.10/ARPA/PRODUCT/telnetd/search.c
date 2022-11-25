/********************************************************************/
/*  MODULE :  get                                                   */
/*                                                                  */
/*                                                                  */
/*  PURPOSE : get device name associated to DTC parameters          */
/*                                                                  */
/*  routines :                                                      */
/*                                                                  */
/*        get_f_device_name                                         */
/********************************************************************/
/*                                                                  */
/*                                                                  */
/*  AUTHOR : Jean-philippe CARADEC               DATE : 12/91       */
/********************************************************************/
#include <sys/termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <utmp.h>
#include <errno.h>
#include <unistd.h>

#include <store_login.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <syslog.h>
/*
* imported module
*/
#include <dpp_const.h>


/*
* a buffer is created composed of the h_coding table plus extra 
* bytes for version number, statistics, and length
* Following are the blocks of data that are linked to
* the H-coding table. For a same H-coding key , the block of data
* composed of IP address, port, board and device name are linked together
*/


/********************************************************************/
/*                                                                  */
/*  Procedure :    get_f_device_name                                */
/*                                                                  */
/*                                                                  */
/*  parameters :                                                    */
/*                                                                  */
/*                                                                  */
/*  purpose : return device name not created if entry exists        */ 
/*                                                                  */
/*                                                                  */
/********************************************************************/
extern
/*-------------------------------------------------------------
  * get_f_device_name       
  *-------------------------------------------------------------*/
get_f_device_name(I_IP_address,
		  I_port_number,
		  I_board_number,
		  O_device_name)

char			*O_device_name;
unsigned	int	I_IP_address,
			I_port_number,
			I_board_number;
		

{
struct 		sto_t_login_file  	*login_buffer_ptr;
struct          sto_t_entry		*entry_pointer;

unsigned	short	int	H_coding_key;
register        unsigned	short	int     next_index;
unsigned		int	version_number, length;
			int		return_status;
			int		fdc;
struct		stat		buf;
FILE		*file_ptr;
struct		utmp 		utmp, *oldu; 
char 		*dev;

/*
*  macro definition to detect if one entry exist in dpp_login.bin file
*
*   input parms ip address, port number of board number.
*
*/
# define  check_valid_entry( get_IP_add, port_nb, board_nb, r_stat)\
	r_stat = -1;\
	H_coding_key =  (  (get_IP_add % 256) % 32 )\
			* ( (board_nb % 3)* 8 \
			 + ((port_nb % 8) + 1));\
        next_index = login_buffer_ptr->\
				h_coding_first_index[H_coding_key];\
	while (next_index != 0) {\
                entry_pointer = &(login_buffer_ptr->entry_ptr [next_index]);\
		if (    (entry_pointer->IP_address == get_IP_add )\
                     && (     (entry_pointer->status.port_defined == FALSE)\
			   || (entry_pointer->port_number == I_port_number )\
                        ) \
                     && (     (entry_pointer->status.board_defined == FALSE)\
			   || (entry_pointer->board_number  == I_board_number )\
		        )\
		   ) {\
			strcpy (O_device_name, entry_pointer->device_name );\
			if  (stat (O_device_name, &buf) != 0 ) {\
			     r_stat = 0;\
			     break;\
			}\
			else \
			    next_index = entry_pointer->link_index;\
		}\
		else \
		    next_index = entry_pointer->link_index;\
	}

/*****************************
*  
*  start of main procedure
*
*****************/
	

	if ( (fdc = open ("/etc/dpp_login.bin", O_RDONLY )) == -1) {
			return(-1);
	        }
	/*
	*
	* lock all file when file is modified to prevent simultaneous
	* file access from DPP program.
	*
	*/
	version_number = 0;
	length	= 0;
	lockf (fdc, F_LOCK, 0);
	read (fdc,
	      &version_number, 
	      sizeof(int));
	/*
	* check version number in file
	*/
        if    (version_number != sto_c_current_version_number) {
		lockf (fdc, F_ULOCK, 0);
		close (fdc);
		return (-1);
	}
	read (fdc,
	      &length, 
	      sizeof(int) );
	login_buffer_ptr = (struct sto_t_login_file *) malloc (length);

	lseek (fdc,0,0);
	read  (fdc,
	       login_buffer_ptr,
	       length );					

	lockf (fdc, F_ULOCK, 0);
	/*
	* now check if full entry exist in H-coding buffer.
	*/
         check_valid_entry      (I_IP_address,
				I_port_number,
				I_board_number,
				return_status)

        if (return_status != 0 ){
		  /*
		  * return_status is updated by macro "check_valid_entry"
		  * entry was not found : set port to zero in case
		  *   of X value (which representation is zero and
                  *  and a flag "port_defined" set to FALSE.). these
		  *  types of entries are using a H-coding key with
		  *  port set to zero.
		  *
		  */
	          check_valid_entry   (I_IP_address,
				       0,
				       I_board_number,
					return_status)

                  if (return_status !=0) {
		        /*
		         * return_status is updated by macro "check_valid_entry"
		         * entry was not found : set board to zero in case
		         *   of X value (which representation is zero and
                         *  and a flag "board_defined" set to FALSE.). these
		         *  types of entries are using a H-coding key with
		         *  board set to zero.
		         *
		         */

		        check_valid_entry    (I_IP_address,
				              I_port_number,
				              0,
					      return_status)
                        if (return_status != 0 ) {
		                /*
		  		 * return_status is updated by macro "check_valid_entry"
		                 * entry was not found : set board and port
				 * to zero in case
		                 *   of X/X value . these
		                 *  types of entries are using a H-coding key with
		                 *  board and port set to zero.
		                 *  CASE where an IP address specifies the port.
		                 */

			        check_valid_entry  (I_IP_address,
						    0,
					      	    0,
						    return_status)
		        }
                 }
        }

	lockf (fdc, F_ULOCK, 0);
 	free (login_buffer_ptr);
	close (fdc);
	
/* Changes for security hole problem - this ensures that only one
   session can be opened from one nailed port */

	if (stat ( O_device_name, &buf ) == 0 ) {
        	 dev = strrchr(O_device_name,'/');
		 dev++;
		 setutent();
		 strncpy(utmp.ut_line, dev, sizeof(utmp.ut_line));
		 if (( oldu = getutline(&utmp)) != ( struct utmp *) 0 ) {
			if ( kill(oldu->ut_pid,0) == 0 ){
	   			fprintf( stderr,"\n");
	   			fprintf( stderr, "%s is busy - Connection refused\n",dev);
				syslog(LOG_ERR,"%s is busy\n",dev);
				syslog(LOG_ERR,"Duplicate connection request from %s refused\n", dev);
	   			exit(1);
        		}
			else {
				unlink(O_device_name);
				memset(utmp.ut_id,'\0',sizeof(utmp.ut_id));
				utmp.ut_type = DEAD_PROCESS;
				pututline(&utmp);
				endutent();
				syslog(LOG_INFO,"%s unlinked and recreated\n",
				dev);
				return (0);
			}
		 }
		 else {
		 unlink(O_device_name);
	         syslog(LOG_INFO,"%s unlinked and recreated\n",dev);
		 return(0);
		 }
	}
/* End of change */
return(return_status);
}



