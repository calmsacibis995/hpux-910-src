/*
 * @(#)ioconfig.h: $Revision: 1.2.83.4 $ $Date: 93/09/17 18:27:21 $
 * $Locker:  $
 */

#ifndef _IOCONFIG_INCLUDED /* allows multiple inclusion */
#define _IOCONFIG_INCLUDED

/*
  Definitions for /etc/ioconfig are found here
*/

#define IOCONFIG_MAGIC	    0x2122494f
#define IOCONFIG_FILE	    "/etc/ioconfig"
#define IOCONFIG_TEMPFILE   "/etc/ioconfigXXXXXX"
#define IOCONFIG_MODE	    (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH) /* 0664 */
#define IOCONFIG_OWNER	    0   /* root */
#define IOCONFIG_GROUP	    3   /* sys */

/* Defines for ioconfig_open "flag" argument */

#define IOCONFIG_READ		(O_RDONLY)
#define IOCONFIG_CREATE		(O_WRONLY|O_CREAT|O_EXCL)
#define IOCONFIG_OVERWRITE	(O_WRONLY|O_CREAT)

/* Defines for the ioconfig_close "action" argument */

#define IOCONFIG_PRESERVE   0   /* preserve original /etc/ioconfig */
#define IOCONFIG_UPDATE	    1	/* update /etc/ioconfig from the temp file */

/* MAX_ID, NONE, and io_name_type are also defined in iotree.h */

#ifndef MAX_ID
#define MAX_ID 16
#endif

#ifndef NONE
#define NONE -1
#endif

#ifndef _IO_NAME_TYPE
#define _IO_NAME_TYPE
typedef char io_name_type[MAX_ID];
#endif

typedef struct {
	int		parent;		/* parent in io_tree */
	int		sibling;	/* sibling in io_tree */
	int		child;		/* child in io_tree */
	io_name_type	manager;	/* manager name */
	io_name_type	module;		/* module name */
	io_name_type	class;		/* device class */
	int		lu;		/* logical unit number */
	int		hdw_address;	/* hardware address */
} ioconfig_entry;

#endif /* not _IOCONFIG_INCLUDED */
