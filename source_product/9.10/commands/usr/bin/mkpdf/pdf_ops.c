/* $Revision: 66.3 $ */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <macros.h>
#include <ctype.h>
#include <sys/sysmacros.h>
#include "pdf.h"

extern char * progname;

/*********************************************************************

   ADD_DELETE - this is a procedure to identify and remove all the
   entries in one BOM file that is not in the other.  The entries that
   are in the old BOM file but not in the new BOM file are written out
   to stdout.  The entries that are in the new BOM but not in the old
   BOM are written out to stdout.  Each entry is stored in a structure
   of type pdfitem.  These structures referenced by an array of
   pointers.  If an "added" or "deleted" file is found it is removed
   from the list.  When all "added" and "deleted" files have been
   removed, each array is passed through again and the holes are filled
   up.  The memory for all the NULL structures are returned to the free
   list.  

   CALLED PROCEDURES - printf, strcmp, free

*********************************************************************/

add_delete(newpdf, oldpdf)

struct pdfitem *newpdf[];     /* array of pointers to pdfitem structs */
struct pdfitem *oldpdf[];     /* for new BOM file and old BOM file    */

{
	int new_offset;            /* offset into newpdf pointer array     */
	int old_offset;            /* offset into oldpdf pointer array     */
	int new_offset_diff;       /* number of NULLed newpdf entries      */
	int old_offset_diff;       /* number of NULLed oldpdf entries      */
	int str_compare;           /* valued returned from strcmp          */
	int temp_offset;           /* temporary offset variable            */
	int index;                 /* index for FOR LOOP                   */
	int changes_made = 0;      /* indicator for additions/deletions    */

	new_offset = 0;
	old_offset = 0;
	new_offset_diff = 0;
	old_offset_diff = 0;

	/* main loop of program, to pass through  each array of pointers
   until the end of one of the arrays is reached - the pointer points
   to a NULL structure (struct pdfitem *)0.                          */

	while ((newpdf[new_offset] != (struct pdfitem *)0)
	    && (oldpdf[old_offset] != (struct pdfitem *)0))
	{
		str_compare = strcmp(newpdf[new_offset]->pathname,
		    oldpdf[old_offset]->pathname);
		if (str_compare < 0)	/* file exists only in newpdf */
		{
			printf("%s: added\n",newpdf[new_offset]->pathname);
			changes_made = 1;
			free(newpdf[new_offset]);
			newpdf[new_offset] = (struct pdfitem *)0;
			new_offset++;
			new_offset_diff++;
		}
		else if (str_compare == 0)
		{
			new_offset++;
			old_offset++;
		}
		else if (str_compare > 0)	/* file exists only in oldpdf */
		{
			printf("%s: deleted\n",oldpdf[old_offset]->pathname);
			changes_made = 1;
			free(oldpdf[old_offset]);
			oldpdf[old_offset] = (struct pdfitem *)0;
			old_offset++;
			old_offset_diff++;
		}
	}

	/* take care of any left over entries in the pointer arrays       */

	if (newpdf[new_offset] != (struct pdfitem *)0)
		while (newpdf[new_offset] != (struct pdfitem *)0)
		{
			printf("%s: added\n",newpdf[new_offset]->pathname);
			changes_made = 1;
			newpdf[new_offset] = (struct pdfitem *)0;
			new_offset++;
			new_offset_diff++;
		}
	else if (oldpdf[old_offset] != (struct pdfitem *)0)
		while (oldpdf[old_offset] != (struct pdfitem *)0)
		{
			printf("%s: deleted\n",oldpdf[old_offset]->pathname);
			changes_made = 1;
			oldpdf[old_offset] = (struct pdfitem *)0;
			old_offset++;
			old_offset_diff++;
		}

	/* these are the last passes through the pointer arrays fill up any
   holes in the array (NULL structure pointers) and to return the
   memory allocated for the NULL structures to the free list     */

	temp_offset = 0;
	for (index = 0; index <= new_offset; index++)
		if (newpdf[index] != (struct pdfitem *)0)
		{
			newpdf[temp_offset] = newpdf[index];
			temp_offset++;
			if (index >= (new_offset - new_offset_diff))
			{
				newpdf[index] = (struct pdfitem *)0;
			}
		}
	temp_offset = 0;
	for (index = 0; index <= old_offset; index++)
		if (oldpdf[index] != (struct pdfitem *)0)
		{
			oldpdf[temp_offset] = oldpdf[index];
			temp_offset++;
			if (index >= (old_offset - old_offset_diff))
			{
				oldpdf[index] = (struct pdfitem *)0;
			}
		}
	return changes_made;
}


/************************************************************************** 
   
   PARSE_LINE - this is a function to take a line from a PDF and
   parse it into the correct fields and store it in the structure
   pdf_holder of type pdfitem.  The address of pdf_holder is passed
   to the function, therefore the function returns no value, but the
   parsed fields are stored in the correct place by side effects.

   CALLED PROCEDURES - sprintf, sscanf 

***************************************************************************/ 

char * next_field();

int
parse_line(line,pdf_holder)
char *line;
struct pdfitem *pdf_holder;

{
	char *ptr;
	int  mode;

	/* read the entry */
	strcpy(pdf_holder->pathname, next_field(line));
	strcpy(pdf_holder->owner, next_field(NULL));
	strcpy(pdf_holder->group, next_field(NULL));
	strcpy(pdf_holder->mode, next_field(NULL));
	strcpy(pdf_holder->fsize, next_field(NULL));
	strcpy(pdf_holder->link_count, next_field(NULL));
	strcpy(pdf_holder->version, next_field(NULL));
	strcpy(pdf_holder->checksum, next_field(NULL));
	strcpy(pdf_holder->link_target, next_field(NULL));

	/* convert mode to string if it is octal */
	ptr = pdf_holder->mode;
	while (isdigit((int)*ptr)) {
		ptr++;
	}
	if ((ptr != pdf_holder->mode) && (*ptr == '\0')) {
		/* we got all digits */
		sscanf(pdf_holder->mode, "%o", &mode);
		setmode(mode, pdf_holder->mode);
	}

	/* return the size of the file */
	if ((pdf_holder->mode[0] == 'c') || (pdf_holder->mode[0] == 'b')) {
		/* device file, zero length */
		return 0;
	} else {
		return atoi(pdf_holder->fsize);
	}
}

char *
next_field(str)
char *str;
{
	static char *ptr;
	char *colon, *retval;

	if (str) {
		/* starting a new line */
		ptr = str;
	}
	if (*ptr == '\0') {
		/* end of line */
		return ptr;
	}
	if (colon = strchr(ptr, ':')) {
		*colon = '\0';
		retval = ptr;
		ptr = colon + 1;
	} else {
		if (colon = strchr(ptr, '\n')) {
			*colon = '\0';
		}
		retval = ptr;
		ptr = "";
	}
	return retval;
}


/****************************************************************************

	PROCESS_LINE - this is a function to fill in new values in the
	appropriate fields of the structure pdf_holder.  Alt_root is a
	pointer to an alternate root that is concatenated to the
	beginning of the pathname from the prototype file.

	The owner, group, and mode fields are set to those of the file.
	If the file is a symbolic link, the contents of the symlink are
	copied into the link_target field.  If the proto has a link_target
	field and that hard link really exists, the field is copied.  If
	the entry is a regular file, then the size, link_count, version,
	and checksum fields are set to match the file characteristics.

	If the pathname being processed is the same as the output file
	the size and checksum are not computed since the data would be
	wrong.  This allows the name of pdf2 to be included in the pdf1.

	If the number of links to the (non-directory) file is > 1 and no
	link_target info is provided in the proto, then the inode and
	pathname are searched/cached to establish link_target info.

	CALLED PROCEDURES - setowner, setgroup, setmode, 
		setversion, setcksum, lstat, strcpy, strcat

**************************************************************************/

struct iname {
    ino_t inode;
    dev_t device;
    char pathname[MAXPATHLEN+1];
};
int inamecmp();

int
process_line(alt_root,pdf_holder)
struct pdfitem *pdf_holder;
char *alt_root;
{
    struct stat f_lstat;	/* lstat structure for the file */
    char full_pathname[MAXPATHLEN];
    int  opt_file = FALSE;	/* is the file optional? */
    int pathname_is_Out_file;	/* don't write size or sum */
    static struct iname *inameroot = NULL, *inamein = NULL;
    struct iname **inameout;

    /* build the full pathname */
    if (alt_root)
	strcpy(full_pathname,alt_root);
    else
	full_pathname[0] = '\0';
    if (pdf_holder->pathname[0] == '?') {
	opt_file = TRUE;
	strcat(full_pathname,&pdf_holder->pathname[1]);
    } else {
	strcat(full_pathname,pdf_holder->pathname);
    }
    pathname_is_Out_file = equal(Out_file,full_pathname);

    /* lstat the file */
    if (lstat(full_pathname, &f_lstat)) {
	if (! opt_file) {
	    perror(full_pathname);
	    return 1;
	} else {
	    return 0;
	}
    }

    /* build the pdf_holder contents */
    sprintf(pdf_holder->link_count, "%d", f_lstat.st_nlink);

    /* handle both symbolic and hard links */
    if (S_ISLNK(f_lstat.st_mode)) { /* read the symlink */
	int linklen = readlink(full_pathname, pdf_holder->link_target, MAXPATHLEN);
	if(linklen==-1) {
	    perror(full_pathname);
	    return 1;
	} else {
	    pdf_holder->link_target[linklen] = '\0';
	}
    } else if (pdf_holder->link_target[0] != '\0') { /* verify hard link */
	struct stat link_stat;
	char link_path[MAXPATHLEN];
	sprintf(link_path, "%s%s", alt_root, pdf_holder->link_target);
	if(lstat(link_path, &link_stat)) {
	    fprintf(stderr, "%s: link target ", full_pathname);
	    perror(link_path);
	    return 1;
	}
	if (f_lstat.st_ino != link_stat.st_ino) {
	    fprintf(stderr, "%s: not a link to %s\n",
		full_pathname, link_path);
	    return 1;
	}
	return 0;
    } else if (!S_ISDIR(f_lstat.st_mode) && (f_lstat.st_nlink > 1)) {
	/* no link_target provided, establish the link_target info */
	if (inamein == NULL &&
		((inamein = (struct iname *)malloc(sizeof(struct iname))) == NULL)) {
	    perror(progname);
	    exit(1);
	}
	inamein->inode = f_lstat.st_ino;
	inamein->device = f_lstat.st_dev;
	strcpy(inamein->pathname, &(pdf_holder->pathname[opt_file ? 1 : 0]));
	if ((inameout = (struct iname **)tsearch((char *)inamein, (char **)&inameroot, inamecmp)) == NULL) {
	    perror(progname);
	    exit (1);
	} else if (*inameout == inamein) {
	    /* found no previous occurance of the inode */
	    /* inamein is used up, force malloc on the next go-around */
	    inamein = NULL;
	} else {
	    /* this is a link to a previous entry */
	    /* skip all the extra work */
	    sprintf(pdf_holder->link_target, (*inameout)->pathname);
	    return 0;
	}
    }

    strcpy(pdf_holder->owner,setowner(f_lstat.st_uid));
    strcpy(pdf_holder->group,setgroup(f_lstat.st_gid));
    setmode(f_lstat.st_mode,pdf_holder->mode);

    /* get info for regular file */
    if (S_ISREG(f_lstat.st_mode))
    {
	if (!pathname_is_Out_file) {
	    sprintf(pdf_holder->fsize, "%d", f_lstat.st_size);
	    if (setcksum(full_pathname,pdf_holder->checksum)) {
		    return 1;
	    }
	}
	setversion(full_pathname,pdf_holder->version);
    }

    /* get major/minor for device file */
    if (S_ISCHR(f_lstat.st_mode) || S_ISBLK(f_lstat.st_mode)) {
	sprintf(pdf_holder->fsize, "%d/0x%06.6x",
	    major(f_lstat.st_rdev), minor(f_lstat.st_rdev));
    }
    return(0);
}

int
inamecmp(iname1, iname2)
struct iname *iname1, *iname2;
{
    if (iname1->device < iname2->device)
	return -1;
    if (iname1->device > iname2->device)
	return 1;
    if (iname1->inode < iname2->inode)
	return -1;
    if (iname1->inode > iname2->inode)
	return 1;
    return 0;
}
