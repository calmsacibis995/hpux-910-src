SHELL	      = /bin/sh

SUBDIR	      =

all:;		@for i in $(SUBDIR);\
		do (\
			echo Making $$i ...;\
			cd $$i;\
			$(MAKE) ROOT=$(ROOT)\
		); done

clean:;		@for i in $(SUBDIR);\
		do (\
			echo Cleaning $$i ...;\
			cd $$i;\
			$(MAKE) clean\
		); done

clobber:;	@for i in $(SUBDIR);\
		do (\
			echo Clobbering $$i ...;\
			cd $$i;\
			$(MAKE) clobber\
		); done

depend:;	@for i in $(SUBDIR);\
		do (\
			echo Creating dependencies for $$i ...;\
			cd $$i;\
			$(MAKE) ROOT=$(ROOT) depend\
		); done

install:;	@for i in $(SUBDIR);\
		do (\
			echo Installing $$i ...;\
			cd $$i;\
			$(MAKE) ROOT=$(ROOT) RELEASE=$(RELEASE) install\
		); done

update:;	@for i in $(SUBDIR);\
		do (\
			echo Updating $$i ...;\
			cd $$i;\
			$(MAKE) ROOT=$(ROOT) RELEASE=$(RELEASE) update\
		); done
