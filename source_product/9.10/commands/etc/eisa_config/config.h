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
*******************************************************************************
*
*                         inc/config.h
*
*     This file contains the basic data structures for keeping track of the
*     configuration information. All structures from the board on down are
*     described here.
*
*******************************************************************************/


/* basic defines */
#define  BYTE        unsigned char
#define  FALSE       0
#define  TRUE        ~FALSE
#define  NOT_AN_INT  -1


/* modes that config() may be called with */
#define RESOLUTION              0
#ifdef MANUAL_VERIFY
#define DETECTION               1
#endif


#ifdef VIRT_BOARD
/* ascii for embedded and virtual */
#define SW_VIRTUAL		"Virtual "
#define SW_EMBEDDED		"Embedded "
#endif

/***********
* Where do the CFG files live?
***********/
#define CFG_FILES_DIR   	"/etc/eisa/"


#define MAX_NUM_SLOTS			16
#define SYSTEM_SLOT             0

#define UNKNOWN_SLOT		(-1)

#ifdef VIRT_BOARD
#define MIN_VIRTUAL_SLOT	32
#define MAX_VIRTUAL_SLOT	64
#define	MIN_LOGICAL_SLOT	16
#define	MIN_EMBEDDED_SLOT	16
#endif


/****************************
* enumerated variable definitions
****************************/
enum board_slot    {bs_isa8,bs_isa16,bs_isa8or16,bs_eisa,bs_oth,bs_vir,bs_emb};
enum data_size     {ds_byte, ds_word, ds_dword};
enum data_type     {dt_string, dt_tripole, dt_value};
enum dma_timing    {dt_default, dt_typea, dt_typeb, dt_typec};
enum item_status   {is_actual, is_locked, is_user, is_eeprom, is_free};
enum init_type     {it_ioport, it_jumper, it_bswitch, it_software};
enum irq_trigger   {it_edge, it_level};
enum jumper_type   {jt_inline, jt_paired, jt_tripole};
enum memory_decode {md_20, md_24, md_32};
enum memory_type   {mt_sys, mt_exp, mt_other, mt_vir};
enum rg_type       {rg_link, rg_dlink, rg_combine, rg_free};
enum resource_type {rt_irq, rt_dma, rt_port, rt_memory};
enum slot_type     {st_other, st_isa8, st_isa16, st_eisa};
enum switch_type   {st_dip, st_rotary, st_slide};



struct pmode {
    unsigned	iodc_slot;		/* slot number for iodc option	      */
#ifdef MANUAL_VERIFY
    unsigned    auto_verify:1;		/* verify after every op?             */
#endif
    unsigned    non_target:1;		/* don't use NVM                      */
    unsigned    nvm_present:1;		/* is NVM accessible?                 */
    unsigned	checkcfg:1;		/* just checking a cfg file?	      */
    unsigned    scratch:1;		/* starting from scratch (sysbrd cfg) */
    unsigned	id:1;			/* doing a basic id of boards only?   */
    unsigned	slotnum:1;		/* are we getting biggest slot num?   */
    unsigned	automatic:1;		/* are we in automatic mode?          */
    unsigned	writefast:1;		/* write NVM directly from SCI file   */
};



/****************
* This data structure is used to keep track of each of the major resource
* types: dma, irq, port, and memory. 
****************/
struct space {
    struct space 	*next;		/*                                    */
    struct space 	*prev;		/*                                    */
    struct resource 	*resource;	/*                                    */
    unsigned long 	sp_min;		/* min value of this resource         */
    unsigned long 	sp_max;		/* max value of this resource         */
    int 		owner;		/* used by config.c only (entry ndx)  */
};



struct config_bits {
    unsigned long previous;		/*                                    */
    unsigned long current;		/*                                    */
    unsigned long initial;		/*				      */
};



struct index {
    int total;				/*                                    */
    int eeprom;				/*                                    */
    int config;				/*                                    */
    int previous;			/*                                    */
    int current;			/*                                    */
};



struct memory_address {
    union {
	struct	{
	    struct value *values;	/*                                    */
	    struct value *config;	/*                                    */
	    struct value *current;	/*                                    */
	} list;				/* used if step = 0                   */
	struct	{
	    unsigned long min;		/*                                    */
	    unsigned long max;		/*                                    */
	    unsigned long config;	/*                                    */
	    unsigned long current;	/*                                    */
	} range;			/* used if step != 0                  */
    } u;
    long 	 step;			/*                                    */
    struct index index;			/*                                    */
    int 	 index_value;		/*                                    */
};



struct slot {
    struct tag 		*tags;		/* from SLOT in sysboard stmt (text)  */
    					/*  describes which cards it accepts  */
    char 		*label;		/* from LABEL in sysboard SLOT stmt   */
					/*  set to slot # if not specified    */
    char 		*label2;	/* !!!written, but never used!!!!     */
    unsigned 		present:1;	/* set if slot exits (is defined)     */
    unsigned         	skirt:1;	/* from SKIRT in sysboard SLOT stmt   */
    unsigned         	busmaster:1;	/* from BUSMASTER in sysboard SLOT stmt*/
    unsigned         	occupied:1;	/* does the slot have a board in it?  */
    enum slot_type 	type;		/* from sysboard SLOT stmt            */
    int 		length;		/* from LENGTH in sysboard SLOT stmt  */
    int 		eisa_slot;	/* usually, the physical slot #       */
					/*  actually, is what slot-specific   */
					/*  addresses this slot decodes --    */
					/*  specified in sysbrd SLOT stmt     */
};



struct system {
    struct board 	*boards;	/* linked list of boards              */
#ifdef SYS_CACHE
    struct cache_map 	*cache_map;	/* ptr to cacheable memory            */
#endif
    struct space 	*irq;		/* irq use list -- config() sets up   */
    struct space 	*dma;		/* dma use list -- config() sets up   */
    struct space 	*port;		/* port use list -- config() sets up  */
    struct space 	*memory;	/* mem use list -- config() sets up   */
    unsigned long 	nonvolatile;	/* from sysboard NONVOLATILE stmt     */
    unsigned long 	amperage;	/* from sysboard AMPERAGE stmt        */
    unsigned long 	default_sizing;	/*                                    */
    struct slot 	slot [MAX_NUM_SLOTS];	/* per-slot data structures           */
    struct {
	int 		slot;		/* slot number, not always = index    */
	unsigned long 	key;		/* used as key for sort, how set?     */
    } sorted [MAX_NUM_SLOTS];
    unsigned         	configured:1;	/* is configuration done?             */
    unsigned         	amp_overload:1;	/* has amp overload message been done?*/
    int 		entries;	/* total entries? used by system      */
					/*  set, but never used               */
};



#ifdef SYS_CACHE
struct cache_map {
    struct cache_map 	*next;		/*                                    */
    unsigned long 	memory;		/*                                    */
    unsigned long 	address;	/*                                    */
    unsigned long 	step;		/*                                    */
    unsigned 		cache:1;	/*                                    */
};
#endif



struct board {
    struct board 	*next;		/* next board or 0                    */
    struct group 	*groups;	/* linked list of function groups     */
    struct ioport 	*ioports;	/*                                    */
    struct jumper 	*jumpers;	/* linked list of jumpers for board   */
    struct bswitch 	*switches;	/* linked list of switches for board  */
    struct software 	*softwares;	/* linked list of softwares for board */
    char 		*id;		/* from BOARD ID stmt                 */
    char 		*name;		/* from BOARD NAME stmt               */
    char 		*mfr;		/* from BOARD MFR stmt                */
    char 		*category;	/* from BOARD CATEGORY stmt           */
    char 		*slot_tag;	/* from BOARD SLOT stmt (opt text)    */
    char 		*comments;	/* from BOARD COMMENTS stmt           */
    char 		*help;		/* from BOARD HELP stmt               */
    unsigned long 	amperage;	/* from BOARD AMPERAGE stmt           */
    unsigned long 	sizing;		/*                                    */
    unsigned short	checksum;	/* of CFG file for this board         */
    unsigned 		duplicate_id;	/* dupid # if CFG name conflict       */
    unsigned 		skirt:1;	/* from BOARD SKIRT stmt              */
    unsigned    	readid:1;	/* from BOARD READID stmt             */
    unsigned    	iocheck:1;	/* from BOARD IOCHECK stmt            */
    unsigned    	disable:1;	/* from BOARD DISABLE stmt            */
    unsigned    	locked:1;	/* is board locked? (unchangeable)    */
    unsigned    	conflict:1;	/*                                    */
    unsigned    	partial_config:1;  /*                                    */
    unsigned    	config_changed:1;  /*                                    */
    unsigned    	duplicate:1;	/* is there another board with same id*/
    int 		length;		/* from BOARD LENGTH stmt             */
    int 		busmaster;	/* from BOARD BUSMASTER stmt          */
    int 		slot_number;	/*                                    */
    int 		eisa_slot;	/*                                    */
    int 		entries;	/*                                    */
    enum board_slot 	slot;		/* from BOARD SLOT stmt               */
};



struct group {
    struct group 	*next;		/* next group or 0                    */
    struct board 	*parent;	/* board this group is describing     */
    struct function 	*functions;	/* linked list of group's functions   */
    char 		*name;		/* from GROUP stmt                    */
    char 		*type;		/* from GROUP TYPE stmt               */
    unsigned 		explicit:1;	/* is group stmt really in CFG file?  */
};



struct function {
    struct function 	*next;		/* next function in group or 0        */
    struct group 	*parent;	/* group this function belongs to     */
    struct subfunction 	*subfunctions;	/* linked list of subfunctions        */
    char 		*name;		/* from FUNCTION stmt                 */
    char 		*type;		/* from FUNCTION TYPE stmt            */
    char 		*connection;	/* from FUNCTION CONNECTION stmt      */
    char 		*comments;	/* from FUNCTION COMMENTS stmt        */
    char 		*help;		/* from FUNCTION HELP stmt            */
    unsigned 		display:1;	/* when FUNCTION SHOW == YES?         */
};



struct subfunction {
    struct subfunction 	*next;		/* next subfunction in list or 0      */
    struct function 	*parent;	/* which function does it belong to   */
    struct choice 	*choices;	/*                                    */
    struct choice 	*config;	/*                                    */
    struct choice 	*current;	/*                                    */
    char 		*name;		/* from SUBFUNCTION stmt              */
    char 		*type;		/* from SUBFUNCTION TYPE stmt         */
    char 		*connection;	/* from SUBFUNCTION CONNECTION stmt   */
    char 		*comments;	/* from SUBFUNCTION COMMENTS stmt     */
    char 		*help;		/* from SUBFUNCTION HELP stmt         */
    unsigned 		explicit:1;	/* is there actually a subfunction stmt?*/
    unsigned 		conflict:1;	/*                                    */
    enum item_status 	status;		/*                                    */
    struct index 	index;		/*                                    */
    int 		entry_num;	/* index used by config() only        */
};



struct choice {
    struct choice 	*next;		/* next choice in the list or 0       */
    struct subfunction 	*parent;	/* subfunction choice belongs to      */
    struct portvar 	*portvars;	/*                                    */
    struct totalmem 	*totalmem;	/*                                    */
    struct subchoice 	*primary;	/*                                    */
    struct subchoice 	*subchoices;	/*                                    */
    struct subchoice 	*config;	/*                                    */
    struct subchoice 	*current;	/*                                    */
    struct eeprom 	*eeprom;		/*                                    */
    char 		*help;		/* from CHOICE HELP stmt              */
    char 		*name;		/* from CHOICE stmt                   */
    char 		*subtype;	/* from CHOICE SUBTYPE stmt           */
    unsigned long 	amperage;	/* from CHOICE AMPERAGE stmt          */
    enum item_status 	status;		/*                                    */
    unsigned 		disable:1;	/* from CHOICE DISABLE stmt           */
    unsigned 		new:1;		/*                                    */
    struct index 	index;		/*                                    */
    int 		entry_num;	/* index used by config() only        */
    unsigned char 	*freeform;	/*                                    */
};



struct totalmem {
    union {
	struct value *values;		/*                                    */
	struct	{
	    unsigned long min;		/*                                    */
	    unsigned long max;		/*                                    */
	} range;			/* used if step != 0                  */
    } u;
    unsigned long step;			/*                                    */
    unsigned long eeprom;			/*                                    */
    unsigned long actual;		/*                                    */
};



struct subchoice {
    struct subchoice 	  *next;	    /* next subchoice in list or 0    */
    struct choice 	  *parent;	    /* choice this subchoice belongs to */
    struct resource_group *resource_groups; /* resources this subchoice uses  */
    unsigned 		  explicit:1;	    /* is there actually a subchoice stmt? */
};



struct resource_group {
    struct resource_group *next;	/* next resource group or 0           */
    struct subchoice 	  *parent;	/* subchoice that owns resources      */
    struct resource 	  *resources;	/* resource chain for this group      */
    struct init 	  *inits;	/* initialization values for resources*/
    enum rg_type 	  type;		/* link, combine, freeform            */
    enum item_status 	  status;	/*                                    */
    struct index 	  index;	/*                                    */
    int 		  entry_num;	/* index used by config() only        */
    unsigned 		  new:1;	/*                                    */
};



struct resource {
    struct resource 	*next;		/* next resource or 0                 */
    struct resource_group *parent;	/* ptr to associated resource group   */
    char 		*share_tag;	/* from resource SHARE stmt           */
    enum resource_type 	type;		/* what type of resource (dma, etc.)  */
    unsigned 		share:1;	/* will it share?                     */
    unsigned 		sharing:1;	/*                                    */
    unsigned 		conflict:1;	/*                                    */
    unsigned 		new:1;		/*                                    */
    int 		entry_num;	/* index used by config() only        */
};



struct irq {
    struct resource 	r;		/* resource header                    */
    struct value 	*values;	/*                                    */
    struct value 	*config;	/*                                    */
    struct value 	*current;	/*                                    */
    enum irq_trigger 	trigger;	/* from IRQ TRIGGER stmt              */
    struct index 	index;		/*                                    */
    int 		index_value;	/*                                    */
};



struct dma {
    struct resource 	r;		/* resource header                    */
    struct value 	*values;	/*                                    */
    struct value 	*config;	/*                                    */
    struct value 	*current;	/*                                    */
    enum data_size 	data_size;	/* from DMA SIZE stmt                 */
    enum dma_timing 	timing;		/* from DMA TIMING stmt               */
    struct index 	index;		/*                                    */
    int 		index_value;	/*                                    */
};



struct port {
    struct resource r;			/* resource header                    */
    union {
	struct	{
	    struct value *values;	/*                                    */
	    struct value *config;	/*                                    */
	    struct value *current;	/*                                    */
	} list;				/* used if step == 0                  */
	struct	{
	    unsigned 	min;		/*                                    */
	    unsigned 	max;		/*                                    */
	    unsigned 	count;		/*                                    */
	    unsigned 	config;		/*                                    */
	    unsigned 	current;	/*                                    */
	} range;			/* used if step != 0                  */
    } u;
    unsigned 		step;		/* if range, # of ports requested     */
    unsigned 		slot_specific:1;/*                                    */
    enum data_size 	data_size;	/* from PORT SIZE stmt                */
    struct index 	index;		/*                                    */
    int 		index_value;	/*                                    */
};



struct memory {
    struct resource 	  r;		/* resource header                    */
    struct memory_address memory;	/* size of memory requested           */
    struct memory_address *address;	/* starting addresses                 */
    unsigned 		  writable:1;	/* from MEMORY WRITABLE stmt          */
    unsigned 		  cache:1;	/* from MEMORY CACHE stmt             */
    unsigned 		  caching:1;	/*                                    */
    enum memory_decode 	  decode;	/* from MEMORY DECODE stmt            */
    enum memory_type 	  memtype;	/* from MEMORY MEMTYPE stmt           */
    enum data_size 	  data_size;	/* from MEMORY SIZE stmt              */
};



struct value {
    struct value 	*next;		/*                                    */
    unsigned long 	min;		/*                                    */
    unsigned long 	max;		/*                                    */
    unsigned 		init_index:15;	/*                                    */
    unsigned 		null_value:1;	/*                                    */
};



struct init {
    struct init 	*next;		/*                                    */
    union {
	struct ioport 	*ioport;	/*                                    */
	struct jumper 	*jumper;	/*                                    */
	struct bswitch 	*bswitch;	/*                                    */
	struct software *software;	/*                                    */
    } ptr;
    struct init_value 	*init_values;	/*                                    */
    struct init_value 	*current;	/*                                    */
    struct {
	unsigned long 	min;		/*                                    */
	unsigned long 	max;		/*                                    */
	unsigned long 	current;	/*                                    */
    } range;
    unsigned long 	mask;		/*                                    */
    unsigned 		memory:1;	/*                                    */
    enum init_type 	type;		/*                                    */
    enum data_type 	data_type;	/*                                    */
};



struct init_value {
    struct init_value 	*next;		/*                                    */
    union {
	char 		*parameter;	/*                                    */
	struct	{
	    unsigned 	data_bits;	/*                                    */
	    unsigned 	tristate_bits;	/*                                    */
	} tripole;
	unsigned 	value;		/*                                    */
    } u;
};



struct portvar {
    struct portvar 	*next;		/*                                    */
    unsigned 		address;	/*                                    */
    unsigned 		slot_specific:1;/*                                    */
    int 		index;		/*                                    */
};



struct ioport {
    struct ioport 	*next;		/*                                    */
    struct {
	unsigned long 	reserved_mask;	/*                                    */
	unsigned long 	unchanged_mask;	/*                                    */
	unsigned long 	forced_bits;	/*                                    */
    } initval;
    struct config_bits 	config_bits;	/*                                    */
    enum data_size 	data_size;	/*                                    */
    int 		portvar_index;	/*                                    */
    int 		index;		/*                                    */
    unsigned 		address;	/*                                    */
    unsigned 		slot_specific:1;/*                                    */
    unsigned 		referenced:1;	/*                                    */
    unsigned 		valid_address:1;/*                                    */
};



struct bswitch {
    struct bswitch 	*next;		/* next switch in chain or 0          */
    struct label 	*labels;	/* from SWITCH LABEL stmt (pos label) */
    char 		*name;		/* from SWITCH NAME stmt              */
    char 		*comments;	/* from SWITCH COMMENTS stmt          */
    char 		*help;		/* from SWITCH HELP stmt              */
    struct {
	unsigned long 	reserved_mask;	/*                                    */
	unsigned long 	forced_bits;	/*                                    */
    } initval;
    struct config_bits 	config_bits;	/*                                    */
    unsigned long 	factory_bits;	/* from SWITCH FACTORY stmt (default) */
    unsigned 		vertical:1;	/* from SWITCH VERTICAL stmt          */
    unsigned 		reverse:1;	/* from SWITCH REVERSE stmt           */
    enum switch_type 	type;		/* from SWITCH STYPE stmt (dip, etc)  */
    int 		width;		/* from SWITCH stmt (# of positions)  */
    int 		index;		/*                                    */
};



struct jumper {
    struct jumper 	*next;		/* next jumper in chain or 0          */
    struct label 	*labels;	/* from JUMPER LABEL stmt (pos label) */
    char 		*name;		/* from JUMPER NAME stmt              */
    char 		*comments;	/* from JUMPER COMMENTS stmt          */
    char 		*help;		/* from JUMPER HELP stmt              */
    struct {
	unsigned long 	reserved_mask;	/*                                    */
	unsigned long 	forced_bits;	/*                                    */
	unsigned long 	tristate_bits;	/*                                    */
    } initval;
    struct {
	unsigned long 	data_bits;	/*                                    */
	unsigned long 	tristate_bits;	/*                                    */
    } factory;
    struct config_bits 	config_bits;	/*                                    */
    struct config_bits 	tristate_bits;	/*                                    */
    unsigned 		vertical:1;	/* from JUMPER VERTICAL stmt          */
    unsigned 		reverse:1;	/* from JUMPER REVERSE stmt           */
    unsigned 		on_post:1;	/*                                    */
    enum jumper_type 	type;		/* from JUMPER JTYPE stmt (inline, etc) */
    int 		width;		/* from JUMPER stmt (# of positions)  */
    int 		index;		/*                                    */
};



struct software {
    struct software 	*next;		/* next software or 0                 */
    char 		*description;	/* from SOFTWARE stmt (text)          */
    char 		*previous;	/*                                    */
    char 		*current;	/*                                    */
    int 		index;		/*                                    */
};



struct label {
    struct label 	*next;		/*                                    */
    char 		*string;	/*                                    */
    int 		position;	/*                                    */
};



struct tag {
    struct tag 		*next;		/* next tag in chain                  */
    char 		*string;	/* tag name                           */
};



struct eeprom {
    struct eeprom 	*next;		/*                                    */
    unsigned char 	offset;		/*                                    */
    unsigned char 	mask;		/*                                    */
    unsigned char 	value;		/*                                    */
};
