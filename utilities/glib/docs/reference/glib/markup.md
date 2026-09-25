Title: Simple XML Subset Parser
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2011, 2014 Matthias Clasen
SPDX-FileCopyrightText: 2012 David King

# Simple XML Subset Parser

The "GMarkup" parser is intended to parse a simple markup format
that's a subset of XML. This is a small, efficient, easy-to-use
parser. It should not be used if you expect to interoperate with
other applications generating full-scale XML, and must not be used if you
expect to parse untrusted input. However, it's very useful for application
data files, config files, etc. where you know your application will be the
only one writing the file.

Full-scale XML parsers should be able to parse the subset used by
GMarkup, so you can easily migrate to full-scale XML at a later
time if the need arises.

GMarkup is not guaranteed to signal an error on all invalid XML;
the parser may accept documents that an XML parser would not.
However, XML documents which are not well-formed (which is a
weaker condition than being valid. See the
[XML specification](http://www.w3.org/TR/REC-xml/)
for definitions of these terms.) are not considered valid GMarkup
documents.

## Simplifications to XML

The simplifications compared to full XML include:

 - Only UTF-8 encoding is allowed
 - No user-defined entities
 - Processing instructions, comments and the doctype declaration
   are "passed through" but are not interpreted in any way
 - No DTD or validation

The markup format does support:

 - Elements
 - Attributes
 - 5 standard entities: `&amp;` `&lt;` `&gt;` `&quot;` `&apos;`
 - Character references
 - Sections marked as CDATA

## An example parser

Here is an example for a markup parser:
[markup-example.c](https://gitlab.gnome.org/GNOME/glib/-/blob/HEAD/glib/tests/markup-example.c)

