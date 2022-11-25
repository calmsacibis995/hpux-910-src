
#include <stdio.h>
#include <fcntl.h>

#include "defines.h"
#include "global.h"
#include "database.h"

#if defined(MSDOS)
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#endif

extern long lseek();

static FILE_ENTRY   file_info;

static int    old_dbase;
static int    new_dbase;

static int    old_count = 0;
static int    new_count = 0;

static long   last_entry = 0l;



OpenDbase() {

#if defined(MSDOS) 
	if((old_dbase = open(dbase_file,O_RDONLY|O_BINARY)) >= 0) {
	   read(old_dbase,&old_count,sizeof(int));
	   new_count = old_count;
	} /* if */

	if((new_dbase=open("bfanew",
		O_RDWR|O_CREAT|O_TRUNC|O_BINARY,S_IWRITE))<0){
	   fprintf(stderr,"Can't open BFA temporary file\n");
	   Exit(BFA_ERROR);       
	} /* if */
#else
	if((old_dbase = open(dbase_file,O_RDONLY)) >= 0) {
	   read(old_dbase,&old_count,sizeof(int));
           new_count = old_count;
        } /* if */

	if((new_dbase = open("bfanew",O_RDWR|O_CREAT|O_TRUNC,0666)) < 0) {
	   fprintf(stderr,"Can't open BFA temporary file\n");
   	   Exit(BFA_ERROR);       
	}
#endif

	write(new_dbase,&new_count,sizeof(int));
 
}  /* OpenDbase */


#define BRANCH_BUFFER	20
CloseDbase() {
	static FILE_ENTRY     old_file;
	static FILE_ENTRY     tmp_file;
	static BRANCH_ENTRY   branch_info[BRANCH_BUFFER];

	int   i;
	int   j;

	if(old_count)
	   lseek(old_dbase,(long)sizeof(int),0);

	for(i=0;i<old_count;i++) {
	   read(old_dbase,&old_file,sizeof(FILE_ENTRY));

	   lseek(new_dbase,(long)sizeof(int),0);
	   do {
	      read(new_dbase,&tmp_file,sizeof(FILE_ENTRY));
	      if(!strcmp(old_file.file_name,tmp_file.file_name))
	         break;
	      else
	         lseek(new_dbase,tmp_file.next_file,0);
	   } while(tmp_file.next_file);

	   if(strcmp(old_file.file_name,tmp_file.file_name)) {
	      lseek(new_dbase,0l,2);
   	      file_info.next_file = lseek(new_dbase,0l,1);
	      lseek(new_dbase,last_entry,0);
	      write(new_dbase,&file_info,sizeof(FILE_ENTRY));

	      file_info.branches  = old_file.branches;
	      file_info.next_file = 0l;
	      strcpy(file_info.file_name,old_file.file_name);

	      lseek(new_dbase,0l,2);
	      last_entry = lseek(new_dbase,0l,1);

	      write(new_dbase,&file_info,sizeof(FILE_ENTRY));

	      for(j=0;j<file_info.branches;j+=BRANCH_BUFFER) {
		 read(old_dbase,branch_info, sizeof(BRANCH_ENTRY) * 
		      (((file_info.branches - j) <  BRANCH_BUFFER)?
			(file_info.branches-j):(BRANCH_BUFFER)));

	         write(new_dbase,branch_info,sizeof(BRANCH_ENTRY)  *
		      (((file_info.branches - j) <  BRANCH_BUFFER)?
			(file_info.branches-j):(BRANCH_BUFFER)));
	      }  /* for */
	   } /* if */
	   lseek(old_dbase,old_file.next_file,0);
	} /* for */

	lseek(new_dbase,last_entry,0);
	write(new_dbase,&file_info,sizeof(FILE_ENTRY));

	lseek(new_dbase,0l,0);
	write(new_dbase,&new_count,sizeof(int));

	close(old_dbase);
	close(new_dbase);

	unlink(dbase_file);

#if defined(MSDOS)
	link("bfanew",dbase_file); 
#else
	if(link("bfanew",dbase_file) < 0) {
	   int  in;
	   int  out;
	   char buffer[BUFSIZ];

	   if((out = open(dbase_file,O_RDWR|O_CREAT,0666)) < 0) {
	      fprintf(stderr,"BFA Error: can't create %s\n",dbase_file);
	      unlink("bfanew");
	   }
	   in  = open("bfanew",O_RDONLY);

	   while(write(out,buffer,read(in,buffer,BUFSIZ)) > 0);

	   close(in);
	   close(out);
	} /* if */
#endif

	unlink("bfanew");

} /* CloseDbase */


NewFile() {

	if(last_entry) {
	   lseek(new_dbase,0l,2);
	   file_info.next_file = lseek(new_dbase,0l,1);
	   lseek(new_dbase,last_entry,0);
	   write(new_dbase,&file_info,sizeof(FILE_ENTRY));
	} /* if */

	if(old_count) {
	   lseek(old_dbase,(long)sizeof(int),0);
	   do {
	      read(old_dbase,&file_info,sizeof(FILE_ENTRY));
	      lseek(old_dbase,file_info.next_file,0);
	   } while(strcmp(file_info.file_name,input_files[cur_input-1])&&
			file_info.next_file);

	   if(strcmp(file_info.file_name,input_files[cur_input-1])) {
	      strcpy(file_info.file_name,input_files[cur_input-1]);
	      new_count++;
	   } /* if */
	} else {
	   strcpy(file_info.file_name,input_files[cur_input-1]);
	   new_count++;
	} /* else */
 
	file_info.branches  = 0;
	file_info.next_file = 0l;


	lseek(new_dbase,0l,2);
	last_entry = lseek(new_dbase,0l,1);
	write(new_dbase,&file_info,sizeof(FILE_ENTRY));
 
}  /* NewFile */


int CurRecord() {
	return(file_info.branches-1);
} /* CurArray */


int NextRecord(kind)
int     kind;
{
	BRANCH_ENTRY   branch_info;

	branch_info.branch_type = kind;
	branch_info.line_num    = line_number;
	branch_info.count       = 0l;

	if(strlen(cur_func) >= MAXID) {
	   fprintf(stderr,"Fucntion name %s to long ->\n",cur_func);
	   cur_func[MAXID-1] = '\0';
	   fprintf(stderr,"\tBeing truncated to %s in the database\n",cur_func);
	} /* if */

	strcpy(branch_info.proc_name,cur_func);

	lseek(new_dbase,0l,2);
	write(new_dbase,&branch_info,sizeof(BRANCH_ENTRY));

	file_info.branches++;
	return(file_info.branches-1);
   
} /* NextRecord */

