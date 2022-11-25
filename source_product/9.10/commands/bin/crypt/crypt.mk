# @(#) $Revision: 27.1 $       

ROOT    =
CFLAGS  = -O -UNLS -DCRYPT
LDFLAGS = -s
INSDIR  = $(ROOT)/bin
CMD     = crypt
OWNER   = bin
GROUP   = bin
MODE    = 555

all: $(CMD)

$(CMD): $(CMD).c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(CMD) $(CMD).c

clean:

clobber: clean
	-rm -f $(CMD)

install: release

release: $(CMD)
	cp    $(CMD)   $(INSDIR)/$(CMD)
	chown $(OWNER) $(INSDIR)/$(CMD)
	chgrp $(GROUP) $(INSDIR)/$(CMD)
	chmod $(MODE)  $(INSDIR)/$(CMD)
