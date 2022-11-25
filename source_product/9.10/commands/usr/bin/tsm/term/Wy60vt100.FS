# FacetTerm  Wy60vt100  (80/132 column)  description file 05/18/90
#    WYSE 60 in vt100 mode with 80/132 column capability - uses 2 screen pages.
#    See also wy60vt100 which is 80 column only and uses 3 screen pages.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal: ENHANCE=ON  RCV HNDSHAKE=XON/XOFF  WIDTH CHANGE CLEAR=OFF
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Wy60vt100.FS,v 70.1 92/03/09 15:55:59 ssa Exp $
use=vt100ans.fc
xon_xoff
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
### NO columns_wide_clears_screen
lines=24
##--- MULTI_PAGE ---##
pages=2
display_page_next=\E[U
display_page_prev=\E[V
use=vt100com.fc
use=vt100.fm
