#!/bin/sh
# @(#) $Revision: 66.1 $     
#	"print daily/summary total accounting (any file in tacct.h format)"
#	"prtacct file [heading]"
PATH=/usr/lib/acct:/bin:/usr/bin:/etc
_filename=${1?"missing filename"}
(cat <<!; acctmerg -t -a <${_filename}; acctmerg -p <${_filename}) | pr -h "$2"
	LOGIN 	   CPU (MINS)	  KCORE-MINS	CONNECT (MINS)	DISK	# OF	# OF	# DISK	FEE
UID	NAME 	 PRIME	NPRIME	PRIME	NPRIME	PRIME	NPRIME	BLOCKS	PROCS	SESS	SAMPLES	
!
