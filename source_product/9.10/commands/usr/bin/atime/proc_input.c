#include "fizz.h"

static struct instruction {
    char *name;
    short type;
} ilist[] = {
{"align",	I_PSOP},
{"asciz",	I_PSOP},
{"assert",	I_ASSERT},
{"assert.b",	I_ASSERT_B},
{"assert.l",	I_ASSERT_L},
{"assert.w",	I_ASSERT_W},
{"bcc",		I_BRANCH},
{"bcc.b",	I_BRANCH},
{"bcc.l",	I_BRANCH},
{"bcc.w",	I_BRANCH},
{"bcs",		I_BRANCH},
{"bcs.b",	I_BRANCH},
{"bcs.l",	I_BRANCH},
{"bcs.w",	I_BRANCH},
{"beq",		I_BRANCH},
{"beq.b",	I_BRANCH},
{"beq.l",	I_BRANCH},
{"beq.w",	I_BRANCH},
{"bge",		I_BRANCH},
{"bge.b",	I_BRANCH},
{"bge.l",	I_BRANCH},
{"bge.w",	I_BRANCH},
{"bgt",		I_BRANCH},
{"bgt.b",	I_BRANCH},
{"bgt.l",	I_BRANCH},
{"bgt.w",	I_BRANCH},
{"bhi",		I_BRANCH},
{"bhi.b",	I_BRANCH},
{"bhi.l",	I_BRANCH},
{"bhi.w",	I_BRANCH},
{"bhs",		I_BRANCH},
{"bhs.b",	I_BRANCH},
{"bhs.l",	I_BRANCH},
{"bhs.w",	I_BRANCH},
{"ble",		I_BRANCH},
{"ble.b",	I_BRANCH},
{"ble.l",	I_BRANCH},
{"ble.w",	I_BRANCH},
{"blo",		I_BRANCH},
{"blo.b",	I_BRANCH},
{"blo.l",	I_BRANCH},
{"blo.w",	I_BRANCH},
{"bls",		I_BRANCH},
{"bls.b",	I_BRANCH},
{"bls.l",	I_BRANCH},
{"bls.w",	I_BRANCH},
{"blt",		I_BRANCH},
{"blt.b",	I_BRANCH},
{"blt.l",	I_BRANCH},
{"blt.w",	I_BRANCH},
{"bmi",		I_BRANCH},
{"bmi.b",	I_BRANCH},
{"bmi.l",	I_BRANCH},
{"bmi.w",	I_BRANCH},
{"bne",		I_BRANCH},
{"bne.b",	I_BRANCH},
{"bne.l",	I_BRANCH},
{"bne.w",	I_BRANCH},
{"bpl",		I_BRANCH},
{"bpl.b",	I_BRANCH},
{"bpl.l",	I_BRANCH},
{"bpl.w",	I_BRANCH},
{"bra",		I_BRANCH},
{"bra.b",	I_BRANCH},
{"bra.l",	I_BRANCH},
{"bra.w",	I_BRANCH},
{"bsr",		I_BSR},
{"bsr.b",	I_BSR},
{"bsr.l",	I_BSR},
{"bsr.w",	I_BSR},
{"bss",		I_BRANCH},
{"bvc",		I_BRANCH},
{"bvc.b",	I_BRANCH},
{"bvc.l",	I_BRANCH},
{"bvc.w",	I_BRANCH},
{"bvs",		I_BRANCH},
{"bvs.b",	I_BRANCH},
{"bvs.l",	I_BRANCH},
{"bvs.w",	I_BRANCH},
{"byte",	I_PSOP},
{"code",	I_CODE},
{"comm",	I_PSOP},
{"comment",	I_COMMENT},
{"data",	I_PSOP},
{"dataname",	I_DATANAME},
{"dataset",	I_DATASET},
{"dbcc",	I_DBCC},
{"dbcc.w",	I_DBCC},
{"dbcs",	I_DBCC},
{"dbcs.w",	I_DBCC},
{"dbeq",	I_DBCC},
{"dbeq.w",	I_DBCC},
{"dbf",		I_DBCC},
{"dbf.w",	I_DBCC},
{"dbge",	I_DBCC},
{"dbge.w",	I_DBCC},
{"dbgt",	I_DBCC},
{"dbgt.w",	I_DBCC},
{"dbhi",	I_DBCC},
{"dbhi.w",	I_DBCC},
{"dbhs",	I_DBCC},
{"dbhs.w",	I_DBCC},
{"dble",	I_DBCC},
{"dble.w",	I_DBCC},
{"dblo",	I_DBCC},
{"dblo.w",	I_DBCC},
{"dbls",	I_DBCC},
{"dbls.w",	I_DBCC},
{"dblt",	I_DBCC},
{"dblt.w",	I_DBCC},
{"dbmi",	I_DBCC},
{"dbmi.w",	I_DBCC},
{"dbne",	I_DBCC},
{"dbne.w",	I_DBCC},
{"dbpl",	I_DBCC},
{"dbpl.w",	I_DBCC},
{"dbr",		I_DBCC},
{"dbr.w",	I_DBCC},
{"dbra",	I_DBCC},
{"dbra.w",	I_DBCC},
{"dbt",		I_DBCC},
{"dbt.w",	I_DBCC},
{"dbvc",	I_DBCC},
{"dbvc.w",	I_DBCC},
{"dbvs",	I_DBCC},
{"dbvs.w",	I_DBCC},
{"dnt_array",	I_PSOP},
{"dnt_begin",	I_PSOP},
{"dnt_const",	I_PSOP},
{"dnt_dvar",	I_PSOP},
{"dnt_end",	I_PSOP},
{"dnt_entry",	I_PSOP},
{"dnt_enum",	I_PSOP},
{"dnt_field",	I_PSOP},
{"dnt_file",	I_PSOP},
{"dnt_fparam",	I_PSOP},
{"dnt_function",I_PSOP},
{"dnt_functype",I_PSOP},
{"dnt_import",	I_PSOP},
{"dnt_label",	I_PSOP},
{"dnt_memenum",	I_PSOP},
{"dnt_module",	I_PSOP},
{"dnt_pointer",	I_PSOP},
{"dnt_set",	I_PSOP},
{"dnt_srcfile",	I_PSOP},
{"dnt_struct",	I_PSOP},
{"dnt_subrange",I_PSOP},
{"dnt_svar",	I_PSOP},
{"dnt_tagdef",	I_PSOP},
{"dnt_typedef",	I_PSOP},
{"dnt_union",	I_PSOP},
{"dnt_variant",	I_PSOP},
{"dntt",	I_PSOP},
{"double",	I_PSOP},
{"extend",	I_PSOP},
{"fbeq",	I_BRANCH},
{"fbeq.l",	I_BRANCH},
{"fbeq.w",	I_BRANCH},
{"fbf",		I_BRANCH},
{"fbf.l",	I_BRANCH},
{"fbf.w",	I_BRANCH},
{"fbge",	I_BRANCH},
{"fbge.l",	I_BRANCH},
{"fbge.w",	I_BRANCH},
{"fbgl",	I_BRANCH},
{"fbgl.l",	I_BRANCH},
{"fbgl.w",	I_BRANCH},
{"fbgle",	I_BRANCH},
{"fbgle.l",	I_BRANCH},
{"fbgle.w",	I_BRANCH},
{"fbgt",	I_BRANCH},
{"fbgt.l",	I_BRANCH},
{"fbgt.w",	I_BRANCH},
{"fble",	I_BRANCH},
{"fble.l",	I_BRANCH},
{"fble.w",	I_BRANCH},
{"fblt",	I_BRANCH},
{"fblt.l",	I_BRANCH},
{"fblt.w",	I_BRANCH},
{"fbne",	I_BRANCH},
{"fbne.l",	I_BRANCH},
{"fbne.w",	I_BRANCH},
{"fbneq",	I_BRANCH},
{"fbneq.l",	I_BRANCH},
{"fbneq.w",	I_BRANCH},
{"fbnge",	I_BRANCH},
{"fbnge.l",	I_BRANCH},
{"fbnge.w",	I_BRANCH},
{"fbngl",	I_BRANCH},
{"fbngl.l",	I_BRANCH},
{"fbngl.w",	I_BRANCH},
{"fbngle",	I_BRANCH},
{"fbngle.l",	I_BRANCH},
{"fbngle.w",	I_BRANCH},
{"fbngt",	I_BRANCH},
{"fbngt.l",	I_BRANCH},
{"fbngt.w",	I_BRANCH},
{"fbnle",	I_BRANCH},
{"fbnle.l",	I_BRANCH},
{"fbnle.w",	I_BRANCH},
{"fbnlt",	I_BRANCH},
{"fbnlt.l",	I_BRANCH},
{"fbnlt.w",	I_BRANCH},
{"fboge",	I_BRANCH},
{"fboge.l",	I_BRANCH},
{"fboge.w",	I_BRANCH},
{"fbogl",	I_BRANCH},
{"fbogl.l",	I_BRANCH},
{"fbogl.w",	I_BRANCH},
{"fbogt",	I_BRANCH},
{"fbogt.l",	I_BRANCH},
{"fbogt.w",	I_BRANCH},
{"fbole",	I_BRANCH},
{"fbole.l",	I_BRANCH},
{"fbole.w",	I_BRANCH},
{"fbolt",	I_BRANCH},
{"fbolt.l",	I_BRANCH},
{"fbolt.w",	I_BRANCH},
{"fbor",	I_BRANCH},
{"fbor.l",	I_BRANCH},
{"fbor.w",	I_BRANCH},
{"fbr",		I_BRANCH},
{"fbr.l",	I_BRANCH},
{"fbr.w",	I_BRANCH},
{"fbra",	I_BRANCH},
{"fbra.l",	I_BRANCH},
{"fbra.w",	I_BRANCH},
{"fbseq",	I_BRANCH},
{"fbseq.l",	I_BRANCH},
{"fbseq.w",	I_BRANCH},
{"fbsf",	I_BRANCH},
{"fbsf.l",	I_BRANCH},
{"fbsf.w",	I_BRANCH},
{"fbsne",	I_BRANCH},
{"fbsne.l",	I_BRANCH},
{"fbsne.w",	I_BRANCH},
{"fbsneq",	I_BRANCH},
{"fbsneq.l",	I_BRANCH},
{"fbsneq.w",	I_BRANCH},
{"fbst",	I_BRANCH},
{"fbst.l",	I_BRANCH},
{"fbst.w",	I_BRANCH},
{"fbt",		I_BRANCH},
{"fbt.l",	I_BRANCH},
{"fbt.w",	I_BRANCH},
{"fbueq",	I_BRANCH},
{"fbueq.l",	I_BRANCH},
{"fbueq.w",	I_BRANCH},
{"fbuge",	I_BRANCH},
{"fbuge.l",	I_BRANCH},
{"fbuge.w",	I_BRANCH},
{"fbugt",	I_BRANCH},
{"fbugt.l",	I_BRANCH},
{"fbugt.w",	I_BRANCH},
{"fbule",	I_BRANCH},
{"fbule.l",	I_BRANCH},
{"fbule.w",	I_BRANCH},
{"fbult",	I_BRANCH},
{"fbult.l",	I_BRANCH},
{"fbult.w",	I_BRANCH},
{"fbun",	I_BRANCH},
{"fbun.l",	I_BRANCH},
{"fbun.w",	I_BRANCH},
{"fdbeq",	I_DBCC},
{"fdbf",	I_DBCC},
{"fdbge",	I_DBCC},
{"fdbgl",	I_DBCC},
{"fdbgle",	I_DBCC},
{"fdbgt",	I_DBCC},
{"fdble",	I_DBCC},
{"fdblt",	I_DBCC},
{"fdbne",	I_DBCC},
{"fdbneq",	I_DBCC},
{"fdbnge",	I_DBCC},
{"fdbngl",	I_DBCC},
{"fdbngle",	I_DBCC},
{"fdbngt",	I_DBCC},
{"fdbnle",	I_DBCC},
{"fdbnlt",	I_DBCC},
{"fdboge",	I_DBCC},
{"fdbogl",	I_DBCC},
{"fdbogt",	I_DBCC},
{"fdbole",	I_DBCC},
{"fdbolt",	I_DBCC},
{"fdbor",	I_DBCC},
{"fdbr",	I_DBCC},
{"fdbra",	I_DBCC},
{"fdbseq",	I_DBCC},
{"fdbsf",	I_DBCC},
{"fdbsne",	I_DBCC},
{"fdbsneq",	I_DBCC},
{"fdbst",	I_DBCC},
{"fdbt",	I_DBCC},
{"fdbueq",	I_DBCC},
{"fdbuge",	I_DBCC},
{"fdbugt",	I_DBCC},
{"fdbule",	I_DBCC},
{"fdbult",	I_DBCC},
{"fdbun",	I_DBCC},
{"float",	I_PSOP},
{"fnop",	I_EXECUTABLE},
{"fpareg",	I_PSOP},
{"fpbeq",	I_BRANCH},
{"fpbeq.b",	I_BRANCH},
{"fpbeq.l",	I_BRANCH},
{"fpbeq.w",	I_BRANCH},
{"fpbf",	I_BRANCH},
{"fpbf.b",	I_BRANCH},
{"fpbf.l",	I_BRANCH},
{"fpbf.w",	I_BRANCH},
{"fpbge",	I_BRANCH},
{"fpbge.b",	I_BRANCH},
{"fpbge.l",	I_BRANCH},
{"fpbge.w",	I_BRANCH},
{"fpbgl",	I_BRANCH},
{"fpbgl.b",	I_BRANCH},
{"fpbgl.l",	I_BRANCH},
{"fpbgl.w",	I_BRANCH},
{"fpbgle",	I_BRANCH},
{"fpbgle.b",	I_BRANCH},
{"fpbgle.l",	I_BRANCH},
{"fpbgle.w",	I_BRANCH},
{"fpbgt",	I_BRANCH},
{"fpbgt.b",	I_BRANCH},
{"fpbgt.l",	I_BRANCH},
{"fpbgt.w",	I_BRANCH},
{"fpble",	I_BRANCH},
{"fpble.b",	I_BRANCH},
{"fpble.l",	I_BRANCH},
{"fpble.w",	I_BRANCH},
{"fpblt",	I_BRANCH},
{"fpblt.b",	I_BRANCH},
{"fpblt.l",	I_BRANCH},
{"fpblt.w",	I_BRANCH},
{"fpbne",	I_BRANCH},
{"fpbne.b",	I_BRANCH},
{"fpbne.l",	I_BRANCH},
{"fpbne.w",	I_BRANCH},
{"fpbneq",	I_BRANCH},
{"fpbneq.b",	I_BRANCH},
{"fpbneq.l",	I_BRANCH},
{"fpbneq.w",	I_BRANCH},
{"fpbnge",	I_BRANCH},
{"fpbnge.b",	I_BRANCH},
{"fpbnge.l",	I_BRANCH},
{"fpbnge.w",	I_BRANCH},
{"fpbngl",	I_BRANCH},
{"fpbngl.b",	I_BRANCH},
{"fpbngl.l",	I_BRANCH},
{"fpbngl.w",	I_BRANCH},
{"fpbngle",	I_BRANCH},
{"fpbngle.b",	I_BRANCH},
{"fpbngle.l",	I_BRANCH},
{"fpbngle.w",	I_BRANCH},
{"fpbngt",	I_BRANCH},
{"fpbngt.b",	I_BRANCH},
{"fpbngt.l",	I_BRANCH},
{"fpbngt.w",	I_BRANCH},
{"fpbnle",	I_BRANCH},
{"fpbnle.b",	I_BRANCH},
{"fpbnle.l",	I_BRANCH},
{"fpbnle.w",	I_BRANCH},
{"fpbnlt",	I_BRANCH},
{"fpbnlt.b",	I_BRANCH},
{"fpbnlt.l",	I_BRANCH},
{"fpbnlt.w",	I_BRANCH},
{"fpboge",	I_BRANCH},
{"fpboge.b",	I_BRANCH},
{"fpboge.l",	I_BRANCH},
{"fpboge.w",	I_BRANCH},
{"fpbogl",	I_BRANCH},
{"fpbogl.b",	I_BRANCH},
{"fpbogl.l",	I_BRANCH},
{"fpbogl.w",	I_BRANCH},
{"fpbogt",	I_BRANCH},
{"fpbogt.b",	I_BRANCH},
{"fpbogt.l",	I_BRANCH},
{"fpbogt.w",	I_BRANCH},
{"fpbole",	I_BRANCH},
{"fpbole.b",	I_BRANCH},
{"fpbole.l",	I_BRANCH},
{"fpbole.w",	I_BRANCH},
{"fpbolt",	I_BRANCH},
{"fpbolt.b",	I_BRANCH},
{"fpbolt.l",	I_BRANCH},
{"fpbolt.w",	I_BRANCH},
{"fpbor",	I_BRANCH},
{"fpbor.b",	I_BRANCH},
{"fpbor.l",	I_BRANCH},
{"fpbor.w",	I_BRANCH},
{"fpbseq",	I_BRANCH},
{"fpbseq.b",	I_BRANCH},
{"fpbseq.l",	I_BRANCH},
{"fpbseq.w",	I_BRANCH},
{"fpbsf",	I_BRANCH},
{"fpbsf.b",	I_BRANCH},
{"fpbsf.l",	I_BRANCH},
{"fpbsf.w",	I_BRANCH},
{"fpbsne",	I_BRANCH},
{"fpbsne.b",	I_BRANCH},
{"fpbsne.l",	I_BRANCH},
{"fpbsne.w",	I_BRANCH},
{"fpbsneq",	I_BRANCH},
{"fpbsneq.b",	I_BRANCH},
{"fpbsneq.l",	I_BRANCH},
{"fpbsneq.w",	I_BRANCH},
{"fpbst",	I_BRANCH},
{"fpbst.b",	I_BRANCH},
{"fpbst.l",	I_BRANCH},
{"fpbst.w",	I_BRANCH},
{"fpbt",	I_BRANCH},
{"fpbt.b",	I_BRANCH},
{"fpbt.l",	I_BRANCH},
{"fpbt.w",	I_BRANCH},
{"fpbueq",	I_BRANCH},
{"fpbueq.b",	I_BRANCH},
{"fpbueq.l",	I_BRANCH},
{"fpbueq.w",	I_BRANCH},
{"fpbuge",	I_BRANCH},
{"fpbuge.b",	I_BRANCH},
{"fpbuge.l",	I_BRANCH},
{"fpbuge.w",	I_BRANCH},
{"fpbugt",	I_BRANCH},
{"fpbugt.b",	I_BRANCH},
{"fpbugt.l",	I_BRANCH},
{"fpbugt.w",	I_BRANCH},
{"fpbule",	I_BRANCH},
{"fpbule.b",	I_BRANCH},
{"fpbule.l",	I_BRANCH},
{"fpbule.w",	I_BRANCH},
{"fpbult",	I_BRANCH},
{"fpbult.b",	I_BRANCH},
{"fpbult.l",	I_BRANCH},
{"fpbult.w",	I_BRANCH},
{"fpbun",	I_BRANCH},
{"fpbun.b",	I_BRANCH},
{"fpbun.l",	I_BRANCH},
{"fpbun.w",	I_BRANCH},
{"fpid",	I_PSOP},
{"fpmode",	I_PSOP},
{"fpwait",	I_PSOP},
{"fteq",	I_EXECUTABLE},
{"ftf",		I_EXECUTABLE},
{"ftge",	I_EXECUTABLE},
{"ftgl",	I_EXECUTABLE},
{"ftgle",	I_EXECUTABLE},
{"ftgt",	I_EXECUTABLE},
{"ftle",	I_EXECUTABLE},
{"ftlt",	I_EXECUTABLE},
{"ftne",	I_EXECUTABLE},
{"ftneq",	I_EXECUTABLE},
{"ftnge",	I_EXECUTABLE},
{"ftngl",	I_EXECUTABLE},
{"ftngle",	I_EXECUTABLE},
{"ftngt",	I_EXECUTABLE},
{"ftnle",	I_EXECUTABLE},
{"ftnlt",	I_EXECUTABLE},
{"ftoge",	I_EXECUTABLE},
{"ftogl",	I_EXECUTABLE},
{"ftogt",	I_EXECUTABLE},
{"ftole",	I_EXECUTABLE},
{"ftolt",	I_EXECUTABLE},
{"ftor",	I_EXECUTABLE},
{"ftseq",	I_EXECUTABLE},
{"ftsf",	I_EXECUTABLE},
{"ftsne",	I_EXECUTABLE},
{"ftsneq",	I_EXECUTABLE},
{"ftst",	I_EXECUTABLE},
{"ftt",		I_EXECUTABLE},
{"ftueq",	I_EXECUTABLE},
{"ftuge",	I_EXECUTABLE},
{"ftugt",	I_EXECUTABLE},
{"ftule",	I_EXECUTABLE},
{"ftult",	I_EXECUTABLE},
{"ftun",	I_EXECUTABLE},
{"global",	I_PSOP},
{"illegal",	I_EXECUTABLE},
{"include",	I_INCLUDE},
{"iterate",	I_ITERATE},
{"jmp",		I_BRANCH},
{"jsr",		I_BSR},
{"lalign",	I_PSOP},
{"lcomm",	I_PSOP},
{"ldopt",	I_LDOPT},
{"long",	I_PSOP},
{"nolist",	I_NOLIST},
{"nop",		I_EXECUTABLE},
{"output",	I_OUTPUT},
{"packed",	I_PSOP},
{"reset",	I_EXECUTABLE},
{"rte",		I_EXECUTABLE},
{"rtr",		I_EXECUTABLE},
{"rts",		I_EXECUTABLE},
{"set",		I_PSOP},
{"short",	I_PSOP},
{"sltnormal",	I_PSOP},
{"sltspecial",	I_PSOP},
{"space",	I_PSOP},
{"stack",	I_STACK},
{"tcc",		I_EXECUTABLE},
{"tcs",		I_EXECUTABLE},
{"teq",		I_EXECUTABLE},
{"text",	I_PSOP},
{"tf",		I_EXECUTABLE},
{"tge",		I_EXECUTABLE},
{"tgt",		I_EXECUTABLE},
{"thi",		I_EXECUTABLE},
{"ths",		I_EXECUTABLE},
{"time",	I_TIME},
{"title",	I_TITLE},
{"tle",		I_EXECUTABLE},
{"tlo",		I_EXECUTABLE},
{"tls",		I_EXECUTABLE},
{"tlt",		I_EXECUTABLE},
{"tmi",		I_EXECUTABLE},
{"tne",		I_EXECUTABLE},
{"tpl",		I_EXECUTABLE},
{"trapv",	I_EXECUTABLE},
{"tt",		I_EXECUTABLE},
{"tvc",		I_EXECUTABLE},
{"tvs",		I_EXECUTABLE},
{"verify",	I_VERIFY},
{"vt",		I_PSOP}
};

/*

    process_input_file()

    Scan the input file, saving pertinent information and creating a code
    file.

*/

#define HIGH (sizeof(ilist) / sizeof(struct instruction))

void process_input_file()
{
    char *ptr, *tk_start, *tk_end;
    BOOLEAN label;
    char *keyword;
    unsigned long start, index, mask;
    int order;

    /* open the input file */
    open_input_file(gInputFile, FALSE);

    /* create code file and open it */
    CREATE_STRING(gCodeFile, L_tmpnam);
    (void) tmpnam(gCodeFile);
    open_output_file(gCodeFile);

    /* create space to store sp, cc, and effective address value */
    Cprint_variables_code();

    /* constants for binary search for instruction mnemonic */
    start = 0x80000000;
    while (!(start & HIGH)) start >>= 1;
    if (HIGH == start) start >>= 1;

    /* process input file */
    while (read_line()) {  /* get next line */

	get_next_token(gBuffer, &tk_start, &tk_end);

	/* check for end-of-line or comment */
	if ((tk_start == (char *) 0) || (*tk_start == '#')) {
	    if ((gMode == EXECUTION_PROFILING) && (gSection == TIME))
		append_instruction();   /* save instruction */
	    fprintf(gOutput, "%s\n", gBuffer); /* dump it */
	    continue;
	};

	/* check for label */
	label = FALSE;
	ptr = strchr(tk_start, ':');
	if ((ptr != (char *) 0) && (ptr < tk_end)) {
	    tk_end = ptr + 1;
	    label = TRUE;
	    get_next_token(tk_end, &tk_start, &tk_end);

	    /* label only? */
	    if ((tk_start == (char *) 0) || (*tk_start == '#')) {
	        if ((gMode == EXECUTION_PROFILING) && (gSection == TIME))
		    append_instruction();
	        fprintf(gOutput, "%s\n", gBuffer);
	        continue;
	    };
	};

	/* save instruction mnemonic */
	index = mask = start;
	CREATE_STRING(keyword, tk_end - tk_start + 1);
	(void) strncpy(keyword, tk_start, tk_end - tk_start);
	keyword[tk_end - tk_start] = '\0';

	/* search for instruction mnemonic in table (binary search) */
	while (mask) {
	    if (!(order = index < HIGH ? strcmp(keyword, ilist[index].name) 
	      : -1)) break;
	    if (order < 0) index &= ~mask;
	    index |= mask >>= 1;
	};
	free(keyword);

	/* switch on instruction type */
	if (mask) switch(ilist[index].type) {
	case I_ASSERT:	/* assert */
		if (label) error_locate(77);
		instr_assert(tk_end);
		break;
	case I_ASSERT_B:/* assert.b */
		if (label) error_locate(78);
		instr_assert_bwl(tk_end, BYTE);
		break;
	case I_ASSERT_L:/* assert.l */
		if (label) error_locate(79);
		instr_assert_bwl(tk_end, LONG);
		break;
	case I_ASSERT_W:/* assert.w */
		if (label) error_locate(80);
		instr_assert_bwl(tk_end, WORD);
		break;
	case I_BRANCH:	/* branch instruction */
		instr_default(label, tk_end, TRAN_EXECUTABLE, I_BRANCH, FALSE);
		break;
	case I_BSR:	/* branch to subroutine */
		instr_default(label, tk_end, TRAN_EXECUTABLE, I_BSR, FALSE);
		break;
	case I_CODE:	/* code even/odd */
		if (label) error_locate(81);
		instr_code(tk_end);
		break;
	case I_COMMENT:	/* comment */
		if (label) error_locate(82);
		instr_comment(tk_end);
		break;
	case I_DATANAME:/* dataname */
		if (label) error_locate(83);
		instr_dataname(tk_end);
		break;
	case I_DATASET:	/* dataset */
		if (label) error_locate(84);
		instr_dataset(tk_end);
		break;
	case I_DBCC:	/* dbcc */
		instr_default(label, tk_end, TRAN_EXECUTABLE, I_DBCC, FALSE);
		break;
	case I_EXECUTABLE:	/* generic executable instruction */
		instr_default(label, tk_end, EXECUTABLE, 0, FALSE);
		break;
	case I_INCLUDE:	/* include */
		if (label) error_locate(85);
		instr_include(tk_end, TRUE);
		break;
	case I_ITERATE:	/* iterate */
		if (label) error_locate(86);
		instr_iterate(tk_end);
		break;
	case I_LDOPT:	/* ldopt */
		if (label) error_locate(87);
		instr_ldopt(tk_end);
		break;
	case I_NOLIST:	/* nolist */
		if (label) error_locate(88);
		instr_nolist(tk_end);
		break;
	case I_OUTPUT:	/* output */
		if (label) error_locate(89);
		instr_output(tk_end);
		break;
	case I_PSOP:	/* generic pseudo-op instruction */
		instr_default(label, tk_end, PSOP, 0, FALSE);
		break;
	case I_STACK:	/* stack even/odd */
		if (label) error_locate(90);
		instr_stack(tk_end);
		break;
	case I_TIME:	/* time */
		if (label) error_locate(91);
		instr_time(tk_end);
		break;
	case I_TITLE:	/* title */
		if (label) error_locate(92);
		instr_title(tk_end);
		break;
	case I_VERIFY:	/* verify */
		if (label) error_locate(93);
		instr_verify(tk_end);
		break;
	} else instr_default(label, tk_end, EXECUTABLE, 0, TRUE);
    };

    return;
}
