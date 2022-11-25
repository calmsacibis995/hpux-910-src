/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tp_sel.c,v 70.1 92/03/09 16:14:35 ssa Exp $ */
/*
 * File:		tp_sel.c
 * Creator:		G.Clark.Brown
 *
 * Section selector loop for the Terminfo Exercisers Program.
 *
 * Module name: 	%M%
 * SID:			%I%
 * Extract date:	%H% %T%
 * Delta date:		%G% %U%
 * Stored at:		%P%
 * Module type:		%Y%
 * 
 */

#include	<sys/types.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>

#include	"tixermacro.h"
#include	"tixer.h"
#include	"tixerext.h"

#ifndef lint
	static char	Sccsid[80]="%W% %E% %G%";
#endif

/* = = = = = = = = = = = = = = = selector = = = = = = = = = = = = = = = =*/
/*
 * Choose a test to run.
 */

selector(p_opts)

struct option_tab	*p_opts;

{

	if (p_opts->test<0 && (p_opts->test = tp_menu()) < 0)
		u_term(-1);
		
	switch(p_opts->test)
	{
		case 0:		/* Test one attribute */
			if((p_opts->captab=get_cap())<0)
				break;
			get_term(p_opts->type);

			open_term(p_opts->type);
			tp_one(p_opts);
			close_term();
			break;

		case 1:		/* Short test. */

			get_term(p_opts->type);

			open_term(p_opts->type);
			tp_short(p_opts);
			close_term();
			break;

		case 2:		/* Test all attributes */
			get_term(p_opts->type);

			open_term(p_opts->type);
			tp_all(p_opts);
			close_term();
			break;

		default:
			fprintf(stderr, "Test number %d not yet implemented.\r\n",
			p_opts->test);
	}
}

/* = = = = = = = = = = = = = = = tp_menu = = = = = = = = = = = = = = = =*/
/*
 * Let the user choose a test from the menu.
 */

tp_menu()

{
	char	reply[257];
	int	test;
	
	reply[0]='\0';
	reply[1]='\0';

	while ((test=parse_tp(reply)) < 0)
	{
		printf("\r\n");
		printf("\t1: Test operation of a single capability.\r\n");
		printf("\ts: Short test of attributes and cursor.\r\n");
		printf("\ta: Test all capabilities.\r\n");
		printf("\r\n");
		printf("Enter your choice or q to quit:");
		if (fgets(reply, sizeof(reply), stdin) == NULL ||
			(reply[0] = tolower(reply[0])) == 'q')
			return -1;
	}
	return test;
}

/* = = = = = = = = = = = = = = = get_term = = = = = = = = = = = = = = = =*/
/*
 * Let the user choose a term to test.
 */

get_term(term)
char	*term;
{
	int	len;
	char	def_term[80];
	char	*ptr;

/*
 * Set default TERM value.
 */

	if ( term[0] == '\0' )
	{
		if((ptr=getenv("TERM")) != NULL)
		{
			U_strcpy(def_term, ptr);
			strcpy( term, def_term );
		}
	}
}

/* = = = = = = = = = = = = = = = get_cap = = = = = = = = = = = = = = = =*/
/*
 * Let the user choose a cap to test.
 */

get_cap()

{
	char	cap[80];
	char	*ptr;
	int	len;
	int	index;

	do
	{
		printf("\r\n");
		printf(
		   "Enter name of Terminfo capability to test (or q to quit):");
		
		if (fgets(cap, 80, stdin) == NULL ||
				(tolower(cap[0])=='q' && cap[1]=='\n'))
			u_term(0);
		if ((len=strlen(cap)) < 80)
			cap[len-1]='\0';	/* kill newline */
	}
	while ((index=check_cap(cap)) < 0);
	return index;
}


/* = = = = = = = = = = = = = = = check_cap = = = = = = = = = = = = = = = =*/
/*
 * Check a cap name to see if it is in the Terminfo database.
 */

check_cap(cap)

char	*cap;

{
	int	i;


	if (!cap[0])
		return -1;	/* NULL string entered, reprompt */

	for (i=0; cap_table[i].variable_name; i++)
	{
		if(
		(strlen(cap) == strlen(cap_table[i].capname) &&
			!strcmp(cap, cap_table[i].capname)) ||
		(strlen(cap) == strlen(cap_table[i].icode) &&
			!strcmp(cap, cap_table[i].icode)) ||
		(strlen(cap) == strlen(cap_table[i].variable_name) &&
			!strcmp(cap, cap_table[i].variable_name)))
			return i;
	}

	ERR1("Entry %s not found in Terminfo database.\r\n", cap);
	return -1;
}
