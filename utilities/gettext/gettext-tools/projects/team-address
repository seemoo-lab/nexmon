#!/bin/sh
# Print the team's address (to stdout) and output additional instructions
# (to stderr).
projectsdir="$1"
progdir="$2"
catalog="$3"  # e.g. "pt_BR"
language="$4" # e.g. "pt"

for project in `cat "$projectsdir/index"`; do
  if /bin/sh "$projectsdir/$project/trigger"; then
    /bin/sh "$projectsdir/$project/team-address" "$projectsdir" "$progdir" "$catalog" "$language"
    exit $?
  fi
done

exit 0
