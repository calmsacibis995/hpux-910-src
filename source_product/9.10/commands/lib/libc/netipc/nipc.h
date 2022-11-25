#if	defined(MODULEID) && !defined(lint)
static char rcsid[]="@(#) $Header: nipc.h,v 66.1 90/01/17 19:16:24 rsh Exp $";
#endif


#include	<errno.h>
#include	<sys/types.h>
#include	<sys/ns_ipc.h>

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b)	(a)<(b)?(a):(b)

/* structure of the head of Netipc options buffer */
struct opthead {
	short	opt_length;	/* length of buffer sans this header */
	short	opt_entries;	/* number of optentrys before data */
};

/* structure of entry info. which is after header and before data */
struct optentry {
	short	opte_code;	/* defines a Netipc specific option */
	short	opte_offset;	/* from beginning of buffer to data */
	short	opte_length;	/* length of data */
	short	opte_fill;	/* force entry to be four byte aligned */
};

/* declare the global errno to be defined externally */
extern int errno;

#define ERR_RETURN(err)	{*result = err; return;}


#define SET_RESULT(err)	 {						\
		if (err < 0) {						\
			if (errno == EINTR)				\
				*result = NSR_SIGNAL_INDICATION;	\
			else						\
				*result = errno - NIPC_ERROR_OFFSET;	\
		}							\
		else							\
			*result = NSR_NO_ERROR;				\
	}

struct result_map {
	int	res_code;
	char	*res_string;
};
