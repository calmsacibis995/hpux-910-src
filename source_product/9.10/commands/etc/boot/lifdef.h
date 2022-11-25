/*
 * @(#)lifdef.h: $Revision: 70.2 $ $Date: 93/11/23 11:01:15 $
 * $Locker:  $
 */

/* @(#) $Revision: 70.2 $ */      

#ifndef _SYS_LIFDEF_INCLUDED /* allows multiple inclusion */
#define _SYS_LIFDEF_INCLUDED

#ifndef _SYS_PARAM_INCLUDED
#ifdef _KERNEL_BUILD
#    include "../h/param.h"			/* for MAXPATHLEN */
#else  /* ! _KERNEL_BUILD */
#    include <sys/param.h>			/* for MAXPATHLEN */
#endif /* _KERNEL_BUILD */
#endif /* _SYS_PARAM_INCLUDED */

#define MAXVOLNAME	6
#define MAXFILENAME     10
#define MAXDIRPATH      MAXPATHLEN
#define	LIFREAD		0
#define LIFWRITE	1
#define LIFUPDATE	2
#define QRT_K		256
#define HALF_K		512
#define ONE_K		1024
#define TWO_K		2048
#define K64		65536
#define K63		64512
#define	EOD		-1
#define DESIZE		32

#define	LIFKEEP		0
#define	LIFREMOVE	1
#define	LIFMINSIZE	2

#define	ASCII		1
#define	BINARY		-2
#define	RAW_TYPE	-3
#define BIN		-23951
#define	PURGED		0
#define	MAXFILESIZE	-1

#define FALSE		0
#define TRUE		1
#define ERROR		2
#define OK		3

#define DATASP		1
#define RECORDSP 	2

#define NOTLIF		20
#define BADLIFNAME	21
#define BADLIFTYPE	22
#define LIFEXISTS	23
#define NOSPACE		24
#undef ISOPEN
#define ISOPEN		25
#define FILENOTFOUND	26
#define NOTOPEN		27
#define ISEOF		28
#define BIGDIRECTORY	29
#define IOERROR		30
#define	VOLSIZERR	31

#define LASTVOL		0100000
#define VOLNUMBER	0067777
#define LIFID		32768

#ifdef _KERNEL
typedef struct lvol *		lifaddr_t;
typedef struct dentry *		diraddr_t;

#define LIFSIZE		8192
#define LIFSECTORSIZE	256
#define LIFSWAPTYPE	21059
#endif /* _KERNEL */

#endif /* _SYS_LIFDEF_INCLUDED */
