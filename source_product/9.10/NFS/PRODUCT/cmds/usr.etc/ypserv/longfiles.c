#ifndef lint
static  char rcsid[] = "@(#)longfiles:	$Revision: 1.11.109.1 $	$Date: 91/11/19 14:23:12 $  ";
#endif

#ifndef NLS
#define catgets(i, sn,mn,s) (s)
#else NLS
#define NL_SETN 1	/* set number */
#include <nl_types.h>
#endif NLS

#include "ypsym.h"
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/trace.h>

extern int errno;
char ypdbpath[] = __YP_PATH_PREFIX;
#ifdef NLS
nl_catd nlmsg_fd;
#endif NLS

main(argc, argv)
	int argc;
	char **argv;
{	
	int max_length;

#ifdef NLS
	nl_init(getenv("LANG"));
	nlmsg_fd = catopen("longfiles",0);
#endif NLS
	max_length = max_file_length(ypdbpath);
	switch (max_length)
	{
		case MAXNAMLEN:
				exit(0);
				break;
		case DIRSIZ_CONSTANT:
				exit(1);
				break;
		default:
			fprintf(stderr,(catgets(nlmsg_fd,NL_SETN,1, "Unable to determine filename length allowed in directory %s\n")), ypdbpath);
			exit(1);
	}				
}
	
/*
 * maximum file length: query the maximum length of file names allowed in
 * the directory referred to by a pathname.  Return value is maximum
 * name length allowed. Return -1 in case of error.
 * NOTE:  If this routine is changed, the same routine in trans_map.c
 *	  should also be changed!!!!! 
 */

max_file_length(dirname)
	char *dirname;
{
	char tmpfile[16];
	char short_file[15];
	char long_path[MAXPATHLEN + 1];
	char short_path[MAXPATHLEN + 1];
	int fd = 0;
	int cnt = 0;
	struct stat buf;
	int return_val;

	TRACE("max_file_length SOP");
	strcpy(tmpfile, "a21435768901334");
	TRACE2("max_file_length: tmpfile is %s", tmpfile);
	sprintf(long_path, "%s/%s", dirname, tmpfile);
	TRACE2("max_file_length: long_path is %s", long_path);
	strncpy(short_file, tmpfile, 14);
	sprintf(short_path, "%s/%s", dirname, short_file);

	while ((stat(short_path, &buf) == 0) || (fd = open(long_path, O_RDONLY | O_CREAT | O_EXCL) == -1)) 
	{
		if ((fd == -1) && (errno != EEXIST))
		{ 
			TRACE2("max_file_length, open failed, return -1,errno %d", errno);
				return(-1);
		}

		if (cnt == 26)
		{
			TRACE("max_file_length, cnt = 26, return -1");
			return(-1);
		}
		cnt++;
		tmpfile[0]++;
		sprintf(long_path, "%s/%s", dirname, tmpfile);
		TRACE3("max_file_length: long_path is now %s and tmpfile is %s", long_path, tmpfile);
		strncpy(short_file, tmpfile, 14);
		sprintf(short_path, "%s/%s", dirname, short_file);
	}
	TRACE2("max_file_length: short_path is %s", short_path);

/* The possible errors for stat are ENOTDIR, EACCESS, EFAULT, all not       */
/* possible because otherwise we would have had a problem and returned      */
/* during the open.  The same applies to ENOENT meaning that the path is    */
/* NULL.  So, if the short file does not exist that means the long filename */
/* was the one created.							    */

	if (stat(short_path, &buf) == -1 && errno == ENOENT)
		return_val = MAXNAMLEN;
	else
		return_val = DIRSIZ_CONSTANT;
	close(fd);
	unlink(long_path);
	TRACE2("max_file_length, EOP, return_val is %d", return_val);
	return(return_val);
}
