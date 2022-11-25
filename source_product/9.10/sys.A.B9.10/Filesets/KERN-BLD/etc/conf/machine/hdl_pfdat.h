/*
 * @(#)hdl_pfdat.h: $Revision: 1.6.84.4 $ $Date: 94/05/05 15:28:57 $
 * $Locker:  $
 */
#ifndef _MACHINE_HDL_PFDAT_INCLUDED
#define _MACHINE_HDL_PFDAT_INCLUDED

/*
 * Hardware-dependent fields for the pfdat data structure
 */
#ifdef _KERNEL_BUILD
#include "../h/vfd.h"
#include "../h/region.h"
#include "../machine/pte.h"
#include "../machine/atl.h"
#else  /* ! _KERNEL_BUILD */
#include <sys/vfd.h>
#include <sys/region.h>
#include <machine/pte.h>
#include <machine/atl.h>
#endif /* _KERNEL_BUILD */



struct hdlpfdat {
	short pf_bits;		/* Master mod/ref bits go here */
	/*
	 * Note: the PTEs must be contiguous, be in order, and be long-aligned
	 */
	union { 
		struct pte pf_ptes[2];	  /* real page table entries */
		struct pfd_atl pf_atl_u;  /* atl for this pfdat */
		struct pte *kvm_pte_ptr[2];  /*  addr of real PTE for KVM  */
	} hdl_pf_u;
};

#define pf_ro_pte hdl_pf_u.pf_ptes[0]
#define pf_rw_pte hdl_pf_u.pf_ptes[1]
#define pf_atl hdl_pf_u.pf_atl_u
#define pf_kpp hdl_pf_u.kvm_pte_ptr[0]

/*
 * Defines for pf_bits.  Note that we also use a couple of things from vfd.h:
 *   VPG_MOD  0x0001
 *   VPG_REF  0x0002
 * The first 3 definitions below are only used with attach lists - they 
 * allow us to properly keep track of whether pages the kernel has just
 * done I/O on are dirty/referenced (the kernel translation for the
 * pages does not have an attach list entry, so the MMU setting of these
 * bits isn't sufficient).
 */
#define PFHDL_PROTECTED 0x10
#define PFHDL_UNSET_REF 0x20
#define PFHDL_UNSET_MOD 0x40
/*
 * If we are using indirect PTEs, we use those bits to record the valid
 * bits of pages that were temporarily mapped into the kernel (and taken
 * away from the user).
 */
#define PFHDL_ROPTE_V   0x10
#define PFHDL_RWPTE_V   0x20

#define PFHDL_UAREA	0x4000
#define PFHDL_COPYFRONT 0x8000

/*
 * Flags for the pf_bits field that indicates type of ptes
 */
#define PFHDL_ATLS	0x80
#define PFHDL_IPTES     0x04
#define PFHDL_DPTES     0x08

#define PFHDL_ZERO_BITS 0x3f00


#ifdef	_KERNEL
/*
 *  When we replace the attach list structure on S300s we can improve the
 *  performance of HDL_GETBITS() by really making it an inline expansion
 *  macro.
 */
#define	HDL_GETBITS(PFN)	hdl_getbits(PFN)
#define	HDL_GETMODBIT(PFN)	(HDL_GETBITS(PFN) & VPG_MOD) 
#define	HDL_GETREFBIT(PFN)	(HDL_GETBITS(PFN) & VPG_REF)
#endif	/* _KERNEL */
#endif /* not _MACHINE_HDL_PFDAT_INCLUDED */
