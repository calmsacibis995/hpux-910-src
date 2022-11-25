/*
 *
 * $Header: Makefile,v 1.2.109.3 95/01/25 13:34:34 mike Exp $
 *  New make file for xntp manual pages in xntp/docs
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

.SOURCE.h : $(SRCDIR:/^$/./) $(SRCDIR:/^$/./)/../include /usr/include

MANINS=		/usr/contrib/man/man1m

INSOPTSM=	-u bin -g bin

all : build

debug : build

bfa : build

build :


install:
	$(INSTALL) $(INSOPTSM) -f $(MANINS) $(TARGET:/^$/ntpdate.1m/)
	$(INSTALL) $(INSOPTSM) -f $(MANINS) $(TARGET:/^$/ntpq.1m/)
	$(INSTALL) $(INSOPTSM) -f $(MANINS) $(TARGET:/^$/ntptrace.1m/)
	$(INSTALL) $(INSOPTSM) -f $(MANINS) $(TARGET:/^$/xntpd.1m/)
	$(INSTALL) $(INSOPTSM) -f $(MANINS) $(TARGET:/^$/xntpdc.1m/)
	# $(INSTALL) $(INSOPTSM) -f $(MANINS) $(TARGET:/^$/tickadj.1m/)

rinstall: install
	$(RINSTALL) $(TARGET:/^$/ntpdate.1m/)
	$(RINSTALL) $(TARGET:/^$/ntpq.1m/)
	$(RINSTALL) $(TARGET:/^$/ntptrace.1m/)
	$(RINSTALL) $(TARGET:/^$/xntpd.1m/)
	$(RINSTALL) $(TARGET:/^$/xntpdc.1m/)
	# $(RINSTALL) $(TARGET:/^$/tickadj.1m/)

clean :
	
clobber : clean
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms
