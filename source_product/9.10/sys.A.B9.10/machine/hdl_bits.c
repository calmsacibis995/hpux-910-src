/* $Source: /source/hpux_source/kernel/sys.SWT68K_300/machine/RCS/hdl_bits.c,v $
 * $Revision: 1.6.84.4 $	$Author: dkm $
 * $State: Exp $   	$Locker:  $
 * $Date: 94/05/05 15:28:55 $
 */

/*
 * Report & set ref bits of PTEs.  Correctly reports ref bit for shared
 *  regions with multiple virtual aliases.
 */
#include "../h/debug.h"
#include "../h/types.h"
#include "../h/pregion.h"
#include "../h/pfdat.h"
#include "../machine/atl.h"
#include "../h/vfd.h"
#include "../machine/pte.h"
#include "../h/vas.h"
#include "../h/vdma.h"


#ifndef PFDAT32
#define PFHDL_UAREA  0x00000040
#endif


/*
 * Return the region pfdat bits from the hardware PTE bits
 */
hdl_getbits(pfn)
	int pfn;
{
	register pfd_t *pfd = pfdat+pfn;
#ifdef PFDAT32
	register short *pf_bits = (short *)&pfd->pf_hdl.pf_bits;
	register unsigned int ptebits;
	struct pte *pp;

        VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
	register int *pf_bits = (int *)&pfd->pf_hdl.pf_bits;

        VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif


	if ((*pf_bits & (VPG_REF | VPG_MOD)) == (VPG_REF | VPG_MOD))
	    return(VPG_REF | VPG_MOD);

#ifdef PFDAT32
	if ((pfd->pf_hdl.pf_bits & (PFHDL_IPTES|PFHDL_DPTES)) == 0)	
#else
	if ((pfd->pf_hdl.pf_bits & PFHDL_IPTES) == 0)
#endif	
		atl_getbits(pfn);
	else {
#ifdef PFDAT32
		if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
#else
		register unsigned int ptebits;
#endif			
			VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
			VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);
			/*
			 * Propagate mod and ref bits.  Note that the read only
			 * pte cannot be modified.
			 */
			/* read the ptes in the pfdat entry */
			ptebits = (*(int *)&pfd->pf_hdl.pf_ro_pte |
						*(int *)&pfd->pf_hdl.pf_rw_pte);
#ifdef PFDAT32
		} else if (pfd->pf_hdl.pf_bits & PFHDL_DPTES)
			ptebits = *(int *) pfd->pf_hdl.pf_kpp;
		else {
			printf("getbits called with no translation, pfd=%x\n", pfd);
			panic("getbits");
		}
#endif		

		/*
		 * update the bits in the pfdat entry
		 */
		if ((ptebits & PG_M) || (*pf_bits & PFHDL_UAREA)) {
			if (ptebits & PG_REF)
				*pf_bits |= VPG_MOD | VPG_REF;
			else
				*pf_bits |= VPG_MOD;
		} else {
			if (ptebits & PG_REF)
				*pf_bits |= VPG_REF;
		}
	} 

	return(*pf_bits & (VPG_REF | VPG_MOD));
}

/* Set the hardware PTE bits from the region pfdat bits */
hdl_setbits(pfn, bits)
	int pfn;
	int bits;
{
	register pfd_t *pfd = pfdat+pfn;
#ifdef PFDAT32
        VASSERT(pfd_is_locked(&pfdat[pfn]));
#else
        VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif        

	/* Flag in pfdat entry; this will suffice for all translations */
	pfd->pf_hdl.pf_bits |= bits;
}

/* Just like hdl_setbits, but clear them instead */
hdl_unsetbits(pfn, bits)
	int pfn;
	register int bits;
{
	register struct atl *a;
	register struct pte *pt;
	register pfd_t *pfd = pfdat+pfn;
#ifdef PFDAT32
	struct pte *pp;

        VASSERT(pfd_is_locked(pfd));
#else
        VASSERT(vm_valusema(&pfdat[pfn].pf_lock) <= 0);
#endif
	/* Clear in master bit register */
	pfd->pf_hdl.pf_bits &= ~bits;

#ifdef PFDAT32
	if ((pfd->pf_hdl.pf_bits & (PFHDL_IPTES|PFHDL_DPTES)) == 0)	
#else
	if ((pfd->pf_hdl.pf_bits & PFHDL_IPTES) == 0)	
#endif	
		atl_unsetbits(pfn, bits);
	else {
#ifdef PFDAT32		
		if (pfd->pf_hdl.pf_bits & PFHDL_IPTES) {
#endif		
			VASSERT(pfd->pf_hdl.pf_ro_pte.pg_m == 0);
			VASSERT(pfd->pf_hdl.pf_ro_pte.pg_prot);

			if (bits & VPG_MOD)
				pfd->pf_hdl.pf_rw_pte.pg_m = 0;

			if (bits & VPG_REF) {
				pfd->pf_hdl.pf_rw_pte.pg_ref = 0;
				pfd->pf_hdl.pf_ro_pte.pg_ref = 0;
			}
#ifdef PFDAT32			
		} else {
			pp = pfd->pf_hdl.pf_kpp;
			if (pp == NULL) {
				printf("pfd is %x, bits are %x\n", pfd, bits);
				panic("unsetbits");
			}
			if (bits & VPG_MOD)
				pp->pg_m = 0;
			if (bits & VPG_REF) 
				pp->pg_ref = 0;
		}
#endif		
	}
}
