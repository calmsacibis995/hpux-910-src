/* $Header: bin.h,v 38.1 87/02/20 17:28:14 jvm Exp $ */

/*
 * Command pathnames
 *
 * Author: Bob Boothby
 */
#define CO              "/usr/bin/co"
#define MERGE           "/usr/bin/merge"
#define RDIFF           "/usr/lib/rdiff"
#define RDIFF3          "/usr/lib/rdiff3"
#define SNOOP           "/usr/lib/snoop"
#define SNOOPFILE	"/usr/adm/rcslog"
#ifdef hp9000ipc
#define DIFF		"/usr/bin/diff"
#define DIFFH		"/usr/bin/diffh"
#define PR		"/usr/bin/pr"
#else
#define DIFF		"/bin/diff"
#define DIFFH		"/usr/lib/diffh"
#define PR		"/bin/pr"
#endif
