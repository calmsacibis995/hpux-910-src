/*
 * vfd.c -- contains routines that will do a btree search.
 *
 */

/*
(c) Copyright 1983, 1984 Hewlett-Packard Company.
(c) Copyright 1979 The Regents of the University of Colorado, a body corporate
(c) Copyright 1979, 1980, 1983 The Regents of the University of California
(c) Copyright 1980, 1984 AT&T Technologies.  All Rights Reserved.
The contents of this software are proprietary and confidential to the Hewlett-
Packard Company, and are limited in distribution to those with a direct need
to know.  Individuals having access to this software are responsible for main-
taining the confidentiality of the content and for keeping the software secure
when not in use.  Transfer to any party is strictly forbidden other than as
expressly permitted in writing by Hewlett-Packard Company.  Unauthorized trans-
fer to or possession by any unauthorized party may be a criminal offense.
*/


#define _VFD_INTERNALS
#include "inc.h"
#include "defs.h"
#include "types.h"
#include "externs.h"
#undef _VFD_INTERNALS
extern int translate_enable;
extern chunk_t * bt_search();




/*
 * Return stored pointer, NULL if not found.
 */
/* MEMSTAT */
struct broot *bt_br=0;
struct broot *find_br=0;
int bt_fail=0;
int find_fail=0;

/* br is localized */
chunk_t *
bt_search(br, key)
struct broot *br;
register unsigned long key;
{
	register struct bnode *bt = br->b_root, *abt;
	register unsigned long *lp, val;
	register int x, depth = br->b_depth;

	abt = GETBYTES(struct bnode *, bt, sizeof(struct bnode));
	if (abt == 0) {
		if (bt_br != br){
			bt_br   = br;
			bt_fail = 0;
		}
		/* Don't be so verbose when its not allocated yet */
		if (bt_fail++ < 1)
			fprintf(outf,"bt_search: localizing abt failed  bt 0x%x\n", bt);
		return(NULL);
	}
	/* While we have a node to consider, search for key */
	while ((depth-- > 0) && bt) {

		/* Linear search across */
		lp = abt->b_key;
		for (x = 0; x < abt->b_nelem; ++x) {

			/* If found/passed key value, go down */
			if ((val = *lp++) >= key) {

				/* If leaf, we have our value */
				if (!depth) {

					/*
					 * Get exact hit, or it
					 *  isn't in the tree
					 */
					if (val == key)
						return((chunk_t *)
						 (abt->b_down[x+1]));
					return(NULL);
				}

				/* Descend a level */
				if (val == key)
					x += 1;
				break;
			}

			/* Otherwise, keep going */
		}

		/* Fell off the end, so pick up the last down pointer */
		bt = abt->b_down[x];
		abt = GETBYTES(struct bnode *, bt, sizeof(struct bnode));
		if (abt == 0) {
			fprintf(outf,"bt_search2: localizing abt failed\n");
			return(NULL);
		}
	}
	return (NULL);
}

/*
 * Return pointer to vfd
 */
vfd_t *
findvfd(rp, i)
register reg_t *rp;
register int i;
{
	register chunk_t *chunk,*vfd;
	register struct broot *br;

	/* VASSERT(rp->r_root); */

	/* Check in the region first */
	if (rp->r_key == CINDEX(i)) {
		chunk = rp->r_chunk;
	} else {
		br = GETBYTES(struct broot *, rp->r_root, sizeof(struct broot));
		if (br == 0) {
			if (find_br != br){
				find_br   = br;
				find_fail = 0;
			}
			/* Don't be so verbose when its not allocated yet */
			if (find_fail++ < 4)
				fprintf(outf,"findvfd: localizing br failed\n");
			return(0);
		}
		/* Search existing entry */
		chunk = (chunk_t *)bt_search(br, CINDEX(i));
	}

	/* First reference.  Allocate it & insert it */
	if (!chunk) {
		return(0);
	}
	vfd=GETBYTES(chunk_t *, chunk, (CHUNKENT * sizeof(struct vfddbd)));
	if (vfd == 0) {
		fprintf(outf,"findvfd: localizing object failed\n");
		return(0);
	}

	/* Return pointer to the correct vfd */
	return( &((*vfd)[COFFSET(i)].c_vfd) );
}

/*
 * Return pointer to dbd
 */
dbd_t *
finddbd(rp, i)
register reg_t *rp;
register int i;
{
	register chunk_t *chunk,*dbd;
	register struct broot *br;

	/* VASSERT(rp->r_root); */

	/* Check in the region first */
	if (rp->r_key == CINDEX(i)) {
		chunk = rp->r_chunk;
	} else {
		br = GETBYTES(struct broot *, rp->r_root, sizeof(struct broot));
		if (br == 0) {
			if (find_br != br){
				find_br   = br;
				find_fail = 0;
			}
			/* Don't be so verbose when its not allocated yet */
			if (find_fail++ < 4)
				fprintf(outf,"finddbd: localizing br failed\n");
			return(NULL);
		}
		/* Search existing entry */
		chunk = (chunk_t *)bt_search(br, CINDEX(i));
	}

	/* First reference.  Allocate it & insert it */
	if (!chunk) {

		return(0);
	}

	dbd=GETBYTES(chunk_t *, chunk, (CHUNKENT * sizeof(struct vfddbd)));
	if (dbd == 0) {
		fprintf(outf,"finddbd2: localizing dbd failed\n");
		return(0);
	}

	/* Return pointer to the correct vfd */
	return( &((*dbd)[COFFSET(i)].c_dbd) );
}

/*
 * Find & return both vfd and dbd
 */
vfd_t *
findentry(rp, i, vfdp, dbdp)
register reg_t *rp;
register int i;
vfd_t **vfdp;
dbd_t **dbdp;
{
	register chunk_t *chunk,*avfd;
	register struct vfddbd *vd;
	register struct broot *br;

	/* VASSERT(rp->r_root); */

	/* Check in the region first */
	if (rp->r_key == CINDEX(i)) {
		chunk = rp->r_chunk;
	} else {
		br = GETBYTES(struct broot *, rp->r_root, sizeof(struct broot));
		if (br == 0) {
			fprintf(outf,"findentry: localizing object failed\n");
			return(NULL);
		}
		/* Search existing entry */
		chunk = (chunk_t *)bt_search(br, CINDEX(i));
	}

	/* First reference.  Allocate it & insert it */
	if (!chunk) {
		return(NULL);
	}

	avfd=GETBYTES(chunk_t *, chunk, (CHUNKENT * sizeof(struct vfddbd)));
	if (avfd == 0) {
		fprintf(outf,"findentry: localizing avfd failed\n");
		return(NULL);
	}
	/* Return pointers */
	vd = &((*avfd)[COFFSET(i)]);
	*vfdp = &(vd->c_vfd);
	*dbdp = &(vd->c_dbd);
	return((vfd_t *)vfdp);
}
