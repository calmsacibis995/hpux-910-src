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
/*****************************************************************************
*
*+*+*+************************************************************************
*
*				src/display.c
*
*  This file contains functions which display the basic data structures
*  of the Fantasia code is a readble format. This code should all be
*  considered debug information only.
*
**++*************************************************************************/

#ifdef DEBUG

#include "config.h"

/*******************
* Functions in this file
*******************/

void display_boards ();
void display_groups ();
void display_functions ();
void display_subfunctions ();
void display_index ();
void display_slot ();
void display_choices ();
void decode_item_status();
void decode_data_size();
void display_subchoices ();
void display_resource_groups ();
void display_resources ();
void display_irq ();
void display_dma ();
void display_memory ();
void display_eeprom ();
void display_tags ();
void display_labels ();
void display_softwares ();
void display_config_bits ();
void display_spaces ();
void display_caches ();
void display_values ();
void display_portvars ();
void display_ioports ();
void display_switches ();
void display_jumpers ();
void display_inits ();
void display_init_values ();
void display_memory_address ();
void display_total_mem ();
void display_ports ();
char *yes_or_no();


/****+++***********************************************************************
*
* Function:     display_system
*
* Parameters:   system_ptr		pointer to a system struct
*
* Used:		external only
*
* Returns:      Nothing
*
* Description:
*
*    This function takes a system pointer and displays its contents via
*    standard printfs.
*
****+++***********************************************************************/

void display_system(system_ptr)
    struct system	*system_ptr;
{
    int			i;
    
    /**************************
    * First, print the stuff that can be handled in this routine.
    **************************/
    printf("\n\n\n*** System data structure decoding ***\n\n");
    printf("    nonvolatile\t\t%u\n", system_ptr->nonvolatile);
    printf("    amperage\t\t%u\n", system_ptr->amperage);
    printf("    default_sizing\t%u\n", system_ptr->default_sizing);
    printf("    configured\t\t%s\n", yes_or_no(system_ptr->configured));
    printf("    amp_overload\t%s\n", yes_or_no(system_ptr->amp_overload));
    printf("    entries\t\t%d\n", system_ptr->entries);
    printf("\n");


#ifdef SYS_CACHE
    /*************************
    * Print the cache information.
    *************************/
    printf("    Cache information:\n");
    display_caches("        ", system_ptr->cache_map);
    printf("\n\n");
#endif

    /*************************
    * Print the dma information.
    *************************/
    printf("    DMA information:\n");
    display_spaces("        ", system_ptr->dma);
    printf("\n\n");

    /*************************
    * Print the irq information.
    *************************/
    printf("    IRQ information:\n");
    display_spaces("        ", system_ptr->irq);
    printf("\n\n");

    /*************************
    * Print the port information.
    *************************/
    printf("    Port information:\n");
    display_spaces("        ", system_ptr->port);
    printf("\n\n");

    /*************************
    * Print the memory information.
    *************************/
    printf("    Memory information:\n");
    display_spaces("        ", system_ptr->memory);
    printf("\n\n");

    /*************************
    * Print the slot information.
    *************************/
    printf("    Slot information:\n");
    for (i=0 ; i<MAX_NUM_SLOTS ; i++) {
	printf("        Slot %d:\n", i);
	display_slot("            ", &system_ptr->slot[i]);
	printf("\n");
    }
    printf("\n\n");

    /*************************
    * Print the sorted slot information.
    *************************/
    printf("    Sorted slot information:\n");
    for (i=0 ; i<MAX_NUM_SLOTS ; i++) {
	printf("        index %d\n", i);
	printf("            slot %d\n", system_ptr->sorted[i].slot);
	printf("            key  0x%08x  (%u)\n\n", system_ptr->sorted[i].key,
	       system_ptr->sorted[i].key);
    }

    /*************************
    * Print the boards information.
    *************************/
    printf("\f");
    printf("    Boards information:\n");
    display_boards("        ", system_ptr->boards);
    printf("\n\n");
}

/****+++***********************************************************************
*
* Function:     display_boards
*
* Parameters:   board_ptr		pointer to the first board to display
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function prints info for each of the boards in the system.
*
****+++***********************************************************************/

void display_boards (prefix, board_ptr)
    char		*prefix;
    struct board	*board_ptr;
{
    struct board	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");

    /***********************
    * Return immediately if there is no board information.
    ***********************/
    ptr = board_ptr;
    if (ptr == 0) {
	printf("%sNo valid board information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the boards and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sslot_number\t\t%d\n", prefix, ptr->slot_number);
	printf("%seisa_slot\t\t%d\n", prefix, ptr->eisa_slot);
	printf("%sid\t\t\t%s\n", prefix, ptr->id);
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%smfr\t\t\t%s\n", prefix, ptr->mfr);
	printf("%scategory\t\t%s\n", prefix, ptr->category);
	printf("%sslot_tag\t\t\t%s\n", prefix, ptr->slot_tag);
	printf("%scomments\t\t\t%s\n", prefix, ptr->comments);
	printf("%shelp\t\t\t%s\n", prefix, ptr->help);
	printf("%samperage\t\t%u\n", prefix, ptr->amperage);
	printf("%ssizing\t\t\t%u\n", prefix, ptr->sizing);
	printf("%schecksum\t\t%u\n", prefix, ptr->checksum);
	printf("%sduplicate_id\t\t%u\n", prefix, ptr->duplicate_id);
	printf("%sskirt\t\t\t%s\n", prefix, yes_or_no(ptr->skirt));
	printf("%sreadid\t\t\t%s\n", prefix, yes_or_no(ptr->readid));
	printf("%siocheck\t\t\t%s\n", prefix, yes_or_no(ptr->iocheck));
	printf("%sdisable\t\t\t%s\n", prefix, yes_or_no(ptr->disable));
	printf("%slocked\t\t\t%s\n", prefix, yes_or_no(ptr->locked));
	printf("%sconflict\t\t%s\n", prefix, yes_or_no(ptr->conflict));
	printf("%spartial_config\t\t%s\n", prefix, yes_or_no(ptr->partial_config));
	printf("%sconfig_changed\t\t%s\n", prefix, yes_or_no(ptr->config_changed));
	printf("%sduplicate\t\t%s\n", prefix, yes_or_no(ptr->duplicate));
	printf("%slength\t\t\t%d\n", prefix, ptr->length);
	if (ptr->busmaster == 0)
	    printf("%sbusmaster\t\tnot a bus master\n", prefix);
	else
	    printf("%sbusmaster\t\t%d\n", prefix, ptr->busmaster);
	printf("%sentries\t\t\t%d\n", prefix, ptr->entries);

	printf("%sslot\t\t\t", prefix);
	switch (ptr->slot) {
	    case bs_isa8:
		printf("isa8\n");
		break;
	    case bs_isa16:
		printf("isa16\n");
		break;
	    case bs_isa8or16:
		printf("isa8or16\n");
		break;
	    case bs_eisa:
		printf("eisa\n");
		break;
	    case bs_oth:
		printf("other\n");
		break;
	    case bs_vir:
		printf("virtual\n");
		break;
	    case bs_emb:
		printf("embedded\n");
		break;
	    default:
		printf("unknown type\n");
	}

	/***********************
	* Display the groups information.
	***********************/
	printf("\n\n\f");
	printf("%sGroup information:\n", prefix);
	display_groups(new_prefix, ptr->groups);

	/***********************
	* Display the ioports information.
	***********************/
	printf("\n\f");
	printf("%sIoport information:\n", prefix);
	display_ioports(new_prefix, ptr->ioports, 0);

	/***********************
	* Display the jumpers information.
	***********************/
	printf("\n\f");
	printf("%sJumper information:\n", prefix);
	display_jumpers(new_prefix, ptr->jumpers, 0);

	/***********************
	* Display the switches information.
	***********************/
	printf("\n\f");
	printf("%sSwitch information:\n", prefix);
	display_switches(new_prefix, ptr->switches, 0);

	/***********************
	* Display the software information.
	***********************/
	printf("\n\f");
	printf("%sSoftware information:\n", prefix);
	display_softwares(new_prefix, ptr->softwares, 0);

	/***********************
	* Advance to the next board.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next board ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_groups
*
* Parameters:   group_ptr		ptr to a group struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This routines displays all group structs starting from the ptr passed in.
*
****+++***********************************************************************/

void display_groups (prefix, group_ptr)
    char		*prefix;
    struct group	*group_ptr;
{
    struct group	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no group information.
    ***********************/
    ptr = group_ptr;
    if (ptr == 0) {
	printf("%sNo valid group information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the groups and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%stype\t\t\t%s\n", prefix, ptr->type);
	printf("%sexplicit\t\t\t%s\n", prefix, yes_or_no(ptr->explicit));

	/***********************
	* Display all function information.
	***********************/
	printf("\n\n%sFunction information:\n", prefix);
	display_functions(new_prefix, ptr->functions);

	/***********************
	* Advance to the next group.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0) {
	    printf("\n\n\f");
	    printf("%s*** next group ***\n\n", prefix);
	}

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_functions
*
* Parameters:   function_ptr	pointer to the a function struct
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This routine displays all functions for a given board.
*
****+++***********************************************************************/

void display_functions (prefix, function_ptr)
    char		*prefix;
    struct function	*function_ptr;
{
    struct function	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no function information.
    ***********************/
    ptr = function_ptr;
    if (ptr == 0) {
	printf("%sNo valid function information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the functions and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%stype\t\t\t%s\n", prefix, ptr->type);
	printf("%sconnection\t\t%s\n", prefix, ptr->connection);
	printf("%scomments\t\t%s\n", prefix, ptr->comments);
	printf("%shelp\t\t\t%s\n", prefix, ptr->help);
	printf("%sdisplay\t\t\t%s\n", prefix, yes_or_no(ptr->display));

	/***********************
	* Display all subfunction information.
	***********************/
	printf("\n%sSubfunction information:\n", prefix);
	display_subfunctions(new_prefix, ptr->subfunctions);

	/***********************
	* Advance to the next function.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n\f%s*** next function ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_subfunctions
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_subfunctions (prefix, subfunction_ptr)
    char		*prefix;
    struct subfunction	*subfunction_ptr;
{
    struct subfunction	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no subfunction information.
    ***********************/
    ptr = subfunction_ptr;
    if (ptr == 0) {
	printf("%sNo valid subfunction information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the subfunctions and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%stype\t\t\t%s\n", prefix, ptr->type);
	printf("%sconnection\t\t\t%s\n", prefix, ptr->connection);
	printf("%scomments\t\t\t%s\n", prefix, ptr->comments);
	printf("%shelp\t\t\t%s\n", prefix, ptr->help);
	printf("%sexplicit\t\t\t%s\n", prefix, yes_or_no(ptr->explicit));
	printf("%sconflict\t\t\t%s\n", prefix, yes_or_no(ptr->conflict));
	printf("%sentry_num\t\t\t%d\n", prefix, ptr->entry_num);

	/***********************
	* Display item_status.
	***********************/
	printf("%sstatus\t\t\t", prefix);
	decode_item_status(ptr->status);
	printf("\n");

	/***********************
	* Display choices.
	***********************/
	printf("\n\f%sChoices:\n", prefix);
	display_choices(new_prefix, ptr->choices);
	printf("\n\f%sConfig:\n", prefix);
	display_choices(new_prefix, ptr->config);
	printf("\n\f%sCurrent:\n", prefix);
	display_choices(new_prefix, ptr->current);

	/***********************
	* Display index.
	***********************/
	printf("\n%sIndex:\n", prefix);
	display_index(new_prefix, &ptr->index);

	/***********************
	* Advance to the next subfunction.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next subfunction ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_index
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_index (prefix, ptr)
    char		*prefix;
    struct index	*ptr;
{

    /***********************
    * Print all fields.
    ***********************/
    printf("%stotal\t\t\t%d\n", prefix, ptr->total);
    printf("%seeprom\t\t\t%d\n", prefix, ptr->eeprom);
    printf("%sconfig\t\t\t%d\n", prefix, ptr->config);
    printf("%sprevious\t\t\t%d\n", prefix, ptr->previous);
    printf("%scurrent\t\t\t%d\n", prefix, ptr->current);

    return;
}

/****+++***********************************************************************
*
* Function:     display_slot
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_slot (prefix, ptr)
    char		*prefix;
    struct slot		*ptr;
{

    /***********************
    * If the slot does not exist, display present and exit.
    ***********************/
    if (ptr->present == 0) {
	printf("%spresent\t\t\t%s\n", prefix, yes_or_no(ptr->present));
	return;
    }

    /***********************
    * Print all fields.
    ***********************/
    printf("%slabel\t\t\t%s\n", prefix, ptr->label);
    printf("%slabel2\t\t\t%s\n", prefix, ptr->label2);
    printf("%spresent\t\t\t%s\n", prefix, yes_or_no(ptr->present));
    printf("%sskirt\t\t\t%s\n", prefix, yes_or_no(ptr->skirt));
    printf("%sbusmaster\t\t\t%s\n", prefix, yes_or_no(ptr->busmaster));
    printf("%soccupied\t\t\t%s\n", prefix, yes_or_no(ptr->occupied));
    printf("%slength\t\t\t%d\n", prefix, ptr->length);
    printf("%seisa_slot\t\t\t%d\n", prefix, ptr->eisa_slot);

    printf("%stype\t\t\t", prefix);
    switch(ptr->type) {
	case st_other:
	    printf("other\n");
	    break;
	case st_isa8:
	    printf("isa8\n");
	    break;
	case st_isa16:
	    printf("isa16\n");
	    break;
	case st_eisa:
	    printf("eisa\n");
	    break;
	default:
	    printf("unknown type\n");
    }

    /********************
    * Print tags.
    ********************/
    printf("%stags\t\t\t", prefix);
    display_tags(ptr->tags);

    return;
}

/****+++***********************************************************************
*
* Function:     display_choices
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_choices (prefix, choice_ptr)
    char		*prefix;
    struct choice	*choice_ptr;
{
    struct choice	*ptr;
    char		new_prefix[80];
    unsigned char	*free;
    int			freect;
    int			i;


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no choice information.
    ***********************/
    ptr = choice_ptr;
    if (ptr == 0) {
	printf("%sNo valid choice information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the choices and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%shelp\t\t\t%s\n", prefix, ptr->help);
	printf("%ssubtype\t\t\t%s\n", prefix, ptr->subtype);
	printf("%samperage\t\t%u\n", prefix, ptr->amperage);
	printf("%sstatus\t\t\t", prefix);
	decode_item_status(ptr->status);
	printf("\n");
	printf("%sdisable\t\t\t%s\n", prefix, yes_or_no(ptr->disable));
	printf("%snew\t\t\t%s\n", prefix, yes_or_no(ptr->new));
	printf("%sentry_num\t\t\t%d\n", prefix, ptr->entry_num);

	/***********************
	* Display freeform stuff.
	***********************/
	free = ptr->freeform;
	freect = *free++;
	if (freect > 0) {
	    printf("%sfreeform\t\t", prefix);
	    for (i=0 ; i<freect ; i++) {
		printf("%u    ", *free++);
	    }
	    printf("\n");
	}
	
	/***********************
	* Display index.
	***********************/
	printf("\n%sIndex:\n", prefix);
	display_index(new_prefix, &ptr->index);

	/***********************
	* Display portvars.
	***********************/
	printf("\n%sPortvars information:\n", prefix);
	display_portvars(new_prefix, ptr->portvars);

	/***********************
	* Display totalmem.
	***********************/
	printf("\n%sTotalmem information:\n", prefix);
	display_total_mem(new_prefix, ptr->totalmem);

	/***********************
	* Display eeprom.
	***********************/
	printf("\n%sEEPROM information:\n", prefix);
	display_eeprom(new_prefix, ptr->eeprom);

	/***********************
	* Display primary.
	***********************/
	printf("\n%sPrimary subchoice information:\n", prefix);
	display_subchoices(new_prefix, ptr->primary);

	/***********************
	* Display subchoices.
	***********************/
	printf("\n%sSubchoice information:\n", prefix);
	display_subchoices(new_prefix, ptr->subchoices);

	/***********************
	* Display config.
	***********************/
	printf("\n%sConfig subchoice information:\n", prefix);
	display_subchoices(new_prefix, ptr->config);

	/***********************
	* Display current.
	***********************/
	printf("\n%sCurrent subchoice information:\n", prefix);
	display_subchoices(new_prefix, ptr->current);

	/***********************
	* Advance to the next choice.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n\f%s*** next choice ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     decode_item_status
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void decode_item_status(val)
    enum item_status	val;
{
    switch (val) {
	case is_actual:
	    printf("actual");
	    break;
	case is_locked:
	    printf("locked");
	    break;
	case is_user:
	    printf("user");
	    break;
	case is_eeprom:
	    printf("eeprom");
	    break;
	case is_free:
	    printf("free");
	    break;
    }
    return;
}

/****+++***********************************************************************
*
* Function:     decode_data_size
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void decode_data_size(val)
    enum data_size	val;
{
    switch (val) {
	case ds_byte:
	    printf("byte");
	    break;
	case ds_word:
	    printf("word");
	    break;
	case ds_dword:
	    printf("dword");
	    break;
    }
    return;
}

/****+++***********************************************************************
*
* Function:     display_subchoices
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_subchoices (prefix, subchoice_ptr)
    char		*prefix;
    struct subchoice	*subchoice_ptr;
{
    struct subchoice	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no subchoice information.
    ***********************/
    ptr = subchoice_ptr;
    if (ptr == 0) {
	printf("%sNo valid subchoice information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the subchoices and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sexplicit\t\t\t%s\n", prefix, yes_or_no(ptr->explicit));

	/***********************
	* Display resource group information.
	***********************/
	printf("\n%sResource group information:\n", prefix);
	display_resource_groups(new_prefix, ptr->resource_groups);

	/***********************
	* Advance to the next subchoice.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next subchoice ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_resource_groups
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_resource_groups (prefix, resource_group_ptr)
    char			*prefix;
    struct resource_group	*resource_group_ptr;
{
    struct resource_group	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no resource_group information.
    ***********************/
    ptr = resource_group_ptr;
    if (ptr == 0) {
	printf("%sNo valid resource_group information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the resource_groups and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sentry_num\t\t\t%d\n", prefix, ptr->entry_num);
	printf("%snew\t\t\t%s\n", prefix, yes_or_no(ptr->new));
	printf("%sstatus\t\t\t", prefix);
	decode_item_status(ptr->status);
	printf("\n");

	printf("%stype\t\t\t", prefix);
	switch (ptr->type) {
	    case rg_link:
		printf("rg_link");
		break;
	    case rg_dlink:
		printf("rg_dlink");
		break;
	    case rg_combine:
		printf("rg_combine");
		break;
	    case rg_free:
		printf("rg_free");
		break;
	}
	printf("\n");

	/***********************
	* Display index.
	***********************/
	printf("\n%sIndex:\n", prefix);
	display_index(new_prefix, &ptr->index);

	/***********************
	* Display resources.
	***********************/
	printf("\n%sResources information:\n", prefix);
	display_resources(new_prefix, ptr->resources);

	/***********************
	* Display inits.
	***********************/
	printf("\n%sInits information:\n", prefix);
	display_inits(new_prefix, ptr->inits);

	/***********************
	* Advance to the next resource_group.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next resource_group ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_resources
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_resources (prefix, resource_ptr)
    char		*prefix;
    struct resource	*resource_ptr;
{
    struct resource	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");

    /***********************
    * Return immediately if there is no resource information.
    ***********************/
    ptr = resource_ptr;
    if (ptr == 0) {
	printf("%sNo valid resource information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the resources and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sshare_tag\t\t%s\n", prefix, ptr->share_tag);
	printf("%sshare\t\t%s\n", prefix, yes_or_no(ptr->share));
	printf("%ssharing\t\t%s\n", prefix, yes_or_no(ptr->sharing));
	printf("%sconflict\t\t%s\n", prefix, yes_or_no(ptr->conflict));
	printf("%snew\t\t\t%s\n", prefix, yes_or_no(ptr->new));
	printf("%sentry_num\t\t%d\n", prefix, ptr->entry_num);
	printf("%stype\t\t", prefix);
	switch (ptr->type) {
	    case rt_irq:
		printf("irq\n");
		printf("\n%sIRQ info:\n", prefix);
		display_irq(new_prefix, ptr);
		break;
	    case rt_dma:
		printf("dma\n");
		printf("\n%sDMA info:\n", prefix);
		display_dma(new_prefix, ptr);
		break;
	    case rt_port:
		printf("port\n");
		printf("\n%sPort info:\n", prefix);
		display_ports(new_prefix, ptr);
		break;
	    case rt_memory:
		printf("memory\n");
		printf("\n%sMemory info:\n", prefix);
		display_memory(new_prefix, ptr);
		break;
	}

	/***********************
	* Advance to the next resource.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next resource ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_irq
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_irq (prefix, irq_ptr)
    char		*prefix;
    struct irq		*irq_ptr;
{
    struct irq		*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no irq information.
    ***********************/
    ptr = irq_ptr;
    if (ptr == 0) {
	printf("%sNo valid irq information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sindex_value\t\t\t%s\n", prefix, ptr->index_value);
    printf("%strigger\t\t\t", prefix);
    switch (ptr->trigger) {
	case it_edge:
	    printf("edge-triggered");
	    break;
	case it_level:
	    printf("level-triggered");
	    break;
    }
    printf("\n");

    /***********************
    * Display index.
    ***********************/
    printf("\n%sIndex:\n", prefix);
    display_index(new_prefix, &ptr->index);

    /***********************
    * Display values.
    ***********************/
    printf("\n%sValues information:\n", prefix);
    display_values(new_prefix, ptr->values);
    printf("\n%sConfig information:\n", prefix);
    display_values(new_prefix, ptr->config);
    printf("\n%sCurrent information:\n", prefix);
    display_values(new_prefix, ptr->current);

    return;
}

/****+++***********************************************************************
*
* Function:     display_dma
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_dma (prefix, dma_ptr)
    char		*prefix;
    struct dma		*dma_ptr;
{
    struct dma		*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no dma information.
    ***********************/
    ptr = dma_ptr;
    if (ptr == 0) {
	printf("%sNo valid dma information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sindex_value\t\t\t%s\n", prefix, ptr->index_value);
    printf("%sdata_size\t\t\t", prefix);
    decode_data_size(ptr->data_size);
    printf("\n");
    printf("%stiming\t\t\t", prefix);
    switch(ptr->timing) {
	case dt_default:
	    printf("default");
	    break;
	case dt_typea:
	    printf("typea");
	    break;
	case dt_typeb:
	    printf("typeb");
	    break;
	case dt_typec:
	    printf("typec");
	    break;
    }
    printf("\n");

    /***********************
    * Display index.
    ***********************/
    printf("\n%sIndex:\n", prefix);
    display_index(new_prefix, &ptr->index);

    /***********************
    * Display values.
    ***********************/
    printf("\n%sValues information:\n", prefix);
    display_values(new_prefix, ptr->values);
    printf("\n%sConfig information:\n", prefix);
    display_values(new_prefix, ptr->config);
    printf("\n%sCurrent information:\n", prefix);
    display_values(new_prefix, ptr->current);

    return;
}

/****+++***********************************************************************
*
* Function:     display_memory
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_memory (prefix, memory_ptr)
    char		*prefix;
    struct memory	*memory_ptr;
{
    struct memory	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");

    /***********************
    * Return immediately if there is no memory information.
    ***********************/
    ptr = memory_ptr;
    if (ptr == 0) {
	printf("%sNo valid dma information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sdata_size\t\t", prefix);
    decode_data_size(ptr->data_size);
    printf("\n");
    printf("%swritable\t\t%s\n", prefix, yes_or_no(ptr->writable));
    printf("%scache\t\t\t%s\n", prefix, yes_or_no(ptr->cache));
    printf("%scaching\t\t\t%s\n", prefix, yes_or_no(ptr->caching));

    printf("%smemory_decode\t\t", prefix);
    switch (ptr->decode) {
	case md_20:
	    printf("20 bits\n");
	    break;
	case md_24:
	    printf("24 bits\n");
	    break;
	case md_32:
	    printf("32 bits\n");
	    break;
    }

    printf("%smemtype\t\t\t", prefix);
    switch (ptr->memtype) {
	case mt_sys:
	    printf("system\n");
	    break;
	case mt_exp:
	    printf("expanded\n");
	    break;
	case mt_other:
	    printf("other\n");
	    break;
	case mt_vir:
	    printf("virtual\n");
	    break;
    }

    /***********************
    * Display memory addresses.
    ***********************/
    printf("\n%sMemory:\n", prefix);
    display_memory_address(new_prefix, &ptr->memory);
    printf("\n%sAddress:\n", prefix);
    display_memory_address(new_prefix, ptr->address);

    return;
}

/****+++***********************************************************************
*
* Function:     display_eeprom
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_eeprom (prefix, eeprom_ptr)
    char		*prefix;
    struct eeprom		*eeprom_ptr;
{
    struct eeprom		*ptr;


    /***********************
    * Return immediately if there is no eeprom information.
    ***********************/
    ptr = eeprom_ptr;
    if (ptr == 0) {
	printf("%sNo valid eeprom information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the eeprom structs and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%soffset\t\t\t%u\n", prefix, ptr->offset);
	printf("%smask\t\t\t0x%08x   (%u)\n", prefix, ptr->mask, ptr->mask);
	printf("%svalue\t\t\t%u\n", prefix, ptr->value);

	/***********************
	* Advance to the next eeprom struct.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next eeprom ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_tags
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_tags (tag_ptr)
    struct tag		*tag_ptr;
{
    struct tag		*ptr;


    /***********************
    * Return immediately if there is no tag information.
    ***********************/
    ptr = tag_ptr;
    if (ptr == 0) {
	printf("\n");
        return;
    }

    /**********************
    * Walk through each of the tags and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%s   ", ptr->string);

	/***********************
	* Advance to the next tag.
	***********************/
	ptr = ptr->next;

    } while ((int)ptr != 0);

    printf("\n");

    return;
}

/****+++***********************************************************************
*
* Function:     display_labels
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_labels (prefix, label_ptr)
    char		*prefix;
    struct label	*label_ptr;
{
    struct label	*ptr;


    /***********************
    * Return immediately if there is no label information.
    ***********************/
    ptr = label_ptr;
    if (ptr == 0) {
	printf("%sNo valid label information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the labels and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sposition\t\t\t%d\n", prefix, ptr->position);
	printf("%sstring\t\t\t%s\n", prefix, ptr->string);

	/***********************
	* Advance to the next label.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next label ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_softwares
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_softwares (prefix, software_ptr, mode)
    char		*prefix;
    struct software	*software_ptr;
    int			mode;
{
    struct software	*ptr;


    /***********************
    * Return immediately if there is no software information.
    ***********************/
    ptr = software_ptr;
    if (ptr == 0) {
	printf("%sNo valid software information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the softwares and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sdescription\t\t%s\n", prefix, ptr->description);
	printf("%sprevious\t\t\t%s\n", prefix, ptr->previous);
	printf("%scurrent\t\t\t%s\n", prefix, ptr->current);
	printf("%sindex\t\t\t%d\n", prefix, ptr->index);

	/***********************
	* Advance to the next software.
	***********************/
	if (mode == 1)
	    return;
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next software ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_config_bits
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_config_bits (prefix, config_bits_ptr)
    char		*prefix;
    struct config_bits	*config_bits_ptr;
{
    struct config_bits	*ptr;


    /***********************
    * Return immediately if there is no config_bit information.
    ***********************/
    ptr = config_bits_ptr;
    if (ptr == 0) {
	printf("%sNo valid config_bits information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sprevious\t\t\t%u\n", prefix, ptr->previous);
    printf("%scurrent\t\t\t%u\n", prefix, ptr->current);

    return;
}

/****+++***********************************************************************
*
* Function:     display_spaces
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_spaces (prefix, space_ptr)
    char		*prefix;
    struct space	*space_ptr;
{
    struct space	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no space information.
    ***********************/
    ptr = space_ptr;
    if (ptr == 0) {
	printf("%sNo valid space information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the spaces and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%ssp_min\t\t\t%u\n", prefix, ptr->sp_min);
	printf("%ssp_max\t\t\t%u\n", prefix, ptr->sp_max);
	printf("%sowner\t\t\t%d\n", prefix, ptr->owner);

	/***********************
	* Display resource.
	***********************/
	printf("\n%sResource information:\n", prefix);
	display_resources(new_prefix, ptr->resource);

	/***********************
	* Advance to the next space.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next space ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

#ifdef SYS_CACHE
/****+++***********************************************************************
*
* Function:     display_caches
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_caches (prefix, cache_ptr)
    char		*prefix;
    struct cache_map	*cache_ptr;
{
    struct cache_map	*ptr;


    /***********************
    * Return immediately if there is no cache information.
    ***********************/
    ptr = cache_ptr;
    if (ptr == 0) {
	printf("%sNo valid cache information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the caches and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%smemory\t\t\t\t0x%08x   (%u)\n", prefix, ptr->memory,
	       ptr->memory);
	printf("%saddress\t\t\t\t0x%08x   (%u)\n", prefix, ptr->address,
	       ptr->address);
	printf("%sstep (cache granularity)\t0x%08x   (%u)\n", prefix, ptr->step,
	       ptr->step);
	printf("%scache\t\t\t\t%s\n", prefix, yes_or_no(ptr->cache));

	/***********************
	* Advance to the next cache.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next cache ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}
#endif

/****+++***********************************************************************
*
* Function:     display_values
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_values (prefix, value_ptr)
    char		*prefix;
    struct value	*value_ptr;
{
    struct value	*ptr;


    /***********************
    * Return immediately if there is no value information.
    ***********************/
    ptr = value_ptr;
    if (ptr == 0) {
	printf("%sNo valid value information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the values and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%smin\t\t\t0x%08x   (%u)\n", prefix, ptr->min, ptr->min);
	printf("%smin\t\t\t0x%08x   (%u)\n", prefix, ptr->max, ptr->max);
	printf("%sinit_index\t\t%u\n", prefix, ptr->init_index);
	printf("%snull_value\t\t%s\n", prefix, yes_or_no(ptr->null_value));

	/***********************
	* Advance to the next value.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next value ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_portvars
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_portvars (prefix, portvar_ptr)
    char		*prefix;
    struct portvar	*portvar_ptr;
{
    struct portvar	*ptr;


    /***********************
    * Return immediately if there is no portvar information.
    ***********************/
    ptr = portvar_ptr;
    if (ptr == 0) {
	printf("%sNo valid portvar information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the portvars and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%saddress\t\t\t%u\n", prefix, ptr->address);
	printf("%sslot_specific\t\t%s\n", prefix, yes_or_no(ptr->slot_specific));
	printf("%sindex\t\t\t%d\n", prefix, ptr->index);

	/***********************
	* Advance to the next portvar.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next portvar ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_ioports
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_ioports (prefix, ioport_ptr, mode)
    char		*prefix;
    struct ioport	*ioport_ptr;
    int			mode;
{
    struct ioport	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no ioport information.
    ***********************/
    ptr = ioport_ptr;
    if (ptr == 0) {
	printf("%sNo valid ioport information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the ioports and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sindex\t\t\t\t%d\n", prefix, ptr->index);
	printf("%saddress\t\t\t\t%u\n", prefix, ptr->address);
	printf("%sinitval.reserved_mask\t\t0x%08x  (%u)\n", prefix,
		ptr->initval.reserved_mask, ptr->initval.reserved_mask);
	printf("%sinitval.unchanged_mask\t\t0x%08x  (%u)\n", prefix,
		ptr->initval.unchanged_mask, ptr->initval.unchanged_mask);
	printf("%sinitval.forced_bits\t\t0x%08x  (%u)\n", prefix,
		ptr->initval.forced_bits, ptr->initval.forced_bits);
	printf("%sportvar_index\t\t\t%d\n", prefix, ptr->portvar_index);
	printf("%sslot_specific\t\t\t%s\n", prefix, yes_or_no(ptr->slot_specific));
	printf("%sreferenced\t\t\t%s\n", prefix, yes_or_no(ptr->referenced));
	printf("%svalid_address\t\t\t%s\n", prefix, yes_or_no(ptr->valid_address));
	printf("%sdata_size\t\t\t", prefix);
	decode_data_size(ptr->data_size);
	printf("\n");

	/***********************
	* Display config bits.
	***********************/
	printf("\n%sConfig_bits information:\n", prefix);
	display_config_bits(new_prefix, &ptr->config_bits);

	/***********************
	* Advance to the next ioport.
	***********************/
	if (mode == 1)
	    return;
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next ioport ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_switches
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_switches (prefix, switch_ptr, mode)
    char		*prefix;
    struct bswitch	*switch_ptr;
    int			mode;
{
    struct bswitch	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no switch information.
    ***********************/
    ptr = switch_ptr;
    if (ptr == 0) {
	printf("%sNo valid switch information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the switches and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%scomments\t\t%s\n", prefix, ptr->comments);
	printf("%shelp\t\t\t%s\n", prefix, ptr->help);
	printf("%sinitval.reserved_mask\t0x%08x   (%u)\n", prefix,
		ptr->initval.reserved_mask, ptr->initval.reserved_mask);
	printf("%sinitval.forced_bits\t0x%08x   (%u)\n", prefix,
		ptr->initval.forced_bits, ptr->initval.forced_bits);
	printf("%sfactory_bits\t\t0x%08x   (%u)\n", prefix,
		ptr->factory_bits, ptr->factory_bits);
	printf("%svertical\t\t%s\n", prefix, yes_or_no(ptr->vertical));
	printf("%sreverse\t\t\t%s\n", prefix, yes_or_no(ptr->reverse));
	printf("%swidth\t\t\t%d\n", prefix, ptr->width);
	printf("%sindex\t\t\t%d\n", prefix, ptr->index);
	printf("%stype\t\t\t", prefix);
	switch (ptr->type) {
	    case st_dip:
		printf("dip\n");
		break;
	    case st_rotary:
		printf("rotary\n");
		break;
	    case st_slide:
		printf("slide\n");
		break;
	}

	/***********************
	* Display label.
	***********************/
	printf("\n%sLabel information:\n", prefix);
	display_labels(new_prefix, ptr->labels);

	/***********************
	* Display config_bits.
	***********************/
	printf("\n%sConfig_bits information:\n", prefix);
	display_config_bits(new_prefix, &ptr->config_bits);

	/***********************
	* Advance to the next switch.
	***********************/
	if (mode == 1)
	    return;
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next switch ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_jumpers
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_jumpers (prefix, jumper_ptr, mode)
    char		*prefix;
    struct jumper	*jumper_ptr;
    int			mode;
{
    struct jumper	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no jumper information.
    ***********************/
    ptr = jumper_ptr;
    if (ptr == 0) {
	printf("%sNo valid jumper information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the jumpers and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%sname\t\t\t%s\n", prefix, ptr->name);
	printf("%scomments\t\t%s\n", prefix, ptr->comments);
	printf("%shelp\t\t\t%s\n", prefix, ptr->help);
	printf("%sinitval.reserved_mask\t0x%08x   (%u)\n", prefix,
		ptr->initval.reserved_mask, ptr->initval.reserved_mask);
	printf("%sinitval.forced_bits\t0x%08x   (%u)\n", prefix,
		ptr->initval.forced_bits, ptr->initval.forced_bits);
	printf("%sinitval.tristate_bits\t0x%08x   (%u)\n", prefix,
		ptr->initval.tristate_bits, ptr->initval.tristate_bits);
	printf("%sfactory.data_bits\t0x%08x   (%u)\n", prefix,
		ptr->factory.data_bits, ptr->factory.data_bits);
	printf("%sfactory.tristate_bits\t0x%08x   (%u)\n", prefix,
		ptr->factory.tristate_bits, ptr->factory.tristate_bits);
	printf("%svertical\t\t%s\n", prefix, yes_or_no(ptr->vertical));
	printf("%sreverse\t\t\t%s\n", prefix, yes_or_no(ptr->reverse));
	printf("%swidth\t\t\t%d\n", prefix, ptr->width);
	printf("%sindex\t\t\t%d\n", prefix, ptr->index);
	printf("%son_post\t\t\t%s\n", prefix, yes_or_no(ptr->on_post));
	printf("%stype\t\t\t", prefix);
	switch (ptr->type) {
	    case jt_inline:
		printf("inline\n");
		break;
	    case jt_paired:
		printf("paired\n");
		break;
	    case jt_tripole:
		printf("tripole\n");
		break;
	}

	/***********************
	* Display label.
	***********************/
	printf("\n%sLabel information:\n", prefix);
	display_labels(new_prefix, ptr->labels);

	/***********************
	* Display config_bits.
	***********************/
	printf("\n%sConfig_bits information:\n", prefix);
	display_config_bits(new_prefix, &ptr->config_bits);

	/***********************
	* Display tristate_bits.
	***********************/
	printf("\n%sTristate_bits information:\n", prefix);
	display_config_bits(new_prefix, &ptr->tristate_bits);

	/***********************
	* Advance to the next jumper.
	***********************/
	if (mode == 1)
	    return;
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next jumper ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_inits
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_inits (prefix, init_ptr)
    char		*prefix;
    struct init		*init_ptr;
{
    struct init		*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no init information.
    ***********************/
    ptr = init_ptr;
    if (ptr == 0) {
	printf("%sNo valid init information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the inits and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	printf("%stype\t\t\t", prefix);
	switch (ptr->type) {
	    case it_ioport:
		printf("ioport\n");
		break;
	    case it_jumper:
		printf("jumper\n");
		break;
	    case it_bswitch:
		printf("bswitch\n");
		break;
	    case it_software:
		printf("software\n");
		break;
	}

	printf("%sdata_type\t\t\t", prefix);
	switch (ptr->data_type) {
	    case dt_string:
		printf("string\n");
		break;
	    case dt_tripole:
		printf("tripole\n");
		break;
	    case dt_value:
		printf("value\n");
		break;
	}

	printf("%srange.min\t\t\t%u\n", prefix, ptr->range.min);
	printf("%srange.max\t\t\t%u\n", prefix, ptr->range.max);
	printf("%srange.current\t\t%u\n", prefix, ptr->range.current);
	printf("%smask\t\t\t%u\n", prefix, ptr->mask);
	printf("%smemory\t\t\t%s\n", prefix, yes_or_no(ptr->memory));

	/***********************
	* Display appropriate record we are initializing.
	***********************/
	printf("\n%s", prefix);
	switch (ptr->type) {
	    case it_ioport:
		printf("Ioport record:\n");
		display_ioports(new_prefix, ptr->ptr.ioport, 1);
		break;
	    case it_jumper:
		printf("Jumper record:\n");
		display_jumpers(new_prefix, ptr->ptr.jumper, 1);
		break;
	    case it_bswitch:
		printf("Switch record:\n");
		display_switches(new_prefix, ptr->ptr.bswitch, 1);
		break;
	    case it_software:
		printf("Software record:\n");
		display_softwares(new_prefix, ptr->ptr.software, 1);
		break;
	}

	/***********************
	* Display appropriate initialization values.
	***********************/
	printf("\n%sInit_values: \n", prefix);
	display_init_values(new_prefix, ptr->init_values, ptr->data_type);
	printf("\n%sCurrent: \n", prefix);
	display_init_values(new_prefix, ptr->current, ptr->data_type);

	/***********************
	* Advance to the next init.
	***********************/
	ptr = ptr->next;
	if ((int)ptr != 0)
	    printf("\n\n%s*** next init ***\n\n", prefix);

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_init_values
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_init_values (prefix, init_value_ptr, data_type)
    char		*prefix;
    struct init_value	*init_value_ptr;
    enum data_type	data_type;
{
    struct init_value	*ptr;


    /***********************
    * Return immediately if there is no init_value information.
    ***********************/
    ptr = init_value_ptr;
    if (ptr == 0) {
	printf("%sNo valid init_value information!\n", prefix);
        return;
    }

    /**********************
    * Walk through each of the init_values and display information for each.
    * Stop when we get to end-of-chain (0 in next field).
    **********************/
    do {

	/***********************
	* First, print the stuff that can be handled in this routine.
	***********************/
	switch (data_type) {
	    case dt_string:
		printf("%su.parameter\t\t%s\n", prefix, ptr->u.parameter);
		break;
	    case dt_tripole:
		printf("%su.tripole.data_bits\t%u\n", prefix, ptr->u.tripole.data_bits);
		printf("%su.tripole.tristate_bits\t%u\n", prefix, ptr->u.tripole.tristate_bits);
		break;
	    case dt_value:
		printf("%su.value\t\t\t0x%08x   (%u)\n", prefix, ptr->u.value,
		       ptr->u.value);
		break;
	}

	/***********************
	* Advance to the next init_value.
	***********************/
	ptr = ptr->next;

    } while ((int)ptr != 0);

    return;
}

/****+++***********************************************************************
*
* Function:     display_memory_address
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_memory_address (prefix, memory_address_ptr)
    char			*prefix;
    struct memory_address	*memory_address_ptr;
{
    struct memory_address	*ptr;
    char			new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no memory_address information.
    ***********************/
    ptr = memory_address_ptr;
    if (ptr == 0) {
	printf("%sNo valid memory_address information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sstep\t\t\t%u\n", prefix, ptr->step);
    printf("%sindex_value\t\t\t%d\n", prefix, ptr->index_value);
    if (ptr->step == 0) {
	printf("\n%sList.values information:\n", prefix);
	display_values(new_prefix, ptr->u.list.values);
	printf("\n%sList.config information:\n", prefix);
	display_values(new_prefix, ptr->u.list.config);
	printf("\n%sList.current information:\n", prefix);
	display_values(new_prefix, ptr->u.list.current);
    }
    else {
	printf("%su.range.min\t\t\t%u\n", prefix, ptr->u.range.min);
	printf("%su.range.max\t\t\t%u\n", prefix, ptr->u.range.max);
	printf("%su.range.config\t\t\t%u\n", prefix, ptr->u.range.config);
	printf("%su.range.current\t\t\t%u\n", prefix, ptr->u.range.current);
    }

    /***********************
    * Display index.
    ***********************/
    printf("%sindex:\n", prefix);
    display_index(new_prefix, &ptr->index);

    return;
}

/****+++***********************************************************************
*
* Function:     display_total_mem
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_total_mem (prefix, total_mem_ptr)
    char		*prefix;
    struct totalmem	*total_mem_ptr;
{
    struct totalmem	*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no total_mem information.
    ***********************/
    ptr = total_mem_ptr;
    if (ptr == 0) {
	printf("%sNo valid total_mem information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sstep\t\t\t%u\n", prefix, ptr->step);
    printf("%seeprom\t\t\t%u\n", prefix, ptr->eeprom);
    printf("%sactual\t\t\t%u\n", prefix, ptr->actual);

    if (ptr->step == 0) {
	printf("\n%sValues information:\n", prefix);
	display_values(new_prefix, ptr->u.values);
    }
    else {
	printf("%su.range.min\t\t\t%u\n", prefix, ptr->u.range.min);
	printf("%su.range.max\t\t\t%u\n", prefix, ptr->u.range.max);
    }

    return;
}

/****+++***********************************************************************
*
* Function:     display_ports
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    xx
*
****+++***********************************************************************/

void display_ports (prefix, port_ptr)
    char		*prefix;
    struct port		*port_ptr;
{
    struct port		*ptr;
    char		new_prefix[80];


    /**********************
    * Build an expanded prefix.
    **********************/
    (void)strcpy(new_prefix, prefix);
    (void)strcat(new_prefix, "    ");


    /***********************
    * Return immediately if there is no port information.
    ***********************/
    ptr = port_ptr;
    if (ptr == 0) {
	printf("%sNo valid port information!\n", prefix);
        return;
    }

    /***********************
    * First, print the stuff that can be handled in this routine.
    ***********************/
    printf("%sstep\t\t\t%u\n", prefix, ptr->step);
    printf("%sslot_specific\t\t\t%u\n", prefix, ptr->slot_specific);
    printf("%sindex_value\t\t\t%d\n", prefix, ptr->index_value);
    printf("%sdata_size\t\t\t", prefix);
    decode_data_size(ptr->data_size);
    printf("\n");

    if (ptr->step == 0) {
	printf("\n%sList.values information:\n", prefix);
	display_values(new_prefix, ptr->u.list.values);
	printf("\n%sList.config information:\n", prefix);
	display_values(new_prefix, ptr->u.list.config);
	printf("\n%sList.current information:\n", prefix);
	display_values(new_prefix, ptr->u.list.current);
    }
    else {
	printf("%su.range.min\t\t\t%u\n", prefix, ptr->u.range.min);
	printf("%su.range.max\t\t\t%u\n", prefix, ptr->u.range.max);
	printf("%su.range.count\t\t\t%u\n", prefix, ptr->u.range.count);
	printf("%su.range.config\t\t\t%u\n", prefix, ptr->u.range.config);
	printf("%su.range.current\t\t\t%u\n", prefix, ptr->u.range.current);
    }

    /***********************
    * Display index.
    ***********************/
    printf("\n%sindex:\n", prefix);
    display_index(new_prefix, &ptr->index);

    return;
}

/****+++***********************************************************************
*
* Function:     yes_or_no
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      "yes" if input was 1, "no" otherwise
*
* Description:
*
*    xx
*
****+++***********************************************************************/

char * yes_or_no (input)
    unsigned		input;
{
    if (input == 0)
	return("no");
    else if (input == 1)
	return("yes");
    else
	return("huh?");
}
#endif
