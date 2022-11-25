/* @(#) $Revision: 70.3 $ */
/*********************
 *
 *  As a prototype, this code lacks:
 *	- handling of <j012>...<j020> syntax;
 *	- treatment of comment characters;
 *	- comments, with a few exceptions;
 *	- mnemonic variable names in several cases;
 *	- complete error handling, e.g. correct charmap syntax;
 *	  (should be tied in to localedef error handling)
 *	- detection of "END CHARMAP" line;
 *      - Non-replacement of escaped escape chars;  (see Issues in 
 *	  charmap.des);
 *
 *  It is still unclear whether *all* escape_chars in the source locale
 *  script should be converted to '\\'s.  (e.g. what about explicit 
 *  representations of that character?).
 *
 *********************/

#include <stdio.h>
#include <limits.h>
#include <search.h>
#include "global.h"

#define MAX_REPL	20
#define METASTR		",;\"\\<>"

int compar();
void error_rout();
void print_entry();
char *skipb();
FILE *out;

typedef struct char_rep{
	unsigned char *string;
	unsigned char replacement[MAX_REPL+1];
	} CHAR_UNIT;

CHAR_UNIT *t_root = NULL;

int
interpret(charfile, script, tmpfile)
unsigned char *charfile, *script;
FILE *tmpfile;
{

	unsigned char comment = '#', escape = '\\';
	short	mb_max = 1, mb_min = 1;
	unsigned char	input[LINE_MAX], output[LINE_MAX];
	unsigned char *ptr1, *ptr2, savptr2;
	unsigned char *optr1, *optr2;	/* output buffer pointers */
	unsigned char *codeset;
	FILE 	*map, *source; 
	CHAR_UNIT	*temp_unit, s_temp, **match;
	CHAR_UNIT **s_ret;

	if((map = fopen(charfile,"r")) == NULL) {
	     fprintf(stderr,(catgets(catd,NL_SETN,9, "cant open input file %s\n")), charfile);
	    exit(5);
	}
	if((source = fopen(script,"r")) == NULL) {
	     fprintf(stderr,(catgets(catd,NL_SETN,9, "cant open input file %s\n")), script);
	    exit(5);
	}

	while(fgets(input,LINE_MAX,map) != NULL && *input != 'C'){
	    input[strlen(input)-1] = '\0';
	    ptr1 = skipb(input);
	    if(!*ptr1 || *ptr1 == comment) continue;
	    ptr1 = strchr(ptr1,'<');
	    if(ptr1 != NULL)
		ptr1++;
	    else
		error_rout();
	    ptr2 = strchr(ptr1,'>');
	    if(ptr2 != NULL)
		*ptr2++ = '\0';
	    else
		error_rout();
	    ptr2 = strtok(ptr2,"\t ");
	    if(!strcmp(ptr1,"escape_char"))
		escape = *ptr2;
	    if(!strcmp(ptr1,"comment_char"))
		comment = *ptr2;
	    if(!strcmp(ptr1,"mb_cur_max"))
		mb_max = atoi(ptr2);
	    if(!strcmp(ptr1,"mb_cur_min"))
		mb_min = atoi(ptr2);
	    if(!strcmp(ptr1,"code_set_name")){
		codeset = (unsigned char *)malloc(strlen(ptr2)+1);
		strcpy(codeset,ptr2);
	    }
	    /*
	     *  Notice that this just lets through any lines not starting
	     *  with 'C' or having the above strings in them.  Not sure
	     *  if that's good or not -- OK for breadboarding.
	     */
	}

	input[strlen(input)-1] = '\0';
	if(!strcmp(input,"CHARMAP\n"))
	    error_rout();

	while(fgets(input,LINE_MAX,map) != NULL && *input != 'E'){
	    input[strlen(input)-1] = '\0';
	    /*
	     * Ignore empty lines, or ones starting with comment_char
	     */
	    ptr1 = skipb(input);
	    if(!*ptr1 || *ptr1 == comment) continue;
	    ptr2 = strchr(ptr1,'>');
	    if(*ptr1++ != '<' || *ptr2 != '>')
		error_rout();
	    *ptr2++ = '\0';
	    ptr2 = skipb(ptr2);
	    /*
	     * Now we've got the parts, create and store a tree entry
	     */
	    temp_unit = (CHAR_UNIT *)malloc(sizeof(CHAR_UNIT));
	    temp_unit->string = (unsigned char *)malloc(strlen(ptr1)+1);
	    strcpy(temp_unit->string,ptr1);
	    strcpy(temp_unit->replacement,ptr2);
	    s_ret = (CHAR_UNIT **)tsearch((void *)temp_unit,(void **)&t_root,compar);

#ifndef DEBUG
	}
#else	/* DEBUG */
	    if(*s_ret == temp_unit){
		printf("temp_unit inserted\n");
		printf("\t string val: %s\n",(*s_ret)->string);
	    }
	    else
		printf("Insertion unsuccessful\n");    
	}

	printf("The stored tree:\n\n");
	twalk((void *)t_root,print_entry);
#endif /* DEBUG */

	while(fgets(input,LINE_MAX,source) != NULL){
	    ptr1 = input;
	    optr1 = output;
	    if(*ptr1 == comment){
		*optr1++ = '#';
		++ptr1;
	    }
	    while(*ptr1){
		if(*ptr1 == escape){
		    *optr1++ = '\\';
		    ++ptr1;
		}
		if(*ptr1 == '<'){
		    ++ptr1;		/* set to char following '<' */
		    ptr2 = strchr(ptr1,'>');
		    while(*(ptr2-1) == escape)
			ptr2 = strchr(ptr2,'>');
		    savptr2 = *ptr2;
		    *ptr2 = '\0';
		    s_temp.string = ptr1;
		    if(match = (CHAR_UNIT **)tfind((void *)&s_temp,
				(void **)&t_root,compar)){
			optr2 = (*match)->replacement;
			/* 
			 * Just worry about metacharacters in single
			 * char replacement;
			 */
			if(strlen(optr2) == 1){
			    if(in(*optr2,METASTR))
				*optr1++ = '\\';
			    *optr1++ = *optr2;
			}
			else{
			    while(*optr2){
			        if(*optr2 == escape){
				    *optr1++ = '\\';
				    ++optr2;
			        }
			        *optr1++ = *optr2++;
			    }
			}
		        ptr1 = ++ptr2;
		    }
		    else{
			*ptr2 = savptr2;
			*optr1++ = *(ptr1-1);
		    }
	        }
	        else
		    *optr1++ = *ptr1++;

	    }
	*optr1 = '\0';
	fputs(output,tmpfile);
    }
    return(1);
}	

void
error_rout()
{
    fprintf(stderr,(catgets(catd,NL_SETN,81, "Error interpreting charmap file\n")));
    exit(5);
}

int
compar(pt1,pt2)
CHAR_UNIT *pt1, *pt2;
{
#ifdef DEBUG
	printf("In compar():\n");
	printf("\tpt1->string = %s\n\tpt2->string = %s\n\n",pt1->string,pt2->string);
#endif /* DEBUG */

	return(strcmp(pt1->string,pt2->string));
}

#ifdef DEBUG
void
print_entry(element,order,level)
CHAR_UNIT **element;
VISIT order;
int level;
{

	if(order == preorder || order == leaf)
	    printf("symbol: %s\treplacement: %s\n",
	           (*element)->string, (*element)->replacement);
}
#endif /* DEBUG */

int
in(c,st)
unsigned char c;
unsigned char *st;
{

	unsigned char *ptr;

	ptr = st;

	while(*ptr++){
	    if(c == *ptr)
		return(1);
	}

	return(0);
}

char *
skipb(str)
char *str;
{
    while(isspace(*str)) *str++;
    return(str);
	 /*   ptr1 = strtok(input,"\t "); */
}
