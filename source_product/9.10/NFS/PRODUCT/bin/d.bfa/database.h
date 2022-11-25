/*----------------------------------------------------------------------*/
/*  Brief Description of the BFA DataBase format:			*/
/*									*/
/*  The bfa database files are comprised of just two different struc-	*/
/*  tures,  FILE_ENTRY and BRANCH_ENTRY and one integer.  The integer	*/
/*  is located at offset 0 in each database file and is stored in	*/
/*  machine readable format.  This integer is a count of the number	*/
/*  of FILE_ENTRY structures that are loacted in the database file.	*/
/*  The first FILE_ENTRY structure is located directly following the	*/
/*  integer count.  Directly following each of the different FILE_ENTRY */
/*  structures are all the BRANCH_ENTRY structures associated with the	*/
/*  previous file entry.  The number of BRANCH_ENTRY structures 	*/
/*  associated with each FILE_ENTRY is contains in the FILE entry	*/
/*  structure.								*/
/*----------------------------------------------------------------------*/

#define FILE_ENTRY struct file_entry
struct file_entry {
   char   file_name[MAXPATH];	/* Name of the source C source file	*/
   int    branches;		/* The number of branches associated	*/
				/* with this FILE_ENTRY structure.	*/
   long   next_file; 		/* The file offset for the next		*/
				/* File_ENTRY structure.  A value of	*/
				/* zero (0) indicates that this is the	*/
				/* last one.				*/
};


#define BRANCH_ENTRY struct branch_entry
struct branch_entry {
   int    branch_type;		/* The branch type.  The possible	*/
				/* values for this field are...		*/
				/* _ENTRY   - Entry point of procedure.	*/
				/* _FOR	    - For loop branch.		*/
				/* _WHILE   - While loop branch.	*/
				/* _DO      - Do loop branch.		*/
				/* _CASE    - Case statement branch.	*/
				/* _DEFAULT - Default statement branch. */
				/* _IF      - If statement branch.	*/
				/* _ELSE    - Else statement branch.	*/
				/* _ELSEIF  - Else if statement branch. */
				/* _SWITCH  - Switch statement branch.	*/
				/* _LABEL   - Label statement branch.	*/
				/* _FALLTHRU- Fall thru branch.		*/
				/* *** NOTE:  The manifest defines are	*/
				/*            located in the file	*/
				/*	      "defines.h"		*/
   int    line_num;		/* The line number in the C source	*/
				/* file where the branch is located.	*/
   long   count;		/* The number of times the branch was	*/
				/* hit.					*/
   char   proc_name[MAXID];	/* The name of the procedure where	*/
				/* the branch occurs.			*/
};

