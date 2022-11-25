# ifdef NLS16 /* multibyte character support. */

# include "ldefs.c"
# include "msgs.h"

 /* This table holds the factoring values for the currently supported locales.*/
Range_Factors locale_rng_table[] = {
   {
     "/:chinese-s;/",
     0xa1, 0xfe, 0x21, 0x7e,
     0,    0,    0xa1, 0xfe },
   {
     "/:chinese-t;/",
     0x80, 0xfe, 0x21, 0x7e,
     0,    0,    0x80, 0xfe },
   {
     "/:japanese;/",
     0x80, 0xa0, 0x21, 0x7e,
     0xe0, 0xff, 0x80, 0xff },
   {
     "/:korean;/",
     0xa1, 0xfe, 0xa1, 0xfe,
     0,    0,    0,    0 }
  };

#define Num_Locales 4 /* Must be set to the # of entries in locale_rng_table. */

/*
** This returns a factor struct for representing the factors of the multibyte
** portion of a '.' character.  It looks up the values in the locale_rng_table
** static table.  If no entry exists, a value of 0 is returned, otherwise
** a pointer to the table entry is returned (a Range_fact_ptr). 
*/
Range_fact_ptr lookup_factors()
{
    int i;
    char * collate_lang;

    collate_lang = setlocale(LC_COLLATE, 0);

    for (i=0; i < Num_Locales; i++) 
       if ( ! strcmp(locale_rng_table[i].locale, collate_lang) )
          return( & locale_rng_table[i] );
    return 0;
}


#define IncrChar(a1, a2)       a2++; if(a2==0x00) a1++;
#define DecrChar(a1, a2)       a2--; if(a2==0xFF) a1--;

/*
** This returns a factor struct for representing the factors of the multibyte
** portion of a '.' character.  This consists of all valid 2 byte characters.
** It uses some pretty fancy RE factoring to develop a representation for
** all valid multibyte characters to use as few tree nodes and as much
** intelligence as possible.
** This will factor HPs current representation of the japanese character
** set into [\200-\240\340-\377][\041-\176\200-\377] (octal) or
**          [80-A0 E0-FF] [21-7E 80-FF] (hex)
** These ranges represent all valid Firstof2 and Secof2 combinations in
** The Japanese character set.  The algorithm used here is general and is able
** to factor most multibyte languages like this.
**
** In the above hex example, the values of the range variables would be:
**       frst1on=80 frst1off=A0     frst2on=E0 frst2off=FF
**       sec1on=21  sec1off=7E      sec2on=80  sec2off=FF
** 
** This routine is affected by the setting of LC_CTYPE and LC_COLLATE.
** If this routine is unable to calculate factors for the current character set
** a value of 0 is returned.
*/
Range_fact_ptr calculate_factors()
{
   static Range_Factors ret_val;
   static uchar frst1on=0, frst2on=0, frst1off=0, frst2off=0;
   static uchar sec1on=0, sec2on=0, sec1off=0, sec2off=0;
   static int   packed_ccl_calculated = 0;
   static int   uncompactable  = 0;

   if (!packed_ccl_calculated) {
      uchar f=0x80, s=0x00, lastfch=0;
      int toggle = 1;
      int twobyte;

      /* set f and s to represent the first valid 2-byte character. */
      while ( !TwoByte(f, s) && (f!=0xff || s!=0xff)) {
         IncrChar(f,s);
      }
      packed_ccl_calculated = 1;
      if (f==0xff && s==0xff)
         return 0;
      frst1on = f;
      sec1on = s;

      /* Examine 2-byte char set looking for patterns (well-behaived holes) */
      while ( f != 0xff  || s != 0xff) {
         IncrChar(f,s);
         twobyte = TwoByte(f, s);
         if ((toggle == 1) && !twobyte) { /* found end of a range */
            toggle = 0;
            DecrChar(f,s);

            /* Retain the FirstOf2 value at which this range ended */
            if (frst1off == 0)
               frst1off = f;
            else
               if (frst1off+1 == f)
                  frst1off++;
               else
                  if (frst1off != f)
                     if (frst2off == 0)
                        frst2off = f;
                     else
                        if (frst2off != f)
                           if (frst2off+1 == f)
                              frst2off++;
                           else {
                              uncompactable  = 1;
                              break; /* Out of while */
                           }

            /* Retain the SecOf2 value at which this range ended */
            if (sec1off == 0)
               sec1off = s;
            else
               if (sec1off != s)
                  if (sec2off == 0) 
                     sec2off = s;
                  else
                     if (sec2off != s) {
                        uncompactable  = 1;
                        break; /* Out of while */
                     }
            IncrChar(f,s);
         } /* end if range end */

         if ((toggle == 0) && twobyte) {  /* found beginning of a range */
            toggle = 1;

            /* Retain the FirstOf2 value at which this range started. */
            if (frst1on != f)
               if (lastfch != f && lastfch+1 != f)
                  if (frst2on == 0)
                     frst2on = f;
                  else
                     if (frst2on != f)
                        if (lastfch != f && lastfch+1 != f) {
                           uncompactable  = 1;
                           break; /* Out of while */
                        }
            lastfch = f;

            /* Retain the SecondOf2 value at which this range started. */
            if (sec1on != s)
               if (sec2on == 0) {
                  sec2on = s;
                  }
               else
                  if (sec2on != s) {
                        uncompactable  = 1;
                        break; /* Out of while */
                  }
         } /* end if range beginning */
      } /* while */
   } /* Heavy factored ccl calculation */

   if (uncompactable) return 0;
   else {
      ret_val.first1on = frst1on;
      ret_val.first1off= frst1off;
      ret_val.sec1on   = sec1on;
      ret_val.sec1off  = sec1off;
      ret_val.first2on = frst2on;
      ret_val.first2off= frst2off;
      ret_val.sec2on   = sec2on;
      ret_val.sec2off  = sec2off;
      return ( & ret_val );
   }
}

# endif /* NLS16 */
