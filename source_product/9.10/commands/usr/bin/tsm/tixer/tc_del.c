/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_del.c,v 70.1 92/03/09 16:13:18 ssa Exp $ */
/*
 * File: tc_del.c
 * Creator: G.Clark.Brown
 *
 * Test routines for character and line deletes in the Terminfo Exerciser
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

static char	teststring[]= "ABCDEFGHIJabcdefghij0123456789";

#ifdef delete_character

/* = = = = = = = = = = = = = tc_del_1char = = = = = = = = = = = = = =*/
/*
 * Test delete_character capability.
 */
tc_del_1char(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	if(delete_character)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tu_cup(5,0);
		printf("    standard -->");
		tu_cup(5,20);
		printf("%s", teststring);
		tc_putchar('\r');
		tc_putchar('\n');
		tu_cup(6,0);
		printf("    using dc -->");
		tu_cup(6,20);
		for (i=0; teststring[i]; i++)
		{
			tc_putchar('#');
			tc_putchar(teststring[i]);
		}
		for (i=0; teststring[i]; i++)
		{
			tu_cup(6, i+20);
			tputs(delete_character, 1, tc_putchar);
		}
		tu_cup(15,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef parm_dch

/* = = = = = = = = = = = = = tc_del_char = = = = = = = = = = = = = =*/
/*
 * Test parm_dch capability.
 */
tc_del_char(p_opts)

struct option_tab	*p_opts;
{
	int	i,j;

	if(parm_dch)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tu_cup(5,0);
		printf("    standard       -->");
		tu_cup(5,25);
		printf("%6.6s", teststring);
		tc_putchar('\r');
		tc_putchar('\n');
		tu_cup(6,0);
		printf("    using parm_dch -->");
		tu_cup(6,25);
		for (i=0; i<6; i++)
		{
			for (j=0; j<i+1; j++)
				tc_putchar('#');
			tc_putchar(teststring[i]);
		}
		for (i=0; i<6; i++)
		{
			tu_cup(6, i+25);
			tputs(tparm(parm_dch, i+1), 1, tc_putchar);
		}
		tu_cup(15,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef delete_line

/* = = = = = = = = = = = = = tc_del_1line = = = = = = = = = = = = = =*/
/*
 * Test delete_line capability.
 */
tc_del_1line(p_opts)

struct option_tab	*p_opts;
{
	int	i,j;

	if(delete_line)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for (i=0; i<5; i++)
		{
			tu_cup(5+i*2,0);
			printf("    line %d --> ", i+1);
			tu_cup(5+i*2,25);
			for (j=0; j<30; j++)
				tc_putchar(teststring[i]);
			tu_cup(6+i*2,0);
			printf("    extra %d --> ", i+1);
			tu_cup(6+i*2,25);
			for (j=0; j<30; j++)
				tc_putchar('#');
		}
		for (i=0; i<5; i++)
		{
			tu_cup(6+i, 0);
			tputs(delete_line, 1, tc_putchar);
		}
		tu_cup(2, 45);
		printf("standard >---v");
		tu_cup(3, 58);
		printf("v");
		for(i=0; i<5; i++)
		{
			tu_cup(5+i, 58);
			tc_putchar(teststring[i]);
		}
		tu_cup(15,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef parm_delete_line

/* = = = = = = = = = = = = = tc_del_line = = = = = = = = = = = = = =*/
/*
 * Test parm_delete_line capability.
 */
tc_del_line(p_opts)

struct option_tab	*p_opts;
{
	int	i, j, k, current;

	if(parm_delete_line)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		current=5;
		for (i=0; i<4; i++)
		{
			tu_cup(current,0);
			printf("    line %d --> ", i+1);
			tu_cup(current++,25);
			for (j=0; j<30; j++)
				tc_putchar(teststring[i]);
			for (k=0; k<i+1; k++)
			{
				tu_cup(current,0);
				printf("    extra %d --> ", i+1);
				tu_cup(current++,25);
					for (j=0; j<30; j++)
						tc_putchar('#');
			}
		}
		for (i=0; i<4; i++)
		{
			tu_cup(6+i, 0);
			tputs(tparm(parm_delete_line, i+1), 1, tc_putchar);
		}
		tu_cup(2, 45);
		printf("standard >---v");
		tu_cup(3, 58);
		printf("v");
		for(i=0; i<4; i++)
		{
			tu_cup(5+i, 58);
			tc_putchar(teststring[i]);
		}
		tu_cup(15,0);
		return 0;
	}
	return 1;
}
#endif
