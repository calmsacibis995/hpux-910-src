/********************************************************************/
/*                                                                  */
/*     MODULE : PROCESS                                             */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*    PURPOSE : handle device file removal and process creation     */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*    Routines :                                                    */
/*                                                                  */
/*                 pro_p_purge_device_file : remove device name     */
/*                                           if required.           */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                 pro_p_create_rtelnet_process : create process.   */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*   AUTHOR : JP CARADEC                        DATE : 11/21/91     */
/*                                                                  */
/********************************************************************/

#include <sys/termio.h>
#include <sys/getaccess.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <utmp.h>
#include <unistd.h>
#include <sys/sysmacros.h>

/*
* Imported module
*/
#include "display_msg.h"
#include "display_msgx.h"
#include "dpp_const.h"





extern
/*-------------------------------------------------------------
  * pro_p_purge_device_file
  *-------------------------------------------------------------*/
pro_p_purge_device_file (I_device_name,
			 option_purge_device,
			 I_program)
char		*I_device_name, *I_program;
unsigned	short	int	option_purge_device;
{
struct	stat	buf;
char		s_command[80], *device;
struct	utmp	utmp, *oldu;
	/*
	* check if that device_file already exist.
	*
	*/
	if (stat (I_device_name, &buf) == 0) {
		/*
		*
		*
		* check first if the ocd process which used or is
		* using the /dev/... has an entry in the utmp file
		* comapre only the last name and not the complete path.
		*/
		if ( (device = strrchr (I_device_name,'/')) != NULL) {
			device++;
		}
		else  device = I_device_name;
		/*
		* open the utmp file
		*/
		setutent();
		strncpy (utmp.ut_line, device, sizeof(utmp.ut_line));
		/*
		* now check if the /dev/... still in utmp file
		*/
		if ( (oldu = getutline(&utmp)) != (struct utmp *) 0) {
			/*
			* the /dev/ is either being used or the ocd 
			* process didn't have time to erase the entry.
			*/
			if (option_purge_device != 0) {
				/*
				* the purge option 
				* delete the device file.
				*/
				if (major(buf.st_rdev) == 17) {
					if (unlink(I_device_name) == -1) {
			  		 	dis_p_display_msg(dpp_c_cannot_purge_device_file,
					     			I_device_name,
					     			I_program);
					}
				}
			}
			else {
				/*
				* no purge option: delete the file 
				* only if the process (pid) doesn't
				* exist. 17 is the major number
				* for PTY slave driver.
				*/
				if (kill (oldu->ut_pid, 0) != 0) {
					if (major(buf.st_rdev) == 17) {
						if (unlink(I_device_name) == -1) {
			  		 		dis_p_display_msg(dpp_c_cannot_purge_device_file,
					     				I_device_name,
					     				I_program);
						}
					}
					else {
			  		     dis_p_display_msg(dpp_c_cannot_purge_device_file,
					      			I_device_name,
					     			I_program);
					}
				}
			}
			memset(utmp.ut_id, '\0', sizeof (utmp.ut_id));
			utmp.ut_type = DEAD_PROCESS;
			pututline(&utmp);
		}
		else {
			/*
			* check that the file is a PTY slave device file
			*/
			if (major(buf.st_rdev) == 17) {
				if (unlink(I_device_name) == -1) {
		 		 	dis_p_display_msg(dpp_c_cannot_purge_device_file,
					     		I_device_name,
					     		I_program);
				}
			}
			else {
	 		 	dis_p_display_msg(dpp_c_cannot_purge_device_file,
					     		I_device_name,
					     		I_program);
			}
		}
		endutent();
 	}

}
/********************************************************************/
/*                                                                  */
/*  Routine :     pro_p_create_rtelnet_process                      */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*  Purpose :  create a new process and start "ocd" program         */
/*                                                                  */
/*             with parameters found in dp file entry.              */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*   Input parameters :                                             */
/*                                                                  */
/*                 I_IP_address   :  IP address found in entry.     */
/*                 I_port_nb      :  port number string.            */
/*                 I_board_nb     :  board number string found.     */
/*                 I_device_name  :  device name string found.      */
/*                 I_config_file  :  config file string found.      */
/*                 I_ocd          :  program to start.              */
/*                                                                  */
/*   output parameters :   none.                                   */
/*                                                                  */
/********************************************************************/
extern
/*-------------------------------------------------------------
  * pro_p_create_rtelnet_process
  *-------------------------------------------------------------*/
pro_p_create_rtelnet_process (  I_IP_address,
				I_port_nb,
				I_board_nb,
				I_device_name,
				I_config_file,
				I_ocd)

unsigned		int	I_IP_address;
char				*I_port_nb, *I_board_nb;
char				*I_device_name, *I_config_file, *I_ocd;

{
char		s_ip[20], s_device[80],s_board[80],
		s_port[80],s_config[80]			;
int		ocd_pid, mode;

	mode = getaccess (I_ocd,
	          	UID_EUID,
		  	NGROUPS_EGID_SUPP,
		  	(int*) 0,
		  	(void *) 0,
		  	(void *) 0);
	/*
	* file must exist and must be an executable file
	* and we must be able to execute it.
	*/
	if (	(mode == -1)
	     || (!(mode & X_OK)) ) {
		dis_p_display_msg(dpp_c_cannot_execute_default_ocd,
			  I_ocd, I_device_name);
		return;
	}
	/*
	*
	* First create a process
	*
	*/
	if ( (ocd_pid = fork()) < 0) {
		dis_p_display_msg (dpp_c_cannot_create_process,
				   I_device_name);
		exit(0);
	}

        if (ocd_pid == 0) {
		/*
		* we are the child , exec the ocd program
		* giving all attributes.
		*
		*/
		setpgrp(); /* becomes a process group leader*/
		sprintf(s_ip,"-n%s", inet_ntoa(I_IP_address));
		sprintf(s_device,"-f%s", I_device_name);
		sprintf(s_board,"-b%s", I_board_nb);
		sprintf(s_port,"-p%s",I_port_nb);
		sprintf(s_config,"-c%s",I_config_file);
		execl  (I_ocd,
                	"ocd",
			s_ip,
			s_device,
			s_board, 
			s_port,
			s_config,
			(char*) 0   );
		/* statement should not be reached */
		dis_p_display_msg(dpp_c_cannot_execute_ocd_program,
				  I_ocd);
		exit(0);

	}

	return (0);		 
}
