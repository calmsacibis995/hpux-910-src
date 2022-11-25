/*
 * @(#)slot.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 20:21:01 $
 * $Locker:  $
 */
#ifndef _SYS_SLOT_INCLUDED /* allows multiple inclusion */
#define _SYS_SLOT_INCLUDED
enum slotstatus {NONE, COMPACT, FOUND, EXIST};
struct slot {
	enum	slotstatus status;
	int	offset;		/* offset of area with free space */
	int	size;		/* size of area at slotoffset */
	struct buf *bp;		/* dir buf where slot is */
	struct direct *ep;	/* pointer to slot */
};
#endif /* _SYS_SLOT_INCLUDED */
