/* SCCS messages.h   REV(64.1);       DATE(92/04/03        14:22:13) */
/* KLEENIX_ID @(#)messages.h	64.1 91/08/28 */
#	define	NUMMSGS	245

#ifndef WERROR
#    define	WERROR	werror
#endif

#ifndef UERROR
#    define	UERROR	uerror
#endif

#ifndef MESSAGE
#    define	MESSAGE(x)	msgtext[ x ]
#endif

extern char	*msgtext[ ];
