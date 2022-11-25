/*****************************************************************************
** Copyright (c) 1986 - 1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftkeys.c,v 70.1 92/03/09 15:43:20 ssa Exp $ */
/**************************************************************************
* ftkeys.c
*	read keyboard returning special codes for non-print keys.
*	Some of the key definitions are from the terminal description file.
**************************************************************************/
#include <stdio.h>
#include "ftproc.h"
#include "keyval.h"
#include "control8.h"
#include "options.h"

struct ft_key
{
	int	code;
	struct ft_key	*next;
	struct ft_key	*also;
	int	value;			/* key value */
};
typedef struct ft_key FT_KEY;
FT_KEY	*Key_root;

unsigned char	Special_key[ 256 ] = { 0 };

/**************************************************************************
* fct_key_init
*	Initialize the decode tree for keys.
**************************************************************************/
fct_key_init()
{
	FT_KEY *get_free_key();

	Key_root = get_free_key();
	if ( Key_root == NULL )
	{
		printf( "ERROR: Key malloc failed\n" );
		wait_return_pressed();
		exit( 1 );
	}
}
/**************************************************************************
* get_free_key
*	Allocate and return a node for the key decoder tree.
**************************************************************************/
FT_KEY *
get_free_key()
{
	FT_KEY		*pkey;

	pkey = ( FT_KEY * ) malloc_run( sizeof( FT_KEY ), "get_free_key" );
	if ( pkey == NULL )
	{
		printf( "too many keys to malloc\n" );
		return( NULL );
	}
	pkey->code = 0;
	pkey->next = NULL;
	pkey->also = NULL;
	pkey->value = 0;
	return( pkey );
}
/**************************************************************************
* key_install
*	Install the string "string" in the key decoder tree.
*	When the sequence is seen return "value" instead of the sequence.
*	"name" is used for error messages.
**************************************************************************/
key_install( name, string, value )
	char	*name;		/* char string for error messages */
	char	*string;	/* termcap string to be installed */
	int	value;		/* value returned for key */
{
	FT_KEY	*pkey;
	UNCHAR	c;
	UNCHAR	*pstring;
	FT_KEY	*install_key();
	int	first;

	pstring = (UNCHAR *) string;
	if ( pstring[ 0 ] == '\0' )	/* do not install null strings */
		return;		

	pkey = Key_root;		/* start at root of decode tree */
	first = 1;
	while ( ( c = *pstring++ ) != '\0' )
	{
		if ( c == 0x80 )
			c = 0;
		if ( first )
		{
			Special_key[ c ] = 1;
			first = 0;
		}
		pkey = install_key( (int) c, pkey );
		if ( pkey == NULL )
		{
			printf( "    Could not install key: %s\n", name );
			return;
		}
	}
	if ( pkey->next != NULL )
	{
		printf( "key is duplicate to longer sequence\n" );
		printf( "    Could not install key:: %s\n", name );
		return;
	}
	pkey->value = value;
}
/**************************************************************************
* install_key
*	Install the next character in the key sequence "code" attached to
*	the node of the tree "pkey".
**************************************************************************/
FT_KEY *
install_key( code, pkey )
	int	code;
	FT_KEY	*pkey;
{
	FT_KEY	*p;
	FT_KEY	*new;

	p = pkey->next;
	if ( p == NULL )
	{					/* add a next */
		if ( (new = get_free_key()) == NULL )
			return( NULL );
		pkey->next = new;
		new->code = code;
		return( new );
	}
	while( 1 )
	{
		if (  p->code == code )
		{
			if ( p->value != 0 )
			{
				printf( "duplicate key\n" );
				return( NULL );
			}
			return( p );
		}
		if ( p->also == NULL )
		{				/* add an also */
			if ( (new = get_free_key()) == NULL )
				return( NULL );
			p->also = new;
			new->code = code;
			return( new );
		}
		p = p->also;
	}
}
/**************************************************************************
* The Key_read_pkt routines allow an ioctl to send in a sequence of keystrokes
* and they are treated as if they were typed on the keyboard after ^W .
**************************************************************************/
int Key_read_pkt_present = 0;		/* has been received */
int Key_read_pkt_active = 0;		/* is in the process of being read */
char Key_read_pkt_chars[ 80 ];		/* the characters */
int Key_read_pkt_index = 0;		/* the next char to be read */
int Key_read_pkt_winno = 0;		/* the window that sent them */
/**************************************************************************
* key_read_pkt_setup
*	Set up to treat "string" like it was typed on the keyboard.
**************************************************************************/
key_read_pkt_setup( string, winno )
	char	*string;
	int	winno;
{
	strcpy( Key_read_pkt_chars, string );
	Key_read_pkt_present = 1;
	Key_read_pkt_winno = winno;
}
/**************************************************************************
* key_read_pkt_pending
*	Return 1 if a window command mode ioctl is ready to go.
**************************************************************************/
key_read_pkt_pending()
{
	return( Key_read_pkt_present );
}
/**************************************************************************
* key_read_pkt_start
*	Start executing it - prompting is off - users confuse easily.
**************************************************************************/
key_read_pkt_start()
{
	if ( Key_read_pkt_present )
	{
		Key_read_pkt_active = 1;
		Key_read_pkt_index = 0;
		turn_prompting_off();
		return( 0 );
	}
	else
	{
		return( -1 );
	}
}
/**************************************************************************
* key_read_pkt_end
*	The key read packet is done - turn prompting back on.
*	Note that we may remain in window command mode.
**************************************************************************/
key_read_pkt_end()
{
	Key_read_pkt_present = 0;
	Key_read_pkt_active = 0;
	turn_prompting_on();
}
/**************************************************************************
* get_key_read_pkt_winno
*	What window does the key read packet apply to?
*	Return -1 if none.
**************************************************************************/
get_key_read_pkt_winno()
{
	if ( Key_read_pkt_active )
	{
		return( Key_read_pkt_winno );
	}
	else
		return( -1 );
}
/**************************************************************************
* key_read_pkt
*	Get the next character from the key read packet.  Return ESC to
*	get us out of window command mode if the packet ended prematurely.
**************************************************************************/
key_read_pkt()
{
	int c;

	c = Key_read_pkt_chars[ Key_read_pkt_index++ ];
	if ( c == 0 )
	{
		Key_read_pkt_index--;
		c = '\033';
	}
	return( c );
}
/**************************************************************************
* key queue is used to hold characters read ahead until they are needed.
**************************************************************************/
#define MAX_KEY_CHARS 10
unsigned char Keybuff[ MAX_KEY_CHARS + 1 ];
int Keybufflen = 0;
/**************************************************************************
* clear_key_queue
**************************************************************************/
clear_key_queue()
{
	Keybufflen = 0;
}
/**************************************************************************
* enqueue_key
**************************************************************************/
enqueue_key( c )
	int	c;
{
	if ( Keybufflen < ( MAX_KEY_CHARS - 1 ) )
		Keybuff[ Keybufflen++ ] = c;
}
/**************************************************************************
* dequeue_key
**************************************************************************/
dequeue_key()
{
	int	c;
	int	i;

	if ( Keybufflen == 0 )
		return( -1 );
	c = Keybuff[ 0 ];
	Keybufflen--;
	for ( i = 0; i < Keybufflen; i++ )
		Keybuff[ i ] = Keybuff[ i + 1 ];
	return( c );
}

#if defined( VPIX ) || defined( SOFTPC )
int	Vpix_shift = 0;
int	Vpix_shift_right = 0;
int	Vpix_control = 0;
#endif

int	Key_timing=0;
/**************************************************************************
* key_read_start
*	Starting a window command mode read of the keyboard.
**************************************************************************/
key_read_start()
{
#if defined( VPIX ) || defined( SOFTPC )
	Vpix_shift = 0;
	Vpix_shift_right = 0;
	Vpix_control = 0;
#endif
	clear_key_queue();
	Key_timing = 0;
}
/**************************************************************************
* key_read_end
*	Ending window command mode read of keyboard - put it back for the
*	receiver.
**************************************************************************/
key_read_end()
{
	if ( Key_timing )
		termio_window();
#ifdef SOFTPC
	if ( terminal_mode_is_scan_code_mode() )
	{
		term_flush_input();
	}
#endif
}
#if defined( VPIX ) || defined( SOFTPC )

#include <termio.h>

/**************************************************************************
* term_flush_input
*	Flush the keyboard, and the read ahead buffer between the sender
*	and receiver - switching scan code to normal etc creates a lot of
*	junk.
**************************************************************************/
term_flush_input()
{
	ioctl( 0, TCFLSH, 0 );
	flush_keys_from_recv();
}
#endif
#if defined( VPIX ) || defined( SOFTPC )
/**************************************************************************
* Arrays to decode the scan codes from the keyboard.
**************************************************************************/
unsigned char Vpix_chars[ 128 ] = 
{
	/* 0	 1	 2	 3	 4	 5	 6	 7	*/
	/* 8	 9	 A	 B	 C	 D	 E	 F	*/
/* 0 */	0,	'\033',	'1',	'2',	'3',	'4',	'5',	'6',
	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	0,
/* 1 */	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
	'o',	'p',	'[',	']',	'\r',	0,	'a',	's',
/* 2 */	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
	'\'',	'`',	0,	'\\',	'z',	'x',	'c',	'v',
/* 3 */	'b',	'n',	'm',	',',	'.',	'/',	0,	0,
	0,	' ',	0,	0,	0,	0,	0,	0,
/* 4 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 5 */	0,	0,	0,	'\177',	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 6 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 7 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0
};
unsigned char Vpix_chars_s[ 128 ] = 
{
	/* 0	 1	 2	 3	 4	 5	 6	 7	*/
	/* 8	 9	 A	 B	 C	 D	 E	 F	*/
/* 0 */	0,	'\033',	'!',	'@',	'#',	'$',	'%',	'^',
	'&',	'*',	'(',	')',	'_',	'+',	'\b',	0,
/* 1 */	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
	'O',	'P',	'{',	'}',	'\r',	0,	'A',	'S',
/* 2 */	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
	'"',	'~',	0,	'|',	'Z',	'X',	'C',	'V',
/* 3 */	'B',	'N',	'M',	'<',	'>',	'?',	0,	0,
	0,	' ',	0,	0,	0,	0,	0,	0,
/* 4 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 5 */	0,	0,	0,	'\177',	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 6 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 7 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0
};
#define CL 0x40
unsigned char Vpix_chars_c[ 128 ] = 
{
	/* 0	 1	 2	 3	 4	 5	 6	 7	*/
	/* 8	 9	 A	 B	 C	 D	 E	 F	*/
/* 0 */	0,	'\033',	'1',	'2',	'3',	'4',	'5',	'6',
	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	0,
/* 1 */	'Q'-CL,	'W'-CL,	'E'-CL,	'R'-CL,	'T'-CL,	'Y'-CL,	'U'-CL,	'I'-CL,
	'O'-CL,	'P'-CL,	'[',	']',	'\r',	0,	'A'-CL,	'S'-CL,
/* 2 */	'D'-CL,	'F'-CL,	'G'-CL,	'H'-CL,	'J'-CL,	'K'-CL,	'L'-CL,	';',
	'\'',	'`',	0,	'\\',	'Z'-CL,	'X'-CL,	'C'-CL,	'V'-CL,
/* 3 */	'B'-CL,	'N'-CL,	'M'-CL,	',',	'.',	'/',	0,	0,
	0,	' ',	0,	0,	0,	0,	0,	0,
/* 4 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 5 */	0,	0,	0,	'\177',	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 6 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0,
/* 7 */	0,	0,	0,	0,	0,	0,	0,	0,
	0,	0,	0,	0,	0,	0,	0,	0
};
#define SC_SHIFT		0x2A
#define SC_SHIFT_RIGHT		0X36
#define SC_CONTROL		0x1D
#endif
#include "touserkey.h"
/**************************************************************************
* key_read
*	Read a key.
*	If a key read packet from an ioctl is in progress, it comes from
*		the packet. If the character is go back to the keyboard
*		then do it.
*	If we had keys that looked like they were special but were not
*		use them until they are gone.
*	"tenths" is a number of tenths of seconds to wait for a keystroke.
*		0 means do not time.
*	If the keyboard is in scan code mode, turn it back into ascii.
*	If you read the first part of a break - read the rest.
*	If you read the first part of a special key - read the rest and
*		encode.
*	Subject the result to the possible language translation in "mapping".
**************************************************************************/
key_read( tenths, mapping )
	int tenths;
	char	*mapping;
{
	int c;
	char	*s;
	char	*command_map();
#if defined( VPIX ) || defined( SOFTPC )
	int	scan_code;
#endif

	term_outgo();
	if ( Key_read_pkt_active )
	{
		c = key_read_pkt();
		if ( c == Window_mode_to_user_char )
			key_read_pkt_end();
		else
			return( c );
	}
	if ( (c = dequeue_key() ) >= 0 )		/* previous unknown */
		return( c );
	if ( (c = ftget_char( tenths ) ) < 0 )		/* timeout */
		return( -1 );
#if defined( VPIX ) || defined( SOFTPC )
	scan_code = 0;
#ifdef SOFTPC
	if ( terminal_mode_is_scan_code_mode() )
		scan_code = 1;
#endif
	if ( scan_code )
	{
		int cc;
		while ( 1 )
		{
			cc = ( (int) c ) & 0xFF;
			if      ( cc == SC_SHIFT )
				Vpix_shift = 1;
			else if ( cc == ( SC_SHIFT | 0x80 ) )
				Vpix_shift = 0;
			else if ( cc == SC_SHIFT_RIGHT )
				Vpix_shift_right = 1;
			else if ( cc == ( SC_SHIFT_RIGHT | 0x80 ) )
				Vpix_shift_right = 0;
			else if ( cc == SC_CONTROL )
				Vpix_control = 1;
			else if ( cc == ( SC_CONTROL | 0x80 ) )
				Vpix_control = 0;
			else if ( cc == 0x4D )
				return( FTKEY_RIGHT );
			else if ( cc == 0x4B )
				return( FTKEY_LEFT );
			else if ( cc == 0x48 )
				return( FTKEY_UP );
			else if ( cc == 0x50 )
				return( FTKEY_DOWN );
			else if ( cc < 0x80 )
			{
				if ( Vpix_shift || Vpix_shift_right )
					cc = ( (int) Vpix_chars_s[cc] ) & 0xFF;
				else if ( Vpix_control )
					cc = ( (int) Vpix_chars_c[cc] ) & 0xFF;
				else
					cc = ( (int) Vpix_chars[cc] ) & 0xFF;
				if ( cc > 0 )
					return( cc );
			}

			if ( (c = ftget_char( tenths ) ) < 0 )	/* timeout */
				return( -1 );
		}
	}
#endif
	if ( ( c == 0x00FF ) && Opt_use_PARMRK_for_break )
	{
		if ( c = ftget_char( 1 ) < 0 ) /* timeout */
		{
			term_beep();
			return( 0x00FF );
		}
		if ( c == 0x00FF )
			return( 0x00FF );
		if ( c != 0 )
		{
			term_beep();
			enqueue_key( c );
			return( 0x00FF );
		}
		if ( c = ftget_char( 1 ) < 0 ) /* timeout */
		{
			term_beep();
			enqueue_key( 0 );
			return( 0x00FF );
		}
		if ( c != 0 )
		{
			term_beep();
			enqueue_key( 0 );
			enqueue_key( c );
			return( 0x00FF );
		}
		return( FTKEY_BREAK );
	}
	if ( F_input_8_bit == 0 )
		c &= 0x7F;
	else
		c &= 0xFF;
	if ( Special_key[ c ] )			/* special */
	{
		enqueue_key( c );
		if ( ( c = check_for_key( c ) ) >= 0 )
			clear_key_queue();
		else
			c = dequeue_key();
	}
	if ( ( s = command_map( c, mapping ) ) != (char *) 0 )
	{
		unsigned char	*p;

		p = ( unsigned char *) s;
		p++;
		while ( ( ( c = *p++ ) != '\0' ) && ( c != 0x0080 ) )
			enqueue_key( c );
		return( *s );
	}
	return( c );
}
#include "signal.h"
#include "errno.h"
#include "keystroke.h"
/**************************************************************************
* ftget_char
*	Get a key with the possible timing specified if "tenths" is greater
*		than 0.
*	Keys read previously that are in the keys pipe go first.
*	Keystroke replay in progress may supply keys.
*	If keystroke capture is in progress, record the keystrokes.
**************************************************************************/
ftget_char( tenths )
	int	tenths;
{
	char	c;
	int	status;
	int	cc;

	term_outgo();
	if ( ( cc = chk_keys_from_recv() ) >= 0 )
		return( cc );
	if ( Keystroke_play_fd )
		status = keystroke_play_read( &c );
	if ( Keystroke_play_fd == 0 )
	{
		if ( tenths != Key_timing )
		{
			if ( tenths )
				termio_window_timed( 0, tenths );
			else
				termio_window();
			Key_timing = tenths;
		}
		status = read( 0, &c, 1 );
	}
	if ( status == 0 )
	{
		if ( Keystroke_capture_fd )
			keystroke_capture_timeout();
		return( -1 );
	}
	else if ( status < 0 )
		fsend_kill( "Facet process - sender read", -1 );
	else
	{
		if ( Keystroke_capture_fd )
			keystroke_capture_key( &c, 1 );
		return( ( (int) c ) & 0xFF );
	}
	/* NOTREACHED */
}
/**************************************************************************
* ftrawread
*	Read straight from the keyboard - skipping the buffering, replaying,
*	etc.
**************************************************************************/
ftrawread()
{
	char	c;
	int	status;

	status = read( 0, &c, 1 );
	if ( status == 0 )
		fsend_kill( "Facet process - sender raw read", 0 );
	else if ( status < 0 )
		fsend_kill( "Facet process - sender raw read", -1 );
	else
		return( ( (int) c ) & 0xFF );
	/* NOTREACHED */
}
/**************************************************************************
* check_for_key
*	See if the key "c" is the beginning of a special sequence.
*	This is a recursive procedure using check_keys_alsos below.
**************************************************************************/
check_for_key( c )
	int	c;
{
	return( check_key_alsos( Key_root->next, c ) );
}
/**************************************************************************
* check_key_alsos
**************************************************************************/
check_key_alsos( pkey, c )	
	FT_KEY	*pkey;
	int	c;
{
	int	value;

	while ( pkey != NULL )
	{
		if ( pkey-> code == c )
		{
			value = pkey->value;
			pkey = pkey->next;
			if ( pkey == NULL )
				return( value );
			else
			{
				if ( (c = ftget_char( 5 ) ) < 0 )
					return( -1 );
				if ( F_input_8_bit == 0)
					c &= 0x7F;
				else
					c &= 0xFF;
				enqueue_key( c );
				return( check_key_alsos( pkey, c ) );
			}
		}
		pkey = pkey->also;
	}
	return( -1 );
}
struct key_list
{
	char	*key_name;
	int	key_value;
};
typedef struct key_list KEY_LIST;
KEY_LIST Key_list[] = 
{
#include "ftkeys.h"
};
/**************************************************************************
* extra_keys
*	TERMINAL DESCRIPTION PARSER module for "key_...".
**************************************************************************/
/*ARGSUSED*/
extra_keys( key_name, key_string, substring1, substring2 )
	char	*key_name;
	char	*key_string;
	char	*substring1;		/* not used */
	char	*substring2;		/* not used */
{
	KEY_LIST	*k;
	char		*dec_encode();
	char		*encoded;

	for ( k = &Key_list[ 0 ]; k->key_name[0] != '\0'; k++ )
	{
		if ( strcmp( key_name, k->key_name ) == 0 )
		{
			encoded = dec_encode( key_string );
			key_install( key_name, encoded, k->key_value );
			return( 1 );
		}
	}
	return( 0 );
}
#ifdef lint
static int lint_alignment_warning_ok_1;
#endif
