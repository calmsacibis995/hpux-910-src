


#include <nlinfo.h>

#define NLCHARTYPE(c)  pcharset[(unsigned char)c]

#define  TWOtoONE       0100  
#define  ONEtoTWO       0200  
#define  DONTCARE       0000
#define  ONEtoONE       0077
#define  NULL           0000
#define  MAXSHORT      65535

#define  GREATER           1
#define  SMALLER          -1

#define  BGREAT            2
#define  BSMALL           -2

#define  tlang    s_table[1]
#define  class    s_table[2]
#define  collseq  s_table[3]
#define  addr21   s_table[5]
#define  flag21   s_table[6]
#define  addr12   s_table[7]
#define  flag12   s_table[8]

void  nlcollatesys (string1,string2,length,result,lang,error,table)
unsigned char *string1, *string2, *table;
short length,  *result,  lang, *error;
{

  unsigned short *s_table;/* To speed up the access to coll. data */
  unsigned char *pcharset;         /* To collate Asian characters */
  struct l_info *n, *getl_info();  /* To collate Asian characters */
  int   count;            /* Character counter in 'for' loops     */
  int   breaktie;         /* Secondary comparison                 */

  int   index1, index2;   /* Used by the collation algorithm      */
  int   c1ord,  c2ord;    /* Orders of the current characters     */
  int   n1ord,  n2ord;    /* Orders of the next characters        */
  int   p1ord,  p2ord;    /* Orders of the previous characters    */
  int   c1type, c2type;   /* Types of the current characters      */
  int   n1type, n2type;   /* Types of the next characters         */
  int   p1type, p2type;   /* Types of the previous characters     */
  int   p1indx, p2indx;   /* Indexes of the previous characters   */
  int   tmpaddr;          /* Temporary address to call scan ()    */
  int   seq1,   seq2;     /* Sequence entries for pairs of chars  */


 /* Initial values */

 *error  = 0;
 *result = 0;          
 s_table = (unsigned short *) table;  




 if ((length<=0) || (string1 == string2)) return; 

 /* Collation depending on the class  */

 table += 2*collseq;
 switch (class) {

 case 1: /* Class 1 languages: binary collation     */
         count = 0;
         while ((string1[count]==string2[count]) && (--length)) count ++;
         if (string1[count] > string2[count]) *result = GREATER;
            else if (string1[count] < string2[count]) *result = (short) SMALLER;
         break;




 case 2: /* Class 2 languages: One to one collation  */

         count = -1;  
         while ((*result==0) && (++count < length)) 
                if (string1 [count] != string2 [count]) {
                      if (table [string1 [count]] >
                          table [string2 [count]]) *result = GREATER;
                      else if (table [string1 [count]] <
                               table [string2 [count]]) *result = SMALLER;
                 }
         break;




 case 3: /* Class 3 languages (European)  */
           
         index1   = 0;  /* Pointer to string1 elements */
         index2   = 0;  /* Pointer to string2 elements */
         breaktie = 0;  /* Break tie for 'amost equal' strings */ 

         seq1    = -1;   /* No two-to-one collation in string 1 */
         seq2    = -1;   /* No two-to-one collation in string 2 */

         p1type  = NULL; /* Initial type of the string 1 */
         p2type  = NULL; /* Initial type of the string 2 */

         p1indx  = NULL; /* Initial index of the string 1 */
         p2indx  = NULL; /* Initial index of the string 2 */
 
         p1ord   = NULL; /* Order of previous character in string 1 */
         p2ord   = NULL; /* Order of previous character in string 2 */


         /* Main loop of class 3 collation algorithm */
         while ((index1 < length) && (index2 < length) && (*result==0)) {

             /* Skip don't care characters in string 1 */
             while (((c1ord = table [2*string1[index1]])==DONTCARE)  
                   && (index1 < length)) index1++;

             /* Skip don't care characters in string 2 */
             while (((c2ord = table [2*string2[index2]])==DONTCARE)  
                   && (index2 < length)) index2++;

             /* Check for end of string in string1 or string2 */
             if ((index1 == length) || (index2 == length)) break;



             if (flag21 != 0) {
                seq1 = -1;
                c1type = table [2*string1[index1]+1];  

                if ((p1indx & TWOtoONE) != TWOtoONE) p1indx = c1type;
                    /* That is, if the previous character was not a */
                    /* two-to-one character the type of the current */
                    /* character is enough.                         */
                else {
                    /* Possible two-to-one character has been found */
                    count = 0;
                    tmpaddr = addr21 + p1indx - TWOtoONE;
                    while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                           (count < flag21))  {  
                       if ((s_table[tmpaddr+2*count]%256)==string1[index1]) {  
                          seq1 = s_table [tmpaddr+2*count+1];
                          count= flag21;
                          p1indx = NULL;
                       }
                       count ++;   /* Keep looking */
                    }
                    if (count <= flag21)  {
                       if (s_table[tmpaddr+2*count] == MAXSHORT) {
                           p1type = s_table[tmpaddr+2*count+1];
                           p1indx = c1type;
                       }
       
                      } /* End of count <= flag21 */
                   }  /* End else (p1type != TWOtoONE) statement */
            }         /* End  if  (flag21 != 0) statement        */

             /*                    E n d                               */
             /* ---- Only for languages with two-to-one collation ---- */


             /* Compute now the character types of characters */
             c1type= table[2*string1[index1]+1];
             c2type= table[2*string2[index2]+1];



             if ((breaktie !=0) || (string1 [index1] != string2 [index2])) {
             /* If characters in string1 and string2 not equal:  */


               /* ----  Only for languages with two-to-one collation ---- */
               /*                       B e g i n                         */
                if (flag21 != 0) {
                   seq2 = -1;

                   if ((p2indx & TWOtoONE) != TWOtoONE) p2indx = c2type;
                   else {
                          count = 0;
                          tmpaddr = addr21 + p2indx - TWOtoONE;
                          while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                                 (count <= flag21))  {  
                                 if ((s_table[tmpaddr+2*count]%256) == 
                                     string2[index2])  {  
                                     seq2 = s_table [tmpaddr+2*count+1];
                                     count= flag21;
                                    /* Now reset p2indx to NULL */
                                    p2indx = NULL;
                                 }
                                 count ++;   /* Keep looking */
                          }
                          if (count <= flag21) {  
                             if (s_table[tmpaddr+2*count] == MAXSHORT) {
                                 p2type = s_table[tmpaddr+2*count+1];
                                 p2indx = c2type;
                             }
         
                         } /* End of count <= flag21 */
                   }  /* End else (p2type != TWOtoONE) statement */
  


                    /* Two-to-one collation: Note that the values are   */
                    /* already there:                                   */
                    /*  if seqX !=-1 stringX contains a two-to-one case */
                
                    if (seq1 != -1) { /* Two-to-one char found in str 1 */
                        /* Reset the 'previous character information'   */
                        c1ord  = seq1/256;
                        c1type = seq1%256;
                        if (seq2 == -1) { /* Only one-to-one in str. 2  */
                            c2ord  = p2ord;
                            c2type = p2type;
                        }
                    } /* End Two-to-one char found in string 1 */


                    if (seq2 != -1) { /* Two-to-one char found in str 2 */
                        /* Reset the 'previous character information  ' */
                        c2ord  = seq2/256;
                        c2type = seq2%256;
                        if (seq1 == -1) { /* Only one-to-one in str. 1  */
                            c1ord  = p1ord;
                            c1type = p1type;
                        } 
                    } /* End Two-to-one char found in string 2 */


                }     

                /*                    E n d                               */
                /* ---- Only for languages with two-to-one collation ---- */




             /* ------  Now check the one-to-two characters  ------    */
             /*                  B e g i n                             */

             if (flag12 != 0) {
     
                n1type = DONTCARE;
                n2type = DONTCARE;
		n1ord  = NULL;
		n2ord  = NULL;

                if (((c2type & ONEtoTWO) == ONEtoTWO) &&  
                    ((c1type & ONEtoONE) == c1type)) {

                    /* Skip don't care characters in string 1 */
                    while ((index1 < length-1) &&
                          ((n1ord = table [2*string1[++index1]])==DONTCARE));  

                    n2ord  = table [2*(addr12+(c2type& ~ONEtoTWO)-collseq)];
                    n2type = table [2*(addr12+(c2type& ~ONEtoTWO)-collseq)+1];
                    n1type = table [2*string1[index1]+1];  
                }
                       
  
                else if (((c1type & ONEtoTWO) == ONEtoTWO) &&
                         ((c2type & ONEtoONE) == c2type)) {

                    /* Skip don't care characters in string 2 */
                    while ((index2 < length-1) &&
                          ((n2ord = table [2*string2[++index2]])==DONTCARE));  

                    n1ord  = table [2*(addr12+(c1type& ~ONEtoTWO)-collseq)];
                    n1type = table [2*(addr12+(c1type& ~ONEtoTWO)-collseq)+1];
                    n2type = table [2*string2[index2]+1];  
                }

                else if (((c1type & ONEtoTWO) == ONEtoTWO) &&
                         ((c2type & ONEtoTWO) == ONEtoTWO)) {
     
                     n1ord  = table [2*(addr12+(c1type& ~ONEtoTWO)-collseq)];
                     n1type = table [2*(addr12+(c1type& ~ONEtoTWO)-collseq)+1];
     
                     n2ord  = table [2*(addr12+(c2type& ~ONEtoTWO)-collseq)];
                     n2type = table [2*(addr12+(c2type& ~ONEtoTWO)-collseq)+1];
                }

             }  




               /* ----  Only for languages with two-to-one collation ---- */
               /*                       B e g i n                         */

                if (flag21 != 0) {
                   if ((n1type & TWOtoONE) == TWOtoONE)  {
                        count = 0;
                        tmpaddr = addr21 + n1type - TWOtoONE;
                        while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                               (count < flag21))    
                                count ++;   /* Keep looking */
                        n1type = s_table[tmpaddr+2*count+1];
                    }
      

                   if ((n2type & TWOtoONE) == TWOtoONE)  {
                        count = 0;
                        tmpaddr = addr21 + n2type - TWOtoONE;
                        while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                               (count < flag21))    
                                count ++;   /* Keep looking */
                        n2type = s_table[tmpaddr+2*count+1];
                    }


                   if ((c1type & TWOtoONE) == TWOtoONE)  {
                        count = 0;
                        tmpaddr = addr21 + c1type - TWOtoONE;
                        while (s_table[tmpaddr+2*count] != MAXSHORT) 
                               count ++;   /* Keep looking */
                        c1type = s_table[tmpaddr+2*count+1];
                    }
      
                   if ((c2type & TWOtoONE) == TWOtoONE)  {
                        count = 0;
                        tmpaddr = addr21 + c2type - TWOtoONE;
                        while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                               (count < flag21))    
                                count ++;   /* Keep looking */
                        c2type = s_table[tmpaddr+2*count+1];
                    }


                   if ((p1type & TWOtoONE) == TWOtoONE)  {
                        count = 0;
                        tmpaddr = addr21 + p1type - TWOtoONE;
                        while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                               (count < flag21))    
                                count ++;   /* Keep looking */
                        p1type = s_table[tmpaddr+2*count+1];
                    }
      
                   if ((p2type & TWOtoONE) == TWOtoONE)  {
                        count = 0;
                        tmpaddr = addr21 + p2type - TWOtoONE;
                        while ((s_table[tmpaddr+2*count] != MAXSHORT) &&
                               (count < flag21))    
                                count ++;   /* Keep looking */
                        p2type = s_table[tmpaddr+2*count+1];
                    }

                }        /* End if (flag21 != 0) statement          */


                   if ((c1type & ONEtoTWO) == ONEtoTWO)  c1type = n1type;
                   if ((c2type & ONEtoTWO) == ONEtoTWO)  c2type = n2type;
          


             /*    -------- Collation algorithm for case 3 --------   */

             if (c1ord == c2ord) {
                if (n1ord > n2ord) *result = GREATER;
                   else if (n1ord < n2ord) *result = SMALLER;
                        else if (breaktie == 0) {

                                 if ((p1type  > p2type) ||

                                    ((p1type == p2type) && (c1type > c2type))||

                                    ((p1type == p2type) && 
                                    ((c1type == c2type) && (n1type > n2type))))

                                     breaktie = BGREAT;

                                 else if ((p1type  < p2type) ||

                                    ((p1type == p2type) && (c1type < c2type))||

                                    ((p1type == p2type) && 
                                    ((c1type == c2type) && (n1type < n2type))))

                                     breaktie = BSMALL;


                              } /* End (breaktie == 0)  */



             } else if (c1ord > c2ord) *result = GREATER;
                        else *result = SMALLER;

             } /* End if string1[index1] != string2 [index2] statement */



             /* ----  Only for languages with two-to-one collation ---- */
             /*                       B e g i n                         */

             if ((flag21 != 0) && (*result == 0)) {

                if (seq1 == -1) {      /* No two-to-one case in string1 */
                    p1ord  = c1ord;
                } else {
                    p1ord  = NULL;
                }


                if (string1[index1]==string2[index2]) {
                    p2ord = p1ord;
                    p2type = p1type;
                    p2indx = p1indx;
                } else  { if (seq2 == -1) { /* No two-to-one case in string2 */
                              p2ord  = c2ord;
                          } else {
                              p2ord  = NULL;
                          }
                } 


             }  /* End if (flag21 != 0 statement                      */

             /*                        E n d                            */
             /* ----  Only for languages with two-to-one collation ---- */




             /*   Advance pointers in both strings                    */

             index1 ++;
             index2 ++;

           } /* End while (index1<length) && (index2<length) statement */







           /* Continue scanning the strings to suppress the remaining  */
           /* don't care characters (if the result is still zero)      */
           
           /* It one of the strings is longer than the other after     */
           /* suppressing common substrings and all the don't care     */
           /* characters, the shorter string collates first            */

           /* If both strings have the same length, the 'breaktie'     */
           /* value determines the result.                             */

           if (*result == 0) {
             
             if (index1 < length) do {
                 /* Skip don't care characters in string 1 */
                 c1ord = s_table [string1[index1] + collseq] / 256;  
                 if (c1ord == DONTCARE) index1 ++;
                } while ((c1ord==DONTCARE) && (index1 < length));


             if (index2 < length) do {
                 /* Skip don't care characters in string 2 */
                 c2ord = s_table [string2[index2] + collseq] / 256;  
                 if (c2ord == DONTCARE) index2 ++;
                } while ((c2ord==DONTCARE) && (index2 < length));



             /* Compute the result */
             if (index1 < index2 )  *result = GREATER;
                 else if (index1 > index2)  *result = SMALLER;
                          else   *result = breaktie;
           }  /* End of (*result == 0) statement */  


           break;



 case 4:   /* Class 4 languages: Asian collation     */
 default:  /* Other class languages: Unknown collation  */

       	 if ((n = getl_info(tlang)) == NULL) {
	    *error = 3;   /* Invalid table entry */
            if ((n = getl_info(0)) == NULL) return;
         }
         pcharset = (unsigned char *) n->char_set_definition;

         for (count=0; ((count < length-1) && (*result == 0)); ) {
             if (NLCHARTYPE(string1[count]) == NLI_FIRSTOF2) {
		 c1ord = 256 * string1 [count] + string1 [count+1];
	     } else c1ord = string1 [count];
             if (NLCHARTYPE(string2[count]) == NLI_FIRSTOF2) {
		 c2ord = 256 * string2 [count] + string2 [count+1];
	         count ++;
	     } else c2ord = string2 [count];
	 if (c1ord > c2ord) *result = GREATER;
	 else if (c1ord < c2ord) *result = SMALLER;
	 count ++;
         }
	 if (*result == 0) {
	    if (string1 [length-1] > string2 [length-1]) *result=GREATER;
	    else if (string1 [length-1] < string2 [length-1]) *result = SMALLER;
         }

         break;


  }   /* End CASE statement */




}     /* End of routine    */
