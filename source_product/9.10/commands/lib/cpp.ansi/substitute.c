/* $Revision: 72.1.1.1 $ */

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

#include <stdio.h>
#include "support.h"
#include "define.h"
#include "file.h"

/******************************************************************************
 * macro substitution module.
 *
 * This module is a tree-based implementation of macro expansion.
 *
 * Basically a macro expansion is represented by a tree. Each instance of
 * a macro name is replaced by the replacement list. The preprocessing tokens
 * on this replacement list are the children of the macro name.    And the 
 * resulting preprocessing token sequence is rescanned for more macro names to 
 * replace. The further macro substitutions are the grandchildren of the 'root'
 * macro. This macro expansion stops when no legitimate macro names are found.
 * Terminals on the tree are strings that cannot be further expanded. Gather-
 * ing the terminals ( by preorder traversal ) produces the result of macro
 * substitution.
 *
 * Example:
 *
 * The process of the following three statement
 * 
 * 	#define X a Y
 * 	#define Y b c
 * 	X
 *
 * can be represented by the tree
 *
 *         X
 *         |\
 *         a Y
 *           |\
 *           b c
 *
 * This module constructs the above tree and then use "preorder traversal" 
 * method to visit each terminal. As a result, gathering the visited terminals 
 * produces the expanded macro.
 *
 * To represent a tree for top-down processing, a doubly linked list is used for
 * the children of each node. Each node contains three links, one for the linked
 * list connecting it to its siblings, one for the linked list of its children,
 * another for its previous node . By using this scheme the above tree is shown
 * as follows:
 *
 *   ------------------
 *   [    X   |sibling]
 * +>[    0   |child  ]
 * | ------------------
 * |           |
 * |           V
 * | ------------------    ------------------
 * | [  "a"   |sibling] -> [    Y   |   0   ]
 * +-[previous|0      ] <- [previous|child  ]
 *   ------------------  +>------------------
 *                       |               |
 *                       |               |
 *                       |               |
 *                       |               |
 *                       |               V
 *                       | ------------------    ------------------
 *                       | [  "b"   |sibling] -> [   "c"  |   0   ]
 *                       +-[previous|0      ] <- [previous|   0   ]
 *                         ------------------    ------------------
 *
 * Handling argument substitution is slightly different from the above. 
 * To do this, first, the arguments for the invocation of function-like macro
 * are identified. Second, ( before a parameter is replaced by the corresponding
 * argument ) all macros that are contained in the argument are completely 
 * expanded. Third, replace the parameter in the parameter-list by the 
 * "expanded" argument. The completely expanded argument is represented by a 
 * tree.
 *
 * During the expansion of a function-like macro or an object-like macro, if 
 * a ## preprocessing token is found, delete it and concatenate the preceding 
 * preprocessing token with the following preprocessing  token. The resulting 
 * token is available for further macro replacement. In other words, this token
 * could be expanded to a tree.
 *
 * During the expansion of a function-like macro, if a # preprocessing token 
 * is found, delete it and replace the following token ( which should be a
 * parameter ) by a single character string literal preprocessing token that
 * contains the spelling of the corresponding argument. The resulting token
 * is a terminal on the tree.
 *
 * Recursive macros will be detected by the "painted blue" scheme. We borrow
 * this idea from "Notes on The Draft C Standard" by Thomas Plum, pages 30-31. 
 * Basically a macro name is disabled temporarily during scanning of its 
 * replacement-list. Each occurrence of the disabled name in a replacement is 
 * marked by 'BLUE_PAINT_MARKER'. The marked names will not be expanded in any 
 * later rescanning. These markers will be removed after all scanning and 
 * replacement.
 *
 ******************************************************************************/

#define ORIGIN (input_buffer+buffersize)

extern boolean DebugOption;
extern boolean StaticAnalysisOption;
extern boolean NLSOption;
extern char id_chars[];

extern char date_string[], time_string[];
extern int current_lineno;

extern char *mark_macros();

#define UNDECIDED			'U'
#define TERMINAL_NODE			'T'  /* Can't be further expanded */
#define OBJECT_MACRO_NODE		'O'  /* Object-like Macro */
#define FUNCTION_MACRO_NODE		'F'  /* Function-like Macro */
#define PARAM_NODE			'P'

#define BLUE_PAINT_MARKER	1

typedef struct replacement_node { /* doubly linked list */
	char node_id;
	char *macro;
	char *string;
	struct replacement_node *child;
	struct replacement_node *sibling;
	struct replacement_node *previous;
} t_replacement_node;

typedef struct replacement_node * t_arg_array[MAXPARAMS];

char *original_line;
static char  *original_line_ptr;
struct replacement_node *last_pptoken_node; /* pointer to last node in a tree */
char *last_pptoken;
int NestLevel;
boolean macrodebug;

extern int num_newlines_in_output;

static char is_special_to_get_sub_line[256] = {
1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,1,
1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,
0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
};


/*
 * 01/08/92 vasta: Memory management routines for get_sub_line.
 * 06/20/92 klee: modified for expand_macro.
 * Expand_macro allocates several large (buffersize) memory blocks each time it
 * is called for temporary workspace. Formerly, it called malloc directly so that
 * the memory could be freed when it was finished (if it used the temp_alloc
 * routines the memory would never actually be freed, so a large amount of virtual
 * memory would be eaten up). However, it ended up calling malloc thousands of
 * times, which is very slow. Also, when built into another application which
 * interleaves calls to malloc for small memory blocks, a large amount of virtual
 * memory was still consumed, since the large blocks on the freelist would get
 * "damaged" by requests for small blocks, so the next time expand_macro was
 * called there would be fewer large blocks available on malloc's freelist.
 *
 * Instead, we maintain an array of already allocated block pointers, and blocks
 * are never freed. When a new block is needed the array is checked to see if one
 * is already allocated. When a block is returned the current array index is just
 * decremented. This means that blocks MUST be returned in the reverse order of
 * allocation. The number of blocks allocated is proportional to the recursion
 * depth of nested macros, which typically is very low (less than 4 in all the
 * code I sampled).
 *
 * Realloc_buffer() reallocates a block to make it larger. In that
 * case the array is searched to find the block pointer's index, and then the
 * block is grown.
 */

static char   **bpointers;		/* array of block pointers */
static int	bpointers_size;		/* size of block pointer array */
static int	bpointers_index;	/* index of next available block slot */

/* Routine to return an allocated block */

static char *alloc_buffer()
{
  int i;

  /* If the block pointer array isn't big enough, grow it */
  if (bpointers_index >= bpointers_size)
    {
      bpointers_size += 8;

      if (!bpointers)
        {
	if ((bpointers = (char **) malloc (bpointers_size * sizeof (*bpointers))) == NULL )
		error("Out of dynamic memory");
        }
      else
	{
	if ((bpointers = (char **) realloc (bpointers, bpointers_size * sizeof (*bpointers))) == NULL)
		error("Out of dynamic memory");
	}

      for (i = bpointers_index; i < bpointers_size; ++i)
	bpointers[i] = NULL;
    }

  /* If no block is allocated for this slot yet, allocate one */
  if (!bpointers[bpointers_index])
    if ((bpointers[bpointers_index] = malloc (buffersize)) == NULL)
		error("Out of dynamic memory");

  return bpointers[bpointers_index++];
}

/* Routine to "free" an allocated block -
 * just decrements the available block index. */
static void free_buffer(bpointer)
char *bpointer;
{
  if (bpointers[--bpointers_index] != bpointer)
    fatal_cpp_error ("Internal memory management error - free_buffer()");
}

/* Routine to reallocate a block - find it in the block pointer array,
 * then grow it and update the pointer. */
static char *realloc_buffer (bpointer, size)
char *bpointer;
int size;
{
  int i;

  /* Search the array backwards since it's more likely that a recently
   * allocated block is being grown. */
  for (i = bpointers_index - 1; i >= 0; --i)
    if (bpointers[i] == bpointer)
      {
	bpointers[i] = bpointer = realloc (bpointer, size);
	if ( bpointer == NULL )
		error("Out of dynamic memory");
	break;
      }

  if (i < 0)
    fatal_cpp_error ("Internal memory management error - realloc_buffer()");

  return bpointer;
}

/*******************************************************************************
 * This routine returns a fully macro replaced line from the input.  Several
 * source lines may be read to form the line returned due to possible comments,
 * preprocessing directives, or macro invocations crossing line boundaries.
 * If the parameter 'string' is non-NULL then it is used as the sole input,
 * otherwise the input is read using 'read_line'.
 * The memory management routines above are used to avoid calling malloc very 
 * often. The 'temp_alloc' is used for the final result buffer since it must 
 * not be deallocated until after the caller does something with the result.
 *
 * Input: 
 *	string:
 *
 * Output:
 *	returns the next logical line. If any instance of macro name is found, 
 *	this logical line contains the expanded macro.
 *
 * Algorithm:
 *	1. copy input to an internal buffer.
 *
 *	2. for each preprocessing token in the internal buffer
 *
 *	   2.1 if preprocessing token is object-like macro or function-like 
 *		macro, call expand_macro to do macro substitution. Since the
 *		expanded macro is represented in a tree, the actual result of
 *		the expanded macro can be obtained by traversing the tree.
 *		The method of traversing the tree is "preorder traversal".
 *		Finally, copy all visited leaves to the output buffer.
 *
 *	   2.2 otherwise copy the current string to the output buffer.
 *
 *	3. return the output buffer.
 *
 */
char *result_buffer;               /* Pointer to the returned line. */
static char *result_ptr;           /* Current output position of result. */
int   result_size;                 /* Size of the output buffer */
char *input_buffer;                /* Pointer to the input buffer */
static boolean DoMacroTracking;
boolean last_id_painted_blue;

char *get_sub_line(string)
char *string;
{
	register char *line;  /* Current input  position of result. */
	register char *chmark;      /* Current input  position of result. */
	t_param_array param_array;
	t_replacement_node *root;
	t_define *define, *last_define;
	int length;
	char *save_original_line = NULL;
#ifdef APOLLO_EXT
	boolean was_null_line;
#endif

	extern char * read_line();
	t_define * get_define();
	t_define * get_last_function_macro();
	t_replacement_node * expand_macro();
	char *parse_arguments();
	char *strsave();

	line = ORIGIN;
	if(string == NULL)
	{
		if(read_line(ORIGIN) == NULL)
			return NULL;
		DoMacroTracking = TRUE;
#ifdef APOLLO_EXT
		if (FirstToken == is_first)
			FirstToken = may_be_first;
#endif
	}
	else
	{
		strcpy(ORIGIN, string);
		DoMacroTracking = FALSE;
	}

#ifdef APOLLO_EXT
	was_null_line = (*line == '\n');
#endif

	result_ptr = result_buffer;	/* Reset output buffer */
	root = NULL;
	while(*line != '\0')
	{
		check_output_buffer(strlen(line)+1);
		if(is_id_start(*line))
		{
			/* Mark the start of the identifier. */
			chmark = line;
			do { line++; } while(is_id(*line)); 
			if( (define = get_define(chmark, length=line-chmark)) != NULL )
			{
				t_replacement_node *last;
				char *macro_end = line;

				if(DoMacroTracking && !save_original_line)
					original_line_ptr=original_line;
				if(define->has_params)
				{
					skip_space_and_com(line);
					/* If end of a line is encountered, then read in a new line and keep skipping. */
					while(*line == '\n')
					{
						register int distance;
						register char *ch_ptr;
						
						/* See how the id must be moved to make room for a new line. */
						distance = line-ORIGIN+1;
						/* Copy the identifier back 'distance' spaces so that there is room
						 * for a new line to be read. */
						for(ch_ptr = chmark; ch_ptr <= line; ch_ptr++)
							*(ch_ptr-distance) = *ch_ptr;
						chmark -= distance;
						line -= distance;
						/* Since the end of the current line was reached, then
						 * all macros will be available for substitution. */
						if(read_line(ORIGIN) == NULL)
							break;
						line++;
						skip_space_and_com(line);
					}
					if(*line != '(')
					{
						/* Else, the identifier wasn't a macro, so just copy it over. */
						while(chmark < line)
						{
							*result_ptr++ = *chmark++;
						}
						continue;
					}
				
					line++;
					if(DoMacroTracking)
						if (save_original_line)
							copy_original_line(macro_end,line-macro_end);
						else
							copy_original_line(chmark,line-chmark);
					/* Parse the arguments into param_array
					 */ 
					line = parse_arguments(line, param_array, define->num_params, ORIGIN, DoMacroTracking);
				}
				else if(DoMacroTracking)
					copy_original_line(chmark,line-chmark);

				root = expand_macro(chmark,length,define,param_array,TRUE);
				if(DoMacroTracking)
				{
					check_output_buffer(2);
					if(DebugOption)
						*result_ptr++ = START_MARK;
#ifndef APOLLO_EXT
					else
						*result_ptr++ = ' ';
#endif
				}
				last_pptoken_node = NULL;
				traverse(root);
				if ( macrodebug )
					dump_tree(root);

				/* For the special case:
				 * #define X Y
				 * #define Y(a) a
				 * X (1)
				 * the current scheme does not check whether the
				 * token next to the expanded macro is '(' or
				 * not. Since we deal one macro name at a time,
				 * it appears a problem: the last token of the 
				 * expanded macro is a name of function-like
				 * macro and it's followed by a '('. To solve
				 * this problem we use the following kludge.
				 */
				if ( (last_define=get_last_function_macro(last_pptoken_node, &last_id_painted_blue )) && strcmp(define->name,last_define->name) != 0 )
				{
					char *p, *q;
					int len;

					p = line;
					skip_space_and_com(p);
					if ( *p == '\n' )
					{
						len = p - line + 1;
						strncpy(original_line_ptr,line,len);
						original_line_ptr += len;
						*original_line_ptr = '\0';
						save_original_line = strsave(original_line);
						if ( read_line(ORIGIN) != NULL )
						{
							q = ORIGIN;
							skip_space_and_com(q);
							if ( *q == '(' )
							{
								len = strlen(last_pptoken_node->string);
								line = ORIGIN - len;
								strncpy(line,last_pptoken_node->string,len);
#ifndef APOLLO_EXT
								result_ptr -= len+1;
#else
								result_ptr -= len;
#endif
								last_pptoken_node->previous->sibling = NULL; /* delete the last pptoken */
								continue;
							}
							else
							{
								q = ORIGIN;
								*(q-1) = '\n';
								line = q - 1;
							}
						}
					}
					else
					if ( *p == '(' )
					{
						*original_line_ptr = '\0';
						save_original_line = strsave(original_line);
						len = strlen(last_pptoken_node->string);
						line -= len;
						strncpy(line,last_pptoken_node->string,len);
#ifndef APOLLO_EXT
						result_ptr -= len+1;
#else
						result_ptr -= len;
#endif
						last_pptoken_node->previous->sibling = NULL; /* delete the last pptoken */
						continue;
					}
				}
				else
				{
					save_original_line = NULL;
				} /* end of kludge */

				if(DoMacroTracking && DebugOption)
                		{
                			int len = original_line_ptr-original_line;

					check_output_buffer(len+2);
                			if(StaticAnalysisOption)
                			{
                        			result_ptr = mark_macros(result_ptr, original_line, len);
#ifndef APOLLO_EXT
						*result_ptr++ = ' ';
#endif
                			}
                			else
                        		{
                        			*result_ptr++ = MACRO_MARK;
                        			strncpy(result_ptr, original_line, len);
                        			result_ptr += len;
                        			*result_ptr++ = END_MARK;
                        		}
                		}
#ifndef APOLLO_EXT
				if(DoMacroTracking && !DebugOption)
				{
					check_output_buffer(1);
					*result_ptr++ = ' ';
				}
#endif
				continue;
			}
			else
			/* Else, the identifier wasn't a macro, so just copy it
			 * over. 
			 */
			{
				while(chmark < line)
					*result_ptr++ = *chmark++;

			}
		}
		else if (isdigit(*line))
		{
			chmark = line;
			line = skip_number(line);
			/* Copy over the number. */
			while(chmark < line)
				*result_ptr++ = *chmark++;
		}
		else if (is_quote(*line))
		{
			skip_quote(line, result_ptr);
		}
		else if (C_Plus_Plus && *(line) == '/' && *((line)+1) == '/')
		{
			*result_ptr++ = *line++;
			*result_ptr++ = *line++;
			while (*line != '\n')
				*result_ptr++ = *line++;
		}
		else if(*(line) == '/' && *((line)+1) == '*')
		{
			*result_ptr++ = *line++;
			*result_ptr++ = *line++;
			while((*line != '*' || *(line+1) != '/') && *line != '\0')
			{
				if(*line == '\n')
					num_newlines_in_output++;
#ifdef NLS
				/* If the character is an NLS character pair 
				 * then skip over the pair to avoid the second 
				 * character being taken as part of a comment 
				 * termination. */
				if(NLSOption && FIRSTof2((unsigned char)*line) && SECof2((unsigned char)*(line+1)))
					*result_ptr++ = *line++;
#endif
				*result_ptr++ = *line++;
			}
			if(*line == '\0')
				error("Non token comment encountered while substituting");
			else
			{
				*result_ptr++ = *line++;
				*result_ptr++ = *line++;
			}
#ifdef APOLLO_EXT
			if (FirstToken == may_be_first)
				FirstToken = is_first;
#endif
		}
		else
		{
		/* Do a whole series of non-special characters in this fast 
		 * loop rather than in the outer slow loop.  A do while is 
		 * used because the first character must be copied regardless. 
		 */
			do
			{
				*result_ptr++ = *line++;
			}
			while(!is_special_to_get_sub_line[(unsigned char)*line]);
		}
	} /* end while */

	if(string == NULL && result_ptr > result_buffer && *(result_ptr-1) == '\n')
		num_newlines_in_output++;

#ifdef APOLLO_EXT
	if (FirstToken == may_be_first)
		if (was_null_line)
			FirstToken = is_first;
		else
			FirstToken = not_first;
#endif
	*result_ptr = '\0';


/* The following was added to correct SWFfc00722. It's a kludge that's */
/* makes sure that a newline will get generated by scan.c that calls   */
/* this procedure.                                                     */

	if (result_ptr == result_buffer)
	   *(result_ptr+1) = '\0';

/***********************************************************************/

	return result_buffer;
} /* get_sub_line */

/*
 * This routine parses the arguments of a macro with parameters into
 * 'param_array'.  The 'line' points to the character immediately following
 * the '(' of the arguments.  All newlines are turned into single spaces. 
 */
char *param_buffer;
char *parse_arguments(line, param_array, expected_num_params, origin, DoMacroTracking)
register char *line;
t_param_array param_array;
int expected_num_params;
char *origin;
boolean DoMacroTracking;
{
	register int i;
	register int num_params;
	register int num_commas;
	register int depth;
	register char *param_ptr;
	char *chmark;
	/* The buffer is 2*buffersize in size so that the check for the paramter
	 * size exceeding buffersize need only be made after each new line and
	 * at the end. */
	static char null_string[] = "\0";

	chmark = line;
	skip_space_and_com(line);
	/* If end of a line is encountered, read in a new line and continue. */
	while(*line == '\n')
	{
		line++;
		/* Copy rest of line into 'original_line' string. */
		if(DoMacroTracking)
			copy_original_line(chmark,line-chmark);
		/* Since the end of the current line was reached, then
		 * all macros will be available for substitution. */
		read_line(origin);
		chmark = line = origin;
		skip_space_and_com(line);
	}

	num_params = 0;
	num_commas = 0;
	while(*line != ')' && *line != '\0')
	{
		/* Each param will first be loaded into param_buffer. */
		param_ptr = param_buffer;
		depth = 0;
		while((depth > 0 || (*line != ',' && *line != ')')) && *line != '\0')
		{
			if(*line == '\n')
			{
				line++;
				/* Copy rest of line into 'original_line' string. */
				if(DoMacroTracking)
					copy_original_line(chmark,line-chmark);
				/* Put out one space for newline. */
				*param_ptr++ = ' ';
				/* Check to see if the parameter is too long. */
				if(param_ptr-param_buffer >= buffersize-1)
				{
					error("Macro param too large - use -H option");
					param_ptr = param_buffer;
				}
				/* Since the end of the current line was reached, then
				 * all macros will be available for substitution. */
				read_line(origin);
				chmark = line = origin;
				skip_space_and_com(line);
			}
			/* Skip over string literals copying them into the parameter string. */
			else if(is_quote(*line))
			{
				skip_quote(line, param_ptr);
			}
			/* Skip over and throw away comments since comments crossing line
			 * boundaries will throw off the line count. */
			else if(*(line) == '/' && *((line)+1) == '*')
			{
				*param_ptr++ = ' ';
				line += 2;
				while(*line != '*' || *(line+1) != '/')
				{
#ifdef NLS
					/* If the character is an NLS character pair then skip over the pair to
					 * avoid the second character being taken as part of a comment termination. */
					if(NLSOption && FIRSTof2((unsigned char)*line) && SECof2((unsigned char)*(line+1)))
						line++;
#endif
					line++;
				}
				line += 2;
			}
			else
			{
				if(*line == '(')
					depth++;
				else if(*line == ')')
					depth--;
				*param_ptr++ = *line++;
			}
		}


		/* Remove spaces from the end of the parameter. */
		back_over_space(param_ptr, param_buffer);
		/* Check to see if the parameter is too long. */
		if(param_ptr-param_buffer >= buffersize-1)
		{
			error("Macro param too large - use -H option");
			param_ptr = param_buffer+buffersize-2;
		}
		*param_ptr++ = '\0';

		/* Copy parsed parameter into param_array.  */
		param_array[num_params] = temp_alloc(param_ptr-param_buffer);
		strcpy(param_array[num_params++], param_buffer);

		if(*line == ',')
		{
			num_commas++;
			line++;
			skip_space_and_com(line);

			while(*line == '\n')
			{
				line++;
				/* Copy rest of line into 'original_line' string. */
				if(DoMacroTracking)
					copy_original_line(chmark,line-chmark);
				/* Since the end of the current line was reached, then
				 * all macros will be available for substitution. */
				read_line(origin);
				chmark = line = origin;
				skip_space_and_com(line);
			}
		}
	}
	if(*line == '\0')
		error("Unexpected end of macro invocation");
	else
		line++;
	if(DoMacroTracking)
		copy_original_line(chmark,line-chmark);

	if(num_params != expected_num_params)
	{
		if (num_commas+1 == expected_num_params)
			warning("Parameter holes filled with a null string");
		else
			error("Wrong number of parameters for macro");

		/* Fill remaining parameter holes with a null string. */
		while(expected_num_params > num_params)
		{
			param_array[--expected_num_params] = null_string;
		}
	}
	return line;
} /* parse_arguments */

copy_original_line(string,length)
char *string;
int length;
{
	if ( original_line_ptr+length >= original_line+buffersize-1)
		error("Macro invocation too large - use -H option");
	else
	{
		strncpy(original_line_ptr,string,length);
		original_line_ptr += length;
	}
} /* copy_original_line */

/*******************************************************************************
 * This routine expands the macro name completely. This includes rescanning, 
 * further replacement, argument substitution, and processing both # and ##
 * operators. The result of the expanded macro is represented by a tree.
 *
 * Input: 1. input - a buffer containing the macro name
 *        2. length - the length of the macro name
 *        3. define - structure for the macro define
 *        4. arg_array - arguments array containing the actual parameters
 *        5. check_marker - a flag indicating whether check 'blue' marker
 *               or not.
 *
 * Output: the expanded macro ( it's a 'tree' )
 *
 * Algorithm:
 *
 *	1. scan the replacement-list. If a parameter is identified, do argument
 *         substitution.  The parameter in the replacement list is replaced by 
 *         the corresponding arguemnt after all macros contained in the
 *         argument have been expanded. Also mark the start and end 
 *         position of the substituted argument. Any macro name found
 *         in the marked string during rescanning is not macro replaced.
 *
 *	2. temporarily disable the current macro name ( but don't delete
 *	   the macro name from the macro define table ). This is to prevent
 *	   endless expansion of recursion.
 *
 *      3. rescan the resulting preprocessing token sequence.
 *
 *	4. enable the current macro name.
 *
 *	5. return the expanded macro.
 *
 */ 

t_replacement_node * expand_macro (input,length,define,arg_array,check_marker)
char *input;
int length;
t_define *define;
t_param_array arg_array;
boolean check_marker;
{
	t_arg_array   arg_array_node;
	register char *string;
	char *macro_buffer;
	char *argument;
	int arg_length;
	int i;
	register boolean overflow;
	register char *macro_buffer_ptr;
	t_replacement_node *tree;
	t_define *get_define();
	t_replacement_node *get_node();
	char *tree_flattener();

	tree = get_node(UNDECIDED,NULL);
	if (define->has_params)
	{
		tree->node_id = FUNCTION_MACRO_NODE;
		for ( i=0; i < define->num_params; i++)
			arg_array_node[i] = ((t_replacement_node *) -1);
	}
	else
	{
		tree->node_id = OBJECT_MACRO_NODE;
	}
	tree->string = define->name;
	if ( macrodebug )
	{
		if (define->has_params)
		{
			int len=0;
			len = strlen(define->name);
			for ( i=0; i < define->num_params; i++)
				len += strlen(arg_array[i]);
			tree->macro  = temp_alloc(len+2+define->num_params+1);
			sprintf(tree->macro,"%s(",define->name);
			for ( i=0; i < define->num_params; i++)
			{
				strcat(tree->macro,arg_array[i]);
				if ( i+2 <= define->num_params )
					strcat(tree->macro,",");
			}
			strcat(tree->macro,")");
		}
		else
			tree->macro = define->name;
	}

	macro_buffer_ptr = macro_buffer = alloc_buffer();
	string = define->string;
	overflow = FALSE;
	/* This loop copies characters from string (the macro replacement 
	 * string) to macro_buffer.  If it encounters a macro parameter, then 
	 * the corresponding argument from arg_array is substituted. */
	while(*string != '\0')
	{
		if(is_special_char(*string))
		{
			if(*string == NORMAL_PARAM)
			{
				string++;
				i = (unsigned char)*string-128;
				if (arg_array_node[i] == ((t_replacement_node *) -1))
				{
				/* Macro substitute the parameter and keep 
				 * track of the macros which were substituted 
				 * in the nested invocation. */
					arg_array_node[i] = get_node(PARAM_NODE,NULL);
					argument_substitute(arg_array[i],arg_array_node[i],check_marker);
					arg_array[i] = tree_flattener(arg_array_node[i]);
				}
				argument = arg_array[i];
				arg_length = strlen(argument);
				if((arg_length) > (buffersize - (macro_buffer_ptr-macro_buffer)-1))
				{
					error("Macro param too large after substitution - use -H option");
					arg_length = buffersize - (macro_buffer_ptr-macro_buffer)-1;
					argument[arg_length-1] = '\0';
					overflow = TRUE;
				}
				strcpy(macro_buffer_ptr, argument);
				macro_buffer_ptr += arg_length;
				if ( overflow )
					break;
			}
			else if(*string == QUOTED_PARAM)
			{
				register int i, len;
				/* Quoted parameters have the argument copied over with double
				 * quotes around it. */
				string++;
				argument =arg_array[(unsigned char)*string-128];
				len = strlen(argument);
				for ( i=0; i<len; )
				{
					if ( argument[i] == BLUE_PAINT_MARKER )
					{
						strcpy(&argument[i],&argument[i+1]);
						len--;
						continue;
					}
					i++;
				}
				{
				register char *arg, *p;
				if ( *argument != '"')
					for (arg=argument; *arg != '\0'; arg++)
						if (*arg == '\t' || *arg == ' ')
						{
							p = arg;
							skip_space(p);
							*arg = ' ';
							strcpy(arg+1,p);
						}
				}
				arg_length = strlen(argument);
				/* Make sure there is room in the chunk for the new argument.
				 * The 2*arg_length is used in case every character is an escaped '"',
				 * and the extra 2 is for the quotes on the ends. */
				if((2*arg_length+2) > (buffersize - (macro_buffer_ptr-macro_buffer)-1))
				{
					error("Quoted macro param too large - use -H option");
					argument[(buffersize-3)/2] = '\0';
					break;
				}
				*macro_buffer_ptr++ = '"';
				while(*argument != '\0')
				{
					/* Every '"' or '\' in a literal should have a '\' prepended. */
					if(is_quote(*argument))
					{
						register char delimiter = *argument;

						if(delimiter == '"')
							*macro_buffer_ptr++ = '\\';
						*macro_buffer_ptr++ = *argument++;
						do
						{
							/* Skip the character following a '\' in case it is the delimiter. */
							if(*argument == '\\')
							{
								*macro_buffer_ptr++ = '\\';
								*macro_buffer_ptr++ = '\\';
								argument++;
								/* The character following the '\' may also be a '"' or '\'. */
								if(*argument == '"' || *argument == '\\')
									*macro_buffer_ptr++ = '\\';
							}
							else if(*argument == '"')
								*macro_buffer_ptr++ = '\\';
						}
						while(*argument != '\0' && (*macro_buffer_ptr++ = *argument++) != delimiter);
					}
					else
					{
						*macro_buffer_ptr++ = *argument++;
					}
				}
				*macro_buffer_ptr++ = '"';
			}
			else if(*string == SQUEEZED_PARAM)
			{
				/* Squeezed parameters simply have the argument copied over. */
				string++;
				argument = arg_array[(unsigned char)*string-128];
				arg_length = strlen(argument);
				if((arg_length) > (buffersize - (macro_buffer_ptr-macro_buffer)-1))
				{
					error("Macro param too large after substitution - use -H option");
					arg_length = buffersize - (macro_buffer_ptr-macro_buffer)-1;
					overflow = TRUE;
				}
				for (i=0; i<arg_length; i++)
					if ( argument[i] != BLUE_PAINT_MARKER )
					{
						*macro_buffer_ptr = argument[i];
						macro_buffer_ptr++;
					}
				if ( overflow )
					break;
			}
			else if(*string == CURRENT_LINE)
			{
				/* Copy the current line number to the output. */
				number_to_string(current_lineno, macro_buffer_ptr);
			}
			else if(*string == CURRENT_FILE)
			{
				/* Copy the current line number to the output. */
				*macro_buffer_ptr++ = '"';
				strcpy(macro_buffer_ptr, current_file->filename);
				macro_buffer_ptr += strlen(current_file->filename);
				*macro_buffer_ptr++ = '"';
			}
			else if(*string == CURRENT_DATE)
			{
				*macro_buffer_ptr++ = '"';
				strcpy(macro_buffer_ptr, date_string);
				macro_buffer_ptr += strlen(date_string);
				*macro_buffer_ptr++ = '"';
			}
			else /* Must be CURRENT_TIME. */
			{
				*macro_buffer_ptr++ = '"';
				strcpy(macro_buffer_ptr, time_string);
				macro_buffer_ptr += strlen(time_string);
				*macro_buffer_ptr++ = '"';
			}
			string++;
		}
		else
		{
			if( buffersize - (macro_buffer_ptr-macro_buffer) - 1 > 0 )
				*macro_buffer_ptr++ = *string++;
			else
				{
				error("Macro buffer too small - use -H option");
				break;
				}
		}
	}
	*macro_buffer_ptr = '\0';

	/*
	 * Rescan the resulting preprocessing token sequence for more macro 
	 * names to replace. But before rescanning, we temporarily disable
	 * the macro name to prevent it from being expanded in any later 
	 * scanning.
	 */
	define->disabled = TRUE;
	rescan(macro_buffer,tree);
	define->disabled = FALSE; 
	free_buffer(macro_buffer);
	return tree;
} /* expand_macro */

/*******************************************************************************
 *
 * This routine will evaluate a parameter.
 *
 * Input: 1. string - a string that need to be examined for macro substitution
 *        2. tree - a node that will be attached children underneath after
 *             completion of the examination of the above argument.
 *        3. check_marker - a flag indicating whether checking 'blue' marker
 *               is required or not.
 *
 * Output: a tree containing the resulting preprocessing tokens.
 *
 * Algorithm:
 *	Identify all macro names in an argument and then break the argument 
 *	into a series of substrings. For those substrings identified 
 *	as macros, do macro expansion. The expanded macros are trees.
 *	The rest of substrings are given a node each ( this node is called 
 *      'terminal' which can not be further expanded ). Then chain terminals 
 *      and macro trees.
 */

argument_substitute (string,tree,check_marker)
char *string;
t_replacement_node *tree;
boolean check_marker;
{
	char *line;  /* Pointer to current position */
	char *macro_name;
	char *start_position;       /* pointer to the beginning of a terminal */
	t_replacement_node *head, *previous_node, *current_node;
	int macro_name_length;
	t_define *define, *last_define;
	t_param_array arg_array;
	t_define *get_define();
	t_replacement_node *expand_macro();

	head = previous_node = get_node(UNDECIDED,NULL); 
	start_position = line = string;
	while(*line != '\0')
	{
		int len;
		char *p;
		if ( get_next_macro_name(&line,&macro_name,&macro_name_length,&define,TRUE) )
		{
			if ( (string < macro_name) && (*(macro_name-1) == BLUE_PAINT_MARKER) && check_marker ) 
			/* 'blue painted' macro, don't expand it to prevent 
			 * endless expansion of recursions. */
				continue;
			if ( define->disabled )
			{
			/* cannot expand this disabled macro, so paint it blue 
			 * to prevent it from being expanded in any later
			 * scanning.
			 */
				int size;
				previous_node->sibling = current_node = get_node(TERMINAL_NODE,previous_node); 
				len = macro_name - start_position;
				size = len + macro_name_length + 1; /* including marker */
				current_node->string  = temp_alloc(size+1);
				strncpy(current_node->string,start_position,len);
				current_node->string[len] = BLUE_PAINT_MARKER;
				strncpy(&(current_node->string[len+1]),macro_name,macro_name_length);
				current_node->string[size] = '\0';
				previous_node = current_node;
				start_position = line;
				continue;
			}
			if ( start_position != macro_name )
			{
			/* there's a substring before macro name, so just
			 * add it to the linked list.
			 */
				previous_node->sibling = current_node = get_node(TERMINAL_NODE,previous_node); 
				len = macro_name-start_position;
				current_node->string  = temp_alloc(len+1);
				strncpy(current_node->string,start_position,len);
				current_node->string[len] = '\0';
				previous_node = current_node;
			}
			if (define->has_params)
			{
				t_replacement_node *tree;
				char *chmark, *params, *new_line;
				int len;

				chmark = line;
				skip_space_and_com(line);
				if(*line == '(')
				{
					line++;
					line = parse_arguments(line, arg_array, define->num_params, line, FALSE );
				}
			}
			current_node = expand_macro(macro_name,macro_name_length,define,arg_array,FALSE);
			p = line;
			skip_space_and_com(p);
			/* in case the last pptoken after the expansion is a
			 * function-like macro name and it is followed by a '('.
			 */
			if (*p == '(' && ((last_define=get_last_function_macro(current_node, &last_id_painted_blue)) != NULL ))
			{
				char *new_line;

				if ( ! last_id_painted_blue )
				{
					*last_pptoken = '\0'; /* delete last pptoken */
					new_line = temp_alloc(strlen(last_define->name)+strlen(line)+1);
					sprintf(new_line,"%s%s",last_define->name,line); /* copy last pptoken */
					line = new_line;
				}
			}
			previous_node->sibling = current_node;
			current_node->previous = previous_node;
			previous_node = current_node;
			start_position = line;
		}
	} /* while */

	if (start_position != line)
	{
	int len;
		previous_node->sibling = current_node = get_node(TERMINAL_NODE,previous_node);
		len = line - start_position;
		current_node->string  = temp_alloc(len+1);
		strncpy(current_node->string,start_position,len);
		current_node->string[len] = '\0';
	}
 	/* attach the linked list to the tree */
	if ( head->sibling )
	{
		tree->child = head->sibling;
        	head->sibling->previous = tree;
	}
	else
		tree->child = NULL;
} /* argument_substitute */

/*******************************************************************************
 *
 * This routine will rescan a string and expand macros if any.
 *
 * Input: 1. string - a string that need to be examined for macro substitution
 *        2. tree - a node that will be attached children underneath after
 *             completion of the examination of the above argument.
 *
 * Output: a tree containing the resulting preprocessing tokens.
 *
 * Algorithm:
 *	Identify all macro names in a string and then break the argument 
 *	into a series of substrings. For those substrings identified 
 *	as macros, do macro expansion. The expanded macros are trees.
 *	The rest of substrings are given a node each. Then chain terminals 
 *      and macro trees.
 */
rescan (string,tree)
char *string;
t_replacement_node *tree;
{
	extern boolean if_section;
	char *line;  /* Pointer to current position */
	char *macro_name;
	char *start_position;       /* pointer to the beginning of a terminal */
	t_replacement_node *head, *previous_node, *current_node;
	int macro_name_length;
	t_define *define, *last_define;
	t_param_array arg_array;
	t_define *get_define();
	t_replacement_node *expand_macro();

	head = previous_node = get_node(UNDECIDED,NULL); 
	start_position = line = string;
	/* DTS # CLLbs00285: klee 09/25/92 */
	if ( if_section )
		evaluate_defined_keyword(line);
	/* DTS # CLLbs00285 */
	while(*line != '\0')
	{
		int len;
		char *p;

		define = NULL;
		if ( get_next_macro_name(&line,&macro_name,&macro_name_length,&define,TRUE) )
		{
			if ( (string < macro_name) && *(macro_name-1) == BLUE_PAINT_MARKER )
			/* 'blue painted' macro, don't expand it to prevent 
			 * endless expansion of recursions. */
				continue;
			if ( define->disabled )
			{
			/* cannot expand this disabled macro, so paint it blue 
			 * to prevent it from being expanded in any later
			 * scanning.
			 */
				int size;
paint_it:
				previous_node->sibling = current_node = get_node(TERMINAL_NODE,previous_node); 
				len = macro_name - start_position;
				size = len + macro_name_length + 1; /* including marker */
				current_node->string  = temp_alloc(size+1);
				strncpy(current_node->string,start_position,len);
				current_node->string[len] = BLUE_PAINT_MARKER;
				strncpy(&(current_node->string[len+1]),macro_name,macro_name_length);
				current_node->string[size] = '\0';
				previous_node = current_node;
				start_position = line;
				continue;
			}
			if ( start_position != macro_name )
			{
			/* there's a substring before macro name,
			 * copy it to the linked list.
			 */
				previous_node->sibling = current_node = get_node(TERMINAL_NODE,previous_node); 
				len = macro_name-start_position;
				current_node->string  = temp_alloc(len+1);
				strncpy(current_node->string,start_position,len);
				current_node->string[len] = '\0';
				previous_node = current_node;
			}
			if (define->has_params)
			{
				t_replacement_node *tree;
				char *chmark, *params, *new_line, *p;
				int len;

				chmark = line;
				skip_space_and_com(line);
				if(*line == '(')
				{
					line++;
					line = parse_arguments(line, arg_array, define->num_params, line, FALSE );
				}
			}
			current_node = expand_macro(macro_name,macro_name_length,define,arg_array,FALSE);
			/* if this macro name is painted blue then remove the
			 * marker because it should not be protected after
			 * expanding itself. Also, make sure the marker is 
			 * vaild.
			 */
			if ( (string < macro_name) && (*(macro_name-1) == BLUE_PAINT_MARKER) )
				*(macro_name-1) = ' ';
			p = line;
			skip_space_and_com(p);
			/* in case the last pptoken after the expansion is a
			 * function-like macro name and it is followed by a '('.
			 */
			if (*p == '(' && ((last_define=get_last_function_macro(current_node, &last_id_painted_blue )) != NULL ))
			{
				char *new_line;
				*last_pptoken = '\0'; /* delete last pptoken */
				new_line = temp_alloc(strlen(last_define->name)+strlen(line)+1);
				sprintf(new_line,"%s%s",last_define->name,line); /* copy last pptoken */
				line = new_line;
			}
			previous_node->sibling = current_node;
			current_node->previous = previous_node;
			previous_node = current_node;
			start_position = line;
		}
		else
		if ( define && define->disabled )
			goto paint_it;
	} /* while */

	if (start_position != line)
	{
	int len;
		previous_node->sibling = current_node = get_node(TERMINAL_NODE,previous_node);
		len = line - start_position;
		current_node->string  = temp_alloc(len+1);
		strncpy(current_node->string,start_position,len);
		current_node->string[len] = '\0';
	}
 	/* attach the linked list to the tree */
	if ( head->sibling )
	{
		tree->child = head->sibling;
        	head->sibling->previous = tree;
	}
	else
		tree->child = NULL;
} /* rescan */

/*******************************************************************************
 * This routine scans a string and returns the first found macro name in the
 * string.
 *
 * Input: 1. buffer_ptr ( by reference ) - pointer to a buffer containg a
 *           string.
 *        2. check_params - flag to check the '(' of a function-like macro
 *
 * Output:1. buffer_ptr ( by reference ) - pointer to the delemiter of a macro
 *           name if found.
 *        2. macro_name - pointer to the beginning of a macro name
 *        3. macro_name_length - the length of the macro name
 *        4. define_ptr - pointer to the structure of a macro define
 *        5. check_params - flag to check if it has a '('
 *        6. the returned value - status of searching for the first macro name
 *
 * Algorithm:
 */

boolean get_next_macro_name(buffer_ptr, macro_name, macro_name_length, define_ptr, check_params )
char **buffer_ptr;
char **macro_name;
int  *macro_name_length;
t_define **define_ptr;
boolean check_params;
{
	register char *line;
	char *chmark;
	boolean FoundIt;

	*define_ptr = NULL;
	FoundIt = FALSE;
	line = *buffer_ptr;
	while(*line != '\0')
	{
		if(is_id_start(*line))
		{
			int length;
			/* Mark the start of the identifier. */
			chmark = line;
			do { line++; } while(is_id(*line)); 
			if( (*define_ptr = get_define(chmark, length=line-chmark)) != NULL )
			{
				*macro_name = chmark;
				*macro_name_length = length;
				if(check_params && (*define_ptr)->has_params)
				{
					skip_space_and_com(line);
					if (*line != '(') /* not a macro */
						continue;
				}
				FoundIt = TRUE;
				break;
			}
			/* Else, the identifier wasn't a macro, so just
			 * skip over. 
			 */
		}
		else if (isdigit(*line))
		{
			line = skip_number(line);
		}
		else if (is_quote(*line))
		{
			register char delimiter = *line;
			line++;
			while(*line != delimiter && *line != '\n' && *line != '\0')
			{
			/* Skip the character following a '\' in case 
			 * it is the delimiter. But don't escape the 
			 * first char of an NLS pair. */
				if((*line == '\\') && (! NLScheck(line+1)))
					line++;
			/* If the character is an NLS character pair 
			 * then skip over the pair to avoid the second 
			 * character being taken as a quote terminating 
			 * the string. */
				if (NLScheck(line))
					line++;
				line++;
			}
			if(*line == delimiter)
				line++;
		}
		else if (C_Plus_Plus && *(line) == '/' && *((line)+1) == '/')
		{
			line += 2;
			/* DTS # CLLbs00334: klee 09/10/92 */
			while (*line != '\n' && *line != '\0')
				line++;
		}
		else if(*(line) == '/' && *((line)+1) == '*')
		{
			line +=2;
			while((*line != '*' || *(line+1) != '/') && *line != '\0')
			{
#ifdef NLS
				/* If the character is an NLS character pair 
				 * then skip over the pair to avoid the second 
				 * character being taken as part of a comment 
				 * termination. */
				if(NLSOption && FIRSTof2((unsigned char)*line) && SECof2((unsigned char)*(line+1)))
					line++;
#endif
				line++;
			}
			line += 2;
#ifdef APOLLO_EXT
			if (FirstToken == may_be_first)
				FirstToken = is_first;
#endif
		}
		else
		{
		/* Do a whole series of non-special characters in this fast 
		 * loop rather than in the outer slow loop.  A do while is 
		 * used because the first character must be copied regardless. 
		 */
			do
			{
				line++;
			}
			while(!is_special_to_get_sub_line[(unsigned char)*line]);
		}
	} /* while */
	*buffer_ptr = line;
	return (FoundIt);
} /* get_next_macro_name */

/*******************************************************************************
 * The tree_flattener() and copy_tree() routines flatten a tree. All 'terminals'
 * in the tree will be copied to the temporay buffer 'tree_buffer'.
 */
char *tree_buffer;      /* Pointer to the buffer */
int   tree_buffer_size; /* Size of the buffer */
static char *tree_buffer_ptr;  /* Current output position of the buffer */

char *tree_flattener (tree)
t_replacement_node *tree;
{
	char *result;
	int len;

	tree_buffer_ptr = tree_buffer;
	copy_tree(tree);
	len = tree_buffer_ptr - tree_buffer;
	result = temp_alloc(len+1);
	strncpy(result,tree_buffer,len);
	result[len] = '\0';
	return (result);
} /* tree_flattener */

copy_tree (tree)
t_replacement_node *tree;
{
	int i, len;
	t_replacement_node *child;
	int new_size;
	char *old_result;
	extern char *realloc();

	if ( tree == NULL ) return;
	if ( tree->node_id == TERMINAL_NODE )
	{
		len = strlen(tree->string);
		/* check buffer size */
		old_result = tree_buffer;
		new_size = tree_buffer_ptr - tree_buffer + (len + 1);
		while(tree_buffer_size < new_size)
		{
			tree_buffer_size += buffersize;
			if ((tree_buffer = realloc(tree_buffer, tree_buffer_size)) == NULL)
				error("Out of dynamic memory");
			tree_buffer_ptr += tree_buffer-old_result;
		}

		strcpy(tree_buffer_ptr,tree->string);
		tree_buffer_ptr += len;
	}

	/* traverse the tree */
	for (child = tree->child; child != NULL; child = child->sibling)
	{
		copy_tree(child);
	}
} /* copy_tree */

/*******************************************************************************
 * This routine will traverse a tree which was built for a macro expassion. 
 * The result will be copied to the output buffer 'result'. This routine uses
 * 'preorder traversal processing'.
 *
 * Input: a macro-replacement tree.
 *
 * Output: 1. result_buffer - pointer to output buffer 
 *         2. result_ptr - pointer to the current position.
 *         3. last_pptoken_node - pointer to the last node in the tree
 *
 * Algorithm:
 *
 * 1. if tree is NULL then return to caller; otherwise go to 2
 *
 * 2  visit the current node pointed by 'tree'.
 *
 * 3  traverse each child of the current node.
 *
 */
traverse (tree)
t_replacement_node *tree;
{
	t_replacement_node *child;
	int i, len;

	if ( tree == NULL ) return;
	/* visiting the current node */
        if ( tree->node_id == TERMINAL_NODE )
	{ /* copy to output buffer */
                last_pptoken_node = tree;
                len = strlen(tree->string);
                check_output_buffer(len+1);
		for (i=0; i<len; i++)
			if ( tree->string[i] != BLUE_PAINT_MARKER )
			{
				*result_ptr = tree->string[i];
                		result_ptr++;
			}
			/* Else, don't copy the blue paint markers */
	}
	/* start preorder traversal processing */
	for (child = tree->child; child != NULL; child = child->sibling)
	{
		traverse(child);
	}
} /* traverse */

/*
 * See if there is room in the allocated buffer for anything that will be
 * copied to the output buffer. If the room is not large engough to fit in
 * then enlarge the buffer.
 */
check_output_buffer(len)
int len;
{
	char *old_result = result_buffer;
	int new_size;
	extern char *realloc();

	new_size = result_ptr - result_buffer + len;
	while(result_size < new_size)
	{
		result_size += buffersize;
		if ((result_buffer = realloc(result_buffer, result_size)) == NULL)
			error("Out of dynamic memory");
		result_ptr += result_buffer-old_result;
	}
} /* check_output_buffer */

/*******************************************************************************
 * This routine finds the last pptoken from a tree. If the last pptoken is
 * is identical to a function-like macro name then return the pointer to
 * the structure of the macro define.
 *
 * Input: node - a node it could be a tree or a TERMINAL_NODE
 *
 * Output: last_id_painted_blue_ptr - pointer to a flag which indicates the
 *               last identifier is painted blue.
 *         returned value - a pointer to the structure of function-like macro 
 *               define
 *         last_pptoken - pointer to the beginning of the macro name in the
 *               string contained in the last node.
 *
 * Algorithm:
 */

t_define * get_last_function_macro(root,last_id_painted_blue_ptr)
t_replacement_node *root;
boolean *last_id_painted_blue_ptr;
{
	char *line;
	t_replacement_node *node, *tree;
	t_define *define, *previous_define;
	char *name;
	int   length;

	*last_id_painted_blue_ptr = FALSE;
	if ( root == NULL )
		return NULL;
	node = tree = root;
loop:
	switch ( tree->node_id )
	{
	case TERMINAL_NODE: break;
	case OBJECT_MACRO_NODE:
	case FUNCTION_MACRO_NODE:
		if ( !( node = tree->child ) )
				return NULL;
		break;
	default:fatal_cpp_error ("Internal macro substitution error - get_last_function_macro()");
		break;
	}
	for ( ;node->sibling; node = node->sibling ) /* go to the last child */
		;
	if ( node->node_id != TERMINAL_NODE )
	{
		tree = node;
		goto loop;
	}
	previous_define = define = NULL;
	line = node->string;
	while (*line != '\0')
	{
		last_pptoken = line;
		name = NULL;
		length = 0;
		if (get_next_macro_name(&line,&name,&length,&define,FALSE))
			previous_define = define;
	}
	if ( name && previous_define && previous_define->has_params > 0 )
	{
		if ( node->string != name && *(name-1) == BLUE_PAINT_MARKER )
			*last_id_painted_blue_ptr = TRUE;
		last_pptoken = name;
		return previous_define;
	}
	else
		return NULL;
} /* get_last_function_macro */

/* 
 * This routine will create a replacement_node.
 *
 * Input: 1. node_id - node identifier
 *        2. previous_node - the node that is before the new node on the liked
 *           list.
 *
 * Output: a node
 *
 * Algorithm:
 */
t_replacement_node *get_node(node_id,previous_node)
char node_id;
t_replacement_node *previous_node;
{
        t_replacement_node *result;

        result = (t_replacement_node *)temp_alloc(sizeof(t_replacement_node));
        result->node_id = node_id;
        result->child = NULL;
        result->sibling = NULL;
        result->previous = previous_node;
        result->string = NULL;
        result->macro = NULL;
        return result;
} /* get_node */

char *strsave(s)
char *s;
{
	char *t;
	t = temp_alloc(strlen(s)+1);
	strcpy(t,s);
	return (t);
}

init_substitute_module()
{
	NestLevel = 0;
}

dump_tree (tree)
t_replacement_node *tree;
{
t_replacement_node *child;

	NestLevel++;
	if ( tree == NULL ) return;

	fprintf(stderr,"%*sNode_id:",NestLevel*4,"");
	switch (tree->node_id)
	{
	case TERMINAL_NODE:     fprintf(stderr," TERMINAL_NODE\n");
				break;
	case OBJECT_MACRO_NODE: fprintf(stderr," OBJECT_MACRO_NODE\n"); 
				break;
	case FUNCTION_MACRO_NODE:fprintf(stderr," FUNCTION_MACRO_NODE\n");
				break;
	case PARAM_NODE:        fprintf(stderr," PARAM_NODE\n");
				break;
	default:                fprintf(stderr," <no name>\n");
	}
	if ( tree->node_id == FUNCTION_MACRO_NODE )
		fprintf(stderr,"%*sString: '%s'\n",NestLevel*4,"",tree->macro);
	else
		fprintf(stderr,"%*sString: '%s'\n",NestLevel*4,"",tree->string);
	fprintf(stderr,"%*sCurr:[%x] ",NestLevel*4,"",tree);
	fprintf(stderr,"Prev:[%x]\n",tree->previous);
	fprintf(stderr,"%*sChild:[%x] ",NestLevel*4,"",tree->child);
	fprintf(stderr,"Sib:[%x]\n\n",tree->sibling);

	for (child = tree->child; child != NULL; child = child->sibling)
	{
		dump_tree(child);
	}
	NestLevel--;
} /* dump_tree */
