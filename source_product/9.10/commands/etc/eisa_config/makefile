#
#	Make file for the EISA Configuration Utility.
#                       HP-UX Version
#


#
#  These are the global (controlling) defines used in the code:
#
#    DEBUG:	This is just your basic debugging code.
#
#
#    LINT:	This turns on some code needed to get a (relatively)
#		clean lint. This is used mostly when there are *bad*
#		header file problems. Also used to handle some problems
#		in lex.c and yyparse.c which I want to leave alone (they
#		are machine-generated files). (Note that I also define
#		__lint in the lint target. This helps with some of the
#		standard HP-UX header files.)
#
#		Used by:
#			lex.c
#			yyparse.c
#
#
#
#    YYDEBUG:	Debugging turned on for the yacc-generated code.
#
#               Used by:
#    			yyparse.c
#
#
#    MANUAL_VERIFY:	Allows a user to specify when verifys should happen.
#			Right now, done after every operation. This is not
#			fully implemented and not tested.
#
#               Used by:
#    			config.h 
#    			cf_util.c
#			main.c
#
#
#    SYS_CACHE:	Allows the system board to specify cache information. Not
#		turned on -- not tested.
#
#               Used by:
#    			config.h 
#			cl_utils.c
#			display.c
#			emitter.c
#			release.c
#
#
#    VIRT_BOARD: Allows a virtual board to be added to the system. Not fully
#		 implemented and not tested. Some of this code will also be
#		 needed if we ever want to add non-slot 0 embedded boards to
#		 the system.
#
#               Used by:
#			add.c
#			err.c
#			err.h
#			help.c
#			main.c
#			nvmsave.c
#			release.c
#			show.c
#			sw.c
#			util.c
#


#
# src and header files which I want to print today
#
PRINT_FILES = add.h cf_util.h compat.h compiler.h config.h		\
	      def.h err.h nvm.h	pr.h sci.h sw.h			        \
	      add.c cf_util.c cfgload.c cl_utils.c config.c             \
  	      err.c globals.c help.c init.c main.c nvmload.c            \
	      nvmlowlevel.c nvmsave.c open_save.c parser.c              \
	      pr.c release.c show.c sw.c util.c


#
# source files used to create the eisa_config program
#
# NOTE: display.c should be added here when it is needed
#
SRCS =	    add.c cf_util.c cfgload.c cl_utils.c config.c           \
	    emitter.c err.c globals.c help.c hpux_rel.c init.c      \
	    lex.c main.c nvmload.c nvmlowlevel.c nvmsave.c	    \
	    open_save.c parser.c pr.c release.c show.c		    \
	    sw.c util.c yyparse.c


#
# object files used to create the eisa_config program
#
# NOTE: display.o should be added here when it is needed
#
OBJS =	    add.o cf_util.o cfgload.o cl_utils.o config.o           \
	    emitter.o err.o globals.o help.o hpux_rel.o init.o      \
	    lex.o main.o nvmload.o nvmlowlevel.o nvmsave.o	    \
	    open_save.o parser.o pr.o release.o show.o		    \
	    sw.o util.o yyparse.o


#
# production targets
#
default:	eisa_config eisa_config.arch

eisa_config:	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o eisa_config

eisa_config.arch:	$(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -Wl,-a,archive -o eisa_config.arch


#
# clean up targets
#
clean:
	rm -f *.o

clobber:	clean
	rm -f eisa_config


#
# print target
#
print:
	pr -l120 $(PRINT_FILES) | lp -depoc -oxref


#
# BFA targets
#
bfa:
	$(MAKE)	CC=/usr/local/bin/bfacc _bfa

_bfa:   $(OBJS)
	$(CC) $(OBJS) -o eisa_config.bfa


#
# lint target
#
lint:   $(SRCS)
	/usr/bin/lint -DLINT -D__lint -Zn8096 -I$(INC) -Nw2000 -Np30000 $(SRCS)


#
# debug targets
#
debug:
	$(MAKE) SPECFLAGS=-g _debug

_debug: $(OBJS)
	$(CC) $(CFLAGS) $(SPECFLAGS) $(LDFLAGS) $(OBJS) /usr/lib/end.o -o eisa_config.debug


#
# source file targets
#
.c.o:
	$(CC) -c $(SPECFLAGS) $(CFLAGS) $*.c

# copy /etc/eisa_config to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/eisa_config
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	eisa_config
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp eisa_config ../../../Filesets.sl/UX-CORE/etc
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa_config
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
# copy /etc/eisa_config to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/eisa_config
# to 0544, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc
	! cp eisa_config ../../../Filesets.sl/UX-CORE/etc/eisa_config
	! chmog 0544 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa_config
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/eisa_config to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/eisa_config
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc/eisa
	! cp cfg_files/!HWP2051.CFG ../../../Filesets.sl/UX-CORE/etc/eisa/HWP2051.CFG
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa/HWP2051.CFG
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/eisa_config to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/eisa_config
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc/eisa
	! cp cfg_files/!HWPC020.CFG ../../../Filesets.sl/UX-CORE/etc/eisa/HWPC020.CFG
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa/HWPC020.CFG
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
# copy /etc/eisa_config to the . relative fileset UX-CORE
# change mode, owner, and group on /etc/eisa_config
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../Filesets.sl/UX-CORE/etc/eisa
	! cp cfg_files/!HWPC031.CFG ../../../Filesets.sl/UX-CORE/etc/eisa/HWPC031.CFG
	! chmog 0555 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa/HWPC031.CFG
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/eisa
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/etc/
	! chmog 555 bin bin ../../../Filesets.sl/UX-CORE/
	! chmog 555 bin bin ../../../Filesets.sl/
