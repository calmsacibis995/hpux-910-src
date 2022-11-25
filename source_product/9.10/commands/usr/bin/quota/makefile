# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/quota/makefile,v $
# $Revision: 66.4.1.10 $

# Makefile for disk quota commands and daemons
# (/usr/etc/rpc.rquotad is delivered by networking)

FINDMSG = /usr/bin/findmsg
GENCAT  = /usr/bin/gencat
SORT    = /bin/sort

PROGS	=	\
		quota \
		edquota \
		quot \
		quotacheck \
		quotaon \
		repquota

OBJS	=	\
		quota.o \
		edquota.o \
		quot.o \
		quotacheck.o \
		quotaon.o \
		repquota.o

COPTS	=	-DQUOTA

NLS	=	quota.cat

default: $(PROGS) # $(NLS) # later when NLS is added

quota:	quota.o
	$(CC) -o $@ quota.o -lrpcsvc

edquota:	edquota.o
	$(CC) -o $@ edquota.o

quot:	quot.o
	$(CC) -o $@ quot.o

quotacheck:	quotacheck.o
	$(CC) -o $@ quotacheck.o

quotaon:	quotaon.o
	$(CC) -o $@ quotaon.o

repquota:	repquota.o
	$(CC) -o $@ repquota.o

.c.o:
	$(CC) $(CFLAGS) -DQUOTA -c $<

quota.cat: quota.msg
	$(GENCAT) $@ quota.msg

quota.msg:
	$(FINDMSG) quota.c | $(SORT) -n +1 -1 > $@

clean:
	rm -f *.o

clobber: clean
	rm -f $(PROGS)

# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.

install:	quota
	mkdir -p ../../../../Filesets.sl/DQUOTA/usr/bin
	! cp quota ../../../../Filesets.sl/DQUOTA/usr/bin
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/usr/bin/quota
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/etc
	! cp edquota ../../../../Filesets.sl/DQUOTA/etc/edquota
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/etc/edquota
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/etc
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/etc
	! cp quot ../../../../Filesets.sl/DQUOTA/etc/quot
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/etc/quot
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/etc
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/etc
	! cp quotacheck ../../../../Filesets.sl/DQUOTA/etc/quotacheck
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/etc/quotacheck
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/etc
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/etc
	! cp quotaon ../../../../Filesets.sl/DQUOTA/etc/quotaon
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/etc/quotaon
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/etc
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/etc
	! cp repquota ../../../../Filesets.sl/DQUOTA/etc/repquota
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/etc/repquota
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/etc
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/usr/bin
	! cp quota ../../../../Filesets.sl/DQUOTA/usr/bin/quota
	! chmog 0555 bin bin ../../../../Filesets.sl/DQUOTA/usr/bin/quota
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/usr/bin
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/usr/
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
# copy /usr/bin/quota to the . relative fileset DQUOTA
# change mode, owner, and group on /usr/bin/quota
# to 0555, bin, and bin
# change mode, owner, and group on all the parent
# directories.
	mkdir -p ../../../../Filesets.sl/DQUOTA/etc
	! ln ../../../../Filesets.sl/DQUOTA/etc/quotaon ../../../../Filesets.sl/DQUOTA/etc/quotaoff
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/etc
	! chmog 555 bin bin ../../../../Filesets.sl/DQUOTA/
	! chmog 555 bin bin ../../../../Filesets.sl/
