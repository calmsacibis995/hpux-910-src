/********************************************************************/
/*                                                                  */
/*                                                                  */
/*   MODULE : decode.                                               */
/*                                                                  */
/*                                                                  */
/*   PURPOSE : decode parser arguments and decode dpp file and      */
/*             each programmatic entry                              */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*   external routines:                                             */
/*                                                                  */
/*            dec_f_decode_option : decode arguments.               */
/*                                                                  */
/*            dec_f_get_next_reverse_entry : get all prog  entries  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*    AUTHOR : JP CARADEC                           DATE : 11/20/91 */
/*                                                                  */
/********************************************************************/
#include <sys/termio.h>
#include <sys/getaccess.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>



#include "display_msg.h"
#include "display_msgx.h"
#include "dpp_const.h"


#define		dpp_s_option_purge_device	"-k"
#define		dpp_s_option_log_on_console	"-v"
#define		dpp_s_option_ocd_directory	"-p"
#define		dpp_s_option_only_check_file    "-c"
#define		dpp_s_option_log_file		"-l"

/********************************************************************/
/*                                                                  */
/*   Procedure : dec_f_decode_options.                              */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*   parameters:                                                    */
/*   +++++++++++                                                    */
/*                                                                  */
/*       INPUT :   I_argc, I_argv : user arguments                  */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*       OUTPUT :    O_option_purge_device : purge device file      */
/*                   O_option_only_check_file: check file only.     */
/*                   O_ocd_directory: optional directory to search  */
/*                                    ocd program.                  */
/*                   O_dpp_file_name: dpp_file name.                */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*  ALgo:  decode all known option by starting with log_file option */
/*       ignore all other fields than known ones.                   */
/*       check that options are valid and exit if one wrong.        */
/*                                                                  */
/********************************************************************/
extern
/*-------------------------------------------------------------
  * dec_f_decode_option
  *-------------------------------------------------------------*/
dec_f_decode_option (    argc,
			 argv,
			 O_option_purge_device,
			 O_option_only_check_file,
			 O_file_name,
			 O_directory_file)

int		argc;
char		*argv[];
unsigned	short	int	*O_option_purge_device,
			 	*O_option_only_check_file ;
char				*O_file_name, *O_directory_file;
{
char				s_argument[200];	/* store all arguments*/
char				log_file[200]; 		/* store log_file name*/
char				option_parm[200];	/* parm linked to option*/
char				ocd_directory[200];	/* store ocd prog*/

unsigned	int		total_length, length;
int				n_items_found, index, pid, mode;
struct		stat		buf;			/* struct for stat routine */

FILE				*stream;		/* used to check log_file*/

	/*
	*
	* default value and init.
	* 
	*
	*/
	total_length = 0;
	strcpy (O_directory_file,"/etc/ocd");

	/*
	*
	* If no argument defined ==> error.
	*
	*/
	if (argc <= 1) {
		dis_p_display_msg(dpp_c_error_bad_parameter);
		exit(0);
	}
	/*
	*
	* Check file logging info in case of invalid arguments
	* create a complete line of command to use scanf function.
	*
	*/
	
	for (index = 1 ; index <= argc; index++) {

		length = sprintf (&s_argument[total_length],
				  " %s", argv[index] );
		total_length = total_length + length;
	}
	s_argument[total_length] = End_of_string;

	/*
	* Start scannin g s_argument to get different options
	*
	* begin with log_file argument.
	*
	*/
	
	if (dec_f_find_string (s_argument,
			       dpp_s_option_log_file,
				option_parm) == 0) {
	
		n_items_found = sscanf (option_parm, "%s",log_file);

		if (n_items_found != 1 ) {
			/*
			* log file name not specified 
			*
			* 
			*/
			dis_p_display_msg(dpp_c_no_log_file);
			exit(0);
		}
		else {
			/*
			* check log_file and redirect error to log_file
			*
			*/
			mode = getaccess (log_file,
				          UID_EUID,
					  NGROUPS_EGID_SUPP,
					  (int*) 0,
					  (void *) 0,
					  (void *) 0);
			/*
			* if file exist: check that we can  access it
			*/

			if (mode != -1) {
				if (    (mode & W_OK)
				    && !(mode & X_OK) ) {
					freopen (log_file,"a",stderr);
				}
				else {
					dis_p_display_msg(dpp_c_cannot_access_log_file,
							  log_file);
					exit(0);
				}
			}
			/*
			* if file doesn't exist try to create it
			*/
			else {
			      if ( (stream = fopen (log_file,"a")) != NULL) {
					fclose	(stream);
					freopen (log_file,"a",stderr);
			      }
			      else {
					dis_p_display_msg(dpp_c_cannot_create_log_file,
							  log_file);
					exit(0);
				}
				

			}
		}

	}
	/*
	*
	* Then try to decode "check only file" option
	*
	*/
	
	if (dec_f_find_string (s_argument,
			       dpp_s_option_only_check_file,
			       option_parm) == 0) {
		*O_option_only_check_file = TRUE;
	}		
	else {
		*O_option_only_check_file = FALSE;
	}

	/*
	*
	* Then decode ocd program to start. check option only
	* if it will be used. (check_only_file not up
	*
	*/
	if (*O_option_only_check_file == FALSE ) {
		if (dec_f_find_string (s_argument,
			       dpp_s_option_ocd_directory,
				option_parm) == 0) {
	
			n_items_found = sscanf (option_parm, "%s",ocd_directory);

			if (n_items_found != 1 ) {
				/*
				* ocd not specified 
				*/
				dis_p_display_msg(dpp_c_no_ocd_program);
				exit(0);
			}
			else {
				mode = getaccess (ocd_directory,
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
					dis_p_display_msg(dpp_c_cannot_execute_ocd_program,
							  ocd_directory);
					exit(0);
				}
				else  {
					strcpy (O_directory_file, ocd_directory);
				}
			}
		}
	}
	/*
	*
	* Then decode purge option
	*
	*/
	
	if (dec_f_find_string (s_argument,
			       dpp_s_option_purge_device,
				option_parm) == 0) {
		*O_option_purge_device = TRUE;
	}
	else {
		*O_option_purge_device = FALSE;

	}

	/*
	* Now decode the dpp_file name
	*
	* it must the first argument before options.
	*/
	if (argv[1][0] != '-') {
		sprintf (O_file_name,"%s",argv[1]);
		mode = getaccess (O_file_name,
			          UID_EUID,
				  NGROUPS_EGID_SUPP,
				  (int*) 0,
				  (void *) 0,
				  (void *) 0);
		if (	(mode == -1)
		     || !(mode & R_OK)
		     ||  (mode & X_OK)  ) {
			dis_p_display_msg(dpp_c_bad_dpp_file,
					  O_file_name);
			exit(0);
		}

	}
	else {
		dis_p_display_msg(dpp_c_no_dpp_file);
		exit(0);
	}		
	return(0);
}
/******************************************************************************/
/*                                                                            */ 
/*  PROCEDURE: dec_f_find_string                                              */ 
/*                                                                            */ 
/*                                                                            */ 
/*  PURPOSE:  recognize a string of char(argument 1) and store in argument 3  */ 
/*            the next char string.                                           */ 
/*                                                                            */ 
/*   argument 1 : string to recognize                                         */ 
/*   argument 2 : buffer of char.                                             */ 
/*   argument 3 : next string.                                                */ 
/*                                                                            */ 
/* WRITTEN: GND lab                                      DATE : 13/03/90     */ 
/******************************************************************************/
dec_f_find_string (buffer, base_string, final_string)
char buffer[1000], base_string[80], *final_string;

{
int index_buffer, index_buff_int, index_string, index_final, ret;


index_buffer   = 0;
index_string   = 0;
index_buff_int = 0;
index_final    = 0;
/*
*
*  recognize the string base_string in the buffer
*
*/
while  (   ( buffer[index_buffer] != '\0' )
        && ( base_string[index_string] != '\0')
       )
             {
             if (buffer[index_buffer] == base_string[index_string] )
                     {
                     index_buffer = index_buffer + 1; 
                     index_string = index_string + 1;
                     
                     }
              else
                     {
                     index_string = 0;
                     index_buffer = index_buff_int + 1;
                     index_buff_int = index_buffer;
                     }
             }
/*
*
* In case of string "base_string" completly analysed 
* consider that string found :
* otherwise return "string_not_found"
*
*/
if  ( base_string[index_string] != '\0' )
              {
              return (1);
              }
final_string[0] = '\0';           /* In case of buffer ending with string*/
/*
*
* Now fill the destination string with the next string in buffer
*
*/
while (    buffer[index_buffer] != '\n'
       &&  buffer[index_buffer] != '\0'
       &&  index_buffer != 999 
      )
              {
               /*
               *
               * Remove all ' ' and tabulation stop string when char is ' ' or '\n'
               */
               if (    (buffer[index_buffer] != ' ' )
                    && (buffer[index_buffer] != '\t') )
		       {
                        final_string[index_final] = buffer[index_buffer];
                        index_final++;
                        index_buffer++;
                       }
               else
                       {
                        if (index_final != 0 )
                             {
                             index_buffer = 999;
                             }
                        else
                             {
                             index_buffer++;
                             }
                       }
               }
final_string[index_final] = '\0';
return (0);

}

/********************************************************************/
/*                                                                  */
/*   routine  :      dec_f_get_next_reverse_entry                   */
/*                                                                  */
/*                                                                  */
/*   purpose  :  decode valid programmatic entries.                 */
/*               store error messages in case of invalid entries    */
/*                                                                  */
/*                                                                  */
/*                                                                  */
/*   IN parameters 	: none                                      */
/*                                                                  */
/*   IN/OUT parameters  :   O_start_pointer   : pointer of string.  */
/*                                                                  */
/*   OUT parameters     :   O_IP_address      : valid IP address    */
/*                                                                  */
/*                          O_port_nb         : valid port string.  */
/*                                                                  */
/*                          O_board_nb        : valid board string. */
/*                                                                  */
/*                          O_device_name     : valid device name.  */
/*                                                                  */
/*                          O_config_file     : valid config file   */
/*                                                                  */
/*    return  values:       0 no more entry.                        */
/*                                                                  */
/*                          1 login entry.                          */
/*                                                                  */
/*                          2 prog entry.                          */
/*                                                                  */
/*  AUTHOR : JP CARADEC                        DATE : 11/20/91      */
/********************************************************************/

extern
/*-------------------------------------------------------------
  * dec_f_get_next_reverse_entry
  *-------------------------------------------------------------*/
dec_f_get_next_reverse_entry (  O_start_pointer,
				O_IP_address,
				O_port_nb,
				O_board_nb,
				O_device_name,
				O_config_file )

char				**O_start_pointer;
unsigned	int		*O_IP_address;
char				*O_port_nb, *O_board_nb;
char				*O_device_name, *O_config_file;
{

unsigned	int	invalid_entry, v_1, v_2, v_3, v_4;
int			v_number_of_arguments, value, svalue, index, return_status;
char			O_port_board_nb[200];


struct 		stat		buf;
	/*
	* Use sscanf to get the multiple variables
	*/
	do {
	    invalid_entry = FALSE;
	    for (;;) { /* use a for loop to be able to use the break */
		v_number_of_arguments = sscanf (*O_start_pointer,
				       		" %u.%u.%u.%u %s %s %s",
	 					&v_1,&v_2,&v_3,&v_4,
						O_port_board_nb,
						O_device_name,
						O_config_file );
		/*
		*  IP address  must be the first argument.
		*/
		if (v_number_of_arguments >= 4) {
			/*
			* at least 4 fields composing the IP address 
			*/
			if (    (v_1 < 256 )
	     		     && (v_2 < 256 )
	     	             && (v_3 < 256 )
	     	             && (v_4 < 256 ) ) {
				*O_IP_address = (v_1*256*256*256)+
						(v_2*256*256) +
						(v_3*256) +
						 v_4 ;
			}
			else {
				dis_p_display_msg (dpp_c_bad_ip_address,
						    *O_start_pointer);
				invalid_entry = TRUE;
				break;
			}
		}
		else {
		        /*
			* if string null or string starting 
			* with a # character 
			* then ignore entry and goto next string.
			*/
			if (   (v_number_of_arguments == -1)
			    || (    (v_number_of_arguments == 0)
				&&  (**O_start_pointer == comment_char) ) ) {
				 	invalid_entry = TRUE;
					break;
			}
			else {
		             dis_p_display_msg (dpp_c_bad_ip_address,
					        *O_start_pointer);
		     	     invalid_entry = TRUE;
			     break;
			}
		}
		if (v_number_of_arguments >= 5) {

			/* 5 arguments means that at least one field
			* is available after the IP address.
			*	
			* Check port values: accept digits or only x chars
			*/
			index = 0;
			/*
			* check separator and remove it
			*/
			do {
				if (O_port_board_nb[index] == '/') {
					 O_port_board_nb[index] = ' ';
				}
				 index++;
			} while ( O_port_board_nb[index] != End_of_string);

			svalue = sscanf (O_port_board_nb,"%s%s",
					O_board_nb,
					O_port_nb );
			/*
			* port and board have been separated
			*/
			/*
			* check that board is a decimal value
			* or is composed of only 'X' or 'x'
			* chars.
			*/
			if (sscanf(O_board_nb,"%d",&value) != 1) {
			    		index = 0;
				while (   (O_board_nb[index] != End_of_string)
			       		&&(   (O_board_nb[index] == don_t_care_char)
			        	   || (O_board_nb[index] == don_t_care_CHAR ) ) ){
					index++;
				}
				if (O_board_nb[index] != End_of_string) {
					dis_p_display_msg (dpp_c_bad_board,
						    	*O_start_pointer);
					invalid_entry = TRUE;
					break;
				}
			}

			if (svalue == 2) {
				/*
				* check that port is a decimal value
				* or is composed of only 'X' or 'x'
				* chars.
				*/
				if (sscanf(O_port_nb,"%d",&value) != 1 ){
			     		index = 0;
			     		while (   (O_port_nb[index] != End_of_string)
			             		&&(   (O_port_nb[index] == don_t_care_char)
				        	   || (O_port_nb[index] == don_t_care_CHAR ) ) ){
						index++;
			     		}
			     		if (O_port_nb[index] != End_of_string) {
						dis_p_display_msg (dpp_c_bad_port,
					    	   		*O_start_pointer);
						invalid_entry = TRUE;
						break;
			     		}
				}

			}
			
			else {
				dis_p_display_msg (dpp_c_bad_port,
					    	   *O_start_pointer);
		      		invalid_entry = TRUE;
		      		break;
			}
		}
		else {
			dis_p_display_msg (dpp_c_port_board_missing,
				    	        *O_start_pointer);
		      	invalid_entry = TRUE;
		      	break;
		}

		if (v_number_of_arguments < 6) {
			/*
			* no device name mentionned
			*/
			dis_p_display_msg (dpp_c_device_name_missing,
					    *O_start_pointer);
			invalid_entry = TRUE;
			break;
		}
		if (v_number_of_arguments == 7) {
			/*
			*  Check config for right access
			*/
			if (O_config_file[0] != comment_char) {
				if (stat (O_config_file, &buf) != 0) {
					invalid_entry = TRUE;
					dis_p_display_msg (dpp_c_bad_config_file_name,
							   O_config_file);
					break;
				}
				else  {
					return_status = 2;
				}
			}
			else {
				/*
				* entry not available for prog access.
				*/
                        	return_status = 1 ; /* return a login entry */
				break;
			}	
		}
		else	{
			/*
			* entry not available for prog access.
			*/
	                return_status = 1 ; /* return a login entry */
			break;
		}
		break;
	     }  /* end of for */
		
	     /*
	     * if bad entry or remaining data to ignore
	     * then goto next end_of_line
	     */
	     while (*(*O_start_pointer)++ != End_of_string) {}

	     /*
	     * check end of buffer
	     */
	     if (   ( **O_start_pointer == End_of_buffer) ){
		    if (invalid_entry == TRUE) {
			return(0);
		    }
		    else {
                        return(return_status); /* entry found */
                    }
             }

	} while (invalid_entry == TRUE);
return(return_status);
}
