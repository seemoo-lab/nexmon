Title: GVariant Text Format

# GVariant Text Format

This page attempts to document the `GVariant` text format as produced by
[`method@GLib.Variant.print`] and parsed by the [`func@GLib.Variant.parse`]
family of functions. In most cases the style closely resembles the
formatting of literals in Python but there are some additions and
exceptions.

The functions that deal with `GVariant` text format absolutely always deal
in UTF-8. Conceptually, `GVariant` text format is a string of Unicode
characters, not bytes. Non-ASCII but otherwise printable Unicode characters
are not treated any differently from normal ASCII characters.

The parser makes two passes. The purpose of the first pass is to determine
the type of the value being parsed. The second pass does the actual parsing.
Based on the fact that all elements in an array have to have the same type,
`GVariant` is able to make some deductions that would not otherwise be
possible. As an example:

    [[1, 2, 3], [4, 5, 6]]

is parsed as an array of arrays of integers (type `aai`), but

    [[1, 2, 3], [4, 5, 6.0]]

is parsed as an array of arrays of doubles (type `aad`).

As another example, `GVariant` is able to determine that

    ["hello", nothing]

is an array of maybe strings (type `ams`).

What the parser accepts as valid input is dependent on context. The API
permits for out-of-band type information to be supplied to the parser (which
will change its behaviour). This can be seen in the GSettings and GDBus
command line utilities where the type information is available from the
schema or the remote introspection information. The additional information
can cause parses to succeed when they would not otherwise have been able to
(by resolving ambiguous type information) or can cause them to fail (due to
conflicting type information). Unless stated otherwise, the examples given
in this section assume that no out-of-band type data has been given to the
parser.

## Syntax Summary

The following table describes the rough meaning of symbols that may appear
inside GVariant text format. Each symbol is described in detail in its own
section, including usage examples.

| Symbol | Meaning |
|--------|---------|
| `true`, `false` | [Booleans](#booleans). |
| `""`, `''` | String literal. See [Strings](#strings) below. |
| numbers | See [Numbers](#numbers) below. |
| `()` | [Tuples](#tuples). |
| `[]` | [Arrays](#arrays). |
| `{}` | [Dictionaries and Dictionary Entries](#dictionaries-and-dictionary-entries). |
| `<>` | [Variants](#variants). |
| `just`, `nothing` | [Maybe Types](#maybe-types). |
| `@` | [Type Annotations](#type-annotations). |
| `boolean`, `byte`, `int16`, `uint16`, `int32`, `uint32`, `handle`, `int64`, `uint64`, `double`, `string`, `objectpath`, `signature` | [Type Annotations](#type-annotations) |
| `b""`, `b''` | [Bytestrings](#bytestrings). |
| `%` | [Positional Parameters](#positional-parameters). |

## Booleans

The strings `true` and `false` are parsed as booleans. This is the only way
to specify a boolean value.

## Strings

Strings literals must be quoted using `""` or `''`. The two are completely
equivalent (except for the fact that each one is unable to contain itself
unescaped).

Strings are Unicode strings with no particular encoding. For example, to
specify the character `é`, you just write `'é'`. You could also give the
Unicode codepoint of that character (`U+E9`) as the escape sequence
`'\u00e9'`. Since the strings are pure Unicode, you should not attempt to
encode the UTF-8 byte sequence corresponding to the string using escapes; it
won't work and you'll end up with the individual characters corresponding to
each byte.

Unicode escapes of the form `\uxxxx` and `\Uxxxxxxxx` are supported, in
hexadecimal. The [usual control sequence escapes][C escape sequences]
`\a`, `\b`, `\f`, `\n`, `\r`, `\t` and `\v` are supported.
Additionally, a `\` before a newline character causes the newline to be ignored.
Finally, any other character following `\` is copied literally
(for example, `\"` or `\\`) but for forwards compatibility with future additions
you should only use this feature when necessary for escaping backslashes or quotes.

The usual octal and hexadecimal escapes `\nnn` and `\xnn` are not supported
here. Those escapes are used to encode byte values and `GVariant` strings
are Unicode.

Single-character strings are not interpreted as bytes. Bytes must be
specified by their numerical value.

## Numbers

Numbers are given by default as decimal values. Octal and hex values can be
given in the usual way (by prefixing with `0` or `0x`). Note that `GVariant`
considers bytes to be unsigned integers and will print them as a two digit
hexadecimal number by default.

Floating point numbers can also be given in the usual ways, including
scientific and hexadecimal notations.

For lack of additional information, integers will be parsed as `int32`
values by default. If the number has a point or an `e` in it, then it will
be parsed as a double precision floating point number by default. If type
information is available (either explicitly or inferred) then that type will
be used instead.

Some examples:

`5` parses as the `int32` value five.

`37.5` parses as a floating point value.

`3.75e1` parses the same as the value above.

`uint64 7` parses seven as a `uint64`. See [Type Annotations](#type-annotations).

## Tuples

Tuples are formed using the same syntax as Python. Here are some examples:

`()` parses as the empty tuple.

`(5,)` is a tuple containing a single value.

`("hello", 42)` is a pair. Note that values of different types are
permitted.

## Arrays

Arrays are formed using the same syntax as Python uses for lists (which is
arguably the term that `GVariant` should have used). Note that, unlike Python
lists, `GVariant` arrays are statically typed. This has two implications.

First, all items in the array must have the same type. Second, the type of
the array must be known, even in the case that it is empty. This means that
(unless there is some other way to infer it) type information will need to
be given explicitly for empty arrays.

The parser is able to infer some types based on the fact that all items in
an array must have the same type. See the examples below:

`[1]` parses (without additional type information) as a one-item array of
signed integers.

`[1, 2, 3]` parses (similarly) as a three-item array.

`[1, 2, 3.0]` parses as an array of doubles. This is the most simple case of
the type inferencing in action.

`[(1, 2), (3, 4.0)]` causes the 2 to also be parsed as a double (but the 1
and 3 are still integers).

`["", nothing]` parses as an array of maybe strings. The presence of
"nothing" clearly implies that the array elements are nullable.

`[[], [""]]` will parse properly because the type of the first (empty) array
can be inferred to be equal to the type of the second array (both are arrays
of strings).

`[b'hello', []]` looks odd but will parse properly. See
[Bytestrings](#bytestrings).

And some examples of errors:

`["hello", 42]` fails to parse due to conflicting types.

`[]` will fail to parse without additional type information.

## Dictionaries and Dictionary Entries

Dictionaries and dictionary entries are both specified using the `{}`
characters.

The dictionary syntax is more commonly used. This is what the printer elects
to use in the normal case of dictionary entries appearing in an array (AKA
"a dictionary"). The separate syntax for dictionary entries is typically
only used for when the entries appear on their own, outside of an array
(which is valid but unusual). Of course, you are free to use the dictionary
entry syntax within arrays but there is no good reason to do so (and the
printer itself will never do so). Note that, as with arrays, the type of
empty dictionaries must be established (either explicitly or through
inference).

The dictionary syntax is the same as Python's syntax for dictionaries. Some
examples:

`@a{sv} {}` parses as the empty dictionary of everyone's favourite type.

`@a{sv} []` is the same as above (owing to the fact that dictionaries are
really arrays).

`{1: "one", 2: "two", 3: "three"}` parses as a dictionary mapping integers
to strings.

The dictionary entry syntax looks just like a pair (2-tuple) that uses
braces instead of parens. The presence of a comma immediately following the
key differentiates it from the dictionary syntax (which features a colon
after the first key). Some examples:

`{1, "one"}` is a free-standing dictionary entry that can be parsed on its
own or as part of another container value.

`[{1, "one"}, {2, "two"}, {3, "three"}]` is exactly equivalent to the
dictionary example given above.

## Variants

Variants are denoted using angle brackets (aka "XML brackets"), `<>`. They
may not be omitted.

Using `<>` effectively disrupts the type inferencing that occurs between
array elements. This can have positive and negative effects.

`[<"hello">, <42>]` will parse whereas `["hello", 42]` would not.

`[<['']>, <[]>]` will fail to parse even though `[[''], []]` parses
successfully. You would need to specify `[<['']>, <@as []>]`.

`{"title": <"frobit">, "enabled": <true>, "width": <800>}` is an example of
perhaps the most pervasive use of both dictionaries and variants.

## Maybe Types

The syntax for specifying maybe types is inspired by Haskell.

The null case is specified using the keyword nothing and the non-null case
is explicitly specified using the keyword just. GVariant allows just to be
omitted in every case that it is able to unambiguously determine the
intention of the writer. There are two cases where it must be specified:

- when using nested maybes, in order to specify the just nothing case
- to establish the nullability of the type of a value without explicitly
  specifying its full type

Some examples:

`just 'hello'` parses as a non-null nullable string.

`@ms 'hello'` is the same (demonstrating how just can be dropped if the type is already known).

`nothing` will not parse without extra type information.

`@ms nothing` parses as a null nullable string.

`[just 3, nothing]` is an array of nullable integers

`[3, nothing]` is the same as the above (demonstrating another place were just can be dropped).

`[3, just nothing]` parses as an array of maybe maybe integers (type `ammi`).

## Type Annotations

Type annotations allow additional type information to be given to the
parser. Depending on the context, this type information can change the
output of the parser, cause an error when parsing would otherwise have
succeeded or resolve an error when parsing would have otherwise failed.

Type annotations come in two forms: type codes and type keywords.

Type keywords can be seen as more verbose (and more legible) versions of a
common subset of the type codes. The type keywords `boolean`, `byte`,
`int16`, `uint16`, `int32`, `uint32`, `handle`, `int64`, `uint64`, `double`,
`string`, `objectpath` and literal signature are each exactly equivalent to
their corresponding type code.

Type codes are an `@` ("at" sign) followed by a definite `GVariant` type
string. Some examples:

`uint32 5` causes the number to be parsed unsigned instead of signed (the
default).

`@u 5` is the same

`objectpath "/org/gnome/xyz"` creates an object path instead of a normal
string

`@au []` specifies the type of the empty array (which would not parse
otherwise)

`@ms ""` indicates that a string value is meant to have a maybe type

## Bytestrings

The bytestring syntax is a piece of syntactic sugar meant to complement the
bytestring APIs in GVariant. It constructs arrays of non-`NUL` bytes (type
`ay`) with a `NUL` terminator at the end. These are normal C strings with no
particular encoding enforced, so the bytes may not be valid UTF-8.
Bytestrings are a special case of byte arrays; byte arrays (also type 'ay'),
in the general case, can contain a `NUL` byte at any position, and need not
end with a `NUL` byte.

Bytestrings are specified with either `b""` or `b''`. As with strings, there
is no fundamental difference between the two different types of quotes.

Like in strings, the [C-style control sequence escapes][C escape sequences]
`\a`, `\b`, `\f`, `\n`, `\r`, `\t` and `\v` are supported. Similarly,
a `\` before a newline character causes the newline to be ignored.
Unlike in strings, you can use octal escapes of the form `\nnn`.
Finally, any other character following `\` is copied literally
(for example, `\"` or `\\`) but for forwards compatibility
with future additions you should only use this feature when necessary
for escaping backslashes or quotes. Unlike in strings, Unicode escapes
are not supported.

`b'abc'` is equivalent to `[byte 0x61, 0x62, 0x63, 0]`.

When formatting arrays of bytes, the printer will choose to display the
array as a bytestring if it contains a nul character at the end and no other
nul bytes within. Otherwise, it is formatted as a normal array.

## Positional Parameters

Positional parameters are not a part of the normal `GVariant` text format,
but they are mentioned here because they can be used with
[`ctor@GLib.Variant.new_parsed`].

A positional parameter is indicated with a `%` followed by any valid
[GVariant Format String](gvariant-format-strings.html). Variable arguments
are collected as specified by the format string and the resulting value is
inserted at the current position.

This feature is best explained by example:

```c
char *t = "xyz";
gboolean en = false;
GVariant *value;

value = g_variant_new_parsed ("{'title': <%s>, 'enabled': <%b>}", t, en);
```

This constructs a dictionary mapping strings to variants (type `a{sv}`) with
two items in it. The key names are parsed from the string and the values for
those keys are taken as variable arguments parameters.

The arguments are always collected in the order that they appear in the
string to be parsed. Format strings that collect multiple arguments are
permitted, so you may require more varargs parameters than the number of `%`
signs that appear. You can also give format strings that collect no
arguments, but there's no good reason to do so.

[C escape sequences]: https://en.wikipedia.org/wiki/Escape_sequences_in_C#Escape_sequences
