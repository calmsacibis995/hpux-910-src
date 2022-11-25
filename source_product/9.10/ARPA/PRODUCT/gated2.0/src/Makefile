SHELL	=	/bin/sh

all :	build

build:	.FOREGROUND
	echo "Building... $(TARGET)"
	cd $(SRCDIR)
	./make $(TARGET)
	/bin/cp *.o $(OBJDIR)
	/bin/cp $(TARGET) $(OBJDIR)
	/bin/cp libgated.a $(OBJDIR)
	/bin/mkdir -p $(OBJDIR)/compat
	cd compat
	/bin/cp  *.o $(OBJDIR)/compat

clobber: .FOREGROUND
	echo "Clobbering... $(TARGET)"
	cd $(SRCDIR)
        /bin/rm -f version.c
	./make cleanall
	cd $(OBJDIR)
	/bin/rm -rf *.o *.a $(TARGET) $(MAKEFILE:B).mo $(MAKEFILE:B).ms

clean: .FOREGROUND
	./make clean

install: .FOREGROUND
	$(INSTALL) $(TARGET)
