/*
 * @(#)_user.h: $Revision: 1.2.83.3 $ $Date: 93/09/17 17:01:29 $
 * $Locker:  $
 */

/* 
 * NOTE: This header file contains information specific to the 
 * internals of the HP-UX implementation. The contents of 
 * this header file are subject to change without notice. Such
 * changes may affect source code, object code, or binary
 * compatibility between releases of HP-UX. Code which uses 
 * the symbols contained within this header file is inherently
 * non-portable (even between HP-UX implementations).
*/

#ifndef _UNSUPP_SYS_USER_INCLUDED
#define _UNSUPP_SYS_USER_INCLUDED

#ifdef	_KERNEL
#ifdef MP_LIFTTW

/*
 * These are all involved with lifting the training wheels
 * For more details, see setjmp_lifttw() and longjmp_lifttw()
 */
#define UF_KS_REAQUIRE	0x0002		/* Reaquire kernel sema */
#define setjmp(ss)	(setjmp_lifttw(ss),setjmp_real(ss))
#define longjmp(ss)	(longjmp_lifttw(ss),longjmp_real(ss))

#endif /* MP_LIFTTW */

#define	crhold(cr)	{register u_int context; MP_SPINLOCK_USAV(cred_lock,context);(cr)->cr_ref++;MP_SPINUNLOCK_USAV(cred_lock,context);}
struct ucred *crget();
struct ucred *crcopy();
struct ucred *crdup();
#endif /* _KERNEL */


#if defined(__hp9000s800) && defined(_KERNEL)
/* WARNING: NEVER, NEVER, NEVER use u as a local variable
 * name or as a structure element in I/O system or elsewhere in the
 * kernel.
 */
#define u (*uptr)
#define udot (*uptr)
#endif /* __hp9000s800 && _KERNEL */

#ifdef _KERNEL
extern	struct user u;
#ifndef KMAP_CLEANUP
extern	struct user swaputl;
extern	struct user forkutl;
extern	struct user xswaputl;
extern	struct user xswap2utl;
extern	struct user pushutl;
extern	struct user vfutl;
#ifdef __hp9000s300
extern	struct user chklockutl;
#endif /* __hp9000s300 */
#endif /* not KMAP_CLEANUP */
#endif /* _KERNEL */

#endif /* _UNSUPP_SYS_USER_INCLUDED */
