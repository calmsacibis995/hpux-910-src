/* $Source: /misc/source_product/9.10/commands.rcs/usr/lib/libio/io_status.c,v $
 * $Revision: 70.2 $        $Author: ssa $
 * $State: Exp $        $Locker:  $
 * $Date: 92/04/07 15:00:57 $
 */

/* AUTOBAHN: fix these when files are put in right place */
#include <sys/libio.h>
#include <sys/ioconfig.h>
#include <string.h>
#include <stdio.h>
#include <varargs.h>
#include <machine/dconfig.h>

/*
 * Library routines to support other libio.a functions. Included are:
 *
 * print_libIO_status    - Print a message to stderr describing a libio status.
 * do_print_libIO_status - Non-varargs version of print_libIO_status.
 */

typedef struct {
	int  value;	/* status value (see libio.h) */
	char *name;	/* corresponding name */
	char *format;	/* printf format spec for message */
} libIO_status;

#ifdef DEBUG

typedef struct {
    char    *name;	/* libIO function name */
    int	    number;	/* corresponding function id number (see libio.h) */
} func_map;

#define _UNKNOWN_FUNCTION    -1

#ifdef _WSIO
static func_map libIO_func[] = {

#ifdef __hp9000s700
	{"io_get_table",	    _IO_GET_TABLE},
#endif  /* hp9000s700 */
	{"io_init",		    _IO_INIT},
	{"io_search_isc",	    _IO_SEARCH_ISC},
	{"string_to_hdw_path",	    _STRING_TO_HDW_PATH},
	{"unknown function",	    _UNKNOWN_FUNCTION}
};

#else /* _WSIO */
static func_map libIO_func[] = {
	{"io_add_lu",		    _IO_ADD_LU},
	{"io_add_path",		    _IO_ADD_PATH},
	{"io_add_tree_node",	    _IO_ADD_TREE_NODE},
	{"io_check_connectivity",   _IO_CHECK_CONNECTIVITY},
	{"io_get_connectivity",	    _IO_GET_CONNECTIVITY},
	{"io_get_mod_info",	    _IO_GET_MOD_INFO},
	{"io_get_table",	    _IO_GET_TABLE},
	{"io_init",		    _IO_INIT},
	{"io_probe_hardware",	    _IO_PROBE_HARDWARE},
	{"io_remove_tree_node",	    _IO_REMOVE_TREE_NODE},
	{"io_search_tree",	    _IO_SEARCH_TREE},
	{"ioconfig_close",	    _IOCONFIG_CLOSE},
	{"ioconfig_open",	    _IOCONFIG_OPEN},
	{"ioconfig_read",	    _IOCONFIG_READ},
	{"ioconfig_write",	    _IOCONFIG_WRITE},
	{"update_ioconfig",	    _UPDATE_IOCONFIG},
	{"string_to_hdw_path",	    _STRING_TO_HDW_PATH},
	{"string_to_mod_path",	    _STRING_TO_MOD_PATH},
	{"unknown function",	    _UNKNOWN_FUNCTION}
};
#endif

#endif /* DEBUG */

#define UNKNOWN_ERROR	-2000	    /* marker for the end of the table */
#define UNKNOWN_WARNING	0x40000000  /* marker for the end of the table */

#ifdef _WSIO
static libIO_status libIO_error[] = {
	{SUCCESS,		"SUCCESS",		"No error!?\n"},
	{ALREADY_OPEN,		"ALREADY_OPEN",		"/dev/config is already open\n"},
	{BAD_ADDRESS_FORMAT,	"BAD_ADDRESS_FORMAT",	"Invalid hardware path -- %s -- element %d\n"},
	{INVALID_KEY,		"INVALID_KEY",		"Invalid \"search_key\" argument (%u)\n"},
	{INVALID_ENTRY_DATA,	"INVALID_ENTRY_DATA",	"Invalid data in isc_entry"},
	{INVALID_QUAL,		"INVALID_QUAL",		"Invalid \"search_qual\" argument (0x%x)\n"},
	{INVALID_TYPE,		"INVALID_TYPE",		"Invalid %s argument (%u)\n"},
	{LENGTH_OUT_OF_RANGE,	"LENGTH_OUT_OF_RANGE",	"%s path length invalid -- %s %d %s\n"},
	{NO_HARDWARE_PRESENT,	"NO_HARDWARE_PRESENT",	"No hardware at %s\n"},
	{NO_MATCH,		"NO_MATCH",		"No such device in the system\n"},
	{NO_SEARCH_FIRST,	"NO_SEARCH_FIRST",	"No SEARCH_FIRST before SEARCH_NEXT\n"},
	{NOT_OPEN,		"NOT_OPEN",		"/dev/config is not open\n"},
	{OUT_OF_MEMORY,		"OUT_OF_MEMORY",	"Unable to allocate memory\n"},
	{PERMISSION_DENIED,	"PERMISSION_DENIED",	"%s permission denied on %s\n"},
	{NO_SUCH_FILE,		"NO_SUCH_FILE",		"/dev/config does not exist\n"},
	{SYSCALL_ERROR,		"SYSCALL_ERROR",	"System call error\n"},
	{INVALID_FLAG,		"INVALID_FLAG",		"Invalid \"flag\" argument (%u)\n"},
	{UNKNOWN_ERROR,		"UNKNOWN_ERROR",	"Unknown libIO error (%d)\n"}
};

static libIO_status libIO_warning[] = {
	{END_OF_SEARCH,		"END_OF_SEARCH",	"End of search"},
	{UNKNOWN_WARNING,	"UNKNOWN_WARNING",	"Unknown libIO warning (0x%x)\n"}
};

#else /* _WSIO */
static libIO_status libIO_error[] = {
	{SUCCESS,		"SUCCESS",		"No error!?\n"},
	{ALREADY_OPEN,		"ALREADY_OPEN",		"/dev/config is already open\n"},
	{BAD_ADDRESS_FORMAT,	"BAD_ADDRESS_FORMAT",	"Invalid hardware path -- %s -- element %d\n"},
	{BAD_CONNECTION,	"BAD_CONNECTION",	"Invalid module path -- %s -- %s cannot connect to %s\n"},
	{BAD_MAGIC,		"BAD_MAGIC",		"Invalid magic number\n"},
	{CLASS_CONFLICT,	"CLASS_CONFLICT",	"Device driver %s has conflicting class %s in %s\n"},
	{COULD_NOT_BIND,	"COULD_NOT_BIND",	"Cannot bind driver%s%s at %s\n"},
	{COULD_NOT_UNBIND,	"COULD_NOT_UNBIND",	"Warning - Cannot remove %s%s%s\n"},
	{IOCONFIG_EXISTS,	"IOCONFIG_EXISTS",	"Cannot overwrite existing %s file\n"},
	{IOCONFIG_NONEXISTENT,	"IOCONFIG_NONEXISTENT", "%s does not exist\n"},
	{INVALID_ACTION,	"INVALID_ACTION",	"Invalid \"action\" argument (%u)\n"},
	{INVALID_FLAG,		"INVALID_FLAG",		"Invalid \"flag\" argument (0x%x)\n"},
	{INVALID_KEY,		"INVALID_KEY",		"Invalid \"search_key\" argument (%u)\n"},
	{INVALID_NODE_DATA,	"INVALID_NODE_DATA",	"Invalid data in io_node"},
	{INVALID_QUAL,		"INVALID_QUAL",		"Invalid \"search_qual\" argument (0x%x)\n"},
	{INVALID_TYPE,		"INVALID_TYPE",		"Invalid %s argument (%u)\n"},
	{LENGTH_OUT_OF_RANGE,	"LENGTH_OUT_OF_RANGE",	"%s path length invalid -- %s %d %s\n"},
	{NO_HARDWARE_PRESENT,	"NO_HARDWARE_PRESENT",	"No hardware at %s\n"},
	{NO_MATCH,		"NO_MATCH",		"No such device in the system\n"},
	{NO_SEARCH_FIRST,	"NO_SEARCH_FIRST",	"No SEARCH_FIRST before SEARCH_NEXT\n"},
	{NO_SUCH_FILE,		"NO_SUCH_FILE",		"/dev/config does not exist\n"},
	{NO_SUCH_MODULE,	"NO_SUCH_MODULE",	"Device driver %s is not in the kernel\n"},
	{NO_SUCH_NODE,		"NO_SUCH_NODE",		"No such node (0x%x)\n"},
	{NODE_HAS_CHILD,	"NODE_HAS_CHILD",	"Node (0x%x) has child - cannot remove\n"},
	{NOT_OPEN,		"NOT_OPEN",		"/dev/config is not open\n"},
	{OUT_OF_MEMORY,		"OUT_OF_MEMORY",	"Unable to allocate memory\n"},
	{PATHS_NOT_SAME_LENGTH,	"PATHS_NOT_SAME_LENGTH", "Hardware path and module path lengths differ\n"},
	{PERMISSION_DENIED,	"PERMISSION_DENIED",	"%s permission denied on %s\n"},
	{STRUCTURES_CHANGED,	"STRUCTURES_CHANGED",	"I/O system busy -- try again\n"},
	{SYSCALL_ERROR,		"SYSCALL_ERROR",	"System call error\n"},
	{UNKNOWN_ERROR,		"UNKNOWN_ERROR",	"Unknown libIO error (%d)\n"}
};

static libIO_status libIO_warning[] = {
	{ADDED_MODULE,		"ADDED_MODULE",		"Module added to the kernel"},
	{ADDED_CLASS,		"ADDED_CLASS",		"Class added to the kernel"},
	{ADDED_MANAGER,		"ADDED_MANAGER",	"Manager added to the kernel"},
	{ADDED_CONNECTIVITY,	"ADDED_CONNECTIVITY",	"Connectivity added to the kernel"},
	{END_OF_FILE,		"END_OF_FILE",		"End of file reached on read of %s\n"},
	{END_OF_SEARCH,		"END_OF_SEARCH",	"End of search"},
	{INCOMPLETE_PATH,	"INCOMPLETE_PATH",	"%s is not a complete path to a device\n"},
	{UNKNOWN_WARNING,	"UNKNOWN_WARNING",	"Unknown libIO warning (0x%x)\n"}
};

#endif /* _WSIO */
#ifdef DEBUG

/**************************************************************************
 * func_name(func)
 **************************************************************************
 *
 * Description:
 *	Return the name of the libio function corresponding to the
 *	input function id number.
 *
 * Input Parameters:
 *	func    The function id number (see libio.h).
 *
 * Output Parameters:	None
 *
 * Returns:
 *	A pointer to a string containing the corresponding function name
 *	or the "unknown function" string if no number matches.
 *
 * Globals Referenced:
 *	libIO_func[]
 *
 * External Calls:	None.
 *
 * Algorithm:
 *	For each entry in the libIO_func table:
 *	    If the function number matches the input func number, or we
 *	    hit _UNKNOWN_FUNCTION, return a pointer to the function name.
 *
 **************************************************************************/

char *
func_name(func)
  int func;
{
    int i;

    for (i = 0; libIO_func[i].number != func &&
	    libIO_func[i].number != _UNKNOWN_FUNCTION; i++)
	/*EMPTY_LOOP*/;
    return(libIO_func[i].name);
}

#endif /* DEBUG */

/**************************************************************************
 * mode_name(flag)
 **************************************************************************
 *
 * Description:
 *	Return a string corresponding to the input open flag value as
 *	used in calls to io_init() or ioconfig_open().
 *
 * Input Parameters:
 *	flag    The open flag for which a string is desired.  One of
 *		O_RDONLY, O_WRONLY, or O_RDWR (for io_init()),
 * S800
 *		IOCONFIG_READ, IOCONFIG_CREATE, or IOCONFIG_OVERWRITE
 *		(for ioconfig_open()).
 *              IOCONFIG_READ is set to O_RDONLY
 *
 * Output Parameters:	None
 *
 * Returns:
 *	A pointer to the corresponding string.
 *
 * Globals Referenced:	None.
 *
 * External Calls:	None.
 *
 * Algorithm:
 * S800
 *	If flag is O_RDONLY, return "Read".
 *	If flag is O_RDWR, return "Read/Write".
 *	Else (flag is O_WRONLY, IOCONFIG_CREATE, or IOCONFIG_OVERWRITE),
 *	return "Write".
 * S700
 *	If flag is O_RDONLY, return "Read".
 *	If flag is O_RDWR, return "Read/Write".
 *	Else (flag is O_WRONLY),
 *	return "Write".
 *
 **************************************************************************/

char *
mode_name(flag)
  int flag;
{
    if (flag == O_RDONLY)
	return("Read");
    else if (flag == O_RDWR)
	return("Read/Write");
    else
	return("Write");
}

/**************************************************************************
 * find_error(error)
 **************************************************************************
 *
 * Description:
 *	Return a pointer to the libIO_error structure corresponding to the
 *	input error code.
 *
 * Input Parameters:
 *	error	The error code (see libio.h).
 *
 * Output Parameters:	None
 *
 * Returns:
 *	A pointer to a libIO_status structure corresponding to the error code
 *	or a pointer to the UNKNOWN_ERROR structure if the code is not in
 *	the table.
 *
 * Globals Referenced:
 *	libIO_error[]
 *
 * External Calls:	None.
 *
 * Algorithm:
 *	For each entry in the libIO_error table:
 *	    If the error number matches the input error code, or we hit
 *	    _UNKNOWN_ERROR, return a pointer to the libIO_status structure.
 *
 **************************************************************************/

libIO_status *
find_error(error)
  int error;
{
    int i;

    for (i = 0; libIO_error[i].value != error &&
            libIO_error[i].value != UNKNOWN_ERROR; i++)
        /*EMPTY_LOOP*/;
    return(&libIO_error[i]);
}

/**************************************************************************
 * find_warning(warning)
 **************************************************************************
 *
 * Description:
 *	Return a pointer to the libIO_warning structure corresponding to the
 *	input warning code.
 *
 * Input Parameters:
 *	warning	    The warning code (see libio.h).
 *
 * Output Parameters:	None
 *
 * Returns:
 *	A pointer to a libIO_status structure corresponding to the warning
 *	code or a pointer to the UNKNOWN_WARNING structure if the code is
 *	not in the table.
 *
 * Globals Referenced:
 *	libIO_warning[]
 *
 * External Calls:	None.
 *
 * Algorithm:
 *	For each entry in the libIO_warning table:
 *	    If the warning number matches the input warning code, or we hit
 *	    _UNKNOWN_WARNING, return a pointer to the libIO_status structure.
 *
 **************************************************************************/

libIO_status *
find_warning(warning)
  int warning;
{
    int i;

    for (i = 0; (libIO_warning[i].value & warning) == 0 &&
            libIO_warning[i].value != UNKNOWN_WARNING; i++)
        /*EMPTY_LOOP*/;
    return(&libIO_warning[i]);
}

/**************************************************************************
 * print_libIO_status(prog, func, status, va_alist)
 **************************************************************************
 *
 * Description:
 *	Print to stderr a status message corresponding to a status value
 *	returned by a libio(3A) library function.  The message has the form:
 *	"<prog>: <error message>" or "<prog>: Warning - <warning message>"
 *
 * Input Parameters:
 *	prog   	    The caller's program name.
 *
 *	func	    The function id number of the function which returned
 *		    the status value.
 *
 *	status	    The status value for which a message is to be printed.
 *
 *	va_alist    Variable portion of the calling convention.  Depends on
 *		    func, as follows:
 * S800
 *
 *	    _IO_ADD_LU		    node_id_type node_id, int lu, int from_file
 *	    _IO_ADD_PATH	    hdw_path_type *hdw_path,
 *				    module_path_type *module_path,
 *				    u_int action, int valid_elements
 *	    _IO_ADD_TREE_NODE	    add_node_type *add_node,
 *				    node_id_type new_node
 *	    _IO_CHECK_CONNECTIVITY  module_path_type *module_path,
 *				    int valid_elements
 *	    _IO_GET_CONNECTIVITY    u_int type, io_name_type module,
 *				    io_name_type *mod_list
 *	    _IO_GET_MOD_INFO	    hdw_path_type *hdw_path,
 *				    mod_entry_type *mod_entry,
 *				    int valid_elements
 *	    _IO_GET_TABLE	    u_int which_table, void *table
 *	    _IO_INIT		    int flag
 *	    _IO_PROBE_HARDWARE	    hdw_path_type *hdw_path, int valid_elements
 *	    _IO_REMOVE_TREE_NODE    node_id_type node_id
 *	    _IO_SEARCH_TREE	    u_int search_type, u_int search_key,
 *				    u_int search_qual, io_node_type *io_node
 *	    _IOCONFIG_CLOSE	    u_int action
 *	    _IOCONFIG_OPEN	    char *file, int flag, int magic
 *	    _IOCONFIG_READ	    ioconfig_entry *entry
 *	    _IOCONFIG_WRITE	    ioconfig_entry *entry
 *	    _STRING_TO_HDW_PATH	    hdw_path_type *hdw_path, char *string
 *	    _STRING_TO_MOD_PATH	    module_path_type *module_path, char *string
 *	    _UPDATE_IOCONFIG	    char *file, int flag, int magic *
 * S700
 *	    _IO_GET_TABLE	    u_int which_table, void *table
 *	    _IO_SEARCH_ISC	    u_int search_type, u_int search_key,
 *				    u_int search_qual, isc_entry_type *isc_entry
 *	    _STRING_TO_HDW_PATH	    hdw_path_type *hdw_path, char *string
 *
 * Output Parameters:	None
 *
 * Returns:
 *	The input status value.
 *
 * Globals Referenced:	None.
 *
 * External Calls:
 *	va_start()  va_end() (see varargs(5))
 *
 * Algorithm:
 *	Call va_start() to initialize the varargs processing.
 *	Call do_print_libIO_status() to do all the actual work.
 *	Call va_end() to clean up the varargs processing.
 *	Return the input status.
 *
 **************************************************************************/

/*VARARGS3*/
int
print_libIO_status(prog, func, status, va_alist)
  char	    *prog;
  int	    func,
	    status;
  va_dcl
{
    va_list ap;
    void    do_print_libIO_status();

    va_start(ap);
    do_print_libIO_status(prog, func, status, ap);
    va_end(ap);
    return(status);
}

/**************************************************************************
 * do_print_libIO_status(prog, func, status, ap)
 **************************************************************************
 *
 * Description:
 *	Print to stderr a status message corresponding to a status value
 *	returned by a libio(3A) library function.  The message has the form:
 *	"<prog>: <error message>" or "<prog>: Warning - <warning message>"
 *
 * Input Parameters:
 *	prog   	    The caller's program name.
 *
 *	func	    The function id number of the function which returned
 *		    the status value.
 *
 *	status	    The status value for which a message is to be printed.
 *
 *	ap	    A va_alist value as set up be va_start() and used by
 *		    va_arg().  Depends on func as described for
 *		    print_libIO_status().
 *
 * Output Parameters:	None
 *
 * Returns:		None.
 *
 * Globals Referenced:	None.
 *
 * S800
 * External Calls:
 *	va_arg()  (see varargs(5))
 *	fprintf(3C)  perror(3C)  io_search_tree(3A)
 *
 * Algorithm:
 *	Switch on func and make va_arg() calls to copy the arguments
 *	into local variables.  For _IO_ADD_TREE_NODE and _IO_REMOVE_TREE_NODE,
 *	also make a call to io_search_tree() to get the hardware path for
 *	the input node_id.
 *	If status is <= 0 (Errors and Success):
 *	    Call find_error() to get a pointer to the libIO_error entry
 *	    matching status.
 *	    Print to stderr the prog name.
 *	    Switch on status.  For most values, simply use the format
 *	    string from the libIO_error entry along with the variables
 *	    passed in to print the error message.  A few cases have
 *	    radically different messages depending on the func or other
 *	    variables, so hardcoded messages are used.  I know, yuck :-(.
 *	    The INVALID_NODE_DATA case is even more complicated because
 *	    the message depends on the search_key as well.
 *	    For SYSCALL_ERROR, we call perror() to print the message.
 *	Else (status is > 0, i.e., Warnings):
 *	    This works very much like the Errors case (see above).
 *	    The major difference is that, because the warnings are encoded
 *	    as a series of bits, more than one can be set at a time.  Thus,
 *	    the printing code is enclosed in a while loop which continues
 *	    until all the warnings have been printed.  There are two
 *	    exceptions to the looping rule:  If func is _IO_ADD_TREE_NODE
 *	    or the value is UNKNOWN_WARNING, then exit the loop after
 *	    printing a single message.
 *	Return.
 * S700
 * External Calls:
 *	va_arg()  (see varargs(5))
 *	fprintf(3C)  perror(3C)
 *
 * Algorithm:
 *	Switch on func and make va_arg() calls to copy the arguments
 *	into local variables.  
 *	If status is <= 0 (Errors and Success):
 *	    Call find_error() to get a pointer to the libIO_error entry
 *	    matching status.
 *	    Print to stderr the prog name.
 *	    Switch on status.  For most values, simply use the format
 *	    string from the libIO_error entry along with the variables
 *	    passed in to print the error message.  A few cases have
 *	    radically different messages depending on the func or other
 *	    variables, so hardcoded messages are used.  I know, yuck :-(.
 *	    The INVALID_ENTRY_DATA case is even more complicated because
 *	    the message depends on the search_key as well.
 *	    For SYSCALL_ERROR, we call perror() to print the message.
 *	Else (status is > 0, i.e., Warnings):
 *	    This works very much like the Errors case (see above).
 *	    The major difference is that, because the warnings are encoded
 *	    as a series of bits, more than one can be set at a time.  Thus,
 *	    the printing code is enclosed in a while loop which continues
 *	    until all the warnings have been printed.  There is one
 *	    exception to the looping rule:  If the value is UNKNOWN_WARNING, 
 *          then exit the loop after printing a single message.
 *	Return.
 *
 **************************************************************************/

void
do_print_libIO_status(prog, func, status, ap)
  char	    *prog;
  int	    func,
	    status;
  va_list   ap;
{
    libIO_status	*stat_entry, *stat_entry2;
    int			flag, from_file, lu, magic, valid_elements;
    u_int		action, search_key, search_qual, search_type,
			type, which_table;
    hdw_path_type	*hdw_path;
    char		*string, *file;
    void		*table;
    int			status2;

#ifdef _WSIO
    isc_entry_type	*isc_entry;
#else /* _WSIO */
    io_node_type	*io_node;
    io_name_ptr		module;
    mod_entry_type	*mod_entry;
    io_name_type	*mod_list;
    module_path_type	*module_path;
    node_id_type	new_node, node_id;
    io_node_type	io_node2;
    add_node_type	*add_node;
    ioconfig_entry	*entry;
#endif /* _WSIO */

#ifdef _WSIO
    switch(func) {
#ifdef __hp9000s700
    case _IO_GET_TABLE:
	which_table = va_arg(ap, u_int);
	table       = va_arg(ap, void *);
	break;
#endif  /* hp9000s700 */
    case _IO_SEARCH_ISC:
	search_type = va_arg(ap, u_int);
	search_key  = va_arg(ap, u_int);
	search_qual = va_arg(ap, u_int);
	isc_entry   = va_arg(ap, isc_entry_type *);
	break;
    case _STRING_TO_HDW_PATH:
	hdw_path       = va_arg(ap, hdw_path_type *);
	string         = va_arg(ap, char *);
	valid_elements = hdw_path->num_elements;
	break;
    case _IO_INIT:
	flag = va_arg(ap, int);
	break;
    }

    if (status <= 0) {	    /* Errors */
	stat_entry = find_error(status);
	fprintf(stderr, "%s: ", prog);

	switch(status) {
	case ALREADY_OPEN:
	case NO_MATCH:
	case NO_SEARCH_FIRST:
	case NO_SUCH_FILE:
	case NOT_OPEN:
	case OUT_OF_MEMORY:
	    fprintf(stderr, stat_entry->format);
	    break;
	case INVALID_KEY:
	    fprintf(stderr, stat_entry->format, search_key);
	    break;
        case INVALID_FLAG:
	    fprintf(stderr, stat_entry->format, flag);
	    break;
	case INVALID_QUAL:
	    fprintf(stderr, stat_entry->format, search_qual);
	    break;
	case INVALID_TYPE:
	    switch(func) {
#ifdef __hp9000s700
	    case _IO_GET_TABLE:
		fprintf(stderr, stat_entry->format,
			"\"which_table\"", which_table);
		break;
#endif  /* hp9000s700 */
	    case _IO_SEARCH_ISC:
		fprintf(stderr, stat_entry->format,
			"\"search_type\"", search_type);
		break;
	    }
	    break;
	case INVALID_ENTRY_DATA:
	    switch(search_key) {
	    case KEY_HDW_PATH:
		if (isc_entry->hdw_path.num_elements < 1) {
		    stat_entry2 = find_error(LENGTH_OUT_OF_RANGE);
		    fprintf(stderr, stat_entry2->format, "Hardware",
			"at least", 1, "element required");
		} else if (isc_entry->hdw_path.num_elements >
			MAX_IO_PATH_ELEMENTS) {
		    stat_entry2 = find_error(LENGTH_OUT_OF_RANGE);
		    fprintf(stderr, stat_entry2->format, "Hardware",
			"only", MAX_IO_PATH_ELEMENTS, "elements allowed");
		} else {
		    stat_entry2 = find_error(BAD_ADDRESS_FORMAT);
		    fprintf(stderr, stat_entry2->format,
			hdw_path_to_string(&isc_entry->hdw_path, 0));
		}
		break;
	    }
	    break;
	case LENGTH_OUT_OF_RANGE:
	   {
		int min = 1;
		int len = hdw_path->num_elements;

		fprintf(stderr, stat_entry->format,
			"Hardware",
			(len < min) ? "at least" : "only",
			(len < min) ? 1 : MAX_IO_PATH_ELEMENTS,
			(len < min) ? "element required" : "elements allowed");
	    }
	    break;
	case NO_HARDWARE_PRESENT:
	    hdw_path->num_elements = valid_elements + 1;
	    fprintf(stderr, stat_entry->format,
		    hdw_path_to_string(hdw_path, valid_elements + 1));
	    break;
	case PERMISSION_DENIED:
	    switch(func) {
	    case _IO_INIT:
		fprintf(stderr, stat_entry->format, mode_name(flag),
		    DEVCONFIG_FILE);
		break;
	    }
	    break;
	case SYSCALL_ERROR:
	    perror(DEVCONFIG_FILE);
	    break;
#ifdef DEBUG
	    fprintf(stderr, "\t(%s returned %s, errno %d)\n",
		    func_name(func), stat_entry->name, errno);
	    return;
#endif /* DEBUG */
	    break;
	default:
	    fprintf(stderr, stat_entry->format, status);
	    break;
	}
#ifdef DEBUG
	fprintf(stderr, "\t(%s returned %s)\n",
	    func_name(func), stat_entry->name);
#endif /* DEBUG */
    } else {	/* status > 0  -- Warnings */
	while (status > 0) {
	    stat_entry = find_warning(status);
	    fprintf(stderr, "%s: Warning - ", prog);
	    switch(stat_entry->value) {
	    case END_OF_SEARCH:
		fprintf(stderr, stat_entry->format);
		break;
	    default:
		fprintf(stderr, stat_entry->format, status);
		break;
	    }
#ifdef DEBUG
	    fprintf(stderr, "\t(%s returned %s)\n",
		func_name(func), stat_entry->name);
#endif /* DEBUG */
	    if (stat_entry->value == UNKNOWN_WARNING)
		break;
	    status &= ~stat_entry->value;
	}
    }
#else /* _WSIO */
    switch(func) {
    case _IO_ADD_LU:
	node_id   = va_arg(ap, node_id_type);
	lu        = va_arg(ap, int);
	from_file = va_arg(ap, int);
	break;
    case _IO_ADD_PATH:
	hdw_path       = va_arg(ap, hdw_path_type *);
	module_path    = va_arg(ap, module_path_type *);
	action         = va_arg(ap, u_int);
	valid_elements = va_arg(ap, int);
	break;
    case _IO_ADD_TREE_NODE:
	add_node = va_arg(ap, add_node_type *);
	new_node = va_arg(ap, node_id_type);
	node_id  = add_node->parent_node;

	if (status > 0) {
	    hdw_path = &io_node2.hdw_path;
	    io_node2.node_id = new_node;
	    if (io_search_tree(SEARCH_SINGLE, KEY_NODE_ID, QUAL_NONE,
		    &io_node2) != SUCCESS)
		hdw_path->num_elements = 0;
	}
	break;
    case _IO_CHECK_CONNECTIVITY:
	module_path    = va_arg(ap, module_path_type *);
	valid_elements = va_arg(ap, int);
	break;
    case _IO_GET_CONNECTIVITY:
	type     = va_arg(ap, u_int);
	module   = va_arg(ap, io_name_type);
	mod_list = va_arg(ap, io_name_type *);
	break;
    case _IO_GET_MOD_INFO:
	hdw_path       = va_arg(ap, hdw_path_type *);
	mod_entry      = va_arg(ap, mod_entry_type *);
	valid_elements = va_arg(ap, int);
	break;
    case _IO_GET_TABLE:
	which_table = va_arg(ap, u_int);
	table       = va_arg(ap, void *);
	break;
    case _IO_INIT:
	flag = va_arg(ap, int);
	break;
    case _IO_PROBE_HARDWARE:
	hdw_path       = va_arg(ap, hdw_path_type *);
	valid_elements = va_arg(ap, int);
	break;
    case _IO_REMOVE_TREE_NODE:
	node_id = va_arg(ap, node_id_type);
	if (status == COULD_NOT_UNBIND) {
	    hdw_path = &io_node2.hdw_path;
	    module = io_node2.module;
	    io_node2.node_id = node_id;
	    if (io_search_tree(SEARCH_SINGLE, KEY_NODE_ID, QUAL_NONE,
		    &io_node2) != SUCCESS) {
		hdw_path->num_elements = 0;
		module = NULL_IO_NAME_PTR;
	    }
	}
	break;
    case _IO_SEARCH_TREE:
	search_type = va_arg(ap, u_int);
	search_key  = va_arg(ap, u_int);
	search_qual = va_arg(ap, u_int);
	io_node     = va_arg(ap, io_node_type *);
	break;
    case _IOCONFIG_CLOSE:
	file  = va_arg(ap, char *);
	action = va_arg(ap, u_int);
	break;
    case _IOCONFIG_OPEN:
    case _UPDATE_IOCONFIG:
	file  = va_arg(ap, char *);
	flag  = va_arg(ap, int);
	magic = va_arg(ap, int);
	break;
    case _IOCONFIG_READ:
    case _IOCONFIG_WRITE:
	file  = va_arg(ap, char *);
	entry = va_arg(ap, ioconfig_entry *);
	break;
    case _STRING_TO_HDW_PATH:
	hdw_path       = va_arg(ap, hdw_path_type *);
	string         = va_arg(ap, char *);
	valid_elements = hdw_path->num_elements;
	break;
    case _STRING_TO_MOD_PATH:
	module_path    = va_arg(ap, module_path_type *);
	string         = va_arg(ap, char *);
	valid_elements = module_path->num_elements;
	break;
    }

    if (status <= 0) {	    /* Errors */
	stat_entry = find_error(status);
	fprintf(stderr, "%s: ", prog);

	switch(status) {
	case ALREADY_OPEN:
	case BAD_MAGIC:
	case NO_MATCH:
	case NO_SEARCH_FIRST:
	case NO_SUCH_FILE:
	case NOT_OPEN:
	case OUT_OF_MEMORY:
	case PATHS_NOT_SAME_LENGTH:
	case STRUCTURES_CHANGED:
	    fprintf(stderr, stat_entry->format);
	    break;
	case BAD_ADDRESS_FORMAT:
	    fprintf(stderr, stat_entry->format,
		    (func == _STRING_TO_HDW_PATH) ?
		    string : hdw_path_to_string(hdw_path,
		    (func == _IO_GET_MOD_INFO) ? valid_elements : 0),
		    valid_elements + 1);
	    break;
	case BAD_CONNECTION:
	    if (func == _IO_ADD_TREE_NODE)
		fprintf(stderr, "%s is corrupt\n", IOCONFIG_FILE);
	    else
		if (valid_elements > 0)
		    fprintf(stderr, stat_entry->format,
			(func == _STRING_TO_MOD_PATH) ?
			string : mod_path_to_string(module_path),
			module_path->name[valid_elements],
			module_path->name[valid_elements - 1]);
		else
		    fprintf(stderr,
			"Invalid module path -- %s -- path cannot begin with %s\n",
			(func == _STRING_TO_MOD_PATH) ?
			string : mod_path_to_string(module_path),
			module_path->name[0]);
	    break;
	case CLASS_CONFLICT:
	    fprintf(stderr, stat_entry->format,
		    add_node->module, add_node->class, IOCONFIG_FILE);
	    break;
	case COULD_NOT_BIND:
	    hdw_path->num_elements = valid_elements + 1;
	    if (func == _IO_ADD_PATH)
		fprintf(stderr, stat_entry->format,
		    " ", module_path->name[valid_elements],
		    hdw_path_to_string(hdw_path, 0));
	    else
		fprintf(stderr, stat_entry->format,
		    "", "", hdw_path_to_string(hdw_path, 0));
	    break;
	case COULD_NOT_UNBIND:
	    /* Only the undocumented function io_remove_tree_node() returns
	     * this error, and the only current user of io_remove_tree_node()
	     * is rmsf(1M) which wants it reported as a warning.
	     */
	    if (hdw_path->num_elements > 0)
		fprintf(stderr, stat_entry->format,
		    module, " at ", hdw_path_to_string(hdw_path ,0));
	    else
		fprintf(stderr, stat_entry->format,
		    "driver", "", "");
	    break;
	case IOCONFIG_EXISTS:
	case IOCONFIG_NONEXISTENT:
	    fprintf(stderr, stat_entry->format, file);
	    break;
	case INVALID_ACTION:
	    fprintf(stderr, stat_entry->format, action);
	    break;
	case INVALID_FLAG:
	    fprintf(stderr, stat_entry->format, flag);
	    break;
	case INVALID_KEY:
	    fprintf(stderr, stat_entry->format, search_key);
	    break;
	case INVALID_QUAL:
	    fprintf(stderr, stat_entry->format, search_qual);
	    break;
	case INVALID_TYPE:
	    switch(func) {
	    case _IO_GET_CONNECTIVITY:
		fprintf(stderr, stat_entry->format, "\"type\"", type);
		break;
	    case _IO_GET_TABLE:
		fprintf(stderr, stat_entry->format,
			"\"which_table\"", which_table);
		break;
	    case _IO_SEARCH_TREE:
		fprintf(stderr, stat_entry->format,
			"\"search_type\"", search_type);
		break;
	    }
	    break;
	case INVALID_NODE_DATA:
	    switch(search_key) {
	    case KEY_NODE_ID:
		fprintf(stderr, "Invalid node id (0x%x)\n", io_node->node_id);
		break;
	    case KEY_MANAGER_NAME:
		if (io_node->mgr_options.pseudo == TRUE)
		    fprintf(stderr,
			"%s is a pseudo-driver\n", io_node->manager);
		else
		    fprintf(stderr, "Device manager %s is not in the kernel\n",
			io_node->manager);
		break;
	    case KEY_MODULE_NAME:
		fprintf(stderr, "Device driver %s is not in the kernel\n",
			io_node->module);
		break;
	    case KEY_CLASS_NAME:
		fprintf(stderr, "Device class %s is not in the kernel\n",
			io_node->class);
		break;
	    case KEY_HDW_PATH:
		if (io_node->hdw_path.num_elements < 1) {
		    stat_entry2 = find_error(LENGTH_OUT_OF_RANGE);
		    fprintf(stderr, stat_entry2->format, "Hardware",
			"at least", 1, "element required");
		} else if (io_node->hdw_path.num_elements >
			MAX_IO_PATH_ELEMENTS) {
		    stat_entry2 = find_error(LENGTH_OUT_OF_RANGE);
		    fprintf(stderr, stat_entry2->format, "Hardware",
			"only", MAX_IO_PATH_ELEMENTS, "elements allowed");
		} else {
		    stat_entry2 = find_error(BAD_ADDRESS_FORMAT);
		    fprintf(stderr, stat_entry2->format,
			hdw_path_to_string(&io_node->hdw_path, 0));
		}
		break;
	    case KEY_PORT_NUMBER:
		fprintf(stderr, "Invalid port number (%d)\n", io_node->port);
		break;
	    case KEY_C_MAJOR:
		if (io_node->mgr_options.pseudo == TRUE)
		    fprintf(stderr,
			"Character major %d corresponds to a pseudo-driver\n",
			io_node->c_major);
		else
		    fprintf(stderr,
			"Character major %d is not in the kernel\n",
			io_node->c_major);
		break;
	    case KEY_B_MAJOR:
		if (io_node->mgr_options.pseudo == TRUE)
		    fprintf(stderr,
			"Block major %d corresponds to a pseudo-driver\n",
			io_node->b_major);
		else
		    fprintf(stderr,
			"Block major %d is not in the kernel\n",
			io_node->b_major);
		break;
	    }
	    break;
	case LENGTH_OUT_OF_RANGE:
	    {
		int min = (func == _IO_PROBE_HARDWARE) ? 0 : 1;
		int len = (func == _IO_CHECK_CONNECTIVITY ||
			func == _STRING_TO_MOD_PATH) ?
			module_path->num_elements :
			hdw_path->num_elements;

		fprintf(stderr, stat_entry->format,
			(func == _IO_CHECK_CONNECTIVITY ||
			func == _STRING_TO_MOD_PATH) ? "Module" :
			(func == _IO_ADD_PATH) ? "Hardware/Module" : "Hardware",
			(len < min) ? "at least" : "only",
			(len < min) ? 1 : MAX_IO_PATH_ELEMENTS,
			(len < min) ? "element required" : "elements allowed");
	    }
	    break;
	case NO_HARDWARE_PRESENT:
	    hdw_path->num_elements = valid_elements + 1;
	    fprintf(stderr, stat_entry->format,
		    hdw_path_to_string(hdw_path,
		    (func == _IO_PROBE_HARDWARE) ? 0 : valid_elements + 1));
	    break;
	case NO_SUCH_MODULE:
	    fprintf(stderr, stat_entry->format,
		    (func == _IO_GET_CONNECTIVITY) ? module :
		    module_path->name[valid_elements]);
	    break;
	case NO_SUCH_NODE:
	case NODE_HAS_CHILD:
	    fprintf(stderr, stat_entry->format, node_id);
	    break;
	case PERMISSION_DENIED:
	    switch(func) {
	    case _IO_ADD_LU:
	    case _IO_ADD_PATH:
	    case _IO_ADD_TREE_NODE:
	    case _IO_PROBE_HARDWARE:
	    case _IO_REMOVE_TREE_NODE:
		fprintf(stderr, stat_entry->format, "Read/Write",
		    DEVCONFIG_FILE);
		break;
	    case _IO_CHECK_CONNECTIVITY:
	    case _IO_GET_CONNECTIVITY:
	    case _IO_GET_MOD_INFO:
	    case _IO_GET_TABLE:
	    case _IO_SEARCH_TREE:
		fprintf(stderr, stat_entry->format, "Read", DEVCONFIG_FILE);
		break;
	    case _IO_INIT:
	    case _IOCONFIG_OPEN:
	    case _UPDATE_IOCONFIG:
		fprintf(stderr, stat_entry->format, mode_name(flag),
		    (func == _IO_INIT) ? DEVCONFIG_FILE : file);
		break;
	    }
	    break;
	case SYSCALL_ERROR:
	    switch(func) {
	    case _IOCONFIG_CLOSE:
	    case _IOCONFIG_READ:
	    case _IOCONFIG_WRITE:
	    case _IOCONFIG_OPEN:
	    case _UPDATE_IOCONFIG:
		perror(file);
		break;
	    default:
		perror(DEVCONFIG_FILE);
		break;
	    }
#ifdef DEBUG
	    fprintf(stderr, "\t(%s returned %s, errno %d)\n",
		    func_name(func), stat_entry->name, errno);
	    return;
#endif /* DEBUG */
	    break;
	default:
	    fprintf(stderr, stat_entry->format, status);
	    break;
	}
#ifdef DEBUG
	fprintf(stderr, "\t(%s returned %s)\n",
	    func_name(func), stat_entry->name);
#endif /* DEBUG */
    } else {	/* status > 0  -- Warnings */
	while (status > 0) {
	    stat_entry = find_warning(status);
	    fprintf(stderr, "%s: Warning - ", prog);
	    switch(stat_entry->value) {
	    case ADDED_CLASS:
		if (func == _IO_ADD_TREE_NODE)
		    if (hdw_path->num_elements > 0)
			fprintf(stderr,
			    "Device class %s is not in the kernel - device at %s unusable\n",
			    add_node->class,
			    hdw_path_to_string(hdw_path, 0));
		    else
			fprintf(stderr,
			    "Device class %s is not in the kernel\n",
			    add_node->class);
		else
		    fprintf(stderr, "Device class is not in the kernel\n");
		break;
	    case ADDED_CONNECTIVITY:
		if (func == _IO_ADD_TREE_NODE)
		    if (hdw_path->num_elements > 0)
			fprintf(stderr,
			    "Device driver %s is improperly connected - device at %s unusable\n",
			    add_node->module,
			    hdw_path_to_string(hdw_path, 0));
		    else
			fprintf(stderr,
			    "Device driver %s is improperly connected\n",
			    add_node->module);
		else
		    fprintf(stderr, "Device driver is improperly connected\n");
		break;
	    case ADDED_MANAGER:
		if (func == _IO_ADD_TREE_NODE)
		    if (hdw_path->num_elements > 0)
			fprintf(stderr,
			    "Device manager %s is not in the kernel - device at %s unusable\n",
			    add_node->manager,
			    hdw_path_to_string(hdw_path, 0));
		    else
			fprintf(stderr,
			    "Device manager %s is not in the kernel\n",
			    add_node->manager);
		else
		    fprintf(stderr, "Device manager is not in the kernel\n");
		break;
	    case ADDED_MODULE:
		if (func == _IO_ADD_TREE_NODE)
		    if (hdw_path->num_elements > 0)
			fprintf(stderr,
			    "Device driver %s is not in the kernel - device at %s unusable\n",
			    add_node->module,
			    hdw_path_to_string(hdw_path, 0));
		    else
			fprintf(stderr,
			    "Device driver %s is not in the kernel\n",
			    add_node->module);
		else
		    fprintf(stderr, "Device driver is not in the kernel\n");
		break;
	    case END_OF_FILE:
		fprintf(stderr, stat_entry->format, file);
		break;
	    case END_OF_SEARCH:
		fprintf(stderr, stat_entry->format);
		break;
	    case INCOMPLETE_PATH:
		fprintf(stderr, stat_entry->format,
			mod_path_to_string(module_path));
		break;
	    default:
		fprintf(stderr, stat_entry->format, status);
		break;
	    }
#ifdef DEBUG
	    fprintf(stderr, "\t(%s returned %s)\n",
		func_name(func), stat_entry->name);
#endif /* DEBUG */
	    if (func == _IO_ADD_TREE_NODE)
		break;
	    if (stat_entry->value == UNKNOWN_WARNING)
		break;
	    status &= ~stat_entry->value;
	}
    }
#endif /* _WSIO */
}
