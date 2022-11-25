default:	df.c
	 $(CC)  df.c -o df
	 findmsg df.c > df.msg
	 gencat df.cat df.msg
clean:
	rm -f *.o *.msg

clobber:	clean
	rm -f df

# copy /bin/df to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/df
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.

install:	df
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp df ../../../Filesets.sl/UX-CORE/bin
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/df
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/df to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/df
# to 04555, root, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp df ../../../Filesets.sl/UX-CORE/bin/df
	! chmog 04555 root bin ../../../Filesets.sl/UX-CORE/bin/df
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
