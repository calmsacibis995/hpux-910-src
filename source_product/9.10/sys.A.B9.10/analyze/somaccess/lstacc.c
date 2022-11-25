/* UNISRC_ID: @(#)lstaccess.c	4.00	85/10/01  */
/*
 *  This is just a version of somaccess.c which does not rely
 *  on all seeks starting from the top of the file.  (Single
 *  soms are all that somaccess.c handles.)
 *  Here, I just note the current file position at the beginning
 *  and seek relative to that.
 *
 *  Note that the user must seek to the beginning of the som_entry
 *  before calling any of these routines.
 */

#include <stdio.h>
#include <a.out.h>
#include "incl_def.h"
#include "somaccess.h"

/*
{**BEGIN_IS**
{**BEGIN_ES**
\#5 "1"
\#4 "1"
\new
\con,in 1
@b@#(OK).@#(2).@#3  Element Location Routine Definitions@s


   These routines will be used to locate information about various
entries contained in SOM components.  Each routine requires the
location of the component being consulted, and some key which
identifies the particular entry which is of interest.  The key
might be an entry index, a string, or a number, depending on
what information is required and how much information is already
known by the calling routine.

   Each routine will return a status variable defining the status
of the function call, plus the required information in the format
defined by the individual function.



{**END_ES**


{**END_IS**

/*
\format
{**BEGIN_IS**
{**BEGIN_ES**
\head "@fc SOM File Access Intrinsics @e space_nmfdr@s"
\new
\con,in 2
@b@#(OK).@#(2).@#(3).@#4 Function space_nmfdr@s
\image
@ft
----------------------------------------------------------------------*/


int space_nmfdr (
             stat,                                /* Output Parameter */
             location,                            /* Input  Parameter */
             sp_name,                             /* Input  Parameter */
             l_d_type,                            /* Input  Parameter */
             index,                               /* Output parameter */
             space_ptr)                           /* Output Parameter */

   status         *stat;
   long_ptr       *location;
   char           *sp_name;
   int             l_d_type;
   int            *index;
   long_ptr       *space_ptr;


/*--------------------------------------------------------------------

   @bDESCRIPTION@s
\format

   This function is used to locate the space record corresponding
   to the space name and type (loadable or defined) for the
   indicated SOM.
   The caller is responsible for checking the content of the
   individual fields of the space record.


   @bPARAMETERS@s

   stat

      The status of the file access.

   location

      This is an input parameter that points to the
      location within the file of the beginning of the
      SOM which defines the space to be found.

   sp_name

      This is an input parameter. Sp_name is the string data
      (in the Space Strings) pointed by the Space_name_pointer
      in the space record.

   l_d_type

      This is an input parameter that qualifies the space name
      as loadable, defined or other combinations.
      Four valid values for this paramater are as follows:

\image
      Value   Meaning

      0       Space loadable and defined
      1       Space loadable but not defined
      2       Space not loadable, not defined
      3       Space not loadable but defined

\format
   index

      This is an output parameter that identifies the
      space record being searched for.  Space
      records will be assigned consecutive indicies
      beginning with 0.  Space index 0 will be the
      first space defined by the SOM. If no space record
      is found for this given sp_name then this index will
      be set to -1.

   space_ptr

      On return to the caller this parameter will
      contain a file pointer to the space record
      being searched for.  If no space record was
      found for the given sp_name, a warning status will
      be returned and the value of this parameter will
      be meaningless.


   @bRETURN VALUE@s

   If no errors occur, this function will return a value
   of zero.  If there are any errors then the function will return a -1.
   If there is an error the stat parameter will contain a value indicating
   what the error was.  If the field stat->error is positive it is a warning,
   if stat->error is negative it is an error.  These error numbers should
   be passed to a error generation facility that can convert the error number
   to a message and print the message on standard output.


   @bASSUMPTIONS AND DEPENDENCIES@s

   For HP-UX, probe always returns true because mapped file is
   not defined.

{**END_ES**

   @bALGORITHM@s

\image
   reset {stat}
   calculate address of space record 0
   {index} = -1
   found = 0
   while (more record in space dictionary && space_dic. total > 0)
      get sp_entry->loadable and sp_entry->defined
      compose temp_type corresponds to these two bit-fields
      if ({l_d_type} == temp_type) then [
         get space_name string from sp_entry->name
         compare ( {sp_name} , space_name string )
         if match then
            {space_ptr} =  address of this space_entry record
            {index} = this record #
            found = 1
            break ]
      else
         --space_dic. total
         get the next record
   if (found) then
       successfully done, return(0)
   else
       {stat} = error
       return(BAD)


{**END_IS**
----------------------------------------------------------------------*/
{  /* space_nmfdr code */
   int debug = 0;
   int i;                  /* loop index */
   int bad;                /* dummy, uses for function calls err. checking */
   int found = 0;          /* flag for matched record */
   int num_char;           /* numbers of character in sp_name string */
   int str_loc;            /* contains space_string_location */
   int str_size;           /* contains space_string_size */
   int sp_loc, sp_total;   /* temporary variable to read file */
   char *the_name;         /* store space name string being fetched from SOM */
   long offset;            /* temporary offset, to access various SOM field */
   int  l_d_temp;          /* l, d bits temporary */
   char *malloc();
   FILE *lstfile;
   long start;
struct space_dictionary_record *space;    /* temp. storage for space_record */



   lstfile = location->hi32.fptr;

   if(debug==1) {

      printf("*****START SPACE NAME FINDER: \n");
      printf("Location = %d\n", lstfile);
      printf("Space name = %s\n", sp_name);
      printf("l_d_type = %d\n", l_d_type);
   
   } /* end debug */

   resetstat(stat);         /* reset status to no errors  */
   *index = 0;              /* set index */

   start = ftell(lstfile);		/* som header starts here */

   /* read space_dic_location for address of space_record # 0 */

   bad = fseek(lstfile, start+SPACE_DIC_LOC, FILE_BEGIN); /*set f. ptr */
   if (bad) {  /* if problem with fseek the som file then exit */
      stat->error = FD_SP_FSEEK_ERR0;   /* set status to fseek error */
      return(BAD);
   }

   bad = fread(&sp_loc, sizeof(sp_loc), ONE, lstfile);/* read loc */
   if (bad != ONE)  {
      stat->error = FD_SP_FREAD_ERR0;
      return(BAD);
   }

   /* if space_dic_loc <= 0 then space_rec not defined! */

   if (sp_loc <= 0) {
      stat->error = FD_SP_0_SPREC0;
      return(BAD);
   }

   bad = fread(&sp_total, sizeof(sp_total), ONE, lstfile);/* read to
tal */

   if (bad != ONE) {
      stat->error = FD_SP_FREAD_ERR1;
      return(BAD);
   }

   /* if space_dic_total <= 0 then space_rec not defined! */

   if (sp_total <= 0) {
      stat->error = FD_SP_0_SPREC1;
      return(BAD);
   }

   /* read string_dic_loc and string_dic_size to find space name latter */

   bad = fseek(lstfile, start+SPACE_STR_LOC, FILE_BEGIN);
   if (bad) {  /* if problem with fseek the som file then exit */
      stat->error = FD_SP_FSEEK_ERR1;   /* set status to fseek error */
      return (BAD);               /* return unsuccessfully */
   }

   bad = fread(&str_loc, sizeof(str_loc), ONE, lstfile);/* read loc
*/
   if (bad != ONE)   {
      stat->error = FD_SP_FREAD_ERR2;
      return(BAD);
   }

   /* if string_dic_loc <= 0 then space_string not defined! */

   if (str_loc <= 0) {
      stat->error = FD_SP_0_SPSTR0;
      return(BAD);
   }

   bad = fread(&str_size, sizeof(str_size), ONE, lstfile);

   if (bad != ONE)  {
      stat->error = FD_SP_FREAD_ERR3;
      return(BAD);
   }

   /* if string_dic_size <= 0 then space_string not defined! */

   if (str_size <= 0) {
      stat->error = FD_SP_0_SPSTR1;
      return(BAD);
   }

   /* allocate space_dictionary_record structure for *space */

   if((space = (struct space_dictionary_record *) 
       malloc(sizeof(struct space_dictionary_record))) == NULL) {
      stat->error = MALLOC_ERR;
      return(BAD);
   }

   if(debug == 1) { /* debugging */

      printf("SPACE DIC. LOC = %x\n", sp_loc);
      printf("SPACE DIC. TOTAL = %x\n", sp_total);
      printf("STRING DIC. LOC = %x\n", str_loc);
      printf("STRING DIC. LOC = %x\n", str_size);

   } /* end debug */

   /* loop1 to read space_record into space structure, one at a time until */
   /* found or run out of space_record  */

   for (i=0; i <= sp_total ; i++)  {
      /* seek to space_record  */
      bad = fseek(lstfile, start+sp_loc, FILE_BEGIN);
      if (bad) {
         stat->error = FD_SP_FSEEK_ERR3;
         break;
      }

      bad = fread(space, sizeof(*space), ONE, lstfile);
      if(bad!=ONE) {
         stat->error = FD_SP_FREAD_ERR4;  /* error from fread space_record */
         break;
      }
      
      /* compose load and define bits */

      if((space->is_loadable) & (space->is_defined)) l_d_temp = LD1_DF1; /* both
 bits on */
      else if(!((space->is_loadable) | (space->is_defined))) l_d_temp = LD0_DF0;
 /* both off */
           else if(space->is_loadable) l_d_temp = LD1_DF0; /* only loadable bit
on */
                else l_d_temp = LD0_DF1; /* only defined on */

      /* get name string only if type requested and l_d_temp are the same */

/* kludge due to nsbofprime bug */
      if (1) {
  /*  if(l_d_temp ==l_d_type) { */
/* end kludge due to nsbofprime bug */

         /* check for valid space_name_pointer */


         if(space->name.n_strx >= str_size) {
            stat->error = FD_SP_SPREC_COR;  /* space record corrupted */
            break;
         }
         if(space->name.n_strx == 0) {
            stat->error = FD_SP_SPNAM_NIL;  /* null name pointer */
            break;
         }
         /* get name string */
         /* calculate offset of name string in SOM, points to the string len */

         offset = space->name.n_strx + str_loc - FOUR;


         /* seek and get the string name that matches the requested l_d type as
composed above, read length first */

         bad = fseek(lstfile, start+offset, FILE_BEGIN);
         if(bad) {
            stat->error = FD_SP_FSEEK_ERR4;
            break;
         }
         bad = fread(&num_char, sizeof(num_char), ONE, lstfile);
         if(bad!=ONE) {
            stat->error = FD_SP_FREAD_ERR5;  /* error from fread string_len  */
            break;
         }
         num_char = num_char + 1;          /* tag in \0 */

         /*  allocate storage for the name string */

         the_name = malloc(num_char);    /* including \0 */

         bad = fread(the_name, sizeof(*the_name), num_char, lstfile);
         if(bad != num_char){
            stat->error = FD_SP_FREAD_ERR6;  /* error from fread string_name  */
            break;
         }
         /* compare the_name and the requested sp_name. If match then we found it */


         if(strcmp(the_name, sp_name) == 0) {    /* it is found!  */
            found = 1;

            /* set up output: file pointer = file pointer to this SOM, file offs
et = offset to this space_record (sp_loc).  */

            space_ptr->hi32.fptr = lstfile;
            space_ptr->offset = start + sp_loc;
            break;   /* get out of loop1 */
         }
      }
      *index += 1;   /* increment to the next space_record */
      sp_loc = sp_loc + sizeof(*space);  /* size of record */

   } /* end loop1  */

   if(debug==1) {

      printf("*****END SPACE NAME FINDER: \n");
      printf("Location = %d\n", lstfile);
      printf("Space name = %s\n", sp_name);
      printf("l_d_type = %d\n", l_d_type);
      printf("FOUND = %d, Index found = %d\n", found, *index);
      printf("Space_ptr fd = %d", space_ptr->hi32.fptr);
      printf("Space_ptr offset = %x\n", space_ptr->offset);
   
   } /* end debug */

   if(found)  /* return good! */
      return(OK);
   else {   /* return bad! */
      *index = -1;
      return(BAD);
   }

} /* end space_nmfdr code */



/*
\format
{**BEGIN_IS**
{**BEGIN_ES**
\head "@fc SOM File Access Intrinsics @e subsp_nmfdr@s"
\new
\con,in 2
@b@#(OK).@#(2).@#(3).@#4 Function subsp_nmfdr@s
\image
@ft
----------------------------------------------------------------------*/


int subsp_nmfdr (
             stat,                                /* Output Parameter */
             location,                            /* Input  Parameter */
             subsp_name,                          /* Input  Parameter */
             ac_type,                             /* Input  Parameter */
             sp_index,                            /* Input  Parameter */
             subsp_index,                         /* Output Parameter */
             subsp_ptr)                           /* Output Parameter */


   status         *stat;
   long_ptr       *location;
   char           *subsp_name;
   unsigned int    ac_type ;
   int             sp_index;
   int            *subsp_index;
   long_ptr       *subsp_ptr;


/*--------------------------------------------------------------------

   @bDESCRIPTION@s
\format

   This function is used to locate the subspace record
   corresponding to the subspace name and its access type
   for the indicated SOM.
   Before returning to the caller this function will probe the pages
   occupied by the subspace record to insure the caller will not
   cause an access violation trying to read the entry.  The caller
   is responsible for checking the content of the individual fields
   of the record.

   @bPARAMETERS@s

   stat

      The status of the file access.

   location

      This is an input parameter that points to the
      location within the file of the beginning of the
      SOM which defines the space to be found.

   subsp_name

      This is an input parameter that indicates the sub space
      name in request. This name must be found in the String
      area or an error will be returned.

   ac_type

      This is an input parameter that specifies the access
      rights for this sub space. Valid values for this parameter
      and their meanings are:
\image

      value        Meaning

      0            Read only data page
      1            Normal data page (read and write)
      2            Normal code page (read, execute)
      3            Dynamic code page (read,write, execute)
      4            Gateway to PL0
      5            Gateway to PL1
      6            Gateway to Pl2
      7            Gateway to PL3

\format
   sp_index

      This is an input parameter which specifies the space
      encompassing the sub space being requested.

   subsp_index

      This is an output parameter that identifies the
      subspace record being searched for.  Subspace
      indicies will be assigned consecutive indicies
      beginning with 0 within the requested space. 
      Subspace index 0 will locate the first subspace
      in the requested space. If no sub_space record
      is found for this given sub_space name then this
      index will be set to -1.

   subsp_ptr

      On return to the caller this parameter will
      contain a file pointer to the subspace record
      being searched for.  If no subspace record was
      found for the given sub_space name, a warning status
      will be returned and the value of this parameter
      will be meaningless.

      NOTE: Upon returning to the caller, if the requested subspace
      record is found, the file offset in this parameter (subsp_ptr)
      is the actual byte offset into the SOM file, NOT the subspace
      dictionary.



   @bRETURN VALUE@s

   If no errors occur, this function will return a value
   of zero.  If there are any errors then the function will return a -1.
   If there is an error the stat parameter will contain a value indicating
   what the error was.  If the field stat->error is positive it is a warning,
   if stat->error is negative it is an error.  These error numbers should
   be passed to a error generation facility that can convert the error number
   to a message and print the message on standard output.


   @bASSUMPTIONS AND DEPENDENCIES@s

   For HP-UX, probe always returns true because mapped file is
   not defined.

{**END_ES**

   @bALGORITHM@s

\image
   reset {stat}
   found = 0
   {subsp_index} = -1
   get (*subspace_dic._location) and (*subspace_dic._total)
   calculate address of space record #{sp_index}
   get sub_space_index and sub_space_quantity in that space record
   if (sub_space_index > *subspace_dic._total) then
      {stat} = error: space record corrupted
      return (BAD)
   if ((sub_space_index+sub_space_quantity) > *subspace_dic.total) then
      {stat} = error: space record / subspace_quantity wrong
      return (BAD)

   start_offset = subspace_index X size of (subsp_entry)
                  +(*subspace_dic.location) + {location}

   end_offset = start_offset + subspace_quantity X size of(subsp_entry)

   while ( start_offset <= end_offset )
      if ({ac_type} == subsp_entry->ac_bits) then
         get subspace_name string from subsp_entry->name
         if ({subsp_name} == subspace_name string) then
            {subsp_ptr} = address of this subsp_entry record
            {subsp_index} = this record #
            found = 1
            break
      else
         start_offset = start_offset + size of (subsp_entry)
   if (found) then
      successfully done, return(0)
   else
      {stat} = error: not found
      return(BAD)


{**END_IS**
----------------------------------------------------------------------*/
{  /* subsp_nmfdr code */

   int debug = 0;
   int i;                  /* loop index */
   int bad;                /* dummy, uses for function calls err. checking */
   unsigned int ac_temp;   /* temporary unsigned for 8 bits access con. */
   int found = 0;          /* flag for matched record */
   int num_char;           /* numbers of character in subsp_name string input */
   int str_loc;            /* contains space/subspace_string_location */
   int str_size;           /* contains space/subspace_string_size */
   int sp_loc, sp_total;   /* temporary variable to read file */
   int sub_loc, sub_total; /* subsp_dic_loc & sub_sp_dic_total in SOM header */
   int start_sub;          /* starting addr. of the subspace  */
   int quantity;           /* quantity of subspace record in the input space */
   char *the_name;         /* store subspace name string being fetched from SOM
*/
   long offset;            /* temporary offset, to access various SOM field */
   char *malloc();
   long start;
   FILE *lstfile;

struct space_dictionary_record *space;   /* tem. storage for space_record */
struct subspace_dictionary_record  *subsp;  /* temp. storage for subsp_record */



   lstfile = location->hi32.fptr;

   if(debug==1) {

      printf("*****START SUBSPACE NAME FINDER: \n");
      printf("Location = %d\n", lstfile);
      printf("Subspace name = %s\n", subsp_name);
      printf("ac_type = %d\n", ac_type);
      printf("space_index = %d\n", sp_index);
   } /* end debug */


   resetstat(stat);               /* reset status    */

   *subsp_index = 0;              /* reset index */

   start = ftell(lstfile);

   /* read space_dic_location for address of space_record # 0 to calculate addre
ss of space record # sp_index */

   bad = fseek(lstfile, start+SPACE_DIC_LOC, FILE_BEGIN); /*set f. ptr */
   if (bad) {  /* if problem with fseek the som file then exit */
      stat->error = FD_SUB_FSEEK_ERR0;   /* set status to fseek error */
      return(BAD);
   }
   bad = fread(&sp_loc, sizeof(sp_loc), ONE, lstfile);/* read loc */

   if (bad != ONE) {
      stat->error = FD_SUB_FREAD_ERR0;
      return(BAD);
   }

   /* if space_dic_loc <= 0 then space_rec not defined! */

   if (sp_loc <= 0) {
      stat->error = FD_SUB_0_SPREC0;
      return(BAD);
   }

   bad = fread(&sp_total, sizeof(sp_total), ONE, lstfile);/* read to
tal */

   if (bad != ONE) {
      stat->error = FD_SUB_FREAD_ERR1;
      return(BAD);
   }

   /* if space_dic_total <= 0 then space_rec not defined! */

   if (sp_total <= 0) {
      stat->error = FD_SUB_0_SPREC1;
      return(BAD);
   }
   /* seek and read subsp_dic_loc and subsp_dic_total */

   bad = fseek(lstfile, start+SUBSP_DIC_LOC, FILE_BEGIN); /*set f. ptr */
   if (bad) {  /* if problem with fseek the som file then exit */
      stat->error = FD_SUB_FSEEK_ERR1;   /* set status to fseek error */
      return(BAD);
   }
   bad = fread(&sub_loc, sizeof(sub_loc), ONE, lstfile);/* read loc
*/

   if (bad != ONE)  {
      stat->error = FD_SUB_FREAD_ERR2;
      return(BAD);
   }

   /* if subsp_dic_loc <= 0 then subspace_rec not defined! */

   if (sub_loc <= 0) {
      stat->error = FD_SUB_0_SPREC2;
      return(BAD);
   }

   bad = fread(&sub_total, sizeof(sub_total), ONE, lstfile);/* read
total */

   if (bad != ONE)  {
      stat->error = FD_SUB_FREAD_ERR3;
      return(BAD);
   }

   /* if subsp_dic_total <= 0 then subspace_rec not defined! */

   if (sub_total <= 0) {
      stat->error = FD_SUB_0_SPREC3;
      return(BAD);
   }

   /* read string_dic_loc and string_dic_size to find subsp name latter */

   bad = fseek(lstfile, start+SPACE_STR_LOC, FILE_BEGIN);
   if (bad) {  /* if problem with fseek the som file then exit */
      stat->error = FD_SUB_FSEEK_ERR2 ;   /* set status to fseek error */
      return (BAD);               /* return unsuccessfully */
   }

   bad = fread(&str_loc, sizeof(str_loc), ONE, lstfile);/* read loc
*/
   if (bad != ONE) {
      stat->error = FD_SUB_FREAD_ERR4;
      return(BAD);
   }

   /* if string_dic_loc <= 0 then subsp_string not defined! */

   if (str_loc <= 0) {
      stat->error = FD_SUB_0_SUBSTR0;
      return(BAD);
   }

   bad = fread(&str_size, sizeof(str_size), ONE, lstfile);/* read si
ze */
   if (bad != ONE) {
      stat->error = FD_SUB_FREAD_ERR5;
      return(BAD);
   }

   /* if string_dic_size <= 0 then subsp_string not defined! */

   if (str_size <= 0) {
      stat->error = FD_SUB_0_SUBSTR1;
      return(BAD);
   }

   /* allocate space and subspace internal record structures */

   if((space = (struct space_dictionary_record *)
                malloc(sizeof(struct space_dictionary_record))) == NULL) {
      stat->error = MALLOC_ERR;
      return(BAD);
   }

   if((subsp = (struct subspace_dictionary_record *)
       malloc(sizeof(struct subspace_dictionary_record))) == NULL) {
      stat->error = MALLOC_ERR;
      return(BAD);
   }

   /* calculate the offset of the input sp_index */

   offset = sp_loc + sizeof(*space) * sp_index;

   /* fetch that space_record into internal structure (space) */

   bad = fseek(lstfile, start+offset, FILE_BEGIN);
   if (bad) {  /* if problem with fseek the som file then exit */
      stat->error = FD_SUB_FSEEK_ERX1 ;   /* set status to fseek error */
      return (BAD);               /* return unsuccessfully */
   }

   bad = fread(space,sizeof(*space),ONE,lstfile);

   if(bad != ONE) {
      stat->error = FD_SUB_FREAD_ERR6;
      return(BAD);
   }

   /* calculate offset of sub_space record #0 encompassed by this space_record,
also extract the sub_space_quantity from this space_record */

   start_sub = sub_loc + sizeof(*subsp) * space->subspace_index;
   quantity = space->subspace_quantity;


   /* loop1 to read subsp_record into sub_space structure, one at a time until f
ound or run out of subspace_record ( bigger than quantity) */

   for (i=0; i <= quantity ; i++)  {
      /* seek to subsp_record  */
      bad = fseek(lstfile, start+start_sub, FILE_BEGIN);
      if (bad) {
         stat->error = FD_SUB_FSEEK_ERR3;
         break;
      }

      /* read subspace into subsp structure */

      bad = fread(subsp, sizeof(*subsp), ONE, lstfile);
      if(bad!=ONE)  {
         stat->error = FD_SUB_FREAD_ERR7;  /* error from fread subsp_record */
         break;
      }
      /* check if acb bits matched */
     
      ac_temp = subsp->access_control_bits;

      if(ac_temp == ac_type) {
      /* get name string only if they are  */
         if(subsp->name.n_strx >= str_size) {
            stat->error = FD_SUB_SUBREC_COR;  /* subsp record corrupted */
            break;
         }
         if(subsp->name.n_strx == 0) {
            stat->error = FD_SUB_SUBNAM_NIL;  /* null name pointer */
            break;
         }
         /* read name len and name string */
         /* calculate offset of name string in SOM, starting at the len  */

         offset = subsp->name.n_strx + str_loc - FOUR ;

         /* seek and get the string name that matches the requested ac_type as
above */

         bad = fseek(lstfile, start+offset, FILE_BEGIN);
         if(bad) {
            stat->error = FD_SUB_FSEEK_ERR4;
            break;
         }

         bad = fread(&num_char, sizeof(num_char), ONE, lstfile);
         if(bad!=ONE) {
            stat->error = FD_SUB_FREAD_ERR8;  /* error from fread string_name */
            break;
         }

         /*  allocate storage for the name string  */

         num_char = num_char + 1;            /* tag in \0 */
         the_name = malloc(num_char);    /* including \0 */

         bad = fread(the_name, sizeof(*the_name), num_char, lstfile);
         if(bad!=num_char) {
            stat->error = FD_SUB_FREAD_ERR9;  /* error from fread string_name  */
            break;
         }

         /* compare the_name and the requested subsp_name. If match then we foun
d it */
         if(strcmp(the_name, subsp_name) == 0) {    /* it is found!  */
            found = 1;

            /* set up output: file pointer = file pointer to this SOM, file offs
et = offset to this subsp_record (sp_loc).  */

            subsp_ptr->hi32.fptr = lstfile;
            subsp_ptr->offset = start + start_sub;
            break;   /* get out of loop1 */
         }
      }
      
      *subsp_index += 1;   /* increment to the next subsp_record */
      start_sub = start_sub + sizeof(*subsp);
      
   } /* end loop1  */

   if(debug==1) {

      printf("****END SUBSPACE NAME FINDER: \n");
      printf("Location = %d\n", lstfile);
      printf("Subspace index = %d\n", *subsp_index);
      printf("Subsp_ptr: fd = %d\n", subsp_ptr->hi32.fptr);
      printf("Subsp_ptr: offset = %o\n", subsp_ptr->offset);
      printf("Access bits = %x\n", subsp->access_control_bits);

   } /* end debug */

   if(found)  /* return good! */
      return(OK);
   else {   /* return bad! */
      *subsp_index = -1;
      return(BAD);
   }

} /* end subsp_nmfdr code */


/****************************************************************************

   function resetstat 
   This function is used to clear the global stat variable   

*****************************************************************************/

int resetstat (

          stat)                /* Input parameter */

   status *stat;

{
   stat->error = 0;
   stat->sysid = 0;

} /* end resetstat */
