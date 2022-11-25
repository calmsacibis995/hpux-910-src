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

#include <stdio.h>
#include <setjmp.h>
#include "support.h"
#include "file.h"

boolean is_error;
FILE *cpp_error_output;
jmp_buf cpp_error_jmp_buf;

extern boolean NoExitOnError;
extern boolean NoWarningsOption;

/*
 * This routine does any initialization required before processing
 * a new file.
 */
void init_error_module()
{
	/* if not set by caller, use stderr for error output */
	if (cpp_error_output == NULL)
		cpp_error_output = stderr;

	is_error = FALSE;
}

/*
 * This routine returns the basename of the given file.
 */
char *basename(filename)
char *filename;
{
	register char *p;

	p = filename;
	/* Go the the end of the filename. */
	while(*p != '\0')
		p++;
	/* Back up to the first character after a '/'. */
	while(p > filename && *(p-1) != '/')
		p--;
	return p;
}

/*
 * This routine is used by other routines in this file to output a warning or
 * error message.
 */

static void print_message(message, type, parm1, parm2, parm3)
char *message, *type;
int parm1, parm2, parm3;
{
	if(current_file != NULL)
		fprintf(cpp_error_output, "%s: %d: %s", basename(current_file->filename), current_file->lineno, type);
	else
#ifdef CPP_CSCAN
		fprintf(cpp_error_output, "cpp.cscan: %s", type);
#else
		fprintf(cpp_error_output, "cpp: %s", type);
#endif
	fprintf(cpp_error_output, message, parm1, parm2, parm3);
	fprintf(cpp_error_output, ".\n");
}


/*
 * This routine prints out the given error message using the given parameters
 * like printf would.  It sets the 'is_error' flag.
 */
error(message, parm1, parm2, parm3)
char *message;
int parm1, parm2, parm3;
{
	is_error = TRUE;
	print_message (message, "", parm1, parm2, parm3);
}


/*
 * This routine prints out the given warning message using the given parameters
 * like printf would.  It does not set the 'is_error' flag.
 */
warning(message, parm1, parm2, parm3)
char *message;
int parm1, parm2, parm3;
{
	if(NoWarningsOption)
		return;
	print_message (message, "warning- ", parm1, parm2, parm3);
}


/*
 * This routine prints an error message and either exits the program
 * (if NoExitOnError is FALSE, the default), or longjmps back to the
 * location stored in cpp_error_jmp_buf, which will take care of returning
 * to the caller of cpp.
 */

fatal_cpp_error(message, parm1, parm2, parm3)
char *message;
int parm1, parm2, parm3;
{
	error (message, parm1, parm2, parm3);

	if (NoExitOnError)
		longjmp (cpp_error_jmp_buf, 1);
	else
		exit (1);
}


/*
 * This routine handles the '#error' directive.  The parameter points to the
 * first character following '#error'.  It generates an error message
 * using the given line as the message.
 */
handle_error(line)
register char *line;
{
	register char *line_ptr;
	register char *new_line;

	skip_space(line);
        /* 901130 choang CLLca01703 - We need to diagnose bad syntax for
         * the #error directive; for example : #error<CR>
         */
        if (*line != '\n') {
	   line_ptr = new_line = perm_alloc(strlen(line));
	   /* Copy 'line' into 'new_line' up to the '\n'. */
	   while(*line != '\n')
	      *line_ptr++ = *line++;
	   *line_ptr = '\0';
	   /* Generate an error message using the directive line as the
              message. */
	   error(new_line);
        }
        else
           error("Bad syntax for #error directive");
}
