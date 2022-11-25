static char *HPUX_ID = "@(#)$Revision: 51.2 $";

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <dbm.h>

#ifndef FALSE
#define FALSE	0
#endif

char	buf[BUFSIZ];

char buffer[BUFSIZ];	/* buffer for filenames opened by dbmopen */

FILE *dbmopen();

main(argc, argv)
	char *argv[];
{
	FILE *dp;
	datum key, content;
	register char *cp, *tp;
	int verbose = 0, entries = 0, maxlen = 0;

	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		verbose++;
		argv++, argc--;
	}
	if (argc != 2) {
		fprintf(stderr, "usage: mkuupath [ -v ] file\n");
		exit(1);
	}
	if (access(argv[1], 04) < 0) {
		fprintf(stderr, "mkuupath: ");
		perror(argv[1]);
		exit(1);
	}
	umask(0);
	dp = dbmopen(argv[1], O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (dp == NULL) {
		fprintf(stderr, "mkuupath: ");
		perror(buffer);
		exit(1);
	}
	dbminit(argv[1]);
	while (fgets(buf, sizeof buf, dp) != NULL) {
		for (cp = buf; *cp && *cp != '\t' && *cp != ' '; cp++)
			if (isupper(*cp))
				*cp = tolower(*cp);
		*cp++ = '\0';
		while (*cp && (*cp == ' ' || *cp == '\t'))
			cp++;
		tp = cp;		/* tp points to address */
		while (*cp && *cp != '\n')
			cp++;
		if (*cp)
			*cp = '\0';
		content.dptr = tp;
		content.dsize = strlen(tp)+1;
		if (verbose)
			printf("store %s, address %s\n", buf, tp);
		key.dptr = buf;
		key.dsize = strlen(buf)+1;
		store(key, content);
		entries++;
		if (cp - buf > maxlen)
			maxlen = cp - buf;
	}
	printf("%d entries, maximum length %d\n", entries, maxlen);
	exit(0);
}

FILE *dbmopen(filename, flags, mode)
char *filename;
int flags, mode;
{
	int fd;

	strcpy(buffer, filename);
	strcat(buffer, ".dir");
	if (fd = open(buffer, flags, mode) < 0)
		return NULL;
	close(fd);
	strcpy(buffer, filename);
	strcat(buffer, ".pag");
	if (fd = open(buffer, flags, mode) < 0)
		return NULL;
	close(fd);
	strcpy(buffer, filename);	/* in case error message printed */
	return (fopen(filename, "r"));
}
