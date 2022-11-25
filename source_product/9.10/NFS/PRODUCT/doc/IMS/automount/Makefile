# $Source: /source/hpux_source/networking/rcs/nfs90_800/doc/IMS/automount/RCS/Makefile,v $
# $Revision: 1.2.109.1 $	$Author: gomathi $
# $State: Exp $   	$Locker:  $
# $Date: 92/04/13 16:45:15 $

DOC   = main
FINAL = port.out

print:;	soelim $(DOC) | tbl | troff -mm -rN4 -od

nroff:;	soelim $(DOC) | tbl | nroff -mm -rN4 > $(FINAL)

view:;	soelim $(DOC) | tbl | tview -mm -rN4

