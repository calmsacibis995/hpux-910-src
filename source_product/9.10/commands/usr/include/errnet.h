/* @(#) $Revision: 64.2 $ */       
#ifndef _ERRNET_INCLUDED /* allows multiple inclusion */
#define _ERRNET_INCLUDED

/* MANX errnet.h 7.1 9/10/84 18:48:38 */
/****************************************************************************/
/**                                                                        **/
/**           /usr/include/errnet.h               #include <errnet.h>      **/
/**                                                                        **/
/****************************************************************************/
/**                                                                        **/
/**   These are all the possible values for the variable errnet.           **/
/**                                                                        **/
/**   'errnet' is reached by saying                                        **/
/**                     extern int errnet;                                 **/
/**                                                                        **/
/**   'errnet' is only defined when errno == ENET.  ENET is defined        **/
/**   in the file #include <errno.h>.                                      **/
/**                                                                        **/
/****************************************************************************/

#include <sys/errnet.h>

#endif /* _ERRNET_INCLUDED */
