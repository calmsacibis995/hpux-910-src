

#include <nlinfo.h> 
#include <stdio.h>

void  nlcollate (string1, string2, length, result, lang, error, table)
unsigned char	*string1;
unsigned char	*string2;
short	length;
short	*result;
short	lang;
short	*error;
short	*table;
{
        void  nlcollatesys ();
	int   len_collate;
	short *c_table;
	void  free ();
	static short n_table [3] =  {3, 0, 1};

	error [0] = error [1] = 0;
        c_table = (short *) 0;
	*result = 0;

	if (length <=0) {
	    error [0] = (short) 4; /* Invalid lenght parameter */
	   *result = 0;
	    return;
        }

        if (table == NULL) {
	       /* load the collation table */

	       if (lang == 0) { 
                   len_collate = 3;
		   c_table = (short *) n_table;
	       }
                 
	       else {
		      nlinfo(L_LENCOLLATE, &len_collate, &lang, error);
	              if (error [0] != 0) {
		          *result = 0;
		          return;
	              }
	              c_table = (short *) malloc (len_collate * 2);
	              nlinfo(L_COLLATE, (int *) c_table, &lang, error);
              } 
	}
	else  {
		if (table [1] == lang)  c_table = table;
		    else {
			  error [0] = E_LNOTCONFIG;
			  *result = 0;
			  return;
                    }
	}

	nlcollatesys (string1,string2,length,result,lang,error,c_table);
	if (*result < EQUAL) *result = (short) -1;
	else if (*result > EQUAL) *result = (short) 1;
	if ((table==NULL)  && (lang != 0))
	    if ((c_table != NULL) && (lang != 0)) free (c_table);
}

