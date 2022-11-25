/*****************************************************************************
** Copyright (c) 1986 - 1991 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: ftwindow.c,v 70.1 92/03/09 15:44:08 ssa Exp $ */
/**************************************************************************
* ftwindow.c
*	Window management modules
**************************************************************************/

#include	"ftchar.h"
#include	"ftproc.h"
#include	<stdio.h>

#include "ftwindow.h"
#include "wins.h"

#include "ftterm.h"
#include "myattrs.h"
#include "modes.h"
#include "features.h"
#include "output.h"
#include "options.h"
#include "scroll.h"
int Rows_scroll_memory = 0;

char	Err_buff[ 80 ];

/**************************************************************************
* fct_linefeed
*	ACTION module for "linefeed=".
**************************************************************************/
fct_linefeed()
{
	int	scroll;
	int	csr_buff_bot_row;

	if ( Outwin->onstatus )
		return;
	Outwin->xenl = 0; /* ??? */
	if ( Outwin->real_xenl )
	{
		Outwin->real_xenl = 0;
		if ( Outwin->onscreen )
			term_linefeed();
		return;
	}
	csr_buff_bot_row = Outwin->csr_buff_bot_row;
        if ( Outwin->row < csr_buff_bot_row )
	{			/* just down if above bottom of region */
		Outwin->row++;
		d_sync_line_attribute_current();	/* linefeed down 1 */
		scroll = 0;
	}
	else if ( Outwin->row == csr_buff_bot_row )
	{			/* scroll if at bottom of region */
		if ( Outwin->auto_scroll_on )
		{
			d_scroll_forward();
			scroll = 1;
		}
		else if ( F_auto_scroll_off_wraps_top )
		{
			Outwin->row = 0;
			d_sync_line_attribute_current(); /* linefeed wrap */
			scroll = 0;
		}
		else
		{		
			return;
		}
	}
	else if ( Outwin->row < Outwin->display_row_bottom )
	{			/* just down if outside region */
		Outwin->row++;
		d_sync_line_attribute_current();	/* linefeed down 1 */
		scroll = 0;
	}
	else			/* sticks on bottom outside region */
		return;
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
		{
			term_linefeed();
		}
		else
		{				/* split screen */
			if ( scroll )
				s_scroll_forward();
			else if ( cursor_above_window() )
				s_pan_up_to_cursor();
			else if ( cursor_not_below_window() )
				term_linefeed();
			else
				s_pan_down( 1 );
		}
	}
}
/**************************************************************************
* fct_carriage_return
*	ACTION module for "carriage_return=".
**************************************************************************/
fct_carriage_return()
{
	Outwin->xenl = 0;
	/* Outwin->real_xenl stays on */
	Outwin->col = Outwin->wrap_margin_left;
	if ( Outwin->onscreen )
		term_carriage_return();
}
/**************************************************************************
* fct_tab
*	ACTION module for "tab".
**************************************************************************/
/* ARGSUSED */
fct_tab( not_control_i )
	int	not_control_i;
{
	int	scrolled;
	int	old_col;

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	old_col = Outwin->col;
	Outwin->col += ( 8 - (Outwin->col % 8 ) );
	if ( old_col <= Outwin->wrap_margin_right )
	{
		if ( Outwin->col > Outwin->wrap_margin_right )
		{
			if ( F_eat_newline_glitch[ Outwin->personality ] )
			{		/* tab doesnt set xenl */
				Outwin->col = Outwin->wrap_margin_right;
			}
			else if ( F_real_eat_newline_glitch )
			{		/* tab doesnt set xenl */
				Outwin->col = Outwin->wrap_margin_right;
			}
			else if ( Outwin->onstatus )
			{
				Outwin->col = Outwin->wrap_margin_right;
			}
			else				/* see wrap or not */
			{
				scrolled = ft_past_right_margin();
				/* ??? does tab wrap anyway
				** even if auto wrap off*/
				if (  scrolled 
				   && Outwin->onscreen && Outwin->fullscreen )
				{
					term_scroll_forward();
				}
			}
		}
	}
	else				/* outside of margins - no wrap */
	{
		if ( Outwin->col > Outwin->col_right_line )
			Outwin->col = Outwin->col_right_line;
	}
	t_cursor();
}
/**************************************************************************
* fct_back_tab
*	ACTION module for "back_tab=".
**************************************************************************/
fct_back_tab()
{ 
	int	back;
	int	old_col;

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	old_col = Outwin->col;
	back = Outwin->col % 8;
	if ( back == 0 )
		back = 8;
	Outwin->col -= back;
	if ( old_col >= Outwin->wrap_margin_left )
	{
		if ( Outwin->col < Outwin->wrap_margin_left )
		{
			if ( Outwin->onstatus )
			{
				Outwin->col = Outwin->wrap_margin_left;
			}
			else if ( F_auto_left_margin && ( Outwin->row > 0 ) )
			{
				Outwin->row--;
				d_sync_line_attribute_current(); /*back_tab up*/
				Outwin->col = Outwin->wrap_margin_right;
				Outwin->col -= ( Outwin->col % 8 );
			}
			else	/* no wrap or off top */
			{
				Outwin->col = Outwin->wrap_margin_left;
			}
		}
	}
	else
	{				/* outside margin - no wrap */
		if ( Outwin->col < 0 )
			Outwin->col = 0;
	}
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen == 0 )
		{
				/* ??? back_tab and change_scroll_region
				   combination not implemented */
			if ( cursor_above_window() )
				s_pan_up( 1 );
		}
		t_cursor();
	}
}
/**************************************************************************
* fct_backspace
*	ACTION module for "backspace=".
**************************************************************************/
fct_backspace()
{ 
	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
        if ( Outwin->col > Outwin->wrap_margin_left )
		Outwin->col--;
	else if ( Outwin->col == Outwin->wrap_margin_left )
	{
		if ( Outwin->onstatus )
			return;			/* would just stick - ignore */
		if ( F_auto_left_margin )
		{
			if ( Outwin->row > 0 )
			{
				Outwin->row--;
				d_sync_line_attribute_current(); /* bs up one */
				Outwin->col = Outwin->wrap_margin_right;
			}
			else
				return;		/* undefined off top - ignore */
		}
		else
			return;			/* would just stick - ignore */
	}
	else if ( Outwin->col > 0 )
	{
		Outwin->col--;
	}
	else
		return;				/* col 0 outside wrap - ignore*/
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_backspace();
		else
		{
				/* ??? auto_left_margin and change_scroll_region
				   combination not implemented */
			if ( cursor_not_above_window() )
				term_backspace();
			else
				s_pan_up( 1 );
		}
	}
}
/**************************************************************************
* cursor_above_window
*	Return non-zero if the row containing the cursor is above the
*	portion of the split screen visible on the terminal.
*	Return 0 otherwise
**************************************************************************/
cursor_above_window()
{
	return( Outwin->row < Outwin->buff_top_row );
}
/**************************************************************************
* cursor_not_above_window
*	Return non-zero if the row containing the cursor is visible on
*	the split screen.
*	Return 0 otherwise
**************************************************************************/
cursor_not_above_window()
{
	return( Outwin->row >= Outwin->buff_top_row );
}
#include "allowbeep.h"
int Allow_beep_offscreen[ TOPWIN ] = { 0 };
/**************************************************************************
* fct_beep
*	ACTION module for "bell".
**************************************************************************/
fct_beep()
{ 
	Outwin->real_xenl = 0;
	if ( Outwin->onscreen || Allow_beep_offscreen[ Outwin->number ] )
		term_beep();
}
/**************************************************************************
* fct_char
*	ACTION module for single characters not recognized by the
*	decoder.
*	Output and record with the current attribute if they look displayable.
*	Complain otherwise.
**************************************************************************/
fct_char( c )
	UNCHAR	c;
{ 
	REG	int	cc;
	REG	FT_WIN	*outwin;	

	cc = c;
	outwin = Outwin;
	if (  ( F_graphics_8_bit == 0 )
	   && ( outwin->graphics_8_bit == 0 ) )
	{
		cc &= 0x7F;
	}
	else
		cc &= 0xFF;
	if ( cc < 0x7F )
	{
		if ( cc < 0x20 )
		{
			if ( cc == 0x1B )
			{
				if ( Opt_error_control_ignore )
				{
				}
				else if ( Opt_error_control_pass )
				{
					term_putc( cc );
				}
				else if ( Opt_error_ignore )
				{
				}
				else if ( Opt_error_pass )
				{
					term_putc( cc );
				}
				else
				{
					fct_char( (UNCHAR) '\\' );
					fct_char( (UNCHAR) ( 'E' ) );
					term_beep();
					term_outgo();
					error_record_char( cc );
					term_beep();
				}
				return;
			}
			else
			{
				if ( Opt_error_control_ignore )
				{
				}
				else if ( Opt_error_control_pass )
				{
					term_putc( cc );
				}
				else if ( Opt_error_ignore )
				{
				}
				else if ( Opt_error_pass )
				{
					term_putc( cc );
				}
				else
				{
					fct_char( (UNCHAR) '^' );
					fct_char( (UNCHAR) ( cc + '@' ) );
					term_beep();
					term_outgo();
					error_record_char( cc );
					term_beep();
				}
				return;
			}
		}
		else if ( F_tilde_glitch && ( cc == '~' ) )
		{
			fct_char( (UNCHAR) '^' );
			fct_char( (UNCHAR) '>' );
			return;
		}
	}
	else if ( ( cc == 0x7F ) && ( mF_delete_is_a_character == 0 ) )
	{
		if ( Opt_error_control_ignore )
		{
		}
		else if ( Opt_error_control_pass )
		{
			term_putc( cc );
		}
		else if ( Opt_error_ignore )
		{
		}
		else if ( Opt_error_pass )
		{
			term_putc( cc );
		}
		else
		{
			fct_char( (UNCHAR) '^' );
			fct_char( (UNCHAR) '?' );
			term_beep();
			term_outgo();
			error_record_char( cc );
			term_beep();
		}
		return;
	}
	outwin->real_xenl = 0;
	if ( outwin->xenl )
		ft_xenl_auto_wrap();
	if ( outwin->onscreen )
	{
		term_putc( (char ) cc );
		if ( outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) ( (FTCHAR) cc | outwin->ftattrs ) );
}
/**************************************************************************
* fct_char_logged
*	If the character "c" looks displayable, output it and record it
*	with the current attribute.
*	Otherwise output it in a displayable manner and record. Do not
*	record an error in this case since the calling module has recorded
*	an error or the current window is in monitor mode.
**************************************************************************/
fct_char_logged( c )
	UNCHAR	c;
{ 
	REG	int	cc;
	REG	FT_WIN	*outwin;	

	cc = c;
	outwin = Outwin;
	if (  ( F_graphics_8_bit == 0 )
	   && ( outwin->graphics_8_bit == 0 ) )
	{
		cc &= 0x7F;
	}
	else
		cc &= 0xFF;
	if ( cc < 0x7F )
	{
		if ( cc < 0x20 )
		{
			if ( cc == 0x1B )
			{
				fct_char( (UNCHAR) '\\' );
				fct_char( (UNCHAR) ( 'E' ) );
				return;
			}
			else
			{
				fct_char( (UNCHAR) '^' );
				fct_char( (UNCHAR) ( cc + '@' ) );
				return;
			}
		}
		else if ( F_tilde_glitch && ( cc == '~' ) )
		{
			fct_char( (UNCHAR) '^' );
			fct_char( (UNCHAR) '>' );
			return;
		}
	}
	else if ( cc == 0x7F )
	{
		fct_char( (UNCHAR) '^' );
		fct_char( (UNCHAR) '?' );
		return;
	}
	outwin->real_xenl = 0;
	if ( outwin->xenl )
		ft_xenl_auto_wrap();
	if ( outwin->onscreen )
	{
		term_putc( (char ) cc );
		if ( outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) ( (FTCHAR) cc | outwin->ftattrs ) );
}
/**************************************************************************
* fct_nomagic
*	ACTION module for "nomagic=".
*	A sequence that takes up a character space that prevents the
*	propogation of 'magic cookie' attributes.
**************************************************************************/
fct_nomagic()
{
	Outwin->real_xenl = 0;
	if ( Outwin->xenl )
		ft_xenl_auto_wrap();
	if ( Outwin->onscreen )
	{
		term_nomagic();
		if ( Outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) ( Outwin->ftattrs | ATTR_MAGIC ) );
}
/**************************************************************************
* fct_magic
*	ACTION module for "magic=".
*	This is primarily a sequence that takes a space on the terminal and
*	causes the propogation of 'magic cookie' attributes but may be used
*	for any arbitrary sequence that takes a space.
*	The 'magic cookie' sequences are numbered in order, according to
*	their position in the terminal description file, starting at 1.
*	"magicno" is the index specifying the sequence.
**************************************************************************/
fct_magic( magicno )
{
	Outwin->real_xenl = 0;
	if ( Outwin->xenl )
		ft_xenl_auto_wrap();
	if ( Outwin->onscreen )
	{
		term_magic( magicno );
		if ( Outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) (Outwin->ftattrs | ATTR_MAGIC | magicno) );
}
typedef struct t_attribute_next_struct T_ATTRIBUTE_NEXT;
/**************************************************************************
* fct_attribute_next
*	ACTION module for "attribute_next=".
*	"t_attribute_next_ptr" is a pointer to a structure holding the
*		details from the terminal description file.
*	"c" is the character to which the sequence applies.
*	"ftattrs_on" are the extra attributes to be assigned to "c".
**************************************************************************/
fct_attribute_next( t_attribute_next_ptr, c, ftattrs_on )
	T_ATTRIBUTE_NEXT	*t_attribute_next_ptr;
	UNCHAR	c;
	FTCHAR	ftattrs_on;
{
	Outwin->real_xenl = 0;
	if ( Outwin->xenl )
		ft_xenl_auto_wrap();
	if ( Outwin->onscreen )
	{
		term_attribute_next( t_attribute_next_ptr );
		term_putc( (char ) c );
		if ( Outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) ( c | Outwin->ftattrs | ftattrs_on ) );
}
/**************************************************************************
* fct_graphics_escape_control
*	ACTION module for the control character "c" was preceeded by an
*	escape on a terminal that had the terminal description file 
*	capablility 'graphics_escape_control'.
**************************************************************************/
fct_graphics_escape_control( c )
	UNCHAR	c;
{
	Outwin->real_xenl = 0;
	if ( Outwin->xenl )
		ft_xenl_auto_wrap();
	if ( Outwin->onscreen )
	{
		term_putc( '\033' );
		term_putc( (char ) c );
		if ( Outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) ( (FTCHAR) c | Outwin->ftattrs ) );
}
/**************************************************************************
* fct_single_shift
*	ACTION module for 'single_shift_0', 'single_shift_1', 'single_shift_2',
*	or 'single_shift_3'.
*	"set" specifies which 'single_shift'.
*	"c" is the character to which it applies.
*	"ftattrs_on" are the attributes which it adds to "c".
**************************************************************************/
fct_single_shift( set, c, ftattrs_on )
	int	set;
	UNCHAR	c;
	FTCHAR	ftattrs_on;
{
	Outwin->real_xenl = 0;
	if ( Outwin->xenl )
		ft_xenl_auto_wrap();
	if ( Outwin->onscreen )
	{
		term_single_shift( set );
		term_putc( (char ) c );
		if ( Outwin->insert_mode )
			term_insert_padding();
	}
	fct_d_char( (FTCHAR) ( c |
			(Outwin->ftattrs & (~F_character_set_attributes)) | 
			ftattrs_on ) );
}
#include "hpattr.h"
/**************************************************************************
* fct_d_char
*	Record the character and attributes "c" at the current cursor
*	position in the window "Outwin".
**************************************************************************/
fct_d_char( c )
	FTCHAR		c;
{
	REG	FT_WIN	*outwin;	
	REG	int	col;
	REG	int	row;
	FTCHAR		*cp;

	outwin = Outwin;
	row = Outwin->row;
	col = Outwin->col;
	if ( outwin->insert_mode )
		d_insert_character( row, col );
	if ( oF_hp_attribute == 0 )
		outwin->ftchars[ row ][ col ] = c;
	else
	{
						/* see also fct_decode for
						   same operation in optimized
						   all character loop.
						*/ 
		cp = &outwin->ftchars[ row ][ col ];
		*cp &= ( ~ FTCHAR_CHAR_MASK );
		*cp |= ( c | ATTR_HP_CHAR_PRESENT );
	}
	outwin->row_changed[ row ] = 1;
	outwin->col_changed[ col ] = 1;
	if ( col < outwin->wrap_margin_right )
		outwin->col++;
	else if ( col == outwin->wrap_margin_right )
	{
		outwin->col++;
		ft_past_right_margin();
	}
	else if ( col < outwin->col_right_line )
		outwin->col++;
}
/**************************************************************************
* ft_xenl_auto_wrap
*	The window "Outwin" is about to do an "eat newline glitch" auto
*	wrap in preparation for outputting a character.
*	Adjust the cursor and set split screen if necessary.
**************************************************************************/
ft_xenl_auto_wrap()
{
	Outwin->xenl = 0;
	ft_auto_wrap();
}
/**************************************************************************
* ft_past_right_margin
*	Cursor went past the right margin - what did the terminal do?
*	1 = scroll 0 = no scroll
**************************************************************************/
ft_past_right_margin()
{
	if ( Outwin->onstatus )
	{				/* stuck */
		Outwin->col = Outwin->wrap_margin_right;
		return( 0 );
	}
	else if ( Outwin->auto_wrap_on == 0 )
	{				/* stuck */
		Outwin->col = Outwin->wrap_margin_right;
		return( 0 );
	}
	else if ( F_eat_newline_glitch[ Outwin->personality ] )
	{				/* stuck - into xenl mode */
		Outwin->col = Outwin->wrap_margin_right;
		Outwin->xenl = 1;
		return( 0 );
	}
	else				/* wrapped */
	{
		if ( F_real_eat_newline_glitch )
			Outwin->real_xenl = 1;
		return ( ft_auto_wrap() );
	}
}
/**************************************************************************
* ft_auto_wrap
*	Terminal wraped (or xenl - will before operation)
*	1 = scrolled   0 = no scroll
**************************************************************************/
ft_auto_wrap()	
{
	Outwin->col = Outwin->wrap_margin_left;
	if ( Outwin->row == Outwin->csr_buff_bot_row )
	{					/* auto-wrap scroll */
		if ( Outwin->auto_scroll_on )
		{
			d_scroll_forward();
			if (  ( Outwin->onscreen )
			   && ( Outwin->fullscreen == 0 ) )
			{	/* terminal would have scrolled if fullscreen */
				/* cursor will be on divider line */
				s_scroll_forward();
			}
			return( 1 );			/* scroll */
		}
		else if ( F_auto_scroll_off_wraps_top )
		{
			Outwin->row = 0;
			d_sync_line_attribute_current();/* autowrap scroll off*/
			if (  ( Outwin->onscreen ) 
			   && ( Outwin->fullscreen == 0 ) )
			{	/* terminal may have wrapped out of window */
				s_cursor_w_pan();
			}
		}
		else
		{				/* auto_wrap - same line */
			if (  ( Outwin->onscreen )
			   && ( Outwin->fullscreen == 0 ) )
			{
				s_cursor();
			}
		}
	}
	else if ( Outwin->row >= Outwin->display_row_bottom )
	{					/* auto_wrap - same line */
		if ( ( Outwin->onscreen ) && ( Outwin->fullscreen == 0 ) )
			s_cursor();
	}
	else
	{					/* auto-wrap no scroll*/
		Outwin->row++;
		d_sync_line_attribute_current();	/* autowrap down one */
		if ( ( Outwin->onscreen ) && ( Outwin->fullscreen == 0 ) )
		{	/* terminal may have wrapped out of window */
			/* no action if cursor on scr */
			s_pan_down_to_cursor();
		}
	}
	return( 0 );
}
#include "meta.h"
/**************************************************************************
* fct_cursor_address
*	ACTION module for 'cursor_address='.
*	Positions cursor to "row" and "col".
**************************************************************************/
fct_cursor_address( row, col )
	int row;
	int col;
{ 
	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	Outwin->onstatus = 0;
	if ( row < 0 )
		Outwin->row = 0;
	else if ( row > Outwin->display_row_bottom )
		Outwin->row = Outwin->display_row_bottom;
	else
		Outwin->row = row;
	if ( col < 0 )
		Outwin->col = 0;
	else if ( col > Outwin->col_right )
		Outwin->col = Outwin->col_right;
	else
		Outwin->col = col;
	d_sync_line_attribute_current();/* may set col or xenl -line_attribute*/
	if ( Outwin->col > Outwin->col_right_line )
	{	
		Outwin->col = Outwin->col_right_line;
		if (  F_eat_newline_glitch[ Outwin->personality ] )
			Outwin->xenl = 1;
		/* F_real_eat_newline_glitch not on line attribute terminals */
	}
	/******************************************************************
	* Positioning the cursor on an HP terminal changes the character under
	* the cursor from a blank place on the screen to a "real" blank.
	******************************************************************/
	if ( oF_hp_attribute )
	{
		int	count;
		int	real_row;

		count = oT_meta_roll_count;
		if ( count <= 0 )
			Outwin->ftchars[ Outwin->row ][ Outwin->col ] |= 
							ATTR_HP_CHAR_PRESENT;
		else 
		{
			real_row = Outwin->row - count;
			if ( real_row >= 0 )
				Outwin->ftchars[ real_row ][ Outwin->col ] |= 
							ATTR_HP_CHAR_PRESENT;

		}
	}
	t_cursor();
}
/**************************************************************************
* fct_row_address
*	ACTION module for 'row_address='.
*	Position cursor to row number "row" in the same column.
**************************************************************************/
fct_row_address( row )
	int row;
{ 

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	Outwin->onstatus = 0;
	if ( row < 0 )
		Outwin->row = 0;
	else if ( row > Outwin->display_row_bottom )
		Outwin->row = Outwin->display_row_bottom;
	else
		Outwin->row = row;
	d_sync_line_attribute_current();/* may set col or xenl -line_attribute*/
	if ( Outwin->col > Outwin->col_right_line )
	{	
		Outwin->col = Outwin->col_right_line;
		if (  F_eat_newline_glitch[ Outwin->personality ] )
			Outwin->xenl = 1;
		/* F_real_eat_newline_glitch not on line attribute terminals */
	}
	t_cursor();
}
/**************************************************************************
* fct_column_address
*	ACTION module for 'column_address='.
*	Position cursor to column "col" on the same row.
**************************************************************************/
fct_column_address( col )
	int col;
{ 

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	if ( col < 0 )
		Outwin->col = 0;
	else if ( col > Outwin->col_right )
		Outwin->col = Outwin->col_right;
	else
		Outwin->col = col;
	t_cursor();
}
/**************************************************************************
* fct_cursor_down
*	ACTION module for 'cursor_down='.
*	Move the cursor down "rows" number of lines.
**************************************************************************/
fct_cursor_down( rows )
        int     rows;
{ 
	int	downrows;
	int	max;

	if ( Outwin->onstatus ) 
		return;
	Outwin->real_xenl = 0;
	if ( Outwin->row <= Outwin->csr_buff_bot_row )
		max = Outwin->csr_buff_bot_row - Outwin->row; /* bot of region*/
	else 
		max = Outwin->display_row_bottom - Outwin->row; /* bot screen*/
	if ( rows <= max )
		downrows = rows;
	else
		downrows = max;
	Outwin->row += downrows;
	d_sync_line_attribute_current();	/* cursor down ??? multiple */

	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_down_mul( downrows );
		else
		{
			if ( s_pan_down_to_cursor() > 0 )
				return;		/* had to pan down -cursor set*/
			term_down_mul( downrows );
		}
	}
}
/**************************************************************************
* fct_cursor_up
*	ACTION module for 'cursor_up='.
*	Move the cursor up "rows" number of lines.
**************************************************************************/
fct_cursor_up( rows )			/* assumes no cursor wrap */
        int     rows;
{ 
	int	uprows;
	int	max;

	if ( Outwin->onstatus ) 
		return;
	Outwin->real_xenl = 0;
	if ( Outwin->row >= Outwin->csr_buff_top_row )
		max = Outwin->row - Outwin->csr_buff_top_row; /* top of region*/
	else 
		max = Outwin->row;			      /* top of screen*/
	if ( rows <= max )
		uprows = rows;
	else
		uprows = max;
	if (  ( uprows == 0 ) && F_cursor_up_at_home_wraps_ll
	   && ( Outwin->row == 0 ) && ( Outwin->col == 0 ) )
	{
		if ( rows == 1 )
			fct_cursor_address( Outwin->display_row_bottom, 0 );
		else
			term_beep();
		return;
	}
	Outwin->row -= uprows;
	d_sync_line_attribute_current();	/* cursor up ??? multiple */
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_up_mul( uprows );
		else
		{
			if ( s_pan_up_to_cursor() > 0 )
				return;
			term_up_mul( uprows );
		}
	}
}
/**************************************************************************
* fct_cursor_right
*	ACTION module for 'cursor_right='.
*	Move the cursor "cols" number of columns to the right.
**************************************************************************/
fct_cursor_right( cols )
        int     cols;
{ 
	int	newcol, rightcols;

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
        newcol = Outwin->col + cols;
	if ( Outwin->col <= Outwin->wrap_margin_right )
	{
		if ( newcol > Outwin->wrap_margin_right )
			newcol = Outwin->wrap_margin_right;
	}
	else 
	{
		if ( newcol > Outwin->col_right_line )
			newcol = Outwin->col_right_line;
	}
	rightcols = newcol - Outwin->col;
	if ( rightcols <= 0 )
		return;
	Outwin->col = newcol;
	if ( Outwin->onscreen )
		term_right_mul( rightcols );
}
/**************************************************************************
* fct_cursor_left
*	ACTION module for 'cursor_left'.
*	Move the cursor "cols" number of columns to the left.
**************************************************************************/
fct_cursor_left( cols )
        int     cols;
{ 
	int	newcol, leftcols;

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
        newcol = Outwin->col - cols;
	if ( Outwin->col >= Outwin->wrap_margin_left )
	{
		if ( newcol < Outwin->wrap_margin_left )
			newcol = Outwin->wrap_margin_left;
	}
	else
	{
		if ( newcol < 0 )
			newcol = 0;
	}
	leftcols = Outwin->col - newcol;
	if( leftcols <= 0 )
		return;
	Outwin->col = newcol;
	if ( Outwin->onscreen )
		term_left_mul( leftcols );
}
/**************************************************************************
* fct_insert_character
*	ACTION module for 'insert_character='.
*	Insert "cols" number of blanks at the current cursor position.
**************************************************************************/
fct_insert_character( cols )
	int	cols;
{ 
	int	i;

	Outwin->real_xenl = 0;
	for ( i = 0; i < cols; i++ )
		d_insert_character( Outwin->row, Outwin->col );
	if ( Outwin->onscreen )
		term_parm_ich( cols );
}
/**************************************************************************
* fct_enter_insert_mode
*	ACTION module for 'enter_insert_mode='.
*	Put the window in insert mode until an exit insert mode sequence
*	is seen.
**************************************************************************/
fct_enter_insert_mode()
{
	Outwin->real_xenl = 0;
	Outwin->insert_mode = 1;
	if ( Outwin->onscreen )
		term_enter_insert_mode();
}
/**************************************************************************
* fct_exit_insert_mode
*	ACTION module for 'exit_insert_mode='.
*	Turn off insert mode for this window.
**************************************************************************/
fct_exit_insert_mode()
{
	Outwin->real_xenl = 0;
	Outwin->insert_mode = 0;
	if ( Outwin->onscreen )
		term_exit_insert_mode();
}
/**************************************************************************
* win_se_insert_mode
*	An event has occurred that has the side effect of turning off
*	insert mode on this window.
**************************************************************************/
win_se_insert_mode( insert_mode )
	int	insert_mode;
{
	Outwin->insert_mode = insert_mode;
}
/**************************************************************************
* fct_delete_character
*	ACTION module for 'delete_character=' and 'parm_delete_character='.
*	Delete "cols" number of characters at the current cursor position.
**************************************************************************/
fct_delete_character( cols )
	int	cols;
{ 
	int	i;

	Outwin->real_xenl = 0;
	for ( i = 0; i < cols; i++ )
		d_delete_character( Outwin->row, Outwin->col );
	if ( Outwin->onscreen )
		term_parm_delete_character( cols );
}
/**************************************************************************
* fct_insert_line
*	ACTION module for 'insert_line=' and 'parm_insert_line='.
*	Insert "rows" number of lines at the current cursor position.
**************************************************************************/
fct_insert_line( rows )
	int	rows;
{ 
	int	i;
	int	row;

	if ( Outwin->onstatus )
	{
		term_beep();
		return;
	}
	Outwin->real_xenl = 0;
	row = Outwin->row;
	if (  ( row < Outwin->csr_buff_top_row ) 
	   || ( row > Outwin->csr_buff_bot_row ) )
	{			/* not valid outside scroll region */
		term_beep();
		return;
	}

	if ( oF_insdel_line_move_col0 )
		Outwin->col = 0;
	for ( i = 0; i < rows; i++ )
		d_insert_line( row );
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
		{
			if ( mF_insert_line_needs_clear_glitch && ( rows > 0 ) )
			{
				term_pos( Rows_terminal - rows, 0 );
								/* affrow */
				term_out_parm_delete_line( rows,
						       Rows_terminal - rows );
				term_pos( Outwin->row, Outwin->col );
			}
			term_parm_insert_line( rows, row ); /* affrow */
		}
		else
		{
			for ( i = 0; i < rows; i++ )
				s_insert_line();
		}
	}
}
/**************************************************************************
* s_insert_line
*	Insert a line at the cursor on an onscreen split window.
**************************************************************************/
s_insert_line()
{
	int	affrow;

	term_pos( Outwin->win_bot_row, 0 );
	term_out_delete_line( Outwin->win_bot_row );
	s_cursor();
	affrow = Outwin->win_top_row;	/* ??? to be safe */
	term_insert_line( affrow );
}
/**************************************************************************
* fct_delete_line
*	ACTION module for 'delete_line=' and 'parm_delete_line=.
*	Delete "rows" number of lines at the current cursor position.
**************************************************************************/
fct_delete_line( rows )
	int	rows;
{ 
	int	i;
	int	row;

	if ( Outwin->onstatus )
	{
		term_beep();
		return;
	}
	Outwin->real_xenl = 0;
	row = Outwin->row;
	if (  ( row < Outwin->csr_buff_top_row ) 
	   || ( row > Outwin->csr_buff_bot_row ) )
	{			/* not valid outside scroll region */
		term_beep();
		return;
	}

	if ( oF_insdel_line_move_col0 )
		Outwin->col = 0;
	for ( i = 0; i < rows; i++ )
		d_delete_line( row );
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_parm_delete_line( rows, row ); /* affrow */
		else
			s_delete_line( rows, row );
	}
}
/**************************************************************************
* s_delete_line
*	Delete "rows" number of lines at row "row" on a onscren split window.
**************************************************************************/
s_delete_line( rows, row )
	int	rows;
	int	row;
{
	int	affrow;			/* top affected row for delays */
	int	rows_on_screen;		/* # of "rows" visible */
	int	i;
	int	full_line;		/* caused an auto_wrap */
	int	screen_row;

	t_sync_attr( (FTCHAR) 0 );
	rows_on_screen = 1 + Outwin->buff_bot_row - row;
	if ( rows < rows_on_screen )
		rows_on_screen = rows;
					/* delete the lines */
	affrow = Outwin->win_top_row; /* ??? to be safe - not same as row */
	term_parm_delete_line( rows_on_screen, affrow );
					/* push the divider back down */
	affrow = Outwin->win_bot_row + 1 - rows_on_screen;
	screen_row = affrow;
	term_pos( screen_row, 0 );
				/* originaly turned attributes off around
				   term_out_parm_insert_line only if 
				   F_insert_line_sets_attributes.  Other
				   attributes (e.g. SCO graphics) can
				   cause CR and LF to display as char so
				   just turned attributes off in here.
				*/
	term_out_parm_insert_line( rows_on_screen, affrow );
					/* now at top of empty screen */
	for ( i = rows_on_screen - 1; i >= 0; i-- )
	{
		full_line = toscreen_row( Outwin,
					    Outwin->buff_bot_row - i, 
					    Outwin->win_bot_row - i,
					    NO_ATTRS_AT_END );
		term_carriage_return();
		screen_row++;
		term_pos( screen_row, 0 );
	}
	t_sync_attr( Outwin->ftattrs );
	s_cursor();
}
/**************************************************************************
* fct_cursor_home_down
*	ACTION module for 'cursor_home_down'.
*	Move to the lowest non-empty line on the screen or scroll if none.
**************************************************************************/
fct_cursor_home_down()
{ 
	int	row;

	Outwin->xenl = 0;
	Outwin->real_xenl = 0;
	Outwin->onstatus = 0;
	/**********************************************************
	** Find lowest non-empty line.
	**********************************************************/
	for ( row = Outwin->display_row_bottom; row >= 0; row-- )
	{
		int	len;
		int	fnb_col;
		int	full_line;
		FTCHAR	*string;

		len = get_non_blank( Outwin, Outwin->ftchars[ row ], 
					Outwin->line_attribute[ row ],
					&fnb_col, &full_line,  &string );
		if ( len > 0 )
			break;
	}
	row++;
	if ( row > Outwin->display_row_bottom )
	{
		d_scroll_forward();
		Outwin->row = Outwin->display_row_bottom;
		Outwin->col = 0;
		d_sync_line_attribute_current();
		if ( Outwin->onscreen )
		{
			if ( Outwin->fullscreen )
			{
				term_pos( Outwin->row, Outwin->col );
				term_scroll_forward();
				term_pos( Outwin->row, Outwin->col );
			}
			else
			{
				s_cursor_w_pan();
				s_scroll_forward();
			}
		}
	}
	else
	{
		Outwin->row = row;
		Outwin->col = 0;
		d_sync_line_attribute_current();
		t_cursor();
	}
}
/**************************************************************************
* fct_scroll_forward
*	ACTION module for 'scroll_forward=' and 'parm_index='.
*	Scroll the screen "rows" number of lines.
**************************************************************************/
fct_scroll_forward( rows )
	int	rows;
{ 
	int	i;
	int	scroll;

	if ( Outwin->onstatus )
	{
		term_beep();
		return;
	}
	Outwin->real_xenl = 0;
	for ( i = 0; i < rows; i++ )
	{
		/* ??? maybe make this a multiple scroll */
		/* ??? assumes cursor stays put */
		if ( oF_scroll_could_be_cursor_only )
		{
			if ( Outwin->row == Outwin->csr_buff_bot_row )
			{
				scroll = 1;
				d_scroll_forward();
			}
			else if ( Outwin->row >= Outwin->display_row_bottom )
			{
				continue;   /* doesnt scroll outside region */
			}
			else
			{
				scroll=0;
				Outwin->row++;
				d_sync_line_attribute_current();/*scrollcur dn*/
			}
		}
		else
		{
			scroll = 1;
			d_scroll_forward();
		}
		if ( Outwin->onscreen )
		{
			if ( Outwin->fullscreen )
				term_scroll_forward();
			else 
			{
				if ( scroll )
					s_scroll_forward();
				else if ( cursor_not_below_window() )
					term_scroll_forward();
				else
					s_pan_down( 1 );
			}
		}
	}
}
/**************************************************************************
* cursor_not_below_window
*	Return non-zero if the row containing the cursor is visible
*	on a split screen or is above the visible part of the window.
**************************************************************************/
cursor_not_below_window()
{
	return( Outwin->row <= Outwin->buff_bot_row );
}
/**************************************************************************
* fct_scroll_reverse
*	ACTION module for 'scroll_reverse=' and 'parm_rindex='.
*	Scroll the screen backwards "rows" number of lines.
**************************************************************************/
fct_scroll_reverse( rows )
	int	rows;
{ 
	int	i;
	int	scroll;

	if ( Outwin->onstatus )
	{
		term_beep();
		return;
	}
	Outwin->real_xenl = 0;
	for ( i = 0; i < rows; i++ )
	{
		/* ??? maybe make this a multiple scroll */
		/* ??? assumes cursor stays put on scroll*/
		if ( oF_scroll_could_be_cursor_only )
		{
			if ( Outwin->row == Outwin->csr_buff_top_row )
			{
				scroll = 1;
				d_scroll_reverse();
				if ( oF_scroll_reverse_move_col0 )
					Outwin->col = 0;
			}
			else if ( Outwin->row == 0 )
			{
				continue;
			}
			else
			{
				scroll = 0;
				Outwin->row--;
				d_sync_line_attribute_current();/*scrollcur up*/
			}
		}
		else
		{
			scroll = 1;
			d_scroll_reverse();
			if ( oF_scroll_reverse_move_col0 )
				Outwin->col = 0;
		}
		if ( Outwin->onscreen )
		{
			if ( Outwin->fullscreen )
				term_scroll_reverse();
			else 
			{
				if ( scroll )
					s_scroll_reverse();
				else if ( cursor_not_above_window() )
					term_scroll_reverse();
				else
					s_pan_up( 1 );
			}
		}
	}
}
/**************************************************************************
* fct_flash_screen
*	ACTION module for 'flash_screen='.
**************************************************************************/
fct_flash_screen()
{
	Outwin->real_xenl = 0;
	if ( Outwin->onscreen )
		term_flash_screen();
}
/**************************************************************************
* fct_change_scroll_region
*	ACTION module for 'change_scroll_region'.
*	Set scroll region to lines "top_row" and "bot_row".
**************************************************************************/
fct_change_scroll_region( top_row, bot_row )
	int	top_row;
	int	bot_row;
{ 
	Outwin->onstatus = 0;
	Outwin->real_xenl = 0;
	/******************************************************************
	* Setting scroll region cancels memory_lock.
	******************************************************************/
	win_se_memory_unlock();
	if ( bot_row > Outwin->display_row_bottom )
		bot_row = Outwin->display_row_bottom;
	if ( top_row >= bot_row )
		top_row = bot_row - 1;
	if ( top_row < 0 )
		top_row = 0;
	if ( bot_row <= top_row )
		bot_row = top_row + 1;
	Outwin->csr_buff_top_row = top_row;
	Outwin->csr_buff_bot_row = bot_row;
	Outwin->row = 0;
	Outwin->col = 0;
	d_sync_line_attribute_current();	/* home */
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_change_scroll_region( Outwin->csr_buff_top_row,
						   Outwin->csr_buff_bot_row );
		else
			s_cursor_w_pan();
	}
}
/**************************************************************************
* win_se_scroll_region_normal
*	An event has occurred that has the side effect of setting the
*	scroll region back to the default.
**************************************************************************/
win_se_scroll_region_normal()
{
	Outwin->csr_buff_top_row = 0;
	Outwin->csr_buff_bot_row = Outwin->display_row_bottom;
}
/**************************************************************************
* fct_memory_lock
*	ACTION module for 'memory_lock='.
**************************************************************************/
fct_memory_lock()
{ 
	/******************************************************************
	* Memory lock does not work if scroll region is in effect
	******************************************************************/
	if (  ( Outwin->csr_buff_top_row != 0 )
	   || ( Outwin->csr_buff_bot_row != Outwin->display_row_bottom ) )
	{
		term_beep();
		term_beep();
		return;
	}
	Outwin->memory_lock_row = Outwin->row;
	Outwin->col = 0;
	if ( Outwin->onscreen )
	{
						/* split screen lock done in
						   software only.
						*/
		if ( Outwin->fullscreen )
			term_memory_lock( Outwin->row );
	}
}
/**************************************************************************
* fct_memory_unlock
*	ACTION module for 'memory_unlock='.
**************************************************************************/
fct_memory_unlock()
{ 
	Outwin->memory_lock_row = -1;
	if ( Outwin->onscreen )
		term_memory_unlock();
}
/**************************************************************************
* win_se_memory_unlock
*	An event has occurred that has the side effect of unlocking any
*	current memory lock on the window "Outwin".
**************************************************************************/
win_se_memory_unlock()
{
	Outwin->memory_lock_row = -1;
}
/**************************************************************************
* t_sync_scroll_region
*	Syncronize the terminal, if necessary, to a scrolling region on
*	the lines "top_row" and "bot_row".
**************************************************************************/
					/* return 1 if changed cursor row */
t_sync_scroll_region( top_row, bot_row )
	int	top_row;
	int	bot_row;
{
	if ( ( top_row != M_csr_top_row ) || ( bot_row != M_csr_bot_row ) )
	{
		term_change_scroll_region( top_row, bot_row );
		return( 1 );
	}
	return( 0 );
}
/**************************************************************************
* t_sync_memory_lock
*	Syncronize the terminal, if necessary, to a memory lock on
*	line nubmer "row".
**************************************************************************/
t_sync_memory_lock( row )
	int	row;
{
	if ( row != M_memory_lock_row )
	{
		if ( row < 0 )
		{
			term_memory_unlock();
			return( 0 );
		}
		else
		{
			term_memory_lock( row );
			return( 1 );
		}
	}
	return( 0 );
}
/**************************************************************************
* t_sync_memory_unlock
*	Synchroize the terminal, if necessary, to have no memory lock.
**************************************************************************/
t_sync_memory_unlock()
{
	if ( M_memory_lock_row != -1 )
		term_memory_unlock();
}
/**************************************************************************
* init_save_cursor
*	Initialize the remembered state of the save cursor storage on the
*	window "Outwin" to the default.
**************************************************************************/
init_save_cursor()
{
	modes_init_save_cursor( Outwin );
}
/**************************************************************************
* modes_init_save_cursor
*	Initialize the remembered state of the save cursor storage on the
*	window "window" to the default.
**************************************************************************/
modes_init_save_cursor( window )
	FT_WIN	*window;	
{
	window->save_cursor_origin_mode = 0;
	window->save_cursor_ftattrs = 0;
	window->save_cursor_character_set = 0;
	window->save_cursor_col = 0;
	window->save_cursor_row = 0;
}
/**************************************************************************
* fct_save_cursor
*	ACTION module for 'save_cursor='.
*	This is done in software since you cannot save 10 windows in hardware.
**************************************************************************/
fct_save_cursor()
{
	Outwin->real_xenl = 0;
	Outwin->save_cursor_origin_mode = Outwin->origin_mode;
	Outwin->save_cursor_ftattrs = Outwin->ftattrs;
	Outwin->save_cursor_character_set = Outwin->character_set;
	Outwin->save_cursor_col = Outwin->col;
	Outwin->save_cursor_row = Outwin->row;
	if ( Outwin->save_cursor_row > Outwin->display_row_bottom )
		Outwin->save_cursor_row = Outwin->display_row_bottom;
}
/**************************************************************************
* fct_restore_cursor
*	ACTION module for 'restore_cursor='.
**************************************************************************/
fct_restore_cursor()
{
	Outwin->onstatus = 0;
	Outwin->real_xenl = 0;
	Outwin->origin_mode = Outwin->save_cursor_origin_mode;
	if ( Outwin->onscreen )
		t_sync_origin_mode( Outwin->origin_mode );
	Outwin->ftattrs = Outwin->save_cursor_ftattrs;
	if ( Outwin->onscreen )
		t_sync_attr( Outwin->ftattrs );
	Outwin->character_set = Outwin->save_cursor_character_set;
	if ( Outwin->onscreen )
		t_sync_character_set();
	Outwin->col = Outwin->save_cursor_col;
	Outwin->row = Outwin->save_cursor_row;
	d_sync_line_attribute_current();	/* restore cursor */
	t_cursor();
}
/**************************************************************************
* win_se_origin_mode
*	An event has occurred that has the side effect of setting the
*	origin mode of the terminal to the state "origin_mode".
**************************************************************************/
win_se_origin_mode( origin_mode )
	int	origin_mode;
{
	Outwin->origin_mode = origin_mode;
}
/**************************************************************************
* t_sync_origin_mode
*	Syncronize the terminal, if necessary, so that its origin mode has
*	the state "origin_mode".
**************************************************************************/
/*ARGSUSED*/
t_sync_origin_mode( origin_mode )
	unsigned  char	origin_mode;
{
	/* ??? */   /* output if different */
}
/**************************************************************************
* term_se_origin_mode
*	An event has occurred that had the side effect of setting the
*	origin mode of the terminal to the state "origin_mode".
**************************************************************************/
/*ARGSUSED*/
term_se_origin_mode( origin_mode )
	int	origin_mode;
{
	/* ??? */
}
/**************************************************************************
* fct_auto_wrap_on
*	ACTION module for 'auto_wrap_on='.
**************************************************************************/
fct_auto_wrap_on()
{
	Outwin->real_xenl = 0;
	Outwin->auto_wrap_on = 1;
	if ( Outwin->onscreen )
		term_auto_wrap_on();
}
/**************************************************************************
* fct_auto_wrap_off
*	ACTION module for 'auto_wrap_off='.
**************************************************************************/
fct_auto_wrap_off()
{
	Outwin->real_xenl = 0;
	Outwin->auto_wrap_on = 0;
	if ( Outwin->onscreen )
		term_auto_wrap_off();
}
/**************************************************************************
* win_se_auto_wrap_on
*	An event has occurred that had the side effect of setting the
*	auto wrap mode of the window to on.
**************************************************************************/
win_se_auto_wrap_on()
{
	Outwin->auto_wrap_on = 1;
}
/**************************************************************************
* win_se_auto_wrap_off
*	An event has occurred that had the side effect of setting the
*	auto wrap mode of the window to on.
**************************************************************************/
win_se_auto_wrap_off()
{
	Outwin->auto_wrap_on = 0;
}
/**************************************************************************
* fct_columns_wide_on
*	ACTION module for 'columns_wide_on'.
**************************************************************************/
fct_columns_wide_on()
{
	int	switched;

	if ( Outwin->columns_wide_mode_on == 0 )
		switched = 1;
	else
		switched = 0;
	Outwin->real_xenl = 0;
	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	Outwin->columns_wide_mode_on = 1;
	Outwin->cols = Cols_wide;
	Outwin->col_right = Col_right_wide;
	d_set_col_right_line();
	win_se_margins_full();
	if ( F_columns_wide_clears_screen )
	{
		Outwin->onstatus = 0;
		if ( F_columns_wide_clears_onstatus )
			win_se_onstatus_clear();
		Outwin->xenl = 0;
		Outwin->real_xenl = 0;
		d_blankwin();
		Outwin->row = 0;
		Outwin->col = 0;
		d_sync_line_attribute_current();	/* wide clear home */
		b_cursor_home();			/* pan buffer to top */
	}
	if ( switched && F_columns_wide_switch_resets_scroll_region )
		win_se_scroll_region_normal();
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_columns_wide_on();
		else
		{
			if ( outwin_is_curwin() )
				term_columns_wide_on();
			redo_split_columns_wide();
		}
	}
}
/**************************************************************************
* win_se_columns_wide_on
*	An event has occurred that had the side effect of setting the
*	columns on the window to the wide state E.g. 132 columns.
**************************************************************************/
win_se_columns_wide_on()
{
	Outwin->columns_wide_mode_on = 1;
	Outwin->cols = Cols_wide;
	Outwin->col_right = Col_right_wide;
	d_set_col_right_line();
	win_se_margins_full();
}
/**************************************************************************
* win_se_columns_wide_off
*	An event has occurred that had the side effect of setting the
*	columns on the window to the narrow state E.g. 80 columns.
**************************************************************************/
win_se_columns_wide_off()
{
	Outwin->columns_wide_mode_on = 0;
	Outwin->cols = Cols;
	Outwin->col_right = Col_right;
	d_set_col_right_line();
	win_se_margins_full();
}
/**************************************************************************
* fct_columns_wide_off
*	ACTION module for 'columns_wide_off='.
*	E.g. 80 columns.
**************************************************************************/
fct_columns_wide_off()
{
	int	switched;

	if ( Outwin->columns_wide_mode_on )
		switched = 1;
	else
		switched = 0;
	Outwin->real_xenl = 0;
	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	Outwin->columns_wide_mode_on = 0;
	Outwin->cols = Cols;
	Outwin->col_right = Col_right;
	d_set_col_right_line();
	win_se_margins_full();
	if ( F_columns_wide_clears_screen )
	{
		Outwin->onstatus = 0;
		if ( F_columns_wide_clears_onstatus )
			win_se_onstatus_clear();
		Outwin->xenl = 0;
		Outwin->real_xenl = 0;
		d_blankwin();
		Outwin->row = 0;
		Outwin->col = 0;
		d_sync_line_attribute_current();	/* narrow clear home */
		b_cursor_home();			/* pan buffer to top */
	}
	if ( switched && F_columns_wide_switch_resets_scroll_region )
		win_se_scroll_region_normal();
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			term_columns_wide_off();
		else
		{
			if ( outwin_is_curwin() )
				term_columns_wide_off();
			redo_split_columns_wide();
		}
	}
}
/**************************************************************************
* redo_split_columns_wide
*	One of the windows of a split screen has changed from 132 columns
*	to 80 columns.
**************************************************************************/
redo_split_columns_wide()
{
	if ( outwin_is_curwin() )
	{
		w_all_off();
		Outwin->onscreen = 1;
		modes_off_redo_screen();
		if ( F_columns_wide_clears_screen == 0 )
			clear_other_split_win();
		split_label_split_row();
		split_number( Outwin->win_bot_row + 1, Outwin->number );
		modes_on_redo_screen();
		s_cursor();
	}
	else
	{
		force_split_outwin_offscreen();
	}
}
/**************************************************************************
* force_split_outwin_offscreen
*	A non-current but on-screen window of a split screen has made
*	a change ( e.g. column width ) that is incompatible with the 
*	current window and must be made off-screen.
**************************************************************************/
force_split_outwin_offscreen()
{
	modes_off_redo_screen();
	Outwin->onscreen = 0;
	if ( window_is_at_top( Outwin ) )
		clear_top_win();
	else
		clear_bottom_win();
	split_label_split_row();
	split_number( Curwin->win_bot_row + 1, Curwin->number );
	modes_on_redo_screen();
}
/**************************************************************************
* is_columns_wide_mode_on_winno
*	Return 1 if window number "winno" is in columns wide mode.
**************************************************************************/
is_columns_wide_mode_on_winno( winno )
	int	winno;
{
	FT_WIN	*win;	

	win = Wininfo[ winno ];
	return ( win->columns_wide_mode_on );
}
/**************************************************************************
* t_sync_columns
*	Syncronize the terminal, if necessary, to the columns mode
*	"columns_wide_mode_on".  E.g. 0=80 columns 1=132 columns.
**************************************************************************/
t_sync_columns( columns_wide_mode_on )
	int	columns_wide_mode_on;
{
	if ( columns_wide_mode_on )
	{
		if ( M_columns_wide_on == 0 )
		{
			term_columns_wide_on();
			return( 1 );
		}
	}
	else
	{
		if ( M_columns_wide_on )
		{
			term_columns_wide_off();
			return( 1 );
		}
	}
	return( 0 );
}
/**************************************************************************
* t_sync_columns_normal
*	Syncronize the terminal, if necessary, to the default columns
*	mode. E.g. 80 column mode.
**************************************************************************/
t_sync_columns_normal()
{
	if ( M_columns_wide_on )
		term_columns_wide_off();
}
/**************************************************************************
* t_cursor
*	If the current output window, 'Outwin', is on the screen, position
*	the cursor at the proper position whether the screen is full screen
*	or split screen.
**************************************************************************/
t_cursor()		/* pos cursor if on screen for full or split */
{
	if ( Outwin->onscreen )
	{
		if ( Outwin->fullscreen )
			f_cursor();
		else
			s_cursor_w_pan();
	}
}
/**************************************************************************
* win_pos_w_pan
*	Position the cursor for window "window" at position "row" and "col".
*	If the screen is split, pan the window if necessary.
**************************************************************************/
win_pos_w_pan( window, row, col )	/* pos cursor on full or split window */
					/* assumes attributes off */
	FT_WIN	*window;	
	int	row;
	int	col;
{
	int	row_in_window;
	int	rows;
	int	screen_row;
	int	i;

	if ( window->fullscreen )
	{
		term_pos( row, col );
	}
	else
	{
		rows = row - window->buff_bot_row; /* how many below */
		for ( i = 0; i < rows; i++ )
		{
			s_roll_forward( window->win_top_row,
					window->win_bot_row );
			window->buff_top_row++;
			window->buff_bot_row++;
			toscreen_row( window,
				      window->buff_bot_row,
				      window->win_bot_row, NO_ATTRS_AT_END );
			term_carriage_return();
		}
		rows = window->buff_top_row - row; /* how many above */
		for ( i = 0; i < rows; i++ )
		{
			s_roll_reverse( window->win_top_row,
					window->win_bot_row );
			window->buff_top_row--;
			window->buff_bot_row--;
			toscreen_row( window,
				      window->buff_top_row,
				      window->win_top_row, NO_ATTRS_AT_END );
			term_carriage_return();
		}
		row_in_window = row - window->buff_top_row;
		screen_row = window->win_top_row + row_in_window;
		term_pos( screen_row, col );
	}
}
/**************************************************************************
* win_pos_limited
*	Position the cursor for window "window" at position "row" and "col".
*	If the screen is split, and the cursor position is not visible,
*	do not pan but rather place the cursor at the closest visible spot.
**************************************************************************/
win_pos_limited( window, row, col )	/* pos cursor on full or split window */
	FT_WIN	*window;	
	int	row;
	int	col;
{
	int	row_in_window;
	int	screen_row;

	if ( window->fullscreen )
	{
		term_pos( row, col );
	}
	else
	{
		row_in_window = row - window->buff_top_row;
		if ( row_in_window < 0 )
			row_in_window = 0;
		screen_row = window->win_top_row + row_in_window;
		if ( screen_row > window->win_bot_row )
			screen_row = window->win_bot_row;
		term_pos( screen_row, col );
	}
}
/* ======================== split screen update ========================= */
/**************************************************************************
* f_cursor
*	Full screen position cursor for window "Outwin".
**************************************************************************/
f_cursor()		/* screen pos cursor for full on-screen window */
{
	char	buff[ 80 ];

	if ( Outwin->onstatus )
	{
		term_out_onstatus( Outwin->col );
	}
	else if ( Outwin->real_xenl )
	{
		if ( ( Outwin->col == 0 ) && ( Outwin->row > 0 ) )
		{
			term_pos( Outwin->row - 1, Outwin->col_right );
			toscreen_char_at_row_col( Outwin->row - 1,
						 Outwin->col_right, 1 );
		}
		else
		{
			Outwin->real_xenl = 0;
			term_beep();
			sprintf( buff, "invalid_real_xenl_row=%d-col=%d",
				 Outwin->row, Outwin->col );
			error_record_msg( buff );
			term_pos( Outwin->row, Outwin->col );
		}
	}
	else
	{
		term_pos( Outwin->row, Outwin->col );
		if ( Outwin->xenl )
			toscreen_char_at_cursor( 1 );
	}
}
/**************************************************************************
* s_cursor_w_pan
*	Split scren position cursor for window 'Outwin' with panning to
*	make the cursor position visible if necessary.
**************************************************************************/
s_cursor_w_pan()
{
	if ( Outwin->onstatus )
	{
		term_out_onstatus( Outwin->col );
		return;
	}
	if ( s_pan_up_to_cursor() > 0 )
		return;				/* had to pan up - cursor set*/
	if ( s_pan_down_to_cursor() > 0 )
		return;				/* had to pan down -cursor set*/
	s_cursor();				/* position is on screen */
}
/**************************************************************************
* s_cursor
*	Position cursor for split on-screen window 'Outwin'.  Cursor position
*	should be visible.
**************************************************************************/
s_cursor()		/* screen position cursor for split on-screen window */
{
	int	row_in_window;
	char	buff[ 80 ];

	if ( Outwin->onstatus )
	{
		term_out_onstatus( Outwin->col );
		return;
	}
	row_in_window = Outwin->row - Outwin->buff_top_row;
	if ( Outwin->real_xenl )
	{
		if ( ( Outwin->col == 0 ) && ( Outwin->row > 0 ) )
		{
			if ( row_in_window == 0 )
			{
				s_pan_up( 1 );	/* calls s_cursor again */
			}
			else
			{
				term_pos( 
				    Outwin->win_top_row + row_in_window - 1, 
				    Outwin->col_right );
				toscreen_char_at_row_col( Outwin->row - 1,
							 Outwin->col_right, 1 );
			}
		}
		else
		{
			Outwin->real_xenl = 0;
			term_beep();
			sprintf( buff, "invalid_real_xenl_s_row=%d-col=%d",
				 Outwin->row, Outwin->col );
			error_record_msg( buff );
			term_pos( Outwin->win_top_row + row_in_window, 
				  Outwin->col );
		}
	}
	else
	{
		term_pos( Outwin->win_top_row + row_in_window, Outwin->col );
		if ( Outwin->xenl )
			toscreen_char_at_cursor( 1 );
	}
}
/**************************************************************************
* s_calc_row_on_screen
*	Return the row on the screen that contains the cursor for the
*	window 'Outwin'
**************************************************************************/
s_calc_row_on_screen()
{
	int	row_in_window;
	int	row_on_screen;

	row_in_window = Outwin->row - Outwin->buff_top_row;
	row_on_screen = Outwin->win_top_row + row_in_window;
	return( row_on_screen );
}
/**************************************************************************
* s_pan_down_to_cursor
*	Pan down until the row containing the cursor is visible.
*	Return the number of rows panned.
**************************************************************************/
s_pan_down_to_cursor()
{
	int	rows;

	rows = Outwin->row - Outwin->buff_bot_row;
	if ( rows > 0 )
		s_pan_down( rows );
	return( rows );
}
/**************************************************************************
* s_pan_up_to_cursor
*	Pan up until the row containing the cursor is visible
*	Return the number of rows panned.
**************************************************************************/
s_pan_up_to_cursor()
{
	int	rows;

	rows = Outwin->buff_top_row - Outwin->row;
	if ( rows > 0 )
		s_pan_up( rows );
	return( rows );
}
/**************************************************************************
* s_pan_down
*	Pan down "rows" number of lines.
**************************************************************************/
s_pan_down( rows )	/* pan down buffer & screen for split onscreen window */
						/* assumes attributes on */
	int	rows;
{
	int	i;

	for ( i = 0; i < rows; i ++ )
	{
		s_roll_forward_with_attr_on( Outwin->win_top_row,
					     Outwin->win_bot_row );
		Outwin->buff_top_row++;
		Outwin->buff_bot_row++;
		toscreen_row( Outwin,
		 	      Outwin->buff_bot_row,
			      Outwin->win_bot_row, ATTRS_AT_END );
		/* NOT NEEDED - TROUBLE IF GRAPHICS MODE MAKES CR A CHAR */
		/* term_carriage_return(); */
	}
	s_cursor();
}
/**************************************************************************
* s_scroll_forward
*	Scroll_forward a split screen window.
**************************************************************************/
s_scroll_forward()
					/* assumes attributes on */
{
	int	below_top;
	int	above_bot;
					/* top line on screen to scroll */
	if ( Outwin->memory_lock_row > 0 )
	{
	    if ( Outwin->memory_lock_row >= Outwin->buff_top_row )
		below_top = Outwin->memory_lock_row - Outwin->buff_top_row;
	    else
		below_top = 0;
	}
	else
	{
	    if ( Outwin->csr_buff_top_row >= Outwin->buff_top_row )
		below_top = Outwin->csr_buff_top_row - Outwin->buff_top_row;
	    else
		below_top = 0;
	}
					/* bot line on screen to scroll */
	if ( Outwin->csr_buff_bot_row <= Outwin->buff_bot_row )
		above_bot = Outwin->buff_bot_row - Outwin->csr_buff_bot_row;
	else
		above_bot = 0;
	/* ??? region ??? */
	s_roll_forward_with_attr_on( Outwin->win_top_row + below_top,
				     Outwin->win_bot_row - above_bot );
	toscreen_row( Outwin, Outwin->buff_bot_row - above_bot,
				Outwin->win_bot_row  - above_bot,
				ATTRS_AT_END );
	/* NOT NEEDED - TROUBLE IF GRAPHICS MODE MAKES CR A CHAR */
	/* term_carriage_return(); */
	s_cursor();
}
/**************************************************************************
* s_scroll_reverse
*	Scroll reverse a split screen window.
**************************************************************************/
s_scroll_reverse()
					/* assumes attributes on */
{
	int	below_top;
	int	above_bot;
					/* top line on screen to scroll */
	if ( Outwin->memory_lock_row > 0 )
	{
	    if ( Outwin->memory_lock_row >= Outwin->buff_top_row )
		below_top = Outwin->memory_lock_row - Outwin->buff_top_row;
	    else
		below_top = 0;
	}
	else
	{
	    if ( Outwin->csr_buff_top_row >= Outwin->buff_top_row )
		below_top = Outwin->csr_buff_top_row - Outwin->buff_top_row;
	    else
		below_top = 0;
	}
					/* bot line on screen to scroll */
	if ( Outwin->csr_buff_bot_row <= Outwin->buff_bot_row )
		above_bot = Outwin->buff_bot_row - Outwin->csr_buff_bot_row;
	else
		above_bot = 0;
	/* ??? region ??? */
	s_roll_reverse_with_attr_on( Outwin->win_top_row + below_top,
				     Outwin->win_bot_row - above_bot );
	toscreen_row( Outwin, Outwin->buff_top_row + below_top,
				Outwin->win_top_row  + below_top, 
				ATTRS_AT_END );
	/* NOT NEEDED - TROUBLE IF GRAPHICS MODE MAKES CR A CHAR */
	/* term_carriage_return(); */
	s_cursor();
}
/**************************************************************************
* b_cursor_home
*	Pan the buffer of a split screen to the home position.
**************************************************************************/
b_cursor_home()
{
	Outwin->buff_bot_row -= Outwin->buff_top_row;
	Outwin->buff_top_row = 0;
}
/**************************************************************************
* b_pan_high_w_cursor
*	Pan the buffer of a split screen as high as possible, still having
*	the cursor on a visible row.
**************************************************************************/
b_pan_high_w_cursor( window, rows ) /* pan buffer highest including cursor */
	FT_WIN	*window;	
	int	rows;
{
	int	downrows;

	window->buff_top_row = 0;
	window->buff_bot_row = rows - 1;
					/* if the cursor is on the status
					   line, show the top of the screen.
					*/
	if ( window->onstatus )
		return;
	downrows = window->row - window->buff_bot_row;
	if ( downrows > 0 )
	{
		window->buff_top_row += downrows;
		window->buff_bot_row += downrows;
	}
}
/**************************************************************************
* s_pan_up
*	Pan a split screen up "rows" number of lines.
**************************************************************************/
s_pan_up( rows )	/* pan up buffer & screen for split onscreen window */
					/* assumes attributes on */
	int	rows;
{
	int	i;

	for ( i = 0; i < rows; i ++ )
	{
		s_roll_reverse_with_attr_on( Outwin->win_top_row, 
					     Outwin->win_bot_row );
		Outwin->buff_top_row--;
		Outwin->buff_bot_row--;
		toscreen_row( Outwin,
				Outwin->buff_top_row,
				Outwin->win_top_row, ATTRS_AT_END );
		/* NOT NEEDED - TROUBLE IF GRAPHICS MODE MAKES CR A CHAR */
		/* term_carriage_return(); */
	}
	s_cursor();
}
/**************************************************************************
* s_roll_forward_with_attr_on
*	Roll the split screen that is visible from "win_top_row" to 
*	"win_bot_row" forward.
*	The current attributes for the window are ON when this module
*	is called.
**************************************************************************/
s_roll_forward_with_attr_on( win_top_row, win_bot_row )
	int	win_top_row;
	int	win_bot_row;
{
	if ( mF_insert_line_sets_attributes )
		t_sync_attr( (FTCHAR) 0 );
	s_roll_forward( win_top_row, win_bot_row );
	if ( mF_insert_line_sets_attributes )
		t_sync_attr( Outwin->ftattrs );
}
/**************************************************************************
* s_roll_forward
*	Roll the split screen that is visible from "win_top_row" to 
*	"win_bot_row" forward.
*	The current attributes for the window are OFF when this module
*	is called.
**************************************************************************/
s_roll_forward( win_top_row, win_bot_row ) /* leave cursor on bottom line */
	int	win_top_row;
	int	win_bot_row;
{
	if ( mF_use_csr )
	{
		t_sync_scroll_region( win_top_row, win_bot_row);
		term_pos( win_bot_row, 0 );
		term_scroll_forward();
		t_sync_scroll_region( 0, Row_bottom_terminal );
		term_pos( win_bot_row, 0 );
	}
	else
	{
		term_pos( win_top_row, 0 );
		term_out_delete_line( win_top_row );
		term_pos( win_bot_row, 0 );
		term_out_insert_line( win_bot_row );
	}
}
/**************************************************************************
* s_roll_reverse_with_attr_on
*	Roll the split screen that is visible from "win_top_row" to 
*	"win_bot_row" backwards.
*	The current attributes for the window are ON when this module
*	is called.
**************************************************************************/
s_roll_reverse_with_attr_on( win_top_row, win_bot_row )
	int	win_top_row;
	int	win_bot_row;
{
	if ( mF_insert_line_sets_attributes )
		t_sync_attr( (FTCHAR) 0 );
	s_roll_reverse( win_top_row, win_bot_row );
	if ( mF_insert_line_sets_attributes )
		t_sync_attr( Outwin->ftattrs );
}
/**************************************************************************
* s_roll_reverse
*	Roll the split screen that is visible from "win_top_row" to 
*	"win_bot_row" backwards.
*	The current attributes for the window are OFF when this module
*	is called.
**************************************************************************/
s_roll_reverse( win_top_row, win_bot_row ) /* leave cursor on top line */
	int	win_top_row;
	int	win_bot_row;
{
	if ( mF_use_csr )
	{
		t_sync_scroll_region( win_top_row, win_bot_row);
		term_pos( win_top_row, 0 );
		term_scroll_reverse();
		t_sync_scroll_region( 0, Row_bottom_terminal );
		term_pos( win_top_row, 0 );
	}
	else
	{
		term_pos( win_bot_row, 0 );
		term_out_delete_line( win_bot_row );
		term_pos( win_top_row, 0 );
		term_out_insert_line( win_top_row );
	}
}
/**************************************************************************
* s_sync_to_buffer
*	The rows visible on a split screen should be moved to their
*	proper place on a full screen.
**************************************************************************/
s_sync_to_buffer()	/* lines on split screen line up to buffer */
{
	s_move_lines( Outwin->win_top_row, Outwin->buff_top_row );
}
/**************************************************************************
* s_sync_to_window
*	The rows of a full screen that will be visible on a new split
*	screen are to be moved to the appropriate place.
**************************************************************************/
s_sync_to_window( window ) /* full screen rows corr to buffer into window */
	FT_WIN	*window;
{
	s_move_lines( window->buff_top_row, window->win_top_row );
}
/**************************************************************************
* s_move_lines
*	Move the lines at line "from_row" and below to the position "to_row"
*	and below.
**************************************************************************/
s_move_lines( from_row, to_row )	/* move screen lines */
	int	from_row;
	int	to_row;
{
	int	i;
	int	rows;

	rows = to_row - from_row;	/* + = down */
	if ( rows > 0 )
	{
		if ( mF_use_csr )
		{
			t_sync_scroll_region( from_row, Row_bottom_terminal );
			term_pos( from_row, 0 );
			for ( i = 0; i < rows; i++ )
				term_scroll_reverse();
			t_sync_scroll_region( 0, Row_bottom_terminal );
		}
		else
		{
			if ( mF_insert_line_needs_clear_glitch )
			{
				term_pos( Rows_terminal - rows, 0 );
								/* affrow */
				term_out_parm_delete_line( rows, 
						       Rows_terminal - rows );
			}
			term_pos( from_row, 0 );
			term_out_parm_insert_line( rows, from_row );/* affrow */
		}
	}
	else if ( rows < 0 )
	{
		if ( mF_use_csr )
		{
			t_sync_scroll_region( to_row, Row_bottom_terminal );
			term_pos( Row_bottom_terminal, 0 );
			for ( i = 0; i < -rows; i++ )
				term_scroll_forward();
			t_sync_scroll_region( 0, Row_bottom_terminal );
		}
		else
		{
			term_pos( to_row, 0 );
			term_out_parm_delete_line( -rows, to_row ); /* affrow */
			if ( mF_memory_below )
			{
				term_pos( Rows_terminal + rows, 0 );
				out_term_clr_memory_below();
				term_pos( to_row, 0 );
			}
		}
	}
}
/********************* SUBS ***********/
/**************************************************************************
* d_sync_line_attribute_current
*	Adjust the cursor position if the line attribute of the new
*	row does not match the line attribute of the old row.
**************************************************************************/
d_sync_line_attribute_current()
{
	unsigned char	line_attribute_new;

	line_attribute_new = Outwin->line_attribute[ Outwin->row ];
	if ( line_attribute_new != Outwin->line_attribute_current )
	{
		d_set_col_right_line();
		Outwin->line_attribute_current = line_attribute_new;
	}
}
/**************************************************************************
* d_set_col_right_line
*	Adjust the location of the right column on the screen based on
*	the current width and line attribute.
**************************************************************************/
d_set_col_right_line()
{
	if ( Outwin->line_attribute[ Outwin->row ] )
		Outwin->col_right_line = ( Outwin->cols >> 1 ) - 1;
	else
		Outwin->col_right_line = Outwin->col_right;
	d_set_wrap_margins();
	if ( Outwin->col > Outwin->col_right_line )
	{	
		Outwin->col = Outwin->col_right_line;
		if ( F_eat_newline_glitch[ Outwin->personality ] )
			Outwin->xenl = 1;
	}
	else if ( Outwin->col < Outwin->col_right_line )
	{
		Outwin->xenl = 0;
	}
	/* Outwin->real_xenl not on terminals with line attribute */
}
/**************************************************************************
* d_scroll_forward
*	Scroll the window buffer forward 1 line.
**************************************************************************/
d_scroll_forward()
{
	int	row;

	if ( Outwin->memory_lock_row > 0 )
		row = Outwin->memory_lock_row;
	else
		row = Outwin->csr_buff_top_row;
	if ( oF_has_scroll_memory )
		remember_scroll_memory( row );
	d_delete_line( row );
}
/**************************************************************************
* d_scroll_reverse
*	Scroll the window buffer back one row.
**************************************************************************/
d_scroll_reverse()
{
	if ( Outwin->memory_lock_row > 0 )
	{
		d_insert_line( Outwin->memory_lock_row );
		return;
	}
	d_insert_line( Outwin->csr_buff_top_row );
}
/**************************************************************************
* d_insert_line
*	Insert a row at "line" in the window buffer.
**************************************************************************/
d_insert_line( line )
	int	line;
{
	REG	int		i;
		FTCHAR		*p;
		int		bot_row;
	REG	FTCHAR		**sc;
	REG	FTCHAR		**dc;
	REG	unsigned char	*sa;
	REG	unsigned char	*da;

	bot_row = Outwin->csr_buff_bot_row;
	set_row_changed( Outwin, line, bot_row );
	set_col_changed_all( Outwin );
	dc = &Outwin->ftchars[ bot_row ];
	sc = dc; sc--;	/* Outwin->ftchars[ bot_row - 1 ]; */
	da = &Outwin->line_attribute[ bot_row ];
	sa = da; sa--;	/* Outwin->line_attribute[ i - 1 ]; */
	p = *dc;	/* p = Outwin->ftchars[ Outwin->csr_buff_bot_row ]; */
	for ( i = bot_row; i > line; i-- )
	{
		*dc-- = *sc--; /* Outwin->ftchars[i] = Outwin->ftchars[ i-1 ];*/
		*da-- = *sa--; /* Outwin->line_attribute[ i ] =
				  Outwin->line_attribute[ i - 1 ]; */
	}
	*dc = p;		/* Outwin->ftchars[ line ] = p; */
	d_blankline( p );
	*da = 0;		/* Outwin->line_attribute[ line ] = 0; */
	d_sync_line_attribute_current();		/* d_insert */
}
/**************************************************************************
* d_delete_line
*	Delete a row at "line" in the window buffer.
**************************************************************************/
d_delete_line( line )		/* assumes line is in scroll_region */
	int	line;
{
	REG	int		i;
		FTCHAR		*p;
		int		bot_row;
	REG	FTCHAR		**sc;
	REG	FTCHAR		**dc;
	REG	unsigned char	*sa;
	REG	unsigned char	*da;

	bot_row = Outwin->csr_buff_bot_row;
	set_row_changed( Outwin, line, bot_row );
	set_col_changed_all( Outwin );
	dc = &Outwin->ftchars[ line ];
	sc = dc; sc++;		/* &Outwin->ftchars[ line + 1 ] */
	da = &Outwin->line_attribute[ line ];
	sa = da; sa++;		/* Outwin->line_attribute[ line + 1 ] */
	p = *dc;		/* p = Outwin->ftchars[ line ]; */
	for ( i = line; i < bot_row; i++ )
	{
		*dc++ = *sc++; /* Outwin->ftchars[ i ] = Outwin->ftchars[i+1];*/
		*da++ = *sa++; /* Outwin->line_attribute[ i ] =
				  Outwin->line_attribute[ i + 1 ]; */
	}
	*dc = p;		/* Outwin->ftchars[ bot_row ] = p; */
	d_blankline( p );
	*da = 0;		/* Outwin->line_attribute[ bot_row ] = 0; */
	d_sync_line_attribute_current();		/* d_delete */
}
/**************************************************************************
* remember_scroll_memory
*	Line "line" in the window buffer is scrolled out to the 
*	scroll memory and the top line of the scroll memory is recycled
*	to the bottom of the window buffer.
**************************************************************************/
remember_scroll_memory( line )
	int	line;
{
	REG	int		i;
		FTCHAR		*top_row;
	REG	FTCHAR		**sc;
	REG	FTCHAR		**dc;

	if ( Rows_scroll_memory <= 0 )
		return;
	dc = Outwin->scroll_memory_ftchars;	/* addr top line pointer */
	sc = dc; sc++;			/* addr second line pointer */
	top_row = *dc;
	for ( i = 1; i < Rows_scroll_memory; i++ )
	{
		*dc++ = *sc++; /* scroll the array */
	}
	*dc = Outwin->ftchars[ line ];	/* top line on screen is bot line mem */
	Outwin->ftchars[ line ] = top_row; /* old top of mem in top screen */
	d_blankline( top_row );
}
/**************************************************************************
* d_clr_eol
*	Clear the window buffer starting at "row" and "col" to the end of
*	the line.
*	The positions affected assume the character and attributes of
*	"blank_char_w_attr".
**************************************************************************/
d_clr_eol( row, col, parm, blank_char_w_attr )
	int row;
	int col;
	FTCHAR	parm;	/* not used */
	FTCHAR	blank_char_w_attr;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	d_blank_w_attr( &Outwin->ftchars[ row ][ col ], Cols_wide - col,
						blank_char_w_attr );
}
/**************************************************************************
* d_clr_eol_chars
*	Clear the characters (only - not attributes) of the window buffer
*	from the position "row" and "col" to the end of the line.
**************************************************************************/
d_clr_eol_chars( row, col, parm, blank_char_w_attr )
	int row;
	int col;
	FTCHAR	parm;	/* not used */
	FTCHAR	blank_char_w_attr;	/* not set */
{
	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	d_blank_chars( &Outwin->ftchars[ row ][ col ], Cols_wide - col );
}
/**************************************************************************
* d_set_attr_eol
*	Set the attributes in the window buffer (without disturbing the
*	characters) from the position "row" and "col" to the end of the 
*	line.
**************************************************************************/
d_set_attr_eol( row, col, parm, blank_char_w_attr )
	int row;
	int col;
	FTCHAR	parm;			/* not used */
	FTCHAR	blank_char_w_attr;	/* not set */
{
	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	d_set_attr( &Outwin->ftchars[ row ][ col ], Cols_wide - col );
}
/**************************************************************************
* d_clr_eol_chars_erasable
*	Clear the characters ( only - attributes are not affected ) in the 
*	window buffer from position "row" and *	"col" to the end of the line.
*	Only erasable characters are affected - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_clr_eol_chars_erasable( row, col, not_erasable_attr, blank_char_w_attr )
	int	row;
	int	col;
	FTCHAR	not_erasable_attr;
	FTCHAR	blank_char_w_attr;	/* not set */
{
	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	d_blank_chars_erasable( &Outwin->ftchars[ row ][ col ], Cols_wide - col,
							not_erasable_attr );
}
/**************************************************************************
* d_clr_bol
*	Clear the characeters and attributes of the window buffer from
*	the position "row" and "col" to the beginning of the line.
**************************************************************************/
d_clr_bol( row, col )
	int row;
	int col;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_bol( Outwin, col );
	d_blank( Outwin->ftchars[ row ], col + 1 );
}
/**************************************************************************
* d_clr_bol_w_attr
*	Clear the characters and attributes of the window buffer from
*	the posiiton "row" and "col" to the beginning of the line.
*	The positions affected assume the current attributes.
**************************************************************************/
d_clr_bol_w_attr( row, col )
	int row;
	int col;
{
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' '
			  | ( Outwin->ftattrs & (~F_character_set_attributes) );
	set_row_changed_1( Outwin, row );
	set_col_changed_bol( Outwin, col );
	d_blank_w_attr( Outwin->ftchars[ row ], col + 1, blank_char_w_attr );
}
/**************************************************************************
* d_clr_bol_chars
*	Clear the characters ( only - not attributes ) of the window buffer
*	from the position "row" and "col" to the beginning of the line.
**************************************************************************/
d_clr_bol_chars( row, col )
	int row;
	int col;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_bol( Outwin, col );
	d_blank_chars( Outwin->ftchars[ row ], col + 1 );
}
/**************************************************************************
* d_clr_bol_chars_erasable
*	Clear the characters ( only - not attributes ) of the window buffer
*	from the position "row" and "col" to the beginning of the line.
*	Only erasable characters are cleared - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_clr_bol_chars_erasable( row, col, not_erasable_attr )
	int	row;
	int	col;
	FTCHAR	not_erasable_attr;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_bol( Outwin, col );
	d_blank_chars_erasable( Outwin->ftchars[ row ], col + 1,
							not_erasable_attr );
}
/**************************************************************************
* d_set_attr_bol
*	Set the attribute ( only - characters unaffected ) of the window buffer
*	from the position "row" and "col" to the beginning of the line
*	to the current attribute.
**************************************************************************/
d_set_attr_bol( row, col )
	int row;
	int col;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_bol( Outwin, col );
	d_set_attr( Outwin->ftchars[ row ], col + 1 );
}
/**************************************************************************
* d_se_onstatus_clear
*	Clear the onstatus line "row" as a side effect of another operation.
**************************************************************************/
d_se_onstatus_clear( row )
	int row;
{
	d_blank( Outwin->ftchars[ row ], Cols_wide );
}
/**************************************************************************
* d_onstatus_load
*	Load the 'onstatus' line of window number "win", which is line number
*	"status_line" with the string "string".
**************************************************************************/
d_onstatus_load( win, status_line, string )
	FT_WIN		*win;
	int		status_line;
	char		*string;
{
	REG	FTCHAR		*d;
	REG	unsigned char	*s;
	REG	int		count;

	d = win->ftchars[ status_line ];
	s = (unsigned char *) string;
	count = Cols;
	while( ( count-- > 0 ) && ( *s != '\0' ) )
		*d++ = *s++;
}
/**************************************************************************
* d_clear_line
*	Clear the characters and attributes of the window buffer on line 
*	number "row" in window "Outwin".
**************************************************************************/
d_clear_line( row )
	int row;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_all( Outwin );
	d_blank( Outwin->ftchars[ row ], Cols_wide );
}
/**************************************************************************
* d_clear_line_w_attr
*	Clear the characters and attributes of the window buffer on line
*	number "row" in window "Outwin". 
*	The positions affected assume the current attributes.
**************************************************************************/
d_clear_line_w_attr( row )
	int row;
{
	FTCHAR  blank_char_w_attr;

	blank_char_w_attr = (FTCHAR) ' '
			  | ( Outwin->ftattrs & (~F_character_set_attributes) );
	set_row_changed_1( Outwin, row );
	set_col_changed_all( Outwin );
	d_blank_w_attr( Outwin->ftchars[ row ], Cols_wide, blank_char_w_attr );
}
/**************************************************************************
* d_clear_line_chars
*	Clear the characters ( only - attributes unaffected )
*	of the window buffer on row number "row".
**************************************************************************/
d_clear_line_chars( row )
	int row;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_all( Outwin );
	d_blank_chars( Outwin->ftchars[ row ], Cols_wide );
}
/**************************************************************************
* d_set_attr_line
*	Set the attributes ( only - characters unaffected ) 
*	of the window buffer on row number "row"
*	to the current attribute.
**************************************************************************/
d_set_attr_line( row )
	int	row;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_all( Outwin );
	d_set_attr( Outwin->ftchars[ row ], Cols_wide );
}
/**************************************************************************
* d_clear_line_chars_erasable
*	Clear the characters ( only - attributes unaffected )
*	of the window buffer on row number "row".
*	Only erasable characters are affected - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_clear_line_chars_erasable( row, not_erasable_attr )
	int	row;
	FTCHAR	not_erasable_attr;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_all( Outwin );
	d_blank_chars_erasable( Outwin->ftchars[ row ], Cols_wide, 
							not_erasable_attr );
}
/**************************************************************************
* d_clr_eos
*	Clear the characters and attributes to the character and attribute
*	in "blank_char_w_attr"
*	of the window buffer from "row" and "col" to the end of the screen.
*	"parm" is not used.
**************************************************************************/
/*ARGSUSED*/
d_clr_eos( row, col, parm, blank_char_w_attr )
	int row;
	int col;
	FTCHAR	parm;		/* not used */
	FTCHAR	blank_char_w_attr;
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_eos( Outwin, row );
	set_col_changed_all( Outwin );
	d_blank_w_attr( &Outwin->ftchars[ row ][ col ], Cols_wide - col,
							blank_char_w_attr );
	if ( col == 0 )
		Outwin->line_attribute[ row ] = 0;
	rows = Outwin->display_rows;
	for ( i = row + 1; i < rows; i++ )
	{
		d_blank_w_attr( Outwin->ftchars[ i ], Cols_wide,
							blank_char_w_attr );
		Outwin->line_attribute[ i ] = 0;
	}
}
/**************************************************************************
* d_clr_eos_chars
*	Clear the characters ( only - attributes unaffected )
*	of the window buffer from "row" and "col" to the end of the screen.
**************************************************************************/
/*ARGSUSED*/
d_clr_eos_chars( row, col, parm, blank_char_w_attr )
	int row;
	int col;
	FTCHAR	parm;			/* not used */
	FTCHAR	blank_char_w_attr;	/* not set */
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_eos( Outwin, row );
	set_col_changed_all( Outwin );
	d_clr_eol_chars( row, col, parm, blank_char_w_attr );
	rows = Outwin->display_rows;
	for ( i = row + 1; i < rows; i++ )
		d_clear_line_chars( i );
}
/**************************************************************************
* d_set_attr_eos
*	Set the attributes ( only - characters unaffected )
*	of the window buffer from "row" and "col" to the end of the screen
*	to the current attribute.
**************************************************************************/
/*ARGSUSED*/
d_set_attr_eos( row, col, parm, blank_char_w_attr )
	int row;
	int col;
	FTCHAR	parm;			/* not used */
	FTCHAR	blank_char_w_attr;	/* not set */
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_eos( Outwin, row );
	set_col_changed_all( Outwin );
	d_set_attr_eol( row, col, parm, blank_char_w_attr );
	rows = Outwin->display_rows;
	for ( i = row + 1; i < rows; i++ )
		d_set_attr_line( i );
}
/**************************************************************************
* d_clr_eos_chars_erasable
*	Clear the characters ( only - attributes unaffected )
*	of the window buffer from "row" and "col" to the end of the screen.
*	Only erasable characters are affected - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_clr_eos_chars_erasable( row, col, not_erasable_attr, blank_char_w_attr )
	int	row;
	int	col;
	FTCHAR	not_erasable_attr;		/* chars with this attr 
						   are not erasable */
	FTCHAR	blank_char_w_attr;		/* not set */
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_eos( Outwin, row );
	set_col_changed_all( Outwin );
	d_clr_eol_chars_erasable( row, col, not_erasable_attr,
							blank_char_w_attr );
	rows = Outwin->display_rows;
	for ( i = row + 1; i < rows; i++ )
		d_clear_line_chars_erasable( i, not_erasable_attr );
}
/**************************************************************************
* d_clr_bos
*	Clear the characters and attributes
*	of the window buffer from "row" & "col" to the beginning of the screen.
**************************************************************************/
/*ARGSUSED*/
d_clr_bos( row, col, parm )
	int row;
	int col;
	int parm;		/* not used */
{
	REG	int	i;

	set_row_changed_bos( Outwin, row );
	set_col_changed_all( Outwin );
	d_clr_bol( row, col );
	for ( i = row - 1; i >= 0; i-- )
	{
		d_clear_line( i );
		Outwin->line_attribute[ i ] = 0;
	}
}
/**************************************************************************
* d_clr_bos_chars_erasable
*	Clear the characters ( only - attributes unaffected )
*	of the window buffer from "row" & "col" to the beginning of the screen.
*	Only erasable characters are affected - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_clr_bos_chars_erasable( row, col, not_erasable_attr )
	int	row;
	int	col;
	FTCHAR	not_erasable_attr;		/* chars with this attr 
					   are not erasable */
{
	REG	int	i;

	set_row_changed_bos( Outwin, row );
	set_col_changed_all( Outwin );
	d_clr_bol_chars_erasable( row, col, not_erasable_attr );
	for ( i = row - 1; i >= 0; i-- )
		d_clear_line_chars_erasable( i, not_erasable_attr );
}
/**************************************************************************
* d_erase_chars
*	Clear the characters and attributes 
*	of the window buffer for "cols" columns starting at position
*	"row' and "col".
**************************************************************************/
d_erase_chars( row, col, cols )
	int row;
	int col;
	int cols;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_count( Outwin, col, cols );
	d_blank( &Outwin->ftchars[ row ][ col ], cols );
}
/**************************************************************************
* d_blankwin_max
*	Clear the characters and attributes and line attributes of Outwin.
*	All allocated rows.
**************************************************************************/
d_blankwin_max()
{
	REG	int	i;

	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	for ( i = 0; i < Rows_max; i++ )
	{
		d_blankline( Outwin->ftchars[ i ] );
		Outwin->line_attribute[ i ] = 0;
	}
}
/**************************************************************************
* d_blankwin
*	Clear the characters and attributes and line attributes of Outwin.
**************************************************************************/
d_blankwin()
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	rows = Outwin->display_rows;
	for ( i = 0; i < rows; i++ )
	{
		d_blankline( Outwin->ftchars[ i ] );
		Outwin->line_attribute[ i ] = 0;
	}
}
/**************************************************************************
* d_blankline
*	Clear the characters and attributes
*	of the window buffer for window "Outwin" for line number "line".
**************************************************************************/
d_blankline( line )
	FTCHAR		*line;
{
	d_blank( line, Cols_wide );
}
/**************************************************************************
* d_blank
*	Clear the characters and attributes for "in_count" positions
*	of the window buffer line starting at the posiont pointed to by
*	"chars".
**************************************************************************/
d_blank( chars, in_count )
	FTCHAR		*chars;
	int		in_count;
{
	REG	FTCHAR		*d;
	REG	int		count;

	d = chars;
	count = in_count;
	while( count-- > 0 )
		*d++ = ' ';
}
/**************************************************************************
* d_blankwin_unprotected
*	Set the characters and attributes of the window buffer of the
*	window "Outwin" to the character and attribute of "blank_char_w_attr".
*	Only the characters without the 'protect' attribute are affected.
**************************************************************************/
d_blankwin_unprotected( protect_ftattrs, blank_char_w_attr )
	FTCHAR	protect_ftattrs;
	FTCHAR	blank_char_w_attr;
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	rows = Outwin->display_rows;
	for ( i = 0; i < rows; i++ )
	{
		d_blank_unprotected( Outwin->ftchars[ i ], Cols_wide,
				     protect_ftattrs, blank_char_w_attr );
		Outwin->line_attribute[ i ] = 0;
	}
}
/**************************************************************************
* d_blank_unprotected
*	Set the character and attribute of the "in_count" number of
*	positions starting at "chars" to the character and attribute of
*	"blank_char_w_attr".
*	Only the characters without the 'protect' attribute are affected.
**************************************************************************/
d_blank_unprotected( chars, in_count, protect_ftattrs, blank_char_w_attr )
	FTCHAR		*chars;
	int		in_count;
	FTCHAR		protect_ftattrs;
	FTCHAR		blank_char_w_attr;
{
	REG	FTCHAR		*d;
	REG	int		count;

	d = chars;
	count = in_count;
	while( count-- > 0 )
	{
		if ( (*d & protect_ftattrs) == 0 )
			*d = blank_char_w_attr;
		d++;
	}
}
/**************************************************************************
* d_blank_fld_unprotected
*	Set the character and attribute of the "in_count" number of
*	positions starting at "chars" to the character and attribute of
*	"blank_char_w_attr".
*	Only the characters without the 'protect' attribute are affected.
*	Stop at the first protected char.
**************************************************************************/
d_blank_fld_unprotected( chars, in_count, protect_ftattrs, blank_char_w_attr )
	FTCHAR		*chars;
	int		in_count;
	FTCHAR		protect_ftattrs;
	FTCHAR		blank_char_w_attr;
{
	REG	FTCHAR		*d;
	REG	int		count;

	d = chars;
	count = in_count;
	while( count-- > 0 )
	{
		if ( (*d & protect_ftattrs) == 0 )
			*d = blank_char_w_attr;
		else
			return;
		d++;
	}
}
/**************************************************************************
* d_cursor_first_unprotected
*	Determine the cursor position corresponding to the first position
*	on the screen that does not have the protect attribute.
**************************************************************************/
d_cursor_first_unprotected()
{
	int	i;
	int	j;
	FTCHAR 	*d;

	for ( i = 0; i <= Outwin->buff_bot_row; i++ )
	{
		d = Outwin->ftchars[ i ];
		for ( j = 0; j < Outwin->col_right; j++ )
		{
			if ( (*d & ATTR_PROTECT) == 0 )
			{
				Outwin->row = i;
				Outwin->col = j;
				return;
			}
			d++;
		}
	}
}
/**************************************************************************
* d_cursor_next_unprotected
*	Determine the cursor position corresponding to the next position
*	on the screen that does not have the protect attribute.
*	This starts at, and includes, the current cursor position.
**************************************************************************/
d_cursor_next_unprotected()
{
	int	i;
	int	j;
	FTCHAR 	*d;
	int	first_col;

	first_col = Outwin->col;
	for ( i = Outwin->row; i <= Outwin->buff_bot_row; i++ )
	{
		d = Outwin->ftchars[ i ];
		for ( j = first_col; j < Outwin->col_right; j++ )
		{
			if ( (*d & ATTR_PROTECT) == 0 )
			{
				Outwin->row = i;
				Outwin->col = j;
				return;
			}
			d++;
		}
		first_col = 0;
	}
}
/**************************************************************************
* d_blankwin_w_attr
*	Clear the characters and attributes and line_attributes
*	of the window buffer for the window "Outwin".
*	The positions affected assume the current attributes.
**************************************************************************/
d_blankwin_w_attr( blank_char_w_attr )
	FTCHAR  blank_char_w_attr;
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	rows = Outwin->display_rows;
	for ( i = 0; i < rows; i++ )
	{
		d_blank_w_attr( Outwin->ftchars[ i ], Cols_wide,
							blank_char_w_attr );
		Outwin->line_attribute[ i ] = 0;
	}
}
/**************************************************************************
* d_blank_w_attr
*	Clear the characters and attributes
*	of the "in_count" positions starting at "chars".
*	The positions affected assume the current attributes.
**************************************************************************/
d_blank_w_attr( chars, in_count, blank_char_w_attr )
	FTCHAR		*chars;
	int		in_count;
	FTCHAR		blank_char_w_attr;
{
	REG	FTCHAR		*d;
	REG	int		count;
	REG	FTCHAR		temp;

	d = chars;
	count = in_count;
	temp = blank_char_w_attr;
	while( count-- > 0 )
		*d++ = temp;
}
/**************************************************************************
* d_blankwin_chars_erasable
*	Clear the characters ( only - attributes unaffected )
*	of the window buffer for the window "Outwin".
*	Only erasable characters are affected - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_blankwin_chars_erasable( not_erasable_attr )
	FTCHAR		not_erasable_attr;
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_all( Outwin );
	set_col_changed_all( Outwin );
	rows = Outwin->display_rows;
	for ( i = 0; i < rows; i++ )
	{
		d_blank_chars_erasable( Outwin->ftchars[ i ], Cols_wide, 
							not_erasable_attr );
	}
}
/**************************************************************************
* d_blank_chars
*	Clear the characters ( only - attributes unaffected )
*	of the "in_count" positions starting at "chars".
**************************************************************************/
d_blank_chars( chars, in_count )
	FTCHAR		*chars;
	int		in_count;
{
	REG	FTCHAR		*d;
	REG	int		count;
	REG	FTCHAR		mask;

	d = chars;
	count = in_count;
	mask = FTCHAR_ATTR_MASK & (~F_character_set_attributes);
	while( count-- > 0 )
	{
		*d &= mask;
		*d |= ' ';
		d++;
	}
}
/**************************************************************************
* d_set_attr
*	Set the attributes ( only - characters unaffected )
*	of the "in_count" positions starting at "chars".
*	to the current attribute.
**************************************************************************/
d_set_attr( chars, in_count )
	FTCHAR		*chars;
	int		in_count;
{
	REG	FTCHAR		*d;
	REG	int		count;
	REG	FTCHAR		temp;

	d = chars;
	count = in_count;
	temp = ( Outwin->ftattrs & (~F_character_set_attributes) );
	while( count-- > 0 )
	{
		*d &= FTCHAR_CHAR_MASK;		/* 0xFF */
		*d |= temp;
		d++;
	}
}
/**************************************************************************
* d_blank_chars_erasable
*	Clear the characters ( only - attributes unaffected )
*	of the "in_count" positions starting at "chars".
*	Only erasable characters are affected - characters without the
*	attribute "not_erasable_attr".
**************************************************************************/
d_blank_chars_erasable( chars, in_count, not_erasable_attr )
	FTCHAR		*chars;
	int		in_count;
	FTCHAR		not_erasable_attr;
{
	REG	FTCHAR		*d;
	REG	int		count;
	REG	FTCHAR		mask;

	d = chars;
	count = in_count;
	mask = FTCHAR_ATTR_MASK & (~F_character_set_attributes);
	while( count-- > 0 )
	{
		if ( (*d & not_erasable_attr) == 0 )
		{
			*d &= mask;
			*d |= ' ';
		}
		d++;
	}
}
/**************************************************************************
* d_insert_character
*	Insert a blank position 
*	in the window buffer of "Outwin" at the position "row" and "col".
**************************************************************************/
d_insert_character( row, col )
	int row;
	int col;
{
	REG	int		i;
	REG	FTCHAR		*p;

	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	p = Outwin->ftchars[ row ];
	for ( i = Outwin->col_right_line; i > col; i-- )
		p [ i ] =  p [ i - 1 ];
	p[ col ] = ' ';
}
/**************************************************************************
* d_delete_character
*	Delete the character
*	in the window buffer of "Outwin" at the position "row" and "col".
**************************************************************************/
				/* hp attributes:
				** If character being deleted has an attribute
				** and the character to the right is "real"
				** (i.e. not a blank caused by a clear)
				** and the character to the right does not
				** have an attribute of its own,
				** then the character to the right inherits
				** the attribute as it moves to the position
				** being deleted.
				*/
d_delete_character( row, col )
	int row;
	int col;
{
	REG	int		i;
	REG	FTCHAR		*p;
		FTCHAR		hp_attribute;
		FTCHAR		hp_color;

	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	p = Outwin->ftchars[ row ];
	if ( oF_hp_attribute )
	{
		hp_attribute = p[ col ] & (ATTR_HP_ATTRIBUTE | ATTR_HP_CHARSET);
		if ( hp_attribute )
		{
			if (  ( col >= Outwin->col_right_line                )
			   || ( ( p[ col + 1 ] & ATTR_HP_CHAR_PRESENT ) == 0 )
			   || (   p[ col + 1 ] & 
				    (ATTR_HP_ATTRIBUTE | ATTR_HP_CHARSET)    ) )
			{
				hp_attribute = 0;
			}
		}
		hp_color = p[ col ] & ATTR_HP_COLOR;
		if ( hp_color )
		{
			if (  ( col >= Outwin->col_right_line                )
			   || ( ( p[ col + 1 ] & ATTR_HP_CHAR_PRESENT ) == 0 )
			   || (   p[ col + 1 ] & ATTR_HP_COLOR               ) )
			{
				hp_color = 0;
			}
		}
	}
	for ( i = col; i < Outwin->col_right_line; i++ )
		p[ i ] = p[ i + 1 ];
	p[ Outwin->col_right_line ] = ' ';
	if ( oF_hp_attribute )
	{
		if ( hp_attribute )
			p[ col ] |= hp_attribute;
		if ( hp_color )
			p[ col ] |= hp_color;
	}
}
/**************************************************************************
* ftproc_puts
*	Write the string "s" which is an internally generated message
*	on the current output window.
**************************************************************************/
ftproc_puts( s )
	char *s;
{
	UNCHAR c;	/* ??? */

	while ( (c = *s++ ) != '\0' )
	{
		if ( c == '\n' )
		{
			fct_carriage_return();
			fct_linefeed();
		}
		else
			fct_char( c );
	}
}
/**************************************************************************
* error_puts
*	Write the error message "s" on the current output window.
**************************************************************************/
error_puts( s )
	char	*s;
{
	UNCHAR	c;

	while ( (c = (UNCHAR) (*s++) ) != '\0' )
		fct_char( c );
}
/* =================================================================== */
/**************************************************************************
* scroll_memory_refresh
*	Refresh the scroll memory of the HP terminal
*	from the scroll memory buffer for the window Outwin.
**************************************************************************/
scroll_memory_refresh()
{
	int	row;
	int	full_line;
	int	last_row;

	if ( mF_has_scroll_memory <= 0 )
		return;
	if ( Rows_scroll_memory <= 0 )
	{
		term_out_scroll_memory_clear();
		return;
	}

	term_scroll_refresh_start();
	for ( row = 0; row < Rows_scroll_memory; row++ )
	{
		out_term_clr_eol();
		if ( F_scroll_refresh_no_scroll_last )
		{
			if ( row == ( Rows_scroll_memory - 1 ) )
				last_row = 1;
			else
				last_row = 0;
		}
		else
			last_row = 0;
		full_line = toscreen_row_scroll_memory( Outwin, 
				Outwin->scroll_memory_ftchars[ row ], last_row, 
				NO_ATTRS_AT_END );
		term_carriage_return();
		if ( ! last_row )
		{
			if ( full_line == 0 )
				term_linefeed();
			else if ( F_eat_newline_glitch[ Outwin->personality ] )
				term_linefeed();
			else if ( F_real_eat_newline_glitch )
			{
				term_carriage_return();
				term_linefeed();
			}
		}
	}
	term_scroll_refresh_end();
}
/**************************************************************************
* term_scroll_refresh_start
*	Output the terminal description file string specified for the
*	beginning of a scroll memory refresh.
**************************************************************************/
term_scroll_refresh_start()
{
	if ( T_scroll_refresh_start != NULL )
		term_tputs( T_scroll_refresh_start );
}
/**************************************************************************
* term_scroll_refresh_end
*	Output the terminal description file string specified for the
*	end of a scroll memory refresh.
**************************************************************************/
term_scroll_refresh_end()
{
	if ( T_scroll_refresh_end != NULL )
		term_tputs( T_scroll_refresh_end );
}
/**************************************************************************
* term_out_scroll_memory_clear
*	Output the terminal description file string specified for clearing
*	the scroll memory.
**************************************************************************/
term_out_scroll_memory_clear()
{
	if ( T_out_scroll_memory_clear != NULL )
		term_tputs( T_out_scroll_memory_clear );
}
/**************************************************************************
* full_refresh
*	Perform a full screen refresh for Outwin.
**************************************************************************/
full_refresh()
{
	f_refresh( 0, Outwin->display_row_bottom );
	onstatus_refresh();
}
/**************************************************************************
* f_refresh_around_split
*	Fill in the lines that were missing after a split screen was put
*	into the proper position on a full screen.
**************************************************************************/
f_refresh_around_split()
{
	int	b_top_row;
	int	b_bot_row;

	b_top_row = Outwin->buff_top_row;
	if ( b_top_row > 0 )
		f_refresh( 0, b_top_row - 1 );
	b_bot_row = Outwin->buff_bot_row;
	if ( b_bot_row < Outwin->display_row_bottom )
		f_refresh( b_bot_row + 1, Outwin->display_row_bottom );
	else
		f_refresh( Outwin->display_row_bottom, 
			   Outwin->display_row_bottom );
}
/**************************************************************************
* f_refresh
*	Refresh rows "top_row" to "bot_row" on a full screen.
**************************************************************************/
f_refresh( top_row, bot_row )
	int	top_row;
	int	bot_row;
{
	int	row;
	int	full_line;

	term_pos( top_row, 0 );
	for ( row = top_row; row <= bot_row; row++ )
	{
		out_term_clr_eol();
		full_line = toscreen_row( Outwin, row, row, NO_ATTRS_AT_END );
		term_carriage_return();
		if ( row != Row_bottom_terminal )
		{
			term_pos( row + 1, 0 );
		}
	}
}
/**************************************************************************
* f_refresh_cols
*	Refresh columns "first_col" to "last_col" on row "row" on a
*	full screen.
*	The line attributes must be off when this module is called.
**************************************************************************/
f_refresh_cols( row, first_col, last_col )	/* assumes line_attribute off */
	int	row;
	int	first_col;
	int	last_col;
{
	toscreen_row_between_cols( Outwin, row, row, first_col, last_col,
							NO_ATTRS_AT_END );
}
/**************************************************************************
* f_refresh_cols_on_row
*	Refresh columns "first_col" to "last_col" on row "row" on a
*	screen row number "screen_row" using the window buffer line
*	pointed to by the pointer "p".
*	This is used by the popup refresh.
**************************************************************************/
f_refresh_cols_on_row( p, screen_row, first_col, last_col )
	FTCHAR	*p;
	int	screen_row;
	int	first_col;
	int	last_col;
{
	term_pos( screen_row, first_col );
	toscreen_w_attr( &p[ first_col ], last_col - first_col + 1, 
							NO_ATTRS_AT_END );
}
/**************************************************************************
* split_refresh
*	Refresh a split screen window "Outwin".
**************************************************************************/
split_refresh()
{
	s_refresh( Outwin,
		   0, Outwin->win_bot_row - Outwin->win_top_row );
	onstatus_refresh();
}
/**************************************************************************
* s_refresh
*	Refresh the split screen window "window" from "first" to "last"
*	screen line in the window.
**************************************************************************/
s_refresh( window, first, last )
	FT_WIN	*window;
	int	first;			/* first screen line in window */
	int	last;			/* last screen line in window */
{
	int	i;
	int	s_row;
	int	b_row;
	int	rows;
	int	full_line;

	s_row = window->win_top_row + first;
	b_row = window->buff_top_row + first;
	rows = last - first + 1;
	term_pos( s_row, 0 );
	for ( i = 0; i < rows; i++ )
	{
		out_term_clr_eol();
		full_line = toscreen_row( window, b_row, s_row,
							NO_ATTRS_AT_END );
		term_carriage_return();
		b_row++;
		s_row++;
		term_pos( s_row, 0 );
	}
}
/**************************************************************************
* onstatus_refresh
*	Refresh a "onstatus" type of status line.
*	These look like additional lines that are cursor addressable.
**************************************************************************/
onstatus_refresh()
{
	int	status_line;

	if ( F_has_status_line == 0 )
		return;
	status_line = Outwin->display_row_bottom + 1;
	if ( status_line > Row_bottom_max )
		return;
	term_out_onstatus( 0 );
	out_term_clr_eol();
	toscreen_row_onstatus( Outwin, status_line, NO_ATTRS_AT_END );
	term_carriage_return();
}
/**************************************************************************
* toscreen_row_onstatus
*	Output the characters and attributes of the onstatus row number
*	"row" on window "window".
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
*	Assumes that the terminal is at col one of blank screen row.
**************************************************************************/
toscreen_row_onstatus( window, row, attr_at_end )
	FT_WIN	*window;
	int	row;
	int	attr_at_end;
{
	int		fnb_col;
	int		full_line;
	FTCHAR		*p;
	int		len;

	term_line_attribute( window->line_attribute[ row ] );
	len = get_non_blank( window, window->ftchars[ row], 
			     window->line_attribute[ row ], 
			     &fnb_col, &full_line, &p );
	if ( len )
	{
		if ( len > window->cols  || len < 0 )
		{
			sprintf( Err_buff, "(<(len = %d)>)", len );
			error_puts( Err_buff );
			return( 0 );
		}
		if ( fnb_col > 0 )
			term_out_onstatus( fnb_col );
		if (  full_line 
		   && ( F_eat_newline_glitch[ window->personality] == 0 ) )
		{
			len -= 1;
			full_line = 0;
		}
		toscreen_w_attr( p, len, attr_at_end );
	}
	return( full_line );
}
/**************************************************************************
* toscreen_row
*	Output the characters and attributes to refresh
*	row number "row" of the window "window".
*	This output is being done on screen row number "screen_row".
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
*	Assumes that the terminal is at col one of blank screen row.
**************************************************************************/
toscreen_row( window, row, screen_row, attr_at_end )
				/* assumes at col one of blank screen row */
	FT_WIN	*window;
	int	row;
	int	screen_row;
	int	attr_at_end;
{
	int		fnb_col;
	int		full_line;
	FTCHAR		*p;
	int		len;

	term_line_attribute( window->line_attribute[ row ] );
	len = get_non_blank( window, window->ftchars[ row ], 
			     window->line_attribute[ row ],
			     &fnb_col, &full_line, &p );
	if ( len )
	{
		if ( len > window->cols  || len < 0 )
		{
			sprintf( Err_buff, "(<(len = %d)>)", len );
			error_puts( Err_buff );
			return( 0 );
		}
		if ( fnb_col > 0 )
			term_pos( screen_row, fnb_col );
		if (  full_line 
		   && (screen_row == Row_bottom_terminal)
		   && ( F_eat_newline_glitch[ window->personality] == 0 ) )
		{
			len -= 1;
			full_line = 0;
		}
		toscreen_w_attr( p, len, attr_at_end );
	}
	return( full_line );
}
/**************************************************************************
* toscreen_row_scroll_memory
*	Output the characters and attributes to refresh
*	the scroll memroy of the terminal for window "window".
*	Use the characters pointed to by "ftchars".
*	If "last_row" is non-zero, then this is the last row of the
*	scroll memory.
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
*	Assumes that the terminal is at col one of blank screen row.
**************************************************************************/
toscreen_row_scroll_memory( window, ftchars, last_row, attr_at_end )
	FT_WIN	*window;
	FTCHAR	*ftchars;
	int	last_row;
	int	attr_at_end;
{
	int		fnb_col;
	int		full_line;
	FTCHAR		*p;
	int		len;

	len = get_non_blank( window, ftchars, 0,
			     &fnb_col, &full_line, &p );
	if ( len )
	{
		if ( len > window->cols  || len < 0 )
		{
			sprintf( Err_buff, "(<(len = %d)>)", len );
			error_puts( Err_buff );
			return( 0 );
		}
		if ( fnb_col > 0 )
		{
			if ( term_column_address( fnb_col ) < 0 )
			{
				p = ftchars;
				len += fnb_col;
			}
		}
		if (  full_line 
		   && last_row
		   && ( F_eat_newline_glitch[ window->personality] == 0 ) )
		{
			len -= 1;
			full_line = 0;
		}
		toscreen_w_attr( p, len, attr_at_end );
	}
	return( full_line );
}
/**************************************************************************
* toscreen_row_between_cols
*	Output the characters and attributes to refresh
*	row number "row" for window "window" 
*	between the columns "first_col" and "last_col".
*	The output is being done on terminal row "screen_row".
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
*	Assumes that the terminal line attributes are off for this line.
**************************************************************************/
toscreen_row_between_cols( window, row, screen_row, first_col, last_col,
			   attr_at_end )
	FT_WIN	*window;
	int	row;
	int	screen_row;
	int	first_col;
	int	last_col;
	int	attr_at_end;
{
	int		len;

	len = last_col - first_col + 1;
	if ( len > window->cols  || len < 0 )
	{
		sprintf( Err_buff, "(<(len = %d)>)", len );
		error_puts( Err_buff );
	}
	term_pos( screen_row, first_col );
	if (  ( last_col == window->col_right )
	   && ( screen_row == Row_bottom_terminal)
	   && ( F_eat_newline_glitch[ window->personality] == 0 ) )
	{
		len -= 1;
	}
	toscreen_w_attr( &window->ftchars[ row ][ first_col ],
							len, attr_at_end );
}
/**************************************************************************
* toscreen_char_at_cursor
*	Refresh the character and attributes of the position under the
*	cursor.
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
**************************************************************************/
toscreen_char_at_cursor( attr_at_end )
	int	attr_at_end;
{
	toscreen_w_attr( &(Outwin->ftchars[ Outwin->row ][ Outwin->col ]), 1,
		attr_at_end );
}
/**************************************************************************
* toscreen_char_at_row_col
*	Refresh the character and attributes of the position
*	at "row" and "col".
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
**************************************************************************/
toscreen_char_at_row_col( row, col, attr_at_end )
	int	row;
	int	col;
	int	attr_at_end;
{
	toscreen_w_attr( &(Outwin->ftchars[ row ][ col ]), 1, attr_at_end );
}
/**************************************************************************
* toscreen_w_attr
*	Refresh the character and attributes of the "len" number of position
*	using the characters in the window buffer pointed to by "p".
*	If "attr_at_end" is non-zero, return the terminal to the current
*	attributes.
*	Otherwise leave the attributes off.
**************************************************************************/
toscreen_w_attr( p, len, attr_at_end )
	FTCHAR		*p;
	int		len;
	int		attr_at_end;
{
	REG	FTCHAR 		ftattrs;
	REG	int		c;
	REG	int		i;
	REG	int		attribute_next;

	for ( i = 0; i < len; i++ )
	{
		ftattrs = *p & FTCHAR_ATTR_MASK;
		c = *p++ & FTCHAR_CHAR_MASK;
		if ( mF_hp_attribute )
		{
			ftattrs &= ( ~ ATTR_HP_CHAR_PRESENT );
			if ( ftattrs )
				t_hp_attribute( ftattrs );
			if (  ( c < 0x20 ) 
			   && mF_graphics_escape_control
			   )
			{
				term_putc( '\033' );
				term_putc( (char) c );
			}
			else if (  ( c == 0x7F ) 
			   && mF_graphics_escape_delete
			   )
			{
				term_putc( '\033' );
				term_putc( (char) c );
			}
			else
				term_putc( (char) c );
		}
		else if ( ftattrs & ATTR_MAGIC )
		{
			ftattrs &= ( ~ATTR_MAGIC );
			attribute_next = t_sync_attr( ftattrs );
			t_sync_character_set_to_attr( ftattrs );
			if ( c == 0 )
				term_nomagic();
			else
				term_magic( c );
		}
		else
		{
			attribute_next = t_sync_attr( ftattrs );
			t_sync_character_set_to_attr( ftattrs );
			if (  ( c < 0x20 ) 
			   && mF_graphics_escape_control
			   && ( attribute_next == 0 ) )
			{
				term_putc( '\033' );
				term_putc( (char) c );
			}
			else if (  ( c == 0x7F ) 
			   && mF_graphics_escape_delete
			   && ( attribute_next == 0 ) )
			{
				term_putc( '\033' );
				term_putc( (char) c );
			}
			else
				term_putc( (char) c );
		}
	}
	if ( attr_at_end == 0 )
	{
		t_sync_attr( (FTCHAR) 0 );
		t_sync_character_set_base();
		if ( mF_hp_attribute )
			t_sync_hp_charset_select_base();
	}
	else if ( attr_at_end == 1 )
	{
		t_sync_attr( Outwin->ftattrs );
		t_sync_character_set();
		if ( mF_hp_attribute )
			t_sync_hp_charset_select_outwin();
	}
}
/**************************************************************************
* get_non_blank
*	Analyze the characters in the window buffer row pointed to
*	by "ftchars" from the window "window" that has line attribute
*	"line_attribute".
*	Return the column number of the first non blank column at the
*		integer pointed to by p_fnb_col.
*	Return a 1 in the integer pointed to by p_full_line if there is
*		a non-blank character in the far right column. 0 Otherwise.
*	Return a pointer to the first non-blank character in the pointer
*		pointed to by p_string.
*	Return the number of non-blank characters on the line.
**************************************************************************/
get_non_blank( window, ftchars, line_attribute, 
					p_fnb_col, p_full_line,  p_string )
	FT_WIN	*window;
	FTCHAR	*ftchars;
	int	line_attribute;
	int	*p_fnb_col;		/* first non blank col */
	int	*p_full_line;		/* 1 = char in col Col_right */
	FTCHAR	**p_string;		/* pointer to first non blank */
{
	int	lnb_col;	/* last non blank column */
	int	fnb_col;	/* first non blank column */
	FTCHAR	*p;
	int	len;
	int	cols_line;
	int	col_right_line;

	*p_full_line = 0;
	cols_line = window->cols;
	if ( line_attribute )
		cols_line >>= 1;
	col_right_line = cols_line - 1;
	p = &ftchars[ col_right_line ];
	for( lnb_col = col_right_line; lnb_col >= 0; lnb_col-- )
	{
		if ( *p-- != ' ' )
		{
			p = &ftchars[ 0 ];
			for( fnb_col = 0; fnb_col < cols_line; fnb_col++ )
			{
				if ( *p != ' ' )
					break;
				p++;
			}
			*p_fnb_col = fnb_col;
			if ( lnb_col == col_right_line )
				*p_full_line = 1;
			*p_string = p;
			len = lnb_col - fnb_col + 1;
			if ( len > cols_line  || len < 0 )
			{
				sprintf( Err_buff, "(<<len = %d>>)", len );
				error_puts( Err_buff );
				return( 0 );
			}
			return( len );
		}
	}
	return( 0 );
}
/**************************************************************************
* clear_row_changed
*	Clear the array that tracks changed rows for the window "win".
**************************************************************************/
clear_row_changed( win )
	FT_WIN	*win;
{
	REG	int	j;
	REG	int	rows;

	win->row_changed_all = 0;
	rows = win->display_rows;
	for ( j = 0; j < rows; j++ )
		win->row_changed[ j ] = 0;
}
/**************************************************************************
* set_row_changed_all
*	Set the flag that all rows have changed on the window "win".
*	E.G. the screen scrolled.
**************************************************************************/
set_row_changed_all( win )
	FT_WIN	*win;
{
	win->row_changed_all = 1;
}
/**************************************************************************
* set_row_changed_1
*	Record that a change was made on row "row" on window "win".
**************************************************************************/
set_row_changed_1( win, row )
	FT_WIN	*win;	
	int	row;
{
	win->row_changed[ row ] = 1;
}
/**************************************************************************
* set_row_changed
*	Record that a change was made on row "top_row" to row "bot_row"
*	on window "win".
**************************************************************************/
set_row_changed( win, top_row, bot_row )
	FT_WIN	*win;	
	int	top_row;
	int	bot_row;
{
	REG	int	i;
	REG	unsigned char	*rc;

	rc = &win->row_changed[ top_row ];
	for ( i = top_row; i <= bot_row; i++ )
		*rc++ = 1;
}
/**************************************************************************
* set_row_changed_eos
*	Record that a change was made on window "win"
*	from the row "top_row" to the end of the screen.
**************************************************************************/
set_row_changed_eos( win, top_row )
	FT_WIN	*win;	
	int	top_row;
{
	REG	int	i;
	REG	unsigned char	*rc;
	REG	int	row_bottom;

	row_bottom = win->display_row_bottom;
	rc = &win->row_changed[ top_row ];
	for ( i = top_row; i <= row_bottom; i++ )
		*rc++ = 1;
}
/**************************************************************************
* set_row_changed_bos
*	Record that a change was made on window "win"
*	from the row "bot_row" to the beginning of the screen.
**************************************************************************/
set_row_changed_bos( win, bot_row )
	FT_WIN	*win;	
	int	bot_row;
{
	REG	int	i;
	REG	unsigned char	*rc;

	rc = &win->row_changed[ bot_row ];
	for ( i = bot_row; i >= 0; i-- )
		*rc-- = 1;
}
/**************************************************************************
* find_row_changed
*	Determine which rows were changed on window "win".
*	Return the top row in the integer pointed to by "p_first_row".
*	Return the bottom row in the integer pointer to by "p_last_row".
**************************************************************************/
find_row_changed( win, p_first_row, p_last_row )
	FT_WIN	*win;	
	int	*p_first_row;
	int	*p_last_row;
{
	int	i;
	int	j;

	if ( win->row_changed_all )
	{
		*p_first_row = 0;
		*p_last_row = win->display_row_bottom;
		return( 1 );
	}
	for ( i = 0; i <= win->display_row_bottom; i++ )
	{
		if ( win->row_changed[ i ] )
			break;
	}
	if ( i > win->display_row_bottom )
	{
		*p_first_row = 0;
		*p_last_row = 0;
		return( 0 );
	}
	*p_first_row = i;
	for ( j = win->display_row_bottom; j > i; j-- )
	{
		if ( win->row_changed[ j ] )
			break;
	}
	*p_last_row = j;
	return( 1 );
}
/**************************************************************************
* clear_col_changed
*	Clear the array that tracks columns changed for window "win".
**************************************************************************/
clear_col_changed( win )
	FT_WIN	*win;
{
	REG	int	j;

	win->col_changed_all = 0;
	for ( j = 0; j < Cols_wide; j++ )
		win->col_changed[ j ] = 0;
}
/**************************************************************************
* set_col_changed_all
*	Set the flag that all columns were changed on window "win".
*	E.G. whole line changed.
**************************************************************************/
set_col_changed_all( win )
	FT_WIN	*win;
{
	win->col_changed_all = 1;
}
/**************************************************************************
* set_col_changed_1
*	Record that column "col" of window "win" changed.
**************************************************************************/
set_col_changed_1( win, col )
	FT_WIN	*win;	
	int	col;
{
	win->col_changed[ col ] = 1;
}
/**************************************************************************
* set_col_changed_eol
*	Record that columns of window "win" changed from column "first_col"
*	to the end of the line.
**************************************************************************/
set_col_changed_eol( win, first_col )
	FT_WIN	*win;	
	int	first_col;
{
	REG	int	i;
	REG	unsigned char	*cc;

	cc = &win->col_changed[ first_col ];
	for ( i = first_col; i <= Col_right_wide; i++ )
		*cc++ = 1;
}
/**************************************************************************
* set_col_changed_bol
*	Record that columns of window "win" changed from column "first_col"
*	to the beginning of the line.
**************************************************************************/
set_col_changed_bol( win, last_col )
	FT_WIN	*win;	
	int	last_col;
{
	REG	int	i;
	REG	unsigned char	*cc;

	cc = &win->col_changed[ 0 ];
	for ( i = 0; i <= last_col; i++ )
		*cc++ = 1;
}
/**************************************************************************
* set_col_changed_count
*	Record that columns of window "win" changed for "count" columns 
*	beginning at column"first_col".
**************************************************************************/
set_col_changed_count( win, first_col, count )
	FT_WIN	*win;	
	int	first_col;
	int	count;
{
	REG	unsigned char	*cc;

	cc = &win->col_changed[ first_col ];
	while( count-- )
		*cc++ = 1;
}
/**************************************************************************
* find_col_changed
*	Determine which columns were changed on window "win".
*	Return the left column in the integer pointed to by "p_first_col".
*	Return the right column in the integer pointer to by "p_last_col".
**************************************************************************/
find_col_changed( win, p_first_col, p_last_col )
	FT_WIN	*win;	
	int	*p_first_col;
	int	*p_last_col;
{
	int	i;
	int	j;

	if ( win->col_changed_all )
	{
		*p_first_col = 0;
		*p_last_col = win->col_right;
		return( 1 );
	}
	for ( i = 0; i <= win->col_right; i++ )
	{
		if ( win->col_changed[ i ] )
			break;
	}
	if ( i > win->col_right )
	{
		*p_first_col = 0;
		*p_last_col = 0;
		return( 0 );
	}
	*p_first_col = i;
	for ( j = win->col_right; j > i; j-- )
	{
		if ( win->col_changed[ j ] )
			break;
	}
	*p_last_col = j;
	return( 1 );
}
/**************************************************************************
* d_clr_eos_unprotected
*	Clear the positions
*	of the window buffer from "row" and "col" to the end of the screen.
*	Only unprotected characters are affected - characters without any of
*	the attributes "protect_ftattrs".
**************************************************************************/
d_clr_eos_unprotected( row, col, protect_ftattrs, blank_char_w_attr )
	int	row;
	int	col;
	FTCHAR	protect_ftattrs;		/* chars with any of these
						   attributes are protected */
	FTCHAR	blank_char_w_attr;
{
	REG	int	i;
	REG	int	rows;

	set_row_changed_eos( Outwin, row );
	set_col_changed_all( Outwin );
	d_blank_unprotected( &Outwin->ftchars[ row ][ col ], Cols_wide - col,
			     protect_ftattrs, blank_char_w_attr );
	rows = Outwin->display_rows;
	for ( i = row + 1; i < rows; i++ )
	{
		d_blank_unprotected( Outwin->ftchars[ i ], Cols_wide, 
				     protect_ftattrs, blank_char_w_attr );
	}
}
/**************************************************************************
* d_clr_eol_unprotected
*	Clear the positions in the
*	window buffer from position "row" and *	"col" to the end of the line.
*	Only unprotected characters are affected - characters without the
*	attribute "protect_ftattrs".
**************************************************************************/
d_clr_eol_unprotected( row, col, protect_ftattrs, blank_char_w_attr )
	int	row;
	int	col;
	FTCHAR	protect_ftattrs;
	FTCHAR	blank_char_w_attr;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	d_blank_unprotected( &Outwin->ftchars[ row ][ col ], Cols_wide - col,
			     protect_ftattrs, blank_char_w_attr );
}
/**************************************************************************
* d_clr_fld_unprotected
*	Clear the positions in the
*	window buffer from position "row" and *	"col" to the end of the line.
*	Only unprotected characters are affected - characters without the
*	attribute "protect_ftattrs".
*	Stop at the first protected character
**************************************************************************/
d_clr_fld_unprotected( row, col, protect_ftattrs, blank_char_w_attr )
	int	row;
	int	col;
	FTCHAR	protect_ftattrs;
	FTCHAR	blank_char_w_attr;
{
	set_row_changed_1( Outwin, row );
	set_col_changed_eol( Outwin, col );
	d_blank_fld_unprotected( &Outwin->ftchars[ row ][ col ],
				 Cols_wide - col,
				 protect_ftattrs, blank_char_w_attr );
}
