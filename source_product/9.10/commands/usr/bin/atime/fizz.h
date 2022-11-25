/*

    This is where we:
	1) include various header files
	2) define several constants, typedefs, and macros
	3) declare global variables
	4) declare routines

*/

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef char BOOLEAN;
#define FALSE 0
#define TRUE 1

typedef void (*printtype)(); /* enables calls to fprintf as register indirect */

/*
    global variables
*/
extern char gBuffer[];			/* input buffer */
extern short gMode;			/* mode: performance,profile,asrtlist */
extern short gSection;			/* init, time, verify */
extern unsigned long gAssertionCount;	/* number of assertions */
extern unsigned long gDatanameCount;	/* number of datanames */
extern unsigned long gDatasetCount;	/* number of datasets */
extern unsigned long gParameterCount;	/* number of dynamic parameters */
extern BOOLEAN gfAbort;			/* delayed prgm abort flag */
extern BOOLEAN gfAssertionFile;		/* assert file exist? */
extern BOOLEAN gfInitSection;		/* is there an init section? */
extern BOOLEAN gfMessages;		/* don't print error messages? */
extern BOOLEAN gfNolist;		/* input file listing? */
extern BOOLEAN gfStackeven;		/* at least one stack even instr? */
extern BOOLEAN gfStackodd;		/* at least one stack odd instr? */
extern BOOLEAN gfTimeSection;		/* is there a time section? */
extern BOOLEAN gfVerifySection;		/* is there a verify section? */
extern unsigned long gIterations;	/* number of iterations */
extern char *gFile;			/* current file name */
extern unsigned long gLine;		/* current line number */
extern FILE *gOutput;			/* output file */
extern char *gInputFile;		/* input file name */
extern char *gAssertFile;		/* assert file name */
extern char *gDriverFile;		/* driver file name */
extern char *gDriverObjFile;		/* driver object  file name */
extern char *gCodeFile;			/* code file name */
extern char *gCodeObjFile;		/* code object file name */
extern char *gExecFile;			/* executable file name */

/*
    routines
*/
char *compact_instruction();
char *get_assertion_name();
char *get_dataname();
char *get_dataset_name();
char *get_datum();
void Cprint_assertion_code();
void Cprint_codeeven_code();
void Cprint_codeodd_code();
void Cprint_init_cleanup_code();
void Cprint_init_startup_code();
void Cprint_parameter_code();
void Cprint_parameter_routines();
void Cprint_profile_count_code();
void Cprint_variables_code();
void Cprint_stackeven_code();
void Cprint_stackodd_code();
void Cprint_time_cleanup_code();
void Cprint_time_startup_code();
void Cprint_verify_cleanup_code();
void Cprint_verify_startup_code();
void Dprint_assertion_listing_routine();
void Dprint_assertion_names_table();
void Dprint_assertion_routine();
void Dprint_cleanup_code();
void Dprint_comment_code();
void Dprint_count_table();
void Dprint_dataset_info_table();
void Dprint_dataset_names_table();
void Dprint_dataset_table_code();
void Dprint_dsetpar_table();
void Dprint_dsetver_table();
void Dprint_info_code();
void Dprint_iterations_code();
void Dprint_line_code();
void Dprint_listing_code();
void Dprint_nonprofiled_instruction_code();
void Dprint_open_code();
void Dprint_parameter_routine();
void Dprint_profile_code();
void Dprint_profile_count_routine();
void Dprint_profile_count_table();
void Dprint_profiled_instruction_table();
void Dprint_stackeven_routine();
void Dprint_stackodd_routine();
void Dprint_startup_code();
void Dprint_stat_code();
void Dprint_time_code();
void Dprint_title_code();
void Dprint_verify_code();
void abort();
void append_instruction();
void assemble_code();
void assemble_driver();
void calculate_iterations();
void cleanup();
void close_input_files();
void close_output_file();
void create_assertion_listing_driver();
unsigned long create_label();
void create_performance_driver();
void error();
void error_locate();
int execl();
void execute();
void exit();
int fclose();
int fprintf();
void free();
unsigned long get_dataname_index();
void get_next_list_item();
void get_next_token();
unsigned long get_instruction_count();
void instr_assert();
void instr_assert_bwl();
void instr_code();
void instr_comment();
void instr_dataname();
void instr_dataset();
void instr_default();
void instr_include();
void instr_iterate();
void instr_ldopt();
void instr_nolist();
void instr_output();
void instr_stack();
void instr_time();
void instr_title();
void instr_verify();
void link_executable();
char *malloc();
unsigned long max_dataset_count();
int open();
void open_input_file();
void open_output_file();
void parse_command_line();
void print_byte_info();
unsigned long printwidth();
void process_assert_file();
void process_input_file();
BOOLEAN read_line();
BOOLEAN search_list();
void signal_abort();
int sprintf();
void store_input_filename();
void store_iterations();
void store_list_name();
void store_nolist();
void store_output_filename();
void store_title();
unsigned long sum_dataset_count();
long time();
int unlink();
void verify_list();

/* mode */
#define PERFORMANCE_ANALYSIS 0
#define EXECUTION_PROFILING 1
#define ASSERTION_LISTING 2

/* command source: from either command line or input file */
#define CMDLINE 0
#define INFILE 1

/* section */
#define FIZZ_INIT 0
#define INIT 1
#define TIME 2
#define VERIFY 3

/* size */
#define BYTE 0
#define WORD 1
#define LONG 2

/* fizz pseudo-ops */
#define I_ASSERT	0
#define I_ASSERT_B	1
#define I_ASSERT_L	2
#define I_ASSERT_W	3
#define I_BRANCH	4
#define I_BSR		5
#define I_CODE		6
#define I_COMMENT	7
#define I_DATANAME	8
#define I_DATASET	9
#define I_DBCC		10
#define I_EXECUTABLE	11
#define I_INCLUDE	12
#define I_ITERATE	13
#define I_LDOPT		14
#define I_NOLIST	15
#define I_OUTPUT	16
#define I_PSOP		17
#define I_STACK		18
#define I_TIME		19
#define I_TITLE		20
#define I_VERIFY	21

/* types of input file instructions */
#define FIZZ_PSOP	0	/* fizz pseudo-op */
#define PSOP		1	/* other pseudo-op */
#define EXECUTABLE	2	/* executable instruction */
#define TRAN_EXECUTABLE 3	/* executable instr that must be translated */
#define NONEXEC		4	/* non-executable instruction */

#define BUFFER_SIZE 200		/* input buffer size */

/*
   Create space for strings and structures. Abort if no memory
   can be allocated.
*/
#define CREATE_STRING(x,l) \
    if ((x = malloc(l)) == (char *) 0) error(2, (char *) 0)
#define CREATE_STRUCT(x,s) \
    if ((x = (struct s *) malloc (sizeof(struct s))) == (struct s*) 0) \
	error(2, (char *) 0)
