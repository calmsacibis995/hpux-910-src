/*
 *	Copyright (c) 1989, SecureWare, Inc.
 *	All rights reserved.
 *
 *	Trusted Path Menu Processing Header File-tp.h
 *	For local use only in building tp*.c commands
 *
 *	ident @(#)tp.h	2.1 12:33:27 1/25/89
 */


/*
 * General defines.
 */

#define TP_MENU_MAX_ARGS	8
#define TP_MENU_MAX_ITEMS	16
#define TP_MENU_MAX_CMDLINE	1024
#define TP_MENU_MAX_INLINE	128

/*
 * Menu type flags.
 */

#define TP_MENU_BUILTIN		0x1
#define TP_MENU_PROGRAM		0x2
#define TP_MENU_ARGUMENTS	0x4
#define TP_MENU_ACTION		0x8

/*
 * Special flags for quoted argument types.
 */

#define TP_ARG_SQUOTED		0x1
#define TP_ARG_DQUOTED		0x2

/*
 * Intrinsic arguments and the indexes.
 */

#define TP_ARG_TERMINAL		"terminal"
#define TP_ARG_USER		"user"

#define TP_TERMINAL_INDEX	TP_MENU_MAX_ARGS+1
#define TP_USER_INDEX		TP_MENU_MAX_ARGS+2
#define TP_LITERAL_INDEX	TP_MENU_MAX_ARGS+3

#define TP_MENU_FILE		"/tcb/files/tpmenu"

/*
 * Trusted Path Menu Control structure
 */

struct tp_menu {
	char	*menu_prompt;				/* menu item prompt */
	char	*func_prompt[TP_MENU_MAX_ARGS];		/* function prompts */
	char	*argnames[TP_MENU_MAX_ARGS];		/* argument names */
	ushort	argsize[TP_MENU_MAX_ARGS];		/* argument sizes */
	ushort	arg_index[TP_MENU_MAX_ARGS];		/* index for cmd */
	char	arg_type[TP_MENU_MAX_ARGS];		/* argument type */
	ushort	arguments;				/* argument count */
	ushort	argvcount;				/* argv[] count */
	ushort	flags;					/* flag field */
	char	*argv[TP_MENU_MAX_ARGS];		/* argv[] for program */
};

extern int menu_items;
