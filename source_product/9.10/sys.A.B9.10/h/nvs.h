/*
 * @(#)nvs.h: $Revision: 1.3.83.4 $ $Date: 93/09/17 18:31:26 $
 * $Locker:  $
 */

#ifndef _SYS_NVS_INCLUDED
#define _SYS_NVS_INCLUDED

/*
 * nvs.h
 * This contains the definition of the nvs join structure.  There is a table
 * declared in space.h which defines one entry for each defined pty.  The pty
 * minor number will be an index into this table.  Nvs_so_int will be typecast
 * to (struct socket *) in the tty_nvs module.  It is left as a long here so
 * we don't have to worry about the order of included header files in space.h
 * such as socketvar.h.  If nvs_so_int is zero, the associated pty is not
 * bound to a socket.  If nvs_so_int is non-zero, it is a pointer to the socket
 * structure of the socket associated with this pty.  The socket structure
 * contains a field for the pty minor number so that the pty can also be found
 * quickly given the socket pointer.  The socket will have a flag set
 * indicating that it is bound to a pty.
 */

struct nvsj {
	long	nvs_so_int;	/* this will be cast to socket pointer */
	u_short	nvs_in_mode;	/* input side mode */
	u_short	nvs_out_mode;	/* output side mode */
};

#endif /* not _SYS_NVS_INCLUDED */
