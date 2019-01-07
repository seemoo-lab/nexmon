# Sed script for modify the files in tests directory.

s|\.in\([1-2]\{1,1\}\)\.po|.i\1-po|g
s|\.ok\.po|.ok-po|g
s|\.in\.po|_in.po|g
s|\.in\.\([cC]\{1,1\}\)|_in.\1|g
s|\.po\.|.po-|g
