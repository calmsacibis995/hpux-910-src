#	@(#)$Header: makefile,v 1.1.109.1 91/11/21 11:38:50 kcs Exp $

PRODUCT	=	util bind bootp fingerd ftp ftpd gated inetd rcp \
		remsh remshd rlogin rlogind ruptime rwho rwhod sendmail \
		telnet telnetd tftp tftpd


all clean clobber:
	for dir in ${PRODUCT}; do \
	    test -d $${dir} && ( cd $${dir}; $(MAKE) -k $@ ) \
	done
