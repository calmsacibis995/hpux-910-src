/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_curs.c,v 70.1 92/03/09 16:13:08 ssa Exp $ */
/*
 * File: tc_curs.c
 * Creator: G.Clark.Brown
 *
 * Test routines for character and line cursor in the Terminfo Exerciser
 * Program.
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
#include	<curses.h>
#include	<term.h>
#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

static char	*testpattern[]= {
					"As she turned, she saw a tall    ",
					"man standing near the door. He   ",
					"was dressed in a grey coat as    ",
					"he had been in her dream. When   ",
					"he began to speak, she knew      ",
					"that he was the man she had      ",
					"met at Caroline's at Christmas   ",
					"time.  He told her that he had   ",
					"been in Europe all summer on     ",
					"business, but now was in the     "
				};



#ifdef cursor_address

/* = = = = = = = = = = = = = = = = tc_cup = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_address capability.
 */
tc_cup(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	left=0, right=29, top=0, bot=9;

	if(cursor_address)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (left<=right && top<=bot)
		{
		/* move right across top */
			for (col=left; col<=right; col++)
			{
				tu_cup(5+top, 20+col);
				tc_putchar((testpattern[top])[col]);
			}
			top++;
		/* move down across right */
			for (row=top; row<=bot; row++)
			{
				tu_cup(5+row , 20+right);
				tc_putchar((testpattern[row])[right]);
			}
			right--;
		/* move left across bot */
			for (col=right; col>=left; col--)
			{
				tu_cup(5+bot, 20+col);
				tc_putchar((testpattern[bot])[col]);
			}
			bot--;
		/* move up across left */
			for (row=bot; row>=top; row--)
			{
				tu_cup(5+row , 20+left);
				tc_putchar((testpattern[row])[left]);
			}
			left++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif
#ifdef cursor_left

/* = = = = = = = = = = = = = = = = tc_cub1 = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_left capability.
 */
tc_cub1(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	left=0, right=29;

	if(cursor_left)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (left<=right)
		{
		/* move down across right */
			for (row=0; row<=9; row++)
			{
				tu_cup(5+row , 20+right);
				tc_putchar((testpattern[row])[right]);
		/* move to left using cursor_left */
				for (col=0; col<=right-left; col++)
					tputs(cursor_left, 1, tc_putchar);
				tc_putchar((testpattern[row])[left]);
			}
			right--;
			left++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef cursor_right

/* = = = = = = = = = = = = = = = = tc_cuf1 = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_right capability.
 */
tc_cuf1(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	left=14, right=15;

	if(cursor_right)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (left>=0)
		{
		/* move down across left */
			for (row=0; row<=9; row++)
			{
				tu_cup(5+row , 20+left);
				tc_putchar((testpattern[row])[left]);
		/* move to right using cursor_right */
				for (col=0; col<right-left-1; col++)
					tputs(cursor_right, 1, tc_putchar);
				tc_putchar((testpattern[row])[right]);
			}
			right++;
			left--;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef cursor_up

/* = = = = = = = = = = = = = = = = tc_cuu1 = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_up capability.
 */
tc_cuu1(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	top=0, bot=9;

	if(cursor_up)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (top<=bot)
		{
		/* move right across the bottom */
			for (col= -1; col<=29;)
			{
				tu_cup(5+bot , 20+col);
				if (col>=0)
					tc_putchar((testpattern[bot])[col]);
				else
					tu_cup(5+bot, 20+col+1);
		/* move to top using cursor_up */
				for (row=0; row<bot-top; row++)
					tputs(cursor_up, 1, tc_putchar);
				if (col++ <29)
					tc_putchar((testpattern[top])[col]);
			}
			bot--;
			top++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef cursor_down

/* = = = = = = = = = = = = = = = = tc_cud1 = = = = = = = = = = = = = = = = =*/
/*
 * Test cursor_down capability.
 */
tc_cud1(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	top=0, bot=9;

	if(cursor_down)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (top<=bot)
		{
		/* move right across the top */
			for (col= -1; col<=29;)
			{
				tu_cup(5+top , 20+col);
				if (col>=0)
					tc_putchar((testpattern[top])[col]);
				else
					tu_cup(5+top, 20+col+1);
		/* move to bottom using cursor_down */
				for (row=0; row<bot-top; row++)
					tputs(cursor_down, 1, tc_putchar);
				if (col++ <29)
					tc_putchar((testpattern[bot])[col]);
			}
			bot--;
			top++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef parm_left_cursor

/* = = = = = = = = = = = = = = = = tc_cub = = = = = = = = = = = = = = = = =*/
/*
 * Test parm_left_cursor capability.
 */
tc_cub(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	left=0, right=29;

	if(parm_left_cursor)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (left<=right)
		{
		/* move down across right */
			for (row=0; row<=9; row++)
			{
				tu_cup(5+row , 20+right);
				tc_putchar((testpattern[row])[right]);
		/* move to left using parm_left_cursor */
				if(right-left+1>0)
					tputs(tparm(parm_left_cursor,
						right-left+1), 1, tc_putchar);
				tc_putchar((testpattern[row])[left]);
			}
			right--;
			left++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef parm_right_cursor

/* = = = = = = = = = = = = = = = = tc_cuf = = = = = = = = = = = = = = = = =*/
/*
 * Test parm_right_cursor capability.
 */
tc_cuf(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	left=0, right=29;

	if(parm_right_cursor)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (left<=right)
		{
		/* move down across left */
			for (row=0; row<=9; row++)
			{
				tu_cup(5+row , 20+left);
				tc_putchar((testpattern[row])[left]);
		/* move to right using parm_right_cursor */
				if (right-left-1 >0)
					tputs(tparm(parm_right_cursor,
						right-left-1), 1, tc_putchar);
				tc_putchar((testpattern[row])[right]);
			}
			right--;
			left++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef parm_up_cursor

/* = = = = = = = = = = = = = = = = tc_cuu = = = = = = = = = = = = = = = = =*/
/*
 * Test parm_up_cursor capability.
 */
tc_cuu(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	top=0, bot=9;

	if(parm_up_cursor)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (top<=bot)
		{
		/* move right across the bottom */
			for (col= -1; col<=29;)
			{
				tu_cup(5+bot , 20+col);
				if (col>=0)
					tc_putchar((testpattern[bot])[col]);
				else
					tu_cup(5+bot, 20+col+1);
		/* move to top using parm_up_cursor */
				if (bot-top >0)
					tputs(tparm(parm_up_cursor,
						bot-top), 1, tc_putchar);
				if (col++ <29)
					tc_putchar((testpattern[top])[col]);
			}
			bot--;
			top++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef parm_down_cursor

/* = = = = = = = = = = = = = = = = tc_cud = = = = = = = = = = = = = = = = =*/
/*
 * Test parm_down_cursor capability.
 */
tc_cud(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	top=0, bot=9;

	if(parm_down_cursor)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (top<=bot)
		{
		/* move right across the top */
			for (col= -1; col<=29;)
			{
				tu_cup(5+top , 20+col);
				if (col>=0)
					tc_putchar((testpattern[top])[col]);
				else
					tu_cup(5+top, 20+col+1);
		/* move to bottom using parm_down_cursor */
				if (bot-top >0)
					tputs(tparm(parm_down_cursor,
						bot-top), 1, tc_putchar);
				if (col++ <29)
					tc_putchar((testpattern[bot])[col]);
			}
			bot--;
			top++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef save_cursor
#ifdef restore_cursor

/* = = = = = = = = = = = = = = = = tc_scrc = = = = = = = = = = = = = = = = =*/
/*
 * Test save_cursor and restore_cursor capabilities.
 */
tc_scrc(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	top=0, bot=9;

	if(save_cursor && restore_cursor)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (top<=bot)
		{
			tu_cup(5+top , 20);
			tputs(save_cursor, 1, tc_putchar);
			for (col=0; col<=29; col++)
			{	
				tu_cup(5+bot, 20+(29-col));
				tc_putchar((testpattern[bot])[29-col]);
				tputs(restore_cursor, 1, tc_putchar);
				tc_putchar((testpattern[top])[col]);
				tputs(save_cursor, 1, tc_putchar);
			}
			bot--;
			top++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}

#endif
#endif
/* = = = = = = = = = = = = = = = = tc_cuborder = = = = = = = = = = = = = = = =*/
/*
 * Put a border around the testpattern (called before the pattern is printed).
 */
tc_cuborder(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	printf("\r\n\n");     		/* NOTE: this assumes that we are
						already on the third screen line
						(from printing the title). */

	printf(
		"                   +vvvvvvvvvvvvvvvvvvvvvvvvvvvvvv+\r\n");
	for (i=0; i<10; i++)
		printf(
		     "                   >                              <\r\n");
	printf(
		"                   +^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^+\r\n");
	return;
}

#ifdef column_address

/* = = = = = = = = = = = = = = = = tc_hpa = = = = = = = = = = = = = = = = =*/
/*
 * Test column_address capability.
 */
tc_hpa(p_opts)

struct option_tab	*p_opts;
{
	int	row,col;
	int	left=0, right=29, top=0, bot=9;

	if(column_address)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		while (left<=right && top<=bot)
		{
		/* move left across top */
			tu_cup(5+top, 0);
			for (col=right; col>=left; col--)
			{
				tputs(tparm(column_address, col+20),1,tc_putchar);
				tc_putchar((testpattern[top])[col]);
			}
			top++;
		/* move down across right */
			for (row=top; row<=bot; row++)
			{
				tu_cup(5+row , 20+right);
				tc_putchar((testpattern[row])[right]);
			}
			right--;
		/* move right across bot */
			tu_cup(5+bot, 0);
			for (col=left; col<=right; col++)
			{
				tputs(tparm(column_address, col+20),1,tc_putchar);
				tc_putchar((testpattern[bot])[col]);
			}
			bot--;
		/* move up across left */
			for (row=bot; row>=top; row--)
			{
				tu_cup(5+row , 20+left);
				tc_putchar((testpattern[row])[left]);
			}
			left++;
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif


#ifdef row_address

/* = = = = = = = = = = = = = = = = tc_vpa = = = = = = = = = = = = = = = = =*/
/*
 * Test row_address capability.
 */
tc_vpa(p_opts)

struct option_tab	*p_opts;
{
	int	row, col, start;
	int	right=29, bot=9;

	if(row_address)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tc_cuborder(p_opts);
		/* move left across top */
		for (col=right; col>= -bot; col--)
		{
			if (col<0)
				start= -col;
			else
				start=0;

			tu_cup(5+start, 20+col+start);
			for (row=start; row<=bot && row<=right-col; row++)
			{
				tputs(tparm(row_address, row+5), 1,
					tc_putchar);
				tc_putchar((testpattern[row])[col+row]);
			}
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif

