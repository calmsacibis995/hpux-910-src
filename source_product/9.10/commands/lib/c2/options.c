/* @(#) $Revision: 70.1 $ */     
#include "o68.h"

char *true_false();

static struct opt_info {
	char opt_char;
	boolean opt_default_value;
	boolean *opt_flag;
	char *message;
	} opt_info[] = {

	'a',	true,	&simplify_addressing,	"simplify addressing",
	'b',	true,	&bit_test_ok,		"bit test generation",
	'd',	false,	&debug_is,		"debug inst sched",
	'e',	false,	&exit_exits,		"assume exit doesn't return",
	'f',	true,	&flow_opts_ok,		"flow optimizations",
	'i',	true,	&inst_sched,		"do instruction scheduling",
	'l',	true,	&eliminate_link_unlk,   "remove link/unlk",
	'm',	true,	&memory_is_ok,		"memory can be trusted",
	'o',	true,	&code_movement_ok,	"move code around",
	'p',	false,	&pass_unchanged,	"disable all optimizations",
	'r',	true,	&leaf_movm_reduce,      "replace perm regs by scratch",
	's',	false,	&print_statistics,	"print optimization statistics",
/* remove -t later ?? */
	't',	true,	&tail_881,              "tail_881_optimization",
/* remove -u later ?? */
	'u',	true,	&ult_881,               "ultimate_881_destination",
	'v',	true,	&invert_loops,		"invert loops",
	'x',	true,	&oforty,		"generate code for 68040",
/* remove -y later ?? */
	'y',	true,	&ult_dragon,            "ultimate_dragon_destination",
	'z',	false,	&data_to_bss,		"move zero data to bss",
	'A',	false,  &allow_asm,		"optimize procedures with asm",
	'B',	false,  &dragon_and_881,	"dragon/881 source from +b",
/* remove -C later ?? */
	'C',	true,	&combine_dragon,        "combine_dragon_optimization",
	'D',	false,  &dragon,		"dragon source from +f",
	'F',	false,  &fort,			"fortran source",
	'S',	true,	&span_dep_opt,		"attempt span dependent opts",
/* remove -T later ?? */
	'T',	true,	&tail_dragon,           "tail_dragon_optimization",
	'U',	true,	&unsafe_opts,		"attempt unsafe opts",

#ifdef PIC
	'q',	false,	&pic,			"generate pic",
	'Q',	false,	&pic,			"generate pic",
#endif

#ifdef DEBUG
	'w',	false,	&worthless_is,		"do is on worthless blocks",
	'j',	false,	&junk_is,		"junk all opts except is",
	'0',	false,	&debug,			"general debug flag",
	'1',	false,	&debug1,		"debug1: output routines",
	'2',	false,	&debug2,		"debug2: input routines",
	'3',	false,	&debug3,		"debug3: equ processing",
	'4',	false,	&debug4,		"debug4: cache processing",
	'5',	false,	&debug5,		"debug5: check allocation",
	'6',	false,	&debug6,		"debug6: verbose allocation",
	'7',	false,	&debug7,		"debug flag 7", 
	'8',	false,	&debug8,		"debug8: buffer output",
	'9',	false,	&debug9,		"debug9: status in main loop",
#endif DEBUG

	'\0',	false,	NULL,			"ERROR: dummy last value"
	};

set_default_options()
{
	register struct opt_info *p;

	for (p=opt_info; p->opt_char!='\0'; p++)
		*p->opt_flag = p->opt_default_value;
}


toggle_option(c)
register char c;
{
	register struct opt_info *p;

#ifdef DEBUG
	/* If a debugging flag, set no buffering */
	/* Except for debug8, which specifies buffering */
	if (isdigit(c) && c!='8' && !debug8)
		setbuf(stdout, (char *) NULL);
#endif DEBUG

	for (p=opt_info; p->opt_char!='\0'; p++) {
		if (c==p->opt_char) {
			*p->opt_flag = ! *p->opt_flag;
#ifdef DEBUG
			bugout("%s: %s", p->message, true_false(*p->opt_flag));
#endif DEBUG
			return;
		}
	}

	fprintf(stderr, "Invalid option '%c'\n", c);

	/*
	 * He made a mistake and boy, will he regret it.
	 * Print out the entire list of options and exit!
	 * Ha ha ha ha ha ha ha ha ha ha ha ha ha ha ha ha ha ha!
	 */
	fprintf(stderr, "\n*** Optimizer options ***\n");
	for (p=opt_info; p->opt_char!='\0'; p++)
		fprintf(stderr, "-%c: (default %s)\t%s\n",
			p->opt_char, true_false(p->opt_default_value),
			p->message);
	exit(2);
}
