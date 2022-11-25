/* @(#) $Revision: 26.1 $ */    
/* This routine has been rewritten because the system III version depended
   on the DEC byte-swapping characteristics.
   KAH  3/11/82
*/
/*
	Replace each occurrence of `old' with `new' in `str'.
	Return `str'.
*/

char  *repl(str,old,new)
char *str;
char old,new;
{
   char *s;

   s = str;
   while (*s)
     { if (*s == old) *s = new;
       s++;
     }
   return(str);
}
