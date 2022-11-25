/*

    parameters-->+-----------+      +----------+    +----------+
	   +-----|   next    |  +-->|   next   |--->| next = 0 |
	   |     +-----------+  |   +----------+    +----------+
	   |     |  segment  |--+   |   index  |    |  index   |
	   |     +-----------+	    +----------+    +----------+
	   |     |   file    |--+   |  segment |-+  |  segment |-+
	   |	 +-----------+  |   +----------+ |  +----------+ |
	   |     |   line    |  |                V               V
	   |     +-----------+  +-->"file"    " mov.l  &"      ",%d0"
	   |     |instruction|----->"mov.l &$x,%d0"
	   |     +-----------+  
	   |                    +---parameters_tail
	   |                    |
           +---->+-----------+<-+   +----------+    +----------+
	         | next = 0  |  +-->|   next   |--->| next = 0 |
	         +-----------+  |   +----------+    +----------+
	         |  segment  |--+   |   index  |    |  index   |
	         +-----------+	    +----------+    +----------+
	         |   file    |--+   |  segment |-+  |  segment |-+
	    	 +-----------+  |   +----------+ |  +----------+ |
	         |   line    |  |                V               V
	         +-----------+  +-->"file"    " mov.l  &"      ",%d0"
	         |instruction|----->"mov.l &$x,%d0"
	         +-----------+
	    
				---*---

	nonprofiled_instruction_list	nonprofiled_instruction_list_tail
	   |							   |
	   +---->+-----------+    +-----------+    +-----------+<--+
	         |   next    |--->|   next    |--->| next = 0  |<-------+
		 +-----------+    +-----------+    +-----------+	|
		 |instruction|    |instruction|    |instruction|	|
		 +-----------+    +-----------+    +-----------+	|
		       |                |                |		|
		       V                V                V		|
                 "   text"          "  fpmode 1"    "  lalign 4"	|
									|
	     (This is a list of all non-profiled instructions in the	|
	      timed section before the first profiled instruction is	|
	      found.)							|
									|
				---*---					|
									|
						instruction_list_tail---+
									|
	     (This variable may point to the end of the current pro-	|
	      filed or non-profiled instruction list, whichever is	|
	      appropriate.)						|
									|
				---*---					|
									|
	profiled_instruction_list					|
	   |								|
	   +--->+----------+    +-----------+    +-----------+		|
	   +----|   next   | +->|   next    |--->| next = 0  |		|
	   |    +----------+ |  +-----------+    +-----------+		|
	   |    |   list   |_+  |instruction|    |instruction|		|
	   |    +----------+    +-----+-----+    +-----+-----+		|
	   |                          |                |		|
	   |                          V                V		|
	   |                    " mov.l %d0,%d1"     " data"		|
	   |								|
	   |                  +---profiled_instruction_list_tail	|
	   |                  |						|
	   +--->+----------+<-+ +-----------+    +-----------+<---------+
	        | next = 0 | +->|   next    |--->| next = 0  |
	        +----------+ |  +-----------+    +-----------+
	        |   list   |_+  |instruction|    |instruction|
	        +----------+    +-----+-----+    +-----+-----+
	                              |                |
	                              V                V
	                        " lsr.l &1,%d1"     " set  size,100"

	     (The first instruction in each list is can be profiled;
	      subsequent instructions cannot be. Whenever a non-
	      profiled instruction is encountered, it is tacked on at 
	      "instruction_list_tail". Profiled instructions are
	      tacked on at "profile_instruction_list_tail".)

*/

#include "fizz.h"

static struct segment_type {
    struct segment_type *next;
    unsigned long index;
    char *segment;
};

static struct parameter_type {
    struct parameter_type *next;
    struct segment_type *segment;
    char *file;
    unsigned long line;
    char *instruction;
} *parameters = (struct parameter_type *) 0,
  *parameters_tail = (struct parameter_type *) 0;

static struct instr_type {
    struct instr_type *next;
    char *instruction;
} *nonprofiled_instruction_list = (struct instr_type *) 0,
  *nonprofiled_instruction_list_tail = (struct instr_type *) 0,
  *instruction_list_tail = (struct instr_type *) 0;

static struct prof_instr_type {
    struct prof_instr_type *next;
    struct instr_type *list;
} *profiled_instruction_list = (struct prof_instr_type *) 0,
  *profiled_instruction_list_tail = (struct prof_instr_type *) 0;

static unsigned long instruction_count = 0;

/*

    translate_instruction()

    Translate those instructions which involve branches. This is needed
    for execution profiling because of the extra code that is inserted into
    the time section. This extra code may make branch targets unreachable
    so translation is necessary.

*/

static void translate_instruction(label, itype)
BOOLEAN label;
int itype;
{
    char c, *ptr, *tk_start, *tk_end;
    unsigned long label1, label2;
    char *buffer;

    /* make a copy of the instruction */
    CREATE_STRING(buffer, strlen(gBuffer) + 1);
    (void) strcpy(buffer, gBuffer);

    get_next_token(buffer, &tk_start, &tk_end);

    /* delete any label */
    if (label) {
	for (ptr = tk_start; ptr < tk_end; ptr++) *ptr = ' ';
	get_next_token(tk_end, &tk_start, &tk_end);
    };

    /* convert bsr's to jsr's */
    if (itype == I_BSR) {
	ptr = tk_start;
	*ptr++ = 'j';
	*ptr++ = 's';
	*ptr++ = 'r';
	while (ptr < tk_end) *ptr++ = ' ';
        fprintf(gOutput, "%s\n", buffer);
	free(buffer);
        return;
    };

    /*
	beq.b  L1     =>           beq.b   label1
			           bra.b   label2
			  label1:  jmp	   L1
			  label2:  space   0

        dbcs   %d2,L2 =>           dbcs	   %d2,label3
				   bra.b   label4
			  label3:  jmp	   L2
			  label4:  space   0
    */
    label1 = create_label();
    label2 = create_label();
    get_next_token(tk_end, &tk_start, &tk_end);
    if (tk_start == (char *) 0) {
        fprintf(gOutput, "%s\n", gBuffer);
	free(buffer);
        return;
    };
    if (itype == I_DBCC) {
	tk_start = strchr(tk_start, ',');
	if ((tk_start == (char *) 0) || (tk_start >= tk_end - 1)) {
	    fprintf(gOutput, "%s\n", gBuffer);
	    free(buffer);
	    return;
	};
	tk_start++;
    };
    c = *tk_start;
    *tk_start = '\0';
    fprintf(gOutput, "%s___Z%u%s\n", buffer, label1, tk_end);
    *tk_start = c;
    *tk_end = '\0';
    fprintf(gOutput, "\tbra.b\t___Z%u\n", label2);
    fprintf(gOutput, "___Z%u:\tjmp\t%s\n", label1, tk_start);
    fprintf(gOutput, "___Z%u:\tspace\t0\n", label2);
    free(buffer);

    return;
}

/*

    instr_default()

    Process a generic instruction. Depending upon mode (performance
    analysis, execution profiling, assertion listing) and section
    (initialization, time, verify) the instruction has to be scanned
    for parameters or saved for printing.

*/

void instr_default(labelled, bufptr, type, itype, param_flag)
BOOLEAN labelled, 	/* instruction is labelled */
param_flag;		/* instruction can have parameters */
char *bufptr;		/* points to character following mnemonic */
int type, 		/* fizz pseudo-op, general pseudo-op, executable
			   instruction, executable instruction requiring
			   translation, non-executable */
itype;			/* branch, decrement-and-branch, branch-to-sub...*/
{
    int c;
    char *ptr, *segment_start, *label, *label_start, *label_end, *param_start; 
    char *param_end, *tk_start, *tk_end, *segment, *parameter, *file;
    unsigned long index;
    struct parameter_type *current_parameter;
    struct segment_type *current_segment, *current_segment_tail;
    struct instr_type *current_instruction;
    struct prof_instr_type *current_profinstr;
    void translate_instruction();

    /* tell the user where we are in case he made a syntax error */
    fprintf(gOutput, "# \"%s\", line %u\n", gFile, gLine);

    /* start up the initialization section if it hasn't been done */
    if (gSection == FIZZ_INIT) {
	gSection = INIT;
	gfInitSection = TRUE;
	Cprint_init_startup_code();
    };

    /* get instruction argument(s) */
    get_next_token(bufptr, &tk_start, &tk_end);
    ptr = tk_start;

    if (gSection == INIT) {
	/* if no argments, done */
        if ((ptr == (char *) 0) || (*ptr == '#')) {
	    fprintf(gOutput, "%s\n", gBuffer);
	    return;
        };

	/* now we must search for parameters */
	current_parameter = (struct parameter_type *) 0;
	current_segment = (struct segment_type *) 0;
	current_segment_tail = (struct segment_type *) 0;
	segment_start = gBuffer;

	/* save label if there is one */
	if (labelled) {
	    get_next_token(gBuffer, &label_start, &label_end);
	    CREATE_STRING(label, label_end - label_start + 1);
	    (void) strncpy(label, label_start, label_end - label_start);
	    label[label_end - label_start] = '\0';
	    segment_start = label_end;
	};

	/* scan for parameters */
	while (((param_start = strchr(ptr, '$')) != (char *) 0) &&
	  (param_start < tk_end)) {

	    /* can this instruction have parameters? */
            if (!param_flag) {	
	        error_locate(108);
	        fprintf(gOutput, "%s\n", gBuffer);
	        return;
	    };

	    /* scan parameter */
	    param_end = param_start + 1;
	    while (isalnum(c = *param_end++) || (c == '_'));
	    --param_end;

	    /* save parameter name if valid */
	    if (param_end > param_start + 1) {
		CREATE_STRING(parameter, param_end - param_start + 1);
		(void) strncpy(parameter, param_start, param_end - param_start);
		parameter[param_end - param_start] = '\0';
		index = get_dataname_index(parameter);
		free(parameter);
	    } else index = 0xffffffff;

	    /* if bad parameter name or if name has not been declared in
	       dataname statement... */
	    if (index == 0xffffffff) {
	        if (param_end == param_start + 1)
		    error_locate(55);
		else error_locate(112);
		/* clean up and quit */
		current_segment_tail = current_segment;
		if (current_parameter != (struct parameter_type *) 0)
		    free(current_parameter);
		while (current_segment_tail != (struct segment_type *) 0) {
		    current_segment = current_segment_tail->next;
		    free(current_segment_tail);
		    current_segment_tail = current_segment;
		};
	        fprintf(gOutput, "%s\n", gBuffer);
	        return;
	    };

	    /* instruction is saved from beginning of line or from end of
	       last parameter as a "segment" along with the dataname
	       "index" that it corresponds to */
	    CREATE_STRUCT(current_segment, segment_type);
	    current_segment->next = (struct segment_type *) 0;
	    current_segment->index = index;
	    CREATE_STRING(segment, param_start - segment_start + 1);
	    (void) strncpy(segment, segment_start, param_start - segment_start);
	    segment[param_start - segment_start] = '\0';
	    current_segment->segment = segment;

	    /* if this is the first segment for this instruction... */
	    if (current_parameter == (struct parameter_type *) 0) {

		/* create a parameter header structure for this line */
		CREATE_STRUCT(current_parameter, parameter_type);
		current_parameter->next = (struct parameter_type *) 0;
		current_parameter->segment = current_segment;
		CREATE_STRING(file, strlen(gFile) + 1);
		(void) strcpy(file, gFile);
		current_parameter->file = file;
		current_parameter->line = gLine;
		current_parameter->instruction = compact_instruction();
	    } else current_segment_tail->next = current_segment;
	    current_segment_tail = current_segment;

	    ptr = segment_start = param_end;
	    if (!*ptr) break;
	};

	/* patch this parametered instruction into the linked list
	   with other parametered instructions */
	if (current_parameter != (struct parameter_type *) 0) {
	    CREATE_STRUCT(current_segment, segment_type);
	    current_segment->next = (struct segment_type *) 0;
	    CREATE_STRING(current_segment->segment,
	      strlen(segment_start) + 1);
	    (void) strcpy(current_segment->segment, segment_start);
	    current_segment_tail->next = current_segment;
	    if (labelled) fprintf(gOutput, "%s\n", label); /* dump a label */
	    /* create code to do parameter */
	    Cprint_parameter_code(gParameterCount);
	    gParameterCount++;
	    if (parameters == (struct parameter_type *) 0)
		parameters = current_parameter;
	    else
		parameters_tail->next = current_parameter;
	    parameters_tail = current_parameter;
	} else fprintf(gOutput, "%s\n", gBuffer);

    } else if (gSection == TIME) {

	/* parameters are a no-no here */
	if ((ptr != (char *) 0) && (*ptr != '#') && ((param_start = 
	  strchr(ptr, '$')) != (char *) 0) && (param_start < tk_end))
	    error_locate(106);

	if (gMode == EXECUTION_PROFILING) {

	    /* every instruction must be saved to be printed to the output */
       
	    if ((type == EXECUTABLE) || (type == TRAN_EXECUTABLE)) {

		/* new link in list of executable instructions */
	        CREATE_STRUCT(current_instruction, instr_type);
	        current_instruction->next = (struct instr_type *) 0;
	        CREATE_STRING(current_instruction->instruction,
	          strlen(gBuffer) + 1);
	        (void) strcpy(current_instruction->instruction, gBuffer);
		CREATE_STRUCT(current_profinstr, prof_instr_type);
		current_profinstr->next = (struct prof_instr_type *) 0;
		current_profinstr->list = current_instruction;
		if (profiled_instruction_list == (struct prof_instr_type *) 0)
		    profiled_instruction_list = current_profinstr;
		else profiled_instruction_list_tail->next = current_profinstr;
		profiled_instruction_list_tail = current_profinstr;

		/* labels must be dumped separately */
		if (labelled) {
		    get_next_token(gBuffer, &tk_start, &tk_end);
		    CREATE_STRING(label, tk_end - gBuffer + 1);
		    (void) strncpy(label, gBuffer, tk_end - gBuffer);
		    label[tk_end - gBuffer] = '\0';
		    fprintf(gOutput, "%s", label);
		};
		Cprint_profile_count_code(instruction_count);
		instruction_count++;
		if (type == EXECUTABLE) {
		    if (labelled) {	/* delete a label */
			ptr = label;
			tk_start = gBuffer;
			while (tk_start++ < tk_end) *ptr++ = ' ';
			fprintf(gOutput, "%s%s\n", label, tk_end);
		    } else fprintf(gOutput, "%s\n", gBuffer);
		} else translate_instruction(labelled, itype);
		if (labelled) free(label);
	        instruction_list_tail = current_instruction;
	    } else {
		/* non-executable; just tack instruction onto the end */
		append_instruction();
		fprintf(gOutput, "%s\n", gBuffer);
	    };
	} else {
	    if ((gMode == PERFORMANCE_ANALYSIS) &&
	      ((type == EXECUTABLE) || (type == TRAN_EXECUTABLE)))
		instruction_count++;  /* for printing statistics */
	    fprintf(gOutput, "%s\n", gBuffer);
	};

    } else {
	if ((ptr != (char *) 0) && (*ptr != '#') && ((param_start = 
	  strchr(ptr, '$')) != (char *) 0) && (param_start < tk_end))
	    error_locate(107);
	fprintf(gOutput, "%s\n", gBuffer);
    };

    return;
}

/*

    append_instruction()

    Append an instruction onto the current profiled or non-profiled 
    instruction list.

*/

void append_instruction()
{
    struct instr_type *current_instruction;

    CREATE_STRUCT(current_instruction, instr_type);
    current_instruction->next = (struct instr_type *) 0;
    CREATE_STRING(current_instruction->instruction,
      strlen(gBuffer) + 1);
    (void) strcpy(current_instruction->instruction, gBuffer);
    if (instruction_list_tail == (struct instr_type *) 0)
        nonprofiled_instruction_list = current_instruction;
    else instruction_list_tail->next = current_instruction;
    instruction_list_tail = current_instruction;

    return;
}
    
/*

    Cprint_parameter_routines()

    Create code to handle dataname parameters.

    Note:
	D = number of datasets 
	P = number of parameter currently considered

		data
		lalign	4
	___Zcodecc: space 2
	___Zcodesp: space 4
		text

		global	___Zcode{D*P+0}
	___Zcode{D*P+0}:	# "file", line number, dataset: {dataset}
				# abbreviated instruction
		mov.w	%cc,___Zcodecc
		mov.l	(%sp)+,___Zcodesp	# return address
		addq.w	&4,%sp			# remove parameter
		mov.w	___Zcodecc,%cc
		{instruction}
		mov.w	%cc,___Zcodecc
		mov.l	___Zcodesp,-(%sp)
		mov.w	___Zcodecc,-(%sp)
		rtr
	___Zcode{D*P+1}:	# "file", line number, dataset: {dataset}
				# abbreviated instruction
		mov.w	%cc,___Zcodecc
		mov.l	(%sp)+,___Zcodesp	# return address
		addq.w	&4,%sp			# remove parameter
		mov.w	___Zcodecc,%cc
		{instruction}
		mov.w	%cc,___Zcodecc
		mov.l	___Zcodesp,-(%sp)
		mov.w	___Zcodecc,-(%sp)
		rtr
		      .
		      .
		      .

*/

static char *zcode1[] = {
	"\tdata",
	"\tlalign\t4",
	"___Zcodecc:\tspace\t2",
	"___Zcodesp:\tspace\t4",
	"\ttext",
};

static char *zcode2[] = {
	"\tmov.w\t%cc,___Zcodecc",
	"\tmov.l\t(%sp)+,___Zcodesp",
	"\taddq.w\t&4,%sp",
	"\tmov.w\t___Zcodecc,%cc",
};

static char *zcode3[] = {
	"\n\tmov.w\t%cc,___Zcodecc",
	"\tmov.l\t___Zcodesp,-(%sp)",
	"\tmov.w\t___Zcodecc,-(%sp)",
	"\trtr",
};

void Cprint_parameter_routines()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long i, j, label;
    struct parameter_type *current_parameter;
    struct segment_type *current_segment;
    unsigned long count;

    for (count = 0; count < sizeof(zcode1)/sizeof(char *); count++)
	print(output, "%s\n", zcode1[count]);
    current_parameter = parameters;
    for (i = 0; i < gParameterCount; i++) {
	for (j = 0; j < gDatasetCount; j++) {
	    label = i * gDatasetCount + j;
	    print(output, "\tglobal\t___Zcode%u\n", label);
	    print(output, "___Zcode%u:\t# \"%s\", line %u, dataset: %s\n", 
	      label, current_parameter->file, current_parameter->line,
	      get_dataset_name(j));
	    print(output, "\t\t\t# %s\n", current_parameter->instruction);
	    for (count = 0; count < sizeof(zcode2)/sizeof(char *); count++)
		print(output, "%s\n", zcode2[count]);
	    current_segment = current_parameter->segment;
	    while (current_segment != (struct segment_type *) 0) {
		print(output, "%s", current_segment->segment);
		if (current_segment->next != (struct segment_type *) 0)
		    print(output, "%s", get_datum(current_segment->index, j));
	        current_segment = current_segment->next;
	    };
	    for (count = 0; count < sizeof(zcode3)/sizeof(char *); count++)
		print(output, "%s\n", zcode3[count]);
	};
	current_parameter = current_parameter->next;
    };

    return;
}

/*

    Dprint_nonprofiled_instruction_code()

    Create code to print any non-exectuable instruction appearing before
    the first executable instruction in the timed section.  This is for
    execution profiling.

		data
	noexec:
		byte	"          ",9
		byte	{instruction}
		byte	10
		byte	"          ",9
		byte	{instruction}
		byte	10
		byte	"          ",9
		byte	{instruction}
		byte	10
		     .
		     .
		     .
		text
		mov.l	file,-(%sp)
		pea	{length}		# length of all noexec text
		pea	1.w
		pea	noexec
		jsr	_fwrite			# fwrite(noexec,1,{length},
						#     file);
		lea	16(%sp),%sp

*/

void Dprint_nonprofiled_instruction_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long length;
    struct instr_type *current_instruction;

    if (nonprofiled_instruction_list == (struct instr_type *) 0) return;
    print(output, "\tdata\n");
    print(output, "noexec:\n");
    length = 0;
    current_instruction = nonprofiled_instruction_list;
    while (current_instruction != (struct instr_type *) 0) {
	print(output, "\tbyte\t\"          \",9\n");
	print_byte_info(current_instruction->instruction);
	print(output, "\tbyte\t10\n");
	length += strlen(current_instruction->instruction) + 12;
	current_instruction = current_instruction->next;
    };
    print(output, "\ttext\n");
    print(output, "\tmov.l\tfile,-(%%sp)\n");
    print(output, "\tpea\t%u\n", length);
    print(output, "\tpea\t1.w\n");
    print(output, "\tpea\tnoexec\n");
    print(output, "\tjsr\t_fwrite\n");
    print(output, "\tlea\t16(%%sp),%%sp\n");

    return;
}

/*

    Dprint_profile_count_table()

    Create a table for counting instruction hits.

		data
		lalign	4
    pcount:
		long	0
		long	0
		long	0
		long	0
		long	0
		long	0
		long	0
		     .
		     .
		     .

*/

void Dprint_profile_count_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register unsigned long i;

    if (!instruction_count) return;
    print(output, "\tdata\n");
    print(output, "\tlalign\t4\n");
    print(output, "pcount:\n");
    for (i = 0; i < instruction_count; i++)
	print(output, "\tlong\t0\n");

    return;
}

/*

    Dprint_profile_code()

    Create code to perform execution profiling.

    Note:
	{datasets}  = number of data sets
	{max_count} = the maximum count value for any of the data sets
	{iterations} = number of iterations
	{instructions} = number of executable instructions

		data
		lalign	4
	prof_a6:space	4
	prof_sp:space	4
	prof1:	byte	"%10u",9,"%s",0

		text

	#
	# Clear out pcount array
	#
		movq	&0,%d0
		lea	pcount,%a0
    #if {instructions} > 65535
		mov.l	&{instructions},%d1
    #else if {instructions} > 128
		mov.w	&{instructions}-1,%d1
    #else
		movq	&{instructions}-1,%d1
    #endif
	prof2:	mov.l	%d0,(%a0)+
    #if {instructions} > 65535
		subq.l	&1,%d1
		bne.b	prof2
    #else
		dbra	%d1,prof2
    #endif

    #if there is more than one data set

     #if there are dynamic parameters
		lea	dsetpar,%a0		# pointers to parameter tables
		mov.l	(%a0)+,cdsetpar		# pointer to table for dataset
		mov.l	%a0,-4(%a6)		# save pointer to next table
     #endif

     #if {datasets} > 65535
		mov.l	&{datasets},-12(%a6)	# dataset counter
     #else
		mov.w	&{datasets},-12(%a6)
     #endif

		lea	count,%a0		# count table
     #if {max_count} > 65535
		mov.l	(%a0)+,-20(%a6)		# count for current dataset
     #else
		mov.w	(%a0)+,-20(%a6)
     #endif
		mov.l	%a0,-16(%a6)		# save pointer to next count

    #else (zero or one datasets)

     #if there are data sets
      #if {max_count} > 65535
		mov.l	&{max_count},-20(%a6)	# count for only dataset
      #else
		mov.w	&{max_count},-20(%a6)
      #endif
     #endif

    #endif

	prof3:	mov.l	%a6,prof_a6	# save %a6 and %sp in case user
		mov.l	%sp,prof_sp	#    stomps on them

    #if there is an initialization section
		subq.w	&6,%sp		# be careful not to affect any
		mov.w	%cc,(%sp)	#   registers or condition codes
		mov.l	&___Zinit,2(%sp)
		mov.l	&prof4,___ZSP	# return is via ___ZSP
		rtr
	prof4:	
    #endif
		subq.w	&6,%sp
		mov.w	%cc,(%sp)
		mov.l	&___Ztime,2(%sp)	# execute the time section
		mov.l	&prof5,___ZSP
		rtr
	prof5:	mov.l	prof_a6,%a6	# restore %a6 and %sp
		mov.l	prof_sp,%sp

    #if there are data sets
     #if {max_count} > 65535
		subq.l	&1,-20(%a6)	# decrement count for current dataset
     #else
		subq.w	&1,-20(%a6)
     #endif
		bne.b	prof3
    #endif

    #if there is more than one data set
     #if {datasets} > 65535
		subq.l	&1,-12(%a6)	# decrement dataset counter
     #else
		subq.w	&1,-12(%a6)
     #endif
		beq.b	prof6

     #if there are dynamic parameters
		mov.l	-4(%a6),%a0	# to next parameter table
		mov.l	(%a0)+,cdsetpar	# save pointer to it
		mov.l	%a0,-4(%a6)	# save pointer to next table
     #endif
	  
		mov.l	-16(%a6),%a0	# pointer to next count
	    #if {max_count} > 65535
		mov.l	(%a0)+,-20(%a6)	# get count
	    #else
		mov.w	(%a0)+,-20(%a6)
	    #endif
		mov.l	%a0,-16(%a6)	# save pointer to next count

		bra.b	prof3
	prof6:
    #endif

	#
	# Print information about instruction hits
	#
    #if {instructions} > 65536			#    +
		mov.l	&{instructions},%d2	#    |
    #else if {instructions} > 128		#    |
		mov.w	&{instructions}-1,%d2	# create instruction counter
    #else					#    |
		movq	&{instructions}-1,%d2	#    |
    #endif					#    +

		lea	pcount,%a2		# table of instruction hits
		lea	proftext,%a3		# table of text
	prof7:	mov.l	(%a3)+,-(%sp)		# text
		mov.l	(%a2)+,-(%sp)		# hits
		pea	prof1			# format
		mov.l	file,-(%sp)		# file
		jsr	_fprintf		# fprintf(file,format,hits
		lea	16(%sp),%sp		#    text);

    #if {instructions} > 65535			#    +
		subq.l	&1,%d2			#    |
		bne.b	prof7			# decrement counter
    #else					#    |
		dbra	%d2,prof7		#    |
    #endif					#    +

*/

static char *prof_code1[] = {
	"\tdata",
	"\tlalign\t4",
	"prof_a6:space\t4",
	"prof_sp:space\t4",
	"prof1:\tbyte\t\"%10u\",9,\"%s\",0",
	"\ttext",
	"\tmovq\t&0,%d0",
	"\tlea\tpcount,%a0",
};

static char *prof_code2[] = {
	"\tlea\tdsetpar,%a0",
	"\tmov.l\t(%a0)+,cdsetpar",
	"\tmov.l\t%a0,-4(%a6)",
};

static char *prof_code3[] = {
	"\tsubq.w\t&6,%sp",
	"\tmov.w\t%cc,(%sp)",
	"\tmov.l\t&___Zinit,2(%sp)",
	"\tmov.l\t&prof4,___ZSP",
	"\trtr",
	"prof4:",
};

static char *prof_code4[] = {
	"\tsubq.w\t&6,%sp",
	"\tmov.w\t%cc,(%sp)",
	"\tmov.l\t&___Ztime,2(%sp)",
	"\tmov.l\t&prof5,___ZSP",
	"\trtr",
	"prof5:\tmov.l\tprof_a6,%a6",
	"\tmov.l\tprof_sp,%sp",
};

static char *prof_code5[] = {
	"\tmov.l\t-4(%a6),%a0",
	"\tmov.l\t(%a0)+,cdsetpar",
	"\tmov.l\t%a0,-4(%a6)",
};

static char *prof_code6[] = {
	"\tmov.l\t%a0,-16(%a6)",
	"\tbra.b\tprof3",
	"prof6:",
};

static char *prof_code7[] = {
	"\tlea\tpcount,%a2",
	"\tlea\tproftext,%a3",
	"prof7:\tmov.l\t(%a3)+,-(%sp)",
	"\tmov.l\t(%a2)+,-(%sp)",
	"\tpea\tprof1",
	"\tmov.l\tfile,-(%sp)",
	"\tjsr\t_fprintf",
	"\tlea\t16(%sp),%sp",
};

void Dprint_profile_code()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    register int count;
    unsigned long max_count;

    if (!instruction_count) return;
    max_count = max_dataset_count();
    for (count = 0; count < sizeof(prof_code1)/sizeof(char *); count++)
	print(output, "%s\n", prof_code1[count]);
    if (instruction_count > 65535)
	print(output, "\tmov.l\t&%u,%%d1\n", instruction_count);
    else
        print(output, "\tmov%s\t&%u,%%d1\n", instruction_count > 128 ?
	  ".w" : "q", instruction_count - 1);
    print(output, "prof2:\tmov.l\t%%d0,(%%a0)+\n");
    if (instruction_count > 65535) {
	print(output, "\tsubq.l\t&1,%%d1\n");
	print(output, "\tbne.b\tprof2\n");
    } else print(output, "\tdbra\t%%d1,prof2\n");
    if (gDatasetCount > 1) {
	if (gParameterCount)
            for (count = 0; count < sizeof(prof_code2)/sizeof(char *); count++)
	        print(output, "%s\n", prof_code2[count]);
        print(output, "\tmov.%c\t&%u,-12(%%a6)\n", gDatasetCount > 65535 ?
          'l' : 'w', gDatasetCount);
        print(output, "\tlea\tcount,%%a0\n");
        print(output, "\tmov.%c\t(%%a0)+,-20(%%a6)\n", max_count > 65535 ?
          'l' : 'w');
	print(output, "\tmov.l\t%%a0,-16(%%a6)\n");
    } else if (gDatasetCount) {
        print(output, "\tmov.%c\t&%u,-20(%%a6)\n", max_count > 65535 ?
          'l' : 'w', max_count);
    };
    print(output, "prof3:\tmov.l\t%%a6,prof_a6\n");
    print(output, "\tmov.l\t%%sp,prof_sp\n");
    if (gfInitSection)
        for (count = 0; count < sizeof(prof_code3)/sizeof(char *); count++)
	    print(output, "%s\n", prof_code3[count]);
    for (count = 0; count < sizeof(prof_code4)/sizeof(char *); count++)
        print(output, "%s\n", prof_code4[count]);
    if (gDatasetCount) {
	print(output, "\tsubq.%c\t&1,-20(%%a6)\n", max_count > 65535 ? 
	  'l' : 'w');
	print(output, "\tbne.b\tprof3\n");
    };
    if (gDatasetCount > 1) {
	print(output, "\tsubq.%c\t&1,-12(%%a6)\n", gDatasetCount > 65535 ?
	  'l' : 'w');
	print(output, "\tbeq.b\tprof6\n");
	if (gParameterCount)
            for (count = 0; count < sizeof(prof_code5)/sizeof(char *); count++)
	        print(output, "%s\n", prof_code5[count]);
	print(output, "\tmov.l\t-16(%%a6),%%a0\n");
	print(output, "\tmov.%c\t(%%a0)+,-20(%%a6)\n", max_count > 65535 ?
	  'l' : 'w');
        for (count = 0; count < sizeof(prof_code6)/sizeof(char *); count++)
	    print(output, "%s\n", prof_code6[count]);
    };
    if (instruction_count > 65535)
	print(output, "\tmov.l\t&%u,%%d2\n", instruction_count);
    else
        print(output, "\tmov%s\t&%u,%%d2\n", instruction_count > 128 ?
	  ".w" : "q", instruction_count - 1);
    for (count = 0; count < sizeof(prof_code7)/sizeof(char *); count++)
	print(output, "%s\n", prof_code7[count]);
    if (instruction_count > 65535) {
	print(output, "\tsubq.l\t&1,%%d2\n");
	print(output, "\tbne.b\tprof7\n");
    } else print(output, "\tdbra\t%%d2,prof7\n");

    return;
}

/*

    Dprint_profiled_instruction_table()

    Create a table of instructions in the timed section. This is used for
    execution profiling. The equivalent of the structure created is:

		char *proftext[];

    Each string in proftext begins with a profiled instruction and may
    be followed by zero or more non-profiled instructions. Therefore,
    the number of entries in proftext is exactly equal to the number
    of profiled instructions.

    Note:
	{instructions} = number of profiled instructions

    #if {instructions} > 0
		data
		lalign	4
	proftext:
		long	proftext0
		long	proftext1
		long	proftext2
		long	proftext3
		long	proftext4
		long	proftext5
		     .
		     .
		     .
	proftext0:
		byte	{profiled instruction}
		byte	10				# linefeed
		byte	"          ",9			# no hits
		byte	{nonprofiled instruction}
		byte	10
		byte	"          ",9
		byte	0
	proftext1:
		byte	{profiled instruction}
		byte	10
		byte	"          ",9
		byte	{nonprofiled instruction}
		byte	10
		byte	{nonprofiled instruction}
		byte	10
		byte	0
	proftext2:
		byte	{profiled instruction}
		byte	10
		byte	"          ",9
		byte	0
		      .
		      .
		      .

*/

void Dprint_profiled_instruction_table()
{
    register FILE *output = gOutput;
    register printtype print = (printtype) fprintf;
    unsigned long count;
    unsigned long *size;
    struct prof_instr_type *current_prof_instr;
    struct instr_type *current_instr;

    if (!instruction_count) return;
    print(output, "\tdata\n");
    print(output, "\tlalign\t4\n");
    print(output, "proftext:\n");

    /* create table of string pointers */
    for (count = 0; count < instruction_count; count++)
	print(output, "\tlong\tproftext%u\n", count);

    /* create strings */
    current_prof_instr = profiled_instruction_list;
    for (count = 0; count < instruction_count; count++) {
	print(output, "proftext%u:\n", count);
	current_instr = current_prof_instr->list;
	print_byte_info(current_instr->instruction);
	print(output, "\tbyte\t10\n");
	current_instr = current_instr->next;
	while (current_instr != (struct instr_type *) 0) {
	    print(output, "\tbyte\t\"          \",9\n");
	    print_byte_info(current_instr->instruction);
	    print(output, "\tbyte\t10\n");
	    current_instr = current_instr->next;
	};
	print(output, "\tbyte\t0\n");
	current_prof_instr = current_prof_instr->next;
    };

    return;
}

/*

    long get_instruction_count()

    Return the number of executable instructions in 
    the timed section.

*/

unsigned long get_instruction_count()
{
    return instruction_count;
}
