/* HPUX_ID: @(#)convertmts.c	1.2     89/03/03  */

/*
 *	convertmts.c - convert hp-ux kernel executable from
 *	               multi-user to single-user
 */

#include <stdio.h>
#include <fcntl.h>
#include <a.out.h>
#include <unistd.h>
#include <sys/utsname.h>

char *whatstring = "@(#)@(#)convertmts.c	1.2 89/03/03";

/* acceptable utsname fields for sanity check */
char	OLD_SYSNAME[]		=	"HP-UX";
char	OLD_RELEASE_PREFIX[]	=	"7.";
char	OLD_VERSION[]		=	"B";

/* the one we change to */
char	NEW_VERSION[]		=	"A";

char	*command_name, *file_name;	/* for error messages */
int	error_occurred = 0;

extern	long	lseek();
extern	char	*strcpy();
extern	void	exit();

void	convert();
void	error();

main (argc, argv)
int	argc;
char	**argv;
{
	int	i;

	if (argc < 2) {
		(void)fprintf (stderr, "Usage: %s file ...\n", argv[0]);
		exit (1);
	}

	command_name = argv[0];

	for (i = 1; i < argc; i++) {
		file_name = argv[i];
		convert (file_name);
	}

	if (error_occurred)
		exit (1);
	else
		exit (0);
	/*NOTREACHED*/
}

void
convert (pathname)
char *pathname;
{
	int		fd;
	struct	exec	hdr;
	struct	utsname	name;
	long		offset;

	static struct nlist nl[] = { { "_utsname", },
				     { (char *)0, } };

	fd = open (pathname, O_RDWR);
	if (fd < 0) {
		error ((char *)0);
		return;
	}

	if (read (fd, &hdr, sizeof (hdr)) != sizeof (hdr) ||
	    hdr.a_magic.system_id != HP9000S200_ID ||
	    hdr.a_magic.file_type != SHARE_MAGIC) {
		error ("not a valid HP-UX kernel executable");
		(void)close (fd);
		return;
	}

	if (nlist (pathname, nl) != 0 || nl[0].n_value == 0) {
		error ("not a valid HP-UX kernel executable");
		(void)close (fd);
		return;
	}

	/* make sure the address is in the data segment */
	if ((nl[0].n_value < EXEC_ALIGN (hdr.a_text)) ||
	    (nl[0].n_value + sizeof (struct utsname) >
	     EXEC_ALIGN (hdr.a_text) + hdr.a_data)) {
		error ("not a valid HP-UX kernel executable");
		(void)close (fd);
		return;
	}

	/* translate the address to an offset in the file */
	offset = DATA_OFFSET (hdr) + nl[0].n_value - EXEC_ALIGN (hdr.a_text);

	if ((lseek (fd, offset, SEEK_SET) == -1L) ||
	    (read (fd, &name, sizeof (name)) != sizeof (name))) {
		error ("corrupted file");
		(void)close (fd);
		return;
	}

	/* Do some sanity checks on the original utsname struct */
	if (strcmp (name.sysname, OLD_SYSNAME) != 0) {
		error ("not a valid HP-UX kernel executable");
		(void)close (fd);
		return;
	}

	if (strncmp (name.release, OLD_RELEASE_PREFIX,
		     strlen (OLD_RELEASE_PREFIX)) != 0) {
		error ("kernel must be revision 6.x");
		(void)close (fd);
		return;
	}

	if (strcmp (name.version, OLD_VERSION) != 0) {
		error ("kernel must be multi-user");
		(void)close (fd);
		return;
	}

	/* It looks okay, modify the structure and write it back */
	(void)strcpy (name.version, NEW_VERSION);

	if ((lseek (fd, offset, SEEK_SET) == -1L) ||
	    (write (fd, &name, sizeof (name)) != sizeof (name))) {
		error ("corrupted file");
		(void)close (fd);
		return;
	}
	(void)close (fd);
}

/* Print an error message - null string means use sys_errlist */
void
error (string)
char *string;
{
	extern int	errno;
	extern int	sys_nerr;
	extern char	*sys_errlist[];
	char		buf[80];

	if (string == 0) {
		if (errno > 0 && errno <= sys_nerr) {
			string = sys_errlist[errno];
		} else {
			(void)sprintf (buf, "unknown errno %d", errno);
			string = buf;
		}
	}
	(void)fprintf (stderr, "%s: %s: %s\n", command_name, file_name, string);
	error_occurred = 1;
}
