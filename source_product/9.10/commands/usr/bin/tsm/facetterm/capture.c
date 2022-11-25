/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: capture.c,v 70.1 92/03/09 15:40:38 ssa Exp $ */
#include <sys/types.h>
#include "stdio.h"
#include "wins.h"

		/**********************************************************
		* For each window, 
		*	the name of the capture file, and
		*	the file descriptor of the capture file if capture
		*	is active.
		**********************************************************/
#define CAPTURE_DIRECTORY_MAX	255
#define CAPTURE_FILENAME_MAX	10
char	Capture_filename[ TOPWIN ][ CAPTURE_FILENAME_MAX + 1 ] = { "" };
#include "capture.h"
FILE	*Capture_file[ TOPWIN ] = { NULL, NULL, NULL, NULL, NULL, 
				 NULL, NULL, NULL, NULL, NULL };
		/**********************************************************
		* Flag indicating whether any capture is active,
		* and the directory for capture files from $FACETCAPTUREDIR
		* (if set).
		**********************************************************/
int	Capture_active = 0;
char	*Capture_directory = NULL;

/**************************************************************************
* set_capture_directory
*	The string "directory" contains a directory-pathname from
*	$FACETCAPTUREDIR.  Store it away for subsequent use in creating
*	capture file pathnames.
**************************************************************************/
set_capture_directory( directory )
	char	*directory;
{
	long	*malloc_run();

	Capture_directory = (char *) malloc_run( strlen( directory) + 1,
						"capture_directory" );
	if ( Capture_directory != NULL )
	{
		strcpy( Capture_directory, directory );
	}
}
char	 *Map_get_command_capture_active = "";
char	*Text_get_command_capture_active =
	"Capture active on '%s' - N=stop Y=new file";
char	 *Map_get_command_capture_inactive = "";
char	*Text_get_command_capture_inactive =
	"Capture stopped on '%s' - Y=capture";
char	*Text_name_input_capture_file =
	"Capture file name";
/**************************************************************************
* get_capture_input
*	User has typed ^Wxc
*	Display current capture status for window number "winno" and
*	the current filename, if any.
*	If capture is active
*		'n' stops it and exits.
*		'y' closes the current file and prompts for a new name.
*	If capture is not active
*		'y' prompts for a new name.
*	The capture file pathname consists of the $FACETCAPTUREDIR
*	directory, followed by the user's capture file name, with the
*	extension ".cap".
*	The file is opened append.
**************************************************************************/
get_capture_input( winno )
	int	winno;
{
	char	filename[ CAPTURE_FILENAME_MAX + 1 ];
	char	prompt[ 81 ];
	int	c;
	FILE	*file;
	char	pathname[ CAPTURE_DIRECTORY_MAX + CAPTURE_FILENAME_MAX + 10 ];

	refresh_prompt_area();
	if ( Capture_file[ winno ] != NULL )
	{
			/* "Capture active on '%s' - N=stop Y=new file" */
		sprintf( prompt, Text_get_command_capture_active,
			Capture_filename[ winno ] );
		if ( (c = get_command_char( prompt, "", 
				Map_get_command_capture_active )) == -1 )
			return;
		if ( c == 'n' || c == 'N' )
		{
			fclose( Capture_file[ winno ] );
			Capture_file[ winno ] = NULL; 
			check_capture_closed();
			return;
		}
		else if ( c == 'y' || c == 'Y' )
		{
			fclose( Capture_file[ winno ] );
			Capture_file[ winno ] = NULL; 
			check_capture_closed();
		}
		else
		{
			term_beep();
			return;
		}
	}
	else
	{
			/* "Capture stopped on '%s' - Y=capture" */
		sprintf( prompt, Text_get_command_capture_inactive,
			Capture_filename[ winno ] );
		if ( (c = get_command_char( prompt, "", 
				Map_get_command_capture_inactive )) == -1 )
			return;
		if ( c == 'y' || c == 'Y' )
		{
		}
		else
		{
			term_beep();
			return;
		}
	}
	refresh_prompt_area();
	strcpy( filename, Capture_filename[ winno ] );
			     /* "Capture file name" */
	if ( get_name_input( filename, CAPTURE_FILENAME_MAX,
				Text_name_input_capture_file, 0 ) == 0 )
	{
		blank_trim( filename );
		strcpy( Capture_filename[ winno ], filename );
		if (  ( Capture_directory != NULL )
		   && ( strlen( Capture_directory ) < CAPTURE_DIRECTORY_MAX ) )
		{
			strcpy( pathname, Capture_directory );
			strcat( pathname, "/" );
		}
		else
		{
			pathname[ 0 ] = '\0';
		}
		strcat( pathname, filename );
		strcat( pathname, ".cap" );
		file = fopen( pathname, "a" );
		if ( file == NULL )
		{
			term_beep();
			return;
		}
		else
		{
			Capture_file[ winno ] = file;
			Capture_active = 1;
		}
	}
}
/**************************************************************************
* close_capture_files
*	All open capture files are closed.
**************************************************************************/
close_capture_files()
{
	int	i;
	FILE	*file;

	for ( i = 0; i < TOPWIN; i++ )
	{
		file = Capture_file[ i ];
		if ( file != NULL )
		{
			fclose( file );
		}
	}
}
/**************************************************************************
* check_capture_closed
*	Reset the Capture_active flag if all capture files are closed.
**************************************************************************/
check_capture_closed()
{
	int	i;
	FILE	*file;

	for ( i = 0; i < TOPWIN; i++ )
	{
		file = Capture_file[ i ];
		if ( file != NULL )
			return;
	}
	Capture_active = 0;
}
char	 *Map_get_command_keystroke_active = "";
char	*Text_get_command_keystroke_active =
"Keystroke active on '%s' - N=stop Y=new file";

char	 *Map_get_command_keystroke_inactive = "";
char	*Text_get_command_keystroke_inactive =
"Keystroke stopped on '%s' - Y=capture";

char	*Text_name_input_keystroke_file =
"Keystroke cap file";

char	*Text_name_input_keystroke_play =
"Keystroke play file";

char	Keystroke_capture_filename[ CAPTURE_FILENAME_MAX + 1 ] = { "" };

#include "keystroke.h"
		/**********************************************************
		* File descriptor of keystroke capture file - capturing.
		**********************************************************/
int	Keystroke_capture_fd = 0;
		/**********************************************************
		* Used to output timing records in keystroke capture file.
		**********************************************************/
long	Keystroke_capture_time = 0;
		/**********************************************************
		* File descriptor of keystroke capture file - replaying.
		**********************************************************/
int	Keystroke_play_fd = 0;


/**************************************************************************
* get_keystroke_capture_input
*	User has pressed ^W x k
*	Start, stop, or restart a keyboard keystroke capture.
**************************************************************************/
#include <fcntl.h>
get_keystroke_capture_input()
{
	char	filename[ CAPTURE_FILENAME_MAX + 1 ];
	char	prompt[ 81 ];
	int	c;
	int	fd;
	char	pathname[ CAPTURE_DIRECTORY_MAX + CAPTURE_FILENAME_MAX + 10 ];
	long	time();

	refresh_prompt_area();
	if ( Keystroke_capture_fd > 0 )
	{
		/**********************************************************
		* Active - 'n' closes - 'y' closes and restarts.
		* Inform receiver of close.
		**********************************************************/
			/* "Keystroke active on '%s' - N=stop Y=new file" */
		sprintf( prompt, Text_get_command_keystroke_active,
			Keystroke_capture_filename );
		if ( (c = get_command_char( prompt, "", 
				Map_get_command_keystroke_active )) == -1 )
			return;
		if ( c == 'n' || c == 'N' )
		{
			keystroke_capture_end();
			close( Keystroke_capture_fd );
			Keystroke_capture_fd = 0; 
			send_keystroke_filename( 'k', "" );
			return;
		}
		else if ( c == 'y' || c == 'Y' )
		{
			keystroke_capture_end();
			close( Keystroke_capture_fd );
			Keystroke_capture_fd = 0; 
			send_keystroke_filename( 'k', "" );
		}
		else
		{
			term_beep();
			return;
		}
	}
	else
	{
		/**********************************************************
		* Inactive - 'y' starts.
		**********************************************************/
			/* "Keystroke stopped on '%s' - Y=capture" */
		sprintf( prompt, Text_get_command_keystroke_inactive,
			Keystroke_capture_filename );
		if ( (c = get_command_char( prompt, "", 
				Map_get_command_keystroke_inactive )) == -1 )
			return;
		if ( c == 'y' || c == 'Y' )
		{
		}
		else if ( c == 'n' || c == 'N' )
		{
			return;
		}
		else
		{
			term_beep();
			return;
		}
	}
	/******************************************************************
	* Prompt for filename and open $FACETCAPTUREDIR/filename.key
	* create or append.
	* Record starting time for timing records.
	* Send pathname to receiver for capturing keystokes there.
	******************************************************************/
	refresh_prompt_area();
	strcpy( filename, Keystroke_capture_filename );
			     /* "Capture file name" */
	if ( get_name_input( filename, CAPTURE_FILENAME_MAX,
				Text_name_input_capture_file, 0 ) == 0 )
	{
		blank_trim( filename );
		strcpy( Keystroke_capture_filename, filename );
		if (  ( Capture_directory != NULL )
		   && ( strlen( Capture_directory ) < CAPTURE_DIRECTORY_MAX ) )
		{
			strcpy( pathname, Capture_directory );
			strcat( pathname, "/" );
		}
		else
		{
			pathname[ 0 ] = '\0';
		}
		strcat( pathname, filename );
		strcat( pathname, ".key" );
		fd = open( pathname, O_RDWR | O_CREAT | O_APPEND, 0666 );
		if ( fd <= 0)
		{
			term_beep();
			return;
		}
		else
		{
			Keystroke_capture_fd = fd;
			Keystroke_capture_time = time( (long *) 0 );
			send_keystroke_filename( 'k', pathname );
		}
	}
}
/**************************************************************************
* send_keystroke_filename
*	Transmit the keystroke capture filename to the receiver for use
*	in capturing keys that it reads.
**************************************************************************/
send_keystroke_filename( type, filename )
	char	type;
	char	*filename;
{
	char	*p;

	fct_window_mode_ans( type );
	for ( p = filename; *p != '\0'; p++ )
		fct_window_mode_ans( *p );
	fct_window_mode_ans( 0 );
}
#include "facetpath.h"
#include <sys/stat.h>
char	*Keystroke_play_fifo = NULL;
char	Keystroke_filename_save[ CAPTURE_FILENAME_MAX + 1 ];
/**************************************************************************
* get_keystroke_play_input
*	User has presses ^W x r
*	Start a keystroke capture replay.
**************************************************************************/
get_keystroke_play_input()
{
	char	filename[ CAPTURE_FILENAME_MAX + 1 ];
	int	fd;
	char	pathname[ CAPTURE_DIRECTORY_MAX + CAPTURE_FILENAME_MAX + 10 ];
	char	*tempnam();
	char	command[ 512 ];
	char	invisible_path[ 512 ];

	sprintf( invisible_path, "%s/sys/%sinvisible",
						Facettermpath, Facetprefix );
	refresh_prompt_area();
	if ( Keystroke_play_fd == 0 )
	{
		/**********************************************************
		* Not active - get filename.
		**********************************************************/
		if ( access( invisible_path, 1 ) < 0 )
		{
			term_beep();
			return;
		}
		strcpy( filename, "" );
				     /* "Capture file name" */
		if ( get_name_input( filename, CAPTURE_FILENAME_MAX,
				     Text_name_input_keystroke_play, 0 ) != 0 )
		{
			return;
		}
		blank_trim( filename );
		strcpy( Keystroke_filename_save, filename );
	}
	else
	{
		/**********************************************************
		* Active. Since keyboard input will terminate a replay,
		* this is being read from a replay - in order to create
		* a loop.  Close it and pretend name was typed again.
		**********************************************************/
		close_keystroke_play();
		strcpy( filename, Keystroke_filename_save );
	}
	/******************************************************************
	* Filename is $CAPTUREDIR/userfilename.key
	******************************************************************/
	if (  ( Capture_directory != NULL )
	   && ( strlen( Capture_directory ) < CAPTURE_DIRECTORY_MAX ) )
	{
		strcpy( pathname, Capture_directory );
		strcat( pathname, "/" );
	}
	else
	{
		pathname[ 0 ] = '\0';
	}
	strcat( pathname, filename );
	strcat( pathname, ".key" );
	if ( access( pathname, 4 ) < 0 )
	{
		term_beep();
		return;
	}
	/******************************************************************
	* In order for both processes to read the file,
	* fct_invisible is used to read the file, translate it and
	* write it into a named pipe or fifo.
	* Both processes read from that named pipe.
	******************************************************************/
	Keystroke_play_fifo = tempnam( NULL, "ftpip" );
	if (  ( Keystroke_play_fifo == NULL )
	   || ( *Keystroke_play_fifo == '\0' ) )
	{
		term_beep();
		return;
	}
	if ( mknod( Keystroke_play_fifo, S_IFIFO | 0777, 0 ) < 0 )
	{
		term_beep();
		return;
	}
	chown( Keystroke_play_fifo, getuid(), getgid() );
	chmod( Keystroke_play_fifo, 0777 );
	sprintf( command, "%s %s > %s &", 
			 invisible_path, pathname, Keystroke_play_fifo );
	if ( mysystem( command ) != 0 )
	{
		term_beep();
		return;
	}
	/******************************************************************
	* Open the fifo, set the keyboard to check-for-character reads,
	* and send fifo name to receiver.
	******************************************************************/
	fd = open( Keystroke_play_fifo, O_RDONLY );
	if ( fd <= 0)
	{
		term_beep();
		return;
	}
	else
	{
		termio_window_timed( 0, 0 );
		Keystroke_play_fd = fd;
		send_keystroke_filename( 'K', Keystroke_play_fifo );
	}
}
/**************************************************************************
* close_keystroke_play
*	Keystroke replay is ending
*	Terminal to normal read.
*	Close and destroy the fifo.
*	Inform the receiver.
**************************************************************************/
close_keystroke_play()
{
	termio_window();
	close( Keystroke_play_fd );
	Keystroke_play_fd = 0;
	unlink( Keystroke_play_fifo );
	send_keystroke_filename( 'K', "" );
}
/**************************************************************************
* keystroke_play_cancel
*	Terminate keystroke replay.
**************************************************************************/
keystroke_play_cancel()				/* SENDER */
{
	close_keystroke_play();
}
