/* $Revision: 70.7 $ */

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

#ifndef SUPPORT_HAS_BEEN_INCLUDED
#define SUPPORT_HAS_BEEN_INCLUDED
#include <ctype.h>
#ifdef NLS
#include <nl_ctype.h>
#endif

#define START_MARK 1
#define DIRECTIVE_START 2
#define DEFINE_START 3
#define MACRO_MARK 2
#define END_MARK 3
#define NL_MARK 4

#define boolean int
#define TRUE 1
#define FALSE 0

#if defined(HP_OSF) || defined(__sun)
/* This is the maximum output line size of stderr. */
#define LINESIZE 80
#endif

/* This is the default size used for internal buffers. It is arranged so
 * that 2 * DEFAULT_BUFFER_SIZE + sizeof(ChunkHeader) (8) is a power of two. */
#define DEFAULT_BUFFER_SIZE 8188
extern int buffersize;

/* These are pointers to the two memory allocation area descriptors (temporary
 * and permanent). Their types are opaque, known only to the allocator routines. */
extern struct allocarea *temp_area_ptr, *perm_area_ptr;

/* These are the memory allocation routines. Malloc and realloc are only used
 * for memory which is even more temporary then "temp" memory (see get_sub_line).
 * The other routines are only accessed via the following macros. */
extern char *malloc(), *realloc();
extern char *area_alloc(), *area_realloc();
extern void area_dealloc();

#define temp_alloc(n)		area_alloc((n),temp_area_ptr)
#define temp_realloc(ptr, n)	area_realloc((ptr),(n),temp_area_ptr)
#define temp_dealloc()		area_dealloc(temp_area_ptr, 0)

#define perm_alloc(n)		area_alloc((n),perm_area_ptr)
#define perm_realloc(ptr, n)	area_realloc((ptr),(n),perm_area_ptr)
#define perm_dealloc()		area_dealloc(perm_area_ptr, 0)

extern boolean NLSOption;
extern char *skip_number();
extern char *skip_def_number();

typedef struct {
	boolean is_unsigned;
	long value;
} t_yystype;

#define YYSTYPE t_yystype

#define NEW(type)  ((type *)perm_alloc(sizeof(type)))
#define bail(message)    {error(message); return;}
#define skip_space(line) while(*(line) == ' ' || *(line) == '\t') (line)++;
#define skip_def_space(p) while(   (*(p)==' ') \
                                || (*(p)=='\t') \
                                || ((*(p)==NL_MARK)?(current_lineno++,1):0)) \
                                 (p)++
#ifdef NLS
/* check for a multibyte char */
#define NLScheck(line)	(NLSOption && FIRSTof2((unsigned char)*line) && SECof2((unsigned char)*(line+1)))
#else
/* its always false */
#define NLScheck(line) FALSE
#endif

extern boolean C_Plus_Plus;
#define skip_space_and_com(line) do {\
	if(*(line) == ' ' || *(line) == '\t')\
		(line)++;\
	else if(C_Plus_Plus && *(line) == '/' && *((line)+1) == '/')\
		{\
		line += 2;\
		while(*line != '\n')\
			line++;\
		line++;\
		}\
	else if(*(line) == '/' && *((line)+1) == '*')\
		{\
		line += 2;\
		while(*line != '*' || *(line+1) != '/')\
			{\
			/* If the character is an NLS character pair then skip over the pair to */\
			/* avoid the second character being taken as part of a comment termination. */\
			if (NLScheck(line))\
				line++;\
			line++;\
			}\
		line += 2;\
		}\
	else\
		break;\
	} while(1);

extern char charmap[];
#define ALFSIZ	256
#define IDSTART	1
#define ID	2
#define QUOTE	4
#define is_id_start(ch)  (charmap[ch] & IDSTART)
#define is_id(ch)        (charmap[ch] & ID)
#define is_quote(ch)     (charmap[ch] & QUOTE)

#define back_over_space(line, minimum)  while(line > (minimum) && (*(line-1) == ' ' || *(line-1) == '\t')) line--;
/* This macro skips a quoted literal assuming the current source character is
 * the delimiter character.  It copies characters from the source to the destination
 * up to and including the next matching delimiter encountered.  It also terminates
 * at newline.  A '\' can escape the delimiter within the literal. */
#define skip_quote(source, dest)   {\
	register char delimiter = *source;\
	*dest++ = *source++;\
	while(*source != delimiter && *source != '\n' && *source != '\0')\
		{\
		/* Skip the character following a '\' in case it is the delimiter.\
		 * But don't escape the first char of an NLS pair. */\
		if((*source == '\\') && (! NLScheck(source+1)))\
			*dest++ = *source++;\
		/* If the character is an NLS character pair then skip over the pair to\
		 * avoid the second character being taken as a quote terminating the string. */\
		if (NLScheck(source))\
			*dest++ = *source++;\
		*dest++ = *source++;\
		}\
	if(*source == delimiter)\
		*dest++ = *source++;\
	}


/*
 * This routine converts the given positive number to a string which is
 * copied out to the 'string' parameter.  The 'string' is incremented
 * as the number is written to it.
 */
#define number_to_string(value, string) {\
	register char number[12], *num_ptr = number;\
	register int temp = value;\
	do\
		{\
		*num_ptr++ = temp%10+'0';\
		temp /= 10;\
		}\
	while(temp > 0);\
	while(num_ptr > number)\
		*string++ = *--num_ptr;\
	}

#ifdef HP_OSF
extern char *getenv();
#endif

#ifdef APOLLO_EXT
enum TokenState {is_first, may_be_first, not_first};
extern enum TokenState FirstToken; 
#endif /* APOLLO_EXT */

#if defined(APOLLO_EXT) || defined(CPP_CSCAN)
extern boolean default_dir_set;
#endif /* APOLLO_EXT || CPP_CSCAN */

#endif /* SUPPORT_HAS_BEEN_INCLUDED */
