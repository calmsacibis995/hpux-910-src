static char *HPUX_ID = "@(#) $Revision: 70.1 $";

/* this version has S5.2 set of argument: -t -f but returns the
 * # of allocatable blocks which already take the minfree
 * into account instead of total # of free blocks
 *   no argument:  returns allocatable blocks and #inodes free
 *   -f  	:  returns allocatable blocks
 *   -t         :  returns total blocks and inodes in file system
 *		:          used blocks and used inodes and
 *		           percentage of minimum reserved free blocks
 */

#include <stdio.h>

#include <sys/types.h>
#include <sys/param.h> 
#include <sys/stat.h>
#include <fcntl.h>    
#ifndef CDROM
#include <sys/fs.h>
#endif /* CDROM */
#ifdef SWFS
#include <sys/swap.h>
#endif /* SWFS */
#include <mntent.h>
#include <sys/vfs.h>
#ifdef TRUX
#include <sys/security.h>
#endif

#include <sys/signal.h>
    
#define EQ(x,y,z) (strncmp(x,y,z)==0)

/*								Wed Jun 12, 1985
**	Modified to print out 512 byte blocks, even though
**	this seems very broken.  Should print out PHYSICAL
**	blocks (logical == physical)					  -- jad
*/

#define KBSHIFT 9            /* log 2 of 512 */
struct mptr {
    struct	mntent	M;
    struct	mptr	*next;
};
struct mptr *tail, *cur, *prev;
struct stat	S;
#ifndef CDROM
struct fs	sblock;
#endif /* CDROM */
int	fflag, tflag;
#ifdef SWFS
int	bflag;
#endif /* SWFS */
#ifndef CDROM
int	fd;
daddr_t	blkno	= 1;
daddr_t	alloc();
#endif /* CDROM */

void myalrm();
int alarmoff = 0;		/* assume no alarm */
int armnfs;
int printval = 0;		/* assume success */

#ifdef LOCAL_DISK
int local_only = 0;		/* local disk flags */
int local_plus_nfs = 0;
cnode_t mysite;			/* cnode id */
int nfs_fs;
#endif /* LOCAL_DISK */

main(argc, argv, environ)
int argc;
char **argv;
char **environ;
{
	FILE *fi; 
	int i;
        struct mntent *mntptr;
	register char	c;
	char	 *dev;
	int	len;
	int	 j;
	extern	char	*malloc();

#ifdef LOCAL_DISK
	mysite = cnodeid();	/* get cnode */
#endif /* LOCAL_DISK */	
	signal(SIGALRM, myalrm);
        cleanenv( &environ, "LANG", "LANGOPTS", "NLSPATH", 0 ); 

#ifdef SecureWare
	if( ISSECURE )
        	df_init(argc, argv);
#endif
	while(argc > 1 && argv[1][0] == '-') {
		switch(c = argv[1][1]) {
#ifdef SWFS
		        case 'b':
				bflag++;
				break;
#endif /* SWFS */
#ifdef LOCAL_DISK
			case 'l':
				if(local_plus_nfs) {
				    (void) fprintf(stderr,
				       "df: conflicting options: -l, -L\n");
				    exit(1);
				}
				local_only++;
				break;
			case 'L':
				if(local_only) {
				    (void) fprintf(stderr,
				       "df: conflicting options: -l, -L\n");
				    exit(1);
				}
				local_plus_nfs++;
				break;
#endif /* LOCAL_DISK */				
			case 'f':
				fflag++;
				break;	
			case 't':
				tflag=1;
				break;

			default:
				fprintf(stderr,"df: illegal arg -%c\n",c);
				exit(1);
		}
		argc--;
		argv++;
	}
	if((fi = setmntent(MNT_MNTTAB,"r")) == NULL) {
		fprintf(stderr,"df: cannot open /etc/mnttab\n");
		exit(1);
	}

        i=0;
	cur = (struct mptr *)malloc(sizeof(struct mptr));
	tail = cur;
	prev = cur;
        while ( ( mntptr = getmntent(fi)) != (struct mntent *) NULL ){
#ifdef LOCAL_DISK
	        /*
		 * if local_only or local_plus_nfs specified,
		 * skip inappropriate entries
		 */
	        nfs_fs = 0;			/* assume other type of fs */
		if(strcmp(mntptr->mnt_type, MNTTYPE_NFS) == 0)
		    nfs_fs = 1;
		if(mntptr->mnt_cnode != mysite &&
		  (local_only || (local_plus_nfs && !nfs_fs)))
		    continue;
#endif /* LOCAL_DISK */		
		cur = (struct mptr *)malloc(sizeof(struct mptr));
		cur->next = prev;
		prev = cur;
		if((cur->M.mnt_fsname = malloc(strlen(mntptr->mnt_fsname)+1)) == NULL)
		{
			fprintf(stderr, "df: cannot alloc %s\n", 
				mntptr->mnt_fsname);
			continue;
		}
		else
                	strcpy(cur->M.mnt_fsname, mntptr->mnt_fsname);

		if((cur->M.mnt_dir = malloc(strlen(mntptr->mnt_dir)+1)) == NULL)
		{
			fprintf(stderr, "df: cannot alloc %s\n", 
				mntptr->mnt_dir);
			continue;
		}
		else
                	strcpy(cur->M.mnt_dir, mntptr->mnt_dir);
/*
 * NOTE:
 * The members mnt_type, 
 * mnt_opts, mnt_time, and
 * mnt_freq are not currently 
 * so they are commented out 
 * until they are needed.
 *
 *		if((cur->M.mnt_type = malloc(strlen(mntptr->mnt_type)+1)) == NULL)
 *	 	{
 *			fprintf(stderr, "df: cannot alloc %s\n", 
 *				mntptr->mnt_type);
 *			continue;
 *		}
 *		else
 *               	strcpy(cur->M.mnt_type, mntptr->mnt_type);
 *
 *		if((cur->M.mnt_opts = malloc(strlen(mntptr->mnt_opts)+1)) == NULL)
 *		{
 *			fprintf(stderr, "df: cannot alloc %s\n", 
 *				mntptr->mnt_opts);
 *			continue;
 *		}
 *		else
 *             		strcpy(cur->M.mnt_opts, mntptr->mnt_opts);
 *		cur->M.mnt_time = mntptr->mnt_time; 
 *		cur->M.mnt_freq = mntptr->mnt_freq;
 */
        }
        endmntent(fi);
        i = (sizeof (struct mntent) )*i;

	while(cur != tail) {
		if(!cur->M.mnt_fsname[0]) /* Skip the unmounted devices */
			continue;

		/* cur->M.mt_dev is special file name reside under /dev */
		if((len = strlen(cur->M.mnt_fsname)) > 1)
		{
			if((dev = malloc(len+1)) == NULL)
			{
				fprintf(stderr, "df: cannot alloc %s\n", dev);
				continue;
			}	
			else
				strncpy(dev, cur->M.mnt_fsname, len+1);
		}
		else
			continue;

		if(argc > 1) {
			for(j = 1; j < argc; ++j) {
			        int fslen, dirlen, argvlen;
				argvlen = strlen(argv[j]);
				fslen = strlen(cur->M.mnt_fsname);
				dirlen = strlen(cur->M.mnt_dir);
				if(EQ(argv[j], dev, len)
				|| EQ(argv[j], cur->M.mnt_fsname,MAX(argvlen,fslen))
				|| EQ(argv[j], cur->M.mnt_dir,MAX(argvlen,dirlen))) {
				        if(cur->M.mnt_type == MNTTYPE_NFS)
					    armnfs = 1;
					printit(dev, cur->M.mnt_dir);
					argv[j][0] = '\0';
				}
			}
		} else
			printit(dev, cur->M.mnt_dir);
		cur = cur->next;
	}
	for(i = 1; i < argc; ++i) {
		if(argv[i][0])
			printit(argv[i], "\0");
	}
	exit(printval);
}

printit(dev, fs_name)
char *dev, *fs_name;
{
#ifdef SWFS
	 struct swapfs_info swapfs_buf;
#endif /* SWFS */
	 long allocblks,   /* total # of fragments - minfree in fs */
	      free,        /* total # of free fragments in fs */
	      avail, 	  /* # of allocatable fragments */
	      used,	  /* # of fragments used */
	      inodes,
	      used_inodes;
	 char	*s;
	 struct statfs buf;
	 int minfree;
	 int autofd;
#ifndef CDROM
int fallback = 0;
#endif /* CDROM */

	/*
	 * There have been complaints about df hanging on a remote
	 * file system.  The fix to that problem is to set an alarm
	 * of 10 seconds prior to the statfs() system call.  The hooker
	 * is that statfs is uninterruptable until the rpc layer in
	 * the kernel.  After several retries, the kernel recognizes
	 * that the remote system is unavailable, and starts to field
	 * signals.  It is at this point that the alarm will be noticed
	 * and honored.  At that point myalrm() is called, a global flag
	 * is set, the signal handler is reset, and program execution
	 * continues.
	 */
	if(armnfs)
	    alarm(10L);		/* set the alarm */
	autofd = open(fs_name, O_RDONLY);
	if(alarmoff) {
	    errmsg(fs_name);
	    printval = 1;
	    return;
	}
	if (fstatfs(autofd, &buf) != 0 ){
	    close(autofd);
/*&&&&*/ /* The following lines are used as a fallback */
	 /* The old df can return information on an umounted file
	    system. However,statfs can only give info for a mounted one.
	    so if statfs failed, try the old method */

#ifndef CDROM
		if((fd = open(dev, 0)) < 0) {
			fprintf(stderr,"df: cannot open %s\n",dev);
			printval = 1;
			return;
		}
#endif /* CDROM */
		sync();
	        autofd = open(dev, O_RDONLY);
	        if(autofd == -1) {
		    fprintf(stderr, "df: open of %s failed\n",dev);
		    printval = 1;
		    return;
		}

		if(fstat(autofd, &S) < 0) {
			fprintf(stderr,"df: cannot stat %s\n",dev);
		    	printval = 1;
			return;
		}
	        close(autofd);
		if( (S.st_mode & S_IFMT) != S_IFBLK && !fflag )
		{
			fprintf(stderr, "df: a block device is required ");
			fprintf(stderr, "when the -f option is not specified\n");
			printval = 1;
			return;
		}
#ifdef CDROM
	        autofd = open(dev, O_RDONLY);
	        if(autofd == -1) {
		    fprintf(stderr, "df: open of %s failed\n",dev);
		    printval = 1;
		    return;
		}
	    
		if (fstatfsdev(autofd, &buf) != 0) {
			fprintf(stderr, "df: cannot determine file system ");
			fprintf(stderr, "statistics for %s\n", dev);
			printval = 1;
			return;
		}
	        close(autofd);
	}
#else /* CDROM */

		/* read super block */
		lseek(fd, (long)SBSIZE, 0);
		read(fd, &sblock, sizeof sblock);

		free = sblock.fs_cstotal.cs_nffree +
			(sblock.fs_cstotal.cs_nbfree << sblock.fs_fragshift);
		used = sblock.fs_dsize - free;
		allocblks = sblock.fs_dsize * (100 - sblock.fs_minfree)/100;
		avail = allocblks > used ? allocblks - used: 0;
	
		inodes = sblock.fs_ncg * sblock.fs_ipg;
		used_inodes = inodes - sblock.fs_cstotal.cs_nifree;
             buf.f_bsize = sblock.fs_fsize;
	     buf.f_ffree = sblock.fs_cstotal.cs_nifree;
	     buf.f_blocks = sblock.fs_dsize;
	     fallback++;
	     goto DONE;
        }
             /*fprintf(stderr,"cannot statfs %s\n",fs_name);
             return;*/
/*&&&&*/ /* end of fallback */
#endif /* CDROM */
        
        close(autofd);
        if(armnfs)
            alarm(0);		/* reset the alarm */
        armnfs = 0;		/* assume no NFS mount */
        free = buf.f_bfree;
        avail = (buf.f_bavail > 0 ? buf.f_bavail : 0 );
        if ((buf.f_blocks != (long) -1) 
            && (free != (long) -1 ))
            used = buf.f_blocks - free;
        else used = -1;
        inodes=buf.f_files;
        if ((inodes != (long ) -1 ) &&
           (buf.f_ffree != (long) -1))used_inodes=buf.f_files - buf.f_ffree;
        else used_inodes = -1;
#ifndef CDROM
DONE:
#endif /* CDROM */
       
	

	/* MNTLEN is defined to be 32 in S5; if we want to printf fs_name
         * and dev in full length, the output will span more than 1 line;
	 * So we specify the maximum field width possible for fs_name
	 * given 80 chars/line.
         */
	if (fflag) {
		if ( *fs_name == '\0' )
			printf("  %-13s  ", " ");
		else
			printf("%-17s", fs_name);
		printf(" (%-15s): %8ld blocks\n", dev,
#ifdef CDROM
			avail * (buf.f_bsize >> KBSHIFT));
#else /* CDROM */
			(avail * buf.f_bsize)>> KBSHIFT);
#endif /* CDROM */
	} else 
	{
/* the number 20 
 * will now be used 
 * for formatting
 * until someone
 * thinks of a better
 * hard coded one (yuk!)
 */
		if(((strlen(fs_name) >= 20 || strlen(dev) >= 20)) || tflag)
			s = "%-16s (%-15s): %8ld blocks       %7d i-nodes\n";
		else
			s = "%-20.20s (%-20.20s): %8ld blocks   %7d i-nodes\n";
		printf(s,
	
#ifdef CDROM
		      fs_name, dev, avail * (buf.f_bsize>>KBSHIFT),
                      buf.f_ffree);
#else /* CDROM */
		      fs_name, dev, (avail * buf.f_bsize)>>KBSHIFT,
                      buf.f_ffree);
#endif /* CDROM */
	}
	if(tflag)
	{
	     printf("%44ld total blocks %7d total i-nodes\n", 
#ifdef CDROM
		     buf.f_blocks * (buf.f_bsize >>KBSHIFT),
#else /* CDROM */
		     (buf.f_blocks * buf.f_bsize) >>KBSHIFT,
#endif /* CDROM */
		     inodes);
	     printf("%44ld used  blocks %7d used i-nodes\n",
#ifdef CDROM
		     used * (buf.f_bsize >>KBSHIFT),
#else /* CDROM */
		     (used * buf.f_bsize) >>KBSHIFT,
#endif /* CDROM */
		     used_inodes);
#ifdef CDROM
	     if ((buf.f_bfree == -1) || (buf.f_bavail == -1))
		     minfree = -1;
#else /* CDROM */
	     if (fallback)
	     	printf("%44d percent minfree\n", sblock.fs_minfree);	
             else if ((buf.f_bfree == -1 ) || (buf.f_bavail == -1 )) minfree = -1;
#endif /* CDROM */
             else if (! buf.f_blocks)
		minfree = 0;
             else      
                minfree = (int ) (100 * ((float)(buf.f_bfree - buf.f_bavail) /(float) buf.f_blocks));
#ifdef CDROM
	     printf("%44d percent minfree\n",minfree);
#else /* CDROM */
             if (!fallback) printf("%44d percent minfree\n",minfree);
#endif /* CDROM */
#ifdef SWFS
	  if (!bflag)
#endif /* SWFS */
	     printf("\n");
	}		      	
#ifdef SWFS
	if (bflag)
	{
	    if (swapfs(fs_name ,&swapfs_buf) == 0)
	    {
                int available;
                int swfree;

                /*
		 * compute the swap space currently available
		 * available = used + file system space available -
		 * reserved.  Reduce the value to the specified
		 * limit (if there is one).
		 */
		available = swapfs_buf.sw_binuse +	/* blocks in use */
		            buf.f_bavail -		/* free blocks */
			    swapfs_buf.sw_breserve;	/* reserved */
		if (available < 0)
		    available = 0;
		if ((available > swapfs_buf.sw_bavail) &&
		    (swapfs_buf.sw_bavail > 0)) {
		    available = swapfs_buf.sw_bavail;
		}
		swapfs_buf.sw_bavail = available;

                swfree = ((swapfs_buf.sw_bavail - swapfs_buf.sw_binuse) *
		      (buf.f_bsize / 512));
                if(swfree < 0)
                    swfree = 0;
                swapfs_buf.sw_bavail =
                    (swapfs_buf.sw_bavail * (buf.f_bsize / 512));
                swapfs_buf.sw_binuse =
                    (swapfs_buf.sw_binuse * (buf.f_bsize / 512));
                if(swapfs_buf.sw_bavail == 0) {
		    swapfs_buf.sw_bavail = (buf.f_bavail * buf.f_bsize) / 512;
		}
		if (tflag)
		{
		    printf("Swapping on %-23s %8ld blocks\n",
			   swapfs_buf.sw_mntpoint, swfree);
		    printf("                                    %8ld total blocks\n",
			   swapfs_buf.sw_bavail);
		    printf("                                    %8ld used blocks\n",
			    swapfs_buf.sw_binuse);
		    if (swapfs_buf.sw_bavail == 0)
			printf("                                           0 percent used\n");
		    else
			printf("                                        %4d percent used\n",
				(long)((double)swapfs_buf.sw_binuse /
				       (double)swapfs_buf.sw_bavail *
				       100.0 + 0.5));
		}
                else if (fflag)
		{
		    printf("Swapping on %-23s: %8d blocks %9d free\n",
			  swapfs_buf.sw_mntpoint, swapfs_buf.sw_binuse,swfree);
		}
		else
		{
		    printf("Swapping on %-31s: %8d blocks %9d free\n",
			  swapfs_buf.sw_mntpoint, swapfs_buf.sw_binuse, swfree);
		}
	    }
	    if (tflag)
		printf("\n");
	}
#endif /* SWFS */
		       
}


void
myalrm()
{
    alarmoff = 1;
    signal(SIGALRM, myalrm);
}


errmsg(fs_name)
char *fs_name;    
{
    fprintf(stderr, "file system %s not responding\n", fs_name);
    alarmoff = 0;
    return;
}
