softkey Print_files
editrule { append("pr"); x = next; append("| lp"); }
help ".p
      You can use this command to print a file.
      By default, each page has a header showing the page number, date
      and time, and file name.
      .p
      By default, this command prints one single-spaced copy to
      standard output.
      For information on redirecting this command's
      output or using commands in a
      command pipeline, refer to the help topic
      \"Redirect_pipe\".
      .p
      For more information, refer to pr(1) and lp(1)."
{
  option double_spaced
  editrule { insert(x, "-d"); x = x+1; }
  help ".p
        This option double-spaces the output.
        By default, the output is single-spaced.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option copy_count
  help ".p
        This option lets you specify how many copies
        should be printed.  By default, one copy is printed.
        .p
        HP-UX OPTION EQUIVALENT: lp -n<number>"
  {
    string <number>
    editrule { append("-n" & argument); }
    required "Enter the number of copies to print."
    ;
  }
  option with_header
  help ".p
        This option allows you to specify the header that
        you want to appear at the top of each page in place of the
        file name.
        .p
        By default, each page has a header with
        the page number, date and time, and file name.
        .p
        HP-UX OPTION EQUIVALENT: pr -h <\"header\">"
  {
    option default disable all
    ;
    string <header>
    editrule { insert(x, "-h " & argument); x = x+1; }
    required "Enter the quoted header to print on each page."
    ;
  }
  option with_banner
  help ".p
        This option allows you to specify the title that
        you want to appear on the banner page.  (The banner
        page is printed at the beginning of your print job to
        separate it from other print jobs.)
        .p
        By default, the banner page has a title that usually includes
        your user name, the date, and the time.
        .p
        HP-UX OPTION EQUIVALENT: lp -t<\"title\">"
  {
    string <banner>
    editrule { append("-t" & argument); }
    required "Enter the quoted title to print on the banner page."
    ;
  }
  option lp_model_option disable -1
  help ".p
        This option allows you to specify printer-dependent options.
        .p
        To see what options are available for
        the printers supported on your system, look in the
        \"/usr/spool/lp/interface\" directory for the model
        script for each printer (e.g., hp2225a).
        .p
        HP-UX OPTION EQUIVALENT: lp -o<option>"
  {
    string <option>
    editrule { append("-o" & argument); }
    required "Enter a printer model option."
    ;
  }
  option to_printer
  help ".p
        This option allows you to specify to which printer
        the file should be sent.  You can use the softkey
        command \"Print_status all_info\" or the HP-UX command \"lpstat
        -t\" to see a list of available printers.
        .p
        By default, the file is sent to the default line
        printer designated by \"lp\".
        You can use the softkey
        command \"Print_status default_dest\" or the HP-UX
        command \"lpstat -d\" to see the default
        printer.
        .p
        HP-UX OPTION EQUIVALENT: lp -d<printer>"
  {
    string <dest>
    editrule { append("-d" & argument); }
    required "Enter the name of the printer."
    help ".p
          Specify to which printer the file should be sent.
          You can use the softkey
          command \"Print_status all_info\" or the HP-UX command \"lpstat
          -t\" to see a list of available printers."
    ;
  }
  string <files> disable -1 command
  editrule { insert(x, argument); x = x+1; }
  required "Enter the name of the file(s) to print."
  ;
}
            
softkey Set_file_attribs command
help ".p
      You can use this command to change the following file attributes:
      .ip * 5
      File access modes, which define read, write, and execute
      permissions for the file's owner, all members of the file's group,
      and all
      others.  For more information, refer to chmod(1).
      You can see file permissions by typing
      \"ll <filename>\" or selecting \"List files\"
      \"long format\" <filename> and looking at the
      first field of the display.
      .ip * 5
      The owner of the file.  The owner can either be a decimal user
      ID or a login name found in the \"/etc/passwd\" file.
      Unless you are the superuser, you can only change the ownership
      of a file you own.  For more information, refer to
      chown(1).
      You can see the file's owner by typing
      \"ll <filename>\" or selecting \"List files\"
      \"long format\" <filename> and looking at the
      third field of the display.
      .ip * 5
      The group ID of the file.  The group ID can either be a decimal
      group ID or a group name found in \"/etc/group\".
      Unless you are the superuser, you can only change the group
      of a file you own.  For more information, refer to
      chgrp(1).
      You can see the file's group by typing
      \"ll <filename>\" or selecting \"List files\"
      \"long format\" <filename> and looking at the
      fourth field of the display."
{
  option mode disable all
  editrule { append("chmod"); append(""); c = ""; }
  required "Select \"mode\", \"owner\", or \"group\"."
  help ".p
        This option allows you to change file access modes, which define
        read, write, and execute
        permissions for the file's owner, all members of the file's group,
        and all others.
        You can see file modes by typing
        \"ll <filename>\" or selecting \"List files\"
        \"long format\" <filename> and looking at the
        first field of the display.
        .p
        For more information, refer to chmod(1)."
  {
    option recurs+ively
    editrule { insert(last, "-R"); }
    help ".p
          This option allows you to change the permissions of
          all files in a directory and any sub-directories at once.
          .p
          HP-UX OPTION EQUIVALENT: -R"
    ;
    option readable_by enable all
    required "Select file modes."
    help ".p
          This option allows you to select who
          can read the file: no one, the file's owner,
          the file's owner and all members of the file's group, or all
          users."
    {
      option none disable all
      editrule { word[last] &= c & "-r"; c = ","; }
      required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
      help ".p
            This option prevents anyone from reading the file."
      ;
      option owner disable all
      editrule { word[last] &= c & "-r,u+r"; c = ","; }
      help ".p
            This option allows only the file's owner to read the file."
      ;
      option owner_group disable all
      editrule { word[last] &= c & "-r,ug+r"; c = ","; }
      help ".p
            This option allows the file's owner and all members of
            the file's group to read the file."
      ;
      option all
      editrule { word[last] &= c & "+r"; c = ","; }
      help ".p
            This option allows anyone to read the file."
      ;
    }
    option writable_by enable all
    help ".p
          This option allows you to select who
          can write to the file: no one, the file's owner,
          the file's owner and all members of the file's group, or all
          users."
    {
      option none disable all
      editrule { word[last] &= c & "-w"; c = ","; }
      required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
      help ".p
            This option prevents anyone from writing to the file."
      ;
      option owner disable all
      editrule { word[last] &= c & "-w,u+w"; c = ","; }
      help ".p
            This option allows only the file's owner to write to the file."
      ;
      option owner_group disable all
      editrule { word[last] &= c & "-w,ug+w"; c = ","; }
      help ".p
            This option allows the file's owner and all members of
            the file's group to write to the file."
      ;
      option all
      editrule { word[last] &= c & "+w"; c = ","; }
      help ".p
            This option allows anyone to write to the file."
      ;
    }
    option executa+ble_by enable all
    help ".p
          This option allows you to select who
          can execute the file: no one, the file's owner,
          the file's owner and all members of the file's group, or all
          users.
          .p
          Note that you must have execute permission for a directory
          to search through that directory."
    {
      option none disable all
      editrule { word[last] &= c & "-x"; c = ","; }
      required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
      help ".p
            This option prevents anyone from executing this
            file.  If the file is a directory, no one can
            search through it."
      ;
      option owner disable all
      editrule { word[last] &= c & "-x,u+x"; c = ","; }
      help ".p
            This option allows only the file's owner to
            execute the file.  If the file is a directory, only
            the owner can search through it."
      ;
      option owner_group disable all
      editrule { word[last] &= c & "-x,ug+x"; c = ","; }
      help ".p
            This option allows the file's owner and all members of the
            file's group to execute the file.
            If the file is a directory, the file's owner and all
            members of the file's group
            can search through it."
      ;
      option all
      editrule { word[last] &= c & "+x"; c = ","; }
      help ".p
            This option allows anyone to execute the file.
            If the file is a directory, anyone can
            search through it."
      ;
    }
    string <files> disabled disable -1
    editrule { append(argument); }
    required "Enter the name of the file(s) or dir(s) to change."
    ;
  }
  option owner disable all
  editrule { append("chown"); }
  help ".p
        This option allows you to change the owner of the file.
        The owner can either be a decimal user
        ID or a login name found in the \"/etc/passwd\" file.
        Unless you are the superuser, you can only change the ownership
        of a file you own.
        You can see the file's owner by typing
        \"ll <filename>\" or selecting \"List files\"
        \"long format\" <filename> and looking at the
        third field of the display.
        .p
        For more information, refer to chown(1)."
  {
    option recurs+ively
    editrule { append("-R"); }
    help ".p
          This option allows you to change the owner of all files
          in a directory and any sub-directories at once.
          .p
          HP-UX OPTION EQUIVALENT: -R"
    ;
    string <user> enable all
    editrule { append(argument); }
    required "Enter the name of the user."
    ;
    string <files> disable -1 disabled
    editrule { append(argument); }
    required "Then, enter the name of the file(s) or directory(s) to change."
    ;
  }
  option group disable all
  editrule { append("chgrp"); }
  help ".p
        This option allows you to change the
        group ID of the file.  The group ID can either be a decimal
        group ID or a group name found in \"/etc/group\".
        Unless you are the superuser, you can only change the group
        of a file you own.
        You can see the file's group by typing
        \"ll <filename>\" or selecting \"List files\"
        \"long format\" <filename> and looking at the
        fourth field of the display.
        .p
        For more information, refer to chgrp(1)."
  {
    option recurs+ively
    editrule { append("-R"); }
    help ".p
          This option allows you to change the group of all files
          in a directory and any sub-directories at once.
          .p
          HP-UX OPTION EQUIVALENT: -R"
    ;
    string <group> enable all
    editrule { append(argument); }
    required "Enter the group name."
    ;
    string <files> disable -1 disabled
    editrule { append(argument); }
    required "Then, enter the name of the file(s) or directory(s) to change."
    ;
  }
}
            
softkey Switch command
editrule { append("/usr/tsm/bin/tsm.command"); }
help ".p
      You can use this command to switch sessions within the
      Terminal Session Manager.
      .p
      For more information, refer to tsm.command(1)."
{
  option 1 disable all automatic
  editrule { append(argument); }
  required "Select the number of the TSM session to switch to."
  ;
  option 2 disable all automatic
  editrule { append(argument); }
  ;
  option 3 disable all automatic
  editrule { append(argument); }
  ;
  option 4 disable all automatic
  editrule { append(argument); }
  ;
  option 5 disable all automatic
  editrule { append(argument); }
  ;
  option 6 disable all automatic
  editrule { append(argument); }
  ;
  option 7 disable all automatic
  editrule { append(argument); }
  ;
  option 8 disable all automatic
  editrule { append(argument); }
  ;
  option 9 disable all automatic
  editrule { append(argument); }
  ;
  option 10 disable all automatic
  editrule { append(argument); }
  ;
}
            
softkey adjust
editrule { append("adjust"); }
help ".p
      You can use this command to format a text file.  You can fill,
      center, and justify the text.
      .p
      By default, leading blanks are converted to tabs and
      the text is left-justified.  Results are written to
      standard output.
      For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      For more information, refer to adjust(1)."
{
  option without_tabs
  editrule { dash("b"); }
  help ".p
        This option prevents leading blanks from being
        converted to tabs.  By default, they are converted.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
  option right_justify disable 2
  editrule { dash("r"); }
  help ".p
        This option fills and right-justifies the text.  By default, text is
        filled and left-justified.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option flush disable 1
  editrule { dash("j"); }
  help ".p
        This option fills and right- and left-justifies the text.  By
        default, text is filled and left-justified.
        .p
        HP-UX OPTION EQUIVALENT: -j"
  ;
  option centered disable 1 enable 2
  editrule { dash("c"); }
  help ".p
        This option centers the text but does not fill it.  By
        default, text is filled and left-justified.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option display_width
  help ".p
        This option allows you to specify the output display
        width.  By default, the width is 72 characters.
        .p
        HP-UX OPTION EQUIVALENT: -m<number>"
  {
    string <number>
    editrule { append("-m" & argument); }
    required "Enter the display width."
    help ".p
          Specify the output display width.  By default, the width is
          72 characters.
          .p
          HP-UX OPTION EQUIVALENT: -m<number>"
    ;
  }
  option about_column disabled
  help ".p
        This option allows you to specify which column should be the
        center-point for centering lines.
        By default, lines are centered about column 40.
        .p
        HP-UX OPTION EQUIVALENT: -m<number>"
  {
    string <number>
    editrule { append("-m" & argument); }
    required "Enter the column number to center lines about."
    help ".p
          Specify which column should be the
          center-point for centering lines.
          By default, lines are centered about column 40.
          .p
          HP-UX OPTION EQUIVALENT: -m<number>"
    ;
  }
  string <files> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to adjust."
  ;
}
            
softkey ar command
editrule { append("ar"); }
help ".p
      You can use this command to create and maintain a
      library archive of object files.
      .p
      Individual object files are inserted without conversion
      into the archive file.
      .p
      For more information, refer to ar(1) and ar(4)."
{
  option table_of_contents disable all
  editrule { append("t"); }
  required "Select an archive function."
  help ".p
        This option prints a table of contents showing all
        files in the archive.  When used with the
        \"verbosely\" option, it provides a long listing of all
        information about the files in the archive.
        .p
        HP-UX OPTION EQUIVALENT: t"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option provides a long listing of all
          information about the files in the archive.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option rebuild_sym_tab
    editrule { word[last] &= "s"; }
    help ".p
          This option forces the archive symbol table to be rebuilt.
          This option is useful for restoring the archive symbol
          table after you have used the strip(1) command on the
          archive.
          .p
          You cannot access or directly use the archive symbol table.
          It is used by the link editor (ld(1)) to search through
          libraries of object files.
          .p
          HP-UX OPTION EQUIVALENT: s"
    ;
    string <lib> enable all
    editrule { append(argument); }
    required "Enter the name of the archive library."
    ;
    string <objs> disabled disable -1
    editrule { append(argument); }
    hint "Enter the name of the object file(s) to list."
    ;
  }
  option delete_objs disable all
  editrule { append("d"); }
  help ".p
        This option allows you to delete object files from an
        archive library.
        .p
        HP-UX OPTION EQUIVALENT: d"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file being
          deleted.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    string <lib> enable all
    editrule { append(argument); }
    required "Enter the name of the archive library."
    ;
    string <objs> disabled disable -1
    editrule { append(argument); }
    required "Then, enter the name of the object file(s) to delete."
    ;
  }
  option archive_objs disable all
  editrule { append("r"); x = last; }
  help ".p
        This option allows you to add new object files to a
        library archive.  If the archive file does not already exist,
        it is created.  By default, new files are added to the end
        of an archive file.
        .p
        HP-UX OPTION EQUIVALENT: r"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file, preceded by
          an \"a\" if the file is being added or an \"r\" if the
          file is being replaced.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option and_place
    help ".p
          This option allows you to specify where
          in the archive file the object file should be placed.
          By default, the object file is added to the end
          of the archive."
    {
      option as_last_obj disable all
      required "Select where the object file(s) should be placed."
      help ".p
            This option specifies that the object file(s) should be
            placed at the end of the archive.  This is the
            default behavior."
      ;
      option before_obj disable all
      help ".p
            This option specifies that the object file
            should be moved to before an existing object file.
            .p
            HP-UX OPTION EQUIVALENT: b <filename>"
      {
        string <obj>
        editrule { word[last] &= "b"; append(argument); }
        required "Enter the name of an existing object file."
        ;
      }
      option after_obj disable all
      help ".p
            This option specifies that the object file
            should be placed after an existing object file.
            .p
            HP-UX OPTION EQUIVALENT: a <filename>"
      {
        string <obj>
        editrule { word[last] &= "a"; append(argument); }
        required "Enter the name of an existing object file."
        ;
      }
    }
    option create_archive
    editrule { word[x] &= "c"; }
    help ".p
          This option suppresses the usual warning message
          generated when you create a new library archive.
          .p
          HP-UX OPTION EQUIVALENT: c"
    ;
    string <lib> enable all
    editrule { append(argument); }
    required "Enter the name of the archive library."
    ;
    string <objs> disabled disable -1
    editrule { append(argument); }
    required "Then, enter the name of the object file(s) to archive."
    ;
  }
  option extract_objs disable all
  editrule { append("x"); }
  help ".p
        This option allows you to extract object files
        from a library archive.
        .p
        HP-UX OPTION EQUIVALENT: x"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file being
          extracted.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option rebuild_sym_tab
    editrule { word[last] &= "s"; }
    help ".p
          This option forces the archive symbol table to be rebuilt.
          This option is useful for restoring the archive symbol
          table after you have used the strip(1) command on the
          archive.
          .p
          You cannot access or directly use the archive symbol table.
          It is used by the link editor (ld(1)) to search through
          libraries of object files.
          .p
          HP-UX OPTION EQUIVALENT: s"
    ;
    string <lib> enable all
    editrule { append(argument); }
    required "Enter the name of the archive library."
    ;
    string <objs> disabled disable -1
    editrule { append(argument); }
    hint "Enter the name of the object file(s) to extract."
    ;
  }
  option print_objs disable all
  editrule { append("p"); }
  help ".p
        This option allows you to print files in the library
        archive to standard output.
        .p
        HP-UX OPTION EQUIVALENT: p"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file being
          printed.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option rebuild_sym_tab
    editrule { word[last] &= "s"; }
    help ".p
          This option forces the archive symbol table to be rebuilt.
          This option is useful for restoring the archive symbol
          table after you have used the strip(1) command on the
          archive.
          .p
          You cannot access or directly use the archive symbol table.
          It is used by the link editor (ld(1)) to search through
          libraries of object files.
          .p
          HP-UX OPTION EQUIVALENT: s"
    ;
    string <lib> enable all
    editrule { append(argument); }
    required "Enter the name of the archive library."
    ;
    string <objs> disabled disable -1
    editrule { append(argument); }
    hint "Enter the name of the object file(s) to print."
    ;
  }
  option move_objs disable all
  editrule { append("m"); }
  help ".p
        This option allows you to move the specified object file(s)
        within a library archive.  By default, the file(s) are moved to
        the end of the archive.
        .p
        HP-UX OPTION EQUIVALENT: m"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file being
          moved.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option to_last_obj disable 2 enable all
    required "Select where the object file(s) should be moved to."
    help ".p
          This option specifies that the object file(s) should be
          moved to the end of the archive.  This is the
          default behavior."
    ;
    option before_obj disable 1 enable all
    help ".p
          This option specifies that the object file
          should be moved to before an existing object file.
          .p
          HP-UX OPTION EQUIVALENT: b <filename>"
    {
      string <obj>
      editrule { word[last] &= "b"; append(argument); }
      required "Enter the name of an existing object file."
      ;
    }
    option after_obj enable all
    help ".p
          This option specifies that the object file
          should be moved to after an existing object file.
          .p
          HP-UX OPTION EQUIVALENT: a <filename>"
    {
      string <obj>
      editrule { word[last] &= "a"; append(argument); }
      required "Enter the name of an existing object file."
      ;
    }
    string <lib> disabled enable all
    editrule { append(argument); }
    required "Enter the name of the archive library."
    ;
    string <objs> disabled disable -1
    editrule { append(argument); }
    required "Then, enter the name of the object file(s) to move."
    ;
  }
}
            
softkey bdf command
editrule { append("bdf"); }
help ".p
      You can use this command to display the number of
      kilobytes of free disk space available on a file
      system.
      By default, the free disk space is displayed for all of the
      normally mounted file systems.
      .p
      For more information, refer to bdf(1)."
{
  option with_inodes
  editrule { append("-i"); }
  help ".p
        This option also displays the number of used and
        free inodes.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option with_swapfs
  editrule { append("-b"); }
  help ".p
        This option also displays the state of any
        file systems currently being used for dynamic swap.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
  option hfs_only disable all
  editrule { append("-t hfs"); }
  help ".p
        This option displays the free disk space on all HFS
        mounted file systems.
        .p
        HP-UX OPTION EQUIVALENT: -t hfs"
  ;
  option nfs_only disable all
  editrule { append("-t nfs"); }
  help ".p
        This option displays the free disk space on all
        NFS-mounted file systems.
        .p
        HP-UX OPTION EQUIVALENT: -t nfs"
  ;
  option cdfs_only disable all
  editrule { append("-t cdfs"); }
  help ".p
        This option displays the free disk space on all
        CD-ROM mounted file systems.
        .p
        HP-UX OPTION EQUIVALENT: -t cdfs"
  ;
  string <files> disable -1
  editrule { append(argument); }
  hint "Enter the name of the block-special device file(s) or directory(s)."
  ;
}
            
softkey cal command
editrule { append("cal"); y = "`date +%Y`"; }
cleanuprule { append(y); }
help ".p
      You can use this command to display a calendar for any
      month or year.  By default, it displays the current month.
      .p
      Note that specifying \"90\" as the year will give you
      a calendar for the year 90.  Specifying \"1990\" will
      give you a calendar for the year 1990.
      .p
      For more information, refer to cal(1)."
{
  option for_month enable all
  help ".p
        This option lets you specify the month for which you
        want a calendar.  By default, the current month
        is displayed.
        .p
        HP-UX OPTION EQUIVALENT: <n>, where n equals the number of
        the month.  For example, 5 is May."
  {
    option january disable all
    editrule { append("1"); }
    required "Select a month."
    ;
    option february disable all
    editrule { append("2"); }
    ;
    option march disable all
    editrule { append("3"); }
    ;
    option april disable all
    editrule { append("4"); }
    ;
    option may disable all
    editrule { append("5"); }
    ;
    option june disable all
    editrule { append("6"); }
    ;
    option july disable all
    editrule { append("7"); }
    ;
    option august disable all
    editrule { append("8"); }
    ;
    option septem+ber disable all
    editrule { append("9"); }
    ;
    option october disable all
    editrule { append("10"); }
    ;
    option november disable all
    editrule { append("11"); }
    ;
    option december disable all
    editrule { append("12"); }
    ;
  }
  string <year>
  editrule { y = argument; }
  hint "Enter the year."
  help ".p
        Note that specifying \"90\" as the year will give you
        a calendar for the year 90.  Specifying \"1990\" will
        give you a calendar for the year 1990."
  ;
}
            
softkey cancel command
editrule { append("cancel"); }
help ".p
      You can use this command to cancel print requests
      that were made with the \"Print_files\" softkey command or
      the \"lp\" command.
      .p
      For more information, refer to cancel(1)."
{
  option all_requests disable all
  editrule { d = "`lpstat -d | awk '{ print $NF }'`"; }
  cleanuprule { append(d); append("-a"); }
  help ".p
        This option lets you cancel all of the outstanding print requests
        you have made.
        .p
        HP-UX OPTION EQUIVALENT: -a"
  {
    option on_printer
    help ".p
          This option lets you cancel all of the
          print requests you have made on a particular local
          or remote printer.
          You can use the softkey
          command \"Print_status all_info\" or the HP-UX command \"lpstat
          -t\" to see a list of available printers."
    {
      string <dest>
      editrule { d = argument; }
      required
        "Enter the name of the printer whose requests should be cancelled."
      ;
    }
  }
  string <req_id> disable all
  editrule { append(argument); }
  required "Enter the ID of the request to cancel."
  help ".p
        If you enter a request ID for a local printer,
        the associated request is cancelled.
        If you enter a request ID for a remote printer, the
        associated request is cancelled, provided you own the request.
        .p
        Use the \"lpstat\" command to see request IDs."
  ;
}
            
softkey cat
editrule { append("cat"); }
help ".p
      You can use this command to concatenate
      and display files.  By default, files are written to
      the standard output.
      .p
      For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      For more information, refer to cat(1)."
{
  option visible enable all
  editrule { dash("v"); }
  help ".p
        This option displays non-printing characters
    in a visible representation (except for tabs, newlines, and formfeeds).
        .p
        HP-UX OPTION EQUIVALENT: -v"
  ;
  option show_tabs enable all disabled
  editrule { dash("t"); }
  help "When used with the \"visible\" option, this option
        displays tabs as a \"^I\".
        .p
        HP-UX OPTION EQUIVALENT: -t"
  ;
  option show_newlines disabled
  editrule { dash("e"); }
  help "When used with the \"visible\" option, this option
        displays newlines as a \"$\".
        .p
        HP-UX OPTION EQUIVALENT: -e"
  ;
  option un+buffered
  editrule { dash("u"); }
  help ".p
        This option causes unbuffered output
        (displayed character-by-character).  By default, output is buffered.
        .p
        HP-UX OPTION EQUIVALENT: -u"
  ;
  option ignore_errors
  editrule { dash("s"); }
  help ".p
        This option ignores errors
        such as non-existent files, identical input and output,
        and write errors.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  string <files> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to concatenate."
  ;
}
            
softkey cd command
editrule { append("cd"); }
help ".p
      You can use this command to change your
      current working directory.  By default, your current
      directory is changed to your home directory.
      .p
      For more information, refer to cd(1)."
{
  option parent_dir disable all
  editrule { append(".."); }
  help ".p
        This option moves you to the parent directory, which
        is the directory immediately above the current directory.
        For example, if you are currently in \"/users/jodi\", the parent
        directory would be \"/users\".
        .p
        HP-UX OPTION EQUIVALENT: .."
  ;
  option previous_dir disable all
  editrule { append("-"); }
  help ".p
        This option moves you back to the last directory
        you were in.
        .p
        HP-UX OPTION EQUIVALENT: -"
  ;
  option user_dir disable all
  help ".p
        This option moves you to the specified user's home directory.
        .p
        HP-UX OPTION EQUIVALENT: ~<username>"
  {
    string <user>
    editrule { append("~" & argument); }
    required
      "Enter the name of the user whose home directory should be moved to."
    ;
  }
  string <dir>
  editrule { append(argument); }
  hint "Enter the name of the directory to move to."
  ;
}
            
softkey cdb command
editrule { append("cdb"); }
help ".p
      You can use this command to perform source
      level debugging of a C program.  The debugger is interactive
      and complex.  If problems occur, consult a manual
      that describes the program or the system.
      .p
      For more information, refer to cdb(1)."
{
  option source_dir disable -1
  help ".p
        This option allows you to specify an alternate
        directory for source files.  Alternate directories are
        searched in the order given.  If a source file is not found
        in any of the alternate directories, the current directory
        is searched last.
        .p
        HP-UX OPTION EQUIVALENT: -d<dir>"
  {
    string <dir>
    editrule { append("-d" & argument); }
    required "Enter the name of a source directory."
    ;
  }
  option record_file
  help ".p
        This option allows you to specify a record
        file that is invoked immediately for overwrite.
        .p
        Then, all commands entered by the user are saved
        in this file for future playback until record mode
        is turned off with the debugger command \">f\".
        .p
        HP-UX OPTION EQUIVALENT: -r<filename>"
  {
    string <file>
    editrule { append("-r" & argument); }
    required "Enter the name of the file to record to."
    ;
  }
  option playback_file
  help ".p
        This option allows you to specify a playback
        file that is invoked immediately.
        .p
        All commands are read from this file before
        you can enter any other commands.
        .p
        HP-UX OPTION EQUIVALENT: -p<filename>"
  {
    string <file>
    editrule { append("-p" & argument); }
    required "Enter the name of the file to play back from."
    ;
  }
  option adopt_process disable 1
  help ".p
        This option allows you to specify the
        process ID of an existing process that you wish to adopt
        and debug.
        .p
        HP-UX OPTION EQUIVALENT: -P<filename>"
  {
    string <pid>
    editrule { append("-P" & argument); }
    required "Enter the pid of the process to adopt and debug."
    ;
  }
  option redirect_to
  help ".p
        This option allows you to redirect the standard
        input, standard output, and standard error to a file.
        .p
        HP-UX OPTION EQUIVALENT: -i <filename> -o <filename> -e <filename>"
  {
    string <file>
    editrule { append("-i" & argument);
               append("-o" & argument);
               append("-e" & argument); }
    required "Enter the name of the file to redirect program i/o to."
    ;
  }
  string <prog> enable all
  editrule { append(argument); }
  required "Enter the name of the program to debug."
  ;
  string <core> disabled
  editrule { append(argument); }
  hint "Enter the name of the core file."
  ;
}
            
softkey chatr command
editrule { append("chatr"); }
help ".p
      You can use this command to change a program's
      internal attributes.
      .p
      For more information, refer to chatr(1)."
{
  option silently
  editrule { append("-s"); }
  help ".p
        This option does not display
        each file's magic number and file attributes.
        By default, this information is displayed.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option shared disable 1 enable all
  editrule { append("-n"); }
  required "Select \"shared\" or \"demand loaded\"."
  help ".p
        This option changes a file from demand loaded to shared.
        .p
        HP-UX OPTION EQUIVALENT: -n"
  ;
  option demand_loaded enable all
  editrule { append("-q"); }
  help ".p
        This option changes a file from shared to demand loaded.
        .p
        HP-UX OPTION EQUIVALENT: -q"
  ;
  string <files> disabled disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to change."
  ;
}
            
softkey chgrp command
editrule { append("chgrp"); }
help ".p
      You can use this command to change the group ID of a file.
      The group ID can either be a decimal group ID
      or a group name found in \"/etc/group\".
      You can see the file's group by typing
      \"ll <filename>\" or selecting \"List files\"
      \"long format\" <filename> and looking at the
      fourth field of the display.
      .p
      For more information, refer to chgrp(1)."
{
  option recurs+ively
  editrule { append("-R"); }
  help ".p
        This option allows you to change the group ID of
        all files in a directory and any sub-directories
        at once.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  string <group> enable all
  editrule { append(argument); }
  required "Enter the group name."
  ;
  string <files> disable -1 disabled
  editrule { append(argument); }
  required "Then, enter the name of the file(s) or directory(s) to change."
  ;
}
            
softkey chmod command
editrule { append("chmod"); append(""); c = ""; }
help ".p
      You can use this command to change the permissions of
      a file.  You can specify read, write, and execute permissions
      for the file's owner, all members of the file's group, and all others.
      .P
      You can see file permissions by typing
      \"ll <filename>\" or selecting \"List files\"
      \"long format\" <filename> and looking at the
      first field in the display.
      .p
      For more information, refer to chmod(1)."
{
  option recurs+ively
  editrule { insert(last, "-R"); }
  help ".p
        This option allows you to change the permissions of
        all files in a directory and its subdirectories at once.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  option readable_by enable all
  required "Select file modes."
  help ".p
        This option allows you to specify who
        can read the file: no one, the file's
        owner, the file's owner and all members of the file's group,
        or all users."
  {
    option none disable all
    editrule { word[last] &= c & "-r"; c = ","; }
    help ".p
          This option prevents anyone from reading the file."
    required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
    ;
    option owner disable all
    editrule { word[last] &= c & "-r,u+r"; c = ","; }
    help ".p
          This option allows only the file's owner to read the file."
    ;
    option owner_group disable all
    editrule { word[last] &= c & "-r,ug+r"; c = ","; }
    help ".p
          This option allows the file's owner and all members of
          the file's group to read the file."
    ;
    option all
    editrule { word[last] &= c & "+r"; c = ","; }
    help ".p
          This option allows anyone to read the file."
    ;
  }
  option writable_by enable all
  help ".p
        This option allows you to specify who
        can write to the file: no one, the file's
        owner, the file's owner and all members of the file's group,
        or all users."
  {
    option none disable all
    editrule { word[last] &= c & "-w"; c = ","; }
    required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
    help ".p
          This option prevents anyone from writing to the file."
    ;
    option owner disable all
    editrule { word[last] &= c & "-w,u+w"; c = ","; }
    help ".p
          This option allows only the file's owner to write to the file."
    ;
    option owner_group disable all
    editrule { word[last] &= c & "-w,ug+w"; c = ","; }
    help ".p
          This option allows the file's owner and all members of the
          file's group to write to the file."
    ;
    option all
    editrule { word[last] &= c & "+w"; c = ","; }
    help ".p
          This option allows anyone to write to the file."
    ;
  }
  option executa+ble_by enable all
  help ".p
        This option allows you to specify who
        can execute the file: no one, the file's
        owner, the file's owner and all members of the file's group,
        or all users.
        .p
        Note that you must have execute permission for a directory
        to search through that directory."
  {
    option none disable all
    editrule { word[last] &= c & "-x"; c = ","; }
    required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
    help ".p
          This option prevents anyone from executing the file.
          If the file is a directory, no one can search through it."
    ;
    option owner disable all
    editrule { word[last] &= c & "-x,u+x"; c = ","; }
    help ".p
          This option allows only the file's owner to execute the file.
          If the file is a directory, only the owner can search through it."
    ;
    option owner_group disable all
    editrule { word[last] &= c & "-x,ug+x"; c = ","; }
    help ".p
          This option allows the file's owner and anyone in the file's
          group to execute the file.
          If the file is a directory, the file's owner and all
          members of the file's group
          can search through it."
    ;
    option all
    editrule { word[last] &= c & "+x"; c = ","; }
    help ".p
          This option allows anyone to execute the file.
          If the file is a directory, anyone can search through it."
    ;
  }
  string <files> disabled disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) or dir(s) to change."
  ;
}
            
softkey chown command
editrule { append("chown"); }
help ".p
      You can use this command to change the owner
      of a file.
      .p
      The owner can be either a decimal user
      ID or a login name found in the \"/etc/passwd\" file.
      You can see the file's owner by typing
      \"ll <filename>\" or selecting \"List files\"
      \"long format\" <filename> and looking at the
      third field of the display.
      .p
      For more information, refer to chown(1)."
{
  option recurs+ively
  editrule { append("-R"); }
  help ".p
        This option allows you to change the owner of
        all files in a directory and any subdirectories
        at once.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  string <user> enable all
  editrule { append(argument); }
  required "Enter the name of the user."
  ;
  string <files> disable -1 disabled
  editrule { append(argument); }
  required "Then, enter the name of the file(s) or directory(s) to change."
  ;
}
            
softkey cmp
editrule { append("cmp"); }
help ".p
      You can use this command to compare two files.  By
      default, \"cmp\" makes no comment if the files are the same;
      if they differ, it displays the byte and line number at
      which the difference occurred.
      .p
      For more information, refer to cmp(1)."
{
  option silently disable 1
  editrule { dash("s"); }
  help ".p
        This option displays nothing for differing files;
        only an exit value is returned.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option all_chars
  editrule { dash("l"); }
  help ".p
        This option displays the byte number (decimal) and the
        differing bytes (octal) for each difference, rather
        than stopping at the first difference.
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
  string <file1> enable all
  editrule { append(argument); }
  required "Enter the name of the first file to compare."
  ;
  string <file2> disabled
  editrule { append(argument); }
  required "Then, enter the name of the second file to compare."
  ;
}
            
softkey col
editrule { append("col"); }
help ".p
      You can use this command to filter reverse line-feeds
      and backspaces.  \"col\" is normally used with nroff(1).
      .p
      For more information, refer to col(1)."
{
  option no_tabs
  editrule { dash("x"); }
  help ".p
        This option suppresses the automatic conversion of white space
        to tabs in the output.
        .p
        HP-UX OPTION EQUIVALENT: -x"
  ;
  option no_over+strike
  editrule { dash("b"); }
  help ".p
        This option assumes that the output device cannot
        backspace.  Thus, if two or more characters are to appear in
        the same place, only the last one will be output.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
}
            
softkey comm
editrule { append("comm"); a = "1"; b = "2"; c = "3"; }
help ".p
      You can use this command to compare two sorted files.
      By default, \"comm\" produces a three-column report: lines
      unique to the first file, lines unique to the second,
      and lines common to both.
      .p
      For more information, refer to comm(1)."
{
  option unique_to_file1
  editrule { a = ""; }
  help ".p
        This option displays those lines that are
        unique to the first file.  By default, the lines unique
        to either file and the common lines are displayed."
  ;
  option unique_to_file2
  editrule { b = ""; }
  help ".p
        This option displays those lines that are
        unique to the second file.  By default, the lines unique
        to either file and the common lines are displayed."
  ;
  option common_to_both
  editrule { c = ""; }
  help ".p
        This option displays those lines that are
        common to both files.  By default, the lines unique
        to either file and the common lines are displayed."
  ;
  string <file1> enable all
  editrule { if (a+b+c == 6) { a = ""; b = ""; c = ""; }
             append("-" & a & b & c); append(argument); }
  required "Enter the name of the first file to compare."
  ;
  string <file2> disabled
  editrule { append(argument); }
  required "Then, enter the name of the second file to compare."
  ;
}
softkey cp command
editrule { append("cp"); }
help ".p
      You can use this command to copy a file to
      a new or existing file, or to an existing directory.
      You can also copy a directory subtree to a
      new or existing directory.
      .ip * 5
      If you copy a file to an existing file, the existing file
      is overwritten.
      .ip * 5
      If you want to copy more than one file at a time, you must
      copy the files to an existing directory rather than another file.
      .ip * 5
      If you want to copy one or more directory
      subtrees to a new or existing directory, you must
      use the \"recursively\" option.
      .p
      For more information, refer to cp(1)."
{
  option recurs+ively
  editrule { dash("r"); }
  help ".p
        This option allows you to copy the directory subtree
        rooted at the specified directory(s) to a
        new or existing directory.
        .p
        If the destination directory exists, a
        subdirectory with the same name as the source
        directory will be created within it.  The
        subtree rooted at the source directory will be copied to
        this subdirectory.
        .p
        If the destination directory does not exist,
        it is created and the source directory
        subtree is copied to it.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option interact+ively disable 1
  editrule { dash("i"); }
  help ".p
        This option prompts you to verify that a file
        should be copied when that copy would
        overwrite an existing file.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option force+fully
  editrule { dash("f"); }
  help ".p
        This option removes any existing file
        with the same name and directory
        location as the file created by copying.
        It then writes a copy of the new file
        to that name and location.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
              
  string <files> disable -1 enable all
  editrule { append(argument); }
  required "Enter the name of the file(s) or dir(s) to copy."
  ;
  option to disabled
  required
    "Enter the name of the file(s) or dir(s) to copy; then select \"to\"."
  {
    string <dest>
    editrule { append(argument); }
    required
      "Then, enter the name of the file or directory to copy the file(s) to."
    ;
  }
}
            
softkey cpio
editrule { append("cpio"); }
help ".p
      You can use this command to copy files to an archive
      or extract files from an archive.
      .p
      You can use this command with \"tcio\"(1) to
      improve throughput and reduce wear and tear on tape
      cartridges and drives.
      For information on using commands in a
      command pipeline, refer to the help topic
      \"Redirect_pipe\".
      .p
      For more information, refer to cpio(1)."
{
  option table_of_contents disable all
  required "Select an archive function."
  editrule { append("-it"); }
  help ".p
        This option prints a table of contents of the
        archive.  No files are created, read, or copied.
        .p
        HP-UX OPTION EQUIVALENT: -it"
  {
    option verbose+ly
    editrule { dash("v"); }
    help ".p
          This option provides a long listing of all
          information about the files in the archive.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    option ascii_headers
    editrule { dash("c"); }
    help ".p
          This option writes and reads header information
          in a more portable ASCII character form.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
    option resynch+ronize
    editrule { dash("R"); }
    help ".p
          This option resynchronizes automatically if
          \"cpio\" gets \"out of phase\".  \"cpio\" will
          attempt to find the next good header in the archive and
          continue processing from there.
          .p
          HP-UX OPTION EQUIVALENT: -R"
    ;
    option not_matching
    editrule { dash("f"); }
    help ".p
          This option copies all files except those that
          match the specified pattern.
          For information about file pattern matching, refer to
          the help topic \"Regexp_pattern\".
          .p
          HP-UX OPTION EQUIVALENT: -f <\"pattern\">"
    ;
    string <pattrn>
    editrule { append(argument); }
    hint "Enter the quoted file pattern to match listed files."
    help ".p
          For information about file pattern matching, refer to
          the help topic \"Regexp_pattern\"."
    ;
  }
  option input_files disable all
  editrule { append("-id"); }
  help ".p
        This option extracts files from the standard input,
        which is assumed to be the product of a previous \"cpio -o\"
        or \"cpio\" \"output_files\".
        .p
        By default, all files are selected.
        .p
        HP-UX OPTION EQUIVALENT: -id"
  {
    option verbose+ly
    editrule { dash("v"); }
    help ".p
          This option displays a list of file names being extracted.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    option retain_mtime
    editrule { dash("m"); }
    help ".p
          This option retains the previous file modification
          times written to the archive, rather than setting them to
          the time when you extract the files.
          .p
          HP-UX OPTION EQUIVALENT: -m"
    ;
    option ascii_headers
    editrule { dash("c"); }
    help ".p
          This option writes and reads header information
          in a more portable ASCII character form.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
    option rename_files
    editrule { dash("r"); }
    help ".p
          This option allows you to rename files
          interactively.  If you press \"Return\" instead of typing a name,
          \"cpio\" skips handling the file.
          .p
          HP-UX OPTION EQUIVALENT: -r"
    ;
    option uncondi+tionally
    editrule { dash("u"); }
    help ".p
          This option copies files unconditionally, so that
          an older file can overwrite a newer file with the same name.
          By default, the newer file is kept.
          .p
          HP-UX OPTION EQUIVALENT: -u"
    ;
    option create_devices
    editrule { dash("x"); }
    help ".p
          This option extracts device special files.  Because
          restoring device files onto a different system can be
          dangerous, this option should only be used by the superuser.
          .p
          HP-UX OPTION EQUIVALENT: -x"
    ;
    option resynch+ronize
    editrule { dash("R"); }
    help ".p
          This option resynchronizes automatically if
          \"cpio\" gets \"out of phase\".  \"cpio\" will
          attempt to find the next good header in the archive and
          continue processing from there.
          .p
          HP-UX OPTION EQUIVALENT: -R"
    ;
    option not_matching
    editrule { dash("f"); }
    help ".p
          This option copies all files EXCEPT those that
          match the specified pattern.
          .p
          For information about file pattern matching, refer to
          the help topic \"Regexp_pattern\".
          .p
          HP-UX OPTION EQUIVALENT: -f <\"pattern\">"
    ;
    string <pattrn>
    editrule { append(argument); }
    hint "Enter the quoted file pattern to match input files."
    ;
  }
  option output_files disable all
  editrule { append("-o"); }
  help ".p
        This option reads the standard input to obtain a
        list of path names and archives those files to the standard
        output.
        .p
        HP-UX OPTION EQUIVALENT: -o"
  {
    option verbose+ly
    editrule { dash("v"); }
    help ".p
          This option displays a list of file names being archived.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    option reset_atime
    editrule { dash("a"); }
    help ".p
          This option resets the access times of output files
          after they are copied.
          .p
          HP-UX OPTION EQUIVALENT: -a"
    ;
    option ascii_headers
    editrule { dash("c"); }
    help ".p
          This option writes and reads header information
          in ASCII character form.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
    option create_devices
    editrule { dash("x"); }
    help ".p
          This option archives device special files.
          By default, device special files are skipped.
          .p
          HP-UX OPTION EQUIVALENT: -x"
    ;
    option follow_symlinks
    editrule { dash("h"); }
    help ".p
          This option follows symbolic links as though they
          were normal files or directories.  By default, \"cpio\"
          archives the link, not the file.
          .p
          HP-UX OPTION EQUIVALENT: -h"
    ;
  }
  option copy_files disable all
  editrule { append("-pd"); }
  help ".p
        This option reads the standard input to obtain a
        list of path names of files and copies these files
        to a tree rooted at the destination directory.
        .p
        HP-UX OPTION EQUIVALENT: -pd"
  {
    option verbose+ly
    editrule { dash("v"); }
    help ".p
          This option displays a list of file names being copied.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    option retain_mtime
    editrule { dash("m"); }
    help ".p
          This option copies the file modification
          times from the input files, rather than setting them to the
          time when you copy the files.
          .p
          HP-UX OPTION EQUIVALENT: -m"
    ;
    option reset_atime
    editrule { dash("a"); }
    help ".p
          This option resets the access times of input files
          after they are copied.
          .p
          HP-UX OPTION EQUIVALENT: -a"
    ;
    option uncondi+tionally
    editrule { dash("u"); }
    help ".p
          This option copies files unconditionally, so that
          an older file can overwrite a newer file with the same name.
          By default, the newer file is kept.
          .p
          HP-UX OPTION EQUIVALENT: -u"
    ;
    option create_devices
    editrule { dash("x"); }
    help ".p
          This option copies device special files.
          .p
          HP-UX OPTION EQUIVALENT: -x"
    ;
    string <dir>
    editrule { append(argument); }
    required "Enter the name of the directory to copy to."
    ;
  }
}
            
softkey cut
editrule { append("cut"); }
help ".p
      You can use this command to cut out a range of columns
      or fields from a file.
      .p
      For more information, refer to cut(1)."
{
  option columns disable all
  editrule { dash("c"); }
  required "Select \"columns\" or \"fields\"."
  help ".p
        This option allows you to cut out columns from a table.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  {
    string <number> enable all
    editrule { word[last] &= trim(argument); }
    required "Enter the number of the first column to cut or select \"thru\"."
    ;
    option thru enable all
    editrule { word[last] &= "-"; }
    help ".p
          This option cuts a range of columns from the first
          column through the column you specify.
          .p
          For example, the
          softkey command \"cut columns thru 5\" copies
          columns 1 through 5.
          .p
          HP-UX OPTION EQUIVALENT: -"
    {
      option last disable all
      ;
      string <number>
      editrule { word[last] &= trim(argument); }
      required "Enter the number of the last column to cut."
      ;
    }
    option and disabled enable -1 disable none
    editrule { word[last] &= ","; }
    help ".p
          This option allows you to specify more than one
          column or range of columns to cut.  For example, the
          softkey command \"cut columns 1 and 7\" copies
          the first and seventh columns.
          .p
          HP-UX OPTION EQUIVALENT: ,"
    ;
    string <files> command disabled disable -1
    editrule { append(argument); }
    required "Enter the name of the file(s) to cut."
    ;
  }
  option fields disable all
  editrule { dash("f"); }
  help ".p
        This option allows you to cut out fields from a file.
        By default, a tab is assumed to separate fields.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  {
    string <number> enable all
    editrule { word[last] &= trim(argument); }
    required "Enter the number of the first field to cut or select \"thru\"."
    ;
    option thru enable all
    editrule { word[last] &= "-"; }
    help ".p
          This option cuts a range of fields from the first field
          through the field you specify.
          .p
          For example, the
          softkey command \"cut fields thru 5\" copies
          fields 1 through 5.
          .p
          HP-UX OPTION EQUIVALENT: -"
    {
      option last disable all
      ;
      string <number>
      editrule { word[last] &= trim(argument); }
      required "Enter the number of the last field to cut."
      ;
    }
    option and disabled enable -1 disable none
    editrule { word[last] &= ","; }
    help ".p
          This option allows you to specify more than one
          field or range of fields to cut.  For example,
          softkey command \"cut fields 1 and 7\" copies
          the first and seventh fields.
          .p
          HP-UX OPTION EQUIVALENT: ,"
    ;
    option delimit+ed_by disabled enable all
    help ".p
          This option allows you to specify a field
          delimiter string.  By default, the delimiter is a tab.
          .p
          HP-UX OPTION EQUIVALENT: -d<\"delimiter\">"
    {
      string <char>
      editrule { append("-d" & argument); }
      required "Enter the quoted field delimiter character."
      ;
    }
    option suppress_empty disabled enable all
    editrule { append("-s"); }
    help ".p
          This option suppresses the displaying
          of lines with no delimiter characters.
          .p
          HP-UX OPTION EQUIVALENT: -s"
    ;
    string <files> command disabled disable -1
    editrule { append(argument); }
    required "Enter the name of the file(s) to cut."
    ;
  }
}
            
softkey dd
editrule { append("dd"); }
help ".p
      You can use this command to copy a tape or file.
      .p
      For more information, refer to dd(1)."
{
  option input_file
  help ".p
        This option allows you to specify the input file to read from.
        .p
        HP-UX OPTION EQUIVALENT: -if=<input_file>"
  {
    string <file>
    editrule { append("if=" & argument); }
    required "Enter the name of the input or raw device file."
    ;
  }
  option output_file
  help ".p
        This option allows you to specify the output file to write to.
        .p
        HP-UX OPTION EQUIVALENT: -of=<output_file>"
  {
    string <file>
    editrule { append("of=" & argument); }
    required "Enter the name of the output or raw device file."
    ;
  }
  option block_size
  help ".p
        This option sets the input and output block sizes
        to the specified size.
        .p
        HP-UX OPTION EQUIVALENT: bs=<size>"
  {
    string <number>
    editrule { append("bs=" & argument); }
    required "Enter the block size in bytes or kilobytes."
    {
      option bytes disable all
      required "Select \"bytes\" or \"kilobytes\"."
      ;
      option kilo+bytes
      editrule { word[last] = trim(word[last]) & "k"; }
      ;
    }
  }
  option skip_count
  help ".p
        This option allows you to specify the
        number of input blocks to skip before starting to copy.
        .p
        HP-UX OPTION EQUIVALENT: skip=<number>"
  {
    string <number>
    editrule { append("skip=" & argument); }
    required "Enter the number of input blocks to skip before copying."
    ;
  }
  option seek_count
  help ".p
        This option allows you to specify the number of output
        blocks to seek past before copying.
        .p
        HP-UX OPTION EQUIVALENT: seek=<number>"
  {
    string <number>
    editrule { append("seek=" & argument); }
    required "Enter the number of output blocks to seek past before copying."
    ;
  }
  option copy_count
  help ".p
        This option allows you to specify the total number of
        blocks to copy.
        .p
        HP-UX OPTION EQUIVALENT: count=<number>"
  {
    string <number>
    editrule { append("count=" & argument); }
    required "Enter the total number of blocks to copy."
    ;
  }
}
            
softkey df command
editrule { append("df"); }
help ".p
      You can use this command to view the number of free
      512 byte blocks and free inodes available on a file system.
      .p
      For more information, refer to df(1)."
{
  option all_info
  editrule { append("-t"); }
  help ".p
        This option displays the total allocated blocks
        available on a file system, in
        addition to the number of free 512 byte blocks and free
        inodes available.
        .p
        HP-UX OPTION EQUIVALENT: -t"
             
  ;
  string <files> disable -1
  editrule { append(argument); }
  hint
  "Enter the name of the block-special device file(s) or mount directory(s)."
  ;
}
            
softkey diff
editrule { append("diff"); }
help ".p
      You can use this command to compare two text files and
      determine which lines are different.
      .p
      For more information, refer to diff(1)."
{
  option ignoring_blanks
  editrule { dash("b"); }
  help ".p
        This option ignores trailing blanks (spaces and tabs)
        in lines of the input file.
        Any number of blanks within a line are compared equally.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
  option ed_script
  editrule { dash("e"); }
  help ".p
        This option produces a script of a, c, and d
        commands.  This script allows the editor ed(1) to recreate
        the second file from the first.
        .p
        HP-UX OPTION EQUIVALENT: -e"
  ;
  string <file1> enable all
  editrule { append(argument); }
  required "Enter the name of the first file to compare."
  ;
  string <file2> disabled
  editrule { append(argument); }
  required "Then, enter the name of the second file to compare."
  ;
}
            
softkey dircmp command
editrule { append("dircmp"); }
help ".p
      You can use this command to compare the contents of
      two directories.
      .p
      By default, \"dircmp\" displays a list
      of files unique to each directory.  It also displays a sorted
      list indicating whether the filenames common to both
      directories have the same contents.
      .p
      For more information, refer to dircmp(1)."
{
  option suppress_equivs
  editrule { dash("s"); }
  help ".p
        This option suppresses messages about identical files.
        By default, these messages are displayed.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option show_diffs
  editrule { dash("d"); }
  help ".p
        This option outputs a list showing which lines are
        different in files that have the same name in both directories.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option display_width
  help ".p
        This option allows you to specify the output display
        width.  By default, the width is 72 characters.
        .p
        HP-UX OPTION EQUIVALENT: -w<number>"
  {
    string <number>
    editrule { append("-w" & argument); }
    required "Enter the display width."
    help ".p
          Specify the output display width.  By default, the width is
          72 characters.
          .p
          HP-UX OPTION EQUIVALENT: -w<number>"
    ;
  }
  string <dir1> enable all
  editrule { append(argument); }
  required "Enter the name of the first directory to compare."
  ;
  string <dir2> disabled
  editrule { append(argument); }
  required "Then, enter the name of the second directory to compare."
  ;
}
            
softkey disable command
editrule { append("disable"); }
help ".p
      You can use this command to disable a line printer
      from accepting print requests.
      .p
      By default, any requests currently printing are
      reprinted on the same printer or on another
      printer of the same class.
      .p
      For more information, refer to disable(1)."
{
  option cancel_requests
  editrule { dash("c"); }
  help ".p
        This option cancels any requests that
        are currently printing.  By default, current requests are
        reprinted.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option for_reason
  help ".p
        This option allows you to specify a reason for
        disabling the printer.  This reason is reported by
        the \"lpstat\" command.
        .p
        HP-UX OPTION EQUIVALENT: -r<\"reason\">"
  {
    string <reason>
    editrule { append("-r" & argument); }
    required "Enter the quoted reason for disabling the printer."
    ;
  }
  string <dests> disable -1
  editrule { append(argument); }
  required "Enter the name of the printer(s) to disable."
  ;
}
        
softkey du command
editrule { append("du"); }
help ".p
      You can use this command to display the number of 512
      byte blocks contained in all files within a directory.
      By default, disk usage is displayed for each specified directory.
      .p
      For more information, refer to du(1)."
{
  option all_files disable 1
  editrule { dash("a"); }
  help ".p
        This option displays the disk usage for each file
        individually.  By default, disk usage is displayed for each
        entire directory.
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option summary_only
  editrule { dash("s"); }
  help ".p
        This option displays only the sum of the disk usage for all files
        and directories specified.  By default, disk usage is also displayed
        for each individual directory.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option hfs_only disable 2
  editrule { append("-t hfs"); }
  help ".p
        This option only displays the disk usage within HFS mounted
        file systems.
        .p
        HP-UX OPTION EQUIVALENT: -t hfs"
  ;
  option nfs_only disable 1
  editrule { append("-t nfs"); }
  help ".p
        This option only displays the disk usage within
        NFS-mounted file systems.
        .p
        HP-UX OPTION EQUIVALENT: -t nfs"
  ;
  option cdfs_only
  editrule { append("-t cdfs"); }
  help ".p
        This option only displays the disk usage within
        CD-ROM mounted file systems.
        .p
        HP-UX OPTION EQUIVALENT: -t cdfs"
  ;
  string <files> disable -1
  editrule { append(argument); }
  hint "Enter the name of the file(s) or directory(s) to report."
  ;
}
            
softkey elm
editrule { append("elm"); }
help ".p
      You can use this command to send and receive
      electronic mail messages.
      .p
      To read incoming messages or send messages,
      use this command without any options.  Follow
      \"elm\"'s on-screen prompts.
      .p
      For more information, refer to elm(1)."
{
  option from_file command disable all
  help ".p
        This option allows you to specify from which file mail
        should be read.  By default, it is read from the
        system mailbox, \"/usr/mail/<username>\".
        .p
        HP-UX OPTION EQUIVALENT: -f <filename>"
  {
    string <file>
    editrule { append("-f " & argument); }
    required "Enter the name of the mail file to read."
    ;
  }
  option with_subject disable all
  help "This option allows you to specify a subject
        for the message you are sending.
        .p
        HP-UX OPTION EQUIVALENT: -s <\"subject\">"
  {
    string <subjct>
    editrule { append("-s " & argument); }
    required "Enter the quoted mail subject line."
    {
      option "" empty
      ;
      option "" empty
      ;
      string <users> disable -1
      editrule { append(argument); }
      required "Enter the name of the user(s) to send to."
      ;
    }
  }
  string <users> disable -1
  editrule { append(argument); }
  hint "Enter the name of the user(s) to send to."
  ;
}
            
softkey enable command
editrule { append("enable"); }
help ".p
      You can use this command to enable a line printer
      so that it can accept print requests.
      .p
      For more information, refer to enable(1)."
{
  string <dests> disable -1
  editrule { append(argument); }
  required "Enter the name of the printer(s) to enable."
  ;
}
            
softkey exit command
editrule { append("exit"); }
help ".p
      You can use this command to exit the currently running shell.
      .p
      For more information, refer to exit(1)."
;
            
softkey find command
editrule { append("find"); }
help ".p
      You can use this command to locate files that match a
      specified Boolean expression.  This command recursively descends the
      directory hierarchy for each path name you give,
      looking for matching files.
      .p
      By default, it prints its results to standard output
      and does not follow symbolic links.
      .p
      For information about file pattern matching, refer to
      the help topic \"Regexp_pattern\".
      .p
      For more information, refer to find(1)."
{
  string <dirs> disable -1 enable 13
  editrule { append(argument); }
  required
    "Enter the name of the directory(s) to search; then select any options."
  ;
  option search_options disabled enable 12
  help ".p
        This option allows you to specify how the
        search should be conducted.  For instance, you can
        specify that symbolic links should be followed and
        entries below mount points should be excluded."
  {
    option follow_symlinks
    editrule { append("-follow"); }
    help ".p
          This option follows symbolic
          links.  By default, they are not followed.
          .p
          HP-UX OPTION EQUIVALENT: -follow"
    ;
    option include_cdfs
    editrule { append("-hidden"); }
    help ".p
          This option includes
          all elements of context-dependent files.
          By default, only the element that matches the
          current context is included.
          .p
          HP-UX OPTION EQUIVALENT: -hidden"
    ;
    option defer_dirs
    editrule { append("-depth"); }
    help ".p
          This option processes the entries
          within a directory before processing the directory itself.
          .p
          This can be useful if you are using \"find\" with \"cpio\"
          to transfer files that are in directories without write
          permission, or if you want to preserve the modification
          dates of directories.
          .p
          HP-UX OPTION EQUIVALENT: -depth"
    ;
    option exclude_mounts
    editrule { append("-mountstop"); }
    help ".p
          This option ignores all entries
          below mount points.
          .p
          HP-UX OPTION EQUIVALENT: -mountstop"
    ;
    option only_fs_type
    editrule { append("-fsonly"); }
    help ".p
          This option stops descending any
          directory whose file system is not of the type you
          specify: HFS, NFS-mounted, or CD-ROM.
          .p
          HP-UX OPTION EQUIVALENT: -fsonly <type>"
    {
      option hfs disable all
      editrule { append("hfs"); }
      required "Select \"hfs\", \"nfs\", or \"cdfs\"."
      help ".p
            This option stops descending any
            directory whose file system is not an HFS file
            system.
            .p
            HP-UX OPTION EQUIVALENT: -fsonly hfs"
      ;
      option nfs disable all
      editrule { append("nfs"); }
      help ".p
            This option stops descending any
            directory whose file system is not an NFS-mounted file
            system.
            .p
            HP-UX OPTION EQUIVALENT: -fsonly nfs"
      ;
      option cdfs disable all
      editrule { append("cdfs"); }
      help ".p
            This option stops descending any
            directory whose file system is not a CD-ROM file
            system.
            .p
            HP-UX OPTION EQUIVALENT: -fsonly cdfs"
      ;
    }
    option done
    required "Select \"done\"."
    ;
  }
  option not disabled enable 12
  editrule { append("!"); }
  help ".p
        This option specifies the logical NOT operator.
        After selecting this option, you must select
        options to define file attributes such as file
        name.  Then, files are found that do NOT have the attributes
        you specify.
        .p
        HP-UX OPTION EQUIVALENT: !"
  ;
  option name disabled disable 11 enable all
  editrule { append("-name"); }
  help ".p
        This option searches for files that match the
        specified file name.  By default, only the last
        component of these files is checked.
        .p
        HP-UX OPTION EQUIVALENT: -name <pattern>"
  {
    option full_path
    editrule { word[last] = "-path"; }
    help ".p
          This option searches for files that match the
          full path of the current file name.  By default,
          only the last component of these files is checked.
          .p
          HP-UX OPTION EQUIVALENT: -path"
    ;
    string <pattrn>
    editrule { append(argument); }
    required "Enter the quoted file name pattern."
    ;
  }
  option type disabled disable 10 enable all
  editrule { append("-type"); }
  help ".p
        This option searches for files of the specified type.
        .p
        HP-UX OPTION EQUIVALENT: -type <type>"
             
  {
    option file disable all
    editrule { append("f"); }
    required "Select the file type."
    help ".p
          This option searches for regular files.
          .p
          HP-UX OPTION EQUIVALENT: -type f"
    ;
    option dir disable all
    editrule { append("d"); }
    help ".p
          This option searches for directories only.
          .p
          HP-UX OPTION EQUIVALENT: -type d"
    ;
    option char-special disable all
    editrule { append("c"); }
    help ".p
          This option searches for character special files only.
          .p
          HP-UX OPTION EQUIVALENT: -type c"
    ;
    option block-special disable all
    editrule { append("b"); }
    help ".p
          This option searches for block special files only.
          .p
          HP-UX OPTION EQUIVALENT: -type b"
    ;
    option symbolic_link disable all
    editrule { append("l"); }
    help ".p
          This option searches for symbolic links only.
          .p
          HP-UX OPTION EQUIVALENT: -type l"
    ;
    option socket disable all
    editrule { append("s"); }
    help ".p
          This option searches for sockets only.
          .p
          HP-UX OPTION EQUIVALENT: -type s"
    ;
    option named_pipe disable all
    editrule { append("p"); }
    help ".p
          This option searches for named pipes (FIFOs) only.
          .p
          HP-UX OPTION EQUIVALENT: -type p"
    ;
    option cdf disable all
    editrule { append("H"); }
    help ".p
          This option searches for context-dependent files only.
          .p
          HP-UX OPTION EQUIVALENT: -type H"
    ;
    option mount_point disable all
    editrule { append("M"); }
    help ".p
          This option searches for mount points only.
          .p
          HP-UX OPTION EQUIVALENT: -type M"
    ;
  }
  option mode disabled disable 9 enable all
  editrule { x = "-"; m = 0; }
  help ".p
        This option searches for files whose modes match the
        specified mode.
        By default, it also searches for files whose
        modes are a superset of the specified mode."
  {
    option exact_match
    editrule { x = ""; }
    help "This option searches for files whose modes
          exactly match the specified mode.
          By default, it searches for files whose
          modes are a superset of the specified mode."
    ;
    option set_uid enable all
    editrule { m += 4000; }
    required "Select file modes."
    help "This option searches for files whose \"set-user-id
          on execution\" bit is set."
    ;
    option set_gid enable all
    editrule { m += 2000; }
    help "This option searches for files whose \"set-group-id
          on execution\" bit is set."
    ;
    option sticky enable all
    editrule { m += 1000; }
    help "This option searches for files whose sticky
          bit is set."
    ;
    option readable_by enable all
    help ".p
          This option searches for files that can be read
          by one of the following: no one, the file's owner,
          the file's owner and all members of the file's group,
          or all users."
    {
      option none disable all
      required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
      help ".p
            This option searches for files that no one can read."
      ;
      option owner disable all
      editrule { m += 400; }
      help ".p
            This option searches for files that can be read
            by the file's owner."
      ;
      option owner_group disable all
      editrule { m += 440; }
      help ".p
            This option searches for files that can be read
            by the file's owner and all members of the file's group."
      ;
      option all
      editrule { m += 444; }
      help ".p
            This option searches for files that anyone can read."
      ;
    }
    option writable_by enable all
    help ".p
          This option searches for files that can be written
          by one of the following: no one, the file's owner,
          the file's owner and all members of the file's group, or all
          users."
    {
      option none disable all
      required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
      help ".p
            This option searches for files that no one can write."
      ;
      option owner disable all
      editrule { m += 200; }
      help ".p
            This option searches for files that can be
            written by the file's owner."
      ;
      option owner_group disable all
      editrule { m += 220; }
      help ".p
            This option searches for files that can be
            written by the file's owner and all members of the file's group."
      ;
      option all
      editrule { m += 222; }
      help ".p
            This option searches for files that can be
            written by anyone."
      ;
    }
    option executa+ble_by enable all
    help ".p
          This option searches for files that can be
          executed by one of the following: no one, the file's owner,
          the file's owner and all members of the file's group, or all
          users."
    {
      option none disable all
      required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
      help ".p
            This option searches for files that no one can execute."
      ;
      option owner disable all
      editrule { m += 100; }
      help ".p
            This option searches for files that the file's owner
            can execute."
      ;
      option owner_group disable all
      editrule { m += 110; }
      help ".p
            This option searches for files that can be
            executed by the file's owner and all members of the
            file's group."
      ;
      option all
      editrule { m += 111; }
      help ".p
            This option searches for files that anyone can execute."
      ;
    }
    option done disabled
    editrule { append("-perm " & x & m); }
    required "Select \"done\"."
    help "This option terminates the file mode selection."
    ;
  }
  option owner disabled disable 8 enable all
  help ".p
        This option searches for files belonging to the
        specified user.
        .p
        HP-UX OPTION EQUIVALENT: -user <username>"
  {
    string <user>
    editrule { append("-user " & argument); }
    required "Enter the name of the user."
                                             
    ;
  }
  option group disabled disable 7 enable all
  help ".p
        This option searches for files belonging to the
        specified group.
        .p
        HP-UX OPTION EQUIVALENT: -group <groupname>"
  {
    string <group>
    editrule { append("-group " & argument); }
    required "Enter the name of the group."
    ;
  }
  option newer_than disabled disable 6 enable all
  help ".p
        This option searches for files that have been
        modified more recently than the specified file.
        .p
        HP-UX OPTION EQUIVALENT: -newer <filename>"
  {
    string <file>
    editrule { append("-newer " & argument); }
    required "Enter the name of the file to compare found files to."
    ;
  }
  option modified disabled disable 5 enable all
  editrule { x = ""; }
  help ".p
        This option searches for files that were
        modified the specified number of days ago.
        .p
        HP-UX OPTION EQUIVALENT: -mtime <days>"
  {
    option within disable 1
    editrule { x = "-"; }
    help ".p
          This option searches for files that have
          been modified within the specified number of days.
          .p
          HP-UX OPTION EQUIVALENT: -mtime -<days>"
    ;
    option before
    editrule { x = "+"; }
    help ".p
          This option searches for files that have NOT
          been modified within the specified number of days.
          .p
          HP-UX OPTION EQUIVALENT: -mtime +<days>"
    ;
    string <days>
    editrule { append("-mtime " & x & argument); }
    required "Enter the number of days since files were modified."
    ;
  }
  option accessed disabled disable 4 enable all
  editrule { x = ""; }
  help ".p
        This option searches for files that were
        accessed the specified number of days ago.
        .p
        HP-UX OPTION EQUIVALENT: -atime <days>"
  {
    option within disable 1
    editrule { x = "-"; }
    help ".p
          This option searches for files that have
          been accessed within the specified number of days.
          .p
          HP-UX OPTION EQUIVALENT: -atime -<days>"
    ;
    option before
    editrule { x = "+"; }
    help ".p
          This option searches for files that have NOT
          been accessed within the specified number of days.
          .p
          HP-UX OPTION EQUIVALENT: -atime +<days>"
    ;
    string <days>
    editrule { append("-atime " & x & argument); }
    required "Enter the number of days since files were accessed."
    ;
  }
  option size disabled disable 3 enable all
  editrule { x = ""; }
  help ".p
        This option searches for files that are the specified
        number of bytes long.
        .p
        HP-UX OPTION EQUIVALENT: -size <bytes>c"
  {
    option less_than disable 1
    editrule { x = "-"; }
    help ".p
          This option searches for files that are less than
          the specified number of bytes long.
          .p
          HP-UX OPTION EQUIVALENT: -size -<bytes>"
    ;
    option greater_than
    editrule { x = "+"; }
    help ".p
          This option searches for files that are more than
          the specified number of bytes long.
          .p
          HP-UX OPTION EQUIVALENT: -size +<bytes>"
    ;
    string <number>
    editrule { append("-size " & x & trim(argument) & "c"); }
    required "Enter the file size in bytes."
    ;
  }
  option link_count disabled disable 2 enable all
  editrule { x = ""; }
  help ".p
        This option searches for files with the specified
        number of links.
        .p
        HP-UX OPTION EQUIVALENT: -links <number>"
  {
    option less_than disable 1
    editrule { x = "-"; }
    help ".p
          This option searches for files with less
          than the specified
          number of links.
          .p
          HP-UX OPTION EQUIVALENT: -links -<number>"
    ;
    option greater_than
    editrule { x = "+"; }
    help ".p
          This option searches for files with more
          than the specified
          number of links.
          .p
          HP-UX OPTION EQUIVALENT: -links +<number>"
    ;
    string <number>
    editrule { append("-links " & x & argument); }
    required "Enter the link count."
    ;
  }
  option inode_number disabled disable 1 enable all
  editrule { x = ""; }
  help ".p
        This option searches for files with the specified
        inode number.
        .p
        HP-UX OPTION EQUIVALENT: -inum <number>"
  {
    option less_than disable 1
    editrule { x = "-"; }
    help ".p
          This option searches for files with inode numbers less
          than the specified inode number.
          .p
          HP-UX OPTION EQUIVALENT: -inum -<number>"
    ;
    option greater_than
    editrule { x = "+"; }
    help ".p
          This option searches for files with inode numbers greater
          than the specified inode number.
          .p
          HP-UX OPTION EQUIVALENT: -inum +<number>"
    ;
    string <number>
    editrule { append("-inum " & x & argument); }
    required "Enter the inode number."
    ;
  }
  option "" empty disabled
  required "Select the file attribute to find."
  ;
  option and disabled disable -14 enable -1
  ;
  option or disabled disable -15 enable -2
  editrule { append("-o"); }
  ;
}
            
softkey fold
editrule { append("fold"); }
help ".p
      You can use this command to wrap lines that are too
      long for the output device.  By default, the display width
      is 80 characters.
      .p
      For more information, refer to fold(1)."
{
  option display_width
  help ".p
        This option allows you to specify the output display
        width.  By default, the width is 80 characters.
        .p
        If the text contains tabs, use multiples of 8
        for the display width, or expand the tabs using expand(1) first.
        .p
        HP-UX OPTION EQUIVALENT: -<number>"
  {
    string <number>
    editrule { append("-" & argument); }
    required "Enter the display width."
    help ".p
          Specify the output display width.  By default, the width is
          80 characters.
          .p
          If the text contains tabs, use multiples of 8
          for width, or expand the tabs using expand(1) first.
          .p
          HP-UX OPTION EQUIVALENT: -<number>"
    ;
  }
  string <files> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to fold."
  ;
}
            
softkey grep
editrule { x = next; append("grep"); }
help ".p
      You can use this command to search files for lines
      matching a specified pattern.
      By default, results are printed to standard output.
      .p
      For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      For information about regular expression pattern matching,
      refer to the help topic \"Regexp_pattern\".
      .p
      For more information, refer to grep(1)."
{
  option ignoring_case
  editrule { dash("i"); }
  help ".p
        This option ignores upper-case and lower-case
        differences during pattern comparison.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option ignoring_errors
  editrule { dash("s"); }
  help ".p
        This option suppresses error messages for nonexistent
        or unreadable files.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option and_print
  help ".p
        This option lets you specify how the results
        of your search should be displayed.
        .p
        By default, the matching lines are displayed."
  {
    option lines disable all
    required "Select the result to print."
    help ".p
          This option displays the lines that match the
          specified pattern.  This is the default behavior.
          .p
          For information about regular expression pattern matching,
          refer to the help topic \"Regexp_pattern\"."
    ;
    option numbered_lines disable all
    editrule { dash("n"); }
    help ".p
          This option precedes each matching line with
          its relative line number in the file.
          .p
          HP-UX OPTION EQUIVALENT: -n"
    ;
    option file_names disable all
    editrule { dash("l"); }
    help ".p
          This option displays only the names of files
          with one or more matching lines.  The lines
          themselves do not appear.
          .p
          HP-UX OPTION EQUIVALENT: -l"
    ;
    option line_counts disable all
    editrule { dash("c"); }
    help ".p
          This option displays only a count of matching lines.
          The lines themselves do not appear.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
  }
  option not_matching
  editrule { dash("v"); }
  help ".p
        This option searches for all lines EXCEPT those that
        match the specified pattern.
        .p
        For information about regular expression pattern matching,
        refer to the help topic \"Regexp_pattern\".
        .p
        HP-UX OPTION EQUIVALENT: -v"
  ;
  string <regexp> enable all
  editrule { if (word[x] == "egrep") {
               word[last] = trim(word[last]) & "\\\\|" & argument;
             } else {
               append(argument);
             } }
  required "Enter the quoted regular expression to search for."
  ;
  option or disabled disable -2 enable -1
  editrule { word[x] = "egrep"; }
  ;
  string <files> disabled command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to search."
  ;
}
            
softkey head
editrule { append("head"); }
help ".p
      You can use this command to display the first lines of
      a file.  By default, the first ten lines are displayed.
      .p
      For more information, see head(1)."
{
  option line_count
  help ".p
        This option allows you to specify how many lines
        from the beginning of the file should be displayed.
        .p
        HP-UX OPTION EQUIVALENT: -<number>"
  {
    string <number>
    editrule { append("-" & argument); }
    required
      "Enter the number of lines to read from the beginning of the file."
    ;
  }
  string <files> disable -1 command
  editrule { append(argument); }
  required "Enter the name of the file(s) to head."
  ;
}
        
softkey jobs command
editrule { append("jobs"); }
help ".p
      You can use this command to identify the current and
      previous jobs.
      .p
      For more information, refer to description of the
      \"jobs\" command in ksh(1)."
{
  option long_format
  editrule { dash("l"); }
  help ".p
        This option provides a long listing of information
        about current and previous jobs, including
        the process group ID information.
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
}
            
softkey kill command
editrule { append("kill"); }
help ".p
      You can use this command to terminate a process.
      Do not use the \"unconditionally\" option unless
      absolutely necessary, because it kills the process
      immediately and unconditionally.
      By default, a process is signalled to terminate and has time
      to exit gracefully.
      .p
      To kill a job, select the \"job\" option, then type the number
      of the background job to kill.  You can see this
      number by typing \"jobs\".
      .P
      To kill a process, select the \"<pid>\" option, then
      type the ID of the process to kill.  You
      can use the \"Process info\" softkey
      to determine the process ID.
      .p
      For more information, refer to kill(1)."
{
  option uncondi+tionally disable 1
  editrule { append("-9"); }
  help ".p
        This option kills the process immediately and unconditionally.
        Do not use this option unless
        absolutely necessary, because it kills the process
        immediately and unconditionally.
        .p
        By default, a process is signalled to terminate and has time
        to exit gracefully.
        .p
        HP-UX OPTION EQUIVALENT: -9"
  ;
  option with_signal
  help ".p
        This option sends the specified signal to the process,
        instead of the software termination signal.
        .p
        For more information on signals, refer to
        signal(5).
        .p
        HP-UX OPTION EQUIVALENT: <signal_number>"
  {
    option sighup disable all
    editrule { append("-1"); }
    required "Select a signal."
    help ".p
          This option sends the \"hangup\" signal to the
          process, instead of the software termination signal.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -1"
    ;
    option sigint disable all
    editrule { append("-2"); }
    help ".p
          This option sends the \"interrupt\" signal to the
          process, instead of the software termination signal.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -2"
    ;
    option sigquit disable all
    editrule { append("-3"); }
    help ".p
          This option sends the \"quit\" signal to the
          process, instead of the software termination signal.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -3"
    ;
    option sigill disable all
    editrule { append("-4"); }
    help ".p
          This option sends the \"illegal instruction\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -4"
    ;
    option sigtrap disable all
    editrule { append("-5"); }
    help ".p
          This option sends the \"trace trap\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -5"
    ;
    option sigabrt disable all
    editrule { append("-6"); }
    help ".p
          This option sends the \"software-generated
          abort\" signal to the process, instead of the usual
          instruction to terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -6"
    ;
    option sigemt disable all
    editrule { append("-7"); }
    help ".p
          This option sends the \"emulation trap\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -7"
    ;
    option sigfpe disable all
    editrule { append("-8"); }
    help ".p
          This option sends the \"floating point exception\"
          signal to the process, instead of the usual
          instruction to terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -8"
    ;
    option sigkill disable all
    editrule { append("-9"); }
    help ".p
          This option sends the \"kill unconditionally\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          The signal cannot be ignored, caught, or
          held off from a stopped process.
          .p
          HP-UX OPTION EQUIVALENT: -9"
    ;
    option sigbus disable all
    editrule { append("-10"); }
    help ".p
          This option sends the \"bus error\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -10"
    ;
    option sigsegv disable all
    editrule { append("-11"); }
    help ".p
          This option sends the \"segmentation violation\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -11"
    ;
    option sigsys disable all
    editrule { append("-12"); }
    help ".p
          This option sends the \"bad argument to system
          call\" signal to the process, instead of the usual
          instruction to terminate.
          .p
          The default action is to terminate the process
          and generate a core image file if possible.
          .p
          HP-UX OPTION EQUIVALENT: -12"
    ;
    option sigpipe disable all
    editrule { append("-13"); }
    help ".p
          This option sends the \"write on a pipe\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -13"
    ;
    option sigalrm disable all
    editrule { append("-14"); }
    help ".p
          This option sends the \"alarm clock\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -14"
    ;
    option sigterm disable all
    editrule { append("-15"); }
    help ".p
          This option sends the \"software termination\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -15"
    ;
    option sigusr1 disable all
    editrule { append("-16"); }
    help ".p
          This option sends the \"user-defined signal 1\"
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -16"
    ;
    option sigusr2 disable all
    editrule { append("-17"); }
    help ".p
          This option sends the \"user-defined signal 2\"
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -17"
    ;
    option sigchld disable all
    editrule { append("-18"); }
    help ".p
          This option sends the \"death of child process\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -18"
    ;
    option sigpwr disable all
    editrule { append("-19"); }
    help ".p
          This option sends the \"power fail\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -19"
    ;
    option sig+vtalrm disable all
    editrule { append("-20"); }
    help ".p
          This option sends the \"virtual timer alarm\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -20"
    ;
    option sigprof disable all
    editrule { append("-21"); }
    help ".p
          This option sends the \"profiling timer alarm\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -21"
    ;
    option sigio disable all
    editrule { append("-22"); }
    help ".p
          This option sends the \"asynchronous I/O\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -22"
    ;
    option sig+window disable all
    editrule { append("-23"); }
    help ".p
          This option sends the \"window change or mouse\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -23"
    ;
    option sigstop disable all
    editrule { append("-24"); }
    help ".p
          This option sends the \"tty stop\" signal to the process,
          instead of the software termination signal.
          .p
          The signal cannot be ignored or caught.
          The default action is to stop the process.
          .p
          HP-UX OPTION EQUIVALENT: -24"
    ;
    option sigtstp disable all
    editrule { append("-25"); }
    help ".p
          This option sends the \"stop\" signal generated
          from the keyboard to the process, instead of
          the software termination signal.
          .p
          The default action is to stop the process.
          .p
          HP-UX OPTION EQUIVALENT: -25"
    ;
    option sigcont disable all
    editrule { append("-26"); }
    help ".p
          This option sends the \"continue after stop\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The signal will not be held off from a
          stopped process.  The default action is to
          ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -26"
    ;
    option sigttin disable all
    editrule { append("-27"); }
    help ".p
          This option sends the \"background read
          attempted from control terminal\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to stop the process.
          .p
          HP-UX OPTION EQUIVALENT: -27"
    ;
    option sigttou disable all
    editrule { append("-28"); }
    help ".p
          This option sends the \"background write
          attempted to control terminal\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to stop the process.
          .p
          HP-UX OPTION EQUIVALENT: -28"
    ;
    option sigurg disable all
    editrule { append("-29"); }
    help ".p
          This option sends the \"urgent data arrived on
          an I/O channel\" signal to the process, instead of
          the software termination signal.
          .p
          The default action is to ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -29"
    ;
    option siglost disable all
    editrule { append("-30"); }
    help ".p
          This option sends the \"file lock lost\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to terminate the process.
          .p
          HP-UX OPTION EQUIVALENT: -30"
    ;
    option sigdil motorola disable all
    editrule { append("-32"); }
    help ".p
          This options sends the \"device independent i/o library\" signal
          to the process, instead of the usual instruction to
          terminate.
          .p
          The default action is to ignore the signal.
          .p
          HP-UX OPTION EQUIVALENT: -32"
    ;
  }
  option job disable -1
  required "Select \"job\" or enter the ID of the process(es)."
  help "To kill a job, select this option, then type the number
        of the background job to kill.  You can see this
        number by typing \"jobs\".
        .P
        To kill a process, select the \"<pid>\" option, then
        type the ID of the process to kill.  You
        can use the \"Process info\" softkey
        to determine the process ID."
  {
    string <job>
    editrule { append("%" & argument); }
    required "Enter the number of the background job to kill"
    help ".p
          Type the number
          of the background job to kill.  You can see this
          number by typing \"jobs\"."
    ;
  }
  string <pid> disable -1
  editrule { append(argument); }
  hint "Enter the ID of the process(es) to kill."
  help ".p
        Type the ID of the process to kill.  Use the \"Process info\"
        softkey
        to determine the process ID of the process you want
        to terminate."
  ;
}
            
softkey ll command
editrule { append("ll"); append("-F"); }
help ".p
      You can use this command to display the contents of a
      directory.
      .p
      By default, the contents of the current
      directory are displayed, sorted alphabetically in columns.
      By default, files that begin with a \".\" are not displayed.
      .p
      For more information, refer to ls(1)."
{
  option all_files
  editrule { dash("a"); }
  help ".p
        This option displays all files in a directory,
        including the files that begin with a \".\".
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option with_inodes
  editrule { dash("i"); }
  help ".p
        This option shows the inode number of each file,
        in addition to displaying the files in a directory.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option "" empty
  ;
  option sorted
  help ".p
        This option lets you choose how the output should be sorted."
  {
    option alpha+betical disable all
    required
      "Select \"alphabetical\", \"oldest-newest\", or \"newest-oldest\"."
    ;
    option oldest-newest disable all
    editrule { dash("rt"); }
    help ".p
          This option lists the files from the least recently modified to the
          most recently modified.
          .p
          HP-UX OPTION EQUIVALENT: -rt"
    ;
    option newest-oldest
    editrule { dash("t"); }
    help ".p
          This option lists the files from the most recently modified
          to the least recently modified.
          .p
          HP-UX OPTION EQUIVALENT: -t"
    ;
  }
  option use_time_of_last
  help ".p
        This option lists the files according to the last
        access, change, or modification time.
        .p
        By default, files are sorted by modification time."
  {
    option access disable all
    editrule { dash("u"); }
    required "Select \"access\", \"change\", or \"modify\"."
    help ".p
          This option lists the files with their last
          access time.  By default, files are listed
          with their modification time.
          .p
          HP-UX OPTION EQUIVALENT: -u"
    ;
    option change disable all
    editrule { dash("c"); }
    help ".p
          This option lists the files with their last
          change time.  By default, files are listed
          with their modification time.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
    option modify disable all
    help ".p
          This option lists the files
          with their last
          modification time.  This is the default behavior."
    ;
  }
  option follow_symlinks
  editrule { dash("L"); }
  help ".p
        This option lists the file or directory to which
        a symbolic link refers, rather than listing the link itself.
        .p
        HP-UX OPTION EQUIVALENT: -L"
  ;
  option show_cdfs
  editrule { dash("H"); }
  help ".p
        This option puts a plus sign (+) after each file
        name if the file is a context dependent file.
        .p
        HP-UX OPTION EQUIVALENT: -H"
  ;
  option recurs+ively disable 1
  editrule { dash("R"); }
  help ".p
        This option recursively displays the contents of any
        sub-directories encountered.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  option dir_info_only disable all
  editrule { dash("d"); }
  help ".p
        This option lists the directory itself, not
        its contents.
        .p
        The listing shows the following fields, from
        left to right:
        .ip * 5
        Permissions (described in \"chmod\"(1)).
        .ip * 5
        Link count (the number of names under which this file appears
        in the file system).
        .ip * 5
        Owner (the name or user ID of the user who owns the file).
        .ip * 5
        Group (the name or group ID of the group to which the file belongs).
        .ip * 5
        Size (in bytes).
        .ip * 5
        Date (the date and time when the file was last modified.)
        .ip * 5
        File name, followed by a \"/\" since the
        file is a directory.
        .p
        HP-UX OPTION EQUIVALENT: -ld"
  {
    string <dirs> disable -1
    editrule { append(argument); }
    required "Enter the name of the directory(s) to list."
    ;
  }
  string <files> disable -1
  editrule { append(argument); }
  hint "Enter the name of the directory(s) or file(s) to list."
  ;
}
            
softkey lp
editrule { append("lp"); }
help ".p
      You can use this command to send requests to an \"lp\"
      line printer or plotter.
      .p
      \"lp\" associates a unique ID with
      each request and prints it.
      .p
      This ID can later be used to
      cancel the request (see cancel(1)), alter it (see lpalt(1)),
      or find its status (see lpstat(1)).
      To find this ID, use the \"lpstat\" command.
      .p
      By default, this command prints one copy of the file.
      .p
      The file is printed on the default line printer
      designated by \"lp\".
      .p
      For more information, refer to lp(1)."
{
  option silently
  editrule { dash("s"); }
  help ".p
        This option suppresses displaying of the ID message.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option copy_count
  help ".p
        This option lets you specify how many copies
        should be printed.  By default, one copy is printed.
        .p
        HP-UX OPTION EQUIVALENT: -n<number>"
  {
    string <number>
    editrule { append("-n" & argument); }
    required "Enter the number of copies to print."
    ;
  }
  option with_banner
  help ".p
        This option allows you to specify the title that
        you want to appear on the banner page (the page printed at
        the beginning of your print job to separate it from other
        print jobs).
        .p
        By default, the banner page has a title that usually includes
        your user name, the date, and the time.
        .p
        HP-UX OPTION EQUIVALENT: -t<\"title\">"
  {
    string <banner>
    editrule { append("-t" & argument); }
    required "Enter the quoted title to print on the banner page."
    ;
  }
  option lp_model_option disable -1
  help ".p
        This option allows you to specify printer-dependent options.
        .p
        To see what options are available for
        the printers supported on your system, look in the
        \"/usr/spool/lp/interface\" directory for the model script for
        each printer (e.g., hp2225a).
        .p
        HP-UX OPTION EQUIVALENT: -o"
  {
    string <option>
    editrule { append("-o" & argument); }
    required "Enter a printer model option."
    ;
  }
  option to_printer
  help ".p
        This option allows you to specify to which printer
        the file should be sent.  You can use the softkey
        command \"Print_status all_info\" or the HP-UX command \"lpstat
        -t\" to see a list of available printers.
        .p
        By default, the file is sent to the default line
        printer designated by \"lp\".
        You can use the softkey
        command \"Print_status default_dest\" or the HP-UX
        command \"lpstat -d\" to see the default
        printer.
        .p
        HP-UX OPTION EQUIVALENT: -d <printer>"
  {
    string <dest>
    editrule { append("-d" & argument); }
    required "Enter the name of the printer."
    help ".p
          Specify to which printer the file should be sent.
          You can use the softkey
          command \"Print_status all_info\" or the HP-UX command \"lpstat
          -t\" to see a list of available printers.
          .p
          HP-UX OPTION EQUIVALENT: -d <printer>"
    ;
  }
  string <files> disable -1 command
  editrule { append(argument); }
  required "Enter the name of the file(s) to print."
  ;
}
            
softkey lpstat command
editrule { append("lpstat"); }
help ".p
      You can use this command to check the current status of the \"lp\"
      line printers and plotters.  By default, the ID numbers
      and status of all requests you have made are displayed.
      .p
      For more information, refer to lpstat(1)."
{
  option all_info disable all
  editrule { append("-t"); }
  help ".p
        This option displays all status information,
        including the following:
        .ip * 5
        Status of the \"lp\" request scheduler and printers.
        .ip * 5
        Status of output requests.
        .ip * 5
        System default destination.
        .ip * 5
        List of class names and their members.
        .ip * 5
        List of printers and their associated devices.
        .ip * 5
        Whether printers are accepting requests.
        .p
        You may want to pipe this command's output
        through the \"more\" command so that
        output does not scroll off the screen.
        For example, select the softkey command
        \"Print_status\" \"all_info\", then type \"| more\".
        For information on using commands in a
        command pipeline, refer to the help topic
        \"Redirect_pipe\".
        .p
        HP-UX OPTION EQUIVALENT: -t"
  ;
  option schedul+er_info
  editrule { append("-r"); }
  help ".p
        This option displays the status of the \"lp\" request scheduler.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option default_dest
  editrule { append("-d"); }
  help ".p
        This option displays the system default destination for \"lp\".
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option of_request disable -1
  help ".p
        This option displays the status of the specified request.
        .p
        HP-UX OPTION EQUIVALENT: -o<request_id>"
  {
    string <req_id>
    editrule { append("-o" & argument); }
    required "Enter the ID of the request."
    ;
  }
  option of_user disable -1
  help ".p
        This option displays the status of output requests
        for the specifed user.
        .p
        HP-UX OPTION EQUIVALENT: -u<username>"
  {
    string <user>
    editrule { append("-u" & argument); }
    required "Enter the name of the user."
    ;
  }
  option of_printer disable -1
  help ".p
        This option displays the status of the specified printer.
        .p
        HP-UX OPTION EQUIVALENT: -p<printer>"
  {
    string <dest>
    editrule { append("-p" & argument); }
    required "Enter the name of the printer."
    ;
  }
}
            
softkey ls command
editrule { append("ls"); append("-F"); l = 0; }
help ".p
      You can use this command to display the contents of a
      directory.
      .p
      By default, the contents of the current
      directory are displayed, sorted alphabetically in columns.
      By default, files that begin with a \".\" are not displayed.
      .p
      For more information, refer to ls(1)."
{
  option all_files
  editrule { dash("a"); }
  help ".p
        This option displays all files in a directory,
        including the files that begin with a \".\".
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option with_inodes
  editrule { dash("i"); }
  help ".p
        This option shows the inode number of each file,
        in addition to displaying the files in a directory.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option long_format enable all
  editrule { dash("l"); l = 1; }
  help ".p
        This option displays the directory contents in long format.
        .p
        The listing shows the following fields, from
        left to right:
        .ip * 5
        Permissions (described in \"chmod\"(1)).
        .ip * 5
        Link count (the number of names under which this file appears
        in the file system).
        .ip * 5
        Owner (the name or user ID of the user who owns the file).
        .ip * 5
        Group (the name or group ID of the group to which the file belongs).
        .ip * 5
        Size (in bytes).
        .ip * 5
        Date (the date and time when the file was last modified.)
        .ip * 5
        File name (followed by a \"/\" if the file is a directory, \"*\"
        if the file is an executable, and \"@\" if the file
        is a symbolic link).
        .p
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
  option sorted enable all
  help ".p
        This option lets you choose how the output should be sorted."
  {
    option alpha+betical disable all
    required
      "Select \"alphabetical\", \"oldest-newest\", or \"newest-oldest\"."
    ;
    option oldest-newest disable all
    editrule { dash("rt"); }
    help ".p
          This option lists the files from the least recently modified to the
          most recently modified.
          .p
          HP-UX OPTION EQUIVALENT: -rt"
    ;
    option newest-oldest
    editrule { dash("t"); }
    help ".p
          This option lists the files from the most recently modified
          to the least recently modified.
          .p
          HP-UX OPTION EQUIVALENT: -t"
    ;
  }
  option use_time_of_last disabled
  help ".p
        This option lists the files according to the last
        access, change, or modification time.
        .p
        By default, files are sorted by modification time."
  {
    option access disable all
    editrule { dash("u"); }
    required "Select \"access\", \"change\", or \"modify\"."
    help ".p
          This option lists the files with their last
          access time.  By default, files are listed
          with their modification time.
          .p
          HP-UX OPTION EQUIVALENT: -u"
    ;
    option change disable all
    editrule { dash("c"); }
    help ".p
          This option lists the files with their last
          change time.  By default, files are listed
          with their modification time.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
    option modify disable all
    help ".p
          This option lists the files
          with their last
          modification time.  This is the default behavior."
    ;
  }
  option follow_symlinks
  editrule { dash("L"); }
  help ".p
        This option lists the file or directory to which
        a symbolic link refers, rather than listing the link itself.
        .p
        HP-UX OPTION EQUIVALENT: -L"
  ;
  option show_cdfs
  editrule { dash("H"); }
  help ".p
        This option puts a plus sign (+) after each file
        name if the file is a context dependent file.
        .p
        HP-UX OPTION EQUIVALENT: -H"
  ;
  option recurs+ively disable 1
  editrule { dash("R"); }
  help ".p
        This option recursively displays the contents of any
        sub-directories encountered.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  option dir_info_only disable all
  editrule { if (! l) { dash("l"); } dash("d"); }
  help ".p
        This option lists the directory itself, not
        its contents.
        .p
        The listing shows the following fields, from
        left to right:
        .ip * 5
        Permissions (described in \"chmod\"(1)).
        .ip * 5
        Link count (the number of names under which this file appears
        in the file system).
        .ip * 5
        Owner (the name or user ID of the user who owns the file).
        .ip * 5
        Group (the name or group ID of the group to which the file belongs).
        .ip * 5
        Size (in bytes).
        .ip * 5
        Date (the date and time when the file was last modified.)
        .ip * 5
        File name, followed by a \"/\" since the
        file is a directory.
        .p
        HP-UX OPTION EQUIVALENT: -ld"
  {
    string <dirs> disable -1
    editrule { append(argument); }
    required "Enter the name of the directory(s) to list."
    ;
  }
  string <files> disable -1
  editrule { append(argument); }
  hint "Enter the name of the directory(s) or file(s) to list."
  ;
}
            
softkey lsf command
editrule { append("lsf"); l = 0; }
help ".p
      You can use this command to display the contents of a
      directory.
      .p
      By default, the contents of the current
      directory are displayed, sorted alphabetically in columns.
      By default, files that begin with a \".\" are not displayed.
      .p
      For more information, refer to ls(1)."
{
  option all_files
  editrule { dash("a"); }
  help ".p
        This option displays all files in a directory,
        including the files that begin with a \".\".
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option with_inodes
  editrule { dash("i"); }
  help ".p
        This option shows the inode number of each file,
        in addition to displaying the files in a directory.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option long_format enable all
  editrule { dash("l"); l = 1; }
  help ".p
        This option displays the directory contents in long format.
        .p
        The listing shows the following fields, from
        left to right:
        .ip * 5
        Permissions (described in \"chmod\"(1)).
        .ip * 5
        Link count (the number of names under which this file appears
        in the file system).
        .ip * 5
        Owner (the name or user ID of the user who owns the file).
        .ip * 5
        Group (the name or group ID of the group to which the file belongs).
        .ip * 5
        Size (in bytes).
        .ip * 5
        Date (the date and time when the file was last modified.)
        .ip * 5
        File name (followed by a \"/\" if the file is a directory, \"*\"
        if the file is an executable, and \"@\" if the file
        is a symbolic link).
        .p
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
  option sorted enable all
  help ".p
        This option lets you choose how the output should be sorted."
  {
    option alpha+betical disable all
    required
      "Select \"alphabetical\", \"oldest-newest\", or \"newest-oldest\"."
    ;
    option oldest-newest disable all
    editrule { dash("rt"); }
    help ".p
          This option lists the files from the least recently modified to the
          most recently modified.
          .p
          HP-UX OPTION EQUIVALENT: -rt"
    ;
    option newest-oldest
    editrule { dash("t"); }
    help ".p
          This option lists the files from the most recently modified
          to the least recently modified.
          .p
          HP-UX OPTION EQUIVALENT: -t"
    ;
  }
  option use_time_of_last disabled
  help ".p
        This option lists the files according to the last
        access, change, or modification time.
        .p
        By default, files are sorted by modification time."
  {
    option access disable all
    editrule { dash("u"); }
    required "Select \"access\", \"change\", or \"modify\"."
    help ".p
          This option lists the files with their last
          access time.  By default, files are listed
          with their modification time.
          .p
          HP-UX OPTION EQUIVALENT: -u"
    ;
    option change disable all
    editrule { dash("c"); }
    help ".p
          This option lists the files with their last
          change time.  By default, files are listed
          with their modification time.
          .p
          HP-UX OPTION EQUIVALENT: -c"
    ;
    option modify disable all
    help ".p
          This option lists the files
          with their last
          modification time.  This is the default behavior."
    ;
  }
  option follow_symlinks
  editrule { dash("L"); }
  help ".p
        This option lists the file or directory to which
        a symbolic link refers, rather than listing the link itself.
        .p
        HP-UX OPTION EQUIVALENT: -L"
  ;
  option show_cdfs
  editrule { dash("H"); }
  help ".p
        This option puts a plus sign (+) after each file
        name if the file is a context dependent file.
        .p
        HP-UX OPTION EQUIVALENT: -H"
  ;
  option recurs+ively disable 1
  editrule { dash("R"); }
  help ".p
        This option recursively displays the contents of any
        sub-directories encountered.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  option dir_info_only disable all
  editrule { if (! l) { dash("l"); } dash("d"); }
  help ".p
        This option lists the directory itself, not
        its contents.
        .p
        The listing shows the following fields, from
        left to right:
        .ip * 5
        Permissions (described in \"chmod\"(1)).
        .ip * 5
        Link count (the number of names under which this file appears
        in the file system).
        .ip * 5
        Owner (the name or user ID of the user who owns the file).
        .ip * 5
        Group (the name or group ID of the group to which the file belongs).
        .ip * 5
        Size (in bytes).
        .ip * 5
        Date (the date and time when the file was last modified.)
        .ip * 5
        File name, followed by a \"/\" since the
        file is a directory.
        .p
        HP-UX OPTION EQUIVALENT: -ld"
  {
    string <dirs> disable -1
    editrule { append(argument); }
    required "Enter the name of the directory(s) to list."
    ;
  }
  string <files> disable -1
  editrule { append(argument); }
  hint "Enter the name of the directory(s) or file(s) to list."
  ;
}
            
softkey mailx
editrule { append("mailx"); }
help ".p
      You can use this command to send and receive
      electronic mail messages.
      .ip * 5
      To read incoming messages, use this command
      without any options.
      .ip * 5
      To send a message, enter this command, then type
      the name(s) of the users you want to send mail to.
      Press \"Return\" and type the message (terminated
      with a period \".\" in the first column).
      .sp
      If you want to mail an existing file,
      use this syntax:
      .sp
      mailx <users> < <filename>
      .p
      For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      For more information, refer to mailx(1)."
{
  option summary_only command disable all
  editrule { append("-H"); }
  help ".p
        This option displays only the message header of
        any messages that are waiting.
        .p
        HP-UX OPTION EQUIVALENT: -H"
  {
    option "" empty
    ;
    option from_file
    help ".p
          This option allows you to specify a file
          to read mail from.  By default, mail is read from the
          system mailbox, \"/usr/mail/<username>\".
          .p
          HP-UX OPTION EQUIVALENT: -f <mailfile>"
    {
      string <file>
      editrule { append("-f " & argument); }
      required "Enter the name of the mail file to read."
      ;
    }
  }
  option from_file command disable all
  help ".p
        This option allows you to specify a file
        to read mail from.  By default, mail is read from the
        system mailbox, \"/usr/mail/<username>\".
        .p
        HP-UX OPTION EQUIVALENT: -f <mailfile>"
  {
    string <file>
    editrule { append("-f " & argument); }
    required "Enter the name of the mail file to read."
    ;
  }
  option with_subject disable all
  help ".p
        This option allows you to specify a subject
        for the message you are sending.
        .p
        HP-UX OPTION EQUIVALENT: -s <\"subject\">"
  {
    string <subjct>
    editrule { append("-s " & argument); }
    required "Enter the quoted mail subject line."
    {
      option "" empty
      ;
      option "" empty
      ;
      option "" empty
      ;
      string <users> disable -1
      editrule { append(argument); }
      required "Enter the name of the user(s) to send to."
      ;
    }
  }
  string <users> disable -1
  editrule { append(argument); }
  hint "Enter the name of the user(s) to send to."
  ;
}
            
softkey make command
editrule { append("make"); }
help ".p
      You can use this command to maintain, update, and
      regenerate groups of programs.
      .p
      For more information, refer to make(1)."
{
  option using_makefile
  help ".p
        This option allows you to specify a makefile
        to use.  A file name of \"-\" denotes the standard input.
        .p
        By default, the file \"makefile\" or
        \"Makefile\" in the current directory is used.
        .p
        HP-UX OPTION EQUIVALENT: -f <makefile>"
  {
    string <file>
    editrule { append("-f " & argument); }
    required "Enter the name of the makefile."
    ;
  }
  option silently disable 2
  editrule { append("-s"); }
  help ".p
        This option prevents command lines from
        being displayed before they are executed.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option show_rules
  editrule { append("-p"); }
  help ".p
        This option displays the complete set of macro
        definitions and target descriptions.
        .p
        HP-UX OPTION EQUIVALENT: -p"
  ;
  option no_execute disable 2
  editrule { append("-n"); }
  help ".p
        This option displays the commands that would
        be used to rebuild the specified targets, but
        does not execute them.
        .p
        HP-UX OPTION EQUIVALENT: -n"
  ;
  option touch_targets disable 1
  editrule { append("-t"); }
  help ".p
        This option touches and updates any target files,
        but does not execute the commands to rebuild them.
        .p
        HP-UX OPTION EQUIVALENT: -t"
  ;
  option ignore_errors
  editrule { append("-i"); }
  help ".p
        This option ignores any error codes returned
        by the commands used to build targets.  By default,
        \"make\" terminates when a command exits with a
        non-zero status.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option env_override
  editrule { append("-e"); }
  help ".p
        This option causes environment variables to
        override any assignments within the makefile.
        .p
        HP-UX OPTION EQUIVALENT: -e"
  ;
  string <target> disable -1
  editrule { append(argument); }
  hint "Enter the name of the target(s) to build."
  ;
}
            
softkey man command
editrule { append("man"); }
help ".p
      You can use this command to view entries from the
      online HP-UX Reference.  You can search for information by
      keyword, file name, or section.  You can also
      specify the name of the command or topic for which you want
      help.
      .p
      By default, this command searches the entire HP-UX Reference to
      find an entry.
      .p
      For more information, refer to man(1)."
{
  option keyword_search disable all
  editrule { append("-k"); }
  help ".p
        This option lets you search for information using one or more
        keywords.  The output displays a one-line description of
        each entry that contains the keyword(s).
        .p
        HP-UX OPTION EQUIVALENT: -k <keyword>"
  {
    string <keys> disable -1
    editrule { append(argument); }
    required "Enter the keyword(s) to search the manual page entries for."
    ;
  }
  option file_search disable all
  editrule { append("-f"); }
  help ".p
        This option lets you search for information about
        specific files.  The output displays a one-line description of
        each entry related to the file(s).
        .p
        HP-UX OPTION EQUIVALENT: -f <filename>"
  {
    string <files> disable -1
    editrule { append(argument); }
    required "Enter the file name(s) to search the manual page entries for."
    ;
  }
  option from_section
  help ".p
        This option lets you search for information in
        particular sections of the HP-UX Reference.
        By default, the entire reference
        manual is searched, rather than just the specified section.
        .p
        HP-UX OPTION EQUIVALENT: <section number>"
  {
    option user_commands disable all
    editrule { append("1"); }
    required "Select a manual section."
    help ".p
          This option searches section 1 of the manual for
          the topic you specify.  Section 1 contains programs that are
          usually invoked directly by users or shell scripts.
          .p
          HP-UX OPTION EQUIVALENT: 1"
    ;
    option admin_commands disable all
    editrule { append("1m"); }
    help ".p
          This option searches section 1m of the manual for
          the topic you specify.  Section 1m contains commands
          for system administration and maintenance tasks.
          .p
          HP-UX OPTION EQUIVALENT: 1m"
    ;
    option system_calls disable all
    editrule { append("2"); }
    help ".p
          This option searches section 2 of the manual for
          the topic you specify.  Section 2 contains
          system calls.
          .p
          HP-UX OPTION EQUIVALENT: 2"
    ;
    option library_calls disable all
    editrule { append("3"); }
    help ".p
          This option searches section 3 of the manual for
          the topic you specify.  Section 3 contains
          subroutines and subroutine libraries.
          .p
          HP-UX OPTION EQUIVALENT: 3"
    ;
    option file_formats disable all
    editrule { append("4"); }
    help ".p
          This option searches section 4 of the manual for
          the topic you specify.  Section 4 documents
          the structure of various types of files.
          .p
          HP-UX OPTION EQUIVALENT: 4"
    ;
    option miscel+laneous disable all
    editrule { append("5"); }
    help ".p
          This option searches section 5 of the manual for
          the topic you specify.  Section 5 contains a
          variety of information, such as descriptions of header
          files, character sets, and macro packages.
          .p
          HP-UX OPTION EQUIVALENT: 5"
    ;
    option device_files disable all
    editrule { append("7"); }
    help ".p
          This option searches section 7 of the manual for
          the topic you specify.  Section 7 discusses the
          characteristics of special device files that link
          HP-UX and system I/O devices.
          .p
          HP-UX OPTION EQUIVALENT: 7"
    ;
  }
  string <topic>
  editrule { append(argument); }
  required "Enter the name of the manual topic."
  ;
}
            
softkey mkdir command
editrule { append("mkdir"); }
help ".p
      You can use this command to create new directories.
      By default, this command will only create the final
      component of a directory path.  This means that, for
      example, you cannot create the directory \"/users/rich/foo\"
      unless \"/users/rich\" already exists.
      .p
      The permission of new directories is determined by the
      file creation mask set with umask(1).
      .p
      For more information, refer to mkdir(1)."
{
  option recurs+ively
  editrule { append("-p"); }
  help ".p
        This option creates any necessary intermediate directories
        in the directory path.
        .p
        For example, if you want to create the directory
        \"/users/rich/foo/bar\", but only \"/users/rich\" currently
        exists, this option would first create \"/users/rich/foo\"
        and then \"/users/rich/foo/bar\".
        .p
        HP-UX OPTION EQUIVALENT: -p"
  ;
  string <dirs> disable -1
  editrule { append(argument); }
  required "Enter the name of the directory(s) to create."
  ;
}
            
softkey more
editrule { append("more"); }
help ".p
      You can use this command to display the contents of a file, one
      screenful at a time.  To continue viewing the file,
      press the space bar after each screenful.
      .p
      Note that while viewing the file, you can type \"h\" to see
      this command's own online help.
      .p
      For more information, refer to more(1)."
{
  option verbose+ly
  editrule { dash("d"); }
  help ".p
        This option gives you a more detailed prompt at the end of each
        screenful.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option no_scroll
  editrule { dash("c"); }
  help ".p
        This option avoids scrolling the screen, making it
        easier to read while the file is being displayed.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option single_spaced
  editrule { dash("s"); }
  help ".p
        This option condenses multiple blank lines into one,
        allowing the maximum amount of information to be
        displayed at one time.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  string <file> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to display."
  ;
}
            
softkey mv command
editrule { append("mv"); }
help ".p
      You can use this command to rename file(s)
      or to move them to an existing directory.
      You can also move or rename a directory
      subtree to a new or existing directory.
      .ip * 5
      If you move or rename a file to an existing file, that
      file is overwritten.  If you do not have write permission
      for the existing file, the command prompts for verification
      before overwriting the file.
      .ip * 5
      If you move or rename a directory to
      an existing directory, a subdirectory with the same name as the source
      directory will be created within it.  The
      subtree rooted at the source directory will be moved to
      this subdirectory.
      .ip * 5
      If the destination directory does not exist,
      it is created and the source directory
      subtree is moved to it.
      .p
      You must specify complete path names.
      If you do not specify the path, the file is moved to
      your current directory.
      .p
      For more information, refer to mv(1).
      .p
      (Note that the HP-UX \"mv\" command is used both to move files and
      to rename them.)"
{
  option interact+ively disable 1
  editrule { dash("i"); }
  help ".p
        This option prompts you to verify that a file
        should be moved when the move would
        overwrite an existing file.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option quiet+ly
  editrule { dash("f"); }
  help ".p
        If the destination file already exists, this option
        writes the source file over it without prompting
        you to verify that the destination file should be removed.
        By default, you will be prompted for verification if
        you do not have write permission for the file.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
  string <files> disable -1 enable all
  editrule { append(argument); }
  required "Enter the name of the file(s) or dir(s) to move."
  ;
  option to disabled
  required
    "Enter the name of the file(s) or dir(s) to move; then select \"to\"."
  {
    string <dest>
    editrule { append(argument); }
    required
      "Then, enter the name of the file or directory to move the file(s) to."
    ;
  }
}
            
softkey nroff
editrule { append("nroff"); }
help ".p
      You can use this command to format text for printing.
      \"nroff\" is best used with a macro package such as \"mm\", which
      provides a high-level interface to \"nroff\"'s low-level interface.
      .p
      For more information, refer to nroff(1)."
{
  option macro_package disable -1
  help ".p
        This option allows you to process text through a
        macro package.  Macros can be used to format memos,
        slides, and reports.
        .p
        For example, the \"mm\" macros define a set of
        macros designed to simplify the formatting of memos.
        .p
        HP-UX OPTION EQUIVALENT: -m<macro>"
  {
    string <macro>
    editrule { if (argument[0] == "m" && strlen(trim(argument)) != 1) {
                 append("-m" & argument[1, strlen(argument)-1]);
               } else {
                 append("-m" & argument);
               } }
    required "Enter the name of a macro package."
    ;
  }
  option stop_for_paper
  editrule { append("-s1"); }
  help ".p
        This option stops printing after every page to allow
        you to load or change paper.  Printing resumes
        after you press \"Return\".
        .p
        HP-UX OPTION EQUIVALENT: -s1"
  ;
  option pages disable all
  editrule { append("-o"); }
  help ".p
        This option allows you to specify which pages should
        be printed.
        .p
        HP-UX OPTION EQUIVALENT: -o"
  {
    string <number> enable all
    editrule { word[last] &= trim(argument); }
    required "Enter the number of the first page to print or select \"thru\"."
    ;
    option thru enable all
    editrule { word[last] &= "-"; }
    help ".p
          If you select \"thru\", \"nroff\" assumes that you
          want to print the specified first page through the specified
          ending page.
          .p
          HP-UX OPTION EQUIVALENT: -"
    {
      option end disable all
      help ".p
            This option prints the specified first page through the
            last page.  This is the default behavior."
      ;
      string <number>
      editrule { word[last] &= trim(argument); }
      required "Enter the number of the last page to print."
      ;
    }
    option and disabled enable -1 disable none
    editrule { word[last] &= ","; }
    help ".p
          This option lets you specify more than one page or
          range of pages to print.
          .p
          HP-UX OPTION EQUIVALENT: ,"
    ;
    string <files> command disabled disable -1
    editrule { append(argument); }
    required "Enter the name of the file(s) to print."
    ;
  }
  string <files> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to print."
  ;
}
            
softkey od
editrule { append("od"); }
help ".p
      You can use this command to create an
      octal dump of a file.
      .p
      For more information, see od(1)."
{
  option octal_bytes disable 3
  editrule { dash("b"); }
  help ".p
        This option interprets bytes in octal.
        .p
        HP-UX OPTION EQUIVALENT: -b"
             
  ;
  option chars disable 2
  editrule { dash("c"); }
  help ".p
        This option interprets bytes in ASCII.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option decimal_words disable 1
  editrule { dash("d"); }
  help ".p
        This option interprets 16-bit words in decimal.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option signed_words
  editrule { dash("s"); }
  help ".p
        This option interprets 16-bit words in signed decimal.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option from_offset
  help "This option specifies the offset in the file where
        dumping is to start.  Dumping continues to the
        end of the file.
        .p
        HP-UX OPTION EQUIVALENT: +<number>"
  {
    string <offset>
    cleanuprule { append("+" & argument); }
    required "Enter the octal or hex byte offset to start dumping at."
    ;
  }
  string <files> command
  editrule { append(argument); }
  required "Enter the name of the file to dump."
  ;
}
            
softkey paste
editrule { append("paste"); }
help ".p
      You can use this command to merge the same lines of
      several input files or subsequent lines of one file.
      By default, lines are merged one from each input file
      and separated by a tab.
      .p
      For more information, refer to paste(1)."
{
  option serially
  editrule { dash("s"); }
  help ".p
        This option allows you to merge subsequent lines
        of one file rather than the corresponding
        lines of several input files.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option paste_chars
  help ".p
        This option allows you to specify a
        quoted list of characters to be used
        for merging lines.
        The default is the tab character.
        .p
        For example, if you specify a colon \":\" as the
        character to paste lines with, the corresponding
        lines from each
        input file will be separated with \":\".
        .p
        This option can also be used with the \"serially\"
        option.  For example, if you specify \"::\\n\" as the
        list of characters to paste lines with, the first line will
        be separated from the second with a \":\", the second line
        will be separated from the third with a \":\", the third
        line will be followed by a newline (\"\\n\"), and so on.
        .p
        HP-UX OPTION EQUIVALENT: -d<\"characters\">"
  {
    string <list>
    editrule { append("-d" & argument); }
    required "Enter the quoted list of characters to paste lines with."
    ;
  }
  string <files> disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to paste."
  ;
}
            
softkey pg
editrule { append("pg"); }
help ".p
      You can use this command to display the contents of a
      file, one screenful at a time.  A prompt appears at the
      end of each screen.  To continue viewing the file,
      press \"Return\".
      .p
      You can also go backwards through a file displayed with \"pg\".
      You cannot do this with a file displayed with \"more\".
      .p
      Note that while viewing the file, you can type \"h\" to see
      \"pg\"'s online help.
                  
      .p
      For more information, refer to pg(1)."
{
  option no_scroll
  editrule { dash("c"); }
  help ".p
        This option avoids scrolling the screen, making it
        easier to read while the file is being displayed.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option no_pause_at_eof
  editrule { dash("e"); }
  help ".p
        This option does not pause at the end of
        each file.  By default, the output pauses at
        the end of each file.
        .p
        HP-UX OPTION EQUIVALENT: -e"
  ;
  option hilite_prompt
  editrule { dash("s"); }
  help ".p
        This option displays all messages and
        prompts in highlighted mode, usually inverse video.
                
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  string <file> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to display."
  ;
}
            
softkey pr
editrule { append("pr"); }
help ".p
      You can use this command to format and display the
      contents of a file.
      .p
      By default, each page of the output has a header showing
      the page number, date and time, and file name.
      .p
      For more information, refer to pr(1)."
{
  option double_spaced
  editrule { append("-d"); }
  help ".p
        This option double-spaces the output.
        By default, the output is single-spaced.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option numbered_lines
  editrule { append("-n"); }
  help ".p
        This option numbers each line of the output.
        By default, the output lines are not numbered.
        .p
        HP-UX OPTION EQUIVALENT: -n"
  ;
  option without_header disable 1
  editrule { append("-t"); }
  help ".p
        This option omits the usual header and trailer for
        each page.
        .p
        HP-UX OPTION EQUIVALENT: -t"
  ;
  option with_header
  help ".p
        This option allows you to specify the header that
        you want to appear at the top of each page in place of the
        file name.
        .p
        By default, each page has a header with
        the page number, date and time, and file name.
        .p
        HP-UX OPTION EQUIVALENT: -h <\"header\">"
  {
    option default disable all
    ;
    string <header>
    editrule { append("-h " & argument); }
    required "Enter the quoted header to print on each page."
    ;
  }
  option multi_column disable 1 enable 2
  help ".p
        This option formats the output into the specified
        number of columns.  By default, output is in one column.
        Lines wider than a column are truncated to fit.
        .p
        HP-UX OPTION EQUIVALENT: -<number>"
  {
    string <number>
    editrule { append("-" & argument); }
    required "Enter the number of columns to format the output in."
    ;
  }
  option merge_files enable 1
  editrule { append("-m"); }
  help ".p
        This option merges and displays the specified files
        side-by-side, one per column.  The \"multi-column\"
        option is ignored.
        .p
        HP-UX OPTION EQUIVALENT: -m"
  ;
  option display_width disabled
  help ".p
        This option allows you to specify the output display
        width.  By default, the width is 72 characters.
        This number is only used for
        multi-column output.
        .p
        HP-UX OPTION EQUIVALENT: -w<number>"
  {
    string <number>
    editrule { append("-w" & argument); }
    required "Enter the display width."
    help ".p
          Specify the output display width.  By default, the width is
          72 characters.
          .p
          HP-UX OPTION EQUIVALENT: -w<number>"
    ;
  }
  string <files> disable -1 command
  editrule { append(argument); }
  required "Enter the name of the file(s) to format."
  ;
}
            
softkey ps command
editrule { append("ps"); }
help ".p
      You can use this command to see the status
      and process IDs of active processes.
      .p
      By default, it displays the following fields
      associated with the current terminal:
      .ip * 5
      PID -- The process ID of the process, which you
      need to know to kill it.
      .ip * 5
      TTY -- The controlling terminal for the process.
      .ip * 5
      TIME -- The cumulative execution time for
      the process (in minutes and seconds).
      .ip * 5
      COMMAND -- The command name.
      .p
      You may want to pipe this command's output
      through the \"more\" command so that
      output does not scroll off the screen.
      For example, select the softkey command
      \"Process_info\" \"every_process\", then type \"| more\".
      For information on using commands in a
      command pipeline, refer to the help topic
      \"Redirect_pipe\".
      .p
      For more information, refer to ps(1)."
{
  option user_info
  editrule { dash("f"); }
  help ".p
        This option displays the following fields:
        .ip * 5
        UID -- The login name of the process owner.
        .ip * 5
        PID -- The process ID of the process, which you
        need to know to kill a process.
        .ip * 5
        PPID -- The process ID of the parent process.
        .ip * 5
        C -- Processor utilization for scheduling.
        .ip * 5
        STIME -- Starting time of the process.  If
        the elapsed time is more than 24 hours, the starting
        date is printed.
        .ip * 5
        TTY -- The controlling terminal for the process.
        .ip * 5
        TIME -- The cumulative execution time for
        the process (in minutes and seconds).
        .ip * 5
        COMMAND -- The command name and options.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
  option system_info
  editrule { dash("l"); }
  help ".p
        This option displays the following fields:
        .ip * 5
        F -- The octal and additive flags associated
        with the process.
        .ip * 5
        S -- The state of the process.
        .ip * 5
        UID -- The real user ID number of the process owner.
        .ip * 5
        PID -- The process ID of the process, which you
        need to know to kill a process.
        .ip * 5
        PPID -- The process ID of the parent process.
        .ip * 5
        C -- Processor utilization for scheduling.
        .ip * 5
        PRI -- The priority of the process.  Higher
        numbers indicate lower priority.
        .ip * 5
        NI -- Nice value; used in computing priority.
        .ip * 5
        ADDR -- The memory address of the process,
        if resident.  Otherwise, the disk address.
        .ip * 5
        SZ -- The size in blocks of the core image
        of the process.
        For the Series 300 and 400, each block
        is 4K bytes.
        For the Series 700 and 800, each block
        is 512 bytes.
        .ip * 5
        WCHAN -- The event for which the process is
        waiting or sleeping.  If blank, the
        process is running.
        .ip * 5
        TTY -- The controlling terminal for the process.
        .ip * 5
        TIME -- The cumulative execution time for
        the process (in minutes and seconds).
        .ip * 5
        COMD -- The command name.
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
  option every_process disable all
  editrule { dash("e"); }
  help ".p
        This option lets you see information about all processes.
        By default, you see information only about
        processes associated with the current terminal.
        .p
        HP-UX OPTION EQUIVALENT: -e"
  ;
  option for_process disable all
  help ".p
        This option lets you see information about the process
        whose ID you specify.
        By default, you see information only about
        processes associated with the current terminal.
        .p
        HP-UX OPTION EQUIVALENT: -p <pid>"
  {
    string <pid>
    editrule { append("-p" & argument); }
    required "Enter the ID of the process."
    ;
  }
  option for_user
  help ".p
        This option lets you see information about all processes
        belonging to the user whose user ID or name you specify.
        By default, you see information only about
        processes associated with the current terminal.
        .p
        HP-UX OPTION EQUIVALENT: -u <user>"
  {
    string <user>
    editrule { append("-u" & argument); }
    required "Enter the name of the user."
    ;
  }
  option for_tty
  help ".p
        This option lets you see information about all processes
        associated with the terminal you specify.
        By default, you see information only about
        processes associated with the current terminal.
        .p
        HP-UX OPTION EQUIVALENT: -t <tty#>"
  {
    string <tty>
    editrule { append("-t" & argument); }
    required "Enter the name of the tty (e.g., tty04)."
    ;
  }
}
            
softkey remsh
editrule { append("remsh"); }
help ".p
      You can use this command to connect to a remote host
      and execute a command on that host.  Normally,
      \"remsh\" terminates when the remote command does.
      .p
      By default, \"remsh\" reads its standard input and
      sends it to the remote command.  \"remsh\" also sends the
      command's standard output and standard error to
      its own stdout and stderr.
      .p
      Note that \"remsh\" will not work unless you have
      the \".rhosts\" file set up correctly on the remote machine.
      .p
      For more information, refer to remsh(1)."
{
  string <host>
  editrule { append(argument); }
  required "Enter the name of the remote host."
  {
    option as_user
    help ".p
          This option allows you to log on with a
          different remote name.  By default, the remote account name
          used is the same as your local account name.
          .p
          HP-UX OPTION EQUIVALENT: -l <name>"
    {
      string <user>
      editrule { append("-l " & argument); }
      required "Enter the name of the user."
      ;
    }
    option close_stdin
    editrule { append("-n"); }
    help ".p
          This option redirects input of the remote command
          from \"/dev/null\".  This prevents \"remsh\" from using
          input not intended for it, or from waiting for tty input
          for the remote command.
          .p
          By default, \"remsh\" reads its standard input and
          sends it to the remote command.
          .p
          HP-UX OPTION EQUIVALENT: -n"
    ;
    string <cmd> disable -1
    editrule { append(argument); }
    required "Enter the quoted shell command."
    ;
  }
}
            
softkey rlogin
editrule { append("rlogin"); }
help ".p
      You can use this command to log on to and execute
      commands on a remote host as if you had logged into it locally.
      .p
      For more information, refer to rlogin(1)."
{
  string <host>
  editrule { append(argument); }
  required "Enter the name of the remote host."
  {
    option escape_char
    help ".p
          This option allows you to define the \"rlogin\"
          escape character.  By default, the escape character is \"~\".
          .p
          HP-UX OPTION EQUIVALENT: -e<\"char\">"
    {
      string <char>
      editrule { append("-e" & argument); }
      required "Enter the quoted escape character."
                                                    
      ;
    }
    option seven_bits disable 1
    editrule { append("-7"); }
    help ".p
          This option sets the character
          size to 7 bits.  The eighth bit of each byte sent
          is set to 0.
          .p
          HP-UX OPTION EQUIVALENT: -7"
    ;
    option eight_bits
    editrule { append("-8"); }
    help ".p
          This option sets the character
          size to 8 bits.  This is the default HP-UX
          behavior.
          .p
          To use eight-bit characters, your terminal must be
          configured to generate either eight-bit characters with no
          parity or seven-bit characters with null parity.  You may
          also need to reconfigure the remote host appropriately.
          .p
          HP-UX OPTION EQUIVALENT: -8"
    ;
    option as_user
    help ".p
          This option allows you to log on with a
          different remote name.  By default, the remote account name
          used is the same as your local account name.
          .p
          HP-UX OPTION EQUIVALENT: -l <name>"
    {
      string <user>
      editrule { append("-l " & argument); }
      required "Enter the name of the user."
      ;
    }
  }
}
            
softkey rm command
editrule { append("rm"); }
help ".p
      You can use this command to delete files from a
      directory.
      To remove a file, you must have permission to write in the
      file's directory, but you do not need to have read or write
      permission for the file itself.
      .p
      With the \"recursively\" option, you can delete
      directories and their contents.  This option first removes
      all files within the directory, then removes the directory itself.
      .p
      If you do not have write permission for a file, you
      will be prompted to verify that the file should be removed,
      unless you select the \"quietly\" option.  Then the
      file will be removed without prompting.
      .p
      For more information, refer to rm(1)."
{
  option recurs+ively
  editrule { dash("r"); }
  help ".p
        This option allows you to remove a directory and
        the directory's contents in one step.  This option first removes
        all files within the directory, then removes the directory itself.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option interact+ively disable 1
  editrule { dash("i"); }
  help ".p
        This option prompts you to verify that a file
        should be removed.  By default, the
        file will be removed without prompting for verification (unless
        you do not have write permission for the file).
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option quietly
  editrule { dash("f"); }
  help ".p
        This option removes a file without prompting
        you to verify that the file should be removed.
        By default, you will be prompted for verification if
        you do not have write permission for the file.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
  string <files> disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to remove."
  ;
}
            
softkey rmdir command
editrule { append("rmdir"); }
help ".p
      You can use this command to remove a directory.
      By default, the directory must be completely empty before it
      can be removed.
      .p
      By default, you will be prompted for verification if
      you do not have write permission for the directory,
      unless you select the \"quietly\" option.  Then the
      directory will be removed without prompting.
      .p
      You can use the \"recursively\" option to remove a
      directory and its contents in one step.
            
      For more information, refer to rmdir(1)."
{
  option recurs+ively
  editrule { dash("r"); }
  help ".p
        This option allows you to remove a directory and
        the directory's contents in one step.  This option first removes
        all files within the directory, then removes the directory itself.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option interact+ively disable 1
  editrule { dash("i"); }
  help ".p
        This option prompts you to verify that a directory
        should be removed.  By default, the directory will be removed
        without prompting for verification (unless you do not
        have write permission for the directory).
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option quietly
  editrule { dash("f"); }
  help ".p
        This option removes a directory without prompting
        you to verify that the directory should be removed.
        By default, you will be prompted for verification if
        you do not have write permission for the directory.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
  string <dirs> disable -1
  editrule { append(argument); }
  required "Enter the name of the directory(s) to remove."
  ;
}
            
softkey sdiff
editrule { append("sdiff"); }
help ".p
      You can use this command to compare two files.  \"sdiff\"
      displays a side-by-side listing of the files indicating
      which lines are different.  By default, the display is
      130 characters wide.
      .p
      Each line of the two files is displayed as follows:
      .ip * 5
      If the lines are identical, a blank gutter appears between them.
      .ip * 5
      If the line only exists in one file, a < indicates that it
      exists in the first file, and a > indicates that it exists
      in the second file.
      .ip * 5
      If the lines are different in both files, a | appears between them.
      .p
      For more information, refer to sdiff(1)."
           
{
  option suppress_equivs
  editrule { dash("s"); }
  help ".p
        This option prevents the display of identical lines.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option display_width
  help ".p
        This option allows you to specify the output display
        width.  By default, the width is 130 characters.
        .p
        HP-UX OPTION EQUIVALENT: -w"
  {
    string <number>
    editrule { append("-w " & argument); }
    required "Enter the display width."
    help ".p
          Specify the output display width.  By default, the width is
          130 characters.
          .p
          HP-UX OPTION EQUIVALENT: -w<number>"
    ;
  }
  option merge_to_file
  help ".p
        This option allows you to merge the two files into an
        output file.  After displaying the differences in the two
        files, \"sdiff\" prompts you with a % and waits for you to type
        one of the following commands:
        .in 2
        .ip l 5
        appends the left column to the output file.
        .il r 5
        appends the right column to the output file.
        .il s 5
        turns on silent mode; does not print identical lines.
        .il v 5
        turns off silent mode.
        .il \"e l\" 5
        calls the editor with the left column.
        .il \"e r\" 5
        calls the editor with the right column.
        .il \"e b\" 5
        calls the editor with both left and right columns.
        .il e 5
        calls the editor with a zero length file.
        .il q 5
        exits from the program.
        .in -2
        .p
        HP-UX OPTION EQUIVALENT: -o<filename>"
  {
    string <file>
    editrule { append("-o " & argument); }
    required "Enter the name of the file to merge the other files into."
    ;
  }
  string <file1> enable all
  editrule { append(argument); }
  required "Enter the name of the first file to compare."
  ;
  string <file2> disabled
  editrule { append(argument); }
  required "Then, enter the name of the second file to compare."
  ;
}
            
softkey set command
editrule { append("set"); }
help ".p
      This command is used to set shell options."
{
  option emacs_mode disable all
  required "Select a mode."
  editrule { append("-o emacs"); }
  help ".p
        This option puts you in an emacs style in-line
        editor for command entry.
        .p
        HP-UX OPTION EQUIVALENT: -o emacs"
  ;
  option vi_mode disable all
  editrule { append("-o vi"); }
  help ".p
        This option puts you in a vi style in-line
        editor for command entry.
        .p
        HP-UX OPTION EQUIVALENT: -o vi"
  ;
  option export_all disable all
  editrule { append("-a"); }
  help ".p
        This option automatically exports all shell
        variables you define.
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option track_all disable all
  editrule { append("-h"); }
  help ".p
        This option causes all commands to become tracked
        aliases.  For tracked aliases, Key Shell remembers the directory where
        a command is located.  When the command is subsequently
        used, Key Shell can go to the right directory
        without first searching $PATH.  This significantly
        speeds execution time.
        .p
        HP-UX OPTION EQUIVALENT: -h"
  ;
  option ignore_eof disable all
  editrule { append("-o ignoreeof"); }
  help ".p
        This option means that you must type \"exit\" to exit
        the shell.  By default, the shell exits on end-of-file,
        usually CTRL-D.
        .p
        HP-UX OPTION EQUIVALENT: -o ignoreeof"
  ;
  option error_if_unset disable all
  editrule { append("-u"); }
  help ".p
        This option treats unset shell variables as errors when substituting.
        .p
        HP-UX OPTION EQUIVALENT: -u"
  ;
}
            
softkey shar command
editrule { append("shar"); }
help ".p
      You can use this command to bundle files into a single
      package that can be mailed or moved.  The files can include
      any data, including executables.
      .p
      The resulting package is a shell script file that can be edited.
      By default, it is written to standard output.
      For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      To unwrap the package, type \"sh <packagename>\".  The
      files in <packagename> are written to the names recorded in
      the archive.
      .p
      For more information, refer to shar(1)."
{
  option verbose+ly
  editrule { dash("v"); }
  help ".p
        This option lists the name of each file as it is
        being packed into the archive.
        .p
        HP-UX OPTION EQUIVALENT: -v"
  ;
  option error_checked
  editrule { dash("sc"); }
  help ".p
        This option performs error checking on the files
        after they are unpacked.
        .p
        HP-UX OPTION EQUIVALENT: -sc"
  ;
  option compres+sed
  editrule { dash("Z"); }
  help ".p
        This option compresses all files as they are being
        packed.  When they are unpacked, it automatically
        uncompresses them using uncompress(1).
        .p
        HP-UX OPTION EQUIVALENT: -Z"
  ;
  option by_basename
  editrule { dash("b"); }
  help ".p
        This option archives files under their base names,
        rather than their original paths.  This means that the files are
        unpacked into the current directory.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
  option no_over+write
  editrule { dash("e"); }
  help ".p
        This option prevents the archive from being unpacked
        if doing so would overwrite existing files.
        .p
        HP-UX OPTION EQUIVALENT: -e"
  ;
  option file_list disable all
  help ".p
        This option allows you to specify a file that
        contains a list of file names.  The files listed
        are then archived.
        .p
        HP-UX OPTION EQUIVALENT: -f <filename>"
  {
    string <file>
    editrule { append("-f " & argument); }
    required "Enter the name of the file that lists the files to archive."
    ;
  }
  string <files> disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to archive."
  ;
}
            
softkey sort
editrule { append("sort"); y = next; }
help ".p
      You can use this command to sort the lines of
      specified files.  By default, lines are sorted
      alphabetically.  The result is written to standard
      output.  For information on redirecting command
      input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      Comparison for sorting is based on one or more sort keys that are
      extracted from each input line.  By default, the sort
      key is the entire input line.
      .p
      For more information, refer to sort(1)."
{
  option ignoring_case
  editrule { dash("f"); y = next; }
  help ".p
        This option ignores the case of letters when sorting lines.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
  option ignoring_blanks
  editrule { dash("b"); y = next; }
  help ".p
        This option ignores leading blanks when determing
        the starting and ending positions of a sort key.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
  option numeric+ally
  editrule { dash("n"); y = next; }
  help ".p
        This option sorts an initial numeric string by increasing arithmetic
        value.
        .p
        HP-UX OPTION EQUIVALENT: -n"
  ;
  option reverse_order
  editrule { dash("r"); y = next; }
  help ".p
        This option sorts lines in reverse order.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option unique_lines
  editrule { dash("u"); y = next; }
  help ".p
        This option means that duplicate lines are stripped
        from the sort output.
        .p
        HP-UX OPTION EQUIVALENT: -u"
  ;
  option starting_at_field
  help "This option allows you to specify the
        starting field number of the input to be used as the sort key.
        .p
        HP-UX OPTION EQUIVALENT: +<number>"
  {
    string <number>
    editrule { append("+" & argument-1); }
    required "Enter the number of the first field to sort."
    ;
  }
  option starting_at_char
  help "This option allows you to specify the
        starting character number of the input to be used as the sort key."
  {
    string <number>
    editrule { if (word[last][0] == "+") { word[last] &= "." & argument-1; }
               else { append("+0." & argument-1); } }
    required "Enter the number of the first character to sort."
    ;
  }
  option ending_at_field
  help "This option allows you to specify the
        ending field number of the input to be used as the sort key.
        .p
        HP-UX OPTION EQUIVALENT: -<number>"
  {
    string <number>
    editrule { append("-" & argument); }
    required "Enter the number of the last field to sort."
    ;
  }
  option delimit+ed_by
  help "This option allows you to specify the
        character used to delimit the individual fields of
        the input.
        .p
        HP-UX OPTION EQUIVALENT: -t<\"character\">"
  {
    string <char>
    editrule { insert(y, "-t" & argument); }
    required "Enter the quoted field delimiter character."
    ;
  }
  string <files> command disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to sort."
  ;
}
            
softkey tail
editrule { append("tail"); }
help ".p
      You can use this command to display the last lines of
      a file.  By default, the last ten lines are displayed.
      .p
      For more information, refer to tail(1)."
{
  option line_count
  help ".p
        This option allows you to specify how many lines
        from the end of the file should be displayed.
        .p
        HP-UX OPTION EQUIVALENT: -<number>"
  {
    string <number>
    editrule { append("-" & trim(argument)); }
    required "Enter the number of lines to read from the end of the file."
    ;
  }
  option and_follow
  editrule { dash("f"); }
  help ".p
        This option causes the program to enter a endless
        loop, where it sleeps for one second and then attempts to
        display more records from the input file.
        .p
        You can use this option to monitor the growth of a file
        that is being written by another process.
        .p
        HP-UX OPTION EQUIVALENT: -f"
  ;
  string <file> command
  editrule { append(argument); }
  required "Enter the name of the file to tail."
  ;
}
            
softkey tar
editrule { append("tar"); }
help ".p
      You can use this command to save and restore archives
      of files.
      .p
      By default, the POSIX format is used to archive
      the files.
      .p
      For more information, refer to tar(1)."
{
  option table_of_contents disable all
  editrule { append("t"); }
  required "Select an archive function."
  help ".p
        This option lists the names of all files on the archive.
        .p
        Used with the \"verbosely\" option, it expands the listing
        to include file modes, file types, and owner and group numbers.
        .p
        HP-UX OPTION EQUIVALENT: t"
             
  {
    option verbose+ly
    editrule { word[last] &= "V"; }
    help ".p
          This option provides a long listing of all
          information about the files in the archive,
          including file modes, file types, and owner and group numbers.
          .p
          HP-UX OPTION EQUIVALENT: V"
    ;
    string <archiv> enable all
    editrule { word[last] &= "f"; append(argument); }
    required "Enter the name of the archive or raw device file."
    ;
    string <files> disabled disable -1
    editrule { append(argument); }
    hint "Enter the name of the file(s) to list."
    ;
  }
  option create_archive disable all
  editrule { append("c"); }
  help ".p
        This option lets you create a new archive.  It
        overwrites all previous information in the archive.
        If you specify
        a directory, the entire contents
        of the directory are added to the archive.
        .p
        HP-UX OPTION EQUIVALENT: c"
  {
    option old_format
    editrule { word[last] = "O" & word[last]; }
    help ".p
          This option specifies that the archive should be
          written in the old pre-POSIX format.
          .p
          By default, the POSIX format is used.
          .p
          HP-UX OPTION EQUIVALENT: O"
    ;
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file handled
          by \"tar\", preceded by a \"c\" indicating that
          the file was added to a new archive.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option follow_symlinks
    editrule { word[last] &= "h"; }
    help ".p
          This option follows symbolic links as
          if they were normal files or directories.  By default,
          symbolic links themselves are archived.
          .p
          HP-UX OPTION EQUIVALENT: h"
    ;
    option include_cdfs
    editrule { word[last] &= "H"; }
    help ".p
          This option causes all entries of context-dependent files to
          be included in the archive.  By default, \"tar\" only writes the
          entry in the CDF that matches the context of the \"tar\"
          process.
          .p
          HP-UX OPTION EQUIVALENT: H"
    ;
    string <archiv> enable all
    editrule { word[last] &= "f"; append(argument); }
    required "Enter the name of the archive or raw device file."
    ;
    string <files> disabled disable -1
    editrule { append(argument); }
    required "Then, enter the name of the file(s) to archive."
    ;
  }
  option add_to_archive disable all
  editrule { append("r"); }
  help ".p
        This option lets you add new files to the end of
        an existing archive.  If you specify
        a directory, the entire contents
        of the directory are added to the archive.
        .p
        HP-UX OPTION EQUIVALENT: r"
  {
    option old_format
    editrule { word[last] = "O" & word[last]; }
    help ".p
          This option specifies that the archive should be
          written in the old pre-POSIX format.
          .p
          By default, the POSIX format is used.
          .p
          HP-UX OPTION EQUIVALENT: O"
    ;
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file handled
          by \"tar\", preceded by an \"r\" indicating
          that the file was appended to the archive.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option follow_symlinks
    editrule { word[last] &= "h"; }
    help ".p
          This option follows symbolic links as
          if they were normal files or directories.  By default,
          symbolic links themselves are archived.
          .p
          HP-UX OPTION EQUIVALENT: h"
    ;
    option include_cdfs
    editrule { word[last] &= "H"; }
    help ".p
          This option causes all entries of context-dependent files to
          be included in the archive.  By default, \"tar\" only writes the
          entry in the CDF that matches the context of the \"tar\"
          process.
          .p
          HP-UX OPTION EQUIVALENT: H"
    ;
    string <archiv> enable all
    editrule { word[last] &= "f"; append(argument); }
    required "Enter the name of the archive or raw device file."
    ;
    string <files> disabled disable -1
    editrule { append(argument); }
    required "Then, enter the name of the file(s) to archive."
    ;
  }
  option extract_files disable all
  editrule { append("x"); o = "o"; }
  help ".p
        This option lets you extract a file from the archive
        and restore it to your system.  If you specify
        a directory in the archive, the entire contents
        of that directory are extracted.
        .p
        HP-UX OPTION EQUIVALENT: x"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option displays the name of each file handled
          by \"tar\", preceded by an \"x\" indicating that
          the file was extracted from the archive.
          .p
          HP-UX OPTION EQUIVALENT: v"
    ;
    option update_mtime
    editrule { word[last] &= "m"; }
    help ".p
          This option does not restore the
          modification time written on the archive.  Instead, the
          modification time of the files is changed to
          the time when the files were extracted from the
          archive.
          .p
          HP-UX OPTION EQUIVALENT: m"
    ;
    option retain_uid_gid
    editrule { word[last] &= "p"; o = ""; }
    help ".p
          This option restores the file to the original file protection
          modes and ownerships written on the archive, if possible.
          .p
          HP-UX OPTION EQUIVALENT: po"
    ;
    string <archiv> enable all
    editrule { word[last] &= o & "f"; append(argument); }
    required "Enter the name of the archive or raw device file."
    ;
    string <files> disabled disable -1
    editrule { append(argument); }
    hint "Enter the name of the file(s) to extract."
    ;
  }
}
            
softkey tcio
editrule { append("tcio"); }
help ".p
      You can use this command with \"cpio\"(1) to
      improve throughput and reduce wear and tear on tape
      cartridges and drives.
      .p
      For information on using commands in a
      command pipeline, refer to the help topic
      \"Redirect_pipe\".
      .p
      For more information, refer to tcio(1)."
{
  option input_tape disable all
  editrule { append("-i"); }
  required "Select \"input_tape\", \"output_tape\", or \"unload_tape\"."
  help ".p
        This option reads the specified cartridge tape unit
        and writes the data to standard output.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option enables verbose mode, so that
          information and error messages are written to standard
          output.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    option eight_k_buffer
    editrule { append("-S 8"); }
    help ".p
          This option specifies a buffer size of 8
          kilobytes.  This buffer can greatly improve performance when
          you are using a tape drive that supports immediate report.
          (These tape drives are HP7941CT, HP9144A, HP35401, HP7942,
          and HP7946.)
          
          .p
          HP-UX OPTION EQUIVALENT: -S 8"
    ;
    string <dev>
    editrule { append(argument); }
    required "Enter the name of the raw device file to read."
    ;
  }
  option output_tape disable all
  editrule { append("-o"); }
  help ".p
        This option allows you to read standard input and
        write the data to the specified cartridge tape unit.
        .p
        HP-UX OPTION EQUIVALENT: -o"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option enables verbose mode, so that
          information and error messages are displayed.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    option no_tape_mark
    editrule { word[last] &= "Z"; }
    help ".p
          This option prevents \"tcio\" from writing a tape mark
          in the first and last blocks.
          .p
          HP-UX OPTION EQUIVALENT: -Z"
               
    ;
    option use_hw_verify
    editrule { word[last] &= "V"; }
    help ".p
          This option disables tape verification.  Use this
          option only if you have a tape drive with hardware for
          verifying data output to the tape.  (These tape
          drives are HP7941CT, HP9144A, HP35401, HP7942,
          HP7946, HP7908, HP7911, HP7912, and HP7914.)
          .p
          HP-UX OPTION EQUIVALENT: -V"
    ;
    option eight_k_buffer
    editrule { append("-S 8"); }
    help ".p
          This option specifies a buffer size of 8
          kilobytes.  This buffer can greatly improve performance when
          you are using a tape drive that supports immediate report.
          (These tape drives are HP7941CT, HP9144A, HP35401, HP7942,
          and HP7946.)
          .p
          HP-UX OPTION EQUIVALENT: -S 8"
    ;
    string <dev>
    editrule { append(argument); }
    required "Enter the name of the raw device file to write."
    ;
  }
  option unload_tape disable all
  editrule { append("-urV"); }
  help ".p
        This option allows you to unload the tape from the
        drive.  On autochanger units, the tape is returned to
        the magazine.
        .p
        HP-UX OPTION EQUIVALENT: -urV"
  {
    option verbose+ly
    editrule { word[last] &= "v"; }
    help ".p
          This option enables verbose mode, so that
          information and error messages are displayed.
          .p
          HP-UX OPTION EQUIVALENT: -v"
    ;
    string <dev>
    editrule { append(argument); }
    required "Enter the name of the raw device file to unload."
    ;
  }
}
            
softkey tee
editrule { append("tee"); }
help ".p
      You can use this command with a command pipeline.
      It copies the data passing between commands to the specified
      file, allowing you to keep a copy for documentation
      or debugging purposes.  Using \"tee\" does not affect how the
      pipeline functions.
      .p
      In the following example, the output from \"who\" is
      piped into the \"tee\" command, which saves a copy of the output
      in the file \"savewho\", then passes the unchanged output to
      the \"wc\" command:
      .p
      .ti 5
      who | tee savewho | wc -l
      .p
      For information on using commands in a
      command pipeline, refer to the help topic
      \"Redirect_pipe\".
      .p
      For more information, refer to tee(1)."
{
  option ignore_intr
  editrule { dash("i"); }
  help ".p
        This option means that all interrupts are ignored.
        .p
        HP-UX OPTION EQUIVALENT: -i"
  ;
  option and_append
  editrule { dash("a"); }
  help ".p
        This option means output is appended to the
        specified file(s) rather than overwriting them.
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  string <files> disable -1
  editrule { append(argument); }
  required "Enter the name of the file(s) to tee to."
  ;
}
            
softkey touch command
editrule { append("touch"); }
help ".p
      You can use this command to update the access,
      modification, and last change times of a file.
      .p
      By default, all three times are updated to the current
      time.  If the specified file does not exist, it is created.
      .p
      For more information, refer to touch(1)."
{
  option access_time
  editrule { dash("a"); }
  help ".p
        This option updates only the file's access time.
        By default, the access, modification, and last change
        times are updated.
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option modify_time
  editrule { dash("m"); }
  help ".p
        This option updates only the file's modification time.
        By default, the access, modification, and last change
        times are updated.
        .p
        HP-UX OPTION EQUIVALENT: -m"
  ;
  option no_create
  editrule { dash("c"); }
  help ".p
        This option prevents the file from being created
        if it does not exist.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option now disable 1 enable all
  required "Select \"now\" or enter the time as <mmddhhmm[yy]>."
  help ".p
        This option allows you to specify the update time
        for the file.
        .p
        Selecting \"now\" updates the
        file to the current time.
        Entering a specific time
        updates the file to that time."
  ;
  string <time> enable all
  editrule { append(argument); }
  required "Enter the time as <mmddhhmm[yy]>."
  ;
  string <files> disabled disable -1
  editrule { append(argument); }
  required "Then, enter the name of the file(s) to touch."
  ;
}
            
softkey tr
editrule { append("tr"); }
help ".p
      You can use this command to translate or delete characters.
      By default, it reads from
      standard input and writes the translated input characters to
      standard output.
      For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      For more information, refer to tr(1)."
{
  option upper_to_lower disable all
  editrule { append("'[A-Z]' '[a-z]'"); }
  required "Select a translation function."
  help ".p
        This option translates all upper-case letters to
        lower-case.
        .p
        HP-UX OPTION EQUIVALENT: [A-Z] [a-z]"
  ;
  option lower_to_upper disable all
  editrule { append("'[a-z]' '[A-Z]'"); }
  help ".p
        This option translates all lower-case letters to
        upper-case.
        .p
        HP-UX OPTION EQUIVALENT: [a-z] [A-Z]"
  ;
  option delete_chars disable all
  editrule { dash("d"); }
  help ".p
        This option deletes all input characters which occur
        in the specified string.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  {
    option not_in
    editrule { dash("c"); }
    help ".p
          This option deletes all input characters which do
          not occur in the specified string.
          .p
          HP-UX OPTION EQUIVALENT: -c <\"string\">"
    ;
    string <chars>
    editrule { append(argument); }
    required "Enter the quoted list of characters to delete."
    ;
  }
  option map_chars
  help ".p
        This option translates any characters in string 1 to
        their corresponding characters in string 2.
        .p
        Note that you can use sequences of characters for
        string 1 and string 2.  For example, [a-z], [A-Z],
        [0-9], etc.  You can also use octal escape sequences,
        such as \"\\012\" for newline and \"\\000\"
        for the NULL character."
  {
    option and_squeeze
    editrule { dash("s"); }
    help ".p
          This option squeezes all strings of repeated
          output characters in string 2 to a single character.
          .p
          HP-UX OPTION EQUIVALENT: -s <\"string\">"
    ;
    string <chars1> enable all
    editrule { append(argument); }
    required "Enter the quoted list of characters to map."
    ;
    string <chars2> disabled
    editrule { append(argument); }
    required "Then, enter the quoted list of chars to map to."
    ;
  }
}
            
softkey umask command
editrule { append("umask"); append(""); c = ""; }
help ".p
      You can use this command to set the file creation mode
      mask.  When you create a new file or directory, the
      mask specifies the read, write, and execute
      permissions for owner, group, and others.
      .p
      For example, the softkey command \"umask writable_by
      owner\" means that a newly created file is
      readable and executable (as appropriate) by all
      others, but
      is writable only by the owner.
      .p
      For more information, refer to chmod(1) and umask(1)."
{
  option readable_by
  required "Select file modes."
  help ".p
        This option allows you to select who
        can read the file: no one, the file's owner,
        the file's owner and all members of the file's group, or all users."
  {
    option none disable all
    editrule { word[last] &= c & "-r"; c = ","; }
    required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
    help ".p
          This option prevents anyone from reading the file."
    ;
    option owner disable all
    editrule { word[last] &= c & "-r,u+r"; c = ","; }
    help ".p
          This option allows only the file's owner to read the file."
    ;
    option owner_group disable all
    editrule { word[last] &= c & "-r,ug+r"; c = ","; }
    help ".p
          This option allows the file's owner and all members of the
          file's group to read the file."
    ;
    option all
    editrule { word[last] &= c & "+r"; c = ","; }
    help ".p
          This option allows anyone to read the file."
    ;
  }
  option writable_by
  help ".p
        This option allows you to select who
        can write to the file: no one, the file's owner,
        the file's owner and all members of the file's group, or all users."
  {
    option none disable all
    editrule { word[last] &= c & "-w"; c = ","; }
    required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
    help ".p
          This option prevents anyone from writing to the file."
    ;
    option owner disable all
    editrule { word[last] &= c & "-w,u+w"; c = ","; }
    help ".p
          This option allows only the file's owner to write to the file."
    ;
    option owner_group disable all
    editrule { word[last] &= c & "-w,ug+w"; c = ","; }
    help ".p
          This option allows the file's owner and all members of the
          file's group to write to the file."
    ;
    option all
    editrule { word[last] &= c & "+w"; c = ","; }
    help ".p
          This option allows anyone to write to the file."
    ;
  }
  option executa+ble_by
  help ".p
        This option allows you to select who
        can execute the file: no one, the file's owner,
        the file's owner and all members of the file's group, or all users.
        .p
        Note that you must have execute permission for a directory
        to search through that directory."
  {
    option none disable all
    editrule { word[last] &= c & "-x"; c = ","; }
    required "Select \"none\", \"owner\", \"owner group\", or \"all\"."
    help ".p
          This option prevents anyone from executing the file.
          If the file is a directory, no one will be able to search
          through it."
    ;
    option owner disable all
    editrule { word[last] &= c & "-x,u+x"; c = ","; }
    help ".p
          This option allows only the file's owner to execute the file.
          If the file is a directory, only the owner will be able to search
          through it."
    ;
    option owner_group disable all
    editrule { word[last] &= c & "-x,ug+x"; c = ","; }
    help ".p
          This option allows the file's owner and all members of the
          file's group to execute the file.
          If the file is a directory, the file's owner and all
          members of the file's group
          can search through it."
    ;
    option all
    editrule { word[last] &= c & "+x"; c = ","; }
    help ".p
          This option means that anyone will be able to execute the file.
          If the file is a directory, anyone will be able to search
          through it."
    ;
  }
}
            
softkey uname command
editrule { append("uname"); }
help ".p
      You can use this command to display information about
      your system.  By default, it only displays the current system
      name of the HP-UX system.
      .p
      For more information, refer to uname(1)."
{
  option all_info disable all
  editrule { dash("a"); }
  help ".p
        This option displays the following information:
        .ip * 5
        Operating system name.
        .ip * 5
        Node name.
        .ip * 5
        Operating system release and version.
        .ip * 5
        Machine hardware name.
        .p
        HP-UX OPTION EQUIVALENT: -a"
  ;
  option system_name
  editrule { dash("s"); }
  help ".p
        This option displays the operating system name.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option node_name
  editrule { dash("n"); }
  help ".p
        This option displays the node name of the system.
        .p
        HP-UX OPTION EQUIVALENT: -n"
  ;
  option os_release
  editrule { dash("r"); }
  help ".p
        This option displays the operating system release.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option os_version
  editrule { dash("v"); }
  help ".p
        This option displays the operating system version.
        .p
        HP-UX OPTION EQUIVALENT: -v"
  ;
  option machine_name
  editrule { dash("m"); }
  help ".p
        This option displays the machine hardware name.
        .p
        HP-UX OPTION EQUIVALENT: -m"
  ;
}
            
softkey vi command
editrule { append("vi"); }
help ".p
      You can use this command to create and edit text
      files.  \"vi\" is a screen-oriented text editor.
      .p
      For information about using \"vi\", refer to
      the \"Beginner's Guide to HP-UX\".
      .p
      For more information, refer to vi(1)."
{
  option recover_file
  editrule { append("-r"); }
  help ".p
        This option allows you to recover a text file that was
        open when the system crashed, or when a window was closed.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option read_only
  editrule { append("-R"); }
  help ".p
        This option lets you view a file in read-only mode so that
        you cannot accidentally overwrite it.
        .p
        HP-UX OPTION EQUIVALENT: -R"
  ;
  option encrypt+ed
  editrule { append("-x"); }
  help ".p
        This option sets encryption mode.  It prompts you for
        a key to allow you to create or edit an encrypted file.
        .p
        HP-UX OPTION EQUIVALENT: -x"
  ;
  option starting_at_line
  help ".p
        This option lets you choose the line number at
        which to start editing the file.
        .p
        HP-UX OPTION EQUIVALENT: +<line_number>"
  {
    string <number>
    editrule { append("+" & argument); }
    required "Enter the line number to start editing at."
    ;
  }
  string <files> disable -1
  editrule { append(argument); }
  hint "Enter the name of the file(s) to edit."
  ;
}
            
softkey wc
editrule { append("wc"); }
help ".p
      You can use this command to count lines, words,
      or characters.  By default,
      lines, words, and characters are counted.
      .p
      This command can read from standard input or from
      a file.  For information on redirecting command input and output,
      refer to the help topic \"Redirect_pipe\".
      .p
      For more information, refer to wc(1)."
{
  option chars
  editrule { dash("c"); }
  help ".p
        Use this option to count the number of characters.
        By default, lines, words, and characters are counted.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option words
  editrule { dash("w"); }
  help ".p
        Use this option to count the number of words.
        By default, lines, words, and characters are counted.
        .p
        HP-UX OPTION EQUIVALENT: -w"
  ;
  option lines
  editrule { dash("l"); }
  help ".p
        Use this option to count the number of lines.
        By default, lines, words, and characters are counted.
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
  string <files> disable -1 command
  editrule { append(argument); }
  required "Enter the name of the file(s) to count."
  ;
}
            
softkey who
editrule { append("who"); }
help ".p
      You can use this command to list information about current
      users on the system, logins, logoffs, and reboots.
      .p
      By default, the listing shows the following fields, from
      left to right:
      .ip * 5
      The name of the user who is logged in.
      .ip * 5
      The name of the terminal to which the user is logged in.
      .ip * 5
      The date and time at which the user logged in.
      .p
      For more information, refer to who(1)."
{
  option verbose+ly
  editrule { dash("uTH"); }
  help ".p
        Use this option to list the following information:
        .ip * 5
        The name of the user who is logged in.
        .ip * 5
        The state of the terminal.  A \"+\" appears if the terminal is
        writable by anyone.  A \"-\" appears if it is not.
        .ip * 5
        The name of the terminal to which the user is logged in.
        .ip * 5
        The date and time at which the user logged in.
        .ip * 5
        The number of hours and minutes since activity last occurred
        on a particular terminal line.  (If more than twenty-four
        hours have elapsed, or the line has not been used since boot
        time, it is marked \"old\".  If the line has had activity in
        the last minute, it is marked \".\".)
        .ip * 5
        The process ID of the login shell associated with the terminal.
        .p
        HP-UX OPTION EQUIVALENT: -uTH"
  ;
  option idle_tty_lines
  editrule { dash("l"); }
  help ".p
        This option lists only those terminal lines which
        are idle and waiting for someone to log on.
        .p
        HP-UX OPTION EQUIVALENT: -l"
  ;
  option time_of_boot
  editrule { dash("b"); }
  help ".p
        This option displays the time and date of the last reboot.
        .p
        HP-UX OPTION EQUIVALENT: -b"
  ;
  option init_run_level
  editrule { dash("r"); }
  help ".p
        This option displays the current run-level of the
        \"init\" process.  The last three fields show the current
        state of \"init\", the number of times that state has been
        entered, and the previous state.
        .p
        HP-UX OPTION EQUIVALENT: -r"
  ;
  option for_all_cnodes
  editrule { dash("c"); }
  help ".p
        This option displays information for an entire HP-UX
        cluster.  It shows the name of each machine, who is logged
        in on the system, their terminal line, and login time.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  string <file>
  editrule { append(argument); }
  hint "Enter the name of the utmp-format file to read."
  ;
}
            
softkey write
editrule { append("write"); }
help ".p
      You can use this command to interactively write to
      another user.  \"write\" copies lines from your terminal to
      the terminal of another user.
      .p
      To establish a connection, type
      \"write username\".  This sends a message to \"username\"
      indicating that you want to write to them.  For two-way
      conversation to take place, the recipient must then type
      \"write yourusername\".
      .p
      To write to a user who is logged in more than once,
      use the \"on_tty\" option to specify which line or
      terminal to write to (e.g., tty00).
      .p
      For more information, refer to write(1)."
{
  string <user> enable all
  editrule { append(argument); }
  required "Enter the name of the user to write to."
  ;
  option on_tty disabled
  help "This option allows you to
        specify which line or terminal to write to (e.g., tty04)
        when you are writing to a user who is logged in more than once.
        .p
        If you do not specify a terminal, the first writable
        instance of the user in \"/etc/utmp\" is used.
        .p
        HP-UX OPTION EQUIVALENT: <tty#>"
  {
    string <tty>
    editrule { append(argument); }
    required "Enter the name of the tty (e.g., tty04)."
    ;
  }
}
            
softkey xd
editrule { append("xd"); }
help ".p
      You can use this command to create a
      hexadecimal dump of a file.
      .p
      For more information, refer to xd(1)."
{
  option hex_bytes disable 3
  editrule { dash("b"); }
  help ".p
        This option interprets bytes in hexadecimal.
        .p
        HP-UX OPTION EQUIVALENT: -b"
             
  ;
  option chars disable 2
  editrule { dash("c"); }
  help ".p
        This option interprets bytes in ASCII.
        .p
        HP-UX OPTION EQUIVALENT: -c"
  ;
  option decimal_words disable 1
  editrule { dash("d"); }
  help ".p
        This option interprets 16-bit words in decimal.
        .p
        HP-UX OPTION EQUIVALENT: -d"
  ;
  option signed_words
  editrule { dash("s"); }
  help ".p
        This option interprets 16-bit words in signed decimal.
        .p
        HP-UX OPTION EQUIVALENT: -s"
  ;
  option from_offset
  help "This option specifies the offset in the file where
        dumping is to start.  Dumping continues to the
        end of the file.
        .p
        HP-UX OPTION EQUIVALENT: +<number>"
  {
    string <offset>
    cleanuprule { append("+" & argument); }
    required "Enter the octal or hex byte offset to start dumping at."
    ;
  }
  string <files> command
  editrule { append(argument); }
  required "Enter the name of the file to dump."
  ;
}
        
softkey xdb command
editrule { append("xdb"); }
help ".p
      You can use this command to perform source level
      debugging of C, HP FORTRAN, and HP Pascal programs.
      .p
      For more information, refer to xdb(1)."
{
  option source_dir disable -1
  help ".p
        This option allows you to specify an alternate
        directory for source files.  Alternate directories are
        searched in the order given.  If a source file is not found
        in any of the alternate directories, the current directory
        is searched last.
        .p
        HP-UX OPTION EQUIVALENT: -d <dir>"
           
  {
    string <dir>
    editrule { append("-d" & argument); }
    required "Enter the name of a source directory."
    ;
  }
  option record_file
  help ".p
        This option allows you to specify a record
        file that is invoked immediately for overwrite.
        .p
        Then, all commands entered by the user are saved
        in this file for future playback until record mode
        is turned off with the debugger command \">f\".
        .p
        HP-UX OPTION EQUIVALENT: -r <filename>"
  {
    string <file>
    editrule { append("-r" & argument); }
    required "Enter the name of the file to record to."
    ;
  }
  option playback_file
  help ".p
        This option allows you to specify a playback
        file that is invoked immediately.
        .p
        All commands are read from this file before
        the user can enter any other commands.
        .p
        HP-UX OPTION EQUIVALENT: -p <filename>"
  {
    string <file>
    editrule { append("-p" & argument); }
    required "Enter the name of the file to play back from."
    ;
  }
  option adopt_process disable 1
  help ".p
        This option allows you to specify the
        process ID of an existing process that you wish to
        adopt and debug.
        .p
        HP-UX OPTION EQUIVALENT: -P <filename>"
  {
    string <pid>
    editrule { append("-P" & argument); }
    required "Enter the pid of the process to adopt and debug."
    ;
  }
  option redirect_to
  help ".p
        This option allows you to redirect the standard
        input, standard output, and standard error to a file.
        .p
        HP-UX OPTION EQUIVALENT: -i <filename> -o <filename> -e <filename>"
  {
    string <file>
    editrule { append("-i" & argument);
               append("-o" & argument);
               append("-e" & argument); }
    required "Enter the name of the file to redirect program i/o to."
    ;
  }
  string <prog> enable all
  editrule { append(argument); }
  required "Enter the name of the program to debug."
  ;
  string <core> disabled
  editrule { append(argument); }
  hint "Enter the name of the core file."
  ;
}
    
