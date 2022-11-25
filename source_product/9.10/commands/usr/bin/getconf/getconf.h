/*
 * @(#) $Revision: 70.4 $
 * This table of parameters is used for calls to
 * confstring, pathconf and sysconf functions.
 */

#define	FT_CONFSTR	1		/* Function Types */
#define	FT_PATHCONF	2
#define	FT_SYSCONF	3

struct parameter_table			/* parameter table */ 
{
	char	*pt_name;		/* variable */
	int 	pt_code;		/* header name value */
	int 	pt_function_type;	/* function to call for this info */
} parm_table[] = {

	/* POSIX.1-1990  Table 2-3, pg 35 */
	{"_POSIX_ARG_MAX",		_SC_ARG_MAX,            FT_SYSCONF},
	{"_POSIX_CHILD_MAX",		_SC_CHILD_MAX,          FT_SYSCONF},
	{"_POSIX_LINK_MAX",		_PC_LINK_MAX,           FT_PATHCONF},
	{"_POSIX_MAX_CANON",		_PC_MAX_CANON,          FT_PATHCONF},
	{"_POSIX_MAX_INPUT",		_PC_MAX_INPUT,          FT_PATHCONF},
	{"_POSIX_NAME_MAX",		_PC_NAME_MAX,           FT_PATHCONF},
	{"_POSIX_NGROUPS_MAX",		_SC_NGROUPS_MAX,        FT_SYSCONF},
	{"_POSIX_OPEN_MAX",		_SC_OPEN_MAX,           FT_SYSCONF},
	{"_POSIX_PATH_MAX",		_PC_PATH_MAX,           FT_PATHCONF},
	{"_POSIX_PIPE_BUF",		_PC_PIPE_BUF,	 	FT_PATHCONF},
	{"_POSIX_STREAM_MAX",		_SC_STREAM_MAX,         FT_SYSCONF},
	{"_POSIX_TZNAME_MAX",		_SC_TZNAME_MAX,         FT_SYSCONF},

	/* POSIX.1-1990  Table 2-4, pg 35 */
	{"NGROUPS_MAX",			_SC_NGROUPS_MAX,        FT_SYSCONF},

	/* POSIX.1-1990  Table 2-5, pg 36 */
	{"ARG_MAX",			_SC_ARG_MAX,            FT_SYSCONF},
	{"CHILD_MAX",			_SC_CHILD_MAX,          FT_SYSCONF},
	{"OPEN_MAX",			_SC_OPEN_MAX,           FT_SYSCONF},
	{"STREAM_MAX",			_SC_STREAM_MAX,         FT_SYSCONF},
	{"TZNAME_MAX",			_SC_TZNAME_MAX,         FT_SYSCONF},

	/* POSIX.1-1990  Table 2-6, pg 36 */
	{"LINK_MAX",			_PC_LINK_MAX,           FT_PATHCONF},
	{"MAX_CANON",			_PC_MAX_CANON,          FT_PATHCONF},
	{"MAX_INPUT",			_PC_MAX_INPUT,          FT_PATHCONF},
	{"NAME_MAX",			_PC_NAME_MAX,           FT_PATHCONF},
	{"PATH_MAX",			_PC_PATH_MAX,           FT_PATHCONF},
	{"PIPE_BUF",			_PC_PIPE_BUF,           FT_PATHCONF},

	/* POSIX.1-1990  Table 2-10, pg 38 */
	{"_POSIX_JOB_CONTROL",		_SC_JOB_CONTROL,        FT_SYSCONF},
	{"_POSIX_SAVED_IDS",		_SC_SAVED_IDS,          FT_SYSCONF},
	{"_POSIX_VERSION",		_SC_VERSION,            FT_SYSCONF},

	/* POSIX.1-1990  Table 2-11, pg 39 */
	{"_POSIX_CHOWN_RESTRICTED",	_PC_CHOWN_RESTRICTED,	FT_PATHCONF},
	{"_POSIX_NO_TRUNC",		_PC_NO_TRUNC,           FT_PATHCONF},
	{"_POSIX_VDISABLE",		_PC_VDISABLE,           FT_PATHCONF},

	/* POSIX.1-1990  Table 4-2, pg 80 */
	{"CLK_TCK",			_SC_CLK_TCK,            FT_SYSCONF},

	/* POSIX.2-DRAFT11.2  Table 2-16, pg 173 */
	{"POSIX2_BC_BASE_MAX",		_SC_BC_BASE_MAX,        FT_SYSCONF},
	{"POSIX2_BC_DIM_MAX",		_SC_BC_DIM_MAX,         FT_SYSCONF},
	{"POSIX2_BC_SCALE_MAX",		_SC_BC_SCALE_MAX,       FT_SYSCONF},
	{"POSIX2_BC_STRING_MAX",	_SC_BC_STRING_MAX,      FT_SYSCONF},
	{"POSIX2_COLL_WEIGHTS_MAX",	_SC_COLL_WEIGHTS_MAX,	FT_SYSCONF},
	{"POSIX2_EXPR_NEST_MAX",	_SC_EXPR_NEST_MAX,      FT_SYSCONF},
	{"POSIX2_LINE_MAX",		_SC_LINE_MAX,           FT_SYSCONF},
	{"POSIX2_RE_DUP_MAX",		_SC_RE_DUP_MAX,         FT_SYSCONF},
	{"POSIX2_VERSION",		_SC_2_VERSION,          FT_SYSCONF},

	/* POSIX.2-DRAFT11.2  Table 2-17, pg 174 and Table B-19, pg 811 */
	{"BC_BASE_MAX",			_SC_BC_BASE_MAX,        FT_SYSCONF},
	{"BC_DIM_MAX",			_SC_BC_DIM_MAX,         FT_SYSCONF},
	{"BC_SCALE_MAX",		_SC_BC_SCALE_MAX,       FT_SYSCONF},
	{"BC_STRING_MAX",		_SC_BC_STRING_MAX,      FT_SYSCONF},
	{"COLL_WEIGHTS_MAX",		_SC_COLL_WEIGHTS_MAX,   FT_SYSCONF},
	{"EXPR_NEST_MAX",		_SC_EXPR_NEST_MAX,      FT_SYSCONF},
	{"LINE_MAX",			_SC_LINE_MAX,           FT_SYSCONF},
	{"RE_DUP_MAX",			_SC_RE_DUP_MAX,         FT_SYSCONF},

	/* POSIX.2-DRAFT11.2  Table 2-18, pg 179 and Table B-19, pg 811 */
	{"POSIX2_C_BIND",		_SC_2_C_BIND,           FT_SYSCONF},
	{"POSIX2_C_DEV",		_SC_2_C_DEV,            FT_SYSCONF},
	{"POSIX2_FORT_DEV",		_SC_2_FORT_DEV,         FT_SYSCONF},
	{"POSIX2_FORT_RUN",		_SC_2_FORT_RUN,         FT_SYSCONF},
	{"POSIX2_LOCALEDEF",		_SC_2_LOCALEDEF,        FT_SYSCONF},
	{"POSIX2_SW_DEV",		_SC_2_SW_DEV,           FT_SYSCONF},

	/* POSIX.2-DRAFT11.2  Table B-18, pg 809 */
	{"CS_PATH",			_CS_PATH,               FT_CONFSTR},

	/* POSIX.2-DRAFT11.3  getconf, Section 4.26.4, pg 450 */
	{"PATH",			_CS_PATH,               FT_CONFSTR},

	/* Other implementation defined values */
	{"SC_XOPEN_VERSION",		_SC_XOPEN_VERSION,      FT_SYSCONF},
	{"SC_PASS_MAX",			_SC_PASS_MAX,           FT_SYSCONF},
	{"SC_PAGE_SIZE",		_SC_PAGE_SIZE,          FT_SYSCONF},
#ifdef TRUX 
	{"SECURITY_CLASS",		_SC_SECURITY_CLASS,     FT_SYSCONF},
#endif /* TRUX */

	/* Terminate table */
	{ (char *)0,			0,			0}
};
