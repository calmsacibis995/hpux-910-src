/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/eisa_sysbrd.c,v $
 * $Revision: 1.2.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:28:39 $
 */

#include "../h/types.h"
#include "../wsio/eisa.h"

/* ident string for this data structure */
static char *sysbrd_rev = "$Header: eisa_sysbrd.c,v 1.2.83.3 93/09/17 20:28:39 kcs Exp $ - UNMODIFIED";

static struct eisa_sysbrd_parms sysbrd_table[] = 
/* Sys Brd ID,   Default # Slot,  EEPROM Slot #,     Iomap Size */
{{0x22f0c020,		4,		8,		1024},	/* Trailways KLUDGE - do not move */
 {0x22f0c000,		1,		8,		1024},  /* Mongoose */
 {0x22f0c010,		4,		8,		1024},	/* Coral */
 {0x22f0c031,		8,		8,		1024},  /* MACE - Bludgeon */
 {0x22f0c051,		8,		8,		1024},  /* PACE */
/* insert new systems boards here */

 {         0,		1,		8,		1024}}; /* Hrdwr Test Default and NULL */


/*----------------------------------------------------------------------------*/
struct eisa_sysbrd_parms *get_eisa_sysbrd_parms(id)
/*----------------------------------------------------------------------------*/
/* This routine  attempts to look up the eisa system board id and return
 * a  pointer  to a  structure  which  contains  relevent  system  board
 * information.  The search will  distinguish  between board  revisions,
 * since it is probable that enhancements affecting the stored paramters
 * will be indicated in this fashion.
 */

int id;
{
	int i;

	/* if table grows, we may want to change the search */
	for (i=0; (sysbrd_table[i].id != id)&&(sysbrd_table[i].id != 0); i++); 

	if (sysbrd_table[i].id == 0) {
		/* if we did not find any */
#ifdef __hp9000s300
		return (&sysbrd_table[0]);  /* Trailways Kludge */
#else
		printf("WARNING: CANNOT IDENTIFY EISA SYSTEM BOARD\n");
		printf("EISA SYSTEM BOARD ID: 0x%x", id);
#endif
	}

	return (&sysbrd_table[i]);
}
