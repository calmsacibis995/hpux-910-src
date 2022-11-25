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
*			src/pr.c
*
*	This file contains general-purpose display functions.
*
*	    pr_break_and_buffer_line()	-- sw.c
*           pr_put_text()		-- 
*           pr_put_text_fancy()		-- 
*           pr_clear_image()		-- show.c
*           pr_put_string()		-- show.c
*           pr_print_image()		-- show.c
*           pr_print_image_nolf()	-- show.c
*	    pr_init_more()
*	    pr_end_more()
*	    pr_init_file_print()
*	    pr_end_file_print()
*
**++***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "pr.h"
#include "err.h"
#include "config.h"

/***************
* Functions declared in this file
***************/
void	pr_break_and_buffer_line();
void	pr_put_text();
void	pr_put_text_fancy();
void	pr_clear_image();
void	pr_put_string();
void	pr_print_image();
void	pr_print_image_nolf();
void    pr_init_more();
void    pr_end_more();
void	pr_init_file_print();
void	pr_end_file_print();


/************
* Global to this file
************/
static int	        print_more_ct=0;  /* number of mores going on -- can
					     be nested */
static int	        print_file_on=0;  /* are we saving to a file? */
static char        	*print_file_name = "/etc/eisa/config.log";
static char        	*print_err_file_name = "/etc/eisa/config.err";


/************
* Globals declared in globals.c
************/
extern char    		more_file_name[];
extern FILE    		*print_file_fd;
extern FILE    		*print_err_file_fd;
extern int     		auto_mode_messages;
extern struct pmode 	program_mode;


/*************
* Various defines 
*************/
#define LFEED			10
#define TAB_FOUND(p)	  	( (*(p) == '\\') && (*(p+1) == 't') )
#define NEW_LINE_FOUND(p) 	( (*(p) == '\\') && (*(p+1) == 'n') )


/****+++***********************************************************************
*
* Function:     pr_break_and_buffer_line()
*
* Parameters:   text      starting location of text to format
*       	max_chars maximum number of output buffer characters
*       	outbuf    output buffer
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void pr_break_and_buffer_line (text, max_chars, outbuf)
    char   **text;
    int    max_chars;
    char   *outbuf;
{
    int	   chars_out;		/* number of characters formatted */
    int	   i;
    int	   indent;		/* number of characters to next tab stop */
    char   *last_white_space;	/* pointer to last white space */
				/* character encountered */
    char   *start_text; 	/* pointer to the beginning of */
				/* the text being formatted    */


    for (i = 0; i < max_chars; i++)
	*(outbuf+i) = '\0';

    last_white_space = *text;
    start_text = *text;

    /***************************************************/
    /* Format characters until the limit is reached    */
    /* or until a LF or NULL character is encountered. */
    /***************************************************/
    for (chars_out = 0; chars_out <= max_chars; ++*text)  {

        /*******************************************************************/
        /* If the next character is a tab, mark it as the last white space */
        /* character encountered, and calculate the number of spaces	 */
        /* needed to pad the output buffer to the next tab stop.	 */
        /*******************************************************************/
        if (**text == '\t' || TAB_FOUND(*text))  {

	    /* skip over the slash (\t) */
	    if (TAB_FOUND(*text))
		++*text;

	    /* calculate needed spaces */
	    last_white_space = *text;
	    indent = 8 - (chars_out % 8);

	    /* if there is room in the output buffer for the indentation */
	    /*   pad the buufer, otherwise, break out of the loop. */
	    if (chars_out + indent <= max_chars)  {
		chars_out += indent;
		do  {
		    *outbuf++ = ' ';
	        } while (--indent);
	     }
	     else {
	         *outbuf++ = ' ' ;
	         break;
	     }

	}

	/* Otherwise, if the next char is a LF or NULL: stop processing input */
        else  {

	    if (**text == LFEED || NEW_LINE_FOUND(*text) || **text == '\0')  {
	        if (NEW_LINE_FOUND(*text))
	            ++*text;
		last_white_space = *text;
		break;
	    }

	    /* place the char into the output buffer */
	    ++chars_out;
	    *outbuf++ = **text;

	    /* if the character is a space - mark it */
	    if (**text == ' ')
		last_white_space = *text;

	}

    }

    /* if the last white space is the starting input char */
    /* make the last white space be the current input text character */
    if (last_white_space == start_text)
        last_white_space = *text;

    /* if the last white space is a SPACE, or TAB, but not LF or NULL */
    /* shorten the output buffer by one word */
    if (*last_white_space == ' ' || *last_white_space == '\t' ||
	TAB_FOUND (last_white_space) )
	do  {
	    --outbuf;
	    --chars_out;
	} while (*outbuf != ' ');

    /* place the input text pointer to the preceding word's end */
    *text = last_white_space;

    *outbuf = '\0';
}


/****+++***********************************************************************
*
* Function:     pr_put_text()
*
* Parameters:   text	          string to display (must be non-empty)
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   Display a string. Break it up so that it looks nice (given sized lines).
*
****+++***********************************************************************/

void pr_put_text (text)
    char 	*text;
{
    char 	*next_text;
    char 	outbuf[MAXCOL+3];  /* char buffer to hold one formatted line */
    int 	line;		   /* count of lines have been output	  */
    image_buf 	image;


    /* initialize "next_text" to the first character to format  */
    next_text = text;

    /**********
    * Format and print lines until we run out of characters or we have
    * reached the maximum number of lines to display.
    **********/
    for (line = 1; line <= 2000; ++line, ++next_text)  {

        /* get one output line of text from the input buffer ("next_text") */
        pr_break_and_buffer_line(&next_text, 80, outbuf);

        /* display the line of text */
        pr_clear_image(image);
	pr_put_string(image, 0, outbuf);
        pr_print_image(image);

        /* stop if the end-of-text is at hand */
        if (!*next_text)
	    break;

    }

    return;
}


/****+++***********************************************************************
*
* Function:     pr_put_text_fancy()
*
* Parameters:   left_margin       left margin to leave
*       	text	          string to display (must be non-empty)
*       	max_lines	  maximum number of output buffer lines
*				  (must be at least 1)
*       	max_cols          width of the "window" in which to display text
*		mode		  1 = print first line without an indent
*				  0 = print all lines with an indent
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*   Display a string. Break it up so that it looks nice (given sized lines).
*
****+++***********************************************************************/

void pr_put_text_fancy (left_margin, text, max_lines, max_cols, mode)
    int 	left_margin;
    char 	*text;
    int 	max_lines;
    int 	max_cols;
    int		mode;
{
    char 	*next_text;
    char 	outbuf[MAXCOL+3];  /* char buffer to hold one formatted line */
    int 	line;		   /* count of lines have been output	  */
    image_buf 	image;


    /* initialize "next_text" to the first character to format  */
    next_text = text;

    /**********
    * Format and print lines until we run out of characters or we have
    * reached the maximum number of lines to display.
    **********/
    for (line = 1; line <= max_lines; ++line, ++next_text)  {

        /* get one output line of text from the input buffer ("next_text") */
        pr_break_and_buffer_line(&next_text, max_cols, outbuf);

        /* display the line of text */
        pr_clear_image(image);
	if (mode == 1) {
	    mode = 0;
	    pr_put_string(image, 0, outbuf);
	    image[max_cols] = 0;
	}
	else
	    pr_put_string(image, (unsigned)left_margin, outbuf);
        pr_print_image(image);

        /* stop if the end-of-text is at hand */
        if (!*next_text)
	    break;

    }

    return;
}


/****+++***********************************************************************
*
* Function:     pr_clear_image()
*
* Parameters:   image             	image to blank out
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*    Make the passed-in "image" all blanks.
*
****+++***********************************************************************/

void pr_clear_image (image)
    image_buf 	image;
{
    (void)sprintf(image, "%*s", MAXCOL+1, " ");
}




/****+++***********************************************************************
*
* Function:     pr_put_string()
*
* Parameters:   image			image to fill in
*		col			starting column
*		string			string to stuff into image
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*    Stick a string into the passed-in image starting at the specified column.
*
****+++***********************************************************************/

void pr_put_string (image, col, string)
    image_buf 	image;
    unsigned	col;
    char 	*string;
{

    if (col + strlen (string) - 1 <= MAXCOL)  {
	(void)sprintf(image + col, string);
	image[col + strlen(string)] = ' ';
    }
}


/****+++***********************************************************************
*
* Function:     pr_print_image()
*
* Parameters:   image             
*
* Used:		internal and external
*
* Returns:      Nothing
*
* Description:
*
*    Display a single line. If we are doing a "more", just add the line to the
*    temporary file. Otherwise, display it here directly.
*
****+++***********************************************************************/

void pr_print_image (image)
    image_buf 	image;
{


    if (print_more_ct == 0) {
	(void)fprintf(stderr, "%s\n", image);
    }

    else {
	(void)fwrite((void *)image, strlen(image), 1, print_file_fd);
	(void)fwrite((void *)"\n", 1, 1, print_file_fd);
    }

}



/****+++***********************************************************************
*
* Function:     pr_print_image_nolf()
*
* Parameters:   image             
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    Display a buffer -- no lf and don't advance the line count.
*
****+++***********************************************************************/

void pr_print_image_nolf (image)
    image_buf 	image;
{

    (void)fprintf(stderr, "%s", image);
	
}


/****+++***********************************************************************
*
* Function:     pr_init_more()
*
* Parameters:   None              
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	This is used for the "more" feature of the show commands' outputs.
*       It starts the more part off.
*
****+++***********************************************************************/

void pr_init_more()
{
    struct stat         buf;
    int			rc;


    print_more_ct++;
    if ( (print_file_on == 0) && (print_more_ct == 1) )
	print_file_fd = fopen(more_file_name, "w");

    /*************
    * If this is the first auto-mode message we have displayed, open config.err
    * and display a header (to both screen and config.err). The fclose for
    * config.err is done in exiter().
    *************/
    if ( (program_mode.automatic) && (auto_mode_messages == 0) ) {

	/*************
	* If config.err is too big, get rid of the first half of it.
	*************/
	rc = stat(print_err_file_name, &buf);
	if ( (rc == 0) && (buf.st_size > 20000) )  {
	    char		temp_file_name[50];
	    FILE		*temp_file_fd;
	    int			i;
	    unsigned char	ch;

	    (void)strcpy(temp_file_name, tempnam(CFG_FILES_DIR, "ECONF"));
	    temp_file_fd = fopen(temp_file_name, "w");
	    print_err_file_fd = fopen(print_err_file_name, "r");
	    (void)fseek(print_err_file_fd, 10000, SEEK_SET);
	    for (i=10000 ; i<buf.st_size ; i++)  {
		rc = fread((void *)&ch, 1, 1, print_err_file_fd);
		if (rc != 1) 
		    break;
		(void)fwrite((void *)&ch, 1, 1, temp_file_fd);
	    }
	    (void)fclose(print_err_file_fd);
	    (void)fclose(temp_file_fd);
	    (void)rename(temp_file_name, print_err_file_name);
	}

	auto_mode_messages = 1;
	print_err_file_fd = fopen(print_err_file_name, "a");
	err_handler(AUTO_MODE_HEADER_ERRCODE);

    }

}


/****+++***********************************************************************
*
* Function:     pr_end_more()
*
* Parameters:   None              
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*	This is used for the "more" feature of the show commands' outputs.
*       It finishes the more and makes the display happen. If the more
*	command is present, that is used. Otherwise, a hacked (simple)
*	internal more is used.
*
*	If we are in automatic mode, we will also add the accumulated
*	messages onto the end of the config.err file.
*
****+++***********************************************************************/

void pr_end_more()
{
    char        command[60];
    struct stat stat_buf;
    int		rc;
    int		c;
    int		c2;
    int		linect;
    FILE 	*fd;


    print_more_ct--;

    if ( (print_file_on == 0)  &&  (print_more_ct == 0) )   {

	(void)fclose(print_file_fd);

	/**************
	* If the more command is around, this is very simple. Build a 
	* command to "more" the temp file and call system to do it.
	**************/
	rc = stat("/usr/bin/more", &stat_buf);
	if ( (rc == 0) && (stat_buf.st_mode & S_IXOTH) )  {
	    (void)strcpy(command, "/usr/bin/more ");
	    (void)strcat(command, more_file_name);
	    (void)system(command);
	}

	/*************
	* "more" wasn't around, so I'll do it myself. It's simple and
	* it's ugly, but it does work.
	*************/
	else {
	    fd = fopen(more_file_name, "r");
	    linect = 0;
	    c = getc(fd);
	    while (c != EOF)  {
		(void)putc(c, stderr);
		if (c == LFEED) {
		    linect++;
		    if (linect == 22) {
			(void)fprintf(stderr, "\n");
			(void)fprintf(stderr, "                   *** Press Return to go on *** ");
		        c2 = getc(stdin);
		        (void)fprintf(stderr, "\n");
		        linect = 0;
		    }
		}
		c = getc(fd);
	    }
	    (void)fclose(fd);
	}

	/************
	* If we are in automatic mode, add the "more" file onto the
	* end of the config.err file.
	************/
	if (auto_mode_messages == 1)  {
	    fd = fopen(more_file_name, "r");
	    c = getc(fd);
	    while (c != EOF)  {
		(void)putc(c, print_err_file_fd);
		c = getc(fd);
	    }
	}

    }

}


/****+++***********************************************************************
*
* Function:     pr_init_file_print()
*
* Parameters:   None              
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*
****+++***********************************************************************/

void pr_init_file_print()
{
    (void)unlink(print_file_name);
    print_file_fd = fopen(print_file_name, "w");
    print_file_on = 1;
}



/****+++***********************************************************************
*
* Function:     pr_end_file_print()
*
* Parameters:   None              
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*
****+++***********************************************************************/

void pr_end_file_print()
{

    (void)fclose(print_file_fd);
    print_file_on = 0;
}
