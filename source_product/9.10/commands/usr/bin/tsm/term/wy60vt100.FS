# FacetTerm  wy60vt100  (80 column only)  description file 05/18/90
#    WYSE 60 in vt100 mode - uses 3 screen pages.
#    See also Wy60vt100 for 80/132 column using 2 screen pages.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: COLUMNS=ECON-80  ENHANCE=ON  RCV HNDSHAKE=XON/XOFF
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy60vt100.FS,v 70.1 92/03/09 16:10:02 ssa Exp $
use=vt100ans.fc
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### NOT_IN_80 columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 columns_wide_clears_screen
lines=24
##--- MULTI_PAGE ---##
pages=3
display_page_next=\E[U
display_page_prev=\E[V
use=vt100com.fc
use=vt100.fm
