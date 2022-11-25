/* HPUX_ID: @(#) $Revision: 72.2 $  */
/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Filewriter.c 
 *	Purpose ............... : extract files from archive and write to file 
 *	Author ................ : David Williams. 
 *	Data Files Accessed ... : 
 *	Trace Flags ........... : 
 *
 *	Description:
 *
 *	
 *
 *	Contents:
 *
 *		filewriter()()
 *
 *-----------------------------------------------------------------------------
 */

#include	"ftio.h"
#include	"define.h"
#include	<errno.h>

static	int	posn_in_buf;
static	int	cur_pkt = 0;

#if defined(DUX) || defined(DISKLESS)
static  int     component_perm[MAXCOMPONENTS];
static  int     component_cnt;
#endif DUX || DISKLESS


filewriter(patterns)
char	**patterns;
{
	struct	stat	stats, old_stats;
	int	usethisfile;

	/*
	 *	Get first packet.
	 */
	get_first_pkt();

	/*
	 *	Get a file then determine what is to be done.
	 */
	while (!getheader(&stats))
	{
		usethisfile = (!Matchpatterns || ckname(Pathname, patterns));

		/*
		 *	Check to see that the file is new enough
		 */
		if (usethisfile && !Dontload && Neweronly)
		{
			/*
			 *	If we can't stat the file, for 
			 *	reasons other than it doesn't exist,
			 *	then we won't restore over it.
			 */
#ifdef SYMLINKS
			if (lstat(Pathname, &old_stats) == -1)
#else
			if (stat(Pathname, &old_stats) == -1)
#endif SYMLINKS
			{
				if (errno != ENOENT)
				{
					ftio_mesg(FM_NSTAT, Pathname);
					ftio_mesg(FM_NREST);
					usethisfile = 0;
				}
			}
			else
			{
				/*
				 *	If the existing file is newer
				 *	than the one on the tape, don't
				 *	restore the tape file.
				 */
				if (old_stats.st_mtime >= stats.st_mtime)
				{
					ftio_mesg(FM_NEW, Pathname);
					usethisfile = 0;
				}
			}
		}

		/*
		 *	Bring up the file..
		 */
		if (usethisfile && !Dontload)
		{
			fileblaster(&stats);

			/*
			 *	List files.
			 */
			if (Listfiles)
			{
				if (Listall)
					printf("** restored: %s\n", Pathname);
				else
					puts(Pathname);
			}
		}
		else
		{
			switch(stats.st_mode & S_IFMT)
			{
			case S_IFREG:
			case S_IFDIR:
			case S_IFNWK:
#ifdef SYMLINKS
			case S_IFLNK:
#endif SYMLINKS
					skipfile((unsigned)stats.st_size);
					break;
			case S_IFCHR:
			case S_IFBLK:
			case S_IFIFO:
					break;
			
			default:
					ftio_mesg(FM_UNSPX, Pathname);
					break;
			}
			
			/*
			 *	List the filename.
			 */
			if (usethisfile)
			{
				if (Listfiles)
				{
					if (Dontload)
						llprint(Pathname, &stats);
					else
						puts(Pathname);
				}
				else    
				{
					if (Dontload)
						puts(Pathname);
				}
			}
			else
			{
				if (Listall)
					puts(Pathname);
			}
		}
	}
	Packets[cur_pkt].status = PKT_FINISH;
	release_packet(INPUT);
#ifdef PKT_DEBUG
printf("ftio: filewriter: PKT_FINISH put in packet %d and released (INPUT=%d).\n", cur_pkt, get_val(INPUT));
fflush(stdout);
#endif PKT_DEBUG
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Fileblaster()
 *	Purpose ............... : Write file out of memory. 
 *
 *	Description:
 *
 *	
 *
 *	Returns:
 *
 *	nothing. 
 *
 */
fileblaster(stats)
struct	stat	*stats;
{
	int	current_fd;
#if defined(DUX) || defined(DISKLESS)
	char    *p;
	struct	stat	stbuf;
#endif DUX || DISKLESS


	switch (stats->st_mode & S_IFMT)
	{
		case S_IFDIR: /*directory*/ 
			/* 
			 * Always skip over the contents.
			 * We don't need to restore the entries
			 * in the directory! Only it's existence.
			 */
			(void)skipfile((unsigned)stats->st_size);

			/*
			 *	If the path exists - as a directory,
			 *	then we don't need to create it.
			 */
			if (makepath(Pathname))
				break;


#if defined(DUX) || defined(DISKLESS)
			p = &Pathname[strlen(Pathname)-1];
			if ( component_perm[component_cnt] && *p == '+' )
			    *p = '\0';
#endif DUX || DISKLESS
			/*
			 *	If the makedir fails - then don't
			 *	do all the chmods, etc.
			 */
			if (makedir(Pathname, stats->st_mode) == -1)
			{
#if defined(DUX) || defined(DISKLESS)
			    if ( component_perm[component_cnt] ) {
				strcat(Pathname,"+");
				if ( makedir(Pathname, stats->st_mode) == -1 )
				    return;
			    } else
#endif DUX || DISKLESS
			    return;
			}

#if defined(DUX) || defined(DISKLESS)
			if ( stat(Pathname,&stbuf) == -1 )
			    ftio_mesg(FM_NSTAT, Pathname);
			if ( component_perm[component_cnt] ) {
			    if ( chmod(Pathname,stbuf.st_mode|04000) == -1 )
				ftio_mesg(FM_NCHMD, Pathname);
			    strcat(Pathname,"+");
			}
#endif DUX || DISKLESS
			break;

		case S_IFREG: /* regular */
			/*
			 * IF we can make the path and open the file ok,
			 * then write out file. 
			 *
			 * ELSE skip.
			 * 
			 * If makepath fails or mulitple links handled by 
			 * dolinks or makefile fails 
			 * then go the big skip.
			 */
			if ( makepath(Pathname) || 
			    (stats->st_nlink > 1 && dolinks(Pathname, stats)) ||
			    (current_fd = makefile(Pathname, 
						(int)stats->st_mode,
					(unsigned)stats->st_size)) == -1
			)
			{
				(void)skipfile((unsigned)stats->st_size);
			}
			else
			{
				(void)writefile(current_fd, 
						(unsigned)stats->st_size
				);

				/*
				 * Close file
				 */
				(void)close(current_fd);
			}
			break;

		case S_IFNWK: 	/* network special file */

			if (!Dospecial)
			{
				(void)ftio_mesg(FM_NDX, Pathname);/* need -x */
				(void)skipfile((unsigned)stats->st_size);
				return;
			}
			/*
			 * fix DSDe410238 we'll test Dospecial before deleting
			 * of special file but not vise versa as before !
			 */
			(void)unlink(Pathname);

			/*
			 * IF we can make the path and open the file ok,
			 * then write out file. 
			 *
			 * ELSE skip.
			 * 
			 * If makepath fails or mulitple links handled by 
			 * dolinks or makefile fails 
			 * then go the big skip.
			 */
			if ( makepath(Pathname) ||
			     (stats->st_nlink > 1 && dolinks(Pathname,stats)) ||
			     (makespecial(Pathname, (int)stats->st_mode, 
						(short)stats->st_rdev,
						stats->st_size) < 0) 
			)
			{
				(void)skipfile((unsigned)stats->st_size);
				return;
			}

			/*
			 * 	For a network file, we must
			 * 	write out the contents of the file
			 */
			if ((current_fd = openfile(Pathname,1)) < 0)
			{
				(void)skipfile((unsigned)stats->st_size);
				return;
			}
			(void)writefile(current_fd, (unsigned)stats->st_size);

			(void)close(current_fd);
			break;

#ifdef SYMLINKS
		case S_IFLNK:   /* symbolic link */
			/*
			 * IF we can make the path and read the link ok,
			 * then create the symbolic link.
			 *
			 * ELSE skip.
			 * 
			 * If makepath fails or multiple links handled by
			 * dolinks or makesymlink fails
			 * then go the big skip.
			 */
			if ( makepath(Pathname) ||
			     (stats->st_nlink > 1 && dolinks(Pathname,stats)) ||
			     (makesymlink(Pathname,(unsigned)stats->st_size) < 0 )
			)
			{
				(void)skipfile((unsigned)stats->st_size);
				return;
			}
			break;
#endif SYMLINKS

		case S_IFCHR: 	/* character special file */
		case S_IFBLK:	/* block special file */
		case S_IFIFO:	/* fifo/pipe special file */

			if (!Dospecial)
			{
				(void)ftio_mesg(FM_NDX, Pathname);/* need -x */
				return;
			}
			/*
			 * fix DSDe410238 we'll test Dospecial before deleting
			 * of special file but not vise versa as before !
			 */
			(void)unlink(Pathname);

			/*
			 * IF we can make the path and open the file ok,
			 * then write out file. 
			 *
			 * ELSE skip.
			 * 
			 * If makepath fails or mulitple links handled by 
			 * dolinks or makefile fails 
			 * then go the big skip.
			 */
			if ( makepath(Pathname) ||
			     (stats->st_nlink > 1 && dolinks(Pathname,stats)) ||
			     (makespecial(Pathname, (int)stats->st_mode, 
						(short)stats->st_rdev,
						stats->st_size) < 0) 
			)
				return;

			break;

	     default: 	/* unknown */
			(void)ftio_mesg(FM_UNSPX, Pathname);
			return;
	
	} /* switch */
	
	/* 
	 * 	Do chmod. This needs to be done always,
	 *      not optionally. I personally think it is
	 * 	very dum, but that's what cpio(1) does!
	 *
	 *      Note: that we don't do a chmod(2) on the symbolic
	 *            link because chmod(2) follows the link.
	 */
#ifdef SYMLINKS
	if ( !Symbolic_link &&  chmod(Pathname, (int)stats->st_mode) == -1 )
#else
	if (chmod(Pathname, (int)stats->st_mode) == -1)
#endif SYMLINKS
		(void)ftio_mesg(FM_NCHMD, Pathname);

	/*
	 * 	If we are super user, chown(2)ership of files.
	 */
	if (!User_id) 
		if (chown(Pathname,(int)stats->st_uid,(int)stats->st_gid) == -1)
			(void)ftio_mesg(FM_NCHWN, Pathname);

	/* 
	 * 	Restore modification times.
	 *
	 *      Note: that we don't do a utime(2) (called in "set_time")
	 *            on the symbolic link because utime(2) follows
	 *            the link instead of operating on the link itself.
	 */
#ifdef SYMLINKS
	if ( !Symbolic_link && Domodtime &&
#else
	if ( Domodtime &&
#endif SYMLINKS
	     set_time(Pathname, stats->st_mtime, stats->st_mtime) == -1
	)
			ftio_mesg(FM_NMODT, Pathname);

}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Makepath()
 *	Purpose ............... : Make all the directories down to a pathname. 
 * 	Author ................ : Clifford Heath, ASO.
 *
 *	Description:
 *
 * 		Makes the directory structure down to a file, but 
 * 		does not make file - see makefile().
 *
 *	Returns:
 *
 *		0	if succesful.
 *		-1	if failed, errno is set.
 *
 */
makepath(name)
char	*name;
{
	extern	int	errno;
	register char	*p = name;
	struct	stat	stbuf;
	char	*strchr();
#if defined(DUX) || defined(DISKLESS)
	int i = 0;
#endif DUX || DISKLESS

	/*
	 * 	First, check if whole directory path exists
	 * 	because if it does, we don't need to stat all
	 * 	the way down the tree!
	 */
	if ( (p = strrchr(p, '/')) != NULL )
	{
		*p = '\0';
		if ( stat(name,&stbuf) == 0 )
		{
			/* 
			 * 	Alright file exists.
			 * 	If it is a directory return ok.
			 */
			if ( (stbuf.st_mode & S_IFMT) == S_IFDIR ) 
			{
				*p = '/';
				return(0);
			}
		}
		/* ELSE  
		 * 	Stat(2) failed or not directory
		 * 	use the unquick path to resolve...
		 */
		*p = '/';
		p = name;
	}

	/*
	 * Make path leading down to file
	 */
	while ((p = strchr(p,'/')) != NULL) 
	{
		*p = '\0';
		if ( stat(name,&stbuf) < 0 )
		{	/* stat failed for some reason */
			if (errno == ENOENT)	/* file or dir doesn't exist */
			/*
			 * ok make the directory
			 */
			{

#if defined(DUX) || defined(DISKLESS)
			    if ( component_perm[i] && *(p-1) == '+' )
				*(p-1)= '\0';
#endif DUX || DISKLESS

				if ( makedir(name, 0775) < 0 )
				{
#if defined(DUX) || defined(DISKLESS)
					if ( component_perm[i] ) {
					    i++;
					    *(p-1)= '+';
					}
#endif DUX || DISKLESS
					*p = '/';
					return(-1);
				}

#if defined(DUX) || defined(DISKLESS)
				if ( stat(name,&stbuf) == -1 )
				    ftio_mesg(FM_NSTAT, name);
				if ( component_perm[i] ) {
				    if ( chmod(name,stbuf.st_mode|04000) == -1 )
					ftio_mesg(FM_NCHMD, name);
				    *(p-1)= '+';
				}
#endif DUX || DISKLESS

			}
			else	/* file exists, but stat failed! */
			{
				(void)ftio_mesg(FM_NSTAT, name);
				*p = '/';
				return (-1);
			}
		}
		else if ((stbuf.st_mode&S_IFMT) != S_IFDIR) 
		{
			fprintf(stderr,"%s: file in path: %s\n", Myname, name);
			return (-1);
		}
#if defined(DUX) || defined(DISKLESS)
		i++;
#endif DUX || DISKLESS
		*p++ = '/';
	}
	return(0);
}

/* 
 * open a file
 *
 * open a file, the file must exist prior to this call
 * mode is the mode of access required.
 * this call prints diagnostic if there is a failure, 
 * it returns the value open returns
 */
openfile(name, mode)
char	*name;
int	mode;
{
	int	fd;
	
	if ( (fd = open(name, mode)) < 0)
	{
		/* 
		 * 	Error opening file!
		 */
		(void)ftio_mesg(FM_NOPEN, name);
		return(-1);
	}
	return(fd);
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : makefile()
 *	Purpose ............... : Makes a regular file 
 *
 *	Description:
 *
 *	Uses creat(2) and optionally prealloc(2) to 
 *	make a regular file. 
 *
 *	Returns:
 *
 *	file descriptor of file if succesful, or
 *
 *	-1 for fail. 
 */

makefile(name, mode, size)
char	*name;
int	mode;
unsigned size;
{
	int	fd;

	/*
	 * 	Create or truncate file.
	 * 	NOTE: creat truncates an existing file - very convenient
	 * 	for setting up for prealloc!
	 */
	if ((fd = creat(name, mode)) == -1)
	{
		/* 
		 * 	Error creating file!
		 */
		(void)ftio_mesg(FM_NCREAT, name);
		return -1;
	}
	
	/* 
	 * 	Do prealloc if requested.
	 */
	if (Prealloc && size)
	{
		if (prealloc(fd, size) == -1)
		{
			/* 
			 * 	Error preallocing
			 */
			(void)ftio_mesg(FM_NPREALLOC, name);	
			
			/* 
			 * 	I have chosen to ignore any failure of 
			 *	prealloc because we will run into probable
			 *	cause very soon and be delt with.
			 */
		}
	}
	return fd;
}


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : makedir()
 *	Purpose ............... : create a directory (name) 
 *
 *	Description:
 *
 *	Returns:
 *
 *	-1 if fail, 0 if success. 
 */

makedir(name, mode)
char	*name;
ushort	mode;
{
	struct	stat	stbuf;
	int	mkdir();

	if (!Makedir) /* but we have to be allowed! */
	{
		(void)ftio_mesg(FM_NDD);
		return -1;
	}

	if (mkdir(name, mode))
	{
		if (errno == EEXIST)	/* file exists */
		{
			if (stat(name, &stbuf) == -1)
			{
				(void)ftio_mesg(FM_NSTAT, name);
				return -1;
			}

			/* 
			 *	If the file exists but is not a directory 
			 */
			if ((stbuf.st_mode & S_IFMT) != S_IFDIR)
			{
				(void)ftio_mesg(FM_NDIRF, name);
				return -1;
				
			}
			else 
			/* 
			 *	File exists and is directory so ok! 
			 */
				return 0;
		}
		(void)ftio_mesg(FM_NMKDIR, name);
		return -1;
	}
	return 0;
}

/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : makespecial()
 *	Purpose ............... : Create a special file. 
 *
 *	Description:
 *
 *	Makespecial() creates a special file. 
 *
 *	Returns:
 *
 *	-1 if fail.	
 *
 */

makespecial(name, mode, lmagic, majmin)
char	*name;
int	mode;
short	lmagic;
dev_t	majmin;
{
	int	alien = 1;

#ifdef	DEBUG
	if (Diagnostic)
		printf("debug: name>%s mode>%o lmagic>%4x majmin>%8x\n", 
			name, mode, lmagic, majmin );
#endif

	switch(mode & S_IFMT)
	{
		case S_IFCHR:
		case S_IFBLK:
		case S_IFNWK:
		case S_IFIFO:
#ifdef CNODE_DEV
	                if (((lmagic >= 0) && (lmagic <= 255)) ||
			    (lmagic == (short)LOCALMAGIC))
			{
			        alien = 0;
			}
#else
			if (lmagic == (short)LOCALMAGIC)
			{
				alien = 0;
			}
#endif /* CNODE_DEV */
			break;

		default:
			(void)ftio_mesg(FM_UNSPX, name);
			return -1;
	}

	/*
	 *	Alien means the file was probably backed up from
	 *	another type of machine.
	 */
	if (alien)
	{
		(void)ftio_mesg(FM_ALIEN, name);
		return(-1);
	}
	else
	{
		/*
		 * 	First unlink existing file (if it exists)
		 */
		if (unlink(name) == -1)
		{
			/* 
			 *	If error was not that the file does not exist,
			 *	then return error.
			 */
			if (errno != ENOENT)	
			{
				(void)ftio_mesg(FM_NUNLINK, name);
				return -1;
			}
		}

		/*
		 *	Finally do the mknod(2).
		 */
#ifdef CNODE_DEV
		if ((lmagic >= 0) && (lmagic <= 255))
		{
		        if (mkrnod(name, mode, majmin, lmagic) == -1)
			{
			        (void)ftio_mesg(FM_NMKNOD, name);
				return -1;
			}
		}
		else
		{
		        if (mknod(name, mode, majmin) == -1)
			{
			        (void)ftio_mesg(FM_NMKNOD, name);
				return -1;
			}
		}
#else
		if (mknod(name, mode, majmin) == -1)
		{
			(void)ftio_mesg(FM_NMKNOD, name);
			return -1;
		}
#endif /* CNODE_DEV */
	}
	return 0;
}


/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : makesymlink()
 *      Purpose ............... : Create symbolic link.
 *
 *	Description:
 *
 *      Makesymlink() creates a symbolic link.
 *
 *	Returns:
 *
 *	-1 if fail.	
 *
 */

#ifdef SYMLINKS
makesymlink(path, filesize)
    char *path;
    unsigned filesize;
{
    static char link_value[MAXPATHLEN];
    unsigned spaceleft;

    link_value[0] = '\0';

    while(filesize)
    {
	spaceleft = Packets[cur_pkt].block_size - posn_in_buf;

	if ( filesize <= spaceleft ) {
	    strncat(link_value,Packets[cur_pkt].block + posn_in_buf,filesize);
	    posn_in_buf += filesize;
	    filesize = 0;
	}
	else    /* goes onto next buffer */
	{
	    strncat(link_value,Packets[cur_pkt].block + posn_in_buf,spaceleft);
	    filesize -= spaceleft;
	    get_next_pkt();
	}
    }

    /*
     *      If binary headers and we are on
     *      an odd address, move to even out.
     */
    if (!Htype && (posn_in_buf & 1))
    {
	posn_in_buf++;
	if (posn_in_buf >= Packets[cur_pkt].block_size)
	    get_next_pkt();
    }

#ifdef DEBUG
fprintf(stderr, "ftio: read value of link <%s>: <%s>\n", path, link_value);
#endif DEBUG

    if ( symlink(link_value,path) < 0 ) {
	(void)ftio_mesg(FM_SYMLINK, link_value, path);
	return 0;
    }

    return 0;
}
#endif SYMLINKS


/*--------------------------------------------------------------------

Title ....................: dolinks()
Purpose...................: Make inode links while retreiving from
			    archive. Called by fileblaster()
Returns...................: 


---------------------------------------------------------------------*/


dolinks(name, stats)
register char *name;
struct	stat	*stats;
{
	int 	found = 0;
	int 	retval = 0; /*caller will invoke writefile() unless this = 1 */ 
	char	*malloc();


  struct sym_struct
      {
	      unsigned long l_dev;
	      unsigned long l_ino;
	      struct sym_struct *next;
	      char name[2];
     	      int count;
      };

   typedef struct sym_struct SYMBOL;

    static SYMBOL **tbl = NULL;
	SYMBOL *sym;
    int key;

	if(stats->st_ino == UNREP_NO || stats->st_dev == OUT_OF_SPACE )
	{
        (void) ftio_mesg(FM_NLINK32R, name);	
	return;
	}

    /*
     * Allocate our hash table if we haven't already
     */
    if (tbl == NULL)
    {
	int i;

	if ((tbl=(SYMBOL **)malloc(sizeof(SYMBOL *)*HASHSIZE)) == NULL)
	{
	    perror("ftio");
	    exit(3);
	}
	for (i = 0; i < HASHSIZE; i++)
	    tbl[i] = NULL;
    }


   /*
	* Search the hash table for this (dev,ino) pair
	*/
	key = ((stats->st_dev ^ stats->st_ino) & 0x7fffffff) % HASHSIZE;
	    for (sym = tbl[key]; sym != NULL; sym = sym->next) {
	       if (sym->l_ino == stats->st_ino && sym->l_dev == stats->st_dev)
		  {

	          /*
		  * Found it, do the link
	          */
			found++;
			break;
		   }
	     }

	/*
	 * if we found a match:
	 */
	if (found) 
	{
		/*
		 * ok, link existing file to new header name.
		 */
		if ( link(sym->name, name) < 0 )
		{
			(void)ftio_mesg(FM_NLINK, name, sym->name);
		}
		else
		{
			(void)ftio_mesg(FM_LINK, name, sym->name);
			retval = 1;
		}

		sym->count--;	/* count down no of links done */
		
		return(retval);	
	}

	else /* new entry to store in link table */	
	
	{

    /*
     * Didn't see this file before, put it into the hash table
     */
    	if ((sym = (SYMBOL *)malloc(sizeof(SYMBOL)+strlen(name)+2)) == NULL)
    	{
		static int first = 1;

		if (first)
			fprintf(stderr, "No memory for links\n");
			first = 0;
    	}

	else
	{
			 /* ok, got the space, enter new entry
			 */
    		sym->l_dev = stats->st_dev;
    		sym->l_ino = stats->st_ino;
    		sym->count = stats->st_nlink - 1;
    		strcpy(sym->name, name);
    		sym->next = tbl[key];
    		tbl[key] = sym;
	
	}
	return(retval);
	
	} /* new entry to store in the link table */
}  /* dolinks */


/*
 *-----------------------------------------------------------------------------
 *
 *	Title ................. : Getheader()
 *	Purpose ............... : Extract a file from the archive. 
 *
 *	Description:
 *
 *	Returns:
 *
 *	0 if found a file, 1 if found end of archive. 
 */
static  char    save_hdr[MAXHDRBUF];

getheader(stats)
struct	stat	*stats;
{
	int	resync_ok = Resync;
	int	spaceleft;
	int 	headersize;
	int	filename_size;
	int	try = 0;
	int	i;

	headersize = (Htype)? CHARS: HDRSIZE;

	/*
	 *	Loop until we sync with a header.
	 */
	while(1)
	{
		spaceleft = Packets[cur_pkt].block_size - posn_in_buf;

		if (spaceleft > headersize)
		{
			if ((filename_size = readheader(Packets[cur_pkt].block 
							+ posn_in_buf,
							stats
				)
			    ) != -1
			)
			{
				posn_in_buf += headersize;
				goto filename;
			}
			else
			{
				/*
				 *	We can't read the header, so do 
				 *	we resync??
				 */
				if (!resync_ok)
				{
					if (ftio_mesg(FM_RSYNC))
						resync_ok++;
					else
						return 1;
				}
				try++;
				posn_in_buf++;
			}
		}
		else
		{
#ifdef	DEBUG
			if (Diagnostic)
				printf("XXX: header over block boundary!\n");
#endif	DEBUG
			(void)memcpy(save_hdr, 
				     Packets[cur_pkt].block + posn_in_buf,
				     spaceleft
			);
#ifdef DEBUG
			if ( spaceleft >= MAXHDRBUF )
			    printf("(1) *** ruined save_hdr: copied %d bytes (max=%d)\n",spaceleft,MAXHDRBUF);
#endif DEBUG

			get_next_pkt();
			(void)memcpy(save_hdr + spaceleft, 
				     Packets[cur_pkt].block, 
				     headersize
			);
#ifdef DEBUG
			if ( (headersize+spaceleft) >= MAXHDRBUF )
			    printf("(2) *** ruined save_hdr: copied %d bytes (max=%d)\n",headersize+spaceleft,MAXHDRBUF);
#endif DEBUG

			/*
			 *	Loop until we can go back into the new
			 *	block.
			 */
			i = 0;
			while(spaceleft)
			{
				if ((filename_size = readheader(save_hdr + i,
								stats
						     )
				    ) != -1
				)
				{
					posn_in_buf += headersize - spaceleft;
					goto filename;
				}
				else
				{
					if (!resync_ok)
					{
						if (ftio_mesg(FM_RSYNC))
							resync_ok++;
						else
							return 1;
					}
					try++;
					i++;
					spaceleft--;
				}
			}
		}
	}

filename:
	if (try)	
		fprintf(stderr, "Resynced after %d tries.\n", try);

	/*
	 *	Are we translating to relative pathnames??
	 */
	if (Makerelative && *(Packets[cur_pkt].block + posn_in_buf) == '/')
	{
		posn_in_buf++;
		filename_size--;

		/*
		 *	Once in a million years that line above can
		 *	cause posn_in_buf to go over the block boundary.
		 */
		if (posn_in_buf > Packets[cur_pkt].block_size)
			(void)get_next_pkt();
	}

	/*
	 *	Now copy over filename.
	 */
	spaceleft = Packets[cur_pkt].block_size - posn_in_buf;

	if (spaceleft > filename_size )
	{
		(void)Pathname_cpy(Packets[cur_pkt].block + posn_in_buf,
			filename_size
		);
		posn_in_buf += filename_size;
	}
	else
	{
#ifdef	DEBUG
		if (Diagnostic)
			printf("XXX: filename over block boundary!\n");
#endif	DEBUG
		(void)Pathname_cpy(Packets[cur_pkt].block + posn_in_buf,
			spaceleft
		);
		(void)get_next_pkt();
		(void)Pathname_cat(Packets[cur_pkt].block,
			filename_size - spaceleft
		);
		posn_in_buf = filename_size - spaceleft;
	}
	
	/*
	 *	If we have found the trailer filename, then
	 *	exit the program.
	 */
	if (!strncmp(Pathname, "TRAILER!!!", 10))
		return 1;
	
	if (!Htype && (posn_in_buf & 1))	
	{
		posn_in_buf++;
		if (posn_in_buf >= Packets[cur_pkt].block_size)
			get_next_pkt();
	}	

#if defined(DUX) || defined(DISKLESS)
#ifdef DEBUG
fprintf(stderr, "pre restore: <%s>\n", Pathname);
#endif DEBUG
	(void) restore(Pathname,component_perm,&component_cnt);
#ifdef DEBUG
fprintf(stderr, "post restore: <%s>\n", Pathname);
#endif DEBUG
#endif DUX || DISKLESS

	return 0;
}

writefile(fd, filesize)
int	fd;
unsigned filesize;
{
	unsigned spaceleft;
	int	writeret;

	if (!filesize)
		return(0);

	while(filesize)
	{
		spaceleft = Packets[cur_pkt].block_size - posn_in_buf;

		if (filesize <= spaceleft)
		{
			if ( (writeret = write(fd, 
					  Packets[cur_pkt].block + posn_in_buf, 
					  filesize
					 )
			     ) == -1
			)
			{
				(void)ftio_mesg(FM_NWRIT, Pathname);
				(void)close(fd);
				(void)skipfile(filesize);
				return 0;
			}

			posn_in_buf += filesize;

			filesize -= writeret;
		}
		else	/* goes onto next buffer */
		{
			if ( (writeret = write(fd, 
					  Packets[cur_pkt].block + posn_in_buf, 
					  spaceleft
					 )
			     ) == -1
			)
			{
				(void)ftio_mesg(FM_NWRIT, Pathname);
				(void)close(fd);
				(void)skipfile(filesize);
				return 0;
			}

			filesize -= writeret;

			get_next_pkt();
		}
	}

	/* 
	 *	If binary headers and we are on 
	 *	an odd address, move to even out. 
	 */
	if (!Htype && (posn_in_buf & 1))	
	{
		posn_in_buf++;
		if (posn_in_buf >= Packets[cur_pkt].block_size)
			get_next_pkt();
	}	
	return(0);
}

skipfile(filesize)
unsigned filesize;
{
	unsigned spaceleft;

	if (!filesize)
		return 0;

	while(filesize)
	{
		spaceleft = Packets[cur_pkt].block_size - posn_in_buf;

		if (filesize <= spaceleft)
		{
			posn_in_buf += filesize;

			filesize = 0;
		}
		else	/* goes onto next buffer */
		{
			filesize = filesize - spaceleft;

			get_next_pkt();
		}
	}
	
	/* 
	 *	If binary headers and we are on 
	 *	an odd address, move to even out. 
	 */
	if (!Htype && (posn_in_buf & 1))	
	{
		posn_in_buf++;
		if (posn_in_buf >= Packets[cur_pkt].block_size)
			get_next_pkt();
	}	
	return 0;
}

get_next_pkt()
{
	/*
	 *	Release the current packet.
	 *	Update the packet number.
	 *	Get a new packet. 
	 *	Set the posn to starrt of buffer.
	 */
#ifdef PKT_DEBUG
printf("ftio: filewriter: before releasing packet %d (INPUT=%d).\n", cur_pkt, get_val(INPUT));
fflush(stdout);
#endif PKT_DEBUG
	release_packet(INPUT);
#ifdef PKT_DEBUG
printf("ftio: filewriter: done with packet %d (released) (INPUT=%d).\n", cur_pkt, get_val(INPUT));
fflush(stdout);
#endif PKT_DEBUG
	cur_pkt = (cur_pkt < (Nobuffers-1)) ? (cur_pkt + 1): 0;
#ifdef PKT_DEBUG
printf("ftio: filewriter: waiting on packet %d (OUTPUT=%d).\n", cur_pkt, get_val(OUTPUT));
fflush(stdout);
#endif PKT_DEBUG
	wait_packet(OUTPUT);
#ifdef PKT_DEBUG
printf("ftio: filewriter: got packet %d (done waiting) (OUTPUT=%d).\n", cur_pkt, get_val(OUTPUT));
fflush(stdout);
#endif PKT_DEBUG
	posn_in_buf = 0;
}

get_first_pkt()
{
	cur_pkt = 0;

#ifdef PKT_DEBUG
printf("ftio: filewriter: released packet %d (INPUT=%d).\n", cur_pkt, get_val(INPUT));
fflush(stdout);
#endif PKT_DEBUG

#ifdef PKT_DEBUG
printf("ftio: filewriter: waiting on packet %d (OUTPUT=%d).\n", cur_pkt, get_val(OUTPUT));
fflush(stdout);
#endif PKT_DEBUG
	wait_packet(OUTPUT);
#ifdef PKT_DEBUG
printf("ftio: filewriter: got packet %d (OUTPUT%d).\n", cur_pkt, get_val(OUTPUT));
fflush(stdout);
#endif PKT_DEBUG
	posn_in_buf = 0;
}



/*
 *-----------------------------------------------------------------------------
 *
 *      Title ................. : restore()
 *      Purpose ............... : scan path for CDF components
 *
 *	Description:
 *
 *
 *      Given a path (possibly an expanded cdf with "+//" in it)
 *      return a compressed path, a bitstring which contains
 *      the permissions of each component, and the number of components.
 *
 *
 *      Returns:
 *
 *          compressed path ("str")
 *          permissions bit string ("bitstr")
 *          number of components in path ("cntc")
 */

#if defined(DUX) || defined(DISKLESS)

restore(str,bitstr,cntc)
    char *str;
    int bitstr[];
    int *cntc;
{
    int i;
    char *p, *cp;

    *cntc = 0;
    for (i=0; i < MAXCOMPONENTS;i++)
	bitstr[i] = 0;

    p = str;
    if ( *p == '/' )
	p++;

    for (; *p && (p-str) < MAXPATHLEN; p++) {
	if (*p == '/')
	    (*cntc)++;

	if ( *cntc >= MAXCOMPONENTS ) {
	    fprintf(stderr, "ftio: too many components in file name %s\n", str);
	    break;
	}

	if ( *p == '+' && *(p+1) == '/' && (*(p+2) == '/' || *(p+2) == 0 ) ) {
	    bitstr[*cntc] = 1;
	    overlapcpy(p+1,p+2);
	}
    }

#ifdef DEBUG
    for(i=0; i < MAXCOMPONENTS; i++) {
	printf("bitstr[%d],%d ",i,bitstr[i]);
	printf("(%s/)\n",str);
    }
#endif DEBUG

    return;
}
#endif DUX || DISKLESS
