/* @(#) $Revision: 70.1 $ */    
#ifndef _MON_INCLUDED /* allow multiple inclusions */
#define _MON_INCLUDED

#ifdef HFS

struct hdr {
	char	*lpc;
	char	*hpc;
	int	nfns;
};

struct cnt {
	char	*fnpc;
	long	mcnt;
};

typedef unsigned short WORD;

#define MON_OUT	"mon.out"
#define MPROGS0	600
#define MSCALE0	4
#define NULL	0

#  ifdef __cplusplus
     extern "C" {
#  endif /* __cplusplus */

#  if defined(__STDC__) || defined(__cplusplus)
     extern void monitor(void (*)(), void(*)(), WORD *, int, int);
#  else /* __STDC__ || __cplusplus */
     extern void monitor();
#  endif /* __STDC__ || __cplusplus */

#  ifdef __cplusplus
     }
#  endif /* __cplusplus */

#endif /* HFS */
#endif /* _MON_INCLUDED */
