/******************************************************************************/
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/******************************************************************************/

#include <sys/termio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>


#include "display_msg.h"



/******************************************************************************/
/*   FUNCTION:  display_msg                                                   */
/*                                                                            */
/*                                                                            */
/* Input :  - error number  to decode.                                        */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
extern
/*-------------------------------------------------------------
 * dis_p_display_msg
 *-------------------------------------------------------------*/

dis_p_display_msg (error_nb, parm1, parm2)
int error_nb;
char	*parm1, *parm2;
{
/******************************************************************************/
/*                                                                            */
/*                    Variable   definition                                   */
/*                                                                            */
/******************************************************************************/
char  	error_msg[300];
int 	fd, length;
FILE	*fp, *fpuser;
long	tictoc;
char	*now;
/******************************************************************************/
/*                                                                            */
/*                    Start of display_msg.h routine.                          */
/*                                                                            */
/******************************************************************************/
tictoc=time(0);now=ctime(&tictoc);*(now+24)=0;                          /*BP1*/

	switch (error_nb) 
             {
            case  dpp_c_error_bad_parameter:
		length = sprintf (error_msg,"DPP %s (error 0): dp file is mandatory\n", now);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_no_dpp_file:
		length = sprintf (error_msg,"DPP %s (error 1): dp file must be the first argument.\n", now);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_bad_dpp_file:
		length = sprintf (error_msg,"DPP %s (error 2): Cannot read dp file (%s). \n", now, parm1);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_no_log_file:
		length = sprintf (error_msg,"DPP %s (error 3): No log file defined (-l option). \n", now);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_cannot_create_log_file:
		length = sprintf (error_msg,"DPP %s (error 4): Cannot create log file (-l %s). \n", now, parm1);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_cannot_access_log_file:
		length = sprintf (error_msg,"DPP %s (error 5): Cannot access log file (-l %s). \n", now, parm1);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_no_ocd_program:
		length = sprintf (error_msg,"DPP %s (error 6): No ocd file defined in program option. \n ", now);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

            case  dpp_c_cannot_execute_ocd_program:
		length = sprintf (error_msg,"DPP %s (error 7): Cannot execute ocd program (-p %s). \n ", now, parm1);
                sprintf (&error_msg[length],"   Usage : dpp <dp_file> -c -k -p <ocd_path> -l <log_file>\n\n" ); 
                break;

	    case  dpp_c_cannot_purge_device_file:
		length = sprintf (error_msg,"DPP %s (error 8): Cannot purge device file  (%s).\n ", now, parm1);
                sprintf (&error_msg[length]," ==>  '%s' process will not be created.\n\n", parm2 ); 
                break;

	    case  dpp_c_cannot_execute_default_ocd:
		length = sprintf (error_msg,"DPP %s (error 9): Cannot execute default program (%s).\n ", now, parm1);
                sprintf (&error_msg[length]," ==>  DTC Device File Access feature not started for (%s).\n\n",parm2); 
                break;

	    case  dpp_c_bad_ip_address:
                length = sprintf (error_msg,"DPP %s (error 10): Entry ignored (Bad IP address).\n",now);
                sprintf (&error_msg[length]," >>>>> %s <<<<<\n\n", parm1 ); 
                break;
	    case  dpp_c_port_board_missing:
                length = sprintf (error_msg,"DPP %s (error 11): Entry ignored (no port/board info).\n",now);
                sprintf (&error_msg[length]," >>>>> %s <<<<<\n\n", parm1 ); 
                break;
	    case  dpp_c_bad_port:
                length = sprintf (error_msg,"DPP %s (error 12): Entry ignored (Bad port number).\n",now);
                sprintf (&error_msg[length]," >>>>> %s <<<<<\n\n", parm1 ); 
                break;
	    case  dpp_c_bad_board:
                length = sprintf (error_msg,"DPP %s (error 13): Entry ignored (Bad board number).\n",now);
                sprintf (&error_msg[length],"  >>>>> %s <<<<<\n\n", parm1 ); 
                break;

	    case  dpp_c_cannot_create_process:
                length = sprintf (error_msg,"DPP %s (error 14): No more process available on system.\n",now);
                sprintf (&error_msg[length]," ==>  '%s' device file will not be available for programmatic access.\n\n", parm1 ); 
                break;

            case  dpp_c_device_name_missing:
                length = sprintf (error_msg,"DPP %s (error 15): Entry ignored (no device_name)\n",now);
                sprintf (&error_msg[length]," >>>>> %s <<<<<\n\n", parm1 ); 
                break;
            case  dpp_c_bad_device_name:
                length = sprintf (error_msg,"DPP %s (error 16): Entry ignored (Bad device_name)\n",now);
                sprintf (&error_msg[length]," >>>>> %s <<<<<\n\n", parm1 ); 
                break;
            case  dpp_c_bad_config_file_name:
                length = sprintf (error_msg,"DPP %s (error 17): Entry ignored  (Bad config_name)\n",now);
                sprintf (&error_msg[length]," >>>>> %s <<<<<\n\n", parm1 ); 
                break;

            case  dpp_c_too_many_login_entries:
                length = sprintf (error_msg,"DPP %s (error 30): Too many login entries  (limited to %s).\n",now, parm1);
                sprintf (&error_msg[length]," Port identification feature for '%s' device file name is disable.\n\n", parm2 ); 
                break;

            case  dpp_c_cannot_access_dpp_login_file:
                length = sprintf (error_msg,"DPP %s (error 31): Cannot access dpp login file (%s).\n",now, parm1);
                sprintf (&error_msg[length]," Port identification feature is disable (Bad installation).\n\n"); 
                break;

        default:
		sprintf (error_msg,"DPP %s : Unknown error (%d)\n\n", now, error_nb);
		break;
       }

/*
*
* The error message is now built and can then be stored in
*  the log_file defined stderr.
* 
*/
fwrite(error_msg, sizeof(char),strlen(error_msg),stderr);
fflush (stderr);
return (0);
}


