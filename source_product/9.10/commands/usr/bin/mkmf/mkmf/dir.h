/* $Header: dir.h,v 66.1 90/01/15 13:18:17 pfm Exp $ */

/*
 * Common directory header file
 *
 * Author: Peter J. Nicklin
 */
#ifndef DIR_H
#define DIR_H

#include "system.h"

#ifdef BSD4
#  include <sys/types.h>
#  include <sys/dir.h>
   typedef struct direct DIRENT;
#endif

#ifdef SYSV
#  include <sys/types.h>
#  include <dirent.h>
   typedef struct dirent DIRENT;
#endif

#ifdef XENIX
#  include <sys/types.h>
#  include <sys/ndir.h>
   typedef struct direct DIRENT;
#endif

#ifndef MAXNAMLEN
#  define MAXNAMLEN 14
#endif

#endif /* DIR_H */
