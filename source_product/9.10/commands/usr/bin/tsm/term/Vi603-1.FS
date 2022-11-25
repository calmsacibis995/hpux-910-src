# FacetTerm  Vi603-1 (80/132 column) description file  10/24/90
# reverse video - 1 page  - Some graphics will work.
#
# See also: Vi603-2 for 2 page limited graphics
#      and: Vi603-6 for 6 page limited graphics with newer "D" version terminal
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Vi603-1.FS,v 70.1 92/03/09 16:51:44 ssa Exp $
use=vt220mode7.fc
use=vi603wide.fc
use=vt220mode.fc
use=vi603com.fc
use=vi603attr.fc
use=vt220func.fc
use=vt220keys.fc
use=vi603graph.fc
delete_is_a_character
pages=1
parm_display_page_next=\E[%p1%dU
parm_display_page_prev=\E[%p1%dV
use=vi603.fm
