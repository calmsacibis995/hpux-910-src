/* $Revision: 70.15 $ */

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
#ifdef APOLLO_EXT 
/* includes for systype checking */ 
#include <apollo/sys/ubase.h>
#include <apollo/sys/pm.h>
#include <apollo/sys/c.h>
#endif
#include "support.h"
#include "file.h"
#include "if.h"

extern boolean PassComments;
extern boolean NLSOption;
extern boolean StaticAnalysisOption;

#ifdef CPP_CSCAN /* jww */
extern boolean K_N_R_Mode;		/* Do K&R comment processing if set */
#endif

# ifdef CXREF
extern int doCXREFdata;
# endif

#ifdef APOLLO_EXT
extern char * UserSystype; 
#endif

/* This variable is used by code which wants to know the current line number.
 * it's value is the first line of a potentially multi-line input line as
 * opposed to current_file->lineno whose value is the last line. */
int current_lineno;


/*
 * This routine reads a newline terminated line of up to buffersize-2 characters
 * from the current input file into 'line'.  It returns a boolean indicating
 * if the read succeded.
 */
boolean input_line(line)
char *line;
{
	int ch;

	/* Set the last read character to a newline for later checking. */
	line[buffersize-2] = '\n';
	if(fgets(line, buffersize, current_file->fp) == NULL)
	{
		return FALSE;
	}
	/* If last read character is not a newline then it was overwritten by an input
	 * line that was too long. */
	if(line[buffersize-2] != '\n' && line[buffersize-2] != '\0')
	{
		error("Input line too long - use -H option");
		/* Eat up the rest of the line. */
		while((ch = getc(current_file->fp)) != '\n' && ch != EOF)
			;
		line[buffersize-2] = '\n';
	}
	current_file->lineno++;
    return TRUE;
}


/* This array is used in 'get_line' to quickly determine if a character is 
 * potentially one requiring special processing.  The characters marked with
 * a '1' in the array are those checked for in the if-else construct of 'get_line'. */
static char is_special_to_get_line[256] = {
0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,1,1,0,0,0,1,0,0,0,0,0,0,0,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};


/*
 * This routine reads lines from the input.  The input line length is
 * limited to buffersize.  It translates trigraphs and connects lines
 * with continuation characters.  Comments are turned into single spaces.
 * It limits the length of the string it returns to buffersize.
 * The 'directive_pos' pointer is used to point to the start of a directive
 * if the line is a directive.
 */
char *return_line;
char *in_line;
char *get_line()
{
	static boolean unterminatedComment = FALSE;
	register char *ret_ptr, *in_ptr;
	register char ch;
	register boolean inLiteral = FALSE;
	register char delimiter;
	register char *directive_pos;
	register boolean passComments;
	boolean canRecognizeDirective = TRUE;
	boolean inDefine = FALSE;
	int cont_count = 0;
	char *handle_directive();

#ifdef CPP_CSCAN  /* jww */
	boolean line_is_define = 0;
#endif

	/* clear out first few bytes of return_line to not pick up */
	/* leftover directive from previous line */
	memset(return_line,'\0',10);
	ret_ptr = return_line;
	in_ptr = in_line;
	/* There may be an non-terminating comment at this point with input
	 * residing in 'line'. */
	if(unterminatedComment)
		unterminatedComment = FALSE;
	else if(!input_line(in_line))
		return NULL;
	directive_pos = NULL;
	passComments = PassComments;
	/* Loop through all characters read until end of line is hit. */
	while((ch = *in_ptr++) != '\0')
	{
#ifdef CPP_CSCAN /* jww */
	    boolean  ch_is_comment = 0;  
	    boolean mashed_comment = 0;
#endif

		if(is_special_to_get_line[(unsigned char)ch])
		{
		boolean TrigraphSequenceSlash; /* DTS # CLLca01895 */

			TrigraphSequenceSlash = FALSE;
			/* Look for a trigraph if the next two characters are '??'. */
			if(ch == '?' && *in_ptr == '?')
			{
				/* Increment 'in_ptr' on the assumption that a trigraph will match.
				 * If no match is made then 'in_ptr' will be unincremented. */
				in_ptr += 2;
				switch(*(in_ptr-1))
				{
					case '=':
						ch = '#';
						break;
					case '(':
						ch = '[';
						break;
					case '/':
						ch = '\\';
						TrigraphSequenceSlash = TRUE;
						break;
					case ')':
						ch = ']';
						break;
					case '\'':
						ch = '^';
						break;
					case '<':
						ch = '{';
						break;
					case '!':
						ch = '|';
						break;
					case '>':
						ch = '}';
						break;
					case '-':
						ch = '~';
						break;
					default:
						in_ptr -= 2;
				}
			}
			/* Check that none of the characters used to represent column position
			 * information in substitute.c are in the input file. */
			/* DTS # CLLbs00004. klee 920303
			 * don't change control characters in literal because 
			 * it will impact NLS users.
			 */
			if( (ch > 0 && ch < 5) && !inLiteral )
			{
				warning("Illegal ^A, ^B, ^C, or ^D in source.  Changed to space");
				ch = ' ';
				canRecognizeDirective = FALSE;
			}
			else if(C_Plus_Plus && !inLiteral &&
				ch == '/' && *in_ptr == '/') {
				/* "//" comment */
				if (passComments)
				{
					*ret_ptr++ = '/';
					*ret_ptr++ = '/';
				}
				in_ptr++;
				while (*in_ptr != '\n')
				{
					/* Check that none of the characters used to represent column position
					 * information in substitute.c are in the input file. */
					/* DTS # CLLbs00004. klee 920303
					 * don't change control characters in
					 * comments because it will impact NLS
					 * users.
					 */
#if 0
					if( (*in_ptr > 0 && *in_ptr < 5) && !inLiteral )
					{
						warning("Illegal ^A, ^B, ^C, or ^D in source.  Changed to space");
						*in_ptr = ' ';
					}
#endif
					/* end. DTS # CLLbs00004. klee 920303 */
					/* No need to check for NLSOption - 
					 * '\n' cannot be the first or second
					 * byte of a 2-byte char          */
					if(passComments)
						*ret_ptr++ = *in_ptr;

/* CLLbs00131: C++ comment is to be continuated onto next line.  pkwan 920318 */
					if ((*in_ptr == '\\') &&
                                            (*(in_ptr+1) == '\n'))
					{
						if(!input_line(in_line))
						{
							error("Non-terminating comment at end of file");
							return NULL;
						}
						in_ptr = in_line;
					}
					else
/* CLLbs00131: C++ comment is to be continuated onto next line.  pkwan 920318 */
						in_ptr++;
				}
#ifdef CPP_CSCAN /* jww */
				ch_is_comment = 1;
#endif
				ch = ' ';
			}
			else if(ch == '/' && *in_ptr == '*' && !inLiteral)
			{
comments_case:
				if(passComments)
				{
					*ret_ptr++ = '/';
					*ret_ptr++ = '*';
				}
				in_ptr++;

#ifdef CPP_CSCAN  /* jww */
				/* Check for mashed comment (used for #define substitution magic) */
				/* -------------------------------------------------------------- */
				if (*in_ptr == '*' && *(in_ptr+1) == '/')
				    mashed_comment = 1;
#endif				

				while(!(*in_ptr == '*' && *(in_ptr+1) == '/'))
				{
					if(*in_ptr == '\0')
					{
						/* See if the output is more than half way throung the max buffer
						 * size.  If so, break up the comment to avoid problems with
						 * a comment causing the buffer size to be exceeded. */
						if(passComments && ret_ptr > return_line+buffersize/2)
						{
							/* Terminate this portion of the comment and copy the remainder
							 * of the comment back to 'line' preceded by new comment start. */
							*ret_ptr++ = '\0';
							ret_ptr = return_line+buffersize/2;
							*in_line = '/';
							*(in_line+1) = '*';
							strcpy(in_line+2, ret_ptr);
							*ret_ptr++ = '*';
							*ret_ptr++ = '/';
							*ret_ptr++ = '\0';
							unterminatedComment = TRUE;
							return return_line;
						}
						if(!input_line(in_line))
						{
							error("Non-terminating comment at end of file");
							return NULL;
						}
						in_ptr = in_line;
					}
					else
					{
						/* Check that none of the characters used to represent column position
						 * information in substitute.c are in the input file. */
						/* DTS # CLLbs00004. klee 920303
					 	* don't change control character
					 	* in comments because it will 
					 	* impact NLS users.
					 	*/
#if 0
						if( (*in_ptr > 0 && *in_ptr < 5) && !inLiteral )
						{
							warning("Illegal ^A, ^B, ^C, or ^D in source.  Changed to space");
							*in_ptr = ' ';
						}
#endif
						/* end. DTS # CLLbs00004. */
#ifdef NLS
						/* If the character is an NLS character pair then skip over the pair to
						 * avoid the second character being taken as part of a comment termination. */
						if(NLSOption && FIRSTof2((unsigned char)*in_ptr) && SECof2((unsigned char)*(in_ptr+1)))
						{
							if(passComments)
								*ret_ptr++ = *in_ptr;
							in_ptr++;
						}
#endif
						/* CLLbs00160 klee 920408
						 * allow breaking up the comment
						 * delimiters - a perrenial test
						 */
						if( strncmp(in_ptr,"*\\\n",3) == 0 )
						{
							if(!input_line(in_line))
							{
								error("Non-terminating comment at end of file");
								return NULL;
							}
							if(passComments)
							{
								strcpy(ret_ptr,"*\\\n");
								ret_ptr += 3;
							}
							in_ptr = in_line;
							if ( *in_ptr == '/' )
							{
								in_ptr--;
								break;
							}
						}
						/* CLLbs00160 klee 920408 */
						if(passComments)
							*ret_ptr++ = *in_ptr;
						in_ptr++;
					}
				}
				in_ptr += 2;
				if(passComments)
				{
					*ret_ptr++ = '*';
					ch = '/';
				}
				else
				{
#ifdef CPP_CSCAN /* jww */
				        ch_is_comment = 1; 
#endif
					ch = ' ';
				}
			}
			else if(ch == '/' && *in_ptr == '\\' && *(in_ptr+1) == '\n' && !inLiteral)
			{
				/* bug fix ( DTS # CLLca01887 ) */
				if(!input_line(in_line))
				{
					error("Non-terminating source line at end of file");
					return NULL;
					}
				in_ptr = in_line;
                                /* CLLbs00100  should not skip over '/' by  */
                                /* (ch = *in_ptr)   pkwan 920221             */
                                /* if ((ch = *in_ptr) == '*' )              */
				if (*in_ptr == '*' )
					goto comments_case;
				else
					goto back_slash_case;
			}
			else if((ch == '"' || ch == '\'') && !inLiteral)
			{
back_slash_case:
				inLiteral = TRUE;
				delimiter = ch;
				canRecognizeDirective = FALSE;
			}
			else if(ch == delimiter && inLiteral)
			{
				inLiteral = FALSE;
				canRecognizeDirective = FALSE;
			}
			else if(ch == '\\')
			{
				if(*in_ptr == '\n')
				{
read_a_line:
					if(!input_line(in_line))
					{
						error("Continuation character on last line of file");
						return NULL;
					}
					if ( (StaticAnalysisOption
# ifdef CXREF
					|| doCXREFdata
# endif
					     )
					&& (directive_pos != NULL) )
						{
						if (inDefine)
							*ret_ptr++ = NL_MARK;
					  	else if (id_cmp(directive_pos,"define"))
							{
							inDefine = TRUE;
							*ret_ptr++ = NL_MARK;
							while(cont_count--)
								*ret_ptr++ = NL_MARK;
							}
					  	else
							cont_count++;
						}
					if(ret_ptr >= return_line+buffersize-1)
						break;
					in_ptr = in_line;
					continue;
				}
				if(inLiteral)
				{
					*ret_ptr++ = '\\';
					/* DTS # CLLca01756 klee 920206 */
					if ( *in_ptr == '\\' && *(in_ptr+1) == '\n' )
						goto read_a_line;
					/* DTS # CLLca01756 */
					/* Check that none of the characters used to represent column position
					 * information in substitute.c are in the input file. */
					/* DTS # CLLbs00004. klee 920303
					 * don't change control characters in
					 * literal because it will impact NLS
					 * users.
					 */
					if( (*in_ptr > 0 && *in_ptr < 5) && !inLiteral )
					{
						warning("Illegal ^A, ^B, ^C, or ^D in source.  Changed to space");
						*in_ptr = ' ';
					}
					if ( TrigraphSequenceSlash )
						continue;
					ch = *in_ptr++;
				}
				canRecognizeDirective = FALSE;
			}
			else if(ch == '#' && canRecognizeDirective)
			{
				directive_pos = ret_ptr+1;
				/* Comments are always stripped in # directives. */
				passComments = FALSE;
				canRecognizeDirective = FALSE;
#ifdef CPP_CSCAN /* jww */
				if (K_N_R_Mode)		/* Do K&R comment processing if set */
			        {
				    /* We must convert "/ * * /" to "##" inside of #define lines for K&R. */
                                    /* So, see if we have a #define line ... 				  */
				    /* ------------------------------------------------------------------ */
				    char *p2 = in_ptr;
				    while (*p2 == ' ' || *p2 == '\t')
					p2++;
				    
				    if (strncmp(p2, "define", 6) == 0)
					line_is_define = 1;
				}
#endif				
			}
			else
				canRecognizeDirective = FALSE;
#ifdef NLS
			/* Skip over NLS characters in inside of string literals. */
			if(NLSOption && FIRSTof2((unsigned char)ch) && SECof2((unsigned char)*in_ptr) && inLiteral)
			{
				*ret_ptr++ = ch;
				ch = *in_ptr++;
			}
#endif
		}
		else
			if(ch != ' ' && ch != '\t')
				canRecognizeDirective = FALSE;

#ifdef CPP_CSCAN /* jww */
	        /* We don't convert comments to blanks in K&R comment mode.          */
	        /* A special case is open/close comment in a #define.  We need to    */
	        /* stuff an "##" in the place of the comment so the define processor */
	        /* does the right thing (jams them together).                        */
                if (K_N_R_Mode && ch_is_comment)
		{
		    if (line_is_define && mashed_comment)
		    {
			char *p2 = in_ptr;
			while (*p2 == ' ' || *p2 == '\t' || *p2 == '\n')
			    p2++;
				    
			/* If not last thing on the line */
			if (*p2)
			{
			    *ret_ptr++ = '#';
			    *ret_ptr++ = '#';
			}
		    }
		}
		else
#endif
		*ret_ptr++ = ch;
	}
	if(ret_ptr >= return_line+buffersize-1)
	{
		error("Catenated input line too long - use -H option");
		*(return_line+buffersize-2) = '\n';
		*(return_line+buffersize-1) = '\0';
	}
	else
	{
		/* Back over the newline, if there was one.  Last line of  */
		/* file doesn't have the \n.                               */
		ret_ptr--;
		if (*ret_ptr != '\n') ret_ptr++;
/* Remove space from the end of the line.  This is important so that comments
		 * on their own line don't generate whitespace, and so macro parameters preceding
		 * the end of line have trailing space removed.  Also, the #undef, #ifdef, #ifndef,
		 * and #else count on it when checking for extra characters at end of line. */
		back_over_space(ret_ptr, return_line);
		*ret_ptr++ = '\n';
		*ret_ptr++ = '\0';
	}
	if(directive_pos != NULL)
		return handle_directive(directive_pos);
	else
		return return_line;
}
	

char *read_line(target)
char *target;
{
	register char *line;
	register char *target_ptr;

	do
	{
		if((line = get_line()) == NULL)
		{
			*target = '\0';
			return NULL;
		}
#ifdef OPT_INCLUDE
		if (*line != '\n' && *line != '\0')
			SET_UNAVOIDABLE();
#endif
	} while(!produce_output);
	target_ptr = target;
	while(*line != '\0')
		*target_ptr++ = *line++;
	*target_ptr = '\0';
	return target;
}


/*
 * This routine takes a pointer to the character following the '#' in a '#' directive.
 * It returns the line which should result from the directive.  This is a simple
 * blank line for all but the pragma.  It assumes that for the pragma (and for
 * Apollo extensions) it is OK to write a character at *(line-1).
 */
char *handle_directive(line)
register char *line;
{
	static char blank_line[] = "\n";
	static char empty_line[] = "";
#ifdef APOLLO_EXT
        static char *temp_line;   /* For saving the line pointer */
#endif

	skip_space(line);
#ifdef APOLLO_EXT
        if (*line == '\n') 
           return blank_line;
        else
           if (FirstToken != not_first && *line != '\p' && !id_cmp (line, "systype") )
              FirstToken = not_first; 
#endif
	switch(*line)
	{
		case '\n':
			return blank_line;
#ifdef APOLLO_EXT
		case 'a':
			if (id_cmp (line, "attribute"))
			{
				*(line-1) = '#';
				return line-1;
			}
			if (id_cmp (line, "apollo"))
			{
				*(line-1) = '#';
				return line-1;
			}
			break;
#endif
		case 'd':
			if(id_cmp(line, "define"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				if(produce_output)
					handle_define(line+6);
				return blank_line;
			}
#ifdef APOLLO_EXT
			if (id_cmp (line, "debug")) 
			{
				*(line-1) = '#';
				return line-1;  
			}
#endif
			break;
		case 'e':
			if(id_cmp(line, "error"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				if(produce_output)
					handle_error(line+5);
				return blank_line;
			}
			if(id_cmp(line, "else"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				handle_else(line+4);
				return blank_line;
			}
			if(id_cmp(line, "elif"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				handle_elif(line+4);
				return blank_line;
			}
			if(id_cmp(line, "endif"))
			{
				handle_endif(line+5);
				return blank_line;
			}
#ifdef APOLLO_EXT
			if (id_cmp (line, "eject"))
			{
				*(line-1) = '#';
				return line-1;
			} 
#endif
			break;
		case 'i':
			if(id_cmp(line, "include"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				if(produce_output)
					handle_include(line+7);
				return empty_line;
			}
			if(id_cmp(line, "if"))
			{
				handle_if(line+2);
				return blank_line;
			}
			if(id_cmp(line, "ifdef"))
			{
				handle_ifdef(line+5);
				return blank_line;
			}
			if(id_cmp(line, "ifndef"))
			{
				handle_ifndef(line+6);
				return blank_line;
			}
#if defined(APOLLO_EXT) || defined(HP_OSF)
			if (id_cmp (line, "ident"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
#ifdef HP_OSF
				return blank_line;
#else
				*(line-1) = '#';
				return line-1;
#endif /* HP_OSF */
			}
#endif
			break;
		case 'l':
			if(id_cmp(line, "line"))
			{
				if(produce_output)
					handle_line(line+4);
				return empty_line;
			}
#ifdef APOLLO_EXT
			if (id_cmp (line, "list"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				*(line-1) = '#';
				return line-1;
			}
#endif
			break;
#ifdef APOLLO_EXT
		case 'm':
			if (id_cmp (line, "module"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				*(line-1) = '#';
				return line-1;
			}
			break;

		case 'n':
			if (id_cmp (line, "nolist"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				*(line-1) = '#';
				return line-1;
			}
			break;
#endif /* APOLLO_EXT */
		case 'p':
			if(id_cmp(line, "pragma"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
#ifdef APOLLO_EXT 
				temp_line = line + 6;
				skip_space(temp_line);
				if(id_cmp(temp_line, "HP_SYSTYPE"))
				{
					do_systype (temp_line+10);  
		        		if (FirstToken != not_first) 
			        		FirstToken = not_first; 
				}
#endif
				*(line-1) = '#';
				return line-1;
			}
			break;
#ifdef APOLLO_EXT
		case 's':
			if (id_cmp (line, "section"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				*(line-1) = '#';
				return line-1;
			} 
			if (id_cmp (line, "systype"))
			{ 
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
                                temp_line = line + 7;
                                skip_space(temp_line);
				do_systype (temp_line);
				if (FirstToken != not_first)
				       FirstToken = not_first;
				*(line-1) = '#';
				return line-1;
			}
			break; 
#endif /* APOLLO_EXT */
		case 'u':
			if(id_cmp(line, "undef"))
			{
#ifdef OPT_INCLUDE
				SET_UNAVOIDABLE();
#endif
				if(produce_output)
					handle_undef(line+5);
				return blank_line;
			}
			break;
	}
/* CLLca01923.  Generarte an error only if unknown directive is inside an
   active piece of code.  pkwan 920115 */
        if (is_true[depth])
	  error("Unknown preprocessing directive");
        else
	  warning("Unknown preprocessing directive");
/* CLLca01923.  pkwan 920115 */
	return blank_line;
}

#ifdef APOLLO_EXT
/*
 *  do_systype is called when a #pragma is seen
 *  It looks at the rest of the line and determines if the user is trying
 *  to set the systype.  The syntax for setting the systype is
 *  #pragma HP_SYSTYPE ident.  Ident may be in quotes or it may not be.
 *  The user can only set the systype once, and it has to be done as the
 *  first non-comment token in the file.  If the user used the -t option
 *  to set the systype from the command line and she also has a #pragma
 *  HP_SYSTYPE, then they have to be the same.
 */
int do_systype (prag_line)
register char* prag_line;
{ 
#define SYSTYPEMAXLEN 10
   char sbuf [40];
   char id[SYSTYPEMAXLEN];
   short id_len = 0;
   pm_$unix_selector_t unix_type;
   pm_$unix_release_t  ytmp;  
   boolean is_quoted = FALSE; 
   char delim;
   char * id_start;
   int i;
   
   if (! (FirstToken == is_first)) {
      error ("pragma systype must be first token in file");
      return ;
      }
   skip_space (prag_line); 
   if (*prag_line == '\n') {
      error ("missing argument for systype");
      return ;
      }
   if (*prag_line == '\"' || *prag_line == '\'')  {
      is_quoted = TRUE;
      delim = *prag_line;
      prag_line++;
      id_start = prag_line;
      } 
   else
      id_start = prag_line;
   while (! (*prag_line == ' ' || *prag_line == '\n')) { 
      id_len++;
      prag_line++;
      }
   if (is_quoted) 
      if (*(prag_line-1) == delim) 
         id_len--; 
      else { 
         error ("missing end of quoted string or mismatched quotes"); 
         return ; 
         }
   if (id_len > SYSTYPEMAXLEN-1)
       error ("invalid systype");
         
   for (i=0; i<id_len; i++)
       id[i] = *id_start++;
   id[id_len] = '\0';
        
   if (c_$decode_version (id, id_len,
         (short *) &unix_type, (short *) &ytmp) == 0) 
     error ("invalid systype");
   else {
      if (UserSystype == NULL) {
         if (unix_type != pm_$any_unix) {
            sprintf (sbuf, "SYSTYPE=%s", id);
            putenv (sbuf);
            }
         }
      else 
         if ((UserSystype) && strcmp (UserSystype, id) == 0) {
            if (unix_type != pm_$any_unix) {
               sprintf (sbuf, "SYSTYPE=%s", UserSystype);
               putenv(sbuf);
               }
            }
         else
            error ("cannot have more than one systype");
      }
   return ;
}
            
/*
 *  handle_systype handles the case where systype is set on the command
 *  line.  The value for the systype is put into UserSystype.  It is
 *  checked with a call to c_$decode_version.
 */
void handle_systype (argument)
char *argument;

{

   char sbuf[40];
   char *ch_ptr;
   pm_$unix_selector_t unix_type;
   pm_$unix_release_t  ytmp;


   if (is_id_start (*argument)) {
      UserSystype = ch_ptr = perm_alloc (strlen (argument) + 1);
      while (is_id (*argument) || (*argument == '.'))
         *ch_ptr++ = *argument++;
      *ch_ptr = '\0';
      if (c_$decode_version (UserSystype, strlen (UserSystype),
            (short *) &unix_type, (short *) &ytmp) == 0)
         error ("invalid systype");
      else {
         if (unix_type != pm_$any_unix) {
            sprintf (sbuf, "SYSTYPE=%s", UserSystype);
            putenv(sbuf);
            }
         }
      }
   else
      error ("bad syntax for systype option");
}
#endif  /* APOLLO_EXT */
