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
*                            src/emitter.c
*
*   Each function in emitter.c is called by yyparse upon
*   recognition of specific token or combination of tokens.
*   The CFG language syntax rules are contained in the file
*   cfg.y, which is the input file to the Abraxas Software
*   PCYACC compiler compiler.
*
**++***************************************************************************/


#define P_WARNING	5

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include "def.h"
#include "config.h"
#include "compat.h"
#include "compiler.h"
#include "err.h"

/***************
* type definitions used in this file
***************/
union rptr {
    struct resource 	*resource;
    struct dma 		*dma;
    struct irq 		*irq;
    struct port 	*port;
    struct memory 	*memory;
};

struct valuelist {
    struct valuelist 	*next;
    struct value 	*head;
    struct value 	*tail;
};

/*******************
*  These are all the current item being processed. All results end up
*  hung off of the global mptr.
*******************/
static struct board 		*bptr;
static struct bswitch 		*sptr;
static struct jumper 		*jptr;
static struct label 		*lptr;
static struct label 		*start_label;
static struct ioport 		*iptr;
static struct group 		*grptr;
static struct function 		*fptr;
static struct subfunction 	*sfptr;
static struct choice 		*chptr;
static struct subchoice 	*schptr;
static struct resource_group 	*rgptr;
static struct resource 	*resptr;
static struct dma 		*dmaptr;
static struct irq 		*irqptr;
static struct port 		*portptr;
static struct memory 		*memptr;

static struct valuelist		valuelist;
static struct valuelist		*rlptr;
static struct init 		*initptr;
static struct init_value 	*initvalueptr;
static struct value 		*valueptr;

static char			*format_string();

/*******************
* These are attributes which must be present in any CFG file (required).
*******************/
static struct {
    unsigned	choice:1;
    unsigned	function:1;
    unsigned	board_name:1;
    unsigned	board_id:1;
    unsigned	board_mfr:1;
    unsigned	board_cat:1;
    unsigned	ctrl_name:1;
    unsigned	ctrl_type:1;
} required;

static unsigned long	loc_list;
static int 		vs_incr;
static int 		vs_index;
static int 		value_count;
static int 		init_val_count;
static int 		last_loc;
static int 		num_loc;
static int 		loc_descending;
static int 		loc_ascending;
static int 		eisaport;
static int 		loc_flag;
static int 		curr_slot;
static int 		ctrl_num;
static int 		first_pass;
static int 		memory_flag;
static int 		memory_overflow;
static int 		sizing_flag;
static int 		eisa_slot;
static int 		not_eisa_slot;
static int 		freeform_total;
static int 		freeform_count;
static int 		freeform_flag;
static int 		loc_pair_flag;
static int 		system_flag;

static char 		*slot_label  = "Slot ";
static char 		*emb_label   = "Embedded";
static char 		*system_slot = "System";
static char 		*null_string = "_";


/***************
* Globals declared in globals.c
***************/
extern struct system 	*mptr;
extern int 		parse_err_flag;
extern int 		emb_char_removed;
extern char 		*sourcefn;
extern unsigned 	lineno;
extern struct pmode	program_mode;
extern int              parse_err_no_print;
extern int  		emb_char_removed;


/**************
* External function used in this file.
**************/
extern void 	mn_trapfree();
extern void 	*mn_trapcalloc();


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
* Used:		internal only
*
* Returns:      the modified string (the input string is also modified)
*
* Description:
*
*    This function removes all \n and \t characters from the input string or
*    converts those characters.
*
****+++***********************************************************************/

static char *format_string(ptr, operation)
    char	*ptr;
    int 	operation;
{
    char    	temp[1000];
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
* Function:     parse_errmsg()
*
* Parameters:   err_code
*		level
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

void parse_errmsg(err_code, level)
    int		err_code;
    int 	level;
{


    parse_err_flag = (level > 1);

    /***************
    * Save away the filename and line number of the error.
    ***************/
    err_add_string_parm(1, sourcefn);
    err_add_num_parm(1, lineno);

    /***************
    * If we are in checkcfg mode, display the real error.
    ***************/
    if (program_mode.checkcfg) {
	err_handler(PARSE76_ERRCODE);
	err_handler(err_code);
    }

    /**************
    * Otherwise, if this is not a warning and errors are to be displayed,
    * do so. There are two classes of errors that are handled here:
    *   (1) Errors that occur because the grammar was not followed.
    *   (2) Errors that occur beacuse of the existing configuration. The only
    *       error of this class is PARSE10_ERRCODE, which happens when
    *       someone tries to specify a second system board.
    *************/
    else if ( (parse_err_flag) && (parse_err_no_print == 0) ) {
	if (err_code == PARSE10_ERRCODE)
	    err_handler(PARSE78_ERRCODE);
	else
	    err_handler(PARSE0_ERRCODE);
    }

}






/****+++***********************************************************************
*
* Function:     allocate()
*
* Parameters:   size              
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

void	*allocate(size)
    unsigned    size;
{
    return(mn_trapcalloc(1, size));
}






/****+++***********************************************************************
*
* Function:     free_valuelist()
*
* Parameters:   rlist             
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

void free_valuelist(rlist)
    struct valuelist *rlist;
{
    if (rlist->next != NULL)
	free_valuelist(rlist->next);
    free((void *)rlist);
}



/****+++***********************************************************************
*
* Function:     process_string()
*
* Parameters:   operation		SHORT_STRING or LONG_STRING
*		string
*		max_len
*		null_replace
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

char *process_string(operation, string, max_len, null_replace)
    int 	operation;
    char	*string;
    int 	max_len;
    int 	null_replace;
{
    char	*format_string();


    if (strlen(string) > max_len) {
	*(string + max_len) = 0;
	parse_errmsg(PARSE48_ERRCODE, 1);
    }

    if (operation == SHORT_STRING) {
	if (null_replace && strlen(string) == 0) {
	    string = (char *) allocate(strlen(null_string) + 1);
	    (void)strcpy(string, null_string);
	}
	(void) format_string(string, REMOVE_EMB_CHAR);
	if (emb_char_removed)
	    parse_errmsg(PARSE8_ERRCODE, 1);
    }
    else
	(void) format_string(string, CONVERT_EMB_CHAR);

    return(string);
}



/****+++***********************************************************************
*
* Function:     beg_board()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*     Allocate board structure and initialize.
*
****+++***********************************************************************/

void beg_board()
{
    struct board 	*brd;

    parse_err_flag = 0;
    brd = (struct board *) allocate(sizeof(struct board ));

    if (mptr->boards == NULL)
	bptr = mptr->boards = brd;
    else {
	bptr = mptr->boards;
	while (bptr->next != NULL)
	    bptr = bptr->next;
	bptr = bptr->next = brd;
    }

    bptr->slot_number = bptr->eisa_slot = -1;
    bptr->slot = bs_isa16;
    bptr->length = DEFAULT_BOARD_LENGTH;
    vs_incr = 0;
    vs_index = 0;
    init_val_count = 0;
    eisaport = 0;
    first_pass = 0;
    memory_flag = 0;
    memory_overflow = 0;
    sizing_flag = 0;
    system_flag = 0;
    required.choice = required.function = 0;
    required.board_name = required.board_id = required.board_mfr = required.board_cat = 0;
    required.ctrl_name = required.ctrl_type = 0;
    valuelist.head = valuelist.tail = NULL;
    valuelist.next = NULL;
}



/****+++***********************************************************************
*
* Function:     board_id()
*
* Parameters:   id
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

void board_id(id)
    char	*id;
{
    int 	i, err;


    bptr->id = strupr(id);
    err = (strlen(id) != 7);

    for (i = 0; i < 3; i++)
	err |= (*(bptr->id + i) < 0x40
	     || (*(bptr->id + i) > 0x5A
	     && *(bptr->id + i) != 0x5E
	     && *(bptr->id + i) != 0x5F));

    for (i = 3; i < 7; i++)
	err |= !(isxdigit((int) *(bptr->id + i)));

    if (err)
	parse_errmsg(PARSE19_ERRCODE, 2);

    required.board_id = 1;
}






/****+++***********************************************************************
*
* Function:     board_name()
*
* Parameters:   name              
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

void board_name(name)
    char	*name;
{
    name = process_string(SHORT_STRING, name, NAME_MAX_LEN, 1);
    bptr->name = name;
    required.board_name = 1;
}





/****+++***********************************************************************
*
* Function:     board_mfr()
*
* Parameters:   mfr
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

void board_mfr(mfr)
    char	*mfr;
{
    mfr = process_string(SHORT_STRING, mfr, MFR_MAX_LEN, 0);
    bptr->mfr = mfr;
    required.board_mfr = 1;
}

/****+++***********************************************************************
*
* Function:     board_cat()
*
* Parameters:   cat
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

void board_cat(cat)
    char	*cat;
{

    if (strlen(cat) != 3)
	parse_errmsg(PARSE18_ERRCODE, 2);
    else
	bptr->category = strupr(cat);
    required.board_cat = 1;
}





/****+++***********************************************************************
*
* Function:     end_board_reqd()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    This function makes sure that all of the required board attributes have
*    been supplied by the CFG file.
*
****+++***********************************************************************/

void end_board_reqd()
{
    if (!required.board_id)
	parse_errmsg(PARSE5_ERRCODE, 2);
    if (!required.board_name)
	parse_errmsg(PARSE7_ERRCODE, 2);
    if (!required.board_mfr)
	parse_errmsg(PARSE6_ERRCODE, 2);
    if (!required.board_cat)
	parse_errmsg(PARSE4_ERRCODE, 2);
}



/****+++***********************************************************************
*
* Function:     board_slot()
*
* Parameters:   type
*		slot_num
*		tag
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
****+++***********************************************************************/

void board_slot(type, slotnum, tag)
    int 	type;
    int 	slotnum;
    char	*tag;
{

    if ((slotnum > EMB_SLOT_MAX) || ((slotnum < 0) && (slotnum != -1)))
	parse_errmsg(PARSE13_ERRCODE, 2);

    switch (type) {
	case ISA8_SLOT:
	    bptr->slot = bs_isa8;
	    break;
	case ISA16_SLOT:
	    bptr->slot = bs_isa16;
	    break;
	case ISA8OR16_SLOT:
	    bptr->slot = bs_isa8or16;
	    break;
	case EISA_SLOT:
	    bptr->slot = bs_eisa;
	    bptr->iocheck = bptr->disable = 1;
	    break;
	case OTH_SLOT:
	    bptr->slot = bs_oth;
	    break;
	case VIR_SLOT:
	    bptr->slot = bs_vir;
	    break;
	case EMB_SLOT:
	    bptr->slot = bs_emb;
	    bptr->eisa_slot = slotnum;
	    break;
    }

    if (tag != NULL) {
	tag = process_string(SHORT_STRING, tag, SLOT_TAG_MAX_LEN, 0);
	bptr->slot_tag = tag;
    }
}





/****+++***********************************************************************
*
* Function:     board_length()
*
* Parameters:   len
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

void board_length(len)
    unsigned long	len;
{
    if (len > BOARD_MAX_LEN)
	parse_errmsg(PARSE31_ERRCODE, 2);
    else
	bptr->length = len;
}



/****+++***********************************************************************
*
* Function:     board_skirt()
*
* Parameters:   skirt             
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

void board_skirt(skirt)
    unsigned long 	skirt;
{
    bptr->skirt = (int)skirt;
}





/****+++***********************************************************************
*
* Function:     board_readid()
*
* Parameters:   rdid              
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

void board_readid(rdid)
    unsigned long 	rdid;
{
    bptr->readid = (int)rdid;
}





/****+++***********************************************************************
*
* Function:     board_bmaster()
*
* Parameters:   val
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

void board_bmaster(val)
    unsigned long 	val;
{
    if (val > BUSMASTER_MAX)
	parse_errmsg(PARSE32_ERRCODE, 2);
    else
	bptr->busmaster = val;
}





/****+++***********************************************************************
*
* Function:     board_amps()
*
* Parameters:   val              
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

void board_amps(val)
    unsigned long	val;
{
    if (val > MAXIMUM_AMP)
	parse_errmsg(PARSE30_ERRCODE, 2);
    else
	bptr->amperage = val;
}



/****+++***********************************************************************
*
* Function:     board_iochk()
*
* Parameters:   val
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

void board_iochk(val)
    unsigned long 	val;
{
    bptr->iocheck = (int)val;
}





/****+++***********************************************************************
*
* Function:     board_disable()
*
* Parameters:   val              
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

void board_disable(val)
    unsigned long 	val;
{
    bptr->disable = (int)val;
}





/****+++***********************************************************************
*
* Function:     board_sizing()
*
* Parameters:   val              
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

void board_sizing(val)
    unsigned long	val;
{
    bptr->sizing = val;
    if (((bptr->slot == bs_emb) && (bptr->eisa_slot == 0))
	 || (val > mptr->default_sizing))
	mptr->default_sizing = val;
    sizing_flag = 1;
}





/****+++***********************************************************************
*
* Function:     board_cmmts()
*
* Parameters:   cmmts             
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

void board_cmmts(cmmts)
    char	*cmmts;
{
    cmmts = process_string(LONG_STRING, cmmts, COMM_MAX_LEN, 0);
    bptr->comments = cmmts;
}

/****+++***********************************************************************
*
* Function:     board_help()
*
* Parameters:   help              
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

void board_help(help)
    char	*help;
{
    help = process_string(LONG_STRING, help, HELP_MAX_LEN, 0);
    bptr->help = help;
}





/****+++***********************************************************************
*
* Function:     end_ctrl_reqd()
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

void end_ctrl_reqd()
{
    if (!required.ctrl_name)
	parse_errmsg(PARSE52_ERRCODE, 2);
    if (!required.ctrl_type)
	parse_errmsg(PARSE53_ERRCODE, 2);
}

/****+++***********************************************************************
*
* Function:     beg_switch()
*
* Parameters:   ndx 
*		numpos
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

void beg_switch(ndx, numpos)
    int 		ndx, numpos;
{
    struct bswitch 	*bsw;


    if ((ndx < 1) || (ndx > CTRL_NDX_MAX))
	parse_errmsg(PARSE22_ERRCODE, 2);
    if ((numpos < 1) || (numpos > 16))
	parse_errmsg(PARSE44_ERRCODE, 2);

    bsw = (struct bswitch *) allocate(sizeof(struct bswitch ));
    if (bptr->switches == NULL)
	sptr = bptr->switches = bsw;
    else {
	sptr = bptr->switches;
	if (sptr->index == ndx)
	    parse_errmsg(PARSE11_ERRCODE, 2);
	while (sptr->next != NULL) {
	    sptr = sptr->next;
	    if (sptr->index == ndx)
		parse_errmsg(PARSE11_ERRCODE, 2);
	}
	sptr = sptr->next = bsw;
    }

    sptr->index = ndx;
    sptr->width = numpos;
    sptr->config_bits.current = sptr->factory_bits = -1;
    sptr->config_bits.initial = -1;
    required.ctrl_name = required.ctrl_type = 0;
    loc_flag = SWITCH_BLOCK;
}





/****+++***********************************************************************
*
* Function:     beg_jumper()
*
* Parameters:   ndx 
*		numpos
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

void beg_jumper(ndx, numpos)
    int 		ndx, numpos;
{
    struct jumper 	*jmp;


    if (ndx < 1)
	parse_errmsg(PARSE22_ERRCODE, 2);
    if ((numpos < 1) || (numpos > 16))
	parse_errmsg(PARSE44_ERRCODE, 2);

    jmp = (struct jumper *) allocate(sizeof(struct jumper ));
    if (bptr->jumpers == NULL)
	jptr = bptr->jumpers = jmp;
    else {
	jptr = bptr->jumpers;
	if (jptr->index == ndx)
	    parse_errmsg(PARSE11_ERRCODE, 2);
	while (jptr->next != NULL) {
	    jptr = jptr->next;
	    if (jptr->index == ndx)
		parse_errmsg(PARSE11_ERRCODE, 2);
	}
	jptr = jptr->next = jmp;
    }

    jptr->index = ndx;
    jptr->width = numpos;
    jptr->config_bits.current = -1;
    jptr->config_bits.initial = -1;
    jptr->tristate_bits.initial = -1;
    jptr->factory.data_bits = jptr->factory.tristate_bits = -1;
    required.ctrl_name = required.ctrl_type = 0;
    loc_flag = JUMPER_BLOCK;
}

/****+++***********************************************************************
*
* Function:     ctrl_name()
*
* Parameters:   name              
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

void ctrl_name(name)
    char	*name;
{
    name = process_string(SHORT_STRING, name, CTRL_NAME_MAX_LEN, 0);
    if (loc_flag == SWITCH_BLOCK)
	sptr->name = name;
    else
	jptr->name = name;
    required.ctrl_name = 1;
}





/****+++***********************************************************************
*
* Function:     switch_type()
*
* Parameters:   type              
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

void switch_type(type)
    int 	type;
{
    switch (type) {
	case DIP_TYPE:
	    sptr->type = st_dip;
	    break;
	case ROTARY_TYPE:
	    sptr->type = st_rotary;
	    break;
	case SLIDE_TYPE:
	    sptr->type = st_slide;
	    break;
    }
    required.ctrl_type = 1;
}





/****+++***********************************************************************
*
* Function:     jumper_type()
*
* Parameters:   type              
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

void jumper_type(type)
    int 	type;
{
    switch (type) {
	case INLINE_TYPE:
	    jptr->type = jt_inline;
	    break;
	case PAIRED_TYPE:
	    jptr->type = jt_paired;
	    break;
	case TRIPOLE_TYPE:
	    jptr->type = jt_tripole;
	    break;
    }
    required.ctrl_type = 1;
}



/****+++***********************************************************************
*
* Function:     ctrl_vert()
*
* Parameters:   vert              
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

void ctrl_vert(vert)
    int 	vert;
{
    if (loc_flag == SWITCH_BLOCK) {
	if (sptr->type != st_rotary)
	    sptr->vertical = vert;
    }
    else
	jptr->vertical = vert;
}





/****+++***********************************************************************
*
* Function:     ctrl_rev()
*
* Parameters:   rev              
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

void ctrl_rev(rev)
    int 	rev;
{
    if (loc_flag == SWITCH_BLOCK)
	sptr->reverse = rev;
    else
	jptr->reverse = rev;
}



/****+++***********************************************************************
*
* Function:     beg_label()
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

void beg_label()
{
    loc_pair_flag = 0;
    if (loc_flag == SWITCH_BLOCK)
	ctrl_num = sptr->width;
    else if (jptr->type == jt_inline)
	ctrl_num = jptr->width + 1;
    else
	ctrl_num = jptr->width;
    start_label = NULL;
}





/****+++***********************************************************************
*
* Function:     lbl_pos()
*
* Parameters:   pos               
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

void lbl_pos(pos)
    int 		pos;
{
    struct label 	*lbl;


    if (jptr->type == jt_inline) {
	if ((pos < 1) || (pos > ctrl_num + 1))
	    parse_errmsg(PARSE29_ERRCODE, 2);
    }
    else {
	if ((pos < 1) || (pos > ctrl_num))
	    parse_errmsg(PARSE29_ERRCODE, 2);
    }

    lbl = (struct label *) allocate(sizeof(struct label ));
    if (start_label == NULL)
	lptr = start_label = lbl;
    else
	lptr = lptr->next = lbl;
    lptr->position = pos;
}




/****+++***********************************************************************
*
* Function:     lbl_range()
*
* Parameters:   pos1
*		pos2
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

void lbl_range(pos1, pos2)
    int 	pos1, pos2;
{
    int 	n;


    if ((pos1 < 1) || (pos2 < 1) || (pos1 > ctrl_num) || (pos2 > ctrl_num))
	parse_errmsg(PARSE29_ERRCODE, 2);

    lptr = start_label = (struct label *) allocate(sizeof(struct label ));
    lptr->position = pos1;
    n = pos1;
    while (n != pos2) {
	if (pos1 < pos2)
	    n += 1;
	else
	    n -= 1;
	lptr = lptr->next = (struct label *) allocate(sizeof(struct label ));
	lptr->position = n;
    }
}

/****+++***********************************************************************
*
* Function:     jmp_pair()
*
* Parameters:   pos1
*		pos2
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Handles INLINE LABEL LOC statements.
*
****+++***********************************************************************/

void jmp_pair(pos1, pos2)
    int 		pos1, pos2;
{
    struct label 	*lbl;


    if (parse_err_flag)
	return;

    if ((pos1 < 1) || (pos2 < 1) || (pos1 > ctrl_num + 1) || (pos2 > ctrl_num + 1))
	parse_errmsg(PARSE29_ERRCODE, 2);
    if ((pos1 != pos2 - 1) && (pos1 != pos2 + 1))
	parse_errmsg(PARSE23_ERRCODE, 2);

    lbl = (struct label *) allocate(sizeof(struct label ));
    if (start_label == NULL)
	lptr = start_label = lbl;
    else
	lptr = lptr->next = lbl;

    if (pos1 < pos2)
	lptr->position = pos1;
    else
	lptr->position = pos2;
}





/****+++***********************************************************************
*
* Function:     end_label_list()
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

void end_label_list()
{
    if (loc_flag == SWITCH_BLOCK)
	sptr->labels = lptr = start_label;
    else
	jptr->labels = lptr = start_label;
}





/****+++***********************************************************************
*
* Function:     ctrl_label()
*
* Parameters:   label             
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Processes strings in jumper/switch labels.
*
****+++***********************************************************************/

void ctrl_label(label)
    char	*label;
{
    if (lptr == NULL)
	parse_errmsg(PARSE28_ERRCODE, 2);
    else {
	label = process_string(SHORT_STRING, label, LABEL_MAX_LEN, 0);
	lptr->string = label;
	lptr = lptr->next;
    }
}

/****+++***********************************************************************
*
* Function:     end_label()
*
* Parameters:   None              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Called at the end of a switch/jumper label.
*
****+++***********************************************************************/

void end_label()
{
    if (lptr != NULL)
	parse_errmsg(PARSE28_ERRCODE, 2);
    if (loc_flag == SWITCH_BLOCK)
	sptr->labels = start_label;
    else {
	jptr->labels = start_label;
	jptr->on_post = (jptr->type == jt_inline && !loc_pair_flag);
    }
}






/****+++***********************************************************************
*
* Function:     beg_loc_list()
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

void beg_loc_list()
{
    loc_list = 0;
    num_loc = 0;
    last_loc = -1;
    loc_descending = 0;
    loc_ascending = 0;
}





/****+++***********************************************************************
*
* Function:     clear_loc_mask()
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

void clear_loc_mask()
{
    initptr->mask = 0;
}

/****+++***********************************************************************
*
* Function:     loc_val()
*
* Parameters:   val              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Switch/jumper/ioport LOC processing.
*
****+++***********************************************************************/

void loc_val(val)
    int		 	val;
{
    unsigned long   	n;
    int 		num;


    if (parse_err_flag)
	return;

    n = BIT0;
    switch (loc_flag) {

	case SWITCH_BLOCK:
	    if ((val < 1) || (val > sptr->width))
		parse_errmsg(PARSE29_ERRCODE, 2);

	    if (sptr->reverse)
		n <<= (sptr->width - val);
	    else
		n <<= (val - 1);

	    if (last_loc != -1) {
		loc_descending |= (val < last_loc);
		loc_ascending |= (val > last_loc);
	    }

	    loc_list |= n;
	    num_loc += 1;
	    break;

	case JUMPER_BLOCK:
	    if ((val < 1) || (val > jptr->width))
		parse_errmsg(PARSE29_ERRCODE, 2);

	    if (jptr->reverse)
		n <<= (jptr->width - val);
	    else
		n <<= (val - 1);

	    if (last_loc != -1) {
		loc_descending |= (val < last_loc);
		loc_ascending |= (val > last_loc);
	    }

	    loc_list |= n;
	    num_loc += 1;
	    break;

	case IOPORT_BLOCK:
	    switch (iptr->data_size) {
		case ds_byte:
		    num = 7;
		    break;
		case ds_word:
		    num = 15;
		    break;
		case ds_dword:
		    num = 31;
		    break;
	    }

	    if ((val < 0) || (val > num))
		parse_errmsg(PARSE29_ERRCODE, 2);
	    if ((last_loc != -1) && (val > last_loc))
		parse_errmsg(PARSE24_ERRCODE, 2);

	    n <<= val;
	    loc_list |= n;
	    num_loc += 1;
	    break;
    }

    if (((loc_descending) && (loc_ascending)) || (last_loc == val))
	parse_errmsg(PARSE24_ERRCODE, 2);

    last_loc = val;
}

/****+++***********************************************************************
*
* Function:     loc_range()
*
* Parameters:   val1
*		val2
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

void loc_range(val1, val2)
    int 	val1, val2;
{
    int 	i;

    if (val1 == val2)
	parse_errmsg(PARSE24_ERRCODE, 2);

    if (val1 < val2)
	for (i = val1; i <= val2; i++)
	    loc_val(i);
    else
	for (i = val1; i >= val2; i--)
	    loc_val(i);
}





/****+++***********************************************************************
*
* Function:     loc_pair()
*
* Parameters:   val1
*		val2
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    LOC processing for inline jumpers.
*
****+++***********************************************************************/

void loc_pair(val1, val2)
    int 		val1, val2;
{
    unsigned long   	n;
    int 		val;


    if (parse_err_flag)
	return;

    n = BIT0;
    if ((val1 < 1) || (val2 < 1) || (val1 > jptr->width + 1) || (val2 > jptr->width + 1))
	parse_errmsg(PARSE29_ERRCODE, 2);
    if ((val1 != val2 - 1) && (val1 != val2 + 1))
	parse_errmsg(PARSE23_ERRCODE, 2);

    val = (val1 < val2) ? val1 : val2;
    if (jptr->reverse)
	n <<= (jptr->width - val);
    else
	n <<= (val - 1);

    if (last_loc != -1) {
	loc_descending |= (val < last_loc);
	loc_ascending |= (val > last_loc);
    }

    loc_list |= n;
    num_loc += 1;
    if (((loc_descending) && (loc_ascending)) || (last_loc == val))
	parse_errmsg(PARSE24_ERRCODE, 2);
    last_loc = val;
}





/****+++***********************************************************************
*
* Function:     end_pair()
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

void end_pair()
{
    if (jptr->type != jt_inline)
	parse_errmsg(PARSE24_ERRCODE, 2);
    loc_pair_flag = 1;
}

/****+++***********************************************************************
*
* Function:     end_initv_list()
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

void end_initv_list()
{
    if (loc_flag == SWITCH_BLOCK)
	sptr->initval.reserved_mask = loc_list;
    else
	jptr->initval.reserved_mask = loc_list;
}





/****+++***********************************************************************
*
* Function:     sw_initval()
*
* Parameters:   vals              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Processes the SWITCH INITVAL statement.
*
****+++***********************************************************************/

void sw_initval(vals)
    char		*vals;
{
    char    		*valptr;
    unsigned long   	n;
    unsigned		sr_flag = 0;
    unsigned		sr_found = 0;


    valptr = vals;

    if ((sptr->type == st_slide) || (sptr->type == st_rotary)) {
	sr_flag = 1;
	sptr->initval.reserved_mask = 0xFFFF;
    }

    if (strlen(vals) != num_loc) {
	parse_errmsg(PARSE28_ERRCODE, 2);
	return;
    }

    if (((sptr->reverse) && (loc_descending)) || ((!sptr->reverse) && (loc_ascending)))
	vals = strrev(vals);

    for (n = BIT15; n != 0; n >>= 1)
	if (loc_list & n) {
	    switch (*vals) {
		case '1':
		    if (sr_flag && sr_found)
			parse_errmsg(PARSE47_ERRCODE, 2);
		    if (sr_flag)
			sr_found = 1;
		    sptr->initval.forced_bits |= n;
		    break;
		case '0':
		    break;
		case 'X':
		    sptr->initval.reserved_mask &= ~n;
		    break;
		case 'N':
		    parse_errmsg(PARSE42_ERRCODE, 2);
		    break;
	    }
	    vals += 1;
	    if (parse_err_flag)
		return;
	}

    if (sr_flag && !sr_found)
	parse_errmsg(PARSE47_ERRCODE, 2);

    mn_trapfree((void *)valptr);
}



/****+++***********************************************************************
*
* Function:     jmp_initval()
*
* Parameters:   vals              
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

void jmp_initval(vals)
    char		*vals;
{
    char    		*valptr;
    unsigned long   	n;


    valptr = vals;
    if (strlen(vals) != num_loc) {
	parse_errmsg(PARSE28_ERRCODE, 2);
	return;
    }

    if (((jptr->reverse) && (loc_descending)) || ((!jptr->reverse) && (loc_ascending)))
	vals = strrev(vals);

    for (n = BIT15; n != 0; n >>= 1)
	if (loc_list & n) {
	    switch (*vals) {
		case '1':
		    jptr->initval.forced_bits |= n;
		    break;
		case '0':
		    break;
		case 'X':
		    jptr->initval.reserved_mask &= ~n;
		    break;
		case 'N':
		    if (jptr->type != jt_tripole)
			parse_errmsg(PARSE42_ERRCODE, 2);
		    else
			jptr->initval.tristate_bits |= n;
		    break;
	    }
	    vals += 1;
	    if (parse_err_flag)
		return;
	}

    mn_trapfree((void *)valptr);
}



/****+++***********************************************************************
*
* Function:     sw_fact()
*
* Parameters:   vals              
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

void sw_fact(vals)
    char		*vals;
{
    char    		*valptr;
    unsigned long   	n;
    unsigned		sr_flag = 0;
    unsigned		sr_found = 0;


    valptr = vals;
    sptr->factory_bits = 0;
    if ((sptr->type == st_slide) || (sptr->type == st_rotary))
	sr_flag = 1;

    if (strlen(vals) != num_loc) {
	parse_errmsg(PARSE28_ERRCODE, 2);
	return;
    }

    if (((sptr->reverse) && (loc_descending)) || ((!sptr->reverse) && (loc_ascending)))
	vals = strrev(vals);

    for (n = BIT15; n != 0; n >>= 1)
	if (loc_list & n) {
	    switch (*vals) {
		case '1':
		    if (sr_flag && sr_found)
			parse_errmsg(PARSE47_ERRCODE, 2);
		    if (sr_flag)
			sr_found = 1;
		    sptr->factory_bits |= n;
		    break;
		case '0':
		    break;
		case 'X':
		    parse_errmsg(PARSE59_ERRCODE, 2);
		    return;
		case 'N':
		    parse_errmsg(PARSE42_ERRCODE, 2);
		    break;
	    }
	    vals += 1;
	    if (parse_err_flag)
		return;
	}
    
    mn_trapfree((void *)valptr);
}



/****+++***********************************************************************
*
* Function:     jmp_fact()
*
* Parameters:   vals              
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

void jmp_fact(vals)
    char		*vals;
{
    char    		*valptr;
    unsigned long   	n;


    valptr = vals;
    jptr->factory.data_bits = jptr->factory.tristate_bits = 0;
    if (strlen(vals) != num_loc) {
	parse_errmsg(PARSE28_ERRCODE, 2);
	return;
    }

    if (((jptr->reverse) && (loc_descending)) || ((!jptr->reverse) && (loc_ascending)))
	vals = strrev(vals);

    for (n = BIT15; n != 0; n >>= 1)
	if (loc_list & n) {
	    switch (*vals) {
		case '1':
		    jptr->factory.data_bits |= n;
		    break;
		case '0':
		    break;
		case 'X':
		    parse_errmsg(PARSE59_ERRCODE, 2);
		    break;
		case 'N':
		    if (jptr->type != jt_tripole)
			parse_errmsg(PARSE42_ERRCODE, 2);
		    else
			jptr->factory.tristate_bits |= n;
		    break;
	    }
	    vals += 1;
	    if (parse_err_flag)
		return;
	}
    
    mn_trapfree((void *)valptr);
}





/****+++***********************************************************************
*
* Function:     ctrl_cmmts()
*
* Parameters:   comm              
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

void ctrl_cmmts(comm)
    char	*comm;
{
    comm = process_string(LONG_STRING, comm, COMM_MAX_LEN, 0);
    if (loc_flag == SWITCH_BLOCK)
	sptr->comments = comm;
    else
	jptr->comments = comm;
}

/****+++***********************************************************************
*
* Function:     prog_port()
*
* Parameters:   ndx 
*		pvndx
*		addr
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

void prog_port(ndx, pvndx, addr)
    int 		ndx, pvndx;
    unsigned long	addr;
{
    struct ioport 	*iop;


    if ((ndx < 1) && (ndx != -1))
	parse_errmsg(PARSE22_ERRCODE, 2);
    if (addr > MAX_PORT)
	parse_errmsg(PARSE27_ERRCODE, 2);

    iop = (struct ioport *) allocate(sizeof(struct ioport ));
    if (bptr->ioports == NULL)
	iptr = bptr->ioports = iop;
    else {
	iptr = bptr->ioports;
	if ((iptr->index == ndx) && (ndx != -1))
	    parse_errmsg(PARSE11_ERRCODE, 2);
	if (ndx > CTRL_NDX_MAX)
	    parse_errmsg(PARSE22_ERRCODE, 2);
	while (iptr->next != NULL)	/* Check for duplicate index. */ {
	    iptr = iptr->next;
	    if ((iptr->index == ndx) && (ndx != -1))
		parse_errmsg(PARSE11_ERRCODE, 2);
	}
	iptr = iptr->next = iop;
    }

    if (!parse_err_flag) {
	iptr->index = ndx;
	iptr->address = addr;
	if (eisaport)
	    iptr->slot_specific = 1;
	eisaport = 0;
	iptr->data_size = ds_byte;
	iptr->portvar_index = pvndx;
	iptr->valid_address = (pvndx == 0);
	iptr->config_bits.current = -1;
	loc_flag = IOPORT_BLOCK;
    }
}





/****+++***********************************************************************
*
* Function:     ioport_size()
*
* Parameters:   val               
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

void ioport_size(val)
    int 	val;
{
    switch (val) {
	case 8:
	    iptr->data_size = ds_byte;
	    break;
	case 16:
	    iptr->data_size = ds_word;
	    if (iptr->index == -1)
		initptr->mask = 0xFFFF;
	    break;
	case 32:
	    iptr->data_size = ds_dword;
	    if (iptr->index == -1)
		initptr->mask = 0xFFFFFFFF;
	    break;
    }

}


/****+++***********************************************************************
*
* Function:     io_initval()
*
* Parameters:   vals              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Handles the IOPORT INITVAL statement.
*
****+++***********************************************************************/

void io_initval(vals)
    char		*vals;
{
    char    		*valptr;
    unsigned long   	mask, n;
    enum data_size 	size;


    valptr = vals;
    if (loc_list != 0) {
	mask = loc_list;
	if (strlen(vals) != num_loc)
	    parse_errmsg(PARSE28_ERRCODE, 2);
    }

    else {
	if (strlen(vals) == 8) {
	    mask = 0xFF;
	    size = ds_byte;
	}
	else if (strlen(vals) == 16) {
	    mask = 0xFFFF;
	    size = ds_word;
	}
	else if (strlen(vals) == 32) {
	    mask = 0xFFFFFFFF;
	    size = ds_dword;
	}
	else {
	    parse_errmsg(PARSE58_ERRCODE, 2);
	    return;
	}
	if (size != iptr->data_size)
	    parse_errmsg(PARSE3_ERRCODE, 2);
    }

    if (parse_err_flag)
	return;

    for (n = BIT31; n != 0; n >>= 1)
	if (mask & n) {
	    switch (*vals) {
		case '1':
		    iptr->initval.reserved_mask |= n;
		    iptr->initval.forced_bits |= n;
		    break;
		case '0':
		    iptr->initval.reserved_mask |= n;
		    break;
		case 'R':
		    iptr->initval.reserved_mask |= n;
		    iptr->initval.unchanged_mask |= n;
		    break;
		case 'X':
		    break;
	    }
	    vals += 1;
	}
    
    mn_trapfree((void *)valptr);
}



/****+++***********************************************************************
*
* Function:     sftware()
*
* Parameters:   ndx 
*		stmt
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

void sftware(ndx, stmt)
    int 		ndx;
    char		*stmt;
{
    struct software 	*soft;
    struct software     *swptr;

    if ((ndx < 1) || (ndx > CTRL_NDX_MAX))
	parse_errmsg(PARSE22_ERRCODE, 2);

    soft = (struct software *) allocate(sizeof(struct software ));
    if (bptr->softwares == NULL)
	swptr = bptr->softwares = soft;
    else {
	swptr = bptr->softwares;
	if (swptr->index == ndx)
	    parse_errmsg(PARSE11_ERRCODE, 2);
	while (swptr->next != NULL) {
	    swptr = swptr->next;
	    if (swptr->index == ndx)
		parse_errmsg(PARSE11_ERRCODE, 2);
	}
	swptr = swptr->next = soft;
    }

    stmt = process_string(LONG_STRING, stmt, SOFT_MAX_LEN, 0);
    swptr->description = stmt;
    swptr->index = ndx;
}



/****+++***********************************************************************
*
* Function:     beg_group()
*
* Parameters:   type
*		name
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

void beg_group(type, name)
    unsigned int	type;
    char		*name;
{
    struct group 	*grp;


    grp = (struct group *) allocate(sizeof(struct group ));
    if (bptr->groups == NULL)
	grptr = bptr->groups = grp;
    else {
	grptr = bptr->groups;
	while (grptr->next != NULL)
	    grptr = grptr->next;
	grptr = grptr->next = grp;
    }

    grptr->parent = bptr;
    if (grptr->explicit = type) {
	vs_index += vs_incr;
	name = process_string(SHORT_STRING, name, GROUP_MAX_LEN, 0);
	grptr->name = name;
    }
}





/****+++***********************************************************************
*
* Function:     group_type()
*
* Parameters:   type              
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

void group_type(type)
    char	*type;
{
    type = process_string(SHORT_STRING, type, TYPE_MAX_LEN, 0);
    grptr->type = strupr(type);
}





/****+++***********************************************************************
*
* Function:     end_group()
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

void end_group()
{
    grptr = NULL;
}

/****+++***********************************************************************
*
* Function:     beg_func()
*
* Parameters:   name              
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

void beg_func(name)
    char		*name;
{
    struct function 	*func;


    if ((bptr->groups == NULL) || (grptr == NULL))
	beg_group(0, "");

    func = (struct function *) allocate(sizeof(struct function ));
    if (grptr->functions == NULL)
	fptr = grptr->functions = func;
    else {
	fptr = grptr->functions;
	while (fptr->next != NULL)
	    fptr = fptr->next;
	fptr = fptr->next = func;
    }

    fptr->parent = grptr;
    vs_index += vs_incr;
    name = process_string(SHORT_STRING, name, FUNC_MAX_LEN, 1);
    fptr->name = name;
    required.function = 1;
    fptr->display = 1;
}





/****+++***********************************************************************
*
* Function:     func_type()
*
* Parameters:   type              
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

void func_type(type)
    char	*type;
{
    type = process_string(SHORT_STRING, type, TYPE_MAX_LEN, 0);
    fptr->type = strupr(type);
}





/****+++***********************************************************************
*
* Function:     func_cmmts()
*
* Parameters:   cmmts             
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

void func_cmmts(cmmts)
    char	*cmmts;
{
    cmmts = process_string(LONG_STRING, cmmts, COMM_MAX_LEN, 0);
    fptr->comments = cmmts;
}

/****+++***********************************************************************
*
* Function:     func_help()
*
* Parameters:   help              
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

void func_help(help)
    char	*help;
{
    help = process_string(LONG_STRING, help, HELP_MAX_LEN, 0);
    fptr->help = help;
}





/****+++***********************************************************************
*
* Function:     func_connect()
*
* Parameters:   connect           
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

void func_connect(connect)
    char	*connect;
{
    connect = process_string(LONG_STRING, connect, CONNECT_MAX_LEN, 0);
    fptr->connection = connect;
}





/****+++***********************************************************************
*
* Function:     func_display()
*
* Parameters:   display
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

void func_display(display)
    unsigned long 	display;
{
    fptr->display = (int)display;
}



/****+++***********************************************************************
*
* Function:     beg_subfunc()
*
* Parameters:   type
*		name
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

void beg_subfunc(type, name)
    unsigned int	type;
    char		*name;
{
    struct subfunction *subfunc;


    ++bptr->entries;
    subfunc = (struct subfunction *) allocate(sizeof(struct subfunction ));
    if (fptr->subfunctions == NULL)
	sfptr = fptr->subfunctions = subfunc;
    else {
	sfptr = fptr->subfunctions;
	while (sfptr->next != NULL)
	    sfptr = sfptr->next;
	sfptr = sfptr->next = subfunc;
    }

    sfptr->parent = fptr;
    sfptr->status = is_free;
    sfptr->index.eeprom = -1;

    if (sfptr->explicit = type) {
	vs_index += vs_incr;
	name = process_string(SHORT_STRING, name, FUNC_MAX_LEN, 1);
	sfptr->name = name;
    }
}





/****+++***********************************************************************
*
* Function:     subfunc_type()
*
* Parameters:   type              
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

void subfunc_type(type)
    char	*type;
{
    type = process_string(SHORT_STRING, type, TYPE_MAX_LEN, 0);
    sfptr->type = strupr(type);
}





/****+++***********************************************************************
*
* Function:     subfunc_cmmts()
*
* Parameters:   cmmts             
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

void subfunc_cmmts(cmmts)
    char	*cmmts;
{
    cmmts = process_string(LONG_STRING, cmmts, COMM_MAX_LEN, 0);
    sfptr->comments = cmmts;
}

/****+++***********************************************************************
*
* Function:     subfunc_help()
*
* Parameters:   help              
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

void subfunc_help(help)
    char	*help;
{
    help = process_string(LONG_STRING, help, HELP_MAX_LEN, 0);
    sfptr->help = help;
}





/****+++***********************************************************************
*
* Function:     subfunc_connect()
*
* Parameters:   connect           
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

void subfunc_connect(connect)
    char	*connect;
{
    connect = process_string(LONG_STRING, connect, CONNECT_MAX_LEN, 0);
    sfptr->connection = connect;
}



/****+++***********************************************************************
*
* Function:     beg_choice()
*
* Parameters:   name              
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

void beg_choice(name)
    char		*name;
{
    struct choice 	*choice;


    freeform_flag = 0;
    ++bptr->entries;
    if (fptr->subfunctions == NULL)
	beg_subfunc(0, "");
    ++sfptr->index.total;

    choice = (struct choice *) allocate(sizeof(struct choice ));
    if (sfptr->choices == NULL)
	chptr = sfptr->choices = sfptr->current = choice;
    else {
	chptr = sfptr->choices;
	while (chptr->next != NULL)
	    chptr = chptr->next;
	chptr = chptr->next = choice;
    }

    schptr = chptr->primary = (struct subchoice *) allocate(sizeof(struct subchoice ));
    chptr->parent = sfptr;
    schptr->parent = chptr;
    chptr->status = is_free;
    chptr->index.eeprom = -1;
    name = process_string(SHORT_STRING, name, CHOICE_MAX_LEN, 1);
    chptr->name = name;
    required.choice = 1;
}





/****+++***********************************************************************
*
* Function:     choice_type()
*
* Parameters:   type              
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

void choice_type(type)
    char	*type;
{
    type = process_string(SHORT_STRING, type, TYPE_MAX_LEN, 0);
    chptr->subtype = strupr(type);
}

/****+++***********************************************************************
*
* Function:     choice_help()
*
* Parameters:   help              
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

void choice_help(help)
    char	*help;
{
    help = process_string(LONG_STRING, help, HELP_MAX_LEN, 0);
    chptr->help = help;
}





/****+++***********************************************************************
*
* Function:     choice_amp()
*
* Parameters:   val               
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

void choice_amp(val)
    unsigned long	val;
{
    if (val > MAXIMUM_AMP)
	parse_errmsg(PARSE30_ERRCODE, 2);
    else
	chptr->amperage = val;
}





/****+++***********************************************************************
*
* Function:     choice_dis()
*
* Parameters:   disable          
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

void choice_dis(disable)
    int disable;
{
    chptr->disable = disable;
}

/****+++***********************************************************************
*
* Function:     beg_subchoice()
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

void beg_subchoice()
{
    struct subchoice *subchoice;

    ++chptr->index.total;
    if (chptr->primary->resource_groups == NULL) {
	free((void *)chptr->primary);
	chptr->primary = NULL;
    }

    subchoice = (struct subchoice *) allocate(sizeof(struct subchoice ));
    if (chptr->subchoices == NULL)
	schptr = chptr->subchoices = chptr->current = subchoice;
    else {
	schptr = chptr->subchoices;
	while (schptr->next != NULL)
	    schptr = schptr->next;
	schptr = schptr->next = subchoice;
    }

    schptr->parent = chptr;
    schptr->explicit = 1;
}



/****+++***********************************************************************
*
* Function:     beg_resource_group()
*
* Parameters:   type              
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

void beg_resource_group(type)
    int 			type;
{
    struct resource_group 	*rgrp;

    if (freeform_flag) {
	parse_errmsg(PARSE71_ERRCODE, 2);
	return;
    }

    rgrp = (struct resource_group *) allocate(sizeof(struct resource_group ));
    if (schptr->resource_groups == NULL)
	rgptr = schptr->resource_groups = rgrp;
    else {
	rgptr = schptr->resource_groups;
	while (rgptr->next != NULL)
	    rgptr = rgptr->next;
	rgptr = rgptr->next = rgrp;
    }

    rgptr->parent = schptr;
    rgptr->index.eeprom = -1;
    rgptr->status = is_free;

    switch (type) {
	case LINKED_GROUP:
	    rgptr->type = rg_link;
	    ++bptr->entries;
	    break;
	case COMBINED_GROUP:
	    rgptr->type = rg_combine;
	    break;
	case FREE_GROUP:
	    rgptr->type = rg_free;
	    /*****  fix -- steveb *****/
	    rgptr->index.total = 0;
	    break;
    }
}


/****+++***********************************************************************
*
* Function:     set_index_value()
*
* Parameters:   resource          
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

unsigned set_index_value(resource)
    struct resource 	*resource;
{
    union rptr 		locptr;
    int 		index_value, total;


    if (resource == NULL)
	return(1);

    locptr.resource = resource;
    index_value = set_index_value(resource->next);
    switch (locptr.resource->type) {
	case rt_dma:
	    locptr.dma->index_value = index_value;
	    total = locptr.dma->index.total;
	    break;
	case rt_irq:
	    locptr.irq->index_value = index_value;
	    total = locptr.irq->index.total;
	    break;
	case rt_port:
	    locptr.port->index_value = index_value;
	    total = locptr.port->index.total;
	    break;
    }
    return(index_value * total);
    
}



/****+++***********************************************************************
*
* Function:     end_resource_group()
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

void end_resource_group()
{
    union rptr locptr;

    memory_flag = 0;
    if (rgptr->resources != NULL) {
	switch (rgptr->type) {
	    case rg_link:
		break;
	    case rg_combine:
	    case rg_free:
	    case rg_dlink:
		locptr.resource = rgptr->resources;
		switch (locptr.resource->type) {
		    case rt_dma:
			locptr.dma->index_value = set_index_value(rgptr->resources->next);
			break;
		    case rt_irq:
			locptr.irq->index_value = set_index_value(rgptr->resources->next);
			break;
		    case rt_port:
			locptr.port->index_value = set_index_value(rgptr->resources->next);
			break;
		    case rt_memory:
			if (locptr.memory->address == NULL)
			    locptr.memory->memory.index_value = 1;
			else {
			    locptr.memory->memory.index_value = locptr.memory->address->index.total;
			    locptr.memory->address->index_value = 1;
			}
			break;
		}
		break;
	}

	if (rgptr->resources->type == rt_memory && chptr->totalmem != NULL) {
	    locptr.resource = rgptr->resources;
	    if (rgptr->type == rg_link)
		rgptr->index.current = locptr.memory->memory.index.current;
	    else
		rgptr->index.current = locptr.memory->memory.index.current
		    *locptr.memory->memory.index_value;
	}
    }
}



/****+++***********************************************************************
*
* Function:     init_index_value()
*
* Parameters:   resource          
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

void init_index_value(resource)
    struct resource 	*resource;
{
    union rptr 		locptr;


    locptr.resource = resource;
    switch (resource->type) {
	case rt_dma:
	    locptr.dma->index_value = 1;
	    break;
	case rt_irq:
	    locptr.irq->index_value = 1;
	    break;
	case rt_port:
	    locptr.port->index_value = 1;
	    break;
	case rt_memory:
	    end_resource_group();
    }
}



/****+++***********************************************************************
*
* Function:     beg_dma()
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

void beg_dma()
{
    struct dma 	*dma;

    if (memory_flag)
	parse_errmsg(PARSE37_ERRCODE, 2);

    if ((rgptr->type == rg_free) && (rgptr->resources != NULL)) {
	init_index_value(rgptr->resources);
	beg_resource_group(FREE_GROUP);
    }

    value_count = 0;
    first_pass = 1;
    rlptr = &valuelist;
    dma = (struct dma *) allocate(sizeof(struct dma ));

    if (rgptr->resources == NULL) {
	dmaptr = dma;
	resptr = rgptr->resources = &(dma->r);
    }
    else {
	resptr = resptr->next = &(dma->r);
	dmaptr = dma;
    }

    dmaptr->r.parent = rgptr;
    dmaptr->r.type = rt_dma;
}





/****+++***********************************************************************
*
* Function:     dma_share()
*
* Parameters:   val
*		txt         
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

void dma_share(val, txt)
    unsigned long 	val;
    char    		*txt;
{

    if (val == -1) {
	txt = process_string(SHORT_STRING, txt, SHARE_MAX_LEN, 0);
	dmaptr->r.share_tag = txt;
	dmaptr->r.share = 1;
    }
    else
	dmaptr->r.share = (int)val;
}

/****+++***********************************************************************
*
* Function:     dma_size()
*
* Parameters:   val
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

void dma_size(val)
    unsigned long 	val;
{

    switch (val) {
	case 8:
	    dmaptr->data_size = ds_byte;
	    break;
	case 16:
	    dmaptr->data_size = ds_word;
	    break;
	case 32:
	    dmaptr->data_size = ds_dword;
	    break;
    }
}





/****+++***********************************************************************
*
* Function:     dma_timing()
*
* Parameters:   val
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

void dma_timing(val)
    unsigned long 	val;
{

    switch (val) {
	case DEF_TIMING:
	    dmaptr->timing = dt_default;
	    break;
	case TYPEA_TIMING:
	    dmaptr->timing = dt_typea;
	    break;
	case TYPEB_TIMING:
	    dmaptr->timing = dt_typeb;
	    break;
	case TYPEC_TIMING:
	    dmaptr->timing = dt_typec;
	    break;
    }
}

/****+++***********************************************************************
*
* Function:     beg_irq()
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

void beg_irq()
{
    struct irq 	*irq;

    if (memory_flag)
	parse_errmsg(PARSE37_ERRCODE, 2);

    if ((rgptr->type == rg_free) && (rgptr->resources != NULL)) {
	init_index_value(rgptr->resources);
	beg_resource_group(FREE_GROUP);
    }

    value_count = 0;
    first_pass = 1;
    rlptr = &valuelist;
    irq = (struct irq *) allocate(sizeof(struct irq ));

    if (rgptr->resources == NULL) {
	irqptr = irq;
	resptr = rgptr->resources = &(irq->r);
    }
    else {
	resptr = resptr->next = &(irq->r);
	irqptr = irq;
    }

    irqptr->r.parent = rgptr;
    irqptr->r.type = rt_irq;
}





/****+++***********************************************************************
*
* Function:     irq_share()
*
* Parameters:   val
*		txt         
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

void irq_share(val, txt)
    unsigned long 	val;
    char    		*txt;
{

    if (val == -1) {
	txt = process_string(SHORT_STRING, txt, SHARE_MAX_LEN, 0);
	irqptr->r.share_tag = txt;
	irqptr->r.share = 1;
    }
    else
	irqptr->r.share = (int)val;
}





/****+++***********************************************************************
*
* Function:     irq_trig()
*
* Parameters:   val
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

void irq_trig(val)
    unsigned long 	val;
{

    switch (val) {
	case IRQ_LEVEL:
	    irqptr->trigger = it_level;
	    break;
	case IRQ_EDGE:
	    irqptr->trigger = it_edge;
	    break;
    }
}

/****+++***********************************************************************
*
* Function:     beg_port()
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

void beg_port()
{
    struct port 	*port;


    if (memory_flag)
	parse_errmsg(PARSE37_ERRCODE, 2);

    if ((rgptr->type == rg_free) && (rgptr->resources != NULL)) {
	init_index_value(rgptr->resources);
	beg_resource_group(FREE_GROUP);
    }

    value_count = 0;
    first_pass = 1;
    rlptr = &valuelist;
    port = (struct port *) allocate(sizeof(struct port ));

    if (rgptr->resources == NULL) {
	portptr = port;
	resptr = rgptr->resources = &(port->r);
    }
    else {
	resptr = resptr->next = &(port->r);
	portptr = port;
    }

    portptr->r.parent = rgptr;
    portptr->r.type = rt_port;
}





/****+++***********************************************************************
*
* Function:     port_range()
*
* Parameters:   first
*		last
*		step
*		count
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

void port_range(first, last, step, count)
    unsigned long   first, last, step, count;
{

    if ((first > MAX_PORT) || (last > MAX_PORT))
	parse_errmsg(PARSE27_ERRCODE, 2);
    if ((step > MAX_PORT) || (step == 0) || (count > MAX_PORT)) {
	parse_errmsg(PARSE72_ERRCODE, 2);
	return;
    }
    if (first > last)
	parse_errmsg(PARSE66_ERRCODE, 2);

    portptr->u.range.min = portptr->u.range.current = first;
    portptr->u.range.max = last;
    portptr->step = step;
    portptr->u.range.count = count;
    value_count = (last - first - count + 1) / step + 1;
}

/****+++***********************************************************************
*
* Function:     set_eisaport()
*
* Parameters:   flag              
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

void set_eisaport(flag)
    int flag;

{
    eisaport = flag;
}





/****+++***********************************************************************
*
* Function:     port_share()
*
* Parameters:   val
*		txt         
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

void port_share(val, txt)
    unsigned long 	val;
    char    		*txt;
{

    if (val == -1) {
	txt = process_string(SHORT_STRING, txt, SHARE_MAX_LEN, 0);
	portptr->r.share_tag = txt;
	portptr->r.share = 1;
    }
    else
	portptr->r.share = (int)val;
}





/****+++***********************************************************************
*
* Function:     port_size()
*
* Parameters:   val               
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

void port_size(val)
    unsigned long	val;
{

    switch (val) {
	case 8:
	    portptr->data_size = ds_byte;
	    break;
	case 16:
	    portptr->data_size = ds_word;
	    break;
	case 32:
	    portptr->data_size = ds_dword;
	    break;
    }
}

/****+++***********************************************************************
*
* Function:     beg_memory()
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

void beg_memory()
{
    struct memory *memory;

    value_count = 0;
    first_pass = 1;
    memory_flag = 1;
    rlptr = &valuelist;

    if ((rgptr->type == rg_free) && (rgptr->resources != NULL)) {
	end_resource_group();
	beg_resource_group(FREE_GROUP);
    }

    if (rgptr->resources != NULL)
	parse_errmsg(PARSE37_ERRCODE, 2);

    memory = (struct memory *) allocate(sizeof(struct memory ));
    memptr = memory;
    resptr = rgptr->resources = &(memory->r);

    memptr->r.parent = rgptr;
    memptr->r.type = rt_memory;
    memptr->decode = md_32;
    memptr->writable = 1;

    switch (bptr->slot) {
	case bs_isa8:
	    memptr->data_size = ds_byte;
	    break;
	case bs_isa16:
	case bs_isa8or16:
	    memptr->data_size = ds_word;
	    break;
	default:
	    memptr->data_size = ds_dword;
    }
}




/****+++***********************************************************************
*
* Function:     mem_range()
*
* Parameters:   first
*		last
*		step
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

void mem_range(first, last, step)
    unsigned long int	first, last;
    long    		step;
 {

    if (first > last)
	parse_errmsg(PARSE66_ERRCODE, 2);
    if ((first > MAX_MEMORY) || (last > MAX_MEMORY))
	parse_errmsg(PARSE34_ERRCODE, 2);
    if ((first | last | step) & (MEMORY_GRANULARITY - 1))
	parse_errmsg(PARSE74_ERRCODE, 2);
    if (step == 0) {
	parse_errmsg(PARSE73_ERRCODE, 2);
	return;
    }

    if ((last - first) % step != 0)
	last = (last - first) / step * step + first;

    memptr->memory.u.range.min = first;
    memptr->memory.u.range.max = last;
    memptr->memory.step = step;
    memptr->memory.index.total = ((last - first) / step) + 1;
    memptr->memory.index.eeprom = -1;

    if (chptr->totalmem != NULL) {
	memptr->memory.u.range.current = last;
	memptr->memory.index.current = memptr->memory.index.total - 1;
    }
    else
	memptr->memory.u.range.current = first;
}

/****+++***********************************************************************
*
* Function:     end_mem_vals()
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

void end_mem_vals()
{
    struct value 	*value, *high;
    int 		i, j, high_index;


    if (memory_overflow)
	parse_errmsg(PARSE34_ERRCODE, 2);

    memory_overflow = 0;
    memptr->memory.u.list.current = memptr->memory.u.list.values = valuelist.next->head;

    if (chptr->totalmem != NULL)
	for (i = 1; i <= value_count; i++) {
	    high = NULL;
	    for (value = memptr->memory.u.list.values, j = 0; 
		                 value != NULL; value = value->next, j++)
		if (value->init_index == 0 && (high == NULL || value->min >= high->min)) {
		    high = value;
		    high_index = j;
		}
	    high->init_index = i;
	    if (i == 1) {
		memptr->memory.u.list.current = high;
		memptr->memory.index.current = high_index;
	    }
	}

    memptr->memory.index.total = value_count;
    memptr->memory.index.eeprom = -1;
    value_count = 0;
    free_valuelist(valuelist.next);
    valuelist.next = NULL;
    first_pass = 1;
}





/****+++***********************************************************************
*
* Function:     addr_range()
*
* Parameters:   first
*		last
*		step
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Handles memory ADDRESS = range statement.
*
****+++***********************************************************************/

void addr_range(first, last, step)
    unsigned long int	first, last;
    long    		step;
 {
    unsigned long	value;
    int 		i;


    if (first > last)
	parse_errmsg(PARSE66_ERRCODE, 2);
    if ((first | last | step) & (ADDRESS_GRANULARITY - 1))
	parse_errmsg(PARSE75_ERRCODE, 2);
    if (step == 0) {
	parse_errmsg(PARSE73_ERRCODE, 2);
	return;
    }

    if ((last - first) % step != 0)
	last = (last - first) / step * step + first;

    memptr->address = (struct memory_address *) allocate(sizeof(struct memory_address ));
    memptr->address->u.range.min = first;
    memptr->address->u.range.max = last;
    memptr->address->step = step;
    memptr->address->index.total = ((last - first) / step) + 1;
    memptr->address->index.eeprom = -1;

    if (rgptr->type == rg_link) {
	memptr->address->index.current = memptr->memory.index.current;
	for (i = memptr->address->index.current, value = first; i > 0; 
	     value += step, i--) ;
	memptr->address->u.range.current = value;
    }
    else
	memptr->address->u.range.current = first;
}

/****+++***********************************************************************
*
* Function:     end_addr_vals()
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

void end_addr_vals()
{
    struct value 	*value;
    int 		i;


    memory_overflow = 0;
    memptr->address = (struct memory_address *) allocate(sizeof(struct memory_address ));
    memptr->address->u.list.values = valuelist.next->head;
    memptr->address->index.total = value_count;
    memptr->address->index.eeprom = -1;

    value_count = 0;
    free_valuelist(valuelist.next);
    valuelist.next = NULL;

    if (rgptr->type == rg_link) {
	memptr->address->index.current = memptr->memory.index.current;
	for (i = memptr->address->index.current,
	    value = memptr->address->u.list.values; 
	    i > 0; value = value->next, i--);
	memptr->address->u.list.current = value;
    }
    else
	memptr->address->u.list.current = memptr->address->u.list.values;
}





/****+++***********************************************************************
*
* Function:     mem_write()
*
* Parameters:   val
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

void mem_write(val)
    int 	val;
{
    memptr->writable = val;
}





/****+++***********************************************************************
*
* Function:     mem_type()
*
* Parameters:   val
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

void mem_type(val)
    int 	val;
{
    switch (val) {
	case SYS_MEM:
	    memptr->memtype = mt_sys;
	    break;
	case EXP_MEM:
	    memptr->memtype = mt_exp;
	    break;
	case OTHER_MEM:
	    memptr->memtype = mt_other;
	    break;
	case VIR_MEM:
	    memptr->memtype = mt_vir;
	    break;
    }
}

/****+++***********************************************************************
*
* Function:     mem_size()
*
* Parameters:   val
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

void mem_size(val)
    int 	val;
{
    switch (val) {
	case 8:
	    memptr->data_size = ds_byte;
	    break;
	case 16:
	    memptr->data_size = ds_word;
	    break;
	case 32:
	    memptr->data_size = ds_dword;
	    break;
    }
}





/****+++***********************************************************************
*
* Function:     mem_cache()
*
* Parameters:   val
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

void mem_cache(val)
    int 	val;
{
    memptr->cache = val;
}





/****+++***********************************************************************
*
* Function:     mem_decode()
*
* Parameters:   val
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

void mem_decode(val)
    unsigned long 	val;
{

    switch (val) {
	case 20:
	    memptr->decode = md_20;
	    break;
	case 24:
	    memptr->decode = md_24;
	    break;
	case 32:
	    memptr->decode = md_32;
	    break;
	default:
	    parse_errmsg(PARSE9_ERRCODE, 2);
    }
}

/****+++***********************************************************************
*
* Function:     mem_share()
*
* Parameters:   val
*		txt         
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

void mem_share(val, txt)
    unsigned long 	val;
    char    		*txt;
{

    if (val == -1) {
	txt = process_string(SHORT_STRING, txt, SHARE_MAX_LEN, 0);
	memptr->r.share_tag = txt;
	memptr->r.share = (int)val;
    }
    else
	memptr->r.share = (int)val;
}

/****+++***********************************************************************
*
* Function:     addr_range()
*
* Parameters:   type
*		val1
*		val2
*		null
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

void add_range(type, val1, val2, null)
    int 		type;
    unsigned long   	val1, val2;
    int 		null;
{
    struct value 	*value;


    if (val1 > val2)
	parse_errmsg(PARSE66_ERRCODE, 2);

    if (rlptr->next == NULL)
	if (first_pass)
	    rlptr = rlptr->next = (struct valuelist *) allocate(sizeof(struct valuelist ));
	else
	    parse_errmsg(PARSE60_ERRCODE, 2);
    else
	rlptr = rlptr->next;

    if (rlptr != valuelist.next)
	if (rgptr->type == rg_combine || rgptr->type == rg_free)
	    parse_errmsg(PARSE61_ERRCODE, 2);

    if (type == IRQ_TYPE && val1 == 2)
	val1 = val2 = 9;

    value = (struct value *) allocate(sizeof(struct value ));
    value->min = val1;
    value->max = val2;
    value->null_value = null;

    if (rlptr->head == NULL)
	rlptr->head = rlptr->tail = value;
    else
	rlptr->tail = rlptr->tail->next = value;

    switch (type) {

	case DMA_TYPE:
	    if (val1 > MAX_DMA && !null)
		parse_errmsg(PARSE63_ERRCODE, 2);
	    if (rlptr->head == value)
		dmaptr->data_size = (value->min < 4) ? ds_byte : ds_word;
	    break;

	case IRQ_TYPE:
	    if (val1 > MAX_IRQ && !null)
		parse_errmsg(PARSE64_ERRCODE, 2);
	    break;

	case PORT_TYPE:
	    if (((val1 > MAX_PORT) || (val2 > MAX_PORT)) && !(null))
		parse_errmsg(PARSE27_ERRCODE, 2);
	    break;

	case MEMORY_TYPE:
	    memory_overflow = memory_overflow
		 || (((val1 > MAX_MEMORY) || (val2 > MAX_MEMORY)) && !(null));
	    if ((val1 | val2) & (MEMORY_GRANULARITY - 1))
		parse_errmsg(PARSE74_ERRCODE, 2);
	    break;

	case ADDRESS_TYPE:
	    memory_overflow = memory_overflow
		 || (((val1 > MAX_MEMORY) || (val2 > MAX_MEMORY)) && !(null));
	    if ((val1 | val2) & (ADDRESS_GRANULARITY - 1))
		parse_errmsg(PARSE75_ERRCODE, 2);
	    break;
    }
}



/****+++***********************************************************************
*
* Function:     end_list()
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

void end_list()
{
    ++value_count;
    if (rlptr->next != NULL)
	parse_errmsg(PARSE60_ERRCODE, 2);
    rlptr = &valuelist;
    first_pass = 0;
}



/****+++***********************************************************************
*
* Function:     set_stmt()
*
* Parameters:   type
*		rlist
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

void set_stmt(type, rlist)
    int 		type;
    struct valuelist 	*rlist;
{

    if ((rgptr->type == rg_combine) || (rgptr->type == rg_free))
	++bptr->entries;

    switch (type) {

	case DMA_TYPE:
	    dmaptr->values = dmaptr->current = rlist->head;
	    dmaptr->index.total = value_count;
	    dmaptr->index.eeprom = -1;
	    break;

	case IRQ_TYPE:
	    irqptr->values = irqptr->current = rlist->head;
	    irqptr->index.total = value_count;
	    irqptr->index.eeprom = -1;
	    break;

	case PORT_TYPE:
	    if (portptr->step == 0)
		portptr->u.list.values = portptr->u.list.current = rlist->head;
	    portptr->index.total = value_count;
	    portptr->index.eeprom = -1;
	    portptr->slot_specific = eisaport;
	    break;

	case MEMORY_TYPE:
	    if (memptr->address == NULL) {
		value_count = memptr->memory.index.total;
	    if ((memptr->memtype == mt_sys) || (memptr->memtype == mt_vir))
		parse_errmsg(PARSE38_ERRCODE, 2);
	    }
	    else {
		if (memptr->memtype == mt_exp)
		    parse_errmsg(PARSE1_ERRCODE, 2);
		if (rgptr->type == rg_link)
		    if ((memptr->memory.index.total == 1) && (memptr->address->index.total != 1))
			rgptr->type = rg_combine;
		    else if (memptr->memory.index.total != memptr->address->index.total)
			parse_errmsg(PARSE2_ERRCODE, 2);
		if (rgptr->type == rg_link)
		    value_count = memptr->address->index.total;
		else
		    value_count = memptr->memory.index.total * memptr->address->index.total;
	    }
    }

    switch (rgptr->type) {
	case rg_link:
	    if (rgptr->index.total == 0) /* No items in the group until now */
		rgptr->index.total = value_count;
	    else if (rgptr->index.total != value_count)
		parse_errmsg(PARSE2_ERRCODE, 2);
	    break;
	case rg_combine:
	    if (rgptr->index.total == 0)
		rgptr->index.total = value_count;
	    else
		rgptr->index.total *= value_count;
	    break;
	case rg_free:
	    if (value_count > rgptr->index.total)
		rgptr->index.total = value_count;
	    break;
	case rg_dlink:
	    rgptr->index.total = value_count;
	    break;
    }

}



/****+++***********************************************************************
*
* Function:     end_res_stmt()
*
* Parameters:   stmt              
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

void end_res_stmt(type)
    int 	type;
{

    rlptr = valuelist.next;
    set_stmt(type, rlptr);

    if (rlptr != NULL) {
	for (rlptr = rlptr->next; rlptr != NULL; rlptr = rlptr->next) {
	    switch (type) {
		case DMA_TYPE:
		    dmaptr = (struct dma *) allocate(sizeof(struct dma ));
		    resptr = resptr->next = &(dmaptr->r);
		    dmaptr->r.parent = rgptr;
		    dmaptr->r.type = rt_dma;
		    break;
		case IRQ_TYPE:
		    irqptr = (struct irq *) allocate(sizeof(struct irq ));
		    resptr = resptr->next = &(irqptr->r);
		    irqptr->r.parent = rgptr;
		    irqptr->r.type = rt_irq;
		    break;
		case PORT_TYPE:
		    portptr = (struct port *) allocate(sizeof(struct port ));
		    resptr = resptr->next = &(portptr->r);
		    portptr->r.parent = rgptr;
		    portptr->r.type = rt_port;
		    break;
	    }

	    set_stmt(type, rlptr);
	}

	free_valuelist(valuelist.next);
	valuelist.next = NULL;
    }
}



/****+++***********************************************************************
*
* Function:     beg_init()
*
* Parameters:   type
*		val
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

void beg_init(type, val)
    int 		type, val;
{
    struct init 	*init;
    struct ioport 	*ioport;
    struct jumper 	*jumper;
    struct bswitch 	*bswitch;
    struct software 	*software;


    init_val_count = 0;
    init = (struct init *) allocate(sizeof(struct init ));
    if (rgptr->inits == NULL)
	initptr = rgptr->inits = init;
    else
	initptr = initptr->next = init;

    switch (type) {

	case IOPORT_INIT:
	    initptr->type = it_ioport;
	    initptr->data_type = dt_value;

	    if (val == -1)
		initptr->ptr.ioport = iptr;
	    else {
		ioport = bptr->ioports;
		while ((ioport->index != val) && (ioport != NULL))
		    ioport = ioport->next;
		if (ioport == NULL)
		    parse_errmsg(PARSE22_ERRCODE, 2);
		else
		    iptr = initptr->ptr.ioport = ioport;
	    }

	    switch (initptr->ptr.ioport->data_size) {
		case ds_byte:
		    initptr->mask = 0xFF;
		    break;
		case ds_word:
		    initptr->mask = 0xFFFF;
		    break;
		case ds_dword:
		    initptr->mask = 0xFFFFFFFF;
		    break;
	    }

	    loc_flag = IOPORT_BLOCK;
	    break;

	case SWITCH_INIT:
	    initptr->type = it_bswitch;
	    initptr->data_type = dt_value;
	    bswitch = bptr->switches;
	    while (( bswitch->index != val) && (bswitch != NULL))
		bswitch = bswitch->next;
	    if (bswitch == NULL)
		parse_errmsg(PARSE22_ERRCODE, 2);
	    else
		sptr = initptr->ptr.bswitch = bswitch;
	    loc_flag = SWITCH_BLOCK;
	    break;

	case JUMPER_INIT:
	    initptr->type = it_jumper;
	    jumper = bptr->jumpers;
	    while (( jumper->index != val) && (jumper != NULL))
		jumper = jumper->next;
	    if (jumper == NULL)
		parse_errmsg(PARSE22_ERRCODE, 2);
	    else {
		jptr = initptr->ptr.jumper = jumper;
		if (jptr->type == jt_tripole)
		    initptr->data_type = dt_tripole;
		else
		    initptr->data_type = dt_value;
	    }
	    loc_flag = JUMPER_BLOCK;
	    break;

	case SOFT_INIT:
	    initptr->type = it_software;
	    initptr->data_type = dt_string;
	    software = bptr->softwares;
	    while (( software->index != val) && (software != NULL))
		software = software->next;
	    if (software == NULL)
		parse_errmsg(PARSE22_ERRCODE, 2);
	    else
		initptr->ptr.software = software;
	    break;
    }
}

/****+++***********************************************************************
*
* Function:     end_init_list()
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

void end_init_list()
{

    if (num_loc != 0)
	initptr->mask = loc_list;

    switch (initptr->type) {
	case it_ioport:
	    if (initptr->mask & initptr->ptr.ioport->initval.reserved_mask)
		parse_errmsg(PARSE67_ERRCODE, 2);
	    break;
	case it_bswitch:
	    if (initptr->mask & initptr->ptr.bswitch->initval.reserved_mask)
		parse_errmsg(PARSE67_ERRCODE, 2);
	    break;
	case it_jumper:
	    if (initptr->mask & initptr->ptr.jumper->initval.reserved_mask)
		parse_errmsg(PARSE67_ERRCODE, 2);
	    break;
    }
}



/****+++***********************************************************************
*
* Function:     init_val
*
* Parameters:   vals              
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Handles INIT = val for IOPORT/SWITCH/JUMPER.
*
****+++***********************************************************************/

void init_val(vals)
    char    		*vals;
{
    struct init_value 	*init_value;
    char		*valptr;
    unsigned long	n;
    int 		sr_flag = 0;
    int 		sr_found = 0;


    valptr = vals;

    if (loc_flag == SWITCH_BLOCK) {
	sr_flag = ((sptr->type == st_rotary) || (sptr->type == st_slide));
	if (((sptr->reverse) && (loc_descending)) || ((!sptr->reverse) && (loc_ascending)))
	    vals = strrev(vals);
    }

    if (loc_flag == JUMPER_BLOCK)
	if (((jptr->reverse) && (loc_descending)) || ((!jptr->reverse) && (loc_ascending)))
	    vals = strrev(vals);

    ++init_val_count;
    init_value = (struct init_value *) (allocate(sizeof(struct init_value )));
    if (initptr->init_values == NULL)
	initvalueptr = initptr->init_values = init_value;
    else
	initvalueptr = initvalueptr->next = init_value;

    if (strlen(vals) != 0) {
	if (num_loc != 0) {
	    if (strlen(vals) != num_loc)
		parse_errmsg(PARSE28_ERRCODE, 2);
	}
	else if (loc_flag == IOPORT_BLOCK)
	    switch (initptr->ptr.ioport->data_size) {
		case ds_byte:
		    if (strlen(vals) != 8)
			parse_errmsg(PARSE58_ERRCODE, 2);
		    break;
		case ds_word:
		    if (strlen(vals) != 16)
			parse_errmsg(PARSE58_ERRCODE, 2);
		    break;
		case ds_dword:
		    if (strlen(vals) != 32)
			parse_errmsg(PARSE58_ERRCODE, 2);
		    break;
	    }

	if (parse_err_flag)
	    return;

	for (n = BIT31; n != 0; n >>= 1) {
	    if (initptr->mask & n) {
		switch (*vals) {
		    case '1':
			if (sr_flag && sr_found)
			    parse_errmsg(PARSE47_ERRCODE, 2);
			if (sr_flag)
			    sr_found = 1;
			if (initptr->data_type == dt_tripole)
			    initvalueptr->u.tripole.data_bits |= n;
			else
			    initvalueptr->u.value |= n;
			break;
		    case '0':
			break;
		    case 'X':
			parse_errmsg(PARSE59_ERRCODE, 2);
			break;
		    case 'N':
			if (initptr->data_type == dt_tripole)
			    initvalueptr->u.tripole.tristate_bits |= n;
			else
			    parse_errmsg(PARSE42_ERRCODE, 2);
			break;
		}

		vals += 1;
		if (parse_err_flag)
		    return;
	    }
	}
    }
    mn_trapfree((void *)valptr);
}

/****+++***********************************************************************
*
* Function:     init_range()
*
* Parameters:   val1
*		val2
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

void init_range(val1, val2)
    char    		*val1, *val2;
{
    unsigned long	n, max, min;
    int 		i, mult;
    char		*v1, *v2;


    if (loc_flag == SWITCH_BLOCK)
	if (((sptr->reverse) && (loc_descending)) || ((!sptr->reverse) && (loc_ascending)))
	    parse_errmsg(PARSE65_ERRCODE, 2);

    if (loc_flag == JUMPER_BLOCK)
	if (((jptr->reverse) && (loc_descending)) || ((!jptr->reverse) && (loc_ascending)))
	    parse_errmsg(PARSE65_ERRCODE, 2);

    init_val_count = 0;
    v1 = val1;
    v2 = val2;

    if (num_loc != 0) {
	if ((strlen(val1) != num_loc) || (strlen(val2) != num_loc))
	    parse_errmsg(PARSE28_ERRCODE, 2);
    }

    else if (loc_flag == IOPORT_BLOCK)
	switch (initptr->ptr.ioport->data_size) {
	    case ds_byte:
		if ((strlen(val1) != 8) || (strlen(val2) != 8))
		    parse_errmsg(PARSE58_ERRCODE, 2);
		break;
	    case ds_word:
		if ((strlen(val1) != 16) || (strlen(val2) != 16))
		    parse_errmsg(PARSE58_ERRCODE, 2);
		break;
	    case ds_dword:
		if ((strlen(val1) != 32) || (strlen(val2) != 32))
		    parse_errmsg(PARSE58_ERRCODE, 2);
		break;
	}

    if (parse_err_flag)
	return;

    for (n = BIT31; n != 0; n >>= 1)
	if (initptr->mask & n) {
	    switch (*val1) {
		case '1':
		    initptr->range.min |= n;
		    break;
		case '0':
		    break;
		default:
		    parse_errmsg(PARSE21_ERRCODE, 2);
		    return;
	    }

	    switch (*val2) {
		case '1':
		    initptr->range.max |= n;
		    break;
		case '0':
		    break;
		default:
		    parse_errmsg(PARSE21_ERRCODE, 2);
		    return;
	    }

	    val1 += 1;
	    val2 += 1;
	}

    if ((initptr->type == it_bswitch)
	 && ((initptr->ptr.bswitch->type == st_rotary)
	 || (initptr->ptr.bswitch->type == st_slide)))  {
	if ( (initptr->range.min == 0) || (initptr->range.max == 0) ) {
	    parse_errmsg(PARSE47_ERRCODE, 2);
	    return;
	}
	if (initptr->range.min <= initptr->range.max)
	    for (init_val_count = 1, n = initptr->range.min; 
		n != initptr->range.max; ++init_val_count)
		n <<= 1;
	else
	    for (init_val_count = 1, n = initptr->range.min; 
		n != initptr->range.max; ++init_val_count)
		n >>= 1;
    }

    else {
	mult = 1;
	min = max = 0;
	for (i = strlen(v1) - 1; i >= 0; i--) {
	    min += ((int) *(v1 + i) - 0x30) * mult;
	    max += ((int) *(v2 + i) - 0x30) * mult;
	    mult *= 2;
	}
	init_val_count = abs((int) (max - min)) + 1;
    }

    mn_trapfree((void *)v1);
    mn_trapfree((void *)v2);
}



/****+++***********************************************************************
*
* Function:     init_string()
*
* Parameters:   val
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Handles SOFTWARE INIT statements.
*
****+++***********************************************************************/

void init_string(val)
    char    		*val;
{
    struct init_value 	*init_value;


    ++init_val_count;
    init_value = (struct init_value *) (allocate(sizeof(struct init_value )));
    if (initptr->init_values == NULL)
	initvalueptr = initptr->init_values = init_value;
    else
	initvalueptr = initvalueptr->next = init_value;
    val = process_string(LONG_STRING, val, SOFT_PARM_MAX_LEN, 0);
    initvalueptr->u.parameter = val;
}



/****+++***********************************************************************
*
* Function:     end_init_stmt()
*
* Parameters:   memory_type        
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

void end_init_stmt(memory_type)
{

    if (memory_type) {
	if (memptr->memory.index.total != init_val_count)
	    parse_errmsg(PARSE2_ERRCODE, 2);
	else if (rgptr->type != rg_link && rgptr->type != rg_dlink)
	    parse_errmsg(PARSE16_ERRCODE, 2);
	rgptr->type = rg_dlink;
	initptr->memory = memory_type;
    }

    else
	switch (rgptr->type) {

	    case rg_link:
		if (rgptr->index.total == 0)
		    rgptr->index.total = init_val_count;
		else {
		    if (rgptr->resources->type == rt_memory) {
			if (((struct memory *) rgptr->resources)->memory.index.total != init_val_count)
			    parse_errmsg(PARSE2_ERRCODE, 2);
		    }
		    else if (rgptr->index.total != init_val_count)
			parse_errmsg(PARSE2_ERRCODE, 2);
		}
		break;

	    case rg_combine:
		if (rgptr->index.total == 0)
		    rgptr->index.total = init_val_count;
		else if (rgptr->index.total != init_val_count)
		    parse_errmsg(PARSE43_ERRCODE, 2);
		break;

	    case rg_free:
		if (init_val_count > rgptr->index.total)
		    rgptr->index.total = init_val_count;
		break;
	}
}





/****+++***********************************************************************
*
* Function:     null_init()
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

void null_init()
{
    rgptr->type = rg_dlink;
}



/****+++***********************************************************************
*
* Function:     set_portvar()
*
* Parameters:   index
*		addr
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

void set_portvar(index, addr)
    int 		index;
    unsigned long   	addr;
 {
    struct ioport 	*ip;
    struct portvar 	*pv;
    struct portvar 	*portvar;


    ip = bptr->ioports;
    if (index > CTRL_NDX_MAX)
	parse_errmsg(PARSE22_ERRCODE, 2);
    else if (addr > MAX_PORT)
	parse_errmsg(PARSE27_ERRCODE, 2);

    else {
	while ((ip != NULL) && (ip->portvar_index != index))
	    ip = ip->next;

	if (ip == NULL)
	    parse_errmsg(PARSE26_ERRCODE, 2);
	else {
	    pv = (struct portvar *) allocate(sizeof(struct portvar ));
	    if (chptr->portvars == NULL) {
		portvar = pv;
		chptr->portvars = portvar;
	    }
	    else {
		portvar = pv;
		portvar->next = pv;
	    }
	    if (eisaport)
		portvar->slot_specific = 1;
	    portvar->index = index;
	    portvar->address = addr;
	    eisaport = 0;
	}
    }
}

/****+++***********************************************************************
*
* Function:     beg_totalmem()
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

void beg_totalmem()
{
    sfptr->status = is_actual;
    chptr->totalmem = (struct totalmem *) allocate(sizeof(struct totalmem ));
    chptr->totalmem->eeprom = -1;
}





/****+++***********************************************************************
*
* Function:     totalmem_val()
*
* Parameters:   memamt            
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

void totalmem_val(memamt)
    unsigned long   	memamt;
{
    struct value 	*value;


    if (memamt & (MEMORY_GRANULARITY - 1))
	parse_errmsg(PARSE74_ERRCODE, 2);

    value = (struct value *) allocate(sizeof(struct value ));
    if (chptr->totalmem->u.values == NULL) {
	chptr->totalmem->actual = memamt;
	valueptr = chptr->totalmem->u.values = value;
    }
    else
	valueptr = valueptr->next = value;

    valueptr->min = valueptr->max = memamt;
}





/****+++***********************************************************************
*
* Function:     totalmem_range()
*
* Parameters:   min
*		max
*		step
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

void totalmem_range(min, max, step)
    unsigned long   	min, max;
    long    		step;
{

    if (min > max)
	parse_errmsg(PARSE66_ERRCODE, 2);
    if ((min | max | step) & (MEMORY_GRANULARITY - 1))
	parse_errmsg(PARSE74_ERRCODE, 2);

    chptr->totalmem->u.range.min = min;
    chptr->totalmem->u.range.max = max;
    chptr->totalmem->step = step;
    chptr->totalmem->actual = min;
}

/****+++***********************************************************************
*
* Function:     end_func()
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

void end_func()
{

    if (!required.choice)
	parse_errmsg(PARSE39_ERRCODE, 2);

}





/****+++***********************************************************************
*
* Function:     end_board()
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

void end_board()
{

    if (!required.function)
	parse_errmsg(PARSE40_ERRCODE, 2);

    mptr->entries += bptr->entries;
    if (bptr->eisa_slot == 0
	 && !strcmpi(bptr->category, "SYS")
	 && !system_flag)
	parse_errmsg(PARSE57_ERRCODE, 2);
    else if (!strcmpi(bptr->category, "SYS")
	 && (bptr->eisa_slot != 0 || bptr->slot != bs_emb))
	parse_errmsg(PARSE14_ERRCODE, 2);

}


/****+++***********************************************************************
*
* Function:     beg_sys()
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

void beg_sys()
{

    if ((bptr->eisa_slot != 0) || (strcmpi(bptr->category, "SYS")))
	parse_errmsg(PARSE56_ERRCODE, 2);

    mptr->default_sizing = (sizing_flag) ? bptr->sizing : DEFAULT_SIZING_GAP;
    curr_slot = eisa_slot = not_eisa_slot = 0;
    system_flag = 1;
}





/****+++***********************************************************************
*
* Function:     sys_nvmem()
*
* Parameters:   nvmem             
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

void sys_nvmem(nvmem)
    unsigned long   nvmem;
{
    if (nvmem >  SYS_NVMEM_MAX)
	parse_errmsg(PARSE36_ERRCODE, 2);
    else
	mptr->nonvolatile = nvmem;
}





/****+++***********************************************************************
*
* Function:     sys_amps()
*
* Parameters:   amps              
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

void sys_amps(amps)
    unsigned long   amps;
{
    if (amps > MAXIMUM_AMP)
	parse_errmsg(PARSE30_ERRCODE, 2);
    else
	mptr->amperage = amps;
}





/****+++***********************************************************************
*
* Function:     beg_sys_slot()
*
* Parameters:   num
*		type
*		eisa
*		specified
*
* Used:		internal only
*
* Returns:      Nothing
*
* Description:
*
*    Handles sysbrd slot entries.
*
****+++***********************************************************************/

void beg_sys_slot(num, type, eisa, specified)
    int 	num, type, eisa, specified;
{
    int 	i;


    if ((eisa != -1) && ((eisa < 0) || (eisa > MAX_NUM_SLOTS - 1)))
	parse_errmsg(PARSE20_ERRCODE, 2);
    if ((num < 0) || (num > MAX_NUM_SLOTS - 1))
	parse_errmsg(PARSE20_ERRCODE, 2);
    else if (mptr->slot[num].present)
	parse_errmsg(PARSE10_ERRCODE, 2);

    else {

	switch (type) {
	    case ISA8_SLOT:
		mptr->slot[num].type = st_isa8;
		break;
	    case ISA16_SLOT:
		mptr->slot[num].type = st_isa16;
		break;
	    case EISA_SLOT:
		if (eisa == -1)
		    parse_errmsg(PARSE17_ERRCODE, 2);
		mptr->slot[num].type = st_eisa;
		mptr->slot[num].busmaster = 1;
		break;
	    case OTH_SLOT:
		mptr->slot[num].type = st_other;
		break;
	}

	mptr->slot[num].present = 1;
	mptr->slot[num].skirt = 1;
	mptr->slot[num].length = DEFAULT_SLOT_LENGTH;
	mptr->slot[num].eisa_slot = eisa;
	eisa_slot |= specified;
	not_eisa_slot |= !specified;

	if (eisa_slot && not_eisa_slot)
	    parse_errmsg(PARSE17_ERRCODE, 2);

	for (i = 0; i < MAX_NUM_SLOTS; i++)
	    if (i == num)
		continue;
	    else if (eisa != 0 && mptr->slot[i].eisa_slot == eisa) {
		parse_errmsg(PARSE10_ERRCODE, 2);
		break;
	    }

	curr_slot = num;
    }
}



/****+++***********************************************************************
*
* Function:     sys_slot_text()
*
* Parameters:   text              
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

void sys_slot_text(text)
    char    	*text;
{
    struct tag *tag;


    text = process_string(SHORT_STRING, text, SLOT_TAG_MAX_LEN, 0);
    tag = (struct tag *) allocate(sizeof(struct tag ));
    tag->string = text;
    tag->next = mptr->slot[curr_slot].tags;
    mptr->slot[curr_slot].tags = tag;

}





/****+++***********************************************************************
*
* Function:     sys_length()
*
* Parameters:   len
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

void sys_length(len)
    int 	len;
{
    if (len > SYS_SLOT_LEN_MAX)
	parse_errmsg(PARSE35_ERRCODE, 2);
    else
	mptr->slot[curr_slot].length = len;
}



/****+++***********************************************************************
*
* Function:     sys_skirt()
*
* Parameters:   val
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

void sys_skirt(val)
    int 	val;
{
    mptr->slot[curr_slot].skirt = val;
}





/****+++***********************************************************************
*
* Function:     sys_bmaster()
*
* Parameters:   val
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

void sys_bmaster(val)
    int 	val;
{
    mptr->slot[curr_slot].busmaster = val;
}





/****+++***********************************************************************
*
* Function:     sys_label()
*
* Parameters:   name
*		label2
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

void sys_label(name, label2)
    char    *name, *label2;
{

    name = process_string(SHORT_STRING, name, SLOT_LABEL_MAX_LEN, 0);
    mptr->slot[curr_slot].label = name;
    if (label2 != NULL) {
	name = process_string(SHORT_STRING, name, SLOT_LABEL_MAX_LEN, 0);
	mptr->slot[curr_slot].label2 = label2;
    }
}

/****+++***********************************************************************
*
* Function:     sys_cache()
*
* Parameters:   memory
*		address
*		step
*		cache
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

#ifdef SYS_CACHE
void sys_cache(memory, address, step, cache)
    unsigned long   	memory, address, step;
    int 		cache;
{
    struct cache_map 	*cache_map;
    struct cache_map 	*cmap;


    cache_map = (struct cache_map *) allocate(sizeof(struct cache_map ));

    if (mptr->cache_map == NULL) {
	mptr->cache_map = cache_map;
	cmap = mptr->cache_map;
    }
    else {
	cmap = cache_map;
	cmap->next = cache_map;
    }

    cache_map->memory = memory;
    cache_map->address = address;
    cache_map->step = step;
    cache_map->cache = cache;
}
#else
/*VARARGS*/
void sys_cache()
{
}
#endif





/****+++***********************************************************************
*
* Function:     set_vs_incr()
*
* Parameters:   val
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

void set_vs_incr(val)
    int 	val;
{
    vs_incr = val;
    vs_index = 0;
}





/****+++***********************************************************************
*
* Function:     incl_stmt()
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

void incl_stmt()
{
}

/****+++***********************************************************************
*
* Function:     slot_compare()
*
* Parameters:   elem1
*		elem2
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

int slot_compare(elem1, elem2)
    struct elem {
	int 		slot;
	unsigned long	key;
    } *elem1, *elem2;
{
    return((elem1->key < elem2->key) ? (-1) : (1));
}





/****+++***********************************************************************
*
* Function:     end_sys()
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

void end_sys()
{
    union {
	unsigned long   key;
	struct {
	    unsigned    slot: 4,
	    		busmaster: 1,
	    		skirt: 1,
	    		length: 10,
	    		slot_tag: 1,
	    		type: 2,
	    		not_present: 1;
	} bits;
    } sort;

    int 	i;
    int		slot_compare();


    mptr->slot[0].present = 1;
    if (mptr->slot[0].label == NULL) {
	mptr->slot[0].label = allocate(strlen(system_slot) + 1);
	(void)strcpy(mptr->slot[0].label, system_slot);
    }

    for (i = 0; i < MAX_NUM_SLOTS; i++) {

	if (sort.bits.not_present = !(mptr->slot[i].present)) {
	    mptr->slot[i].label = allocate(strlen(emb_label) + 1);
	    (void)strcpy(mptr->slot[i].label, emb_label);
	}
	else if (mptr->slot[i].label == NULL) {
	    mptr->slot[i].label = allocate(SLOT_LABEL_MAX_LEN + 1);
	    (void)strcpy(mptr->slot[i].label, slot_label);
	    /* (void)strcat(mptr->slot[i].label, itoa(i, numstr, 10)); */ /*SAB*/
	    (void)strcat(mptr->slot[i].label, ltoa((long)i));
	}

	sort.bits.type = mptr->slot[i].type;
	sort.bits.slot_tag = (mptr->slot[i].tags != NULL);
	sort.bits.length = mptr->slot[i].length;
	sort.bits.skirt = mptr->slot[i].skirt;
	sort.bits.busmaster = mptr->slot[i].busmaster;
	sort.bits.slot = i;
	mptr->sorted[i].key = sort.key;
	mptr->sorted[i].slot = i;

    }

    qsort((void *)mptr->sorted, MAX_NUM_SLOTS, sizeof(*mptr->sorted), slot_compare);
}

/****+++***********************************************************************
*
* Function:     beg_freeform()
*
* Parameters:   count             
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

void beg_freeform(count)
    unsigned long 	count;
{

    if (count > FREEFORM_MAX)
	parse_errmsg(PARSE69_ERRCODE, 2);
    else {
	freeform_count = 0;
	freeform_total = count;
	freeform_flag = 1;
	chptr->freeform = (unsigned char *) mn_trapcalloc((unsigned)(count+1),
	                                                 sizeof(unsigned char));
	chptr->freeform[0] = (int) count;
    }
}




/****+++***********************************************************************
*
* Function:     freeform_val()
*
* Parameters:   val
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

void freeform_val(val)
    unsigned long 	val;
{
    if (++freeform_count > freeform_total)
	parse_errmsg(PARSE70_ERRCODE, 2);
    else if (val > 255)
	parse_errmsg(PARSE68_ERRCODE, 2);
    else
	chptr->freeform[freeform_count] = (int) val;
}





/****+++***********************************************************************
*
* Function:     end_freeform()
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

void end_freeform()
{
    if (freeform_count != freeform_total)
	parse_errmsg(PARSE70_ERRCODE, 2);
}
