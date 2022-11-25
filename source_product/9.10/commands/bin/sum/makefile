# @(#) $Revision: 70.9 $
#
# makefile -- makefile for building the sum(1) command
#

FINDMSG = /usr/bin/findmsg
GENCAT	= /usr/bin/gencat

default: sum sum.cat

sum: sum.o algsum.o hpux_rel.o
	$(CC) $(CFLAGS) $(LDFLAGS) sum.o algsum.o hpux_rel.o -o sum

sum.o: sum.c
	$(CC) $(CFLAGS) -c sum.c

algsum.o: algsum.s
	/lib/cpp algsum.s | as -o algsum.o

hpux_rel.o: hpux_rel.c
	$(CC) $(CFLAGS) -c hpux_rel.c

clean:
	rm -f algsum.o sum.o hpux_rel.o *.msg

clobber: clean
	rm -f sum sum.cat

sum.cat: sum.msg
	$(GENCAT) $@ sum.msg

sum.msg:
	$(FINDMSG) *.c > $@

# copy /bin/sum to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sum
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	sum
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sum ../../../Filesets.sl/UX-CORE/bin
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sum
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /bin/sum to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sum
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! cp sum ../../../Filesets.sl/UX-CORE/bin/sum
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/bin/sum
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sum to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sum
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! cp sum.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sum.cat
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sum.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sum to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sum
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/bin
	! ln ../../../Filesets.sl/UX-CORE/bin/sum ../../../Filesets.sl/UX-CORE/bin/cksum
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/bin
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /bin/sum to the . relative fileset UX-CORE
# change mode, owner, and group on /bin/sum
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/sum.cat ../../../Filesets.sl/UX-CORE/usr/lib/nls/C/cksum.cat
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/C
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/nls/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
