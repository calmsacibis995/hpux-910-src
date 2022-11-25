static char *HPUX_ID = "@(#) $Revision: 64.1 $";
/* HPUX_ID: @(#) $Revision: 64.1 $  */
/*
    fpaload file device

    download file to floating point accelerator

    file = file to download
    device = target device file

    The format for the download file is:

	==============================
	| magic number               |
	------------------------------
	| download file size         |
	==============================
	| fpa offset 1               |
	------------------------------
	| fpa size 1 (in bytes)      |
	==============================
	|                            |
	|                            |
	|            DATA            |
	|                            |
	|                            |
	|                            |
	|                            |
	==============================
	| fpa offset 2               |
	------------------------------
	| fpa size 2 (in bytes)      |
	==============================
	|                            |
	|                            |
	|            DATA            |
	|                            |
	|                            |
	|                            |
	|                            |
	==============================
	|                            |
		       .
		       .
		       .

*/

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/iomap.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DRAGON_BASE	0x00800000
#define BOARD_READY	0x0000000C
#define REG_BANK	0x00000004
#define STATUS_REG	0x00040004
#define CONTROL_REG	0x00040008
#define INTERRUPT_REG	0x00000003
#define RESET		0x00000000
#define MAGIC_NUMBER	0x1234CDEF
#define SEQ_INIT	0x00042001
#define UCODE		0x00080000

extern  int  close(), fclose(), fprintf(), ioctl(), open(), stat();
extern  void exit(), perror();

static unsigned char *fpa_base;

typedef int BOOLEAN;
#define FALSE	0
#define TRUE	1

#define OFF 	0
#define ON 	1

#pragma OPT_LEVEL 1

main(argc, argv)
int argc;
char *argv[];
{
    BOOLEAN errflag, relflag;
    FILE *source;
    char *dst_name, *src_name;
    int *fpa_ptr;
    int count, destination, index, item, status;
    off_t fileptr, filesize;
    struct stat statbuffer;
    struct { int offset, size; } header;
    int reg_bank, status_reg, control_reg;
    int *p_reg_bank, *p_status_reg, *p_control_reg;
    unsigned char interrupt_reg, *p_interrupt_reg, *ptr;

    /*------------------------------------------------------------*/
    /*                        INITIALIZATION                      */
    /*------------------------------------------------------------*/

    /* Make sure board is present */
    asm("   tst.w flag_fpa");
    asm("   bne.b continue");
    asm("   pea   0");
    asm("   jsr   _exit");
    asm("continue:");

    status = 0;

    /* Check for correct number of arguments */
    if (argc != 3) {
        (void) fprintf(stderr, "Usage: fpaload file device\n");
        exit(1);
    };
    src_name = argv[1];
    dst_name = argv[2];

    /* Open the source file */
    if (stat(src_name, &statbuffer) < 0) {
        perror(src_name);
        exit(1);
    };
    filesize = statbuffer.st_size;
    if ((source = fopen(src_name, "r")) == NULL) {
        (void) fprintf(stderr, "%s: cannot open\n", src_name);
        exit(1);
    };

    /* Open the destination device */

    if ((destination = open(dst_name, O_RDWR)) < 0) {
        perror(dst_name);
        if (fclose(source) == EOF)
            (void) fprintf(stderr, "%s: cannot close\n", src_name);
        exit(1);
    };

    /* Base address of fpa */
    fpa_base = (unsigned char *) DRAGON_BASE;

    /* Map in fpa */
    if (ioctl(destination, IOMAPMAP, &fpa_base) < 0) {
        perror(dst_name);
        if (close(destination) < 0) perror(dst_name);
        if (fclose(source) == EOF)
            (void) fprintf(stderr, "%s: cannot close\n", src_name);
        exit(1);
    };

    /*------------------------------------------------------------*/
    /*                        DOWNLOAD                            */
    /*------------------------------------------------------------*/

    errflag = FALSE;
    fileptr = sizeof(header);
    if (fileptr <= filesize) {
        if (fread((char *) &header, sizeof(header), 1, source) == 1) {
	    if ((header.offset != MAGIC_NUMBER) ||
	      (header.size != filesize)) errflag = TRUE;
        } else errflag = TRUE; 
    } else errflag = TRUE; 

    /* disable board */
    fpa_ptr = (int *) (fpa_base + BOARD_READY);
    *fpa_ptr = OFF;

    p_reg_bank = (int *) (fpa_base + REG_BANK);
    reg_bank = *p_reg_bank;
    p_status_reg = (int *) (fpa_base + STATUS_REG);
    status_reg = *p_status_reg;
    p_control_reg = (int *) (fpa_base + CONTROL_REG);
    control_reg = *p_control_reg;
    p_interrupt_reg = (unsigned char *) (fpa_base + INTERRUPT_REG);
    interrupt_reg = *p_interrupt_reg;
    fpa_ptr = (int *)(fpa_base + UCODE);
    *fpa_ptr++ = 0x8000;
    *fpa_ptr++ = 0x0;
    *fpa_ptr++ = 0x8000;
    *fpa_ptr++ = 0x0;
    fpa_ptr = (int *) (fpa_base + RESET);
    *fpa_ptr = 0;

    /* load code */
    while (!errflag && (fileptr < filesize)) {

	/* get header */
	if (fread((char *) &header, sizeof(header), 1, source) != 1) {
	    errflag = TRUE; 
	    break; 
        };

	/* check for header errors */
	if (relflag = header.size & 0x80000000 ? 1 : 0) 
	    header.size &= 0x7FFFFFFF;
	fileptr += sizeof(header) + header.size;
	if ((header.offset % sizeof(int)) ||
	  (header.size % sizeof(int)) ||
	  (fileptr > filesize)) {
	    errflag = TRUE; 
	    break; 
        };

        /* transfer */
	fpa_ptr = (int *) (fpa_base + header.offset);
	count = header.size / sizeof(int);
	for (index = 0; index < count; index++) {
	    if (fread((char *) &item, sizeof(item), 1, source) != 1) {
	        errflag = TRUE; 
	        break; 
            };
	    if (relflag) item += (int) fpa_base;
	    *fpa_ptr++ = item;
        };
    };

    if (errflag) {
        (void) fprintf(stderr, "%s: download error\n", src_name);
        status = 1;
    } else {
	fpa_ptr = (int *) (fpa_base + RESET);
	*fpa_ptr = 0;
	*p_reg_bank = reg_bank;
	*p_status_reg = status_reg;
	*p_control_reg = control_reg;
	*p_interrupt_reg = interrupt_reg;
    };

    /* enable board */
    fpa_ptr = (int *) (fpa_base + BOARD_READY);
    *fpa_ptr = ON;

    /* execute one instruction to initialize sequencer */
    ptr = fpa_base + SEQ_INIT;
    *ptr = 0;

    /*------------------------------------------------------------*/
    /*                        CLEANUP                             */
    /*------------------------------------------------------------*/

    if (fclose(source) == EOF) {
        (void) fprintf(stderr, "%s: cannot close\n", src_name);
	status = 1;
    };

    (void) ioctl(destination, IOMAPUNMAP, 0);

    if (close(destination) < 0) {
	perror(dst_name);
	status = 1;
    };

    exit(status);
}

#pragma OPT_LEVEL 2
