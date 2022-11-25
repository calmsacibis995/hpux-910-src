static char *HPUX_ID = "@(#) $Revision: 70.1 $";

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>

#include <stdio.h>
#include <errno.h>
#include <signal.h>

/*
 * Password file editor with locking.
 */
char	*temp = "/etc/ptmp";
char	*passwd = "/etc/passwd";
char	buf[BUFSIZ];
char	*getenv();
extern	int errno;

main(argc, argv)
	char *argv[];
{
	int fd;
	FILE *ft, *fp;
	char *editor;
	struct stat sbuf1;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	setbuf(stderr, NULL);
	umask(0);

	/* Stat password file to save group/owner/mode */
	if (stat(passwd, &sbuf1) < 0) {
		fprintf(stderr, "vipw: cannot stat %s\n",passwd);
		exit(1);
	}

	fd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (fd < 0) {
		if (errno == EEXIST) {
			fprintf(stderr, "vipw: password file busy\n");
			exit(1);
		}
		fprintf(stderr, "vipw: "); perror(temp);
		exit(1);
	}
	ft = fdopen(fd, "w");
	if (ft == NULL) {
		fprintf(stderr, "vipw: "); perror(temp);
		goto bad;
	}
	fp = fopen(passwd, "r");
	if (fp == NULL) {
		fprintf(stderr, "vipw: "); perror(passwd);
		goto bad;
	}
	while (fgets(buf, sizeof (buf) - 1, fp) != NULL)
		fputs(buf, ft);
	fclose(ft); fclose(fp);
	editor = getenv("EDITOR");
	if (editor == 0)
		editor = "vi";
	sprintf(buf, "%s %s", editor, temp);
	if (system(buf) == 0) {
		struct stat sbuf;
		int ok;

		/* sanity checks */
		if (stat(temp, &sbuf) < 0) {
			fprintf(stderr,
			    "vipw: can't stat temp file, %s unchanged\n",
			    passwd);
			goto bad;
		}
		if (sbuf.st_size == 0) {
			fprintf(stderr, "vipw: bad temp file, %s unchanged\n",
			    passwd);
			goto bad;
		}
		ft = fopen(temp, "r");
		if (ft == NULL) {
			fprintf(stderr,
			    "vipw: can't reopen temp file, %s unchanged\n",
			    passwd);
			goto bad;
		}
		ok = 0;
		while (fgets(buf, sizeof (buf) - 1, ft) != NULL) {
			register char *cp;

			cp = strchr(buf, '\n');
			if (cp == 0)
				continue;
			*cp = '\0';
			cp = strchr(buf, ':');
			if (cp == 0)
				continue;
			*cp = '\0';
			if (strcmp(buf, "root"))
				continue;
			/* password */
			cp = strchr(cp + 1, ':');
			if (cp == 0)
				break;
			/* uid */
			if (atoi(cp + 1) != 0)
				break;
			cp = strchr(cp + 1, ':');
			if (cp == 0)
				break;
			/* gid */
			cp = strchr(cp + 1, ':');
			if (cp == 0)
				break;
			/* gecos */
			cp = strchr(cp + 1, ':');
			if (cp == 0)
				break;
			/* login directory */


			/*
			 *  This test was deemed too restrictive by customers.
			 *  I.e., it's OK to verify the format but not the
			 *  content of this field.
			 *
			if ((strncmp(++cp, "/:", 2)) && (strncmp(cp, "/users/root:", 12)))
				break;
			 */
			++cp;


			/* move cp to end of login directory */
			while (*cp++ != ':');

			if (*cp && strcmp(cp, "/bin/sh") &&
			    strcmp(cp, "/bin/csh") && strcmp(cp, "/bin/ksh"))
				/* brian added to cmp for /bin/ksh */
				break;
			ok++;
		}
		fclose(ft);
		if (ok) {
			if (chown(temp, sbuf1.st_uid, sbuf1.st_gid) < 0) {
				fprintf(stderr, "vipw: "), perror("chown");
				goto bad;
			}
			if (chmod(temp, sbuf1.st_mode) < 0) {
				fprintf(stderr, "vipw: "), perror("chmod");
				goto bad;
			}
			if (rename(temp, passwd) < 0)
				fprintf(stderr, "vipw: "), perror("rename");
		} else
			fprintf(stderr,
			    "vipw: you mangled the temp file, %s unchanged\n",
			    passwd);
	} else
		fprintf(stderr, 
			"vipw: error detected at \"%s\", %s unchanged\n", buf, passwd);
bad:
	unlink(temp);
}
