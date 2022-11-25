#
#	Makefile for gated root directory
#
#	This Makefile just invokes the makefiles in the proper
#	subdirectories
#
#	make 			Compile the source
#	make install-man	Install the man page
#	make install		Install the source and man page
#	make dist		Make a distribution tar file

SRCFILES	=	src/Makefile.* src/*.[chly] src/util\
			src/*.awk src/asm.sed src/make src/ChangeLog\
			src/compat/Makefile.* src/compat/*.[ch]\
			src/compat/asm.sed src/compat/make

ALTSRCFILES	=	src.alt/Makefile src.alt/*.[ch]\
			src.alt/{version,sigconv}.awk src/asm.sed
ROOTFILES	=	Acknowledgements CHANGES Copyright Licensing Makefile README
ROOTDIRS	=	aux conf man rfc
TARFILES	=	${ROOTFILES} ${ROOTDIRS} ${SRCFILES}
ALTFILES	=	${ROOTFILES} ${ROOTDIRS} ${ALTSRCFILES}
VERSION		=	2.1
TARDIR		=	tar
TMPDIR		=	/tmp
TARNAME		=	gated-${VERSION}
DISTFILE	=	${TARNAME}.tar.Z
TESTFILE	=	tar/gated.tar.test
ALTFILE		=	tar/gated.tar.alt
TAPE		=	/dev/rmt8
MAILTO		=	nobody

gated:	src
	(cd src; ./make)

true:;

install:	src man install-man
	(cd src; ./make install)

install-man:
	(cd man; ./make install)

dist:	${TARDIR}/${TARNAME}.tar.Zu
	cd ${TARDIR} ; split ${TARNAME}.tar.Zu ${TARNAME}.tar.Zus.

mail:	${TARDIR}/${TARNAME}.tar.Zu
	mail -s ${TARNAME}.tar.Zu ${MAILTO} < ${TARDIR}/${TARNAME}.tar.Zu

${TARDIR}/${TARNAME}.tar.Zu:	${TARDIR}/${TARNAME}.tar.Z
	cd ${TARDIR} ; uuencode ${TARNAME}.tar.Z ${TARNAME}.tar.Z > ${TARNAME}.tar.Zu
	
${TARDIR}/${TARNAME}.tar.Z:	true
	@rm -r -f ${TMPDIR}/${TARNAME}
	@mkdir ${TMPDIR}/${TARNAME}
	@echo "Building distribution tree: ${TARNAME}"
	@tar cFFf - ${TARFILES} | ( cd ${TMPDIR}/${TARNAME} ; tar xf - )
	@echo "Adding copyright"
	cd ${TMPDIR}/${TARNAME}/src ; find . -name \*.\[chly\] -print | xargs util/copyright
	@echo "Cleaning distribution tree"
	@find ${TMPDIR}/${TARNAME} \( -name \*.orig -o -name \*.old -o -name \*~ -o -name _\* \) -print | xargs rm
	@echo "Setting groups and modes"
	@chgrp -R staff ${TMPDIR}/${TARNAME}
	@chmod -R u=rwX,go=rX ${TMPDIR}/${TARNAME}
	tar cFFf - -C ${TMPDIR} ${TARNAME} | compress -c > ${TARDIR}/${TARNAME}.tar.Z
	@rm -r -f ${TMPDIR}/${TARNAME}

alt-dist:	${ALTFILE}

${ALTFILE}:	${ALTFILES} version
	tar cf ${ALTFILE} ${ALTFILES}
	compress -c ${ALTFILE} > ${ALTFILE}.Z

test-dist:	${TESTFILE}

${TESTFILE}:	${SRCFILES} version
	tar cf ${TESTFILE} ${SRCFILES} src/conf.*
	compress ${TESTFILE}

tape:	version
	tar cvf ${TAPE} ${TARFILES}

rdist:
	(rdist)
	(cd src; rdist)
	(cd man; rdist)

version:	true
	cd src; ./make version.c; rm -f parser.c parser.h task_sig.h lexer.c

