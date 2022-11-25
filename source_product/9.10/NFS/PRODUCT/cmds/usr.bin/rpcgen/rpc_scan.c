/*
 @(#)rpc_scan:	$Revision: 1.12.109.1 $	$Date: 91/11/19 14:09:47 $
*/

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 * 
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

/*
 * rpc_scan.c, Scanner for the RPC protocol compiler 
 * Copyright (C) 1987, Sun Microsystems, Inc. 
 */
#include <stdio.h>
#include <ctype.h>
#include "rpc_scan.h"
#include "rpc_util.h"

/* NOTE: rpc_main.c, rpc_parse.c, rpc_scan.c and rpc_util.c share a 	*/
/* single message catalog (rpgen.cat).  For that reason we have 	*/
/* allocated messages 1 through 20 for rpc_main.c, 21 through 40 for  	*/
/* rpc_parse.c, 41 through 60 to rpc_scan.c and from 61 on for		*/
/* rpc_util.c.  If we need more than 20 messages in this file we will 	*/
/* need to take into account the message numbers that are already 	*/
/* used by the other files.						*/


#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_ctype.h>
#include <nl_types.h>
extern nl_catd nlmsg_fd;
#endif NLS

#define startcomment(where) (where[0] == '/' && where[1] == '*')
#define endcomment(where) (where[-1] == '*' && where[0] == '/')

static int pushed = 0;	/* is a token pushed */
static token lasttok;	/* last token, if pushed */

extern char tempbuf[];

/*
 * scan expecting 1 given token 
 */
void
scan(expect, tokp)
	tok_kind expect;
	token *tokp;
{
	get_token(tokp);
	if (tokp->kind != expect) {
		expected1(expect);
	}
}

/*
 * scan expecting 2 given tokens 
 */
void
scan2(expect1, expect2, tokp)
	tok_kind expect1;
	tok_kind expect2;
	token *tokp;
{
	get_token(tokp);
	if (tokp->kind != expect1 && tokp->kind != expect2) {
		expected2(expect1, expect2);
	}
}

/*
 * scan expecting 3 given token 
 */
void
scan3(expect1, expect2, expect3, tokp)
	tok_kind expect1;
	tok_kind expect2;
	tok_kind expect3;
	token *tokp;
{
	get_token(tokp);
	if (tokp->kind != expect1 && tokp->kind != expect2
	    && tokp->kind != expect3) {
		expected3(expect1, expect2, expect3);
	}
}


/*
 * scan expecting a constant, possibly symbolic 
 */
void
scan_num(tokp)
	token *tokp;
{
	get_token(tokp);
	switch (tokp->kind) {
	case TOK_IDENT:
		break;
	default:
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,41, "constant or identifier expected")));
		error(tempbuf);
	}
}


/*
 * Peek at the next token 
 */
void
peek(tokp)
	token *tokp;
{
	get_token(tokp);
	unget_token(tokp);
}


/*
 * Peek at the next token and scan it if it matches what you expect 
 */
int
peekscan(expect, tokp)
	tok_kind expect;
	token *tokp;
{
	peek(tokp);
	if (tokp->kind == expect) {
		get_token(tokp);
		return (1);
	}
	return (0);
}



/*
 * HPNFS
 * Get the next token, printing out any directive that are encountered. 
 * This has been NLSized by Cristina Mahon and Mike Shipley at CND
 * We are concerned with handling 16-bit characters inside comments
 * and inside quoted literals.
 */
void
get_token(tokp)
	token *tokp;
{
	int commenting;

	if (pushed) {
		pushed = 0;
		*tokp = lasttok;
		return;
	}
	commenting = 0;
	for (;;) {
		if (*where == 0) {
			for (;;) {
				if (!fgets(curline, MAXLINESIZE, fin)) {
					tokp->kind = TOK_EOF;
					*where = 0;
					return;
				}
				linenum++;
				if (commenting) {
					break;
				} else if (cppline(curline)) {
					docppline(curline, &linenum, 
						  &infilename);
				} else if (directive(curline)) {
					printdirective(curline);
				} else {
					break;
				}
			}
			where = curline;
		} else if (isspace(*where)) {
			while (isspace(*where)) {
				where++;	/* eat */
			}
		} else if (commenting) {
			where++;
#ifdef NLS
			if (FIRSTof2(where[-1]) ) {
				/* 
				 * Since this is the first of a two byte
				 * character, it cannot terminate a comment.
				 * 
				 * Make sure that the second part of a two
				 * byte character is legal.
				 */
				if (SECof2(where[0]) ) {
					/*
					 * Just skip over the second part of
					 * the two byte character. 
					 */
					where++;
				} else {
					strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,42, "(NLS) illegal second byte in 16-bit character")));
					error(tempbuf);
				}
			} else {
#endif NLS
				if (endcomment(where)) {
					where++;
					commenting--;
				}
#ifdef NLS
			}  /* if (FIRSTof2(where[-2]) ) */
#endif NLS
		} else if (startcomment(where)) {
			where += 2;
			commenting++;
		} else {
			break;
		}
	}

	/*
	 * 'where' is not whitespace, comment or directive Must be a token! 
	 */
	switch (*where) {
	case ':':
		tokp->kind = TOK_COLON;
		where++;
		break;
	case ';':
		tokp->kind = TOK_SEMICOLON;
		where++;
		break;
	case ',':
		tokp->kind = TOK_COMMA;
		where++;
		break;
	case '=':
		tokp->kind = TOK_EQUAL;
		where++;
		break;
	case '*':
		tokp->kind = TOK_STAR;
		where++;
		break;
	case '[':
		tokp->kind = TOK_LBRACKET;
		where++;
		break;
	case ']':
		tokp->kind = TOK_RBRACKET;
		where++;
		break;
	case '{':
		tokp->kind = TOK_LBRACE;
		where++;
		break;
	case '}':
		tokp->kind = TOK_RBRACE;
		where++;
		break;
	case '(':
		tokp->kind = TOK_LPAREN;
		where++;
		break;
	case ')':
		tokp->kind = TOK_RPAREN;
		where++;
		break;
	case '<':
		tokp->kind = TOK_LANGLE;
		where++;
		break;
	case '>':
		tokp->kind = TOK_RANGLE;
		where++;
		break;

	case '"':
		tokp->kind = TOK_STRCONST;
		findstrconst(&where, &tokp->str);
		break;
        
	case '\'':
		tokp->kind = TOK_CHARCONST;
		findcharconst(&where, &tokp->str);
		break;

	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		tokp->kind = TOK_IDENT;
		findconst(&where, &tokp->str);
		break;


	default:
		if (!(isalpha(*where) || *where == '_')) {
			char buf[100];
			char *p;

			s_print(buf, (catgets(nlmsg_fd,NL_SETN,43, "illegal character in file: ")));
			p = buf + strlen(buf);
			if (isprint(*where)) {
				s_print(p, "%c", *where);
			} else {
				s_print(p, "%d", *where);
			}
			error(buf);
		}
		findkind(&where, tokp);
		break;
	}
}



static
unget_token(tokp)
	token *tokp;
{
	lasttok = *tokp;
	pushed = 1;
}


static
findstrconst(str, val)
	char **str;
	char **val;
{
	char *p;
	int size;

	p = *str;
#ifdef NLS
	do {
		*p++;
                if (FIRSTof2(*p) ) {
                        /*
                         * Since this is the first of a two byte
                         * character, it cannot terminate a string literal.
                         *
                         * Just skip to the second part of
                         * the two byte character.
                         */
                        p++;

                        /*
                         * Make sure that the second part of a two
                         * byte character is legal.
                         */
                        if (! SECof2(*p) ) {
				strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,44, "(NLS) illegal second byte in 16-bit character")));
				error(tempbuf);
                        }
		} else if (*p == '"') {
			  break;
		       } 
	} while (*p);
#else NLS
	do {
		*p++;
	} while (*p && *p != '"');
#endif NLS
	if (*p == 0) {
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,45, "unterminated string constant")));
		error(tempbuf);
	}
	p++;
	size = p - *str;
	*val = alloc(size + 1);
	(void) strncpy(*val, *str, size);
	(*val)[size] = 0;
	*str = p;
}

static
findconst(str, val)
	char **str;
	char **val;
{
	char *p;
	int size;

	p = *str;
	if (*p == '0' && *(p + 1) == 'x') {
		p++;
		do {
			p++;
		} while (isxdigit(*p));
	} else {
		do {
			p++;
		} while (isdigit(*p));
	}
	size = p - *str;
	*val = alloc(size + 1);
	(void) strncpy(*val, *str, size);
	(*val)[size] = 0;
	*str = p;
}

static 
findcharconst(str, val)
/*
 * HPNFS  Added by Mike Shipley   CND
 * Allows the use of character constants in the pattern of the case portion
 * of a union declaration.
 * Looking for a character constant of the form
 * 'char'  or  '\alpha'  or '\octal_const'
 * The octal_const can be up to three octal digits.
 */
	char **str;
	char **val;
{
	char *old_place;
	int size;
	int num_of_digits = 0;
	int found = 0;

	old_place = *str;
	(*str)++;
	if (isalnum(**str)) {
	    /* A normal single character constant ??? */
	    (*str)++;
	    if (**str != '\'') {
	        (*str)++;
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,46, "illegal character constant")));
	        error(tempbuf);
	    }
	} else if (**str == '\\') {
		   (*str)++;
		   if (isdigit(**str)) {
		       /* An octal constant ??? It can only be 3 digits*/
		       while (num_of_digits <= 2) {
		           (*str)++;
			   if (isdigit(**str)) {
			       num_of_digits++;
			   } else if (**str == '\'') {
				      found++;
				      break;
			          } else {
	                              (*str)++;
				      strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,47, "illegal character constant")));
				      error(tempbuf);
				  }
		       }  /* while */
		       if (! found ) {
	                   (*str)++;
			   strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,48, "illegal character constant")));
		           error(tempbuf);
		       }
		   } else if (isalpha(**str)) {
			      /* An unprintable character constant (\t) ??? */
			      (*str)++;
			      if (**str != '\'') {
	                          (*str)++;
				  strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,49, "illegal character constant")));
			          error(tempbuf);
			      }
			  } else {
	                          (*str)++;
				  strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,50, "illegal character constant")));
			          error(tempbuf);
			  }
	       } else {
	                (*str)++;
			strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,51, "illegal character constant")));
		        error(tempbuf);
	       } 
	(*str)++;
	size = *str -  old_place;
	*val = alloc(size+1);
	(void) strncpy(*val, old_place, size);
	(*val)[size] = 0;
}



static token symbols[] = {
			  {TOK_CONST, "const"},
			  {TOK_UNION, "union"},
			  {TOK_SWITCH, "switch"},
			  {TOK_CASE, "case"},
			  {TOK_DEFAULT, "default"},
			  {TOK_STRUCT, "struct"},
			  {TOK_TYPEDEF, "typedef"},
			  {TOK_ENUM, "enum"},
			  {TOK_OPAQUE, "opaque"},
			  {TOK_BOOL, "bool"},
			  {TOK_VOID, "void"},
			  {TOK_CHAR, "char"},
			  {TOK_INT, "int"},
			  {TOK_UNSIGNED, "unsigned"},
			  {TOK_SHORT, "short"},
			  {TOK_LONG, "long"},
			  {TOK_FLOAT, "float"},
			  {TOK_DOUBLE, "double"},
			  {TOK_STRING, "string"},
			  {TOK_PROGRAM, "program"},
			  {TOK_VERSION, "version"},
			  {TOK_EOF, "??????"},
};


static
findkind(mark, tokp)
	char **mark;
	token *tokp;
{

	int len;
	token *s;
	char *str;

	str = *mark;
	for (s = symbols; s->kind != TOK_EOF; s++) {
		len = strlen(s->str);
		if (strncmp(str, s->str, len) == 0) {
			if (!isalnum(str[len]) && str[len] != '_') {
				tokp->kind = s->kind;
				tokp->str = s->str;
				*mark = str + len;
				return;
			}
		}
	}
	tokp->kind = TOK_IDENT;
	for (len = 0; isalnum(str[len]) || str[len] == '_'; len++);
	tokp->str = alloc(len + 1);
	(void) strncpy(tokp->str, str, len);
	tokp->str[len] = 0;
	*mark = str + len;
}

static
cppline(line)
	char *line;
{
	return (line == curline && *line == '#');
}

static
directive(line)
	char *line;
{
	return (line == curline && *line == '%');
}

static
printdirective(line)
	char *line;
{
	f_print(fout, "%s", line + 1);
}

static
docppline(line, lineno, fname)
	char *line;
	int *lineno;
	char **fname;
{
	char *file;
	int num;
	char *p;

	line++;
	while (isspace(*line)) {
		line++;
	}
	num = atoi(line);
	while (isdigit(*line)) {
		line++;
	}
	while (isspace(*line)) {
		line++;
	}
	if (*line == '"') {
	    line++;
	    p = file = alloc(strlen(line) + 1);
#ifdef NLS
	    while(*line) {
                if (FIRSTof2(*line) ) {
                        /*
                         * Since this is the first of a two byte
                         * character, it cannot terminate a string literal.
                         *
                         * Just copy the first part of
                         * the two byte character.
                         */
                        *p++ = *line++;
                        
			/*
                         * Make sure that the second part of a two
                         * byte character is legal.
                         */
                        if (! SECof2(*line) ) {
				strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,53, "(NLS) illegal second byte in 16-bit character")));
                                error(tempbuf);
                        }
                } else if (*line == '"') {
                          break;
                       } 
		*p++ = *line++;
            } 
#else NLS
	    while (*line && *line != '"') {
		*p++ = *line++;
	    }
#endif NLS
	    if (*line == 0) {
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,54, "preprocessor error")));
		error(tempbuf);
	    }
	    *p = 0;
	    if (*file == 0) {
		*fname = NULL;
	    } else {
		*fname = file;
	    }
	} else if (*line != 0) {
		strcpy(tempbuf,(catgets(nlmsg_fd,NL_SETN,54, "preprocessor error")));
		error(tempbuf);
	}
	*lineno = num - 1;
}
