/*
 *
 *  $Header: Makefile,v 1.1.109.1 91/11/21 11:48:55 kcs Exp $
 *  New make file for BFA databases directory
 *
 */

.SOURCE :   $(SRCDIR:/^$/./)

bfa:;

build:;

debug:;

install:
	$(INSTALL) $(TARGET)

rinstall: install
	$(RINSTALL) $(TARGET)

clean :
	$(RM) $(RMFLAGS) $(OBJS)

clobber : clean
	$(RM) $(RMFLAGS) $(TARGET)
	$(RM) $(RMFLAGS) $(MAKEFILE:B).mo $(MAKEFILE:B).ms make?a*
