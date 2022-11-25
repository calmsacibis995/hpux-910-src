/*
 * @(#)duxparam.h: $Revision: 1.7.83.3 $ $Date: 93/09/17 16:43:36 $
 * $Locker:  $
 */

/* HPUX_ID: %W%		%E% */

#ifndef _SYS_DUXPARAM_INCLUDED /* allows multiple inclusion */
#define _SYS_DUXPARAM_INCLUDED

/*
 * Variable parameters used in DUX
 */

#define MAXSITE 256	/*maximum site in a cluster*/
#define PRIDM (37+PTIMESHARE)	/*dont ask me why 37*/

/* Parameters defining SDO definition */
#define SDOCHAR '+'	/*special character for SDO override*/
#define ISSDO(ip) (((ip)->i_mode&(IFMT|ISUID)) == (IFDIR|ISUID))

/*
 * Because the context is passed between machines, the sizes below must
 * match on both machines.  The sizes must also ensure 4-byte alignment
 */ 
#define NUM_CNTX_PTR 16
#define CNTX_BUF_SIZE 128
struct dux_context
{
	char *ptr[NUM_CNTX_PTR];
	char buf[CNTX_BUF_SIZE];
};

/*
 * Defines to use when the blocksize on a server is greater than
 * MAXDUXBSIZE.  DUX can only handle block sizes up to MAXDUXBSIZE.
 */
#define MAXDUXBSIZE	8192
#define MAXDUXBMASK	0xffffe000
#define MAXDUXBSHIFT	13

#ifdef _KERNEL
#ifdef __hp9000s300
    extern void get_precise_time();
#   define dux_gettimeofday(t)	(get_precise_time(t))
#else
    extern struct timeval ms_gettimeofday();
#   define dux_gettimeofday(t)	(*(t) = ms_gettimeofday())
#endif
#endif /* _KERNEL */
#endif /* _SYS_DUXPARAM_INCLUDED */
