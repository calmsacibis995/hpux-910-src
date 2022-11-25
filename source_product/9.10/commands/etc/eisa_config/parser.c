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
*                          src/parser.c
*
*     This file contains a function which is the interface to the cfg file
*     parser.
*     This file contains the error messages for the cfg file parser.
*
*	parser()	-- cfgload.c, main.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "err.h"

extern unsigned		lineno;
extern char		*strbuf;
extern char		*sourcefn;
extern int 		(*get_char)();
extern struct system 	*mptr;
extern int 		parse_err_flag;	  /* This flag is set within errmsg() */
extern struct pmode	program_mode;

extern void 		mn_trapfree();
extern void 		*mn_trapcalloc();
extern void 		initlex();


/****+++***********************************************************************
*
* Function:     parser
*
* Parameters:   sys		pointer to a system struct, filled up here
*		name		the name of the cfg file
*		cfg_getchar	pointer to a get character function
*
* Used:		external only
*
* Returns:      error message	if there was an error
*		NULL		if there was no error
*
* Description:
*
*    This function is called to parse a cfg file. The results of that parsing
*    are put into the sys parameter. An error string is returned if there
*    was a problem in doing the parse.
*
****+++***********************************************************************/

int parser(sys, name, cfg_getchar)
    struct system 	*sys;
    char 		*name;
    int 		(*cfg_getchar)();

{
    int 		result;
    char 		parse_buffer[1000];


    mptr = sys;
    sourcefn = name;
    get_char = cfg_getchar;
    strbuf = parse_buffer;
    initlex();
    lineno = 1;
    parse_err_flag = 0;

    result = yyparse();

    if ( (program_mode.checkcfg) && (parse_err_flag == 0) )
	err_handler(PARSE51_ERRCODE);

    return(result || parse_err_flag);
}
