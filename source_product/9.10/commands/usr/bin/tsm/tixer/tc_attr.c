/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tc_attr.c,v 70.1 92/03/09 16:12:54 ssa Exp $ */
/*
 * File: tc_attr.c
 * Creator: G.Clark.Brown
 *
 * Test routines for character attributes in the Terminfo Exerciser Program.
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

static char	teststring[]=
			"ABCDEFGHIJabcdefghij0123456789)!@#$%^&*(<,./;'[]-=";

#ifdef enter_alt_charset_mode
#ifdef exit_alt_charset_mode

/* = = = = = = = = = = = = = tc_attr_alt_charset = = = = = = = = = = = = = =*/
/*
 * Test alt_charset attribute.
 */
tc_attr_alt_charset(p_opts)

struct option_tab	*p_opts;
{
	if(enter_alt_charset_mode && exit_alt_charset_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    alt_charset -->");
		tputs(enter_alt_charset_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_alt_charset_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif
#endif


#ifdef enter_blink_mode

/* = = = = = = = = = = = = = tc_attr_blink = = = = = = = = = = = = = =*/
/*
 * Test blink attribute.
 */
tc_attr_blink(p_opts)

struct option_tab	*p_opts;
{
	if(enter_blink_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    blink       -->");
		tputs(enter_blink_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_attribute_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif


#ifdef enter_bold_mode

/* = = = = = = = = = = = = = tc_attr_bold = = = = = = = = = = = = = =*/
/*
 * Test bold attribute.
 */
tc_attr_bold(p_opts)

struct option_tab	*p_opts;
{
	if(enter_bold_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    bold        -->");
		tputs(enter_bold_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_attribute_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif



#ifdef enter_dim_mode


/* = = = = = = = = = = = = = tc_attr_dim = = = = = = = = = = = = = =*/
/*
 * Test dim attribute.
 */
tc_attr_dim(p_opts)

struct option_tab	*p_opts;
{
	if(enter_dim_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    dim         -->");
		tputs(enter_dim_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_attribute_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif


#ifdef enter_protected_mode

/* = = = = = = = = = = = = = tc_attr_protected = = = = = = = = = = = = = =*/
/*
 * Test protected attribute.
 */
tc_attr_protected(p_opts)

struct option_tab	*p_opts;
{
	if(enter_protected_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    protected   -->");
		tputs(enter_protected_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_attribute_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif



#ifdef enter_reverse_mode

/* = = = = = = = = = = = = = tc_attr_reverse = = = = = = = = = = = = = =*/
/*
 * Test reverse attribute.
 */
tc_attr_reverse(p_opts)

struct option_tab	*p_opts;
{
	if(enter_reverse_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    reverse     -->");
		tputs(enter_reverse_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_attribute_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif



#ifdef enter_secure_mode

/* = = = = = = = = = = = = = tc_attr_secure = = = = = = = = = = = = = =*/
/*
 * Test secure attribute.
 */
tc_attr_secure(p_opts)

struct option_tab	*p_opts;
{
	if(enter_secure_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    secure      -->");
		tputs(enter_secure_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_attribute_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif



#ifdef enter_standout_mode
#ifdef exit_standout_mode

/* = = = = = = = = = = = = = tc_attr_standout = = = = = = = = = = = = = =*/
/*
 * Test standout attribute.
 */
tc_attr_standout(p_opts)

struct option_tab	*p_opts;
{
	if(enter_standout_mode && exit_standout_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    standout    -->");
		tputs(enter_standout_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_standout_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		tc_attr_note(p_opts);
		return 0;
	}
	return 1;
}

/* = = = = = = = = = = = = = tc_attr_xmc = = = = = = = = = = = = = =*/
/*
 * Test xmc attribute.
 */
tc_attr_xmc(p_opts)

struct option_tab	*p_opts;
{
	if(enter_standout_mode && exit_standout_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    standout    -->");
		tputs(enter_standout_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_standout_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		tc_attr_note(p_opts);
		return 0;
	}
	else if (magic_cookie_glitch != -1)
	{
		printf( "    Cannot test because standout is not defined.");
		return 0;
	}
	return 1;
}
#endif
#endif



#ifdef enter_underline_mode
#ifdef exit_underline_mode

/* = = = = = = = = = = = = = tc_attr_underline = = = = = = = = = = = = = =*/
/*
 * Test underline attribute.
 */
tc_attr_underline(p_opts)

struct option_tab	*p_opts;
{
	if(enter_underline_mode && exit_underline_mode)
	{
		print_cap_title(p_opts);
		tc_attr_normal(p_opts);
		printf("    underline   -->");
		tputs(enter_underline_mode, 1, tc_putchar);
		printf(teststring);
		tputs(exit_underline_mode, 1, tc_putchar);
		tc_putchar('\r');
		tc_putchar('\n');
		tc_attr_normal(p_opts);
		return 0;
	}
	return 1;
}
#endif
#endif


#ifdef set_attributes

/* = = = = = = = = = = = = = tc_attr_sgr = = = = = = = = = = = = = =*/
/*
 * Test sgr attribute.
 */
tc_attr_sgr(p_opts)

struct option_tab	*p_opts;
{
	if(set_attributes && exit_attribute_mode)
	{
		print_cap_title(p_opts);
#ifdef enter_alt_charset_mode
		if(enter_alt_charset_mode)
		{
			tc_attr_normal(p_opts);
			printf("    alt_charset -->");
			tputs(tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 0, 1),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_blink_mode
		if(enter_blink_mode)
		{
			tc_attr_normal(p_opts);
			printf("    blink       -->");
			tputs(tparm(set_attributes, 0, 0, 0, 1, 0, 0, 0, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_bold_mode
		if(enter_bold_mode)
		{
			tc_attr_normal(p_opts);
			printf("    bold        -->");
			tputs(tparm(set_attributes, 0, 0, 0, 0, 0, 1, 0, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_dim_mode
		if(enter_dim_mode)
		{
			tc_attr_normal(p_opts);
			printf("    dim         -->");
			tputs(tparm(set_attributes, 0, 0, 0, 0, 1, 0, 0, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_protected_mode
		if(enter_protected_mode)
		{
			tc_attr_normal(p_opts);
			printf("    protected   -->");
			tputs(tparm(set_attributes, 0, 0, 0, 0, 0, 0, 0, 1, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_reverse_mode
		if(enter_reverse_mode)
		{
			tc_attr_normal(p_opts);
			printf("    reverse     -->");
			tputs(tparm(set_attributes, 0, 0, 1, 0, 0, 0, 0, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif 
#ifdef enter_secure_mode
		if(enter_secure_mode)
		{
			tc_attr_normal(p_opts);
			printf("    secure      -->");
			tputs(tparm(set_attributes, 0, 0, 0, 0, 0, 0, 1, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_standout_mode
		if(enter_standout_mode)
		{
			tc_attr_normal(p_opts);
			printf("    standout    -->");
			tputs(tparm(set_attributes, 1, 0, 0, 0, 0, 0, 0, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
#ifdef enter_underline_mode
		if(enter_underline_mode)
		{
			tc_attr_normal(p_opts);
			printf("    underline   -->");
			tputs(tparm(set_attributes, 0, 1, 0, 0, 0, 0, 0, 0, 0),
				1, tc_putchar);
			printf(teststring);
			tputs(exit_attribute_mode, 1, tc_putchar);
			tc_putchar('\r');
			tc_putchar('\n');
		}
#endif
		tc_attr_normal(p_opts);
		tc_attr_note(p_opts);
		return 0;
	}
	return 1;
}
#endif

#ifdef flash_screen

/* = = = = = = = = = = = = = tc_attr_flash = = = = = = = = = = = = = =*/
/*
 * Test flash attribute.
 */
tc_attr_flash(p_opts)

struct option_tab	*p_opts;
{
	if(flash_screen)
	{
		print_cap_title(p_opts);
		tputs(flash_screen, 1, tc_putchar);
		return 0;
	}
	return 1;
}
#endif


#ifdef bell

/* = = = = = = = = = = = = = tc_attr_bell = = = = = = = = = = = = = =*/
/*
 * Test bell attribute.
 */
tc_attr_bell(p_opts)

struct option_tab	*p_opts;
{
	if(bell)
	{
		print_cap_title(p_opts);
		tputs(bell, 1, tc_putchar);
		return 0;
	}
	return 1;
}
#endif

#ifdef magic_cookie_glitch

/* = = = = = = = = = = = = = tc_attr_normal = = = = = = = = = = = = = =*/
/*
 * Write out the "normal" teststring to be used for comparison.
 */
tc_attr_normal(p_opts)

struct option_tab	*p_opts;
{
	int	i;

	printf("    normal      -->");
	for (i=0; i<magic_cookie_glitch; i++)
		tc_putchar(' ');

	printf(teststring);
	tc_putchar('\r');
	tc_putchar('\n');

	return;
}

/* = = = = = = = = = = = = = tc_attr_note = = = = = = = = = = = = = =*/
/*
 * Write out the message about magic cookies.
 */
tc_attr_note(p_opts)

struct option_tab	*p_opts;
{
	if (magic_cookie_glitch == -1)
		printf(
"     The magic_cookie_glitch is not defined for this terminal.\r\n     The lines of text should line up vertically.\r\n");
	else
		printf(
"    The magic_cookie value is %d.\r\n     If this is correct, the lines of text should line up vertically.\r\n",
			magic_cookie_glitch);
	return;
}
#else
/* = = = = = = = = = = = = = tc_attr_normal = = = = = = = = = = = = = =*/
/*
 * Write out the "normal" teststring to be used for comparison.
 * This version does not use magic cookies.
 */
tc_attr_normal(p_opts)

struct option_tab	*p_opts;
{
	printf("    normal      -->");
	printf(teststring);
	tc_putchar('\r');
	tc_putchar('\n');

	return;
}

/* = = = = = = = = = = = = = tc_attr_note = = = = = = = = = = = = = =*/
/*
 * Write out the message about magic cookies.
 * This version is on a machine that has no magic cookies.
 */
tc_attr_note(p_opts)

struct option_tab	*p_opts;
{
	return;
}
#endif


