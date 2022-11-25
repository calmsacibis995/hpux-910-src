#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>

#include "defines.h"
#include "global.h"

#if defined(UNIX)
#include <sys/dir.h>
#endif

extern char *Malloc();

main(argc,argv)
int    argc;
char   *argv[];
{
	int   i;
	int   j;

	
	c_args[0] = '\0';
	getcwd(dbase_file,MAXPATH);
	strcat(dbase_file,"/");
	strcat(dbase_file,BFADBASE);


#if defined(MSDOS)
	strcpy(cpp,"MSC ");
	strcpy(cpp_end," /E;");
#endif

#if defined(UNIX)
	strcpy(cpp,"cc -E ");
	strcpy(cpp_end," ");
#endif

	main_ids[main_cnt++] = "main";

/* Process the command line arguments  */

	for(i=1;i<argc;i++) {
	   if(argv[i][0] == '-') {
	      
	      switch(argv[i][1]) {
	         case 'h': h_flag = 1;
	                   a_flag = 0; 
	                   break;
	         case 'a': a_flag = 1;
	                   h_flag = 0;
	                   break;
	         case 'q': q_flag = 1;
	                   break;
	         case 'm': m_flag = 1; 
	                   break;
	         case 'i': i_flag = 1;
	                   break;
	         case 'e': e_flag = 0;
	                   break;
		 case 'c' :
			   if(argv[i][2] == 'p' && argv[i][3] == 'p' &&
			      argv[i][4] == 's') {
			      strcpy(cpp,&argv[i][5]);
			   } else if(argv[i][2] == 'p' && argv[i][3] == 'p' &&
			             argv[i][4] == 'e') {
			      strcpy(cpp_end,&argv[i][5]);
			   } else  {
#if defined(UNIX)
	                      fprintf(stderr,"Unknown option %s\n",argv[i]);
                              exit(BFA_ERROR); 
#endif
#if defined(MSDOS)
                              fprintf(stderr,
           "Unknown BFA option %s passing it to the C Preprocessor\n",argv[i]);
                              strcat(c_args,argv[i]);
                              strcat(c_args," ");
#endif
			   } /* else */
			   break;
		 case 'C':
			   if(argv[i][2] == NULL) 
			      stack_elements = atoi(argv[++i]);
			   else
			      stack_elements = atoi(&argv[i][2]);
			   break;
		 case 'S':
			   if(argv[i][2] == NULL) 
			      stream_len = atoi(argv[++i]);
			   else
			      stream_len = atoi(&argv[i][2]);
			   break;
		 case 'E':
			   if(argv[i][2] == NULL) 
			      else_len = atoi(argv[++i]);
			   else
			      else_len = atoi(&argv[i][2]);
			   break;
		
#if defined(FORK)
	         case 'f': f_flag = 0;
	                   break;
#endif

#if defined(EXEC)
	         case 'x': x_flag = 0;
	                   break;
#endif


#if defined(BFA_KERNEL)
		case 'k': k_flag = 1; 
	                  e_flag = 0;
#if defined(EXEC)
                  	  x_flag = 0;
#endif
#if defined(FORK)
	                  f_flag = 0;
#endif
	                  break;
#endif

#if defined(TSR)
	       case 'r' : r_flag = 1;
	                  if(argv[i][2] != '\0')
	                     vector = atoi(&argv[i][2]);
	                  break;
#endif

#if defined(BFAWIN)
	       case 'w' : w_flag = 1;
			  break;
#endif

	       case 'd': strcpy(dbase_file,&argv[i][2]);
	                 break;
	       case 'u': if(main_cnt == MAXMAINS) {
	                    fprintf(stderr,"Too many update ids specified\n");
	                    exit(BFA_ERROR);
	                 }

	                 main_ids[main_cnt] = Malloc(strlen(argv[i]));
	                 strcpy(main_ids[main_cnt++],&argv[i][2]);
	                 break;
	       case 'U': strcat(c_args,argv[i]);
	                 strcat(c_args," ");
	                 break;
	       case 'I': strcat(c_args,argv[i]);
	                 strcat(c_args," ");
	                 break;
	       case 'D': strcat(c_args,argv[i]);
	                 strcat(c_args," ");
	                 break;
#if defined(HPUX)
	       case 't': if(argv[i][2] == '\0') 
	                 i++;
	                 {
	                    int k;

	                    for(k=0;argv[i][k]!=',' && argv[i][k] != '\0';k++);

	                    strcat(c_args,&argv[i][k+1]);
	                    strcat(c_args," ");
	                 }
	                 break;
	       case 'W': strcat(c_args,argv[i]);
	                 strcat(c_args," ");
	                 break;
#endif
#if defined(UNIX)
	       default : fprintf(stderr,"Unknown option %s\n",argv[i]);
	                 exit(BFA_ERROR); 
#endif
#if defined(MSDOS)
	       default : fprintf(stderr,
		"Unknown BFA option %s passing it to the C Preprocessor\n",
			argv[i]);
	                 strcat(c_args,argv[i]);
	                 strcat(c_args," ");
#endif

	      } /* switch */
	   } else {
	      FILE   *tmp;
	
	      if(num_input == MAXFILES) {
	         fprintf(stderr,"Too many source files specified\n");
	         exit(BFA_ERROR);
	      }

	      input_files[num_input++] = argv[i];

	      if(strcmp(&argv[i][strlen(argv[i])-2],".c") &&
	         strcmp(&argv[i][strlen(argv[i])-2],".C")) {
	         fprintf(stderr,
		    "Illegal file name - %s Files must end in `.c'\n",argv[i]);
	         exit(BFA_ERROR);
	      }

	      if((tmp = fopen(input_files[num_input-1],"r")) == NULL) {
	         fprintf(stderr,"Can't open %s\n",input_files[num_input-1]);
	         exit(BFA_ERROR);
	      }
	      fclose(tmp);

#if defined(MSDOS)
	      strupr(input_files[num_input-1]);
#endif
	   } /* else */
	} /* for(i=1;i<argc;i++) { */

	stack = (STACK_ELEMENT *)Malloc((unsigned)(
		(unsigned)sizeof(STACK_ELEMENT) * (unsigned)stack_elements));
	stream_string = (char *)Malloc((unsigned)(
		(unsigned)sizeof(char) * (unsigned)stream_len));
	else_string = (char *)Malloc((unsigned)(
		(unsigned)sizeof(char) * (unsigned)else_len));

	if(!num_input)
	   exit(0);

	ProcessFiles();
	exit(bfa_ret);
} /* main */



ProcessFiles() {
	int   ret_val;


	OpenDbase();
	NextFile();
	
	do {
	   InitScanner();
	   NewFile();

	   yylex();

	   if(!q_flag)
	      fprintf(stderr,"-> %s\n",cur_output);

	   CloseFile();
	} while(NextFile());

	CloseDbase();
} /* ProcessFiles */




Exit(ret_val)
int   ret_val;
{
   pclose(yyin);
   fclose(outfile);
   exit(ret_val);
} /* Exit */



char *Malloc(size)
unsigned int	size;
{
	extern	char	*malloc();
	extern	int	errno;

	char	*tmp;

	if((tmp = malloc(size)) == NULL) {
	   fprintf(stderr,"Malloc failed trying to allocate %d bytes\n",size);
	   fprintf(stderr,"Errno = %d - Aborting BFA\n",errno);
	   exit(1);
	} /* if */

	return(tmp);
} /* Malloc */



#if defined(MSDOS)

#include <sys/stat.h>
#include <io.h>

#define TEMPFILE "bfatemp.$$$"
#define MAXFILE  20

int pclose(file_ptr)
FILE   *file_ptr;
{
	int   ret_val;

	ret_val = fclose(file_ptr);
	unlink(TEMPFILE);
	return(ret_val);
} /* pclose */


FILE *popen(command,attribute)
char   command[];
char   attribute[];
{
	FILE   *temp_file;
	int    handle;
	char   prog[32];

	static char *args[64];
	static char arglist[512];

	int i = 0;
	int j = 0;
	int k = 0;
	
	while(command[i] == ' ')
	   i++;

	while(command[i] != ' ')
	   prog[j++] = command[i++];
	prog[j] = '\0';

	
	strcpy(arglist,command);

	for(i=0,k=0;arglist[i] == ' ';i++);

	args[k] = &arglist[i];

	for(j=strlen(arglist);i < j;i++) {
	   if(arglist[i] == ' ' || arglist[i] == '\t') {
	      arglist[i] = '\0';

	      for(i++;arglist[i] == ' ' || arglist[i] == '\t';i++);
	      args[++k] = &arglist[i];
	   }
	}
	args[++k] = NULL;

	if((temp_file = freopen(TEMPFILE,"w+",stdout)) == NULL) {
	   fprintf(stderr,"Error creating temporary file\n");
	   exit(BFA_ERROR);
	}

	if(spawnvp(0,prog,args)) 
	   return(NULL);
   
	if((temp_file = freopen("con","w+",temp_file)) == NULL) {
	   fprintf(stderr,"Error recreating standard output link\n");
	   exit(BFA_ERROR);
	}

	return(fopen(TEMPFILE,attribute));
} /* popen */


int link(from,to)
char   from[];
char   to[];
{
	static char buff[BUFSIZ];

	int   from_id;
	int   to_id;

	if((from_id = open(from,O_RDONLY|O_BINARY)) < 0) {
	  return(-1);
	}

	if((to_id = open(to,O_RDWR|O_BINARY|O_CREAT,S_IWRITE)) < 0) {
	  return(-1);
	}

	while(write(to_id,buff,read(from_id,buff,BUFSIZ)) > 0);

	close(from_id);
	close(to_id);
	unlink(from);
	return(0);
} /* link */

#endif

