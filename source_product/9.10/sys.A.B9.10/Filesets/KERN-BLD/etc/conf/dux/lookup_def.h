/*
 * @(#)lookup_def.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:44:13 $
 * $Locker:  $
 */

/*
 * This needs to be in a seperate file (not dux_lookup.h) so that cdrom
 * can be configured without dskless, if dskless is not configured, the linker
 * will complain about this undefined symbol.
 */

/*operations and sizes associated with lookup*/
typedef struct
{
	short	lk_request_size;	/* number of bytes in request */
	short	lk_reply_size;		/* number of bytes in reply */
	int	(*lk_request_pack)();	/* function to pack request */
	int	(*lk_reply_unpack)();	/* function to unpack reply */
	int	(*lk_serve)();		/* function to service request */
} lookup_ops_t;

extern lookup_ops_t lookup_ops[];
