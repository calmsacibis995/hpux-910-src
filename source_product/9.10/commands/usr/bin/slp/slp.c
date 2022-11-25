
/*  $Source: /misc/source_product/9.10/commands.rcs/usr/bin/slp/slp.c,v $  */
/*  $Revision: 70.5 $  */

#ifndef lint
static char *HPUX_ID = "$Revision: 70.5 $";
#endif
#include <stdio.h>
#include <signal.h>
#include <values.h>
#include <sys/ioctl.h>
#include <sys/lprio.h>

#define max(a,b)		(a<b ? b : a)
#define min(a,b)		(a>b ? b : a)
#define range(lo, hi, x)	max(lo, min(hi, x))

struct lprio lprsave;

main(argc, argv)
int argc;
char **argv;
{
	short s;
	int c;
	extern char *optarg;
	struct lprio lprstruct;
        char modify_drv = 'F';
	int newmode;
	int report_setting = 0;
	int set_default = 0;

	if(ioctl(1, LPRGET, &lprsave) == -1) {
		fprintf(stderr, "slp: error -- stdout is not a printer!\n");
		exit(1);
	}

	lprstruct = lprsave;
        newmode = COOKED_MODE;

	while((c = getopt(argc, argv, "abc:di:kl:norC:O:")) != EOF) {
		switch(c) {
		case 'a':
			report_setting = 1;
			break;
		case 'c':
			lprstruct.col = range(1, 227, atoi(optarg));
                        modify_drv = 'T';
			break;
		case 'd':
                        modify_drv = 'T';
			lprstruct.col = 0;		/* Flag reset to defaults at next open */
			set_default = 1;
			break;
		case 'i':
                        modify_drv = 'T';
			lprstruct.ind = range(0, 227, atoi(optarg));
			break;
		case 'k':
                        modify_drv = 'T';
                        newmode = COOKED_MODE;
                        break;
                case 'o':
                        modify_drv = 'T';
                        lprstruct.bksp = OVERSTRIKE;
			break;
		case 'l':
                        modify_drv = 'T';
			lprstruct.line = range(1, MAXSHORT, atoi(optarg));
			break;
		case 'n':
                        modify_drv = 'T';
			lprstruct.line = 0;		/* Flag to indicate never form-feed */
			break;
		case 'r':
                        modify_drv = 'T';
                        newmode = RAW_MODE;
			break;
		case 'O':
                        modify_drv = 'T';
			lprstruct.open_ej = range(0, 227, atoi(optarg));
			break;
		case 'C':
                        modify_drv = 'T';
			lprstruct.close_ej = range(0, 227, atoi(optarg));
			break;
		case 'b':
                        modify_drv = 'T';
			lprstruct.bksp = PASSTHRU;
			break;
		case '?':
			/* Usage message from HP-UX standard */
			fprintf(stderr, "Usage: %s [-a] [-b] [-ccols] [-d] [-iindent] [-k] [-llines] [-n] [-r] [-Cpages] [-Opages]\n");
			exit(1);
		}
	}
        if( newmode == RAW_MODE ) {
                lprstruct = lprsave;
        }
	lprstruct.raw_mode = newmode;

        if ( modify_drv == 'T' ) {
	   if(ioctl(1, LPRSET, &lprstruct) == -1) {
	       fprintf(stderr, "slp: error -- printer characteristics unchanged.\n");
	       exit(1);
	}
        }
	if ( ! report_setting )
	    exit(0);

	if ( set_default ) {
	    fprintf(stderr,"printer options are set to the defaults\n");
	    exit(0);
	}
	
	if (lprstruct.raw_mode == RAW_MODE) {
	    fprintf(stderr,"raw mode\n");
	    exit(0);
	}
	fprintf(stderr, "cooked mode; ");
	    if ( lprstruct.line == 0 )		/* never form-feed */
		fprintf(stderr, "lines = infinite; ");
	    else
		fprintf(stderr, "lines = %d; ", lprstruct.line);
	
	fprintf(stderr, "columns = %d; ", lprstruct.col);
	fprintf(stderr, "open ejects = %d; ", lprstruct.open_ej);
	fprintf(stderr, "close ejects = %d; ", lprstruct.close_ej);
	fprintf(stderr, "%s printer; ", 
		(lprstruct.bksp) ? "line" : "character");
	fprintf(stderr, "indent = %d;\n", lprstruct.ind);
	exit(0);
}
