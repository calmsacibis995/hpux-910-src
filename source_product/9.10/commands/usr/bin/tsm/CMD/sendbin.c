/* @(#) $Header: sendbin.c,v 70.1 92/03/09 15:39:03 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

/* sendbin.c -- simple xmodem file transmission program */

#define WAIT_BLOCK_ACK_NAK	15
#define WAIT_EOT_ACK_NAK	15

unsigned char soh = 0x01;
unsigned char eot = 0x04;
unsigned char ack = 0x06;
unsigned char nak = 0x15;
unsigned char can = 0x18;

#include "sys/types.h"
#include "stdio.h"
#include "signal.h"
#include "termio.h"
#include "sys/stat.h"


struct termio save;
struct termio new;
struct stat statbuf;

int lfproc = 0;
int textfile = 0;
int debug = 0;

FILE *errors;

extern int	alarmfn();
extern int	kleenex();

main(argc, argv)
int argc;
char **argv;
{
    unsigned char buf[130];
    unsigned char blocknum = 1;
    unsigned char c_blocknum = 254;
    unsigned char cksum;
    unsigned char inch;
    char file[128];



    int i,j;
    int numread;



    FILE *fp;

    /* debug++;            have a log file */

    if (strcmp(argv[0],"sendtext") == 0)
    {
#ifdef DONT_DO_THIS
            lfproc++;   /* text file */
#endif
	    textfile++;
    }

    if ( argc < 2 )
    {
        usage();
        exit( 1 );
    }
    if ( argc >= 3 )
	debug++;

    if (textfile) {
        if (debug) errors = fopen("sendtext.log", "w");
        if (debug && errors == NULL) {
            fprintf(stderr, "Unable to open sendtext.log for error logging\n");
            exit( 1 );
        }

    } else {
        if (debug) errors = fopen("send.log", "w");
        if (debug && errors == NULL) {
            fprintf(stderr, "Unable to open send.log for error logging\n");
            exit( 1 );
        }
    }

    if (debug) setbuf(errors, NULL);

    strcpy(file, argv[ 1 ]);

    if ((fp=fopen(file, "r")) == NULL) {
        fprintf(stderr, "File not found: %s\n", file);
        exit( 1 );
    }

    ioctl(1, TCGETA, &save);
    ioctl(1, TCGETA, &new);

    signal(14, alarmfn);
    signal(3, kleenex);
    signal(1, kleenex);

    stat(file, &statbuf);

    fprintf(stderr, "File '%s' ready for transmission, ", file);
    fprintf(stderr, "%d blocks\n", statbuf.st_size / 128 + 1);

    if (debug) {
        fprintf(errors, "File '%s' ready for transmission,\n", file);
        fprintf(errors, "%d blocks, mode is ", statbuf.st_size / 128 + 1);
        if (textfile) 
            fprintf(errors,"text\n");
        else       
            fprintf(errors,"binary\n");
    }
    fprintf(stderr, "Type control-X to exit\n");
    fflush(stdin);

    new.c_iflag = 0;
    new.c_oflag = 0;
    new.c_lflag &= ~(ICANON|ECHO);
    new.c_cc[4] = 1;
    new.c_cflag &= ~PARENB;
    new.c_cflag |= CS8;


    ioctl(1, TCSETAW, &new);

    while( 1 )
    {
	get_x_char(0, &buf[0]);
	if ( buf[0] == can )
		kleenex(0);
	if (buf[0] == nak)
		break;
    }

    numread = read_in(fp, buf, 128);
    while (numread > 0) {
        if (numread < 128) {
            for (i=numread; i< 128; i++) buf[i] = 0;
        }


        write(1, &soh, 1);
        write(1, &blocknum, 1);
        write(1, &c_blocknum, 1);
        write(1, buf, 128);

        cksum = 0;
        for (i=0;i<128;i++) cksum += buf[i];
        cksum = cksum % 256;
        write(1, &cksum, 1);

	while( 1 )
	{
		j = get_x_char( WAIT_BLOCK_ACK_NAK, &inch);
		if ( j >= 0 )
			break;
		if (debug) 
			fprintf(errors,   "Got a timeout after block %d\n",
				blocknum);
	}
        if ( inch == nak)
	{
            if (debug)
		fprintf(errors,   "Got a nak after block %d\n",blocknum);
            clear_iq();
            continue;
        }

        if ( inch == can)
	{
            if (debug) 
		fprintf(errors,   "Got a cancel after block %d\n",blocknum);
            kleenex(0);
        }

        if ( inch != ack)
	{
            if (debug) 
		fprintf(errors, 
			  "Got a non-ack after block %d, char was %o\n",
			blocknum,inch);
            clear_iq();
            continue;
        }


        numread = read_in(fp, buf, 128);
        blocknum = (blocknum + 1) % 256;
        c_blocknum = 255 - blocknum;

    }


    while (1) {
        write(1, &eot, 1);

        j = get_x_char( WAIT_EOT_ACK_NAK, &inch);
        if (j >= 0 && inch == nak) {
            if (debug) fprintf(errors,   "Got a nak after block %d\n",blocknum);
            clear_iq();
            continue;
        }

        if (j >= 0 && inch == can) {
            if (debug) fprintf(errors,   "Got a cancel during EOT\n");
            kleenex(0);
        }

        if (j >= 0 && inch != ack) {
            if (debug) 
		fprintf(errors,   
			  "Got a non-ack during EOT, char was %o\n",inch);
            clear_iq();
            continue;
        }

        if (j == -1) {
            if (debug) fprintf(errors, "Got a timeout during EOT\n");
            continue;
        }

        break;
    }
    kleenex(-1);
}


kleenex(x)
int x;
{
    ioctl(1, TCSETA, &save);
    if (x==-1) {
        sleep(8);
        fprintf(stderr,"\nTransfer complete\n");
        fflush(stderr);
        if (debug) fprintf(errors,"\nTransfer complete\n");
    }
    else {
        if (debug && x !=0) fprintf(errors, "Caught signal %d\n", x);
    }
    exit(0);
}


alarmfn(ignore)
	int	ignore;
{
    signal(14, alarmfn);

}


read_in(fp, buf, num)
FILE *fp;
unsigned char *buf;
int num;
{
    int i;
    int c;
    static int cr_held = 0;
    static eof_fts = 0;

    i = 0;

    if (cr_held) {
        buf[i] = '\n';
        i++;
        cr_held = 0;
    }

    for(   ;i < num;i++) {
        if (eof_fts == 1) return(i);
        c = getc(fp);

        if (c == EOF) {
            eof_fts = 1;
            c = 26; /* MS-DOS eot Character */
        }

        if (c == '\n' && lfproc) {
            buf[i] = '\r';
            if (i == 127) {
                cr_held = 1;
                return(128);
            }
            buf[i+1] = '\n';
            i++;
        }

        else {
            buf[i] = c;
        }
    }
    return(i);
}


clear_iq() {
    int j;
    unsigned char ch;

    signal(14, alarmfn);

    do {
        alarm(2);
        j = read(0, &ch, 1);
        alarm(0);
        if (debug && (j > -1)) fprintf(errors,   "cleared line of %o\n", ch);
    } while (j > -1);
}


get_x_char(timeout, cp)
int timeout;
unsigned char *cp;
{
    int ch;
    extern int errno;
    int ret;

    alarm(timeout);

    if ((ch=getchar()) == EOF) {
        /*if (debug) fprintf(errors, "get_x_char returning read error\n");*/
        /*if (debug) fprintf(errors, "Errno == %d\n", errno);*/
        return(-1);
    }

    alarm(0);
    *cp = ch;
    return( ch );
}


usage() 
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "sendtext filename [debug]\n");
    fprintf(stderr, "       to send a text file\n");
    fprintf(stderr, "sendbin filename [debug]\n");
    fprintf(stderr, "       to send a binary file\n");
}
