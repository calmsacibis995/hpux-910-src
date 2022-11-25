/*
/* Note:  Depending upon what is changing in a particular release, you
/*	  may not be able to compile mvtd using the Build Environment
/*	  compiler/linker because the resulting executible may only
/*	  run properly on the target system while you need to run mvtd
/*	  on the build system.  If you run into this problem copy 
/*	  a.out.h and sys/magic.h from the Build Environment to the
/*	  current directory and build mvtd (using the current system's
/*	  compiler):
/*
/*		cc -I. -o mvtd mvtd
*/


#include <stdio.h>
#include <a.out.h>
#include <fcntl.h>

#define BUFSIZE  1024

#ifdef hp9000s200
#define SYSTEM_ID HP9000S200_ID
#endif

struct magic s200magic = {
	SYSTEM_ID,
	EXEC_MAGIC
};

main(argc,argv)
    int argc;
    char *argv[];
{
    int infd,outfd;
    long roffset;
    long lseek();
    long temp;
    struct exec ebufin, ebufout;
    register char *buf;

    if (argc < 3) {
	fprintf(stderr,"mvtd: No object file specified.\n");
	exit(1);
    }

    /* Open input object file */

    if ((infd = open(argv[1],O_RDWR)) < 0) {
	fprintf(stderr,"mvtd: Could not open object file %s\n",argv[1]);
	exit(1);
    }

    /* Read a.out header */

    if (read(infd,&ebufin,sizeof (struct exec)) != sizeof (struct exec)) {
	fprintf(stderr,"mvtd: Could not read object header info.\n");
	exit(1);
    }

    if ( ebufin.a_magic.file_type != EXEC_MAGIC
      || ebufin.a_magic.system_id != SYSTEM_ID ) {
	fprintf(stderr,"mvtd: %s Bad Magic number.\n",argv[1]);
	exit(1);
    }

    /* Open output object file */

    if ((outfd = open(argv[2],O_RDWR)) < 0) {
	fprintf(stderr,"mvtd: Could not open object file %s\n",argv[2]);
	exit(1);
    }

    /* Read a.out header */

    if (read(outfd,&ebufout,sizeof (struct exec)) != sizeof (struct exec)) {
	fprintf(stderr,"mvtd: Could not read object header info.\n");
	exit(1);
    }

    if ( ebufout.a_magic.file_type != RELOC_MAGIC
      || ebufout.a_magic.system_id != SYSTEM_ID ) {
	fprintf(stderr,"mvtd: %s Bad Magic number.\n",argv[2]);
	exit(1);
    }

    /* check to make sure the text and data segments are the same size */

    if (ebufin.a_text != ebufout.a_text)
    {	fprintf(stderr,"mvtd: text not the same size\n");
	fprintf(stderr,"SYSTEXT:\t%d\nSYSDEBUG:\t%d\n", ebufin.a_text, ebufout.a_text);
    }

    if (ebufin.a_data != ebufout.a_data)
    {	fprintf(stderr,"mvtd: data not the same size\n");
	fprintf(stderr,"SYSTEXT:\t%d\nSYSDEBUG:\t%d\n", ebufin.a_data, ebufout.a_data);
    }

    if ((ebufin.a_text != ebufout.a_text) ||
	(ebufin.a_data != ebufout.a_data))
    {	fprintf(stderr,"mvtd: text or data not the same size\n");
	exit(1);
    }

    /* Seek to beginning of text segment */

    roffset = TEXT_OFFSET(ebufin);
    if (lseek(infd,roffset,0) != roffset) {
	fprintf(stderr,"mvtd: lseek error on %s\n",argv[1]);
	exit(1);
    }

    /* get space for the text segment and read it in */

    temp = ebufin.a_text;
    buf = (char *) malloc(temp);
    read(infd,buf,temp);

    /* Seek to beginning of text segment and write text out*/

    if (lseek(outfd,roffset,0) != roffset) {
	fprintf(stderr,"mvtd: lseek error on %s\n",argv[1]);
	exit(1);
    }
    write(outfd,buf,temp);
    free(buf);

    /* Seek to beginning of data segment */

    roffset = DATA_OFFSET(ebufin);
    if (lseek(infd,roffset,0) != roffset) {
	fprintf(stderr,"mvtd: lseek error on %s\n",argv[1]);
	exit(1);
    }

    /* get space for the data segment and read it in */

    temp = ebufin.a_data;
    buf = (char *) malloc(temp);
    read(infd,buf,temp);

    /* Seek to beginning of data segment and write text out*/

    if (lseek(outfd,roffset,0) != roffset) {
	fprintf(stderr,"mvtd: lseek error on %s\n",argv[1]);
	exit(1);
    }

    write(outfd,buf,temp);
    free(buf);

    /* finally, write an exec_magic value out to the output file */

    if (lseek(outfd,0,0) != 0) {
	fprintf(stderr,"mvtd: lseek error on %s\n",argv[1]);
	exit(1);
    }
    write(outfd,&s200magic,sizeof s200magic);

    exit(0);
}
