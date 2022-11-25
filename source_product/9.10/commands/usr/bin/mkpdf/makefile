# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/mkpdf/makefile,v $
# $Revision: 66.4.1.6 $

# Makefile for pdfmk, pdfdif, pdfck and revck

SHELL	= /bin/sh
FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat
SORT    = /bin/sort
CFLAGS	= -v -O -DDISKLESS -DSYMLINKS -DNLS -DNLS16

SRCS= \
	hpux_rel.c \
	pdf.h \
	main.c \
	pdf_ops.c \
	attrib.c \
	mem_crc.c \
	revck.c

OBJS= \
	hpux_rel.o \
	main.o \
	pdf_ops.o \
	attrib.o \
	mem_crc.o \
	revck.o

INST_DIR = /usr/bin

default: mkpdf # mkpdf.cat # No NLS yet

mkpdf:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lPW

$(OBJS):	pdf.h

mkpdf.cat: mkpdf.msg
	$(GENCAT) $@ mkpdf.msg

mkpdf.msg:
	$(FINDMSG) *.c | $(SORT) -n +1 -1 > $@

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f mkpdf

shar: $(SRCS) makefile
	shar $(SRCS) makefile > shar

listing: $(SRCS)
	@ echo Requires Fort Collins EPOC printer
	pr -l120 -n $(SRCS) | lp -depoc -oxref

oldinstall:    mkpdf
	# for general-purpose distribution
	if [ `id -u` -eq 0 ]; then \
	    cpset mkpdf $(INST_DIR) 555 bin bin; \
	    ln $(INST_DIR)/mkpdf $(INST_DIR)/pdfdiff; \
	    ln $(INST_DIR)/mkpdf $(INST_DIR)/pdfck; \
	    ln $(INST_DIR)/mkpdf $(INST_DIR)/pdfpr; \
	    ln $(INST_DIR)/mkpdf $(INST_DIR)/revck; \
	else \
	    echo "\007Must be super-user to install"; \
	fi
# copy /usr/bin/mkpdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/mkpdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
install:
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! cp mkpdf ../../../../Filesets.sl/UX-CORE/usr/bin/mkpdf
	! chmog 0555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin/mkpdf
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkpdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/mkpdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! ln ../../../../Filesets.sl/UX-CORE/usr/bin/mkpdf ../../../../Filesets.sl/UX-CORE/usr/bin/pdfck
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkpdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/mkpdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! ln ../../../../Filesets.sl/UX-CORE/usr/bin/mkpdf ../../../../Filesets.sl/UX-CORE/usr/bin/pdfdiff
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkpdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/mkpdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! ln ../../../../Filesets.sl/UX-CORE/usr/bin/mkpdf ../../../../Filesets.sl/UX-CORE/usr/bin/pdfpr
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/mkpdf to the . relative fileset UX-CORE
# change mode, owner, and group on /usr/bin/mkpdf
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/UX-CORE/usr/bin
	! ln ../../../../Filesets.sl/UX-CORE/usr/bin/mkpdf ../../../../Filesets.sl/UX-CORE/usr/bin/revck
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../../Filesets.sl/
