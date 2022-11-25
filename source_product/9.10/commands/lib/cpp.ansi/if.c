/* $Revision: 72.1 $ */

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
#ifdef OPT_INCLUDE
#include "file.h"
#endif
#define MAX_IF_DEPTH 100

/* DTS # CLLca01939 klee 920302. 
 * The old scheme doesn't detect multiple #else statements. To solve this
 * problem, we use a stack mechanism. This mechanism consists of a stack
 * and four functions: push(), pop(), top(), and pop_if(). The push()
 * function is called by handle_ifdef(), handle_ifndef(), and handle_if().
 * And handle_endif() calls the pop_if() function to pop out the matching
 * #ifdef, or #ifndef, or #if and any directives in between the current
 * #endif directive and the matching directive. Both the handle_elif() and
 * handle_else() functions check the previous directive ( on top of the
 * stack ), if it's #else then issue an error message; otherwise push
 * itself into the stack. The stack size is initialized to STACK_SIZE and 
 * will be enlarged if run out of stack space.
 */

/* Identifiers for the directive words: */
#define INVALID	0	/* invalid directive word */
#define IFDEF	1	/* #ifdef */
#define IFNDEF	2	/* #ifndef */
#define IF	3	/* #if */
#define ELIF	4	/* #elif */
#define ELSE	5	/* #else */
#define ENDIF	6	/* #endif */

/* Stack info: */
#define STACK_SIZE	256
char *stack;		/* pointer to the stack ( a dynamic storage ). The first
			 * entry of the stack ( i.e. stack[0] ) is called the 
			 * bottom of the stack which cannot be used.
			 */

char *available;	/* pointer to the next available entry of the stack.
			 * It starts from the second entry of the stack ( i.e.
			 * stack[1] ).
			 */

int  stack_size;	/* the size of the stack. It is initialzed to STACK_SIZE
			 * in the init_if_module() function. And it will be
			 * enlarged if no more stack space is deteced by the
			 * push() function. 
			 */
/* DTS # CLLca01939 */

boolean has_been_true[MAX_IF_DEPTH+1];
boolean is_true[MAX_IF_DEPTH+1];
int depth;
char *if_line;
long if_value;

extern boolean StaticAnalysisOption;

extern char *directive_macro_info;
extern int current_lineno;

#if defined(CXREF)
extern int doCXREFdata;
#endif

/*
 * This routine does any initialization required before processing
 * a new file.
 */
void init_if_module()
{
	depth = 0;
	has_been_true[depth] = 1;
	is_true[depth] = 1;
	stack_size = STACK_SIZE;
	stack = perm_alloc(stack_size);
	available = stack + 1;
}

/*
 * This routine pushs a directive id into the stack. If it runs out of the
 * stack space then enlarge the stack by a double size and re-arrange the stack 
 * pointers.
 */
push(id)
char id;
{
	if ( available >= stack + stack_size ) 
	/* run out of stack space. Double the stack size. */
		{
		char *p;

		stack_size *= 2;
		p = perm_alloc(stack_size);
		memmove(p,stack,stack_size / 2);
		stack = p;
		available = stack + (stack_size / 2);
		}
	*available = id;
	available++;
}

/*
 * This routine pops out a directive id from the stack. If the stack is NOT 
 * empty return 1 otherwise 0.
 */
char pop()
{
	if ( available <= stack + 1 )
		return 0;
	available--;
	return 1;
}

/*
 * This routine returns the stuff on top of the stack. If the stack is empty 
 * return INVALID otherwise the directive id.
 */
char top()
{
	if ( available - 1 <= stack )
		return INVALID;
	else
		return ( *(available - 1) );
}

/*
 * This routine pops everything above #ifdef or #ifndef or #if ( including 
 * itself ) on the stack. If one of the above mentioned three directives is 
 * seen, return 1; otherwise 0.
 */
char pop_if()
{
char *save_available;
char rtn;

	save_available = available;
	do
	{
		rtn = top();
		if ( rtn == IFDEF || rtn == IFNDEF || rtn == IF ) 
		{
			pop();
			return 1;
		}
	} while ( pop() );

	/* couldn't find #ifdef or #ifndef or #if, reset the pointer */
	available = save_available;
	return 0;
}

/*
 * This routine handles an '#ifdef' directive.  
 */
handle_ifdef(line)
char *line;
{
	register char *id;
	register char *ch_ptr;
	register int length;
	extern char *mark_macros();

	push(IFDEF);
	/* Don't parse the directive if it is inside an inactive piece of code. */
	if(!is_true[depth])
	{
		start_if(FALSE);
		return;
	}
	skip_space(line);
	if(is_id_start(*line))
	{
		id = line;
		/* Create directive_macro_info in case any macros are in the ifdef directive. */
		if(StaticAnalysisOption)
		{
			ch_ptr = line;
			while(*ch_ptr != '\n')
				ch_ptr++;
			directive_macro_info = temp_alloc(8+2*(ch_ptr-line)+2);
			*directive_macro_info = DIRECTIVE_START;
			strcpy(directive_macro_info+1, "#ifdef ");
			mark_macros(directive_macro_info+8, line, ch_ptr-line);
		}
		while(is_id(*line))
			line++;
		length = line-id;
		if(*line == '\n')
		{
#if defined(CXREF)
			if(doCXREFdata)
				ref(id,current_lineno);
#endif			
#ifdef OPT_INCLUDE
			set_enclosing_if(ENCLOSING_IFDEF, id, length);
#endif
			start_if(get_define(id, length) != NULL);
			return;
		}
	}
	error("Bad syntax for #ifdef");
	start_if(FALSE);
}

/*
 * This routine handles an '#ifndef' directive.  
 */
handle_ifndef(line)
char *line;
{
	register char *id;
	register char *ch_ptr;
	register int length;
	extern char *mark_macros();

	push(IFNDEF);
	/* Don't parse the directive if it is inside an inactive piece of code. */
	if(!is_true[depth])
	{
		start_if(FALSE);
		return;
	}
	skip_space(line);
	if(is_id_start(*line))
	{
		id = line;
		/* Create directive_macro_info in case any macros are in the ifndef directive. */
		if(StaticAnalysisOption)
		{
			ch_ptr = line;
			while(*ch_ptr != '\n')
				ch_ptr++;
			directive_macro_info = temp_alloc(9+2*(ch_ptr-line)+2);
			*directive_macro_info = DIRECTIVE_START;
			strcpy(directive_macro_info+1, "#ifndef ");
			mark_macros(directive_macro_info+9, line, ch_ptr-line);
		}
		while(is_id(*line))
			line++;
		length = line-id;
		if(*line == '\n')
		{
#if defined(CXREF)
			if(doCXREFdata)
				ref(id,current_lineno);
#endif			
#ifdef OPT_INCLUDE
			set_enclosing_if(ENCLOSING_IFNDEF, id, length);
#endif
			start_if(get_define(id, length) == NULL);
			return;
		}
	}
	error("Bad syntax for #ifndef");
	start_if(FALSE);
}

#ifdef OPT_INCLUDE
boolean get_if_value(line)
char *line;
{
	char *get_sub_line();

	evaluate_defined_keyword(line);
	if_line = get_sub_line(line);
	zzparse();
	return (if_value != 0);
}
#endif

/* DTS # CLLbs00285: klee 09/25/92 */
boolean if_section = FALSE; 

handle_if(line)
char *line;
{
	register char *ch_ptr;
	char *get_sub_line();
	extern char *mark_macros();

	push(IF);
	/* Don't parse the directive if it is inside an inactive piece of code. */
	if(!is_true[depth])
	{
		start_if(FALSE);
		return;
	}
	/* Create directive_macro_info in case any macros are in the if directive. */
	if(StaticAnalysisOption)
	{
		ch_ptr = line;
		while(*ch_ptr != '\n')
			ch_ptr++;
		directive_macro_info = temp_alloc(5+2*(ch_ptr-line)+2);
		*directive_macro_info = DIRECTIVE_START;
		strcpy(directive_macro_info+1, "#if ");
		mark_macros(directive_macro_info+5, line, ch_ptr-line);
	}
	if_section = TRUE;
#ifdef OPT_INCLUDE
	set_enclosing_if(ENCLOSING_IF, line, 0);
	start_if(get_if_value(line));
#else
	evaluate_defined_keyword(line);
	if_line = get_sub_line(line);
	zzparse();
	start_if(if_value != 0);
#endif
	if_section = FALSE;
}

#ifdef OPT_INCLUDE
/*
 * See if this is the enclosing #if.  If it is, remember which kind
 * of #if it is, and what the parameters to it are.  If we have already
 * done something which makes this file unavoidable, don't bother.
 */
set_enclosing_if(type, str, len)
int type;
char *str;
int len;
{
	if (current_file->seen_if) {
		current_file->avoidable = FALSE;
		return;
	}
	if (!current_file->inside_if && current_file->avoidable) {
		current_file->inside_if = TRUE;
		current_file->outer_if_depth = depth;
		current_file->if_type = type;
		if (len == 0)
			len = strlen(str);
		current_file->condition = perm_alloc(len + 1);
		strncpy(current_file->condition, str, len);
		current_file->condition[len] = '\0';
	}
}


/*
 * See if this is the close to a enclosinging if.  If it is, set up to
 * check for further output which would make this file unavoidable.
 */
end_enclosing_if()
{
	if (current_file->inside_if && depth == current_file->outer_if_depth) {
		current_file->inside_if = FALSE;
		current_file->seen_if = TRUE;
	}
}
#endif

handle_elif(line)
char *line;
{
	register char *ch_ptr;
	char *get_sub_line();
	extern char *mark_macros();

	if ( top() == ELSE )
	{
		error("Illegal multiple #else statements");
		return;
	}
	else
		push(ELIF);
	if(depth <= 0)
	{
		error("#elif without matching #if");
		return;
	}
	/* Create directive_macro_info in case any macros are in the elif directive. */
	if(StaticAnalysisOption)
	{
		ch_ptr = line;
		while(*ch_ptr != '\n')
			ch_ptr++;
		directive_macro_info = temp_alloc(7+2*(ch_ptr-line)+2);
		*directive_macro_info = DIRECTIVE_START;
		strcpy(directive_macro_info+1, "#elif ");
		mark_macros(directive_macro_info+7, line, ch_ptr-line);
	}
	/* If a true if condition has already occurred then this else is false. */
	if(has_been_true[depth])
	{
		is_true[depth] = FALSE;
		return;
	}
	evaluate_defined_keyword(line);
	if_section = TRUE;
	if_line = get_sub_line(line);
	if_section = FALSE;
	zzparse();
	is_true[depth] = if_value != 0;
	has_been_true[depth] = if_value != 0;
}


/*
 * This routine handles an '#else' directive.  
 */
handle_else(line)
char *line;
{
	if ( top() == ELSE )
	{
		error("Illegal multiple #else statements");
		return;
	}
	else
		push(ELSE);
	if(*line != '\n')
		warning("extra characters on #else");
	if(depth <= 0)
	{
		error("#else without matching #if");
		return;
	}
	is_true[depth] = !has_been_true[depth];
	has_been_true[depth] = TRUE;
}


/*
 * This routine handles an '#endif' directive.  
 */
handle_endif(line)
char *line;
{
	if(*line != '\n')
		warning("extra characters on #endif");
	pop_if();
	if(depth <= 0)
	{
		error("#endif without matching #if");
		return;
	}
	depth--;
#ifdef OPT_INCLUDE
	end_enclosing_if();
#endif
}


start_if(condition_is_true)
boolean condition_is_true;
{
	depth++;
	if(depth > MAX_IF_DEPTH)
	{
		error("Conditional compilation nested too deeply");
		return;
	}
	if(!is_true[depth-1])
	{
		has_been_true[depth] = TRUE;
		is_true[depth] = FALSE;
	}
	else
	{
		has_been_true[depth] = condition_is_true;
		is_true[depth] = condition_is_true;
	}
}


/*
 * This routine searches for the keyword 'defined' followed by an
 * identifier.  If the following identifier is a defined macro name
 * then the identifiers are replaced by '1'.  Otherwise, the identifiers
 * are replaced by '0'.  The following identifier may optionally be
 * enclosed in parentheses.
 */
evaluate_defined_keyword(line)
register char *line;
{
	register char *target;
	register char *id;
	boolean is_paren;

	target = line;
	while(*line != '\0')
	{
		if(*line == 'd')
		{
			id = line;
			while(is_id(*line))
				line++;
			if(strncmp(id, "defined", line-id) == 0 && line-id == 7)
			{
				skip_space(line);
				if(*line == '(')
				{
					line++;
					is_paren = TRUE;
					skip_space(line);
				}
				else
					is_paren = FALSE;
				if(!is_id_start(*line))
				{
					error("bad macro name after 'defined'");
					return;
				}
				id = line;
				while(is_id(*line))
					line++;
#if defined(CXREF)
				if(doCXREFdata)
					ref(xcopy(id,line),current_lineno);
#endif				
				if(get_define(id, line-id) == NULL)
					*target++ = '0';
				else
					*target++ = '1';
				if(is_paren)
				{
					skip_space(line);
					if(*line != ')')
						error("missing right paren after 'defined(macro'");
					else
						line++;
				}
			}
			else
			{
				while(id < line)
					*target++ = *id++;
			}
		}
		else
			*target++ = *line++;
	}
	*target = '\0';
}


/*
 * This routine checks if all conditional compilation nestings
 * have been terminated.
 */
check_if_ifs_are_ended()
{
	if(depth != 0)
		error("Missing #endif at end of file");
}
