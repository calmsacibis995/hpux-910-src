/* @(#) $Revision: 72.1 $ */       

/*
 * This file defines certain local parameters
 * A symbol should be defined in Makefile for local conditional
 * compilation, e.g. IIASA or ERNIE, to be tested here and elsewhere.
 */

/*
 * Fundamental definitions which may vary from system to system.
 *
 *	BUFSIZ		The i/o buffering size; also limits word size
 *	SHELLPATH	Where the shell will live; initalizes $shell
 *	MAILINTVL	How often to mailcheck; more often is more expensive
 *	HZ		Cycle of ac power
 *	OTHERSH		Shell for scripts which don't start with #
 */

#define	BUFSIZ	512		/* default buffer size */

#ifdef V4FS
#	define SHELLPATH	"/usr/bin/csh"
#	define	OTHERSH		"/usr/bin/sh"
#else
#	define SHELLPATH	"/bin/csh"
#	define	OTHERSH		"/bin/sh"
#endif	/* V4FS */

#ifdef hp9000s800
#define HZ	100		/* for division into seconds */
#endif
#ifdef hp9000s200
#define HZ	50
#endif
#ifdef hp9000s500
#define HZ	60
#endif

#define FORKSLEEP	10	/* delay loop on non-interactive fork failure */
#define	MAILINTVL	600	/* 10 minutes */

/*
 * NCARGS is from <sys/param.h> which we choose not
 * to wholly include
 */
#define	NCARGS	10240		/* Max. chars in an argument list */

/*
 * The shell moves std in/out/diag and the old std input away from units
 * 0, 1, and 2 so that it is easy to set up these standards for invoked
 * commands.  If possible they should go into descriptors closed by exec.
 */

#define	FSHTTY	15		/* /dev/tty when manip pgrps */
#define	FSHIN	16		/* Preferred desc for shell input */
#define	FSHOUT	17		/* ... shell output */
#define	FSHDIAG	18		/* ... shell diagnostics */
#define	FOLDSTD	19		/* ... old std input */

/*#define	V7*/

#ifdef IIASA
#undef	HZ
#define	HZ	60
#undef	OTHERSH
#endif

#ifdef VMUNIX
#include <pagsiz.h>
#undef BUFSIZ
#define	BUFSIZ	BSIZE
#endif
