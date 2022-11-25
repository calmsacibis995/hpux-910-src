/*
 * @(#)lkup_dep.h: $Revision: 1.3.83.3 $ $Date: 93/09/17 16:44:08 $
 * $Locker:  $
 */

/* HPUX_ID: @(#)lkup_dep.h	51.1		88/01/20 */
/*
 *This file contains the opcode dependent structures passed to the lookup
 *command
 */

#include "../h/acl.h"

/* Create and open structure */
struct copenops
{
	struct vattr *vap;	/*only used for create (NULL for open)*/
	enum vcexcl excl;	/*only used for create*/
	int mode;
	int filemode;
	struct vnode **compvpp;	/*where result should be placed.
				 *kludge:  we should use the compvp parameter,
				 *however, on exclusive create, this is set
				 *to 0, under the assumption that lookup will
				 *not return it.  We, however, do return 
				 *on a remote create.
				 */
	int is_open;		/*there is one call we should make only
				 *on an open.  see dux_copen_unpack
				 *for details
				 */
};

/* Setattr structure */
struct setattrops
{
	struct vattr *vap;
	int null_time;		/*kludge*/
};


/* lookup dependent structure for setacl */

struct setaclops
{
	int ntuples;
	struct acl_tuple acl[NACLTUPLES];
};

/* lookup dependent structure for getacl */

struct getaclops
{
	int ntuples;
	struct acl_tuple_user *tupleset;
};
