/* $Revision: 66.4 $ */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
/* #include <stdlib.h> */
#include "pdf.h"

extern int numeric_ids;

/**********************************************************************

   BOM_CHECK_SGID - this is a function to return a 1 if the new entry is
   a set gid program and the old entry is not, a 2 if the old entry is
   a set gid program and the new entry is not, a 3 if both entries are
   set gid files, and a 0 if neither entries are set gid files.

   CALLED PROCEDURES -

************************************************************************/

int
pdf_check_sgid(new_mode,old_mode)

char *new_mode;
char *old_mode;

{
   int new_set = FALSE;
   int old_set = FALSE;
   int set = 0;

   if ((new_mode[6] == 'S') || (new_mode[6] == 's'))
      new_set = TRUE;
   if ((old_mode[6] == 'S') || (old_mode[6] == 's'))
      old_set = TRUE;
   if (new_set)
      set = 1;
   if (old_set)
      set += 2;
   return(set);
}


/*********************************************************************

   BOM_CHECK_SUID - this is a function to return a 1 if the new entry is
   a set uid file and the old entry is not, a 2 if the old entry is
   a set uid file and the new entry is not, a 3 if both entries are
   set uid files, and a 0 (FALSE) if neither entries are set uid
   files.

   CALLED PROCEDURES -

**********************************************************************/

int
pdf_check_suid(new_mode,old_mode)

char *new_mode;
char *old_mode;

{
   int new_set = FALSE;
   int old_set = FALSE;
   int set = 0;

   if ((new_mode[3] == 'S') || (new_mode[3] == 's'))
      new_set = TRUE;
   if ((old_mode[3] == 'S') || (old_mode[3] == 's'))
      old_set = TRUE;
   if (new_set)
      set = 1;
   if (old_set)
      set += 2;
   return(set);
}


/*********************************************************************

   SETCKSUM - this function generates a string that contains the
   checksum value of the file whose name was passed as a parameter.
   This checksum (using mem_crc) will give you the same value as the
   POSIX sum command with the -p option.

   CALLED PROCEDURES - sprintf, mem_crc

**********************************************************************/

int
setcksum(full_pathname,checksum_pt)
char *full_pathname, *checksum_pt;
{
	FILE *file;
	char crc_buf[BUFSIZ];
	register unsigned long checksum = 0;
	int len;

	/* open the file */
	if ((file = fopen(full_pathname,"r")) == NULL)
	{
		perror(full_pathname);
		return 1;
	}

	/* read file and compute the checksum */
	while( (len = fread(crc_buf, 1, BUFSIZ, file)) > 0)
	{
		checksum = mem_crc(crc_buf, len, checksum);
	}
	sprintf(checksum_pt,"%lu",checksum);

	fclose(file);
	return 0;
}


/**************************************************************************

   SETGROUP - this is a function to return a pointer to a string that
   contains the group name. The bin_gid is the binary uid of a group
   that is stored as an unsigned short. The function getgrgid is 
   called to check the groups file and return a pointer to a structure.
   In the grp structure is a pointer to a string that contains the name
   of the group associated with the bin_gid. This pointer is then 
   returned.

   CALLED PROCEDURES - getgrpid

****************************************************************************/

char 
*setgroup(bin_gid)

ushort bin_gid;

{
    static char gp_string[10];
    struct group *grp;

    if (numeric_ids) {
	sprintf(gp_string, "%d", bin_gid);
	return gp_string;
    } else if ((grp = getgrgid(bin_gid)) == NULL) {
	fprintf(stderr,"mkpdf: %d is not a defined group id.\n",
	    bin_gid);
	sprintf(gp_string,"%d",bin_gid);
	return(gp_string);
	}
    else
	return (grp->gr_name);
}


/**************************************************************************** 

   SETMODE - this is a function to return a string that contains the entry
   type and entry mode. The format for this string is the same as the ls 
   command. The bin_mode is a binary represention of the mode. The string 
   representation of the mode is stored in build_mode which pointed to by
   mode_string. Type is used when the bits need to be masked off to determine
   the type and protection modes. The definitions for the different types and
   modes can be found in /usr/include/sys/stat.h. 

   CALLED PROCEDURES -  

****************************************************************************/

void 
setmode(bin_mode,mode_string)

unsigned short bin_mode;
char *mode_string;

{
   unsigned short type;

   type = bin_mode & S_IFMT;
   switch(type)
      {
        case S_IFDIR : 
           {
#if defined (DUX) || defined (DISKLESS)
             if (S_CDF & bin_mode) 
                mode_string[0] = 'H';
             else
#endif
                mode_string[0] = 'd';
             break;
           }
        case S_IFCHR :
           {
             mode_string[0] = 'c';
             break;
           }
        case S_IFBLK :
           {
             mode_string[0] = 'b';
             break;
           }
        case S_IFREG :
           {
             mode_string[0] = '-';
             break;
           }
        case S_IFIFO :
           {
             mode_string[0] = 'p';
             break;
           }
#ifdef RFA
        case S_IFNWK :
           {
             mode_string[0] = 'n';
             break;
           }
#endif /* RFA */
#ifdef SYMLINKS
        case S_IFLNK :
           {
             mode_string[0] = 'l';
             break;
           }
#endif /* SYMLINKS */
        default :
           mode_string[0] = '?';
      }

   /* find protections modes, mask off the file type bits */

   type = bin_mode & ~S_IFMT;
   if (S_IREAD & type)
      mode_string[1] = 'r';
   else
      mode_string[1] = '-';
   if (S_IWRITE & type)
      mode_string[2] = 'w'; 
   else
      mode_string[2] = '-';
   if (S_ISUID & type)
      if (S_IEXEC & type)
         mode_string[3] = 's';
      else
         mode_string[3] = 'S';
   else if (S_IEXEC & type)
      mode_string[3] = 'x';
   else 
      mode_string[3] = '-'; 
   if (S_IREAD & (type << 3))      /* Shift the type 3 bits to the  */
      mode_string[4] = 'r';             /* left to move the group protection */
   else                                /* to the correct position for       */
      mode_string[4] = '-';             /* comparing with the definition of  */
   if (S_IWRITE & (type << 3))     /* the modes.                        */
      mode_string[5] = 'w';
   else
      mode_string[5] = '-';
   if (S_ISGID & type)
      if (S_IEXEC & (type << 3))
         mode_string[6] = 's';
      else 
         mode_string[6] = 'S';
   else if (S_IEXEC & (type << 3))
      mode_string[6] = 'x';
   else
      mode_string[6] = '-';
   if (S_IREAD & (type << 6))     /* Shift the type 6 bit to the   */
      mode_string[7] = 'r';            /* left to move the other protection */
   else                               /* to the correct position for       */
      mode_string[7] = '-';            /* comparing with the definition of  */
   if (S_IWRITE & (type << 6))    /* the modes.                        */
      mode_string[8] = 'w';
   else 
      mode_string[8] = '-';
   if (S_ISVTX & type)
      if (S_IEXEC & (type << 6))
         mode_string[9] = 't';
      else
         mode_string[9] = 'T';
   else if (S_IEXEC & (type << 6))
      mode_string[9] = 'x';
   else 
      mode_string[9] = '-';
   mode_string[10] = '\0';
   return ;
}


/****************************************************************************

   SETOWNER - the is a function to return a pointer to a string that
   contains the owner name. The bin_uid is the binary uid a users that
   is stored as an unsigned short. The function getpwuid is called to 
   check the password file and return a pointer to a structure. In the
   pwd structure is a pointer to a string that contains the name of the
   user associated with the bin_uid. This pointer is then returned.

   CALLED PROCEDURES - getpwuid

****************************************************************************/

char 
*setowner(bin_uid)
ushort bin_uid;
{
    static char   pw_string[10];
    struct passwd *pwd;

    if (numeric_ids) {
	sprintf(pw_string, "%d", bin_uid);
	return pw_string;
    } else if ((pwd = getpwuid(bin_uid)) == NULL) {
	fprintf(stderr,"mkpdf: %d is not a defined owner id.\n",
	    bin_uid);
	sprintf(pw_string,"%d",bin_uid);
	    return(pw_string);
    } else
	return (pwd->pw_name);
}


/*****************************************************************************

   SETVERSION - this is a function to determine the version number of
   the entry.  The filename is passed to the function and the results of
   the what(1) command (with -s for performance) are read with a pipe.
   Then the 2nd line is read from the pipe (the first line always
   contains just the filename), and the line is scanned for Revision or
   Header and the revision number is obtained from that string.  If the
   what(1) command should fail to yield a revision, the ident(1) command
   will be run on the file through a pipe.  This is search for first
   word that contains only numbers and '.'s (e.g.  24.2.1.4).  This is
   then taken as the version string.

   CALLED PROCEDURES - fopen, fscanf

*****************************************************************************/
#define TEMPBUFSIZE 1024

void 
setversion(file_name,version_pt)

char *file_name;
char *version_pt;

{
    char cmd[MAXPATHLEN];
    char temp[TEMPBUFSIZE];
    char *match_pt = NULL;
    static char *compiled_exp = NULL;
    FILE *pipe_pt;

/* try to get a revision with the 'what(1)' command */
    sprintf(cmd,"/usr/bin/what -s %s 2> /dev/null",file_name);
    if ((pipe_pt = popen(cmd,"r")) != NULL) {
/* skip the filename line from what(1), then read revision */
	if ((fgets(temp,TEMPBUFSIZE,pipe_pt) != NULL) &&
		(fgets(temp,TEMPBUFSIZE,pipe_pt) != NULL)) {
	    if (!compiled_exp)
		compiled_exp = regcmp("(([0-9]+\\.)+[0-9]+)$0",0);
	    match_pt = regex(compiled_exp,temp,version_pt);
	}
	pclose(pipe_pt);
	if (match_pt)
	    goto replace_colons;
    }

/* try to find the version using the ident(1) command */
    sprintf(cmd,"/usr/bin/ident %s 2> /dev/null",file_name);
    if ((pipe_pt = popen(cmd,"r")) == NULL) {
	*version_pt = '\0';
	return;
    }
/* skip the filename line from ident(1), then read revision */
    if ((fgets(temp,TEMPBUFSIZE,pipe_pt) == NULL) ||
	    (fgets(temp,TEMPBUFSIZE,pipe_pt) == NULL)) {
	*version_pt = '\0';
	pclose(pipe_pt);
	return;
    }
/* search for a "Revision" or "Header" line with the version number */
    do {
	if (sscanf(temp," $Revision: %s",version_pt))
	    break;
	if (sscanf(temp," $Header: %s %s", version_pt,version_pt))
	    break;
    } while (fgets(temp,TEMPBUFSIZE,pipe_pt) != NULL);
    pclose(pipe_pt);

replace_colons:
    /* replace colons (:) to prevent mixing up the pdf */
    while (match_pt = strchr(version_pt, ':')) {
	*match_pt = '_';
    }
    return ;
}
