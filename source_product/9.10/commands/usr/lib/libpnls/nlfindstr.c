


#include <stdio.h>
#include <nlinfo.h>

#define E_INVALIDLENGTH1  3
#define E_INVALIDLENGTH2  4
#define NLCHARTYPE(c)	  pcharset[(unsigned char)c]

short nlfindstr(langid, string1, length1, string2, length2, err, charset)
unsigned short err[];
short langid, length1, length2;
char *string1, *string2, *charset;
{
	char	*pcharset;
	struct l_info *n, *getl_info();
	int	i, j, k, skip_two_chars;

	err[0] = err[1] = 0;
	skip_two_chars = NL_FALSE;

	/*
	 * if the user didn't supply the table:
	 *	get the l_info structure
	 */
	if ((pcharset = charset) == NULL){
		if (langid == -1) {
		    err [0] = E_LNOTCONFIG;
		    return (short) (-1);
                }
		if ((n = getl_info(langid)) == NULL) {
		    err [0] = E_LNOTCONFIG;
		    return (short) (-1);
                }
		pcharset = n->char_set_definition;
	}

	if (length1 <= 0){
		err[0] = E_INVALIDLENGTH1;
		return (short) (-1);
	}

	if (length2 <= 0  ||  length2 > length1){
		err[0] = E_INVALIDLENGTH2;
		return (short) (-1);
	}


	/* Algorithm:
	 *
	 *      For each character in string1
	 *
	 *          For each character in string2
	 *
	 *              Determine if char in string1 is a first of two
	 *              if string1[i] equal to string2[k] continue
	 *
	 */


        for (i=0; i < length1; i++) {

             if (NLCHARTYPE(string1[i]) == NLI_FIRSTOF2)
	          skip_two_chars = NL_TRUE;
             else skip_two_chars = NL_FALSE;

	     for (j=i, k=0; (j < (length1)) && (k < length2); j++, k++)  
		  if (string1[j] != string2 [k]) k = length2 + 1;
              
	     if (k == length2) return ((short) i);

	     if (skip_two_chars) { 
		 i++;  /* skip the second byte of a two-byte character */
		 skip_two_chars = NL_FALSE;
             }
         }
	 /* Substring not found: return minus one */
	 return  (unsigned short) (-1);
}
