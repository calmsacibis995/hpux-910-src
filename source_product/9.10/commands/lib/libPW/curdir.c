/* @(#) $Revision: 70.1 $ */      

# include	"sys/types.h"
# include	<sys/stat.h>

#include <stdio.h>
#include "sys/param.h"
#include <mntent.h>
#include <ndir.h>
/*
	current directory.
	Places the full pathname of the current directory in `str'.
	Handles file systems not mounted on a root directory
	via /etc/mtab (see mtab(V)).
	NOTE: PWB systems don't use mtab(V), but they don't mount
	file systems anywhere but on a root directory (so far, at least).

	returns 0 on success
	< 0 on failure.

	Current directory on return:
		success: same as on entry
		failure: UNKNOWN!
*/
static char *curdirp;

static char	*flg[] = {
	"read/write",
	"read only"
	};

static struct mntent *mp;

curdir(str)
char *str;
{
	register int n;

	curdirp = str;
	n = findir(0);
	return(n+chdir(str));
}


# define ADDSLASH	if (flag) *curdirp++ = '/';
# define QUIT		{ return(-1); }
# define STOP		{ closedir(dirp); return(-1); }

findir(flag)
{
	register DIR *dirp;
	register int inum;
	register char *tp;
	char *slashp, tmp[MAXPATHLEN];
	int dev, r;
	FILE *fpntr, *fopen();	        /* Used to check mnttab */
	struct direct *entry;
	struct stat s;
	struct stat inode_stat;		/* only to get current test inode */
	static struct stat nextstat;	/* stat of parent directory */
	char	fname[MAXNAMLEN];	/* holds current directory name */
					/* through recursion */

	if (flag)	/* not initial calling, stat known from last recurse */
		s = nextstat;
	else		/* initial calling, get current directory stat */
		if (stat (".", &s) < 0)
			return(-1);

	if (stat ("..", &nextstat) < 0)
		return(-1);

	if ((inum = s.st_ino) == nextstat.st_ino) {	/* root is own parent */
		dev = s.st_dev;
		if ((dirp = opendir("/")) == (DIR *)NULL) return(-1);
		if (fstat(dirp->dd_fd,&s)<0)
			STOP;
		if (dev == s.st_dev) {
			*curdirp++ = '/';
			*curdirp = 0;
			closedir(dirp);
			return(0);
		}
		while ((entry = readdir(dirp)) != (struct direct *)NULL) {
			if (entry->d_ino == 0) continue;
			slashp = &entry->d_name[-1]; /* prepend a '/' to name */
			*slashp = '/';
			if (stat(slashp,&s)<0) continue;
			if (s.st_dev != dev) continue;
			if ((s.st_mode&S_IFMT) != S_IFDIR) continue;
			for (tp = slashp; *curdirp = (*tp++); curdirp++);
			ADDSLASH;
			*curdirp = 0;
			closedir(dirp);
			return(0);
		}
		closedir(dirp);

		if (( fpntr = fopen( MNT_MNTTAB, "r")) == NULL)
		  {
		    fprintf(stderr,"curdir: cannot open %s\n",MNT_MNTTAB);
		    return(-1);
		  }
		while ( (mp = getmntent( fpntr )) != NULL )
		  {
		    if ( mp->mnt_fsname != NULL )
			{
			  sprintf(tmp,"%s", mp->mnt_fsname);
			  if ( stat(tmp,&s) < 0 )
			    QUIT;
			  if ( s.st_rdev != dev ) continue;
			  /* Had to alter mt_filesys to mnt_dir */
			  for ( tp = mp->mnt_dir; *curdirp = (*tp++); curdirp++ );
			  ADDSLASH;
			  fclose( fpntr );
#ifdef DEBUG
			  printf("%s\n\t%s\n\t%s\n\t%s\n\t%d\n\t%d\n\t%ld\n\n",
			           mp->mnt_fsname, mp->mnt_dir,
			           mp->mnt_type, mp->mnt_opts,
			           mp->mnt_freq, mp->mnt_passno,
			           mp->mnt_time);
#endif
				return(0);
			}
		}
	
		QUIT;
	}

	if ((dirp = opendir("..")) == (DIR *)NULL) return(-1);

	do {  /* search for name with same inode as ".." from previous stat */
		if ((entry = readdir (dirp)) == (struct direct *)NULL)
			STOP
		sprintf (tmp, "../%s", entry->d_name);
		if (stat (tmp, &inode_stat) < 0)
			STOP
	}
	while (inode_stat.st_ino != inum);

	closedir(dirp);
	strcpy (fname, entry->d_name);	/* preserve current directory name */
				/* (*entry) is overwritten in recursion */
	if (chdir("..")<0) return(-1);
	if (findir(-1)<0) r = -1;
	else r = 0;
	for (tp = fname ; *curdirp = *tp++ ; curdirp++);
	ADDSLASH;
	*curdirp = 0;
	return(r);
}
