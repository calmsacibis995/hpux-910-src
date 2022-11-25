/*****************************************************************************
 (C) Copyright Hewlett-Packard Co. 1991. All rights
 reserved.  Copying or other reproduction of this program except for archival
 purposes is prohibited without prior written consent of Hewlett-Packard.

			  RESTRICTED RIGHTS LEGEND

Use, duplication, or disclosure by Government is subject to restrictions
set forth in paragraph (b) (3) (B) of the Rights in Technical Data and
Computer Software clause in DAR 7-104.9(a).

HEWLETT-PACKARD COMPANY
Fort Collins Engineering Operation, Ft. Collins, CO 80525

******************************************************************************/
/******************************************************************************
*   (C) Copyright COMPAQ Computer Corporation 1985, 1989
*+*+*+*************************************************************************
*
*                           src/string.c
*
*   This file contains the parser's string-handling functions.
*
*	format_string()		-- emitter.c
*	savetext()		-- lex.c
*
**++***************************************************************************/


#define MAXSTRING	1000		

#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "def.h"	


/* Functions declared locally */
char		*format_string();
char		*savetext();


extern int  	emb_char_removed;


/****+++***********************************************************************
*
* Function:     format_string
*
* Parameters:   ptr			the string to modify
*		operation:
*                  REMOVE_EMB_CHAR      remove \t's and \n's
*                  CONVERT_EMB_CHAR     convert \t's and \n's (the two char
*					versions) into single chars
*
* Used:		external only
*
* Returns:      the modified string (the input string is also modified)
*
* Description:
*
*    This function removes all \n and \t characters from the input string or
*    converts those characters.
*
****+++***********************************************************************/

char *format_string(ptr, operation)
    char	*ptr;
    int 	operation;
{
    char    	temp[MAXSTRING];
    int 	pcount;
    int 	tcount;


    if (operation == REMOVE_EMB_CHAR)
	emb_char_removed = 0;

    for (pcount = 0, tcount = 0; ptr[pcount] != 0; pcount++) {
	if (ptr[pcount] == '\\') {
	    ++pcount;
	    switch (ptr[pcount]) {

		case 't':
		case 'T':
		    if (operation == REMOVE_EMB_CHAR) {
			emb_char_removed = 1;
			continue;
		    }
		    else
			temp[tcount] = '\t';
		    break;

		case 'n':
		case 'N':
		    if (operation == REMOVE_EMB_CHAR) {
			emb_char_removed = 1;
			continue;
		    }
		    else
			temp[tcount] = '\n';
		    break;

		default:
		    temp[tcount] = ptr[pcount];
		    break;
	    }

	}
	else
	    temp[tcount] = ptr[pcount];

	++tcount;
    }

    temp[tcount] = 0;
    (void)strcpy(ptr, temp);
    return(ptr);
}



/****+++***********************************************************************
*
* Function:     savetext
*
* Parameters:   str		the string to modify
*
* Used:		external only
*
* Returns:      the modified string
*
* Description:
*
*    This function removes leading and trailing white space from the string.
*
****+++***********************************************************************/

char *savetext(str)
    char	*str;
{
    char    	*ptr;
    char    	*savestr;
    int 	cnt;
    char   	 skipws = 1;


    /* save a pointer to the saved string */
    ptr = str;

    /* set up the pointer for saving the string */
    savestr = str;

    for (cnt = 0; *str != '\0'; ++str) {
	if (*str == '\n') {

	    /* truncate trailing whitespace */
	    skipws = 1;
	    while (cnt > 0) {
		--savestr;
		if (isspace(*savestr) && (unsigned)*savestr < 128)
		    --cnt;
		else {
		    ++savestr;
		    if (!((*(str - 2) == '\\') && (*(str - 1) == 'n')))
			*savestr++ = ' ';
		    ++cnt;
		    break;
		}
	    }

	    continue;

	}

	else if (skipws && isspace(*str) && (unsigned)*str < 128)
	    continue;

	skipws = 0;

	*savestr++ = *str;
	++cnt;
    }

    *savestr++ = '\0';

    return(ptr);
}
