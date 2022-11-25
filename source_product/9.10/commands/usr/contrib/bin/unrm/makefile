

default: unrm

unrm: newutil.o unrm.o hpux_rel.o
	cc unrm.o newutil.o hpux_rel.o -o unrm

unrm.o: hfsio.h unrm.c
	cc -c unrm.c

newutil.o: hfsio.h newutil.c
	cc -c newutil.c

hpux_rel.o: hfsio.h hpux_rel.c
	cc -c hpux_rel.c

clean:
	rm -f unrm.o newutil.o

clobber: clean
	rm -f unrm


install:	unrm
# copy /usr/contrib/bin/unrm to the . relative fileset UNSUPP
# change mode, owner, and group on /usr/contrib/bin/unrm
# to 02555, bin, and sys
# change mode, owner, and group on all the parent
# directories.
	! mkdir -p ../../../../../Filesets.sl/UNSUPP/usr/contrib/bin
	! cp unrm ../../../../../Filesets.sl/UNSUPP/usr/contrib/bin/unrm
	! chmog 02555 bin sys ../../../../../Filesets.sl/UNSUPP/usr/contrib/bin/unrm
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/contrib/bin
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/contrib/
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/
	! chmog 555 bin bin ../../../../../Filesets.sl/
# copy /usr/contrib/bin/unrm to the . relative fileset UNSUPP
# change mode, owner, and group on /usr/contrib/bin/unrm
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! mkdir -p ../../../../../Filesets.sl/UNSUPP/usr/contrib/man/man1
	! cp unrm.1 ../../../../../Filesets.sl/UNSUPP/usr/contrib/man/man1/unrm.1
	! chmog 0444 bin bin ../../../../../Filesets.sl/UNSUPP/usr/contrib/man/man1/unrm.1
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/contrib/man/man1
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/contrib/man/
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/contrib/
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/usr/
	! chmog 555 bin bin ../../../../../Filesets.sl/UNSUPP/
	! chmog 555 bin bin ../../../../../Filesets.sl/
