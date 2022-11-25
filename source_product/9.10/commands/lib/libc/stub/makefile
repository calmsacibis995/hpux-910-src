# @(#) $Revision: 66.3 $
#
# Common makefile for libc/stub subdirectory
#
SUBNAME=stub

default: archive

archive:
	ksh ./mkstubs -v -d ../tmp
	touch ../tmp/OK.$(SUBNAME)

shared:
	ksh ./mkstubs -v -d ../tmp_sl -DPIC
	touch ../tmp_sl/OK.$(SUBNAME)

profile:
	ksh ./mkstubs -v -d ../tmp_p -DPROFILE
	ksh ./mkstubs -v -d ../tmp_p -DNON_PROFILE _brk_noprof
	touch ../tmp_p/OK.$(SUBNAME)

archive_pic:
	ksh ./mkstubs -v -d ../tmp_pic -DPIC
	touch ../tmp_pic/OK.$(SUBNAME)

clean:
	/bin/true

clobber:
	@echo "Clobbering is done by the top-level libc makefile"
