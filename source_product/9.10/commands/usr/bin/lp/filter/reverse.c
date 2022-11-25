static char *HPUX_ID = "@(#) $Revision: 66.1 $";

/* @(#) $Revision: 66.1 $ */      
/*
** This program implements a filter which reads in its input and
** then prints it in reverse page order.  It currently is limited
** to files of MAX_PAGES (2,000) in length.  It understands that
** a new page starts after a form feed.  It takes one option which
** indicates the page size.
*/

#include <stdio.h>


main (argc, argv)
    int		argc;	/* 
                        ** The number of arguments.  The first is the
			** command name
			*/

    char **	argv;	/* pointer to array of string pointes to args. */
{

#include <stdio.h>


#define ASCII_LINE_FEED		'\012'
#define ASCII_FORM_FEED		'\014'
#define DEFAULT_PAGE_LEN	66
#define FSEEK_FAIL		(-1)
#define FSEEK_FROM_START	0
#define MAX_PAGES		2000


    extern int 		errno;
    extern FILE * 	tmpfile();

    long	cur_offset;
    int		io_char;	/* need int to handle sign on io of chars */
    long	limit_offset;
    int		num_lines;
    int		num_pages;	/* number of pages in the input file	  */
    int 	page_len;	/* number of lines per printer page	  */
    long	page_offset[MAX_PAGES];  /* offset of each page in the file */
    FILE *	temp_file_ptr;

    if ( argc > 1 )
    {
	if (strncmp(argv[1],"-l",2) == 0)
	    page_len = atoi(&argv[1][2]);
	else
	    page_len = atoi(argv[1]); /* Leave old syntax for backwards */
				      /* compatibility.                 */

        if ( page_len < 0 )
        {
	    page_len = -page_len;
        }
	if ( page_len == 0 )
	{
	    page_len = DEFAULT_PAGE_LEN;
	}
    }
    else
    {
        page_len = DEFAULT_PAGE_LEN;
    }
    /* printf("The page length is %d\n", page_len); */

    
    temp_file_ptr = tmpfile(); 		/* create the temporary file */

    /*
    ** Copy standard input to the temporary files and note where each
    ** page starts.
    */

    num_pages = 0;   	/* number of pages scanned so far 		*/
    page_offset[0] = 0; /* char offset of current page 			*/
    cur_offset = 0;	/* current char offset in the input file 	*/
    num_lines = 0;  	/* number of lines read on this page 		*/
    while ( (io_char = getc(stdin)) != EOF )
    {
	cur_offset ++;
	putc(io_char, temp_file_ptr);
	if ( io_char == ASCII_LINE_FEED )
	{
	    num_lines++;
	}
	if ( (num_lines == page_len) || (io_char == ASCII_FORM_FEED) )
	{
	    /* start a new page */

	    page_offset[++num_pages] = cur_offset;
	    if (num_pages >= MAX_PAGES)
	    {
		printf("Output exceeds reverse limit of %d pages\n",
			MAX_PAGES);
		exit(1);
	    }
	    num_lines = 0;
	}
    }
    if ( page_offset[num_pages] != cur_offset )
    {
	/*
	** If the last page was not a full page, add a form feed and
	** note where the limit of the page is.
	*/

	putc(ASCII_FORM_FEED, temp_file_ptr);
        page_offset[++num_pages] = ++cur_offset;
	if (num_pages >= MAX_PAGES)
	{
	    printf("Output exceeds reverse limit of %d pages\n", MAX_PAGES);
	    exit(1);
	}
    }

    
    /* 
    ** There are num_pages pages in the file.  Print them out in
    ** reverse order.
    */

    while ( num_pages-- > 0 )
    {   /* print out page numpage + 1 */
	cur_offset = page_offset[num_pages];
        if ( fseek(temp_file_ptr, 
		   cur_offset,
		   FSEEK_FROM_START
		  ) == FSEEK_FAIL 
	   ) 
        {
	    (void) printf("Error on fseek of temproary file.\n");
	    exit(1);
        }
	limit_offset = page_offset[num_pages + 1];
	while ( (int) (cur_offset++ < limit_offset ) )
	{
	    putc( getc(temp_file_ptr), stdout);
	}
    }   /* print out page numpage */

    
    
    exit(0);		/* done */

} /* end main */
