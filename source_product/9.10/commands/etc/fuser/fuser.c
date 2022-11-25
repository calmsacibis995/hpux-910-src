static char *HPUX_ID = "@(#) $Revision: 72.6 $";

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <sys/dir.h>
#include <sys/signal.h>
#include <sys/sysmacros.h>
#include <sys/file.h>
#include <sys/vfs.h>
#include <sys/inode.h>
#include <sys/stat.h>
#include <sys/cdfsdir.h>
#include <sys/cdnode.h>
#include <netinet/in.h>
#include <nfs/nfs.h>
#include <nfs/rnode.h>
#include <nfs/nfs_clnt.h>
#include <string.h>
#include <sys/unistd.h>

#if defined DUX || defined DISKLESS
#include <mntent.h>
#endif DUX || DISKLESS

#include <sys/param.h>
#include <nlist.h>
#include <fcntl.h>

#ifdef hp9000s800
#include <sys/libIO.h>
#endif

#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vm.h>

#if defined (hp9000s200) 
#include <ctype.h>
#endif

#if !defined(UMM) && defined(SecureWare)
#include <sys/security.h>
#endif

#define kmemf "/dev/kmem"

int kmem;

char *system="/hp-ux";

#ifdef hp9000s800
struct nlist nl[] = {
	{ "proc"},
#define X_PROC	        0
	{"file"},
#define X_FILE          1
	{"inode"},
#define X_INODE         2
	{ "nproc" },
#define X_NPROC	        3
	{"fileNFILE"},
#define X_NFILE         4
	{"inodeNINODE"},
#define X_NINODE        5
	{"cdnode"},
#define X_CDNODE        6
	{"cdnodeNCDNODE"},
#define X_NCDNODE       7
	{ "brmtdev" },
#define X_BRMTDEV	8
        { "lvm_maj" },
#define X_LVMMAJ        9
	{"rootvfs"},
#define X_ROOTVFS       10
	{ 0 },
};
#else
struct nlist nl[] = {
	{ "_proc"},
#define X_PROC	        0
	{"_file"},
#define X_FILE          1
	{"_inode"},
#define X_INODE         2
	{ "_nproc" },
#define X_NPROC	        3
	{"_fileNFILE"},
#define X_NFILE         4
	{"_inodeNINODE"},
#define X_NINODE        5
	{"_cdnode"},
#define X_CDNODE        6
	{"_cdnodeNCDNODE"},
#define X_NCDNODE       7
	{"_rootvfs"},
#define X_ROOTVFS       8
	{ 0 },
};
#endif /*hp9000s800*/

int nproc;  /* ---> nl[X_NPROC] ---> _nproc */
off_t procptr;  /* ---> nl[X_PROC] ---> _proc */
off_t fileptr;     /* ---> nl[X_FILE] ---> _file */
off_t endfile;  /* ---> nl{X_NFILE] ---> _fileNFILE */
off_t inodeptr;    /* ---> nl[X_INODE] ---> _inode */
off_t endinode; /* ---> nl[X_NINODE] ---> _inodeNINODE */
off_t cdnodeptr;    /* ---> nl[X_CDINODE] ---> _cdinode */
off_t endcdnode; /* ---> nl[X_NCDNODE] ---> _cdnodeNCDNODE */
off_t rootvfs; /* ---> nl[X_ROOTVFS] ---> _rootvfs */


#define ieq( X, Y) ((((X).i_dev == (Y).i_dev) && ((X).i_number == (Y).i_number)) || \
	( (((X).i_mode & IFMT) == IFBLK) && ((X).i_rdev == (Y).i_dev) ))
#define min( X, Y)  ( (X < Y) ? X : Y )

#define valid_file(X) (fileptr <= (unsigned) X &&\
	(unsigned) X < endfile)

#define valid_inode(X) (inodeptr <= (unsigned) X &&\
	(unsigned) X < endinode)

#define valid_cdnode(X) (cdnodeptr <= (unsigned) X &&\
	(unsigned) X < endcdnode)

void exit(), perror();


main( argc, argv)
    int argc;
    char **argv;
{	
    int file = 0, gun = 0, uname = 0, mount_point=0, stale=0;
    int i, j, k;
    struct inode ina, inb;
    struct proc p;
    int l;
    int result;
    struct ofile_t file_chunk;
    struct vfs *vfs;
    struct ofile_t *ptr_chunk[MAXFUPLIM];
    struct ofile_t *op;
    
#if !defined(UMM) && defined(SecureWare)
   if (ISSECURE)  {
	set_auth_parameters(argc, argv);

#ifdef B1
       if (ISB1) {
	       initprivs();
	       (void) forcepriv(SEC_ALLOWDACACCESS);
	       (void) forcepriv(SEC_ALLOWMACACCESS);
	}
#endif

       if (!authorized_user("mem"))  {
	       fprintf(stderr, "fuser: no authorization to run fuser\n");
	       exit(1);
	}
   }
#endif

    /* open file to access kmem */
    if ( (kmem = open( kmemf, O_RDONLY)) == -1) {
	fprintf( stderr, "%s: ", argv[0]);
	perror( kmemf);
	exit( 1);
    }

    nlist(system, nl); /* get values of system variables */
    getkvars();        /* reads nlist values from kmem into variables */
    
    if ( argc < 2 ) {
      	fprintf( stderr, "Usage:  %s [-kums] [file1 . . .]\n", argv[0]);
	exit( 1);
    }

    /* For each of the arguments in the command string */
    for ( i = 1; i < argc; i++) {
	if ( argv[i][0] == '-' ) {
	    /* options processing */
	    if ( file ) {
		gun = 0;
		uname = 0;
		mount_point = 0;
		stale = 0;
	    }
	    file = 0;
	    for ( j = 1; argv[i][j] != '\0'; j++)
	      switch ( argv[i][j]) {
		case 'k':	++gun; break;
		case 'u':	++uname; break;
		case 'm':	++mount_point; break;
		case 's':	++stale; ++mount_point; break;
		default:
		  fprintf( stderr,
			  "Illegal option %c ignored.\n",
			  argv[i][j]);
	      }
	    continue;
	} 
	else file = 1;

	/* convert the path into an inode, save result to mimic old
	 * behavior.
	 */
	vfs = NULL;
	result = path_to_inode( argv[i], &ina, mount_point, stale, &vfs);

	/* First print its name on stderr (so the output can
	 * be piped to kill) */
	fprintf( stderr, "%s: ", argv[i]);

	if ( result == -1 )
	  continue;

	/* Look for mounts on this file system if -m and we have a vfs pointer*/
	if (mount_point && (vfs != NULL)) {
		check_mounts(vfs, argv[i]);
	}

	/* then for each process */
	for ( j = 0; j < nproc; j++) {	
	    /* read in the per-process info */
	    rread( kmem, (long) (procptr + (j * sizeof(p))),
		  (char *) &p, sizeof(p));
	    if ( p.p_stat == 0 || p.p_stat == SZOMB ) continue;

	    /* Greater than 60 open files change. */
	    /* As of 8.0 a process's current directory 
	       (u.u_cdir), its root directory (u.u_rdir), 
	       it's open file table pointers (u.u_ofilep), 
	       and its limit on open files for a process 
	       (u.u_maxof) have been moved into the proc table 
	       since it is no longer possible to grab a process's
	       u-area when it has been swapped out. */
	    
	    if ( (p.p_cdir != 0) &&
#ifdef HP_NFS
		vnode_to_inode( p.p_cdir, &inb, kmem) == 0 )
#else
	        read_inode( p.p_cdir, &inb, kmem) == 0 )
#endif HP_NFS
	       if ( ieq( ina, inb) ) {
		   fprintf( stdout, " %7d", (int) p.p_pid);
		   fflush(stdout);
		   fprintf( stderr, "c");
		   if (uname) 
		     puname( p.p_uid);
		   if (gun) 
		     kill( p.p_pid, 9);
		   continue;
	       }

	   if ( (p.p_rdir != 0) &&
#ifdef HP_NFS
	      vnode_to_inode( p.p_rdir, &inb, kmem) == 0 )
#else
	      read_inode( p.p_rdir, &inb, kmem) == 0 )
#endif HP_NFS
             if ( ieq( ina, inb) ) {
		 fprintf( stdout, " %7d", (int) p.p_pid);
		 fflush(stdout);
		 fprintf( stderr, "r");
		 if (uname) 
		   puname( p.p_uid);
		 if (gun) 
		   kill( p.p_pid, 9);
		 continue;
	     }
	
	     /* then, for each file */
	
             if (p.p_ofilep != NULL) {
		 rread(kmem,(long)p.p_ofilep,(char *)ptr_chunk,sizeof(struct ofile_t *) * (NFDCHUNKS(p.p_maxof)));
		 for (k = 0; k < NFDCHUNKS(p.p_maxof); k++) {
		
		     if (ptr_chunk[k] == NULL)
		       continue;
		     else  {
			/*
			 * the content of ptr_chunk[k] might not be
			 * valid if the file is closed by the process
			 * before it gets read; so ignore the error 
			 * from lseek() and read() if the address is 
			 * no longer valid.
			 */
			 if (noerr_rread (kmem, (long)ptr_chunk[k],
				(char *)&file_chunk, sizeof(file_chunk)))
			    continue;
			 op = &file_chunk;
		     }
		     for (l = 0; l < SFDCHUNK; l++) {
			 if ( ! valid_file( op->ofile[l] ))
			   continue;
			 if (file_to_inode( op->ofile[l], &inb, kmem))
			   continue;
			 if ( ieq( ina, inb) ){
			     fprintf(stdout, " %7d", (int) p.p_pid);
			     fflush(stdout);
			     if (uname) 
			       puname( p.p_uid);
			     if (gun) 
			       kill( p.p_pid, 9);
			     goto found;
			 }
		     }
		 }
	     }

	     /* Check text, swapping and memory mapped files */
	     if (check_regions(&p, &ina)) {
	         fprintf(stdout, " %7d", (int) p.p_pid);
	         fflush(stdout);
		 if (uname)
		     puname( p.p_uid);
		 if (gun)
		     kill( p.p_pid, 9);
	     }
             found: ;
        }  /* for each process */

        fprintf( stderr, "\n");
    }  /* for each argument */
    printf( "\n");

    return(0);
}

/* code from V.2---modified slightly */
file_to_inode(file_adr, in, memdev)
    int memdev;
    struct inode *in;
    struct file *file_adr;
{
    /* Takes a pointer to one of the system's file entries
     * and copies the inode  to which it refers into the area
     * pointed to by "in". "file_adr" is a system pointer, usually
     * from the user area. */
    
    struct file f;
    union {                /* used to convert pointers to numbers */
	char    *ptr;
	unsigned addr; /* must be same length as char * */
    } convert;
    
    convert.ptr = (char *) file_adr;
    if ( convert.addr == 0 ) 
      return( -1);
    
    /* read the file table entry */

    rread( memdev, (long) convert.addr, (char *) &f, sizeof (f));
    if ( f.f_flag )
#ifdef HP_NFS
      return( vnode_to_inode( f.f_data, in, memdev));
#else
      return( read_inode( f.f_data, in, memdev));
#endif HP_NFS
    else return( -1);
}

#ifdef HP_NFS
vnode_to_inode(vnode_adr, in, memdev)
    int memdev;
    struct inode *in;
    struct vnode *vnode_adr;
{
    /* Takes a pointer to one of the system's vnode entries
     * and copies the inode  to which it refers into the area
     * pointed to by "in". "vnode_adr" is a system pointer. */
    
    struct vnode v;
    union {                /* used to convert pointers to numbers */
	char    *ptr;
	unsigned addr; /* must be same length as char * */
    } convert;

    struct cdnode cd;
    struct rnode rd;
    
    convert.ptr = (char *) vnode_adr;
    if ( convert.addr == 0 ) 
      return( -1);

    /* read the vnode  */

    rread( memdev, (long) convert.addr, (char *) &v, sizeof (v));
    if ( v.v_count != 0)
      if((v.v_fstype == VUFS) || (v.v_fstype == VDUX)) 
	return( read_inode( v.v_data, in, memdev));
      else 
	if ((v.v_fstype == VCDFS) || (v.v_fstype == VDUX_CDFS)) {
	    if(read_cdnode( v.v_data, &cd, memdev) == -1)
	      return(-1);
	    else {
		in->i_dev = cd.cd_dev;
		in->i_number = cd.cd_num;
		in->i_mode = cd.cd_mode;
		in->i_rdev = cd.cd_rdev;
		return(0);
	    }
	}
	else if (v.v_fstype == VNFS) {
	    int dev;

	    if(read_rnode( v.v_data, &rd, &dev, memdev) == -1)
	      return(-1);
	    else {
		in->i_dev = dev;
		in->i_number = rd.r_nfsattr.na_nodeid;
		in->i_mode = rd.r_nfsattr.na_mode;
		in->i_rdev = rd.r_nfsattr.na_rdev;
		return(0);
	    }
	}

    return(-1);
}
#endif HP_NFS

/* This routine fakes up an inode by reading /etc/mnttab and the vfs
 * list from the kernel.  It is used when we want to look at an NFS mounted
 * file system and the NFS server is not responding.
 */
get_stale_inode( path, in, vfs_return)
    char *path;
    struct inode *in;
    struct vfs **vfs_return;
{
    FILE *fp;
    struct mntent *mnt;
    char devname[MAXPATHLEN];
    char fstype[MAXPATHLEN];
    int found;
    union {                /* used to convert pointers to numbers */
	char    *ptr;
	unsigned addr; /* must be same length as char * */
    } convert;
    struct vfs vfs;
    struct mntinfo mi;

    fp = setmntent("/etc/mnttab", "r+");

    if (fp == NULL) return (-1);

    /* Search the mount table looking for the entry that matches the given one*/
    found = 0;
    while ((mnt = getmntent(fp)) != NULL) {
	/* Ignore swap and automounter mount points. */
        if (!strcmp(mnt->mnt_type, MNTTYPE_IGNORE) ||
            !strcmp(mnt->mnt_type, MNTTYPE_SWAP) ||
            !strcmp(mnt->mnt_type, MNTTYPE_SWAPFS))
                continue;

        if (check_paths(path, mnt->mnt_dir)) {
            found = 1;
	    strncpy(devname, mnt->mnt_fsname, MAXPATHLEN);
	    strncpy(fstype, mnt->mnt_type, MAXPATHLEN);
	    break;
        }
    }

    endmntent(fp);

    /* If we didn't find it then return -1 so the user gets an error message. */
    if (!found)
	return(-1);

    /* If its not NFS, we don't need to fake the inode information so let the
     * other routine handle it.  But this is the only way we can determine if
     * it is NFS if the server is down.
     */
    if (strcmp(MNTTYPE_NFS, fstype)) {
	return(1); }

    /* Have to special case the root file system */
    if (!strcmp(path, "/")) {
	strcpy(devname, "/dev/root");
    }

    /* Search the kernel vfs list looking for the entry that matches the
     * one from /etc/mnttab.
     */
    convert.ptr = (char *)rootvfs;
    found = 0;
    while (convert.ptr != 0) {
        if (rread( kmem, (long) convert.addr, (char *) &vfs,
						sizeof(struct vfs)))
            return( -1);
	
	if (check_paths(devname, vfs.vfs_name)) {
	    found = 1;
	    break;
	}
	convert.ptr = (char *)vfs.vfs_next;
    }

    /* This should never happen, but... */
    if (!found) return(-1);

    /* Some other routines need to know the vfs address so return it here. */
    *vfs_return = (struct vfs*)convert.ptr;

    /* Need to read the mountinfo struct to get the device number */
    convert.ptr = (char *) vfs.vfs_data;

    /* Check everything */
    if ( convert.addr == 0 ) 
      return( -1);
    
    /* read the mntinfo struct */
    if (rread( kmem, (long) convert.addr, (char *) &mi,
    					sizeof(struct mntinfo)))
      return(-1);
    
    /* Check everything */
    if (mi.mi_rootvp == 0)
      return(-1);

    /* Here we finally fake up the inode */
    in->i_mode = IFBLK;
    /* Only important field */
    in->i_dev = 0xff000000 | mi.mi_mntno;
    in->i_number = 0;
    in->i_rdev = in->i_dev;

    return(0);
}

path_to_inode( path, in, mount_point, stale, vfs_return)
    char *path;
    struct inode *in;
    int mount_point;
    struct vfs **vfs_return;
{	/* Converts a path to inode info by the stat system call */
    struct stat s;
#if defined DUX || defined DISKLESS
    FILE *mntfp;
    struct mntent *mntp;
    char pathp[MAXPATHLEN];
    char newpath[MAXPATHLEN];
    char save_fs[MAXPATHLEN];
    int found = 0;
    int myerror;
#endif DUX || DISKLESS
#ifdef hp9000s800
    int brmtdev;
    int lvm_maj;
#endif
    
#if defined DUX || defined DISKLESS
    /* First just try path as is. */
    myerror = 0;
    if (stale) {
	int result;
	result = get_stale_inode(path, in, vfs_return);

	/* If we succeeded getting the data or we had a problem return.
	 * The other case is where -s was given, but it wasn't an NFS
	 * file system.  Go ahead and use the other methods then.
	 */
	if ((result == 0) || (result == -1)) {
		if (result == -1) {
    			fprintf( stderr, "Could not read\n");
		}
		return(result);
	}
    }

    if ( stat( path, &s) == 0 ){ 
	if (mount_point == 0) {
	    in->i_mode = s.st_mode;
	    in->i_dev = s.st_dev;
	    in->i_number = s.st_ino;
	    in->i_rdev = s.st_rdev;
	}
	else {
	    struct inode a;

	    /* Use this mount point */
	    in->i_mode = IFBLK;
	    in->i_rdev = s.st_dev;

	    /* Shorten the path name back to the actual mount point.  We do
	     * that by taking off the last component of the path and stat'ing
	     * the result.  When the dev changes we know we went too far.
	     * Notice this doesn't work well for symbolic links.  Another
	     * option would be to use a combination of the kernel vfs list
	     * with /etc/mnttab to determine the mount point.
	     */

	    for (;;) {
		char new_path[MAXPATHLEN];
		int i;

		strcpy(new_path, path);
		for (i = strlen(new_path); i > 0; i--) {
			if (new_path[i] == '/') {
				new_path[i] = '\0';
				break;
			}
		}

		/* Got down to "/" */
		if (i == 0) {
			if (new_path[0] == '/') {
				new_path[1] = '\0';
			}
			else {
				break;
			}
		}

    		if ( stat( new_path, &s) != 0 ) { 
			break;
		}

		if (in->i_rdev != s.st_dev) {
			break;
		}
		strcpy(path, new_path);

		/* Stop at "/" */
		if (strlen(path) == 1)
			break;
	    }

	    /* Just want the vfs */
	    return(get_stale_inode(path, &a, vfs_return));
	}

	/* If we are on a cluster, it could be that path is a filesystem, 
	   which doesn't reside on our local system.  If so, we have to 
	   manually put	in->i_mode = IFBLK and in->i_rdev = s.st_dev into
	   our inode structure.  Let's look into /etc/mnttab to see if this
	   is a filesystem.*/
	if (cnodes((cnode_t *) 0)) { 
	    /* If the filesystem is not local to this system, look for 
	       entries in /etc/mnttab which match path.	*/
	    mntfp = setmntent("/etc/mnttab", "r+");
	    if (mntfp != NULL) {
		while ((mntp = getmntent(mntfp)) != NULL) {
		    if (check_paths(mntp->mnt_fsname, path)) {
			if ( stat(mntp->mnt_dir, &s) == 0) {
			    in->i_mode = IFBLK;
			    in->i_rdev = s.st_dev;
			    endmntent(mntfp);
			    break;
			}
			else return(-1);
		    }
		}
	    }
	}
	
    }
    else {
	/* Save errno, we may need it later. */
	myerror = errno;
	
	if (cnodes((cnode_t *) 0)) { 
	    /* If the filesystem is not local to this system, look for 
	       entries in /etc/mnttab which match path.  Since 8.0 entries 
	       in /etc/mnttab have changed from the /dev/dsk/cXdYsZ format 
	       to the /dev+/<context>/dsk/cXdYsZ format.  So, we will match 
	       path to /dev+/<anything>/dsk/cXdYsZ.  If we find more than
	       one match in mnttab, ask the user to be more specific about
	       path. */
	    *pathp = NULL;
	    mntfp = setmntent("/etc/mnttab", "r+");
	    if (mntfp != NULL) {
		found = 0;
		while ((mntp = getmntent(mntfp)) != NULL) {
		    convert_string(mntp->mnt_fsname,newpath);
		    if (check_paths(newpath, path)) {
			found++;
			if (found == 2) {
			    printf("Ambigious path name.\n");
			    printf("%s matches the following files in /etc/mnttab.\n",path);
			    printf("\t%s\n\t%s\n",save_fs,mntp->mnt_fsname);
			}
			if (found > 2) 
			  printf("\t%s\n",mntp->mnt_fsname);
			strcpy(pathp,mntp->mnt_dir);
			strcpy(save_fs,mntp->mnt_fsname);
		    }
		}
		if (found > 1) {
		    printf("Please specify the complete path name as shown in /etc/mnttab.\n");
		    endmntent(mntfp);
		    return(-1);
		}
		
	    }
	    if (*pathp == NULL) {
		printf("%s\n",strerror(myerror));
		endmntent(mntfp);
		return(-1);
	    }
	    if ( stat( pathp, &s) == -1 ) {
	      	perror( "");
		endmntent(mntfp);
		return( -1);
	    }
	    if (found == 1) {
		in->i_mode = IFBLK;
		in->i_rdev = s.st_dev;
	    }
	    else {
	    	in->i_mode = s.st_mode;
	    	in->i_rdev = s.st_rdev;
	    }
	    in->i_dev = s.st_dev;
	    in->i_number = s.st_ino;
	    endmntent(mntfp);
	}
    }
#else
    
    if ( stat( path, &s) == -1 ) {
	perror( "");
	return( -1);
    }
    in->i_mode = s.st_mode;
    in->i_dev = s.st_dev;
    in->i_number = s.st_ino;
    in->i_rdev = s.st_rdev;
#endif DUX || DISKLESS
    
#if defined (hp9000s800)
    if(sysconf(_SC_IO_TYPE) == IO_TYPE_SIO) {
      /* 
       * If we are on the system where the device physically resides, then
       * replace the lu in i_dev and i_rdev with the associated
       * manager index.  If the inode is for a regular file which resides
       * on an lvm device, don't do the conversion.  If the inode is for a 
       * block or char special device which is lvm, don't do the conversion.
       */
      rread(kmem, (long)nl[X_BRMTDEV].n_value, (char *)&brmtdev, sizeof(int));
      rread(kmem, (long)nl[X_LVMMAJ].n_value, (char *)&lvm_maj, sizeof(int));
      if ((in->i_mode & IFMT) == IFBLK) {
	if ((brmtdev !=  major(in->i_rdev)) && 
	    (lvm_maj != major(in->i_rdev)))
	  lu_to_mi(&(in->i_rdev)); 
      }
      if ((brmtdev != major(in->i_dev)) && (lvm_maj != major(in->i_dev)))
	lu_to_mi(&(in->i_dev));
    }
#endif
    return( 0);
}

puname(uid)
    uid_t uid;
{	
    struct passwd *getpwuid(), *p;
	
    p = getpwuid( uid);
    if ( p == NULL ) 
      return;
    fprintf( stderr, "(%s)", p->pw_name);
}

read_inode( inode_adr, i, memdev)
    int memdev;
    struct inode *i, *inode_adr;
{
    /* Takes a pointer to one of the system's inode entries
     * and reads the inode into the structure pointed to by "i" */
    
    union {                /* used to convert pointers to numbers */
	char    *ptr;
	unsigned addr; /* must be same length as char * */
    } convert;
    
    convert.ptr = (char *) inode_adr;

    /* 
     * make sure the inode addr that passed in from the caller
     * is in the reasonable boundary
     */
    if (! valid_inode (convert.addr))
      return( -1);
    
    /* read the inode */

    rread( memdev, (long) convert.addr, (char *) i, sizeof (struct inode));

#ifndef HP_NFS /* count is checked in vnode_to_inode() if HP_NFS is defined */

    if(i->i_count == 0)
      return(-1);
#endif
    return( 0);
}

read_cdnode( cdnode_adr, c, memdev)
    int memdev;
    struct cdnode *c, *cdnode_adr;
{
    /* Takes a pointer to one of the system's cdnode entries
     * and reads the cdnode into the structure pointed to by "c" */
    
    union {                /* used to convert pointers to numbers */
	char    *ptr;
	unsigned addr; /* must be same length as char * */
    } convert;
    
    convert.ptr = (char *) cdnode_adr;
    if ( convert.addr == 0 ) 
      return( -1);
    
    /* read the cdnode */
    
    rread( memdev, (long) convert.addr, (char *) c, sizeof(struct cdnode));
    /* If rread failed, the whole cdnode struct will be zeroed out, so
       check a field that will never be zero unless there was a read 
       error. */
    
    if (c->cd_num == 0)
      return(-1);
    else
      return( 0);
}

read_rnode( rnode_adr, r, dev, memdev)
    struct rnode *r, *rnode_adr;
    int *dev;
    int memdev;
{
    /* Takes a pointer to one of the system's rnode entries
     * and reads the rnode into the structure pointed to by "r" */
    
    union {                /* used to convert pointers to numbers */
	char    *ptr;
	unsigned addr; /* must be same length as char * */
    } convert;
    struct vfs vfs;
    struct mntinfo mi;
    
    convert.ptr = (char *) rnode_adr;
    if ( convert.addr == 0 ) 
      return( -1);
    
    /* read the rnode */
    
    if (rread( memdev, (long) convert.addr, (char *) r, sizeof(struct rnode)))
      return( -1);

    /* Need to read the vfs */
    convert.ptr = (char *) r->r_vnode.v_vfsp;
    if ( convert.addr == 0 ) 
      return( -1);
    
    /* read the vfs */
    
    if (rread( memdev, (long) convert.addr, (char *) &vfs, sizeof(struct vfs)))
      return( -1);
    
    if (vfs.vfs_op == 0)
      return(-1);

    /* Need to read the mountinfo struct to get the device number */
    convert.ptr = (char *) vfs.vfs_data;
    if ( convert.addr == 0 ) 
      return( -1);
    
    /* read the mntinfo struct */
    
    if (rread( memdev, (long) convert.addr, (char *) &mi, sizeof(struct mntinfo)))
      return(-1);
    
    if (mi.mi_rootvp == 0)
      return(-1);

    *dev = 0xff000000 | mi.mi_mntno;

    return(0);
}


getkvars()
    /*  This routine reads in the system variables needed by this command  */
{
    
    /*
     * read to find proc table size
     */
    rread(kmem, (long)nl[X_NPROC].n_value, (char *)&nproc, sizeof(nproc));
    /*
     * Locate proc table
     */
    rread(kmem, (long)nl[X_PROC].n_value,  (char *)&procptr,
	  sizeof(procptr));
    rread(kmem, (long)nl[X_FILE].n_value,  (char *)&fileptr,
	  sizeof(fileptr));
    rread(kmem, (long)nl[X_NFILE].n_value, (char *)&endfile,
	  sizeof(endfile));
    rread(kmem, (long)nl[X_INODE].n_value, (char *)&inodeptr,
	  sizeof(inodeptr));
    rread(kmem, (long)nl[X_NINODE].n_value,	(char *)&endinode,
	  sizeof(endinode));
    rread(kmem, (long)nl[X_ROOTVFS].n_value,	(char *)&rootvfs,
	  sizeof(rootvfs));
}

/*  code from V.2 fuser  */

/* Hey, it now returns -1 if it fails so we can simply check the return code.
 * What a concept.
 */
rread( device, position, buffer, count)
    char *buffer;
    int count, device;
    long position;
{	
    /* Seeks to "position" on device "device" and reads "count"
     * bytes into "buffer". Zeroes out the buffer on errors.
     */
    int i;
    long lseek();
    
    if ( lseek( device, position, 0) == (long) -1 ) {
	fprintf( stderr, "Seek error for file number %d: ", device);
	perror( "");
	for ( i = 0; i < count; buffer++, i++) 
	  *buffer = '\0';
	return -1;
    }
    if ( read( device, buffer, (unsigned) count) == -1 ) {
	fprintf( stderr, "Read error for file number %d: ", device);
	perror( "");
	for ( i = 0; i < count; buffer++, i++) 
	  *buffer = '\0';
	return -1;
    }
    return 0;
}

/*
 * noerr_rread(): a clone routine of rread() without showing 
 * the error messages.
 */
noerr_rread( device, position, buffer, count)
    char *buffer;
    int count, device;
    long position;
{	
    /* Seeks to "position" on device "device" and reads "count"
     * bytes into "buffer". Zeroes out the buffer on errors.
     */
    int i;
    long lseek();
    
    if ( lseek( device, position, 0) == (long) -1 ) {
	for ( i = 0; i < count; buffer++, i++) 
	  *buffer = '\0';
	return -1;
    }
    if ( read( device, buffer, (unsigned) count) == -1 ) {
	for ( i = 0; i < count; buffer++, i++) 
	  *buffer = '\0';
	return -1;
    }
    return 0;
}

#if defined(hp9000s800)
#define FAILURE		1  /* SUCCESS is defined in libIO.h */
#define LU_MASK         0x0000ff00
#define MI_MASK         0x0000ff00
#define LU_SHIFT        8
#define MI_SHIFT        8
#define LU(dev)         ((dev & LU_MASK) >> LU_SHIFT)
#define MI(dev)         ((dev & LU_MASK) >> LU_SHIFT)
  

int lu_to_mi(devp)
    dev_t	 *devp;
{
    io_node_type  io_node;
    int		  status, key;
    
    if ((status = io_init(O_RDONLY)) != SUCCESS) {
	(void)print_libIO_status("fuser", _IO_INIT, status, O_RDONLY);
	return(FAILURE);
    }
    
    /* Set the search key */
    key = KEY_B_MAJOR;
    io_node.b_major = major(*devp);
    
    /* Copy the lu into the io_node to qualify the search */
    io_node.lu = LU(*devp);
    
    if ((status = io_search_tree(SEARCH_SINGLE, key, QUAL_LU, &io_node)) < 0) {
	io_end();
	return(print_libIO_status("fuser", _IO_SEARCH_TREE, status,
				  SEARCH_SINGLE, key, QUAL_LU, &io_node));
    }
    if (io_node.mgr_index == NONE) {
	fprintf(stderr, "No associated manager index for major %d lu %d\n",
		io_node.b_major, io_node.lu);
	exit(1);
    }
    
    *devp = (*devp & ~MI_MASK) | (io_node.mgr_index << MI_SHIFT);
    io_end();
    return(SUCCESS);
}

#endif /* hp9000s800 */

convert_string(old_path,newpath)
    char *old_path;
    char *newpath;
{
    char path[MAXPATHLEN];
    char *s1;
    char *s2;
    char c;
    
    strcpy(path,old_path);
    c = '+';
    if ((s1 = strchr(path,c)) == NULL) {
	*newpath = NULL;
	return;
    }	
    strcpy(newpath,s1);
    *s1 = NULL;
    c = '/';
    s1 = strchr(newpath, c);
    s2 = &s1[1];
    s1 = strchr(s2, c);
    strcpy(newpath,strcat(path,s1));
}

/* This routine searchs the region list for the given process looking for
 * regions that match the given inode.  Note that it really isn't always an
 * inode, but we'll all pretend.  And it does work.
 */
int check_regions(uproc, ina)
        struct proc  *uproc;
	struct inode *ina;
{
        struct vas v;
        struct pregion p, *prp;
	struct inode inb;
        struct region r;
        int    sanity=1000;

        /* Make sure it exists */
        if (uproc->p_vas == NULL) {
                return(0);
        }

	/* Read the vas */
	if (rread(kmem, (long)uproc->p_vas, (char *)&v, sizeof(v))) {
                return(0);
        }

	/* search the list of pregions */
        prp = v.va_next;
        while (prp != (struct pregion *) uproc->p_vas) {

                /* Make sure we don't loop forever */
                if (--sanity == 0) {
                        fprintf(stderr,
                        "infinite loop reading regions for pid %d\n",
                                                        uproc->p_pid);
			return(0);
                }

                /* Read the pregion */
                if (rread(kmem,  (int)prp, (char *)&p, sizeof(p))) {
			return(0);
                }

                /* Read the region */
                if (rread(kmem, (int)p.p_reg, (char *)&r, sizeof(r))) {
                        return(0);
                }

                /* See if we are swapping onto file or file system */
                if (r.r_bstore &&
			vnode_to_inode( r.r_bstore, &inb, kmem) == 0 ) {
	       			if ( ieq( *ina, inb) )
					return(1);
                }

		/* See if we're swapping from the file or file system */
                if (r.r_fstore &&
			vnode_to_inode( r.r_fstore, &inb, kmem) == 0 ) {
	       			if ( ieq( *ina, inb) )
					return(1);
                }

                /* Do the next pregion in the chain */
                prp = p.p_next;
        }
        return(0);
}

/* This routine looks for file systems that are mounted on the given file
 * system.
 */
int check_mounts(vfs, file_system)
	struct vfs  *vfs;
	char *file_system;
{
	struct vfs vfs_entry;
	struct vfs check_vfs;
	struct vfs *vfs_addr;

	/* Get the entire vfs structure we're looking for */
        if (rread(kmem, (int)vfs, &check_vfs, sizeof(check_vfs))) {
		return;
	}

	/* Now search the vfs list looking for mounts on this mount point */
	vfs_addr = (struct vfs *)rootvfs;
	while (vfs_addr) {
        	if (rread(kmem, (int)vfs_addr, &vfs_entry, sizeof(vfs_entry))) {
			return;
		}

		/* If the covered vnode is on our file system, then we know
		 * that this file system is mounted on ours.
		 */
		if (vfs_entry.vfs_vnodecovered &&
			check_vnode(vfs_entry.vfs_vnodecovered, vfs)) {
			char *mnttab=MNT_MNTTAB;
			struct mntent *mnt;
			FILE *fp;
			int found=0;

			/* Look for the file system in /etc/mnttab so we can
	 		 * get the mounted on directory.
	 		 */
			if ((fp = setmntent(mnttab, "r")) == NULL) {
				return;
			}
			while ((mnt = getmntent(fp)) != NULL) {
				/* We allow MNTTYPE_IGNORE because of the
				 * automounter.
				 */
				if (!strcmp(mnt->mnt_type, MNTTYPE_SWAP) ||
		    		    !strcmp(mnt->mnt_type, MNTTYPE_SWAPFS))
					continue;

				if (check_paths(mnt->mnt_fsname, vfs_entry.vfs_name)) {

					fprintf(stderr, "File system %s mounted on %s (%s)\n",
						mnt->mnt_fsname, file_system,
						mnt->mnt_dir);
					found++;
					break;
				}
			}

			/* Make sure it gets printed even we didn't
			 * find it in /etc/mnttab.
			 */
			if (!found)
				fprintf(stderr,"File system %s mounted on %s\n",
						vfs_entry.vfs_name, file_system);

			/* Close the mnttab file */
			endmntent(fp);
		}
		vfs_addr = vfs_entry.vfs_next;
	}
}

int check_vnode(vp, vfs)
        struct vnode *vp;
        struct vfs   *vfs;
{
        struct vnode vnode;

        /* Read the vnode out of kmem */
        if (rread(kmem, (int)vp, &vnode, sizeof(vnode))) {
                return(0);
        }

	/* Check if the vfs pointers match */
        return (vnode.v_vfsp == vfs);
}

/* This routine does a smart check of two paths to see if they match.  It
 * allows the user to be sloppy in typing in path names and still have it
 * found in /etc/mnttab.  It also helps if things like rpc.mountd don't
 * use clean names.
 */
int check_paths(p1, p2)
	char *p1;
	char *p2;
{
	char first[MAXPATHLEN];
	char second[MAXPATHLEN];
	char *strchr();

	char *ptr1, *ptr2;

	/* Make sure that they both start with the same character.  This is
	 * because the checks below strip off the first '/' and if they would
	 * not notice that one had a '/' and one didn't.
	 */
	if (*p1 != *p2) return(0);

	for (;;) {

		/* Get rid of multiple '/' characters */
		while (*p1 == '/') p1++;
		while (*p2 == '/') p2++;

		/* If the first is at end of string then if the second
		 * path is also end of string we have succeeded.  Otherwise
		 * we failed.
		 */
		if (*p1 == '\0') {
			return(*p2 == '\0');
		}

		if (*p2 == '\0') {
			return 0;
		}

		/* Get the next component of each path name */
		ptr1 = strchr(p1, '/');
		if (ptr1 == NULL) {
			strcpy(first, p1);
			p1 += strlen(p1);
		}
		else {
			*ptr1 = '\0';
			strcpy(first, p1);
			p1 = ptr1+1;
			*ptr1 = '/';
		}

		ptr2 = strchr(p2, '/');
		if (ptr2 == NULL) {
			strcpy(second, p2);
			p2 += strlen(p2);
		}
		else {
			*ptr2 = '\0';
			strcpy(second, p2);
			p2 = ptr2+1;
			*ptr2 = '/';
		}

		/* If these components don't match, then fail the whole test */
		if (strcmp(first, second)) return(0);
	}
}
