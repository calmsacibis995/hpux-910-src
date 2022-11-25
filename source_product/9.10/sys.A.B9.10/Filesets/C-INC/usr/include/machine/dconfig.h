/*
 * @(#)dconfig.h: $Revision: 1.2.83.6 $ $Date: 93/11/02 14:56:01 $
 * $Locker:  $
 */

#ifndef _DEVCONFIG_INCLUDED /* allows multiple inclusion */
#define _DEVCONFIG_INCLUDED

/*
  Definitions for the devconfig pseudo-driver are found here
*/

#ifndef _KERNEL_BUILD
#include <sys/ioctl.h>
#endif /* ! _KERNEL_BUILD */

#define DEVCONFIG_FILE	"/dev/config"
#define TRUE		1
#define FALSE		0
#define NONE		-1

/* typedefs for ioctl structures passed to/from devconfig */

#ifdef __hp9000s700

typedef struct {
	u_int		which_table;
	size_t		size;
	int		status;		/* also num_elements */
} config_get_table_t;

#endif  /* __hp9000s700 */

typedef struct {
	u_int		search_type;
	u_int		search_key;
	u_int		search_qual;
	isc_entry_type	isc_entry;
	int		status;
} config_search_isc_t;

#ifdef _KERNEL

/* Defines for config_search_isc */

/* AUTOBAHN: restore this after testing is complete */
/* #define SEARCH_ENTRIES_PER_ENTRY 30 */
#define SEARCH_ENTRIES_PER_ENTRY 3

typedef struct config_SEARCH_LIST_t {
	struct 	isc_table_type	*entry[SEARCH_ENTRIES_PER_ENTRY];
	short		first_entry;
	short		entry_count;
	struct config_SEARCH_LIST_t *next;
} config_search_list_t;

#define NULL_SEARCH_LIST_PTR	((config_search_list_t *)0)

#endif /* _KERNEL */

/* devconfig ioctl command defines */

#ifdef __hp9000s700
#define CONFIG_GET_TABLE	    _IOWR('c', 0, config_get_table_t)
#endif  /* __hp9000s700 */

#define CONFIG_SEARCH_ISC	    _IOWR('c', 1, config_search_isc_t)

/*
 * This is an undocumented ioctl used by ioscan to test if a character
 * device driver is present in the kernel.
 */
typedef struct cfg_drv_stat{
	int drv_major;
	int drv_status;
} config_drv_status;

#define CONFIG_DRIVER_STATUS	    _IOWR('c', 2, config_drv_status)

struct hpib_ident_type {
    int ident;
    int unit1_present;
    char class[12];
    char driver[12];
};
struct ioctl_ident_hpib {
    int sc;
    struct hpib_ident_type dev_info[8];
};

struct scsi_ident_type {
    int dev_info;
    int dev_type;
    char dev_desc[28];
};

struct ioctl_ident_scsi {
    int sc;
    struct scsi_ident_type ident_type[8];
};

struct driver_table_type {
    char device_drivers_present[150];
    char intf_drivers_present[MAXISC];
};

#define CONFIG_SCSI_IDENTIFY	_IOWR('c', 3, struct ioctl_ident_scsi)
#define CONFIG_HPIB_IDENTIFY	_IOWR('c', 4, struct ioctl_ident_hpib)
#define CONFIG_DRIVERS_PRESENT	_IOWR('c', 5, struct driver_table_type)

#endif /* not _DEVCONFIG_INCLUDED */
