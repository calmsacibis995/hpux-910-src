FILES	= \
	C_assert.o\
	C_codeeven.o\
	C_codeodd.o\
	C_initend.o\
	C_initstrt.o\
	C_param.o\
	C_profcode.o\
	C_sp.o\
	C_stkeven.o\
	C_stkodd.o\
	C_timeend.o\
	C_timestrt.o\
	C_verend.o\
	C_verstart.o\
	D_aname.o\
	D_asrtlist.o\
	D_assert.o\
	D_cleanup.o\
	D_comment.o\
	D_dname.o\
	D_dsetinfo.o\
	D_dsetpar.o\
	D_info.o\
	D_iter.o\
	D_line.o\
	D_listing.o\
	D_open.o\
	D_param.o\
	D_prof.o\
	D_startup.o\
	D_stat.o\
	D_stkeven.o\
	D_stkodd.o\
	D_time.o\
	D_title.o\
	D_verify.o\
	_list.o\
	abort.o\
	asm_code.o\
	asm_driver.o\
	calc_iter.o\
	cleanup.o\
	compact.o\
	cr_asrt.o\
	cr_label.o\
	cr_perf.o\
	cr_prof.o\
	dsets.o\
	error.o\
	err_locate.o\
	execute.o\
	i_asrt.o\
	i_asrt_bwl.o\
	i_code.o\
	i_default.o\
	i_include.o\
	i_iter.o\
	i_ldopt.o\
	i_nolist.o\
	i_output.o\
	i_stack.o\
	i_time.o\
	i_title.o\
	i_verify.o\
	input.o\
	list_item.o\
	main.o\
	next_token.o\
	output.o\
	parse_cmd.o\
	printwidth.o\
	proc_input.o\
	proc_asrt.o\
	prtbytinfo.o\
	s_asrtfile.o\
	s_infile.o\
	s_iter.o\
	s_nolist.o

default: atime

atime:	$(FILES) fizz.h
	$(CC) $(FILES) $(LDFLAGS) -o atime

clean:
	/bin/rm -f *.o

clobber: clean
	/bin/rm -f atime

# copy /usr/bin/atime to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/atime
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	atime
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp atime ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/atime
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
# copy /usr/bin/atime to the . relative fileset PROG-AUX
# change mode, owner, and group on /usr/bin/atime
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/PROG-AUX/usr/bin
	! cp atime ../../../../Filesets.sl/PROG-AUX/usr/bin/atime
	! chmog 0555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin/atime
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/PROG-AUX/
	! chmog 555 bin bin ../../../../Filesets.sl/
