#! /usr/bin/sed -f
#
# Each line of the form ^...   .* contains the code for a language.
# The categorization in column 60 should be a space, denoting a living language,
# or an U.
# The speakers number in columns 62..69 should be at least a million.
#
/^...   ......................................................[ U]..[^ ]/ {
  h
  s/^...   \(.*[^ ]\)   .*$/\1./
  s/ç/@,{c}/g
  s/´/'/g
  x
  s/^\(...\).*/@item \1/
  G
  p
}
#
# delete the rest
#
d
