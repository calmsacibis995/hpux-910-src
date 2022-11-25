/* $Source: /misc/source_product/9.10/commands.rcs/etc/meas_sys/translate.c,v $
 * $Revision: 4.21 $		$Author: dah $
 * $State: Exp $		$Locker:  $
 * $Date: 87/03/10 14:11:11 $
 *
 * $Log:	translate.c,v $
 * Revision 4.21  87/03/10  14:11:11  14:11:11  dah
 * iscan wasn't terminating strings with nul's if they ended on 
 * word boundaries (for reconstructed ms_grab_id records
 * 
 * Revision 4.20  87/02/23  15:35:29  15:35:29  dah (Dave Holt)
 * works
 * 
 * Revision 4.19.1.1  87/02/02  16:52:04  16:52:04  dah (Dave Holt)
 * branch to try handling long records better.
 * Now handle 8000 (up from 1000), and if failure, print out the
 * offensive length.
 * 
 * Revision 4.19  86/06/24  14:57:35  14:57:35  dah (Dave Holt)
 * declare kmemf_flg so common.c will be happy.
 * 
 * Revision 4.18  86/05/12  15:35:52  dah (Dave Holt)
 * handle MS_ADMIN_INFO record type.
 * return 0 if no errors.
 * 
 * Revision 4.17  86/02/10  14:45:17  dah (Dave Holt)
 * workaround old meas_sys.h in 9.0
 * 
 * Revision 4.16  86/02/04  10:26:18  dah (Dave Holt)
 * pull \n's from bail_out calls
 * 
 * Revision 4.15  86/01/27  17:49:37  dah (Dave Holt)
 * reject extra arguments (DSDtg00042)
 * 
 * Revision 4.14  86/01/27  16:27:09  dah (Dave Holt)
 * again
 * 
 * Revision 4.10  85/12/12  13:55:42  dah (Dave Holt)
 * give it the right usage message
 * 
 * Revision 4.9  85/12/11  09:41:26  dah (Dave Holt)
 * try to handle short reads
 * (fread's lack of error return isn't very helpful)
 * 
 * Revision 4.8  85/12/10  15:52:39  dah (Dave Holt)
 * this version works with common.c 1.16
 * 
 * Revision 4.7.1.2  85/12/10  15:49:25  dah (Dave Holt)
 * include nlist.h
 * 
 * Revision 4.7.1.1  85/12/10  15:47:21  dah (Dave Holt)
 * modify to work with common.c
 * 
 * Revision 4.7  85/12/09  13:49:05  dah (Dave Holt)
 * modify to work with common
 * 
 * Revision 4.5  85/12/03  16:38:39  dah (Dave Holt)
 * added standard firstci header
 * 
 * $Endlog$
 */

/*
 * version of tranlate to take input like <len> <data>
 * instead of <len> <data> <len>
 * For release 4 of meas sys
 */
 
#include <stdio.h>
#include <nlist.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/meas_sys.h>

int debug = 0;
char *progname = "translate";
int chatty = 1;

/* Add stuff to make common.c happy.  Most is irrelevant to us. */
/* Make the values bogus, so that it will die if anyone tries to use them. */
int kmem = -17;
struct nlist nlst[1] = { { 0 } };
char *nlist_filename = "";
int ms_id = -17;
int my_rev = -17;
int ms_bin_id = -17;
int wizard = 0;

int kmemf_flg = 0;	/* to keep common.c happy */

#define MAX_REC_LEN 8000

main(argc,argv)
int argc;
char **argv;
{
    long    lseek ();
    int     rlen, id, type;
    int	    recbuf[MAX_REC_LEN +1];
    int     i;
    int cnt;

/*
 * MS_INCLUDE_REV should be defined.  The meas_sys.h shipped with
 * 9.0 is old, so it doesn't; hence this workaround.
 */
#ifdef MS_INCLUDE_REV
    (void) MS_INCLUDE_REV;	/* so we can ident the .o */
#endif
    
    handle_runstring(argc, argv);

    chat("$Header: translate.c,v 4.21 87/03/10 14:11:11 dah Exp $\n");

/*  While not eof, read the len of the record */
    while (fread ((char *) &recbuf[0], sizeof (int), 1, stdin) == 1) {

	rlen = recbuf[0];
	if (rlen >= MAX_REC_LEN) {
	    fprintf(stderr, "length %d\n", rlen);
	    bail_out("record too big");
	}

/*	read the rest of the record */
	cnt = fread ((char *) &recbuf[1], sizeof (int), rlen -1, stdin);
	if (cnt != rlen -1) {
		bail_out("fread: expected %d words, got %d", rlen -1, cnt);
	}
	id = recbuf[1];
	type = recbuf[2];

/* 	print len on stdout */
	printf("%d ",rlen);
	
/*	print id on stdout */
	printf("%d ",id);

/*	print type on stdout */
	printf("%d ",type);

	switch (type) {
	    case MS_GRAB_ID:
	    case MS_COMMENT:
	    case MS_ERROR:
	    case MS_GRAB_BIN_ID:
		recbuf[rlen] = 0;	/* ensure final nul */
	        printf("'%s'",&recbuf[3]);
		break;
	    case MS_PUT_TIME:
	    case MS_DATA:
	    case MS_POST_BIN:
	    case MS_DATA_V:
	    case MS_OVERRUN_ERR:
	    case MS_LOST_DATA:
	    case MS_LOST_BUFFER:
	    case MS_ADMIN_INFO:
	        for (i = 3; i < rlen-1; i++)	/* all but trailing len */
		    printf("%d ", recbuf[i]);
		printf("%d", recbuf[i++]);	/* but avoid trailing blank */
		break;
	    default:
	        fprintf(stderr, "unknown record type: %d\n", type);
	        for (i = 3; i < rlen-1; i++)	/* all but trailing len */
		    printf("%d ", recbuf[i]);
		printf("%d", recbuf[i++]);	/* but avoid trailing blank */
		/* break; */	/* commented to please lint */
	}

/* 	append newline */
	printf("\n");
    }
    clean_up(0);
}

handle_runstring(argc, argv)
    int argc;
    char **argv;
{
    int c;
    extern char *optarg;
    extern int optind;

    while ((c = getopt(argc, argv, "q")) != EOF) {
    	switch (c) {
	    case 'q':
	    	chatty = 0;
		break;
	    case '?':
	    	bail_out("Usage: translate -q");
    	    	break;
	    default:
	    	bail_out("getopt returned %c", c);
	}
    }
    if (argc > optind) {
	bail_out("extra arg: %s", argv[optind]);
    }
}

