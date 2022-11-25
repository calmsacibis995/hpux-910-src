##
#	@(#)$Revision: 1.40.109.1 $	$Date: 91/11/19 14:00:49 $
# /* NFSSRC (#)Makefile	2.2 86/04/17 */
#	Makefile nfs.1.0 85/06/28 SAI; from 4.17 83/08/19
#
SHELL	= /bin/sh

CSOURCE	= domainname.c
TARGETS	= domainname
BFATARG	= domainname.b

OCFLAGS	= -O ${CFLAGS}
OLFLAGS = ${LDFLAGS}
TCFLAGS	= -DTRACEON ${CFLAGS}
TLFLAGS	=
DCFLAGS	= -g -DDEBUG -DTRACEON ${CFLAGS}
DLFLAGS	= -g
BCFLAGS	= -DBFA ${CFLAGS}
BLFLAGS	= ${LDFLAGS}

.SUFFIXES:.b
.c:
	@-rm -f $*
	$(CC) ${CFLAGS} ${LDFLAGS} ${IDIRS} -o $* $< ${LIBC}
.c.b:
	@-rm -f /nfs/BFA/$*.B $@ _$< _$*.o
	$(BFACC) ${BCFLAGS} ${BLFLAGS} ${IDIRS} -o $@ $? ${BLIBC}  -B"-d/nfs/BFA/$*.B" 

all:		${TARGETS}
bfa:		${BFATARG}

lint:;		lint -bnx ${CFLAGS} ${IDIRS} ${CSOURCE}

clean:;		-rm -f *.b *.o _*.c ,* a.out core errs ERRS Makefile.bak

cleanbfa:;	-rm *.b _*.c

clobber:	clean
	-rm -f ${TARGETS} ${BFATARG}

linstall:	all
	test "${INS}" && ${INS} domainname bin root other 0555 

trace:;	$(MAKE) CFLAGS="${TCFLAGS}" LDFLAGS="${TLFLAGS}" all

debug:;	$(MAKE) CFLAGS="${DCFLAGS}" LDFLAGS="${DLFLAGS}" all

prod:;	$(MAKE) CFLAGS="${OCFLAGS}" LDFLAGS="${OLFLAGS}" clean all

depend:;
	maketd ${CFLAGS} ${IDIRS} ${CSOURCE} > ,makedep
	@echo '/^# DO NOT DELETE THIS LINE/+1,$$d' > ,eddep
	@echo '$$r ,makedep' >> ,eddep
	@echo 'w' >> ,eddep
	cp Makefile Makefile.bak
	ed - Makefile < ,eddep
	rm -f ,eddep ,makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	@echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- maketd depends upon it


