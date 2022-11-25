/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/s200io/RCS/kernel.c,v $
 * @(#)$Revision: 1.10.84.6 $	$Author: rpc $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/10/11 08:02:22 $
 */

#include "../h/param.h"
#include "../h/tty.h"		/* tty structure */
#include "../wsio/hpibio.h"
#include "../h/utsname.h"
#include "../s200io/bootrom.h"
#include "../s200io/dragon.h"
#include "../machine/cpu.h"	/* HAS_DIO_II define */
#include "../graf.300/graphics.h"
#include "../h/debug.h"
#include "../h/pregion.h"
#include "../h/vas.h"
#include "../h/mman.h"

extern caddr_t dio2_map_mem();
extern caddr_t dio2_map_page();
extern int (*vme_init_routine)();
extern int (*eisa_init_routine)();

struct isc_table_type *isc_table[ISC_TABLE_SIZE];
struct isc_table_type *cons_isc;
int cons_dev;

struct isc_table_type *calloc();
	/* calloc is really typeless, but this sure makes life easy */

#define ISCSIZE sizeof(struct isc_table_type)

extern struct tty *cons_tty;
extern short npci;

#ifdef DDB
extern int ddb_boot;
#endif DDB

/* Initial presence of drivers is unknown at this point. */
char ite_present = FALSE;
char windex_present = FALSE;
char kbd_present = FALSE;
char cons_up = FALSE;	/* Console is ready */
char remote_console = FALSE;	/* A remote console has not been found yet */

/* we need to know if the dragon hardware is present */
int dragon_hw_present = 0;

struct tty_driver *tty_list;

extern int tty_array_size;

/* Find all ttys including the console.
** 	Someday:  This routine should be integrated with kernel_initialize()>
*/
tty_init()
{
	int pci_total = 0, total_megs;
	register char *addr, *laddr;
	register sc;
	register struct isc_table_type *isc;
	int cs = 0;
	register il;
	int cons_sc = -1;
	register i,j, offset;
	register struct tty_driver *td;
	char id;
	struct tty_driver *types[TTYDRIVS];
	extern struct tty_driver sionull;

/*
 * 'npci' is the maximum number of users on this system, and is enforced by
 * user_count() when ttopen() is called.  Since this causes problems with
 * uucp and single-user systems, the user-count test in the kernel is bogus.
 * For single-user systems, the user limit is enforced by login(1).  Verify
 * that login.c still has this test in it.  It was there on 12/05/85.
 */
 	/* How many pci's do we allow them? */
 	/* If single-user, allow 1 */
 	/* If multi-user, allow 16 */
/* 	if (utsname.version[0]=='A')		/* single user */
/* 		npci = 1;
 	/* otherwise, leave it at what it was (we hope 16!) */

	/* Find out what software drivers are present */

	for (i=0; i<TTYDRIVS; i++) {
		/* Null device for now */
		types[i] = (struct tty_driver *)&sionull;
		td = tty_list;
		while (td) {
			if (td->type == i) {
				types[i] = td;
				break;
			}
			td = td->next;
		}
	}

	/* Set-up Null Console in case nothing shows up */
/*
	tp->t_drvtype = types[SIONULL];
*/
	/* clear out the isc_table array */
	for (sc=0; sc<ISC_TABLE_SIZE ; sc++)
		isc_table[sc] = NULL;

	cs = 0;	/* see if any one takes the console */

	/* Check for ITE next */
#define ITE_PWR (*types[ITE200]->pwr_init)

	/* Internal hi-res bit-mapped display */
	ITE_PWR(EXTERNALISC+22, HIGH_TYPE, (char *)DISPLAY_HI_RES,
		0, 0, &cs, &pci_total);

	/* Internal low-res bit-mapped display */
	ITE_PWR(EXTERNALISC+19, LOW_TYPE, (char *)DISPLAY_LO_RES,
		0, 0, &cs, &pci_total);

	/* Internal alpha display */
	ITE_PWR(EXTERNALISC+19, LOW_TYPE, NULL, 0, 0, &cs, &pci_total);


        /*
	 * Lets make sure that the fields my_isc are initialized
	 * since we cannot always depend on the pwr init routines
	 * of the driver to do this.
	 */
	if(isc_table[EXTERNALISC+22] != NULL)
	     isc_table[EXTERNALISC+22]->my_isc = EXTERNALISC+22;

	if(isc_table[EXTERNALISC+19] != NULL)
	     isc_table[EXTERNALISC+19]->my_isc = EXTERNALISC+19;

	/* Check for a graphics card in DIO II space */
	if (HAS_DIO_II) {
		for (sc=MIN_DIOII_ISC; !ite_present && sc<=MAXISC; sc++) {
			/*
			** get the starting address for this select code
			*/
			addr = (caddr_t)(DIOII_START_ADDR + 
				     (sc - MIN_DIOII_ISC) * DIOII_SC_SIZE);

			/*
			** map in the page containing addr
			*/
			laddr = dio2_map_page(addr);

			/*
			** see if there is a card out there
			*/
			if (testr((caddr_t)laddr + 1, 1) == FALSE)
				/* not there so try next isc_num */
				continue;
			/*
			** get the id of the card and the 
			** interrupt level of the card
			*/
			id = *((caddr_t)laddr+1) & 0x7F;/* mask off remote id */
			if (id != 0x39) continue;	/* Not a crt */

			/*
			** determine the number of pages to map in
			*/
			total_megs = *((caddr_t)laddr+0x101) + 1;

			/*
			** map in the pages for this select code
			*/
			addr = dio2_map_mem(addr, total_megs * 0x100000);

			/* Is there a piggybacked STI device here? */
			offset = (((addr[0x61] & 0xff)<<24)
				+ ((addr[0x63] & 0xff)<<16)
				+ ((addr[0x65] & 0xff)<<8)
				+ (addr[0x67] & 0xff));

			if (offset!=0) {
			    extern struct graphics_descriptor *graphics_ite_dev;
			    struct graphics_descriptor *add_sti_device(), *p;
			    p = add_sti_device(addr+offset, sc);
			    if (p!=NULL) {
				graphics_ite_dev = p;
				ITE_PWR(sc,SGC_TYPE,NULL,0,0,&cs,&pci_total);
			    }
			}

			if (!ite_present) {
				/* Look for an ITE here.  */
				ITE_PWR(sc,DIO_TYPE,addr,0,0,&cs,&pci_total);
			}

			/* Release the pages if we didn't like it. */
			if (!ite_present)
				dio2_unmap_mem(addr, total_megs * 0x100000);

        		/*
	 		 * Lets make sure that the fields my_isc are
	 		 * initialized since we cannot always depend on
	 		 * the pwr_init routines of the driver to do this.
			 */
		        if(isc_table[sc] != NULL)
	     	        	isc_table[sc]->my_isc = sc;
		}
	}

	if (!ite_present) {
		i = sgc_init(1);	/* scan sgc cards for a console */
		if (i != -1)		/* If we found one... */
			ITE_PWR(i,SGC_TYPE,NULL,0,0,&cs,&pci_total);
	}
		

	/* Find all the rest of the tty's */
	for (sc=0; sc<32; sc++) {
#ifdef DDB
	if ((ddb_boot) && (sc == 9))
		continue;
#endif DDB
		/* select codes 5 & 6 are the Apollo builtin RS-232 ports */
		if ((sc == 6) || (sc == 5)) {
apollo_stuff:		addr = (char *)(0x41C000 + LOG_IO_OFFSET);
			if (testr(addr, 1) == FALSE)
				continue;
			id = APOLLOPCI;
			il = 5; /* XXX */
		} else {
			addr = (char *)(0x600000 + sc*65536 + LOG_IO_OFFSET);
			if (testr(addr + 1, 1) == FALSE) {
				if (sc==9)
					goto apollo_stuff;	/* try this */
				continue;
			}
			id = *(addr+1) & 0x7F;
			il = ((*(addr+3)>>4)&0x3)+3;
		}

		/* See if anyone claims this card */
		if (isc_table[sc] == NULL) {
			for (td = tty_list; td; td=td->next) {
				if (td->type == ITE200)
					continue;
				(*td->pwr_init)(sc,addr,id,il,&cs,&pci_total);
				if (cons_sc == -1 && remote_console) {
					cons_sc = sc;
					break;
				}
			}
		}


        	/*
	 	 * Lets make sure that the fields my_isc are
	 	 * initialized since we cannot always depend on
	 	 * the pwr_init routines of the driver to do this.
		 */
		if(isc_table[sc] != NULL)
	     		isc_table[sc]->my_isc = sc;

		switch (id) {	/* Check for wider cards */
		case 28:
		case 29:
		case 29+32:
		case 29+64:
		case 29+96:
		case 57:	/* Renaissance or da Vinci graphics */
			sc++;	/* Card is two slots big */
			continue;
		case 26: 
		case 26+32:
		case 26+64:
		case 26+96:
			sc += 3;	/* Card is quad wide */
		default:
			continue;
		}
	}

	/* Initialize Console */
	cons_up = TRUE;
	if ((cs == 0) && (cons_sc == -1)) {
		/* find the first valid selcode and use it */
		for (sc = 0; sc < 32 ; sc++)
			if ((isc = isc_table[sc]) != NULL) {
				cons_isc = isc;
				cons_tty = NULL; /* so driver will set it */
				cons_sc = sc;
				break;
			}
		if (cons_sc == -1) /* can't do anything */
			panic("Help, no console");
		(*cons_isc->tty_routine->who_init)(cons_tty, -1, cons_sc);
	}
	else
		/* internal console */
		(*cons_tty->t_drvtype->who_init)(cons_tty, -1, cons_sc);


	if (ite_present || windex_present) {
		msg_printf("    ITE + %d port(s)\n", pci_total-1);
	} else
		msg_printf("    %d port(s)\n", pci_total);
}


int
data_com_type(isc)
struct isc_table_type *isc;
{
	register int i,j;
	char name[10];

	register caddr_t addr = (caddr_t)isc->card_ptr;
	register dc_dummy;
	if (!testr(addr+0x400D,1))
		return(-1);
	if (*(addr+0x400D))
		return(-1);

	/* add check for rje card */

	for(j=0x400F; j<=0x4019; j+=2)
	   if(!testr(addr+j,1))
		goto try_srm;
	i=0;		      /*Get ASCII name of card*/
	for(j=0x400F; j<=0x4019; j+=2)
	   name[i++]= *(addr+j);

	name[i]='\0';
	if( (name[0] == '9') && (name[1] == '8') &&
	    (name[2] == '6') && (name[3] == '4') &&
	    (name[4] == '1') )
		return 99;

	if( (name[0] == '9') && (name[1] == '8') &&
	    (name[2] == '6') && (name[3] == '4') &&
	    (name[4] == '9') )
		return 49;

	if( (name[0] == '3') && (name[1] == '6') &&
	    (name[2] == '9') && (name[3] == '4') &&
	    (name[4] == '1') )
		return 54;

try_srm:
	if (!testr(addr+16395,1) || !testr(addr+16393,1))
		return(-1);
	dc_dummy = (((int)*(unsigned char *)(addr+16395))<<8)
			+(int)*(unsigned char *)(addr+16393);
	if (dc_dummy > 32767)
		return(-1);
	if (!testr(addr+dc_dummy*2+1,1))
		return(-1);
	if (((*(addr+dc_dummy*2+1)) & 127) != 1)
		return(-1);
	if (!testr(addr+16431,1))
		return(-1);
	return((int)*(unsigned char *)(addr+16431));
}


/*
** This routine recognizes all supported select-code oriented interfaces.
** Optional drivers, if present in the system, will intercept control
** before this routine, and request that an isc_table entry be made if
** applicable.  Otherwise, reaching this routine usually means that
** some interface is present, but that its interface driver is missing, 
** so an isc_table entry should not be made.
*/
int last_make_entry(id, isc)
int id;
struct isc_table_type *isc;
{
	register char *card_name;

	switch(id) {

	case -1:
		card_name = "Internal HP-IB Interface";
		break;
	case 1:
		card_name = "HP98624 HP-IB Interface";
		break;
	case 2:
		card_name = "HP98626 RS-232C Serial Interface";
		break;
	case 2+64:
		card_name = "HP98644 RS-232C Interface";
		break;
	case 3:
		card_name = "HP98622 GPIO Interface";
		break;
	case 5: 
		card_name = "HP98642 RS-232C Multiplexer";
		break;
	case 0x07:	/* single ended SCSI with 32 bit DMA */
	case 0x27:	/* single ended SCSI with 16 bit DMA */
	/* case 0x47:	/* differential SCSI with 32 bit DMA (non-existent) */
	/* case 0x67:	/* differential SCSI with 16 bit DMA (non-existent) */
		card_name = "HP98265 SCSI Interface";
		break;
	case 8:
		card_name = "HP98625 High-Speed HP-IB Interface";
		break;
	case 17:
		card_name = "HP98646 VMEbus Interface";
		break;
	case 20+32:
		switch (data_com_type(isc)) {
		case 1:	/* Async 628 */
			card_name = "HP98628 RS-232C Datacomm Interface";
			break;
		case 3: /* Ganglia 629 */
			card_name = "HP98629 SRM/MUX Interface";
			break;
		case 49: /* SNA card -- unsup. by HP-UX, identify for RMB-UX */
			isc->card_type = HP98649;
			card_name = "";
#ifndef QUIETBOOT
			printf("    Card ID %d", id);
#endif /* ! QUIETBOOT */
			io_inform(card_name, isc, -1);
			return 1;
                case 54: /* X.25 interface */
                        card_name = "HP36941 X.25 Interface";
                        break;
		case 99: /* RJE card */
			card_name = "HP98641 RJE";
			break;
		case 2: /* FDL 628 */
		default:
			isc->card_type = HP98691;
			io_inform("HP98691", isc, -2, " ignored; unrecognized card option");
			return 1;
		}
		break;
	case 21:
		card_name = "HP98643 LAN/300 Link";
		break;

		/*
		isc->card_type = HP98643;
		return io_inform("HP98643", isc, 5);
		*/

	case 25:
		card_name = "High Resolution Display";
		break;
	case 26: 
	case 26+32:
	case 26+64:
	case 26+96:
		isc->card_type = HP98630Q;
		card_name = "Quad Wide card";
		io_inform(card_name, isc, -1);
		return 1;
	case 28:
		card_name = "HP98627 Color Graphics Interface";
		isc->card_type = HP98627A;
		break;

		/*
		isc->card_type = HP98627A;
		return io_inform("HP98627", isc->my_isc, 0);
		*/

	case 29: 
		card_name = "Double Wide card";
		isc->card_type = HP98633;
		io_inform(card_name, isc, -1);
		return 1;
	case 29+32:
	case 29+64:
	case 29+96:
		card_name = "Double Wide card";
		isc->card_type = HP98630D;
		io_inform(card_name, isc, -1);
		return 1;
	case 49:
		card_name = "HP98577 VMEbus Expander";
		break;
	default:	/* cards not supported by HP-UX */
		switch(id) {
			/* reserve isc for these so RMB-UX can find them */
			case 4:
				isc->card_type = HP98623;
				break;
			case 12:
				isc->card_type = HP98647;
				break;
			case 18:
				isc->card_type = HP98640;
				break;
			case 22:
				isc->card_type = HP98695;
				break;
			case 27:
				isc->card_type = HP98253;
				break;
			case 30:
				isc->card_type = HP98259;
				break;
			default: /* unsupported by HP-UX and not recognizing for RMB-UX */
#ifndef QUIETBOOT
				printf("    Card ID %d", id);
#endif /* ! QUIETBOOT */
				card_name = "";
				return io_inform(card_name, isc, -1);
		}

		/* RMB-UX cards -- print out as if unrecognized but save an isc
		   for these cards so RMB-UX can find them later
		*/
#ifndef QUIETBOOT
		printf("    Card ID %d", id);
#endif /* ! QUIETBOOT */
		card_name = "";
		io_inform(card_name, isc, -1);
		return 1;
	}
	return io_inform(card_name, isc, -1);
}


/*
** This procedure variable forms the head of a chain of procedures called
** by kernel_initialize when it's scanning the backplane to make entries
** in the isc_table.  Drivers insert themselves in the chain by saving the
** current procedure address, and putting in their own procedure's address.
** When their procedure is called, it checks to see if it recognizes the card.
**
** If the procedure recognizes the card, it prints out any appropriate
** information, usually via the io_inform routine, and then either returns
** TRUE or FALSE, depending upon whether it wants the isc entry entered in
** the isc_table or not.  Fields within the isc entry may be filled in at
** this time; however, THE ADDRESS OF THIS ISC ENTRY MUST NOT BE SAVED, as
** it only points to a local variable in kernel_initialize, and DOES NOT
** POINT TO PERMANENT STORAGE!!!
**
** If the procedure does not recognize the card, it calls the procedure
** whose address it saved when it linked itself in, and returns the value
** of that called procedure.  An exception is the last_make_entry procedure,
** always at the tail of the chain; it handles the unrecognized card
** and optional driver absent situations.
*/
int (*make_entry)() = last_make_entry;

/*
 * 	From the DIO THIN LAN ERS - 98643a - S. Haddock 85.4.10
 */
struct lan_nyb_word {	/* See par. 5.2.3 */
	unsigned	unused:12;
	unsigned	nyb:4;
} 

#define NLANNYBS 12	/* 12 nybble (6 byte) station address */
#define STATION_ADDRESS_START	0xc008	/* from start of card */
#define SC_STEP 0x10000	/* 64K per card */
#define MAX_SC 31
#define LAN_ID_MASK 0x1f	/* primary only */
#define DIO_LAN_ID 21
#define STD_LAN_SC 21

get_system_id()
{
	int sc, id;

	if (is_lan_sc(STD_LAN_SC, &id))	/* success == nonzero return */
		return;

	sc = 0;
	while (sc <= MAX_SC) {
		if (is_lan_sc(sc, &id))
			return;

		switch (id) {	/* handle bigger cards */
		case 26:
			sc += 4; break;
		case 28:
		case 29:
			sc += 2; break;
		default:
			sc += 1; break;
		}
	}
}

/*
 *	If sc is the select code of a 98643, fill in
 *	utsname.idnumber with the station address and return 1.
 *	Otherwise return 0.
 */
int
is_lan_sc(sc, idp) int sc, *idp;
{
	char *unamep;
	char *cardp;
	struct lan_nyb_word *lnwp;
	int k;

	*idp = 0;
	/*	ID reg is 2nd byte on card */

	cardp = (char *)(START_ADDR + sc * SC_STEP);
	if (testr(cardp+1, 1) == FALSE)
		return 0;

	*idp = (int)((*(cardp+1)) & LAN_ID_MASK);
	if (*idp != DIO_LAN_ID)
		return 0;

	lnwp = (struct lan_nyb_word *)(cardp + STATION_ADDRESS_START);
	if (testr((char *)lnwp, NLANNYBS*sizeof(struct lan_nyb_word))
			== FALSE)
		return 0; 	/* panic? */

	unamep = utsname.idnumber;
	/* *unamep++ = '0'; */
	/* *unamep++ = 'x'; */
	for (k = 0; k < NLANNYBS; k++, lnwp++)
		*unamep++ = "0123456789abcdef"[lnwp->nyb];
	*unamep = '\0';
	return 1;
}

/*
** This routine searches for interfaces, generating an entry in the isc_table
** for each one it finds.  Select code numbers 0-31 are now used for both the
** external IO space interfaces and the internal IO space interfaces.
** NOTE: internal io space is scanned for specific devices only.
*/
kernel_initialize()
{
	register int isc_num, id, int_lvl;
	register struct isc_table_type *isc;
	register caddr_t addr, laddr;
	struct isc_table_type temp_isc_entry;
	register int primary_id;
	extern int dragon_present;
	int total_megs = 0;
	int loopmax;

	dma_initialize();

	/* only scan DIOII space on those spu's that have it */
	if (HAS_DIO_II)
		loopmax = MAXISC;
	else
		loopmax = MAXEXTERNALISC;

	/* 
	** determine what interfaces are present
	*/
	for (isc_num = MINISC; isc_num <= loopmax; isc_num++) {

		/* skip the undefined select codes */
		if (isc_num == (MAXEXTERNALISC + 1))
			isc_num = MIN_DIOII_ISC;


		/*
		** If isc_num >= MIN_DIOII_ISC then the isc_num is an
		** external plug-in interface in DIOII space.
		**
		** If isc_num >= EXTERNALISC  and isc_num < MIN_DIOII_ISC
		** then the isc_num is in internal IO space.  Only check 
		** for specific isc_num's.
		**
		** If isc_num is 7 then decide between internal & external IO
		** space.
		**
		** If isc_num is less than EXTERNALISC then the isc_num is a
		** external plug-in interface.
		*/
		if (isc_num >= MIN_DIOII_ISC) {

			/*
			** get the starting address for this select code
			*/
			addr = (caddr_t)(DIOII_START_ADDR + 
				     (isc_num - MIN_DIOII_ISC) * DIOII_SC_SIZE);

			/*
			** map in the page containing addr
			*/
			laddr = dio2_map_page(addr);

			/*
			** see if there is a card out there
			*/
			if (testr((caddr_t)laddr + 1, 1) == FALSE)
				/* not there so try next isc_num */
				continue;
			/*
			** get the id of the card and the 
			** interrupt level of the card
			*/
			id = *((caddr_t)laddr+1) & 0x7F; /* mask off remote id bit */
			int_lvl = ((*((caddr_t)laddr+3) >> 4) & 0x3) + 3;

			/*
			** read the number of megabytes to map in
			*/
			total_megs = *((caddr_t)laddr+0x101) + 1;

			/*
			** map in the pages for this select code
			*/
			addr = dio2_map_mem(addr, total_megs * 0x100000);
			if ((int)addr == 0) {
				printf("Warning: Card Id %d at select code %d\n   ignored; cannot map card into kernel logical address space\n", id, isc_num);
				continue;
			}


		} else if (isc_num >= EXTERNALISC) {
			switch (isc_num) {
			case EXTERNALISC+19:
			case EXTERNALISC+22:
			/* graphics devices */
				addr = (caddr_t)(IOSTART_ADDR + 
					       (isc_num - EXTERNALISC) * 65536);
				if (testr(addr + 1, 1) == FALSE)
					continue;  /* try next isc_num */
				id = *(addr+1) & 0x7F; /* mask remote id bit */
				break;

			case EXTERNALISC+28:
				/* Dragon card is at 5c0000, old float card
				 * also resides here.
				 */
				addr = (caddr_t)(IOSTART_ADDR + 
					       (isc_num - EXTERNALISC) * 65536);
				if (testr(addr + 1, 1) == FALSE)
					continue;  /* try next isc_num */
				id = *(addr+1) & 0x7F; /* mask remote id bit */
				if (id != DRAGON_ID)
					continue;  /* try next isc_num */
				int_lvl = ((*(addr+3) >> 4) & 0x3) + 3;
				dragon_present = 1;
				dragon_hw_present = 1;
				continue;
			default:
				continue;
			}
		} else if (isc_num == 7 && (sysflags & NOHPIB) == 0) {  
			/* int HPIB */
			addr = (caddr_t)INTERNAL_CARD;
			id = -1;
			int_lvl = 3;
		} else if (isc_num == 5 || isc_num == 6) {  
			/* Apollo Utility chip UARTS */
try_apollo:		addr = (caddr_t)(0x41C000 + LOG_IO_OFFSET);
			if (testr(addr, 1) == FALSE)
				continue;  /* try next isc_num */
			id = APOLLOPCI;
			int_lvl = 5;
		} else {
			/* external plug-in interface */
			addr = (caddr_t)(START_ADDR + isc_num * 65536);
			if (testr(addr + 1, 1) == FALSE) {
				if (isc_num==9)
					goto try_apollo;
				continue;  /* try next isc_num */
			}
			id = *(addr+1) & 0x7F; /* mask remote id bit */
			int_lvl = ((*(addr+3) >> 4) & 0x3) + 3;
		}

		/*
		** temporarily use a local variable until sure of need
		*/
		isc = &temp_isc_entry;

		/*
		** certain isc_table fields are always initialized
		*/
		bzero((caddr_t)isc, ISCSIZE);
		isc->card_ptr = (int *)addr;
		isc->int_lvl = int_lvl;
		isc->my_isc = isc_num;
		isc->bus_type = DIO_BUS;
		isc->ftn_no = -1;
		isc->next_ftn = NULL;

		/*
		** if a driver recognizes the card and wants the entry made,
		** allocate and assign a permanent entry
		*/
		
		if ((*make_entry)(id, isc)) {
			*(isc_table[isc_num] = calloc(ISCSIZE)) = *isc;
		} else {
			/*
			** if dio2 throw away the pages we mapped in for this SC
			*/
			if (isc_num >= MIN_DIOII_ISC)
				dio2_unmap_mem(addr, total_megs * 0x100000);
		}

		/*
		** handle DIOII multi select code  interfaces
		*/
		if ((isc_num >= MIN_DIOII_ISC) && (total_megs > 4))
			isc_num += (total_megs - 1) / 4;


		/*
		** handle DIO quad & double-wide interfaces
		*/
		if ((primary_id = id & 31) == 26)
			isc_num += 3;
		else if (primary_id == 28 || primary_id == 29)
			isc_num++;
		else if (primary_id == 57 &&   /* Renaissance or daVinci card */
			 isc_num>=MINISC && isc_num<EXTERNALISC)
			isc_num++;
  	}

	/*
	** initialize the interfaces and/or interfaces drivers
	*/
    	for (isc_num = MINISC; isc_num <= loopmax; isc_num++) {
		/* skip the undefined select codes */
		if (isc_num == (MAXEXTERNALISC + 1))
			isc_num = MIN_DIOII_ISC;

      		if ((isc = isc_table[isc_num]) != NULL &&
        	    isc->iosw != NULL &&
        	    isc->iosw->iod_init != NULL)
			(*isc->iosw->iod_init)(isc);
	}

	/* call MTV initialization routine, if configured in */
	(*vme_init_routine)();

	/* call EISA initialization routine, if configured in */
	(*eisa_init_routine)();

	if(dragon_present)
		dragon_init();
	led_init();
        sgc_init(0);	/* scan for graphics cards, don't look for a console */
	get_system_id();
}


/* Routine to do io card message printing and/or checking
 *	card_name	pointer to name of card
 *	isc		pointer to select code structure
 *	req_lvl		required level (0 means none required)
 *				       (-1 means interface driver missing)
 *				       (-2 means "p_fmt", with up to 3 parms)
 */
io_inform(card_name, isc, req_lvl, p_fmt, p1, p2, p3)
char *card_name;
register struct isc_table_type *isc;
int req_lvl;
char *p_fmt;
int p1, p2, p3;
{
#ifdef QUIETBOOT
	if (req_lvl < 0 || req_lvl && req_lvl != isc->int_lvl)
		return (0);
	else
		return (1);
#else
	printf("    %s at select code %d", card_name, isc->my_isc);
	if (req_lvl < 0 || req_lvl && req_lvl != isc->int_lvl) {
#ifdef DDB
		if ((req_lvl == -3) && (ddb_boot) && (isc->my_isc == 9))
			printf(" ignored;\n    in use by the ddb kernel debugger");
		else
#endif DDB
		if (req_lvl == -2)
			printf(p_fmt, p1, p2, p3);
		else if (req_lvl == -1)
			printf(" ignored;\n    interface driver not present");
		else
			printf(" ignored;\n    interrupt level at %d instead of %d",
							isc->int_lvl, req_lvl);
		printf("\n");
		return (0);
	}
	printf("\n");
	return (1);
#endif /* QUIETBOOT */
}


/******************** Stubs ************************************/
k_nop()
{
	return(0);
}
struct tty *k_nop2()
{
	return((struct tty *)0);
}

/* Stubs for missing tty drivers */
struct tty_driver sionull = {
SIONULL, k_nop, k_nop, k_nop, k_nop, k_nop, k_nop, k_nop,
	k_nop, k_nop2, k_nop, 0,
};

/* Stubs for missing ite driver */
int (*ite_scrl)() = k_nop;

ite_scroller(a,b,c,d)
{
	(*ite_scrl)(a,b,c,d);
}

/*
********************************************************************************
********************************************************************************
  
	Hidden Mode EEPROM Register Access Routines

********************************************************************************
********************************************************************************
*/

volatile unsigned char	*sc_hidden_reg_map[32];

hidden_mode_initialize()
{
	register unsigned char	*reg, sc;

#ifdef OSDEBUG
	printf("Initializing hidden mode interfaces\n");
#endif /* OSDEBUG */
	for (reg = HIDDEN_REG_BASE; reg <= HIDDEN_REG_EXTENT; reg += 2) {
		if (testr(reg, 1)) {
			sc_hidden_reg_map[sc = *reg & 0x1f] = reg;
#ifdef OSDEBUG
			printf("  adding interface at 0x%x select code = %d\n",reg,sc);
#endif /* OSDEBUG */
		}
	}
#ifdef OSDEBUG
	printf("Hidden mode initialization complete\n");
#endif /* OSDEBUG */

}


/*
** returns a pointer to the select code register for the interface
** at select_code
*/
unsigned char *
hidden_mode_exists(select_code)
register unsigned char select_code;
{
	return(sc_hidden_reg_map[select_code]);
}


enable_hidden_mode(select_code)
register unsigned char select_code;
{
	VASSERT(sc_hidden_reg_map[select_code]);

	*sc_hidden_reg_map[select_code] = *sc_hidden_reg_map[select_code];
}


disable_hidden_mode(select_code)
register unsigned char select_code;
{
	VASSERT(sc_hidden_reg_map[select_code]);

	select_code = *sc_hidden_reg_map[select_code];
}
