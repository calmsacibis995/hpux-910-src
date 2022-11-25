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
*				src/sw.c
*
*   This file contains routines to draw a picture of a switch or jumper
*   to be displayed.
*
*	sw_disp()		-- init.c, show.c
*	sw_status()		-- init.c
*
**++***************************************************************************/


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "config.h"
#include "sw.h"
#include "pr.h"
#include "err.h"


/*************
* Globals declared in globals.c
*************/
extern struct system 		glb_system;


/*************
* Global to this file only
*************/
static char *space38 = "                                      ";
static char *space34 = "                                  ";
static char *space25 = "                         ";
static char *space22 = "                      ";
static char *space21 = "                     ";
static char *space15 = "               ";
static char *space13 = "             ";
static char *space11 = "           ";
static char *space10 = "          ";
static char *space9  = "         ";
static char *space8  = "        ";
static char *space7  = "       ";
static char *space5  = "     ";
static char *space4  = "    ";
static char *space3  = "   ";
static char *space2  = "  ";

static int  global_flags;
static char **table_ptr;

static char *sw_onOffStr = "    on   off";
static char *sw_horzFact = " Default setting\n";
static char *sw_horzNew = " Required setting\n";
static char *sw_softStr = " Software Statement  ";
static char *sw_swname = " Switch Name:  ";
static char *sw_jpname = " Jumper Name:  ";
static char *sw_labels = " Labels\n";
static char *sw_defjmplabels = " Jumper Positions\n";
static char *sw_factorySpace = "               ";
static char *sw_vertDefault =  "     Default   ";
static char *sw_vertChange = "      Required";

#define MAXLABEL 		10
/* #define MAXCOL 			76 */
#define MINUS1			(long)-1
#define TOTAL_SLOTS		64

#define TITLE_SIZE		500
#define BODY_SIZE		5000

/************
* These are indexes into the string_tbl defined below.
************/
#define DIP1			0
#define DIP2			1
#define DIP3			2
#define DIP4			3
#define DIP5			4

#define ROTARY1			5
#define ROTARY2			6
#define ROTARY3			7
#define ROTARY4			8
#define ROTARY5			9
#define ROTARY6			10
#define ROTARY7			11
#define ROTARY8			12
#define ROTARY9			13

#define SLIDE1			14
#define SLIDE2			15

#define DIP6			16
#define DIP7			17
#define DIP8			18
#define DIP9			19
#define DIP10			20
#define DIP11			21
#define DIP12			22

#define SLIDE3			23
#define SLIDE4			24

#define DIP13			25
#define DIP14			26
#define DIP15			27
#define DIP16			28
#define DIP17			29
#define DIP18			30

#define JUMPER1			31
#define JUMPER2			32
#define JUMPER3			33
#define JUMPER4 		34
#define JUMPER5 		35
#define JUMPER6 		36
#define JUMPER7 		37
#define JUMPER8 		38
#define JUMPER9 		39
#define JUMPER10 		40
#define JUMPER11		41
#define JUMPER12		42
#define JUMPER13		43
#define JUMPER14 		44
#define JUMPER15 		45
#define JUMPER16 		46
#define JUMPER17 		47
#define JUMPER18 		48
#define JUMPER19 		49
#define JUMPER20 		50
#define JUMPER21		51
#define JUMPER22		52
#define JUMPER23		53
#define JUMPER24 		54
#define JUMPER25 		55
#define JUMPER26 		56
#define JUMPER27 		57
#define JUMPER28 		58
#define JUMPER29 		59
#define JUMPER30 		60
#define JUMPER31		61
#define JUMPER32		62
#define JUMPER33		63
#define JUMPER34 		64
#define JUMPER35 		65
#define JUMPER36 		66
#define JUMPER37 		67
#define JUMPER38 		68


/************
* This table contains strings to draw the switch and jumper pictures.
************/
static char *string_tbl[] = {
	 "+---+\n",		/* DIP1 */
	 "+---+\n",		/* DIP2 */
	 "+---+\n",		/* DIP3 */
	 "x   |",		/* DIP4 */
	 "|   x",		/* DIP5 */

	 "x  ----+",		/* ROTARY1 */
	 "   ----+",		/* ROTARY2 */
	 "x  ----+",		/* ROTARY3 */
	 "   ----+",		/* ROTARY4 */
	 "       |\n",		/* ROTARY5 */
	 "x  ----|",		/* ROTARY6 */
	 "   ----|",		/* ROTARY7 */
	 "x  -----",		/* ROTARY8 */
	 "   -----",		/* ROTARY9 */

	 "| x |",		/* SLIDE1 */
	 "|   |",		/* SLIDE2 */

	 "+-",			/* DIP6 */
	 "x",			/* DIP7 */
	 "-",			/* DIP8 */
	 "-+-",			/* DIP9 */
	 "-+on\n",		/* DIP10 */
	 "-+\n",		/* DIP11 */
	 "|   ",		/* DIP12 */
	
	 "| x ",		/* SLIDE3 */
	 "|   ",		/* SLIDE4 */

	 "|\n",			/* DIP13 */
	 "+-",			/* DIP14 */
	 "x",			/* DIP15 */
	 "-+-",			/* DIP16 */
	 "-+off\n",		/* DIP17 */
	 "-+\n",		/* DIP18 */


	 "| o |",		/* JUMPER1  inline */
	 "+---+\n",		/* JUMPER2 */
	 "+---+\n",		/* JUMPER3 */
	 "| | |",		/* JUMPER4 */
	 "|   |",		/* JUMPER5 */

	 "+----+\n",		/* JUMPER6 paired */
	 "+----+\n",		/* JUMPER7 */
	 "|    |\n",		/* JUMPER8 */
	 "|o--o|",		/* JUMPER9 */
	 "|o  o|",		/* JUMPER10 */

	 "+-------+\n",		/* JUMPER11  tripole */
	 "+-------+\n",		/* JUMPER12 */
	 "|       |\n", 	/* JUMPER13 */
	 "|o  o--o|",		/* JUMPER14 */
	 "|o  o  o|",		/* JUMPER15 */
	 "|o--o  o|",		/* JUMPER16 */

	 "       +",		/* JUMPER17 horizontal */
	 "+",			/* JUMPER18 */
	 "----",		/* JUMPER19 */
	 "---",			/* JUMPER20 */
	 "+\n",			/* JUMPER21 */
	 "       +",		/* JUMPER22 */
	 "+",			/* JUMPER23 */
	 "+\n",			/* JUMPER24 */

	 "       | ",		/* JUMPER25 inline jumper horizontal */
	 "o",			/* JUMPER26 */
	 "   ",			/* JUMPER27 */
	 "o |\n",		/* JUMPER28 */

	 "         | ",		/* JUMPER29 paired horizontal jumper */
	 "o   ",		/* JUMPER30 */
	 "o ",			/* JUMPER31 */
	 "|\n",			/* JUMPER32 */
	 "|",			/* JUMPER33 */
	 " ",			/* JUMPER34 */
	 " |\n",		/* JUMPER35 */
	 "o",			/* JUMPER36 */
	 "    ",		/* JUMPER37 */
	 " |  \n"  		/* JUMPER38 */
};



/************
* Functions declared elsewhere and used here
************/
extern void 	pr_break_and_buffer_line();
extern void	*mn_trapcalloc();
extern void	mn_trapfree();


/************
* Functions declared in this file
************/
void			sw_disp();
int			sw_status();
static void		switch_piece();
static void 		jumper_piece();
static char      	*jumper_picture();
static char      	*jumper_horz_picture();
static void		paired_jumper_pic();
static void		inline_jumper_pic();
static void		tripole_jumper_pic();
static char      	*switch_picture();
static char      	*switch_horz_picture();
static char      	*software_string();
static char      	*build_title();
static void		sw_put_text();
static void 		sw_print_buffer();


/****+++***********************************************************************
*
* Function:     sw_disp()
*
* Parameters:   flags		what should be printed (each is a bit):
*				  SWITCH_SOFTWARE:  show software parameters
*				  SWITCH_HARDWARE:  show hardware switches and
*						    jumpers
*				  SWITCH_ONESLOT:   show for one slot only
*				  SWITCH_CHANGED:   only show stuff that changed
*    		board		ptr to board to display (if SWITCH_ONESLOT)
*
* Used:		external only
*
* Returns:	Nothing
*
* Description:
*
*    Builds and displays the picture of the switches specified.
*
****+++***********************************************************************/

void sw_disp(flags, board)
    unsigned 		flags;
    struct board 	*board;
{
    struct bswitch 	*sw;
    struct jumper 	*jmp;
    struct software 	*soft_ptr;
    int			didone=0;
    char		*title;
    char		*body;
    int 		curslot = 0;



    /**************
    * Do some initialization.
    **************/
    table_ptr = string_tbl;
    global_flags = flags;

    /***************
    * If we will display information for all boards, set up a pointer to
    * the first one.
    ***************/
    if ((global_flags & SWITCH_ONESLOT) == 0)
	board = glb_system.boards;

    /**************
    * Walk through all of the boards (or just the one passed in if
    * SWITCH_ONESLOT). Depending on the flags passed in, display
    * hardware switches and jumpers or software statements or both.
    **************/
    while (board) {

	/***************
	* Handle hardware switches and jumpers for this board.
	***************/
	if (global_flags & SWITCH_HARDWARE) {

	    /**************
	    * If there are any switches on this board, display them if:
	    *     o   the user wants everything displayed   -or-
	    *     o   there has been a change and the user only wants changes
	    *************/
	    sw = board->switches;
	    while (sw) {
		if ( (sw->config_bits.current != sw->config_bits.initial)  ||
		     ( (global_flags & SWITCH_CHANGED) == 0) ) {
		    didone = 1;
		    title = build_title(board);
		    if ( (sw->vertical) || (sw->type == st_rotary) )
			body = switch_picture(sw);
		    else
			body = switch_horz_picture(sw);
		    sw_print_buffer(title, body);
		}
		sw = sw->next;
	    }

	    /**************
	    * If there are any jumpers on this board, display them if:
	    *     o   the user wants everything displayed   -or-
	    *     o   there has been a change and the user only wants changes
	    *************/
	    jmp = board->jumpers;
	    while (jmp) {
		if ((jmp->config_bits.current != jmp->config_bits.initial)  || 
		    (jmp->tristate_bits.current!=jmp->tristate_bits.initial)|| 
		    ( (global_flags & SWITCH_CHANGED) == 0) ) {
		    didone = 1;
		    title = build_title(board);
		    if (jmp->vertical)
			body = jumper_picture(jmp);
		    else
			body = jumper_horz_picture(jmp);
		    sw_print_buffer(title, body);
		}
		jmp = jmp->next;
	    }

	}

	/**************
	* Handle software statements for this board.
	* If there are any software statements on this board, display them if:
	*     o   the user wants everything displayed   -or-
	*     o   there has been a change and the user only wants changes
	*************/
	if (global_flags & SWITCH_SOFTWARE) {
	    soft_ptr = board->softwares;
	    while (soft_ptr) {
		if ( (strcmp(soft_ptr->previous, soft_ptr->current) != 0)  ||
		     ( (global_flags & SWITCH_CHANGED) == 0) ) {
		    didone = 1;
		    title = build_title(board);
		    body = software_string(soft_ptr);
		    sw_print_buffer(title, body);
		}
		soft_ptr = soft_ptr->next;
	    }
	}

	/***************
	* If this is SWITCH_ONESLOT mode, terminate the loop. Otherwise,
	* get the next board to be displayed.
	***************/
	board = 0;
	if ( (global_flags & SWITCH_ONESLOT) == 0)  {
	    while (!board && (++curslot < TOTAL_SLOTS)) {
		board = glb_system.boards->next;
		while ((board->slot_number != curslot) && board)
		    board = board->next;
	    }
	}

    }

    /**************
    * If nothing was printed, tell the user about it.
    **************/
    if (didone == 0)  {
	if (global_flags & SWITCH_CHANGED) {
	    if (global_flags & SWITCH_ONESLOT)
		err_handler(NO_SWITCH_CHANGE_IN_SLOT_ERRCODE);
	    else
		err_handler(NO_SWITCH_CHANGE_ERRCODE);
	}
	else if (global_flags & SWITCH_HARDWARE) {
	    if (global_flags & SWITCH_ONESLOT)
		err_handler(NO_SWITCHES_IN_SLOT_ERRCODE);
	    else
		err_handler(NO_SWITCHES_ERRCODE);
	}
    }

}


/****+++***********************************************************************
*
* Function:     sw_status()
*
* Parameters:   sys		ptr to system struct to check for
*
* Used:		external only
*
* Returns:	0		no switches/jumpers are present
*		1		some switches/jumpers are present
*
* Description:
*
*    Checks the system struct to see if there are any switches or jumpers.
*
****+++***********************************************************************/

int sw_status(sys)
    struct system 	*sys;
{
    struct board	*board;


    /**************
    * Walk through all of the boards. If the board has a switch or jumper,
    * return immediately.
    **************/
    board = sys->boards;
    while (board) {

	if (board->switches)
	    return(1);

	if (board->jumpers)
	    return(1);

	board = board->next;

    }

    return(0);

}


/****+++***********************************************************************
*
* Function:     build_title()
*
* Parameters:   board		ptr to board data structure
*
* Used:		internal only
*
* Returns:      string containing the slot number and board name
*
* Description:
*
*    Builds the board name and slot number for a given board.
*
****+++***********************************************************************/

static char *build_title(board)
    struct board 	*board;
{
    char		*str;


    /*************
    * Allocate space for the returned string. It consists of:
    *   o   the slot number line
    *   o   the board name line
    *************/
    str = (char *)mn_trapcalloc(TITLE_SIZE, 1);

    /*************
    * Build the slot number line.
    *************/
    (void)strcpy(str, "\n ");
#ifdef VIRT_BOARD
    if (board->slot_number >= MAX_NUM_SLOTS * 2 - 1)
	(void)strcat(str, SW_VIRTUAL);
    else if (board->slot_number >= MAX_NUM_SLOTS - 1)
	(void)strcat(str, SW_EMBEDDED);
    else
#endif
	(void)strcat(str, glb_system.slot[board->slot_number].label);

    /*************
    * Build the title.
    *************/
    (void)strcat(str, "\n ");
    (void)strcat(str, board->name);

    return(str);
}


/****+++***********************************************************************
*
* Function:     software_string()
*
* Parameters:   soft      Pointer to the software block in the data structure
*
* Used:		internal only
*
* Returns:      string with the software statement block
*
* Description:
*
*    Builds the software text string.
*
****+++***********************************************************************/

static char *software_string(soft)
    struct software 	*soft;
{
    char    		ultoa_buf[10];
    char		*ultoa_ptr=ultoa_buf;
    char		*retstr;


    /*************
    * Get some space for the header label. Copy the label and the software
    * index into it.
    *************/
    retstr = (char *)mn_trapcalloc(BODY_SIZE, 1);
    (void)strcpy(retstr, sw_softStr);
    ultoa_ptr = ultoa((unsigned long)soft->index);
    (void)strcat(retstr, ultoa_ptr);
    (void)strcat(retstr, "\n\n");

    /*************
    * Have sw_put_text() format the description. Add the description onto
    * the retstr.
    *************/
    sw_put_text(1, soft->description, retstr);

    /**************
    * Add the software parameters onto the retstr.
    **************/
    if (soft->current)
	sw_put_text(1, soft->current, retstr);

    return(retstr);
}


/****+++***********************************************************************
*
* Function:     switch_picture()
*
* Parameters:   sw      Pointer to the switch/jumper in the data structure
*
* Used:		internal only
*
* Returns:      Pointer to the switch/jumper picture
*
* Description:
*
*    Builds the picture of the switches specified.
*
****+++***********************************************************************/

static char *switch_picture(sw)
    struct bswitch 	*sw;
{
    char    		*bstr;
    char    		ultoa_buf[100];
    char		*ultoa_ptr=ultoa_buf;
    int 		i;
    int 		swnum;
    struct label 	*labelptr;
    unsigned long   	bit_mask = 1;
    unsigned long   	current_bit = 0;


    /**************
    * Allocate space for the header line and fill it in. The header looks
    * like:
    *      Switch name:  xxxxxxxx
    **************/
    bstr = (char *) mn_trapcalloc(BODY_SIZE, 1);
    (void)strcpy(bstr, sw_swname);
    (void)strcat(bstr, sw->name);
    (void)strcat(bstr, "\n\n");

    /**************
    * Add the switch comments (if any) onto bstr.
    **************/
    if (sw->comments) {
	sw_put_text(1, sw->comments, bstr);
	(void)strcat(bstr, "\n");
    }

    /***************
    * Add the header line for the factory defaults (if there are any).
    ***************/
    if (sw->factory_bits != MINUS1)
	(void)strcat(bstr, sw_vertDefault);
    else
	(void)strcat(bstr, sw_factorySpace);
    (void)strcat(bstr, sw_vertChange);

    /**************
    * If this is a DIP switch, add an on/off label. Otherwise, do the
    * equivalent number of spaces.
    **************/
    if (sw->type == st_dip)
	(void)strcat(bstr, sw_onOffStr);
    else {
	for (i=0 ; i!=strlen(sw_onOffStr) ; i++)
	    (void)strcat(bstr, " ");
    }

    /*************
    * If there are switch labels, add the label header.
    *************/
    if (sw->labels) {
	(void)strcat(bstr, space10);
	(void)strcat(bstr, sw_labels);
    }
    else
	(void)strcat(bstr, "\n");

    /************
    * If this isn't a rotary switch, add a header line (top of switch picture).
    ************/
    if (sw->type != st_rotary) {
	(void)strcat(bstr, space34);
	switch_piece(bstr, sw->type, current_bit, 1);
    }

    /************
    * Set up to print the default and required switch settings. If we are
    * doing reverse numbering, start from one. Otherwise, start from
    * switch length.
    ************/
    if (sw->reverse)
	swnum = 1;
    else
	swnum = sw->width;
    bit_mask = 1;
    bit_mask = bit_mask << (sw->width - 1);

    /*************
    * Walk through each of the positions displaying the default and required
    * settings.
    *************/
    for (i = 1; i <= sw->width; i++) {

	/*************
	* Do the default settings (1 or 0).
	*************/
	(void)strcat(bstr, space8);
	if (sw->factory_bits != MINUS1) {
	    if (bit_mask & sw->factory_bits)
		(void)strcat(bstr, "1");
	    else
		(void)strcat(bstr, "0");
	}
	else
	    (void)strcat(bstr, " ");
	(void)strcat(bstr, space15);

	/*************
	* Do the required settings (1 or 0).
	*************/
	if (bit_mask & sw->config_bits.current)
	    (void)strcat(bstr, "1");
	else
	    (void)strcat(bstr, "0");
	(void)strcat(bstr, space9);

	/**************
	* Do the required settings (picture form).
	**************/
	current_bit = bit_mask & sw->config_bits.current;
	if ((sw->type == st_rotary) && (i == 1)) {
	    if (sw->width == 1)
		switch_piece(bstr, sw->type, current_bit, 4);
	    else
		switch_piece(bstr, sw->type, current_bit, 1);
	}
	else {
	    if ((sw->type == st_rotary) && (i == sw->width))
		switch_piece(bstr, sw->type, current_bit, 0);
	    else
		switch_piece(bstr, sw->type, current_bit, 3);
	}

	/*************
	* Add the switch number.
	*************/
	if (sw->type == st_rotary)
	    (void)strcat(bstr, space5);
	else
	    (void)strcat(bstr, space8);
	ultoa_ptr = ultoa((unsigned long)swnum);
	(void)strcat(bstr, ultoa_ptr);
	if (swnum < 10)
	    (void)strcat(bstr, space4);
	else
	    (void)strcat(bstr, space3);

	/*************
	* Add the label.
	*************/
	labelptr = sw->labels;
	while ( (labelptr != NULL) && (labelptr->position != swnum) )
	    labelptr = labelptr->next;
	if (labelptr != NULL)
	    (void)strcat(bstr, labelptr->string);
	(void)strcat(bstr, "\n");

	/*************
	* Advance to the next switch position.
	*************/
	if (sw->reverse)
	    swnum++;
	else
	    swnum--;
	if (i < sw->width) {
	    (void)strcat(bstr, space34);
	    current_bit = 0;
	    switch_piece(bstr, sw->type, current_bit, 2);
	}
	bit_mask = bit_mask >> 1;

    }

    /*************
    * Finish up with the bottom line of the picture.
    *************/
    if (sw->type != st_rotary) {
	(void)strcat(bstr, space34);
	current_bit = 0;
	switch_piece(bstr, sw->type, current_bit, 0);
    }
    (void)strcat(bstr, "\n");

    return(bstr);

}


/****+++***********************************************************************
*
* Function:     switch_piece()
*
* Parameters:   str			pointer to string to add to
*               type 			type of switch, dip,rotary,slide
*		bit_set 		configuration bit setting
*		position 		0 bottom,1 top,2 middle,3 setting
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This routine adds the correct switch piece to the string that is passed in.
*
****+++***********************************************************************/

static void switch_piece(str, type, bit_set, position)
    char		*str;
    int 		type;
    unsigned long	bit_set;
    int 		position;
{

    switch (type) {
	case st_dip:
	    switch (position) {
		case 0:
		    (void)strcat(str, table_ptr[DIP1]);
		    break;
		case 1:
		    (void)strcat(str, table_ptr[DIP2]);
		    break;
		case 2:
		    (void)strcat(str, table_ptr[DIP3]);
		    break;
		case 3:
		    if (bit_set)
			(void)strcat(str, table_ptr[DIP4]);
		    else
			(void)strcat(str, table_ptr[DIP5]);
	    }
	    break;

	case st_rotary:
	    switch (position) {
		case 0:
		    if (bit_set)
			(void)strcat(str, table_ptr[ROTARY1]);
		    else
			(void)strcat(str, table_ptr[ROTARY2]);
		    break;
		case 1:
		    if (bit_set)
			(void)strcat(str, table_ptr[ROTARY3]);
		    else
			(void)strcat(str, table_ptr[ROTARY4]);
		    break;
		case 2:
		    (void)strcat(str, table_ptr[ROTARY5]);
		    break;
		case 3:
		    if (bit_set)
			(void)strcat(str, table_ptr[ROTARY6]);
		    else
			(void)strcat(str, table_ptr[ROTARY7]);
		    break;
		case 4:
		    if (bit_set)
			(void)strcat(str, table_ptr[ROTARY8]);
		    else
			(void)strcat(str, table_ptr[ROTARY9]);
		    break;
	    }
	    break;

	case st_slide:
	    switch (position) {
		case 0:
		    (void)strcat(str, table_ptr[DIP1]);
		    break;
		case 1:
		    (void)strcat(str, table_ptr[DIP2]);
		    break;
		case 2:
		    (void)strcat(str, table_ptr[DIP3]);
		    break;
		case 3:
		    if (bit_set)
			(void)strcat(str, table_ptr[SLIDE1]);
		    else
			(void)strcat(str, table_ptr[SLIDE2]);
	    }
	    break;

    }

}


/****+++***********************************************************************
*
* Function:     switch_horz_picture()
*
* Parameters:   sw       Pointer to the switch/jumper in the data structure
*
* Used:		internal only
*
* Returns:      Pointer to the switch/jumper picture
*
* Description:
*
*    Builds the picture of the switches specified.
*
****+++***********************************************************************/

static char *switch_horz_picture(sw)
    struct bswitch 	*sw;
{
    char    		*bstr;
    char    		ultoa_buf[10];
    char		*ultoa_ptr=ultoa_buf;
    int 		i;
    int 		j;
    int 		num;
    int 		remaining_letters;
    struct label 	*labelptr;
    unsigned long   	bit_mask = 1;


    /**************
    * Allocate space for the header line and fill it in. The header looks
    * like:
    *      Switch name:  xxxxxxxx
    **************/
    bstr = (char *) mn_trapcalloc(BODY_SIZE, 1);
    (void)strcpy(bstr, sw_swname);
    (void)strcat(bstr, sw->name);
    (void)strcat(bstr, "\n\n");

    /**************
    * Add the switch comments (if any) onto bstr.
    **************/
    if (sw->comments) {
	sw_put_text(1, sw->comments, bstr);
	(void)strcat(bstr, "\n");
    }

    /***************
    * Add the lines for the factory defaults (if there are defaults).
    ***************/
    if (sw->factory_bits != MINUS1) {
	(void)strcat(bstr, sw_horzFact);
	(void)strcat(bstr, space9);
	bit_mask = 1;
	bit_mask = bit_mask << (sw->width - 1);
	for (i = 1; i <= sw->width; i++) {
	    if (bit_mask & sw->factory_bits)
		(void)strcat(bstr, "1");
	    else
		(void)strcat(bstr, "0");
	    (void)strcat(bstr, space3);
	    bit_mask = bit_mask >> 1;
	}
	(void)strcat(bstr, "\n");
    }
    else
	(void)strcat(bstr, " ");

    /***************
    * Add the lines for the required settings (0s and 1s).
    ***************/
    (void)strcat(bstr, sw_horzNew);
    (void)strcat(bstr, space9);
    bit_mask = 1;
    bit_mask = bit_mask << (sw->width - 1);
    for (i = 1; i <= sw->width; i++) {
	if (bit_mask & sw->config_bits.current)
	    (void)strcat(bstr, "1");
	else
	    (void)strcat(bstr, "0");
	(void)strcat(bstr, space3);
	bit_mask = bit_mask >> 1;
    }
    (void)strcat(bstr, "\n");

    /****************
    * Draw the top row of the switch picture.
    ****************/
    (void)strcat(bstr, space7);
    (void)strcat(bstr, table_ptr[DIP6]);
    bit_mask = 1;
    bit_mask = bit_mask << (sw->width - 1);
    for (i = 1; i <= sw->width; i++) {
	if ( (bit_mask & sw->config_bits.current) && (sw->type != st_slide) )
	    (void)strcat(bstr, table_ptr[DIP7]);
	else
	    (void)strcat(bstr, table_ptr[DIP8]);
	bit_mask = bit_mask >> 1;
	if (i != sw->width)
	    (void)strcat(bstr, table_ptr[DIP9]);
	else {
	    if (sw->type == st_dip)
		(void)strcat(bstr, table_ptr[DIP10]);
	    else
		(void)strcat(bstr, table_ptr[DIP11]);
	}
    }

    /****************
    * Draw the middle row of the switch picture.
    ****************/
    (void)strcat(bstr, space7);
    if (sw->type == st_dip) {
	for (i = 1; i <= sw->width; i++)
	    (void)strcat(bstr, table_ptr[DIP12]);
    }
    else {
	bit_mask = 1;
	bit_mask = bit_mask << (sw->width - 1);
	for (i = 1; i <= sw->width; i++) {
	    if (bit_mask & sw->config_bits.current)
		(void)strcat(bstr, table_ptr[SLIDE3]);
	    else
		(void)strcat(bstr, table_ptr[SLIDE4]);
	    bit_mask = bit_mask >> 1;
	}
    }
    (void)strcat(bstr, table_ptr[DIP13]);

    /****************
    * Draw the bottom row of the switch picture.
    ****************/
    (void)strcat(bstr, space7);
    (void)strcat(bstr, table_ptr[DIP14]);
    bit_mask = 1;
    bit_mask = bit_mask << (sw->width - 1);
    for (i = 1; i <= sw->width; i++) {
	if ( (bit_mask & sw->config_bits.current) || (sw->type == st_slide) )
	    (void)strcat(bstr, table_ptr[DIP8]);
	else
	    (void)strcat(bstr, table_ptr[DIP15]);
	bit_mask = bit_mask >> 1;
	if (i != sw->width)
	    (void)strcat(bstr, table_ptr[DIP16]);
	else {
	    if (sw->type == st_dip)
		(void)strcat(bstr, table_ptr[DIP17]);
	    else
		(void)strcat(bstr, table_ptr[DIP18]);
	}
    }

    /***************
    * Add the switch position numbers.
    ***************/
    if (sw->reverse) {
	(void)strcat(bstr, space9);
	for (i = 1; i <= sw->width; i++) {
	    ultoa_ptr = ultoa((unsigned long)i);
	    (void)strcat(bstr, ultoa_ptr);
	    if (i + 1 < 10)
		(void)strcat(bstr, space3);
	    else
		(void)strcat(bstr, space2);
	}
    }
    else {
	if (sw->width > 10)
	    (void)strcat(bstr, space8);
	else
	    (void)strcat(bstr, space9);
	for (i = sw->width; i > 0; i--) {
	    ultoa_ptr = ultoa((unsigned long)i);
	    (void)strcat(bstr, ultoa_ptr);
	    if (i - 1 < 10)
		(void)strcat(bstr, space3);
	    else
		(void)strcat(bstr, space2);
	}
    }
    (void)strcat(bstr, "\n\n");

    /**************
    * If there are switch position labels, add them. Note that the labels
    * must be displayed vertically, a single character of each label per line.
    **************/
    if (sw->labels)
	(void)strcat(bstr, sw_labels);
    else
	(void)strcat(bstr, "\n");

    remaining_letters = 1;
    for (num=0 ; ((num < MAXLABEL) && remaining_letters && sw->labels); num++) {

	if (sw->reverse)
	    j = 1;
	else
	    j = sw->width;

	remaining_letters = 0;
	(void)strcat(bstr, space9);

	for (i = 1; i <= sw->width; i++) {
	    labelptr = sw->labels;
	    while ((labelptr->position != j) && (labelptr != NULL))
		labelptr = labelptr->next;
	    if ((labelptr != NULL) && (strlen(labelptr->string) > num)) {
		remaining_letters = 1;
		ultoa_buf[0] = labelptr->string[num];
		ultoa_buf[1] = 0;
		(void)strcat(bstr, ultoa_buf);
		(void)strcat(bstr, space3);
	    }
	    else
		(void)strcat(bstr, space4);
	    if (sw->reverse)
		j++;
	    else
		j--;
	}

	(void)strcat(bstr, "\n");

    }

    (void)strcat(bstr, "\n");

    return(bstr);
}


/****+++***********************************************************************
*
* Function:     jumper_picture()
*
* Parameters:   jmp     Pointer to the switch/jumper in the data structure
*
* Used:		internal only
*
* Returns:      Pointer to the switch/jumper picture
*
* Description:
*
*    Builds the picture of the jumpers specified.
*
****+++***********************************************************************/

static char *jumper_picture(jmp)
    struct jumper 	*jmp;
{
    char    		*bstr;
    char    		ultoa_buf[10];
    char		*ultoa_ptr=ultoa_buf;
    int 		i;
    int 		jmpnum;
    unsigned long   	bit_mask = 1;
    unsigned long   	current_bit = 0;
    unsigned long   	tri_bit = 0;
    struct label 	*labelptr;


    /**************
    * Allocate space for the header line and fill it in. The header looks
    * like:
    *      Jumper name:  xxxxxxxx
    **************/
    bstr = (char *) mn_trapcalloc(BODY_SIZE, 1);
    (void)strcpy(bstr, sw_jpname);
    (void)strcat(bstr, jmp->name);
    (void)strcat(bstr, "\n\n");

    /**************
    * Add the jumper comments (if any) onto bstr.
    **************/
    if (jmp->comments) {
	sw_put_text(3, jmp->comments, bstr);
	(void)strcat(bstr, "\n");
    }

    /***************
    * Add the header line for the factory defaults (if there are any).
    ***************/
    if (jmp->factory.data_bits != MINUS1)
	(void)strcat(bstr, sw_vertDefault);
    else
	(void)strcat(bstr, sw_factorySpace);
    (void)strcat(bstr, sw_vertChange);

    /*************
    * If there are jumper labels, add the label header.
    *************/
    switch (jmp->type) {
	case jt_tripole:
	    (void)strcat(bstr, space25);
	    break;
	case jt_paired:
	    (void)strcat(bstr, space22);
	    break;
	case jt_inline:
	    (void)strcat(bstr, space21);
	    break;
    }
    if (jmp->labels)
	(void)strcat(bstr, sw_labels);
    else
	(void)strcat(bstr, sw_defjmplabels);

    /**************
    * Add a header line (top of jumper picture).
    **************/
    (void)strcat(bstr, space38);
    jumper_piece(bstr, jmp->type, current_bit, tri_bit, 1);

    /**************
    * Set up to walk through jumper positions.
    **************/
    if (jmp->reverse)
	jmpnum = 1;
    else if (jmp->on_post)
	jmpnum = jmp->width + 1;
    else
	jmpnum = jmp->width;

    /*************
    * Move to the correct label (if any) and display it if on_post.
    *************/
    labelptr = jmp->labels;
    while ( (labelptr != NULL) && (labelptr->position != jmpnum) )
	labelptr = labelptr->next;
    if (labelptr) {
	if (jmp->on_post) {
	    (void)strcat(bstr, space8);
	    (void)strcat(bstr, labelptr->string);
	    labelptr = labelptr->next;
	}
    }

    /**************
    * If we have an inline jumper with no labels, use the position as the
    * label.
    **************/
    if (jmp->type == jt_inline) {
	if (jmp->labels == NULL) {
	    (void)strcat(bstr, space8);
	    ultoa_ptr = ultoa((unsigned long)jmpnum);
	    (void)strcat(bstr, ultoa_ptr);
	}
	(void)strcat(bstr, "\n");
    }

    /*****************
    * Walk through each of the jumper positions displaying settings.
    *****************/
    bit_mask = 1;
    bit_mask = bit_mask << (jmp->width - 1);
    for (i = 1; i <= jmp->width; i++) {

	/*************
	* Do the default settings (1, 0, or n).
	*************/
	(void)strcat(bstr, space8);
	if (jmp->factory.data_bits != MINUS1) {
	    if (bit_mask & jmp->factory.data_bits)
		(void)strcat(bstr, "1");
	    else {
		if (bit_mask & jmp->factory.tristate_bits)
		    (void)strcat(bstr, "n");
		else
		    (void)strcat(bstr, "0");
	    }
	}
	else
	    (void)strcat(bstr, " ");
	(void)strcat(bstr, space15);

	/*************
	* Do the required settings (1, 0, or n).
	*************/
	/* check current setting */
	if (bit_mask & jmp->config_bits.current)
	    (void)strcat(bstr, "1");
	else {
	    if (bit_mask & jmp->tristate_bits.current)
		(void)strcat(bstr, "n");
	    else
		(void)strcat(bstr, "0");
	}
	(void)strcat(bstr, space13);

	/*************
	* Do the picture part for this position.
	*************/
	current_bit = bit_mask & jmp->config_bits.current;
	tri_bit = bit_mask & jmp->tristate_bits.current;
	jumper_piece(bstr, jmp->type, current_bit, tri_bit, 3);

	/************
	* Add labels if there are any.
	************/
	labelptr = jmp->labels;
	while ((labelptr->position != jmpnum) && (labelptr != NULL))
	    labelptr = labelptr->next;
	if (labelptr) {
	    if (!jmp->on_post) {
		(void)strcat(bstr, space8);
		(void)strcat(bstr, labelptr->string);
		labelptr = labelptr->next;
	    }
	}

	/***************
	* Add numbers if labels do not exist.
	***************/
	if (!jmp->labels && (jmp->type != jt_inline)) {
	    (void)strcat(bstr, space8);
	    ultoa_ptr = ultoa((unsigned long)jmpnum);
	    (void)strcat(bstr, ultoa_ptr);
	}

	/* if reverse numbering start from one */
	if (jmp->reverse)
	    jmpnum++;
	else
	    jmpnum--;
	(void)strcat(bstr, "\n");

	if (i < jmp->width) {

	    (void)strcat(bstr, space38);
	    current_bit = 0;
	    jumper_piece(bstr, jmp->type, current_bit, tri_bit, 2);

	    labelptr = jmp->labels;
	    while ((labelptr->position != jmpnum) && (labelptr != NULL))
		labelptr = labelptr->next;

	    /* add labels if there are any */
	    if (labelptr) {
		if (jmp->on_post) {
		    (void)strcat(bstr, space8);
		    (void)strcat(bstr, labelptr->string);
		    labelptr = labelptr->next;
		}
	    }

	    if (jmp->type == jt_inline) {
		if (jmp->labels == NULL) {
		    (void)strcat(bstr, space8);
		    ultoa_ptr = ultoa((unsigned long)jmpnum);
		    (void)strcat(bstr, ultoa_ptr);
		}
		(void)strcat(bstr, "\n");
	    }

	}

	bit_mask = bit_mask >> 1;
    }

    (void)strcat(bstr, space38);
    current_bit = 0;

    if (jmp->type == jt_inline) {

	jumper_piece(bstr, jmp->type, current_bit, tri_bit, 2);

	if (labelptr = jmp->labels) {

	    while ((labelptr->position != jmpnum) && (labelptr != NULL))
		labelptr = labelptr->next;

	    /* add labels if there are any */
	    if (labelptr) {
		if (labelptr->position == jmpnum) {
		    if (jmp->on_post) {
			(void)strcat(bstr, space8);
			(void)strcat(bstr, labelptr->string);
			labelptr = labelptr->next;
		    }
		}
	    }

	}

	else {
	    (void)strcat(bstr, space8);
	    ultoa_ptr = ultoa((unsigned long)jmpnum);
	    (void)strcat(bstr, ultoa_ptr);
	}

	(void)strcat(bstr, "\n");
	(void)strcat(bstr, space38);

    }

    jumper_piece(bstr, jmp->type, current_bit, tri_bit, 0);
    (void)strcat(bstr, "\n");
    return(bstr);
}


/****+++***********************************************************************
*
* Function:     jumper_piece()
*
* Parameters:   str	    pointer to string to add piece to
*               type	    type of switch, dip,rotary,slide
*		bit_set	    configuration bit setting
*		tri_set	    tristate bit setting
*		position    0:bottom, 1:top, 2:middle, 3:setting
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This routine adds a piece of a jumper picture to the specified string.
*
****+++***********************************************************************/

static void jumper_piece(str, type, bit_set, tri_set, position)
    char		*str;
    int 		type;
    unsigned long	bit_set;
    unsigned long	tri_set;
    int 		position;
{

    switch (type) {

	case jt_inline:
	    switch (position) {
		case 0:
		    (void)strcat(str, table_ptr[JUMPER2]);
		    break;
		case 1:
		    (void)strcat(str, table_ptr[JUMPER3]);
		    (void)strcat(str, space38);
		    (void)strcat(str, table_ptr[JUMPER1]);
		    break;
		case 2:
		    (void)strcat(str, table_ptr[JUMPER1]);
		    break;
		case 3:
		    if (bit_set)
			(void)strcat(str, table_ptr[JUMPER4]);
		    else
			(void)strcat(str, table_ptr[JUMPER5]);
	    }
	    break;

	case jt_paired:
	    switch (position) {
		case 0:
		    (void)strcat(str, table_ptr[JUMPER6]);
		    break;
		case 1:
		    (void)strcat(str, table_ptr[JUMPER7]);
		    break;
		case 2:
		    (void)strcat(str, table_ptr[JUMPER8]);
		    break;
		case 3:
		    if (bit_set)
			(void)strcat(str, table_ptr[JUMPER9]);
		    else
			(void)strcat(str, table_ptr[JUMPER10]);
	    }
	    break;

	case jt_tripole:
	    switch (position) {
		case 0:
		    (void)strcat(str, table_ptr[JUMPER11]);
		    break;
		case 1:
		    (void)strcat(str, table_ptr[JUMPER12]);
		    break;
		case 2:
		    (void)strcat(str, table_ptr[JUMPER13]);
		    break;
		case 3:
		    if (bit_set)
			(void)strcat(str, table_ptr[JUMPER14]);
		    else if (tri_set)
			(void)strcat(str, table_ptr[JUMPER15]);
		    else
			(void)strcat(str, table_ptr[JUMPER16]);
	    }
	    break;

    }

}


/****+++***********************************************************************
*
* Function:     jumper_horz_picture()
*
* Parameters:   jmp      Pointer to the switch/jumper in the data structure
*
* Used:		internal only
*
* Returns:      Pointer to the switch/jumper picture
*
* Description:
*
*    Builds the horizontal picture of the specified.
*
****+++***********************************************************************/

static char *jumper_horz_picture(jmp)
    struct jumper 	*jmp;
{
    char    		*bstr;
    char    		ultoa_buf[10];
    char		*ultoa_ptr=ultoa_buf;
    int 		i;
    int 		j;
    int 		num;
    int 		remaining_letters;
    int 		count;
    struct label 	*labelptr;
    unsigned long   	bit_mask = 1;


    /**************
    * Allocate space for the header line and fill it in. The header looks
    * like:
    *      Jumper name:  xxxxxxxx
    **************/
    bstr = (char *) mn_trapcalloc(BODY_SIZE, 1);
    (void)strcpy(bstr, sw_jpname);
    (void)strcat(bstr, jmp->name);
    (void)strcat(bstr, "\n\n");

    /**************
    * Add the jumper comments (if any) onto bstr.
    **************/
    if (jmp->comments) {
	sw_put_text(3, jmp->comments, bstr);
	(void)strcat(bstr, "\n");
    }

    /**************
    * Display the factory settings (if any).
    **************/
    if (jmp->factory.data_bits != MINUS1) {
	(void)strcat(bstr, sw_horzFact);
	(void)strcat(bstr, space11);
	bit_mask = 1;
	bit_mask = bit_mask << (jmp->width - 1);
	for (i = 1; i <= jmp->width; i++) {
	    if (bit_mask & jmp->factory.data_bits)
		(void)strcat(bstr, "1");
	    else if (bit_mask & jmp->factory.tristate_bits)
		(void)strcat(bstr, "n");
	    else
		(void)strcat(bstr, "0");
	    (void)strcat(bstr, space3);
	    bit_mask = bit_mask >> 1;
	}
    }

    /**************
    * Display the required settings (1, 0, or n).
    **************/
    (void)strcat(bstr, "\n");
    (void)strcat(bstr, sw_horzNew);
    (void)strcat(bstr, space11);
    bit_mask = 1;
    bit_mask = bit_mask << (jmp->width - 1);
    for (i = 1; i <= jmp->width; i++) {
	if (bit_mask & jmp->config_bits.current)
	    (void)strcat(bstr, "1");
	else if (bit_mask & jmp->tristate_bits.current)
	    (void)strcat(bstr, "n");
	else
	    (void)strcat(bstr, "0");
	(void)strcat(bstr, space3);
	bit_mask = bit_mask >> 1;
    }
    (void)strcat(bstr, "\n");

    /**************
    * Draw the top row of the jumper picture.
    **************/
    if (jmp->type == jt_inline) {
	(void)strcat(bstr, table_ptr[JUMPER17]);
	j = jmp->width + 1;
    }
    else {
	(void)strcat(bstr, space9);
	(void)strcat(bstr, table_ptr[JUMPER18]);
	j = jmp->width;
    }
    for (i = 1; i <= j; i++) {
	if (i < j)
	    (void)strcat(bstr, table_ptr[JUMPER19]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER20]);
    }
    (void)strcat(bstr, table_ptr[JUMPER21]);

    /**************
    * Draw the post rows of the jumper picture.
    **************/
    switch (jmp->type) {
	case jt_inline:
	    inline_jumper_pic(jmp, bstr);
	    break;
	case jt_paired:
	    paired_jumper_pic(jmp, bstr);
	    break;
	case jt_tripole:
	    tripole_jumper_pic(jmp, bstr);
	    break;
    }

    /**************
    * Draw the bottom row of the jumper picture.
    **************/
    if (jmp->type == jt_inline)
	(void)strcat(bstr, table_ptr[JUMPER22]);
    else {
	(void)strcat(bstr, space9);
	(void)strcat(bstr, table_ptr[JUMPER23]);
    }
    for (i = 1; i <= j; i++) {
	if (i < j)
	    (void)strcat(bstr, table_ptr[JUMPER19]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER20]);
    }
    (void)strcat(bstr, table_ptr[JUMPER24]);

    /************
    * Set up to do labels.
    ************/
    if (jmp->type == jt_inline)
	count = jmp->width + 1;
    else
	count = jmp->width;

    /***************
    * Add labels.
    ***************/
    if (jmp->labels) {
	(void)strcat(bstr, "\n");
	(void)strcat(bstr, sw_labels);
    }
    else {
	(void)strcat(bstr, "\n");
	(void)strcat(bstr, sw_defjmplabels);
    }

    /***************
    * Add jumper numbers if labels are not defined.
    ***************/
    if (!jmp->labels) {

	if (jmp->reverse) {
	    if (jmp->type == jt_inline)
		(void)strcat(bstr, space9);
	    else
		(void)strcat(bstr, space11);
	    for (i = 1; i <= count; i++) {
		ultoa_ptr = ultoa((unsigned long)i);
		(void)strcat(bstr, ultoa_ptr);
		if (i + 1 < 10)
		    (void)strcat(bstr, space3);
		else
		    (void)strcat(bstr, space2);
	    }
	}

	else {
	    if (jmp->width > 10) {
		if (jmp->type == jt_inline)
		    (void)strcat(bstr, space8);
		else
		    (void)strcat(bstr, space10);
	    }
	    else {
		if (jmp->type == jt_inline)
		    (void)strcat(bstr, space9);
		else
		    (void)strcat(bstr, space11);
	    }
	    for (i = count; i > 0; i--) {
		ultoa_ptr = ultoa((unsigned long)i);
		(void)strcat(bstr, ultoa_ptr);
		if (i - 1 < 10)
		    (void)strcat(bstr, space3);
		else
		    (void)strcat(bstr, space2);
	    }
	}

	(void)strcat(bstr, "\n");
    }

    /***************
    * Else, display the labels
    ***************/
    else  {

	remaining_letters = 1;
	for (num = 0; ((num < MAXLABEL) && remaining_letters && jmp->labels); num++) {
	    if (jmp->reverse)
		j = 1;
	    else if (jmp->on_post)
		j = jmp->width + 1;
	    else
		j = jmp->width;

	    remaining_letters = 0;

	    if (jmp->on_post) {
		count = jmp->width + 1;
		(void)strcat(bstr, space9);
	    }
	    else {
		count = jmp->width;
		(void)strcat(bstr, space11);
	    }

	    for (i = 1; i <= count; i++) {

		labelptr = jmp->labels;
		while ((labelptr->position != j) && (labelptr != NULL))
		    labelptr = labelptr->next;

		/* a character was found */
		if ((labelptr != NULL) && (strlen(labelptr->string) > num)) {
		    remaining_letters = 1;
		    ultoa_buf[0] = labelptr->string[num];
		    ultoa_buf[1] = 0;
		    (void)strcat(bstr, ultoa_buf);
		    (void)strcat(bstr, space3);
		}
		else
		    (void)strcat(bstr, space4);

		labelptr = labelptr->next;
		if (jmp->reverse)
		    j++;
		else
		    j--;

	    }

	    (void)strcat(bstr, "\n");

	}

    }

    (void)strcat(bstr, "\n");

    return(bstr);
}


/****+++***********************************************************************
*
* Function:     inline_jumper_pic()
*
* Parameters:   jmp   Pointer to the jumper in the data structure
*		bstr  string to put it in
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Builds the horizontal picture of the jumper specified.
*
****+++***********************************************************************/

static void inline_jumper_pic(jmp, bstr)
    struct jumper 	*jmp;
    char		*bstr;
{
    int 		i;
    unsigned long   	bit_mask = 1;


    bit_mask = bit_mask << (jmp->width - 1);
    (void)strcat(bstr, table_ptr[JUMPER25]);
    for (i = 1; i <= jmp->width; i++) {
	(void)strcat(bstr, table_ptr[JUMPER26]);
	if (bit_mask & jmp->config_bits.current)
	    (void)strcat(bstr, table_ptr[JUMPER20]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER27]);
	bit_mask = bit_mask >> 1;
    }

    (void)strcat(bstr, table_ptr[JUMPER28]);

}


/****+++***********************************************************************
*
* Function:     paired_jumper_pic()
*
* Parameters:   iibptr     Pointer to the jumper in the data structure
*		bstr
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*      Builds the horizontal picture of the specified.
*
****+++***********************************************************************/

static void paired_jumper_pic(jmp, bstr)
    struct jumper 	*jmp;
    char		*bstr;
{
    int 		i;
    unsigned long   	bit_mask = 1;


    (void)strcat(bstr, table_ptr[JUMPER29]);
    for (i = 1; i <= jmp->width; i++) {
	if (i < jmp->width)
	    (void)strcat(bstr, table_ptr[JUMPER30]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER31]);
    }
    (void)strcat(bstr, table_ptr[JUMPER32]);

    (void)strcat(bstr, table_ptr[JUMPER29]);
    bit_mask = 1;
    bit_mask = bit_mask << (jmp->width - 1);
    for (i = 1; i <= jmp->width; i++) {
	if (bit_mask & jmp->config_bits.current) {
	    (void)strcat(bstr, table_ptr[JUMPER33]);
	    if (i < jmp->width)
		(void)strcat(bstr, space3);
	}
	else {
	    if (i < jmp->width)
		(void)strcat(bstr, space4);
	    else
		(void)strcat(bstr, table_ptr[JUMPER34]);
	}
	bit_mask = bit_mask >> 1;
    }

    (void)strcat(bstr, table_ptr[JUMPER35]);
    (void)strcat(bstr, table_ptr[JUMPER29]);

    for (i = 1; i <= jmp->width; i++)
	if (i < jmp->width)
	    (void)strcat(bstr, table_ptr[JUMPER30]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER36]);

    (void)strcat(bstr, table_ptr[JUMPER35]);
}


/****+++***********************************************************************
*
* Function:     tripole_jumper_pic()
*
* Parameters:   iibptr     Pointer to the jumper in the data structure
*		bstr
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*     Builds the horizontal picture of the specified.
*
****+++***********************************************************************/

static void tripole_jumper_pic(jmp, bstr)
    struct jumper 	*jmp;
    char		*bstr;
{
    int 		i;
    unsigned long   	bit_mask = 1;


    (void)strcat(bstr, table_ptr[JUMPER29]);
    for (i = 1; i <= jmp->width; i++)
	if (i < jmp->width)
	    (void)strcat(bstr, table_ptr[JUMPER30]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER31]);
    (void)strcat(bstr, table_ptr[JUMPER32]);

    (void)strcat(bstr, table_ptr[JUMPER29]);
    bit_mask = 1;
    bit_mask = bit_mask << (jmp->width - 1);
    for (i = 1; i <= jmp->width; i++) {
	if (bit_mask & jmp->config_bits.current) {
	    (void)strcat(bstr, table_ptr[JUMPER33]);
	    if (i < jmp->width)
		(void)strcat(bstr, space3);
	}
	else {
	    if (i < jmp->width)
		(void)strcat(bstr, table_ptr[JUMPER37]);
	    else
		(void)strcat(bstr, table_ptr[JUMPER34]);
	}
	bit_mask = bit_mask >> 1;
    }
    (void)strcat(bstr, table_ptr[JUMPER38]);

    /* draw a row of posts */
    (void)strcat(bstr, table_ptr[JUMPER29]);
    for (i = 1; i <= jmp->width; i++)
	if (i < jmp->width)
	    (void)strcat(bstr, table_ptr[JUMPER30]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER31]);
    (void)strcat(bstr, table_ptr[JUMPER32]);

    bit_mask = 1;
    bit_mask = bit_mask << (jmp->width - 1);
    (void)strcat(bstr, table_ptr[JUMPER29]);
    for (i = 1; i <= jmp->width; i++) {

	/* check for no connection */
	if (bit_mask & jmp->tristate_bits.current) {
	    if (i < jmp->width)
		(void)strcat(bstr, table_ptr[JUMPER37]);
	    else
		(void)strcat(bstr, table_ptr[JUMPER34]);
	}

	else {
	    if (bit_mask & jmp->config_bits.current) {
		if (i < jmp->width)
		    (void)strcat(bstr, table_ptr[JUMPER37]);
		else
		    (void)strcat(bstr, table_ptr[JUMPER34]);
	    }
	    else {
		(void)strcat(bstr, table_ptr[JUMPER33]);
		if (i < jmp->width)
		    (void)strcat(bstr, space3);
	    }
	}

	bit_mask = bit_mask >> 1;

    }
    (void)strcat(bstr, table_ptr[JUMPER38]);

    (void)strcat(bstr, table_ptr[JUMPER29]);
    for (i = 1; i <= jmp->width; i++) {
	if (i < jmp->width)
	    (void)strcat(bstr, table_ptr[JUMPER30]);
	else
	    (void)strcat(bstr, table_ptr[JUMPER31]);
    }
    (void)strcat(bstr, table_ptr[JUMPER32]);

}


/****+++***********************************************************************
*
* Function:     sw_put_text()
*
* Parameters:   left_margin    left margin of the window to display in
*	        text	       starting location of the text to format
*		return_txt     string to add to
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*     Format text to end of a current string.
*
****+++***********************************************************************/

static void sw_put_text (left_margin, text, return_txt)
    int 			left_margin;
    char 			*text;
    char 			*return_txt;
{
    char 			*next_text;
    char 			outbuf[MAXCOL+2];
    int 			i;
    int 			line;
#define MAXLINELEN 		70
#define MAXNUMLINES		20


    next_text = text;

    /* set left margins */
    for (i=0 ; i<left_margin ; i++)
        (void)strcat(return_txt, " ");

    /* format lines and add them to return_txt until eof */
    for (line=1 ; line<=MAXNUMLINES  ; ++line, ++next_text)  {
        pr_break_and_buffer_line(&next_text, MAXLINELEN, outbuf);
        (void)strcat(return_txt, outbuf);
        (void)strcat(return_txt, "\n");
        for (i=0 ; i<left_margin ; i++)
	    (void)strcat(return_txt, " ");
        if (!*next_text)
	    break;
    }

}






/****+++***********************************************************************
*
* Function:     sw_print_buffer
*
* Parameters:   title		string with a title
*		body		string with switch body
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*     Display a switch picture and release the memory it consumed.
*
*     NOTE: should do a "more" functionality here!
*
****+++***********************************************************************/

static void sw_print_buffer(title, body)
    char 	*title;
    char 	*body;
{

    pr_put_text(title);
    pr_put_text(body);

    mn_trapfree((void *)title);
    mn_trapfree((void *)body);
	 
}
