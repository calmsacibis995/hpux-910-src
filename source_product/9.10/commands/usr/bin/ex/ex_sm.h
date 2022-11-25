/* @(#) $Revision: 56.2 $ */      
#ifdef ED1000

/* All definitions for ed1000 */
#define SM_SS 20                   /* basic screen size */
#define SM_MAXLLEN 150             /* max line len */

#undef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
var line *sm_start;                /* first line listed in screen mode */
var line *sm_end;                  /* last line listed in screen mode */
var int linesinbuf;              
var int sm_tmp;                  /* tmp to store stuff just read from screen */
var bool sm_clean;
var bool ask;                    /* whether to ask user which screen to save */
var bool insm;                   /* in screen mode. */
var bool oinsm;                  /* holds screen mode state (temp). */
var bool insmcmd;                /* in screen mode command loop */
var int ttyfd;                   /* tty */
var char ttybuf[20];
var int ss_size;                 /* This variable may be useful in the future to
                                    implement the set screen size command.Right
                                    now it does not do any thing important */
var int ss_max;                  /* maxium screen size possible */
var wrtsl();
var wrtel();
var sm_cmdloop();
var sm_main();
var int sm_get();
var sm_linebuf2[SM_MAXLLEN+1];        /* used to read line from screen */
var sm_linebuf[SM_MAXLLEN+1];
var char *escstr;
var bool chdot;                       /* should change the value of dot before
                                         restarting screen mode */
var int offset;                       /* cursor offset from top of screen */
var char cmdline[SM_MAXLLEN + 1];
var char sm_ch;
var int textptr;
var sm_help();
var jmp_buf sm_reslab;            /* error throws */
var strinsert();
var down;                         /* how many lines to move down after screen
                                     mode list.Usually 0 or center */
extern char sm_ctrltab[32];          /* How to print control characters */
#endif ED1000
