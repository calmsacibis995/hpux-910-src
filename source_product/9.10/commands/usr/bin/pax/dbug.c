/******************************************************************************
 *									      *
 *	                           N O T I C E				      *
 *									      *
 *	              Copyright Abandoned, 1987, Fred Fish		      *
 *									      *
 *									      *
 *	This previously copyrighted work has been placed into the  public     *
 *	domain  by  the  author  and  may be freely used for any purpose,     *
 *	private or commercial.						      *
 *									      *
 *	Because of the number of inquiries I was receiving about the  use     *
 *	of this product in commercially developed works I have decided to     *
 *	simply make it public domain to further its unrestricted use.   I     *
 *	specifically  would  be  most happy to see this material become a     *
 *	part of the standard Unix distributions by AT&T and the  Berkeley     *
 *	Computer  Science  Research Group, and a standard part of the GNU     *
 *	system from the Free Software Foundation.			      *
 *									      *
 *	I would appreciate it, as a courtesy, if this notice is  left  in     *
 *	all copies and derivative works.  Thank you.			      *
 *									      *
 *	The author makes no warranty of any kind  with  respect  to  this     *
 *	product  and  explicitly disclaims any implied warranties of mer-     *
 *	chantability or fitness for any particular purpose.		      *
 *									      *
 ******************************************************************************
 */


/*
 *  FILE
 *
 *	dbug.c   runtime support routines for dbug package
 *
 *  SCCS
 *
 *	@(#)dbug.c	1.19 9/5/87
 *
 *  DESCRIPTION
 *
 *	These are the runtime support routines for the dbug package.
 *	The dbug package has two main components; the user include
 *	file containing various macro definitions, and the runtime
 *	support routines which are called from the macro expansions.
 *
 *	Externally visible functions in the runtime support module
 *	use the naming convention pattern "_db_xx...xx_", thus
 *	they are unlikely to collide with user defined function names.
 *
 *  AUTHOR(S)
 *
 *	Fred Fish		(base code)
 *	(Currently at Motorola Computer Division, Tempe, Az.)
 *	hao!noao!mcdsun!fnf
 *	(602) 438-3614
 *
 *	Binayak Banerjee	(profiling enhancements)
 *	seismo!bpa!sjuvax!bbanerje
 */


#include "pax.h"
#include <stdio.h>

#ifdef AMIGA
#define HZ (50)			/* Probably in some header somewhere */
#endif

/*
 *	Manifest constants that should not require any changes.
 */

#define FALSE		0	/* Boolean FALSE */
#define TRUE		1	/* Boolean TRUE */
#define EOS		'\000'	/* End Of String marker */

/*
 *	Manifest constants which may be "tuned" if desired.
 */

#define PRINTBUF	1024	/* Print buffer size */
#define INDENT		4	/* Indentation per trace level */
#define MAXDEPTH	200	/* Maximum trace depth default */

/*
 *	The following flags are used to determine which
 *	capabilities the user has enabled with the state
 *	push macro.
 */

#define TRACE_ON	000001	/* Trace enabled */
#define DEBUG_ON	000002	/* Debug enabled */
#define FILE_ON 	000004	/* File name print enabled */
#define LINE_ON		000010	/* Line number print enabled */
#define DEPTH_ON	000020	/* Function nest level print enabled */
#define PROCESS_ON	000040	/* Process name print enabled */
#define NUMBER_ON	000100	/* Number each line of output */
#define PROFILE_ON	000200	/* Print out profiling code */

#define TRACING (stack -> flags & TRACE_ON)
#define DEBUGGING (stack -> flags & DEBUG_ON)
#define PROFILING (stack -> flags & PROFILE_ON)
#define STREQ(a,b) (strcmp(a,b) == 0)

/*
 *	Typedefs to make things more obvious.
 */

typedef int BOOLEAN;

/*
 *	The following define is for the variable arguments kluge, see
 *	the comments in _db_doprnt_().
 *
 *	Also note that the longer this list, the less prone to failing
 *	on long argument lists, but the more stuff that must be moved
 *	around for each call to the runtime support routines.  The
 *	length may really be critical if the machine convention is
 *	to pass arguments in registers.
 *
 *	Note that the default define allows up to 16 integral arguments,
 *	or 8 floating point arguments (doubles), on most machines.
 *
 *	Someday this may be replaced with true varargs support, when
 *	ANSI C has had time to take root.
 */

#define ARGLIST a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15

/*
 * The default file for profiling.  Could also add another flag
 * (G?) which allowed the user to specify this.
 */

#define PROF_FILE	"dbugmon.out"

/*
 *	Variables which are available externally but should only
 *	be accessed via the macro package facilities.
 */

FILE *_db_fp_ = stderr;		/* Output stream, default stderr */
FILE *_db_pfp_ = (FILE *)0;	/* Profile stream, 'dbugmon.out' */
char *_db_process_ = "dbug";	/* Pointer to process name; argv[0] */
BOOLEAN _db_on_ = FALSE;		/* TRUE if debugging currently on */
BOOLEAN _db_pon_ = FALSE;	/* TRUE if debugging currently on */

/*
 *	Externally supplied functions.
 */

#ifdef unix			/* Only needed for unix */
extern void perror ();		/* Print system/library error */
extern int chown ();		/* Change owner of a file */
extern int getgid ();		/* Get real group id */
extern int getuid ();		/* Get real user id */
extern int access ();		/* Test file for access */
#else
#if !(AMIGA && LATTICE)
static void perror ();		/* Fake system/library error print routine */
#endif
#endif

# if BSD4_3 || sun
extern int getrusage ();
#endif

/*
 *	The user may specify a list of functions to trace or 
 *	debug.  These lists are kept in a linear linked list,
 *	a very simple implementation.
 */

struct db_link {
    char *string;		/* Pointer to link's contents */
    struct db_link *next_link;	/* Pointer to the next link */
};


/*
 *	Debugging states can be pushed or popped off of a
 *	stack which is implemented as a linked list.  Note
 *	that the head of the list is the current state and the
 *	stack is pushed by adding a new state to the head of the
 *	list or popped by removing the first link.
 */

struct state {
    int flags;				/* Current state flags */
    int maxdepth;			/* Current maximum trace depth */
    unsigned int delay;			/* Delay after each output line */
    int level;				/* Current function nesting level */
    FILE *out_file;			/* Current output stream */
    FILE *prof_file;			/* Current profiling stream */
    struct db_link *functions;		/* List of functions */
    struct db_link *p_functions;	/* List of profiled functions */
    struct db_link *keywords;		/* List of debug keywords */
    struct db_link *processes;		/* List of process names */
    struct state *next_state;		/* Next state in the list */
};

static struct state *stack = NULL;	/* Linked list of stacked states */

/*
 *	Local variables not seen by user.
 */

static int lineno = 0;		/* Current debugger output line number */
static char *func = "?func";	/* Name of current user function */
static char *user_file = "?file";	/* Name of current user file */
static BOOLEAN init_done = FALSE;/* Set to TRUE when initialization done */

#if unix || AMIGA
static int jmplevel;		/* Remember nesting level at setjmp () */
static char *jmpfunc;		/* Remember current function for setjmp */
static char *jmpfile;		/* Remember current file for setjmp */
#endif

static struct db_link *ListParse ();/* Parse a debug command string */
static char *StrDup ();		/* Make a fresh copy of a string */
static void OpenFile ();	/* Open debug output stream */
static void OpenProfile ();	/* Open profile output stream */
static void CloseFile ();	/* Close debug output stream */
static void PushState ();	/* Push current debug state */
static void ChangeOwner ();	/* Change file owner and group */
static BOOLEAN DoTrace ();	/* Test for tracing enabled */
static BOOLEAN Writable ();	/* Test to see if file is writable */
static unsigned long Clock ();	/* Return current user time (ms) */
static char *DbugMalloc ();	/* Allocate memory for runtime support */
static char *BaseName ();	/* Remove leading pathname components */
static void DoPrefix ();	/* Print debugger line prefix */
static void FreeList ();	/* Free memory from linked list */
static void Indent ();		/* Indent line to specified indent */
#ifndef STRTOK
static char *strtok ();		/* System V string scanning function */
#endif /* STRTOK */

/*
 *	The following local variables are used to hold the state information
 *	between the call to _db_pargs_() and _db_doprnt_(), during
 *	expansion of the DBUG_PRINT macro.  This is the only macro
 *	that currently uses these variables.  The DBUG_PRINT macro
 *	and the new _db_doprnt_() routine replace the older DBUG_N macros
 *	and their corresponding runtime support routine _db_printf_().
 *
 *	These variables are currently used only by _db_pargs_() and
 *	_db_doprnt_().
 */

static int u_line = 0;		/* User source code line number */
static char *u_keyword = "?";	/* Keyword for current macro */

/*
 *	Miscellaneous printf format strings.
 */
 
#define ERR_MISSING_RETURN "%s: missing DBUG_RETURN or DBUG_VOID_RETURN macro in function \"%s\"\n"
#define ERR_OPEN "%s: can't open debug output stream \"%s\": "
#define ERR_CLOSE "%s: can't close debug file: "
#define ERR_ABORT "%s: debugger aborting because %s\n"
#define ERR_CHOWN "%s: can't change owner/group of \"%s\": "
#define ERR_PRINTF "%s: obsolete object file for '%s', please recompile!\n"

/*
 *	Macros and defines for testing file accessibility under UNIX.
 */

#ifdef unix
#  define A_EXISTS	00		/* Test for file existance */
#  define A_EXECUTE	01		/* Test for execute permission */
#  define A_WRITE	02		/* Test for write access */
#  define A_READ	03		/* Test for read access */
#  define EXISTS(pathname) (access (pathname, A_EXISTS) == 0)
#  define WRITABLE(pathname) (access (pathname, A_WRITE) == 0)
#else
#  define EXISTS(pathname) (FALSE)	/* Assume no existance */
#endif

/*
 *	Translate some calls among different systems.
 */

#ifdef unix
# define Delay sleep
extern unsigned int sleep ();	/* Pause for given number of seconds */
#endif

#ifdef AMIGA
extern int Delay ();		/* Pause for given number of ticks */
#endif


/*
 *  FUNCTION
 *
 *	_db_push_    push current debugger state and set up new one
 *
 *  SYNOPSIS
 *
 *	void _db_push_ (control)
 *	char *control;
 *
 *  DESCRIPTION
 *
 *	Given pointer to a debug control string in "control", pushes
 *	the current debug state, parses the control string, and sets
 *	up a new debug state.
 *
 *	The only attribute of the new state inherited from the previous
 *	state is the current function nesting level.  This can be
 *	overridden by using the "r" flag in the control string.
 *
 *	The debug control string is a sequence of colon separated fields
 *	as follows:
 *
 *		<field_1>:<field_2>:...:<field_N>
 *
 *	Each field consists of a mandatory flag character followed by
 *	an optional "," and comma separated list of modifiers:
 *
 *		flag[,modifier,modifier,...,modifier]
 *
 *	The currently recognized flag characters are:
 *
 *		d	Enable output from DBUG_<N> macros for
 *			for the current state.  May be followed
 *			by a list of keywords which selects output
 *			only for the DBUG macros with that keyword.
 *			A null list of keywords implies output for
 *			all macros.
 *
 *		D	Delay after each debugger output line.
 *			The argument is the number of tenths of seconds
 *			to delay, subject to machine capabilities.
 *			I.E.  -#D,20 is delay two seconds.
 *
 *		f	Limit debugging and/or tracing, and profiling to the
 *			list of named functions.  Note that a null list will
 *			disable all functions.  The appropriate "d" or "t"
 *			flags must still be given, this flag only limits their
 *			actions if they are enabled.
 *
 *		F	Identify the source file name for each
 *			line of debug or trace output.
 *
 *		g	Enable profiling.  Create a file called 'dbugmon.out'
 *			containing information that can be used to profile
 *			the program.  May be followed by a list of keywords
 *			that select profiling only for the functions in that
 *			list.  A null list implies that all functions are
 *			considered.
 *
 *		L	Identify the source file line number for
 *			each line of debug or trace output.
 *
 *		n	Print the current function nesting depth for
 *			each line of debug or trace output.
 *	
 *		N	Number each line of dbug output.
 *
 *		p	Limit debugger actions to specified processes.
 *			A process must be identified with the
 *			DBUG_PROCESS macro and match one in the list
 *			for debugger actions to occur.
 *
 *		P	Print the current process name for each
 *			line of debug or trace output.
 *
 *		r	When pushing a new state, do not inherit
 *			the previous state's function nesting level.
 *			Useful when the output is to start at the
 *			left margin.
 *
 *		t	Enable function call/exit trace lines.
 *			May be followed by a list (containing only
 *			one modifier) giving a numeric maximum
 *			trace level, beyond which no output will
 *			occur for either debugging or tracing
 *			macros.  The default is a compile time
 *			option.
 *
 *	Some examples of debug control strings which might appear
 *	on a shell command line (the "-#" is typically used to
 *	introduce a control string to an application program) are:
 *
 *		-#d:t
 *		-#d:f,main,subr1:F:L:t,20
 *		-#d,input,output,files:n
 *
 *	For convenience, any leading "-#" is stripped off.
 *
 */


void _db_push_ (control)
char *control;
{
    Reg1 char *scan;
    Reg2 struct db_link *temp;

    if (control && *control == '-') {
	if (*++control == '#') {
	    control++;
	}	
    }
    control = StrDup (control);
    PushState ();
    scan = strtok (control, ":");
    for (; scan != NULL; scan = strtok ((char *)NULL, ":")) {
	switch (*scan++) {
	    case 'd': 
		_db_on_ = TRUE;
		stack -> flags |= DEBUG_ON;
		if (*scan++ == ',') {
		    stack -> keywords = ListParse (scan);
		}
	    	break;
	    case 'D': 
		stack -> delay = 0;
		if (*scan++ == ',') {
		    temp = ListParse (scan);
		    stack -> delay = DelayArg (atoi (temp -> string));
		    FreeList (temp);
		}
		break;
	    case 'f': 
		if (*scan++ == ',') {
		    stack -> functions = ListParse (scan);
		}
		break;
	    case 'F': 
		stack -> flags |= FILE_ON;
		break;
	    case 'g': 
		_db_pon_ = TRUE;
		OpenProfile(PROF_FILE);
		stack -> flags |= PROFILE_ON;
		if (*scan++ == ',') {
		    stack -> p_functions = ListParse (scan);
		}
		break;
	    case 'L': 
		stack -> flags |= LINE_ON;
		break;
	    case 'n': 
		stack -> flags |= DEPTH_ON;
		break;
	    case 'N':
		stack -> flags |= NUMBER_ON;
		break;
	    case 'o': 
		if (*scan++ == ',') {
		    temp = ListParse (scan);
		    OpenFile (temp -> string);
		    FreeList (temp);
		} else {
		    OpenFile ("-");
		}
		break;
	    case 'p':
		if (*scan++ == ',') {
		    stack -> processes = ListParse (scan);
		}
		break;
	    case 'P': 
		stack -> flags |= PROCESS_ON;
		break;
	    case 'r': 
		stack -> level = 0;
		break;
	    case 't': 
		stack -> flags |= TRACE_ON;
		if (*scan++ == ',') {
		    temp = ListParse (scan);
		    stack -> maxdepth = atoi (temp -> string);
		    FreeList (temp);
		}
		break;
	}
    }
    free (control);
}



/*
 *  FUNCTION
 *
 *	_db_pop_    pop the debug stack
 *
 *  DESCRIPTION
 *
 *	Pops the debug stack, returning the debug state to its
 *	condition prior to the most recent _db_push_ invocation.
 *	Note that the pop will fail if it would remove the last
 *	valid state from the stack.  This prevents user errors
 *	in the push/pop sequence from screwing up the debugger.
 *	Maybe there should be some kind of warning printed if the
 *	user tries to pop too many states.
 *
 */

void _db_pop_ ()
{
    Reg1 struct state *discard;

    discard = stack;
    if (discard != NULL && discard -> next_state != NULL) {
	stack = discard -> next_state;
	_db_fp_ = stack -> out_file;
	_db_pfp_ = stack -> prof_file;
	if (discard -> keywords != NULL) {
	    FreeList (discard -> keywords);
	}
	if (discard -> functions != NULL) {
	    FreeList (discard -> functions);
	}
	if (discard -> processes != NULL) {
	    FreeList (discard -> processes);
	}
	if (discard -> p_functions != NULL) {
	    FreeList (discard -> p_functions);
	}
	CloseFile (discard -> out_file);
	CloseFile (discard -> prof_file);
	free ((char *) discard);
    }
}


/*
 *  FUNCTION
 *
 *	_db_enter_    process entry point to user function
 *
 *  SYNOPSIS
 *
 *	void _db_enter_ (_func_, _file_, _line_, _sfunc_, _sfile_, _slevel_)
 *	char *_func_;		points to current function name
 *	char *_file_;		points to current file name
 *	int _line_;		called from source line number
 *	char **_sfunc_;		save previous _func_
 *	char **_sfile_;		save previous _file_
 *	int *_slevel_;		save previous nesting level
 *
 *  DESCRIPTION
 *
 *	Called at the beginning of each user function to tell
 *	the debugger that a new function has been entered.
 *	Note that the pointers to the previous user function
 *	name and previous user file name are stored on the
 *	caller's stack (this is why the ENTER macro must be
 *	the first "executable" code in a function, since it
 *	allocates these storage locations).  The previous nesting
 *	level is also stored on the callers stack for internal
 *	self consistency checks.
 *
 *	Also prints a trace line if tracing is enabled and
 *	increments the current function nesting depth.
 *
 *	Note that this mechanism allows the debugger to know
 *	what the current user function is at all times, without
 *	maintaining an internal stack for the function names.
 *
 */

void _db_enter_ (_func_, _file_, _line_, _sfunc_, _sfile_, _slevel_)
char *_func_;
char *_file_;
int _line_;
char **_sfunc_;
char **_sfile_;
int *_slevel_;
{
    if (!init_done) {
	_db_push_ ("");
    }
    *_sfunc_ = func;
    *_sfile_ = user_file;
    func = _func_;
    user_file = BaseName (_file_);
    stack -> level++;
    *_slevel_ = stack -> level;
    if (DoProfile ()) {
	(void) fprintf (_db_pfp_, "%s\tE\t%ld\n",func, Clock());
	(void) fflush (_db_pfp_);
    }
    if (DoTrace ()) {
	DoPrefix (_line_);
	Indent (stack -> level);
	(void) fprintf (_db_fp_, ">%s\n", func);
	(void) fflush (_db_fp_);
	(void) Delay (stack -> delay);
    }
}


/*
 *  FUNCTION
 *
 *	_db_return_    process exit from user function
 *
 *  SYNOPSIS
 *
 *	void _db_return_ (_line_, _sfunc_, _sfile_, _slevel_)
 *	int _line_;		current source line number
 *	char **_sfunc_;		where previous _func_ is to be retrieved
 *	char **_sfile_;		where previous _file_ is to be retrieved
 *	int *_slevel_;		where previous level was stashed
 *
 *  DESCRIPTION
 *
 *	Called just before user function executes an explicit or implicit
 *	return.  Prints a trace line if trace is enabled, decrements
 *	the current nesting level, and restores the current function and
 *	file names from the defunct function's stack.
 *
 */

void _db_return_ (_line_, _sfunc_, _sfile_, _slevel_)
int _line_;
char **_sfunc_;
char **_sfile_;
int *_slevel_;
{
    if (!init_done) {
	_db_push_ ("");
    }
    if (stack -> level != *_slevel_ && (TRACING || DEBUGGING || PROFILING)) {
	(void) fprintf (_db_fp_, ERR_MISSING_RETURN, _db_process_, func);
    } else if (DoProfile ()) {
	(void) fprintf (_db_pfp_, "%s\tX\t%ld\n", func, Clock());
    } else if (DoTrace ()) {
	DoPrefix (_line_);
	Indent (stack -> level);
	(void) fprintf (_db_fp_, "<%s\n", func);
    }
    (void) fflush (_db_fp_);
    (void) Delay (stack -> delay);
    stack -> level = *_slevel_ - 1;
    func = *_sfunc_;
    user_file = *_sfile_;
}


/*
 *  FUNCTION
 *
 *	_db_pargs_    log arguments for subsequent use by _db_doprnt_()
 *
 *  SYNOPSIS
 *
 *	void _db_pargs_ (_line_, keyword)
 *	int _line_;
 *	char *keyword;
 *
 *  DESCRIPTION
 *
 *	The new universal printing macro DBUG_PRINT, which replaces
 *	all forms of the DBUG_N macros, needs two calls to runtime
 *	support routines.  The first, this function, remembers arguments
 *	that are used by the subsequent call to _db_doprnt_().
*
 */

void _db_pargs_ (_line_, keyword)
int _line_;
char *keyword;
{
    u_line = _line_;
    u_keyword = keyword;
}


/*
 *  FUNCTION
 *
 *	_db_doprnt_    handle print of debug lines
 *
 *  SYNOPSIS
 *
 *	void _db_doprnt_ (format, ARGLIST)
 *	char *format;
 *	long ARGLIST;
 *
 *  DESCRIPTION
 *
 *	When invoked via one of the DBUG macros, tests the current keyword
 *	set by calling _db_pargs_() to see if that macro has been selected
 *	for processing via the debugger control string, and if so, handles
 *	printing of the arguments via the format string.  The line number
 *	of the DBUG macro in the source is found in u_line.
 *
 *	Note that the format string SHOULD NOT include a terminating
 *	newline, this is supplied automatically.
 *
 *  NOTES
 *
 *	This runtime support routine replaces the older _db_printf_()
 *	routine which is temporarily kept around for compatibility.
 *
 *	The rather ugly argument declaration is to handle some
 *	magic with respect to the number of arguments passed
 *	via the DBUG macros.  The current maximum is 3 arguments
 *	(not including the keyword and format strings).
 *
 *	The new <varargs.h> facility is not yet common enough to
 *	convert to it quite yet...
 *
 */

/*VARARGS1*/
void _db_doprnt_ (format, ARGLIST)
char *format;
long ARGLIST;
{
    if (_db_keyword_ (u_keyword)) {
	DoPrefix (u_line);
	if (TRACING) {
	    Indent (stack -> level + 1);
	} else {
	    (void) fprintf (_db_fp_, "%s: ", func);
	}
	(void) fprintf (_db_fp_, "%s: ", u_keyword);
	(void) fprintf (_db_fp_, format, ARGLIST);
	(void) fprintf (_db_fp_, "\n");
	(void) fflush (_db_fp_);
	(void) Delay (stack -> delay);
    }
}

/*
 *	The following routine is kept around temporarily for compatibility
 *	with older objects that were compiled with the DBUG_N macro form
 *	of the print routine.  It will print a warning message on first
 *	usage.  It will go away in subsequent releases...
 */

/*VARARGS3*/
void _db_printf_ (_line_, keyword, format, ARGLIST)
int _line_;
char *keyword,  *format;
long ARGLIST;
{
    static BOOLEAN firsttime = TRUE;

    if (firsttime) {
	(void) fprintf (stderr, ERR_PRINTF, _db_process_, user_file);
	firsttime = FALSE;
    }
    _db_pargs_ (_line_, keyword);
    _db_doprnt_ (format, ARGLIST);
}


/*
 *  FUNCTION
 *
 *	ListParse    parse list of modifiers in debug control string
 *
 *  SYNOPSIS
 *
 *	static struct db_link *ListParse (ctlp)
 *	char *ctlp;
 *
 *  DESCRIPTION
 *
 *	Given pointer to a comma separated list of strings in "cltp",
 *	parses the list, building a list and returning a pointer to it.
 *	The original comma separated list is destroyed in the process of
 *	building the linked list, thus it had better be a duplicate
 *	if it is important.
 *
 *	Note that since each link is added at the head of the list,
 *	the final list will be in "reverse order", which is not
 *	significant for our usage here.
 *
 */

static struct db_link *ListParse (ctlp)
char *ctlp;
{
    Reg1 char *start;
    Reg2 struct db_link *new;
    Reg3 struct db_link *head;

    head = NULL;
    while (*ctlp != EOS) {
	start = ctlp;
	while (*ctlp != EOS && *ctlp != ',') {
	    ctlp++;
	}
	if (*ctlp == ',') {
	    *ctlp++ = EOS;
	}
	new = (struct db_link *) DbugMalloc (sizeof (struct db_link));
	new -> string = StrDup (start);
	new -> next_link = head;
	head = new;
    }
    return (head);
}


/*
 *  FUNCTION
 *
 *	InList    test a given string for member of a given list
 *
 *  SYNOPSIS
 *
 *	static BOOLEAN InList (linkp, cp)
 *	struct db_link *linkp;
 *	char *cp;
 *
 *  DESCRIPTION
 *
 *	Tests the string pointed to by "cp" to determine if it is in
 *	the list pointed to by "linkp".  Linkp points to the first
 *	link in the list.  If linkp is NULL then the string is treated
 *	as if it is in the list (I.E all strings are in the null list).
 *	This may seem rather strange at first but leads to the desired
 *	operation if no list is given.  The net effect is that all
 *	strings will be accepted when there is no list, and when there
 *	is a list, only those strings in the list will be accepted.
 *
 */

static BOOLEAN InList (linkp, cp)
struct db_link *linkp;
char *cp;
{
    Reg1 struct db_link *scan;
    Reg2 BOOLEAN accept;

    if (linkp == NULL) {
	accept = TRUE;
    } else {
	accept = FALSE;
	for (scan = linkp; scan != NULL; scan = scan -> next_link) {
	    if (STREQ (scan -> string, cp)) {
		accept = TRUE;
		break;
	    }
	}
    }
    return (accept);
}


/*
 *  FUNCTION
 *
 *	PushState    push current state onto stack and set up new one
 *
 *  SYNOPSIS
 *
 *	static void PushState ()
 *
 *  DESCRIPTION
 *
 *	Pushes the current state on the state stack, and initializes
 *	a new state.  The only parameter inherited from the previous
 *	state is the function nesting level.  This action can be
 *	inhibited if desired, via the "r" flag.
 *
 *	The state stack is a linked list of states, with the new
 *	state added at the head.  This allows the stack to grow
 *	to the limits of memory if necessary.
 *
 */

static void PushState ()
{
    Reg1 struct state *new;

    new = (struct state *) DbugMalloc (sizeof (struct state));
    new -> flags = 0;
    new -> delay = 0;
    new -> maxdepth = MAXDEPTH;
    if (stack != NULL) {
	new -> level = stack -> level;
    } else {
	new -> level = 0;
    }
    new -> out_file = stderr;
    new -> functions = NULL;
    new -> p_functions = NULL;
    new -> keywords = NULL;
    new -> processes = NULL;
    new -> next_state = stack;
    stack = new;
    init_done = TRUE;
}


/*
 *  FUNCTION
 *
 *	DoTrace    check to see if tracing is current enabled
 *
 *  SYNOPSIS
 *
 *	static BOOLEAN DoTrace ()
 *
 *  DESCRIPTION
 *
 *	Checks to see if tracing is enabled based on whether the
 *	user has specified tracing, the maximum trace depth has
 *	not yet been reached, the current function is selected,
 *	and the current process is selected.  Returns TRUE if
 *	tracing is enabled, FALSE otherwise.
 *
 */

static BOOLEAN DoTrace ()
{
    Reg1 BOOLEAN trace;

    trace = FALSE;
    if (TRACING) {
	if (stack -> level <= stack -> maxdepth) {
	    if (InList (stack -> functions, func)) {
		if (InList (stack -> processes, _db_process_)) {
		    trace = TRUE;
		}
	    }
	}
    }
    return (trace);
}


/*
 *  FUNCTION
 *
 *	DoProfile    check to see if profiling is current enabled
 *
 *  SYNOPSIS
 *
 *	static BOOLEAN DoProfile ()
 *
 *  DESCRIPTION
 *
 *	Checks to see if profiling is enabled based on whether the
 *	user has specified profiling, the maximum trace depth has
 *	not yet been reached, the current function is selected,
 *	and the current process is selected.  Returns TRUE if
 *	profiling is enabled, FALSE otherwise.
 *
 */

static BOOLEAN DoProfile ()
{
    Reg1 BOOLEAN profile;

    profile = FALSE;
    if (PROFILING) {
	if (stack -> level <= stack -> maxdepth) {
	    if (InList (stack -> p_functions, func)) {
		if (InList (stack -> processes, _db_process_)) {
		    profile = TRUE;
		}
	    }
	}
    }
    return (profile);
}


/*
 *  FUNCTION
 *
 *	_db_keyword_    test keyword for member of keyword list
 *
 *  SYNOPSIS
 *
 *	BOOLEAN _db_keyword_ (keyword)
 *	char *keyword;
 *
 *  DESCRIPTION
 *
 *	Test a keyword to determine if it is in the currently active
 *	keyword list.  As with the function list, a keyword is accepted
 *	if the list is null, otherwise it must match one of the list
 *	members.  When debugging is not on, no keywords are accepted.
 *	After the maximum trace level is exceeded, no keywords are
 *	accepted (this behavior subject to change).  Additionally,
 *	the current function and process must be accepted based on
 *	their respective lists.
 *
 *	Returns TRUE if keyword accepted, FALSE otherwise.
 *
 */

BOOLEAN _db_keyword_ (keyword)
char *keyword;
{
    Reg1 BOOLEAN accept;

    if (!init_done) {
	_db_push_ ("");
    }
    accept = FALSE;
    if (DEBUGGING) {
	if (stack -> level <= stack -> maxdepth) {
	    if (InList (stack -> functions, func)) {
		if (InList (stack -> keywords, keyword)) {
		    if (InList (stack -> processes, _db_process_)) {
			accept = TRUE;
		    }
		}
	    }
	}
    }
    return (accept);
}


/*
 *  FUNCTION
 *
 *	Indent    indent a line to the given indentation level
 *
 *  SYNOPSIS
 *
 *	static void Indent (indent)
 *	int indent;
 *
 *  DESCRIPTION
 *
 *	Indent a line to the given level.  Note that this is
 *	a simple minded but portable implementation.
 *	There are better ways.
 *
 *	Also, the indent must be scaled by the compile time option
 *	of character positions per nesting level.
 *
 */

static void Indent (indent)
int indent;
{
    Reg1 int count;
    auto char buffer[PRINTBUF];

    indent *= INDENT;
    for (count = 0; (count < (indent - INDENT)) && (count < (PRINTBUF - 1)); count++) {
	if ((count % INDENT) == 0) {
	    buffer[count] = '|';
	} else {
	    buffer[count] = ' ';
	}
    }
    buffer[count] = EOS;
    (void) fprintf (_db_fp_, buffer);
    (void) fflush (_db_fp_);
}


/*
 *  FUNCTION
 *
 *	FreeList    free all memory associated with a linked list
 *
 *  SYNOPSIS
 *
 *	static void FreeList (linkp)
 *	struct db_link *linkp;
 *
 *  DESCRIPTION
 *
 *	Given pointer to the head of a linked list, frees all
 *	memory held by the list and the members of the list.
 *
 */

static void FreeList (linkp)
struct db_link *linkp;
{
    Reg1 struct db_link *old;

    while (linkp != NULL) {
	old = linkp;
	linkp = linkp -> next_link;
	if (old -> string != NULL) {
	    free (old -> string);
	}
	free ((char *) old);
    }
}


/*
 *  FUNCTION
 *
 *	StrDup   make a duplicate of a string in new memory
 *
 *  SYNOPSIS
 *
 *	static char *StrDup (string)
 *	char *string;
 *
 *  DESCRIPTION
 *
 *	Given pointer to a string, allocates sufficient memory to make
 *	a duplicate copy, and copies the string to the newly allocated
 *	memory.  Failure to allocated sufficient memory is immediately
 *	fatal.
 *
 */


static char *StrDup (string)
char *string;
{
    Reg1 char *new;

    new = DbugMalloc (strlen (string) + 1);
    (void) strcpy (new, string);
    return (new);
}


/*
 *  FUNCTION
 *
 *	DoPrefix    print debugger line prefix prior to indentation
 *
 *  SYNOPSIS
 *
 *	static void DoPrefix (_line_)
 *	int _line_;
 *
 *  DESCRIPTION
 *
 *	Print prefix common to all debugger output lines, prior to
 *	doing indentation if necessary.  Print such information as
 *	current process name, current source file name and line number,
 *	and current function nesting depth.
 *
 */
  
 
static void DoPrefix (_line_)
int _line_;
{
    lineno++;
    if (stack -> flags & NUMBER_ON) {
	(void) fprintf (_db_fp_, "%5d: ", lineno);
    }
    if (stack -> flags & PROCESS_ON) {
	(void) fprintf (_db_fp_, "%s: ", _db_process_);
    }
    if (stack -> flags & FILE_ON) {
	(void) fprintf (_db_fp_, "%14s: ", user_file);
    }
    if (stack -> flags & LINE_ON) {
	(void) fprintf (_db_fp_, "%5d: ", _line_);
    }
    if (stack -> flags & DEPTH_ON) {
	(void) fprintf (_db_fp_, "%4d: ", stack -> level);
    }
    (void) fflush (_db_fp_);
}


/*
 *  FUNCTION
 *
 *	OpenFile    open new output stream for debugger output
 *
 *  SYNOPSIS
 *
 *	static void OpenFile (name)
 *	char *name;
 *
 *  DESCRIPTION
 *
 *	Given name of a new file (or "-" for stdout) opens the file
 *	and sets the output stream to the new file.
 *
 */

static void OpenFile (name)
char *name;
{
    Reg1 FILE *fp;
    Reg2 BOOLEAN newfile;

    if (name != NULL) {
	if (strcmp (name, "-") == 0) {
	    _db_fp_ = stdout;
	    stack -> out_file = _db_fp_;
	} else {
	    if (!Writable (name)) {
		(void) fprintf (_db_fp_, ERR_OPEN, _db_process_, name);
		perror ("");
		(void) fflush (_db_fp_);
		(void) Delay (stack -> delay);
	    } else {
		if (EXISTS (name)) {
		    newfile = FALSE;
		} else {
		    newfile = TRUE;
		}
		fp = fopen (name, "a");
		if (fp == NULL) {
 		    (void) fprintf (_db_fp_, ERR_OPEN, _db_process_, name);
		    perror ("");
		    (void) fflush (_db_fp_);
		    (void) Delay (stack -> delay);
		} else {
		    _db_fp_ = fp;
		    stack -> out_file = fp;
		    if (newfile) {
			ChangeOwner (name);
		    }
		}
	    }
	}
    }
}


/*
 *  FUNCTION
 *
 *	OpenProfile    open new output stream for profiler output
 *
 *  SYNOPSIS
 *
 *	static void OpenProfile (name)
 *	char *name;
 *
 *  DESCRIPTION
 *
 *	Given name of a new file, opens the file
 *	and sets the profiler output stream to the new file.
 *
 *	It is currently unclear whether the prefered behavior is
 *	to truncate any existing file, or simply append to it.
 *	The latter behavior would be desirable for collecting
 *	accumulated runtime history over a number of separate
 *	runs.  It might take some changes to the analyzer program
 *	though, and the notes that Binayak sent with the profiling
 *	diffs indicated that append was the normal mode, but this
 *	does not appear to agree with the actual code. I haven't
 *	investigated at this time [fnf; 24-Jul-87].
 */

static void OpenProfile (name)
char *name;
{
    Reg1 FILE *fp;
    Reg2 BOOLEAN newfile;

    if (name != NULL) {
	if (!Writable (name)) {
	    (void) fprintf (_db_fp_, ERR_OPEN, _db_process_, name);
	    perror ("");
	    (void) fflush (_db_fp_);
	    (void) Delay (stack -> delay);
	} else {
	    if (EXISTS (name)) {
		newfile = FALSE;
	    } else {
		newfile = TRUE;
	    }
	    fp = fopen (name, "w");
	    if (fp == NULL) {
		(void) fprintf (_db_fp_, ERR_OPEN, _db_process_, name);
		perror ("");
		(void) fflush (_db_fp_);
		(void) Delay (stack -> delay);
	    } else {
		_db_pfp_ = fp;
		stack -> prof_file = fp;
		if (newfile) {
		    ChangeOwner (name);
		}
	    }
	}
    }
}


/*
 *  FUNCTION
 *
 *	CloseFile    close the debug output stream
 *
 *  SYNOPSIS
 *
 *	static void CloseFile (fp)
 *	FILE *fp;
 *
 *  DESCRIPTION
 *
 *	Closes the debug output stream unless it is standard output
 *	or standard error.
 *
 */

static void CloseFile (fp)
FILE *fp;
{
    if (fp != stderr && fp != stdout) {
	if (fclose (fp) == EOF) {
	    (void) fprintf (stderr, ERR_CLOSE, _db_process_);
	    perror ("");
	    (void) fflush (stderr);
	    (void) Delay (stack -> delay);
	}
    }
}


/*
 *  FUNCTION
 *
 *	DbugExit    print error message and exit
 *
 *  SYNOPSIS
 *
 *	static void DbugExit (why)
 *	char *why;
 *
 *  DESCRIPTION
 *
 *	Prints error message using current process name, the reason for
 *	aborting (typically out of memory), and exits with status 1.
 *	This should probably be changed to use a status code
 *	defined in the user's debugger include file.
 *
 */
 
static void DbugExit (why)
char *why;
{
    (void) fprintf (stderr, ERR_ABORT, _db_process_, why);
    (void) fflush (stderr);
    (void) Delay (stack -> delay);
    exit (1);
}


/*
 *  FUNCTION
 *
 *	DbugMalloc    allocate memory for debugger runtime support
 *
 *  SYNOPSIS
 *
 *	static char *DbugMalloc (size)
 *	int size;
 *
 *  DESCRIPTION
 *
 *	Allocate more memory for debugger runtime support functions.
 *	Failure to to allocate the requested number of bytes is
 *	immediately fatal to the current process.  This may be
 *	rather unfriendly behavior.  It might be better to simply
 *	print a warning message, freeze the current debugger state,
 *	and continue execution.
 *
 */
 
static char *DbugMalloc (size)
int size;
{
    Reg1 char *new;

    new = malloc ((unsigned int) size);
    if (new == NULL) {
	DbugExit ("out of memory");
    }
    return (new);
}


#ifndef STRTOK
/*
 *	This function may be eliminated when strtok is available
 *	in the runtime environment (missing from BSD4.1).
 */

static char *strtok (s1, s2)
char *s1, *s2;
{
    static char *end = NULL;
    Reg1 char *rtnval;

    rtnval = NULL;
    if (s2 != NULL) {
	if (s1 != NULL) {
	    end = s1;
	    rtnval = strtok ((char *) NULL, s2);
	} else if (end != NULL) {
	    if (*end != EOS) {
		rtnval = end;
		while (*end != *s2 && *end != EOS) {end++;}
		if (*end != EOS) {
		    *end++ = EOS;
		}
	    }
	}
    }
    return (rtnval);
}
#endif /* STRTOK */


/*
 *  FUNCTION
 *
 *	BaseName    strip leading pathname components from name
 *
 *  SYNOPSIS
 *
 *	static char *BaseName (pathname)
 *	char *pathname;
 *
 *  DESCRIPTION
 *
 *	Given pointer to a complete pathname, locates the base file
 *	name at the end of the pathname and returns a pointer to
 *	it.
 *
 */

static char *BaseName (pathname)
char *pathname;
{
    Reg1 char *base;

    base = rindex (pathname, '/');
    if (base++ == NULL) {
	base = pathname;
    }
    return (base);
}


/*
 *  FUNCTION
 *
 *	Writable    test to see if a pathname is writable/creatable
 *
 *  SYNOPSIS
 *
 *	static BOOLEAN Writable (pathname)
 *	char *pathname;
 *
 *  DESCRIPTION
 *
 *	Because the debugger might be linked in with a program that
 *	runs with the set-uid-bit (suid) set, we have to be careful
 *	about opening a user named file for debug output.  This consists
 *	of checking the file for write access with the real user id,
 *	or checking the directory where the file will be created.
 *
 *	Returns TRUE if the user would normally be allowed write or
 *	create access to the named file.  Returns FALSE otherwise.
 *
 */

static BOOLEAN Writable (pathname)
char *pathname;
{
    Reg1 BOOLEAN granted;
#ifdef unix
    Reg2 char *lastslash;
#endif

#ifndef unix
    granted = TRUE;
#else
    granted = FALSE;
    if (EXISTS (pathname)) {
	if (WRITABLE (pathname)) {
	    granted = TRUE;
	}
    } else {
	lastslash = rindex (pathname, '/');
	if (lastslash != NULL) {
	    *lastslash = EOS;
	} else {
	    pathname = ".";
	}
	if (WRITABLE (pathname)) {
	    granted = TRUE;
	}
	if (lastslash != NULL) {
	    *lastslash = '/';
	}
    }
#endif
    return (granted);
}


/*
 *  FUNCTION
 *
 *	ChangeOwner    change owner to real user for suid programs
 *
 *  SYNOPSIS
 *
 *	static void ChangeOwner (pathname)
 *
 *  DESCRIPTION
 *
 *	For unix systems, change the owner of the newly created debug
 *	file to the real owner.  This is strictly for the benefit of
 *	programs that are running with the set-user-id bit set.
 *
 *	Note that at this point, the fact that pathname represents
 *	a newly created file has already been established.  If the
 *	program that the debugger is linked to is not running with
 *	the suid bit set, then this operation is redundant (but
 *	harmless).
 *
 */

static void ChangeOwner (pathname)
char *pathname;
{
#ifdef unix
    if (chown (pathname, getuid (), getgid ()) == -1) {
	(void) fprintf (stderr, ERR_CHOWN, _db_process_, pathname);
	perror ("");
	(void) fflush (stderr);
	(void) Delay (stack -> delay);
    }
#endif
}


/*
 *  FUNCTION
 *
 *	_db_setjmp_    save debugger environment
 *
 *  SYNOPSIS
 *
 *	void _db_setjmp_ ()
 *
 *  DESCRIPTION
 *
 *	Invoked as part of the user's DBUG_SETJMP macro to save
 *	the debugger environment in parallel with saving the user's
 *	environment.
 *
 */

void _db_setjmp_ ()
{
   jmplevel = stack -> level;
   jmpfunc = func;
   jmpfile = user_file;
}


/*
 *  FUNCTION
 *
 *	_db_longjmp_    restore previously saved debugger environment
 *
 *  SYNOPSIS
 *
 *	void _db_longjmp_ ()
 *
 *  DESCRIPTION
 *
 *	Invoked as part of the user's DBUG_LONGJMP macro to restore
 *	the debugger environment in parallel with restoring the user's
 *	previously saved environment.
 *
 */

void _db_longjmp_ ()
{
    stack -> level = jmplevel;
    if (jmpfunc) {
	func = jmpfunc;
    }
    if (jmpfile) {
	user_file = jmpfile;
    }
}


/*
 *  FUNCTION
 *
 *	DelayArg   convert D flag argument to appropriate value
 *
 *  SYNOPSIS
 *
 *	static int DelayArg (value)
 *	int value;
 *
 *  DESCRIPTION
 *
 *	Converts delay argument, given in tenths of a second, to the
 *	appropriate numerical argument used by the system to delay
 *	that that many tenths of a second.  For example, on the
 *	AMIGA, there is a system call "Delay()" which takes an
 *	argument in ticks (50 per second).  On unix, the sleep
 *	command takes seconds.  Thus a value of "10", for one
 *	second of delay, gets converted to 50 on the amiga, and 1
 *	on unix.  Other systems will need to use a timing loop.
 *
 */

static int DelayArg (value)
int value;
{
    int delayarg = 0;
    
#ifdef unix
    delayarg = value / 10;		/* Delay is in seconds for sleep () */
#endif
#ifdef AMIGA
    delayarg = (HZ * value) / 10;	/* Delay in ticks for Delay () */
#endif
    return (delayarg);
}


/*
 *	A dummy delay stub for systems that do not support delays.
 *	With a little work, this can be turned into a timing loop.
 */

#ifndef unix
#ifndef AMIGA
Delay ()
{
}
#endif
#endif


/*
 *  FUNCTION
 *
 *	perror    perror simulation for systems that don't have it
 *
 *  SYNOPSIS
 *
 *	static void perror (s)
 *	char *s;
 *
 *  DESCRIPTION
 *
 *	Perror produces a message on the standard error stream which
 *	provides more information about the library or system error
 *	just encountered.  The argument string s is printed, followed
 *	by a ':', a blank, and then a message and a newline.
 *
 *	An undocumented feature of the unix perror is that if the string
 *	's' is a null string (NOT a NULL pointer!), then the ':' and
 *	blank are not printed.
 *
 *	This version just complains about an "unknown system error".
 *
 */

#if !unix && !(AMIGA && LATTICE)
static void perror (s)
char *s;
{
    if (s && *s != EOS) {
	(void) fprintf (stderr, "%s: ", s);
    }
    (void) fprintf (stderr, "<unknown system error>\n");
}
#endif	/* !unix && !(AMIGA && LATTICE) */

/*
 * Here we need the definitions of the clock routine.  Add your
 * own for whatever system that you have.
 */

#if unix

# include <sys/param.h>
# if BSD4_3 || sun

/*
 * Definition of the Clock() routine for 4.3 BSD.
 */

#include <sys/time.h>
#include <sys/resource.h>

/*
 * Returns the user time in milliseconds used by this process so
 * far.
 */

static unsigned long Clock ()
{
    struct rusage ru;

    (void) getrusage (RUSAGE_SELF, &ru);
    return ((ru.ru_utime.tv_sec * 1000) + (ru.ru_utime.tv_usec / 1000));
}

#else

static unsigned long Clock ()
{
    return (0);
}

# endif

#else

#if AMIGA

struct DateStamp {		/* Yes, this is a hack, but doing it right */
	long ds_Days;		/* is incredibly ugly without splitting this */
	long ds_Minute;		/* off into a separate file */
	long ds_Tick;
};

static int first_clock = TRUE;
static struct DateStamp begin;
static struct DateStamp elapsed;

static unsigned long Clock ()
{
    Reg1 struct DateStamp *now;
    Reg1 unsigned long millisec = 0;
    extern void *AllocMem ();

    now = (struct DateStamp *) AllocMem ((long) sizeof (struct DateStamp), 0L);
    if (now != NULL) {
	if (first_clock == TRUE) {
	    first_clock = FALSE;
	    (void) DateStamp (now);
	    begin = *now;
	}
	(void) DateStamp (now);
	millisec = 24 * 3600 * (1000 / HZ) * (now -> ds_Days - begin.ds_Days);
	millisec += 60 * (1000 / HZ) * (now -> ds_Minute - begin.ds_Minute);
	millisec += (1000 / HZ) * (now -> ds_Tick - begin.ds_Tick);
	(void) FreeMem (now, (long) sizeof (struct DateStamp));
    }
    return (millisec);
}

#endif	/* AMIGA */
#endif	/* unix */
