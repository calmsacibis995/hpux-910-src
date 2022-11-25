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
*                             src/nvmsave.c
*
*      This module contains the high level routines for saving the current
*      system configuration to EISA eeprom or an sci file.
*
*	nvm_save_board()	-- open_save.c
*	nvm_init_size()		-- nvmlowlevel.c
*
**++***************************************************************************/


#include <stdio.h>
#include <memory.h>
#include "config.h"
#include "nvm.h"
#include "err.h"


/* functions declared in this file */
int		nvm_save_board();
int 		nvm_init_size();
static	int 	nvm_write_empty_slot();
static	int 	nvm_build_board();
static	void 	nvm_append_to_board();
static	int 	nvm_build_subfunction();
static	int 	nvm_build_resources();
static	int 	nvm_build_irq();
static	int 	nvm_build_dma();
static	int 	nvm_build_port();
static	int 	nvm_build_memory();
static	void	nvm_build_init();
static	int 	nvm_ioport_size();
static	void 	nvm_add_selection();
static	void 	nvm_append_type();
static	void 	nvm_build_dupid();
static  void    nvm_build_id();
static  int	hex2bin();


/* functions used here but declared elsewhere */
extern  int     nvm_write_slot();
extern  int  	hex2bin();
extern  void    *mn_trapcalloc();
extern  void    mn_trapfree();


/* global to this file only */
static unsigned int 	nvm_slot_number;


/* resource errors */
#define TOO_MANY_IRQS		1
#define TOO_MANY_DMAS		2
#define TOO_MANY_PORTS		3
#define TOO_MANY_MEMORYS	4
#define TOO_MANY_INITS		5


/****+++***********************************************************************
*
* Function:     nvm_save_board
*
* Parameters:   board		ptr to board data structure to save
*		slot_number	slot number to write it to
*		target		where to write to - NVM_EEPROM or NVM_SCI
*		ignore_too_many_resources  should "too many resources" messages
*					  be printed?
*
* Used:		external only
*
* Returns:      NVM_SUCCESSFUL	  successful (or resource overrun)
*   		NVM_WRITE_ERROR	  failure to write to eeprom or sci file
*
* Description:
*
*    The nvm_save_board function will build the information block for a
*    given board and save it either to eeprom or to an sci file.
*
*    If the slot is empty, we still need to write a record with a
*    slot_id = -1.
*
*    If the board in question is invalid beacuse it uses too many of the
*    various types of EISA resources, we will display a warning message and
*    write an empty slot. NVM_SUCCESSFUL will be returned in this case.
*    If the ignore_too_many_resources flag is set, we will not display
*    the warning message. This can happen when the eeprom has already been
*    written and now we are trying the sci file -- no need to tell the user
*    twice!
*
****+++***********************************************************************/

nvm_save_board(board, slot_number, target, ignore_too_many_resources)
    struct board 	*board;
    unsigned int	slot_number;
    unsigned int	target;
    int			ignore_too_many_resources;
{
    unsigned char   	*buffer;	    	/* nvm information */
    unsigned int    	length;	    		/* bytes in buffer	*/
    unsigned int    	err;
    unsigned char    	*p;
    struct ioport 	*ioport_init;


    /***************
    * Allocate buffer space. Save the slot number away in a global and
    * handle an empty slot (if necessary).
    ***************/
    buffer = (unsigned char *)mn_trapcalloc(2048, 1);
    nvm_slot_number = slot_number;
    if (board == NULL)
	err = nvm_write_empty_slot(buffer, target);

    /*******************
    * Build the data structure for this board. If the build was successful,
    * we need to add 4 bytes onto the end of the record (after resizing).
    * The first two bytes are a function length of 0 (a marker). The second
    * two bytes are the checksum for the cfg file corresponding to this board.
    * Note that the checksum is LSB followed by MSB.
    *******************/
    else {
	ioport_init = board->ioports;
	err = nvm_build_board(board, buffer, &length, ioport_init);
	if (err == 0) {
	    p = (unsigned char *)(buffer + length);
	    *p++ = 0;
	    *p++ = 0;
	    *p++ = board->checksum & 0xff;
	    *p = (board->checksum >> 8) & 0xff;
	    length += 4;
	    err = nvm_write_slot(slot_number, buffer, length, target);
	}

	/***************
	* The build was a failure because the board needed more resources of a 
	* given type than is permissible. Display a warning message (unless
	* we have been told not to), and write out an empty slot. Return
	* successful.
	***************/
	else {
	    if (ignore_too_many_resources == 0) {
		err_add_num_parm(1, slot_number);
		switch (err) {
		    case TOO_MANY_IRQS:
			err_handler(TOO_MANY_IRQS_ERRCODE);
			break;
		    case TOO_MANY_DMAS:
			err_handler(TOO_MANY_DMAS_ERRCODE);
			break;
		    case TOO_MANY_PORTS:
			err_handler(TOO_MANY_PORTS_ERRCODE);
			break;
		    case TOO_MANY_MEMORYS:
			err_handler(TOO_MANY_MEMORYS_ERRCODE);
			break;
		    case TOO_MANY_INITS:
			err_handler(TOO_MANY_INITS_ERRCODE);
			break;
		}
	    }
	    err = nvm_write_empty_slot(buffer, target);
	}
    }

    mn_trapfree((void *)buffer);

    return(err);
}


/****+++***********************************************************************
*
* Function:     nvm_write_empty_slot()
*
* Parameters:   buffer		buffer to fill in before writing to nvm/sci
*		target		where to write to - NVM_EEPROM or NVM_SCI
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	  successful
*   		NVM_WRITE_ERROR	  failure to write to eeprom or sci file
*
* Description:
*
*    This function builds a slot record for an empty slot and writes it
*    to the nvm or sci for the slot specified by the global nvm_slot_number.
*
****+++***********************************************************************/

static int nvm_write_empty_slot(buffer, target)
    unsigned char	*buffer;
    unsigned int	target;
{
    unsigned int    	err;


    /*******************
    * Write a slot id of -1 to tell the kernel that the slot is empty.
    * We need 12 bytes here:
    *       board id             -- 4 bytes
    *       dupid/ext rev        -- 4 bytes
    *       function length (0)  -- 2 bytes
    *       checksum             -- 2 bytes
    *******************/
    buffer[0] = 0xff;
    buffer[1] = 0xff;
    buffer[2] = 0xff;
    buffer[3] = 0xff;
    buffer[8] = 0;
    buffer[9] = 0;
    err = nvm_write_slot(nvm_slot_number, buffer, 12, target);

    return(err);
}


/****+++***********************************************************************
*
* Function:     nvm_build_board
*
* Parameters:   board		ptr to a board data structure (non-NULL)
*		buffer		buffer to fill up (target)
*		length		length of the buffer
*		ioport_init	ptr to the board's ioport struct
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	     successful
*   		TOO_MANY_IRQS	     too many irqs were needed for this board
*   		TOO_MANY_DMAS	     too many dmas were needed for this board
*   		TOO_MANY_PORTS	     too many ports were needed for this board
*   		TOO_MANY_MEMORYS     too many memorys were needed for this board
*
* Description:
*
*   The nvm_build_board function builds a nvm information block from
*   the data in a board structure.
*
****+++***********************************************************************/

static nvm_build_board(board, buffer, length, ioport_init)
    struct board 	*board;
    unsigned char	*buffer;
    unsigned int	*length;
    struct ioport 	*ioport_init;
{
    int 		err;
    unsigned int    	count;
    unsigned char   	selections[NVM_MAX_SELECTIONS + 1];
    unsigned char	type[NVM_MAX_TYPE + 1];
    unsigned char	data[NVM_MAX_DATA + 1];
    nvm_memory		memory[NVM_MAX_MEMORY];
    nvm_irq		irq[NVM_MAX_IRQ];
    nvm_dma		dma[NVM_MAX_DMA];
    nvm_port		port[NVM_MAX_PORT];
    nvm_init		inits[NVM_MAX_INIT/4];
    nvm_fib		fib;
    struct group 	*group;
    struct function 	*function;
    struct subfunction 	*subf;


    /*****************
    * This is not an empty slot. Set the length for the static
    * header and length field.
    *****************/
    *length = 8;

    /*****************
    * Build the header section of the slot information:
    *	- the board id information
    *	- the duplicate id information
    *   - the cfg file extension rev level (if the board is ISA, set the 
    *     lower bit of the first byte, otherwise make it 0)
    *****************/
    nvm_build_id((unsigned char *)board->id, buffer);
    nvm_build_dupid(board, (nvm_dupid *)((buffer) + 4));
    if ( (board->slot == bs_isa8) || (board->slot == bs_isa16) ||
	 (board->slot == bs_isa8or16) )
	(void)memset((void *)(buffer + 6), 1, 1);
    else
	(void)memset((void *)(buffer + 6), 0, 1);
    (void)memset((void *)(buffer + 7), 0, 1);

    /*****************
    * Traverse the function tree building the nvm information for
    * each group/function/subfunction.
    *****************/
    err = 0;
    for (group=board->groups ; group ; group=group->next)

	for (function=group->functions ; function ; function=function->next) {

	    /*****************
	    * We are starting a new function. Clean up
	    * all the nvm information blocks.
	    *****************/
	    (void)memset((void *)type, 0, sizeof(type));
	    (void)memset((void *)selections, 0, sizeof(selections));
	    (void)memset((void *)memory, 0, sizeof(memory));
	    (void)memset((void *)irq, 0, sizeof(irq));
	    (void)memset((void *)dma, 0, sizeof(dma));
	    (void)memset((void *)port, 0, sizeof(port));
	    (void)memset((void *)inits, 0, sizeof(inits));
	    (void)memset((void *)data, 0, sizeof(data));
	    *(unsigned char *) & fib = 0;

	    /******************
	    * Build the function type field.
	    ******************/
	    nvm_append_type(type, group->type, "");
	    nvm_append_type(type, function->type, ",");

	    /*******************
	    * Walk through all of the board's initialization records and
	    * stuff them into our local inits buffer. If we have hit the
	    * maximum number of bytes allowed for inits (60), stop writing them
	    * and exit with an error.
	    *******************/
	    if (function->subfunctions->current->freeform == NULL)
		for (count=0 ; ioport_init; ) {
		    if (ioport_init->referenced && ioport_init->valid_address) {
			count += nvm_ioport_size(ioport_init);
			if (count <= 60)
			    nvm_build_init((nvm_init *)inits, &fib,ioport_init);
			else
			    return(TOO_MANY_INITS);
		    }
		    ioport_init = ioport_init->next;
		}

	    /*****************
	    * For each of the function's subfunctions, record what they
	    * use in our data structures.
	    *****************/
	    for (subf=function->subfunctions ; subf ; subf=subf->next) {
		nvm_append_type(type, subf->type, ",");
		err = nvm_build_subfunction(subf, &fib, selections, type,
					      memory, irq, dma, port, data);
		if (err != 0)
		    return(err);
	    }

	    /*****************
	    * Append the function information to other functions
	    * already processed for this board.
	    *****************/
	    nvm_append_to_board(buffer, length, &fib, selections, type, memory,
				irq, dma, port, (nvm_init *)inits, data);

	}

    return(NVM_SUCCESSFUL);
}


/****+++***********************************************************************
*
* Function:     nvm_append_to_board
*
* Parameters:   buffer		Current board info
*   		length		Length of block
*   		fib	    	New function info
*   		selections   	New choice table
*   		type		New type string
*   		memory    	New memory block
*   		irq	    	New IRQ block
*   		dma	    	New DMA block
*   		port	    	New PORT block
*   		init	    	New INIT block
*   		data		Free Form data
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_append_to_board function will append the function block that
*   consists of the fib, choices, type, memory, irq, dma, port and
*   inits to the current board info.
*
****+++***********************************************************************/

static void nvm_append_to_board(buffer, length, fib, selections, type, memory,
			        irq, dma, port, init, data)
    unsigned char	*buffer;
    unsigned int	*length;
    nvm_fib	    	*fib;
    unsigned char	*selections;
    unsigned char	*type;
    nvm_memory  	*memory;
    nvm_irq	    	*irq;
    nvm_dma	    	*dma;
    nvm_port    	*port;
    nvm_init    	*init;
    unsigned char	*data;
{
    unsigned short int 	func_len;
    unsigned		init_len;
    nvm_irq	 	*old_irq;
    nvm_dma	 	*old_dma;
    nvm_memory	 	*old_memory;
    nvm_port	 	*old_port;
    nvm_init	 	*old_init;
    unsigned char   	*new_info;
    unsigned char   	*len_ptr;


    /****************
    * If there is a type field, set the fib bit.
    ****************/
    if (type[0])
	fib->type = 1;

    /****************
    * Save away where we will stuff the function length. Also, get the
    * first address for use to start copying function data.
    ****************/
    len_ptr = buffer + *length;
    new_info = len_ptr + 2;

    /****************
    * Stuff the selections array and the fib byte into the buffer.
    ****************/
    (void)memcpy((void *)new_info, (void *)selections, (size_t)(selections[0]+1));
    new_info += selections[0] + 1;
    (void)memcpy((void *)new_info, (void *)fib, 1);
    new_info++;

    /***************
    * If there is a type string, add it into the buffer.
    ***************/
    if (fib->type) {
	(void)memcpy((void *)new_info, (void *)type, (size_t)(type[0] + 1));
	new_info += type[0] + 1;
    }

    /***************
    * If we have freeform data, stuff it into the buffer. Otherwise, stuff
    * each of the resource blocks into the buffer.
    ***************/
    if (fib->data) {
	(void)memcpy((void *)new_info, (void *)data, (size_t)(data[0] + 1));
	new_info += data[0] + 1;
    }
    else {
	if (fib->memory)  {
	    do {
		old_memory = memory;
		(void)memcpy((void *)new_info, (void *)memory, NVM_MEMORY_SIZE);
		new_info += NVM_MEMORY_SIZE;
		memory++;
	    } while (old_memory->more);
	}
	if (fib->irq)  {
	    do {
		old_irq = irq;
		(void)memcpy((void *)new_info, (void *)irq, NVM_IRQ_SIZE);
		new_info += NVM_IRQ_SIZE;
		irq++;
	    } while (old_irq->more);
	}
	if (fib->dma)  {
	    do {
		old_dma = dma;
		(void)memcpy((void *)new_info, (void *)dma, NVM_DMA_SIZE);
		new_info += NVM_DMA_SIZE;
		dma++;
	    } while (old_dma->more);
	}
	if (fib->port)  {
	    do {
		old_port = port;
		(void)memcpy((void *)new_info, (void *)port, NVM_PORT_SIZE);
		new_info += NVM_PORT_SIZE;
		port++;
	    } while (old_port->more);
	}
	if (fib->init)  {
	    do {
		old_init = init;
		init_len = nvm_init_size(init);
		(void)memcpy((void *)new_info, (void *)init, init_len);
		new_info += init_len;
		init++;
	    } while (old_init->more);
	}
    }

    /***************
    * Calculate the function length and stuff it into the buffer.
    * The length is the size of the entire buffer minus the length it
    * started at minus the size of the function length field (2).
    ***************/
    func_len = new_info - buffer - *length - 2;
    *len_ptr++ = func_len & 0xff;
    *len_ptr = (func_len >> 8) & 0xff;

    /**************
    * Now set the new length of the buffer.
    **************/
    *length = new_info - buffer;

}


/****+++***********************************************************************
*
* Function:     nvm_build_subfunction
*
* Parameters:   subfunction    	Subfunction data
*               fib 	    	Current function info
*   		selections      Current choice table
*   		type 		Current type string
*   		memory     	Current memory block
*   		irq 	    	Current IRQ block
*   		dma 	    	Current DMA block
*   		port 	    	Current PORT block
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	     successful
*   		TOO_MANY_IRQS	     too many irqs were needed for this board
*   		TOO_MANY_DMAS	     too many dmas were needed for this board
*   		TOO_MANY_PORTS	     too many ports were needed for this board
*   		TOO_MANY_MEMORYS     too many memorys were needed for this board
*
* Description:
*
*   The nvm_build_subfunction function builds the nvm information
*   block for the subfunction passed in the appropriate buffers (fib,
*   selections, type, memory, irq, dma, and port).
*
****+++***********************************************************************/

static nvm_build_subfunction(subfunction, fib, selections, type, memory, irq,
			     dma, port, data)
    struct subfunction 		*subfunction;
    nvm_fib	    		*fib;
    unsigned char		*selections;
    unsigned char		*type;
    nvm_memory  		*memory;
    nvm_irq	    		*irq;
    nvm_dma	    		*dma;
    nvm_port    		*port;
    unsigned char		*data;
{
    struct choice 		*choice;
    struct subchoice 		*subchoice;
    unsigned char   		*freeform;
    int 			err;
    unsigned char 		count;


    nvm_add_selection(selections, (unsigned)subfunction->index.current, 1);

    /*****************
    * If this choice contains any type and resource information,
    * Process them now before the subchoices.
    *****************/
    err = 0;
    choice = subfunction->current;
    fib->disable |= choice->disable;
    nvm_append_type(type, choice->subtype, ";");

    /*****************
    * If there is any free form data, copy it to the data buffer and
    * move the data pointer.
    *****************/
    freeform = choice->freeform;
    if (freeform) {
	fib->data = 1;
	count = NVM_MAX_DATA - *data;
	if (*freeform < count)
	    count = *freeform;
	(void)memcpy((void *)(data + *data + 1), (void *)(freeform + 1), (size_t)count);
	*data += count;
    }

    /*****************
    * Fill in the resources structures for the primary subchoice.
    * **NOTE** Does this fill in some of the structures that the next call
    *          to the same function doesn't get? Why do we do this twice?
    *****************/
    subchoice = choice->primary;
    if (subchoice)
	err = nvm_build_resources(subchoice->resource_groups, fib, selections,
				    memory, irq, dma, port);

    /*****************
    * Now process the currently selected subchoice if subchoices
    * actually exist.
    *****************/
    subchoice = choice->current;
    if ( (err == 0) && ((int)subchoice != 0) ) { 
	nvm_add_selection(selections, (unsigned)choice->index.current, 1);
	err = nvm_build_resources(subchoice->resource_groups, fib, selections,
				    memory, irq, dma, port);
    }

    return(err);
}


/****+++***********************************************************************
*
* Function:     nvm_build_resources
*
* Parameters:   resgrp   	Resource group
*   		fib 	    	Current function info
*   		selections      Current selections table
*   		memory     	Current memory block
*   		irq 	    	Current IRQ block
*   		dma 	    	Current DMA block
*   		port 	    	Current PORT block
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	     successful
*   		TOO_MANY_IRQS	     too many irqs were needed for this board
*   		TOO_MANY_DMAS	     too many dmas were needed for this board
*   		TOO_MANY_PORTS	     too many ports were needed for this board
*   		TOO_MANY_MEMORYS     too many memorys were needed for this board
*
* Description:
*
*   The nvm_build_resources function builds the nvm information for the
*   resource group passed to it in the appropriate structures (fib,
*   selections, memory, irq, dma, and port).
*
****+++***********************************************************************/

static nvm_build_resources(resgrp, fib, selections, memory, irq, dma, port)
    struct resource_group 	*resgrp;
    nvm_fib	    		*fib;
    unsigned char		*selections;
    nvm_memory  		*memory;
    nvm_irq	    		*irq;
    nvm_dma	    		*dma;
    nvm_port    		*port;
{
    struct resource 		*res;
    int 			err;
    int 			size;


    for (err=0 ; !err && resgrp ; resgrp=resgrp->next) {

	/*****************
	* Determine size of selection index.  Resource groups with
	* memory statements require 16 bit indexes while all others
	* require only 8 bit indexes. Add the new selection.
	*****************/
	size = 1;
	for (res=resgrp->resources ; res ; res=res->next)
	    if (res->type == rt_memory) {
		size = 2;
		break;
	    }
	nvm_add_selection(selections, (unsigned)resgrp->index.current, size);

	/*****************
	* Build initialization information for all resource statements
	* in the resource group.
	*****************/
	for (res=resgrp->resources ; !err && res ; res=res->next)
	    switch (res->type) {
		case rt_irq:
		    err = nvm_build_irq(irq, fib, (struct irq *)res);
		    break;
		case rt_dma:
		    err = nvm_build_dma(dma, fib, (struct dma *)res);
		    break;
		case rt_port:
		    err = nvm_build_port(port, fib, (struct port *)res);
		    break;
		case rt_memory:
		    err = nvm_build_memory(memory, fib, (struct memory *)res);
		    break;
	    }

    }

    return(err);
}


/****+++***********************************************************************
*
* Function:     nvm_build_irq
*
* Parameters:   irqs       Current IRQ info
*		fib        Current function info
*		irq        IRQ resource statement
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	      successful
*   		TOO_MANY_IRQS	      too many irqs were needed for this board
*
* Description:
*
*   The nvm_build_irq function builds the nvm information for the IRQ
*   statement.
*
****+++***********************************************************************/

static nvm_build_irq(irqs, fib, irq)
    nvm_irq	    	*irqs;
    nvm_fib	    	*fib;
    struct irq 		*irq;
{
    register int    	line;
    nvm_irq	 	*ip;


    if (!irq->current->null_value) {

	/*****************
	* Find the last used IRQ item in the block.
	*****************/
	ip = irqs;
	if (fib->irq) {
	    while (ip->more)
		ip++;
	    if ((ip + 1) >= &irqs[NVM_MAX_IRQ])
		return(TOO_MANY_IRQS);
	    else
		(ip++)->more = 1;
	}

	line = irq->current->min;
	fib->irq = 1;

	/*****************
	* Set the IRQ information in the block
	*****************/
	ip->share = irq->r.sharing;
	if (irq->trigger == it_level)
	    ip->trigger = NVM_IRQ_LEVEL;
 	else
	    ip->trigger = NVM_IRQ_EDGE;
	ip->line = line;
    }

    return(NVM_SUCCESSFUL);
}


/****+++***********************************************************************
*
* Function:     nvm_build_dma
*
* Parameters:   dmas		current dma info
*		fib		current function info
*		dma		dma resource statement
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	      successful
*   		TOO_MANY_DMAS	      too many dmas were needed for this board
*
* Description:
*
*   The nvm_build_dma function builds the nvm information for the DMA
*   statement.
*
****+++***********************************************************************/

static nvm_build_dma(dmas, fib, dma)
    nvm_dma	    	*dmas;
    nvm_fib	    	*fib;
    struct dma 		*dma;
{
    register int    	channel;
    nvm_dma	 	*dp;


    if (!dma->current->null_value) {

	/*****************
	* Find the last used DMA item in the block.
	*****************/
	dp = dmas;
	if (fib->dma) {
	    while (dp->more)
		dp++;
	    if ((dp + 1) >= &dmas[NVM_MAX_DMA])
		return(TOO_MANY_DMAS);
	    else
		(dp++)->more = 1;
	}

	channel = dma->current->min;
	fib->dma = 1;

	/*****************
	* Set the DMA information in the block
	*****************/
	dp->share = dma->r.sharing;
	switch (dma->timing) {
	    case dt_default:
		dp->timing = NVM_DMA_DEFAULT;
		break;
	    case dt_typea:
		dp->timing = NVM_DMA_TYPEA;
		break;
	    case dt_typeb:
		dp->timing = NVM_DMA_TYPEB;
		break;
	    case dt_typec:
		dp->timing = NVM_DMA_TYPEC;
		break;
	}

	switch (dma->data_size) {
	    case ds_byte:
		dp->width = NVM_DMA_BYTE;
		break;
	    case ds_word:
		dp->width = NVM_DMA_WORD;
		break;
	    case ds_dword:
		dp->width = NVM_DMA_DWORD;
		break;
	}

	dp->channel = channel;
    }

    return(NVM_SUCCESSFUL);
}


/****+++***********************************************************************
*
* Function:     nvm_build_port
*
* Parameters:   ports		current port info
*		fib		current function info
*		port		port resource statement
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	      successful
*   		TOO_MANY_PORTS	      too many ports were needed for this board
*
* Description:
*
*   The nvm_build_port function builds the nvm information for the PORT
*   statement.
*
****+++***********************************************************************/

static nvm_build_port(ports, fib, port)
    nvm_port    	*ports;
    nvm_fib	    	*fib;
    struct port 	*port;
{
    register unsigned 	count;
    unsigned int    	address;
    struct value 	*value;
    nvm_port	     	*pp;


    if ( (port->step) || (!port->u.list.current->null_value) ) {

	/*****************
	* Find the last used PORT item in the block.
	*****************/
	pp = ports;
	if (fib->port) {
	    while (pp->more)
		pp++;
	    pp->more = 1;
	    pp++;
	}

	fib->port = 1;
	if (port->step) {

	    address = port->u.range.current;
	    count = port->u.range.count;

	    for (; ; ) {

		if (pp >= &ports[NVM_MAX_PORT]) {
		    (--pp)->more = 0;
		    return(TOO_MANY_PORTS);
		}

		pp->share = port->r.sharing;
		pp->address_lsb = address & 0xff;
		pp->address_msb = (address >> 8) & 0xff;
		if (port->slot_specific)
		    pp->address_msb |= (nvm_slot_number << 4);

		/************************
		* If more than 32 ports are used, do 32 here and start a new
		* record.
		************************/
		if (count > 32) {
		    pp->count = 31;
		    address += 32;
		    count -= 32;
		    pp->more = 1;
		    pp++;
		}
		else {
		    pp->count = count - 1;
		    break;
		}
	    }

	}

	 else {

	    value = port->u.list.current;
	    count = value->max - value->min + 1;
	    address = value->min;

	    for (; ; ) {

		if (pp >= &ports[NVM_MAX_PORT]) {
		    (--pp)->more = 0;
		    return(TOO_MANY_PORTS);
		}

		pp->share = port->r.sharing;
		pp->address_lsb = address & 0xff;
		pp->address_msb = (address >> 8) & 0xff;
		if (port->slot_specific)
		    pp->address_msb |= (nvm_slot_number << 4);

		/************************
		* If more than 32 ports are used, do 32 here and start a new
		* record.
		************************/
		if (count > 32) {
		    pp->count = 31;
		    address += 32;
		    count -= 32;
		    pp->more = 1;
		    pp++;
		}
		else {
		    pp->count = count - 1;
		    break;
		}
	    }
	}
    }

    return(NVM_SUCCESSFUL);
}


/****+++***********************************************************************
*
* Function:     nvm_build_memory
*
* Parameters:   memorys		current memory info
*		fib		current function info
*		memory		memory resource statement
*
* Used:		internal only
*
* Returns:      NVM_SUCCESSFUL	     successful
*   		TOO_MANY_MEMORYS     too many memorys were needed for this board
*
* Description:
*
*   The nvm_build_memory function builds the nvm information for the
*   memory statement.
*
****+++***********************************************************************/

static nvm_build_memory(memorys, fib, memory)
    nvm_memory  	*memorys;
    nvm_fib	    	*fib;
    struct memory 	*memory;
{
    nvm_memory	     	*mp;
    unsigned int   	address;
    unsigned int   	amount;
    unsigned char   	type;
    unsigned char   	decode;
    unsigned char   	width;


    /*****************
    * Find the next available memory configuration block
    *****************/
    if (memory->memory.step)
	amount = memory->memory.u.range.current;
    else
	amount = memory->memory.u.list.current->min;

    if (amount == 0)
	return(NVM_SUCCESSFUL);

    mp = memorys;
    if (fib->memory) {
	while (mp->more)
	    mp++;
	if ((mp + 1) >= &memorys[NVM_MAX_MEMORY])
	    return(TOO_MANY_MEMORYS);
	else
	    (mp++)->more = 1;
    }

    fib->memory = 1;

    switch (memory->memtype) {
	case mt_sys:
	    type = NVM_MEMORY_SYS;
	    break;
	case mt_exp:
	    type = NVM_MEMORY_EXP;
	    break;
	case mt_other:
	    type = NVM_MEMORY_OTH;
	    break;
	case mt_vir:
	    type = NVM_MEMORY_VIR;
	    break;
    }

    switch (memory->data_size) {
	case ds_byte:
	    width = NVM_MEMORY_BYTE;
	    break;
	case ds_word:
	    width = NVM_MEMORY_WORD;
	    break;
	case ds_dword:
	    width = NVM_MEMORY_DWORD;
	    break;
    }

    switch (memory->decode) {
	case md_20:
	    decode = NVM_MEMORY_20BITS;
	    break;
	case md_24:
	    decode = NVM_MEMORY_24BITS;
	    break;
	case md_32:
	default:
	    decode = NVM_MEMORY_32BITS;
	    break;
    }

    if (memory->address)
	if (memory->address->step)
	    address = memory->address->u.range.current;
	else
	    address = memory->address->u.list.current->min;
    else
	address = 0;

    /*****************
    * Save the memory address and size into the memory struct. The
    * address saved is first divided by 0x100h (which means we only need
    * to save 24 bits).
    * The size is first divided by 0x400h (which means we only need to save 16
    * bits).
    ****************/
    address >>=	8;
    mp->start_lsbyte = address & 0xff;
    mp->start_middlebyte = (address >> 8) & 0xff;
    mp->start_msbyte = (address >> 16) & 0xff;
    amount >>= 10;
    mp->size_lsbyte = amount & 0xff;
    mp->size_msbyte = (amount >> 8) & 0xff;

    /**************
    * Save all of the other attributes away. Note that cache and share are
    * only set if the memory is actually being cached/shared (not just
    * whether they can be).
    **************/
    mp->write = memory->writable;
    mp->cache = memory->caching;
    mp->share = memory->r.sharing;
    mp->type = type;
    mp->width = width;
    mp->decode = decode;

    return(NVM_SUCCESSFUL);
}


/****+++***********************************************************************
*
* Function:     nvm_build_init
*
* Parameters:   init			current init info
*		fib			current function info
*		ioport			ioport resource
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_build_init function builds the nvm information for the
*   ioport resource statement.
*
****+++***********************************************************************/

static void nvm_build_init(init, fib, ioport)
    nvm_init    	*init;
    nvm_fib	    	*fib;
    struct ioport 	*ioport;
{
    nvm_init	 	*ip;


    /*****************
    * Find the last used INIT item in the block. If this is the first item,
    * just set the init bit in the fib structure. Otherwise, walk through the
    * existing entries, set the more bit on the last one, and make sure
    * there is enough space to add our new entry.
    *****************/
    ip = init;
    if (fib->init) {
	while (ip->more)
	    ip++;
	ip->more = 1;
	ip++;
    }
    fib->init = 1;

    /*****************
    * Set the INIT information in the nvm block.
    *****************/
    ip->port_lsb = ioport->address & 0xff;
    ip->port_msb = (ioport->address >> 8) & 0xff;
    if (ioport->slot_specific)
	ip->port_msb |= (nvm_slot_number << 4);
    if (ioport->initval.unchanged_mask != 0)
	ip->mask = 1;
    else
	ip->mask = 0;

    /*****************
    * Write the initialization data (and mask, if necessary) into the
    * nvm block.
    *****************/
    switch (ioport->data_size) {

	case ds_byte:
	    ip->type = NVM_IOPORT_BYTE;
	    if (ip->mask) {
		ip->valmask_1 = (unsigned int)ioport->config_bits.current;
		ip->valmask_2 = (unsigned int)ioport->initval.unchanged_mask;
	    }
	    else
		ip->valmask_1 = (unsigned int)ioport->config_bits.current;
	    break;

	case ds_word:
	    ip->type = NVM_IOPORT_WORD;
	    if (ip->mask) {
		ip->valmask_1 = (unsigned int)ioport->config_bits.current &0xff;
		ip->valmask_2 = (unsigned int)(ioport->config_bits.current >> 8) & 0xff;
		ip->valmask_3 = (unsigned int)ioport->initval.unchanged_mask &
				0xff;
		ip->valmask_4 = (unsigned int)(ioport->initval.unchanged_mask >> 8) & 0xff;
	    }
	    else {
		ip->valmask_1 = (unsigned int)ioport->config_bits.current &0xff;
		ip->valmask_2 = (unsigned int)(ioport->config_bits.current >> 8) & 0xff;
	    }
	    break;

	case ds_dword:
	    ip->type = NVM_IOPORT_DWORD;
	    if (ip->mask) {
		ip->valmask_1 = (unsigned int)ioport->config_bits.current &0xff;
		ip->valmask_2 = (unsigned int)(ioport->config_bits.current >> 8) & 0xff;
		ip->valmask_3 = (unsigned int)(ioport->config_bits.current >> 16) & 0xff;
		ip->valmask_4 = (unsigned int)(ioport->config_bits.current >> 24) & 0xff;
		ip->valmask_5 = (unsigned int)ioport->initval.unchanged_mask & 
		                0xff;
		ip->valmask_6 = (unsigned int)(ioport->initval.unchanged_mask >> 8) & 0xff;
		ip->valmask_7 = (unsigned int)(ioport->initval.unchanged_mask >> 16) & 0xff;
		ip->valmask_8 = (unsigned int)(ioport->initval.unchanged_mask >> 24) & 0xff;
	    }
	    else {
		ip->valmask_1 = (unsigned int)ioport->config_bits.current &0xff;
		ip->valmask_2 = (unsigned int)(ioport->config_bits.current >> 8) & 0xff;
		ip->valmask_3 = (unsigned int)(ioport->config_bits.current >> 16) & 0xff;
		ip->valmask_4 = (unsigned int)(ioport->config_bits.current >> 24) & 0xff;
	    }
	    break;
    }

}


/****+++***********************************************************************
*
* Function:     nvm_ioport_size
*
* Parameters:   ioport		ioport initializer
*
* Used:		internal only
*
* Returns:      The number of bytes occupied by the init item.
*
* Description:
*
*   The nvm_ioport_size function returns the size of an ioport init item
*   in bytes.
*
****+++***********************************************************************/

static nvm_ioport_size(ioport)
    struct ioport 	*ioport;
{
    int 		size;


    /*******************
    * Start by setting size to the size of the initialization data.
    *******************/
    switch (ioport->data_size) {
	case ds_byte:
	    size = 1;
	    break;
	case ds_word:
	    size = 2;
	    break;
	case ds_dword:
	    size = 4;
	    break;
    }

    /******************
    * If there's also a mask, double the size.
    ******************/
    if (ioport->initval.unchanged_mask)
	size *= 2;

    /******************
    * Finally, add 3 bytes to account for the header and the port address.
    ******************/
    return(size + 3);
}





/****+++***********************************************************************
*
* Function:     nvm_init_size
*
* Parameters:   init		current init info
*
* Used:		internal and external
*
* Returns:      the number of bytes occupied by the init item
*
* Description:
*
*   The nvm_init_size function returns the size of an ioport init item
*   in bytes.
*
****+++***********************************************************************/

int nvm_init_size(init)
    nvm_init    *init;
{
    int 	size;


    /*******************
    * Set the size for the various types.
    *******************/
    switch (init->type) {
	case NVM_IOPORT_BYTE:
	    size = 1;
	    break;
	case NVM_IOPORT_WORD:
	    size = 2;
	    break;
	case NVM_IOPORT_DWORD:
	    size = 4;
	    break;
    }

    /*****************
    * If there is also a mask value, we'll need twice as much space
    * to store the entry.
    *****************/
    if (init->mask)
	size *= 2;

    /****************
    * Finally, add in 3 bytes to cover the header information and the
    * port address.
    ****************/
    return(size + 3);
}


/****+++***********************************************************************
*
* Function:     nvm_add_selection
*
* Parameters:   selections      Current selections table
*   		index		Next index to set
*   		size		Size of index in bytes
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_add_selection function adds a new selection index to the
*   table of current selections.
*
****+++***********************************************************************/

static void nvm_add_selection(selections, index, size)
    unsigned char	*selections;
    unsigned int	index;
    int 		size;
{
    register int    	offset;


    offset = selections[0] + 1;
    if ( (offset + size) <= NVM_MAX_SELECTIONS) {
	if (size > 1)
	    *(unsigned short int *)(selections + offset) = index;
	else
	    selections[offset] = index;
	selections[0] += size;
    }

}






/****+++***********************************************************************
*
* Function:     nvm_append_type
*
* Parameters:   buffer		Base buffer (also target buffer)
*   		type		Type string to add
*   		sep		Separator string
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*   The nvm_append_type function appends a type string to an existing
*   string using the specified separator. The function will check that
*   the length of the final string does not exceed 80 characters and will
*   start truncating at 80.
*
*   The first byte of buffer (on exit) contains the size of the buffer.
*
****+++***********************************************************************/

static void nvm_append_type(buffer, type, sep)
    unsigned char	*buffer;
    char		*type;
    char		*sep;
{
    register unsigned 	l;
    unsigned char   	*p;


    if (type) {

	l = *buffer;
	p = &buffer[l + 1];

	if ( (l)  &&  (l < NVM_MAX_TYPE)  && (*p = *sep) ) {
	    p++;
	    l++;
	}

	while ( (l < NVM_MAX_TYPE)  &&  (*type) ) {
	    *p++ = *type++;
	    l++;
	}

	*buffer = l;
    }
}


/****+++***********************************************************************
*
* Function:     nvm_build_dupid()
*
* Parameters:   board		board data structure to get dup for
*		dupid		resulting structure
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This routine looks at a board structure and pulls out the dup info.
*    It then shoves it into a nvm_dupid that has been passed in. This
*    struct will be part of the eeprom/sci record.
*
****+++***********************************************************************/

static void nvm_build_dupid(board, dupid)
    struct board 	*board;
    nvm_dupid   	*dupid;
{

    dupid->dups = board->duplicate;
    dupid->dup_id = board->duplicate_id;

    switch (board->slot) {
#ifdef VIRT_BOARD
	case bs_vir:
	    dupid->type = 2;
	    break;
#endif
	case bs_emb:
	    dupid->type = 1;
	    break;
	default:
	    dupid->type = 0;
	    break;
    }

    dupid->noreadid = !board->readid;
    dupid->disable = board->disable;
    dupid->iocheck = board->iocheck;
    dupid->partial = board->partial_config;
}





/****+++***********************************************************************
*
* Function:     nvm_build_id()
*
* Parameters:   board_name		name of the board
*		board_id		compressed board id (filled in here)
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function converts a board name into a compressed board id.
*
****+++***********************************************************************/

static void nvm_build_id(board_name, board_id)
    unsigned char	*board_name;
    unsigned char	*board_id;
{

    board_id[0] = ((board_name[0] - '@') << 2) +
    		  (((board_name[1] - '@') >> 3) & 3);
    board_id[1] = (((board_name[1] - '@') & 7) << 5) + (board_name[2] - '@');
    board_id[2] = (hex2bin(board_name[3]) << 4) + hex2bin(board_name[4]);
    board_id[3] = (hex2bin(board_name[5]) << 4) + hex2bin(board_name[6]);
}


/****+++***********************************************************************
*
* Function:     hex2bin()
*
* Parameters:   chr		hex character to convert
*
* Used:		internal only
*
* Returns:      integer value of the hex character
*
* Description:
*
*    Convert from a hex character to its integer equivalent.
*
****+++***********************************************************************/

static int hex2bin(chr)
    char	chr;
{
    int		value;

    if((value = (chr - '0')) > 9)
	if((value -= 'A' - '0' - 10) > 15)
	    value -= 'a' - 'A';

    return(value);
}
