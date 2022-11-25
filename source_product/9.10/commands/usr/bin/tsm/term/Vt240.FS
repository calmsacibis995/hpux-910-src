# FacetTerm  Vt240  (80/132 column)  description file  06/13/91
#    graphics-pass-through set for 20/20.
#    See also vt240 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Vt240.FS,v 70.1 92/03/09 15:55:49 ssa Exp $
xon_xoff
use=vt220wide.fc
pass-7bitmode=\E F
##--- INIT ---##
pass=\E[62;1"p
use=vt240graph.fc
use=vt220ans.fc
use=vt220com.fc
use=vt220attr.fc
use=vt220func.fc
use=vt220keys.fc
use=vt100.fm
