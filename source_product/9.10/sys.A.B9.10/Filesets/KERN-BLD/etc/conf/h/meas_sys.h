/*
 * @(#)meas_sys.h: $Revision: 4.9.83.4 $ $Date: 93/09/17 18:29:29 $
 * $Locker:  $
 * 
 */

#ifndef _SYS_MEAS_SYS_INCLUDED
#define _SYS_MEAS_SYS_INCLUDED

/*
 * for defining the type of the record written to ms_buf 
 */
#define MS_GRAB_ID	0
#define MS_PUT_TIME	1
#define MS_COMMENT	2
#define MS_DATA		3
#define MS_ERROR	4
#define MS_GRAB_BIN_ID	5
#define MS_POST_BIN	6
#define MS_DATA_V   	7

/*
 * types of records written only by iscan 
 */
#define MS_OVERRUN_ERR	100
#define MS_LOST_DATA	101
#define MS_LOST_BUFFER	102
#define MS_ADMIN_INFO	103

/* 
 * Used in bin_counter calls
 * Legal id's for bins are allocated in the range
 * 	MS_MIN_BIN_ID..MS_MAX_BIN_ID 
 * start at 8192 to keep ranges from overlapping 
 */
#define MS_MIN_BIN_ID		8192
#define MS_NUMBER_BIN_IDS	200
#define MS_MAX_BIN_ID		(MS_MIN_BIN_ID + MS_NUMBER_BIN_IDS -1)
#define MS_BIN_SLOTS		64

/*
 * Used in vanilla calls 
 */
#define MS_MIN_NORMAL_ID	1
#define MS_NUMBER_NORMAL_IDS	256
#define MS_MAX_NORMAL_ID	(MS_MIN_NORMAL_ID + MS_NUMBER_NORMAL_IDS -1)


/*
 * Limits string sizes in routines which check length with ms_paranoid_strlen.
 * Also used in copying in strings from user address space; used to allocate
 * buffer on kernel stack for this purpose. It is dangerous to make it to big.
 */
#define MS_MAX_STR_LEN	256


/*
 * Rev change to 4 since old iscans will no longer be able to find the data
 */
#define MS_REV			4

#define MS_BUF_MIN_SHIFT	5

/* char ms_include_rev[] = "$Revision: 4.9.83.4 $"; */
#define MS_INCLUDE_REV "$Revision: 4.9.83.4 $"

/*
 * structure to maintain bin_count data 
 */
struct ms_bin_struct {
	int shift;
	int base;
	int underflow;
	int overflow;
	int bins[MS_BIN_SLOTS];
};

/*
 *	Shift is the base 2 logarithm of the bin width (which is how the
 *	width is stored internally).
 */

/*
 * Structure passed back in DCT_START call 
 *
 * Each entry is the address of the variable of the same name.
 */

#endif /* not  _SYS_MEAS_SYS_INCLUDED */
