#!/bin/sh
# Print the team's address (to stdout) and output additional instructions
# (to stderr).
projectsdir="$1"
progdir="$2"
catalog="$3"  # e.g. "pt_BR"
language="$4" # e.g. "pt"

url=`cat "$projectsdir/TP/teams.url"`
sed_absolute_dotdot_urls="s,href=\"\\.\\./,href="`echo "$url" | sed -e 's,/[^/]*/[^/]*\$,/,'`",g"
html=`"$progdir/urlget" "$url" "$projectsdir/TP/teams.html" | sed -e "$sed_absolute_dotdot_urls"`
sed_addnl='s,</tr>,</tr>\
,g'
address=`echo "$html" | tr '\012' '|' | sed -e "$sed_addnl" | sed -n -e "s,^.*<td>$catalog</td>[^<>]*<td>[^|]*</td>[^<>]*<td><a href=\"\\([^\"]*\\)\">[^<>]*</a></td>.*\$,\\1,p" | sed 1q`
if test -n "$address"; then
  case "$address" in
    mailto:*) address=`echo "$address" | sed -e 's,^mailto:,<,' -e 's,$,>,'` ;;
  esac
  (echo "Please visit your translation team's homepage at"
   echo "  "`echo "$html" | tr '\012' '|' | sed -e "$sed_addnl" | sed -n -e "s,^.*<td>$catalog</td>[^<>]*<td><a href=\"\\([^\"]*\\)\">[^<>]*</a></td>.*\$,\\1,p" | sed 1q`
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/teams.html"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/translators.html"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/index.html"
   echo "and consider joining your translation team's mailing list"
   echo "  $address"
  ) 1>&2
  echo "$address"
  exit 0
fi
address=`echo "$html" | tr '\012' '|' | sed -e "$sed_addnl" | sed -n -e "s,^.*<td>$language</td>[^<>]*<td>[^|]*</td>[^<>]*<td><a href=\"\\([^\"]*\\)\">[^<>]*</a></td>.*\$,\\1,p" | sed 1q`
if test -n "$address"; then
  case "$address" in
    mailto:*) address=`echo "$address" | sed -e 's,^mailto:,<,' -e 's,$,>,'` ;;
  esac
  (echo "A translation team exists for your language ($language) but not for"
   echo "your local dialect ($catalog).  You can either join the existing"
   echo "translation team for $language or create a new translation team for $catalog."
   echo
   echo "Please visit the existing translation team's homepage at"
   echo "  "`echo "$html" | tr '\012' '|' | sed -e "$sed_addnl" | sed -n -e "s,^.*<td>$language</td>[^<>]*<td><a href=\"\\([^\"]*\\)\">[^<>]*</a></td>.*\$,\\1,p" | sed 1q`
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/teams.html"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/translators.html"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/index.html"
   echo "and consider joining the translation team's mailing list"
   echo "  $address"
   echo
   echo "If you want to create a new translation team for $catalog, please visit"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/teams.html"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/leaders.html"
   echo "  http://www.iro.umontreal.ca/contrib/po/HTML/index.html"
  ) 1>&2
  echo "$address"
  exit 0
fi
(echo "A translation team for your language ($language) does not exist yet."
 echo "If you want to create a new translation team for $language"`test "$catalog" = "$language" || echo " or $catalog"`", please visit"
 echo "  http://www.iro.umontreal.ca/contrib/po/HTML/teams.html"
 echo "  http://www.iro.umontreal.ca/contrib/po/HTML/leaders.html"
 echo "  http://www.iro.umontreal.ca/contrib/po/HTML/index.html"
) 1>&2
exit 0
