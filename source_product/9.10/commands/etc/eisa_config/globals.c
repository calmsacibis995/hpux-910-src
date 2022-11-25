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
*+*+*+*************************************************************************
*
*                          src/globals.c
*
*	This module contains all of the globals which are used by eisa_config.
*	the system.
*
*	Note: vb* functions are not in here yet (if ever).
*
**++***************************************************************************/

#include <sys/param.h>
#include <stdio.h>
#include "config.h"
#include "add.h"


struct system   *mptr;

int 		parse_err_flag;
int		parse_err_no_print = 0;  /* display parse err messages? */
int 		emb_char_removed;

unsigned short  nvm_current;            /* Current source/target memory */

struct pmode    program_mode;

struct system   glb_system;

#ifdef VIRT_BOARD
unsigned int    glb_virtual;                    /* Next virtual slot number */
unsigned int    glb_logical;                    /* Next logical slot number */
unsigned int    glb_embedded;                   /* Next embedded slot number */
#endif

char   		sci_name[MAXPATHLEN];
char   		iodc_name[MAXPATHLEN];		/* outfile for iodc option */

unsigned        lineno;
char            *strbuf;
char            *sourcefn;			/* cfg fname - used in err msg*/
int             (*get_char)();

int		eeprom_fd;			/* fd of eeprom driver */
int		eeprom_opened=0;		/* flag to tell if we have 
						   opened eeprom driver yet */

int		changed_since_start = 0;	/* has config changed since the
						   start of eisa_config?      */

int		changed_since_save = 0;		/* has config changed since the
						   last eeprom save?          */

char    	more_file_name[50] = "";	/* filename of temp file used
						   for printing with more     */
FILE    	*print_file_fd;			/* fd of temp file used
						   for printing with more     */

FILE    	*print_err_file_fd;		/* fd of config.err	      */
int		auto_mode_messages = 0;		/* are we doing messages yet? */
