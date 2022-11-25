#include "ldefs.c"
#include "msgs.h"

FILE *errorf;
nl_catd catd;

/*
**  ROUTINE: error()
**  This routine is used to output error messages and exit the program.
**  The legal errnum values are defined in msgs.h.  To add a new message
**  consult the documentation in msgs.h.
**  This routine can take up to 7 optional arguments 
**  to be inserted in the error message that is printed to the user.
*/
error(errnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
    int errnum;
    char *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7;
{
    print_msg(Error_String, errnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
   if(
# ifdef DEBUG
      debug ||
# endif
      report == 1) 
      	   statistics();
    exit(1);
}

/*
**  ROUTINE: warning()
**  This routine is used to output warning messages. 
**  The legal warnnum values are defined in msgs.h.  To add a new message
**  consult the documentation in msgs.h.
**  This routine can take up to 7 optional arguments 
**  to be inserted in the warning message that is printed to the user.
*/
warning(warnnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
    int warnnum;
    char  *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7;
{
    print_msg(Warning_String, warnnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}


/*
**  ROUTINE: message()
**  This routine is used to output descriptive messages. 
**  The legal msgnum values are defined in msgs.h.  To add a new message
**  consult the documentation in msgs.h.
**  This routine can take up to 7 optional arguments 
**  to be inserted in the message that is printed to the user. 
*/
message(msgnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
    int msgnum;
    char  *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7;
{
    print_msg(Message_String, msgnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
}


#define COLONMAX 20 /* Max size for the Colon String (:) */
#define TYPEMAX 20  /* Max size for the message type string. */
#define LINEMAX 20  /* Max size for the line number string. */
#define MSGMAX  400 /* Max size for the body of a message. */

print_msg(msg_type, msgnum, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
    int msg_type, msgnum;
    char *arg1, *arg2, *arg3, *arg4, *arg5, *arg6, *arg7;
{
    char msg[MSGMAX];
    char type[TYPEMAX];
    char linemsg[LINEMAX];
    char colon[COLONMAX];
    int  status;

    /* Get the local representation of the colon seperator. */
    status = catread(catd, Const_Set, COLON, colon, COLONMAX);

    /* Get the message type (e.g., error, warning). */
    if (msg_type != Message_String) {
       status = catread(catd, Const_Set, msg_type, type, TYPEMAX);
       if (status < 0)
	  switch (msg_type) {
		case Warning_String: strcpy(&type[0], "warning:"); break;
		case Error_String:   strcpy(&type[0], "error:"); break;
 	  }
    }

    /* Get the string for "line" */
    status = catread(catd, Const_Set, Line_String, linemsg, LINEMAX);

    /* Get the basic message (the body of the message). */
    status = catread(catd, Msg_Set, msgnum, msg, MSGMAX, 
                     arg1, arg2, arg3, arg4, arg5, arg6, arg7);

    /* Print the message. */
    if (msg_type == Message_String) 
	/* Don't output file name, diagnostic type or line number */
    	fprintf(errorf, "%s", msg);
    else 
	/* Add file, line number and diagnostic type to the message */
        /* Sample Message:                                          */
        /*       "Foo.l" line 0: warning 2: Unknown option 'u'      */
    	fprintf(errorf, "\"%s\" %s %d%s %s %d%s %s",
                    	   inputfile, 
                                linemsg, 
                                   yyline, 
                                     colon, 
                                        type, 
                                           msgnum, 
                                             colon, 
                                                msg);
    
    fflush(errorf);
    fflush(fout);
    fflush(stdout);
}

open_err_cat()
{
    extern int errno;
    errno=0;

    catd = catopen(MF_LEX,NL_CAT_LOCALE);
    errorf = stderr;
    if ( catd == (nl_catd) -1) {
       catd = catopen("/usr/lib/nls/C/lex.cat",NL_CAT_LOCALE);
       if (strcmp("", getenv("LANG")))
          fprintf(errorf,"lex: warning: Cannot read %s message catalog; using default language\n",getenv("LANG"));
       else 
          fprintf(errorf,"lex: warning: Cannot read message catalog; using default language\n");
     }
}

/* Utility routines used to convert characters and integers to strings on
   the fly while outputting error/warning messages.                        */

char *itos(buf,x)
   char buf[];
   int x;
{
   sprintf(buf,"%d",x);
   return (&buf[0]);
}

char *ctos(buf,x)
   char buf[];
   char x;
{
   sprintf(buf,"%c",x);
   return (&buf[0]);
}

