# $Source: /misc/source_product/9.10/commands.rcs/usr/bin/text/roff.d/terms.d/makefile,v $
# $Revision: 66.1 $

# Common makefile for the nroff terminal driving tables

TFILES1 = tab300.c tab300-12.c tab300s.c tab300s-12.c \
	  tab37.c tab382.c tab4000a.c tab4000A.c tabklp.c
TFILES2 = tab450.c tab450-12.c tab832.c taba1.c tablp.c tabtn300.c tabX.c
TFILES3 = tab2631.c tab2631-c.c tab2631-e.c tablj.c
FILES1  = tab300 tab300-12 tab300s tab300s-12 tab37 tab382 tab4000a tab4000A \
	  tabklp
FILES2  = tab450 tab450-12 tab832 taba1 tablp tabtn300 tabX
FILES3  = tab2631 tab2631-c tab2631-e tablj
FILES   = $(FILES1) $(FILES2) $(FILES3)
PFILES  = maketerms.c terms.hd

default:	$(FILES)

$(FILES1):	maketerms1
	  echo "./maketerms1"; ./maketerms1

$(FILES2):	maketerms2
	  echo "./maketerms2"; ./maketerms2;

$(FILES3):	maketerms3
	  echo "./maketerms3"; ./maketerms3;

maketerms1:	$(TFILES1) $(PFILES) code.lp code.300
	$(CC) -Wl,-a,archive $(CFLAGS) -DPART1 -o maketerms1 maketerms.c

maketerms2:	$(TFILES2) $(PFILES) code.lp code.300
	$(CC) -Wl,-a,archive $(CFLAGS) -DPART2 -o maketerms2 maketerms.c

maketerms3:	$(TFILES3) $(PFILES) code.lp code.300
	$(CC) -Wl,-a,archive $(CFLAGS) -DPART3 -o maketerms3 maketerms.c

clean:
	/bin/rm -f maketerms
	/bin/rm -f maketerms1 maketerms2 maketerms3

clobber:  clean
	/bin/rm -f $(FILES)
