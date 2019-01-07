/* -*- Mode: C; indent-tabs-mode:nil; c-basic-offset: 8-*- */

/*
 * This file is part of The Croco Library
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 3 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 * 
 * Author: Dodji Seketeli
 * See COPYRIGHTS file for copyrights information.
 */

#include <config.h>
#include <stdio.h>
#include <string.h>
#include "cr-rgb.h"
#include "cr-term.h"
#include "cr-parser.h"

static CRRgb gv_standard_colors[] = {
        {"aliceblue", 240, 248, 255, 0,},
        {"antiquewhite", 250, 235, 215, 0},
        {"aqua", 0, 255, 255, 0},
        {"aquamarine", 127, 255, 212, 0},
        {"azure", 240, 255, 255, 0},
        {"beige", 245, 245, 220, 0},
        {"bisque", 255, 228, 196, 0},
        {"black", 0, 0, 0, 0},
        {"blanchedalmond", 255, 235, 205, 0},
        {"blue", 0, 0, 255, 0},
        {"blueviolet", 138, 43, 226, 0},
        {"brown", 165, 42, 42, 0},
        {"burlywood", 222, 184, 135, 0},
        {"cadetblue", 95, 158, 160, 0},
        {"chartreuse", 127, 255, 0, 0},
        {"chocolate", 210, 105, 30, 0},
        {"coral", 255, 127, 80, 0},
        {"cornflowerblue", 100, 149, 237, 0},
        {"cornsilk", 255, 248, 220, 0},
        {"crimson", 220, 20, 60, 0},
        {"cyan", 0, 255, 255, 0},
        {"darkblue", 0, 0, 139, 0},
        {"darkcyan", 0, 139, 139, 0},
        {"darkgoldenrod", 184, 134, 11, 0},
        {"darkgray", 169, 169, 169, 0},
        {"darkgreen", 0, 100, 0, 0},
        {"darkgrey", 169, 169, 169, 0},
        {"darkkhaki", 189, 183, 107, 0},
        {"darkmagenta", 139, 0, 139, 0},
        {"darkolivegreen", 85, 107, 47, 0},
        {"darkorange", 255, 140, 0, 0},
        {"darkorchid", 153, 50, 204, 0},
        {"darkred", 139, 0, 0, 0},
        {"darksalmon", 233, 150, 122, 0},
        {"darkseagreen", 143, 188, 143, 0},
        {"darkslateblue", 72, 61, 139, 0},
        {"darkslategray", 47, 79, 79, 0},
        {"darkslategrey", 47, 79, 79, 0},
        {"darkturquoise", 0, 206, 209, 0},
        {"darkviolet", 148, 0, 211, 0},
        {"deeppink", 255, 20, 147, 0},
        {"deepskyblue", 0, 191, 255, 0},
        {"dimgray", 105, 105, 105, 0},
        {"dimgrey", 105, 105, 105, 0},
        {"dodgerblue", 30, 144, 255, 0},
        {"firebrick", 178, 34, 34, 0},
        {"floralwhite", 255, 250, 240, 0},
        {"forestgreen", 34, 139, 34, 0},
        {"fuchsia", 255, 0, 255, 0},
        {"gainsboro", 220, 220, 220, 0},
        {"ghostwhite", 248, 248, 255, 0},
        {"gold", 255, 215, 0, 0},
        {"goldenrod", 218, 165, 32, 0},
        {"gray", 128, 128, 128, 0},
        {"grey", 128, 128, 128, 0},
        {"green", 0, 128, 0, 0},
        {"greenyellow", 173, 255, 47, 0},
        {"honeydew", 240, 255, 240, 0},
        {"hotpink", 255, 105, 180, 0},
        {"indianred", 205, 92, 92, 0},
        {"indigo", 75, 0, 130, 0},
        {"ivory", 255, 255, 240, 0},
        {"khaki", 240, 230, 140, 0},
        {"lavender", 230, 230, 250, 0},
        {"lavenderblush", 255, 240, 245, 0},
        {"lawngreen", 124, 252, 0, 0},
        {"lemonchiffon", 255, 250, 205, 0},
        {"lightblue", 173, 216, 230, 0},
        {"lightcoral", 240, 128, 128, 0},
        {"lightcyan", 224, 255, 255, 0},
        {"lightgoldenrodyellow", 250, 250, 210, 0},
        {"lightgray", 211, 211, 211, 0},
        {"lightgreen", 144, 238, 144, 0},
        {"lightgrey", 211, 211, 211, 0},
        {"lightpink", 255, 182, 193, 0},
        {"lightsalmon", 255, 160, 122, 0},
        {"lightseagreen", 32, 178, 170, 0},
        {"lightskyblue", 135, 206, 250, 0},
        {"lightslategray", 119, 136, 153, 0},
        {"lightslategrey", 119, 136, 153, 0},
        {"lightsteelblue", 176, 196, 222, 0},
        {"lightyellow", 255, 255, 224, 0},
        {"lime", 0, 255, 0, 0},
        {"limegreen", 50, 205, 50, 0},
        {"linen", 250, 240, 230, 0},
        {"magenta", 255, 0, 255, 0},
        {"maroon", 128, 0, 0, 0},
        {"mediumaquamarine", 102, 205, 170, 0},
        {"mediumblue", 0, 0, 205, 0},
        {"mediumorchid", 186, 85, 211, 0},
        {"mediumpurple", 147, 112, 219, 0},
        {"mediumseagreen", 60, 179, 113, 0},
        {"mediumslateblue", 123, 104, 238, 0},
        {"mediumspringgreen", 0, 250, 154, 0},
        {"mediumturquoise", 72, 209, 204, 0},
        {"mediumvioletred", 199, 21, 133, 0},
        {"midnightblue", 25, 25, 112, 0},
        {"mintcream", 245, 255, 250, 0},
        {"mistyrose", 255, 228, 225, 0},
        {"moccasin", 255, 228, 181, 0},
        {"navajowhite", 255, 222, 173, 0},
        {"navy", 0, 0, 128, 0},
        {"oldlace", 253, 245, 230, 0},
        {"olive", 128, 128, 0, 0},
        {"olivedrab", 107, 142, 35, 0},
        {"orange", 255, 165, 0, 0},
        {"orangered", 255, 69, 0, 0},
        {"orchid", 218, 112, 214, 0},
        {"palegoldenrod", 238, 232, 170, 0},
        {"palegreen", 152, 251, 152, 0},
        {"paleturquoise", 175, 238, 238, 0},
        {"palevioletred", 219, 112, 147, 0},
        {"papayawhip", 255, 239, 213, 0},
        {"peachpuff", 255, 218, 185, 0},
        {"peru", 205, 133, 63, 0},
        {"pink", 255, 192, 203, 0},
        {"plum", 221, 160, 221, 0},
        {"powderblue", 176, 224, 230, 0},
        {"purple", 128, 0, 128, 0},
        {"red", 255, 0, 0, 0},
        {"rosybrown", 188, 143, 143, 0},
        {"royalblue", 65, 105, 225, 0},
        {"saddlebrown", 139, 69, 19, 0},
        {"salmon", 250, 128, 114, 0},
        {"sandybrown", 244, 164, 96, 0},
        {"seagreen", 46, 139, 87, 0},
        {"seashell", 255, 245, 238, 0},
        {"sienna", 160, 82, 45, 0},
        {"silver", 192, 192, 192, 0},
        {"skyblue", 135, 206, 235, 0},
        {"slateblue", 106, 90, 205, 0},
        {"slategray", 112, 128, 144, 0},
        {"slategrey", 112, 128, 144, 0},
        {"snow", 255, 250, 250, 0},
        {"springgreen", 0, 255, 127, 0},
        {"steelblue", 70, 130, 180, 0},
        {"tan", 210, 180, 140, 0},
        {"teal", 0, 128, 128, 0},
        {"thistle", 216, 191, 216, 0},
        {"tomato", 255, 99, 71, 0},
        {"turquoise", 64, 224, 208, 0},
        {"violet", 238, 130, 238, 0},
        {"wheat", 245, 222, 179, 0},
        {"white", 255, 255, 255, 0},
        {"whitesmoke", 245, 245, 245, 0,},
        {"yellow", 255, 255, 0, 0,},
        {"yellowgreen", 154, 205, 50, 0,},
        {"transparent", 255, 255, 255, 0, 0, 1}
};

/**
 * cr_rgb_new:
 *
 *The default constructor of #CRRgb.
 *
 *Returns the newly built instance of #CRRgb
 */
CRRgb *
cr_rgb_new (void)
{
        CRRgb *result = NULL;

        result = g_try_malloc (sizeof (CRRgb));

        if (result == NULL) {
                cr_utils_trace_info ("No more memory");
                return NULL;
        }

        memset (result, 0, sizeof (CRRgb));

        return result;
}

/**
 * cr_rgb_new_with_vals:
 *@a_red: the red component of the color.
 *@a_green: the green component of the color.
 *@a_blue: the blue component of the color.
 *@a_unit: the unit of the rgb values.
 *(either percentage or integer values)
 *
 *A constructor of #CRRgb.
 *
 *Returns the newly built instance of #CRRgb.
 */
CRRgb *
cr_rgb_new_with_vals (gulong a_red, gulong a_green,
                      gulong a_blue, gboolean a_is_percentage)
{
        CRRgb *result = NULL;

        result = cr_rgb_new ();

        g_return_val_if_fail (result, NULL);

        result->red = a_red;
        result->green = a_green;
        result->blue = a_blue;
        result->is_percentage = a_is_percentage;

        return result;
}

/**
 * cr_rgb_to_string:
 *@a_this: the instance of #CRRgb to serialize.
 *
 *Serializes the rgb into a zero terminated string.
 *
 *Returns the zero terminated string containing the serialized
 *rgb. MUST BE FREED by the caller using g_free().
 */
guchar *
cr_rgb_to_string (CRRgb * a_this)
{
        guchar *result = NULL;
        GString *str_buf = NULL;

        str_buf = g_string_new (NULL);
        g_return_val_if_fail (str_buf, NULL);

        if (a_this->is_percentage == 1) {
                g_string_append_printf (str_buf, "%ld", a_this->red);

                g_string_append (str_buf, "%, ");

                g_string_append_printf (str_buf, "%ld", a_this->green);
                g_string_append (str_buf, "%, ");

                g_string_append_printf (str_buf, "%ld", a_this->blue);
                g_string_append_c (str_buf, '%');
        } else {
                g_string_append_printf (str_buf, "%ld", a_this->red);
                g_string_append (str_buf, ", ");

                g_string_append_printf (str_buf, "%ld", a_this->green);
                g_string_append (str_buf, ", ");

                g_string_append_printf (str_buf, "%ld", a_this->blue);
        }

        if (str_buf) {
                result = str_buf->str;
                g_string_free (str_buf, FALSE);
        }

        return result;
}

/**
 * cr_rgb_dump:
 *@a_this: the "this pointer" of
 *the current instance of #CRRgb.
 *@a_fp: the destination file pointer.
 *
 *Dumps the current instance of #CRRgb
 *to a file.
 */
void
cr_rgb_dump (CRRgb * a_this, FILE * a_fp)
{
        guchar *str = NULL;

        g_return_if_fail (a_this);

        str = cr_rgb_to_string (a_this);

        if (str) {
                fprintf (a_fp, "%s", str);
                g_free (str);
                str = NULL;
        }
}

/**
 * cr_rgb_compute_from_percentage:
 *@a_this: the current instance of #CRRgb
 *
 *If the rgb values are expressed in percentage,
 *compute their real value.
 *
 *Returns CR_OK upon successful completion, an error code otherwise.
 */
enum CRStatus
cr_rgb_compute_from_percentage (CRRgb * a_this)
{
        g_return_val_if_fail (a_this, CR_BAD_PARAM_ERROR);

        if (a_this->is_percentage == FALSE)
                return CR_OK;
        a_this->red = a_this->red * 255 / 100;
        a_this->green = a_this->green * 255 / 100;
        a_this->blue = a_this->blue * 255 / 100;
        a_this->is_percentage = FALSE;
        return CR_OK;
}

/**
 * cr_rgb_set:
 *@a_this: the current instance of #CRRgb.
 *@a_red: the red value.
 *@a_green: the green value.
 *@a_blue: the blue value.
 *
 *Sets rgb values to the RGB.
 *
 *Returns CR_OK upon successful completion, an error code
 *otherwise.
 */
enum CRStatus
cr_rgb_set (CRRgb * a_this, gulong a_red,
            gulong a_green, gulong a_blue, gboolean a_is_percentage)
{
        g_return_val_if_fail (a_this, CR_BAD_PARAM_ERROR);
        if (a_is_percentage != FALSE) {
                g_return_val_if_fail (a_red <= 100
                                      && a_green <= 100
                                      && a_blue <= 100, CR_BAD_PARAM_ERROR);
        }

        a_this->is_percentage = a_is_percentage;

        a_this->red = a_red;
        a_this->green = a_green;
        a_this->blue = a_blue;
        a_this->inherit = FALSE ;
        a_this->is_transparent = FALSE ;
        return CR_OK;
}

/**
 * cr_rgb_set_to_inherit:
 *@a_this: the current instance of #CRRgb
 *
 *sets the value of the rgb to inherit.
 *Look at the css spec from chapter 6.1 to 6.2 to understand
 *the meaning of "inherit".
 *
 * Returns CR_OK upon succesful completion, an error code otherwise.
 */
enum CRStatus 
cr_rgb_set_to_inherit (CRRgb *a_this, gboolean a_inherit)
{
        g_return_val_if_fail (a_this, CR_BAD_PARAM_ERROR) ;

        a_this->inherit = a_inherit ;

        return CR_OK ;
}

/**
 * cr_rgb_is_set_to_inherit:
 *
 * @a_this: the current instance of #CRRgb.
 *
 * Returns TRUE if the rgb is set to the value "inherit", FALSE otherwise.
 */
gboolean
cr_rgb_is_set_to_inherit (CRRgb *a_this)
{
        g_return_val_if_fail (a_this, CR_BAD_PARAM_ERROR) ;

        return a_this->inherit ;
}

/**
 * cr_rgb_is_set_to_transparent:
 *@a_this: the current instance of
 *#CRRgb
 *
 *Tests if the the rgb is set to the
 *value "transparent" or not.
 *
 *Returns TRUE if the rgb has been set to
 *transparent, FALSE otherwise.
 */
gboolean 
cr_rgb_is_set_to_transparent (CRRgb *a_this)
{
        g_return_val_if_fail (a_this, FALSE) ;
        return a_this->is_transparent ;
}


/**
 * cr_rgb_set_to_transparent:
 *@a_this: the current instance of #CRRgb
 *@a_is_transparent: set to transparent or not.
 *
 *Sets the rgb to the "transparent" value (or not)
 *Returns CR_OK upon successfull completion, an error code otherwise.
 */
enum CRStatus 
cr_rgb_set_to_transparent (CRRgb *a_this, 
                           gboolean a_is_transparent)
{
        g_return_val_if_fail (a_this, CR_BAD_PARAM_ERROR) ;        
        a_this->is_transparent = a_is_transparent ;
        return CR_OK ;
}

/**
 * cr_rgb_set_from_rgb:
 *@a_this: the current instance of #CRRgb.
 *@a_rgb: the rgb to "copy"
 *
 *Sets the rgb from an other one.
 *
 *Returns CR_OK upon successful completion, an error code otherwise.
 */
enum CRStatus
cr_rgb_set_from_rgb (CRRgb * a_this, CRRgb * a_rgb)
{
        g_return_val_if_fail (a_this && a_rgb, CR_BAD_PARAM_ERROR);

        cr_rgb_copy (a_this, a_rgb) ;

        return CR_OK;
}

/**
 * cr_rgb_set_from_name:
 * @a_this: the current instance of #CRRgb
 * @a_color_name: the color name
 * 
 * Returns CR_OK upon successful completion, an error code otherwise.
 */
enum CRStatus
cr_rgb_set_from_name (CRRgb * a_this, const guchar * a_color_name)
{
        gulong i = 0;
        enum CRStatus status = CR_OK;

        g_return_val_if_fail (a_this && a_color_name, CR_BAD_PARAM_ERROR);

        for (i = 0; i < sizeof (gv_standard_colors); i++) {
                if (!strcmp (a_color_name, gv_standard_colors[i].name)) {
                        cr_rgb_set_from_rgb (a_this, &gv_standard_colors[i]);
                        break;
                }
        }

        if (i < sizeof (gv_standard_colors))
                status = CR_OK;
        else
               status = CR_UNKNOWN_TYPE_ERROR;

        return status;
}

/**
 * cr_rgb_set_from_hex_str:
 * @a_this: the current instance of #CRRgb
 * @a_hex: the hexadecimal value to set.
 *
 * Returns CR_OK upon successful completion.
 */
enum CRStatus
cr_rgb_set_from_hex_str (CRRgb * a_this, const guchar * a_hex)
{
        enum CRStatus status = CR_OK;
        gulong i = 0;
        guchar colors[3] = { 0 };

        g_return_val_if_fail (a_this && a_hex, CR_BAD_PARAM_ERROR);

        if (strlen (a_hex) == 3) {
                for (i = 0; i < 3; i++) {
                        if (a_hex[i] >= '0' && a_hex[i] <= '9') {
                                colors[i] = a_hex[i] - '0';
                                colors[i] = (colors[i] << 4) | colors[i];
                        } else if (a_hex[i] >= 'a' && a_hex[i] <= 'z') {
                                colors[i] = 10 + a_hex[i] - 'a';
                                colors[i] = (colors[i] << 4) | colors[i];
                        } else if (a_hex[i] >= 'A' && a_hex[i] <= 'Z') {
                                colors[i] = 10 + a_hex[i] - 'A';
                                colors[i] = (colors[i] << 4) | colors[i];
                        } else {
                                status = CR_UNKNOWN_TYPE_ERROR;
                        }
                }
        } else if (strlen (a_hex) == 6) {
                for (i = 0; i < 6; i++) {
                        if (a_hex[i] >= '0' && a_hex[i] <= '9') {
                                colors[i / 2] <<= 4;
                                colors[i / 2] |= a_hex[i] - '0';
                                status = CR_OK;
                        } else if (a_hex[i] >= 'a' && a_hex[i] <= 'z') {
                                colors[i / 2] <<= 4;
                                colors[i / 2] |= 10 + a_hex[i] - 'a';
                                status = CR_OK;
                        } else if (a_hex[i] >= 'A' && a_hex[i] <= 'Z') {
                                colors[i / 2] <<= 4;
                                colors[i / 2] |= 10 + a_hex[i] - 'A';
                                status = CR_OK;
                        } else {
                                status = CR_UNKNOWN_TYPE_ERROR;
                        }
                }
        } else {
                status = CR_UNKNOWN_TYPE_ERROR;
        }

        if (status == CR_OK) {
                status = cr_rgb_set (a_this, colors[0],
                                     colors[1], colors[2], FALSE);
                cr_rgb_set_to_transparent (a_this, FALSE) ;
        }
        return status;
}

/**
 * cr_rgb_set_from_term:
 *@a_this: the instance of #CRRgb to set
 *@a_value: the terminal from which to set
 *
 *Set the rgb from a terminal symbol
 *
 * Returns CR_OK upon successful completion, an error code otherwise.
 */
enum CRStatus
cr_rgb_set_from_term (CRRgb *a_this, const struct _CRTerm *a_value)
{
        enum CRStatus status = CR_OK ;
        g_return_val_if_fail (a_this && a_value,
                              CR_BAD_PARAM_ERROR) ;

	switch(a_value->type) {
	case TERM_RGB:
                if (a_value->content.rgb) {
                        cr_rgb_set_from_rgb
                                (a_this, a_value->content.rgb) ;
                }
		break ;
	case TERM_IDENT:
                if (a_value->content.str
                    && a_value->content.str->stryng
                    && a_value->content.str->stryng->str) {
			if (!strncmp ("inherit",
                                      a_value->content.str->stryng->str,
                                      sizeof ("inherit")-1)) {
				a_this->inherit = TRUE;
                                a_this->is_transparent = FALSE ;
			} else  {
                        	status = cr_rgb_set_from_name
                                        (a_this,
                                         a_value->content.str->stryng->str) ;
			}
                } else {
                        cr_utils_trace_info 
                                ("a_value has NULL string value") ;
                }
		break ;
	case TERM_HASH:
                if (a_value->content.str
                    && a_value->content.str->stryng
                    && a_value->content.str->stryng->str) {
                        status = cr_rgb_set_from_hex_str
                                (a_this, 
                                 a_value->content.str->stryng->str) ;
                } else {
                        cr_utils_trace_info
                                ("a_value has NULL string value") ;
                }
                break ;
	default:
                status =  CR_UNKNOWN_TYPE_ERROR ;
	}
        return status ;
}

enum CRStatus 
cr_rgb_copy (CRRgb *a_dest, CRRgb*a_src)
{
        g_return_val_if_fail (a_dest && a_src,
                              CR_BAD_PARAM_ERROR) ;

        memcpy (a_dest, a_src, sizeof (CRRgb)) ;
        return CR_OK ;
}

/**
 * cr_rgb_destroy:
 *@a_this: the "this pointer" of the
 *current instance of #CRRgb.
 *
 *Destructor of #CRRgb.
 */
void
cr_rgb_destroy (CRRgb * a_this)
{
        g_return_if_fail (a_this);
        g_free (a_this);
}

/**
 * cr_rgb_parse_from_buf:
 *@a_str: a string that contains a color description
 *@a_enc: the encoding of a_str
 *
 *Parses a text buffer that contains a rgb color
 *
 *Returns the parsed color, or NULL in case of error
 */
CRRgb *
cr_rgb_parse_from_buf (const guchar *a_str,
                              enum CREncoding a_enc)
{
	enum CRStatus status = CR_OK ;
	CRTerm *value = NULL ;
	CRParser * parser = NULL;
	CRRgb *result = NULL;
	
	g_return_val_if_fail (a_str, NULL);
	
	parser = cr_parser_new_from_buf ((guchar*)a_str, strlen (a_str), 
                                         a_enc, FALSE) ;

	g_return_val_if_fail (parser, NULL);

	status = cr_parser_try_to_skip_spaces_and_comments (parser) ;
	if (status != CR_OK)
	    	goto cleanup;

	status = cr_parser_parse_term (parser, &value);
	if (status != CR_OK)
	    	goto cleanup;

	result = cr_rgb_new ();
	if (!result)
	    	goto cleanup;

	status = cr_rgb_set_from_term (result, value);

cleanup:
	if (parser) {
	    	cr_parser_destroy (parser);
		parser = NULL;
	}
	if (value) {
	    	cr_term_destroy(value);
		value = NULL;
	}
	return result ;
}

