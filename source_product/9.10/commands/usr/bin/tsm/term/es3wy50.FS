# FacetTerm  es3wy50  (80 columns only)  description file 05/18/90
#    4 page ESPRIT 3-in-1 in Wyse 50 mode - single host.
#    See also es3wy50-2 which uses 2 pages.
#    See also Es3wy50 and Es3wy50-2 for 80/132 columns.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to:  Setup_A(Single Host)
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: es3wy50.FS,v 70.1 92/03/09 15:58:05 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide_off=\E`:
pass=\E`:
lines=24
use=wy50com.fc
use=wysekeys.fc
use=wy50.fm
use=wy50prnt.fc
##--- MULTI-PAGE ---##
pages=4
display_page_number=\Ew%p1%d
##--- INIT ---##
### 24_lines_page=\EZJ0 xon_xoff=\EZt1 auto_lf_off=\EZR0 auto_wrap_on=\EZS1
### auto_scroll=\EZU1 embedded_attr=\EZC0 
### auto_page_off=\Ed*
enter_ca_mode=\EZJ0\EZt1\EZR0\EZS1\EZU1\EZC0\Ed*
