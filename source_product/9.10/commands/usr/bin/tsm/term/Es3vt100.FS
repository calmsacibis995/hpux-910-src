# FacetTerm  Es3vt100  (80/132 column)  description file 05/13/91
#    4 page Esprit 3-in-1 in vt100 mode - single host.
#    see also Es3vt100-2 which uses 2 pages.
#    see also es3vt100 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: Mode-No_AutoPage  Setup_A(Single Host)
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Es3vt100.FS,v 70.1 92/03/09 15:52:58 ssa Exp $
use=vt100ans.fc
xon_xoff
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
##--- MULTI-PAGE ---##
pages=4
display_page_number=\E[%p1%{1}%+%dW
### 24_lines_page=\EzJ0 xon_xoff=\Ezt1 auto_lf_off=\EzR0 auto_wrap_on=\EzS1
### auto_scroll=\EzU1 hidden_attr=\EzC1
enter_ca_mode=\EzJ0\Ezt1\EzR0\EzS1\EzU1\EzC1
use=vt100com.fc
use=vt220func.fc
use=es3vtkeys.fc
use=vt100.fm
