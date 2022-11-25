
#include <stdio.h>
#include <fcntl.h>

#include "defines.h"
#include "global.h"
#include "database.h"

#define SIGN_OFF struct sign_off
struct sign_off {
	char		*database;
	char		*module;
	int		branch;
	SIGN_OFF	*next;
};

SIGN_OFF	*sign_list;	/* Pointer to list of sign off records	*/


int    s_flag = 1;		/* Summary report flag			*/
int    f_flag = 0;		/* File report flag			*/
int    p_flag = 0;		/* Procedure report flag		*/
int    b_flag = 0;		/* Branch report flag			*/
int    i_flag = 0;              /* Report on index count also.          */
int    e_flag = 0;		/* Produce a report for everything	*/
int    m_flag = 0;              /* Merge report flag			*/
int    z_flag = 0;		/* Zero the database after reports	*/
int    r_flag = 0;		/* Report file flag (truncate)		*/
int    a_flag = 0;              /* Report file flag (append)		*/
int    F_flag = 1;              /* Include fall thrus in report		*/
int    l_flag = 0;              /* Check for sign off branches		*/

char   r_name[MAXPATH];		/* Report file name (truncate)		*/
FILE   *rep_file;		/* Report file stream pointer		*/

char   *f_name[MAXFILES];	/* Function name to report on		*/
char   *p_name[MAXFILES];	/* Procedure names to report on		*/
int    p_count = 0;
int    f_count = 0;

char   *dbases[MAXFILES];	/* Name of BFA database			*/
int    dbase_count = 0;		/* Number of BFA database files.        */
int    dbase;			/* BFA database file descriptors	*/

long   cur_time;		/* Integer time value			*/
char   *time_string;		/* ASCII Time representation		*/

extern long   time();
extern char   *ctime();

FILE_ENTRY     file_info;
BRANCH_ENTRY   branch_info;

extern char *malloc();
extern long lseek();


char *Malloc(size)
int	size;
{
	char	*tmp;

	if((tmp = malloc(size)) == NULL) {
	   fprintf(stderr,"Can't malloc memory, Aborting\n");
	   exit(1);
	}
	return(tmp);
} /* Malloc */



main(argc,argv) 
int   argc;
char  *argv[];
{
   int     i;

   sign_list = NULL;

   for(i=1;i<argc;i++) {
      if(argv[i][0] == '-') {
         switch(argv[i][1]) {
            case 's' : s_flag = 0;
                       break;
            case 'f' : f_flag = 1;
                       break;
            case 'p' : p_flag = 1;
                       break;
            case 'b' : b_flag = 1;
                       break;
            case 'i' : i_flag = 1;
		       b_flag = 1;
                       break;
            case 'e' : e_flag = 1;
                       break;
            case 'm' : m_flag = 1;
                       break;
            case 'a' : if(r_flag) {
                          fprintf(stderr,"\nReport file already specified, ");
                          fprintf(stderr,"Ignoring subsequent request.\n");
                          break;
                       }
                       a_flag = 1;
                       strcpy(r_name,&argv[i][2]);
                       break;
            case 'r' : if(a_flag) {
                          fprintf(stderr,"\nReport file already specified, ");
                          fprintf(stderr,"Ignoring subsequent request.\n");
                          break;
                       }
                       r_flag = 1;
                       strcpy(r_name,&argv[i][2]);
                       break;
/************************************************************************/
/* Not yet implemented							*/
/*
            case 'P' : p_name[p_count] = Malloc(strlen(&argv[i][2])+1);
                       strcpy(p_name[p_count++],&argv[i][2]);
                       break; 
            case 'F' : f_name[f_count] = Malloc(strlen(&argv[i][2])+1);
                       strcpy(f_name[f_count++],&argv[i][2]);
                       break; 
									*/
/************************************************************************/
	    case 'F' : F_flag = 0;
		       break;
            case 'z' : z_flag = 1;
                       break;
	    case 'l' : l_flag = 1;
		       ProcessSignOffList(&argv[i][2]);
		       break;
            default  : fprintf(stderr,"Error - unknown option %s\n",argv[i]);
                       exit(BFA_ERROR);
         } /* switch */
      } else {
         dbases[dbase_count] = Malloc(strlen(argv[i])+1);
         strcpy(dbases[dbase_count++],argv[i]);
      }
   } /* for */

   if(!dbase_count) {
      dbases[dbase_count] = Malloc(strlen(BFADBASE)+1);
      strcpy(dbases[dbase_count++],BFADBASE);
   }

   if(a_flag) {
      if((rep_file = fopen(r_name,"a")) == NULL) {
         fprintf(stderr,"Can't open %s as report file\n",r_name);
         exit(BFA_ERROR);
      }
   } else if(r_flag) {
      if((rep_file = fopen(r_name,"w")) == NULL) {
         fprintf(stderr,"Can't open %s as report file\n",r_name);
         exit(BFA_ERROR);
      }
   } else
      rep_file = stdout;

   if(s_flag || e_flag)
      SummaryReport();

   if(f_flag || e_flag) 
      FileReport();

   if(p_flag || e_flag)
      ProcedureReport();
  
   if(b_flag || e_flag) 
      BranchReport();
   
   if(z_flag)
      ZeroDbases();

} /* main */



ReadLine(fd,string)
int	fd;
char	*string;
{
	char	c;
	int	i = 0;

	while( read(fd,&c,1) == 1) {
		if(c == '\n') {
		   string[i] = '\0';
		   return;
		}
		string[i++] = c;
	}
	string[i] = '\0';
} /* ReadLine */


ProcessSignOffList(command)
char	*command;
{
	SIGN_OFF	*tmp;

	char	string[MAXPATH];
	char	dbase[MAXPATH];
	char	mod[MAXPATH];
	char	num[MAXPATH];

	int	i = 0;
	int	j = 0;
	int	file_flag = 0;
	int	fd;

	if(command[0] == '&') {
	   if((fd = open(&command[1],O_RDONLY)) < 0) {
	      fprintf(stderr,"Error: Can't open sign off data file %s\n",
			&command[1]);
	      exit(1);
	   }
	   ReadLine(fd,string);
	   file_flag = 1;
	} else 
	   strcpy(string,command);

	while(string[i] != '\0') {
	   for(j=0;string[i] != '\0' && string[i] != ',' && string[i] != ';' &&
		   string[i] != '\n' ;i++,j++)
	      dbase[j] = string[i];
	   dbase[j] = '\0';

	   if(string[i] != ',') {
	      fprintf(stderr,"ERROR: Expected database ");
	      fprintf(stderr,"name in sign off list\n");
	      exit(1);
	   }

	   for(i++,j=0;string[i] != '\0' && string[i] != ',' && 
		   string[i] != ';' && string[i] != '\n' ;i++,j++)
	      mod[j] = string[i];
	   mod[j] = '\0';

	   if(string[i] != ',') {
	      fprintf(stderr,"ERROR: Expected module ");
	      fprintf(stderr,"name in sign off list\n");
	      exit(1);
	   }

	   for(i++;string[i] != '\0' && string[i] != ';' && 
		   string[i] != '\n';) {
	      for(j=0;string[i] != '\0' && string[i] != ';' &&
		      string[i] != '\n' && string[i] != ',';i++,j++)
		 num[j] = string[i];
	      num[j] = '\0';
	   
	      if(string[i] == ',')
	         i++;

	      tmp = (SIGN_OFF *)Malloc(sizeof(SIGN_OFF));

	      if(! strlen(dbase))
	         tmp->database = NULL;
	      else {
		 tmp->database = Malloc(strlen(dbase)+1);		    
		 strcpy(tmp->database,dbase);
	      }

	      if(! strlen(mod))
	         tmp->module   = NULL;
	      else {
		 tmp->module   = Malloc(strlen(mod)+1); 
		 strcpy(tmp->module,mod);
	      }

	      tmp->branch       = atoi(num);

	      tmp->next = sign_list;
	      sign_list = tmp;

	   } /* for */

	   if(string[i] != '\0')
	      i++;

	   if(string[i] == NULL && file_flag) {
		i = 0;
		ReadLine(fd,string);
	   }
	} /* while */
} /* ProcessSignOffList */



int CheckSignOff(dbase,mod,branch)
char	*dbase;
char	*mod;
int	branch;
{
	SIGN_OFF	*tmp;

	if(! l_flag)
	   return(0);

	for(tmp=sign_list;tmp!=NULL;tmp=tmp->next) {
	   if((tmp->database == NULL || !strcmp(dbase,tmp->database)) &&
	      (tmp->module   == NULL || !strcmp(mod,tmp->module)) &&
	      tmp->branch == branch)
	      return(1);
	}

	return(0);
} /* CheckSignOff */



PrintLine(c)
char  c;
{
   int   i;

   fputc('\n',rep_file);
   fputc(' ',rep_file);

   for(i=0;i<78;i++)
      fputc(c,rep_file);

   fputc('\n',rep_file);
   fputc('\n',rep_file);
} /* PrintLine */



SummaryReport() {
   int   dbase;
   int   dbase_files;
   int   i;
   int   j;
   int   k;

   long  file       = 0l;
   long  file_hit   = 0l;
   long  proc       = 0l;
   long  proc_hit   = 0l;
   long  branch     = 0l;
   long  branch_hit = 0l;
   long  sign_hit   = 0l;


   for(i=0;i<dbase_count;i++) {
     
#if defined(MSDOS)
      if((dbase = open(dbases[i],O_RDONLY|O_BINARY)) < 0) {
#else
      if((dbase = open(dbases[i],O_RDONLY)) < 0) {
#endif
         fprintf(stderr,"Can't open database file %s\n",dbases[i]);
	 exit(BFA_ERROR);
      }
    
      read(dbase,&dbase_files,sizeof(int));

      for(j=0;j<dbase_files;j++) {
	 int  hit = 0;

         read(dbase,&file_info,sizeof(FILE_ENTRY));

         for(k=0;k<file_info.branches;k++) {
            read(dbase,&branch_info,sizeof(BRANCH_ENTRY));
	    if(!(branch_info.branch_type == _FALLTHRU && ! F_flag)) {
	       branch++;
	       if(branch_info.count || 
			CheckSignOff(dbases[i],file_info.file_name,k)) {

	          branch_hit++;

		  sign_hit += CheckSignOff(dbases[i],file_info.file_name,k);
	       }

               if(branch_info.branch_type == _ENTRY) {
                  proc++;
	          if(branch_info.count ||
			CheckSignOff(dbases[i],file_info.file_name,k)) {
		     proc_hit++;
		     hit++;
	          }
	       } /* if(branch.info... */
	    }
	 } /* for(k=0;... */

         file++;
	 if(hit || !file_info.branches)
	    file_hit++;

      } /* for(j=0;... */

      close(dbase);

      if(! m_flag)  {
         PrintSummaryReport(i,i+1,file,file_hit,proc,
			    proc_hit,branch,branch_hit,sign_hit);
         file = file_hit = proc = proc_hit = branch = branch_hit = sign_hit =0l;
      }
   } /* for(i=0;... */

   if(m_flag)  
      PrintSummaryReport(0,dbase_count,file,file_hit,proc,
			 proc_hit,branch,branch_hit,sign_hit);

} /* SummaryReport */


PrintSummaryReport(start,end,file,file_hit,proc,proc_hit,branch,branch_hit,
		   sign_hit)
int    start;
int    end;
long   file;
long   file_hit;
long   proc;
long   proc_hit;
long   branch;
long   branch_hit;
long   sign_hit;
{
   float   p;
   int     i;

   cur_time    = time((long *)0);
   time_string = ctime(&cur_time);

   PrintLine('=');
   fprintf(rep_file,"   BFA Database Statistics Summary Report");
   fprintf(rep_file,"            %s\n\n",time_string);

   for(i=start;i<end;i++)
      fprintf(rep_file,"   Database file name - %s\n",dbases[i]);

   PrintLine('-');
   fprintf(rep_file,"\t\t\t   Total #\t    # Hit\t   %% Hit\n");

   p = (file ? ((float)file_hit / (float)file) * 100.00 : 0.0);
   fprintf(rep_file,"\tFiles\t\t%8ld\t%8ld\t%8.2f%%\n", file,file_hit,p);

   p = (proc ? ((float)proc_hit / (float)proc) * 100.00 : 0.0);
   fprintf(rep_file,"\tProcedures\t%8ld\t%8ld\t%8.2f%%\n",proc,proc_hit,p);

   p = (branch ? ((float)branch_hit / (float)branch) * 100.00 : 0.0);
   fprintf(rep_file,"\tBranches\t%8ld\t%8ld\t%8.2f%%\n",branch,branch_hit,p);

   if( l_flag) {
      p = (branch ? ((float)sign_hit / (float)branch) * 100.00 : 0.0);
      fprintf(rep_file,"\t     Signed Off\t\t\t%8ld\t%8.2f%%\n",
			  sign_hit,p);
   }

   PrintLine('=');
   fprintf(rep_file,"\f \n");

} /* PrintSummaryReport */



FileReport() {
   int   dbase;
   int   dbase_files;
   int   i;
   int   j;
   int   k;

   long   proc       = 0l;
   long   proc_hit   = 0l;
   long   branch     = 0l;
   long   branch_hit = 0l;
   long   sign_hit   = 0l;

   float  p;


   for(i=0;i<dbase_count;i++) {
#if defined(MSDOS)
      if((dbase = open(dbases[i],O_RDONLY|O_BINARY)) < 0) {
#else
      if((dbase = open(dbases[i],O_RDONLY)) < 0) {
#endif
	 fprintf(stderr,"Can't open database file %s\n",dbases[i]);
	 exit(BFA_ERROR);
      }

      read(dbase,&dbase_files,sizeof(int));

      for(j=0;j<dbase_files;j++) {
         read(dbase,&file_info,sizeof(FILE_ENTRY));

	 for(k=0;k<file_info.branches;k++) {
            
	    read(dbase,&branch_info,sizeof(BRANCH_ENTRY));

	    if(!(branch_info.branch_type == _FALLTHRU && ! F_flag)) {
	       branch++;
	       if(branch_info.count ||
			CheckSignOff(dbases[i],file_info.file_name,k)) {

	          branch_hit++;

		  sign_hit += CheckSignOff(dbases[i],file_info.file_name,k);
	       }


	       if(branch_info.branch_type == _ENTRY) {
                  proc++;
	          if(branch_info.count ||
			CheckSignOff(dbases[i],file_info.file_name,k))
		     proc_hit++;

	       }
	    }
	 } /* for(k=0;... */

         cur_time    = time((long *)0);
         time_string = ctime(&cur_time);

         PrintLine('=');
         fprintf(rep_file,"   BFA Database File Report              ");
         fprintf(rep_file,"            %s\n\n",time_string);

         fprintf(rep_file,"   Database file name - %s\n",dbases[i]);
         fprintf(rep_file,"   Source file name   - %s\n",
	                  file_info.file_name);
         PrintLine('-');
         fprintf(rep_file,"\t\t\t   Total #\t    # Hit\t   %% Hit\n");

         p = (proc ? ((float)proc_hit / (float)proc) * 100.00 : 0.0);
         fprintf(rep_file,"\tProcedures\t%8ld\t%8ld\t%8.2f%%\n",
	    	          proc,proc_hit,p);

         p = (branch ? ((float)branch_hit / (float)branch) * 100.00 : 0.0);
         fprintf(rep_file,"\tBranches\t%8ld\t%8ld\t%8.2f%%\n",
			  branch,branch_hit,p);

         if( l_flag) {
            p = (branch ? ((float)sign_hit / (float)branch) * 100.00 : 0.0);
            fprintf(rep_file,"\t     Signed Off\t\t\t%8ld\t%8.2f%%\n",
			        sign_hit,p);
         }

         PrintLine('=');
         fprintf(rep_file,"\f \n");

	 proc = proc_hit = branch = branch_hit = sign_hit = 0l;

      } /* for(j=0;... */
      close(dbase);
   } /* for(i=0;... */
} /* FileReport */




ProcedureReport() {
   int   dbase;
   int   dbase_files;
   int   i;
   int   j;
   int   k;

   long   proc       = 0l;
   long   proc_hit   = 0l;
   long   branch     = 0l;
   long   branch_hit = 0l;
   long   sign_hit   = 0l;

   long	  prev_addr;

   float  p;

   char	  tmp[BUFSIZ];

   for(i=0;i<dbase_count;i++) {
#if defined(MSDOS)
      if((dbase = open(dbases[i],O_RDONLY|O_BINARY)) < 0) {
#else
      if((dbase = open(dbases[i],O_RDONLY)) < 0) {
#endif
	 fprintf(stderr,"Can't open database file %s\n",dbases[i]);
	 exit(BFA_ERROR);
      }

      read(dbase,&dbase_files,sizeof(int));

      for(j=0;j<dbase_files;j++) {
         read(dbase,&file_info,sizeof(FILE_ENTRY));

	 cur_time    = time((long *)0);
         time_string = ctime(&cur_time);
	 
         PrintLine('=');
         fprintf(rep_file,"   BFA Database Procedure Report         ");
         fprintf(rep_file,"            %s\n\n",time_string);

         fprintf(rep_file,"   Database file name - %s\n",dbases[i]);
         fprintf(rep_file,"   Source file name   - %s\n",
	                  file_info.file_name);
         PrintLine('-');

	 fprintf(rep_file,"\t\t\t    Branch Hit Procedure Report\n\n");
	 fprintf(rep_file,"\tProcedure Name            # Branches       ");
	 fprintf(rep_file,"# Hit         %% Hit\n");
	 fprintf(rep_file,"\t-------------------------------------------");
	 fprintf(rep_file,"-------------------\n");

	 if(file_info.branches) {
	    prev_addr = lseek(dbase,0L,1);
	    read(dbase,&branch_info,sizeof(BRANCH_ENTRY));
	 }

	 for(k=0;k<file_info.branches;) {
	    if(branch_info.branch_type == _ENTRY) {
               proc++;
	       if(branch_info.count ||
			CheckSignOff(dbases[i],file_info.file_name,k)) {

		  proc_hit++;
		  sign_hit += CheckSignOff(dbases[i],file_info.file_name,k);
	       }
         
	       strcpy(tmp,branch_info.proc_name);
	       tmp[25] = '\0';

               fprintf(rep_file,"\t%-25s",tmp);

	       do {
	          if(!(branch_info.branch_type == _FALLTHRU && ! F_flag)) {
                     branch++;
	             if(branch_info.count ||
			CheckSignOff(dbases[i],file_info.file_name,k)) {

		        branch_hit++;
		        sign_hit+=CheckSignOff(dbases[i],file_info.file_name,k);
	             }
		  }
		  if(++k < file_info.branches)
	             read(dbase,&branch_info,sizeof(BRANCH_ENTRY));

	       } while(k < file_info.branches &&
		       branch_info.branch_type != _ENTRY);

	    } else {
              fprintf(stderr,"BFA Database Error - ProcedureReport\n");
	      exit(BFA_ERROR);
	    }

            p = (branch ? ((float)branch_hit / (float)branch) * 100.00 : 0.0);
            fprintf(rep_file,"%8ld      %8ld      %8.2f%%\n",
			       branch,branch_hit,p);

	    branch = branch_hit = 0l;
	 } /* for(k=0;... */
	 fprintf(rep_file,"\t-------------------------------------------");
	 fprintf(rep_file,"-------------------\n");

         p = (proc ? ((float)proc_hit / (float)proc) * 100.00 : 0.0);

         fprintf(rep_file,"\t\t\t%8.2f%% of Procedures Hit\n",p);

         proc = proc_hit = sign_hit = 0l;

	 if(! l_flag) {
            PrintLine('=');
            fprintf(rep_file,"\f \n");
	    continue;
	 }


         PrintLine('-');
	 fprintf(rep_file,"\t\t\t    Sign Off Procedure Report\n\n");
	 fprintf(rep_file,"\tProcedure Name            # Branches  ");
	 fprintf(rep_file,"# Signed Off  %% Signed Off\n");
	 fprintf(rep_file,"\t-------------------------------------");
	 fprintf(rep_file,"---------------------------\n");

   
	 if(file_info.branches) {
	    lseek(dbase,prev_addr,0);
	    read(dbase,&branch_info,sizeof(BRANCH_ENTRY));
	 }

	 for(k=0;k<file_info.branches;) {
	    if(branch_info.branch_type == _ENTRY) {

	       strcpy(tmp,branch_info.proc_name);
	       tmp[25] = '\0';

               fprintf(rep_file,"\t%-25s",tmp);

	       do {
	          if(!(branch_info.branch_type == _FALLTHRU && ! F_flag)) {
                     branch++;

	             if(CheckSignOff(dbases[i],file_info.file_name,k)) 
	         	sign_hit++;
		   }
		   if(++k < file_info.branches)
	              read(dbase,&branch_info,sizeof(BRANCH_ENTRY));

	       } while(k < file_info.branches &&
	           branch_info.branch_type != _ENTRY);

	    } else {
              fprintf(stderr,"BFA Database Error - ProcedureReport\n");
	      exit(BFA_ERROR);
	    }

            p = (branch ?((float)sign_hit / (float)branch) * 100.00 : 0.0);
            fprintf(rep_file,"%8ld     %8ld      %8.2f%%\n",
		          branch,sign_hit,p);

	    branch = branch_hit = sign_hit = 0l;
	 } /* for(k=0;... */
	 fprintf(rep_file,"\t-------------------------------------");
	 fprintf(rep_file,"---------------------------\n");

         PrintLine('=');
         fprintf(rep_file,"\f \n");

      } /* for(j=0;... */

      close(dbase);
   } /* for(i=0;... */
} /* ProcedureReport */



BranchReport() {
   int   dbase;
   int   dbase_files;
   int   i;
   int   j;
   int   k;
   int   count;

   long   branch     = 0l;
   long   branch_hit = 0l;
   long   sign_hit   = 0l;

   float  p;

   int	  ft_line[1000];
   int	  ft_index = 0;

   int	  switch_flag = 0;



   for(i=0;i<dbase_count;i++) {
#if defined(MSDOS)
      if((dbase = open(dbases[i],O_RDONLY|O_BINARY)) < 0) {
#else
      if((dbase = open(dbases[i],O_RDONLY)) < 0) {
#endif
	 fprintf(stderr,"Can't open database file %s\n",dbases[i]);
	 exit(BFA_ERROR);
      }

      read(dbase,&dbase_files,sizeof(int));

      for(j=0;j<dbase_files;j++) {
         read(dbase,&file_info,sizeof(FILE_ENTRY));


	 if(file_info.branches)
	    read(dbase,&branch_info,sizeof(BRANCH_ENTRY));

	 for(k=0;k<file_info.branches;) {
	    if(branch_info.branch_type == _ENTRY) {
	       int   num = 1;

         
	       cur_time    = time((long *)0);
               time_string = ctime(&cur_time);
	 
               PrintLine('=');
               fprintf(rep_file,"   BFA Database Branch Report            ");
               fprintf(rep_file,"            %s\n\n",time_string);

               fprintf(rep_file,"   Database file name - %s\n",dbases[i]);
               fprintf(rep_file,"   Source file name   - %s\n",
	                        file_info.file_name);
               fprintf(rep_file,"   Procedure name     - %s\n",
	                        branch_info.proc_name);
               PrintLine('-');

	       if(!i_flag) {
	          fprintf(rep_file,"          Branch #       Type        ");
	          fprintf(rep_file,"Line #       # Hits     Not Hit\n");
	          fprintf(rep_file,"          ---------------------------");
	          fprintf(rep_file,"-------------------------------\n");
	       } else {
	          fprintf(rep_file,"      Branch #       Type        ");
	          fprintf(rep_file,"Line #    Index #    # Hits     Not Hit\n");
	          fprintf(rep_file,"      ---------------------------");
	          fprintf(rep_file,"---------------------------------------\n");
	       }

	       do {
	          if(!(branch_info.branch_type == _FALLTHRU && ! F_flag)) {
                     branch++;
		     if(branch_info.count ||
			CheckSignOff(dbases[i],file_info.file_name,k)) {

	                branch_hit++;

		        sign_hit+=CheckSignOff(dbases[i],file_info.file_name,k);
	             }


		     if(!i_flag)
	                fprintf(rep_file,"         ");
		     else
	                fprintf(rep_file,"     ");

                     fprintf(rep_file,"%6d       ",num);

		     switch(branch_info.branch_type) {
                        case _ENTRY   : fprintf(rep_file,"   entry      ");
		   		        break;
                        case _FOR     : ft_line[ft_index++] = 
						branch_info.line_num;
					fprintf(rep_file,"     for      ");
				        break;
                        case _WHILE   : ft_line[ft_index++] =
						branch_info.line_num;
					fprintf(rep_file,"   while      ");
				        break;
                        case _DO      : ft_line[ft_index++] =
						branch_info.line_num;
					fprintf(rep_file,"      do      ");
				        break;
                        case _CASE    : if(! switch_flag) {
					   switch_flag = 1;
					   ft_line[ft_index++] =
						branch_info.line_num;
					}
					fprintf(rep_file,"    case      ");
				        break;
                        case _DEFAULT : if(! switch_flag) {
					   switch_flag = 1;
					   ft_line[ft_index++] =
						branch_info.line_num;
					}
					fprintf(rep_file," default      ");
				        break;
                        case _IF      : ft_line[ft_index++] =
						branch_info.line_num;
					fprintf(rep_file,"      if      ");
				        break;
                        case _ELSE    : 
					fprintf(rep_file,"    else      ");
				        break;
                        case _ELSEIF  :
					fprintf(rep_file,"  elseif      ");
				        break;
                        case _LABEL   :
					fprintf(rep_file,"   label      ");
				        break;
                        case _FALLTHRU: ft_index--; 
					switch_flag = 0;
					fprintf(rep_file,"fallthru/%-5d",
						ft_line[ft_index]);
				        break;
		        default       : fprintf(stderr,"Unknown Branch Type\n");
				        fprintf(stderr,"BranchReport()\n");
				        exit(BFA_ERROR);
		     }
		     if(i_flag) 
		        fprintf(rep_file,"%5d %9d %11ld          ",
			        branch_info.line_num,k,branch_info.count);
		     else
		        fprintf(rep_file,"%7d %11ld          ",
			        branch_info.line_num,branch_info.count);
   
		     if(branch_info.count)
		        fprintf(rep_file," \n");
		     else if(CheckSignOff(dbases[i],file_info.file_name,k)) 
		        fprintf(rep_file,"SO\n");
		     else
		        fprintf(rep_file,"*\n");

		     num++;
  		  } 

                  k++;

		  if(k < file_info.branches)
	             read(dbase,&branch_info,sizeof(BRANCH_ENTRY));

	       } while(k < file_info.branches &&
		       branch_info.branch_type != _ENTRY);

	    } else {
              fprintf(stderr,"BFA Database Error - BranchReport\n");
	      exit(BFA_ERROR);
	    }
	    if(!i_flag) {
	       fprintf(rep_file,"          ---------------------------");
	       fprintf(rep_file,"-------------------------------\n");
	    } else {
	       fprintf(rep_file,"      -------------------------------");
	       fprintf(rep_file,"-----------------------------------\n");
	    }

            p = (branch ? ((float)branch_hit / (float)branch) * 100.00 : 0.0);

	    fprintf(rep_file,"\t\t\t%8.2f%% of Branches Hit\n",p);

	    if(l_flag) {
               p = (branch ? ((float)sign_hit / (float)branch) * 100.00 : 0.0);

	       fprintf(rep_file,"\t\t\t%8.2f%% of Branches Signed Off\n",p);
	    }

            PrintLine('=');
            fprintf(rep_file,"\f \n");
	    branch = branch_hit = sign_hit = 0l;
	 } /* for(k=0;... */

      } /* for(j=0;... */
      close(dbase);
   } /* for(i=0;... */
} /* BranchReport */



ZeroDbases() {
   int   dbase;
   int   dbase_files;

   int   i;
   int   j;
   int   k;

   long  pos;

   for(i=0;i<dbase_count;i++) {
#if defined(MSDOS)
      if((dbase = open(dbases[i],O_RDWR|O_BINARY)) < 0) {
#else
      if((dbase = open(dbases[i],O_RDWR)) < 0) {
#endif
	 fprintf(stderr,"Can't open database file %s\n",dbases[i]);
	 exit(BFA_ERROR);
      }
      read(dbase,&dbase_files,sizeof(int));

      for(j=0;j<dbase_files;j++) {
          read(dbase,&file_info,sizeof(FILE_ENTRY));
	  for(k=0;k<file_info.branches;k++) {
             pos = lseek(dbase,0L,1);
	     read(dbase,&branch_info,sizeof(BRANCH_ENTRY));
	     branch_info.count = 0l;
	     lseek(dbase,pos,0);
	     write(dbase,&branch_info,sizeof(BRANCH_ENTRY));
	  }
      }
      close(dbase);
   }
} /* ZeroDbases() */


