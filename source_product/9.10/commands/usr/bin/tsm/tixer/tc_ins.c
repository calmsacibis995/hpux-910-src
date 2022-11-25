/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_ins.c,v 70.1 92/03/09 16:13:28 ssa Exp $ */
/*
 * File: tc_ins.c
 * Creator: G.Clark.Brown
 *
 * Test routines for character and line inserts in the Terminfo Exerciser
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

#ifdef insert_character

/* = = = = = = = = = = = = = tc_ins_1char = = = = = = = = = = = = = =*/
/*
 * Test insert_character capability.
 */
tc_ins_1char(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	if(insert_character)
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
		printf("    using ic -->");
		tu_cup(6,20);
		for (i=0; i < strlen( teststring ); i+=2)
		{
			tc_putchar(teststring[i]);
		}
		for (i=1; i < strlen( teststring ); i+=2)
		{
			tu_cup(6, i+20);
			tputs(enter_insert_mode, 1, tc_putchar);
			tputs(insert_character, 1, tc_putchar);
			tc_putchar(teststring[i]);
			tputs(exit_insert_mode, 1, tc_putchar);
		}
		tu_cup(15,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef parm_ich

/* = = = = = = = = = = = = = tc_ins_char = = = = = = = = = = = = = =*/
/*
 * Test parm_ich capability.
 */
tc_ins_char(p_opts)

struct option_tab	*p_opts;
{
	int	i,j,k;

	if(parm_ich)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tu_cup(5,0);
		printf("    standard       -->");
		tu_cup(5,25);
		printf("%s", teststring);
		tc_putchar('\r');
		tc_putchar('\n');
		tu_cup(6,0);
		printf("    using parm_ich -->");
		tu_cup(6,25);
		for (j=1, i=0; i<strlen(teststring); j++ && (i+=j))
		{
			tc_putchar(teststring[i]);
		}
		for (j=1, i=0; i<strlen(teststring); ++j && (i+=j))
		{
			tu_cup(6, i+26);
			tputs(tparm(parm_ich, j), 1, tc_putchar);
			for (k=i+1; k<i+j+1 && (k < strlen(teststring)); k++)
				tc_putchar(teststring[k]);
		}
		tu_cup(15,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef insert_line

/* = = = = = = = = = = = = = tc_ins_1line = = = = = = = = = = = = = =*/
/*
 * Test insert_line capability.
 */
tc_ins_1line(p_opts)

struct option_tab	*p_opts;
{
	int	i,j;

	if(insert_line)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for (i=0; i<10; i+=2)
		{
			tu_cup(5+i/2,0);
			printf("    line %d --> ", i+1);
			tu_cup(5+i/2,25);
			for (j=0; j<strlen( teststring ); j++)
				tc_putchar(teststring[i]);
		}
		for (i=1; i<10; i+=2)
		{
			tu_cup(5+i, 0);
			tputs(insert_line, 1, tc_putchar);
			printf("ins line %d --> ", i+1);
			tu_cup(5+i,25);
			for (j=0; j<strlen( teststring ); j++)
				tc_putchar(teststring[i]);
		}
		tu_cup(2, 45);
		printf("standard >---v");
		tu_cup(3, 58);
		printf("v");
		for(i=0; i<10; i++)
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

#ifdef parm_insert_line

/* = = = = = = = = = = = = = tc_ins_line = = = = = = = = = = = = = =*/
/*
 * Test parm_insert_line capability.
 */
tc_ins_line(p_opts)

struct option_tab	*p_opts;
{
	int	i, j, k, m;

	if(parm_insert_line)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		for (i=0, j=1; i<11; ++j && (i+=j))
		{
			tu_cup(4+j,0);
			printf("    line %d --> ", i+1);
			tu_cup(4+j,25);
			for (k=0; k<strlen( teststring ); k++)
				tc_putchar(teststring[i]);
		}
		for (i=0, j=1; i<10; j++ && (i+=j))
		{
			tu_cup(6+i, 0);
			tputs(tparm(parm_insert_line, j), 1, tc_putchar);
			for (k=i+1; k<i+1+j; k++)
			{
				tu_cup(5+k, 0);
				printf("ins line %d --> ", k+1);
				tu_cup(5+k,25);
				for (m=0; m<strlen( teststring ); m++)
					tc_putchar(teststring[k]);
			}
		}
		tu_cup(2, 45);
		printf("standard >---v");
		tu_cup(3, 58);
		printf("v");
		for(i=0; i<14; i++)
		{
			tu_cup(5+i, 58);
			tc_putchar(teststring[i]);
		}
		tu_cup(20,0);
		return 0;
	}
	return 1;
}
#endif

#ifdef enter_insert_mode

/* = = = = = = = = = = = = = tc_ins_mode = = = = = = = = = = = = = =*/
/*
 * Test enter_insert_mode capability.
 */
tc_ins_mode(p_opts)

struct option_tab	*p_opts;
{
	int	i, j, k, current;

	if(enter_insert_mode && !insert_character)
	{
		tu_clear_screen();
		print_cap_title(p_opts);
		tu_cup(5,0);
		printf("    standard          -->");
		tu_cup(5,25);
		printf("%s", teststring);
		tc_putchar('\r');
		tc_putchar('\n');
		tu_cup(6,0);
		printf("    using insert_mode -->");
		tu_cup(6,25);
		for (j=1, i=0; i<strlen(teststring); j++ && (i+=j))
		{
			tc_putchar(teststring[i]);
		}
		for (j=1, i=0; i<strlen(teststring); ++j && (i+=j))
		{
			tu_cup(6, i+26);
			tputs(enter_insert_mode, 1, tc_putchar);
			for (k=i+1; k<i+j+1 && (k < strlen( teststring )); k++)
				tc_putchar(teststring[k]);
			tputs(exit_insert_mode, 1, tc_putchar);
		}
		tu_cup(15,0);
		return 0;
	}
	if (enter_insert_mode)
		printf(
"On this terminal, use the insert_character capability to test insert_mode\r\n.");

	return 1;
}
#endif
