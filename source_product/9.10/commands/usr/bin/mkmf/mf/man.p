DEST	      = .

MAKEFILE      = Makefile

PRINT	      = nroff -man

SHELL	      = /bin/sh

SRCS	      =

SUFFIX	      = .1:s  .1c:s .1m:s .2:s .3:s .3c:s .3d:s .3g:s .3m:s \
		.3s:s .3x:s .4:s  .5:s .6:s .7:s  .8:s  .9:s

all:;

clean:;

clobber:;

depend:;	@mkmf -f $(MAKEFILE)

echo:;		@echo $(SRCS)

install:;	@echo Installing $(SRCS) in $(DEST)
		@if [ $(DEST) != . ]; then \
			for i in $(SRCS);\
			do (\
				rm -f $(DEST)/$$i;\
				cp $$i $(DEST);\
			); done;\
		fi

print:;		@$(PRINT) $(SRCS)

update:		$(DEST)

$(DEST):	$(SRCS)
		@$(MAKE) -f $(MAKEFILE) DEST=$(DEST) install
