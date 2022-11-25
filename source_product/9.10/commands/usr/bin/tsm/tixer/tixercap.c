/*****************************************************************************
** Copyright (c)        1990 Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: tixercap.c,v 66.1 90/09/20 14:52:30 kb Exp $ */
/*
 * File: tixercap.c
 * Creator: G.Clark.Brown
 *
 * Execution table definition for the Terminfo Exerciser Program.
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

#ifndef lint
	static char	Sccsid[]="%W% %E% %G%";
#endif


extern		tp_none();
extern		tp_key_none();
/*
 * From tc_attr.c...
 */
extern		tc_attr_alt_charset();
extern		tc_attr_blink();
extern		tc_attr_bold();
extern		tc_attr_dim();
extern		tc_attr_protected();
extern		tc_attr_reverse();
extern		tc_attr_secure();
extern		tc_attr_standout();
extern		tc_attr_xmc();
extern		tc_attr_underline();
extern		tc_attr_sgr();
extern		tc_attr_flash();
extern		tc_attr_bell();
/*
 * From tc_del.c...
 */
extern		tc_del_1char();
extern		tc_del_char();
extern		tc_del_1line();
extern		tc_del_line();
/*
 * From tc_ins.c...
 */
extern		tc_ins_1char();
extern		tc_ins_char();
extern		tc_ins_1line();
extern		tc_ins_line();
extern		tc_ins_mode();
/*
 * From tc_curs.c...
 */
extern		tc_cup();
extern		tc_cub1();
extern		tc_cuf1();
extern		tc_cuu1();
extern		tc_cud1();
extern		tc_cub();
extern		tc_cuf();
extern		tc_cuu();
extern		tc_cud();
extern		tc_scrc();
extern		tc_vpa();
extern		tc_hpa();
/*
 * From tc_star.c...
 */
extern		tc_home();
extern		tc_ll();
extern		tc_cr();
extern		tc_nel();
extern		tc_cols();
extern		tc_lines();
extern		tc_xenl();
/*
 * From tc_status.c...
 */
extern		tc_tsl();
/*
 * From tc_clear.c...
 */
extern		tc_clear();
extern		tc_el();
extern		tc_ed();
extern		tc_ech();
/*
 * From tc_tab.c...
 */
extern		tc_ht();
extern		tc_it();
extern		tc_bt();
/*
 * From tc_cursor.c...
 */
extern		tc_cnorm();
/*
 * From tc_init.c...
 */
extern		tc_init();
extern		tc_reset();
/*
 * From tc_scroll.c...
 */
extern		tc_csr();
extern		tc_ind();
extern		tc_ri();
extern		tc_indn();
extern		tc_rin();
extern		tc_meml();


/* = = = = = = = = = = = = = = = cap_table = = = = = = = = = = = = = = = =*/
/*
 * Define the structure "cap_table" and fill it with data.  This table is used
 * as an execution table by the rest of the program. The data comes from
 * tci.c.
 */

struct	cap_table_type	cap_table[TIXC_MAXCAPS]={

#include	"tci.h"

	};

