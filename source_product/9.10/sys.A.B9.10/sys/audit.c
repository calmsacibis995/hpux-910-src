/* $Source: /source/hpux_source/kernel/sys.SWT68K_800/sys/RCS/audit.c,v $
 * $Revision: 1.9.83.3 $	$Author: kcs $
 * $State: Exp $   	$Locker:  $
 * $Date: 93/09/17 20:03:35 $
 */
#ifdef AUDIT
#include "../h/types.h"
#include "../h/signal.h"
#include "../h/param.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/audit.h"	
#include "../h/acl.h"
#include "../h/audparam.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../h/kernel.h"
#include "../h/time.h"
#include "../h/pathname.h"

/*
 *	save_aud_data -- collects auditing data and writes an audit
 *			 record if needed.
 *		NOTE 1: If any more parameter types are added, the
 *		        definition of "struct flag" in audit.h needs
 *		        to be increased from 4 bits to 5. 
 *		NOTE 2: The max size of the audit record is decided at compile
 *			time.  As additional system calls are added which can
 *			be audited, the definition of MAX_AUD_DATA in
 *			../h/audit.h should be looked at.  If the new
 * 			system call may save data which could overrun 
 *			the array in audit_bdy_data, which is of size 
 *			MAX_AUD_DATA, MAX_AUD_DATA should be increased.
 */

extern struct	aud_type aud_syscall[];		/* auditing support */
extern struct	aud_flag audparam[];		/* auditing support */

save_aud_data()

{
	int m, n, i, k, *iptr, tmpint;
	long tmplong;
	register struct proc *p;
	struct audit_arg args[4];
	struct audit_hdr aud_head;
	struct audit_bdy_data aud_info;
	struct file *fp;
	struct inode *ip;
	struct timeval atv;
	struct timezone atz;
	struct acl_tuple_user *aclptr;
	struct audit_filename nullpn;
	struct audit_sock nullsock;
	struct audit_string nullstr;
	int arg_index, save_uerror;
	int *uap = u.u_arg;
	struct audit_filename *pathsav = (struct audit_filename *)0;
	struct audit_string *strsav = (struct audit_string *)0;
	
	if (!AUDITON()) {
		rel_aud_mem();
		return;
	}

	/* getprivgrp and setprivgrp both have syscall #: 151 (privgrp)
	 *	The calls are sorted by the kernel based on the first
	 * 	argument.  To further complicate the issue, they
	 * 	also have different parameters.  Only setprivgrp is 
	 *  	audited, so its arg. list is the one known to auditing.  
	 *	If getprivgrp falls through, a buserror will occur.
	 */
	if (u.u_syscall == 151 && (int)u.u_arg[0] == 0)
		return;

	if (aud_syscall[u.u_syscall].logit == NONE) {
		rel_aud_mem();
		return;
	}
	else {
		save_uerror = u.u_error;
		if ((u.u_syscall >= 298) && (u.u_syscall <= 311)) {
			/* ipc system calls */
			/* errno is in last param for ipc calls */
			if (copyin((caddr_t)
				   uap[audparam[u.u_syscall].param_count - 1],
			           (caddr_t) &tmpint, sizeof(int))) {
				rel_aud_mem();
				return;
			}
			u.u_error = tmpint;
		}

/* always restore u_error before returning from this point on */
#define return1 u.u_error = save_uerror; return;

		if ((aud_syscall[u.u_syscall].logit == PASS) &&
			(u.u_error != 0)) {
			rel_aud_mem();
			return1;
		} 
		else if ((aud_syscall[u.u_syscall].logit == FAIL) &&
			(u.u_error == 0)) {
			rel_aud_mem();
			return1;
		}
	}

	/* audit this call */
	p = u.u_procp;
	n = 0;
	arg_index = 2;
	/* initalize args array */
	for (i=0; i < MAX_AUD_ARGS; i++) {
		args[i].data = NULL;
		args[i].len = 0;
	}
	
	/* put the audit record together */
	aud_head.ah_time = time.tv_sec;
	aud_head.ah_pid = p->p_pid;
	aud_head.ah_error = u.u_error;
	aud_head.ah_event = u.u_syscall;

	/* save system call parameters */
	if (audparam[u.u_syscall].rtn_val1) {
		if (u.u_error)
			aud_info.data[n++].ival = -1;
		else
			aud_info.data[n++].ival = u.u_r.r_val1;
	}
	if (audparam[u.u_syscall].rtn_val2)
		aud_info.data[n++].ival = u.u_r.r_val2;

	/* save start of path and string chains */
	/* so that can be released later	*/
	pathsav = u.u_audpath;
	strsav = u.u_audstr;

	for (i = 0; i < audparam[u.u_syscall].param_count; i++) {
		switch (audparam[u.u_syscall].param[i]) {
		case 0:			/* don't save */
			break;
		case 1: 		/* integer */
			aud_info.data[n++].ival = (int)uap[i];
			break;
		case 2:			/* unsigned long */
			aud_info.data[n++].ulval = (u_long)uap[i];
			break;
		case 3:			/* long */
			aud_info.data[n++].lval = (long)uap[i];
			break;
		case 4:			/* string */
			if (u.u_audstr == NULL) {
				if (u.u_error == 0) {
					/* Don't write the record - incomplete
					   data due to turning the auditing
					   system (or system call) on in the
					   middle of this call */
					return1;
				} else {  /* error occured - string not saved */
					/* set string length to 0 */
					nullstr.a_strlen = 0;
					nullstr.a_str[0] = '\0';
					if (arg_index < MAX_AUD_ARGS) {
						args[arg_index].data = (char *)&nullstr;
						args[arg_index].len = sizeof(unsigned) + sizeof('\0');
						arg_index++;
					} else /* if it gets here - increase MAX_AUD_ARGS */
						printf ("save_aud_data: Too many parameters for the audit system\n");
				}
			} else {
				if (arg_index < MAX_AUD_ARGS) {
					args[arg_index].data = (char *)u.u_audstr;
					args[arg_index].len = 
						u.u_audstr->a_strlen + sizeof(unsigned);
					arg_index++;
				} else	   /* if it gets here MAX_AUD_ARGS needs
						to be increased */
					printf ("save_aud_data: Too many parameters for the audit system\n");
				u.u_audstr = u.u_audstr->next;
			}
			break;
		case 5:			/* int* */
			if (u.u_error = copyin((caddr_t)uap[i], 
				(caddr_t)&tmpint, sizeof (int))) {
				aud_info.data[n++].ival = 0;
			}
			else {
				aud_info.data[n++].ival = tmpint;
			}
			break;
		case 6:			/* int array */
			/* this assume the length is the first integer in the
			   array.  The "length" integer is not counted in the
			   length of the array */
			if (u.u_audxparam == NULL) {
				if (u.u_error == 0) {
					/* Don't write the record - incomplete
					   data due to turning the auditing
					   system (or system call) on in the
					   middle of this call */
					return1;
				} else {
					/* error occured before data was
					   copied in - set size to 0 */
					aud_info.data[n++].ival = 0;
				}
			} else {
				iptr = (int *)u.u_audxparam;
				k = *iptr++;
				/* NGROUPS is the largest integer array 
				   currently in use */
				if (k > NGROUPS)
					k = NGROUPS;	/* truncate data */
				/* save length for display program */
				aud_info.data[n++].ival = k;
				for (m = 0; m < k; m++)
					aud_info.data[n++].ival = *iptr++; 
			}
			break;
		case 7:			/* struct sockaddr */
			if (u.u_audsock == NULL) {
				if (u.u_error == 0) {
					/* Don't write the record - incomplete
					   data due to turning the auditing
					   system (or system call) on in the
					   middle of this call */
					return1;
				} else {  /* error occured - data not saved */
					/* pad with zeros for consistency */
					nullsock.a_sock.sa_family = 0;
					nullsock.a_sock.sa_data[0] = '\0';
					nullsock.a_socklen = 
							sizeof(struct sockaddr);
					if (arg_index < MAX_AUD_ARGS) {
						args[arg_index].data = (char *)&nullsock;
						args[arg_index].len = sizeof(nullsock);
						arg_index++;
					} else /* if it gets here - increase MAX_AUD_ARGS */
						printf ("save_aud_data: Too many parameters for the audit system\n");
				}
			} else {
				if (arg_index < MAX_AUD_ARGS) {
					args[arg_index].data = (char *)u.u_audsock;
					args[arg_index].len = 
						sizeof(struct audit_sock);
					arg_index++;
				} else	   /* if it gets here MAX_AUD_ARGS needs
						to be increased */
					printf ("save_aud_data: Too many parameters for the audit system\n");
			}
			break;
		case 8:			/* pathname */
			if (u.u_audpath == NULL) {
				if (u.u_error == 0) {
					/* Don't write the record - incomplete
					   data due to turning the auditing
					   system (or system call) on in the
					   middle of this call */
					return1;
				} else {  /* pad with zero for consistency */
					nullpn.apn_cnode = 0;
					nullpn.apn_dev = 0;
					nullpn.apn_inode = 0;
					nullpn.apn_len = 0;
					if (arg_index < MAX_AUD_ARGS) {
						args[arg_index].data = (char *)&nullpn;
						args[arg_index].len = 
						    ((3 * sizeof(long)) + sizeof(short));
						arg_index++;
					} else	   /* if it gets here 
						      MAX_AUD_ARGS needs
						      to be increased */
						printf ("save_aud_data: Too many parameters for the audit system\n");
				}
			} else {  /* copy pathname info which was previously saved */ 
				if (arg_index < MAX_AUD_ARGS) {
					args[arg_index].data = (char *)u.u_audpath;
					args[arg_index].len = 
					   (3 * sizeof(long) + sizeof(short) +
						u.u_audpath->apn_len);
					arg_index++;
				} else	   /* if it gets here MAX_AUD_ARGS needs
						to be increased */
					printf ("save_aud_data: Too many parameters for the audit system\n");
				u.u_audpath = u.u_audpath->next;
			}
			break;
		case 9:			/* file descriptor */
			fp = getf(uap[i]);
			if (fp == 0) {
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].ulval = 0;
			} else {
				ip = VTOI((struct vnode *)fp->f_data);
				aud_info.data[n++].ulval = 0;
				    /* the cnode is no longer available */
				aud_info.data[n++].ulval = ip->i_dev;
				aud_info.data[n++].ulval = ip->i_number;
			}
			break;
		case 10:		/* struct timeval */
			if (u.u_error = copyin((caddr_t)uap[i], 
				(caddr_t)&atv, sizeof (struct timeval))) {
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].lval = 0;
			}
			else {
				aud_info.data[n++].ulval = atv.tv_sec;
				aud_info.data[n++].lval = atv.tv_usec;
			}
			break;
		case 11:		/* struct timezone */
			if (uap[i]) {
			   	u.u_error = copyin((caddr_t)uap[i], 
			  		(caddr_t)&atz, sizeof (atz));
				if (u.u_error == 0) {
					aud_info.data[n++].ival = atz.tz_minuteswest;
					aud_info.data[n++].ival = atz.tz_dsttime;
					break;
				}
			}
			aud_info.data[n++].ival = 0;
			aud_info.data[n++].ival = 0;
			break;
		case 12:		/* save address of char* */
			aud_info.data[n++].ulval = (unsigned)uap[i];
			break;
		case 13:		/* char* cast to a long* */
			if (u.u_error = copyin((caddr_t)uap[i], 
				(caddr_t)&tmplong, sizeof (long))) {
				aud_info.data[n++].ulval = 0;
			}
			else {
				aud_info.data[n++].ulval = tmplong;
			}
			break;
		case 14:		/* access control list */
			if (u.u_audxparam == NULL) {
				if (u.u_error == 0) {
					/* Don't write the record - incomplete
					   data due to turning the auditing
					   system (or system call) on in the
					   middle of this call */
					return1;
				} else {
					/* error occured before data was
					   copied in - set size to 0 */
					aud_info.data[n-1].ival = 0;
				}
			} else {
				/* this assumes the len was the prev param */
				k = aud_info.data[n-1].ival;
				if (k > NACLTUPLES)
					k = NACLTUPLES;
				aclptr = (struct acl_tuple_user *)u.u_audxparam;
				for (m = 0; m < k; m++)
				{
					aud_info.data[n++].lval = aclptr->uid;
					aud_info.data[n++].lval = aclptr->gid;
					aud_info.data[n++].ucval = aclptr->mode;
					aclptr++;
				}
			}
			break;
		case 15:		/* file descriptors as return values */
			fp = getf(u.u_r.r_val1);
			if (fp == 0) {
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].ulval = 0;
			} else {
				ip = VTOI((struct vnode *)fp->f_data);
				aud_info.data[n++].ulval = 0;
				    /* the cnode is no longer available */
				aud_info.data[n++].ulval = ip->i_dev;
				aud_info.data[n++].ulval = ip->i_number;
			}
			fp = getf(u.u_r.r_val2);
			if (fp == 0) {
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].ulval = 0;
				aud_info.data[n++].ulval = 0;
			} else {
				ip = VTOI((struct vnode *)fp->f_data);
				aud_info.data[n++].ulval = 0;
				    /* the cnode is no longer available */
				aud_info.data[n++].ulval = ip->i_dev;
				aud_info.data[n++].ulval = ip->i_number;
			}
			break;
		}
	}
	args[1].data = (char *)&aud_info;
	args[1].len = n * (sizeof(union ab_data));   
	aud_head.ah_len = 0;
	for (i=1; i < MAX_AUD_ARGS; i++)
		aud_head.ah_len += args[i].len;
	args[0].data = (char *)&aud_head;
	args[0].len = sizeof(aud_head);

	/* let kern_aud_wr write it */
	kern_aud_wr(args);

	/* restore u_error */
	u.u_error = save_uerror;
	/* restore path and string pointers so rel_aud_mem */
	/* can free them				   */

	u.u_audpath = pathsav;
        u.u_audstr = strsav;
	rel_aud_mem();	/* release any kernel memory */
}

/*
 *  Release any kernel memory allocated to save audit data.
 *		(auditing support)
 */
rel_aud_mem()

{
	struct audit_filename *pnptr;
	struct audit_string *strptr;
	struct audit_sock *sockptr;

	while (u.u_audpath != NULL) {
		pnptr = u.u_audpath;
		u.u_audpath = u.u_audpath->next;
		kmem_free((caddr_t)pnptr, sizeof(struct audit_filename));
	}
	while (u.u_audstr != NULL) {
		strptr = u.u_audstr;
		u.u_audstr = u.u_audstr->next;
		kmem_free((caddr_t)strptr, sizeof(struct audit_string));
	}
	if (u.u_audsock != NULL) {
		sockptr = u.u_audsock;
		kmem_free((caddr_t)sockptr, sizeof(struct audit_sock));
		u.u_audsock = NULL;
	}
	if (u.u_audxparam != NULL) {
		kmem_free(u.u_audxparam, sizeof(struct acl_tuple_user) * NACLTUPLES);
		u.u_audxparam = NULL;
	}
}

extern struct aud_type aud_syscall[];

/*
 * get_pn_info():
 *
 *	this function is only called by lookuppn().
 *	we get here ONLY if the length of the path
 *	name is greater than 0.  i.e., only non-null
 *	paths will be passed in.   thus, in theory,
 *	this function will always locate the record
 *	in question.
 *
 *	we must check for inode == 0 because we are
 *	going to set them later; a system call may
 *	pass in mutiple paths that are the same, so
 *	both records must be updated properly.  if
 *	we only did a strncmp(), then only one of the
 *	records in the linked list would ever have
 *	the dev and inode set properly.
 */
struct audit_filename *
get_pn_info(pnp)
register struct pathname *pnp;
{
	struct audit_filename *curptr;


	for (curptr = u.u_audpath; curptr != NULL; curptr = curptr->next) {
		if (curptr->apn_inode == (u_long)0) {
			if (strncmp(pnp->pn_buf, curptr->apn_path,
			    pnp->pn_pathlen) == 0) {
				return(curptr);
			}
		}
	}
	return(NULL);
}

/*
 * save_pn_info():
 *
 *	if auditing is not on, return.
 *
 *	allocate an audit_filename record.
 *
 *	copy in the path if it exists, zero
 *	it out if not.
 *
 *	initialize remainder of record.
 *
 *	put record into linked list.
 */
struct audit_filename *
save_pn_info(path)
register char *path;
{
	register struct audit_filename *pathptr;
	register struct audit_filename *prevptr;
	unsigned length;


	/*
	 * check to see if this information needs to be saved
	 */
	if ((!AUDITON()) || (aud_syscall[u.u_syscall].logit == NONE))
		return NULL;

	/*
	 * allocate memory for pathname information
	 */
	pathptr = (struct audit_filename *)
		kmem_alloc(sizeof(struct audit_filename));

	/*
	 * if copyinstr fails, just zero fill the path
	 */
	if (copyinstr(path, pathptr->apn_path, MAXPATHLEN, &length) != 0) {
		pathptr->apn_path[0] = (char)0;
		pathptr->apn_len = 0;
	}
	else {
		pathptr->apn_len = length - 1;	/* don't count null char */
	}

	/*
	 * initialize rest of record
	 */
	pathptr->apn_inode = (u_long)0;
	pathptr->apn_dev = (u_long)0;
	pathptr->apn_cnode = (u_long)0;
	pathptr->next = NULL;

	/*
	 * put record into linked list
	 */
	if (u.u_audpath == NULL) {
		u.u_audpath = pathptr;
	}
	else { 
		prevptr = u.u_audpath;
		while (prevptr->next != NULL) 
			prevptr = prevptr->next;
		prevptr->next = pathptr;
	}

	return(pathptr);
}

save_vp_info(vp, pathptr)
struct vnode *vp;
register struct audit_filename *pathptr;

{
	struct vattr vattr;

	/* check to see if this information needs to be saved */
	if (pathptr == NULL)
		/* don't save the info -- auditing is either not on
		   or has just been turned on (not a complete record) */
		return;
	
	switch(vp->v_fstype) {
        case VUFS:
        case VDUX:
                VOP_GETATTR(vp, &vattr, u.u_cred, VSYNC);
                pathptr->apn_cnode = vattr.va_fssite;         
                pathptr->apn_dev = vattr.va_realdev;
                pathptr->apn_inode = vattr.va_nodeid;
                break;
	default:
                VOP_GETATTR(vp, &vattr, u.u_cred, VASYNC);
                pathptr->apn_cnode = 0;
                pathptr->apn_dev = 0;
                pathptr->apn_inode = 0;
                break;
	}
}

/*
 * save_str():
 *
 *	if auditing is not on, return.
 *
 *	allocate an audit_string record.
 *
 *	copy in the string if it exists, zero
 *	it out if not.
 *
 *	initialize remainder of record.
 *
 *	put record into linked list.
 */
struct audit_string *
save_str(string)
register char *string;
{
	register struct audit_string *strptr;
	register struct audit_string *prevptr;
	unsigned length;


	/*
	 * check to see if this information needs to be saved
	 */
	if ((!AUDITON()) || (aud_syscall[u.u_syscall].logit == NONE))
		return NULL;

	/*
	 * allocate memory for pathname information
	 */
	strptr = (struct audit_string *)
		kmem_alloc(sizeof(struct audit_string));

	/*
	 * if copyinstr fails, just zero fill the string
	 */
	if (copyinstr(string, strptr->a_str, MAXAUDSTRING, &length) != 0) {
		strptr->a_str[0] = (char)0;
		strptr->a_strlen = 0;
	}
	else {
		strptr->a_strlen = length - 1;	/* don't count null char */
	}

	/*
	 * initialize rest of record
	 */
	strptr->next = NULL;

	/*
	 * put record into linked list
	 */
	if (u.u_audstr == NULL) {
		u.u_audstr = strptr;
	}
	else { 
		prevptr = u.u_audstr;
		while (strptr->next != NULL) 
			prevptr = prevptr->next;
		prevptr->next = strptr;
	}

	return(strptr);
}

/* This routine saves a sockaddr parameter for use later in the audit record.
 * Kernel memory is allocated to store the information.  It will
 * be freed when the information is moved to the audit record. 
 * 		( auditing support )
 */
save_sockaddr(strptr, len)
char *strptr;
unsigned len;

{
	struct audit_sock *audsock;

	/* check to see if this information needs to be saved */
	if ((!AUDITON()) || (aud_syscall[u.u_syscall].logit == NONE))
		return;

	/* allocate memory for the sockaddr */
	audsock =(struct audit_sock *)kmem_alloc(sizeof(struct audit_sock));
	/* attach to the u-area */
	u.u_audsock = audsock;
	len = MIN(len, sizeof(struct sockaddr));
	bcopy(strptr, (char *)&audsock->a_sock, len);
	audsock->a_socklen = len;
}

/* This routine saves the acl parameter for use later in the audit record.
 * Kernel memory is allocated to store the information.  It will
 * be freed when the information is moved to the audit record. 
 * 		( auditing support )
 */
save_acl(aclptr, len)
char *aclptr;
unsigned len;

{
	char *audacl;

	/* check to see if this information needs to be saved */
	if ((!AUDITON()) || (aud_syscall[u.u_syscall].logit == NONE))
		return;

	/* allocate memory for the acls */
	audacl =(char *)kmem_alloc(sizeof(struct acl_tuple_user) * NACLTUPLES);
	/* attach to the u-area - in the "generic" audit data location */
	u.u_audxparam = audacl;
	bcopy(aclptr, audacl, len);
}

/* This routine saves an integer array for use later in the audit record.
 * Kernel memory is allocated to store the information.  It will
 * be freed when the information is moved to the audit record. 
 * To allow this to work in conjuction with save_acl(), more memory
 * than is required for this parameter is allocated.
 * 		( auditing support )
 */
save_iarray(arrayptr, len)
char *arrayptr;
unsigned len;

{
	char *audarray;
	int int_len, real_len;

	/* check to see if this information needs to be saved */
	if ((!AUDITON()) || (aud_syscall[u.u_syscall].logit == NONE))
		return;

	/* allocate memory - this allocates the same amount of memory
	   as save_acl so rel_aud_mem knows how much memory it needs to
	   release */
	audarray=(char *)kmem_alloc(sizeof(struct acl_tuple_user) * NACLTUPLES);

	/* attach to the u-area - in the "generic" audit data location */
	u.u_audxparam = audarray;

	len = MIN(len, sizeof(struct acl_tuple_user)*NACLTUPLES);
	/* prefix the length of the array to the array */
	int_len = sizeof(len);		/* get size of an integer */
	real_len = len / int_len;	/* get the number of int's in array */
	bcopy((char *)&real_len, audarray, int_len);

	/* copy the array itself */
	bcopy(arrayptr, &audarray[int_len], len);
}

#endif AUDIT
