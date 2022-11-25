/*	@(#)ypsrvancil.c	$Revision: 1.32.109.1 $	$Date: 91/11/19 14:24:03 $  
	ypserv_ancil.c	2.1 86/04/16 NFSSRC
	"ypserv_ancil.c 1.1 86/02/05 Copyr 1984 Sun Micro";
*/

#include "ypsym.h"

/*  HPNFS
 *  The "translate_mapname" function is needed to swap standard and HP mapnames.
 *  Add the header file which defines "Sun_mapname" and "HP_mapname."
 *  HPNFS
 */

#ifdef hpux
extern bool longfiles;		/* Does ypdbpath support long filenames */
#endif /* hpux */

extern int max_length;		/* What is the max_length of a filename that */
				/* is supported on the ypdbpath directory    */

extern char *translate_mapname();
#include "trans.h"
#ifdef	TRACEON
# define LIBTRACE
#endif
#include <arpa/trace.h>

bool onmaplist();

/*
 * This constructs a file name from a passed domain name, a passed map name,
 * and a globally known Network Information Service data base path prefix.
 */
/*  HPNFS
 *
 *  The ypmkfilename() function now returns an integer value indicating either
 *  success (MAPNAME_OK), the full pathname is too long (PATH_TOO_LONG) or the
 *  requested map exceeds the number of characters allowed for a mapname (which
 *  implies it is a non-standard mapname) - MAPNAME_TOO_LONG.  These #defines
 *  can be found in the local "ypsym.h" file.
 *
 *  The length limit to mapnames is DIRSIZ minus four, on the s300, because 
 *  the dbm routines add the four-character suffixes ".dir" and ".pag" to any 
 *  mapname, and DIRSIZ describes the filename length limit.
 *  Since we can have long or short filesystems max_length determines whether 
 *  the filesystem we are interested in supports supports long filenames (chm).
 *  Dave Erickson.
 *
 *  HPNFS
 */
int
ypmkfilename(domain, map, path)
	char *domain;
	char *map;
	char *path;
{
	char *temp_mapname;
	struct stat buf_stat;
	char dbm_path[MAXPATHLEN + 1];

	TRACE3("ypmkfilename:  SOP - domain = \"%s\", map = \"%s\"",
		domain, map);
	/* +2 below for the two slashes in the path - prabha */
	if ((strlen(domain) + strlen(map) + strlen(ypdbpath) + 2) > MAXPATHLEN) {
		TRACE("ypmkfilename:  EOP - PATH_TOO_LONG");
		return (PATH_TOO_LONG);
	}

#ifdef hpux
	if (longfiles)
	{
		TRACE2("ypmkfilename: max_length: %d", max_length);
		temp_mapname = map; 
	}
	else
		temp_mapname = translate_mapname(map, HP_mapname);

#else not hpux

	temp_mapname = translate_mapname(map, HP_mapname);

#endif /* hpux */

	TRACE2("ypmkfilename: temp_mapname = \"%s\"", temp_mapname);

	/* temp_name + .dir [.pag] should not exceed max_length */
	if (strlen(temp_mapname) > (max_length - 4)) {
		TRACE("ypmkfilename:  EOP - MAPNAME_TOO_LONG");
		return (MAPNAME_TOO_LONG);
	}

	(void) sprintf(path, "%s/%s/%s", ypdbpath, domain, temp_mapname);
	(void) sprintf(dbm_path, "%s.dir", path);

#ifdef hpux
	/* If we should have found a long filename but do not, then we */
	/* try a short filename.  If the stat failed for any other     */
	/* reason other than ENOENT, ypserv wil have problems when it  */
	/* tries to access that map.				       */

	TRACE2("ypmkfilename: dbm_path is %s", dbm_path);
	TRACE2("ypmkfilename: path is %s", path);
	if (longfiles && (stat(dbm_path, &buf_stat) == -1) && (errno == ENOENT))
	{
	      TRACE("ypmkfilename:  stat of longfilename failed");
	      temp_mapname = translate_mapname(map, HP_mapname);
	      (void) sprintf(path, "%s/%s/%s", ypdbpath, domain, temp_mapname);
	      TRACE2("ypmkfilename: path became %s", path);
	}
#endif /* hpux */

	TRACE2("ypmkfilename:  EOP - path = \"%s\"", path);
	return (MAPNAME_OK);
}

/*
 * This checks to see whether a domain name is present at the local node as a
 *  subdirectory of ypdbpath
 */
bool
ypcheck_domain(domain)
	char *domain;
/*  HPNFS
 *
 *  The Sun code stats every file in the ypdbpath directory, but
 *  it needs to look ONLY at the desired domain subdirectory!
 *  I've shortened this to do only the job required.
 *  Dave Erickson.
 *
 *  HPNFS
 */
{
	char path[MAXPATHLEN + 1];
	struct stat buf;

	TRACE2("ypcheck_domain:  SOP - domain = \"%s\"", domain);
	(void) sprintf(path, "%s/%s", ypdbpath, domain);
	TRACE3("ypcheck_domain:  EOP - path = \"%s\", stat = %d", path,
		(stat(path, &buf) < 0  ?  0 : buf.st_mode & S_IFDIR));
	return (stat(path, &buf) < 0  ?  0 : buf.st_mode & S_IFDIR);

}

/*
 * This generates a list of the maps in a domain.
 */
int
yplist_maps(domain, list)
	char *domain;
	struct ypmaplist **list;
{
	DIR *dirp;
	struct direct *dp;
	char	*mp;

/*  HPNFS
 *
 *  "the_mapname" is used to construct
 *  the list of maps: it points to either the Sun-standard name of a map
 *  or the name of the local map (if it's a non-standard name).
 *  Dave Erickson.
 *
 *  HPNFS
 */
	char domdir[MAXPATHLEN + 1];
	char *the_mapname;
	int error;
	char *ext;
	struct ypmaplist *map;
	int namesz;

	TRACE2("yplist_maps:  SOP - domain = \"%s\"", domain);
	*list = (struct ypmaplist *) NULL;

	if (!ypcheck_domain(domain) ) {
		TRACE("yplist_maps:  EOP - YP_NODOM");
		return (YP_NODOM);
	}
	
	(void) sprintf(domdir, "%s/%s", ypdbpath, domain);

	if ( (dirp = opendir(domdir) ) == NULL) {
		TRACE2("yplist_maps:  EOP - YP_YPERR, cannot open domdir = \"%s\"",
			domdir);
		return (YP_YPERR);
	}

	TRACE2("yplist_maps:  reading domdir = \"%s\"", domdir);
	error = YP_TRUE;
	
	for (dp = readdir(dirp); error == YP_TRUE && dp != NULL;
	    dp = readdir(dirp) ) {
		/*
		 * If it's possible that the file name is one of the two files
		 * implementing a map, remove the extension (".pag" or ".dir")
		 */
		namesz =  strlen(dp->d_name);

		if (namesz < 5)	/* because .pag or .dir itself is 4 long */
			continue;		/* Too Short */

		ext = &(dp->d_name[namesz - 4]);

		if (strcmp (ext, ".pag") != 0 && strcmp (ext, ".dir") != 0)
			continue;		/* No dbm file extension */

		dp->d_name[namesz - 4] = '\0';

/*  HPNFS
 *
 *  Sun's code is an unneeded expense of time.  A filename is constructed
 *  from what's read from the directory and then checked to see if it exists!
 *  The only time this would show a file does not exist is if it is deleted
 *  just after the directory entry was read.
 *
 *  First, convert the local mapname to the Sun-standard mapname (if it
 *  IS a standard map) for placement in the list.
 *  Dave Erickson.
 *
 *  HPNFS
 */
		the_mapname = translate_mapname(dp->d_name, Sun_mapname);

		if (!onmaplist(the_mapname, *list)) {

			/* if ((map = (struct ypmaplist *) mem_alloc( */
			if ((map = (struct ypmaplist *) malloc(
					(unsigned) sizeof (struct ypmaplist)))
			         == NULL) {
				TRACE("yplist_maps:  malloc unsuccessful");
				error = YP_YPERR;
				break;
			}
			TRACE("yplist_maps:  malloc successful");

			map->ypml_next = *list;
			*list = map;
/*  HPNFS
 *
 *  Use the converted mapname, not the name retrieved straight
 *  from the directory.
 *  Dave Erickson.
 *
 *  HPNFS
 */
			namesz = strlen(the_mapname);

			if (namesz > YPMAXMAP)
				namesz = YPMAXMAP;
			mp = map->ypml_name;
			(void) memcpy(mp, the_mapname, namesz);
			*(mp+namesz) = '\0';
		}
	}

	closedir(dirp);
	TRACE2("yplist_maps:  EOP - error = %d", error);
	return(error);
}
		
/*
 * This returns TRUE if map is on list, and FALSE otherwise.
 */
static bool
onmaplist(map, list)
	char *map;
	struct ypmaplist *list;
{
	struct ypmaplist *scan;

	TRACE2("onmaplist:  SOP - map = \"%s\"", map);
	for (scan = list; scan; scan = scan->ypml_next) {

		if (strcmp(map, scan->ypml_name) == 0) {
			TRACE("onmaplist:  EOP - map in list");
			return (TRUE);
		}
	}

	TRACE("onmaplist:  EOP - map not in list");
	return (FALSE);
}
