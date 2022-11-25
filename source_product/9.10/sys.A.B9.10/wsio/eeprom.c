/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/eeprom.c,v $
 * $Revision: 1.7.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:27:59 $
 */

#include "../h/types.h"
#include "../h/errno.h"
#include "../wsio/eeprom.h"
#include "../wsio/eisa.h"
#include "../h/buf.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/vas.h"
#include "../h/pregion.h"
#include "../h/mman.h"
#include "../h/io.h"
#include "../h/debug.h"
 
extern caddr_t kmem_alloc();
extern caddr_t host_map_mem();
extern caddr_t unmap_host_mem();

int read_from_eeprom();
int write_to_eeprom();
u_short eeprom_section_chksum();
int eeprom_locate_cfg();
int eeprom_write_enable();
int eeprom_write_disable();
int eeprom_exists();
int eeprom_init_eisa_section();

extern struct bus_info_type *eisa[];
extern int eisa_int_enable;
extern int eisa_exists;
extern int eisa_boot_slot;


/* 
 * global structures which mirror the eeprom for easier access 
 */
#ifdef __hp9000s300
struct eeprom_section_header *eeprom_cfg_header;
#endif

struct eeprom_gen_info *eeprom_geninfo;
struct eeprom_per_slot_info *eeprom_slot_info[EISA1_NUM_SLOTS_MAX];
struct eeprom_per_slot_info *esb_slot_info;

#if defined (__hp9000s700) && defined (_WSIO)
struct eisa_boot_info boot_info;
#endif

caddr_t	eeprom_reg_addr;    		/* virtual address of eeprom registers */
caddr_t	eeprom_addr;	    		/* virtual address of eeprom data */

int eeprom_end_offset;			/* offset to end of eisa data in eeprom */
int eeprom_cfg_base = -1;		/* absolute address to geninfo in eeprom */
int esb_slot_base = ESB_SLOT_BASE;	/* special offset for slot 0 information */ 


/* 
 * global variables set by ioctl *_PREP ioctl's and used by 
 * read and write they are initialized on open 				    
 */
int slot_to_read;
int slot_to_write;
int function_to_read;

#if defined (__hp9000s700) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
/* globals used for non-boot, boot testing */
int write_to_boot=0;
struct eeprom_per_slot_info s_info;
#endif

/* need an ident string for backpatch identification */
static char eeprom_revision[] = "$Header: eeprom.c,v 1.7.83.3 93/09/17 20:27:59 kcs Exp $";

/* exclusive open flag */
char eeprom_opened = 0;	

/* flag to allow strict "all attach then all init" protocol to be preserved */
int eisa_reinit = 0;


/* convert driver error codes to meaningful errno values */
#define ERRNO(x)	((x) == 0 ? 0 : (x) == 1 ? ENXIO : (x) == 2 ? ENOENT \
			: (x) == 3 ? ESRCH : (x) == 4 ? EINVAL : ENOEXEC)



/*----------------------------------------------------------------------------*/
eeprom_open(dev, flags)
/*----------------------------------------------------------------------------*/
/* This is the driver open  routine.  It  operates  along the  principle
 * that first open does all required  initialization.  Main  elements of
 * that initialization  include:  allocating space for slot information,
 * checksum validation, and reading configuration header information.
 */

dev_t dev;
int flags;
{
	int i, num_slots;

	/* this will use iomap() to map in address space */
	if (!eeprom_exists())
		return(ENODEV);

	/* notify eisa_config that eisa does not exist */
	if (!eisa_exists)
		return(ENXIO);

	/* enforce exclusive open */
	if (eeprom_opened)	
		return(EBUSY);

	/* see if first open attempt */
	if (eeprom_cfg_base == -1) {	
		/* this sets up eeprom_cfg_base and eeprom_end_offset */
		if (eeprom_locate_cfg() != 0)
			return(ESRCH);

		/* 
		 * allocate space for general info structure 
		 */
		if ((eeprom_geninfo = (struct eeprom_gen_info *)
			kmem_alloc(sizeof(struct eeprom_gen_info))) == NULL) {
		    printf("EISA_WARNING: unable to allocate eeprom_geninfo\n");
#ifdef __hp9000s300
			/* 
			 * section    specific   space    allocated   in
			 * eeprom_locate_cfg, freed here on error
			 */
			kmem_free(eeprom_cfg_header, sizeof(struct eeprom_section_header));
			eeprom_cfg_header = NULL;
#endif
			return(ENOMEM);
		}

		/* 
		 * load the general info structure
		 */
		read_from_eeprom(eeprom_geninfo, eeprom_cfg_base, sizeof(struct eeprom_gen_info));


		/* 
		 * See if this section has *NEVER* been initialized -- new box 
		 * These values in eeprom are set up by manufacturing          
		 */
		if ((eeprom_geninfo->flags & SECTION_NOT_INITIALIZED) ||
		    (eeprom_geninfo->number_of_slots == 0)) {
			/* Initialize EISA section */
			if (clear_eeprom(-1) < 0)
				return(EIO);
		}

		/* 
		 * allocate space for in RAM copy of slot 0 info 
		 */
		if ((esb_slot_info = (struct eeprom_per_slot_info *)
		    kmem_alloc(sizeof(struct eeprom_per_slot_info))) == NULL) {
			printf("EISA_WARNING: unable to allocate eeprom_slot_info for slot 0\n");
			kmem_free(eeprom_geninfo, sizeof(struct eeprom_gen_info));
			eeprom_geninfo == NULL;
			return(ENOMEM);
		}

		/* 
		 * allocate  enough  space  for the in RAM  copy of slot
		 * info we also  chose to deal  with the  scenerio  of a
		 * customer  slot   expansion   upgrade/downgrade.  this
		 * forces us to  allocate  the  maximum  amount of space
		 * required for either operation.
		 */

		num_slots = MAX(eeprom_geninfo->number_of_slots, 
			((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->number_of_slots);

		for (i = 0; i < num_slots; i++) {
			if ((eeprom_slot_info[i] = (struct eeprom_per_slot_info *)
			    kmem_alloc(sizeof(struct eeprom_per_slot_info))) == NULL) {
				printf("EISA_WARNING: unable to allocate eeprom_slot_info\n");
				for (i--; i >= 0; i--) {
					kmem_free(eeprom_slot_info[i],
					sizeof(struct eeprom_per_slot_info));
					eeprom_slot_info[i] = NULL;
				}
				kmem_free(eeprom_geninfo, sizeof(struct eeprom_gen_info));
				eeprom_geninfo == NULL;
				kmem_free(esb_slot_info, sizeof(struct eeprom_per_slot_info));
				esb_slot_info == NULL;
#ifdef __hp9000s300
				kmem_free(eeprom_cfg_header, sizeof(struct eeprom_section_header));
				eeprom_cfg_header == NULL;
#endif
				return(ENOMEM);
			}
		} /* space allocation */

#if defined (hp9000s800) && defined (_WSIO)
		/* 
		 * check checksum to see if it is worth reading the info stored here
		 */
		if (eeprom_geninfo->checksum != eeprom_section_chksum()) {
			printf("EISA_WARNING: EEPROM checksum invalid, clearing eeprom\n");
			/* since we found garbage, we must put the system into
			 * a known safe state.
			 */
			if (clear_eeprom(-1) < 0)
				return(EIO);
		}
#endif

		/* 
		 * read in per_slot_info structure for slot 0 
		 */
		read_from_eeprom(esb_slot_info, eeprom_cfg_base + esb_slot_base,
			sizeof(struct eeprom_per_slot_info));

		/* 
		 * read in per_slot_info structures 
		 */
		for (i = 0; i < eeprom_geninfo->number_of_slots; i++) {
		    read_from_eeprom(eeprom_slot_info[i],
			eeprom_cfg_base + sizeof(struct eeprom_gen_info)
			+ sizeof(struct eeprom_per_slot_info) * i,
			sizeof(struct eeprom_per_slot_info));
		}
	} /* first open */

	/* 
	 * if we get here it is a successful open 
	 */
	eeprom_opened = 1;

	/* this will be the same number if eeprom was bad, else the real number */
	((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->number_of_slots 
		= eeprom_geninfo->number_of_slots;

	/* set to invalid values to disallow a read or write without requisite ioctl call */
	slot_to_write = -1;
	slot_to_read = -1;
	function_to_read = -1;

	return(0);
}


/*----------------------------------------------------------------------------*/
eeprom_close(dev)
/*----------------------------------------------------------------------------*/
/* provide for exclusive open cleanup
 */

dev_t dev;
{
	eeprom_opened = 0;
}


/*----------------------------------------------------------------------------*/
clear_eeprom(cfg_rev)
/*----------------------------------------------------------------------------*/
/* Assumes eeprom_cfg_header is valid 
 * Assumes that space has been allocated for all possible slots
 */

short	cfg_rev;
{
	int	i,ret = 0;
	int initial_eeprom = 0;
	u_short write_count;
	struct eeprom_per_slot_info *tmp_slot_info;

	if ((eeprom_geninfo->flags & SECTION_NOT_INITIALIZED) ||
	    (eeprom_geninfo->number_of_slots == 0)) {
		initial_eeprom = 1;
		write_count = 0;
	} else 
		write_count = eeprom_geninfo->write_count;

	bzero(eeprom_geninfo,sizeof(struct eeprom_gen_info));
	eeprom_geninfo->flags = 0;	/* clear not-initialized bit */
	eeprom_geninfo->major_cfg_rev = cfg_rev >> 8;
	eeprom_geninfo->minor_cfg_rev = cfg_rev & 0xff;
	eeprom_geninfo->write_count = ++write_count;

	/* set to system board ID derived default number of slots */
	eeprom_geninfo->number_of_slots 
		= ((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->number_of_slots;

#ifdef __hp9000s300
	eeprom_cfg_header->write_count++;
#endif	

	/* write out gen_info header */
	if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base, sizeof(struct eeprom_gen_info)) < 0)
		ret = -EIO;

	if (!initial_eeprom) {
		/* save it out for normal slots */
		for (i = 0; i < eeprom_geninfo->number_of_slots; i++) {
			bzero(eeprom_slot_info[i], sizeof(struct eeprom_per_slot_info));
			eeprom_slot_info[i]->slot_id = SLOT_IS_EMPTY;

			if (write_to_eeprom(eeprom_slot_info[i], eeprom_cfg_base
					+ sizeof(struct eeprom_gen_info)
					+ sizeof(struct eeprom_per_slot_info) * i,
					sizeof(struct eeprom_per_slot_info)) < 0)
				ret = -EIO;
		}

		/* save out for slot 0 */
		bzero(esb_slot_info, sizeof(struct eeprom_per_slot_info));
		esb_slot_info->slot_id = SLOT_IS_EMPTY;
		if (write_to_eeprom(esb_slot_info, eeprom_cfg_base + esb_slot_base,
					sizeof(struct eeprom_per_slot_info)) < 0)
				ret = -EIO;

	} else {
		/* allocate a "clear" slot info structure */
		if ((tmp_slot_info = (struct eeprom_per_slot_info *)
		kmem_alloc(sizeof(struct eeprom_per_slot_info))) == NULL) {
			printf("EISA_WARNING: unable to allocate eeprom_slot_info\n");
			return(-EIO);
		}

		/* set it up to be empty */
		bzero(tmp_slot_info, sizeof(struct eeprom_per_slot_info));
		tmp_slot_info->slot_id = SLOT_IS_EMPTY;

		/* save it out for normal slots */
		for (i = 0; i < eeprom_geninfo->number_of_slots; i++) {
			if (write_to_eeprom(tmp_slot_info, eeprom_cfg_base
					+ sizeof(struct eeprom_gen_info)
					+ sizeof(struct eeprom_per_slot_info) * i,
					sizeof(struct eeprom_per_slot_info)) < 0)
				ret = -EIO;
		}

		/* save out for slot 0 */
		if (write_to_eeprom(tmp_slot_info, eeprom_cfg_base + esb_slot_base,
					sizeof(struct eeprom_per_slot_info)) < 0)
				ret = -EIO;
	}

	/* 
	 * calculate and store checksum 
	 */
#ifdef __hp9000s300
	eeprom_cfg_header->checksum = eeprom_section_chksum();
	if (write_to_eeprom(eeprom_cfg_header, eeprom_cfg_base-sizeof(struct eeprom_section_header),
				sizeof(struct eeprom_section_header)) < 0)
		ret = -EIO;
#else
	eeprom_geninfo->checksum = eeprom_section_chksum();
	if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base, sizeof(struct eeprom_gen_info)) < 0)
		ret = -EIO;
#endif

	return(ret);
}


/*----------------------------------------------------------------------------*/
eeprom_ioctl(dev, function, arg, flag)
/*----------------------------------------------------------------------------*/

dev_t	dev;
int	function;
int	*arg;
int	flag;
{
	struct func_slot *func_slot;
	struct eeprom_slot_data	*slot_data;
	int i, ret = 0;
	struct isc_table_type *isc;
	struct eisa_system_board *esb;
	struct eeprom_per_slot_info *tmp_slot_info;

#if defined (hp9000s800) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	write_to_boot = 0;
#endif

	switch (function) {

	case CLEAR_EEPROM:
		return(-clear_eeprom(*arg));

	case READ_SLOT_INFO:
		slot_data = (struct eeprom_slot_data *)arg;
#if defined (hp9000s800) && defined (_WSIO)
		/* 
		 * kludge to return  eisa_config real contents of eeprom
		 * for a slot which we booted from and used default boot
		 * info from PDC/IODC, rather than info we used to boot
		 */
		if (slot_data->slot_info == eisa_boot_slot) {
			int ret_code = 0;
			if ((tmp_slot_info = (struct eeprom_per_slot_info *)
			    kmem_alloc(sizeof(struct eeprom_per_slot_info))) == NULL)
				return(ENOMEM);

			read_from_eeprom(tmp_slot_info,
				eeprom_cfg_base + sizeof(struct eeprom_gen_info)
				+ sizeof(struct eeprom_per_slot_info) * (eisa_boot_slot - 1),
				sizeof(struct eeprom_per_slot_info));

			/* the rest of the code in this if is lifted from read_eisa_slot_data */
			if (eeprom_cfg_base == -1) {
				kmem_free(tmp_slot_info, sizeof(struct eeprom_per_slot_info));
				return(ESRCH);
			}

			if (eisa_boot_slot < 0 || eisa_boot_slot > eeprom_geninfo->number_of_slots){
				kmem_free(tmp_slot_info, sizeof(struct eeprom_per_slot_info));
				return(ENXIO);	/* invalid slot */
			}

			if (tmp_slot_info->slot_id == SLOT_IS_EMPTY) {
				kmem_free(tmp_slot_info, sizeof(struct eeprom_per_slot_info));
				return(ENOENT);	/* no slot info */
			}

			slot_data->slot_id             = tmp_slot_info->slot_id;
			slot_data->slot_info           = tmp_slot_info->slot_info;
			slot_data->minor_cfg_rev       = eeprom_geninfo->minor_cfg_rev;
			slot_data->major_cfg_rev       = eeprom_geninfo->major_cfg_rev;
			slot_data->checksum            = tmp_slot_info->checksum;
			slot_data->number_of_functions = tmp_slot_info->number_of_functions;
			slot_data->function_info       = tmp_slot_info->function_info;
			slot_data->flags               = tmp_slot_info->flags;

			return(0);
		} else
#endif
			return(ERRNO(-read_eisa_slot_data(slot_data->slot_info, arg)));

	case READ_FUNCTION_PREP:
		func_slot = (struct func_slot *)arg;

		if (func_slot->slot == 0) {
			if (func_slot->function >= esb_slot_info->number_of_functions)
				return(EINVAL);
                        function_to_read = func_slot->function;
                        slot_to_read = func_slot->slot;
			return(0);
		}

		if (func_slot->slot > eeprom_geninfo->number_of_slots || func_slot->slot < 0)
			return(ENXIO);
		else if (eeprom_slot_info[func_slot->slot - 1] ->slot_id == SLOT_IS_EMPTY)
			return(ENOENT);		/* slot empty */
		else if (func_slot->function < 0 || func_slot->function >= 
			eeprom_slot_info[func_slot->slot - 1]->number_of_functions)
			return(EINVAL);

		function_to_read = func_slot->function;
		slot_to_read = func_slot->slot;
		return(0);

	case WRITE_SLOT_PREP:
		if (*arg < 0 || *arg > eeprom_geninfo->number_of_slots)
			return(ENXIO);

		slot_to_write = *arg;
		return(0);

	case READ_SLOT_ID: /* Read physical card ID register */
		if ( !(0 <= *arg && *arg <= eeprom_geninfo->number_of_slots))
			return(ENXIO);

		if (*arg == 0) 
			*arg = ((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->sysbrd_parms->id;
		else  
			*arg = read_eisa_slot_id(*arg);

		return(0);

	case CHG_NUM_SLOTS:
		/* 
		 * this ioctl allows the number of EISA slots to be changed.
		 */
		if (!(0 <= *arg && *arg <= ((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->sysbrd_parms->max_slots))
			return(ENXIO);

		/* 
		 * get space for new slot information 
		 */
		for (i = eeprom_geninfo->number_of_slots; i < *arg; i++) {
			if ((eeprom_slot_info[i] = (struct eeprom_per_slot_info *)
			    kmem_alloc(sizeof(struct eeprom_per_slot_info))) == NULL) {

				/* 
				 * since we failed, give what we took back 
				 */
				for (i--; i >= eeprom_geninfo->number_of_slots; i--) {
					kmem_free(eeprom_slot_info[i],
					sizeof(struct eeprom_per_slot_info));
					eeprom_slot_info[i] = NULL;
				}
				return(ENOMEM);
			}
		} /* space allocation */

		/* save the new value */
		eeprom_geninfo->number_of_slots = *arg; 

		/*
		 * write it out
		 */
		if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base,
					sizeof(struct eeprom_gen_info)) < 0)
			return(EIO);
#ifdef __hp9000s300
		eeprom_cfg_header->write_count++;
		eeprom_cfg_header->checksum = eeprom_section_chksum();
		if (write_to_eeprom(eeprom_cfg_header, eeprom_cfg_base
		    - sizeof(struct eeprom_section_header),
		    sizeof(struct eeprom_section_header)) < 0)
			return(EIO);
#endif	
		return(0);
		
	case GET_NUM_SLOTS:
		*arg = eeprom_geninfo->number_of_slots;
		return(0);

	case BREAK_CHKSUM:
#if defined (__hp9000s800) && defined (_WSIO)
		/* calculate checksum */
		eeprom_geninfo->checksum = eeprom_section_chksum() - 13;
		if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base,
 		    sizeof(struct eeprom_gen_info)) < 0)
			return(EIO);
#else
		/* calculate checksum */
		eeprom_cfg_header->checksum = eeprom_section_chksum() - 13;
		if (write_to_eeprom(eeprom_cfg_header, eeprom_cfg_base - 
		    sizeof(struct eeprom_section_header),
		    sizeof(struct eeprom_section_header)) < 0)
			return(EIO);
#endif
		return(0);

#if defined (__hp9000s700) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	case CREATE_BOOT:  /* Creates boot info just like PDC :-) */
		if (*arg < 1 || *arg > eeprom_geninfo->number_of_slots)
			return(ENXIO);
			
		slot_to_write = *arg;
		write_to_boot = 1;

		return(0);

	case READ_BOOT: /* Read boot info */
		read_from_eeprom(arg, EEPROM_BOOT_OFFSET, sizeof(struct eisa_boot_info));
		return(0);
		
	case READ_BOOT_FUNC: /* Read boot info */
		function_to_read = 0;
		slot_to_read = 1; /* dummy value */
		write_to_boot = 1;
		return(0);

	case FIX_CHKSUM:
		/* calculate checksum */
		eeprom_geninfo->checksum = eeprom_section_chksum();
		if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base, 
			sizeof(struct eeprom_gen_info)) < 0)
			return(EIO);
		return(0);

	case WRITE_EEPROM_DATA:
		/* used for various eeprom tests */
		write_to_eeprom(arg, 0, 3072);
	 /*	write_to_eeprom((char *)((int)arg + 3072), 0x1c00, 512); */
		return(0);

	case READ_EEPROM_DATA:
		/* used for various eeprom tests */
		read_from_eeprom(arg, 0, 3072);
	/*	read_from_eeprom((char *)((int)arg + 3072), 0x1c00, 512); */
		return(0);
#endif

	case INIT_NEW_CARDS:
		/*
		 * In order to allow a driver to fully  preserve the all
		 * attach  then all init  protocol,  it can look at this
		 * flag and indicate a failure to  initialize_slot  from
		 * it's attach routine.
		 *
		 * Also, IND's lan drivers need to specifically  know if
		 * this is a reinit case in their init routines.
		 */
		eisa_reinit = 1;

		msg_printf("EISA Card Initialization\n");
     		for (i = 1; i <= ((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->number_of_slots; i++) {

			/* only do those cards which have not yet been done */
                	if (isc_table[EISA1_MIN + i] == NULL) {

				/* call driver's attach routine */
				msg_printf("Slot %d: ",i);
                        	if (!initialize_slot(i)) {
					msg_printf(" -- Skipping\n");
					continue;
				}

				/* call driver's init routine(s) */
				isc = isc_table[EISA1_MIN + i];
				while (isc != NULL) {
					if (isc->gfsw != NULL && isc->gfsw->init != NULL)
						/* call init routine */
						(*isc->gfsw->init)(isc); 

					/* next function of multi-function card */
					isc = isc->next_ftn;
				}
			}

        	}

		/* enable interrupts on the system board */
		esb = ((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->esb;
		esb->int1_int_mask_reg = eisa_int_enable & 0xff;
		esb->int2_int_mask_reg = eisa_int_enable >> 8;
			
		eisa_reinit = 0;
		return(0);

	case WAS_BOARD_INITED: /* did driver accept this card? */
		if (*arg < 0 || *arg > eeprom_geninfo->number_of_slots)
			return(ENXIO);

                if (isc_table[EISA1_MIN + *arg] == NULL)
			return(ENXIO);

		else return(0);

	case CHECK_CHKSUM:
#if defined (__hp9000s800) && defined (_WSIO)
		*arg = eeprom_geninfo->checksum == eeprom_section_chksum();
#else
		*arg = eeprom_cfg_header->checksum == eeprom_section_chksum();
#endif
		return(0);

	default:
		return(EINVAL);
	}
}


/*----------------------------------------------------------------------------*/
eeprom_strategy(bp)
/*----------------------------------------------------------------------------*/
/* slot_to_read,  function_to_read,  and slot_to_write are globals which
 * must be first set up the "prep" ioctls.
 */

struct buf *bp;
{
	int ret;

	bp->b_resid = 0;

	if (bp->b_flags & B_READ) {
		if (bp->b_bcount != sizeof(struct eeprom_function_data)) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EINVAL;
		} else if (ret = read_eisa_function_data(slot_to_read,
				function_to_read, bp->b_un.b_addr)) {
			bp->b_flags |= B_ERROR;
			bp->b_error = ERRNO(-ret);
		}
	} else if (ret = write_eisa_slot_data(slot_to_write,
					bp->b_un.b_addr, bp->b_bcount)) {
			bp->b_flags |= B_ERROR;
			bp->b_error = -ret;
	}

	iodone(bp);
}


/*----------------------------------------------------------------------------*/
eeprom_read(dev, uio)
/*----------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	int ret;

	if (slot_to_read == -1 || function_to_read == -1)
		return(EACCES);

	ret = physio(eeprom_strategy, NULL, dev, B_READ, minphys, uio);

	slot_to_read = -1;
	function_to_read = -1;

	return(ret);
}


/*----------------------------------------------------------------------------*/
eeprom_write(dev, uio)
/*----------------------------------------------------------------------------*/

dev_t dev;
struct uio *uio;
{
	int ret;

	if (slot_to_write == -1) 
		return(EACCES);
	
	ret = physio(eeprom_strategy, NULL, dev, B_WRITE, minphys, uio);

	slot_to_write = -1;

	return(ret);
}


/*----------------------------------------------------------------------------*/
read_eisa_slot_data(slot, slot_data)
/*----------------------------------------------------------------------------*/
/* This routine loads the request structure with the in memory copy of
 * the slot information.
 */

int slot;
struct eeprom_slot_data	*slot_data;
{
	struct eeprom_per_slot_info *slot_info;

	if (eeprom_cfg_base == -1)
		return(-3);

	if ( !(0 <= slot && slot <= eeprom_geninfo->number_of_slots))
		return(-1);	/* invalid slot */

	if (slot == 0) {
		slot_info = esb_slot_info;
        } else {
		slot_info = eeprom_slot_info[slot - 1];
	} 

	if (slot_info->slot_id == SLOT_IS_EMPTY)
		return(-2);	/* no slot info */

	slot_data->slot_id = slot_info->slot_id;
	slot_data->slot_info = slot_info->slot_info;
	slot_data->minor_cfg_rev = eeprom_geninfo->minor_cfg_rev;
	slot_data->major_cfg_rev = eeprom_geninfo->major_cfg_rev;
	slot_data->checksum = slot_info->checksum;
	slot_data->number_of_functions = slot_info->number_of_functions;
	slot_data->function_info = slot_info->function_info;
	slot_data->flags = slot_info->flags;

	return(0);
}


/*----------------------------------------------------------------------------*/
read_eisa_function_data(slot, function, efd)
/*----------------------------------------------------------------------------*/

int slot; 
int function;
struct eeprom_function_data *efd;
{
	int this_function, function_size, function_offset, function_base;
	int i, j, offset;
	int num_selections_defined, num_type_bytes;
	short size;
	u_char *func_data;  /* data in write format to be converted */
	u_char *ptr;
	struct eeprom_per_slot_info *slot_info;

	if (eeprom_cfg_base == -1)
		return(-3);

	if (slot < 0 || slot > eeprom_geninfo->number_of_slots)
		return(-1);	/* invalid slot */

#if defined (__hp9000s800) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	if (write_to_boot) {
		slot_info = &s_info; 
		read_from_eeprom(slot_info, EEPROM_BOOT_OFFSET+sizeof(int), sizeof(s_info));
	} else
#endif

	if (slot == 0) {
		slot_info = esb_slot_info;
        } else {
		slot_info = eeprom_slot_info[slot - 1];
	}

	if (slot_info->slot_id == SLOT_IS_EMPTY)
		return(-2);	/* slot empty */

	if (function < 0 || function >= slot_info->number_of_functions)
		return(-4);	/* invalid function */
	
	bzero(efd, sizeof(struct eeprom_function_data));

	efd->slot_id = slot_info->slot_id;
	efd->slot_info = slot_info->slot_info;
	efd->slot_features = slot_info->slot_features;
	efd->minor_cfg_ext_rev = slot_info->minor_cfg_ext_rev;
	efd->major_cfg_ext_rev = slot_info->major_cfg_ext_rev;

	/* 
	 * convert this function's data (write_eisa_slot) format into 
	 * eeprom_function_data (read_eisa_function) format           
	 */
	offset = slot_info->cfg_data_offset;

	size = slot_info->cfg_data_size;
	if ((func_data = (u_char*)kmem_alloc(size)) == NULL) {
		printf("EISA WARNING: unable to allocate: func_data\n");
		return(-5);
	}
	read_from_eeprom(func_data, offset, size);

	/* 
	 * locate the data for the function of interest within the slot data 
	 */
	function_size = function_offset = 0;
	this_function = 0;
	do {
		function_base = function_offset += function_size;
		function_size = func_data[function_offset++];
		function_size += func_data[function_offset++] << 8;
	} while (this_function++ < function) ;

	/* 
	 * copy selections 
	 */
	num_selections_defined = func_data[function_offset++];
	for (i = 0; i < num_selections_defined && i < MAX_SEL_ENTRIES; i++)
		efd->selection[i] = func_data[function_offset++];

	if (i < num_selections_defined)
		return(-5);	/* error -- corrupt data */

	/* 
	 * function info byte 
	 */
	efd->function_info = func_data[function_offset++];

	/* 
	 * copy type string 
	 */
	if (efd->function_info & HAS_TYPE_ENTRY) {
		num_type_bytes = func_data[function_offset++];
		for (i = 0; i < num_type_bytes && i < MAX_TYPE_ENTRIES; i++)
			efd->type[i] = func_data[function_offset++];
		if (i < num_type_bytes)
			return(-5);	/* error -- corrupt data */
	}
	
	if (efd->function_info & HAS_MEMORY_ENTRY) {
		for (i = 0; i < MAX_MEM_ENTRIES; i++) {
			for (j = 0; j < 7; j++)
				efd->mem_cfg[i][j]
						= func_data[function_offset++];
			if (!(efd->mem_cfg[i][0] & MEM_MORE_ENTRIES))
				break;		/* don't increment i */
		}
		if (i == MAX_MEM_ENTRIES)
			return(-5);	/* error -- corrupt data */
	}
			
	if (efd->function_info & HAS_IRQ_ENTRY) {
		for (i = 0; i < MAX_INT_ENTRIES; i++) {
			for (j = 0; j < 2; j++)
				efd->intr_cfg[i][j]
						= func_data[function_offset++];
			if (!(efd->intr_cfg[i][0] & INT_MORE_ENTRIES))
				break;
		}
		if (i == MAX_INT_ENTRIES)
			return(-5);	/* error -- corrupt data */
	}
	
	if (efd->function_info & HAS_DMA_ENTRY) {
		for (i = 0; i < MAX_DMA_ENTRIES; i++) {
			for (j = 0; j < 2; j++)
				efd->dma_cfg[i][j]
						= func_data[function_offset++];
			if (!(efd->dma_cfg[i][0] & DMA_MORE_ENTRIES))
				break;
		}
		if (i == MAX_DMA_ENTRIES)
			return(-5);	/* error -- corrupt data */
	}

	if (efd->function_info & HAS_PORT_ENTRY) {
		for (i = 0; i < MAX_PORT_ENTRIES; i++) {
			for (j = 0; j < 3; j++)
				efd->port_cfg[i][j]
						= func_data[function_offset++];
			if (!(efd->port_cfg[i][0] & PORT_MORE_ENTRIES))
				break;
		}
		if (i == MAX_PORT_ENTRIES)
			return(-5);	/* error -- corrupt data */
	}
	
	if (efd->function_info & HAS_PORT_INIT) {
		for (i = 0; i < 60; i++) {
			if (function_offset - function_base
							== function_size + 2)
				break;
			efd->init_data[i] = func_data[function_offset++];
		}
	}

	if (efd->function_info & CFG_FREEFORM) {
		ptr = (u_char*)(efd->mem_cfg);
		*ptr++ = size = func_data[function_offset++];
		if (size > MAX_FREEFORM_SIZE)
			return(-5);	/* error -- corrupt data */
		for (i = 0; i < size; i++)
			*ptr++ = func_data[function_offset++];
	}

	if (function_offset - function_base != function_size + 2)
		return(-5);	/* error -- corrupt data */

	return(0);
}


/*----------------------------------------------------------------------------*/
write_eisa_slot_data(slot, write_data, nbytes)
/*----------------------------------------------------------------------------*/

int slot, nbytes;
caddr_t	write_data;
{
	short func_base, size; 
	int nfuncs, offset, i;
	u_char *data, func_info;
	int ret = 0;
	int num_slots;
	int index;
	struct eeprom_per_slot_info *slot_info;

	if (slot < 0 || slot > eeprom_geninfo->number_of_slots)
		return(-ENOENT);	/* invalid slot */

	data = (u_char*)write_data;

#if defined (__hp9000s800) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	if (write_to_boot)
		slot_info = &s_info;
	else
#endif

	if (slot == 0) {
		slot_info = esb_slot_info;
	} else {
		slot_info = eeprom_slot_info[slot - 1];
	}

	/* 
	 * calculate number of functions in slot 
	 */
	for (nfuncs = 0, func_base = 8, func_info = 0;
			size = data[func_base] + (data[func_base + 1] << 8);
			nfuncs++, func_base += size + 2) {
		func_info |= data[func_base + 2 + data[func_base + 2] + 1];
	}
	func_base += 2;

	if (func_base != nbytes - 2)	/* sanity */
		return(-EINVAL);

	/* we should do more elaborate data validation here */

	slot_info->slot_id = (data[0]<<24)+(data[1]<<16)+(data[2]<<8)+data[3];
	slot_info->slot_info = data[4];
	slot_info->slot_features = data[5];
	slot_info->minor_cfg_ext_rev = data[6];
	slot_info->major_cfg_ext_rev = data[7];
	slot_info->number_of_functions = nfuncs;
	slot_info->cfg_data_size = nbytes - 10;
	slot_info->function_info = func_info;
	slot_info->checksum = data[func_base++];
	slot_info->checksum += data[func_base++] << 8;
	if (slot_info->minor_cfg_ext_rev & SLOT_IS_ISA)
                slot_info->flags = SLOT_IS_ISA;
        else slot_info->flags = 0;

	offset = 0;
	for (i = 0; i < slot - 1; i++)
		offset += eeprom_slot_info[i]->cfg_data_size;

#if defined (__hp9000s800) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	if (write_to_boot) 
		slot_info->cfg_data_offset = EEPROM_BOOT_OFFSET+sizeof(struct eisa_boot_info);
	else 
#endif

	if (slot == 0) {
		int i;
		slot_info->cfg_data_offset = eeprom_cfg_base + esb_slot_base
			+ sizeof(struct eeprom_per_slot_info);

#define SET_NUM_SLOTS 1
#ifdef SET_NUM_SLOTS
		/* get offset to function information */
		index = 8 + 2 + data[10] + 1;

		/* verify that this is freeform, not a good check, but better than none */
		if (data[index] & 0x40 == 0x40) {
			index++; /* point to type field length */
			index += data[index]; /* point to first data */
			index++; /* one more for the type count itself */
			num_slots = data[index]-1;

			if (!(0 <= num_slots && num_slots <= ((struct eisa_bus_info *)
				eisa[EISA1]->spec_bus_info)->sysbrd_parms->max_slots))
				ret = EIO;

			/* 
			 * get space for new slot information 
			 */
			for (i = eeprom_geninfo->number_of_slots; i < num_slots; i++) {
				if ((eeprom_slot_info[i] = (struct eeprom_per_slot_info *)
				    kmem_alloc(sizeof(struct eeprom_per_slot_info))) == NULL) {

					/* 
					 * since we failed, give what we took back 
					 */
					for (i--; i >= eeprom_geninfo->number_of_slots; i--) {
						kmem_free(eeprom_slot_info[i],
						sizeof(struct eeprom_per_slot_info));
						eeprom_slot_info[i] = NULL;
					}
					ret = ENOMEM;
				}
			} /* space allocation */

			/* see if we are still on solid ground */
			if (ret == 0) {
				if (eeprom_geninfo->number_of_slots != num_slots)
					printf("*** Changing the number of EISA slots to: %d\n", num_slots);
				/* save the new value */
				eeprom_geninfo->number_of_slots = num_slots; 
			}
		}
#endif /* set num slots */
	} else {
		slot_info->cfg_data_offset = eeprom_cfg_base + sizeof(struct eeprom_gen_info)
			+ sizeof(struct eeprom_per_slot_info)
			* eeprom_geninfo->number_of_slots + offset;
	}


#ifdef __hp9000s300
	eeprom_geninfo->write_count = ++eeprom_cfg_header->write_count;
#else
	eeprom_geninfo->write_count++;
#endif
	if (write_to_eeprom(&data[8],slot_info->cfg_data_offset,nbytes - 10)< 0)
		ret = -EIO;

#if defined (__hp9000s800) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	if (write_to_boot) {
	if (write_to_eeprom(&slot,EEPROM_BOOT_OFFSET, sizeof(slot)) < 0)
		ret = -EIO;
	if (write_to_eeprom(slot_info,EEPROM_BOOT_OFFSET+sizeof(int),
			sizeof(struct eeprom_per_slot_info)) < 0)
		ret = -EIO;
	} else {
#endif

	if (slot == 0) {
		if (write_to_eeprom(slot_info, eeprom_cfg_base + esb_slot_base,
			sizeof(struct eeprom_per_slot_info)) < 0)
		ret = -EIO;
	} else {
		if (write_to_eeprom(slot_info, eeprom_cfg_base
				+ sizeof(struct eeprom_gen_info)
				+ sizeof(struct eeprom_per_slot_info) * (slot - 1),
				sizeof(struct eeprom_per_slot_info)) < 0)
			ret = -EIO;

	} 


	/* write out incremented write count and new number of slots if set above */
	if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base, sizeof(struct eeprom_gen_info)) < 0)
		ret = -EIO;
#if defined (__hp9000s800) && defined (_WSIO) && defined(EISA_TESTS_BOOT)
	} /* a normal write to eeprom */
#endif

#ifdef __hp9000s300
	eeprom_cfg_header->checksum = eeprom_section_chksum();
	if (write_to_eeprom(eeprom_cfg_header, eeprom_cfg_base
			- sizeof(struct eeprom_section_header),
			sizeof(struct eeprom_section_header)) < 0)
		ret = -EIO;
#else
	/* calculate checksum */
	eeprom_geninfo->checksum = eeprom_section_chksum();
	if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base,
				sizeof(struct eeprom_gen_info)) < 0)
		ret = -EIO;
#endif
	return(ret);
}


#ifdef __hp9000s300
#define LOCATION (eeprom_addr + (offset*2) + 1)
#else
#define LOCATION (eeprom_addr + offset)
#endif

/*----------------------------------------------------------------------------*/
read_from_eeprom(buf, offset, size)
/*----------------------------------------------------------------------------*/
/* no return value; always succeeds */

caddr_t buf;
int	offset, size;
{
	for (; size--; offset++)
		*(buf++) = *(LOCATION);
}


/*----------------------------------------------------------------------------*/
int write_to_eeprom(buf, offset, size)
/*----------------------------------------------------------------------------*/

caddr_t	buf;
int	offset, size;
{

	char	char_to_write;
	int	i,j;

	if (eeprom_write_enable() == -1) 
		return(-1);	

	/* LOCATION is declared above */
	for (snooze(500); size--; offset++, snooze(500)) {
		char_to_write = *(buf++);


#ifdef TURN_ON_EEPROM_WRITE_CHECKING
#if defined(__hp9000s700) && defined(_WSIO)
		if (!(offset < eeprom_end_offset || (0x1c00 <= offset && offset < 0x2000))) {
			eeprom_write_disable();
			return(-4);
		}
#else
		/* check to ensure we don't write outside our space */
		if ((offset*2 + 1) >= eeprom_end_offset) {
			eeprom_write_disable();
			return(-4);
		}
#endif
#endif
		*(LOCATION) = char_to_write;

		for (i = 1; i <= 20; i++) { /* wait up to 10ms */
			snooze(500);  /* wait .5ms */
			if (*(LOCATION) == char_to_write) 
				break;
		}
		if (*(LOCATION) != char_to_write) {
			eeprom_write_disable();
			return(-2);
		}
	}

	if (eeprom_write_disable() < 0)
		return(-3);
	return(0);
}


#ifdef __hp9000s300
/*----------------------------------------------------------------------------*/
u_short eeprom_section_chksum()
/*----------------------------------------------------------------------------*/
/* Calculate  the eeprom  section  checksum  using the  contents  of the
 * global  structure  *eeprom_cfg_header  and the contents of the eeprom
 * thereafter.
 */

{
	u_int	sum, i;
	u_short	two_bytes;

	for (sum = 0, i = 0; i < eeprom_cfg_header->size; sum += two_bytes, i += 2) {

		/* don't include checksum in checksum calculation */
		if (i == (char*)(&eeprom_cfg_header->checksum)
						- (char*)eeprom_cfg_header)
			two_bytes = -1;
		else if (i < sizeof(struct eeprom_section_header))
			two_bytes = *(short*)((char*)eeprom_cfg_header + i);
		else
			read_from_eeprom(&two_bytes, eeprom_cfg_base
				- sizeof(struct eeprom_section_header) + i, 2);
	}

	return(~((sum & 0xffff) + ((sum & 0xffff0000)>>16)));
}


#else
/*----------------------------------------------------------------------------*/
u_short eeprom_section_chksum()
/*----------------------------------------------------------------------------*/
/* Calculate  the eeprom  section  checksum  using the  contents  of the
 * global  structure  *eeprom_cfg_header  and the contents of the eeprom
 * thereafter.
 */

{
	u_int	sum, i;
	u_short	two_bytes;
	unsigned int size;

	size = (unsigned int)eeprom_end_offset - (unsigned int)eeprom_cfg_base;
	for (sum = 0, i = 0; i < size; sum += two_bytes, i += 2) {

		/* don't include checksum in checksum calculation */
		if ((unsigned int)eeprom_cfg_base + i == 
			((unsigned int)&(eeprom_geninfo->checksum)-(unsigned int)(eeprom_geninfo)))
			two_bytes = -1;
		else
			read_from_eeprom(&two_bytes, (unsigned int)eeprom_cfg_base + i, 2);
	}

	return(~((sum & 0xffff) + ((sum & 0xffff0000)>>16)));
}
#endif

/*
 * The following routines setup the essential offset used in manipulating eisa configuration
 * data in the eeprom.
 *

	        300/400
+---------------------+  --> eeprom_addr -----------
|                     |     |                     |
|                     |     |                     |
.                     .      \                    |
.                     .       | eisa_section_start \
.                     .      /      (dynamic)       | eeprom_cfg_base
|                     |     |                      / 
|                     |     |                     |
+---------------------+ ------                    |
|   Section Header    |                           |
+- - - - - - - - - - -+ ----------------------------
|     EISA Header     |  |                   |
+- - - - - - - - - - -+  |                   |
|                     |   \                  |
|                     |    | esb_slot_base    \
|      5 slots        |   /                    | eeprom_end_offset
|                     |  |                    /
|                     |  |                   |
+- - - - - - - - - - -+ ---                  |
|    Slot 0 Info      |                      | 
+---------------------+ -----------------------
|                     | 
.                     .
.                     .
.                     .
+---------------------+

	         700
+---------------------+ --> real eeprom address
|                     |
|                     |
.                     .
.                     .
.                     .
|                     |
|                     |
+---------------------+ --> eeprom_addr -- eeprom_cfg_base = 0 
|     EISA Header     | |                   |        |        |
+- - - - - - - - - - -+ |                   |        |        |
|                     | \                   |        |        |
|                     | | eeprom_end_offset |        |        |
|                     | /                   |        |        |
|                     | |                   \        |        |
|                     | |                   | 0x1c00 |        |
+---------------------+ --                  /        \        |
|                     |                     |        | 0x1e00 |
.                     .                     |        /        \
.    Non-EISA Stuff   .                     |        |        | esb_slot_base  
.                     .                     |        |        /    (0x1f62)
|                     |                     |        |        |
+---------------------+ ----------------------       |        |
|      Console        |                              |        |
+- - - - - - - - - - -+                              |        |
|                     |                              |        |
+- - - - - - - - - - -+ -------------------------------       |
|       Boot          |                                       |
+- - - - - - - - - - -+ ---------------------------------------- 
|    Slot 0 Info      |
+---------------------+

 *
 */


#ifdef __hp9000s300
/*----------------------------------------------------------------------------*/
eeprom_locate_cfg()
/*----------------------------------------------------------------------------*/
/* called  once, on first  open, to locate  eeprom  section  header, and
 * initialize eeprom_cfg_header, *eeprom_cfg_header and eeprom_cfg_base
 */

{
	/* offset from eeprom absolute address to eisa section */
	int eisa_section_start; 

	/* allocate memory for section header */
	if ((eeprom_cfg_header = (struct eeprom_section_header *)kmem_alloc
			(sizeof(struct eeprom_section_header))) == NULL) {
		printf("EISA WARNING: unable to allocate eeprom_cfg_header\n");
		return(-1);
	}

	/* find the eisa section */
	if ((eisa_section_start = eeprom_locate_section(CFG_ID, eeprom_cfg_header)) < 0) {
		kmem_free(eeprom_cfg_header, sizeof(eeprom_cfg_header));
		eeprom_cfg_header = NULL;
		return(-1);
	}

	/* 
	 * specific eisa section address calculations 
	 */

	/* offset from eeprom_addr to config header */
	eeprom_cfg_base = eisa_section_start + sizeof(struct eeprom_section_header);

	/* offset from end of cfg header to to end of eisa section */
	eeprom_end_offset = eisa_section_start + eeprom_cfg_header->size - sizeof(eeprom_cfg_base);

	/* pointer to slot 0 area */
	esb_slot_base = eisa_section_start + eeprom_cfg_header->size - sizeof(eeprom_cfg_base);
	esb_slot_base = eeprom_end_offset - MAX_SLOT_ZERO_SIZE;

	/* verify checksum -- clear eeprom if necessary */
	if (eeprom_section_chksum() != eeprom_cfg_header->checksum) {
		printf("Checksum error in EISA section of eeprom:\n");
		printf("\tClearing the EISA section and continuing.\n");

		if (eeprom_init_eisa_section() != 0) {
			printf("\tCouldn't clear eeprom.\n");
			return(-EIO);
		}
	}

	return(0);
}


#else
/*----------------------------------------------------------------------------*/
eeprom_locate_cfg()
/*----------------------------------------------------------------------------*/
{
	eeprom_cfg_base = 0;
	eeprom_end_offset = 3072;
	return(0);
}
#endif /* snakes */


#ifdef __hp9000s300
#define END_OF_SECTIONS	0xff
/*----------------------------------------------------------------------------*/
eeprom_locate_section(id, hdr)
/*----------------------------------------------------------------------------*/
/* return the offset of eeprom section "id" and the contents of the section
 * header in "hdr"
 */

int id;
struct eeprom_section_header *hdr;
{
	int offset;

	if (!eeprom_exists())
		return(-1);

	offset = 0;

	do {
		read_from_eeprom(hdr, offset, sizeof(*hdr));
		offset += hdr->size;
	} while (hdr->section_id != id && hdr->section_id != END_OF_SECTIONS) ;

	if (hdr->section_id == END_OF_SECTIONS)
		return(-1);

	return(offset - hdr->size);
}


/*----------------------------------------------------------------------------*/
eeprom_get_token(token, buf, size)
/*----------------------------------------------------------------------------*/
/* get the data associated with token from the  miscellaneous  constants
 * section of the  eeprom,  copy up to "size"  bytes in "buf" and return
 * the actual number of bytes copied.
 */

int token;
char *buf;
int size;
{
	int offset;
	struct eeprom_section_header hdr;
	u_char token_buf, size_buf;

	if ((offset = eeprom_locate_section(5, &hdr)) == -1)
		return(-2);
	offset += sizeof(hdr);

	do {
		read_from_eeprom(&token_buf, offset, 1);
		if (token_buf == 0) {
			return(-3);
		} else if (token_buf == 0xfe) {
			offset++;
		} else {
			read_from_eeprom(&size_buf, offset + 1, 1);
			if (token_buf == token) {
				read_from_eeprom(buf, offset + 2,
						MIN(size, size_buf - 2));
				break;
			} else {
				offset += size_buf;
			}
		}
	} while (1) ;

	return(MIN(size, size_buf));
}
#endif /* s300 */


#ifdef __hp9000s300
/*----------------------------------------------------------------------------*/
eeprom_write_enable()
/*----------------------------------------------------------------------------*/
{
	*(eeprom_reg_addr + EEPROM_CONTROL_OFFSET) = EEPROM_WRITE_ENABLE_1;
	*(eeprom_reg_addr + EEPROM_SEC_ID_OFFSET) = EEPROM_WRITE_ENABLE_2;

	if (*(eeprom_reg_addr + EEPROM_CONTROL_OFFSET) & EEPROM_ENABLED)
		return(0);
	else
		return(-1);
}

#else /* s700 */
/*----------------------------------------------------------------------------*/
eeprom_write_enable()
/*----------------------------------------------------------------------------*/
{
	char *enable_addr; 

	/* enable eeprom */
	enable_addr  = eeprom_reg_addr;
	*enable_addr = EEPROM_ENABLE;

	return(0);
}
#endif


#ifdef __hp9000s300
/*----------------------------------------------------------------------------*/
eeprom_write_disable()
/*----------------------------------------------------------------------------*/
{
	*(eeprom_reg_addr + EEPROM_ID_OFFSET) = 0;	/* reset eeprom */
	if (*(eeprom_reg_addr + EEPROM_CONTROL_OFFSET) & EEPROM_ENABLED)
		return(-1);
	else
		return(0);
}


#else /* s700 */
/*----------------------------------------------------------------------------*/
eeprom_write_disable()
/*----------------------------------------------------------------------------*/
{
	char *enable_addr; 

	/* disable eeprom */
	enable_addr  = eeprom_reg_addr;
	*enable_addr = EEPROM_DISABLE;

	return(0);
}
#endif
	

/*----------------------------------------------------------------------------*/
eeprom_exists()
/*----------------------------------------------------------------------------*/
/* Maps in data area into kernel VM and sets global eeprom_addr if eeprom 
 * found
 */

{
	static exists = -1;

	if (exists != -1) return(exists);

	exists = 1;

	if ((eeprom_reg_addr = host_map_mem(EEPROM_REG_BASE, NBPG)) == (caddr_t)0) {
		printf("EISA WARNING: unable to map eeprom registers\n");
		return(0);
	}

#ifdef __hp9000s300
	if (!testr(eeprom_reg_addr + EEPROM_ID_OFFSET, 1)
			|| !(*(eeprom_reg_addr + EEPROM_ID_OFFSET) == EEPROM_ID)
			|| !(*(eeprom_reg_addr + EEPROM_SEC_ID_OFFSET)
							== EEPROM_SEC_ID)) {
		printf("EISA WARNING: invalid hardware, eeprom missing\n");
		exists = 0;
	}

#endif

	/* 
	 * map in eeprom data space 
	 */
#ifdef __hp9000s300
	if ((eeprom_addr = host_map_mem(EEPROM_BASE_ADDR, 4*NBPG)) == (caddr_t)0) {
                printf("EISA WARNING: unable to map eeprom\n");
                exists = 0;
        }
#else /* s700 */
	/* 
	 * mapped  in  by  core_map_mem  call  to  map_io_space.
	 * map_io_space  calls are  guaranteed  to be  logical =
	 * physical, so use physical here to set up pointer 
	 */

	eeprom_addr = EEPROM_BASE_ADDR;
#endif

	/* cleanup */
	if (exists == 0) {
	 	unmap_host_mem(eeprom_reg_addr, 1);
#ifdef __hp9000s300
	 	if (eeprom_addr) unmap_host_mem(eeprom_addr, 4);
#endif
	}

	return(exists);
}


#ifdef __hp9000s300
/*----------------------------------------------------------------------------*/
eeprom_init_eisa_section()
/*----------------------------------------------------------------------------*/
/* This routine returns the eisa section to its initial (all ones) state. 
 */

{
	int	i,j;
	int	offset;
	int	size;

	if (eeprom_write_enable() != 0) 
		return(-1);	

	offset = eeprom_cfg_base;
	size = eeprom_cfg_header->size - sizeof(struct eeprom_section_header);

	for (snooze(500); size--; offset++, snooze(500)) {
		*(eeprom_addr + offset*2 + 1) = -1;

		for (i = 0; i < 20; i++) { /* wait up to 10ms */
			snooze(500);
			if (*(eeprom_addr + offset*2 + 1) == -1) 
				break;
		}

		if (*(eeprom_addr + offset*2 + 1) != -1) {
			eeprom_write_disable();
			return(-2);
		}
	}

	if (eeprom_write_disable() < 0)
		return(-3);

	eeprom_cfg_header->write_count = 0;
	eeprom_cfg_header->checksum = eeprom_section_chksum();

	if (write_to_eeprom(eeprom_cfg_header,eeprom_cfg_base-sizeof(struct eeprom_section_header),
			sizeof(struct eeprom_section_header)) < 0)
		return(-4);

	return(0);
}


#else
/*----------------------------------------------------------------------------*/
eeprom_init_eisa_section()
/*----------------------------------------------------------------------------*/
/* This routine returns the eisa section to its initial (all ones) state. 
 */

{
	int	i,j;
	int	offset;
	int	size;

	offset = eeprom_cfg_base;
	size = eeprom_end_offset - eeprom_cfg_base;

	for (snooze(500); size--; offset++, snooze(500)) {
		*(eeprom_addr + offset) = -1;
		for (i = 0; i < 20; i++) { /* wait up to 10ms */
			snooze(500);
			if (*(eeprom_addr + offset) == -1) 
				break;
		}

		if (*(eeprom_addr + offset) != -1) {
			return(-2);
		}
	}

	eeprom_geninfo->write_count = 0;
	eeprom_geninfo->checksum = eeprom_section_chksum();

	if (write_to_eeprom(eeprom_geninfo, eeprom_cfg_base,
			sizeof(struct eeprom_gen_info)) < 0)
		return(-4);

	return(0);
}
#endif


#if defined (__hp9000s700) && defined (_WSIO)
/*----------------------------------------------------------------------------*/
valid_eeprom_boot_area()
/*----------------------------------------------------------------------------*/
{
	int slot;
	int empty_slot = SLOT_IS_EMPTY;
	struct eeprom_per_slot_info *slot_info;
	
	/* Do the actual check of the flag that PDC left */
	read_from_eeprom(&slot, EEPROM_BOOT_OFFSET, sizeof(int));
	if (slot == SLOT_IS_EMPTY) 
		return(slot);

	/* Disable use of boot area to ensure normal operation next time */	
	write_to_eeprom(&empty_slot, EEPROM_BOOT_OFFSET, sizeof(int));

	/* Load the slot info field with the eeprom data */
	slot_info = eeprom_slot_info[slot - 1];  
	read_from_eeprom(slot_info, EEPROM_BOOT_OFFSET+sizeof(int),
		sizeof(struct eeprom_per_slot_info));

	return(slot);
	
}


/*----------------------------------------------------------------------------*/
eisa_cards_present()
/*----------------------------------------------------------------------------*/
/* This routine is called at 1st level config time if Mongoose is found.
 * It makes an attempt  to  determine  whether  any EISA cards  might be
 * found and  configured by second level config.  This is done so the VM
 * system  can make a guess as to how much  space will be  requested  on
 * behalf  of EISA  cards.  Also  note that at 1st  level  config we are
 * still in real  mode, so we are using  physical  address to get to the
 * eeprom and card ids.
 *
 * It's main purpose in life is to set the value of the global  variable
 * eisa_cards_present.  We assume that if we find  EEPROM  data, then we
 * could have an ISA card, otherwise we  specifically  look at each slot
 * for an ID since eisa_config automatic mode may find a card and reinit
 * it.
 */

{
	struct eeprom_gen_info geninfo;
	struct eeprom_per_slot_info slot_info;
	int i;
	caddr_t card_addr;

	int eeprom_cfg_base = 0;

	/* eeprom_addr is used by read_from_eeprom */
	eeprom_addr = EEPROM_BASE_ADDR;

	/* get the general info */
	read_from_eeprom(&geninfo, eeprom_cfg_base, sizeof(struct eeprom_gen_info));

	/* 
	 * See if this section has *NEVER* been initialized -- new box 
	 * These values in eeprom are set up by manufacturing          
	 */
	if (! ((geninfo.flags & SECTION_NOT_INITIALIZED) || (geninfo.number_of_slots == 0))) {
		/* 
		 * read in per_slot_info structures looking for any 
		 * non-empty slot_id, it could be an ISA card meaning
		 * we won't catch it below.
		 */
		for (i = 0; i < geninfo.number_of_slots; i++) {
			read_from_eeprom(&slot_info, 
				eeprom_cfg_base + sizeof(struct eeprom_gen_info)
				+ sizeof(struct eeprom_per_slot_info) * i, 
				sizeof(struct eeprom_per_slot_info));

			if (slot_info.slot_id != SLOT_IS_EMPTY) {
				return(1);
			}
				
		}
	} else
		return(0);


	/*
	 * lets see if there are any real cards out there. Doing an
	 * EEPROM check is not enough since eisa_config automatic mode
	 * can add cards and then run their attach routines.
	 */

	for (i = 0; i < geninfo.number_of_slots; i++) {
		/*
		 * physical equals virtual, and we are running from
		 * realmain() at this point meaning virtual translations 
		 * are not yet on.
		 */
		if (read_eisa_cardid(EISA1_BASE_ADDR + 0x1000*(i+1)) != 0) {
			return(1);
		}
	}
	return(0);
}
#endif
