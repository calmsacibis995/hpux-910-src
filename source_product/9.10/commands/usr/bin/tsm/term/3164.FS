# FacetTerm  3164  description file 09/06/91
#   IBM 3164 ASCII color display station
## Copyright (c) Structured Software Solutions, Inc. 1991.  All Rights Reserved.
lines=24
use=31xxcom.fc
use=3164attr.fc
use=3164stat.fc
use=3151keys.fc
ignore-create_viewport.........=\E r!! 8"P
ignore-set_viewport_to_24_by_80=\Eq! 8"0C
pass-unknown=\E /
use=3164.fm
use=3164split.fc
use=3164ctrl.fc
enter_ca_mode-24by80=\E r!! 8"P
enter_ca_mode-24by80=\Eq! 8"0C
pass-disable_ca_transmission=\E$;
### SETUP_CANCELS_THIS enter_ca_mode-4 pages=\033q! 8"0D
### SETUP_CANCELS_THIS pages=4
### SETUP_CANCELS_THIS display_page_number=\E p%p1%'A'%+%c\Ep%p1%'A'%+%c
##-- END --##
