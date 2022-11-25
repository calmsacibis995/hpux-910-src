/* @(#) $Revision: 56.1 $ */   
/*
	Process semaphore.
	Try repeatedly (`count' times) to create `lockfile' mode 444.
	Sleep 10 seconds between tries.
	If `tempfile' is successfully created, write the process ID
	`pid' in `tempfile' (in binary), link `tempfile' to `lockfile',
	and return 0.
	If `lockfile' exists and it hasn't been modified within the last
	minute, and either the file is empty or the process ID contained
	in the file is not the process ID of any existing process,
	`lockfile' is removed and it tries again to make `lockfile'.
	After `count' tries, or if the reason for the create failing
	is something other than EACCES, return xmsg().
 
	Unlockit will return 0 if the named lock exists, contains
	the given pid, and is successfully removed; -1 otherwise.
*/

# include	"sys/types.h"
# include	"macros.h"
# include	"errno.h"

lockit(lockfile,count,pid)
register char *lockfile;
register unsigned count;
unsigned pid;
{
	register int fd;
	int ret;
	unsigned opid;
	long ltime;
	long omtime;
	extern int errno;
	static char tempfile[512];
	char	dir_name[512];

	copy(lockfile,dir_name);

	sprintf(tempfile,"%s/%u.%ld",                   /* Make a lock file name */
			dname(dir_name),                        /* directory name */
			pid,                                    /* Process dependant id */
			(time((long *)0) % (long) 1000000));    /* Time dependant id */
                                     /* ^ The mod is to insure the file */
									 /* name remains less than 14 chars */
/* if the directory is not writeable, then we can return immediately -
   we do not need to wait a few minutes */

	if (canwritedir(dir_name) == -1)    /* access(2) doesn't work here */
		return(EACCES);             /* because it uses real uid's - */
					    /* this must work for effective */
					    /* uid's as well */

	for (++count; --count; sleep(10)) {
		if (onelock(pid,tempfile,lockfile) == 0)
			return(0);
		if (!exists(lockfile))
			continue;
		omtime = Statbuf.st_mtime;
		if ((fd = open(lockfile,0)) < 0)
			continue;
		ret = read(fd,&opid,sizeof(opid));
		close(fd);
		if (ret != sizeof(pid) || ret != Statbuf.st_size) {
			unlink(lockfile);
			continue;
		}
		/* check for pid */
		if (kill(opid,0) == -1 && errno == ESRCH) {
			if (exists(lockfile) &&
				omtime == Statbuf.st_mtime) {
					unlink(lockfile);
					continue;
			}
		}
		if ((ltime = time((long *)0) - Statbuf.st_mtime) < 60L) {
			if (ltime >= 0 && ltime < 60)
				sleep(60 - ltime);
			else
				sleep(60);
		}
		continue;
	}
	return(-1);
}


unlockit(lockfile,pid)
register char *lockfile;
unsigned pid;
{
	register int fd, n;
	unsigned opid;

	if ((fd = open(lockfile,0)) < 0)
		return(-1);
	n = read(fd,&opid,sizeof(opid));
	close(fd);
	if (n == sizeof(opid) && opid == pid)
		return(unlink(lockfile));
	else
		return(-1);
}


onelock(pid,tempfile,lockfile)
unsigned pid;
char *tempfile;
char *lockfile;
{
	int	fd;
	extern int errno;

	if ((fd = creat(tempfile,0444)) >= 0) {
		write(fd,&pid,sizeof(pid));
		close(fd);
		if (link(tempfile,lockfile) < 0) {
			unlink(tempfile);
			return(-1);
		}
		unlink(tempfile);
		return(0);
	}
	if (errno == ENFILE) {
		unlink(tempfile);
		return(-1);
	}
	if (errno != EACCES)
		return(xmsg(tempfile,"lockit"));
	return(-1);
}


mylock(lockfile,pid)
register char *lockfile;
unsigned pid;
{
	register int fd, n;
	unsigned opid;

	if ((fd = open(lockfile,0)) < 0)
		return(0);
	n = read(fd,&opid,sizeof(opid));
	close(fd);
	if (n == sizeof(opid) && opid == pid)
		return(1);
	else
		return(0);
}

static int canwritedir(name)            /* check write permission on a */
char *name;                             /* directory using euid and egid */
{
    struct stat buf;

    if (stat(name, &buf) != 0)      /* if stat fails, interpret that */
	return(-1);                 /* as "not writeable" */

    if (buf.st_uid == geteuid())        /* if owner of file */
	if (buf.st_mode & 0200)
	    return(0);
	else                        /* if owner does not have write */
	    return(-1);             /* permission, then fail */

    if (buf.st_gid == getegid())        /* if growner of file */
	if (buf.st_mode & 020)
	    return(0);
	else                        /* if growner doesn't have write */
	    return(-1);             /* permission, then fail also */

    if (buf.st_mode & 02)           /* if stranger */
	return(0);
    else
	return(-1);
}
