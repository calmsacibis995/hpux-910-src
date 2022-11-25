/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: utility.c,v 70.1 92/03/09 16:14:54 ssa Exp $ */
/*
 * File:		utility.c
 * Creator:		G.Clark.Brown
 * Creation Date:	10/10/88
 *
 * Utility routines for the Terminfo Exercisers Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */

#include	<stdio.h>
#include	<string.h>
#include	<errno.h>
#include	<ctype.h>

#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

#ifndef lint
	static char	Sccsid[80]="%W% %E% %G%";
#endif


extern int	errno;
extern int	sys_nerr;
extern char	*sys_errlist[];

/* = = = = = = = = = = = = = = = u_term = = = = = = = = = = = = = = = =*/
/*
 * Terminate the process.
 */

u_term(code)

int	code;

{

	close_term();
	if(exit(code))
		perror("Exit Failure!");

}
/* = = = = = = = = = = = = = = = u_popen = = = = = = = = = = = = = = = =*/
/*
 * Call popen(3) and check for errors.
 */

FILE	*u_popen(command, mode, message_level)

	char	*command;
	char	*mode;
	int	message_level;

{
	FILE	*temp;

	if((temp = popen(command, mode)) == NULL)
	{
		switch(message_level)
		{
			case U_TERMINATE:
				ERR2("FATAL ERROR: Cannot execute %s for %s.", command,
					mode);
				perror("Reason: ");
				u_term(-1);
				break;

			case U_ERROR:
				ERR2("ERROR: Cannot execute %s for %s.",
					command, mode);
				perror("Reason: ");
				break;

			case U_WARNING:
				ERR2("Cannot execute %s for %s.", command,
					mode);
				perror("Reason: ");
				break;

			case U_MESSAGE:
				printf("Cannot execute %s for %s.",
					command, mode);
				if (errno <= sys_nerr)
					printf("\tReason: %s",
						sys_errlist[errno]);
				else
					printf("\tReason: %d", errno);

				break;

			case U_NOMESSAGE:
				break;

			default:
				ERR1(
			"Internal Error in u_popen.  Unknown message level: %d",
					message_level);
				u_term(-1);
				break;
		}
	}
	return temp;
}

/* = = = = = = = = = = = = = = continue_prompt = = = = = = = = = = = = = = = =*/
/*
 * Ask the user if he wants to repeat, continue, or quit.
 */

continue_prompt(p_opts)

struct option_tab	*p_opts;

{
	int	reply;

	if (at_pause(p_opts))
		return 1;
	while (TRUE)	/* loop forever */
	{
		printf(
	"Enter R to repeat, C to continue, or Q to quit: (default:C) ");
		fflush(stdout);

		reply = fgetc(stdin);
		printf( "\r\n" );
		if (reply == EOF ||
			(reply = tolower(reply)) == 'q')
			u_term(0);

		else if (reply == 'c' || reply == '\n' || reply == '\r')
			return 1;

		else if (reply == 'r')
			return 0;
	}
}

/* = = = = = = = = = = = = = = at_pause = = = = = = = = = = = = = = = =*/
/*
 * Execute the requested command at each pause.
 */

at_pause(p_opts)

struct option_tab	*p_opts;

{
	if(p_opts->command[0])
	{
		fflush(stdout);
		sleep( 1 );
		u_print_command(p_opts->command);
		/* die on error */
	}
	return p_opts->runaway;
}

/* = = = = = = = = = = = = = = = u_print_command = = = = = = = = = = = = = = = =*/
/*
 * Print the results of a command onto the output.
 */

u_print_command(command)

char	*command;

{
	FILE	*file;
	int	c;

#ifdef PROTECT_TERMIO_ON_POPEN
	termio_save();
#endif
	file = u_popen(command, "r", U_TERMINATE);
	while ((c=fgetc(file)) != EOF)
		fputc(c, stdout);
	pclose(file);
#ifdef PROTECT_TERMIO_ON_POPEN
	termio_restore();
#endif
}

#ifdef PROTECT_TERMIO_ON_POPEN

#include <termio.h>
struct termio T_save;

/**************************************************************************
* termio_save
*	Store the current termio settings for subsequent restoration.
**************************************************************************/
termio_save()
{
	int	status;

	status = ioctl( 1, TCGETA, &T_save );
	if ( status < 0 )
	{
		perror( "ioctl TCGETA failed" );
		return( -1 );
	}
	return( 0 );
}
/**************************************************************************
* termio_restore
*	Restore the previously saved termio settings.
**************************************************************************/
termio_restore()
{
	int	status;

	status = ioctl( 1, TCSETAW, &T_save );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW normal failed" );
		return( -1 );
	}
	return( 0 );
}

#endif
