/*	@(#)trans_map.c	$Revision: 1.23.109.1 $	$Date: 91/11/19 14:23:24 $  
*/

#include "trans.h"
#ifdef	TRACEON
# define LIBTRACE
#endif
#include <arpa/trace.h>

#ifdef hpux

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <ndir.h>
#include <errno.h>
#include <fcntl.h>

extern int errno;
#endif hpux

char *translate_mapname (mapname, return_what)
	char *mapname;
	int return_what;

/*****************************************************************************
 *
 *  This function keeps a list of the standard mapnames (as determined by
 *  Sun) and their corresponding shorter names which are used on an HP
 *  Network Information Service server.  (Recall that HP has a fourteen-character length
 *  restriction on filenames.  For NIS maps, each name is suffixed with
 *  ".dir" and ".pag", so the standard Sun mapnames are truncated to at
 *  most ten characters.)
 *
 *  The array "mapname_pairs" contains paired Sun standard and HP mapnames,
 *  in that order.  For completeness, all of Sun's mapnames are listed,
 *  even those which are sufficiently short, i.e., netgroup and ypservers.
 *
 *  The argument "mapname" is compared to one of each pair of strings in
 *  "mapname_pairs".  If a match is found, the other string of that pair
 *  is returned.  If no match is made in the entire array, "mapname"
 *  itself is returned.  The argument "return_what" is used to determine
 *  which string of a pair to return, and consequently, which to compare to.
 *  "Sun_mapname" and "HP_mapname" are defined in the local "trans.h".
 *
 *  translate_mapname is used by both ypserv and ypxfr.
 *
 *  Dave Erickson, 3-20-87.
 *
 *****************************************************************************
 */

{
#define	NAMECOUNT	20	/*  The number of paired mapnames  */

	static char *mapname_pairs [NAMECOUNT] [2] = {

/*	 Sun Standard Name	  HP name
	======================================		*/

	{ "ethers.byaddr",	"ether.byad" },
	{ "ethers.byname",	"ether.byna" },
	{ "group.bygid",	"group.bygi" },
	{ "group.byname",	"group.byna" },
	{ "hosts.byaddr",	"hosts.byad" },
	{ "hosts.byname",	"hosts.byna" },
	{ "mail.aliases",	"mail.alias" },
	{ "netgroup",		"netgroup"   },
	{ "netgroup.byhost",	"netgr.byho" },
	{ "netgroup.byuser",	"netgr.byus" },
	{ "networks.byaddr",	"netwk.byad" },
	{ "networks.byname",	"netwk.byna" },
	{ "passwd.byname",	"passw.byna" },
	{ "passwd.byuid",	"passw.byui" },
	{ "protocols.byname",	"proto.byna" },
	{ "protocols.bynumber",	"proto.bynu" },
	{ "rpc.bynumber",	"rpc.bynu"   },
	{ "rpc.byname",		"rpc.byna"   },
	{ "services.byname",	"servi.byna" },
	{ "ypservers",		"ypservers"  },
	};

	int i, to_compare, to_return;

	TRACE2("translate_mapname:  SOP - mapname = \"%s\"", mapname);
	if (return_what == Sun_mapname) {	/*  Return the Sun mapname?  */
		to_return = 0;
		to_compare = 1;
	} else {				/*  No - HP mapname assumed.  */
		to_return = 1;
		to_compare = 0;
	}

	for (i = 0; i < NAMECOUNT; i++)		/*  Look for the mapname.  */
		if (strcmp(mapname, mapname_pairs[i][to_compare]) == 0) {
			TRACE2("translate_mapname:  EOP - translated mapname = \"%s\"",
				mapname_pairs[i][to_return]);
			return (mapname_pairs[i][to_return]);
		}

	TRACE("translate_mapname:  EOP - no match");
	return (mapname);			/*  No match was found.  */
}

#ifdef hpux
/*
 * maximum file length: query the maximum length of file names allowed in
 * the directory referred to by a pathname.  Return value is maximum
 * name length allowed. Return -1 in case of error.
 * NOTE:  If this routine is changed, the same routine in longfiles.c
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

#endif hpux
