/*
 * @(#)nipc_sr.h: $Revision: 1.2.83.4 $ $Date: 93/09/17 19:11:28 $
 * $Locker:  $
 */

/*
 *  Header file for socket registry name table. 
 */

#ifndef NIPC_SR.H
#define NIPC_SR.H

#define SR_HASH_SIZE	10

struct name_record {
	struct name_record	*nr_nextname;
	int			nr_nlen;
	char			nr_name[NS_MAX_SOCKET_NAME];
	struct file		*nr_ftab;
};
#endif /* NIPC_SR.H */
