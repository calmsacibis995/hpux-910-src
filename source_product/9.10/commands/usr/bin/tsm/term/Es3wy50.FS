# FacetTerm  Es3wy50 (80/132 column)  description file 05/18/90
#    4 page Esprit 3-in-1 in Wyse 50 mode - single host.
#    see also Es3wy50-2 which uses 2 pages.
#    see also es3wy50 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to:  Setup_A(Single Host)
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Es3wy50.FS,v 70.1 92/03/09 15:53:20 ssa Exp $
xon_xoff
columns=80
lines=24
use=wy50com.fc
use=wysekeys.fc
use=wy50wid.fc
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
