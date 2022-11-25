/*
 * $Header: nipc_sr.c,v 1.3.83.4 93/09/17 19:09:31 kcs Exp $
 * $Source: /source/hpux_source/kernel/sys.SWT68K_800/nipc/RCS/nipc_sr.c,v $
 * $Revision: 1.3.83.4 $		$Author: kcs $
 * $State: Exp $		$Locker:  $
 * $Date: 93/09/17 19:09:31 $
 */

#if defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) nipc_sr.c $Revision: 1.3.83.4 $";
#endif

/* 
 * This file contains routines which define and manipulate the socket registry
 * data structure.  Interface routines are provided for adding, deleting and
 * looking up entries as well as a routine that deletes the name record(s) for
 * a socket when the socket is going away.
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/malloc.h"
#include "../h/mbuf.h"
#include "../h/socket.h"
#include "../h/socketvar.h"
#include "../h/ns_ipc.h"
#include "../nipc/nipc_err.h"
#include "../nipc/nipc_kern.h"
#include "../nipc/nipc_cb.h"
#include "../nipc/nipc_name.h"
#include "../nipc/nipc_sr.h"


struct name_record *	 nipc_sockreg[SR_HASH_SIZE] = 
				{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};


sr_add(name, nlen, ftab, nr)

char		*name;	/* socket name to add to socket registry    */
int		nlen;	/* length of socket name     		    */
struct file	*ftab;	/* file table entry for socket being named  */
caddr_t		*nr;	/* (output) identifier of added name record */

{
	struct name_record *newnr; /* pointer to new name record */	
	struct file	*fp;	/* local file table pointer   */
	int		error;	/* local error 		      */
	int		i;	/* hash index for socket name */

	/* check for duplicate name */
	error = sr_lookup(name, nlen, &fp);
	if (!error)
		return (E_DUPNAME);

	/* allocate a name record for the new socket name */
	MALLOC(newnr, struct name_record *, sizeof(struct name_record),
		M_NIPCSR, M_WAITOK);
	bzero((caddr_t)newnr, sizeof(struct name_record));
	mbstat.m_mtypes[MT_SONAME]++;

	/* hash the name to an array index of nipc_sockreg */
	i = sr_hashname(name, nlen);

	/* link the new name record at the beginning of the list */
	/* at index i of the nipc_sockreg array			 */
	newnr->nr_nextname = nipc_sockreg[i];
	nipc_sockreg[i] = newnr;

	/* fill in the name record with the caller's data */
	newnr->nr_nlen = nlen;
	newnr->nr_ftab = ftab;
	bcopy(name, newnr->nr_name, nlen);

	/* return the address of the name record in the caller's output parm */
	*nr = (caddr_t) newnr;

	return (0);

}  /* sr_add */


sr_delete(name, nlen)

char *name;	/* socket name of name record to be deleted */
int  nlen;	/* length of socket name in bytes	    */

{
	int	i;	/* array index of the name record 	   */
	int	found;	/* flag indicating if name record exists   */
	struct name_record *nr, *prev_nr;  /* name record pointers */

	/* hash name to the array index where the record should be located */
	i = sr_hashname(name, nlen);

	/* search for the name record in the list at index i of nipc_sockreg */
	found  = 0;
	prev_nr = (struct name_record *)0;
	for (nr = nipc_sockreg[i]; nr; nr = nr->nr_nextname) {
		if ((nr->nr_nlen == nlen) && (bcmp(nr->nr_name, name, nlen) == 0)) {
			found++;
			break;
		}
		else
			prev_nr = nr;
	}
	if (!found)
		return E_NAMENOTFOUND;

	/* remove the name record from the list and free it */
	if (!prev_nr) 
		/* name record was first on the list */
		nipc_sockreg[i] = nr->nr_nextname;
	else 
		prev_nr->nr_nextname = nr->nr_nextname;
	FREE(nr, M_NIPCSR);
	mbstat.m_mtypes[MT_SONAME]--;
			
	return (0);

}  /* sr_delete */


sr_hashname(name, nlen)

char *name;	/* socket name    */
int  nlen;	/* length of name */

{
	u_long	sum = 0;
	char	*c;

	/* calculate the sum of the characters in the name */
	for (c = name; nlen; c++) {
		sum += *c;
		nlen--;
	}

	/* modulo the sum to get the sockreg hash array index */
	return (sum % SR_HASH_SIZE);

}  /* sr_hashname */
 

sr_lookup(name, nlen, ftab)

char		*name;	/* socket name to lookup 		 */
int		nlen;	/* byte length of name   		 */
struct file	**ftab; /* (output) file pointer of named socket */

{
	int		i;	/* hash index of name  */
	int		found;	/* name record found?  */
	struct name_record *nr; /* name record pointer */

	/* hash name to the array index where the record should be located */
	i = sr_hashname(name, nlen);

	/* search for the name record in the list at index i of nipc_sockreg */
	found  = 0;
	for (nr = nipc_sockreg[i]; (nr); nr = nr->nr_nextname)
		if ((nr->nr_nlen == nlen) && (bcmp(nr->nr_name, name, nlen) == 0)) {
			found++;
			break;
		}

	/* return the ftab pointer of the name record if we found one */
	if (!found)
		return E_NAMENOTFOUND;
	else {
		*ftab = nr->nr_ftab;
		return (0);
	}

}  /* sr_lookup */


sr_sodelete(namerecord, ftab)

caddr_t		namerecord;	/* address of the name record for the socket */
struct file	*ftab;		/* file table pointer for the socket	     */

{
	struct name_record	*nr, *prevnr, *nextnr;	/* name record pointers */
	int			i;		/* array index         */

	/* if a name record address was passed, merely delete it and return */
	if (namerecord) {
		nr = (struct name_record *) namerecord;
		if (sr_delete(nr->nr_name, nr->nr_nlen))
			panic("sr_sodelete");
		return;
	}

	/* otherwise there was a name collision so search through the */
	/* entire socket registry, deleting all name records whose    */
	/* ftab pointer matches the input ftab pointer of the socket  */
	for (i = 0; i < SR_HASH_SIZE; i++) {
		prevnr = ((struct name_record *)0);
		for (nr = nipc_sockreg[i]; (nr); nr = nextnr) {
			nextnr = nr->nr_nextname;
			if (nr->nr_ftab == ftab) {
				if (!prevnr)
					/* name record is first on the list */
					nipc_sockreg[i] = nextnr;
				else
					prevnr->nr_nextname = nextnr;
				FREE(nr, M_NIPCSR);
			} else
				prevnr = nr;
		}
	}

}  /* sr_sodelete */
	
