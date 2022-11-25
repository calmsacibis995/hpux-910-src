/* $Revision: 64.1 $ */

#include "lp.h"

#define	REPRINT		-2
#define	ERROR		-1
#define	OK		0

extern	char	arg[BUFSIZ];
extern	char	*RM;		/* remote machine name */
extern	char	*RP;		/* remote printer name */

extern	int	errno;

extern	char	*printer;

extern	char	work[BUFSIZ];

/*
 * Send the control file (cf) and any data files.
 * Return -1 if a non-recoverable error occured,
 *         1 if a recoverable error and
 *         0 if all is well.
 */
int
sendit(file)
char	*file;	/* name of the temp control file (full path) */
{
	int	err = 0;
	int	gre_result;
	int	index;
	int	pfd;			/* printer file descriptor */

	char	cf_path[FILEMAX];
	char	cf_name[FILEMAX];
	char	otype;
	char	type;

	char	*line;

	FILE	*cfp;			/* control file */
	extern	char	*strrchr();
	extern	char	*strcpy();
	extern	char	*strncpy();

	pfd = openpr();
	strncpy(cf_path,file,FILEMAX);
	index = '/';
	line = strrchr(cf_path,index);
	line++;
	index = ((FILEMAX) - strlen(cf_name));
	strncpy(cf_name,line,index);
	
	/*
	 * open control file
	 */
	if ((cfp = fopen(file, "r")) == NULL) {
		log("control file (%s) open failure <errno = %d>", file, errno);
		return(0);
	}
	/*
	 *      read the control file for work to do
	 *
	 *      file format -- first character in the line is a command
	 *      rest of the line is the argument.
	 *      commands of interest are:
	 *
	 *            a-z -- "file name" name of file to print
	 */

	while (getrent(&type,arg,cfp) != EOF) {
		if ((type >= 'a') && (type <= 'z')) {
			otype = type;
			index = ((FILEMAX) - (line - cf_path));
			strncpy(line, arg, index);
			while ((gre_result = getrent(&type,arg,cfp)) != EOF){
				if (otype == type){
					if (strcmp(line, arg))
						break;
				}else{
					break;
				}
			}
			switch (sendfile('\3', cf_path, NULL, pfd)) {
			case OK:
				if (gre_result == EOF){
					break;
				}else{
					continue;
				}
			case REPRINT:
				(void) fclose(cfp);
				return(REPRINT);
/*
			case ACCESS:
				sendmail(logname, ACCESS);
*/
			case ERROR:
				err = ERROR;
			}
			break;
		}
	}
	cf_name[0] = 'c';
	if (err == OK && sendfile('\2', file, cf_name, pfd) > 0) {
		(void) fclose(cfp);
		return(REPRINT);
	}
	(void) fclose(cfp);
	sendit_status("%s: ready and waiting", printer);
	return(err);
}

/*
 * Send a data file to the remote machine and spool it.
 * Return positive if we should try resending.
 */
int
sendfile(type, file, name, pfd)
char	type;	/* type of request */
char	*file;	/* name of file to send */
char	*name;	/* name of file to be used by the receiver */
int	pfd;			/* printer file descriptor */
{
	int	f, i, amt;
	struct	stat stb;
	char	buf[BUFSIZ];
	int	sizerr;
	int	index;
	int	resp;
	char	*line;
	extern	char	*strrchr();

/*	open the file and get the size.  The size must be greater
	than zero. */

	if ((f = open(file, O_RDONLY)) < 0 || fstat(f, &stb) < 0) {
		log("file (%s) open failure <errno = %d>", file, errno);
		return(-1);
	}
	if (name == NULL){
		index = '/';
		line = strrchr(file,index);
		line++;
	}else{
		line = name;
	}
	(void) sprintf(buf, "%c%d %s\n", type, stb.st_size, line);
	amt = strlen(buf);
	for (i = 0;  ; i++) {
		if (write(pfd, buf, amt) != amt ||
		    (resp = noresponse(pfd)) < 0 || resp == '\1') {
			(void) close(f);
			return(REPRINT);
		} else if (resp == '\0')
			break;
		if (i == 0)
			sendit_status("no space on remote; waiting for queue to drain");
		if (i == 10)
			log("%s: can't send to %s; queue full", printer, RM);
		sleep(5 * 60);
	}
	if (i)
		sendit_status("sending to %s", RM);

	sizerr = 0;
	for (i = 0; i < stb.st_size; i += BUFSIZ) {
		amt = BUFSIZ;
		if (i + amt > stb.st_size)
			amt = stb.st_size - i;
		if (sizerr == 0 && read(f, buf, amt) != amt)
			sizerr = 1;
		if (write(pfd, buf, amt) != amt) {
			(void) close(f);
			return(REPRINT);
		}
	}
	(void) close(f);
	if (sizerr) {
		log("%s: changed size", file);
		(void) write(pfd, "\1", 1);  /* tell recvjob to ignore this file */
		return(ERROR);
	}
	if (write(pfd, "", 1) != 1)
		return(REPRINT);
	if (noresponse(pfd))
		return(REPRINT);
	return(OK);
}

/*
 * Check to make sure there have been no errors and that both programs
 * are in sync with eachother.
 * Return non-zero if the connection was lost.
 */
int
noresponse(pfd)
int	pfd;
{
	char resp;

	if (read(pfd, &resp, 1) != 1){
		log("%s: lost connection",printer);
		return(REPRINT);
	}
	return((int) resp);
}

/*
 * Acquire line printer or remote connection.
 */
int
openpr()
{
	int i;
	int n;
	int local_pfd;
	int	resp;

	for (i = 1; ; i = i < 256 ? i << 1 : i) {
		local_pfd = getport(RM);
		if (local_pfd >= 0) {
			(void) sprintf(arg, "\2%s\n", RP);
			n = strlen(arg);
			if ((write(local_pfd, arg, n) == n) &&
				((resp = noresponse(local_pfd)) == 0))
				break;
			(void) close(local_pfd);
		}
		if (i == 1)
			if (resp < 0){
				sendit_status("waiting for %s to come up", RM);
			}else{
				sendit_status("waiting for queue to be enabled on %s", RM);
				i = 256;
			}
		sleep((unsigned long) i);
 	}
 		sendit_status("sending to %s", RM);
	return(local_pfd);
}
/*VARARGS1*/
sendit_status(msg, a1, a2, a3)
char *msg;
{
	int	i;
	int	fd;

	char	buf[BUFSIZ];

	FILE	*sfp;
	FILE	*lockf_open();
	extern	char	*strcat();

	umask(0);
	/*
	 * log to the status file.
	 */
	sprintf(work, "%s/%s/%s", REQUEST, printer, SENDINGSTATUS);
	sfp = lockf_open(work, "w", TRUE);
	if (sfp == NULL) {
		fatal("cannot create status file",0);
		return;
	} 
	fd = fileno(sfp);
	ftruncate(fd,0L);

	sprintf(buf, msg, a1, a2, a3);
	strcat(buf, "\n");
	i = strlen(buf);
	(void) fwrite(buf, 1, i, sfp);
	(void) fclose(sfp);
}
