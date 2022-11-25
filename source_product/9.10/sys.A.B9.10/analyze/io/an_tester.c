/***************************************************************************/
/**                                                                       **/
/**   This is for testing new analyze functionality                       **/
/**                                                                       **/
/***************************************************************************/

#include <stdio.h>
#include <sio/llio.h>
#include <h/types.h>
#include <sio/iotree.h>
#include "aio.h"			/* all analyze i/o defs		      */

int an_tester (call_type, port_num, out_fd, option_string)

    int			call_type;	/* type of actions to perform	      */
    port_num_type	port_num;	/* manager port number		      */
    FILE		*out_fd;	/* file descriptor of output file     */
    char		*option_string;	/* driver-specific options user typed */

{
    int			an_tester_decode_message();
    int			an_tester_decode_status();


    /* perform the correct action depending on what call_type is */
    switch (call_type)  {

	case AN_MGR_INIT:
	    break;

	case AN_MGR_BASIC:
	    an_tester_all(out_fd);
	    break;

	case AN_MGR_DETAIL:
	    an_tester_all(out_fd);
	    break;

	case AN_MGR_OPTIONAL:
	    an_tester_optional(out_fd,option_string);
	    break;

	case AN_MGR_HELP:
	    an_tester_help(out_fd);
	    break;

    }

    return(0);

}

an_tester_optional (out_fd, option_string)

    FILE		*out_fd;
    char		*option_string;

{
    char 		*token;
    int			all_flag=0;
    int			chunk_flag=0;
    int			display_flag=0;
    int			dump_flag=0;
    int			em_fe_flag=0;
    int			extern_flag=0;
    int			iotree_flag=0;
    int			vtor_flag=0;
    int			message_flag=0;
    int			mgr_info_flag=0;
    int			mgr_name_flag=0;
    int			mgr_table_flag=0;
    int			queued_flag=0;
    int			rev_flag=0;
    int			status_flag=0;

    /* read through option string and set appropriate flags */
    if ( (token = strtok(option_string," ")) == NULL)  {
	an_tester_help(out_fd);
	return(0);
    }

    do  {

	if (strcmp(token,"all") == 0)              all_flag = 1;
	else if (strcmp(token,"chunk") == 0)       chunk_flag = 1;
	else if (strcmp(token,"display") == 0)     display_flag = 1;
	else if (strcmp(token,"dump") == 0)        dump_flag = 1;
	else if (strcmp(token,"em_fe") == 0)       em_fe_flag = 1;
	else if (strcmp(token,"extern") == 0)      extern_flag = 1;
	else if (strcmp(token,"iotree") == 0)      iotree_flag = 1;
	else if (strcmp(token,"vtor") == 0)        vtor_flag = 1;
	else if (strcmp(token,"message") == 0)     message_flag = 1;
	else if (strcmp(token,"mgr_info") == 0)    mgr_info_flag = 1;
	else if (strcmp(token,"mgr_name") == 0)    mgr_name_flag = 1;
	else if (strcmp(token,"mgr_table") == 0)   mgr_table_flag = 1;
	else if (strcmp(token,"queued") == 0)      queued_flag = 1;
	else if (strcmp(token,"rev") == 0)         rev_flag = 1;
	else if (strcmp(token,"status") == 0)      status_flag = 1;
	else  {
	    fprintf(stderr,"invalid option <%s>\n",token);
	    an_tester_help(out_fd);
	    return(0);
	}

    } while ( (token = strtok(NULL," ")) != NULL);

    /* now process the flags that were set */
    if (all_flag)         an_tester_all(out_fd);
    if (chunk_flag)       an_tester_chunk(out_fd);
    if (display_flag)     an_tester_display(out_fd);
    if (dump_flag)        an_tester_dump(out_fd);
    if (em_fe_flag)       an_tester_em_fe(out_fd);
    if (extern_flag)      an_tester_extern(out_fd);
    if (iotree_flag)      an_tester_iotree(out_fd);
    if (vtor_flag)        an_tester_vtor(out_fd);
    if (message_flag)     an_tester_message(out_fd);
    if (mgr_info_flag)    an_tester_mgr_info(out_fd);
    if (mgr_name_flag)    an_tester_mgr_name(out_fd);
    if (mgr_table_flag)   an_tester_mgr_table(out_fd);
    if (queued_flag)      an_tester_queued(out_fd);
    if (rev_flag)         an_tester_rev(out_fd);
    if (status_flag)      an_tester_status(out_fd);

}


an_tester_all (out_fd)

    FILE		*out_fd;

{
    an_tester_chunk(out_fd);
    an_tester_display(out_fd);
    an_tester_dump(out_fd);
    an_tester_em_fe(out_fd);
    an_tester_extern(out_fd);
    an_tester_iotree(out_fd);
    an_tester_vtor(out_fd);
    an_tester_message(out_fd);
    an_tester_mgr_info(out_fd);
    an_tester_mgr_name(out_fd);
    an_tester_mgr_table(out_fd);
    an_tester_queued(out_fd);
    an_tester_rev(out_fd);
    an_tester_status(out_fd);
}

an_tester_decode_status (llio_status, out_fd)

    llio_status_type	llio_status;
    FILE		*out_fd;

{
    fprintf(out_fd,"tester llio_status = <0x%08x> means ",llio_status);

    switch (llio_status.u.error_num)  {

	case 1:
	    fprintf(out_fd,"status_1\n");
	    break;

	case 2:
	    fprintf(out_fd,"status_2\n");
	    break;

	default:
	    fprintf(out_fd,"NOTHING! -- can't decode!\n");

    }
}

an_tester_decode_message (msg, prefix, out_fd)

    io_message_type		*msg;
    char			*prefix;
    FILE			*out_fd;

{
    fprintf(out_fd,"tester message decoding: message_ptr = <0x%08x>\n",(int)msg);
    fprintf(out_fd,"  not implemented yet\n");
}

an_tester_help (out_fd)

    FILE		*out_fd;

{
    fprintf(out_fd,"\nvalid options:\n");
    fprintf(out_fd,"   chunk       --  test an_grab_xxx_chunk routines\n");
    fprintf(out_fd,"   display     --  test an_display_sym\n");
    fprintf(out_fd,"   dump        --  test an_dump_hex_ascii\n");
    fprintf(out_fd,"   em_fe       --  test aio_em_fe\n");
    fprintf(out_fd,"   extern      --  test an_grab_extern\n");
    fprintf(out_fd,"   iotree      --  test aio_get_iotree_entry\n");
    fprintf(out_fd,"   message     --  test message decoding\n");
    fprintf(out_fd,"   mgr_info    --  test aio_get_mgr_info\n");
    fprintf(out_fd,"   mgr_name    --  test aio_get_mgr_name\n");
    fprintf(out_fd,"   mgr_table   --  test aio_get_mgr_table_entry\n");
    fprintf(out_fd,"   queued      --  test aio_queued_message\n");
    fprintf(out_fd,"   rev         --  test aio_rev_mismatch\n");
    fprintf(out_fd,"   status      --  test status decoding\n");
    fprintf(out_fd,"   vtor        --  test an_vtor\n");
    fprintf(out_fd,"\n");
    fprintf(out_fd,"   all         --  do all of the above\n");
    fprintf(out_fd,"   help        --  print this help screen\n");
}

an_tester_chunk (out_fd)

    FILE		*out_fd;

{
    char		buf1[100];
    char		buf2[100];
    unsigned		phys;
    int			i;

    fprintf(out_fd,"\n<<<< Testing an_grab_***_chunk routines >>>>\n\n");


    fprintf(out_fd,"TEST 1 -- good grabs from each rtn and compare\n");
    fprintf(out_fd,"grab 100 bytes from 0.0x10000 using an_grab_virt_chunk\n");
    if (an_grab_virt_chunk(0,0x10000,buf1,100) != 0)
	fprintf(out_fd,"    virtual address not mapped\n");
    else
	fprintf(out_fd,"    successful\n");
    fprintf(out_fd,"\n");


    fprintf(out_fd,"do vtor on 0.0x10000 -- result is ");
    if (an_vtor(0,0x10000,&phys) != 0) 
	fprintf(out_fd,"    virtual address not mapped\n");
    else  {
	fprintf(out_fd,"0x%08x\n",phys);
	fprintf(out_fd,"now grab 100 bytes from 0x%08x using an_grab_real_chunk\n",phys);
	if (an_grab_real_chunk(phys,buf2,100) != 0)
	    fprintf(out_fd,"    read from core failed\n");
	else {
	    fprintf(out_fd,"    successful\n");
	    fprintf(out_fd,"now compare the two buffers\n");
	    for (i=0 ; i<100 ; i++)
		if (buf1[i] != buf2[i]) {
		    fprintf(out_fd,"     miscompare!!!\n");
		    break;
		}
	    if (i == 100)
		fprintf(out_fd,"    successful\n");

	}
	fprintf(out_fd,"\n");
    }


    fprintf(out_fd,"TEST 2 -- grabs from a bad (maybe) virtual address\n");
    fprintf(out_fd,"grab 100 bytes from 17.0x10000 using an_grab_virt_chunk\n");
    if (an_grab_virt_chunk(17,0x10000,buf1,100) != 0)
	fprintf(out_fd,"    virtual address not mapped\n");
    else
	fprintf(out_fd,"    successful\n");
    fprintf(out_fd,"\n");


    fprintf(out_fd,"do vtor on 17.0x10000 -- result is ");
    if (an_vtor(17,0x10000,&phys) != 0) 
	fprintf(out_fd,"    virtual address not mapped\n");
    else  {
	fprintf(out_fd,"0x%08x\n",phys);
	fprintf(out_fd,"now grab 100 bytes from 0x%08x using an_grab_real_chunk\n",phys);
	if (an_grab_real_chunk(phys,buf2,100) != 0)
	    fprintf(out_fd,"    read from core failed\n");
	else {
	    fprintf(out_fd,"    successful\n");
	    fprintf(out_fd,"now compare the two buffers\n");
	    for (i=0 ; i<100 ; i++)
		if (buf1[i] != buf2[i]) {
		    fprintf(out_fd,"     miscompare!!!\n");
		    break;
		}
	    if (i == 100)
		fprintf(out_fd,"    successful\n");

	}
	fprintf(out_fd,"\n");
    }


    fprintf(out_fd,"TEST 3 -- grab from a bad (maybe) real address\n");
    fprintf(out_fd,"grab 100 bytes from 0x3000000 using an_grab_real_chunk\n");
    if (an_grab_real_chunk(0x30000000,buf2,100) != 0)
	fprintf(out_fd,"    read from core failed\n");
    else
	fprintf(out_fd,"    successful\n");
    fprintf(out_fd,"\n");

}

an_tester_display (out_fd)

    FILE		*out_fd;

{
    unsigned		addr;
    extern char		*cursym;


    fprintf(out_fd,"\n<<<< Testing an_display_sym routine >>>>\n\n");

    /* the verification of these calls depends on the global cursym */

    fprintf(out_fd,"absolute address 0x00000700 is  ");
    an_display_sym(0x0700,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x0000a000 is  ");
    an_display_sym(0xa000,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x0000a004 is  ");
    an_display_sym(0xa004,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x0000a007 is  ");
    an_display_sym(0xa007,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x0000a009 is  ");
    an_display_sym(0xa009,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x00013009 is  ");
    an_display_sym(0x13009,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x00027009 is  ");
    an_display_sym(0x27009,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x0012a000 is  ");
    an_display_sym(0x12a000,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x0112a000 is  ");
    an_display_sym(0x112a000,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);


    fprintf(out_fd,"absolute address 0x1112a000 is  ");
    an_display_sym(0x1112a000,out_fd,AIO_SYMBOL_AND_OFFSET);
    fprintf(out_fd,"\n");
    an_grab_extern(cursym,&addr);
    fprintf(out_fd,"    %s value is 0x%08x\n\n",cursym,addr);

}

an_tester_dump (out_fd)

    FILE		*out_fd;

{
    int			i;
    char		buf[200];


    fprintf(out_fd,"\n<<<< Testing an_dump_hex routine >>>>\n\n");


    fprintf(out_fd,"for reference -- grab a 128 byte chunk from address\n");
    fprintf(out_fd,"   0.0x10000 using an_grab_virt_chunk:\n");
    if (an_grab_virt_chunk(0,0x10000,buf,128) != 0)  {
	fprintf(out_fd,"can't get the buffer!!!\n");
	return;
    }
    for (i=0 ; i<128 ; i++)  {
	if ( (i % 16) == 0)
	    fprintf(out_fd,"\n         ");
  	fprintf(out_fd,"%02x ",(bit8)buf[i]);
    }
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 100 bytes from 0.0x10000:\n\n");
    if (an_dump_hex_ascii(0,0x10000,100,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 101 bytes from 0.0x10000:\n\n");
    if (an_dump_hex_ascii(0,0x10000,101,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 102 bytes from 0.0x10000:\n\n");
    if (an_dump_hex_ascii(0,0x10000,102,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 103 bytes from 0.0x10000:\n\n");
    if (an_dump_hex_ascii(0,0x10000,103,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 100 bytes from 0.0x10001:\n\n");
    if (an_dump_hex_ascii(0,0x10001,100,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 100 bytes from 0.0x10002:\n\n");
    if (an_dump_hex_ascii(0,0x10002,100,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 100 bytes from 0.0x10003:\n\n");
    if (an_dump_hex_ascii(0,0x10003,100,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 40001 bytes from 0.0x10000:\n\n");
    if (an_dump_hex_ascii(0,0x10000,40001,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");


    fprintf(out_fd,"an_dump_hex_ascii -- dump 100 bytes from 1023.0x1246:\n\n");
    if (an_dump_hex_ascii(1023,0x1246,100,out_fd) != AIO_OK)
	fprintf(out_fd,"    problem -- too big!");
    fprintf(out_fd,"\n\n");

}

an_tester_em_fe (out_fd)

    FILE		*out_fd;

{
    unsigned		addr;
    int			in_em_fe;


    fprintf(out_fd,"\n<<<< Testing aio_em_fe routine >>>>\n\n");


    fprintf(out_fd,"Test with a memory address -- 0x12345678:\n");
    if (aio_em_fe(0x12345678,&in_em_fe) != AIO_OK)
	fprintf(out_fd,"    bad address\n\n");

    fprintf(out_fd,"Now test a bunch of i/o pages:\n");
    for (addr=0xfff78000 ; addr<0xffffc001 ; addr+=0x800)  {
	fprintf(out_fd,"    address 0x%08x -- ",addr);
	if (aio_em_fe(addr,&in_em_fe) != AIO_OK)
	    fprintf(out_fd,"bad address -- no such page\n");
	else if (in_em_fe)
	    fprintf(out_fd,"in emulated fe mode\n");
	else
	    fprintf(out_fd,"not in emulated fe mode\n");
    }

}

an_tester_extern (out_fd)

    FILE		*out_fd;

{
    unsigned		addr;
    int			val;


    fprintf(out_fd,"\n<<<< Testing an_grab_extern routine >>>>\n\n");


    fprintf(out_fd,"grab num_cio_ca0\n");
    if (an_grab_extern("num_cio_ca0",&addr) != 0)
	fprintf(out_fd,"symbol not found!\n");
    else
	if (an_grab_real_chunk(addr,&val,4) != 0)
	    fprintf(out_fd,"got symbol (address 0x%08x) can't get contents!\n",
		    (int)addr);
	else
	    fprintf(out_fd,"got symbol (address 0x%08x) contents is %d\n",
		    (int)addr,val);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"grab max_num_ports\n");
    if (an_grab_extern("max_num_ports",&addr) != 0)
	fprintf(out_fd,"symbol not found!\n");
    else
	if (an_grab_real_chunk(addr,&val,4) != 0)
	    fprintf(out_fd,"got symbol (address 0x%08x) can't get contents!\n",
		    (int)addr);
	else
	    fprintf(out_fd,"got symbol (address 0x%08x) contents is %d\n",
		    (int)addr,val);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"grab probably_not_there\n");
    if (an_grab_extern("probably_not_there",&addr) != 0)
	fprintf(out_fd,"symbol not found!\n");
    else
	if (an_grab_real_chunk(addr,&val,4) != 0)
	    fprintf(out_fd,"got symbol (address 0x%08x) can't get contents!\n",
		    (int)addr);
	else
	    fprintf(out_fd,"got symbol (address 0x%08x) contents is %d\n",
		    (int)addr,val);
    fprintf(out_fd,"\n");

}

an_tester_iotree (out_fd)

    FILE		*out_fd;

{
    int				i;
    unsigned			addr;
    struct iotree_type    	entry;
    char			name[MAX_ID];


    fprintf(out_fd,"\n<<<< Testing aio_get_iotree_entry routine >>>>\n\n");


    fprintf(out_fd,"display some entries:\n\n");

    for (i=-2 ; i<45 ; i++)

	if (aio_get_iotree_entry(i,&entry) != AIO_OK)
	    fprintf(out_fd,"iotree entry %d does not exist\n\n",i);

	else  {

	    fprintf(out_fd,"iotree entry %d:\n",i);

	    fprintf(out_fd,"    parent           = %d\n",entry.parent);
	    fprintf(out_fd,"    sibling          = %d\n",entry.sibling);
	    fprintf(out_fd,"    child            = %d\n",entry.child);
	    fprintf(out_fd,"    state            = %d\n",entry.state);
	    fprintf(out_fd,"    parm             = %d\n",entry.parm);
	    fprintf(out_fd,"    lu               = %d\n",entry.lu);
	    fprintf(out_fd,"    port             = %d\n",entry.port);
	    fprintf(out_fd,"    mgr_table index  = %d\n",entry.mgr);

	    if (entry.module == 0)
		fprintf(out_fd,"    iotree_names ndx = %d \n\n",entry.module);
	    else  {
		if (an_grab_extern("iotree_names",&addr) != 0)
		    fprintf(out_fd,"Problem with this index!\n");
		if (an_grab_real_chunk(addr+(entry.module*MAX_ID),name,MAX_ID) != 0)
		    fprintf(out_fd,"Problem with this index!\n");
		fprintf(out_fd,"    iotree_names ndx = %d (%s)\n\n",entry.module,name);
	    }


	}
}

an_tester_vtor (out_fd)

    FILE		*out_fd;

{
    unsigned		phys;


    fprintf(out_fd,"\n<<<< Testing an_vtor routine >>>>\n\n");


    fprintf(out_fd,"Virtual address 0.0x0000d800 is ");
    if (an_vtor(0,0x0000d800,&phys) != 0)
	fprintf(out_fd,"not translatable!\n");
    else
	fprintf(out_fd,"physical address 0x%08x\n",(int)phys);


    fprintf(out_fd,"Virtual address 0.0x00102000 is ");
    if (an_vtor(0,0x00102000,&phys) != 0)
	fprintf(out_fd,"not translatable!\n");
    else
	fprintf(out_fd,"physical address 0x%08x\n",(int)phys);


    fprintf(out_fd,"Virtual address 0.0x01102000 is ");
    if (an_vtor(0,0x01102000,&phys) != 0)
	fprintf(out_fd,"not translatable!\n");
    else
	fprintf(out_fd,"physical address 0x%08x\n",(int)phys);


    fprintf(out_fd,"Virtual address 7.0x0000d800 is ");
    if (an_vtor(7,0x0000d800,&phys) != 0)
	fprintf(out_fd,"not translatable!\n");
    else
	fprintf(out_fd,"physical address 0x%08x\n",(int)phys);


    fprintf(out_fd,"Virtual address 14.0x0000d800 is ");
    if (an_vtor(14,0x0000d800,&phys) != 0)
	fprintf(out_fd,"not translatable!\n");
    else
	fprintf(out_fd,"physical address 0x%08x\n",(int)phys);
}

an_tester_message (out_fd)

    FILE		*out_fd;

{

    fprintf(out_fd,"\n<<<< Testing aio_init_message_decode routine >>>>\n\n");

    fprintf(out_fd,"Case 1: Normal Initialization  low=1280  high=1289\n");
    if (aio_init_message_decode(1280,1289,an_tester_decode_message) != AIO_OK)
	fprintf(out_fd,"  Initialization failed\n\n");
    else
	fprintf(out_fd,"  Initialization succeeded\n\n");

    fprintf(out_fd,"Case 2: Overlapping range      low=1270  high=1289\n");
    if (aio_init_message_decode(1280,1289,an_tester_decode_message) != AIO_OK)
	fprintf(out_fd,"  Initialization failed\n\n");
    else
	fprintf(out_fd,"  Initialization succeeded\n\n");

    fprintf(out_fd,"Case 3: Overlapping range      low=1280  high=1292\n");
    if (aio_init_message_decode(1280,1289,an_tester_decode_message) != AIO_OK)
	fprintf(out_fd,"  Initialization failed\n\n");
    else
	fprintf(out_fd,"  Initialization succeeded\n\n");

    fprintf(out_fd,"Case 4: Overlapping range      low=1282  high=1284\n");
    if (aio_init_message_decode(1280,1289,an_tester_decode_message) != AIO_OK)
	fprintf(out_fd,"  Initialization failed\n\n");
    else
	fprintf(out_fd,"  Initialization succeeded\n\n");

    fprintf(out_fd,"Case 5: Backward range         low=1289  high=1270\n");
    if (aio_init_message_decode(1280,1289,an_tester_decode_message) != AIO_OK)
	fprintf(out_fd,"  Initialization failed\n\n");
    else
	fprintf(out_fd,"  Initialization succeeded\n\n");

}

an_tester_mgr_info (out_fd)

    FILE			*out_fd;

{

    int				i;
    struct mgr_info_type	mgr;


    fprintf(out_fd,"\n<<<< Testing aio_get_mgr_info routine -- ports  >>>>\n\n");

    for (i=-2 ; i < 40 ; i++)

	if (aio_get_mgr_info(AIO_PORT_NUM_TYPE,i,&mgr) != AIO_OK)
	    fprintf(out_fd,"port %d does not exist\n\n",i);

	else  {

	    fprintf(out_fd,"manager info table for port %d:\n",i);
	    fprintf(out_fd,"    name           = %s\n",mgr.mgr_name);
	    fprintf(out_fd,"    hw address     = %s\n",mgr.hw_address);
	    fprintf(out_fd,"    pda address    = 0x%08x\n",mgr.pda_address);
	    fprintf(out_fd,"    enabled subqs  = 0x%08x\n",mgr.enabled_subqs);
	    fprintf(out_fd,"    active subqs   = 0x%08x\n",mgr.active_subqs);
	    fprintf(out_fd,"    iotree index   = %d\n",mgr.iotree_index);
	    fprintf(out_fd,"    next iotree ndx= %d\n",mgr.next_iotree_index);
	    fprintf(out_fd,"    mgr table index= %d\n",mgr.mgr_table_index);
	    fprintf(out_fd,"    blocked        = %d\n",mgr.blocked);
	    fprintf(out_fd,"    eim            = %d\n",mgr.eim);
	    fprintf(out_fd,"    in poll list   = %d\n\n",mgr.in_poll_list);

	}


    fprintf(out_fd,"\n<<<< Testing aio_get_mgr_info routine -- iotree  >>>>\n\n");

    for (i=-2 ; i < 50 ; i++)

	if (aio_get_mgr_info(AIO_IOTREE_TYPE,i,&mgr) != AIO_OK)
	    fprintf(out_fd,"iotree entry %d does not exist\n\n",i);

	else  {

	    fprintf(out_fd,"manager info table for iotree index %d:\n",i);
	    fprintf(out_fd,"    name           = %s\n",mgr.mgr_name);
	    fprintf(out_fd,"    hw address     = %s\n",mgr.hw_address);
	    fprintf(out_fd,"    pda address    = 0x%08x\n",mgr.pda_address);
	    fprintf(out_fd,"    enabled subqs  = 0x%08x\n",mgr.enabled_subqs);
	    fprintf(out_fd,"    active subqs   = 0x%08x\n",mgr.active_subqs);
	    fprintf(out_fd,"    iotree index   = %d\n",mgr.iotree_index);
	    fprintf(out_fd,"    next iotree ndx= %d\n",mgr.next_iotree_index);
	    fprintf(out_fd,"    mgr table index= %d\n",mgr.mgr_table_index);
	    fprintf(out_fd,"    blocked        = %d\n",mgr.blocked);
	    fprintf(out_fd,"    eim            = %d\n",mgr.eim);
	    fprintf(out_fd,"    in poll list   = %d\n\n",mgr.in_poll_list);

	}
}

an_tester_mgr_table (out_fd)

    FILE			*out_fd;

{
    int				i;
    unsigned			addr;
    struct mgr_table_type 	entry;
    char			name[MAX_ID];


    fprintf(out_fd,"\n<<<< Testing aio_get_mgr_table_entry routine >>>>\n\n");


    fprintf(out_fd,"display some entries\n\n");

    for (i=-2 ; i<40 ; i++)

	if (aio_get_mgr_table_entry(i,&entry) != AIO_OK)
	    fprintf(out_fd,"manager table entry %d does not exist\n\n",i);

	else  {

	    fprintf(out_fd,"manager table entry %d:\n",i);

	    if (an_grab_extern("iotree_names",&addr) != 0)
		fprintf(out_fd,"Problem with this index!\n");
	    if (an_grab_real_chunk(addr+(entry.driver*MAX_ID),name,MAX_ID) != 0)
		fprintf(out_fd,"Problem with this index!\n");
	    fprintf(out_fd,"    driver name      = %s\n",name);

	    fprintf(out_fd,"    port server rtn  = ");
	    if ((unsigned)entry.entry == 0)
		fprintf(out_fd,"none");
	    else
		an_display_sym((unsigned)entry.entry,out_fd);
	    fprintf(out_fd,"\n");

	    fprintf(out_fd,"    attach rtn       = ");
	    if ((unsigned)entry.attach == 0)
		fprintf(out_fd,"none");
	    else
		an_display_sym((unsigned)entry.attach,out_fd);
	    fprintf(out_fd,"\n");

	    fprintf(out_fd,"    init rtn         = ");
	    if ((unsigned)entry.init == 0)
		fprintf(out_fd,"none");
	    else
		an_display_sym((unsigned)entry.init,out_fd);
	    fprintf(out_fd,"\n");

	    fprintf(out_fd,"    ldm flag         = %d\n",entry.ldm);
	    fprintf(out_fd,"    bdev number      = %d\n",entry.b_major);
	    fprintf(out_fd,"    cdev number      = %d\n",entry.c_major);
	    fprintf(out_fd,"    state            = %d\n\n",entry.state);
	}

}

an_tester_mgr_name (out_fd)

    FILE		*out_fd;

{
    int			i;
    char		name[16];


    fprintf(out_fd,"\n<<<< Testing aio_get_mgr_name routine >>>>\n\n");


    for (i=-5 ; i<40 ; i++)
	if (aio_get_mgr_name(i,name) != AIO_OK)
	    fprintf(out_fd,"Nothing at port %2d\n",i);
 	else
	    fprintf(out_fd,"Manager at port %2d is %s\n",i,name);

}

an_tester_queued (out_fd)

    FILE		*out_fd;

{
    int				i;
    int				j;
    int				k;
    struct mgr_info_type	mgr;
    int 			empty_flag=0;
    unsigned       		msg;


    fprintf(out_fd,"\n<<<< Testing aio_queued_message routine >>>>\n\n");


    fprintf(out_fd,"Bad subq number <-2>:\n");
    if (aio_queued_message(4,-2,1,&msg) != AIO_OK)
	fprintf(out_fd,"  invalid subq number\n\n");

    fprintf(out_fd,"Bad subq number <34>:\n");
    if (aio_queued_message(4,34,1,&msg) != AIO_OK)
	fprintf(out_fd,"  invalid subq number\n\n");

    fprintf(out_fd,"Bad port number <-2>:\n");
    if (aio_queued_message(-2,4,1,&msg) != AIO_OK)
	fprintf(out_fd,"  invalid port number\n\n");


    for (i=1; i < 40 ; i++)

	if (aio_get_mgr_info(AIO_PORT_NUM_TYPE,i,&mgr) == AIO_OK)

	    /* if any managers have queued messages, print them */
	    if (mgr.active_subqs)  {
		fprintf(out_fd,"Should have message(s) queued on port %d:\n",i);
		for (k=0 ; k<32 ; k++)
		    if (mgr.active_subqs & ELEMENT_OF_32(k)) {
			fprintf(out_fd,"   subq %2d:\n",k);
			for (j=0 ; j<15 ; j++)  {
			    if (aio_queued_message(mgr.port_num,k,j,&msg) != AIO_OK)
				break;
			    aio_decode_message(msg,"        ",out_fd);
			    fprintf(out_fd,"\n");
			}
		    }
	    }

	    /* otherwise, try an empty subq (if not already tried) */
	    else if (empty_flag == 0)  {
		empty_flag = 1;
		fprintf(out_fd,"Shouldn't have messages queued on port %d:\n",i);
		for (k=0 ; k<32 ; k++)
		    if (mgr.active_subqs & ELEMENT_OF_32(k)) {
			fprintf(out_fd,"   subq %2d:\n",k);
			for (j=0 ; j<15 ; j++)  {
			    if (aio_queued_message(mgr.port_num,k,j,&msg) != AIO_OK)
				break;
			    aio_decode_message(msg,"        ",out_fd);
			}
		    }
	    }


}

an_tester_rev (out_fd)

    FILE		*out_fd;

{
    fprintf(out_fd,"\n<<<< Testing aio_rev_mismatch routine >>>>\n\n");


    fprintf(out_fd,"case 1 -- bad value for <what_kind>\n");
    if (aio_rev_mismatch("case1",7,11,12,out_fd) != AIO_OK)  {
	fprintf(out_fd,"!!! error detected !!!\n");
    }
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 2 -- AIO_NO_REV_INFO\n");
    if (aio_rev_mismatch("case2",AIO_NO_REV_INFO,11,12,out_fd) != AIO_OK)  {
	fprintf(out_fd,"!!! error detected !!!\n");
    }
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 3 -- AIO_INCOMPAT_REVS -- dvr older than analyze\n");
    if (aio_rev_mismatch("case3",AIO_INCOMPAT_REVS,11,12,out_fd) != AIO_OK)  {
	fprintf(out_fd,"!!! error detected !!!\n");
    }
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 4 -- AIO_INCOMPAT_REVS -- analyze older than dvr\n");
    if (aio_rev_mismatch("case4",AIO_INCOMPAT_REVS,17,4,out_fd) != AIO_OK)  {
	fprintf(out_fd,"!!! error detected !!!\n");
    }
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 5 -- AIO_INCOMPAT_REVS -- analyze same age as dvr\n");
    if (aio_rev_mismatch("case5",AIO_INCOMPAT_REVS,5,5,out_fd) != AIO_OK)  {
	fprintf(out_fd,"!!! error detected !!!\n");
    }
    fprintf(out_fd,"\n");

}

an_tester_status (out_fd)

    FILE		*out_fd;

{
    llio_status_type		status;
    int				an_tester_decode_status();


    fprintf(out_fd,"\n<<<< Testing status decoding routines >>>>\n\n");


    fprintf(out_fd,"case 1 -- add a new status decoding rtn (ok)\n");
    if (aio_init_llio_status_decode(2771,an_tester_decode_status) != AIO_OK)
	fprintf(out_fd,"!!! error detected !!!\n");
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 2 -- add a new status decoding rtn (same subsys!)\n");
    if (aio_init_llio_status_decode(2771,an_tester_decode_status) != AIO_OK)
	fprintf(out_fd,"!!! error detected !!!\n");
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 3 -- decode a generic i/o status (ok!)\n");
    status.is_ok = 0;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 4 -- decode a generic i/o status (ok!)\n");
    status.is_ok = 0xfe000000;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 5 -- decode a generic i/o status (bad!)\n");
    status.is_ok = 0x17000000;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 6 -- decode a status which doesn't match anything!)\n");
    status.u.proc_num = -1;
    status.u.subsystem = 14;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 7 -- decode an hpib status (ok!)\n");
    status.u.error_num = -50;
    status.u.proc_num = -1;
    status.u.subsystem = SUBSYS_HPIB0;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 8 -- decode an hpib status (bad!)\n");
    status.u.error_num = -124;
    status.u.proc_num = -1;
    status.u.subsystem = SUBSYS_HPIB0;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 9 -- decode a tester status (ok!)\n");
    status.u.error_num = 1;
    status.u.proc_num = -1;
    status.u.subsystem = 2771;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 10 -- decode a tester status (ok!)\n");
    status.u.error_num = 2;
    status.u.proc_num = -1;
    status.u.subsystem = 2771;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");


    fprintf(out_fd,"case 11 -- decode a tester status (bad!)\n");
    status.u.error_num = 3;
    status.u.proc_num = -1;
    status.u.subsystem = 2771;
    aio_decode_llio_status(status,out_fd);
    fprintf(out_fd,"\n");

}
