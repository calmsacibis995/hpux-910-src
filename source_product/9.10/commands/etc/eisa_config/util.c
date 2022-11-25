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
*                          src/util.c
*
*
*  This file contains various utility functions.
*	
*	mn_trapcalloc()		-- everybody
*	mn_trapfree()		-- everybody
*	mn_traprealloc()	-- everybody
*
*	find_avail_slots()	-- main.c, show.c
*	get_this_board()	-- help.c, main.c, show.c
*	slot_ok()		-- add.c
*	slot_type_ok()		-- add.c
*
*       strupr
*	strlower
*	strrev
*	strcmpi
*	strncmpi
*
**++***************************************************************************/


#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "err.h"


/* Globals declared in globals.c */
extern struct pmode	program_mode;
extern struct system 	glb_system;


/* Functions used here but declared elsewhere */
extern  void  		del_release_system();


/* Functions declared in this file */
void			*mn_trapcalloc();
void			mn_trapfree();
void			*mn_traprealloc();
int			find_avail_slots();
struct board 		*get_this_board();
int 			slot_type_ok();
int 			slot_ok();
char			*strupr();
char			*strlower();
char			*strrev();
int			strcmpi();
int			strncmpi();


/* Data structures for tracking memory allocations */
#ifdef DEBUG
#define MAX_MALLOC_CALLS	2000
struct	DB_malloc_template {
    unsigned	count;
    void	*pointer[MAX_MALLOC_CALLS];
};
struct	DB_malloc_template	DB_malloc = { 0, 0 };
#endif


/****+++***********************************************************************
*
* Function:     mn_trapcalloc
*
* Parameters:   count		number of elements to allocate
*		amount		size of each element
*
* Used:		external only
*
* Returns:      the ptr to the memory buffer allocated
*
* Description:
*
*	This routine attempts to allocate the amount of memory
*	requested. If successful, the pointer is returned. If not,
*	a error message is displayed before exiting the program.
*
****+++***********************************************************************/

void *mn_trapcalloc (cnt, amount)
    unsigned	cnt;
    unsigned	amount;
{
    void	*buffer;
#ifdef DEBUG
    void	**ptr;
#endif


    /* If the allocation fails then report it and exit.	*/
    buffer = calloc(cnt, amount);
    if (buffer == NULL)  {
	err_handler(NO_MORE_MEMORY_ERRCODE);
	del_release_system(&glb_system);
	exit(EXIT_RESOURCE_ERR);
    }

#ifdef DEBUG
    /* If the maximum number of tracked malloc's has been exceeded  */
    /* then exit with the appropriate error message. */
    if (++(DB_malloc.count) == MAX_MALLOC_CALLS) {
	err_handler(DEBUG_ALLOC_FAILS_ERRCODE);
	del_release_system(&glb_system);
	exit(EXIT_DBG_RESOURCE_ERR);
    }

    /* Find an empty spot in the list of pointers and save the	*/
    /* buffer's address for cross checking when it is free'd.       */
    ptr = DB_malloc.pointer;
    while (*ptr++ != NULL);
    *(--ptr) = buffer;
#endif

    return(buffer);
}


/****+++***********************************************************************
*
* Function:     mn_trapfree()
*
* Parameters:   address		the address of the memory buffer to be freed
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    This routine frees up the specified buffer.
*
****+++***********************************************************************/

void mn_trapfree(address)
    void	*address;
{
#ifdef DEBUG
    void	**ptr;
#endif


    /* If the address is NULL then just return.	*/
    if (address == NULL)
	return;

#ifdef DEBUG
    /* The address is checked against the list of pointers. */
    ptr = DB_malloc.pointer;
    while (*ptr != address && ptr != (DB_malloc.pointer + MAX_MALLOC_CALLS))
	ptr++;

    /* If the address is not matched then return without exiting.	*/
    if (*ptr != address) {
	err_handler(DEBUG_FREE_FAILS_ERRCODE);
	del_release_system(&glb_system);
	exit(EXIT_DBG_RESOURCE_ERR);
    }
    
    *ptr = NULL;
    DB_malloc.count--;
#endif

    free(address);
    return;
}


/****+++***********************************************************************
*
* Function:     mn_traprealloc()
*
* Parameters:   address			buffer to be freed
*		amount			of memory to be allocated
*
* Used:		external only
*
* Returns:      ptr to the new memory buffer
*
* Description:
*
*	This routine attempts to reallocate the amount of memory
*	requested. If successful, the pointer is returned. If not,
*	a error message is displayed before exiting the program.
*
****+++***********************************************************************/

void *mn_traprealloc(address, amount)
    void 	*address;
    unsigned	amount;
{
    void	*buffer;
#ifdef DEBUG
    void	**ptr;
#endif


#ifdef DEBUG
    /* The address is checked against the list of pointers. */
    ptr = DB_malloc.pointer;
    while (*ptr != address && ptr != (DB_malloc.pointer + MAX_MALLOC_CALLS))
	ptr++;

    /* If the address is not matched then exit.	*/
    if (*ptr != address) {
	err_handler(DEBUG_ALLOC_FAILS_ERRCODE);
	del_release_system(&glb_system);
	exit(EXIT_DBG_RESOURCE_ERR);
    }

    *ptr = NULL;
#endif

    /* If the allocation fails then report it and exit.	*/
    buffer = realloc(address, amount);
    if (buffer == NULL) {
	err_handler(NO_MORE_MEMORY_ERRCODE);
	del_release_system(&glb_system);
	exit(EXIT_RESOURCE_ERR);
    }

#ifdef DEBUG
    /* Find an empty spot in the list of pointers and save the	*/
    /* buffer's address for cross checking when it is free'd.       */
    ptr = DB_malloc.pointer;
    while (*ptr++ != NULL);
    *(--ptr) = buffer;
#endif

    return(buffer);
}


/****+++***********************************************************************
*
* Function:     slot_type_ok()
*
* Parameters:   brd_type	type of board (isa8, eisa, etc.) to put in slot
*		sys_slot	slot we want to put board in
*
* Used:		internal and external
*
* Returns:      TRUE		board and slot match
*		FALSE		board does not match slot
*
* Description:
*
*  This function figures out if a board's type matches the type of a
*  particular slot.
*
*  The following combinations are allowed:
*	o  the board is embedded
*       o  the slot is ISA8 and the board is ISA8 or ISA8OR16
*       o  the slot is ISA16 and the board is ISA8 or ISA16 or ISA8OR16
*       o  the slot is EISA and the board is ISA8 or ISA16 or ISA8OR16 or EISA
*       o  the slot is OTHER and the board is OTHER
*
****+++***********************************************************************/

int slot_type_ok(brd_type, sys_slot)
    enum board_slot 	brd_type;
    struct slot		*sys_slot;
{


#ifdef VIRT_BOARD
    if (brd_type == bs_emb)
	return(TRUE);
#endif

    switch (sys_slot->type) {

	case st_isa8:
	    if ( (brd_type != bs_isa8)  &&
	         (brd_type != bs_isa8or16) )
		return(FALSE);
	    break;

	case st_isa16:
	    if ( (brd_type != bs_isa8)  &&
	         (brd_type != bs_isa16) && 
		 (brd_type != bs_isa8or16) )
		return(FALSE);
	    break;

	case st_eisa:
	    if ( (brd_type != bs_isa8)     &&
	         (brd_type != bs_isa16)    && 
		 (brd_type != bs_isa8or16) &&
		 (brd_type != bs_eisa) )
		return(FALSE);
	    break;

	case st_other:
	    if (brd_type != bs_oth)
		return(FALSE);
	    break;

    }

    return(TRUE);
}


/****+++***********************************************************************
*
* Function:     slot_ok()
*
* Parameters:	brd		board to test for
*		sys_slot	slot to try this board in
*
* Used:		internal and external
*
* Returns:	TRUE if this board will work in this slot (with the
*			possible exception of a type conflict)
*		FALSE if this board cannot go in this slot
*
* Description:
*
*    This function figures out whether a given board can be used in a given
*    slot. (Note that type conflicts are checked for by slot_type_ok()).
*
*    The following conditions will disallow this board in this slot:
*	o  the slot is not present (and the board is not embedded)
*	o  the slot is already occupied
*	o  the board has a skirt but the the slot does not allow skirts
*	o  the board is a busmaster, but the slot does not support busmasters
*	o  the board's length is larger than the slot supports
*	o  the board has a tag and the slot does not have a matching tag
*
*    Otherwise, the board is allowed in this slot.
*
****+++***********************************************************************/

int slot_ok(brd, sys_slot)
    struct board 	*brd;
    struct slot 	*sys_slot;
{
    struct tag 		*tag;


    if (!sys_slot->present &&  (brd->slot != bs_emb)) 
	return(FALSE);

    if (sys_slot->occupied) 
	return(FALSE);

    if (brd->skirt)
	if (!sys_slot->skirt  || 
	     (brd->slot == bs_isa8 && sys_slot->type != st_isa8))
	    return(FALSE);

    if (brd->busmaster && !sys_slot->busmaster) 
	return(FALSE);

    if (sys_slot->length)
	if (brd->length > sys_slot->length)
	    return(FALSE);

    if (brd->slot_tag) {

	for (tag = sys_slot->tags; tag; tag = tag->next)
	    if (strcmpi(tag->string, brd->slot_tag) == 0)
		return(TRUE);

	return(FALSE);
    }

    return(TRUE);
}


/****+++***********************************************************************
*
* Function:     find_avail_slots()
*
* Parameters:	sys		the system data structure
*		brd		the board to be added
*		avail_slots	the slots which are available for this board
*				in priority order (the return value is the
*				number of valid elements of this array)
*
* Used:		external only
*
* Returns:      -1		the board passed in needs only a virtual slot
*		-2		the board passed in needs an embedded slot
*		other		the number of slots available
*
* Description:
*
*	This function builds a list of slots which are available for the
*	board that is passed in.
*
****+++***********************************************************************/

int find_avail_slots(sys, brd, avail_slots)
    struct system 	*sys;
    struct board 	*brd;
    int			avail_slots[];
{
    int			slot_count=0;
    int			slot_ndx;
    int			i;
    enum board_slot	brd_type;


    /**************
    * Leave immediately if this is a virtual or embedded board.
    **************/
    if (brd->slot == bs_vir)
	return(-1);
    if (brd->slot == bs_emb)
	return(-2);

    /*************
    * Walk through each of the sorted slots. If the slot and slot_type are
    * ok for this board, add this slot to our list of possibilities. There
    * is one twist here. If the board is ISA8OR16, treat it as if it were
    * ISA16 instead. This has the effect of excluding slots that are ISA8
    * when the board is ISA8OR16. The next clause also deals with this.
    *************/
    if (brd->slot == bs_isa8or16)
	brd_type = bs_isa16;
    else
	brd_type = brd->slot;
    for (i = 0; i < MAX_NUM_SLOTS; i++) {
	slot_ndx = sys->sorted[i].slot;
	if ( (slot_ok(brd, &(sys->slot[slot_ndx])))  &&
	     (slot_type_ok(brd_type, &(sys->slot[slot_ndx]))) )
	    avail_slots[slot_count++] = slot_ndx;
    }

    /****************
    * If the board is ISA8OR16, go back and add the slots that were  
    * excluded in the previous clause (if any) to the end of the
    * prioritized list. This will have the effect of making ISA8 slots
    * the least desirable when the board is ISA8OR16.
    *****************/
    if (brd->slot == bs_isa8or16)
	for (i = 0; i < MAX_NUM_SLOTS; i++) {
	    slot_ndx = sys->sorted[i].slot;
	    if ( (slot_ok(brd, &(sys->slot[slot_ndx])))  &&
		 (sys->slot[slot_ndx].type == st_isa8) )
		avail_slots[slot_count++] = slot_ndx;
	}

    /****************
    * Return the number of slots that were found.
    ****************/
    return(slot_count);

}


/****+++***********************************************************************
*
* Function:     get_this_board()
*
* Parameters:   slotnum		slot number to find board for
*
* Used:		internal and external
*
* Returns:      NULL		no board in this slotnum
*		other		ptr to a board struct in this slotnum
*
* Description:
*
*	This function returns a pointer to the board struct associated
*	with a given slot number (if any). If the slot is empty or
*       out of range, the message will be displayed here.
*
****+++***********************************************************************/

struct board *get_this_board(slotnum)
    int			slotnum;
{
    struct board 	*board;


    board = glb_system.boards;
    while (board != NULL) {
	if (board->slot_number == slotnum)
	    break;
	board = board->next;
    }

    if (board == NULL) {
	err_add_num_parm(1, (unsigned)slotnum);
	if (slotnum >= MAX_NUM_SLOTS) 
	    err_handler(NONEXISTENT_SLOT_ERRCODE);
	else if (glb_system.slot[slotnum].present)
	    err_handler(EMPTY_SLOT_ERRCODE);
	else
	    err_handler(NONEXISTENT_SLOT_ERRCODE);
    }
    return(board);

}


/****+++***********************************************************************
*
* Function:     strupr
*
* Parameters:   str		the string to modify
*
* Used:		external only
*
* Returns:      the modified string
*
* Description:
*
*     Convert each lower-case character in str to its upper-case
*     equivalent. Convert the input parameter. Also return a pointer to the
*     modified string.
*
****+++***********************************************************************/

char * strupr(str)
    char	*str;
{
    int		len;
    int		i;

    len = strlen(str);
    for (i=0 ; i<len ; i++)
	str[i] = toupper(str[i]);
    return(str);
}




/****+++***********************************************************************
*
* Function:     strlower
*
* Parameters:   str		the string to modify
*
* Used:		external only
*
* Returns:      the modified string
*
* Description:
*
*     Convert each upper-case character in str to its lower-case
*     equivalent. Convert the input parameter. Also return a pointer to the
*     modified string.
*
****+++***********************************************************************/

char *strlower(str)
    char	*str;
{
    int		len;
    int		i;

    len = strlen(str);
    for (i=0 ; i<len ; i++)
	str[i] = tolower(str[i]);
    return(str);
}


/****+++***********************************************************************
*
* Function:     strrev
*
* Parameters:   str		the string to modify
*
* Used:		external only
*
* Returns:      the modified string
*
* Description:
*
*      Take a string and reverse it. Return the reversed string. The original 
*      string is also modified. The string must be non-null on entrance.
*
****+++***********************************************************************/

char * strrev(str)
    char	*str;
{
    int		len;
    int		i,j;
    char	new_str[256];

    len = strlen(str);
    for (i=len-1,j=0 ; i>=0 ; i--,j++)
	new_str[j] = str[i];
    new_str[len] = 0;

    (void)strcpy(str, new_str);
    return(str);
}




/****+++***********************************************************************
*
* Function:     strcmpi
*
* Parameters:   str1		the first string to compare
*               str2		the second string to compare
*
* Used:		external only
*
* Returns:      0		the 2 strings are equal
*		<0		str1 is less than str2
*		>0		str1 is greater than str2
*
* Description:
*
*     This function is just like strcmp() except it is case-insensitive. The
*     return values are just like for strcmp().
*
****+++***********************************************************************/

int strcmpi (str1, str2)
    char	*str1;
    char	*str2;
{
    char	new_str1[256];
    char	new_str2[256];


    (void)strcpy(new_str1, str1);
    (void)strcpy(new_str2, str2);

    (void)strupr(new_str1);
    (void)strupr(new_str2);

    return(strcmp(new_str1, new_str2));
}


/****+++***********************************************************************
*
* Function:     strncmpi
*
* Parameters:   str1		the first string to compare
*               str2		the second string to compare
*		ct		number of chars to compare
*
* Used:		external only
*
* Returns:      0		the 2 strings are equal
*		<0		str1 is less than str2
*		>0		str1 is greater than str2
*
* Description:
*
*     This function is just like strncmp() except it is case-insensitive. The
*     return values are just like for strncmp().
*
****+++***********************************************************************/

int strncmpi (str1, str2, ct)
    char	*str1;
    char	*str2;
    unsigned    ct;
{
    char	new_str1[256];
    char	new_str2[256];


    (void)strcpy(new_str1, str1);
    (void)strcpy(new_str2, str2);

    (void)strupr(new_str1);
    (void)strupr(new_str2);

    return(strncmp(new_str1, new_str2, ct));
}
