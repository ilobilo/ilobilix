# Begin ~/.bashrc
# Written for Beyond Linux From Scratch
# by James Robertson <jameswrobertson@earthlink.net>

# Personal aliases and functions.

# Personal environment variables and startup programs should go in
# ~/.bash_profile.  System wide environment variables and startup
# programs are in /etc/profile.  System wide aliases and functions are
# in /etc/bash.bashrc.

if [ -f "/etc/profile" ] ; then
  source /etc/profile
fi

if [ -f "/etc/bash.bashrc" ] ; then
  source /etc/bash.bashrc
fi

# End ~/.bashrc