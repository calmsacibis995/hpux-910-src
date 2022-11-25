/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/******************************************************************************
*   (C) Copyright COMPAQ Computer Corporation 1985, 1989
*******************************************************************************
*
*                     inc/compiler.h
*
*     This file contains definitions used by the modules which are involved
*     in parsing cfg files.
*
*******************************************************************************/


/* global variables */
extern unsigned 	lineno;
extern char 		toggles;
extern char 		mask_bits;
extern char 		*strbuf;

/* function prototypes */
extern int 		(*get_char)();
extern int 		yyparse();
extern int 		yylex();

/* emitter.c functions */
extern  void 		*allocate();
extern  void		free_valuelist();
extern  void		beg_board();
extern  void		board_id();
extern  void		board_name();
extern  void		board_mfr();
extern  void		board_cat();
extern  void		end_board_reqd();
extern  void		board_slot();
extern  void		board_length();
extern  void		board_skirt();
extern  void		board_readid();
extern  void		board_bmaster();
extern  void		board_amps();
extern  void		board_iochk();
extern  void		board_disable();
extern  void		board_sizing();
extern  void		board_cmmts();
extern  void		board_help();
extern  void		end_ctrl_reqd();
extern  void		beg_switch();
extern  void		beg_jumper();
extern  void		ctrl_name();
extern  void		switch_type();
extern  void		jumper_type();
extern  void		ctrl_vert();
extern  void		ctrl_rev();
extern  void		beg_label();
extern  void		lbl_pos();
extern  void		lbl_range();
extern  void		jmp_pair();
extern  void		end_label_list();
extern  void		ctrl_label();
extern  void		end_label();
extern  void		beg_loc_list();
extern  void		clear_loc_mask();
extern  void		loc_val();
extern  void		loc_range();
extern  void		loc_pair();
extern  void		end_pair();
extern  void		end_initv_list();
extern  void		sw_initval();
extern  void		jmp_initval();
extern  void		sw_fact();
extern  void		jmp_fact();
extern  void		ctrl_cmmts();
extern  void		prog_port();
extern  void		ioport_size();
extern  void		io_initval();
extern  void		sftware();
extern  void		beg_group();
extern  void		group_type();
extern  void		end_group();
extern  void		beg_func();
extern  void		func_type();
extern  void		func_cmmts();
extern  void		func_help();
extern  void		func_connect();
extern  void		func_display();
extern  void		beg_subfunc();
extern  void		subfunc_type();
extern  void		subfunc_cmmts();
extern  void		subfunc_help();
extern  void		subfunc_connect();
extern  void		beg_choice();
extern  void		choice_type();
extern  void		choice_help();
extern  void		choice_amp();
extern  void		choice_dis();
extern  void		beg_subchoice();
extern  void		beg_resource_group();
extern  unsigned int 	set_index_value();
extern  void		end_resource_group();
extern  void		init_index_value();
extern  void		beg_dma();
extern  void		dma_share();
extern  void		dma_size();
extern  void		dma_timing();
extern  void		beg_irq();
extern  void		irq_share();
extern  void		irq_trig();
extern  void		beg_port();
extern  void		port_range();
extern  void		set_eisaport();
extern  void		port_share();
extern  void		port_size();
extern  void		beg_memory();
extern  void		mem_range();
extern  void		end_mem_vals();
extern  void		addr_range();
extern  void		end_addr_vals();
extern  void		mem_write();
extern  void		mem_type();
extern  void		mem_size();
extern  void		mem_cache();
extern  void		mem_decode();
extern  void		mem_share();
extern  void		add_range();
extern  void		end_list();
extern  void		set_stmt();
extern  void		end_res_stmt();
extern  void		beg_init();
extern  void		end_init_list();
extern  void		init_val();
extern  void		init_range();
extern  void		init_string();
extern  void		end_init_stmt();
extern  void		null_init();
extern  void		set_portvar();
extern  void		beg_totalmem();
extern  void		totalmem_val();
extern  void		totalmem_range();
extern  void		end_func();
extern  void		end_board();
extern  void		beg_sys();
extern  void		sys_nvmem();
extern  void		sys_amps();
extern  void		beg_sys_slot();
extern  void		sys_slot_text();
extern  void		sys_length();
extern  void		sys_skirt();
extern  void		sys_bmaster();
extern  void		sys_label();
extern  void		sys_cache();
extern  void		set_vs_incr();
extern  void		incl_stmt();
extern  int 		slot_compare();
extern  void		end_sys();
extern  void		beg_freeform();
extern  void		freeform_val();
extern  void		end_freeform();


#define MAXLIST 	512		/* max. # of values in a list */
#define MAXSTRBUF	4096		/* max. size of string buffer */
#define MAXFILESIZE	65515		/* max. size of a source file */
#define DATABUFSIZE	8192		/* default size of main data buffer */
#define MAXDATABUF	MAXFILESIZE	/* max. size of main data buffer */
#define MINDATABUF	1024		/* min. size of main data buffer */
