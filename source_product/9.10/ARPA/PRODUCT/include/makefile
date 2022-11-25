#	@(#)$Header: makefile,v 1.2.109.1 91/11/21 11:59:48 kcs Exp $
#
#	Makefile for installation of include files.
#

# OBJECT FILE DEPENDENCIES
INCS		= resolv.h netdb.h
ARPAINCS	= nameser.h

# PRIMARY TARGETS
all clobber clean prod pfa:

install:
	test "${INS}" && { ${INS} ${INCS} ; cd arpa ; ${INS} ${ARPAINCS} ; }

