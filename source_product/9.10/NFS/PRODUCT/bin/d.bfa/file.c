#include <stdio.h>
#include <sys/types.h>

#include "defines.h"
#include "global.h"
#include "database.h"

#if defined(UNIX)
#include <sys/dir.h>

#define FAR_S		""
#define CDECL_S		""
#define CHAR_FAR	""
#define LONG_FAR	""
#endif

#if defined(MSDOS)
	static char   update_file[BUFSIZ];

FILE	*popen();

#define FAR_S		"far"
#define CDECL_S		"cdecl"
#define CHAR_FAR	"(char far *)"
#define LONG_FAR	"(long far *)"
#endif




CloseFile() {
	FILE   *c_file;
   
	int	i;
	int	c;
	int	num_rec;
	 
	char	buffer[BUFSIZ];

	fseek(outfile,0L,0);
	if((c_file = fopen(cur_output,"w")) == 0) {
	   fprintf(stderr,"Error opening output file %s\n",cur_output);
	   fprintf(stderr," << File file.c, Procedure CloseFile >>\n");
	   Exit(1);
	} /* if */


#if defined(MSDOS)
	for(i=0;input_files[cur_input-1][i] != '\0';i++)
	   if(input_files[cur_input-1][i] == '\\')
	      input_files[cur_input-1][i] = '/';

	strupr(dbase_file);
	for(i=0;dbase_file[i] != '\0';i++)
	   if(dbase_file[i] == '\\')
	      dbase_file[i] = '/';
#endif

#if defined(BFA_KERNEL)
	if(k_flag)
	   fprintf(c_file,"extern long %s * %s %s _BFA_KernelData();\n",
		FAR_S,FAR_S,CDECL_S);
	else
#endif

#if defined(TSR)
	if(r_flag)
	   fprintf(c_file,"extern long %s * %s %s _BFAMemoryRequest();\n",
		FAR_S,FAR_S,CDECL_S);
	else
#endif
	{
	   fprintf(c_file,"extern void %s %s _InsertDbase();\n",
		FAR_S,CDECL_S);
	   fprintf(c_file,"extern void %s %s _UpdateBFA();\n",
		FAR_S,CDECL_S);
	   fprintf(c_file,"extern long %s * %s %s _CallocBFA();\n",
		FAR_S,FAR_S,CDECL_S);
	   fprintf(c_file,"extern int %s %s _UpdateDbase();\n",
		FAR_S,CDECL_S);
	   fprintf(c_file,"extern void %s %s _ExitBFA();\n",
		FAR_S,CDECL_S);
	   fprintf(c_file,"extern void %s %s ExitBFA();\n",
		FAR_S,CDECL_S);

#if defined(FORK)
	   fprintf(c_file,"extern int %s %s _ForkBFA();\n",
		FAR_S,CDECL_S);
	   fprintf(c_file,"extern int %s %s _VForkBFA();\n",
		FAR_S,CDECL_S);
#endif

#if defined(EXEC)
	   fprintf(c_file,"extern char %s * %s %s _StringUpdateBFA();\n",
		FAR_S,FAR_S,CDECL_S);
#endif
	}

	num_rec = CurRecord()+1;

	if(m_flag) {
	   fprintf(c_file,"\nstatic long %s *_bfa_array;\n",FAR_S);
	   fprintf(c_file,"static int  _bfa_init = 0;\n\n");

	   fprintf(c_file,"static _InitializeBFA() {\n");
	   fprintf(c_file,"   _bfa_array = _CallocBFA(%d);\n",num_rec);

	   fprintf(c_file,"   _InsertDbase(%s\"%s\",%s\"%s\",%s_bfa_array);\n",
		CHAR_FAR,dbase_file,CHAR_FAR,input_files[cur_input-1],LONG_FAR);

	   fprintf(c_file,"   _bfa_init=1;\n}\n\n");


#if defined(BFA_KERNEL)
	} else if(k_flag) {
	   fprintf(c_file,"\nstatic long %s *_bfa_array;\n",FAR_S);
	   fprintf(c_file,"static int  _bfa_init = 0;\n\n");

	   fprintf(c_file,"static _UpdateBFA() {\n   return;\n}\n\n");

	   fprintf(c_file,"static _InitializeBFA() {\n");
	   fprintf(c_file," _bfa_array=_BFA_KernelData(");
	   fprintf(c_file,"%s\"%s\",%s\"%s\",%d);\n",
	  	CHAR_FAR,dbase_file,CHAR_FAR,input_files[cur_input-1],num_rec);
	   fprintf(c_file,"   _bfa_init=1;\n}\n\n");
#endif

#if defined(TSR)
	} else if(r_flag) {
	   fprintf(c_file,"\nstatic long %s *_bfa_array;\n",FAR_S);
	   fprintf(c_file,"static int  _bfa_init = 0;\n\n");

	   fprintf(c_file,"static _UpdateBFA() {\n   return;\n}\n\n");

	   fprintf(c_file,"static _InitializeBFA() {\n");
	   fprintf(c_file," _bfa_array=_BFAMemoryRequest(");
	   fprintf(c_file,"%s\"%s\",%s\"%s\",%d,%d);\n",CHAR_FAR,
		dbase_file,CHAR_FAR,input_files[cur_input-1],num_rec,vector);
	   fprintf(c_file,"   _bfa_init=1;\n}\n\n");
#endif

#if defined(BFAWIN)
	} else if(w_flag) {
	   fprintf(c_file,"\nstatic long %s *_bfa_array;\n",FAR_S);
	   fprintf(c_file,"static char %s *_bfa_string;\n",FAR_S);
	   fprintf(c_file,"static unsigned short _bfa_handle;\n");
	   fprintf(c_file,"static int  _bfa_init = 0;\n\n");

	   fprintf(c_file,"\nstatic int _BFA_strlen(string)\n");
	   fprintf(c_file,"char	*string;\n{\n");
	   fprintf(c_file,"int i;\n");
	   fprintf(c_file,"for(i=0;string[i] != '\\0';i++);\n");
	   fprintf(c_file,"return(i);\n}\n\n");

	   fprintf(c_file,"\nstatic _InitializeBFA() {\n");
	   fprintf(c_file,"int   i,j,k,l;\n");
	   fprintf(c_file,"char  *dbase=\"%s\";\n",dbase_file);
	   fprintf(c_file,"char  *cfile=\"%s\";\n",input_files[cur_input-1]);
	   fprintf(c_file,"long  size;\n\n");

	   fprintf(c_file,"unsigned short far pascal GlobalAlloc();\n");
	   fprintf(c_file,"char far *     far pascal GlobalLock();\n");
	   fprintf(c_file,"void       far cdecl _InsertBFAMemoryHandle();\n\n");

	   fprintf(c_file,"size =(long)sizeof(long)*%d;\n",num_rec+1);
	   fprintf(c_file,"size+=(long)_BFA_strlen(dbase)+1;\n");
	   fprintf(c_file,"size+=(long)_BFA_strlen(cfile)+1;\n\n");

	   fprintf(c_file,"_bfa_handle = GlobalAlloc(0,size);\n");
	   fprintf(c_file,"_InsertBFAMemoryHandle(_bfa_handle);\n");
	   fprintf(c_file,"_bfa_array  = (long far *)GlobalLock(_bfa_handle);\n\n");

	   fprintf(c_file,"_bfa_string = (char far *)_bfa_array;\n");
	   fprintf(c_file,"k = sizeof(long) * %d;\n\n",num_rec + 1);

	   fprintf(c_file,"for(j=k,i=0;dbase[i] != '\\0';i++,j++)\n");
	   fprintf(c_file,"   _bfa_string[j] = dbase[i];\n");
	   fprintf(c_file,"_bfa_string[j++] = '\\0';\n\n");

	   fprintf(c_file,"for(l=j,i=0;cfile[i] != '\\0';i++,j++)\n");
	   fprintf(c_file,"   _bfa_string[j] = cfile[i];\n");
	   fprintf(c_file,"_bfa_string[j++] = '\\0';\n\n");

	   fprintf(c_file,"for(i=0;i<%d;i++)\n",num_rec);
	   fprintf(c_file,"   _bfa_array[i] = 0l;\n\n");

	   fprintf(c_file,"_InsertDbase(%s%s,%s%s,%s_bfa_array);\n\n",
		CHAR_FAR,"&_bfa_string[k]",CHAR_FAR,"&_bfa_string[l]",LONG_FAR);

	   fprintf(c_file,"_bfa_init=1;\n}\n\n");


#endif

	} else {
	   fprintf(c_file,"static long _bfa_array[%d];\n",num_rec+1);
	   fprintf(c_file,"static int  _bfa_init = 0;\n\n");

	   fprintf(c_file,"static _InitializeBFA() {\n");
	   fprintf(c_file,"   int   i;\n");
	   fprintf(c_file,"   for(i=0;i<%d;i++)\n",num_rec);
	   fprintf(c_file,"      _bfa_array[i] = 0l;\n");
	   fprintf(c_file,"   _InsertDbase(%s\"%s\",%s\"%s\",%s_bfa_array);\n",
		CHAR_FAR,dbase_file,CHAR_FAR,input_files[cur_input-1],LONG_FAR);

	   fprintf(c_file,"   _bfa_init=1;\n}\n\n");
	}  /* else */

	while(fwrite(buffer,1,fread(buffer,1,BUFSIZ,outfile),c_file) > 0);

	fclose(c_file);
	fclose(outfile);
	pclose(yyin);
	unlink(BFATEMP);

} /* CloseFile */



int NextFile() {
	char   command[BUFSIZ];

	if(cur_input < num_input) {
	   sprintf(command,"%s %s %s %s",cpp,input_files[cur_input++],
					 c_args,cpp_end);

	   strcpy(cur_output,input_files[cur_input-1]);
	   CreateFile(cur_output);

	   if((outfile = fopen(BFATEMP,"w+")) == NULL) {
	      fprintf(stderr,"Error opening temp file %s\n",BFATEMP);
	      fprintf(stderr," << File file.c, Procedure NextFile >>\n");
	      Exit(BFA_ERROR);
	   } /* if */


	   if((yyin = popen(command,"r")) == NULL) {
	      fprintf(stderr,"Error C Preprocessing %s (%s)\n",
			input_files[cur_input-1],command);
	      Exit(BFA_ERROR);
	   } /* if */

	   line_number = 1;

	   if(!q_flag)
	      fprintf(stderr,"%s  ",input_files[cur_input-1]);

	   return(1);
	} /* if */

	return(0);
} /* NextFile */



CreateFile(new_file)
char   new_file[];
{
	char   file_name[BUFSIZ];

	int   len;
	int   i;

	i = strlen(new_file);

#if defined(MSDOS)
	while(new_file[i] != '\\' && new_file[i] != '/' && i >= 0)
	   i--;
#else
	while(new_file[i] != '/' && i >= 0)
	   i--;
#endif

	strcpy(file_name,&new_file[++i]);
	file_name[strlen(file_name)-2] = '\0';
	 
	strcpy(new_file,"_");
	if(strlen(file_name) == FILELEN) {
	   file_name[strlen(file_name)-1] = '\0';
	} /* if */

	strcat(new_file,file_name);
	strcat(new_file,".c");
 
} /* CreateFile */

