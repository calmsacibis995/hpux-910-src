/*****************************************************************************
** Copyright (c) 1990 Structured Software Solutions, Inc.                   **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: fmenu_term.c,v 70.1 92/03/09 16:15:50 ssa Exp $ */
/*
 * fmenu_term.c
 *
 * Routines to do screens with terminfo (and some extra capabilities taken
 * from Facet/Term files.
 *
 * Copyright (c) Structured Software Solutions, Inc 1989. All rights reserved.
 */

/* #include	"mycurses.h" */

/* #include	<term.h> */
#include	<stdio.h>
#include	<termio.h>
#include	<signal.h>
#include	<varargs.h>
#include	"uiterm.h"
#include	"fmenu.h"
#include	"facetpath.h"

extern char *getenv();

static char blank_line[]="                                                                                ";
static int Cur_attr = FM_ATTR_NONE;
static int Altcharset_on = 0;

int		Columns = 80;
int		Lines = 24;
int		Fm_magic_cookie = 0;
int		Fm_ceol_standout_glitch = 0;
static char	*Fm_cursor_address = { (char *) 0 };
static char	*Fm_clear_screen = { (char *) 0 };
static char	*Fm_clr_eol = { (char *) 0 };
static char	*Fm_ena_acs = { (char *) 0 };
static char	*Fm_init_string = { (char *) 0 };
static char	*Fm_enter_alt_charset_mode = { (char *) 0 };
static char	*Fm_exit_alt_charset_mode = { (char *) 0 };
static char	*Fm_acs_chars = { (char *) 0 };
static char	*Fm_keypad_xmit = { (char *) 0 };
static char	*Fm_attrs[ FM_NBR_ATTRS ] = { (char *) 0 };

/*
 * term_char
 *
 * Normal output routine for terminfo functions
 *
 */

term_char( c )
char c;
{
	fputc( c, stdout );
}

/*
 * term_putchar
 *
 * puts a single character on the screen and if necessary, puts out
 * the appropriate attribute with the character.
 *
 */

term_putchar( c )
char c;
{
	if ( Altcharset_on )
		term_alt_char( c );
	else
		putchar( c );
}


/*
 * term_puts
 *
 * puts a string on the screen, and if necessary does it one character
 * at a time using term_putchar.
 *
 */

term_puts( s )
char *s;
{
	int return_value;

	return_value = strlen( s );
	if ( Fm_magic_cookie > 0 || Fm_ceol_standout_glitch || Altcharset_on )
	{
		while( *s )
		{
			term_putchar( *s++ );
		}
	}
	else
	{
		fputs( s, stdout );
	}
	return( return_value );
}


/*
 * term_write
 *
 * Formatted output routine for drawing screens similar to printf.
 * term_write( format_string, arg, arg, arg, ... );
 *
 * Formats which are interpreted are:
 *
 *	%s	write supplied string.
 *	%c	write supplied character.
 *	%r	repeat supplied character for the specified number of times.
 *	%b	write specified number of blanks.
 *	%a	set specified attribute.
 *	%g1	graphics on (enter alternate character set)
 *	%g0	graphics off (exit alternate character set)
 *	%n	do not flush output.
 *
 */

/*VARARGS*/
term_write( va_alist )
va_dcl
{
	char *fmt;
	va_list ap;
	char *fmtptr;
	char buff[ 80 ];
	char *strptr;
	int len;
	int i;
	int attr;
	int nbr_written;
	int do_output;

	va_start( ap );
	fmt = va_arg( ap, char * );
	do_output = 1;
	nbr_written = 0;
	for ( fmtptr = fmt; *fmtptr; fmtptr++ )
	{
		if ( *fmtptr == '%' )
		{
			fmtptr++;
			if ( *fmtptr == 's' ) /* string */
			{
				strptr = va_arg( ap, char * );
				len = strlen( strptr );
				nbr_written += term_puts( strptr );
			}
			else if ( *fmtptr == 'c' ) /* single character */
			{
				buff[0] = va_arg( ap, int );
				len = 1;
				nbr_written++;
				term_putchar( buff[0] );
			}
			else if ( *fmtptr == 'r' ) /* repeated character */
			{
				buff[0] = va_arg( ap, int );
				len = va_arg( ap, int );
				for ( i = 1; i < len; i++ )
					buff[i] = buff[0];
				buff[ len ] = 0;
				nbr_written += term_puts( buff );
			}
			else if ( *fmtptr == 'b' ) /* blanks */
			{
				len = va_arg( ap, int );
				if ( len > 0 && len <= 80 )
				{
					blank_line[len] = 0;
					nbr_written += term_puts( blank_line );
					blank_line[len] = ' ';
				}
			}
			else if ( *fmtptr == 'a' ) /* attribute */
			{
				attr = va_arg( ap, int );
				if ( attr >= FM_NBR_ATTRS )
					continue;
				if ( Fm_attrs[ attr ] == (char *) 0 )
					continue;
				if ( *Fm_attrs[ attr ] == '\0' )
					continue;
				if ( ( attr == Cur_attr ||
				       ( strcmp( Fm_attrs[ attr ],
						 Fm_attrs[ Cur_attr ] ) == 0 ) )
				     && !( Fm_magic_cookie ||
					   Fm_ceol_standout_glitch ) )
				{
					Cur_attr = attr;
					continue;
				}
				tputs( Fm_attrs[ attr ], 1, term_char );
				if ( Fm_magic_cookie > 0 )
					nbr_written += Fm_magic_cookie;
				Cur_attr = attr;
			}
			else if ( *fmtptr == 'g' ) /* graphics on or off */
			{
				fmtptr++;
				if ( *fmtptr == '1' )
					term_enter_alt_charset_mode();
				else if ( *fmtptr == '0' )
					term_exit_alt_charset_mode();
			}
			else if ( *fmtptr == 'n' ) /* no output */
			{
				do_output = 0;
			}
			else
			{
				strncpy( buff, fmtptr - 1, 2 );
				buff[2] = 0;
				nbr_written += term_puts( buff );
			}
		}
		else
		{
			buff[0] = *fmtptr;
			nbr_written++;
			term_putchar( buff[ 0 ] );
		}
	}
	if ( do_output )
		term_outgo();
	va_end( ap );
	return( nbr_written );
}
term_cursor_address( row, col )
	int	row;
	int	col;
{
	char	*tparm();

	tputs( tparm( Fm_cursor_address, row, col ), 1, term_char );
	term_outgo();
}
term_ena_acs()
{
	if (  (  Fm_ena_acs != (char *) 0 )
	   && ( *Fm_ena_acs != '\0'       ) )
	{
		tputs( Fm_ena_acs, 1, term_char );
	}
}
term_enter_alt_charset_mode()
{
	if (  (  Fm_enter_alt_charset_mode != (char *) 0 )
	   && ( *Fm_enter_alt_charset_mode != '\0'       ) )
	{
		tputs( Fm_enter_alt_charset_mode, 1, term_char );
	}
	Altcharset_on = 1;
}
term_exit_alt_charset_mode()
{
	if (  (  Fm_exit_alt_charset_mode != (char *) 0 )
	   && ( *Fm_exit_alt_charset_mode != '\0'       ) )
	{
		tputs( Fm_exit_alt_charset_mode, 1, term_char );
	}
	Altcharset_on = 0;
}
short	Acs_chars_array[ 128 ] = { 0 };
short	Acs_default_chars_array[ 128 ] = { 0 };
term_init_acs_chars()
{
	char	*s;
	char	c;
	int	i;
	
	if (  (  Fm_acs_chars != (char *) 0 )
	   && ( *Fm_acs_chars != '\0'       ) )
	{
		s = Fm_acs_chars;
		while( (c = *s++) != '\0' )
		{
			if ( *s == '\0' )
				break;
			Acs_chars_array[ c & 0x7F ] = *s++;
		}
	}
	for( i = 0; i < 128; i++ )
		Acs_default_chars_array[ i ] = '*';
	Acs_default_chars_array[ 'j' ] = '+';
	Acs_default_chars_array[ 'k' ] = '+';
	Acs_default_chars_array[ 'l' ] = '+';
	Acs_default_chars_array[ 'm' ] = '+';
	Acs_default_chars_array[ 'q' ] = '-';
	Acs_default_chars_array[ 't' ] = '+';
	Acs_default_chars_array[ 'u' ] = '+';
	Acs_default_chars_array[ 'v' ] = '+';
	Acs_default_chars_array[ 'w' ] = '+';
	Acs_default_chars_array[ 'x' ] = '|';
}
term_alt_char( vt100 )
	char	vt100;
{
	int	c;

	c = Acs_chars_array[ vt100 ];
	if ( c <= 0 )
	{
		term_exit_alt_charset_mode();
		putchar( (char) Acs_default_chars_array[ vt100 ] );
		term_enter_alt_charset_mode();
	}
	else
	{
		putchar( (char) c );
	}
}
term_clear_line()
{
	tputs( Fm_clr_eol, 1, term_char );
	term_outgo();
}
term_clear_screen()
{
	tputs( Fm_clear_screen, 1, term_char );
	term_outgo();
}
term_outgo()
{
	fflush( (FILE *) stdout );
}
					/* Terminfo initialization and code
					** to setup stdin and the window
					** for the control program.
					*/
struct termio Stdout_normal, Stdout_raw;
unsigned char Intr_char;
term_init()
{
	int	term_quit();
	char	*term;

	/* setupterm( (char *) 0, 1, (int *) 0 ); */
	if ( ( term = getenv( "FACETTERM" ) ) == NULL )
		if ( ( term = getenv( "TSMTERM" ) ) == NULL )
			term = getenv( "TERM" );
	if ( term != NULL )
		get_extra( term );
	if ( fm_caps_missing() )
		exit( 1 );
	term_init_acs_chars();
	if ( make_raw( 0, &Stdout_normal, &Stdout_raw ) < 0 )
		exit( 1 );
	Intr_char = Stdout_raw.c_cc[ VINTR ];
	term_init_string();
	term_ena_acs();
	term_keypad_xmit();
	return( 0 );
}
term_init_string()
{
	if (  (  Fm_init_string != (char *) 0 )
	   && ( *Fm_init_string != '\0'       ) )
	{
		tputs( Fm_init_string, 1, term_char );
	}
}
term_keypad_xmit()
{
	if (  (  Fm_keypad_xmit != (char *) 0 )
	   && ( *Fm_keypad_xmit != '\0'       ) )
	{
		tputs( Fm_keypad_xmit, 1, term_char );
	}
}
term_quit( clear_flag )
int clear_flag;
{
	if ( clear_flag )
	{
		term_clear_screen();
		term_cursor_address( Lines - 1, 0 );
		term_outgo();
	}
	make_normal( 0, &Stdout_normal );
	exit( 0 );
}
make_raw( fd, normal, raw )
	int	fd;
	struct termio	*normal;
	struct termio	*raw;
{
	int	status;

	status = ioctl( fd, TCGETA, normal );
	if ( status < 0 )
	{
		perror( "ioctl TCGETA failed" );
		return( -1 );
	}
	status = ioctl( fd, TCGETA, raw );
	if ( status < 0 )
	{
		perror( "ioctl TCGETA failed" );
		return( -1 );
	}
	raw->c_iflag &= ~(INLCR | ICRNL | IUCLC | BRKINT);
	raw->c_oflag &= ~ OPOST;
	raw->c_lflag &= ~(ICANON | ISIG | ECHO);
	raw->c_cc[ VMIN ] = 10;
	raw->c_cc[ VTIME ] = 1;
	status = ioctl( fd, TCSETAW, raw );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW window failed" );
		return( -1 );
	}
	return( 0 );
}
set_vmin_vtime( fd, vmin, vtime )
int fd, vmin, vtime;
{
	struct termio raw;
	int status;

	status = ioctl( fd, TCGETA, &raw );
	if ( status < 0 )
	{
		perror( "ioctl TCGETA failed" );
		return( -1 );
	}
	raw.c_cc[ VMIN ] = vmin;
	raw.c_cc[ VTIME ] = vtime;
	status = ioctl( fd, TCSETAW, &raw );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW window failed" );
		return( -1 );
	}
	return( 0 );
}
make_normal( fd, normal )
	int	fd;
	struct termio	*normal;
{
	int	status;

	status = ioctl( fd, TCSETAW, normal );
	if ( status < 0 )
	{
		perror( "ioctl TCSETAW window failed" );
		return( -1 );
	}
	return( 0 );
}


/****************************************************************
*								*
* open_facetinfo						*
*								*
*      This routine opens the <name>.fi file.  The routine      *
* checks for the file in the following order:			*
*								*
*	current directory					*
*	directory specified in the FACETINFO env variable	*
*	directory specified in the HOME env variable		*
*	/usr/facetterm/localterm directory			*
*	/usr/facetterm/term directory				*
*								*
****************************************************************/

#define MAX_ALIAS	40
FILE	*
open_facetinfo( infoname )
	char	*infoname;
{
	char	filename[ 256 ];
	FILE	*fd;
	char	*directory;

	fd = fopen( infoname, "r" );
	if ( fd == NULL )
	{
		directory = getenv( "FACETINFO" );
		if ( directory == NULL )
			directory = getenv( "TSMINFO" );
		if ( directory != NULL )
		{
			strcpy( filename, directory );
			strcat( filename, "/" );
			strcat( filename, infoname );
			fd = fopen( filename, "r" );
		}
	}
	if ( fd == NULL )
	{
		directory = getenv( "HOME" );
		if ( directory != NULL )
		{
			strcpy( filename, directory );
			strcat( filename, "/" );
			strcat( filename, infoname );
			fd = fopen( filename, "r" );
		}
	}
	if ( fd == NULL )
	{
		sprintf( filename, "%s/localterm/%s", Facettermpath, infoname );
		fd = fopen( filename, "r" );
	}
	if ( fd == NULL )
	{
		sprintf( filename, "%s/term/%s", Facettermpath, infoname );
		fd = fopen( filename, "r" );
	}
	if ( fd == NULL )
	{
		strcpy( filename, Facettermpath );
		strcat( filename, "/" );
		strcat( filename, infoname );
		fd = fopen( filename, "r" );
	}
	if ( fd == NULL )
	{
		char	alias[ MAX_ALIAS + 1 ];
		int	status;

		status = get_facetinfo_alias( infoname, alias, MAX_ALIAS );
		if ( status == 0 )
		{
			fd = open_facetinfo( alias );
		}
	}
	return( fd );
}
/**************************************************************************
* open_facetinfo_alias_search
*	
**************************************************************************/
get_facetinfo_alias( infoname, alias, max_alias )
	char	*infoname;
{
	char	*getenv();
	char	*directory;
	char	alias_filename[ 256 ];
	char	alias_pathname[ 256 ];
	int	status;

	sprintf( alias_filename, ".%saliasM", Facetname );
	status = search_facetinfo_alias( alias_filename, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	sprintf( alias_filename, ".%salias", Facetname );
	status = search_facetinfo_alias( alias_filename, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	directory = getenv( "FACETINFO" );
	if ( directory == NULL )
		directory = getenv( "TSMINFO" );
	if ( directory != NULL )
	{
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		strcat( alias_pathname, "M" );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
	}
	directory = getenv( "HOME" );
	if ( directory != NULL )
	{
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		strcat( alias_pathname, "M" );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
		strcpy( alias_pathname, directory );
		strcat( alias_pathname, "/" );
		strcat( alias_pathname, alias_filename );
		status = search_facetinfo_alias( alias_pathname, infoname, 
						 alias, max_alias );
		if ( status == 0 )
			return( 0 );
	}
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/localterm/" );
	strcat( alias_pathname, alias_filename );
	strcat( alias_pathname, "M" );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/localterm/" );
	strcat( alias_pathname, alias_filename );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/term/" );
	strcat( alias_pathname, alias_filename );
	strcat( alias_pathname, "M" );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/term/" );
	strcat( alias_pathname, alias_filename );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/" );
	strcat( alias_pathname, alias_filename );
	strcat( alias_pathname, "M" );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );
	strcpy( alias_pathname, Facettermpath );
	strcat( alias_pathname, "/" );
	strcat( alias_pathname, alias_filename );
	status = search_facetinfo_alias( alias_pathname, infoname, 
					 alias, max_alias );
	if ( status == 0 )
		return( 0 );

	return( -1 );
}
/**************************************************************************
* search_facetinfo_alias
*	Look for "infoname" in "alias_pathname",
*	If "alias_pathname does not exist or infoname not there, return -1.
*	If found, store alias in "alias" with maximum length "max_alias"
*	and return 0.
**************************************************************************/
search_facetinfo_alias( alias_pathname, infoname, alias, max_alias )
	char	*alias_pathname;
	char	*infoname;
	char	*alias;
	int	max_alias;
{
	FILE	*fd;
	char	buffer[ 1024 + 1 ];
	char	*strpbrk();
	char	*p;
	char	*x;

	fd = fopen( alias_pathname, "r" );
	if ( fd == NULL )
		return( -1 );
	while( read_dotfacet( buffer, 1024, fd ) >= 0 )
	{
		/**********************************************************
		* Truncate at first tab or space which must be present.
		**********************************************************/
		p = strpbrk( buffer, "\t " );
		if ( p == ( char * ) 0 )
			continue;
		*p++ = '\0';
		/**********************************************************
		* Skip any remaining tab or space.
		**********************************************************/
		p += strspn( p, "\t " );
		/**********************************************************
		* alias must have non-zero length.
		**********************************************************/
		if ( *p == '\0' )
			continue;
		/**********************************************************
		* Truncate at optional tab or space.
		**********************************************************/
		x = strpbrk( p, "\t " );
		if ( x != ( char * ) 0 )
			*x = '\0';
		if ( strcmp( buffer, infoname ) == 0 )
		{
			strncpy( alias, p, max_alias );
			alias[ max_alias ] = '\0';
			close( fd );
			return( 0 );			/* found */
		}
	}
	close( fd );
	return( -1 );					/* not found */
}

/****************************************************************
*								*
* get_extra							*
*								*
*      This routine opens the *.fi file, adds certain control	*
* characters to the decode tree, calls get_extra_strings to	*
* process data from the *.fi file and closes the *.fi file.	*
*								*
****************************************************************/

get_extra( terminal )
	char	*terminal;
{
	FILE	*fd;
	int	status;
	char	infoname[ 80 ];

	strcpy( infoname, terminal );
	strcat( infoname, ".fi" );
	fd = open_facetinfo( infoname );
	if ( fd == NULL )
		return( 1 );

	status = get_extra_strings( fd );
	close( fd );
	return( status );
}

/****************************************************************
*								*
* get_extra_strings						*
*								*
*      This is a recursive routine used to read the *.fi file	*
* and install the appropriate data in the decode tree.		*
*								*
****************************************************************/

get_extra_strings( fd )
	FILE	*fd;
{
	char	buff[ 401 ];
	char	*buffptr;
	char	*p;
	char	*string;
	int	len;
	FILE	*new_fd;
	int	status;
	char	*name;
	char	*dec_encode();

	while( fgets( buff, 400, fd ) != NULL )
	{
		if ( buff[ 0 ] == '#' )		/* comment */
		{
			if ( strncmp( buff, "##-menu-cap-", 12 ) != 0 )
				continue;
		}

		buffptr = buff;
		len = strlen( buffptr );
		if ( len <= 1 ) 
			continue;
		p = &(buffptr[ len - 1 ]);
		if ( *p == '\n' )		/* remove trailing newline */
			*p-- = '\0';
		string = NULL;
		for ( p = buffptr; *p != '\0'; p++ )/* split @ first = if any */
		{
			if ( *p == '=' )	/* point string after = */
			{
				*p++ = '\0';
				string = p;
				break;
			}
		}

		if ( strcmp( buffptr, "use" ) == 0 )
		{
			new_fd = open_facetinfo( string );
			if ( new_fd == NULL )
				return( 1 );
			status = get_extra_strings( new_fd );
			close( new_fd );
			if ( status )
				return( -1 );
		}

		if ( strncmp( buffptr, "##-menu-cap-", 12 ) != 0 )
		{
			facetterm_strings( buffptr, string );
			continue;
		}

		len = 0;
		name = &buffptr[ 12 ];
		if ( strcmp( name, "init_string" ) == 0 )
		{
			Fm_init_string = dec_encode( string );
		}
		else if ( strcmp( name, "cursor_address" ) == 0 )
		{
			Fm_cursor_address = dec_encode( string );
		}
		else if ( strcmp( name, "clear_screen" ) == 0 )
		{
			Fm_clear_screen = dec_encode( string );
		}
		else if ( strcmp( name, "clr_eol" ) == 0 )
		{
			Fm_clr_eol = dec_encode( string );
		}
		else if ( strcmp( name, "ena_acs" ) == 0 )
		{
			Fm_ena_acs = dec_encode( string );
		}
		else if ( strcmp( name, "enter_alt_charset_mode" ) == 0 )
		{
			Fm_enter_alt_charset_mode = dec_encode( string );
		}
		else if ( strcmp( name, "exit_alt_charset_mode" ) == 0 )
		{
			Fm_exit_alt_charset_mode = dec_encode( string );
		}
		else if ( strcmp( name, "acs_chars" ) == 0 )
		{
			Fm_acs_chars = dec_encode( string );
		}
		else if ( strcmp( name, "keypad_xmit" ) == 0 )
		{
			Fm_keypad_xmit = dec_encode( string );
		}
		else if ( strcmp( name, "no_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_NONE ] = dec_encode( string) ;
		}
		else if ( strcmp( name, "shadow_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_SHADOW ] = dec_encode( string) ;
		}
		else if ( strcmp( name, "item_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_ITEM ] = dec_encode( string) ;
		}
		else if ( strcmp( name, "box_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_BOX ] = dec_encode( string) ;
		}
		else if ( strcmp( name, "title_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_TITLE ] = dec_encode( string) ;
		}
		else if ( strcmp( name, "highlight_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_HIGHLIGHT ] = dec_encode( string) ;
		}
		else if ( strcmp( name, "highlight_blink_attr" ) == 0 )
		{
			Fm_attrs[ FM_ATTR_HIGHLIGHT_BLINK ] =
							dec_encode( string) ;
		}
		else if ( strcmp( name, "magic_cookie" ) == 0 )
		{
			if ( stoi( string, &Fm_magic_cookie ) )
			{
				printf( "\007Invalid magic_cookie='%s'\r\n",
					string );
				Fm_magic_cookie = 0;
			}
		}
		else if ( strcmp( name, "ceol_standout_glitch" ) == 0 )
		{
			Fm_ceol_standout_glitch = 1;
		}
		else if ( extra_keys( name, string ) )
		{
		}
		else
		{
			printf( "\007Unknown menu capability='%s'\r\n", name );
		}
	}
	fclose( fd );
	return( 0 );
}
/****************************************************************
* Handle an ordinary facetterm capability ( not ##-menu-cap ) if needed.
****************************************************************/
facetterm_strings( name, string )
	char	*name;
	char	*string;
{
	int	x;

	if ( strcmp( name, "columns" ) == 0 )
	{
		x = atoi( string );
		if ( x > 0 )
			Columns = x;
		else
			printf( "ERROR: Invalid columns = '%s'\r\n", string );
		return( 1 );
	}
	if ( strcmp( name, "lines" ) == 0 )
	{
		x = atoi( string );
		if ( x > 0 )
			Lines = x;
		else
			printf( "ERROR: Invalid lines = '%s'\r\n", string );
		return( 1 );
	}
	return( 0 );
}

#define ENCODE_LEN_MAX 2000
char	*Dummy_string = "";
#include <malloc.h>
/**************************************************************************
* dec_encode
*	Encode a string from a terminal description file.  Malloc room
*	to store it, store, and return a pointer.
**************************************************************************/
char *
dec_encode( string )
	char	*string;
{
	char	extra_buff[ ENCODE_LEN_MAX + 1 ];
	char	*new;
	int	len;

	if ( strlen( string ) > ENCODE_LEN_MAX )
	{
		printf( "ERROR: encode string too long '%s'\n", string );
		return( Dummy_string );
	}
	len = string_encode( string, extra_buff );
	len += 1;
	new = (char *) malloc( len );
	if ( new == NULL )
	{
		printf( "ERROR: malloc in dec_encode failed.\n" );
		printf( "len=%d\r\n", len );
		return( Dummy_string );
	}
	memcpy( new, extra_buff, len );
	return( new );
}

/****************************************************************
*								*
* string_encode							*
*								*
*      This routine takes an ASCII string converts it to binary *
* and stores it in a passed buffer.				*
* returns length
*								*
****************************************************************/

string_encode( string, store )
	char	*string;
	char	*store;
{
	char	c;
	char	*s;
	char	out[ 10 ];
	int	value;
	int	digit;
	char	*p;
	int	len;

	p = store;
	if ( string == NULL )
	{
		*store = '\0';
		return( 0 );
	}
	s = string;
	while ( ( c = *s++ ) != '\0' )
	{
		if ( c == '\\' )
		{
			c = *s++;
			if ( ( c == 'E' ) || ( c == 'e' ) )
				*store++ = 0x1b;
			else if ( c == '\\' )
				*store++ = '\\';
			else if ( c == '^' )
				*store++ = '^';
			else if ( c == 'r' )
				*store++ = '\r';
			else if ( c == 'n' )
				*store++ = '\n';
			else if ( c == 'b' )
				*store++ = '\b';
			else if ( c == 's' )
				*store++ = ' ';
			else if ( c == '[' )
				*store++ = 0x40 + '[';	/* 8_bit */
			else if ( c == 'O' )
				*store++ = 0x40 + 'O';	/* 8_bit */
			else if ( c >= '0' && c <= '9' )
			{
				value = c - '0';
				digit = 1;
				while ( *s >= '0' && *s <= '9' && digit < 3 )
				{
					c = *s++;
					value <<= 3;
					value += ( c - '0' );
					digit++;
				}
				if ( value == 0 )
					*store++ = 0x80;
				else
					*store++ = value;
			}
			else
			{
				out[ 0 ] = c;
				out[ 1 ] = '\0';
			}
		}
		else if ( c == '^' )
		{
			c = *s++;
			if ( c == '?' )
				*store++ = '\177';
			else if ( c > 0x60 )
				*store++ = c - 0x60;
			else if ( c > 0x40 )
				*store++ = c - 0x40;
			else if ( c == '\0' )
				*store++ = '^';
			else
			{
				out[ 0 ] = c;
				out[ 1 ] = '\0';
			}
		}
		else
		{
			*store++ = c;
		}
	}
	*store++ = '\0';
	len = store - p - 1;
	return( len );
}
fm_caps_missing()
{
	int	error;

	error = 0;
	if (  (  Fm_cursor_address == (char *) 0 )
	   || ( *Fm_cursor_address == '\0'       ) )
	{
		printf(
		    "ERROR: Capability is not defined: cursor_address\r\n" );
		error = 1;
	}
	if (  (  Fm_clear_screen == (char *) 0 )
	   || ( *Fm_clear_screen == '\0'       ) )
	{
		printf( "ERROR: Capability is not defined: clear_screen\r\n" );
		error = 1;
	}
	if (  (  Fm_clr_eol == (char *) 0 )
	   || ( *Fm_clr_eol == '\0'       ) )
	{
		printf( "ERROR: Capability is not defined: clr_eol\r\n" );
		error = 1;
	}
	if (  (  Fm_attrs[ FM_ATTR_NONE ] == (char *) 0 )
	   || ( *Fm_attrs[ FM_ATTR_NONE ] == '\0'       ) )
	{
		int	i;

		for ( i = 1; i < FM_NBR_ATTRS; i++ )
		{
			if (  (  Fm_attrs[ i ] != (char *) 0 )
			   && ( *Fm_attrs[ i ] != '\0'       ) )
			{
				printf(
			"ERROR: Capability is not defined: no_attr\r\n" );
				error = 1;
				break;
			}
		}
	}
	return( error );
}

/*
show_seq(msg,str)
char *msg,*str;
{
int i;
dprint("%s ",msg);
for(i=0;i<strlen(str);i++)
dprint("0x%x ",*str++);
dprint("\n");
}
*/
#ifdef lint
static int lint_alignment_warning_ok_7;
#endif
