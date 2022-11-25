#ifndef SYS532
#endif

/************************************************************************/
/************************************************************************/
/************************************************************************/
#ifdef SYS532
#define SYS5_STREAM_PTYS		1
#define USE_PUTMSG_FOR_IOCTL		1
#define USE_WINSIG_FOR_SIGNALS		1
#define USE_PTY_FOR_CONTROL		1
#define PUSH_PTEM_LDTERM_ON_OPEN	1
#define SET_TERMIO_ON_SLAVE		1
#define WAIT_FOR_CLOSE_AFTER_KILL	1
#define USE_PTY_NAME_ONLY_ON_PS		1
#endif
/************************************************************************/
/************************************************************************/
				/* has ioctls through pty */
				/* has signals to master pty */
#define USE_PIPE_FOR_CONTROL		1
#define USE_SELECT			1
				/* sets termio from master side */
#define USE_PTY_NAME_ONLY_ON_PS		1

/************************************************************************/
/************************************************************************/
#ifdef DG431
#define NO_FILES_IN_DEV			1
#endif
/************************************************************************/
				/* DG431  AIXRS6000 AIXPS2 SUN41 */

