Title: Base64 Encoding
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2006, 2009 Matthias Clasen

# Base64 Encoding

Base64 is an encoding that allows a sequence of arbitrary bytes to be
encoded as a sequence of printable ASCII characters. For the definition
of Base64, see [RFC 1421](http://www.ietf.org/rfc/rfc1421.txt) or
[RFC 2045](http://www.ietf.org/rfc/rfc2045.txt).
Base64 is most commonly used as a MIME transfer encoding for email.

GLib supports incremental encoding using [func@GLib.base64_encode_step] and
[func@GLib.base64_encode_close]. Incremental decoding can be done with
[func@GLib.base64_decode_step]. To encode or decode data in one go, use
[func@GLib.base64_encode] or [func@GLib.base64_decode]. To avoid memory
allocation when decoding, you can use [func@GLib.base64_decode_inplace].

Support for Base64 encoding was added in GLib 2.12.

