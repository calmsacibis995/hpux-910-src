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
*                                src/cfgload.c
*
*   This module contains the low level routines for loading a cfg file.
*
*	cfg_load()		-- add.c
*	cfg_load_temp()		--
*	cfg_nextchar()		-- parser.c
*	open_cfg_file() 	-- add.c, main.c
*       make_cfg_file_name()    -- open_save.c, init.c
*       get_cfg_file_base_name() -- show.c
*	make_board_id()		-- nvmlowlevel.c
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/param.h>
#include <fcntl.h>
#include "config.h"
#include "compat.h"
#include "err.h"
#include "add.h"
#include "cf_util.h"
#include "nvm.h"


/***********
* State defines -- are we in a comment, a string, or elsewhere
***********/
#define PP_NORMAL       0
#define PP_STRING       1
#define PP_COMMENT      2

/***********
* cfg_source values -- once we get into PP_ABORT, we cannot get out
***********/
#define PP_ABORT        -2 	/* value tied to lex.c (returned when problem)*/
#define PP_CFGFILE      -3


/***********
* Maximum size of a single token
***********/
#define MAXTKNLEN 	80


/*******************
* Functions in this file
*******************/
struct board 		*cfg_load();
struct system		*cfg_load_temp();
int 			cfg_nextchar();
FILE 			*open_cfg_file();
void			make_cfg_file_name();
void			get_cfg_file_base_name();
int			make_board_id();
static int 		cfg_getchar();
static void 		cfg_ungetchar();
static void 		make_id();


/*******************
* Global to this file only
*******************/
static int      	state;			/* what are we looking for */
static char    		tkn_buffer[MAXTKNLEN+1];/* token's worth of chars  */
static char    		*tkn_ptr;		/* cur loc in the token    */
static int      	tkn_length;		/* cur size of the token   */
static int         	cfg_peek;		/* char pushed back        */
static int         	cfg_source;		/* cfgfile or abort	   */
static FILE            	*cfg_stream;		/* fd of cfg file	   */
static char            	cfg_filename[MAXPATHLEN];/* cfg file name	   */
static unsigned int     cfg_flags;		/* add options		   */
static unsigned short   cfg_checksum;		/* cfg file checksum	   */


/***********
* Globals used here and declared in globals.c
***********/
extern struct system	glb_system;
extern int              parse_err_no_print;


/***********
* Functions used here but declared elsewhere
***********/
extern  void  		del_release_board();
extern  void  		del_release_system();
extern  int		parser();
extern  void		*mn_trapcalloc();
extern  int		sci_make_cfg_file();


/****+++***********************************************************************
*
* Function:     cfg_load()
*
* Parameters:   cfg_name 	Name of cfg file (full pathname)
*   		cfg_handle      Open file handle for cfg file
*		flags		add options
*
* Used:		external only
*
* Returns:      NULL		some sort of problem
*		other		ptr to resulting board
*
* Description:
*
*   The cfg_load function will process a cfg file. It calls parser()
*   to do the real work. This is the only path to the cfg parser
*   (except in the check cfg option). The result of this function (if
*   successful) is a filled in board structure tacked onto glb_system.
*
*   If there is an error in parsing, err_handler() will be called here
*   (or by one of our descendents). NULL will be returned.
*
****+++***********************************************************************/

struct board *cfg_load(cfg_name, cfg_handle, flags)
    char        	*cfg_name;
    FILE        	*cfg_handle;
    unsigned int    	flags;
{
    unsigned int        dupid;
    struct board        *board;
    char		*basename;
    int			err;


    /*****************
    * Set up the state variables for this module.
    *****************/
    tkn_length = 0;
    cfg_peek = EOF;
    cfg_checksum = 0;
    cfg_flags = flags;
    cfg_stream = cfg_handle;
    cfg_source = PP_CFGFILE;
    state = PP_NORMAL;

    /*****************
    * Set up the global for the cfg file name. Also get the duplicate info
    * now -- will be used to update the board structure later.
    *****************/
    (void)strcpy(cfg_filename, cfg_name);
    basename = &cfg_name[sizeof(CFG_FILES_DIR)-1];
    make_id(basename, &dupid);

    /*************************
    * Walk through all of the existing boards. board is positioned to the last
    * board on the list.
    *************************/
    for (board = glb_system.boards; board && board->next; board = board->next)
        ;

    /*************************
    * Call the parser and handle the error case.
    *************************/
    err = parser(&glb_system, cfg_filename, cfg_nextchar);
    if (err != 0) {
        if (board != NULL)
            del_release_board(&glb_system, board->next);
        else
            del_release_board(&glb_system, glb_system.boards);
        board = NULL;
    }

    /*************************
    * If the cfg file parse was successful, update the new board record.
    *************************/
    else {

        for (board = glb_system.boards; board->next; board = board->next)
            ;

        board->duplicate_id = dupid & 0xF;
        board->checksum = cfg_checksum;

    }

    /********************
    * Close the cfg file and get out of here.
    ********************/
    (void)fclose(cfg_handle);
    return(board);
}


/****+++***********************************************************************
*
* Function:     cfg_load_temp()
*
* Parameters:   cfg_name 	Name of cfg file
*
* Used:		external only
*
* Returns:      NULL		some sort of problem
*		other		ptr to resulting system
*
* Description:
*
*	This function is called to load a single cfg file, parse it, and stuff
*	it into a system struct. It will be used in those situations where the
*	user wants some information about a particular board that is not part
*	of the configuration. For example, the user might want help information
*	for a board she has not yet added.
*
*	The caller is responsible for releasing the system structure when it
*	is done using it.
*
****+++***********************************************************************/

struct system *cfg_load_temp(cfg_name)
    char        	*cfg_name;
{
    struct board        *board;
    struct system	*sys;
    char                full_filename[MAXPATHLEN];
    int			err;
    FILE		*cfg_handle;


    /*****************
    * Try to open the cfg file.
    *****************/
    cfg_handle = open_cfg_file(cfg_name, full_filename);
    if (cfg_handle == NULL) {
	if (parse_err_no_print == 0) {
	    err_add_string_parm(1, cfg_name);
	    err_handler(NO_CFG_FILE_ERRCODE);
	}
	return((struct system *)NULL);
    }

    /*****************
    * Set up the state variables for this module.
    *****************/
    tkn_length = 0;
    cfg_peek = EOF;
    cfg_checksum = 0;
    cfg_flags = 0;
    cfg_stream = cfg_handle;
    cfg_source = PP_CFGFILE;
    state = PP_NORMAL;

    /*****************
    * Set up the global for the cfg file name.
    *****************/
    (void)strcpy(cfg_filename, full_filename);

    /*************************
    * Allocate space for the system structure.
    *************************/
    sys = (struct system *)mn_trapcalloc(1, sizeof(struct system));

    /*************************
    * Call the parser and handle the error case.
    *************************/
    err = parser(sys, cfg_filename, cfg_nextchar);
    if (err != 0) {
	del_release_system(sys);
	(void)fclose(cfg_handle);
	return((struct system *)NULL);
    }

    /*************************
    * If the cfg file parse was successful, update the new board record.
    *************************/
    board = sys->boards;
    board->checksum = cfg_checksum;

    /********************
    * Close the cfg file and get out of here.
    ********************/
    (void)fclose(cfg_handle);
    return(sys);
}


/****+++***********************************************************************
*
* Function:     cfg_nextchar()
*
* Parameters:   None              
*
* Used:		internal and external
*
* Returns:      next char from the input stream (or EOF)
*
* Description:
*
*   The cfg_nextchar function will return the next character from the
*   current input stream.  The routine will process the input stream
*   looking for include statement. This function is invoked by the parser
*   to get the next character. It actually does more than just a getchar
*   because it needs to handle include statements in the input stream.
*   Includes are stripped out here and saved away till later (when they
*   will be given to the parser as a normal cfg file). The parser never
*   directly handles an include statement.
*
****+++***********************************************************************/

int cfg_nextchar()
{
    int		chr;


    /*************
    * If we have previously stuffed away characters from the input string
    * into tkn_buffer, return them to the user until tkn_buffer is empty again.
    *************/
    if (tkn_length != 0) {
        tkn_length--;
        return(*tkn_ptr++);
    }

    /***********
    * If we are currently processing a string, just return the character.
    * If the character is the end of string marker ("), change our state
    * back to normal.
    ***********/
    if (state == PP_STRING) {
	chr = cfg_getchar();
	if (chr == '"')
	    state = PP_NORMAL;
	return(chr);
    }

    /***********
    * If we are currently processing a comment, just return the character.
    * If the character is the end of comment marker (CR), change our state
    * back to normal.
    ***********/
    if (state == PP_COMMENT) {
	chr = cfg_getchar();
	if (chr == '\n')
	    state = PP_NORMAL;
	return(chr);
    }

    /***********
    * Otherwise, process the next character. We will pull off a token
    * (if possible) and check it for an include statement. If we grabbed
    * a token (that was not an include), call ourselves recursively to
    * give the parser the first character of the token we squirreled
    * away. If we couldn't grab a token, just return the character we read
    * (after checking for the beginning of a comment or a string).
    ***********/
    chr = cfg_getchar();

    if ( (chr != PP_ABORT)  &&  (isalnum(chr)) )  {

	/************
	* Grab the next token (chars until a non-alphanumeric).
	************/
	tkn_ptr = tkn_buffer;
	tkn_buffer[tkn_length++] = chr;
	while (tkn_length < MAXTKNLEN) {
	    chr = cfg_getchar();
	    if ( (chr != PP_ABORT)  &&  (!isalnum(chr)) )
		break;
	    tkn_buffer[tkn_length++] = chr;
	}
	cfg_ungetchar(chr);
	tkn_buffer[tkn_length] = '\0';

	/***********
	* If the token is the include keyword, flag it as an error
	* right here.
	***********/
	if ( (tkn_length == 7)  &&  (!strcmpi(tkn_buffer, "include")) ){
	    cfg_source = PP_ABORT;
	    if (parse_err_no_print == 0) {
		err_add_string_parm(1, cfg_filename);
		err_handler(ILLEGAL_INCLUDE_ERRCODE);
	    }
	}

	/***********
	* We grabbed a token, so call ourselves to get the first
	* character out of the token (if any).
	**********/
	return(cfg_nextchar());

    }

    /**********
    * Handle the start of a comment.
    **********/
    if (chr == ';') {
	state = PP_COMMENT;
	return(chr);
    }

    /**********
    * Handle the start of a string.
    **********/
    if (chr == '"') {
	state = PP_STRING;
	return(chr);
    }

    /*********
    * No token grabbed, so just return the next character.
    *********/
    return(chr);

}


/****+++***********************************************************************
*
* Function:     cfg_getchar()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      the next char from the input stream (or EOF)
*
* Description:
*
*   The cfg_getchar function will return the next character from the
*   current input stream. There are three cases:
*	o  we have encountered an error in include processing -- return PP_ABORT
*	o  we previously did a cfg_ungetchar() -- grab the pushed character
*	o  neither of the above -- grab the next character from the stream
*
*   If a valid character is returned, add it into the checksum for this cfg
*   file.
*
****+++***********************************************************************/

static int cfg_getchar()
{
    int    chr;


    if (cfg_source == PP_ABORT)
	return(PP_ABORT);

    if (cfg_peek != EOF) {
	chr = cfg_peek;
	cfg_peek = EOF;
	return(chr);
    }

    chr = fgetc(cfg_stream);
    cfg_checksum += chr;
    return(chr);

}




/****+++***********************************************************************
*
* Function:     cfg_ungetchar()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The cfg_ungetchar function will simulate pushing back the specified
*   character onto the current input stream.  Only one character can be pushed
*   back at a time.
*
****+++***********************************************************************/

static void cfg_ungetchar(chr)
    int		chr;
{

    cfg_peek = chr;
}


/****+++***********************************************************************
*
* Function:     make_id
*
* Parameters:   basename		base file name
*		dupid			resulting duplicate id (0 if none)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The make_id function will construct the duplicate id info
*   from a base file name. The duplicate number is the first character
*   in the file name (where the special case '!' is 0).
*
****+++***********************************************************************/

static void make_id(basename, dupid)
    char   	    *basename;
    unsigned int    *dupid;
{

    if (isdigit(*basename))
        *dupid = 0x80 + *basename - '0';

    else
        if (isxdigit(*basename))
            *dupid = 0x80 + toupper(*basename) - 'A' + 10;
        else
            *dupid = 0;

}


/****+++***********************************************************************
*
* Function:     open_cfg_file()
*
* Parameters:   in_name		cfg file name passed in
*		out_name	cfg file name that was successfully opened
*					(full path name)
*
* Used:		internal and external
*
* Returns:      0		Open was not successful
*		other		file descriptor (cfg file was opened)
*
* Description:
*
*     This function is used to provide a uniform way to open a cfg file.
*     The in_name may need the following modifications to match a cfg file
*     which is present:
*	o  it may need to have the pathname for the eisa directory added
*          to its beginning -- this is the only place we look for cfg files
*	o  it may need a .cfg or .CFG suffix added
*	o  it may need to be lower-case
*	o  it may need to be upper-case.
*
*     If all of those fail, we will try getting the CFG file out of the
*     system sci file.
*
*     There are some other checks that could be added in the future:
*       o  may want to try adding a "!" or number as the first char of the
*	   basename if there are no matches without.
*	o  may want to try to match cases like !Test1.cfg when they input
*          !test1 -- this could be done (to an extent) by getting the names
*	   of all files in the directory and converting them all to lowercase,
*          then doing a check
*
****+++***********************************************************************/

FILE *open_cfg_file(in_name, out_name)
    char		*in_name;
    char		*out_name;
{

    FILE		*fd;
    char		base_name[MAXPATHLEN];
    char		*temp;
    char		suffix[20];
    int			bang_prefix = 0;
    unsigned int	boardid;


    /**************
    * If we were given a full path name, make sure it is for the correct
    * directory. If not, toss it out. Either way, copy the base_name into
    * a local for manipulation.
    **************/
    if (*in_name == '/') {
	if ( (strncmp(in_name, CFG_FILES_DIR, strlen(CFG_FILES_DIR)) != 0)  ||
	     (strlen(in_name) == strlen(CFG_FILES_DIR)) )
	    return((FILE *)0);
	(void)strcpy(base_name, &in_name[strlen(CFG_FILES_DIR)]);
    }
    else
	(void)strcpy(base_name, in_name);

    /**************
    * Look for a ! prefix.
    **************/
    if (*base_name == '!')
	bang_prefix = 1;

    /**************
    * Now handle the suffix. If we had a .CFG or a .cfg, strip off the suffix
    * and save it. Otherwise, start the suffix at nothing (what they gave us).
    **************/
    temp = strstr(base_name, ".CFG");
    if (temp == NULL)
	temp = strstr(base_name, ".cfg");
    if (temp != NULL) {
	(void)strcpy(suffix, temp);
	*temp = 0;
    }
    else
	*suffix = 0;

    /**************
    * Try the open on what they gave us (we may have added the right 
    * directory path).
    **************/
    (void)strcpy(out_name, CFG_FILES_DIR);
    (void)strcat(out_name, base_name);
    (void)strcat(out_name, suffix);
    if ((fd = fopen(out_name, "r")) != NULL)
	return(fd);
    if (!bang_prefix) {
	(void)strcpy(out_name, CFG_FILES_DIR);
	(void)strcat(out_name, "!");
	(void)strcat(out_name, base_name);
	(void)strcat(out_name, suffix);
	if ((fd = fopen(out_name, "r")) != NULL)
	    return(fd);
    }

    /**************
    * Try their base with a ".CFG" suffix.
    **************/
    (void)strcpy(out_name, CFG_FILES_DIR);
    (void)strcat(out_name, base_name);
    (void)strcat(out_name, ".CFG");
    if ((fd = fopen(out_name, "r")) != NULL)
	return(fd);
    if (!bang_prefix) {
	(void)strcpy(out_name, CFG_FILES_DIR);
	(void)strcat(out_name, "!");
	(void)strcat(out_name, base_name);
	(void)strcat(out_name, ".CFG");
	if ((fd = fopen(out_name, "r")) != NULL)
	    return(fd);
    }

    /**************
    * Try their base with a ".cfg" suffix.
    **************/
    (void)strcpy(out_name, CFG_FILES_DIR);
    (void)strcat(out_name, base_name);
    (void)strcat(out_name, ".cfg");
    if ((fd = fopen(out_name, "r")) != NULL)
	return(fd);
    if (!bang_prefix) {
	(void)strcpy(out_name, CFG_FILES_DIR);
	(void)strcat(out_name, "!");
	(void)strcat(out_name, base_name);
	(void)strcat(out_name, ".cfg");
	if ((fd = fopen(out_name, "r")) != NULL)
	    return(fd);
    }

    /**************
    * Try all lower case (base and suffix).
    **************/
    (void)strlower(base_name);
    (void)strcpy(out_name, CFG_FILES_DIR);
    (void)strcat(out_name, base_name);
    (void)strcat(out_name, ".cfg");
    if ((fd = fopen(out_name, "r")) != NULL)
	return(fd);
    if (!bang_prefix) {
	(void)strcpy(out_name, CFG_FILES_DIR);
	(void)strcat(out_name, "!");
	(void)strcat(out_name, base_name);
	(void)strcat(out_name, ".cfg");
	if ((fd = fopen(out_name, "r")) != NULL)
	    return(fd);
    }

    /**************
    * Try all upper case (base and suffix).
    **************/
    (void)strupr(base_name);
    (void)strcpy(out_name, CFG_FILES_DIR);
    (void)strcat(out_name, base_name);
    (void)strcat(out_name, ".CFG");
    if ((fd = fopen(out_name, "r")) != NULL)
	return(fd);
    if (!bang_prefix) {
	(void)strcpy(out_name, CFG_FILES_DIR);
	(void)strcat(out_name, "!");
	(void)strcat(out_name, base_name);
	(void)strcat(out_name, ".CFG");
	if ((fd = fopen(out_name, "r")) != NULL)
	    return(fd);
    }

    /**************
    * Try to create the cfg file we need from the system sci file.
    **************/
    if (make_board_id(base_name, &boardid) == 0)
	if (sci_make_cfg_file(boardid, out_name) == 0)
	    if ((fd = fopen(out_name, "r")) != NULL)
		return(fd);

    /**************
    * Did not work, so exit with 0 status.
    **************/
    return(0);
   
}


/****+++***********************************************************************
*
* Function:     make_cfg_file_name()
*
* Parameters:   buffer			target buffer for name
*		boardid			compressed board ID
*		dupid			duplicate ID info
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   The make_cfg_file_name routine will establish a cfg file name from the
*   boardid and dup information passed in. The file name created will be
*   a full pathname.
*
*   The function will first construct a base file name
*   from the compressed board ID and the duplicate ID information.
*
*   The base file name (what goes into "tbuffer") is of the following
*   format:
*              abbbcccd
*   where:
*      a     = the duplicate number (! if there are no duplicates, 1 if this is
*              the first duplicate, 2 if this is the second duplicate, and so
*              on up to F for the fifteenth duplicate)
*      bbb   = the three character manufacturer's code
*      ccc   = the three hex digit product number
*      d     = the one hex digit product revision number
*
*   The compressed board id format (comes in as "boardid") is:
*
*       byte 0    bit 7        0 (reserved)
*
*                 bit 6        compressed character 1 of manufacturer's code 
*                 bit 2
*
*                 bit 1        compressed character 2 of manufacturer's code
*                 bit 0
*       byte 1    bit 7
*                 bit 5
*
*                 bit 4        compressed character 3 of manufacturer's code
*                 bit 0
*
*       byte 2    bit 7        hex digit 1 of the product number
*                 bit 4
*
*                 bit 3        hex digit 2 of the product number
*                 bit 0
*
*       byte 3    bit 7        hex digit 3 of the product number
*                 bit 4
*
*                 bit 3        hex digit 1 of the product revision number
*                 bit 0
*
*       NOTE: Each of the compressed ascii characters is only five bits.
*             The character "A" is represented by 1, and so on.
*
****+++***********************************************************************/

void make_cfg_file_name(buffer, boardid, dupid)
    char		*buffer;
    unsigned char	*boardid;
    nvm_dupid   	*dupid;
{
    char		tbuffer[10];
    char		*ptr;


    /* Put the duplicate id information into the file name. */
    ptr = tbuffer;
    *ptr++ = "!123456789ABCDEF"[dupid->dup_id];

    /* Put the manufacturer's code name into the file name. */
    *ptr++ = ((boardid[0] & 0x7C) >> 2) + '@';
    *ptr++ = ((boardid[0] & 3) << 3) + ((boardid[1] >> 5) & 7) + '@';
    *ptr++ = (boardid[1] & 0x1F) + '@';

    /* Put the product number and revision level into the file name. */
    (void)sprintf(ptr, "%04X", ((unsigned int)boardid[2]<<8) + boardid[3]);

    /* Add the directory prefix and the cfg suffix */
    (void)sprintf(buffer, "%s%s.CFG", CFG_FILES_DIR, tbuffer);
}


/****+++***********************************************************************
*
* Function:     make_board_id()
*
* Parameters:   name			file name coming in (should just be
*					  a base name - possibly with a leading
*				          !)
*		boardid			resulting board id
*
* Used:		internal and external
*
* Returns:      0			successful
*		-1			name was invalid
*
* Description:
*
*   This function takes a cfg file base name and builds the corresponding
*   compressed (4-byte) board id. This is the analog of the 
*   make_cfg_file_name() function.
*
****+++***********************************************************************/

int make_board_id(name, boardid)
    char		*name;
    unsigned int	*boardid;
{
    unsigned char	buffer[10];


    /******************
    * Make sure we have a valid name. There are lots of reasons to
    * reject a name:
    *    o  incorrect length
    *    o  first 3 chars not alpha
    *    o  last 4 chars not hex digits
    * If the verifications pass, copy the base name to buffer and convert it
    * to upper-case.
    ******************/
    if ( (*name == '!')  &&  (strlen(name) == 8) )
	(void)strcpy((char *)buffer, &name[1]);
    else if ( (*name != '!')  &&  (strlen(name) == 7) )
	(void)strcpy((char *)buffer, name);
    else
	return(-1);
    if ( isalpha(buffer[0]) && isalpha(buffer[1]) && isalpha(buffer[2]) &&
	 isxdigit(buffer[3]) && isxdigit(buffer[4]) &&
	 isxdigit(buffer[5]) && isxdigit(buffer[6]) )
	(void)strupr((char *)buffer);
    else
	return(-1);

    /******************
    * Dump the manufacturer's code into the word first.
    ******************/
    *boardid = 0;
    *boardid += (buffer[0] - '@') << 26;
    *boardid += (buffer[1] - '@') << 21;
    *boardid += (buffer[2] - '@') << 16;

    /******************
    * Now dump the product id and revision number in.
    ******************/
    *boardid += strtol((char *)&buffer[3], (char **)NULL, 16);

    return(0);

}


/****+++***********************************************************************
*
* Function:     get_cfg_file_base_name()
*
* Parameters:   full_name		(potentially) full cfg file name
*		base_name		Rreturned basename
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	This function takes a cfg file name and returns only the base portion
*	of it. The full file name may look like this:
*             /etc/eisa/!xxxyyyz.cfg
*       or, at least, something like that.
*
*       The basename for the above example would be:
*             !xxxyyyz
*
****+++***********************************************************************/

void get_cfg_file_base_name(full_name, base_name)
    char		*full_name;
    char		*base_name;
{
    char		*temp;


    /****************
    * If the full_name has a prefix that is the CFG file directory,
    * strip it off.
    ****************/
    if (strncmp(full_name, CFG_FILES_DIR, strlen(CFG_FILES_DIR)) == 0) {
	temp = &full_name[strlen(CFG_FILES_DIR)];
	(void)strcpy(base_name, temp);
    }
    else
	(void)strcpy(base_name, full_name);

    /**************
    * Now strip off a .CFG or a .cfg suffix.
    **************/
    temp = strstr(base_name, ".CFG");
    if (temp != NULL) {
	*temp = 0;
    }
    else {
	temp = strstr(base_name, ".cfg");
	if (temp != NULL) {
	    *temp = 0;
	}
    }

}
