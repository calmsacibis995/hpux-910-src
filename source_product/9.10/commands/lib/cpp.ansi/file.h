/* $Revision: 70.4 $ */

/*
 * (c) Copyright 1989,1990,1991,1992 Hewlett-Packard Company, all rights reserved.
 *
 * #   #  ##  ##### ####
 * ##  # #  #   #   #
 * # # # #  #   #   ####
 * #  ## #  #   #   #
 * #   #  ##    #   ####
 *
 * Please read the README file with this source code. It contains important
 * information on conventions which must be followed to make sure that this
 * code keeps working on all the platforms and in all the environments in
 * which it is used.
 */

#include "support.h"

typedef struct search_path_node {
	char *pathname;
	struct search_path_node *next;
} t_search_path;

typedef struct include_file_node {
	FILE *fp;
	char *filename;
	char *filedate;
	int lineno;
#ifdef OPT_INCLUDE
	int avoidable:1;	/* has anything made this file unavoidable? */
	int inside_if:1;	/* are we inside an outer if? */
	int seen_if:1;		/* seen the if/endif for an enclosing candidate? */
	int if_type:2;		/* ifdef, ifndef, or if? */
	int outer_if_depth;	/* depth at outer if */
	char *condition;	/* condition for the if */
#endif
	struct include_file_node *parent;
} t_include_file;

#ifdef OPT_INCLUDE
/* types of enclosing ifs for a file */
#define	ENCLOSING_IFDEF		0
#define	ENCLOSING_IFNDEF	1
#define	ENCLOSING_IF		2
#endif

extern t_include_file *current_file;

#ifdef hpe
extern int num_file_params;         /* number of files seen on command line;
                                     * num_file_params is referenced by file.c
                                     * and startup.c
                                     */
#endif

extern boolean start_file();
extern boolean end_file();

#ifdef OPT_INCLUDE
#define	SET_UNAVOIDABLE()	if (!current_file->inside_if) \
					current_file->avoidable = FALSE; \
				else
#endif
