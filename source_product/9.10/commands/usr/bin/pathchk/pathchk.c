/* -*-C-*-
********************************************************************************
*
* File:         pathchk.c
* RCS:          $Header: pathchk.c,v 70.3 92/03/16 12:06:00 ssa Exp $
* Description:  Implement P1003.2 pathchk program
* Author:       
* Created:      Mon Mar  2 12:39:00 1992
* Modified:     Mon Mar  2 16:50:06 1992
* Language:     C
* Package:      HP-UX Commands
* Status:       Development
*
* (c) Copyright 1992,  , all rights reserved.
*
********************************************************************************
*/

#include <unistd.h>
#include <limits.h>
#include <locale.h>
#include <nl_types.h>
#include <nl_ctype.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

extern int optind; /* For getopt() */

#ifdef NL_SETN
#undef NL_SETN
#endif
#define NL_SETN NL_SETD

extern int check_pathname();

char *prog; /* My name */
nl_catd nls_catalog;

int main(argc, argv)
int argc;
char **argv;
{
    int c;
    int exit_code = 0;
    int portable_checks = 0;

    /* Get my name for error messages */
    if (prog = strrchr(argv[0], '/'))
	prog++;
    else prog = argv[0];

    /* Set the NLS locale and the message catalog */
    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr,
                "%s: Cannot set locale, continuing with \"C\" locale.\n",
                prog);
        nls_catalog = (nl_catd) -1;
    } else {
        nls_catalog = catopen("pathchk", 0);
    }

    /* Process option arguments */
    while ((c = getopt(argc, argv, "p")) != EOF) {
        switch (c)
        {
	case 'p':
	    portable_checks = 1;
	    break;
	case '?':
	default:
	    exit(1);
	}
    }

    /* Process pathname arguments */
    for (; optind < argc; optind++) {
	exit_code |= check_pathname(argv[optind], portable_checks);
    }
    exit(exit_code);
}

/* Returns 0 if pathname is OK,
 * Returns 1 if pathname is not OK
 * If pathname is not OK, prints message to setderr */

int check_pathname(pathname, portable)
unsigned char *pathname; /* pathname to check */
int portable; /* if true, only check against portable POSIX limits */
{
    unsigned char *cur_component;  /* Current component of pathname */
    unsigned char *cur_compchar;   /* Current character in current component */
    unsigned char *next_slash;     /* Next occurrence of '/' in pathname */
    unsigned char *cur_dir;        /* Start of directory name for component */
    int end_of_dir = 0;            /* Found end of directory name for component */
    int path_max_left;             /* Number of bytes remaining allowed in PATH_MAX or _POSIX_PATH_MAX */
    int name_max;                  /* NAME_MAX or _POSIX_NAME_MAX for current component */
    int i;                         /* temporary */

    /* Set cur_component to start of first component of name */
    for (cur_component = pathname; CHARAT(cur_component) == '/'; ADVANCE(cur_component)) {
	/* Do nothing */
    }

    /* Set cur_dir to start of directory entry */
    cur_dir = pathname;

    /* Set name_max and path_max_left */
    if (portable) {
	path_max_left = _POSIX_PATH_MAX;
	name_max      = _POSIX_NAME_MAX;
    } else {
	/* TODO: Need to check for errors from pathconf */
	switch (CHARAT(cur_dir)) {
	case '/':
	    /* A rooted path, start at root directory */
	    path_max_left = pathconf("/", _PC_PATH_MAX);
	    name_max      = pathconf("/", _PC_NAME_MAX);
	    break;
	case '.':
	    /* Explicit dot or dot-dot */
	    ADVANCE(cur_dir);
	    if (CHARAT(cur_dir) == '.') {
		/* dot-dot */
		path_max_left = pathconf("..", _PC_PATH_MAX);
		name_max      = pathconf("..", _PC_NAME_MAX);
	    } else {
		/* dot */
		path_max_left = pathconf(".", _PC_PATH_MAX);
		name_max      = pathconf(".", _PC_NAME_MAX);
	    }
	    cur_dir = pathname;
	    break;
	default:
	    /* Implicit dot */
	    path_max_left = pathconf(".", _PC_PATH_MAX);
	    name_max      = pathconf(".", _PC_NAME_MAX);
	    break;
	}
    }
    /* Update remaining characts to cur_component */
    path_max_left -= (cur_component - cur_dir);

    /* Loop to process components.  Invariant:
     * cur_component is start of component or end of string
     * next_slash    is next '/' at or after cur_component (or NULL if none)
     * path_max_left is bytes allowed from cur_component to end of string
     */
    while (CHARAT(cur_component) != '\0') {
	if (next_slash = (unsigned char *)strchr(cur_component, '/')) {
	    *next_slash = '\0';
	}

	/* Loop to process a single component.  Invariant:
	 * cur_compchar  is the start of the (potentially multibyte) character
	 *               to be processed next
	 */
	for (cur_compchar = cur_component; CHARAT(cur_compchar) != '\0'; ADVANCE(cur_compchar)) {
	    if (portable) {
		i = CHARAT(cur_compchar);
		if (i < 0 || i > 255 || !(isupper(i) || islower(i) || isdigit(i) || i == '-' || i == '_' || i == '.')) {
		    if (next_slash) *next_slash = '/';
		    fprintf(stderr, (catgets(nls_catalog,NL_SETN,1, "Path name (%s) contains non-portable characters\n")), pathname);
		    return 1;
		}
	    } else {
		/* TODO: check if legal locally */
	    }
	}

	/* Update name_max */
	if ((cur_compchar - cur_component) > name_max) {
	    if (next_slash) *next_slash = '/';
	    fprintf(stderr, (catgets(nls_catalog,NL_SETN,2, "Component of path name (%s) exceeds NAME_MAX bytes\n")), pathname);
	    return 1;
	}
	if (!portable && !end_of_dir) {
	    i = pathconf(cur_dir, _PC_NAME_MAX);
	    if (i == -1) {
		switch (errno) {
		case EACCES:
		    if (next_slash) *next_slash = '/';
		    fprintf(stderr, (catgets(nls_catalog,NL_SETN,3, "Path name (%s) has inaccessible component\n")), pathname);
		    return 1;
		    break;
		case ENOTDIR:
		    if (next_slash) *next_slash = '/';
		    fprintf(stderr, (catgets(nls_catalog,NL_SETN,4, "Path name (%s) impossible to create locally\n")), pathname);
		    return 1;
		    break;
		case EINVAL:
		    /* Name invalid or not a directory */
		    break;
		case ENOENT:
		    /* TODO: Check that we could create:
		     * backup to previous '/'
		     * use access() to check permissions
		     */
		    end_of_dir = 1;
		    break;
		case ENAMETOOLONG:
		    /* Assume NFS supports at least POSIX minimum */
		    if (next_slash) *next_slash = '/';
		    fprintf(stderr, (catgets(nls_catalog,NL_SETN,5, "Path name (%s) exceeds PATH_MAX bytes\n")), pathname);
		    return 1;
		    break;
		case EOPNOTSUPP:
		    name_max = _POSIX_NAME_MAX;
		    break;
		}
	    } else {
		name_max = i;
	    }
	}

	/* Update path_max_left */
	path_max_left -= (cur_compchar - cur_component);
	if (path_max_left < 0) {
	    if (next_slash) *next_slash = '/';
	    fprintf(stderr, (catgets(nls_catalog,NL_SETN,5, "Path name (%s) exceeds PATH_MAX bytes\n")), pathname);
	    return 1;
	}
	if (!portable && !end_of_dir) {
	    i = pathconf(cur_dir, _PC_PATH_MAX);
	    if (i == -1) {
		switch (errno) {
		case EOPNOTSUPP:
		    /* Assume NFS supports at least POSIX minimum */
		    i = _POSIX_PATH_MAX;
		    if (i < path_max_left) path_max_left = i;
		    break;
		default:
		    break;
		}
	    } else {
		if (i < path_max_left) path_max_left = i;
	    }
	}

	/* Clean up the pathname by removing the null */
	if (next_slash) *next_slash = '/';

	/* Set cur_component to start of next component if any */
	cur_component = cur_compchar;
	if (CHARAT(cur_component) != '\0') ADVANCE(cur_component);
    }
    return 0;
}
