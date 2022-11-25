/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/vm_textcache.c,v $
 * $Revision: 1.4.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:19:19 $
 */

#include "../h/debug.h"
#include "../h/types.h"
#include "../h/param.h"
#include "../h/spinlock.h"
#include "../h/region.h"

#include "../h/proc.h"		/* reglock() uses p_reglocks */
#include "../h/user.h"		/* reglock() uses u_procp */


/*
 * Hash algorithm for text hash.
 * For now just grab a few bits out of the VP pointer.
 */
#define TEXTHSHSZ 32
#define TEXTHASH(VP, DATA) (((u_int)(VP)>>7) & (TEXTHSHSZ-1))

/*
 * The actual hash entries and spinlock declaration.
 */
vm_lock_t text_hash;
reg_t *texts[TEXTHSHSZ];

/*
 * Spinlock to govern hash list.
 */
#define textlstlock() vm_spinlock(text_hash)
#define textlstunlock() vm_spinunlock(text_hash)

/*
 * Initialize text hash list.  This list is used to hold 
 * unaligned 0410 executable text regions.
 */
textinit()
{
	static textinitted = 0;
	if (textinitted)
		return;
	else {
		int x;

		vm_initlock(text_hash, TEXT_HASH_LOCK_ORDER, "text lock");
		/*
		 * Initialize the text hash stuff.
		 */
		for (x = 0; x < TEXTHSHSZ; x++)
			texts[x] = (reg_t *)NULL;
		textinitted = 1;
	}
}

/*
 * Peform an insert of the region into the 
 * text hash list.
 */
reginsert(rp, vp, data)
	register reg_t *rp;
	register struct vnode *vp;
	u_int data;
{

	VASSERT(vm_valulock(text_hash) <= 0);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);

	/*
	 * insert newcomers at head of bucket
	 */
	rp->r_hchain = texts[TEXTHASH(vp, data)];
	texts[TEXTHASH(vp, data)] = rp;

	/*
	 * Mark it as hashed.
	 */
	VASSERT(rp->r_fstore == vp);
	VASSERT(rp->r_byte == data);
	rp->r_flags |= RF_HASHED;
}

/*
 * Remove a region from the text hash list.
 */
text_unhash(rp)
	register reg_t *rp;
{
	register reg_t **rp2;

	VASSERT(rp);
	VASSERT(vm_valusema(&rp->r_lock) <= 0);
	VASSERT(rp->r_flags&RF_HASHED);
	
	textlstlock();

	/*
	 * See if it is on the list.
	 */
	rp2 = &texts[TEXTHASH(rp->r_fstore, rp->r_byte)];
	for (; *rp2 != NULL; rp2 = &((*rp2)->r_hchain)) {
		if (*rp2 == rp)
			break;
	}

	/*
	 * Disassociate region from hash
	 */
	if (*rp2 != rp)
		panic("text_unhash: hashed region not found");
	else
		*rp2 = rp->r_hchain;
	textlstunlock();
	rp->r_flags &= ~RF_HASHED;
	return;
}

/*
 * Perform an atomic test and set of the text hash
 * for a given region.  This routine has the following
 * exit conditions:
 *
 *	return	newrp		meaning
 *	-1	hashed rp	Found but some other process holds the lock
 *	 0	hashed rp	Found and caller now holds the lock
 *	 1	NULL		No region found
 */
int
text_insert(vp, data, rp, newrp)
	struct vnode *vp;
	u_int data;
	reg_t *rp;
	reg_t **newrp;
{
	register reg_t *rp2;
	
	/*
	 * Place to do this for now.
	 */
	textinit();

	/*
	 * Inserting a region in the hash requires first 
	 * holding the vnode.
	 */
	textlstlock();
	rp2 = texts[TEXTHASH(vp, data)];
	for( ; rp2 != NULL ; rp2 = rp2->r_hchain) {
		if ((rp2->r_byte == data) && (rp2->r_fstore == vp)) {
			if (!creglock(rp2)) { 
				/*
				 * Could not grab lock but region
				 * hashed.
				 */
				textlstunlock();
				*newrp = rp2;
				return(-1);
			}
			VASSERT(rp2->r_flags&RF_HASHED);
			/*
			 * Found it and returning it locked.
			 */
			textlstunlock();
			*newrp = rp2;
			return(0);
		}
	}
	/*
	 * Not found, if they gave us one insert it.
	 */
	if (rp) {
		VASSERT(vm_valusema(&rp->r_lock) <= 0);
		reginsert(rp, vp, data);
		textlstunlock();
		*newrp = rp;
		return(0);
	}
	textlstunlock();
	*newrp = (reg_t *)NULL;
	return(1);
}
