/** 	 		version_id.h   			**/

/*
 *  @(#) $Revision: 64.2 $
 *
 *  (c) Copyright Hewlett-Packard Company, 1986, 1987, 1988, 1989
 *  (c) Copyright Yokogawa-Hewlett-Packard Ltd., 1988, 1989
 *
 *  Acknowledgment is made to Dave Taylor for his creation of
 *  the original version of this software.
 *
 *  This file is included by version_id.[mstr]c.
 *  These messages are displayed by 'elm -v'.
 */

#ifdef USE_EMBEDDED_ADDRESSES 
	"From: and Reply-To: addresses are good: USE_EMBEDDED_ADDRESSES",
#else
     "From: and Reply-To: addresses are bad: not USE_EMBEDDED_ADDRESSES",
#endif

#ifdef NO_VM
	"No virtual memory calls available: NO_VM",
#endif

#ifdef DONT_TOUCH_ADDRESSES
	"Not allowed to touch addresses: DONT_TOUCH_ADDRESSES",
#endif

#ifdef ENABLE_NAMESERVER
	"User-specified name resolver enabled: ENABLE_NAMESERVER",
#else
	"User-specified name resolver disabled: not ENABLE_NAMESERVER",
#endif

#ifdef INTERNET_ADDRESS_FORMAT
	"Prefers internet address formats: INTERNET_ADDRESS_FORMAT",
#else
	"Doesn't like internet addresses: not INTERNET_ADDRESS_FORMAT",
#endif

#ifdef SITE_HIDING
	"Site hiding has been turned ON **not supported**",
#endif

#ifdef SHORTNAMES
	"Compiled with SHORTNAMES enabled *not supported*",
#endif

#ifdef DEBUG
	"Debug options are available in this version: DEBUG",
#else
	"No debug options are available: not DEBUG",
#endif

#ifdef ENCRYPTION_SUPPORTED
	"Crypt function turned ON: ENCRYPTION_SUPPORTED *not supported*",
	"     (warning: there are some serious bugs in the crypt routine)",
#else
	"crypt function turned OFF: not ENCRYPTION_SUPPORTED",
#endif

#ifdef ALLOW_MAILBOX_EDITING
	"Mailbox editing included: ALLOW_MAILBOX_EDITING *not supported*",
#else
	"Mailbox editing not included: not ALLOW_MAILBOX_EDITING",
#endif

#ifdef FORMS_MODE_SUPPORTED
	"Forms mode turned ON: FORMS_MODE_SUPPORTED *not supported*",
#else
	"Forms mode turned OFF: not FORMS_MODE_SUPPORTED",
#endif

#ifdef BOUNCEBACK_ENABLED
	"Mail bounceback turned ON: BOUNCEBACK_ENABLED",
#else
	"Mail bounceback turned OFF: not BOUNCEBACK_ENABLED",
#endif
