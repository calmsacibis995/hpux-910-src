#include <stdio.h>
#include <locale.h>
#include <limits.h>
#include <stdlib.h>
#include <nl_ctype.h>

#ifndef PATH_MAX
# define PATH_MAX 	1023
#endif /* PATH_MAX */

extern unsigned char Dot[];

/*
 *  Struct lang_struct is used if MANPATH includes constructs of the
 *  type %L, %l, %t, %c.  The function parse_lang() breaks up $LANG
 *  into the corresponding components, and puts the strings and
 *  their lengths into the structure.  This is done so that parsing
 *  doesn't have to be repeated for multiple instances of '%'.
 */
#ifdef NLS
typedef struct {
	unsigned char lang[SL_NAME_SIZE];
	int l_lang;
	unsigned char language[SL_NAME_SIZE];
	int l_language;
	unsigned char territory[SL_NAME_SIZE];
	int l_territory;
	unsigned char codeset[SL_NAME_SIZE];
	int l_codeset;
    } lang_struct;
#endif /* NLS */

/****************
 *  next_dir() returns the next pathname from the string pointed to by
 *  by *next.  This is normally MANPATH.  Leading, trailing or adjacent
 *  colons indicate '.' as the path.  
 *  If the pathname contains a '%', it is expanded with <language>, 
 *  <territory> and <codeset> components as indicated.  
 *  The function parse_lang() gets those components from LANG and puts
 *  them in a structure for future use.
 *  As a side-effect, the pointer pointed to by next is left pointing to
 *  the beginning of the next pathname in MANPATH.
 ****************/

unsigned char *
next_dir(next)
unsigned char **next;
{
	/* 
	 * pathspace holds the parsed-out pathname 
	 * path is set at every call to point to pathspace.
	 */
	static unsigned char pathspace[PATH_MAX + 1];
	unsigned char *path = pathspace;

	/* this_one is used to walk down path while copying */
	unsigned char *this_one = pathspace;

	/*  
	 *  save_last points to end of pathspace, used to check for
	 *  boundary overflo
	 */
	unsigned char *save_last;

	/* 
	 * two boolean flags to tell if LANG has been broken up or
	 * a Dot (cwd) has been returned.
	 */
	static short lang_parsed = 0;
	static short Dot_returned = 0;
	short done = 0;

	/*  lang_s holds the stuff parsed out of LANG by parse_lang() */
	static lang_struct lang_s;

	save_last = &pathspace[PATH_MAX];
	pathspace[0] = '\0';

	/*
	 * If upon, entering, we're looking at a NULL pointer or a null
	 * character, we're done.
	 */
	if(*next == NULL || **next == '\0')
	    return(NULL);

	/*
	 *  A colon at this point is either the first character in MANPATH
	 *  or means there were at least two adjacent.  In either case,
	 *  get Dot and return it, set Dot_returned because we'll only 
	 *  do this once.
	 *  We want to eat up multiple adjacent colons as if they were one.
	 */
	while(**next == ':'){
	    (*next)++;
	    if(! Dot_returned){
		++Dot_returned;
		return(Dot);
	    }
	}
	    
	while(!done && this_one < save_last){
	    switch(**next){
	    /*
	     * Because of the above check, this cannot be a first colon,
	     * so it signifies that we've reached the end of a pathname.
	     * If the next character is '\0', this is a trailing colon,
	     * meaning Dot.  We set *next to point to Dot so it will be
	     * found on the next call, then go ahead and return the
	     * pathname just ended.
	     */
	    case ':':
	    	*(*next)++;
	    	if(**next == '\0' && ! Dot_returned)
	    	    *next = Dot;
	    	done = 1;
	    	break;
#ifdef NLS
	    /*
	     *  The '%' specifiers can indicate the LANG variable or
	     *  the language, territory or codeset components thereof.
	     *  On the first one, parse_lang is called to figure out what
	     *  all those are.
	     *  Before any component is added in, a check is made to 
	     *  ensure that the expanded pathname is still within the
	     *  legal length limit.
	     */
	    case '%':
	    	if(! lang_parsed){
	    	    parse_lang(&lang_s);
		    lang_parsed++;
		}
	    	switch(*++*next){
	    	    case 'L': 	/* LANG */
			if((this_one + lang_s.l_lang) < save_last){
	    		    strcpy(this_one,lang_s.lang);
	    		    ++*next;
	    		    this_one += lang_s.l_lang;
	    		    break;
			}
			else
			    goto toolong;
	    	    case 'l':	/* language subcomponent */
			if((this_one + lang_s.l_lang) < save_last){
	    		    strcpy(this_one,lang_s.language);
	    		    ++*next;
	    		    this_one += lang_s.l_language;
	    		    break;
			}
			else
			    goto toolong;
	    	    case 't':	/* territory subcomponent */
			if((this_one + lang_s.l_lang) < save_last){
	    		    strcpy(this_one,lang_s.territory);
	    		    ++*next;
	    		    this_one += lang_s.l_territory;
	    		    break;
			}
			else
			    goto toolong;
	    	    case 'c':	/* codeset subcomponent */
			if((this_one + lang_s.l_lang) < save_last){
        		    strcpy(this_one,lang_s.codeset);
        		    ++*next;
        		    this_one += lang_s.l_codeset;
        		    break;
			}
			else
			    goto toolong;
	    	    case '%':
	    		*this_one++ = '%';
	    		break;
	    	    default:
	    		--*next;
	    		*this_one++ = '%';
	    		break;
	    	}   /* inner switch */
		break;
#endif /* NLS */
	    case '\0':
	    	done = 1;
	    	break;
	    default:
	    	if(FIRSTof2(**next))
	    	    *this_one++ = *(*next)++;
	    	*this_one++ = *(*next)++;
	    }  /* switch */
	}   /* while */
	*this_one = '\0';

	/*
	 *  Check to see if we fell out of the loop because of getting
	 *  too long.  This is an error.  Print a message, and exit with
	 *  an error value.
	 */
	if(this_one >= save_last){
toolong:    fprintf(stderr,"Directory path too long.\n");
	    exit(1);
	}

	/*
	 *  A pathlength of 0 means we had adjacent colons (this is 
	 *  a redundant check), pathlength of 1, and that one character
	 *  is '.' means get and return cwd.
	 */
	if((strlen(path) == 0 || (strlen(path) == 1 && path[0] == '.')) && ! Dot_returned){
	    path = Dot;
	}

	return(path);
}

#ifdef NLS
/******************
 *  parse_lang() breaks up the LANG environment variable into language
 *  territory and codeset parts ( <language>[_<territory>][.<codeset>])
 *  and sets fields in a struct lang_struct for each component.  The 
 *  length of each component, l_<component>, is also set for array 
 *  bound checks later on.
 *****************/

int
parse_lang(langptr)
lang_struct *langptr;
{

    unsigned char *tmp;
    unsigned char *lang;

    /*
     *  If there is no LANG variable set, we really shouldn't even be
     *  here.  But it could happen, so just set arrays to "" and lengths
     *  to 0.  Will cause "" to get inserted back in next_dir(), which
     *  is useful in some contexts.
     */
    if((lang = getenv("LANG")) == NULL){	
	langptr->lang[0] = langptr->language[0] = langptr->territory[0] = langptr->codeset[0] = '\0';
	langptr->l_lang = langptr->l_language = langptr->l_territory = langptr->l_codeset = 0;
    }
    /*
     *  LANG can have form <language>[_<territory>][.<codeset>].
     *  The whole value of LANG is put in the lang array.  The other
     *  arrays get components, if they exist, except that, if there are
     *  no components, ->language gets same value as ->lang.
     */
    else{
        strcpy(langptr->lang,lang);
        tmp = langptr->language;

	/*
	 *  Copy until come to '_' or '.' (or go off end) 
	 */
        while (*lang && *lang != '_' && *lang != '.'){
	    *tmp = *lang;
	    if(FIRSTof2(*lang))
	        *++tmp = *++lang;
	    ++tmp; ++lang;
        }
        if (*lang){
	    /*
	     *  Must have found a '_' or '.'.  If '_' we advance lang,
	     *  finish off what tmp points to with a '\0' (->language),
	     *  and start on ->territory.
	     */
	    if (*lang == '_'){
		++lang;
		*tmp = '\0';
		tmp = langptr->territory;
		while (*lang && *lang != '.'){
		    *tmp = *lang;
		    if(FIRSTof2(*lang))
			*++tmp = *++lang;
		    ++tmp; ++lang;
		}
	    }
	    if(*lang){
		/*
		 *  To be here, must have found a '.'.  Finish last thing
		 *  with '\0' and start on ->codeset.
		 */
	        ++lang;
	        *tmp = '\0';	/* delimit the territory string */
	        tmp = langptr->codeset;
	        while(*lang){
		    *tmp = *lang;
		    if(FIRSTof2(*lang))
		        *++tmp = *++lang;
		    ++tmp; ++lang;
	        }
	        *tmp = '\0';
	    }
        }
    }

    /*
     *  Fill in lengths of all components so we can check for
     *  array overflow.
     */
    langptr->l_lang = strlen(langptr->lang);
    langptr->l_language = strlen(langptr->language);
    if (langptr->territory)
	langptr->l_territory = strlen(langptr->territory);
    if (langptr->codeset)
	langptr->l_codeset = strlen(langptr->codeset);
}
#endif /* NLS */
