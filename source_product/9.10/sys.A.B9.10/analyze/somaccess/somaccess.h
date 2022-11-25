/* UNISRC_ID: @(#)somaccess.h	4.00	85/10/01  */

/* The status of a function call is returned in a 32 bit variable */

typedef struct
           {short int error:16;
            short int sysid:16;
           }
                     status, *stat_ptr;


/* A 64 bit buffer will contain either a SPECTRUM long pointer */
/* or, for HP-UX, a 32 bit FILE pointer and a 32 bit offset */

typedef struct
           {union
              {unsigned int space_id;
               FILE         *fptr;
              } hi32;
            unsigned int offset;
           }
                     long_ptr;

