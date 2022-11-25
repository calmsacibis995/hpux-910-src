/* $Revision: 70.10 $ */

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

#ifdef CPP_CSCAN
extern boolean K_N_R_Mode;		/* Do K&R processing if set */
#endif	

#define NUMCHARS 256

t_define *define_table[TABLESIZE];

char id_chars[NUMCHARS];

/*
 * These function pointers, if set, are used to notify the caller of cpp
 * when a macro is defined or undefined. The caller can choose to remember
 * this information so that subsequent preprocessing of the same file later
 * can be skipped. The caller can provide the macro defines and undefines
 * which would have been encountered. */
void (*notify_define) ();
void (*notify_undefine) ();

typedef struct pre_define_node {
	char *name;
	char *string;
	struct pre_define_node *next;
} t_pre_define;

typedef struct pre_undefine_node {
	char *name;
	struct pre_undefine_node *next;
} t_pre_undefine;
	
static t_pre_define *pre_define_list;
static t_pre_undefine *pre_undefine_list;

extern boolean StaticAnalysisOption;

extern char *directive_macro_info;
extern int current_lineno;

#if defined(CXREF)
extern int doCXREFdata;
extern char *xcopy();
#endif

/*
 * This routine does any initialization required by this module
 * for each main source file processed.
 */
void init_define_module()
{
	int i;

	for (i = 0; i < TABLESIZE; ++i)
		define_table[i] = NULL;
	for (i = 0; i < NUMCHARS; ++i)
		id_chars[i] = 0;
	pre_define_list = NULL;
	pre_undefine_list = NULL;
}

/*
 * This routine handles the '#undef' directive.  The line passed in points
 * to the character immediately following the '#undef'.  This routine parses
 * out the name of the macro being undeffed and removes it from the table
 * of defined macros.
 */
handle_undef(line)
register char *line;
{
	register char *id;
	register char *ch_ptr;
	register int id_length;
	register int index;
	register t_define *define_list;
	boolean is_illegal();
	extern char *mark_macros();

	/* Move 'line' up to the first character of the macro. */
	skip_space(line);
	/* Create directive_macro_info in case any macros are in the undef. */
	if(StaticAnalysisOption)
	{
		ch_ptr = line;
		while(*ch_ptr != '\n')
			ch_ptr++;
		directive_macro_info = temp_alloc(8+2*(ch_ptr-line)+2);
		*directive_macro_info = DIRECTIVE_START;
		strcpy(directive_macro_info+1, "#undef ");
		mark_macros(directive_macro_info+8, line, ch_ptr-line);
	}
	if(!is_id_start(*line))
		bail("Missing or illegal macro name");
	/* Parse the macro name. */
	id = line;
	while(is_id(*line))
		line++;
#if defined(CXREF)
	if(doCXREFdata)
		ref(id,current_lineno);
#endif			
	if(is_illegal(id, line-id))
		error("Illegal macro name in #undef");
	remove_define(id, line-id);

	/* Notify caller of undefine, if requested */
	if (notify_undefine)
	{
		char *macro_name = temp_alloc (line - id + 1);
		strncpy (macro_name, id, line - id);
		macro_name[line - id] = '\0';
		(*notify_undefine) (macro_name);
	}

	/* Make sure the macro name was the only thing on the line. */
	if(*line != '\n')
		error("Extra characters at end of undef");
}


/*
 * This routine handles a #define directive.  The 'line' points to the character
 * immediately following the 'define'.  The routine distinguishes between macros
 * with parameters and those without.  The resulting macro is recorded in the
 * define_table data structure for later reference.  All whitespace in a macro
 * replacement string is represented by one space to make all whitespace
 * identical for purposes of recognizing a macro redefinition.
 */
handle_define(line)
register char *line;
{
	t_param_array param_array;
	register char *ch_ptr;
	register char *macro_name;
	register char *param_name;
	register t_define *new_define;
	register int num_params = 0;
	register boolean has_params;
	char *start_of_macro;
	char *make_replacement_string();
	char *mark_macros();
	boolean is_illegal ();

	/* Find first character of macro name. */
	if (StaticAnalysisOption
# ifdef CXREF
	     || doCXREFdata
# endif
	   )
		{
		arrange_NLMARKS(line);
		}
	start_of_macro = line;
	skip_def_space(line);
	if(!is_id_start(*line))
		bail("Missing or illegal macro name");
	/* Scan to end of macro name and record name in 'macro_name' */
	ch_ptr = line;
	while(is_id(*line))
		line++;
	macro_name = perm_alloc(line-ch_ptr+1);
	strncpy(macro_name, ch_ptr, line-ch_ptr);
	macro_name[line-ch_ptr] = '\0';
#if defined(CXREF) && !defined(REF_ONLY)
	if(doCXREFdata)
		def(macro_name,current_lineno);
#endif			
	/* See if the macro name is illegal. */
	if(is_illegal(macro_name, line-ch_ptr))
		error("Illegal macro name in #define");
	if (StaticAnalysisOption
# ifdef CXREF
	     || doCXREFdata
# endif
	   )
	{ /* Skip any NL_MARKS after the macro name */
		while(*line==NL_MARK)
		{
			line++;
			current_lineno++;
		}
	}
	/* See if this is a macro with parameters. */
	has_params = (*line == '(');
	if(has_params)
	{
#if defined(CXREF) && !defined(REF_ONLY)
		if(doCXREFdata)
			newf(macro_name,current_lineno);
#endif			
		line++;
		skip_def_space(line);
		/* Parse the parameter names into 'param_array' */
		while(*line != ')')
		{
			if(!is_id_start(*line))
				bail("Bad macro parameter");
			ch_ptr = line;
			while(is_id(*line))
				line++;
			param_name = perm_alloc(line-ch_ptr+1);
			strncpy(param_name, ch_ptr, line-ch_ptr);
			param_name[line-ch_ptr] = '\0';
			if(num_params >= MAXPARAMS)
				bail("Too many macro parameters");
			param_array[num_params++] = param_name;
#if defined(CXREF) && !defined(REF_ONLY)
			if(doCXREFdata)
				def(param_name,current_lineno);
#endif			
			skip_def_space(line);
			if(*line == ',')
			{
				line++;
				skip_def_space(line);
			}
		}
		line++;
	}
	/* Skip spaces preceding the replacement string. */
	skip_def_space(line);
	/* Mark the beginning of the replacement string. */
	ch_ptr = line;
	while(*line != '\n')
		line++;
	/* Copy the replacement string using mark_macros. */
	if(StaticAnalysisOption)
	{
		directive_macro_info = temp_alloc(8+(ch_ptr-start_of_macro)+2*(line-ch_ptr)+2);
		*directive_macro_info = DEFINE_START;
		strcpy(directive_macro_info+1, "#define");
		strncpy(directive_macro_info+8, start_of_macro, ch_ptr-start_of_macro);
		mark_macros(directive_macro_info+8+(ch_ptr-start_of_macro), ch_ptr, line-ch_ptr);
	}
	/* Now using ch_ptr as a temp for the replacement string. */
	ch_ptr = make_replacement_string(ch_ptr, has_params, num_params, param_array);
	add_define(macro_name, ch_ptr, has_params, num_params, param_array);

	/* Notify caller of macro define, if requested */
	if (notify_define)
		(*notify_define) (macro_name, ch_ptr, has_params, num_params, param_array);
}


/*
 * This routine shifts embedded new line marks to the right of the 
 * preprocessing tokens.  Makes it much easier to process the define
 * when there are no ids or numbers with embedded new line marks.
 */
arrange_NLMARKS(string)
register char *string;
{
register char *tok,*dest;

	while(*string != '\n')
	{
		if(is_id_start(*string))
		{
			tok = string;
			while(is_id(*string)||(*string==NL_MARK))
				string++;
			shift_embedded_NLMARKS(tok, string);
		}
		else if(isdigit(*string))
		{
			tok = string;
			string = skip_def_number(string);
			shift_embedded_NLMARKS(tok, string);
		}
		else if(is_quote(*string))
		{
			dest = tok = string;
			skip_quote(string, dest);
			shift_embedded_NLMARKS(tok, string);
		}
		else if(*string == '#')
		{
			tok = string++;
			while(*string==NL_MARK)
				string++;
			if(*string=='#')
				shift_embedded_NLMARKS(tok, ++string);
		}
		else
			string++;
	}
}
/*
 * Between 'start' and 'stop' there are embedded NL_MARKS.  The purpose of
 * this routine is to shift all the embedded NL_MARKS towards 'stop', leaving
 * the preprocessing token at the left with no embedded NL_MARKS 
 */
#define PLACE_HOLD_NL 1
char *shift_buffer;
shift_embedded_NLMARKS(start,stop)
register char *start,*stop;
{
int col_count = 0;
boolean embedded_NLs = FALSE;
	/*for each NL_MARK; shift left over it and put ' '  at end*/
	while((start < stop) && (*start != PLACE_HOLD_NL))
	{
		if (*start == NL_MARK)
		{
			embedded_NLs = TRUE;
			col_count = 0;
			memmove(start,start+1,stop-start-1);
			*(stop-1) = PLACE_HOLD_NL;
		}
		else
		{
			col_count++;
			start++;
		}
	}
	/* put back NL_MARKS at end */
	while(start < stop) /* put back NL_MARKS */
		*start++ = NL_MARK;
	/* now at end of token, pad with blanks to column adjust for */
	/* that part of the token after the last NL_MARK             */
	if(col_count && embedded_NLs)
	{
		strcpy(shift_buffer,stop);
		strcpy(stop+col_count,shift_buffer);
		while(col_count--)
			*(++stop) = ' ';
	}
}

/*
 * This routine copies the replacement string from 'original' to 'string'
 * changing parameter names into a two character sequence.  The first character
 * indicates the type of parameter.  The second character has value
 * equal to the parameter's position.  The types of parameters are:
 *   NORMAL_PARAM   - normal 
 *   QUOTED_PARAM   - preceded by #
 *   SQUEEZED_PARAM - preceded or followed by ##, but not preceded by #
 * Also, whitespace surrounding '##' is removed.
 */
char *replace_buffer;
char *make_replacement_string(original, has_params, num_params, param_array)
register char *original;
int has_params;
int num_params;
t_param_array *param_array;
{
	register char *string;
	register char *id;
	register int number;
	register boolean in_host_string = 0;
	int param_type;
	
	string = replace_buffer;
	while(*original != '\n')
	{
		/* Turn consecutive spaces into a single space unless inside a string. */
		if((*original==' ' || *original=='\t' || *original==NL_MARK)
		    && !in_host_string)
		{
			*string++ = ' ';
			skip_def_space(original);
		}
		else if(is_quote(*original))
		{
		/* CLLca01942. Checking if the closing quote is missing (
		 * perennial ). The standard says (2.1.1.2): a source file 
		 * shall not end in a partial preprocessing token ...
		 * klee: 920225
		 */
			char *p;
			boolean found_closing_quote=FALSE;

			for (p=original+1;*p != '\0';p++)
			   if (*p == *original) {
			      found_closing_quote=TRUE;
			      break;
			   }
			if ( !	found_closing_quote )
			   error("unterminated string or character constant");
		 /* CLLca01942 */
			skip_quote(original, string);
		}
		/* Skip over character following '\' to avoid having following character be terminating '"'. */
		/* If the character is an NLS character pair then skip over the pair to
		 * avoid the second character being taken as a quote terminating the string. */
#ifdef NLS
		else if(in_host_string && ((*original == '\\' && !FIRSTof2((unsigned char)*(original+1)))
		|| (NLSOption && FIRSTof2((unsigned char)*original) && SECof2((unsigned char)*(original+1)))))
#else
		else if(in_host_string && (*original == '\\'))
#endif
		{
			*string++ = *original++;
			*string++ = *original++;
		}
		/* Check for '#' operator or start of '##' operator. */
		else if(*original == '#')
		{
			++original;
			/* Check for '##' operator. */
			if(*original == '#')
			{
				++original;
				/* Is this the start of the replacement string? */
				if(string == replace_buffer)
				{
					error("Cannot have '##' at beginning of replacement string");
					continue;
				}
				/* Remove whitespace preceding the '##' operator. */
				back_over_space(string, replace_buffer);
				/* Remove whitespace following the '##' operator. */
				skip_def_space(original);
				/* Is this the end of the replacement string? */
				if(*original == '\n')
				{
					error("Cannot have '##' at end of replacement string");
					continue;
				}
				/* A parameter preceded or followed by '##' is type SQUEEZED_PARAM. */
				param_type = SQUEEZED_PARAM;
				/* Mark a parameter which was followed by this '##', but wasn't preceded
				 * by a '#' as type SQUEEZED_PARAM. */
				if((string-2 >= replace_buffer) 
					&& (*(string-2) == NORMAL_PARAM))
				{
				/* don't clobber an already marked paramater */
					if ((string-3 >= replace_buffer) && (*(string-3) <= SQUEEZED_PARAM))
						;
					else
						*(string-2) = param_type;
				}
			}
			else
			{
				/* A parameter preceded by '#' is type QUOTED_PARAM. */
				param_type = QUOTED_PARAM;
			}
			/* Skip to start of id potentially following the '#' or '##' operator. */
			skip_def_space(original);
			id = original;
			while(is_id(*original))
				original++;
			/* If the id matched a parameter then copy over that
			 * parameter's number.  Otherwise, copy over the id. */
			if((number = param_number(id, original-id, num_params, param_array)) != NON_PARAM)
			{
				*string++ = param_type;
/* CLLca01931. Encoding parameter number by adding 127 (set 8th bit of char */
/* on) to it.  This is to differentiate it from the tab character, which is */
/* removed by back_over_space() when processing the concatenator '##'.      */
/* pkwan 920117 */
/*				*string++ = number+1; */
				*string++ = (unsigned char) (number+128);
/*CLLca01931*/
			}
			else
			{
				if(param_type == QUOTED_PARAM)
				{
					error("The '#' operator must precede a parameter");
					continue;
				}
				else
				{
					strncpy(string, id, original-id);
					string += original-id;
				}
			}
		}
		else if(has_params && is_id_start(*original))
		{
			id = original;
			while(is_id(*original))
				original++;
			/* If the id matched a parameter then copy over that
			 * parameter's number.  Otherwise, copy over the id. */
			if((number = param_number(id, original-id, num_params, param_array)) != NON_PARAM)
			{
				*string++ = NORMAL_PARAM;
/* CLLca01931  pkwan  920117 */
/*				*string++ = number+1; */
				*string++ = (unsigned char) (number+128);
/*CLLca01931*/
			}
			else
			{
				strncpy(string, id, original-id);
				string += original-id;
			}
		}
       	/* ignore the NL_MARK, but increment the current_lineno */ 
		else if(*original == NL_MARK)
		{
			++original;
			++current_lineno;
		}
		else
			*string++ = *original++;
	}
	*string = '\0';
	/* Now dynamically allocate the right amount of space for the replacement
	 * string and return that. */
	string = perm_alloc(string-replace_buffer+1);
	strcpy(string, replace_buffer);
	return string;
}

/*
 * This routine returns the parameter number of the given id if it is a
 * parameter.  Otherwise, it returns NON_PARAM.
 */
int param_number(id, length, num_params, param_array)
char *id;
int length;
int num_params;
t_param_array param_array;
{
	register int index;

	/* See if the id matches a parameter. */
	for(index = 0; index < num_params; index++)
		if(strncmp(id, param_array[index], length) == 0 && param_array[index][length] == '\0')
#if defined(CXREF) && ! defined (NO_MACRO_FORMAL)
		{
			if(doCXREFdata)
				ref(param_array[index],current_lineno);
			return index;
		}
#else
			return index;
#endif			
	return NON_PARAM;
}


add_define(name, string, has_params, num_params, param_array)
char *name;
char *string;
boolean has_params;
int num_params;
t_param_array param_array;
{
	register int index, param_count, param_count2;
	register t_define *new_define, *define_list;
	register int length, bitval;
	register char *ch_ptr;

	length = strlen(name);
	index = string_hash(name, length)%TABLESIZE;
	/* See if this is a redefinition of a previously defined macro name. */
	define_list = define_table[index];
	while(define_list != NULL)
	{
		if(strcmp(define_list->name, name) == 0)
		{
			/* Redeclaration not an error if it is identical to previous one. */
			if(strcmp(define_list->string, string) != 0
			|| define_list->has_params != has_params
			|| define_list->num_params != num_params)
			{
				warning("Redefinition of macro %s", name);
				remove_define(name, strlen(name));
				break;
			}
			else
			{
				param_count = num_params;
				while(param_count-- > 0)
				{
					if(strcmp(define_list->param_array[param_count], param_array[param_count]) != 0)
					{
						warning("Redefinition of param names for macro %s", name);
						remove_define(name, strlen(name));
						goto add_new_define;
					}
				}
			}
			return;
		}
		define_list = define_list->next;
	}
add_new_define:
	new_define = NEW(t_define);
	new_define->name = name;
	new_define->string = string;
	new_define->has_params = has_params;
	new_define->num_params = num_params;
	new_define->disabled = FALSE;
	new_define->param_array = (char **)perm_alloc(num_params*sizeof(char *));
	new_define->next = define_table[index];
	/* Copy the parameter names into new_define->param_array and make
	 * sure there are no duplicates. */
	param_count = num_params;
	while(param_count-- > 0)
	{
		new_define->param_array[param_count] = param_array[param_count];
		param_count2 = num_params;
		while(--param_count2 > param_count)
		{
			if(strcmp(new_define->param_array[param_count2],
			          new_define->param_array[param_count]) == 0)
			{
				error("Duplicate param names for macro %s", name);
				break;
			}
		}
	}
	define_table[index] = new_define;
	/* Mark the id_chars array with the character information from this define. */
	if((length = strlen(name)) > 8)
		length = 8;
	ch_ptr = name;
	bitval = 1;
	while(length > 0)
	{
		id_chars[*ch_ptr] |= bitval;
		ch_ptr++;
		bitval <<= 1;
		length--;
	}
}


remove_define(name, length)
char *name;
int length;
{
	register int index;
	register t_define *define_list;

	/* Find the hash list which would contain this macro. */
	index = string_hash(name, length)%TABLESIZE;
	/* If the list is not empty, then search for the macro and
	 * delete it if it is found. */
	if(define_table[index] != NULL)
	{
		/* Check the first macro in the list. */
		if(strncmp(define_table[index]->name, name, length) == 0 && define_table[index]->name[length] == '\0')
		{
			define_table[index] = define_table[index]->next;
			return;
		}
		/* Check all other macros. */
		define_list = define_table[index];
		while(define_list->next != NULL)
		{
			if(strncmp(define_list->next->name, name, length) == 0 && define_list->next->name[length] == '\0')
			{
				define_list->next = define_list->next->next;
				return;
			}
			define_list = define_list->next;
		}
	}
}


/*
 * This routine returns the define macro which matches the given id.  If none
 * match then it returns NULL.
 */
t_define *get_define(id, length)
char *id;
int length;
{
	register t_define *define;

	define = define_table[string_hash(id, length)%TABLESIZE];
	while(define != NULL)
	{
		/* See if the macro matches.  This means that the names match and that if the
		 * defined macro has parameters then the invocation needs to have parameters. */
		if(strncmp(id, define->name, length) == 0 && define->name[length] == '\0')
#if defined(CXREF)
		{
			if(doCXREFdata)
				ref(define->name,current_lineno);
			return define;
		}
#else
			return define;
#endif		
		define = define->next;
	}
	return NULL;
}


handle_pre_define(argument)
char *argument;
{
	char *ch_ptr;
	t_pre_define *pre_define;
	
	if(is_id_start(*argument))
	{
		pre_define = NEW(t_pre_define);
		ch_ptr = pre_define->name = perm_alloc(strlen(argument)+1);
		while(is_id(*argument))
			*ch_ptr++ = *argument++;
		*ch_ptr = '\0';
		if(*argument == '=')
		{
			argument++;
			ch_ptr = pre_define->string = perm_alloc(strlen(argument)+1);
			while(*argument != '\0')
				*ch_ptr++ = *argument++;
			*ch_ptr = '\0';
		}
		else
			pre_define->string = "1";
		if(*argument == '\0')
		{
			pre_define->next = pre_define_list;
			pre_define_list = pre_define;
			return;
		}
	}
	error("bad syntax for argument to '-D' option");
}

handle_pre_undefine(argument)
char *argument;
{
	char *ch_ptr;
	t_pre_undefine *pre_undefine;
	
	if(is_id_start(*argument))
	{
		pre_undefine = NEW(t_pre_undefine);
		ch_ptr = pre_undefine->name = perm_alloc(strlen(argument)+1);
		while(is_id(*argument))
			*ch_ptr++ = *argument++;
		*ch_ptr = '\0';
		if(*argument == '\0')
		{
			pre_undefine->next = pre_undefine_list;
			pre_undefine_list = pre_undefine;
			return;
		}
	}
	error("bad syntax for argument to '-U' option");
}

init_defines()
{
	char *string;
	t_pre_define *pre_define;
	t_pre_undefine *pre_undefine;

	/* put definitions here which are predefined inside the preprocessor */
	/* but MAY BE REDEFINED by the user. */
                
#if defined(APOLLO_EXT)
	/* If we are on Apollo, and we are not the C++ preprocessor,
	   then both __STDC__ and __STDCPP__ can be undefined on the command line */
	/* But we don't define them if we are in K&R mode */
	if (!C_Plus_Plus
#ifdef CPP_CSCAN
	    && !K_N_R_Mode
#endif
	   )
	{
	add_define ("__STDCPP__", "1", FALSE, 0, NULL);
	add_define("__STDC__", "1", FALSE, 0, NULL); 
        }
#endif /* APOLLO_EXT */

	/* Then set up the -D defines. */
	pre_define = pre_define_list;
	while(pre_define != NULL)
	{
		add_define(pre_define->name, pre_define->string, FALSE, 0, NULL);
		pre_define = pre_define->next;
	}
	/* Then undo the -U macros. */
	pre_undefine = pre_undefine_list;
	while(pre_undefine != NULL)
	{
		remove_define(pre_undefine->name, strlen(pre_undefine->name));
		pre_undefine = pre_undefine->next;
	}
	/* Finally set up the special predefines which cannot be undeffed. */
	string = perm_alloc(2);
	*string = CURRENT_LINE;
	*(string+1) = '\0';
	add_define("__LINE__", string, FALSE, 0, NULL);
	string = perm_alloc(2);
	*string = CURRENT_FILE;
	*(string+1) = '\0';
	add_define("__FILE__", string, FALSE, 0, NULL);
	string = perm_alloc(2);
	*string = CURRENT_DATE;
	*(string+1) = '\0';
	add_define("__DATE__", string, FALSE, 0, NULL);
	string = perm_alloc(2);
	*string = CURRENT_TIME;
	*(string+1) = '\0';
	add_define("__TIME__", string, FALSE, 0, NULL);

	if (C_Plus_Plus)
		add_define("__STDCPP__", "1", FALSE, 0, NULL);

#ifndef APOLLO_EXT
        /* Non-Apollo users of this cpp don't allow __STDC__
	   to be undefined on the command line.
	   The undefine is forbidden by ANSI within the program text but
	   it says nothing about the command line */
	if (
#ifdef CPP_CSCAN
	    !K_N_R_Mode
#else
	    !C_Plus_Plus
#endif
	   )
		add_define("__STDC__", "1", FALSE, 0, NULL); 
#endif /* APOLLO_EXT */
}

/*
 * This routine indicates if a macro name is illegal in a #define or #undef.
 */
boolean is_illegal(macro, length)
char *macro;
int length;
{
	/* See if the macro name is 'defined' which is illegal. */
	if(strncmp(macro, "defined", 7) == 0 && length == 7)
		return TRUE;
	if (
#ifdef CPP_CSCAN
	    !K_N_R_Mode &&
#endif
	    C_Plus_Plus &&
	    length == 10 &&
	    strncmp(macro, "__STDCPP__", 10) == 0)
		return TRUE;
	if(length != 8)
		return FALSE;
	if(strncmp(macro, "__LINE__", 8) == 0
	|| strncmp(macro, "__FILE__", 8) == 0
	|| strncmp(macro, "__DATE__", 8) == 0
	|| strncmp(macro, "__TIME__", 8) == 0
	|| (
#ifdef CPP_CSCAN
	    !K_N_R_Mode &&
#endif
	    (!C_Plus_Plus && strncmp(macro, "__STDC__", 8) == 0)
           )
	)
		return TRUE;
	return FALSE;
}

/*
 * This routine copies from line into result inserting a ^B in front
 * of any defined macros.  It returns a pointer to the '\0' at the
 * end of result.
 */
char *mark_macros(result, line, length)
register char *result, *line;
register int length;
{
	register char *chmark;
	register int idlength;
	char *oldcharptr, oldchar;

	
	oldcharptr = &line[length];
	oldchar = *oldcharptr;
	*oldcharptr = '\0';
	while(*line != '\0')
	{
		if(is_id_start(*line))
		{
			chmark = line;
			while(is_id(*line))
				line++;
			idlength = line-chmark;
			if(get_define(chmark, idlength))
				*result++ = MACRO_MARK;
			strncpy(result, chmark, idlength);
			result += idlength;
		}
		else if(isdigit(*line))
		{
			chmark = line;
			line = skip_number(line);
			/* Copy over the number. */
			while(chmark < line)
				*result++ = *chmark++;
		}
		else if(is_quote(*line))
		{
			chmark = line;
			skip_quote(line, result);
		}
		/* Skip over comments copying them into the result string and keeping
		 * track of the number of newlines copied over to keep the output line
		 * count in sync. */
		else if(*(line) == '/' && *((line)+1) == '*')
		{
			chmark = line;
			*result++ = *line++;
			*result++ = *line++;
			while((*line != '*' || *(line+1) != '/') && *line != '\0')
			{
#ifdef NLS
				/* If the character is an NLS character pair then skip over the pair to
				 * avoid the second character being taken as part of a comment termination. */
				if(NLSOption && FIRSTof2((unsigned char)*line) && SECof2((unsigned char)*(line+1)))
					*result++ = *line++;
#endif
				*result++ = *line++;
			}
			if(*line == '\0')
				error("Non token comment encountered while marking macros");
			else
			{
				*result++ = *line++;
				*result++ = *line++;
			}
		}
		else if(*line == '\n')
		{
			line++;
			*result++ = NL_MARK;
		}
		else
		{
			*result++ = *line++;
		}
	}
	*result++ = END_MARK;
	*result = '\0';
	*oldcharptr = oldchar;
	return result;
}


#ifdef DEBUG
print_defines()
{
	int index;
	t_define *define;
	char *p;

	for(index = 0; index < TABLESIZE; index++)
	{
		define = define_table[index];
		if(define != NULL)
		fprintf(stderr, "At table index %d:\n", index);
		while(define != NULL)
		{
			if(define->has_params)
			{
				fprintf(stderr, "%s(%d) = ", define->name, define->num_params);
				p = define->string;
				while(*p != '\0')
				{
					if(*p == NORMAL_PARAM)
						fprintf(stderr, "$%d", *++p);
					else if(*p == QUOTED_PARAM)
						fprintf(stderr, "#%d", *++p);
					else if(*p == SQUEEZED_PARAM)
						fprintf(stderr, "##%d", *++p);
					else if(*p == CURRENT_LINE)
						fprintf(stderr, "<line>");
					else if(*p == CURRENT_FILE)
						fprintf(stderr, "<file>");
					else
						fprintf(stderr, "%c", *p);
					p++;
				}
				fprintf(stderr, "\n");
			}
			else
				fprintf(stderr, "%s = %s\n", define->name, define->string);
			define = define->next;
		}
	}
}


print_id_chars()
{
	register int i, j, bitval;

	for(i = 0; i < NUMCHARS; i++)
	{
		if(id_chars[i] != 0)
		{
			fprintf(stderr, "%c:", i);
			bitval = 1;
			for(j = 0; j < 8; j++)
			{
				if(id_chars[i] & bitval)
					fprintf(stderr, " %d", j);
				bitval <<= 1;
			}
			fprintf(stderr, "\n");
		}
	}
}
#endif
