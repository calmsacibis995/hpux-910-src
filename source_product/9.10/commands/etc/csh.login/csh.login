
# @(#) $Revision: 70.1 $      

# Default (example of) system-wide profile file (/bin/csh initialization).
# This should be kept to the bare minimum every user needs.

	# default path for all users.
	set path=(/bin/posix /bin /usr/bin /usr/contrib/bin /usr/local/bin)
	set prompt="[\!] % "

	# default MANPATH
	setenv MANPATH /usr/man:/usr/contrib/man:/usr/local/man

	if ( -r /etc/src.csh ) then
		source /etc/src.csh		# set the TZ variable
	else
	     setenv TZ MST7MDT      # change this for local time.
	endif

 	if ( ! $?TERM ) then			# if TERM is not set,
 		setenv TERM hp			#   use the default
 	endif

# This is to meet legal requirements...

	cat /etc/copyright			# copyright message.

# Miscellaneous shell-only actions:

	if ( -f /etc/motd ) then
		cat /etc/motd			# message of the day.
	endif

	if ( -f /bin/mail ) then
		mail -e 	 		# notify if mail.

		if ( $status == 0 ) echo  "You have mail."
	endif

	if ( -f /usr/bin/news ) then
		news -n				# notify if new news.
	endif

	if ( -r /tmp/changetape ) then		# might wish to delete this:
		echo
		echo "You are the first to log in since backup:"
		echo "Please change the backup tape.\n"

		rm -f /tmp/changetape
	endif

