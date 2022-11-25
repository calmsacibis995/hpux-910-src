# FacetTerm  Es3vt200  (80/132 column)  description file 06/13/91
#    4 page Esprit 3-in-1 in vt200 mode - single host.
#    see also Es3vt200-2 which uses 2 pages.
#    see also es3vt200 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: Mode-No_AutoPage  Setup_A(Single Host)
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Es3vt200.FS,v 70.1 92/03/09 15:53:10 ssa Exp $
xon_xoff
use=vt220wide.fc
pass-7bitmode=\E F
##--- MULTI_PAGE ---##
pages=4
display_page_number=\E[%p1%{1}%+%dW
### 24_lines_page=\EzJ0 xon_xoff=\Ezt1 auto_lf_off=\EzR0 auto_wrap_on=\EzS1
### auto_scroll=\EzU1 hidden_attr=\EzC1
enter_ca_mode=\EzJ0\Ezt1\EzR0\EzS1\EzU1\EzC1
use=vt220ans.fc
use=vt220com.fc
use=vt220attr.fc
use=vt220func.fc
use=es3vtkeys.fc
use=vt100.fm
