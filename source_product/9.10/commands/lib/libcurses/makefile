# $Source: /misc/source_product/9.10/commands.rcs/lib/libcurses/makefile,v $
# $Revision: 66.446 $

# Common makefile for libcurses

default: libcurses.a libcurses.sl term_data tic

tic:
	@echo "\tMaking tic..."; 		\
	cd screen;				\
	$(MAKE) tic

libcurses.a:
	@echo "\tMaking archive libcurses...";	\
	cd screen;				\
	$(MAKE) libcurses.a

libcurses.sl:
	@echo "\tMaking shared libcurses...";	\
	cd screen;				\
	$(MAKE) libcurses.sl

term_data:
	@echo "\tMaking terminfo database...";	\
	cd terminfo;				\
	$(MAKE)

#
# screen* targets -- are used to build subsets of libcurses.a
# libcurses_wrap  -- is used to wrap up the parts into the resulting
#                    libcurses.a
# 
screen_x:
	@echo "\tMaking all regular objects";		\
	cd screen;					\
	$(MAKE) PART="all" objects

screen1 screen2 screen3 screen4 screen5 screen6 screen7 screen8:
	@part=`expr $@ : 'screen\(.\)'`;		\
	echo "\tMaking part $$part of regular objects";	\
	cd screen;					\
	$(MAKE) PART="$$part" objects

screen_x.sl:
	@echo "\tMaking all shared objects";		\
	cd screen;					\
	$(MAKE) PART="all" objects.sl

screen1.sl screen2.sl screen3.sl screen4.sl \
screen5.sl screen6.sl screen7.sl screen8.sl:
	@part=`expr $@ : 'screen\(.\).*'`;		\
	echo "\tMaking part $$part of shared objects";	\
	cd screen;					\
	$(MAKE) PART="$$part" objects.sl

libcurses_wrap:
	cd screen; $(MAKE) wrapup

libcurses_wrap.sl:
	cd screen; $(MAKE) wrapup.sl

hpterm:
	cd terminfo; $(MAKE) HPTERM.targets

nonhpterm:
	cd terminfo; $(MAKE) NONHPTERM.targets

ti4:
	cd screen ; $(MAKE) ti4
	
clean:
	cd screen ; $(MAKE) clean
	cd terminfo ; $(MAKE) clean

clobber: 
	cd screen ; $(MAKE) clobber
	cd terminfo ; $(MAKE) clobber
# copy /lib/libcurses to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libcurses
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:
# make all directories
	mkdir -p ../../../Filesets.sl/BUILDENV/lib
	mkdir -p ../../../Filesets.sl/BUILDENV/usr/lib
	mkdir -p ../../../Filesets.sl/CORE-SHLIBS/lib
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/2
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/j
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/k
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/y
	mkdir -p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z
	mkdir -p ../../../Filesets.sl/PROG-MIN/usr/lib
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/E
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/G
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/V
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/X
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/b
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/d
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/g
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/k
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/m
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/n
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/s
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/u
	mkdir -p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v
#
# copy all files
#
	! cp screen/libcurses.sl ../../../Filesets.sl/BUILDENV/lib/libcurses.sl
# copy /lib/libcurses to the . relative fileset CORE-SHLIBS
# change mode, owner, and group on /lib/libcurses
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp screen/libcurses.sl ../../../Filesets.sl/CORE-SHLIBS/lib/libcurses.sl
# copy /lib/libcurses to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp screen/libcurses.a ../../../Filesets.sl/BUILDENV/usr/lib/libcurses.a
# copy /lib/libcurses to the . relative fileset PROG-MIN
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp screen/libcurses.a ../../../Filesets.sl/PROG-MIN/usr/lib/libcurses.a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/1/110 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/110
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/1/120 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/120
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/1/150 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/1/1620-m8 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1620-m8
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/1/1line ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1line
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/1/1linepty ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1linepty
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2382 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2382
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2392 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2393 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2394 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2397 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621-48 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-48
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621-ba ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-ba
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621-nl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-nl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621-nt ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-nt
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621-wl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-wl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621k45 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621k45
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2621p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2622 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2623 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2624 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2625 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2625
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626-12 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-12
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626-12-s ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-12-s
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626-12x40 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-12x40
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626-ns ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-ns
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626-s ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-s
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2626-x40 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-x40
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2627 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2628 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2628
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/262x ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/262x
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2640 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2640b ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2644 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2644
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2645 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2645
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2647 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2647
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2648 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/2/2703 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2703
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/30 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/30
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/300h ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300h
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/300l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/3045 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/3045
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/33 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/33
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/36 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/36
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/3/37 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/37
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4025-17 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025-17
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4025-17ws ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025-17ws
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4025ex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025ex
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4112 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4112-5 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112-5
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4112-d ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112-d
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/425e_l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e_l
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/43 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/43
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4424 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4424-2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424-2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/4/4424-el-2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424-el-2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/5/50 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/50
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/5/5520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/5520
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/6/60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6/60
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/7/70092 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/7/70092-w ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092-w
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/7/70094 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/7/70094-w ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/7/75 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/75
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/8/8001 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/8001
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/8/85 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/85
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9020 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9020
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9122p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9122p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/912b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/912cc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912cc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9202p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9202p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/920b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9502p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9502p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9504p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9504p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/950rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/950rv2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950rv2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/950rv4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950rv4p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9816te ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816te
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9816teb ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816teb
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98204b ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98204b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9826 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9826
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9835 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9836 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/9837 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98541 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98543 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98543
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98546 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98546
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98548 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98548
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98549 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98549
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98550 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98550
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/9/98765 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/D/D1182A ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/a1096a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/a1416a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/a980 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/a980
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aa
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-18 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-18
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-18-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-18-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-20 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-20
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-22 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-22
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-24 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-24
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-24-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-24-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-26 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-26
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-28 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-28
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-29 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-29-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29-ctxt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-29-np ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29-np
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-29-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-ctxt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-rv-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-rv-ctxt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-s-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-ctxt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-s-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-36 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-36
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-36-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-36-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-40-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-40-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-48 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-48
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-48-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-48-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-59 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-59
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-60-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-60-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-60-s-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60-s-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-db ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-db
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-rv-unk ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-rv-unk
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-unk ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-unk
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa18 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa18
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa20 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa20
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa22 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa22
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa24 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa24
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa26 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa26
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa28 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa28
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa29 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa29
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa30 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa30
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa36 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa36
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa48 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa48
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa59 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa59
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa60
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaadb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaadb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/act5s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/act5s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/addrinfo ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/addrinfo
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm1a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm1a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm21 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm21
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm3 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm31 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm31
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm3a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm3a+ ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a+
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm42 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm42
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm42-nl ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm42-nl
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/adm5 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm5
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aed512 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aed512
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/agile ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/agile
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aj830 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aj830
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/altoh19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/altoh19
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/ampex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ampex
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/ansi ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/ansi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/apple ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/apple
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/arpanet ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/arpanet
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bantam ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bantam
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/beacon ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/beacon
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bh3m ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bh3m
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bitgraph-ni ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-ni
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bitgraph-nv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-nv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/blit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blit
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/blit-pb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blit-pb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/blitlayer ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blitlayer
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/b/bussiplexer ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/b/bussiplexer
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100-rv-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100-rv-pp ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-pp
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c1004p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c1004p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rv4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rv4pna ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv4pna
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rv4ppp ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv4ppp
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rvna ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rvna
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rvpp ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rvpp
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100rvs ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rvs
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c100s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-4 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-4
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-8 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-8
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-rv-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-rv-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/c108-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/ca ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ca
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/carlock ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/carlock
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/cbblit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cbblit
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/cdc456 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdc456
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/cdc456tst ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdc456tst
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/cdi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/cg7900 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cg7900
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh-6lpi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-6lpi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh-8lpi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-8lpi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh-comp ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-comp
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh-elite ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-elite
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh-pica ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-pica
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/citoh-prop ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-prop
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/compucolor2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/compucolor2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/c/ct8500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ct8500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/d132 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d132
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/d200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d200
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/d800 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d800
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/datapoint ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/datapoint
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/delta ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/delta
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dg
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/diablo ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/diablo
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dialup ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/d/dialup
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/digilog ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/digilog
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dm1520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm1520
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dm2500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm2500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dm3025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm3025
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dt80 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dt80-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dtc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dtc300s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc300s
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dumb ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/d/dumb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dw1 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dw2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dw3 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw3
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/d/dw4 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw4
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/env230 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/env230
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/ep40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/ep48 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep48
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/esprit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/esprit
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/ethernet ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e/ethernet
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/ex3000 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ex3000
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/e/exidy ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/exidy
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/f/f100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/f/f100-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/f/f1720 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f1720
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/f/falco ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/falco
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/f/falco-p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/falco-p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/f/fox ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/fox
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/g/gigi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gigi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/g/gsi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gsi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/g/gt40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gt40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/g/gt42 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gt42
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1000 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1000
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1420 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1420
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1510 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1510
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1520
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1552 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1552
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h1552-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1552-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19-a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19-bs ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-bs
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19-pb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-pb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19-smul ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-smul
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19-u ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-u
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19bs ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19bs
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h19u ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19u
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/h2000 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h2000
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hk ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hk
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hp ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hp2621nl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621nl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hp2621nt ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621nt
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hp2621wl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621wl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1940a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1940a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1991ac16 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1991ac19 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac19
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa1991agray ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991agray
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa2091a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpa2269a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2269a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpsub ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpsub
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/h/hpterm ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/i100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/i400 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i400
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/ibm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/infoton ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/infoton
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/intext ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intext
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/intext2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intext2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/iq140 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/iq140
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/it ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/i/it2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/l/lpr ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lpr
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mdl110 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mdl110
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/megatek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/megatek
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/microb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/microkit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microkit
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/microterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/microterm5 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microterm5
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime-3a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-3a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime-3ax ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-3ax
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime-fb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-fb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime-hb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-hb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime2a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime2a-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2a-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime2as ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2as
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mime2av ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2av
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mimefb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mimefb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mimehb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mimehb
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/minansi ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/m/minansi
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mransi ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/m/mransi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/m/mw2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mw2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/n/netx ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/netx
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/n/nucterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/nucterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/oadm31 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oadm31
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/obitgraph ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/obitgraph-nv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph-nv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/obitgraph-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/oblit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oblit
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/oc100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oc100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/omron ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/omron
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/ovi300 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/ovi300
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/o/owl ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/owl
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/p/pbox ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/pbox
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/p/plasma ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/plasma
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/p/plugboard ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plugboard
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/p/plus ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plus
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/p/pty ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/pty
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/q/qume5 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q/qume5
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/q/qvt101 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q/qvt101
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/ramtek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/ramtek
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/reach ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/reach
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent20 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent20
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent40-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent40-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent60
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/r/regent60-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent60-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/s1500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/s1500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/sb1 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/sb2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/sbi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sbi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/screwpoint ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/screwpoint
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/sexidy ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sexidy
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/smarterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/smarterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/smartvid ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/smartvid
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/sol ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sol
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/soroc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/soroc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/sun ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sun
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/superbrain ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/superbrain
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/switch ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/s/switch
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/swtp ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/swtp
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/s/synertek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/synertek
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/t1061 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t1061
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/t1061f ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t1061f
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/t3700 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t3700
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/t3800 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t3800
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/t500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tab132 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tab132-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tab132-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tab132-w-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132-w-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tec ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tec
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tec400 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tec400
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tec500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tec500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek4013 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4013
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek4014 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4014
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek4014-sm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4014-sm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek4015 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4015
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek4015-sm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4015-sm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tek4023 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4023
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/teletec ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/teletec
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/terak ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terak
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/terminet1200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet1200
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/ti ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/ti700 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti700
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/ti800 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti800
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/trs80 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/trs80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/ttywilliams ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ttywilliams
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi905 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi905
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi910+ ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi910+
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi912 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi912-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi9122p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9122p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi9202p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9202p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi925 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi925
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi925E ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi925E
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-ap ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-ap
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-ns ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-ns
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-rv-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950-rv-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi9502p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9502p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi9504p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9504p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950ns ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950ns
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950rv2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950rv2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/t/tvi950rv4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950rv4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/u/ubell ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u/ubell
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/u/unitrm18 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u/unitrm18
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/u/unknown ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/u/unknown
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc303 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc303-a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303-a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc404 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc404-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc404-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc404-s-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404-s-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vc415 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc415
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi200-f ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-f
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi200-ic ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-ic
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi200-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi200-rv-ic ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-rv-ic
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi300 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi300-aw ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300-aw
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi300-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi300-ss ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300-ss
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vi550 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi550
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/viewpoint ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/viewpoint
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/virtual ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v/virtual
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-nam ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nam
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-nav ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nav
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-nav-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nav-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-np ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-np
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-s-bot ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s-bot
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100-w-nam ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w-nam
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100am ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100am
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100nam ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100nam
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt100w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt125 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt125
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt132 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt132
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt220 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt220
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt320 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt320
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt50 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt50
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt50h ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt50h
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/v/vt52 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt52
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy120-25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy120-25-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy120-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy120-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy120-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy30-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy30-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy50-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy50-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy50-wvb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-25-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-25-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-42 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-42
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-42-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-42-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-43 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-43
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-43-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-43-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy60-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy75-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy75-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy75-wvb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy85-nx ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-nx
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy85-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy85-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/w/wy85-wvb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/x/x1720 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1720
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/x/xitex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xitex
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/x/xl83 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xl83
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/x/xterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/x/xterms ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xterms
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/y/ya ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/y/ya
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/z/zen30 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z/zen30
# copy /lib/libcurses to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/BUILDENV/usr/lib/libcurses.a ../../../Filesets.sl/BUILDENV/usr/lib/libtermcap.a
# copy /lib/libcurses to the . relative fileset PROG-MIN
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/PROG-MIN/usr/lib/libcurses.a ../../../Filesets.sl/PROG-MIN/usr/lib/libtermcap.a
# copy /lib/libcurses to the . relative fileset BUILDENV
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/BUILDENV/usr/lib/libcurses.a ../../../Filesets.sl/BUILDENV/usr/lib/libtermlib.a
# copy /lib/libcurses to the . relative fileset PROG-MIN
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/PROG-MIN/usr/lib/libcurses.a ../../../Filesets.sl/PROG-MIN/usr/lib/libtermlib.a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/100
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1003
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1003a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1003g
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1096a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plus ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/110plus
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1416a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm1520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1520
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm1520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1521
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/diablo ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1620
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/diablo ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1640
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1620-m8 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1640-m8
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1720 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1700
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm2500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/2/2500
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-fl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621P
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621nl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621nl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621nt ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621nt
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621wl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621wl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626P
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2647 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2647F
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2703 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2709
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc300s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/300
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc300s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/300s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm31 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/31
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/3101
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/digilog ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/333
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/35
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hk ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/35714
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hk ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/35714a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/synertek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/380
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/382
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/3a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a+ ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/3a+
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i400 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/400
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4012
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4013 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4013
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4014 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4014
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4014-sm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4014-sm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4015 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4015
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4015-sm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4015-sm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4023 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4023
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4024
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025cu
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4027
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025-17 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4027-17
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025-17ws ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4027-17ws
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4027cu
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025ex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4027ex
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4080
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112-nd
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4113
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4114
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm42 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/42
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm42-nl ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/42-nl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e_h
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/45
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/diablo ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/450
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/700-41
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092-w ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092A-w
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092-w ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092a-w
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094-w ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094A-w
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094-w ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094a-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti700 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/735
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/743
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/745
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/8510
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi905 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/905
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi910+ ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/910+
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912c
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920c
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi925 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/925
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi925E ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/925E
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950-rv-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950-rv-4p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9826 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9826ite
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836ite
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816te ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836te
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816teb ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836teb
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837ite
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9845
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98542
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98542a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98543 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98543a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98545
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98545a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98546 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98546a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98548 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98548a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98549 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98549a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98550 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98550a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98702
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98702a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98704
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98704a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705c
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98710
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98710a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98721
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98721a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98731
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98731a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98735
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98735a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98737
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98737a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98766
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1439A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1454A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1659A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1924A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1991AC16
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac19 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1991AC19
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991agray ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A1991AGrayscale
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A2091A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2269a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/A/A2269A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C/CRX
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C/CRX24
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C/CRX24Z
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C/CRX48Z
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C/CRX_H
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1940a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/C/CRX_L
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182A ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/E/EVRX
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/E/EVRX_H
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e_l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/E/EVRX_L
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/G/GRX
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/G/GRX_H
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA1439A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA1454A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA1659A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA1924A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1940a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA1940A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA2091A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2269a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/H/HPA2269A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/V/VGA
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpterm ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/X/X-hpterm
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1439a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1454a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2091a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991agray ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2208a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991agray ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2209a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2210a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2211a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2212a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac19 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2213a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac19 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2214a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac19 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2215a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2222a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2269a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a2269a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-ctxt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-rv-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-rv-ctxt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-s-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/act4
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microterm5 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/act5
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/viewpoint ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ad
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm1a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/36 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm36
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a+ ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3aplus
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aed512 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aed
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/agile ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/agiles
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aj830 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aj
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aj830 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aj832
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/altoh19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/alto
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/altoh19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/altoheath
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ambas
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ambassador
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aa ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/annarbor
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bh3m ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/beehiveIIIm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-ni ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg-ni
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-nv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg-nv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg1.25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph-nv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg1.25-nv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg1.25-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-ni ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg2.0
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-nv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg2.0-nv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg2.0-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oc100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-1p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-4p-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-4p-pp
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003g
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c104
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-8p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-8p-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-8p-rv-na
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-na-8p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-8p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-na-8p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-w-8p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ca ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ca22851
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v/virtual ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/cb-unix
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v/virtual ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/cbunix
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdc456 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdi1203
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cg7900 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/chromatics
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ci8510
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-prop ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-ps
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/compucolor2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/compucolorii
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept100-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept100rvna
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept108-4p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept108-8p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-na ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept108-na8
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-4p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept108-rv4
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept108-rv8
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/concept108-w-8
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/crx
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/crx24
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/crx24z
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/crx48z
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/crx_h
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1940a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/crx_l
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/swtp ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ct82
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ampex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d132 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/datagraphix
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm2500 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/datamedia2500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dataspeed40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/delta ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dd5000
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/decwriter
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dg ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dg6053
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ampex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dialogue
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ampex ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dialogue80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d800 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/direct
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d800 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/direct800
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm1520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm1521
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/3045 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm3045
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm80-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dmdt80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dmdt80-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/datapoint ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dp3
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/datapoint ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dp3360
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/ds40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/ds40-2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc382
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/env230 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/envision230
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep4000
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep48 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep4080
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e/evrx
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e/evrx_h
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e_l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e/evrx_l
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/exidy ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/exidy2500
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f1720 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f1720a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cbblit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/fixterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/freedom
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100-rv ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/freedom-rv
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/freedom100
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/g/grx
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/g/grx_h
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gt100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gt100a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19A
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/reach ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h89
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/heath
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/heath-19
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/heath-ansi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/heathkit
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/heathkit-a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp1096a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/110 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp110
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp1416a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp150
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp150A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp150a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2382 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2382
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2382 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2382a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2392
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2392A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2392a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2393
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2393A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2393a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2394
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2394A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2394a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2397
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2397A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2397a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-ba ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621-ba
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621-fl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-nl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621-nl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-nt ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621-nt
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-wl ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621-wl
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621k45 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621k45
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621p ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2622
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2622a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2622p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2623
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2623a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2623p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2624
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2624a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2624p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2625 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2625
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2626
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2626a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2626p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2627
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2627a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2627p
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2628 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2628
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2640a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640b ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2640b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2644 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2644
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2645 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2645
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2648
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2648a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2703 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2703
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2703 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2709
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300h ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp300h
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp300l
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hk ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp35714
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hk ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp35714a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp425e
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp425e_h
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e_l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp425e_l
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2644 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp44
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2645 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp45
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9020 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9020
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816te ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9816te
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816teb ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9816teb
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98204b ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98204b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9826 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9826
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9835
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9836
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9836a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816te ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9836te
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816teb ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9836teb
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9837
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9837a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp9845
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98541
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98541a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98542
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98542a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98543 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98543
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98543 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98543a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98544
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98544a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98545
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98545a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98546 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98546
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98546 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98546a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98548 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98548
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98548 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98548a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98549 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98549
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98549 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98549a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98550 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98550
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98550 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98550a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98700
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98700a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98702
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98702a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98704
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98704a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98705
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98705a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98705b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98705c
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98710
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98710a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98720
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98720a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98721
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98721a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98730
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98730a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98731
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98731a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98735
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98735a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98736
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98736a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98736b
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98737
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98737a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98765
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98765a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98766
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp98766a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpA1439A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
#	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454A
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpA1454A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2269a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpA2269A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182A ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpD1182
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182A ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpD1182A
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182A ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpD1182a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1096
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1096a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1416
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1416a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1439a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpex
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i3101
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm3101
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intertec
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intertube
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intertube2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intext2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intextii
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-prop ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ips
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/soroc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/iq120
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/8001 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/isc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/8001 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/isc8001
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/j/jerq
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621k45 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/k/k45
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/carlock ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/k/klc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/synertek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/k/ktm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw3 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/la120
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blitlayer ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/layer
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lpr ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lp
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microbee
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-3a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-adm3a
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-3ax ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-adm3ax
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2a-v
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mimei
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mimeii
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microkit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mkt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/5520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/nec
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/netx ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/netronics
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e/ethernet ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/n/network
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/110 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/n/nomad
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/nucterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/nuc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oadm31 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/o31
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oc100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oconcept
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oblit ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/ojerq
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plugboard ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/patch
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plugboard ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/patchboard
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bantam ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/pe550
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lpr ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/print
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lpr ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/printer
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/pbox ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/printerbox
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lpr ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/printing
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/ps
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q/qume5 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q/qume
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/nucterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/rayterm
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent200
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent40-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent40+
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb3
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb1 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sbg
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/smarterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/smarterm-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sol ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sol1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sol ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sol2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/5520 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/spinwriter
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sun ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sun1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sbi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/superbee
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/system1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t1061 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t10
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/y/ya ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t16
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4012
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4024
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4025
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4027
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4112
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t3700 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/teleray
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/televideo950
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet1200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet1200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet300
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4023 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tex
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti700 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti733
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti700 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti735
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti745
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet1200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tn1200
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet1200 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tn300
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/trs80 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/trs-80
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/falco ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ts-1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/falco ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ts1
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/33 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/33 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty33
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/37 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty37
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/40 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty40
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/43 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty43
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty4424
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424-2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty4424-2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424-el-2 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tty4424-el-2
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912c
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912cc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912cc
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912cc ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912cup@
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi920
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi920-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi920b
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920b ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi920c
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u/ubell ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u/ubellchar
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc103
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc203
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303-a ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc403a
# copy /lib/libcurses to the . relative fileset UX-CORE
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541 ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v/vga
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-f ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/visual
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blitlayer ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vitty
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gigi ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vk100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xterm ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vs100
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xterms ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vs100s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-am
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s-bot ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-bot-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w-nam ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nam-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s-top
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-top-s
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w-am
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nav-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w-nav
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt220 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt220-am
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt320 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt320-am
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/120 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/120 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy150
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy150-25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy150-25-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy150-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy150-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy150-w-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/30 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy30
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/50 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6/60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/75 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/85 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6/60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy99gt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-2p ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy99gt-2p
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy99gt-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy99gt-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy99gt-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/120 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse120
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse120-25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse120-25-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse120-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse120-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse120-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/120 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse150
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse150-25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse150-25-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse150-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse150-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse150-w-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/30 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse30
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy30-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse30-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/50 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse50
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse50-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse50-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-wvb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse50-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6/60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-25 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-25
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-25-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-25-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-42 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-42
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-42-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-42-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-43 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-43
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-43-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-43-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse60-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/75 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse75
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse75-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse75-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-wvb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse75-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/85 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse85
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-nx ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse85-nx
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse85-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse85-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-wvb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse85-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6/60 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse99gt
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse99gt-vb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse99gt-w
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w-vb ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wyse99gt-wvb
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1720 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1700
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1720 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1750
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z/z19
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z/zen30 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z/z30
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! ln ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19 ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z/zenith
# copy /lib/libcurses to the . relative fileset NONHPTERM
# change mode, owner, and group on /lib/libcurses
# to 0444, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	! cp lib/a/aaa-30-s-rv-ctxt ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-rv-ct
#
# chmog all the files and dirs
#
	! chmog 0444 bin bin ../../../Filesets.sl/BUILDENV/usr/lib/libcurses.a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/120
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1/1620-m8
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/30
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/3045
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/33
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/36
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3/37
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/40
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025-17
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025-17ws
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4025ex
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112-5
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4112-d
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/43
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424-2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4/4424-el-2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/50
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5/5520
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6/60
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7/75
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/8001
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8/85
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9122p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912b
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/912cc
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9202p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/920b
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9502p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/9504p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950rv2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9/950rv4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/a980
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aa
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-18
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-18-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-20
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-22
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-24
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-24-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-26
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-28
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29-ctxt
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29-np
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-29-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-ctxt
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-rv-ctxt
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-ctxt
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-30-s-rv-ct
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-36
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-36-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-40
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-40-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-48
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-48-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-59
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60-s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-60-s-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-db
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-rv-unk
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa-unk
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa18
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa20
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa22
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa24
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa26
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa28
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa29
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa30
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa36
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa40
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa48
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa59
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaa60
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aaadb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/act5s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/addrinfo
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm1a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm21
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm31
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm3a+
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm42
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm42-nl
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/adm5
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aed512
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/agile
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/aj830
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/altoh19
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/ampex
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a/apple
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bantam
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bc
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/beacon
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bg
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bh3m
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-ni
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/bitgraph-nv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blit
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blit-pb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b/blitlayer
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-na
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100-rv-pp
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c1004p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv4pna
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rv4ppp
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rvna
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rvpp
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100rvs
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c100s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-4
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-8
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-na
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-rv-na
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/c108-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ca
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/carlock
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cbblit
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdc456
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdc456tst
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cdi
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/cg7900
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-6lpi
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-8lpi
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-comp
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-elite
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-pica
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/citoh-prop
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/compucolor2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c/ct8500
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d132
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d200
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/d800
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/datapoint
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/delta
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dg
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/diablo
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/digilog
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm1520
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm2500
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dm3025
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dt80-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dtc300s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw1
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw3
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d/dw4
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/env230
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep40
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ep48
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/esprit
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/ex3000
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e/exidy
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f100-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/f1720
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/falco
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/falco-p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f/fox
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gigi
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gsi
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gt40
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g/gt42
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1000
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1420
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1500
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1510
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1520
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1552
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h1552-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-bs
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-pb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-smul
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19-u
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19b
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19bs
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h19u
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h/h2000
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/i400
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/ibm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/infoton
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intext
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/intext2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/iq140
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i/it2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l/lpr
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mdl110
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/megatek
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microkit
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microterm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/microterm5
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-3a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-3ax
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-fb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime-hb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2a-s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2as
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mime2av
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mimefb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mimehb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m/mw2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/netx
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n/nucterm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oadm31
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph-nv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/obitgraph-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oblit
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/oc100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/omron
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/ovi300
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o/owl
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/pbox
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p/plasma
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q/qume5
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q/qvt101
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/ramtek
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/reach
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent20
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent25
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent40
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent40-s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent60
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r/regent60-na
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/s1500
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb1
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sb2
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sbi
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/screwpoint
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sexidy
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/smarterm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/smartvid
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sol
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/soroc
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/sun
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/superbrain
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/swtp
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s/synertek
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t1061
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t1061f
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t3700
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t3800
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/t500
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tab132-w-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tec
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tec400
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tec500
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4013
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4014
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4014-sm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4015
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4015-sm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tek4023
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/teletec
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terak
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/terminet1200
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti700
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ti800
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/trs80
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/ttywilliams
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi905
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi910+
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi912-2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9122p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9202p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi925
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi925E
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-ap
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-b
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-ns
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv-2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950-rv-4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9502p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi9504p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950b
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950ns
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950rv2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t/tvi950rv4p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u/ubell
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u/unitrm18
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc303-a
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404-na
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404-s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc404-s-na
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vc415
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-f
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-ic
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi200-rv-ic
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300-aw
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300-rv
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi300-ss
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vi550
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/viewpoint
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nam
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nav
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-nav-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-np
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-s-bot
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100-w-nam
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100am
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100nam
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100s
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt100w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt125
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt132
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt220
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt320
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt50
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt50h
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v/vt52
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy100
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-25-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy120-w-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy30-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy50-wvb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-25
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-25-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-2p
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-42
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-42-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-43
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-43-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy60-w-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy75-wvb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-nx
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-vb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-w
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w/wy85-wvb
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/x1720
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xitex
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xl83
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xterm
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x/xterms
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/y/ya
	! chmog 0444 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z/zen30
	! chmog 0444 bin bin ../../../Filesets.sl/PROG-MIN/usr/lib/libcurses.a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/110
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/150
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1line
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1/1linepty
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2382
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2392
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2393
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2394
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2397
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-48
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-ba
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-nl
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-nt
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621-wl
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621k45
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2621p
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2622
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2623
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2624
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2625
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-12
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-12-s
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-12x40
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-ns
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-s
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2626-x40
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2627
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2628
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/262x
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2640b
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2644
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2645
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2647
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2648
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2/2703
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300h
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3/300l
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4/425e_l
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70092-w
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7/70094-w
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9020
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816te
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9816teb
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98204b
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9826
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9835
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9836
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/9837
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98541
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98543
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98544
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98546
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98548
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98549
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98550
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98700
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98705
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98720
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98730
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98736
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9/98765
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D/D1182A
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1096a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/a1416a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/ansi
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a/arpanet
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/b/bussiplexer
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c/c1003
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/d/dialup
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/d/dumb
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e/ethernet
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hk
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621nl
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621nt
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hp2621wl
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1454a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1659a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1924a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1940a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac16
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991ac19
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa1991agray
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2091a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpa2269a
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpsub
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h/hpterm
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/m/minansi
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/m/mransi
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plugboard
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/plus
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p/pty
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/s/switch
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/u/unknown
	! chmog 0444 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v/virtual
	! chmog 0555 bin bin ../../../Filesets.sl/BUILDENV/lib/libcurses.sl
	! chmog 0555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib/libcurses.sl
	! chmog 555 bin bin ../../../Filesets.sl/
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/lib
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/usr/
	! chmog 555 bin bin ../../../Filesets.sl/BUILDENV/usr/lib
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/
	! chmog 555 bin bin ../../../Filesets.sl/CORE-SHLIBS/lib
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/1
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/3
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/4
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/5
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/6
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/7
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/8
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/9
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/a
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/b
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/c
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/d
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/e
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/f
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/g
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/h
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/i
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/j
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/k
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/l
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/m
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/n
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/o
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/p
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/q
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/r
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/s
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/t
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/u
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/v
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/w
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/x
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/y
	! chmog 555 bin bin ../../../Filesets.sl/NONHPTERM/usr/lib/terminfo/z
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/usr/
	! chmog 555 bin bin ../../../Filesets.sl/PROG-MIN/usr/lib
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/1
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/2
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/3
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/4
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/7
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/9
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/D
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/a
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/b
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/c
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/d
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/e
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/g
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/h
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/m
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/p
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/s
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/u
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/usr/lib/terminfo/v
	mkdir -p ../../../Filesets.sl/SYS-ADMIN/usr/bin
	! cp screen/tic ../../../Filesets.sl/SYS-ADMIN/usr/bin/tic
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/bin/tic
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr/bin
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN/usr
	! chmog 555 bin bin ../../../Filesets.sl/SYS-ADMIN
	! chmog 555 bin bin ../../../Filesets.sl
