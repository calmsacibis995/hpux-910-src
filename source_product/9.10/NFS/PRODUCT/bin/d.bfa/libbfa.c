#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>

#if defined(UNIX)
#include <sys/lock.h>
#include <unistd.h>
#endif

#include "defines.h"
#include "global.h"
#include "database.h"

extern long lseek(); 


static FILE_ENTRY   file_info;
static BRANCH_ENTRY branch_info;


#define UPDATE_STRUCT struct update_struct
struct update_struct {
	char FAR	*database;
	char FAR	*file;
	long FAR	*branches;
};

static int		bfa_file_cnt = 0;
static UPDATE_STRUCT	bfa_file_info[MAXFILES];


#if defined(BFAWIN)
static unsigned short	mem_handles[MAXFILES];
static int		mem_count = 0;


void FAR _InsertBFAMemoryHandle(handle)
unsigned short	handle;
{
	mem_handles[mem_count++] = handle;

} /* _InsertBFAMemoryHandle */
#endif



void FAR _InsertDbase(database,file,branches)
char FAR	*database;
char FAR	*file;
long FAR	*branches;
{
	if(bfa_file_cnt == MAXFILES) {
	   fprintf(stderr,"\nBFA Runtime Fatal Error - CANNOT PROCEED\n");
	   fprintf(stderr,"Maximum number of files (%d) exceeded\n",MAXFILES);
	   fprintf(stderr,"BFA Library aborting from _InsertDbase\n");
	   exit(BFA_ERROR);
	} /* if */

	bfa_file_info[bfa_file_cnt].database     = database;
	bfa_file_info[bfa_file_cnt].file         = file;
	bfa_file_info[bfa_file_cnt].branches     = branches;

	bfa_file_cnt++;

} /* _InsertDbase */




void FAR _UpdateBFA() {
	int	i;
#if defined(BFAWIN)
	int		far pascal GlobalUnlock();
	unsigned short	far pascal GlobalFree();
#endif

	int FAR	_UpdateDbase();

	for(i=0;i<bfa_file_cnt;i++)
	   _UpdateDbase((char FAR *)bfa_file_info[i].database,
	                (char FAR *)bfa_file_info[i].file,
	                (long FAR *)bfa_file_info[i].branches);

#if defined(BFAWIN)
	for(i=0;i<mem_count;i++) {
	   GlobalUnlock(mem_handles[i]);
	   GlobalFree(mem_handles[i]);
	}
#endif

} /* _UpdateBFA */



long FAR * FAR _CallocBFA(number)
int	number;
{
	long FAR	*ptr;
	extern char 	*calloc();

	if((ptr = (long FAR *)calloc(number,sizeof(long))) == NULL) {
	   fprintf(stderr,"\nBFA Runtime Fatal Error - CANNOT PROCEED\n");
	   fprintf(stderr,"Can't allocate dynamic memory.\n");
	   fprintf(stderr,"BFA Library aborting from _CallocBFA\n");
	   exit(BFA_ERROR);
	} /* if */

	return((long FAR *)ptr);

} /* _CallocBFA */



#if defined(MSDOS)
int FAR _UpdateDbase(dbase_file_temp,update_file_temp,array)
char FAR	*dbase_file_temp;
char FAR	*update_file_temp;
long FAR	*array;
#else
int _UpdateDbase(dbase_file,update_file,array)
char	*dbase_file;
char	*update_file;
long	*array;
#endif
{
	int	dbase;
	int	count;
	int	i;
	int	j;

	long	cur_pos;
	long	num_bytes;


#if defined(MSDOS) 
	static char dbase_file[MAXPATH];
	static char update_file[MAXPATH];

	for(i=0;dbase_file_temp[i] != '\0';i++)
	   dbase_file[i] = dbase_file_temp[i];
	dbase_file[i] = '\0';

	for(i=0;update_file_temp[i] != '\0';i++)
	   update_file[i] = update_file_temp[i];
	update_file[i] = '\0';
#endif


#if defined(MSDOS)
	if((dbase = open(dbase_file,O_RDWR|O_BINARY)) < 0) {
#else
   	if((dbase = open(dbase_file,O_RDWR)) < 0) {
#endif
	   fprintf(stderr,"\nBFA Update Fatal Error\n");
	   fprintf(stderr,"Error - can't open %s to update\n",dbase_file);
	   fprintf(stderr,"BFA Library _UpdateDbase\n");
	   return(1);
	} /* if */


	num_bytes = lseek(dbase,0L,2);
	lseek(dbase,0L,0);

#if defined(UNIX)
	for(j=0;lockf(dbase,F_LOCK,num_bytes) == -1 && j < 6;j++) {
	   sleep(5);
	} /* for */ 
#endif

	read(dbase,&count,sizeof(int));

	do { 
	   read(dbase,&file_info,sizeof(FILE_ENTRY));
	   if(!strcmp(file_info.file_name,update_file))
	      break;

	   lseek(dbase,file_info.next_file,0);
	} while(file_info.next_file);

	if(strcmp(file_info.file_name,update_file)) {
	   fprintf(stderr,"\nBFA Update Fatal Error\n");
	   fprintf(stderr,"No database entry for %s\n",update_file);
	   fprintf(stderr,"BFA Library _UpdateDbase\n");
	   return(1);
	} /* if */


	for(i=0;i<file_info.branches;i++) {
	   cur_pos = lseek(dbase,0l,1);
	   read(dbase,&branch_info,sizeof(BRANCH_ENTRY));


	   if(array[i]) {
	      branch_info.count += array[i];
	      lseek(dbase,cur_pos,0);
	      write(dbase,&branch_info,sizeof(BRANCH_ENTRY));
	      array[i] = 0l;
	   } /* if */
	} /* for */

	lseek(dbase,0L,0);

#if defined(UNIX)
	lockf(dbase,F_ULOCK,num_bytes); 
#endif
	close(dbase);  

	return(0);
} /* _UpdateDbase */



void FAR ExitBFA(x)
int	x;
{
	int FAR	_UpdateDbase();

	_UpdateBFA();
	exit(x);
} /* ExitBFA */



void FAR _ExitBFA(x)
int	x;
{
	int FAR	_UpdateDbase();

	_UpdateBFA();
	_exit(x);
} /* _ExitBFA */


#if defined(FORK)
int FAR _ForkBFA()
{
	int FAR	_UpdateDbase();

	_UpdateBFA();
	return(fork());
} /* _ForkBFA */


int FAR _VForkBFA()
{
	int FAR	_UpdateDbase();

	_UpdateBFA();
	return(vfork());
} /* _ForkBFA */
#endif


#if defined(EXEC)

char FAR * FAR _StringUpdateBFA(string)
char FAR *string;
{
	int FAR	_UpdateDbase();

	_UpdateBFA();

	return((char FAR *)string);
} /* _StringUpdateBFA */
#endif



#if defined(TSR)


static char	file_name[MAXPATH];
static char	dbase_name[MAXPATH];

extern ENTRY far * far _BfaFileRequest();
extern long  far * far _BfaDataRequest();
extern int   far       _BfaCountRequest();


int _BfaStore(vector,quiet)
int	vector;
int	quiet;
{
	int	ret_val = 0;
	int	i;
	int	j;


	long  far	*bfadata;
	ENTRY far	*bfafiles;
	int		num_entries;


	if((bfadata = _BfaDataRequest(vector)) == (long far *)NULL) {
	   fprintf(stderr,"ERROR: _BfaStore can't attach to the data\n");
	   return(1);
	}
	
	if((bfafiles = _BfaFileRequest(vector)) == (ENTRY far *)NULL) {
	   fprintf(stderr,"ERROR: _BfaStore can't attach to the files\n");
	   return(1);
	}

	if((num_entries = _BfaCountRequest(vector)) == -1) {
	   fprintf(stderr,"ERROR: _BfaStore can't attach to the count\n");
	   return(1);
	}

	for(i=0;i<num_entries;i++) {
	   if(! quiet) {
		for(j=0;bfafiles[i].database[j] != '\0';j++)
			dbase_name[j] = bfafiles[i].database[j];
		dbase_name[j] = '\0';

		for(j=0;bfafiles[i].file[j] != '\0';j++)
			file_name[j] = bfafiles[i].file[j];
		file_name[j] = '\0';

		fprintf(stderr,"Database %s File %s being updated\n",
			dbase_name,file_name);
	   } /* if */
		ret_val+=_UpdateDbase((char far *)bfafiles[i].database,
			              (char far *)bfafiles[i].file,
			              (long far *)&bfadata[bfafiles[i].offset]);
	} /* for */

	return(ret_val);
} /* _BfaStore */



#endif
