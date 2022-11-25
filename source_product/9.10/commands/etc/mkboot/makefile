# This is the makefile for newfs.

SRC=		main.c lifdir.c lifio.c mkboot.c swapinfo.c tio.c hpux_rel.c

OBJECTS=	main.o lifdir.o lifio.o mkboot.o

INCLUDE=	lifdir.h lifio.h global.h

OBJ=		main.o lifdir.o lifio.o mkboot.o swapinfo.o tio.o hpux_rel.o

#CFLAGS=-g
CFLAGS=-D_HPUX -D_LVM

default: all

all:	mkboot mkboot.arch

mkboot:	$(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mkboot $(OBJ)

mkboot.arch:  $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-a,archive -o $@ $(OBJ)

clean:
	/bin/rm -f *.o 

clobber: clean
	/bin/rm -f mkboot rmboot

lint:
	/usr/bin/lint -uvx $(SRC)

print:
	pr -n $(SRC) $(INCLUDE) | remsh hpfclj lp -depoc -olp

$(OBJECTS):	$(INCLUDE)



# copy /etc/mkboot to the . relative fileset KERN-BLD
# change mode, owner, and group on /etc/mkboot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	mkboot
	mkdir -p ../../../Filesets.sl/KERN-BLD/etc
	! cp mkboot ../../../Filesets.sl/KERN-BLD/etc
	! chmog 0555 bin bin ../../../Filesets.sl/KERN-BLD/etc/mkboot
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/etc
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/
# copy /etc/mkboot to the . relative fileset KERN-BLD
# change mode, owner, and group on /etc/mkboot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/KERN-BLD/etc
	! ln ../../../Filesets.sl/KERN-BLD/etc/mkboot ../../../Filesets.sl/KERN-BLD/etc/rmboot
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/etc
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/mkboot to the . relative fileset KERN-BLD
# change mode, owner, and group on /etc/mkboot
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/KERN-BLD/etc
	! cp mkboot.arch ../../../Filesets.sl/KERN-BLD/etc/mkboot
	! chmog 0555 bin bin ../../../Filesets.sl/KERN-BLD/etc/mkboot
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/etc
	! chmog 555 bin bin ../../../Filesets.sl/KERN-BLD/
	! chmog 555 bin bin ../../../Filesets.sl/
