/*
 * @(#)nfs_log.h: $Revision: 1.5.83.4 $ $Date: 93/09/17 19:07:57 $
 * $Locker:  $
 *
 */

/*
 * (c) Copyright 1987 Hewlett-Packard Company
 */

/*
 * Header file to be used to interface the printf's in the RPC and NFS
 * kernel code to whatever logging mechanism we will use
 */

/* h/nfs_log.h   Hewlett_Packard    */

#include "../h/subsys_id.h"
#include "../h/net_diag.h"
#include "../h/netdiag1.h"

#define  SP_BUF_SIZE   100

/* This should be removed whenever the 800 people want to go */
/* through NS event logging.                                 */
/* #ifdef  hp9000s800 */
#define  USING_printf_LOG  1 

/*
 * It is not clear in the kernel version of sprintf how much you 
 * writing into a string, so I will add an extra byte for safety.
 * sprintf_buf will be defined in nfs_subr.c.  As long as there is
 * not a chance of the process sleeping once it has begun the logging
 * it should be OK to share the memory among the routines of RPC and NFS.
 */

extern   char  sprintf_buf[SP_BUF_SIZE+1];

/*
 * These macros will be used by the kernel code to get their messages
 * out through a common interface.  For the beginning the macros will
 * continue to use printf's, but this will be changed as a better
 * logging mechanism is available.
 *
 * NFS_LOG is used to log a single string
 * NFS_LOGn  these are routines that are used to print formatted input
 *           such as an integer using for example %d inside a string.
 *           The kernel's sprintf will be used to construct the sting
 *           from the pieces before it is logged.  The number after
 *           NFS_LOG refers to the number of pieces of information
 *           that can be written into the output string.
 *
 *           Example:
 *           NFS_LOG2("Test string for host %s and number %d\n", host, num);
 */

#ifdef USING_printf_LOG

#define  NFS_LOG(str)	 printf(str); 

#define  NFS_LOG1(format_str, str1)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1);  \
      printf(sprintf_buf);  }

#define  NFS_LOG2(format_str, str1, str2)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2);  \
      printf(sprintf_buf);  }

#define  NFS_LOG3(format_str, str1, str2, str3)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2, str3);  \
      printf(sprintf_buf); }

/*
 * This is a special define that is used for printing info that is not
 * of an error status.  It is here for printing the warning message that
 * a NFS call got interrupted.  In the case of printf's, I will just 
 * not print anything.
 */
#define  NFS_LOG3_WARN(format_str, str1, str2, str3)

#define  NFS_LOG4(format_str, str1, str2, str3, str4)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2, str3, str4); \
      printf(sprintf_buf); }

#else

#define  NFS_LOG(str)		\
	{ NS_LOG_EVENT_STR(LE_NFS_ERR, NS_LC_ERROR, NS_LS_NFS, 0,0,0, str); }

#define  NFS_LOG1(format_str, str1)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1); \
      NS_LOG_EVENT_STR(LE_NFS_ERR, NS_LC_ERROR, NS_LS_NFS, 0,0,0, sprintf_buf);\
    }

#define  NFS_LOG2(format_str, str1, str2)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2); \
      NS_LOG_EVENT_STR(LE_NFS_ERR, NS_LC_ERROR, NS_LS_NFS, 0,0,0, sprintf_buf);\
    }

#define  NFS_LOG3(format_str, str1, str2, str3)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2, str3); \
      NS_LOG_EVENT_STR(LE_NFS_ERR, NS_LC_ERROR, NS_LS_NFS, 0,0,0, sprintf_buf);\
    }

#define  NFS_LOG3_WARN(format_str, str1, str2, str3)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2, str3); \
      NS_LOG_EVENT_STR(LE_NFS_ERR, NS_LC_WARNING, NS_LS_NFS, 0,0,0, sprintf_buf);\
    }

#define  NFS_LOG4(format_str, str1, str2, str3, str4)	\
    { sprintf(sprintf_buf, SP_BUF_SIZE, format_str, str1, str2, str3, str4); \
      NS_LOG_EVENT_STR(LE_NFS_ERR, NS_LC_ERROR, NS_LS_NFS, 0,0,0, sprintf_buf);\
    }

#endif /* USING_printf_LOG */
