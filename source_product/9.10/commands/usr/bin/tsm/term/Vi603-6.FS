# FacetTerm  Vi603-6 (80/132 column, 6 pages) description file  05/13/91
# reverse video - 6 pages - Most graphics will not work properly.
# Will only work with rev. "D" and later terminals.
#
# IMPORTANT: In setup DISPLAY, set MULTIPLE PAGES to Independent
#                                  SCREEN MODE CHANGE to Non-Destructive
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Vi603-6.FS,v 70.1 92/03/09 16:51:24 ssa Exp $
use=vt220mode7.fc
use=vi603wide.fc
use=vt220mode.fc
use=vi603com.fc
use=vi603attr.fc
use=vt220func.fc
use=vt220keys.fc
use=vi603graph.fc
delete_is_a_character
pages=6
parm_display_page_next=\E[%p1%dU
parm_display_page_prev=\E[%p1%dV
display_page_number=\E[%p1%{1}%+%d\040P
enter_ca_mode_2=\E[1 P
use=vi603.fm
