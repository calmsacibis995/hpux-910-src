# ifdef NLS16 /* multibyte character support. */

# include "ldefs.c"
# include "msgs.h"

#define TwoByteEl(arr,i,len) (i<len && FIRSTof2(arr[i]) && SECof2(arr[i+1]))
#define Hyphen_op     1
#define Character     2
#define Esc_Character 3

/* 
** This is a special version of the usescape routine in sub2.c that advances
** an array index along an input array while determining the escaped value.
** the array index is modified to indicate how much movement has occurred.  
**
*/
int usescape_nls(input,inloc)
  uchar input[];
  int *inloc;
{
        int loc = *inloc; 
        uchar d;
        unsigned int c=input[++loc];

        switch(c){
        case 'n': c = '\n'; loc++; break;
        case 'r': c = '\r'; loc++; break;
        case 't': c = '\t'; loc++; break;
        case 'b': c = '\b'; loc++; break;
        case 'f': c = 014;  loc++; break;             /* form feed for ascii */
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
                c -= '0';
                while('0' <= (d=input[loc++]) && d <= '7'){
                        c = c * 8 + (d-'0');
                        if(!('0' <= input[loc] && input[loc] <= '7')) break;
                        }
                break;
        case 'x': /* Hexidecimal escape sequences POSIX requirement 8/30/91 */
                c = 0;
                loc++; /* skip over the x */
                while(('0' <= (d=input[loc++]) && d <= '9') ||
                      ('A' <= d && d <= 'F')         ||
                      ('a' <= d && d <= 'f'))
                        {
                        if ('0' <= d && d <= '9')
                              c = c * 16 + (d-'0');
                        else if ('a' <= d && d <= 'f')
                              c = c * 16 + (d- 'a' + 10);
                        else
                              c = c * 16 + (d- 'A' + 10);
                        if(!((input[loc] >= '0' && input[loc] <= '9') ||
                             (input[loc] >= 'a' && input[loc] <= 'f') ||
                             (input[loc] >= 'A' && input[loc] <= 'F'))) break;
                        }
                break;
	default: loc++; break;
        }
        *inloc = loc;
        return(c);
}

/*
** This copies the contents of oldccl into newccl with all escaped characters
** being converted to their appropriate values.  The return value is the size
** of the new ccl.
*/
int unescape_ccl(newccl, oldccl, oldlen)
   uchar newccl[], oldccl[];
   int   oldlen;
{
   int iold, inew;
   unsigned int val;

   for(iold=0, inew=0; iold<=oldlen; iold++) 
     if (oldccl[iold] == '\\') {
        if ((iold==0) || (iold > 0 && !TwoByte(oldccl[iold-1], oldccl[iold]))) {
          val=usescape_nls(oldccl, &iold);
          if (val <= 0xFF)
             newccl[inew++] = val;
          else { /* esc seq represents more than a single byte value */
             /* Big Assumption- we are assumming maximum representable value */
             /* in an escape sequence can fit in a 32 bit word.              */
             while((val & 0xFF000000) == 0) 
                  val = val << 8; /* shift out '0' bytes */
             while(val != 0) { 
                  /* copy the valid bytes- leftmost first. */
                  newccl[inew++] = (val >> 24) & 0xFF;
                  val = val << 8;
             }
          }
          iold--;
        }
        else newccl[inew++] = oldccl[iold];
     }
     else newccl[inew++] = oldccl[iold];
  return(inew-1);
}


/*
** This builds a CCL for a range of single byte characters.  It is similar
** to the code found in parser.y in yylex when scanning the '[' character.
** It returns a tree node (index).
*/
int build_sb_rng(from, to)
   uchar from, to;
{
   extern uchar symbol[NCH];
   extern uchar *p;
   static uchar token[TOKENSIZE];
   int ret_ccl, i, j=0;
   if (from > to)
      error(BADRANGE1, ctos(b_1,from), ctos(b_2,to));
   for(i = 1; i < from; i++) symbol[i] = 0;
   for(i = from; i <= to; i++) {
      symbol[i] = 1;
      token[j++] = i;
   }
   token[j] = 0;
   for(i = to + 1; i < NCH; i++) symbol[i] = 0;

   /* try to pack the ccl */
   p = ccptr;
   if (optim) {
      p = ccl;
      while(p <ccptr && scomp(token,p) != 0) p++;
   }
   if (p < ccptr)  /* Found a matching ccl. */
      ret_ccl = (int) p;
   else {
      int sl;
      ret_ccl = (int) ccptr;
      sl = slength(token) + 1;
      if(ccptr + sl >= ccl + cclsize)
         error(MANYCLASSES);
      scopy(token,ccptr);
      ccptr = ccptr + sl;
   }
   cclinter(1); /* 1 indicates this is a CCL */
   return (mn1(RCCL, ret_ccl));
}


/*
** This builds a CCL for a "holed" range of single byte characters.  It is 
** similar to the code found in parser.y in yylex when scanning the '[' 
** character.  A "holed" range looks like [a-gr-z] where the "hole" would
** be [h-q].  It returns a tree node (index).
*/
int build_sbholed_rng(from1, to1, from2, to2)
   uchar from1, to1, from2, to2;
{
   extern uchar symbol[NCH];
   extern uchar *p;
   static uchar token[TOKENSIZE];
   int ret_ccl, i, j=0;
   if (from1 > to1 || from2 > to2)
      error(BADRANGE2, ctos(b_1,from1), ctos(b_2,from2), 
                       ctos(b_3,to1), ctos(b_4,to2));

   for(i = 1; i < NCH; i++) symbol[i] = 0;
   for(i = from1; i <= to1; i++) {
      symbol[i] = 1;
      token[j++] = i;
   }
   for(i = from2; i <= to2; i++) {
      symbol[i] = 1;
      token[j++] = i;
   }
   token[j] = (uchar) 0;

   /* try to pack the ccl */
   p = ccptr;
   if (optim) {
      p = ccl;
      while(p <ccptr && scomp(token,p) != 0) p++;
   }
   if (p < ccptr)  /* Found a matching ccl. */
      ret_ccl = (int) p;
   else {
      int sl;
      ret_ccl = (int) ccptr;
      sl = slength(token) + 1;
      if(ccptr + sl >= ccl + cclsize)
         error(MANYCLASSES);
      scopy(token,ccptr);
      ccptr = ccptr + sl;
   }
   cclinter(1); /* 1 indicates this is a CCL */
   return (mn1(RCCL, ret_ccl));
}


                               /* Builds a R.E. of the form ch[fr-to] */
#define CatCharRng(ch, fr, to) mn2(RCAT, mn0(ch), build_sb_rng(fr, to))

                               /* Builds a R.E. of the form [a1-a2][b1-b2] */
#define CatRngRng(a1,a2,b1,b2) mn2(RCAT,build_sb_rng(a1,a2),  \
                                         build_sb_rng(b1,b2))

#define IncrChar(a1, a2)       a2++; if(a2==0x00) a1++;
#define DecrChar(a1, a2)       a2--; if(a2==0xFF) a1--;

/*
** This takes a multibyte range [f1f2-t1t2] and breaks it into multiple
** subranges that are '|'ed together.  Subranges are created whenever a
** hole in the characterset is incountered.  Aside from holes, it factors
** a range of the form [ay-bz] as follows:
**    if (a==b)
**        Result:     (a[y - z])
**    if (a+1 == b)
**        Result:     (a[y - 0xFF] | b[0x00 - z])
**    otherwise
**        Result:     (a[y - 0xFF] | [a+1 - b-1][0x00 - 0xFF] | b[0x00 - z])
**
** This routine is effecient for small ranges.  However, large ranges with
** many holes will result in a large internal data tree.
*/
int build_mb_rng(f1, f2, t1, t2)
   uchar f1, f2, t1, t2; /* 1st & 2nd bytes of the from & to multibyte chars. */
{
   uchar tmpto1, tmpto2;
   int  ret_tree, is_full_range = 0;

   if ((f1 > t1) || (f1==t1 && f2 > t2))
       error(BADRANGE2, ctos(b_1,f1), ctos(b_2,f2), ctos(b_3,t1), ctos(b_4,t2));

   /* First determine if this range has any holes in it. */
   tmpto1 = f1;
   tmpto2 = f2;
   while (tmpto1<t1 || (tmpto1==t1 && tmpto2<t2)) {
      if ( !(TwoByte(tmpto1,tmpto2)) )
         break;
      IncrChar(tmpto1, tmpto2);
   }
   if (tmpto1==t1 && tmpto2==t2 && TwoByte(tmpto1,tmpto2))
      is_full_range = 1;
   else {
      DecrChar(tmpto1, tmpto2);
   }
   
   /* Build a tree for the range (or partial range if there is a hole). */
   if (f1==tmpto1) {
      ret_tree = CatCharRng(f1,f2,tmpto2);
   }
   else
   if (f1+1 == tmpto1) {
      ret_tree = mn2(BAR, CatCharRng(f1, f2, 0xFF), 
                          CatCharRng(tmpto1, 0x00, tmpto2));
   }
   else {
      ret_tree = mn2(BAR, mn2(BAR, CatCharRng(f1, f2, 0xFF),
                                   CatRngRng(f1+1, tmpto1-1, 0x00, 0xFF)),
                          CatCharRng(tmpto1, 0x00, tmpto2));
   }
   if (is_full_range)
      return(ret_tree);
   else {
      /* There's a hole, find the end and '|' the curr tree with the remainder*/
      do {
         IncrChar(tmpto1, tmpto2);
      } while ((tmpto1<t1 || (tmpto1==t1 && tmpto2<t2)) && 
               !(TwoByte(tmpto1,tmpto2)));
      return(mn2(BAR, ret_tree, build_mb_rng(tmpto1, tmpto2, t1, t2))); 
    }
}


static uchar local_ccl[CCLSIZE];

/*
** Called by do_multibyte_class, this will determine if the input (which is
** a range, such as a-z) is composed of multibyte characters or single byte
** characters and will call the appropriate routine to build the tree.
*/
int build_range_set(in_range, len)
{
   int multi_from, multi_to;

   len = unescape_ccl(local_ccl, in_range, len);
   multi_from = TwoByteEl(local_ccl, 0, len);
   multi_to   = TwoByteEl(local_ccl, 3, len);
   
   if (!multi_from && !multi_to)
      return(build_sb_rng(local_ccl[0], local_ccl[2]));
   else
   if (multi_from && multi_to)
      return(packed_mb_range(local_ccl[0],local_ccl[1],local_ccl[3],local_ccl[4]));
   else
      error(UNEQUALRNG);
}

/*
** Creates a tree that recognizes a ccl with no ranges such a [abcryz].
** It basically '|'s together each of the characters into a tree.
*/
int build_char_set(in_ccl, len)
   uchar in_ccl[];
   int len;
{
   int j, i=0;
   int tree, child;

   len = unescape_ccl(local_ccl, in_ccl, len);

   tree = mn0(local_ccl[i++]);
   if (TwoByteEl(local_ccl, 0, len))
      tree = mn2(RSTR, tree, local_ccl[i++]);

   for (j = i; j < len; j++) {
      child = mn0(local_ccl[j]);
      if (TwoByteEl(local_ccl, j, len))
         child = mn2(RSTR, child, local_ccl[++j]);
      tree = mn2(BAR, tree, child);
   }
   return (tree);
}




/*
** Used in searching for ranges in multibyte CCLs.  Returns Character if the 
** character at index "loc" of "input" is not a Hyphen ("-").  That includes
** any multibyte character or escaped character.  Also, if a "-" is found at
** first location in a CCL, is is not considered to be a range operator 
** (hyphen). This routine also updates the input arg "loc" to reflect how many
** bytes were consumed on this character.
** Input: unsigned char array "input" which is a CCL with a trailing ] but
**             no leading [
**        pointer to an integer "loc" which is the index of the element to
**             check.
** Output: *loc is updated to reflect a 1 or 2 byte advancement over the input.
**         Return value of 'Character' if input[*loc] is a non-hyphen char
**         Return value of 'Esc_Character' if input[*loc] is a non-hyphen 
**           escaped hexidecimal sequence, octal sequence, or escaped character.
**         Return value of 'Hyphen_op' if input[*loc] is a range operator ('-')
*/
int rsymtype(input, loc)
   uchar input[];
   int *loc;
{
   int j = *loc;
   uchar last = input[j++];
   uchar curr = input[j];
   if (TwoByte(last, curr)) {
         j++;
         *loc = j;
         return(Character);
      }
   else if (last == '\\') {
         *loc = j-1;
         usescape_nls(input, loc);
         return(Esc_Character);
      }
   else   {
         *loc = j;
         if (last != '-')
            return(Character);
         else
            return (Hyphen_op);
      }
}


/*
** Used in searching for ranges in multibyte CCLs.  Returns a -1 if the    
** input CCL doesnt have a range in it (e.g., a-z).  If a range does exist 
** it returns the index that the range starts at.                          
** Input: unsigned char array "input" which is a CCL with a trailing ] but
**             no leading [
**        integer "len" which is the length of the CCL (not including the
**             trailing ']'
** Output: Return value of -1 if the multibyte input CCL does not contain a 
**         range. Otherwise, it returns the starting index of the range.
*/
int findrange(input,len)
   uchar input[];
   int len;
{
   int cur_loc=0, last_loc, c1, c2;
   int ret_val= -1;
   if (len <= 1) return (-1);
   for ( c2=rsymtype(input,&cur_loc); cur_loc <= len && ret_val == -1; ) 
      {
         last_loc = cur_loc;
         c1 = c2;
         c2 = rsymtype(input,&cur_loc);
         if (((c1 == Character) || (c1 == Esc_Character)) && (c2 == Hyphen_op)) 
            ret_val = last_loc;
         if (cur_loc >= len) 
            ret_val = (c2 == Hyphen_op) ? last_loc : -1;
      }
   /* patch up the return val to point to the beginning of the range. */
   switch (ret_val) {
      case -1: break;
      case  1: ret_val = 0; 
	       break;
      default: if (c1 == Esc_Character)
                  while(input[ret_val] != '\\')
                     ret_val--;
               else if (FIRSTof2(input[ret_val-2]))
                  ret_val = ret_val - 2;
               else 
                  ret_val = ret_val - 1;
      }
   return(ret_val);
}


/*
** Used in searching for ranges in multibyte CCLs.  It takes a Character Class
** (CCL) as input and the starting index of a range in the CCL.  It returns the
** index of the last character in the range.  
** Input: unsigned char array "input" which is a CCL with a trailing ] but
**             no leading [
**        integer "len" which is the length of the CCL (not including the
**             trailing ']'
**        integer "loc" which is the starting index of the range to work with.
**             trailing ']'
** Output: Return value is index of the end of the range.  If the range looks
**             like ...a-] (i.e. no 'to' value) it returns "len".
*/
int range_end(input, len, loc)
   uchar input[];
   int len, loc;
{
   if (len-loc <= 2) return len;
   else { /* consume 2 chars (the "from" and "-") */
#ifdef NLSDEBUG
      int x;
      if (((x=rsymtype(input, &loc)) != Character) && x!=Esc_Character)
         printf("Error in range_end: input is not a char");
      if (rsymtype(input, &loc) != Hyphen_op)
         printf("Error in range_end: input is not a Hyphen");
#else
      rsymtype(input, &loc);
      rsymtype(input, &loc);
#endif
      return(loc);
   }
}

/*
** This will break a CCL into its component ranges ([a-z]) and groups of 
** characters ([abc]) and will create and '|' together the components.
** It works recursively by separating the CCL until a single range or
** a single group of characters is found.  It then farms out the work
** to build_range_set for ranges and build_char_set for character
** groups.
*/
int do_multibyte_class(in_ccl, len)
   uchar in_ccl[]; 
   int           len;  
{
   int rng_strt, rng_end;   
   int next_char, rng_only;
   
   if (len == 0) {
#if NLSDEBUG
      printf("Illegal length of 0 in do_multibyte_class");
#endif
      return(0);
   }

   rng_strt = findrange(in_ccl, len);
   if (rng_strt == -1) { /* there is no range in this CCL */
      return( build_char_set(in_ccl, len) );
   }
   else {
      next_char = rng_end = range_end(in_ccl, len, rng_strt);
      if ( FIRSTof2(in_ccl[rng_end]) )
         next_char = next_char + 2;
      else {
         next_char++;
         /* include entire character of an escape sequence. */
         if (in_ccl[rng_end] == '\\') 
            if (in_ccl[next_char] == 'x') {
                /* Hex escape sequence for POSIX 8/30/91 */
                next_char++;
                while((in_ccl[next_char] >= '0' && in_ccl[next_char] <= '9') ||
                      (in_ccl[next_char] >= 'a' && in_ccl[next_char] <= 'f') ||
                      (in_ccl[next_char] >= 'A' && in_ccl[next_char] <= 'F')) 
                {
                   next_char++; 
                }
            }
            else
               while(in_ccl[next_char] >= '0' && in_ccl[next_char] <= '7')
                  next_char++;
      }
      rng_only = (rng_strt == 0) && (next_char >= len);

      if (rng_only) {
         return( build_range_set(in_ccl, len) );
      }
      else { /* We have comb. of ranges and characters. Divide and conquer */
         if (rng_strt == 0) {
            return( mn2(BAR,
           		do_multibyte_class(in_ccl,next_char),
               		do_multibyte_class(in_ccl + rng_end, len - rng_end)));
         }
         else {
            return( mn2(BAR,
                        do_multibyte_class(in_ccl, rng_strt),
                        do_multibyte_class(in_ccl + rng_strt, len - rng_strt)));
         }
      }
   }
}


/*
**  This is a more sophisticated version of build_mb_rng.  It attempts to
**  find patterns in the current character set and take advantage of them
**  to build a representation of a range with as few nodes as possible.
**  It is similar to do_multibyte_dot except that the starting and ending 
**  points are not the same.  
**  It breaks the range into small leading and trailing ranges and combines
**  that with a large middle section that pays attention to the character set 
**  patterns.  See the do_multibyte_dot header for more information.
**  If this routine cannot factor the input multibyte Character Range it
**  will ship off all the work to the default routine build_mb_rng.
*/
int packed_mb_range(f1,s1,f2,s2)
   uchar f1,s1,f2,s2;
{
   uchar fbody1, sbody1, fbody2, sbody2, sbend;
   int   tree1, tree2, tree3, ltree, rtree;
   Range_fact_ptr factors, lookup_factors(), calculate_factors();

   if ( !(TwoByte(f1, s1)) || !(TwoByte(f2, s2)))
      error(BADRANGE3);

   if (f1 == f2) /* Small range, use default. */
      return(build_mb_rng(f1,s1,f2,s2));

   factors = lookup_factors();
   if ( ! factors )
      factors = calculate_factors();
   if ( ! factors ) 
      return(build_mb_rng(f1,s1,f2,s2)); /* Can't factor, use defaault */

   /* Calculate the middle section boundaries (the body of the range). */
   if ((f1 == factors->first1off) && (factors->first2on !=0))
      fbody1 = factors->first2on;
   else 
      if ((f1 == factors->first1off) || (f1 == factors->first2off))
         return (build_mb_rng(f1,s1,f2,s2));/* Small, use default */
      else
         fbody1 = f1 + 1;
   sbody1 = factors->sec1on;
   if (f2 == factors->first2on)
      fbody2 = factors->first1off;
   else
      if (f2 == factors->first1on)
         return (build_mb_rng(f1,s1,f2,s2));/* Small, use default */
      else 
         fbody2 = f2-1;
   if (factors->sec2off != 0)
      sbody2 = factors->sec2on;
   else
      sbody2 = factors->sec1off;
         
   sbend = factors->sec2on ? factors->sec2off : factors->sec1off;

   /* It's factorable. Combine, beginning, middle and trailing components. */
   tree1 = build_mb_rng(f1, s1, f1, sbend);           /* beginning */
   if (f1+1 < f2) {
      if ((fbody2 <= factors->first1off) ||           /* middle */
          ((factors->first2on != 0) && (fbody1 >= factors->first2on)))
         ltree = build_sb_rng(fbody1, fbody2);
      else
         ltree = build_sbholed_rng(fbody1, factors->first1off, factors->first2on, fbody2);
      if (factors->sec2on == 0)
         rtree = build_sb_rng(sbody1, sbody2);
      else 
         rtree = build_sbholed_rng(sbody1, factors->sec1off, sbody2, factors->sec2off);
      tree2 = (mn2(RCAT, ltree, rtree));
      tree1 = (mn2(BAR, tree1, tree2));
   }
   tree3 = build_mb_rng(f2, factors->sec1on, f2, s2);  /* end */

   return (mn2(BAR, tree1, tree3));
}


/*
** This returns a node for the multibyte portion of a '.' character.  This
** consists of all valid 2 byte characters.  It first looks up the current
** locale in a table (via lookup_factors).  If there is no table entry, it
** trys to calculate the factors (via calculate_factors).  If that fails,
** it uses a brut force algorithm of building a range from [0000-FFFF].
*/
int do_multibyte_dot()
{
   static int   calculated = 0;
   Range_fact_ptr calculate_factors(), lookup_factors();
   Range_fact_ptr factors;
   int ltree, rtree;

   factors = lookup_factors();
   if ( ! factors )
      factors = calculate_factors();
   if ( factors ) {
      if (factors->first2on == 0 && factors->first2off == 0)
         ltree = build_sb_rng(factors->first1on, factors->first1off);
      else
         ltree = build_sbholed_rng(factors->first1on, factors->first1off, 
                                   factors->first2on, factors->first2off);
      if (factors->sec2on == 0 && factors->sec2off == 0)
         rtree = build_sb_rng(factors->sec1on, factors->sec1off);
      else
         rtree = build_sbholed_rng(factors->sec1on, factors->sec1off, 
                                   factors->sec2on, factors->sec2off);
      return (mn2(RCAT, ltree, rtree));
   }
   else {
      /* Not well behaived enough to factor heavily.  Use brut force algo. */
      static uchar a1=0x80, a2=0x00, b1=0xff, b2=0xff;
      static int boundaries_calculated;

      if (!boundaries_calculated) {
         while ( !TwoByte(a1, a2)) { IncrChar(a1,a2); }
         while ( !TwoByte(b1, b2)) { DecrChar(b1,b2); }
         boundaries_calculated = 1;
      }
      return (build_mb_rng(a1,a2,b1,b2));
   }
}
# endif /* NLS16 */

#ifdef POSIX_ERE

/*
** This is used to handle "collating symbols" as decribed in Posix 1003.2    
** Collating symbols are enclosed within bracket-period ([.  .]) delimiters.
** They are used to specifically define a multibyte collating symbol, such 
** as the letter "ch" in Spanish.  e.g, [.ch.].                           
*/
void  do_posix_collat_sym(symbol, type)
   uchar symbol[];
   int type;
{
   static uchar colsym_req[8];
   uchar c;
   int trnode, len, i;

   /* scan for the entire collating symbol. */
   for (len=0; ((((c = gch()) != '.') || (peek != ']')) && (c != '\n')); len++){
       if (c == '\\') c = usescape(c=gch());
       colsym_req[len] = c;
       }
   gch(); /* consume the trailing ']' */
   if (c=='\n') {
       error(BADCOLLSYM);
       return;
       }

   switch (len) {
       case 0:
            warning(EMPTYCOLLSYM);
            return;
       case 1: 
            /* enter single byte collating symbol directly into array */
            symbol[ colsym_req[0] ] = 1;
            return;
       default: 
            /* create tree node to be added later. */
            trnode = mn0(colsym_req[0]);
            for(i=1; i < len; i++)
                 trnode = mn2(RSTR, trnode, colsym_req[i]);
            if (col_syms == None) 
               col_syms = trnode;
            else 
               col_syms = mn2(BAR, col_syms, trnode);
            return;
       }
}


/*
** This is used to handle "equivalence classes" as decribed in Posix 1003.2    
** Equivalence classes are enclosed within bracket-equal ([=  =]) delimiters.
** They are used to specify a set of characters coressponding to a given base
** character.  For example, under German, [=a=] would correspond to "a", "a 
** umlat", "a accent", etc. 
*/
void do_posix_equiv_class(symbol)
   uchar symbol[];
{
   uchar eq_char = gch();
   int  i, equiv_class;

   if (gch() != '=') {
        error(BADEQVCLS);
        return;
        }
   else if (gch() != ']') {
        error(BADEQVCLS);
        return;
        }

   equiv_class = _seqtab[ eq_char ]; 
   for (i=0; i < 256; i++) 
        if ( _seqtab[i] == equiv_class)
             symbol[i] = 1;
   
}


/*
**  The declarations below are all used in building POSIZ character classes
**  Which are part of Extended Regular Expressions (ERE's).  They are used in
**  the routine do_posix_ccl().
*/

typedef struct ccl_elem {
    uchar *name;       /* name of the character class expression */
    uchar name_size;   /* # of chars in the name string */
    int   initialized; /* set if this values have been calculated for this ccl*/
    int   token;       /* integer code for the routine */
    int   num_elements;/* number of characters in this ccl (if initialiazed) */
    uchar data[NCH];   /* the elements of this ccl ("num_elements" of them) */
} ccl_type;

/* These #defines must match the order they are used in the initialization */
/* of the array "table" below. */
#define isalnum_tok   0
#define isalpha_tok   1
#define iscntrl_tok   2
#define isdigit_tok   3
#define isgraph_tok   4
#define islower_tok   5
#define isprint_tok   6
#define ispunct_tok   7
#define isspace_tok   8
#define isupper_tok   9
#define isxdigit_tok 10

static ccl_type table[]= { {"alnum",5,0,isalnum_tok}, 
                           {"alpha",5,0,isalpha_tok}, 
                           {"cntrl",5,0,iscntrl_tok}, 
                           {"digit",5,0,isdigit_tok}, 
                           {"graph",5,0,isgraph_tok}, 
                           {"lower",5,0,islower_tok}, 
                           {"print",5,0,isprint_tok}, 
                           {"punct",5,0,ispunct_tok}, 
                           {"space",5,0,isspace_tok}, 
                           {"upper",5,0,isupper_tok}, 
                           {"xdigit",6,0,isxdigit_tok}
                         };
#define Table_size  10  /* MUST EQUAL the # entries - 1 in table */

/*
** Routine: do_posix_ccl()
**
** This is used to handle "character class expressions" as decribed in Posix 
** 1003.2. Character class expressions enclosed within bracket-colon ([:  :]) 
** delimiters.  They are used to specify a set of characters as defined 
** in the LC_CTYPE category of the current locale.  Examples of character
** class expressions are [:alpha:], [:digit:], [:cntrl:], etc.
** The functions (macros) used to determine this information are defined by
** X/Open and are the is* functions.
*/
void do_posix_ccl(symbol)
   uchar symbol[];
{
   static uchar ccl_req[NCH];
   uchar c;
   int i,j,not_done=1;

   /* scan for the name of the Posix ccl. */
   for(i=0; ((((c = gch()) != ':') || (peek != ']')) && (c != '\n')); i++)
      ccl_req[i] = c;
   gch(); /* consume the trailing ']' */
   if (c=='\n') {
      error(BADCHARCLS);
      return;
      }

   /* get the characters in the ccl */
   for(i=0; (i < Table_size) && not_done; i++) {
      if (!strncmp(table[i].name, ccl_req, table[i].name_size)) {
          if (table[i].initialized == 0) { /* initialize this entry */
                 table[i].initialized = 1; 
                 table[i].num_elements = 0;
                 switch (table[i].token) {
                    case isalnum_tok:
                         for(j=0; j<NCH; j++) 
                            if (isalnum(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isalpha_tok:
                         for(j=0; j<NCH; j++) 
                            if (isalpha(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case iscntrl_tok:
                         for(j=0; j<NCH; j++) 
                            if (iscntrl(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isdigit_tok:
                         for(j=0; j<NCH; j++) 
                            if (isdigit(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isgraph_tok:
                         for(j=0; j<NCH; j++) 
                            if (isgraph(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case islower_tok:
                         for(j=0; j<NCH; j++) 
                            if (islower(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isprint_tok:
                         for(j=0; j<NCH; j++) 
                            if (isprint(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case ispunct_tok:
                         for(j=0; j<NCH; j++) 
                            if (ispunct(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isspace_tok:
                         for(j=0; j<NCH; j++) 
                            if (isspace(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isupper_tok:
                         for(j=0; j<NCH; j++) 
                            if (isupper(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    case isxdigit_tok:
                         for(j=0; j<NCH; j++) 
                            if (isxdigit(j)) 
                               table[i].data[ table[i].num_elements++ ]= j;
                         break;
                    default:
                         error(BADCHARCLS);
                    } /* end switch */
                 } /* end if not initialized */

           /* transfer the data */
           for(j=0; j < table[i].num_elements; j++) {
                 symbol[ table[i].data[j] ] = 1;
                 }
           not_done = 0;
           }
       }
   if (not_done) error(BADCHARCLS);
}

/* Not implemented:                                                        */
/* (1) "^" classes containing multi-byte collating symbols                 */
/* (2) Use of LC_COLLATE in determining ranges in classes.  E.g., under    */
/* Spanish, [a-d] only includes a, b, c, and d.  When in fact it should    */
/* contain a, b, c, ch and d.                                              */
/* A reguest is in with the INTL group to provide support routines to aide */
/* with this.                                                              */

#endif /* POSIX_ERE */

