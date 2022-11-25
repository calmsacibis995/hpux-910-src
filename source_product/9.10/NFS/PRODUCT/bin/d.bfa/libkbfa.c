#if defined(BFA_KERNEL)

#include <stdio.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/lock.h>
#include <unistd.h>

#include "defines.h"
#include "global.h"
#include "database.h"


long  bfa_data[MAX_BRANCHES];
ENTRY bfa_entries[MAX_ENTRIES];

int   cur_entry = 0;
int   cur_data  = 0;
int   bfa_error = 0;


long *_BFA_KernelData(database,file,size)
char	*database;
char	*file;
int	size;
{
	long *data;

	if(cur_entry == MAX_ENTRIES) {
	   printf("BFA Runtime Error \n");
	   printf("File: libkbfa.c   Procedure: _BFA_KernelData\n");
	   printf("Entry table overflow\n");
	   printf("Entry # %d   Size %d  Database %s  File %s\n",cur_entry,
	   size,database,file);
	   cur_entry = 0;
	   bfa_error = 1;
	   return(bfa_data);
	}

	if((cur_data + size) >= MAX_BRANCHES) {
	   printf("BFA Runtime Error \n");
	   printf("File: libkbfa.c   Procedure: _BFA_KernelData\n");
	   printf("Data space overflow\n");
	   printf("Entry # %d   Size %d  Database %s  File %s\n",cur_entry,
	   size,database,file);
	   cur_data = 0;
	   bfa_error = 2;
	   return(bfa_data);
	}

	strcpy(bfa_entries[cur_entry].database,database);
	strcpy(bfa_entries[cur_entry].file,file);
	bfa_entries[cur_entry].length = size;
	bfa_entries[cur_entry].offset = cur_data;
	cur_entry++;
	
	data      = &bfa_data[cur_data];
	cur_data += size;

	return(data);
} /* _BFA_KernelData */

#endif

#if defined(TSR)
#include <stdio.h>
#include <dos.h>

#include "defines.h"
#include "global.h"
#include "database.h"

int far int86xBFA();

char far * far FixAddress(addr)
char far	*addr;
{
	return(addr);
} /* FixAddress */


int far CheckIfBfaInstalled(vector)
int	vector;
{
	char	 string[20];
	char far *addr;
	int  far *addr1;

	int	i;

	int	offset;
	int	segment;

	offset = vector * 4;
	segment= 0;

	addr1 = (int far *)FixAddress(offset,segment);
	addr  = FixAddress(addr1[0],addr1[1]);

	for(i=0;i<12;i++)
		string[i] = addr[i+3];
	string[i] = '\0';

	return(! strcmp("BFAInterrupt",string));

} /* CheckIfBfaInstalled */


int far CallBfaInterrupt(kind,ptr,vector)
int		kind;
char far	*ptr;
int		vector;
{
	union  REGS	in;
	struct SREGS	sin;


	if(! CheckIfBfaInstalled(vector)) 
		return(-1);

	in.x.bx = 0;
	in.x.ax = kind;
	in.x.dx = FP_OFF(ptr);
	sin.ds  = FP_SEG(ptr);

	while(! in.x.bx)
		int86xBFA((int)vector,
			(char far *)&in,
			(char far *)&in,
			(char far *)&sin);

	return(in.x.ax);
} /* CallBfaInterrupt */


long far * far _BFAMemoryRequest(database,file,branches,vector)
char far	*database;
char far	*file;
int		branches;
int		vector;
{
	MEM_REQUEST	info;
	int		real_vector;


	real_vector = vector == 0 ? INTERRUPT_VECTOR : vector;

	info.database = database;
	info.file     = file;
	info.branches = branches;
	info.data     = (long far *)NULL;

	if(CallBfaInterrupt(MEM_MODE,(char far *)&info,real_vector))
		return((long far *)NULL);

        return(info.data); 
} /* _BFAMemoryRequest */


long far * far _BfaDataRequest(vector)
int	vector;
{
	DATA_REQUEST	info;
	int		real_vector;


	real_vector = vector == 0 ? INTERRUPT_VECTOR : vector;

	info.data = (char far *)NULL;

	if(CallBfaInterrupt(DATA_MODE,(char far *)&info,real_vector))
		return((long far *)NULL);

        return((long far *)info.data); 
} /* _BfaDataRequest */


ENTRY far * far _BfaFileRequest(vector)
int	vector;
{
	DATA_REQUEST	info;
	int		real_vector;


	real_vector = vector == 0 ? INTERRUPT_VECTOR : vector;

	info.data = (char far *)NULL;

	if(CallBfaInterrupt(FILE_MODE,(char far *)&info,real_vector))
		return((ENTRY far *)NULL);

        return((ENTRY far *)info.data); 
} /* _BfaFileRequest */


int far _BfaCountRequest(vector)
int	vector;
{
	COUNT_REQUEST	info;
	int		real_vector;


	real_vector = vector == 0 ? INTERRUPT_VECTOR : vector;

	info.data = 0;

	if(CallBfaInterrupt(COUNT_MODE,(char far *)&info,real_vector))
		return(0);

        return(info.data); 
} /* _BfaCountRequest */



#endif

