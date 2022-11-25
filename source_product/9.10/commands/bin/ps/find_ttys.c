/* $Source: /misc/source_product/9.10/commands.rcs/bin/ps/find_ttys.c,v $
 * $Revision: 70.1 $            $Author: ssa $
 * $State: Exp $               $Locker:  $
 * $Date: 91/09/10 16:36:40 $
 */
#ifndef lint
static char *HPUX_ID = "@(#) find_ttys.c  $Revision: 70.1 $ $Date: 91/09/10 16:36:40 $";
#endif

/* This file defines a set of functions used to map tty devices into
 * tty file names, which are printed for the user.  These functions are
 * used both by ps and by top.  This file makes the tty mapping independent
 * of the rest of the application.  These functions also extend the
 * current ps definition of having all controlling tty devices under the
 * /dev directory, to having them in any of several pre-defined directories.
 * Finally, it extends the definition to include up to 3300 character
 * device files, since that is the recently extended upper limit on the
 * number of pty's in the system (2900 + slosh).  The number of device
 * entries should probably be dynamically allocated, so as not to waste
 * the space, but that will be left as a later extension.
 *
 *			Jon Bayh, 3/19/89
 */

#include <stdio.h>

#ifdef BUILDFROMH

#include <h/param.h>
#include <h/stdsyms.h>
#include <h/types.h>
#include <h/stat.h>
#include <h/dir.h>

#else  BUILDFROMH

#include <sys/param.h>
#include <sys/stdsyms.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#endif BUILDFROMH

char *malloc();

/* Define a list of directories in which to search for tty names.  This
 * list also defines some directories which are required, and some which
 * are not.  If required directories do not exist in the system, the
 * program aborts.  "/dev" is the only current required directory.
 */

struct ttydirs {
	char * name;
	int    required;
};

struct ttydirs ttydir_list [] = {
	"/dev", 1,
	"/dev/pty", 0,
	NULL, 0
};

#define NDEV	4096
int	number_of_devices = 0;
struct devl {
	int	name_index;
	dev_t	dev;
} devl[NDEV];

int name_buffer_max_size = 0;
char *name_buffer = NULL;

static long cons_dev;

/* ttydirs_times_ok - make sure that the last-modify-time and creation-time
 * for each of the dev directories are older than the given timestamp,
 * that is, the timestamp of the psdata file.  If any are not, return
 * 0.  If a directory is required and we can't stat it, return 0.  If
 * a directory is not required and we can't stat it, ignore it.
 */

ttydirs_times_ok (psfile_timestamp)
	time_t psfile_timestamp;
{
	struct ttydirs *ptr;
	int ret;
	struct stat sbuf;

	ptr = ttydir_list;
	for ( ; ptr->name != NULL ; ptr++) {
		ret = stat (ptr->name, &sbuf);
		if ((ret < 0) && ptr->required)
			return (0);
		if (ret < 0)
			continue;
		if (psfile_timestamp <= sbuf.st_mtime)
			return (0);
		if (psfile_timestamp <= sbuf.st_ctime)
			return (0);
	}
	return (1);
}

/* read_tty_data - given a file descriptor, read the tty data from that
 * file.
 */

read_tty_data (fd)
	int fd;
{
	int ret;
	int readndev;
	int where_am_i;
	int readcount;

	if (fd < 0) {
		return (0);
	}
	ret = read (fd, &number_of_devices, sizeof (number_of_devices));
	if (ret < sizeof (number_of_devices)) {
		fprintf (stderr, "read_tty_data: could not read number of tty entries.\n");
		return (0);
	}

	if (number_of_devices > NDEV) {
		fprintf (stderr, "read_tty_data: tty device count (%d) is greater than max allowed (%d).\n",
			number_of_devices, NDEV);
		readndev = NDEV;
		where_am_i = lseek (fd, 0, 1);
	} else {
		readndev = number_of_devices;
	}

	readcount = readndev * sizeof (*devl);
	ret = read (fd, devl, readcount);
	if (ret < readcount)  {
		fprintf (stderr, "read_tty_data: read of tty device descriptors failed\n");
		return (0);
	}

	if (number_of_devices > NDEV) {
		ret = lseek (fd, where_am_i + (number_of_devices * sizeof (*devl)), 0);
		if (ret == -1) {
			fprintf (stderr, "read_tty_data: lseek to end of tty device descriptors failed\n");
			return (0);
		}
		number_of_devices = NDEV;
	}

	ret = read (fd, &name_buffer_max_size, sizeof (name_buffer_max_size));
	if (ret < 0) {
		fprintf (stderr, "read_tty_data: read of tty name buffer size failed.\n");
		return (0);
	}

	name_buffer = malloc (name_buffer_max_size);
	if (name_buffer == NULL) {
		fprintf (stderr, "read_tty_data: malloc of %d bytes for name buffer failed\n",
			name_buffer_max_size);
		return (0);
	}

	ret = read (fd, name_buffer, name_buffer_max_size);
	if (ret < 0) {
		fprintf (stderr, "read_tty_data: read of name buffer failed.\n");
		return (0);
	}
	return (1);
}

write_tty_data (fd)
	int fd;
{
	int ret;

	ret = write(fd, &number_of_devices, sizeof(number_of_devices));
	if (ret < 0) {
		fprintf (stderr, "write_tty_data: write of tty count failed\n");
		return (0);
	}

	ret = write(fd, devl, number_of_devices * sizeof(*devl));
	if (ret < 0) {
		fprintf (stderr, "write_tty_data: write of tty descriptors failed\n");
		return (0);
	}

	ret = write(fd, &name_buffer_max_size,
		sizeof(name_buffer_max_size));
	if (ret < 0) {
		fprintf (stderr, "write_tty_data: write of name buffer count failed\n");
		return (0);
	}

	ret = write(fd, name_buffer, name_buffer_max_size);
	if (ret < 0) {
		fprintf (stderr, "write_tty_data: write of name buffer failed\n");
		return (0);
	}
	return (1);
}

set_console_device (device)
	long device;
{
	cons_dev = device;
}

get_tty_devices ()
{

	struct stat sbuf;
	struct direct *dp;
	DIR *dirp;
	register ino_t inode;
	register char *curptr;
	register char *endptr;
	register int   length;
	register ino_t syscon_inode;
	register ino_t systty_inode;
	struct ttydirs *ptr;
	int ret;


	name_buffer_max_size;
	for (ptr = ttydir_list; ptr->name != NULL; ptr++) {
		ret = stat (ptr->name, &sbuf);
		if (ret < 0 && ptr->required) {
			fprintf (stderr, "get_tty_devices: Required directory %s does not exist.\n",
				ptr->name);
			return (0);
		}
		if (ret < 0)
			continue;
		/* The list of names of tty devices can't possibly be longer
		 * than the sum of the sizes of the directories containing
		 * tty device filenames, so use this as an upper bound.
		 */
		name_buffer_max_size += sbuf.st_size;
	}
	name_buffer = malloc (name_buffer_max_size + 1);
	if (name_buffer == NULL) {
		fprintf (stderr, "get_tty_devices: Could not allocate %d bytes for name buffer.\n",
			name_buffer_max_size);
		return (0);
	}
	curptr = name_buffer;
	endptr = name_buffer + name_buffer_max_size;

	/* We're going to get a little tricky here.  The original code
	 * compares each entry in the directories against the strings
	 * "syscon" and "systty".  It is a lot faster to compare against
	 * inode numbers rather than string names, so we'll find out
	 * what the inode numbers of "/dev/syscon" and "/dev/systty"
	 * are, and sort by inode number instead.  The idea is to
	 * eliminate syscon and systty as valid tty names for the
	 * console, leaving only "console".
	 */
	
	systty_inode = 0;  /* Zero is an invalid inode number. */
	syscon_inode = 0;

	ret = stat ("/dev/systty", &sbuf);
	if (ret >= 0) {
		systty_inode = sbuf.st_ino;
	}
	ret = stat ("/dev/syscon", &sbuf);
	if (ret >= 0) {
		syscon_inode = sbuf.st_ino;
	}

	number_of_devices = 0;
	for (ptr = ttydir_list; ptr->name != NULL; ptr++) {
		dirp = opendir(ptr->name);
		if (dirp == NULL && ptr->required) {
			fprintf(stderr, "get_tty_devices: cannot open '%s'\n", ptr->name);
			return (0);
		}
		if (dirp == NULL)
			continue;
		
		if (chdir (ptr->name) < 0) {
			if (ptr->required) {
				fprintf (stderr, "get_tty_devices: cannot change directory to '%s'\n",
					ptr->name);
				closedir (dirp);
				return (0);
			} else {
				closedir (dirp);
				continue;
			}
		}

		for (dp = readdir(dirp); dp != NULL; dp=readdir(dirp)) {
			if (stat(dp->d_name, &sbuf) < 0)
				continue;
			if ((sbuf.st_mode&S_IFMT) != S_IFCHR)
				continue;
			inode = sbuf.st_ino;
			if ((inode == syscon_inode)
				&& (!strcmp (dp->d_name, "syscon")))
				continue;
			if ((inode == systty_inode)
				&& (!strcmp (dp->d_name, "systty")))
				continue;
			length = strlen (dp->d_name);
			if ((curptr + length + 1) >= endptr) {
				fprintf (stderr, "get_tty_devices: overflowed name buffer\n");
				return (0);
			}
			strcpy(curptr, dp->d_name);
			devl[number_of_devices].dev = sbuf.st_rdev;
			devl[number_of_devices].name_index = curptr - name_buffer;
			curptr += length+1;
			number_of_devices++;
			if (number_of_devices >= NDEV) {
				fprintf (stderr, "get_tty_devices: number of device files exceeds the limit (%d).\n",
					NDEV);
				break;
			}
		}
		closedir(dirp);
	}
	name_buffer_max_size = curptr - name_buffer;
	return (1);
}


/* get_tty returns the user's tty number or ? if none  */
char *
get_tty (ttyp, ttyd)
	register long ttyp;
	register long ttyd;
{
	register i;
	register char *cp;
	char buf,buf2;
	long minor_number=0, major_number=0;

	if ((ttyp==-1) && (ttyd ==-1))
	{
		return("?");
	}

	if (ttyd == cons_dev) {   /* Alias for console. */
		ttyd = 0;             /* Real console */
	}
	
	for (i=0; i<number_of_devices; i++) {
		minor_number = (devl[i].dev & 0x00ffffff);
		major_number = (0xff000000 & devl[i].dev );
		major_number = (major_number >> 24);
		if ((minor_number == ttyd) && (major_number == ttyp)) {
			cp = name_buffer + devl[i].name_index;
			break;
		}
	}

	if (i >= number_of_devices)
		return("?");
	else
		return(cp);
}
