#include "code.h"

softkey Keysh_config
code CODEnop
help ".p
      You can use this command to configure Key Shell's
      appearance and behavior.  The options to this command allow
      you to do the following tasks:
      .ip * 5
      Add, move, and delete softkeys.
      .ip * 5
      Change global options.
      .ip * 5
      Change the status line.
      .ip * 5
      Write configuration changes to your .keyshrc file.
      .ip * 5
      Restart Key Shell.
      .ip * 5
      Undo any configuration changes."
{
                  
/****************************** softkey **************************************/
                  
  option softkey disable all
  required "Select a configuration option."
  help ".p
        This option allows you to add, move, or delete softkeys
        from the top-level softkey menu.
        .p
        It also allows you to add additional invisible softkeys
        and backup softkeys."
  {
                              
/**************************** softkey add ************************************/
                              
    option add disable all
    code CODEsoftKeyAdd
    required "Select \"add\", \"move\", or \"delete\"."
    help ".p
          This option allows you to add new softkeys
          to the top-level softkey menu.
          It also allows you to add additional invisible softkeys
          or backup softkeys.
          All pre-configured softkeys are listed under the
          help topics \"visibles\" and \"invisibles\".
          .p
          By default, softkeys are read from the standard softkey
          definitions file, \"/usr/keysh/C/softkeys\".
          By default, visible softkeys are labelled
          in the top-level
          softkey menu with the name of the softkey itself.
          .p
          Note that when you add a softkey,
          the remaining softkeys from that file are automatically
          loaded for use as invisible softkey commands."
    {
      option invisi+bles disable all
      code CODEsoftKeyInvisibles
      help ".p
            This option allows you to add all of the
            softkeys from a particular softkey file as invisible
            softkey commands.
            .p
            By default, invisible softkeys are read from the standard
            softkey definitions file, \"/usr/keysh/C/softkeys\".
            .p
            For information about
            invisible softkeys and a list of the pre-configured
            invisible softkeys, refer to the help topic \"invisibles\"."
             
      {
        option from_user disable 1
        help ".p
              This option allows you to add
              invisible softkeys from the \"$HOME/.softkeys\" file of
              another user.
              .p
              By default, invisible softkeys are read from the standard
              softkey definitions file, \"/usr/keysh/C/softkeys\"."
        {
          string <user>
          code CODEsoftKeyUser
          required
            "Enter the name of the user whose .softkeys file should be read."
          ;
        }
        option from_file
        code CODEsoftKeyIFrom
        help ".p
              This option allows you to add
              invisible softkeys from a particular softkey file.
              .p
              By default, invisible softkeys are read from the standard
              softkey definitions file, \"/usr/keysh/C/softkeys\"."
        {
          string <file>
          code CODEsoftKeyFile
          required "Enter the name of the custom softkey file."
          ;
        }
      }
                    
      option backups disable all
      code CODEsoftKeyBackups
      help ".p
            This option allows you to add backup
            softkeys.  Backup softkeys are displayed when Key Shell
            cannot display any of its own softkeys (for example,
            when a command is executing.)
            .p
            Key Shell does not contain pre-defined backup softkeys:
            you must create them.
            By default, backup softkeys are read from the \".softkeys\"
            file in your $HOME directory.
            .p
            For more information, refer to the online help for
            \"Keysh_config options backups\".
            Also refer to keysh(1) and softkeys(4)."
      {
        option from_user disable 1
        help ".p
              This option allows you to add
              backup softkeys from the \"$HOME/.softkeys\" file of
              another user.
              .p
              By default, backup softkeys are read from the
              \".softkeys\" file in your $HOME directory."
        {
          string <user>
          code CODEsoftKeyUser
          required
            "Enter the name of the user whose .softkeys file should be read."
          ;
        }
        option from_file
        code CODEsoftKeyBFrom
        help ".p
              This option allows you to add
              backup softkeys from a particular softkey file.
              .p
              By default, backup softkeys are read from the
              \".softkeys\" file in your $HOME directory."
        {
          string <file>
          code CODEsoftKeyFile
          required "Enter the name of the custom softkey file."
          ;
        }
      }
                    
      string <sftkey> disable all
      code CODEsoftKeyASoftKey
      required "Enter the name of the softkey to add."
      {
        option with_label
        code CODEsoftKeyWith
        help ".p
              This option allows you to specify an alternate,
              less cryptic label for a top-level softkey.
              .p
              By default, the label displayed in the top-level
              softkey menu will be the softkey name itself."
        {
          string <label>
          code CODEsoftKeyLabel
          required "Enter the alternate label for the softkey."
          ;
        }
        option from_user disable 1
        help ".p
              This option allows you to add
              softkeys from the \"$HOME/.softkeys\" file of
              another user.
              .p
              By default, softkeys are read from the standard
              softkey definitions file, \"/usr/keysh/C/softkeys\".
              .p
              Note that when you add a softkey,
              the remaining softkeys from that file are automatically
              loaded for use as invisible softkey commands."
        {
          string <user>
          code CODEsoftKeyUser
          required
            "Enter the name of the user whose .softkeys file should be read."
          ;
        }
        option from_file
        code CODEsoftKeyFrom
        help ".p
              This option allows you to add
              softkeys from a particular softkey file.
              .p
              By default, softkeys are read from the standard
              softkey definitions file, \"/usr/keysh/C/softkeys\".
              .p
              Note that when you add a softkey,
              the remaining softkeys from that file are automatically
              loaded for use as invisible softkey commands."
        {
          string <file>
          code CODEsoftKeyFile
          required "Enter the name of the custom softkey file."
          ;
        }
        option and_place
        help ".p
              This option allows you to specify where in the
              top-level softkey menu the new softkey should be placed.
              .p
              By default, it will be placed
              on the last bank of softkeys, after
              all the other softkeys."
        {
          option as_first_softkey disable all
          code CODEsoftKeyFirst
          required "Select where the new softkey should be placed."
          help ".p
                This option allows you to place the
                new softkey in the first bank of softkeys, before
                all the other softkeys.
                .p
                By default, it will be placed
                on the last bank of softkeys, after
                all the other softkeys."
          ;
          option as_last_softkey disable all
          help ".p
                This option places the new softkey
                in the default location:
                on the last bank of softkeys, after
                all the other softkeys."
          ;
          option before_softkey disable all
          help ".p
                This option allows you to place the
                new softkey before a particular
                softkey in the top-level softkey menu.
                .p
                By default, it will be placed
                on the last bank of softkeys, after
                all the other softkeys."
          {
            string <sftkey>
            code CODEsoftKeyBefore
            required "Enter the name or label of an existing softkey."
            ;
          }
        }
      }
    }
                
/****************************** softkey move *********************************/
                
    option move disable all
    code CODEsoftKeyPlace
    help ".p
          This option allows you to move softkeys
          within the top-level softkey menu."
    {
      string <sftkey>
      code CODEsoftKeyPSoftKey
      required "Enter the name or label of the softkey to move."
      {
        option to_first_softkey disable all
        code CODEsoftKeyFirst
        required "Select where the softkey should be moved to."
        help ".p
              This option causes the softkey to be moved to the
              first bank of softkeys, before all the other
              softkeys."
        ;
        option to_last_softkey disable all
        help ".p
              This option causes the new softkey to be
              moved to the last bank of softkeys, after all the
              other softkeys."
        ;
        option before_softkey disable all
        help ".p
              This option allows you to place the
              new softkey before a particular
              softkey in the top-level softkey menu."
        {
          string <sftkey>
          code CODEsoftKeyBefore
          required "Enter the name or label of an existing softkey."
          ;
        }
      }
    }
                
/**************************** softkey delete *********************************/
                
    option delete
    code CODEsoftKeyDelete
    help ".p
          This option allows you to delete softkeys from
          the top-level softkey menu.
          .p
          Note that after you delete a softkey, you can still
          access that softkey as an invisible command by
          typing the command name."
    {
      string <sftkey>
      code CODEsoftKeyDSoftKey
      required "Enter the name or label of the softkey to delete."
      ;
    }
  }
            
/********************************* options ***********************************/
            
  option options disable all
  code CODEopt
  help ".p
        This option allows you to turn the global configuration options
        on and off.  Global options allow you to control such things as
        which type of softkeys are available, whether HP-UX translations are
        displayed, and whether prompts are given.
        .p
        An asterisk displayed next to an option name means that
        the option is on.
        In the options menu, global options can be turned on
        and off with the following softkey commands:
        .p
        \"Keysh_config options <option softkey> on\" .br
        \"Keysh_config options <option softkey> off\"
        .p
        The following list explains the global options:
        .p
        .ti 10
        GLOBAL OPTIONS
        .in 2
        .ip OPTION 20
        DESCRIPTION
        .ip backups 20
        Enabled by default.  Backup softkeys are the softkeys that
        Key Shell can display when it cannot display its own softkeys
        (for example, when the \"vi\" editor is running).
        If you set backups to off,
        Key Shell blanks the softkeys when it cannot
        display its own softkeys.
        For more information, see the
        online help for \"Keysh_config options backups\".
        .ip help 20
        Enabled by default.  If you set help to off, the
        \"--Help--\" softkey disappears, but help is generally
        still available by using the \"Tab\" key.
        .ip invisibles 20
        Enabled by default.  If you set
        invisibles to off, Key Shell will not recognize invisible
        softkey commands.
        .ip prompts 20
        Enabled by default.  If you set prompts to off,
        Key Shell will not display prompt messages describing
        actions that are required to complete the current softkey
        command.
        .ip selectors 20
        Disabled by default.
        If you set \"selectors\" to on, an uppercase selector
        character appears in each softkey label.
        Typing this character (unquoted) selects
        the softkey.  Key Shell automatically sets
        \"selectors\" to on if you are using a
        terminal that does not support a sufficient
        number of softkeys.
        .ip translations 20
        Enabled by default.  If you set
        translations to off, Key Shell will not display the HP-UX
        translations of softkey commands before executing them.
        .ip visibles 20
        Enabled by default.  If you set visibles
        to off, Key Shell will not display softkey commands
        on the top-level softkey menu.
        .sp
        If you are familiar with HP-UX commands (but not
        necessarily with the options), you may wish to
        set \"visibles\" to off.  You can then decrease Key
        Shell start-up time by editing \".keyshrc\" and
        removing the lines that add visible softkeys.
        .in -2"
  {
    option backups
    required "Select option(s) to turn on or off."
    code CODEoptBackups
    help ".p
          This option allows you to enable or disable the
          programming of backup softkeys.
          Backup softkeys are the softkeys that Key Shell
          can display when it cannot display its own softkeys
          (for example, when the \"vi\" editor is running).
          Backup softkeys provide the static softkey control that
          you may have used before.
          You can program backup softkeys
          to generate sequences of characters that you type often,
          or that are hard to remember.
          .p
          By default, Key Shell does not contain backup softkeys.
          To use backup softkeys, you must define them in your
          \".softkeys\" file and add them with the
          \"Keysh_config softkey add backups\" command.
          .p
          If the \"backups\" option is enabled, Key Shell displays the backup
          softkeys and
          programs the terminal function keys appropriately whenever
          it has no other softkeys to display (such as when a command
          is running).  Otherwise Key Shell uses the terminfo(4) defaults.
          .p
          Visible and invisible softkeys are interpreted by Key Shell,
          and thus
          will work on any terminal.
          Backup softkeys are interpreted by the terminal itself, and
          thus may not work on every terminal.
          .p
          To avoid confusion with Key Shell's softkeys, you
          may wish to use either backup softkeys or visible
          and invisible softkeys, but not both.
          .p
          BACKUP SOFTKEY FORMAT:
          .p
          A backup softkey has the following general
          format:
          .sp
          .in 5
          backup softkey <softkey>  literal <literal> ;
          .sp
          .in -5
                    
          <softkey> is the label for this softkey.  <literal>
          is the literal text string used to program the
          terminal function key.
          .p
          For example, the following lines define a backup
          softkey that transmits two \"escapes\" to perform
          file name completion on the command line:
          .sp
          .in 5
          backup softkey \"esc_esc\" literal \"\\033\\033\" ;
          .sp
          .in -5
          For more information, refer to keysh(1)
          and softkeys(4)."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptBackupsOn
      ;
      option off
      code CODEoptBackupsOff
      ;
    }
    option help
    code CODEoptHelp
    help  ".p
          Enabled by default.  If you set help to off, the
          \"--Help--\" softkey disappears, but help is generally
          still available by using the \"Tab\" key.
          .p
          For more information on using help, refer
          to the help topic \"Online help\".
          "
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptHelpOn
      ;
      option off
      code CODEoptHelpOff
      ;
    }
    option invisi+bles
    code CODEoptInvisibles
    help ".p
          Enabled by default.  If you set
          invisibles to off, Key Shell will not recognize invisible
          softkey commands.
          .p
          For more information on invisible
          softkeys, refer to the help topic \"invisibles\"."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptInvisiblesOn
      ;
      option off
      code CODEoptInvisiblesOff
      ;
    }
    option prompts
    code CODEoptPrompts
    help ".p
          Enabled by default.  If you set prompts to off,
          Key Shell will not display a prompt message describing
          actions that are required to complete the current softkey
          command."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptPromptsOn
      ;
      option off
      code CODEoptPromptsOff
      ;
    }
    option select+ors
    code CODEoptAccelerators
    help ".p
          Disabled by default.
          When \"selectors\" is on, an uppercase selector
          character appears in each softkey label.
          Typing this character (unquoted) selects
          the softkey.  Key Shell automatically sets
          \"selectors\" to on if you are using a
          terminal that does not support a sufficient
          number of softkeys."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptAcceleratorsOn
      ;
      option off
      code CODEoptAcceleratorsOff
      ;
    }
    option transla+tions
    code CODEoptTranslation
    help ".p
          Enabled by default.  If you set
          translations to off, Key Shell will not display the HP-UX
          translations of softkey commands."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptTranslationOn
      ;
      option off
      code CODEoptTranslationOff
      ;
    }
    option visibles
    code CODEoptVisibles
    help ".p
          Enabled by default.  If you set visibles
          to off, Key Shell will not display softkey commands
          on the top-level softkey menu.
          .p
          For more information, refer to the help topic
          \"visibles\"."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEoptVisiblesOn
      ;
      option off
      code CODEoptVisiblesOff
      ;
    }
  }
            
/******************************* status line *********************************/
            
  option status_line disable all
  code CODEstatus
  help ".p
        This option allows you to configure the
        status line by enabling and disabling these status
        line indicators.  An asterisk next to an indicator
        means that the indicator is on.
        .p
        .ti 10
        STATUS LINE INDICATORS
        .in 2
        .ip ITEM 20
        DESCRIPTION
        .ip user_name 20
        Your user name.  Disabled by default.
        .il host_name 20
        Your host name.  Enabled by default.
        .il current_dir 20
        The current directory.  Enabled by default.
        .il mail_status 20
        The mail status (\"You have mail\", \"No mail\", or
        \"You have new mail\").  Enabled by default.
        .il date 20
        The date.  Disabled by default.
        .il time 20
        The time of day.  Enabled by default.
        "
  {
    option user_name
    code CODEstatusUser
    required "Select status line indicator(s) to turn on or off."
    help ".p
          This option allows you to enable or disable the
          display of the user name in the status line."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEstatusUserOn
      ;
      option off
      code CODEstatusUserOff
      ;
    }
    option host_name
    code CODEstatusHost
    help ".p
          This option allows you to enable or disable the
          display of the host name in the status line."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEstatusHostOn
      ;
      option off
      code CODEstatusHostOff
      ;
    }
    option current_dir
    code CODEstatusCurrentDir
    help ".p
          This option allows you to enable or disable the
          display of the current directory in the status line."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEstatusCurrentDirOn
      ;
      option off
      code CODEstatusCurrentDirOff
      ;
    }
    option mail_status
    code CODEstatusMail
    help ".p
          This option allows you to enable or disable the
          display of the mail status (e.g., \"You have mail\",
          \"No mail\", or \"You have new mail\")
          in the status line."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEstatusMailOn
      ;
      option off
      code CODEstatusMailOff
      ;
    }
    option date
    code CODEstatusDate
    help ".p
          This option allows you to enable or disable the
          display of the date in the status line."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEstatusDateOn
      ;
      option off
      code CODEstatusDateOff
      ;
    }
    option time
    code CODEstatusTime
    help ".p
          This option allows you to enable or disable the
          display of the time in the status line."
    {
      option on disable all
      required "Select \"on\" or \"off\"."
      code CODEstatusTimeOn
      ;
      option off
      code CODEstatusTimeOff
      ;
    }
  }
            
/************************************ restart ********************************/
            
  option restart disable all
  code CODEreRead
  help ".p
        This option causes Key Shell to reread its
        configuration from the \"$HOME/.keyshrc\" file
        or from the default user configuration file,
        \"/usr/keysh/C/keyshrc\".
        .p
        After changing Key Shell's configuration in one window, you can
        update the configuration of any other windows by
        selecting the \"restart\" option in those windows.
        .p
        If you want Key Shell to restart from the original default
        configuration, select this option, then
        select the \"default\" option.  Any changes you have made
        to the \".keyshrc\" file will be lost.
        .p
        Refer also to the online help for \"Keysh_config write\"."
  {
    option default
    code CODEdefault
    help ".p
          This option causes Key Shell to restart
          using the default configuration information in
          the \"/usr/keysh/C/keyshrc\" file.
          Any changes you have made to your \".keyshrc\" file
          will be lost.
          .p
          Without this option, Key Shell restarts with the configuration
          information in the \"$HOME/.keyshrc\" file."
    ;
  }
            
/*********************************** write ***********************************/
            
  option write disable all
  code CODEflush
  help ".p
        This option will manually write the configuration
        changes you have made to your \".keyshrc\" file.
        Key Shell automatically writes configuration changes
        as you make them.
        .p
        For example, if you have
        configured Key Shell differently
        in two windows and you do not want to
        keep one of the configurations, go to
        the window with the configuration you want and select
        this option to write that configuration to \".keyshrc\".
        Then go to the other window and restart Key Shell.
        .p
        Refer also to the online help for \"Keysh_config restart\"."
  ;
            
/*********************************** undo ************************************/
            
  option undo disable all
  code CODErestore
  help ".p
        This option will undo all configuration changes made since
        Key Shell was invoked or since the last \"Keysh_config undo\"
        command.  It then rewrites your \".keyshrc\" file to
        reflect the undone changes.
        .p
        Selecting this option a second time will restore your
        configuration changes."
  ;
            
}
        
        
/*****************************************************************************/
