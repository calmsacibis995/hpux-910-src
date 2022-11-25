/* @(#) $Revision: 70.1 $ */   

#include "ivalues.h"
#include "verify.h"
#include <stdio.h>

main(argc, argv)
int argc;
char *argv[];
{
int indx;
int ptr;
extern struct verify_entry verify_table[];
short buildmatch[I_LASTOP];
int prev;
int this;

   /* initialize the temporary array */
   for (indx=0; indx < I_LASTOP; indx++) 
      buildmatch[indx] = -1;

   /* Now scan the verify table and record the index of the first instance
      of each icode.  icode=-1 signals end-of-table */
   prev = -1;
   for (ptr=0; (this = verify_table[ptr].icode) != -1; ptr++) {
      if (this != prev) {
	 if (buildmatch[this] != -1)
	    fprintf(stderr,"%s: duplicate entry for icode %d\n",argv[0],this);
	 buildmatch[this]=ptr;
	 prev = this;
	 }
      }

   /* Finally, write the index array, in a form that can be compiled  */

   printf("short matchindex[%d] = { \n",I_LASTOP);
   for (indx=0; indx < I_LASTOP; indx++) {
      printf("%d",buildmatch[indx]);
      if (indx<I_LASTOP-1)
	 printf(",  ");
      if ((indx % 8) == 7)
	 printf("\n");
      }
   printf("};\n");

   exit(0);
}

