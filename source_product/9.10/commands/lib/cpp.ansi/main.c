/*
 * (c) Copyright 1989,1990,1991,1992 Hewlett-Packard Company, all rights reserved.
 *
 * #   #  ##  ##### ####
 * ##  # #  #   #   #
 * # # # #  #   #   ####
 * #  ## #  #   #   #
 * #   #  ##    #   ####
 *
 * Please read the README file with this source code. It contains important
 * information on conventions which must be followed to make sure that this
 * code keeps working on all the platforms and in all the environments in
 * which it is used.
 */

#ifdef hpe
#  pragma VERSIONID "@(#)MPE/iX C Language Preprocessor (ANSI) Version A.04.5x"
#endif
#include <stdio.h>
#include "support.h"

extern boolean is_error;
extern FILE *outfile;

#ifdef hpe
char *src_group_account_ptr = NULL;   /* points group/account of the source
                                         file */
#endif


#if defined ( CXREF)
FILE *outfp;          /* output file for cxref */
static int ready = 0; /* true: emit xref data */
int doCXREFdata = 0;  /* True: produce cxref data */
#define NAMESIZE 255
int ncps = 255;       /* variable for name size */
#endif

main(argc, argv)
int argc;
char *argv[];
{
#if defined(HP_OSF) || defined(__sun)
	extern boolean ShowDependents;
#endif /* HP_OSF || __sun */
	static char *line = NULL;
	char *get_output_line();

#ifdef hpe
        register char *src_group_account;
        int i;

        /* Only parse the source file name's group and account if a
         * source file name is specified in the info string.
         */
        if (argc > 1)
        {
           for (i=0; argv[1][i] != '.' && argv[1][i] != '\0'; i++)
              ;   /* skipping to first dot */
           if ( argv[1][i] != '\0' )
           {
              if ( (src_group_account = malloc(19)) == NULL &&
                   (src_group_account_ptr = malloc(19)) == NULL )
              {
                 error("Out of dynamic memory");
                 exit(1);
              }
              else
              {
                 src_group_account_ptr = src_group_account;
                 *src_group_account++ = argv[1][i++];
                 while (argv[1][i] != '\0')
                    *src_group_account++ = argv[1][i++];
                 *src_group_account = '\0';
              }
           }
        }
#endif /* hpe */

	startup(argc, argv);
#if defined(CXREF)
	ready = 1;
#endif	
	while((line = get_output_line()) != NULL)
#ifdef hpe
		/* DTS# CLLbs00242. klee 082592
		 * Check the return value of fputs.
		 */
		if (fputs(line, outfile) < 0)
			{
			error("Output file too small ( use -Fsize option )\n");
			break;
			}
#else
		fputs(line, outfile);
#endif
#if defined(HP_OSF) || defined(__sun)
	if (ShowDependents)
		fprintf(stderr,"\n");
#endif /* HP_OSF || __sun */
	if(is_error)
		exit(1);
	else
		exit(0);
}

#if defined(CXREF)
ref (name, line)
   char *name;
   int     line;
{
    fprintf (outfp, "R");
    while(*name  != '\n' && *name != '\r' && *name != '\0')
       fputc (*name++,outfp);
    fprintf (outfp, "\t%05d\n", line);
}

def (name, line)
   char *name;
   int line;
{
    if(ready)
     {
	 fprintf (outfp, "D");
	 while(*name  != '\n' && *name != '\r' && *name != '\0')
	     fputc (*name++,outfp);
	 fprintf (outfp, "\t%05d\n", line);
     }
}

newf (name, line)
   char *name;
   int line;
{
    fprintf (outfp, "F");
    while(*name  != '\n' && *name != '\r' && *name != '\0')
       fputc (*name++,outfp);
    fprintf (outfp, "\t%05d\n", line);
}
            
char *xcopy (ptr1, ptr2)
   register char *ptr1, *ptr2;
{
      static char    name [NAMESIZE];
      char          *saveptr, ch;
      register char *ptr3 = name;

   /* Locate end of name; save character there */
   if ((ptr2 - ptr1) > ncps) {
      saveptr = ptr1 + ncps;
      ch = *saveptr;
      *saveptr = '\0';
   } else {
      ch = *ptr2;
      *ptr2 = '\0';
      saveptr = ptr2;
   }
   while (*ptr3++ = *ptr1++); /* copy name */
   *saveptr = ch;             /* replace character */
   return (name);
} /* xcopy */

xfname(name)
    char *name;
{
	fprintf (outfp, "\"%s\"\n", name);
}
#endif
