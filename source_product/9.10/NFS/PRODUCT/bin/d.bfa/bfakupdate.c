#include <stdio.h>
#include <sys/param.h>
#include <fcntl.h>
#include <nlist.h>

#include "defines.h"
#include "global.h"
#include "database.h"

#define DEFAULT_DEV    "/dev/kmem"
#define DEFAULT_KERNEL "/hp-ux"


char dev_file[MAXPATH];
char k_file[MAXPATH];
char d_file[MAXPATH];

ENTRY entries[MAX_ENTRIES];
long  branches[MAX_BRANCHES];

#ifdef S300
struct nlist nl[6] = {
   { "_cur_entry" },
   { "_bfa_entries" },
   { "_cur_data" },
   { "_bfa_data" },
   { "_bfa_error" },
   { 0 }
};
#endif


#ifdef S800
struct nlist nl[6] = {
   { "cur_entry" },
   { "bfa_entries" },
   { "cur_data" },
   { "bfa_data" },
   { "bfa_error" },
   { 0 }
};
#endif


main(argc,argv)
int   argc;
char  *argv[];
{
   int   mem;
   int   num_entries;
   int	 bfa_error;

   int   i,j;
   int   error_val = 1;

   strcpy(dev_file,DEFAULT_DEV);
   strcpy(k_file,DEFAULT_KERNEL);
   d_file[0] = '\0';

   for(i=1;i<argc;i++) {
	if(argv[i][0] == '-') {
	   switch(argv[i][1]) {
	      case 'k': if(argv[i][2] == ' ') 
			   strcpy(k_file,&argv[i][3]);
			else
			   strcpy(k_file,&argv[i][2]);
			break;
              case 'd' : if(argv[i][2] == ' ')
	                    strcpy(dev_file,&argv[i][3]);
			 else
	                    strcpy(dev_file,&argv[i][2]);
			break;
	      default : fprintf(stderr,"\nERROR - ");
	                fprintf(stderr,"Unknown command line option\n");
			exit(BFA_ERROR);
	   } /* switch */
	} else {
	   if(strlen(d_file)) {
	      fprintf(stderr,"\nERROR - ");
	      fprintf(stderr,"Database file already specified.\n");
	      exit(BFA_ERROR);
	   }
	   if(argv[i][0] != '/') {
	      fprintf(stderr,"\nERROR - ");
	      fprintf(stderr,"Absolute path needed for database file.\n");
	      exit(BFA_ERROR);
	   }
	   strcpy(d_file,argv[i]);
	} /* else */
   } /* for */

   if((mem = open(dev_file,O_RDWR)) < 0) {
      fprintf(stderr,"ERROR - bfakupdate\n");
      fprintf(stderr,"Can't open %s for reading and writing\n",dev_file);
      exit(BFA_ERROR);
   } 

   if( nlist(k_file,nl) == -1) {
      fprintf(stderr,"ERROR - bfakupdate\n");
      fprintf(stderr,"nlist failure\n");
      exit(BFA_ERROR);
   }

   if(nl[0].n_type == 0 || nl[1].n_type == 0 ||
      nl[2].n_type == 0 || nl[3].n_type == 0 ||
      nl[4].n_type == 0 ) {
      fprintf(stderr,"ERROR - bfakupdate\n");
      fprintf(stderr,"All needed symbols not defined in the kernel\n");
      exit(BFA_ERROR);
   }

   lseek(mem,nl[4].n_value,0);
   read(mem,&bfa_error,sizeof(int));

   if(bfa_error) {
	fprintf(stderr,"Error occurred in the kernel\n");
	if(bfa_error == 1)
	        fprintf(stderr,"Not enough file entry structures\n");
	else
		fprintf(stderr,"Not enough data space\n");
	
	exit(BFA_ERROR);
   }

   lseek(mem,nl[0].n_value,0);
   read(mem,&num_entries,sizeof(int));
   lseek(mem,nl[1].n_value,0);
   read(mem,entries,(sizeof(ENTRY) * num_entries));

   for(i=0;i<num_entries;i++) {
      if(!strcmp(entries[i].database,d_file) || d_file[0] == '\0') {
         lseek(mem,(nl[3].n_value + (entries[i].offset * sizeof(long))),0);
	 read(mem,branches,(sizeof(long) * entries[i].length));
         
	 if(_UpdateDbase(entries[i].database,entries[i].file,branches)) {
		fprintf(stderr,"Database NOT updated\n");
		fprintf(stderr,"Could not find database to update\n");
		exit(BFA_ERROR);
	 }

	 for(j=0;j<entries[i].length;j++) 
	    branches[j] = 0l;

         lseek(mem,(nl[3].n_value + (entries[i].offset * sizeof(long))),0);
	 write(mem,branches,(sizeof(long) * entries[i].length));
	 error_val = 0;
      }
   } /* for */

   if(error_val) {
   	fprintf(stderr,"WARNING - No databases in the kernel matched the\n");
   	fprintf(stderr,"          database specified on the command line.\n");
   }
   exit(error_val);
} /* main */

