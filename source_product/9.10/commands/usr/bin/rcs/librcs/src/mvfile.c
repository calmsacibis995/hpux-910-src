/* $Header: mvfile.c,v 4.1 85/08/14 15:51:09 scm HP_UXRel2 $ */

/* Function: renames a file with the name given by from to the name given by to.
 * unlinks the to-file if it already exists. returns -1 on error, 0 otherwise.
 */

int mvfile(from, to)
char * from, *to;
{
	unlink(to);      /* any errors will be caught by link */
                         /* no harm done if file "to" does not exist */

        if (link(from,to)<0) return -1;
        return(unlink(from));
}
