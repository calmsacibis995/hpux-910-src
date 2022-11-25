/* @(#) $Header: recvbin.c,v 70.1 92/03/09 15:38:54 ssa Exp $ */
#ifndef lint
static char rcsID[] = "@(#) $Revision: 70.1 $ Copyright (c) 1990 by SSSI";
#endif

/* recvbin.c -- simple xmodem file reception program */

#define WAIT_BLOCK_SOH		5
#define WAIT_BLOCK_BLOCKNUM	5
#define WAIT_BLOCK_CBLOCKNUM	5
#define WAIT_BLOCK_BODY		5
#define WAIT_BLOCK_CHECKSUM	5
#define WAIT_PURGE_SEND		2

unsigned char soh = 0x01;
unsigned char eot = 0x04;
unsigned char ack = 0x06;
unsigned char nak = 0x15;
unsigned char can = 0x18;

#include "sys/types.h"
#include "stdio.h"
#include "signal.h"
#include "termio.h"

struct termio save;
struct termio new;

int lfproc = 0;
int debug = 0;

FILE *errors;


int  linefd;
extern int	alarmfn();
extern int	quit();

main(argc, argv)
int argc;
char **argv;
{
    int i,j;
    unsigned char buf[130];
    unsigned char blocknum = 1;
    unsigned char c_blocknum = 254;
    char	  file[128];
    unsigned char cksum;
    unsigned char inch;
    unsigned char prev_blocknum = 200;
    unsigned char prev_c_blocknum = 200;
    unsigned char duplicate = 0;




    int numread;

    FILE *fp;


    /* debug++;             have a log file */

    if (strcmp(argv[0],"recvtext") == 0)
            lfproc++;   /* text file */

    if (argc < 2 ) {
        usage();
        exit( 1 );
    }
    if ( argc >= 3 )
	debug++;

    if (lfproc) {
        if (debug) errors = fopen("recvtext.log", "w");
        if (debug && errors == NULL) {
            fprintf(stderr, "Unable to open recvtext.log for error logging\n");
            exit( 1 );
        }

    } else {
        if (debug) errors = fopen("recvbin.log", "w");
        if (debug && errors == NULL) {
            fprintf(stderr, "Unable to open recvbin.log for error logging\n");
            exit( 1 );
        }
    }

    if (debug) setbuf(errors, NULL);

    strcpy(file, argv[ 1 ]);















    if ((fp=fopen(file, "w")) == NULL) {
        fprintf(stderr, "File not writeable: %s\n", file);
        exit( 1 );
    }

    linefd = 1;






    fprintf(stderr,"File '%s' ready for reception,\nmode is ", file);
    if (debug) fprintf(errors,"File '%s' ready for reception, mode is ", file);
    if (lfproc) {
        fprintf(stderr,"text\n");
        if (debug) fprintf(errors,"text\n");
    } else {
        fprintf(stderr, "binary\n");
        if (debug) fprintf(errors, "binary\n");
    }
    fprintf(stderr,"Type control-X to exit.\n");


    fflush(stdin);

    ioctl(linefd, TCGETA, &save);
    ioctl(linefd, TCGETA, &new);

    signal(3, quit);
    signal(2,quit);
    signal(1,quit);

    new.c_iflag = IGNBRK|IGNPAR;
    new.c_oflag = 0;
    new.c_lflag = 0;
    new.c_cc[4] = 1;
    new.c_cflag &= ~PARENB;
    new.c_cflag |= CS8;



    ioctl(linefd, TCSETAW, &new);

    sleep(5);

    signal(14, alarmfn);













    write(linefd, &nak, 1);
    while (1) {
TOP:


        i = get_x_char( WAIT_BLOCK_SOH, &inch);

        if (i == -1) {
            if (debug) 
		fprintf(errors,   "Timeout on block %d during SOH\n",blocknum);
            purge_send(nak);
            continue;
        }

        if (inch == can) {
            if (debug) 
		fprintf(errors,   "CAN received at block %d\n", blocknum);
            quit(0);
        }

        if (inch == eot) {
            if (debug) 
		fprintf(errors,   "EOT received at block %d\n", blocknum);
            break;
        }

        if (inch != soh) {
            if (debug) 
		fprintf(errors,   "Bad SOH on block %d\n",blocknum);
            purge_send(nak);
            continue;
        }

        i = get_x_char( WAIT_BLOCK_BLOCKNUM, &inch);
        if (i == -1 ) {
            if (debug) 
		fprintf(errors,   "Timeout on block %d during blocknum\n",
			blocknum);
            purge_send(nak);
            continue;
        }

        if (inch != blocknum) {
	    if ( inch == prev_blocknum )
	    {
		if (debug) 
			fprintf(errors, 
		      "Duplicate blocknum on block %d, expected %d, got %d\n",
				blocknum,blocknum,i);
		duplicate = 1;
	    }
	    else
	    {
		if (debug) 
			fprintf(errors, 
			  "Bad blocknum on block %d, expected %d, got %d\n",
				blocknum,blocknum,i);
		purge_send(nak);
		continue;
	    }
        }
	else
	    duplicate = 0;

        i = get_x_char( WAIT_BLOCK_CBLOCKNUM, &inch);
        if (i == -1 ) {
	    if (debug) 
		fprintf(errors,   "Timeout on block %d during c_blocknum\n",
			blocknum);
            purge_send(nak);
            continue;
        }

        if (inch != c_blocknum) 
	{
		if ( duplicate && ( inch == prev_c_blocknum ) )
		{
		}
		else
		{
			if (debug)
				fprintf(errors,
  "Bad c_blocknum on block %d, expected %d, got %d\n",
					blocknum,c_blocknum, i);
			purge_send(nak);
			continue;
		}
        }

	for (i=0;i<128;i++)
	{
            j = get_x_char( WAIT_BLOCK_BODY, &inch);
            if (j == -1)
	    {
                if (debug) 
			fprintf(errors, 
  "Timeout during block %d, during data recv, on char %d\n", 
				blocknum, i);
                purge_send(nak);
                goto TOP; /* AUGGH, a GOTO. horrendous */
            }
            buf[i] = inch;
        }

        cksum = 0;
	for (i=0;i<128;i++) 
	{
            cksum = cksum + buf[i];
            /*if (debug) 
		fprintf(errors,"Added %d to checksum, now %d\n",j,cksum);*/
        }
        cksum = cksum % 256;

        i = get_x_char( WAIT_BLOCK_CHECKSUM, &inch);
        if (i == -1 ) {
            if (debug) 
		fprintf(errors, 
			  "Timeout on block %d during checksum\n",blocknum);
            purge_send(nak);
            continue;
        }

        if (cksum != inch) {
            if (debug) 
		fprintf(errors, 
			  "Bad checksum on block %d, expected %d, got %d\n",
			blocknum,cksum,i);
            purge_send(nak);
            continue;
        }

        /* if (debug) fprintf(errors, "Validated block %d\n",blocknum); */

        write(linefd, &ack, 1);

        {	int iput;
		static int eot_flag = 0;
		iput=128;  /* assume full buffer */

		if (lfproc)
		{
			iput=0;    
			for (i=0;i<128;i++)
			{
				if (buf[i] == 26)
				{ /* MS-DOS eot */
					eot_flag = 1;  /* 1st stage */
					break;
				}

				if (buf[i] != 13) buf[iput++] = buf[i];
			}
		}

		if (eot_flag < 2)
		{
			if ( duplicate == 0 )
				fwrite(buf,iput,1,fp);
			if (eot_flag == 1) eot_flag=2; /* 2nd stage */
		}
	}
	if ( duplicate == 0 )
	{
		prev_blocknum = blocknum;
		prev_c_blocknum = c_blocknum;
		blocknum = (blocknum+1) % 256;
		c_blocknum = 255 - blocknum;

	}
    }

    write(linefd, &ack, 1);
    quit(-1);
}


quit(x)
int x;
{
    ioctl(linefd, TCSETAW, &save);
    if (x==-1) {
        sleep(5);
        fprintf(stderr,"\nTransfer complete\n");
        fflush(stderr);
        if (debug) fprintf(errors,"\nTransfer complete\n");
    }
    else {
	if (debug && x !=0)
	{
		fprintf(errors,   "Caught signal %d\n", x);



	}
    }
    exit(0);
}


alarmfn(ignore)
	int	ignore;
{
    signal(14, alarmfn);
}

#include "errno.h"
get_x_char(timeout, cp)
int timeout;
unsigned char *cp;
{
    int ch;
    extern int errno;
    int ret;
    unsigned char cc;
    int status;

    alarm(timeout);

    status = read( linefd, &cc, 1 );
    if ( status < 0 )
    {
	if ( errno != EINTR )
	{
		if ( debug )
			fprintf( errors, "\nread error\n" );
		quit( -1 );
	}
	return( -1 );
    }
    else if ( status == 0 )
    {
	if ( debug )
		fprintf( errors, "\nget_x_char status = 0\n" );
	quit( -1 );
    }
    ch = ( (int) cc ) & 0xFF;

    alarm(0);
    *cp = ch;
    return( ch );
}


purge_send(ch)
unsigned char ch;
{
    int j;
    unsigned char cp;
    /* if (debug) fprintf(errors,   "Begin purge_send(%d)\n", ch); */
    do {
        j = get_x_char( WAIT_PURGE_SEND, &cp);
    } while (j != -1);
    write(linefd, &ch, 1);
    /* if (debug) fprintf(errors,   "End purge_send(%d)\n", ch); */
}


usage()
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "recvtext filename [debug]\n");
    fprintf(stderr, "   to receive a text file\n");
    fprintf(stderr, "recvbin filename [debug]\n");
    fprintf(stderr, "   to receive a binary file\n");
}
