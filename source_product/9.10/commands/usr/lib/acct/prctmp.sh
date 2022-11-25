#!/bin/sh
# @(#) $Revision: 66.1 $      
#	"print session record file (ctmp.h/ascii) with headings"
#	"prctmp file [heading]"
PATH=/usr/lib/acct:/bin:/usr/bin:/etc
(cat <<!; cat $*) | pr -h "SESSIONS, SORTED BY ENDING TIME"

MAJ/MIN				CONNECT SECONDS	START TIME	SESSION START
DEVICE		UID	LOGIN	PRIME	NPRIME	(NUMERIC)	DATE	TIME


!
