# $Source: /misc/source_product/9.10/commands.rcs/etc/privgrp/makefile,v $
# $Revision: 64.2.1.1 $

# Common makefile for getprivgrp/setprivgrp

# This needed to be left in here because of the kludge below.
INC=/usr/include

default: all

all: getprivgrp setprivgrp

getprivgrp: getprivgrp.o
	$(CC) $(LDFLAGS) -o getprivgrp getprivgrp.o

setprivgrp: setprivgrp.o
	$(CC) $(LDFLAGS) -o setprivgrp setprivgrp.o

clean:
	/bin/rm -f *.o privnames.h

clobber: clean
	/bin/rm -f setprivgrp getprivgrp

# This section will grab from the sys/privgrp.h header file, those
# lines beginning with PRIVILEGE (privileged groups and accociated
# numbers).
privnames.h: $(INC)/sys/privgrp.h
	echo "/*" > Privnames.h
	echo " * Created automatically by messaging <sys/privgrp.h>" >> Privnames.h
	echo " */" >> Privnames.h
	echo "struct {" >> Privnames.h
	echo "    char * name;" >> Privnames.h
	echo "    char   number;" >> Privnames.h
	echo "} privnames [] = {" >> Privnames.h
	awk '/PRIVILEGE/ { printf "%s\t%s\n", $$2, $$3; }' \
	    $(INC)/sys/privgrp.h | \
		sed 's/PRIV_\(.*\)	\(.*\)$$/    {"\1", \2},/' | \
		tr 'A-Z' 'a-z' >> Privnames.h
	echo "};" >> Privnames.h
	mv Privnames.h privnames.h

getprivgrp.o: privnames.h 
setprivgrp.o: privnames.h 

install:
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/etc
	! cp setprivgrp ../../../Filesets.sl/SYS-ADMIN/etc/setprivgrp
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc/setprivgrp
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/etc
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN
	! chmog 555 bin bin ../../../Filesets.sl
	mkdir -p ../../../Filesets.sl/CMDS-AUX/usr/bin
	! cp getprivgrp ../../../Filesets.sl/CMDS-AUX/usr/bin/getprivgrp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/bin/getprivgrp
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX/usr
	! chmog 555 bin bin ../../../Filesets.sl/CMDS-AUX
	! chmog 555 bin bin ../../../Filesets.sl
