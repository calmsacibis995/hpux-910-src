/* HPUX_ID: %W%     %E%  */

#include <sys/ioctl.h>

/* max ram volumes cannot exceed 16 */
#define RAM_MAXVOLS 16

/* ioctl to deallocate ram volume */
#define RAM_DEALLOCATE	_IOW('R', 1, int)

/* ioctl to reset the access counter to ram volume */
#define RAM_RESETCOUNTS	_IOW('R', 2, int)

/* io mapping minor number macros */
/* up to 1048575 - 256 byte sectors */
#define	RAM_SIZE(x)	((x) & 0xfffff) 	/* XXX */

/* up 16 disc allowed */
#define	RAM_DISC(x)	(((x) >> 20) & 0xf) 	/* XXX */
#define	RAM_MINOR(x)	((x) & 0xffffff) 	/* XXX */

#define LOG2SECSIZE 8	/* (256 bytes) "sector" size (log2) of the ram discs */

#define RAM_RETURN 1

struct ram_descriptor {
	char	*addr;
	int	size;
	short	opencount;
	short	flag;
	int	rd1k;
	int	rd2k;
	int	rd3k;
	int	rd4k;
	int	rd5k;
	int	rd6k;
	int	rd7k;
	int	rd8k;
	int	rdother;
	int	wt1k;
	int	wt2k;
	int	wt3k;
	int	wt4k;
	int	wt5k;
	int	wt6k;
	int	wt7k;
	int	wt8k;
	int	wtother;
} ram_device[RAM_MAXVOLS];
