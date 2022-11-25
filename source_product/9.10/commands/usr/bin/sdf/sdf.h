/* @(#) $Revision: 37.1 $ */      
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                  sdf.h                                *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define	SDFMAX		20
#define	DEVFDMAX	10
#define	CACHEMAX	30
#define	FNAMEMAX	50

struct sdfinfo {			/* ----------------------------- */
	int fd;				/* native file descriptor	 */
	int opened;			/* used/unused flag		 */
	int offset;			/* logical offset in file	 */
	int diraddr;			/* physical addr of dir entry    */
	int inumber;			/* inode number of SDF file	 */
	int pinumber;			/* inode of parent directory     */
	int validi;			/* first FIR after free map	 */
	int written;			/* has device been written?	 */
	dev_t dev;			/* dev_t entry for SDF device	 */
	struct dev_fd *dp;		/* dev_fd entry for this dev     */
	struct filsys filsys;		/* copy of SDF super block	 */
	struct dinode inode;		/* file'e inode / spare buffer	 */
	/* char devnm[FNAMEMAX];		character string for device   */
};					/* ----------------------------- */

struct cache {				/* -------------------- */
	int valid;			/* is entry valid? 	*/
	int fd;				/* native fd of device	*/
	int bnum;			/* block number of data	*/
	long atime;			/* cache access time	*/
	char *block;			/* ptr to the cache buf	*/
};					/* -------------------- */

struct dev_fd {				/* -------------------- */
	dev_t dev;			/* dev_t entry for dev  */
	char devnm[FNAMEMAX];		/* name of device	*/
	int fd;				/* native fd of device	*/
	int count;			/* reference count	*/
	int bfree;			/* blocks free          */
	struct dinode fainode;		/* file system header   */
};					/* -------------------- */

#define	READ		0		/* used to denote "no-write"	*/
#define	WRITE		1		/* used to denote "write"	*/
#define	SDF_OFFSET	1000		/* differentiate native&sdf fds */
