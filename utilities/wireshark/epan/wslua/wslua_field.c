/*
 *  wslua_field.c
 *
 * Wireshark's interface to the Lua Programming Language
 *
 * (c) 2006, Luis E. Garcia Ontanon <luis@ontanon.org>
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

#include "config.h"

#include <epan/dfilter/dfilter.h>
#include <epan/ftypes/ftypes-int.h>

/* WSLUA_MODULE Field Obtaining dissection data */

#include "wslua.h"

/* any call to checkFieldInfo() will now error on null or expired, so no need to check again */
WSLUA_CLASS_DEFINE(FieldInfo,FAIL_ON_NULL_OR_EXPIRED("FieldInfo"));
/*
   An extracted Field from dissected packet data. A `FieldInfo` object can only be used within
   the callback functions of dissectors, post-dissectors, heuristic-dissectors, and taps.

   A `FieldInfo` can be called on either existing Wireshark fields by using either `Field.new()`
   or `Field()` before-hand, or it can be called on new fields created by Lua from a `ProtoField`.
 */

static GPtrArray* outstanding_FieldInfo = NULL;

FieldInfo* push_FieldInfo(lua_State* L, field_info* f) {
    FieldInfo fi = (FieldInfo) g_malloc(sizeof(struct _wslua_field_info));
    fi->ws_fi = f;
    fi->expired = FALSE;
    g_ptr_array_add(outstanding_FieldInfo,fi);
    return pushFieldInfo(L,fi);
}

CLEAR_OUTSTANDING(FieldInfo,expired,TRUE)

/* WSLUA_ATTRIBUTE FieldInfo_len RO The length of this field. */
WSLUA_METAMETHOD FieldInfo__len(lua_State* L) {
    /*
       Obtain the Length of the field
       */
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushnumber(L,fi->ws_fi->length);
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_offset RO The offset of this field. */
WSLUA_METAMETHOD FieldInfo__unm(lua_State* L) {
    /*
       Obtain the Offset of the field
       */
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushnumber(L,fi->ws_fi->start);
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_value RO The value of this field. */
WSLUA_METAMETHOD FieldInfo__call(lua_State* L) {
    /*
       Obtain the Value of the field.

       Previous to 1.11.4, this function retrieved the value for most field types,
       but for `ftypes.UINT_BYTES` it retrieved the `ByteArray` of the field's entire `TvbRange`.
       In other words, it returned a `ByteArray` that included the leading length byte(s),
       instead of just the *value* bytes. That was a bug, and has been changed in 1.11.4.
       Furthermore, it retrieved an `ftypes.GUID` as a `ByteArray`, which is also incorrect.

       If you wish to still get a `ByteArray` of the `TvbRange`, use `FieldInfo:get_range()`
       to get the `TvbRange`, and then use `Tvb:bytes()` to convert it to a `ByteArray`.
       */
    FieldInfo fi = checkFieldInfo(L,1);

    switch(fi->ws_fi->hfinfo->type) {
        case FT_BOOLEAN:
                lua_pushboolean(L,(int)fvalue_get_uinteger64(&(fi->ws_fi->value)));
                return 1;
        case FT_UINT8:
        case FT_UINT16:
        case FT_UINT24:
        case FT_UINT32:
        case FT_FRAMENUM:
                lua_pushnumber(L,(lua_Number)(fvalue_get_uinteger(&(fi->ws_fi->value))));
                return 1;
        case FT_INT8:
        case FT_INT16:
        case FT_INT24:
        case FT_INT32:
                lua_pushnumber(L,(lua_Number)(fvalue_get_sinteger(&(fi->ws_fi->value))));
                return 1;
        case FT_FLOAT:
        case FT_DOUBLE:
                lua_pushnumber(L,(lua_Number)(fvalue_get_floating(&(fi->ws_fi->value))));
                return 1;
        case FT_INT64: {
                pushInt64(L,(Int64)(fvalue_get_sinteger64(&(fi->ws_fi->value))));
                return 1;
            }
        case FT_UINT64: {
                pushUInt64(L,fvalue_get_uinteger64(&(fi->ws_fi->value)));
                return 1;
            }
        case FT_ETHER: {
                Address eth = (Address)g_malloc(sizeof(address));
                alloc_address_tvb(NULL,eth,AT_ETHER,fi->ws_fi->length,fi->ws_fi->ds_tvb,fi->ws_fi->start);
                pushAddress(L,eth);
                return 1;
            }
        case FT_IPv4:{
                Address ipv4 = (Address)g_malloc(sizeof(address));
                alloc_address_tvb(NULL,ipv4,AT_IPv4,fi->ws_fi->length,fi->ws_fi->ds_tvb,fi->ws_fi->start);
                pushAddress(L,ipv4);
                return 1;
            }
        case FT_IPv6: {
                Address ipv6 = (Address)g_malloc(sizeof(address));
                alloc_address_tvb(NULL,ipv6,AT_IPv6,fi->ws_fi->length,fi->ws_fi->ds_tvb,fi->ws_fi->start);
                pushAddress(L,ipv6);
                return 1;
            }
        case FT_FCWWN: {
                Address fcwwn = (Address)g_malloc(sizeof(address));
                alloc_address_tvb(NULL,fcwwn,AT_FCWWN,fi->ws_fi->length,fi->ws_fi->ds_tvb,fi->ws_fi->start);
                pushAddress(L,fcwwn);
                return 1;
            }
        case FT_IPXNET:{
                Address ipx = (Address)g_malloc(sizeof(address));
                alloc_address_tvb(NULL,ipx,AT_IPX,fi->ws_fi->length,fi->ws_fi->ds_tvb,fi->ws_fi->start);
                pushAddress(L,ipx);
                return 1;
            }
        case FT_ABSOLUTE_TIME:
        case FT_RELATIVE_TIME: {
                NSTime nstime = (NSTime)g_malloc(sizeof(nstime_t));
                *nstime = *(NSTime)fvalue_get(&(fi->ws_fi->value));
                pushNSTime(L,nstime);
                return 1;
            }
        case FT_STRING:
        case FT_STRINGZ: {
                gchar* repr = fvalue_to_string_repr(NULL, &fi->ws_fi->value,FTREPR_DISPLAY,BASE_NONE);
                if (repr)
                {
                    lua_pushstring(L, repr);
                    wmem_free(NULL, repr);
                }
                else
                {
                    luaL_error(L,"field cannot be represented as string because it may contain invalid characters");
                }
                return 1;
            }
        case FT_NONE:
                if (fi->ws_fi->length > 0 && fi->ws_fi->rep) {
                    /* it has a length, but calling fvalue_get() on an FT_NONE asserts,
                       so get the label instead (it's a FT_NONE, so a label is what it basically is) */
                    lua_pushstring(L, fi->ws_fi->rep->representation);
                    return 1;
                }
                return 0;
        case FT_BYTES:
        case FT_UINT_BYTES:
        case FT_REL_OID:
        case FT_SYSTEM_ID:
        case FT_OID:
            {
                ByteArray ba = g_byte_array_new();
                g_byte_array_append(ba, (const guint8 *) fvalue_get(&fi->ws_fi->value),
                                    fvalue_length(&fi->ws_fi->value));
                pushByteArray(L,ba);
                return 1;
            }
        case FT_PROTOCOL:
            {
                ByteArray ba = g_byte_array_new();
                tvbuff_t* tvb = (tvbuff_t *) fvalue_get(&fi->ws_fi->value);
                g_byte_array_append(ba, (const guint8 *)tvb_memdup(wmem_packet_scope(), tvb, 0,
                                            tvb_captured_length(tvb)), tvb_captured_length(tvb));
                pushByteArray(L,ba);
                return 1;
            }

        case FT_GUID:
        default:
                luaL_error(L,"FT_ not yet supported");
                return 1;
    }
}

/* WSLUA_ATTRIBUTE FieldInfo_label RO The string representing this field. */
WSLUA_METAMETHOD FieldInfo__tostring(lua_State* L) {
    /* The string representation of the field. */
    FieldInfo fi = checkFieldInfo(L,1);

    if (fi->ws_fi->value.ftype->val_to_string_repr) {
        gchar* repr = NULL;

        if (fi->ws_fi->hfinfo->type == FT_PROTOCOL || fi->ws_fi->hfinfo->type == FT_PCRE) {
            repr = fvalue_to_string_repr(NULL, &fi->ws_fi->value,FTREPR_DFILTER,BASE_NONE);
        }
        else {
            repr = fvalue_to_string_repr(NULL, &fi->ws_fi->value,FTREPR_DISPLAY,fi->ws_fi->hfinfo->display);
        }

        if (repr) {
            lua_pushstring(L,repr);
            /* fvalue_to_string_repr() wmem_alloc's the string's buffer */
            wmem_free(NULL, repr);
        }
        else {
            lua_pushstring(L,"(unknown)");
        }
    }
    else if (fi->ws_fi->hfinfo->type == FT_NONE) {
        lua_pushstring(L, "(none)");
    }
    else {
        lua_pushstring(L,"(n/a)");
    }

    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_display RO The string display of this field as seen in GUI. */
static int FieldInfo_get_display(lua_State* L) {
    /* The display string of this field as seen in GUI. */
    FieldInfo fi = checkFieldInfo(L,1);
    gchar         label_str[ITEM_LABEL_LENGTH+1];
    gchar        *label_ptr;
    gchar        *value_ptr;

    if (!fi->ws_fi->rep) {
        label_ptr = label_str;
        proto_item_fill_label(fi->ws_fi, label_str);
    } else
        label_ptr = fi->ws_fi->rep->representation;

    if (!label_ptr) return 0;

    value_ptr = strstr(label_ptr, ": ");
    if (!value_ptr) {
        /* just use whatever's there */
        lua_pushstring(L, label_ptr);
    } else {
        value_ptr += 2;  /* get past the ': ' */
        lua_pushstring(L, value_ptr);
    }

    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_type RO The internal field type, a number which
   matches one of the `ftype` values in `init.lua`.

   @since 1.99.8
 */
static int FieldInfo_get_type(lua_State* L) {
    FieldInfo fi = checkFieldInfo(L,1);

    if (fi->ws_fi->hfinfo) {
        lua_pushnumber(L, fi->ws_fi->hfinfo->type);
    }
    else {
        lua_pushnil(L);
    }

    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_source RO The source `Tvb` object the `FieldInfo` is derived
    from, or nil if there is none.

   @since 1.99.8
 */
static int FieldInfo_get_source(lua_State* L) {
    FieldInfo fi = checkFieldInfo(L,1);

    if (fi->ws_fi->ds_tvb) {
        push_Tvb(L, fi->ws_fi->ds_tvb);
    }
    else {
        lua_pushnil(L);
    }

    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_range RO The `TvbRange` covering this field. */
static int FieldInfo_get_range(lua_State* L) {
    /* The `TvbRange` covering this field. */
    FieldInfo fi = checkFieldInfo(L,1);

    if (push_TvbRange (L, fi->ws_fi->ds_tvb, fi->ws_fi->start, fi->ws_fi->length)) {
        return 1;
    }

    return 0;
}

/* WSLUA_ATTRIBUTE FieldInfo_generated RO Whether this field was marked as generated (boolean). */
static int FieldInfo_get_generated(lua_State* L) {
    /* Whether this field was marked as generated. */
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushboolean(L,FI_GET_FLAG(fi->ws_fi, FI_GENERATED));
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_hidden RO Whether this field was marked as hidden (boolean).

   @since 1.99.8
 */
static int FieldInfo_get_hidden(lua_State* L) {
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushboolean(L,FI_GET_FLAG(fi->ws_fi, FI_HIDDEN));
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_is_url RO Whether this field was marked as being a URL (boolean).

   @since 1.99.8
 */
static int FieldInfo_get_is_url(lua_State* L) {
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushboolean(L,FI_GET_FLAG(fi->ws_fi, FI_URL));
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_little_endian RO Whether this field is little-endian encoded (boolean).

   @since 1.99.8
 */
static int FieldInfo_get_little_endian(lua_State* L) {
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushboolean(L,FI_GET_FLAG(fi->ws_fi, FI_LITTLE_ENDIAN));
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_big_endian RO Whether this field is big-endian encoded (boolean).

   @since 1.99.8
 */
static int FieldInfo_get_big_endian(lua_State* L) {
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushboolean(L,FI_GET_FLAG(fi->ws_fi, FI_BIG_ENDIAN));
    return 1;
}

/* WSLUA_ATTRIBUTE FieldInfo_name RO The filter name of this field.

   @since 1.99.8
 */
static int FieldInfo_get_name(lua_State* L) {
    /* The filter name of this field. */
    FieldInfo fi = checkFieldInfo(L,1);

    lua_pushstring(L,fi->ws_fi->hfinfo->abbrev);
    return 1;
}

WSLUA_METAMETHOD FieldInfo__eq(lua_State* L) {
    /* Checks whether lhs is within rhs. */
    FieldInfo l = checkFieldInfo(L,1);
    FieldInfo r = checkFieldInfo(L,2);

    /* it is not an error if their ds_tvb are different... they're just not equal */
    if (l->ws_fi->ds_tvb == r->ws_fi->ds_tvb &&
        l->ws_fi->start == r->ws_fi->start &&
        r->ws_fi->length == l->ws_fi->length) {
        lua_pushboolean(L,1);
    } else {
        lua_pushboolean(L,0);
    }
    return 1;
}

WSLUA_METAMETHOD FieldInfo__le(lua_State* L) {
    /* Checks whether the end byte of lhs is before the end of rhs. */
    FieldInfo l = checkFieldInfo(L,1);
    FieldInfo r = checkFieldInfo(L,2);

    if (l->ws_fi->ds_tvb != r->ws_fi->ds_tvb)
        WSLUA_ERROR(FieldInfo__le,"Data source must be the same for both fields");

    if (r->ws_fi->start + r->ws_fi->length <= l->ws_fi->start + l->ws_fi->length) {
        lua_pushboolean(L,1);
    } else {
        lua_pushboolean(L,0);
    }
    return 1;
}

WSLUA_METAMETHOD FieldInfo__lt(lua_State* L) {
    /* Checks whether the end byte of rhs is before the beginning of rhs. */
    FieldInfo l = checkFieldInfo(L,1);
    FieldInfo r = checkFieldInfo(L,2);

    if (l->ws_fi->ds_tvb != r->ws_fi->ds_tvb) {
        WSLUA_ERROR(FieldInfo__lt,"Data source must be the same for both fields");
        return 0;
    }

    if (r->ws_fi->start + r->ws_fi->length < l->ws_fi->start) {
        lua_pushboolean(L,1);
    } else {
        lua_pushboolean(L,0);
    }
    return 1;
}

/* Gets registered as metamethod automatically by WSLUA_REGISTER_META */
static int FieldInfo__gc(lua_State* L) {
    FieldInfo fi = toFieldInfo(L,1);

    if (!fi) return 0;

    if (!fi->expired)
        fi->expired = TRUE;
    else
        /* do NOT free fi->ws_fi */
        g_free(fi);

    return 0;
}

/* This table is ultimately registered as a sub-table of the class' metatable,
 * and if __index/__newindex is invoked then it calls the appropriate function
 * from this table for getting/setting the members.
 */
WSLUA_ATTRIBUTES FieldInfo_attributes[] = {
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,range),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,generated),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,hidden),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,is_url),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,little_endian),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,big_endian),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,name),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,display),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,type),
    WSLUA_ATTRIBUTE_ROREG(FieldInfo,source),
    { "label", FieldInfo__tostring, NULL },
    { "value", FieldInfo__call, NULL },
    { "tvb", FieldInfo_get_range, NULL },
    { "len", FieldInfo__len, NULL },
    { "offset", FieldInfo__unm, NULL },
    { NULL, NULL, NULL }
};

WSLUA_META FieldInfo_meta[] = {
    WSLUA_CLASS_MTREG(FieldInfo,tostring),
    WSLUA_CLASS_MTREG(FieldInfo,call),
    WSLUA_CLASS_MTREG(FieldInfo,len),
    WSLUA_CLASS_MTREG(FieldInfo,unm),
    WSLUA_CLASS_MTREG(FieldInfo,eq),
    WSLUA_CLASS_MTREG(FieldInfo,le),
    WSLUA_CLASS_MTREG(FieldInfo,lt),
    { NULL, NULL }
};

int FieldInfo_register(lua_State* L) {
    WSLUA_REGISTER_META(FieldInfo);
    WSLUA_REGISTER_ATTRIBUTES(FieldInfo);
    return 0;
}


WSLUA_FUNCTION wslua_all_field_infos(lua_State* L) {
    /*
    Obtain all fields from the current tree.  Note this only gets whatever fields the underlying
    dissectors have filled in for this packet at this time - there may be fields applicable to
    the packet that simply aren't being filled in because at this time they're not needed for anything.
    This function only gets what the C-side code has currently populated, not the full list.
    */
    GPtrArray* found;
    int items_found = 0;
    guint i;

    if (! lua_tree || ! lua_tree->tree ) {
        WSLUA_ERROR(wslua_all_field_infos,"Cannot be called outside a listener or dissector");
        return 0;
    }

    found = proto_all_finfos(lua_tree->tree);

    if (found) {
        for (i=0; i<found->len; i++) {
            push_FieldInfo(L, (field_info *)g_ptr_array_index(found,i));
            items_found++;
        }

        g_ptr_array_free(found,TRUE);
    }

    return items_found;
}

WSLUA_CLASS_DEFINE(Field,FAIL_ON_NULL("Field"));
/*
   A Field extractor to to obtain field values. A `Field` object can only be created *outside* of
   the callback functions of dissectors, post-dissectors, heuristic-dissectors, and taps.

   Once created, it is used *inside* the callback functions, to generate a `FieldInfo` object.
 */

static GPtrArray* wanted_fields = NULL;
static dfilter_t* wslua_dfilter = NULL;

/* We use a fake dfilter for Lua field extractors, so that
 * epan_dissect_run() will populate the fields.  This won't happen
 * if the passed-in edt->tree is NULL, which it will be if the
 * proto_tree isn't created by epan_dissect_init().  But that's by
 * design - if shark doesn't pass in a proto_tree, it's probably for
 * a good reason and we shouldn't override that. (right?)
 */
void wslua_prime_dfilter(epan_dissect_t *edt) {
    if (wslua_dfilter && edt && edt->tree) {
        dfilter_prime_proto_tree(wslua_dfilter, edt->tree);
    }
}

/* Check if we have any registered field extractors. */
gboolean wslua_has_field_extractors(void) {
    return (wslua_dfilter && dfilter_has_interesting_fields(wslua_dfilter));
}

/*
 * field extractor registration is tricky, In order to allow
 * the user to define them in the body of the script we will
 * populate the Field value with a pointer of the abbrev of it
 * to later replace it with the hfi.
 *
 * This will be added to the wanted_fields array that will
 * exists only while they can be defined, and be cleared right
 * after the fields are primed.
 */

static gboolean fake_tap = FALSE;
void lua_prime_all_fields(proto_tree* tree _U_) {
    GString* fake_tap_filter = g_string_new("frame");
    guint i;
    gchar *err_msg;

    for(i=0; i < wanted_fields->len; i++) {
        Field f = (Field)g_ptr_array_index(wanted_fields,i);
        gchar* name = *((gchar**)f);

        *f = proto_registrar_get_byname(name);

        if (!*f) {
            report_failure("Could not find field `%s'",name);
            *f = NULL;
            g_free(name);
            continue;
        }

        g_free(name);

        g_string_append_printf(fake_tap_filter," || %s",(*f)->abbrev);
        fake_tap = TRUE;
    }

    g_ptr_array_free(wanted_fields,TRUE);
    wanted_fields = NULL;

    if (fake_tap && fake_tap_filter->len > strlen("frame")) {
        /* a boring tap :-) */
        GString* error = register_tap_listener("frame",
                &fake_tap,
                fake_tap_filter->str,
                0, /* XXX - do we need the protocol tree or columns? */
                NULL, NULL, NULL);

        if (error) {
            report_failure("while registering lua_fake_tap:\n%s",error->str);
            g_string_free(error,TRUE);
        } else if (!dfilter_compile(fake_tap_filter->str, &wslua_dfilter, &err_msg)) {
            report_failure("while compiling dfilter \"%s\" for wslua: %s", fake_tap_filter->str, err_msg);
            g_free(err_msg);
        }
    }
    g_string_free(fake_tap_filter, TRUE);
}

WSLUA_CONSTRUCTOR Field_new(lua_State *L) {
    /*
       Create a Field extractor.
       */
#define WSLUA_ARG_Field_new_FIELDNAME 1 /* The filter name of the field (e.g. ip.addr) */
    const gchar* name = luaL_checkstring(L,WSLUA_ARG_Field_new_FIELDNAME);
    Field f;

    if (!proto_registrar_get_byname(name) && !wslua_is_field_available(L, name)) {
        WSLUA_ARG_ERROR(Field_new,FIELDNAME,"a field with this name must exist");
        return 0;
    }

    if (!wanted_fields) {
        WSLUA_ERROR(Field_new,"A Field extractor must be defined before Taps or Dissectors get called");
        return 0;
    }

    f = (Field)g_malloc(sizeof(void*));
    *f = (header_field_info*)(void*)g_strdup(name); /* cheating */

    g_ptr_array_add(wanted_fields,f);

    pushField(L,f);
    WSLUA_RETURN(1); /* The field extractor */
}

WSLUA_CONSTRUCTOR Field_list(lua_State *L) {
    /* Gets a Lua array table of all registered field filter names.

       NOTE: this is an expensive operation, and should only be used for troubleshooting.

       @since 1.11.3
     */
    void *cookie, *cookie2;
    int i = -1;
    int count = 0;
    header_field_info *hfinfo = NULL;

    lua_newtable(L);

    for (i = proto_get_first_protocol(&cookie); i != -1;
         i = proto_get_next_protocol(&cookie)) {

        for (hfinfo = proto_get_first_protocol_field(i, &cookie2); hfinfo != NULL;
         hfinfo = proto_get_next_protocol_field(i, &cookie2)) {

            if (hfinfo->same_name_prev_id != -1) /* ignore duplicate names */
                continue;

            count++;
            lua_pushstring(L,hfinfo->abbrev);
            lua_rawseti(L,1,count);
        }
    }

    WSLUA_RETURN(1); /* The array table of field filter names */
}

/* the following is used in Field_get_xxx functions later */
#define GET_HFINFO_MEMBER(luafunc, member)                          \
    if (wanted_fields) {                                            \
        /* before registration, so it's a gchar** of the abbrev */  \
        const gchar* name = (const gchar*) *fi;                     \
        if (name) {                                                 \
            hfinfo = proto_registrar_get_byname(name);              \
            if (!hfinfo) {                                          \
                /* could be a Lua-created field */                  \
                ProtoField pf = wslua_is_field_available(L, name);  \
                if (pf) {                                           \
                    luafunc(L, pf->member);                         \
                    return 1;                                       \
                }                                                   \
            }                                                       \
        } else {                                                    \
            luaL_error(L, "Field." #member ": unknown field");      \
            return 0;                                               \
        }                                                           \
    } else {                                                        \
        hfinfo = *fi;                                               \
    }                                                               \
                                                                    \
    if (hfinfo) {                                                   \
        luafunc(L,hfinfo->member);                                  \
    } else                                                          \
        lua_pushnil(L)


/* WSLUA_ATTRIBUTE Field_name RO The filter name of this field, or nil.

   @since 1.99.8
 */
static int Field_get_name(lua_State* L) {
    Field fi = checkField(L,1);
    header_field_info* hfinfo = NULL;

    GET_HFINFO_MEMBER(lua_pushstring, abbrev);

    return 1;
}

/* WSLUA_ATTRIBUTE Field_display RO The full display name of this field, or nil.

   @since 1.99.8
 */
static int Field_get_display(lua_State* L) {
    Field fi = checkField(L,1);
    header_field_info* hfinfo = NULL;

    GET_HFINFO_MEMBER(lua_pushstring, name);

    return 1;
}

/* WSLUA_ATTRIBUTE Field_type RO The `ftype` of this field, or nil.

   @since 1.99.8
 */
static int Field_get_type(lua_State* L) {
    Field fi = checkField(L,1);
    header_field_info* hfinfo = NULL;

    GET_HFINFO_MEMBER(lua_pushnumber, type);

    return 1;
}

WSLUA_METAMETHOD Field__call (lua_State* L) {
    /* Obtain all values (see `FieldInfo`) for this field. */
    Field f = checkField(L,1);
    header_field_info* in = *f;
    int items_found = 0;

    if (! in) {
        luaL_error(L,"invalid field");
        return 0;
    }

    if (! lua_pinfo ) {
        WSLUA_ERROR(Field__call,"Fields cannot be used outside dissectors or taps");
        return 0;
    }

    while (in) {
        GPtrArray* found = proto_get_finfo_ptr_array(lua_tree->tree, in->id);
        guint i;
        if (found) {
            for (i=0; i<found->len; i++) {
                push_FieldInfo(L, (field_info *) g_ptr_array_index(found,i));
                items_found++;
            }
        }
        in = (in->same_name_prev_id != -1) ? proto_registrar_get_nth(in->same_name_prev_id) : NULL;
    }

    WSLUA_RETURN(items_found); /* All the values of this field */
}

WSLUA_METAMETHOD Field__tostring(lua_State* L) {
    /* Obtain a string with the field filter name. */
    Field f = checkField(L,1);

    if (wanted_fields) {
        lua_pushstring(L,*((gchar**)f));
    } else {
        lua_pushstring(L,(*f)->abbrev);
    }

    return 1;
}

static int Field__gc(lua_State* L _U_) {
    /* do NOT free Field */
    return 0;
}

WSLUA_ATTRIBUTES Field_attributes[] = {
    WSLUA_ATTRIBUTE_ROREG(Field,name),
    WSLUA_ATTRIBUTE_ROREG(Field,display),
    WSLUA_ATTRIBUTE_ROREG(Field,type),
    { NULL, NULL, NULL }
};

WSLUA_METHODS Field_methods[] = {
    WSLUA_CLASS_FNREG(Field,new),
    WSLUA_CLASS_FNREG(Field,list),
    { NULL, NULL }
};

WSLUA_META Field_meta[] = {
    WSLUA_CLASS_MTREG(Field,tostring),
    WSLUA_CLASS_MTREG(Field,call),
    { NULL, NULL }
};

int Field_register(lua_State* L) {

    wanted_fields = g_ptr_array_new();

    WSLUA_REGISTER_CLASS(Field);
    WSLUA_REGISTER_ATTRIBUTES(Field);
    outstanding_FieldInfo = g_ptr_array_new();

    return 0;
}

int wslua_deregister_fields(lua_State* L _U_) {
    if (wslua_dfilter) {
        dfilter_free(wslua_dfilter);
        wslua_dfilter = NULL;
    }

    if (fake_tap) {
        remove_tap_listener(&fake_tap);
        fake_tap = FALSE;
    }

    return 0;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
