/*
nftw - file tree walk routine.

@(#) $Revision: 70.7 $
   
Algorithm:
Verify the depth parameter is in the range 1 to OPEN_MAX and set the depth
   variable equal to it.  If it is unreasonable then return(-1), set errno
   to EINVAL.

Save the starting working directory for use in creating global path names.

disc_set=FALSE


if FTW_PHYS
   stat_fn=lstat
else
   stat_fn=stat


if FTW_CDF
   malloc path_name+ buffer

return ( recursive call( path, fn, depth, flags, level=0, ...


**********************
Start of recursive part:  ( path, fn, depth, flags, level )


if FTW_CDF
   stat_fn the file pointed to by path+
   if the file is a cdf 
      make path+ the path name
   else
      stat_fn the file pointed to by path
else         
   stat_fn the file pointed to by path

save the stat errno

if the stat failed because the file is a cdf with no valid context 
ie: if not FTW_CDF and stat failed 
      if stat ( path+,,  succeeds 
         return (0)

set FTW.base and FTW.level

if the stat failed then
   if FTW_SERR specified 
      return (fn( path, stat undefined, FTW_NS, FTW )
   else
      if errno=EACCES then
         fn( path, stat undefined, FTW_NS, FTW )
         errno=EACCES
         return(-1)
      else
         Set errno as appropriate based on stat errno value.
         return(-1)


if FTW_MOUNT is set then            
   if not disc_set then
      global_device = device this file is on
      disc_set = TRUE
   else 
      if this file is on a different device then
         return(0)


if path points to a symbolic link then       (can only happen on FTW_PHYS)
   return  ( fn(path, stat, FTW_SL, FTW) )


if path points to a directory then

   if this dir has been visited before
      return(0)
   else
      add it's inode/device number to the list of visited inodes

   if FTW_CHDIR then
      chdir to path 

   if the directory does not have read permission then
      return (fn(path, stat, FTW_DNR, FTW))
   
   open the directory using the global path name

   if FTW_DEPTH is not set then
      ret = fn( path, stat, FTW_D, FTW )
      if ret != 0 then
         close the dir 
         return (ret)

   for each entry in the directory
      read the entry

      if entry = . or .. then
         continue

      if depth <= 1 then 
         save the location in the directory 
         close the directory

      ret = recursive call
      if ret != 0 then
         close the directory if it is open
         return ( ret )

      if depth <= 1 then
         open the directory using the global path name
         seek to the previous position

   close the directory

   if FTW_DEPTH is set then
      return (fn( path, stat, FTW_DP, FTW ))
   else
      return(0)


(At this point it is know path doesn't point to a symlink or directory.)
return (fn( path, stat, FTW_F, FTW ))


NOTE: All recursive calls will reduce the depth parameter by 1.
      All recursive calls will increase the level parameter by 1.

************************************************************************/

#ifdef _NAMESPACE_CLEAN
#  ifdef _ANSIC_CLEAN
#	define	free	_free
#	define	malloc _malloc
#  endif /* _ANSIC_CLEAN */
#	define	strrchr	_strrchr
#	define	sysconf _sysconf
#	define	stat	_stat
#	define	lstat	_lstat
#	define	getcwd	_getcwd
#	define	strlen	_strlen
#	define	strcat	_strcat
#	define	strcmp	_strcmp
#	define	strcpy	_strcpy
#	define	access	_access
#	define	chdir	_chdir
#	define	opendir	_opendir
#	define	closedir	_closedir
#	define	readdir	_readdir
#	define	telldir	_telldir
#	define	seekdir	_seekdir
#	define	nftw	_nftw
#	define	nftwh	_nftwh
#endif	/* _NAMESPACE_CLEAN */



#include <unistd.h>
#include <errno.h>
#include <ftw.h> 
#include <sys/types.h>
#include <sys/stat.h>

#include <limits.h>
#include <malloc.h>
#include <string.h>
#include <dirent.h>


#ifdef _NAMESPACE_CLEAN
#	undef	nftw
#	undef	nftwh
#	pragma	_HP_SECONDARY_DEF	_nftw	nftw
#	pragma	_HP_SECONDARY_DEF	_nftwh	nftwh
#	define	nftw	_nftw
#	define	nftwh	_nftwh
#endif	/* _NAMESPACE_CLEAN */


/* Global variables */
#define TRUE 1
#define FALSE 0

struct inode_leaf {
	ino_t	inode;
	struct inode_leaf *left;
	struct inode_leaf *right;
	};

struct dev_struct {
	dev_t	device;
	struct dev_struct *next;
	struct inode_leaf *ihead;
	};

static struct dev_struct *dev_root;	/* root of device/inode tree */
static dev_t global_device;			/* device where path resides */
static int disc_set;				/* boolean, 1=disc set, 0=disc not set */

static char *start_cwd; 		/* absolute cwd of starting point */
static int (*stat_fn)();		/* pointer to the stat function */

/**************************************************************
This function removes from memory the inode storage subtree from 
a single device list entry.  The input is the root of the inode 
tree.
This is a recursive function.
*/

static void 
free_itree ( iroot )

struct inode_leaf *iroot;
{
if ( iroot == NULL )
	return;
else {
	free_itree ( iroot->left );
	free_itree ( iroot->right );
	free ( iroot );
	}
}
	
/**************************************************************
This function removes the entire device/inode data storage tree
from memory.
*/

static void 
free_dev_list()
{
struct dev_struct *dev_ptr;

while ( dev_root != NULL ) {
	free_itree ( dev_root->ihead );
	dev_ptr = dev_root;
	dev_root = dev_root->next;
	free ( dev_ptr );
	}
}
		
/**************************************************************
This function creates and initializes a new leaf on the inode
storage tree.  A pointer to the new leaf is returned.
*/

static struct inode_leaf *
create_new_inode ( inode )

ino_t inode;
{
struct inode_leaf *ptr;

ptr = (struct inode_leaf *) malloc ( sizeof ( struct inode_leaf ) );
if ( ptr == NULL )
	return ( NULL );

ptr->inode = inode;
ptr->right = NULL;
ptr->left = NULL;

return ( ptr );
}

/**************************************************************************
This function searches for the inode/device in the inode/device data
structure.  This data structure is a linked list of devices.  Each
device contains a pointer to the head of a binary tree of inodes visited
so far on that device.

Return value:
If the inode/device is in the list a 1 is returned.
If the inode/device is not in the list a 0 is returned and it is added
	to the list.
if an error is encountered a -1 is returned.
*/


static int 
visited_before ( device, inode )

dev_t	device;
ino_t	inode;
{

struct inode_leaf *itree;		/* inode tree */
struct dev_struct *dev_ptr;		/* device entry pointer */

dev_ptr = dev_root;

while ( dev_ptr != NULL )
	if ( dev_ptr->device == device )
		break;
	else
		dev_ptr = dev_ptr->next;

if ( dev_ptr == NULL ) 	{ 		/* add new device record */
	dev_ptr = ( struct dev_struct *) malloc ( sizeof ( struct dev_struct ) );
	if ( dev_ptr == NULL )
		return ( -1 );

	dev_ptr->device = device;
	dev_ptr->ihead = NULL;

	dev_ptr->next = dev_root;		/* add new node to front of list */
	dev_root = dev_ptr; 
	}

/* A valid device record exists at this point */
itree = dev_ptr->ihead;

while ( itree != NULL )
	if ( itree->inode == inode )
		return ( 1 );
	else
		if ( inode < itree->inode )
			if ( itree->left == NULL ) {
				itree->left = create_new_inode ( inode );
				if ( itree->left == NULL )
					return ( -1 );
				return ( 0 );
				}
			else
				itree = itree->left;
		else
			if ( itree->right == NULL ) {
				itree->right = create_new_inode ( inode );
				if ( itree->right == NULL )
					return ( -1 );
				return ( 0 );
				}
			else
				itree = itree->right;
		
/* The next line should only be executed when the itree is initially empty */
dev_ptr->ihead = create_new_inode ( inode );
if ( dev_ptr->ihead == NULL )
	return ( -1 );
return ( 0 );
}

/*********************************************************
set_ftw  This function sets the values in the ftw structure.

Algorithm:
find the last "/" in the path
if path == "/" or no "/" found
   return ftw.base=0
else
   if the last "/" is the last char. in the path then
      walk back through the string till next "/" or the start is reached
      if "/" found then
         set ftw.base = offset of "/" + 1
      else
         set ftw.base = 0 
   else
      ftw.base= last "/" + 1
*/

static void 
set_ftw ( path, level, ftw )

char *path;
int level;
struct FTW *ftw;
{
char *ptr;

ftw->level = level;

ptr = strrchr ( path, '/' );
if ( ptr == NULL || strcmp ( path, "/" ) == 0 )
	ftw->base = 0;
else {
	if ( *(ptr + 1) == '\0' ){     /* if trailing / */
		while ( ptr != path && *ptr == '/' )      /* skip all trailing / */
			ptr--;

		while ( ptr != path && *ptr != '/' )      /* look for previous /   */
			ptr--;

		if ( *ptr == '/' )       /* if previous / found */
			ftw->base = (int) ( ptr - path + 1 );
		else
			ftw->base = 0;
		}
	else
		ftw->base = (int) ( ptr - path ) + 1;
	}
}
/*********************************************************
Main nftw routine. 

*/
int 
nftw ( Xpath, fn, depth, flags )

char *Xpath;
int (*fn)();
int depth;
int flags;
{
int ret;
char path[ MAXPATHLEN + 1 ];

if ( strlen ( Xpath ) > MAXPATHLEN ) {
	errno = ENAMETOOLONG;
	return ( -1 );
	}
else
	strcpy ( path, Xpath );		/* put the path into a large array */

dev_root = NULL;			/* root of device/inode tree */
disc_set = FALSE;			/* boolean, TRUE=disc device set, FALSE=not set */

if ( depth < 1 ) 
	depth = 1;
else if ( depth > sysconf( _SC_OPEN_MAX ) ) {
	errno = EINVAL;
	return ( -1 );
	}


if ( flags & FTW_PHYS )		/* select the proper stat function */
	stat_fn = lstat;
else
	stat_fn = stat;

start_cwd = ( char *) malloc ( MAXPATHLEN + 1 );
if ( start_cwd == NULL )	
		return ( -1 );

if ( path [ 0 ] == '/' )		/* absolute path input */
	start_cwd [ 0 ] = '\0';
else {
	(void) getcwd ( start_cwd, MAXPATHLEN + 1 );
	if ( start_cwd [ 0 ] == '\0' )
		return ( -1 );
	if ( start_cwd [ strlen ( start_cwd ) - 1 ] != '/' ) /*if cwd ends with / */
		if ( strlen ( start_cwd ) < MAXPATHLEN )
			strcat ( start_cwd, "/" );
		else {
			errno = ENAMETOOLONG;
			return ( -1 );
			}
	}

ret = do_file_walk ( path, fn, depth, flags, 0 );


/* free allocated memory */
free_dev_list();
free ( start_cwd );


return ( ret );

}
/************************************************************
This function actually does the walk of the file tree.

*/
static int do_file_walk ( path, fn, depth, flags, level )
char *path;			/* path of file to start search */
int (*fn)();		/* function to call for each file */
int depth;			/* number of file descriptors this routine can open */
int flags;			/* controlling flags */
int level;			/* depth in search tree so far */
{
int ret;
int stat_ret;				/* return value from stat call */
struct stat stat_buf;	/* stat buffer */
struct FTW ftw;
int save_errno;

struct dirent *dir_entry;		/* ptr to a directory entry */
DIR *dir_file;						/* ptr to the directory file */
long dir_ptr;						/* ptr to the last position in the dir file*/
char full_dir_entry [ MAXPATHLEN + 1 ];	/* the full directory entry */
char dir_path [ MAXPATHLEN + 1 ];	/* absolute path to current directory */


strcpy ( dir_path, start_cwd );
strcat ( dir_path, path );

if ( flags & FTW_CDF ) {
	if ( strlen ( dir_path ) < MAXPATHLEN )
		strcat ( dir_path, "+" );
	else {
		errno = ENAMETOOLONG;
		return ( -1 );
		}

	stat_ret = (*stat_fn) ( dir_path, &stat_buf );
	if ( stat_ret == 0 ) 
		if ( strlen ( path ) < MAXPATHLEN )
			strcat ( path, "+" );
		else {
			errno = ENAMETOOLONG;
			return ( -1 );
			}
	else {
		*( strrchr ( dir_path, '+' ) ) = '\0';	/* remove + from end of name */
		stat_ret = (*stat_fn) ( dir_path, &stat_buf );
		}
	}
else
	stat_ret = (*stat_fn) ( dir_path, &stat_buf );
save_errno = errno;


if ( ( stat_ret != 0 )  && ( ! ( flags & FTW_CDF ) ) ) {
	if ( strlen ( dir_path ) < MAXPATHLEN )
		strcat ( dir_path, "+" );
	else { 
		errno = ENAMETOOLONG;
		return ( -1 );
		}

	stat_ret = stat ( dir_path, &stat_buf );
	if ( stat_ret == 0 ) 
		return ( 0 );		/* the file is a cdf with no valid context */
	else 
		*( strrchr ( dir_path, '+' ) ) = '\0';    /* remove trailing + */
	
}


set_ftw ( path, level, &ftw );


if ( stat_ret == -1 ) 
	if ( flags & FTW_SERR ) 
		return (fn ( path, &stat_buf, FTW_NS, ftw ) );
	else {
		if ( save_errno == EACCES ) {
			fn ( path, &stat_buf, FTW_NS, ftw );
			errno = save_errno;  	
			return ( -1 );
			}	
		else {
			errno = save_errno;
			return ( -1 );
			}
		}
		

if ( flags & FTW_MOUNT ) 
	if ( disc_set == FALSE ) {
		global_device = stat_buf.st_dev;
		disc_set = TRUE;
		}
	else 
		if ( global_device != stat_buf.st_dev )
			return ( 0 );		/* skip if on different device */



if ( ( stat_buf.st_mode & S_IFMT ) == S_IFLNK )
	return ( fn ( path, &stat_buf, FTW_SL, ftw ) );



if ( ( stat_buf.st_mode & S_IFMT ) == S_IFDIR ) { 

	switch ( visited_before ( stat_buf.st_dev, stat_buf.st_ino ) ) {
		/* detect loops */
		case -1: return ( -1 );      /* error found */
		case 0:	break;               /* node has NOT been visited before */
		case 1:	return ( 0 );        /* node has been visited before */
		}

	if ( flags & FTW_CHDIR )
		if ( chdir ( dir_path ) != 0 )
			return ( -1 );

	if ( access ( dir_path, R_OK ) == -1 )
		return ( fn ( path, &stat_buf, FTW_DNR, ftw ) );


	dir_file = opendir ( dir_path );
	if ( dir_file == NULL )
		return ( -1 );

	if ( ! ( flags & FTW_DEPTH ) ) {
		ret = fn ( path, &stat_buf, FTW_D, ftw );
		if ( ret != 0 ) {
			closedir ( dir_file );
			return ( ret );
			}
		}


	while ( (dir_entry = readdir ( dir_file ) ) != NULL ) {

		if ( ( strcmp ( dir_entry->d_name, "." ) == 0 ) ||  
			  ( strcmp ( dir_entry->d_name, ".." ) == 0 ) )
			continue;

		if ( depth <= 1 ) {
			if ( ( dir_ptr = telldir ( dir_file ) ) == -1 )
				return ( -1 );
			if ( closedir ( dir_file ) != 0 )
				return ( -1 );
			}


		if ( strlen ( path ) < MAXPATHLEN )
			strcpy ( full_dir_entry, path );   /* this is a relative path */
		else { 
			errno = ENAMETOOLONG;
			return ( -1 );
			}
		if (  *(full_dir_entry + strlen ( full_dir_entry ) - 1 )  != '/' )
			strcat ( full_dir_entry, "/" );

		if ((strlen(full_dir_entry) + strlen(dir_entry->d_name)) <= MAXPATHLEN)
			strcat ( full_dir_entry, dir_entry->d_name );
		else {
			errno = ENAMETOOLONG;
			return ( -1 );
			}

		ret = do_file_walk ( full_dir_entry, fn, depth - 1, flags,
									level + 1 );
	
		if ( ret != 0 ) {
			if ( depth > 1 )
				closedir ( dir_file );
			return ( ret );
			}

		if ( depth <= 1 ) {
			dir_file = opendir ( dir_path );
			if ( dir_file ==  NULL )
				return ( -1 );
			seekdir ( dir_file, dir_ptr );
			}
		} /* while */

	closedir ( dir_file );

	if ( flags & FTW_DEPTH )
		return ( fn ( path, &stat_buf, FTW_DP, ftw ) );
	else
		return ( 0 );
}


/* file in not a dir or symbolic link */
return ( fn ( path, &stat_buf, FTW_F, ftw ) );

}

/*****************************************************************
nftwh merely calls nftw with the FTW_CDF flag set 
*/

int 
nftwh ( path, fn, depth, flags )

char *path;
int (*fn)();
int depth;
int flags;
{
return ( nftw ( path, fn, depth, flags | FTW_CDF ) );
}
