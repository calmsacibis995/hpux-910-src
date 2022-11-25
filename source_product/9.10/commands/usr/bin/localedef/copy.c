/* @(#) $Revision: 70.9 $ */
#include <stdio.h>
/* #include "lctypes.h" */
#include "global.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "error.h"

#define LCPATHLEN    80
#define ERRLEN       90

int gotstr;

void
copy_init(token)
int token;
{

	extern void copy_str();
	extern void copy_finish();

	if (op) {
    	/* IF WE ARE NOT THE FIRST, FINISH UP LAST KEYWORD */
    	   if (finish == NULL ) error(STATE);
           (*finish)();
	}


	op = token;
	string = copy_str;
	finish = copy_finish;

	gotstr = FALSE; /* Revisit - do you need this */
}

#define CATINFO_SIZE (sizeof(struct catinfotype))
#define HEADER_SIZE  (sizeof(struct lctable_header))

void
copy_str()
{
    extern void getstr();
    extern int copy_flag;
    int fd1;
    int n_mod=0;
    struct lctable_header *lcbuf;
    struct catinfotype catinfo_buf[N_CATEGORY*MAXCMNUM];
    char lcpath[LCPATHLEN], errmsg[ERRLEN], *cp;
    unsigned char buf[30]; /* Revisit on the array size */
    FILE *fp, *fp1;
    char *file_str;
    int loop_var=0, arr_var=0, k, l; 
    char ch;

    if (gotstr) error (STATE);

    /*
    ** open the "locale.inf" file
    */
    (void) getstr(buf);
    (void) strcpy(lcpath, NLSDIR);
    (void) strcat(lcpath, buf);
    for (cp = lcpath; *cp; cp++)    /* replace '_' and '.' with '/' */
            if (*cp == '_' || *cp == '.')
                    *cp = '/';
    (void) strcat(lcpath, "/locale.inf");
    if((fd1 = open(lcpath, O_RDONLY)) == NULL) {
            (void) fprintf(stderr, catgets(catd,NL_SETN,9,"can't open input file: %s\n"), lcpath);
            exit(4);
    }

    lcbuf=(struct lctable_header *) malloc(HEADER_SIZE);
    if ( lcbuf == NULL )
      	error(NOMEM); 

    if ( read(fd1,lcbuf,HEADER_SIZE) != HEADER_SIZE )  {
       (void) strcpy(errmsg, catgets(catd,NL_SETN,78,"read error on file: "));
       (void) strcat(errmsg, lcpath);
       Error(errmsg,4);
    }

    if ( read(fd1,catinfo_buf,(lcbuf->cat_no+lcbuf->mod_no)*CATINFO_SIZE) 
	 != (lcbuf->cat_no+lcbuf->mod_no)*CATINFO_SIZE) {
       (void) strcpy(errmsg, catgets(catd,NL_SETN,78,"read error on file: "));
       (void) strcat(errmsg, lcpath);
       Error(errmsg,4);
    }

    for (k=0; k<(lcbuf->mod_no+lcbuf->cat_no); k++) {
	int offset, n, ino;
	struct stat stat_buf;

	if ( catinfo_buf[k].catid != cur_cat )
	   continue;
	else {
	   cmlog[cur_cat].cm[cur_mod].size = catinfo_buf[k].size;
    	   cmlog[cur_cat].cm[cur_mod].pad = 0; /*size already has padding 
					         built into it*/
    	   (void) strcpy (cmlog[cur_cat].cm[cur_mod].mod_name, 
		       	  catinfo_buf[k].mod_name);
	   fp1 = tmpfile();
	   ino = fileno(fp1);
	   fstat (ino, &stat_buf);
           cmlog[cur_cat].cm[cur_mod].tmp = fp1;

	   offset = HEADER_SIZE+(lcbuf->cat_no+lcbuf->mod_no)*CATINFO_SIZE +
	      	    (k ? 
		    (catinfo_buf[k-1].addr+catinfo_buf[k-1].size)
		    : 0);

	   if ( lseek(fd1, offset, SEEK_SET) == -1) {
             (void)strcpy(errmsg,catgets(catd,NL_SETN,79,"can't lseek file: "));
             (void)strcat(errmsg, lcpath);
             Error(errmsg,4);
           }

	   for (l=0; l<cmlog[cur_cat].cm[cur_mod].size; l++) {
               if ( read(fd1, &ch, sizeof(ch)) != sizeof(ch) ) {
                  (void) strcpy(errmsg, catgets(catd,NL_SETN,78,
			        "read error on file: "));
                  (void) strcat(errmsg, lcpath);
                  Error(errmsg,4);
               }
               (void) fprintf(fp1,"%c",ch);
           }

           fflush(fp1);
           rewind(fp1);

           cur_mod++;
	}
    }

    cmlog[cur_cat].n_mod = cur_mod;
    copy_flag=TRUE;
}	

void
copy_finish()
{

/* any other error checking */

}
