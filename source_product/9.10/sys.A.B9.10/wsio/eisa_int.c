/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/wsio/RCS/eisa_int.c,v $
 * $Revision: 1.3.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:28:33 $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../wsio/eisa.h"
#include "../h/io.h"

#ifdef __hp9000s300
#define M68K_EISA_IRQ0_VECTOR 128
#endif

extern int (*eisa_eoi_routine)();
extern struct bus_info_type *eisa[];
extern int eisa_exists;
int eisa_IRQ();

#if defined (__hp9000s800) && defined (_WSIO)
/*---------------------------------------------------------------------------*/
eisa_int()
/*---------------------------------------------------------------------------*/
{
	int this_irq_level;
	struct sw_intloc *intloc;

	this_irq_level = *(((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->iack_reg);
#ifdef EISA_TESTS
	ch_stat(0x30 | this_irq_level);
#endif

	if ((0 <= this_irq_level) && (this_irq_level < 16)) {
		sw_trigger(&(((struct eisa_bus_info *)
			(eisa[EISA1]->spec_bus_info))->eisa_intloc[this_irq_level]), 
			eisa_IRQ, this_irq_level, 5, 0);
	} else {
		printf("EISA: IRQ Level: 0x%02x\n", this_irq_level);
		panic("EISA: Invalid IRQ level from EISA interrupt"); 
	} 
}
#endif /* snakes */


/*---------------------------------------------------------------------------*/
eisa_IRQ(vector)
/*---------------------------------------------------------------------------*/

int vector;
{
	register struct irq_isr *interrupt_list;
	register struct irq_isr *this_guy;
	register int serviced;
	register int irq;

#ifdef __hp9000s300
	irq = vector - M68K_EISA_IRQ0_VECTOR;
#else
	irq = vector; 
#endif
	/* check for spurious interrupt on IRQs 7 and 15 */
	if (eisa_exists) {
		if (irq == 7) {
			((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->esb->int1_ocw3 = OCW3 | READ_ISR;  /* OCW 3 */

			if (!(((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->esb->int1_isr & 0x80))
				return(1);        
		}

		if (irq == 15) {
			((struct eisa_bus_info *)
			(eisa[EISA1]->spec_bus_info))->esb->int2_ocw3 = OCW3 | READ_ISR;  /* OCW 3 */

			if (!(((struct eisa_bus_info *)eisa[EISA1]->spec_bus_info)->esb->int2_isr & 0x80))
				return(1);        
		}
	}

	interrupt_list = eisa_rupttable[irq];
	serviced = 0;
	
	if (interrupt_list != NULL) {
		this_guy = interrupt_list;
		while (!serviced) {
			serviced = (*this_guy->isr)(this_guy->isc, this_guy->arg);
			if (this_guy->next == NULL)
				break;
			else this_guy = this_guy->next;
		}
	}

	(*eisa_eoi_routine)(irq);
	return;
}
