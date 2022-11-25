/*
 * @(#)assert.h: $Revision: 1.4.83.3 $ $Date: 93/09/17 17:01:52 $
 * $Locker:  $
 */

#ifndef _ASSERT_INCLUDED
#define	_ASSERT_INCLUDED

#ifdef	MPDEBUG

#define MP_ASSERT(invariant, message) 		\
	if (!uniprocessor && !(invariant)) {	\
		assfail(message, __FILE__, __LINE__); \
	}

#define UP_ASSERT(invariant, message) 		\
	if (uniprocessor && !(invariant)) {	\
		assfail(message, __FILE__, __LINE__); \
	}

#define DO_ASSERT(invariant, message) 		\
	if (!(invariant)) {	\
		assfail(message, __FILE__, __LINE__); \
	}

#else	/* ! MPDEBUG */

#define	MP_ASSERT(invariant, message)
#define	UP_ASSERT(invariant, message)
#define	DO_ASSERT(invariant, message)

#endif	/* ! MPDEBUG */

#ifdef	SEMAPHORE_DEBUG
#define SD_ASSERT(invariant, message) 		\
	if (!(invariant)) {	\
		assfail(message, __FILE__, __LINE__); \
	}
#else	/* ! SEMAPHORE_DEBUG */
#define SD_ASSERT(invariant, message)
#endif	/* ! SEMAPHORE_DEBUG */

#endif	/* ! _ASSERT_INCLUDED */
