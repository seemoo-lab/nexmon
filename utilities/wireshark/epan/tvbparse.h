/* tvbparse.h
*
* an API for text tvb parsers
*
* Copyright 2005, Luis E. Garcia Ontanon <luis@ontanon.org>
*
* Wireshark - Network traffic analyzer
* By Gerald Combs <gerald@wireshark.org>
* Copyright 1998 Gerald Combs
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
 The intention behind this is to ease the writing of dissectors that have to
 parse text without the need of writing into buffers.

 It was originally written to avoid using lex and yacc for the xml dissector.

 the parser is able to look for wanted elements these can be:

 simple tokens:
 - a char out of a string of needles
 - a char not belonging to a string of needles
 - a sequence of chars that belong to a set of chars
 - a sequence of chars that do not belong to a set of chars
 - a string
 - a caseless string
 - all the characters up to a certain wanted element (included or excluded)

 composed elements:
 - one of a given group of wanted elements
 - a sequence of wanted elements
 - some (at least one) instances of a wanted element

 Once a wanted element is successfully extracted, by either tvbparse_get or
 tvbparse_find, the parser will invoke a given callback
 before and another one after every of its component's subelement's callbacks
 are being called.

 If tvbparse_get or tvbparse_find fail to extract the wanted element the
 subelements callbacks are not going to be invoked.

 The wanted elements are instantiated once by the proto_register_xxx function.

 The parser is instantiated for every packet and it mantains its state.

 The element's data is destroyed before the next packet is dissected.
 */

#ifndef _TVB_PARSE_H_
#define _TVB_PARSE_H_

#include <epan/tvbuff.h>
#include <glib.h>
#include "ws_symbol_export.h"

typedef struct _tvbparse_elem_t tvbparse_elem_t;
typedef struct _tvbparse_wanted_t tvbparse_wanted_t;
typedef struct _tvbparse_t tvbparse_t;


/*
 * a callback function to be called before or after an element has been
 * successfuly extracted.
 *
 * Note that if the token belongs to a composed token the callbacks of the
 * components won't be called unless the composed token is successfully
 * extracted.
 *
 * tvbparse_data: the private data of the parser
 * wanted_data: the private data of the wanted element
 * elem: the extracted element
 */
typedef void (*tvbparse_action_t)(void* tvbparse_data, const void* wanted_data, struct _tvbparse_elem_t* elem);

typedef int (*tvbparse_condition_t)
(tvbparse_t*, const int,
 const tvbparse_wanted_t*,
 tvbparse_elem_t**);


typedef enum  {
    TP_UNTIL_INCLUDE, /* last elem is included, its span is spent by the parser */
    TP_UNTIL_SPEND, /* last elem is not included, but its span is spent by the parser */
    TP_UNTIL_LEAVE /* last elem is not included, neither its span is spent by the parser */
} until_mode_t;


struct _tvbparse_wanted_t {
    int id;
    tvbparse_condition_t condition;

    union {
        const gchar* str;
        struct _tvbparse_wanted_t** handle;
        struct {
            union {
                gint64 i;
                guint64 u;
                gdouble f;
            } value;
            gboolean (*comp)(void*,const void*);
            void* (*extract)(tvbuff_t*,guint);
        } number;
        enum ftenum ftenum;
        struct {
            until_mode_t mode;
            const tvbparse_wanted_t* subelem;
        } until;
        struct {
            wmem_map_t* table;
            struct _tvbparse_wanted_t* key;
            struct _tvbparse_wanted_t* other;
        } hash;
        GPtrArray* elems;
        const tvbparse_wanted_t* subelem;
        void* p;
    } control;

    int len;

    guint min;
    guint max;

    const void* data;

    tvbparse_action_t before;
    tvbparse_action_t after;
};

/* an instance of a per packet parser */
struct _tvbparse_t {
    tvbuff_t* tvb;
    int offset;
    int end_offset;
    void* data;
    const tvbparse_wanted_t* ignore;
};


/* a matching token returned by either tvbparser_get or tvb_parser_find */
struct _tvbparse_elem_t {
    int id;

    tvbuff_t* tvb;
    int offset;
    int len;

    void* data;

    struct _tvbparse_elem_t* sub;

    struct _tvbparse_elem_t* next;
    struct _tvbparse_elem_t* last;

    const tvbparse_wanted_t* wanted;
};


/*
 * definition of wanted token types
 *
 * the following functions define the tokens we will be able to look for in a tvb
 * common parameters are:
 *
 * id: an arbitrary id that will be copied to the eventual token (don't use 0)
 * private_data: persistent data to be passed to the callback action (wanted_data)
 * before_cb: an callback function to be called before those of the subelements
 * after_cb: an callback function to be called after those of the subelements
 */


/*
 * a char element.
 *
 * When looked for it returns a simple element one character long if the char
 * at the current offset matches one of the the needles.
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_char(const int id,
                                 const gchar* needles,
                                 const void* private_data,
                                 tvbparse_action_t before_cb,
                                 tvbparse_action_t after_cb);

/*
 * a not_char element.
 *
 * When looked for it returns a simple element one character long if the char
 * at the current offset does not match one of the the needles.
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_not_char(const int id,
                                     const gchar* needle,
                                     const void* private_data,
                                     tvbparse_action_t before_cb,
                                     tvbparse_action_t after_cb);

/*
 * a chars element
 *
 * When looked for it returns a simple element one or more characters long if
 * one or more char(s) starting from the current offset match one of the needles.
 * An element will be returned if at least min_len chars are given (1 if it's 0)
 * It will get at most max_len chars or as much as it can if max_len is 0.
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_chars(const int id,
                                  const guint min_len,
                                  const guint max_len,
                                  const gchar* needles,
                                  const void* private_data,
                                  tvbparse_action_t before_cb,
                                  tvbparse_action_t after_cb);

/*
 * a not_chars element
 *
 * When looked for it returns a simple element one or more characters long if
 * one or more char(s) starting from the current offset do not match one of the
 * needles.
 * An element will be returned if at least min_len chars are given (1 if it's 0)
 * It will get at most max_len chars or as much as it can if max_len is 0.
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_not_chars(const int id,
                                      const guint min_len,
                                      const guint max_len,
                                      const gchar* needles,
                                      const void* private_data,
                                      tvbparse_action_t before_cb,
                                      tvbparse_action_t after_cb);

/*
 * a string element
 *
 * When looked for it returns a simple element if we have the given string at
 * the current offset
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_string(const int id,
                                   const gchar* string,
                                   const void* private_data,
                                   tvbparse_action_t before_cb,
                                   tvbparse_action_t after_cb);

/*
 * casestring
 *
 * When looked for it returns a simple element if we have a matching string at
 * the current offset
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_casestring(const int id,
                                       const gchar* str,
                                       const void* data,
                                       tvbparse_action_t before_cb,
                                       tvbparse_action_t after_cb);

/*
 * until
 *
 * When looked for it returns a simple element containing all the characters
 * found until the first match of the ending element if the ending element is
 * found.
 *
 * When looking for until elements it calls tvbparse_find so it can be very slow.
 *
 * It won't have a subelement, the ending's callbacks won't get called.
 */

/*
 * op_mode values determine how the terminating element and the current offset
 * of the parser are handled
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_until(const int id,
                                  const void* private_data,
                                  tvbparse_action_t before_cb,
                                  tvbparse_action_t after_cb,
                                  const tvbparse_wanted_t* ending,
                                  until_mode_t until_mode);

/*
 * one_of
 *
 * When looked for it will try to match to the given candidates and return a
 * composed element whose subelement is the first match.
 *
 * The list of candidates is terminated with a NULL
 *
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_set_oneof(const int id,
                                      const void* private_data,
                                      tvbparse_action_t before_cb,
                                      tvbparse_action_t after_cb,
                                      ...);

/*
 * hashed
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_hashed(const int id,
                                   const void* data,
                                   tvbparse_action_t before_cb,
                                   tvbparse_action_t after_cb,
                                   tvbparse_wanted_t* key,
                                   tvbparse_wanted_t* other,
                                   ...);

WS_DLL_PUBLIC
void tvbparse_hashed_add(tvbparse_wanted_t* w, ...);

/*
 * sequence
 *
 * When looked for it will try to match in order all the given candidates. If
 * every candidate is found in the given order it will return a composed
 * element whose subelements are the matcheed elemets.
 *
 * The list of candidates is terminated with a NULL.
 *
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_set_seq(const int id,
                                    const void* private_data,
                                    tvbparse_action_t before_cb,
                                    tvbparse_action_t after_cb,
                                    ...);

/*
 * some
 *
 * When looked for it will try to match the given candidate at least min times
 * and at most max times. If the given candidate is matched at least min times
 * a composed element is returned.
 *
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_some(const int id,
                                 const guint min,
                                 const guint max,
                                 const void* private_data,
                                 tvbparse_action_t before_cb,
                                 tvbparse_action_t after_cb,
                                 const tvbparse_wanted_t* wanted);

#define tvbparse_one_or_more(id, private_data, before_cb, after_cb, wanted)\
    tvbparse_some(id, 1, G_MAXINT, private_data, before_cb, after_cb, wanted)


/*
 * handle
 *
 * this is a pointer to a pointer to a wanted element (that might have not
 * been initialized yet) so that recursive structures
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_handle(tvbparse_wanted_t** handle);

#if 0

enum ft_cmp_op {
    TVBPARSE_CMP_GT,
    TVBPARSE_CMP_GE,
    TVBPARSE_CMP_EQ,
    TVBPARSE_CMP_NE,
    TVBPARSE_CMP_LE,
    TVBPARSE_CMP_LT
};

/* not yet tested */
tvbparse_wanted_t* tvbparse_ft(int id,
                               const void* data,
                               tvbparse_action_t before_cb,
                               tvbparse_action_t after_cb,
                               enum ftenum ftenum);

/* not yet tested */
tvbparse_wanted_t* tvbparse_end_of_buffer(int id,
                                          const void* data,
                                          tvbparse_action_t before_cb,
                                          tvbparse_action_t after_cb);
/* not yet tested */
tvbparse_wanted_t* tvbparse_ft_numcmp(int id,
                                      const void* data,
                                      tvbparse_action_t before_cb,
                                      tvbparse_action_t after_cb,
                                      enum ftenum ftenum,
                                      int little_endian,
                                      enum ft_cmp_op ft_cmp_op,
                                      ... );

#endif

/*  quoted
 *  this is a composed candidate, that will try to match a quoted string
 *  (included the quotes) including into it every escaped quote.
 *
 *  C strings are matched with tvbparse_quoted(-1,NULL,NULL,NULL,"\"","\\")
 */
WS_DLL_PUBLIC
tvbparse_wanted_t* tvbparse_quoted(const int id,
                                   const void* data,
                                   tvbparse_action_t before_cb,
                                   tvbparse_action_t after_cb,
                                   const char quote,
                                   const char escape);

/*
 * a helper callback for quoted strings that will shrink the token to contain
 * only the string andnot the quotes
 */
WS_DLL_PUBLIC
void tvbparse_shrink_token_cb(void* tvbparse_data,
                              const void* wanted_data,
                              tvbparse_elem_t* tok);




/* initialize the parser (at every packet)
 * tvb: what are we parsing?
 * offset: from where
 * len: for how many bytes
 * private_data: will be passed to the action callbacks
 * ignore: a wanted token type to be ignored (the associated cb WILL be called when it matches)
 */
WS_DLL_PUBLIC
tvbparse_t* tvbparse_init(tvbuff_t* tvb,
                          const int offset,
                          int len,
                          void* private_data,
                          const tvbparse_wanted_t* ignore);

/* reset the parser */
WS_DLL_PUBLIC
gboolean tvbparse_reset(tvbparse_t* tt, const int offset, int len);

WS_DLL_PUBLIC
guint tvbparse_curr_offset(tvbparse_t* tt);
guint tvbparse_len_left(tvbparse_t* tt);



/*
 * This will look for the wanted token at the current offset or after any given
 * number of ignored tokens returning FALSE if there's no match or TRUE if there
 * is a match.
 * The parser will be left in its original state and no callbacks will be called.
 */
WS_DLL_PUBLIC
gboolean tvbparse_peek(tvbparse_t* tt,
                       const tvbparse_wanted_t* wanted);

/*
 * This will look for the wanted token at the current offset or after any given
 * number of ignored tokens returning NULL if there's no match.
 * if there is a match it will set the offset of the current parser after
 * the end of the token
 */
WS_DLL_PUBLIC
tvbparse_elem_t* tvbparse_get(tvbparse_t* tt,
                              const tvbparse_wanted_t* wanted);

/*
 * Like tvbparse_get but this will look for a wanted token even beyond the
 * current offset.
 * This function is slow.
 */
WS_DLL_PUBLIC
tvbparse_elem_t* tvbparse_find(tvbparse_t* tt,
                               const tvbparse_wanted_t* wanted);


WS_DLL_PUBLIC
void tvbparse_tree_add_elem(proto_tree* tree, tvbparse_elem_t* curr);

#endif
