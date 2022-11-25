
#	   @(#)1.2	87/02/13
all: checkdate
bfa:;		# We do not need to make BFA for bin/checkdate

CFLAGS = -O
LDFLAGS = -s

checkdate: checkdate.o netconn.o
	cc $(LDFLAGS) -o checkdate checkdate.o netconn.o -ln


