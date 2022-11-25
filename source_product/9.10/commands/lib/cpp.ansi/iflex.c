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
#include <ctype.h> 
#ifdef DSEE
/* If building under DSEE, find result of translating ifgram.y this way */
#include "$(ifgram.y)/y.tab.h"
#else
#include "y.tab.h"
#endif
#include "support.h"

extern char *if_line;

extern YYSTYPE zzlval;

zzerror()
{
	error("Bad syntax for #if condition");
}


#define C_DOUBLE_QUOTE		0042
#define C_SINGLE_QUOTE		0047
#define C_BACKSLASH			0134
#define C_BELL				0007
#define C_BACKSPACE			0010
#define C_FORM_FEED			0014
#define C_NEWLINE			0012
#define C_CARRIAGE_RETURN	0015
#define C_HORIZONTAL_TAB	0011
#define C_VERTICAL_TAB		0013


static int hex_value(ch)
char ch;
{
	if(isdigit(ch))
		return ch-'0';
	else if(ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	else
		return ch-'A'+10;
}

/*
 * This routine reads a character literal from if_line and
 * returns its value.
 */
static long readchar()
{
	register int val;

	if(*if_line == '\'')
	{
		error("Empty character literal in #if line");
		val = 0;
	}
	else if(*if_line != '\\')
		val = *if_line++;
	else
	{
		if_line++;
		switch(*if_line)
		{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				/* Octal character constant. */
				val = *if_line++-'0';
				if(*if_line >= '0' && *if_line <= '7')
				{
					val = val*8+*if_line++-'0';
					if(*if_line >= '0' && *if_line <= '7')
						val = val*8+*if_line++-'0';
				}
				if (val > 255)
				{
					error("Octal character value too large");
					val = 0;
				}
				/* Since character constants are signed ints we need to extend
				 * a negative character constant to full int size. */
				if (val > 127)
					val -= 256;
				break;
			case 'x':	/* Hexadecimal constant. */
			case 'X':
				if_line++;
				if(isxdigit(*if_line))
				{
					val = hex_value(*if_line++);
					if(isxdigit(*if_line))
					{
						val = val*16+hex_value(*if_line++);
						if(isxdigit(*if_line))
							val = val*16+hex_value(*if_line++);
					}
				}
				else
				{
					error("No digits in hex constant");
					val = 0;
				}
				if (val > 255)
				{
					error("Octal character value too large");
					val = 0;
				}
				/* Since character constants are signed ints we need to extend
				 * a negative character constant to full int size. */
				if (val > 127)
					val -= 256;
				break;
			case '\"':
				if_line++;
				val = C_DOUBLE_QUOTE;
				break;
			case '\'':
				if_line++;
				val = C_SINGLE_QUOTE;
				break;
			case '\\':
				if_line++;
				val = C_BACKSLASH;
				break;
			case 'a':
				if_line++;
				val = C_BELL;
				break;
			case 'b':
				if_line++;
				val = C_BACKSPACE;
				break;
			case 'f':
				if_line++;
				val = C_FORM_FEED;
				break;
			case 'n':
				if_line++;
				val = C_NEWLINE;
				break;
			case 'r':
				if_line++;
				val = C_CARRIAGE_RETURN;
				break;
			case 't':
				if_line++;
				val = C_HORIZONTAL_TAB;
				break;
			case 'v':
				if_line++;
				val = C_VERTICAL_TAB;
				break;
			default:
				val = *if_line++;
				break;
		}
	}
	/* Scan the rest of the character literal. */
	while(*if_line != '\'' && *if_line != '\n')
	{
		if(*if_line == '\\')
			if_line++;
		if_line++;
	}
	if(*if_line == '\n')
		error("Missing right quote on character literal");
	else
		if_line++;
	return val;
}


/*
 * This routine skips the optional 'l' or 'L' and  'u' or 'U' which may
 * be appended to constants.  If there is a 'u' or 'U' it returns TRUE,
 * otherwise FALSE.
 */
static boolean skip_suffix()
{
	if(*if_line == 'l' || *if_line == 'L')
	{
		if_line++;
		if(*if_line == 'u' || *if_line == 'U')
		{
			if_line++;
			return TRUE;
		}
	}
	else if(*if_line == 'u' || *if_line == 'U')
	{
		if_line++;
		if(*if_line == 'l' || *if_line == 'L')
			if_line++;
		return TRUE;
	}
	return FALSE;
}


zzlex()
{
	unsigned long val;

	for(;;)
	{
		switch(*if_line++)
		{
			case '\n':
			case '\0':
				return -1;
				break;
			case '(':
				return(LPAREN);
			case ')':
				return(RPAREN);
			case '<':
				if(*if_line == '<')
				{
					if_line++;
					return LSHIFT;
				}
				else if(*if_line == '=')
				{
					if_line++;
					return LESS_THAN_OR_EQUAL;
				}
				else
					return LESS_THAN;
			case '>':
				if(*if_line == '>')
				{
					if_line++;
					return RSHIFT;
				}
				else if(*if_line == '=')
				{
					if_line++;
					return(GREATER_THAN_OR_EQUAL);
				}
				else
					return(GREATER_THAN);
			case '=':
				if(*if_line == '=')
				{
					if_line++;
					return DOUBLE_EQUALS;
				}
				else
				{
					error("Illegal character '=' in #if directive");
					break;
				}
			case '!':
				if(*if_line == '=')
				{
					if_line++;
					return NOT_EQUALS;
				}
				else
					return BANG;
			case '+':
				return(PLUS);
			case '-':
				return(MINUS);
			case '*':
				return(STAR);
			case '&':
				if(*if_line == '&')
				{
					if_line++;
					return AND;
				}
				else
					return(AMPERSAND);
			case '~':
				return(TILDE);
			case '/':
				return(SLASH);
			case '%':
				return PERCENT;
			case '^':
				return CARAT;
			case '|':
				if(*if_line == '|')
				{
					if_line++;
					return(OR);
				}
				else
					return(BAR);
			case '?':
				return(QUESTION);
			case ':':
				return(COLON);
			case ' ':
			case '\t':
				break;
			case '\'':
				zzlval.value = readchar();
				zzlval.is_unsigned = FALSE;
				return(CONSTANT);
			case '0':
				val = 0;
				if(*if_line == 'x' || *if_line == 'X')
				{
					if_line++;
					if(!isxdigit(*if_line))
						error("No digits in hex constant");
					while(isxdigit(*if_line))
					{
						if(val >= 1<<28)
						{
							error("Constant literal too large");
							val = 0;
						}
						val = val*16+hex_value(*if_line++);
					}
				}
				else
				{
					while(*if_line >= '0' && *if_line <= '7')
					{
						if(val >= 1<<29)
						{
							error("Constant literal too large");
							val = 0;
						}
						val = val*8+*if_line++-'0';
					}
				}
				zzlval.value = val;
				zzlval.is_unsigned = skip_suffix() || (zzlval.value < 0);
				/* Start looking for pp-number at if_line-1 in case the last char
				 * was an 'e' or 'E' and the next char is '+' or '-'. */
				if(if_line != skip_number(if_line-1))
					warning("unrepresentable preprocessor number");
				return CONSTANT;
			default:
				if(*(if_line-1) == 'L' && *if_line == '\'')
				{ /* wide character constant */
					if_line++;
					zzlval.value = readchar();
					zzlval.is_unsigned = FALSE;
					return(CONSTANT);
				}
				if(is_id_start(*(if_line-1)))
				{
					while(is_id(*if_line))
						if_line++;
					zzlval.value = 0;
					zzlval.is_unsigned = FALSE;
					return CONSTANT;
				}
				if(isdigit(*(if_line-1)))
				{
					if_line--;
					val = 0;
					while(isdigit(*if_line))
					{
						if(val >= 0x1999999A || (val == 0x19999999 && *if_line > '5'))
						{
							error("Constant literal too large");
							val = 0;
						}
						val = val*10+*if_line++-'0';
					}
					zzlval.value = val;
					zzlval.is_unsigned = skip_suffix() || (zzlval.value < 0);
					if(if_line != skip_number(if_line))
						warning("unrepresentable preprocessor number");
					return CONSTANT;
				}
				error("Illegal character on '#if' line");
		}
	}
}
