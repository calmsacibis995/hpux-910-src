#	@(#)$Header: Makefile,v 1.22.109.2 94/12/13 15:54:55 mike Exp $
##
#	Makefile for ARPA/Berkeley services
#

SHELL	= /bin/sh

LINK	= inetd netconfig.link include
SERVICES= ARPA ftp rcp remsh rexecd rlogin rwho sendmail telnet netconfig.serv \
	xntp
MANPAGES= ARPA_MAN man

#
# These are hard and soft links that must be delivered with the product.
#
INSLINKS= mailq newaliases mailq.1m newaliases.1m endhostent.3n \
	  endnetent.3n endprotoent.3n endservent.3n gethostbyad.3n \
	  gethostbyna.3n getnetbyadd.3n getnetbynam.3n getprotobyn.3n \
	  getservbyn.3n getservbyna.3n getservbyp.3n getservbypo.3n \
	  htonl.3n htons.3n inet_addr.3n inet_lnaof.3n inet_makead.3n \
	  inet_netof.3n inet_networ.3n inet_ntoa.3n ntohl.3n ntohs.3n \
	  rresvport.3n ruserok.3n sethostent.3n setnetent.3n \
	  setprotoent.3n setservent.3n

PRODUCT	=  ${SERVICES} ${LINK} ${MANPAGES}

all prod trace debug clean clobber pfa:
	for dir in ${PRODUCT}; do \
	    test -d $${dir} && ( cd $${dir}; $(MAKE) -k $@ ) \
	done

install: .FORCE
	test "${INS}" && ${INS} ${EXTRA}
	test "${INS}" && \
	    for dir in ${PRODUCT}; do \
		test -d $${dir} && ( cd $${dir} ; $(MAKE) -k $@ ) \
	    done
	test "${INS}" && ${INS} ${INSLINKS}

.FORCE:
	@rm -rf .FORCE
