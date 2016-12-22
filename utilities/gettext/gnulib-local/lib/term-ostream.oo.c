/* Output stream for attributed text, producing ANSI escape sequences.
   Copyright (C) 2006-2008, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2006.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include "term-ostream.h"

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "fatal-signal.h"
#include "full-write.h"
#include "terminfo.h"
#include "xalloc.h"
#include "xsize.h"
#include "gettext.h"

#define _(str) gettext (str)

#if HAVE_TPARAM
/* GNU termcap's tparam() function requires a buffer argument.  Make it so
   large that there is no risk that tparam() needs to call malloc().  */
static char tparambuf[100];
/* Define tparm in terms of tparam.  In the scope of this file, it is called
   with at most one argument after the string.  */
# define tparm(str, arg1) \
  tparam (str, tparambuf, sizeof (tparambuf), arg1)
#endif

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* =========================== Color primitives =========================== */

/* A color in RGB format.  */
typedef struct
{
  unsigned int red   : 8; /* range 0..255 */
  unsigned int green : 8; /* range 0..255 */
  unsigned int blue  : 8; /* range 0..255 */
} rgb_t;

/* A color in HSV (a.k.a. HSB) format.  */
typedef struct
{
  float hue;        /* normalized to interval [0,6) */
  float saturation; /* normalized to interval [0,1] */
  float brightness; /* a.k.a. value, normalized to interval [0,1] */
} hsv_t;

/* Conversion of a color in RGB to HSV format.  */
static void
rgb_to_hsv (rgb_t c, hsv_t *result)
{
  unsigned int r = c.red;
  unsigned int g = c.green;
  unsigned int b = c.blue;

  if (r > g)
    {
      if (b > r)
        {
          /* b > r > g, so max = b, min = g */
          result->hue = 4.0f + (float) (r - g) / (float) (b - g);
          result->saturation = 1.0f - (float) g / (float) b;
          result->brightness = (float) b / 255.0f;
        }
      else if (b <= g)
        {
          /* r > g >= b, so max = r, min = b */
          result->hue = 0.0f + (float) (g - b) / (float) (r - b);
          result->saturation = 1.0f - (float) b / (float) r;
          result->brightness = (float) r / 255.0f;
        }
      else
        {
          /* r >= b > g, so max = r, min = g */
          result->hue = 6.0f - (float) (b - g) / (float) (r - g);
          result->saturation = 1.0f - (float) g / (float) r;
          result->brightness = (float) r / 255.0f;
        }
    }
  else
    {
      if (b > g)
        {
          /* b > g >= r, so max = b, min = r */
          result->hue = 4.0f - (float) (g - r) / (float) (b - r);
          result->saturation = 1.0f - (float) r / (float) b;
          result->brightness = (float) b / 255.0f;
        }
      else if (b < r)
        {
          /* g >= r > b, so max = g, min = b */
          result->hue = 2.0f - (float) (r - b) / (float) (g - b);
          result->saturation = 1.0f - (float) b / (float) g;
          result->brightness = (float) g / 255.0f;
        }
      else if (g > r)
        {
          /* g >= b >= r, g > r, so max = g, min = r */
          result->hue = 2.0f + (float) (b - r) / (float) (g - r);
          result->saturation = 1.0f - (float) r / (float) g;
          result->brightness = (float) g / 255.0f;
        }
      else
        {
          /* r = g = b.  A grey color.  */
          result->hue = 0; /* arbitrary */
          result->saturation = 0;
          result->brightness = (float) r / 255.0f;
        }
    }
}

/* Square of distance of two colors.  */
static float
color_distance (const hsv_t *color1, const hsv_t *color2)
{
#if 0
  /* Formula taken from "John Smith: Color Similarity",
       http://www.ctr.columbia.edu/~jrsmith/html/pubs/acmmm96/node8.html.  */
  float angle1 = color1->hue * 1.04719755f; /* normalize to [0,2π] */
  float angle2 = color2->hue * 1.04719755f; /* normalize to [0,2π] */
  float delta_x = color1->saturation * cosf (angle1)
                  - color2->saturation * cosf (angle2);
  float delta_y = color1->saturation * sinf (angle1)
                  - color2->saturation * sinf (angle2);
  float delta_v = color1->brightness
                  - color2->brightness;

  return delta_x * delta_x + delta_y * delta_y + delta_v * delta_v;
#else
  /* Formula that considers hue differences with more weight than saturation
     or brightness differences, like the human eye does.  */
  float delta_hue =
    (color1->hue >= color2->hue
     ? (color1->hue - color2->hue >= 3.0f
        ? 6.0f + color2->hue - color1->hue
        : color1->hue - color2->hue)
     : (color2->hue - color1->hue >= 3.0f
        ? 6.0f + color1->hue - color2->hue
        : color2->hue - color1->hue));
  float min_saturation =
    (color1->saturation < color2->saturation
     ? color1->saturation
     : color2->saturation);
  float delta_saturation = color1->saturation - color2->saturation;
  float delta_brightness = color1->brightness - color2->brightness;

  return delta_hue * delta_hue * min_saturation
         + delta_saturation * delta_saturation * 0.2f
         + delta_brightness * delta_brightness * 0.8f;
#endif
}

/* Return the index of the color in a color table that is nearest to a given
   color.  */
static unsigned int
nearest_color (rgb_t given, const rgb_t *table, unsigned int table_size)
{
  hsv_t given_hsv;
  unsigned int best_index;
  float best_distance;
  unsigned int i;

  assert (table_size > 0);

  rgb_to_hsv (given, &given_hsv);

  best_index = 0;
  best_distance = 1000000.0f;
  for (i = 0; i < table_size; i++)
    {
      hsv_t i_hsv;

      rgb_to_hsv (table[i], &i_hsv);

      /* Avoid converting a color to grey, or fading out a color too much.  */
      if (i_hsv.saturation > given_hsv.saturation * 0.5f)
        {
          float distance = color_distance (&given_hsv, &i_hsv);
          if (distance < best_distance)
            {
              best_index = i;
              best_distance = distance;
            }
        }
    }

#if 0 /* Debugging code */
  hsv_t best_hsv;
  rgb_to_hsv (table[best_index], &best_hsv);
  fprintf (stderr, "nearest: (%d,%d,%d) = (%f,%f,%f)\n    -> (%f,%f,%f) = (%d,%d,%d)\n",
                   given.red, given.green, given.blue,
                   (double)given_hsv.hue, (double)given_hsv.saturation, (double)given_hsv.brightness,
                   (double)best_hsv.hue, (double)best_hsv.saturation, (double)best_hsv.brightness,
                   table[best_index].red, table[best_index].green, table[best_index].blue);
#endif

  return best_index;
}

/* The luminance of a color.  This is the brightness of the color, as it
   appears to the human eye.  This must be used in color to grey conversion.  */
static float
color_luminance (int r, int g, int b)
{
  /* Use the luminance model used by NTSC and JPEG.
     Taken from http://www.fho-emden.de/~hoffmann/gray10012001.pdf .
     No need to care about rounding errors leading to luminance > 1;
     this cannot happen.  */
  return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
}


/* ============================= Color models ============================= */

/* The color model used by the terminal.  */
typedef enum
{
  cm_monochrome,        /* No colors.  */
  cm_common8,           /* Usual terminal with at least 8 colors.  */
  cm_xterm8,            /* TERM=xterm, with 8 colors.  */
  cm_xterm16,           /* TERM=xterm-16color, with 16 colors.  */
  cm_xterm88,           /* TERM=xterm-88color, with 88 colors.  */
  cm_xterm256           /* TERM=xterm-256color, with 256 colors.  */
} colormodel_t;

/* ----------------------- cm_monochrome color model ----------------------- */

/* A non-default color index doesn't exist in this color model.  */
static inline term_color_t
rgb_to_color_monochrome ()
{
  return COLOR_DEFAULT;
}

/* ------------------------ cm_common8 color model ------------------------ */

/* A non-default color index is in the range 0..7.
                       RGB components
   COLOR_BLACK         000
   COLOR_BLUE          001
   COLOR_GREEN         010
   COLOR_CYAN          011
   COLOR_RED           100
   COLOR_MAGENTA       101
   COLOR_YELLOW        110
   COLOR_WHITE         111 */
static const rgb_t colors_of_common8[8] =
{
  /* R    G    B        grey  index */
  {   0,   0,   0 }, /* 0.000   0 */
  {   0,   0, 255 },
  {   0, 255,   0 },
  {   0, 255, 255 },
  { 255,   0,   0 },
  { 255,   0, 255 },
  { 255, 255,   0 },
  { 255, 255, 255 }  /* 1.000   7 */
};

static inline term_color_t
rgb_to_color_common8 (int r, int g, int b)
{
  rgb_t color;
  hsv_t hsv;

  color.red = r; color.green = g; color.blue = b;
  rgb_to_hsv (color, &hsv);

  if (hsv.saturation < 0.065f)
    {
      /* Greyscale approximation.  */
      float luminance = color_luminance (r, g, b);
      if (luminance < 0.500f)
        return 0;
      else
        return 7;
    }
  else
    /* Color approximation.  */
    return nearest_color (color, colors_of_common8, 8);
}

/* Convert a cm_common8 color in RGB encoding to BGR encoding.
   See the ncurses terminfo(5) manual page, section "Color Handling", for an
   explanation why this is needed.  */
static inline int
color_bgr (term_color_t color)
{
  return ((color & 4) >> 2) | (color & 2) | ((color & 1) << 2);
}

/* ------------------------- cm_xterm8 color model ------------------------- */

/* A non-default color index is in the range 0..7.
                       BGR components
   COLOR_BLACK         000
   COLOR_RED           001
   COLOR_GREEN         010
   COLOR_YELLOW        011
   COLOR_BLUE          100
   COLOR_MAGENTA       101
   COLOR_CYAN          110
   COLOR_WHITE         111 */
static const rgb_t colors_of_xterm8[8] =
{
  /* The real xterm's colors are dimmed; assume full-brightness instead.  */
  /* R    G    B        grey  index */
  {   0,   0,   0 }, /* 0.000   0 */
  { 255,   0,   0 },
  {   0, 255,   0 },
  { 255, 255,   0 },
  {   0,   0, 255 },
  { 255,   0, 255 },
  {   0, 255, 255 },
  { 255, 255, 255 }  /* 1.000   7 */
};

static inline term_color_t
rgb_to_color_xterm8 (int r, int g, int b)
{
  rgb_t color;
  hsv_t hsv;

  color.red = r; color.green = g; color.blue = b;
  rgb_to_hsv (color, &hsv);

  if (hsv.saturation < 0.065f)
    {
      /* Greyscale approximation.  */
      float luminance = color_luminance (r, g, b);
      if (luminance < 0.500f)
        return 0;
      else
        return 7;
    }
  else
    /* Color approximation.  */
    return nearest_color (color, colors_of_xterm8, 8);
}

/* ------------------------ cm_xterm16 color model ------------------------ */

/* A non-default color index is in the range 0..15.
   The RGB values come from xterm's XTerm-col.ad.  */
static const rgb_t colors_of_xterm16[16] =
{
  /* R    G    B        grey  index */
  {   0,   0,   0 }, /* 0.000   0 */
  { 205,   0,   0 },
  {   0, 205,   0 },
  { 205, 205,   0 },
  {   0,   0, 205 },
  { 205,   0, 205 },
  {   0, 205, 205 },
  { 229, 229, 229 }, /* 0.898   7 */
  {  77,  77,  77 }, /* 0.302   8 */
  { 255,   0,   0 },
  {   0, 255,   0 },
  { 255, 255,   0 },
  {   0,   0, 255 },
  { 255,   0, 255 },
  {   0, 255, 255 },
  { 255, 255, 255 }  /* 1.000  15 */
};

static inline term_color_t
rgb_to_color_xterm16 (int r, int g, int b)
{
  rgb_t color;
  hsv_t hsv;

  color.red = r; color.green = g; color.blue = b;
  rgb_to_hsv (color, &hsv);

  if (hsv.saturation < 0.065f)
    {
      /* Greyscale approximation.  */
      float luminance = color_luminance (r, g, b);
      if (luminance < 0.151f)
        return 0;
      else if (luminance < 0.600f)
        return 8;
      else if (luminance < 0.949f)
        return 7;
      else
        return 15;
    }
  else
    /* Color approximation.  */
    return nearest_color (color, colors_of_xterm16, 16);
}

/* ------------------------ cm_xterm88 color model ------------------------ */

/* A non-default color index is in the range 0..87.
   Colors 0..15 are the same as in the cm_xterm16 color model.
   Colors 16..87 are defined in xterm's 88colres.h.  */

static const rgb_t colors_of_xterm88[88] =
{
  /* R    G    B        grey  index */
  {   0,   0,   0 }, /* 0.000   0 */
  { 205,   0,   0 },
  {   0, 205,   0 },
  { 205, 205,   0 },
  {   0,   0, 205 },
  { 205,   0, 205 },
  {   0, 205, 205 },
  { 229, 229, 229 }, /* 0.898   7 */
  {  77,  77,  77 }, /* 0.302   8 */
  { 255,   0,   0 },
  {   0, 255,   0 },
  { 255, 255,   0 },
  {   0,   0, 255 },
  { 255,   0, 255 },
  {   0, 255, 255 },
  { 255, 255, 255 }, /* 1.000  15 */
  {   0,   0,   0 }, /* 0.000  16 */
  {   0,   0, 139 },
  {   0,   0, 205 },
  {   0,   0, 255 },
  {   0, 139,   0 },
  {   0, 139, 139 },
  {   0, 139, 205 },
  {   0, 139, 255 },
  {   0, 205,   0 },
  {   0, 205, 139 },
  {   0, 205, 205 },
  {   0, 205, 255 },
  {   0, 255,   0 },
  {   0, 255, 139 },
  {   0, 255, 205 },
  {   0, 255, 255 },
  { 139,   0,   0 },
  { 139,   0, 139 },
  { 139,   0, 205 },
  { 139,   0, 255 },
  { 139, 139,   0 },
  { 139, 139, 139 }, /* 0.545  37 */
  { 139, 139, 205 },
  { 139, 139, 255 },
  { 139, 205,   0 },
  { 139, 205, 139 },
  { 139, 205, 205 },
  { 139, 205, 255 },
  { 139, 255,   0 },
  { 139, 255, 139 },
  { 139, 255, 205 },
  { 139, 255, 255 },
  { 205,   0,   0 },
  { 205,   0, 139 },
  { 205,   0, 205 },
  { 205,   0, 255 },
  { 205, 139,   0 },
  { 205, 139, 139 },
  { 205, 139, 205 },
  { 205, 139, 255 },
  { 205, 205,   0 },
  { 205, 205, 139 },
  { 205, 205, 205 }, /* 0.804  58 */
  { 205, 205, 255 },
  { 205, 255,   0 },
  { 205, 255, 139 },
  { 205, 255, 205 },
  { 205, 255, 255 },
  { 255,   0,   0 },
  { 255,   0, 139 },
  { 255,   0, 205 },
  { 255,   0, 255 },
  { 255, 139,   0 },
  { 255, 139, 139 },
  { 255, 139, 205 },
  { 255, 139, 255 },
  { 255, 205,   0 },
  { 255, 205, 139 },
  { 255, 205, 205 },
  { 255, 205, 255 },
  { 255, 255,   0 },
  { 255, 255, 139 },
  { 255, 255, 205 },
  { 255, 255, 255 }, /* 1.000  79 */
  {  46,  46,  46 }, /* 0.180  80 */
  {  92,  92,  92 }, /* 0.361  81 */
  { 115, 115, 115 }, /* 0.451  82 */
  { 139, 139, 139 }, /* 0.545  83 */
  { 162, 162, 162 }, /* 0.635  84 */
  { 185, 185, 185 }, /* 0.725  85 */
  { 208, 208, 208 }, /* 0.816  86 */
  { 231, 231, 231 }  /* 0.906  87 */
};

static inline term_color_t
rgb_to_color_xterm88 (int r, int g, int b)
{
  rgb_t color;
  hsv_t hsv;

  color.red = r; color.green = g; color.blue = b;
  rgb_to_hsv (color, &hsv);

  if (hsv.saturation < 0.065f)
    {
      /* Greyscale approximation.  */
      float luminance = color_luminance (r, g, b);
      if (luminance < 0.090f)
        return 0;
      else if (luminance < 0.241f)
        return 80;
      else if (luminance < 0.331f)
        return 8;
      else if (luminance < 0.406f)
        return 81;
      else if (luminance < 0.498f)
        return 82;
      else if (luminance < 0.585f)
        return 37;
      else if (luminance < 0.680f)
        return 84;
      else if (luminance < 0.764f)
        return 85;
      else if (luminance < 0.810f)
        return 58;
      else if (luminance < 0.857f)
        return 86;
      else if (luminance < 0.902f)
        return 7;
      else if (luminance < 0.953f)
        return 87;
      else
        return 15;
    }
  else
    /* Color approximation.  */
    return nearest_color (color, colors_of_xterm88, 88);
}

/* ------------------------ cm_xterm256 color model ------------------------ */

/* A non-default color index is in the range 0..255.
   Colors 0..15 are the same as in the cm_xterm16 color model.
   Colors 16..255 are defined in xterm's 256colres.h.  */

static const rgb_t colors_of_xterm256[256] =
{
  /* R    G    B        grey  index */
  {   0,   0,   0 }, /* 0.000   0 */
  { 205,   0,   0 },
  {   0, 205,   0 },
  { 205, 205,   0 },
  {   0,   0, 205 },
  { 205,   0, 205 },
  {   0, 205, 205 },
  { 229, 229, 229 }, /* 0.898   7 */
  {  77,  77,  77 }, /* 0.302   8 */
  { 255,   0,   0 },
  {   0, 255,   0 },
  { 255, 255,   0 },
  {   0,   0, 255 },
  { 255,   0, 255 },
  {   0, 255, 255 },
  { 255, 255, 255 }, /* 1.000  15 */
  {   0,   0,   0 }, /* 0.000  16 */
  {   0,   0,  42 },
  {   0,   0,  85 },
  {   0,   0, 127 },
  {   0,   0, 170 },
  {   0,   0, 212 },
  {   0,  42,   0 },
  {   0,  42,  42 },
  {   0,  42,  85 },
  {   0,  42, 127 },
  {   0,  42, 170 },
  {   0,  42, 212 },
  {   0,  85,   0 },
  {   0,  85,  42 },
  {   0,  85,  85 },
  {   0,  85, 127 },
  {   0,  85, 170 },
  {   0,  85, 212 },
  {   0, 127,   0 },
  {   0, 127,  42 },
  {   0, 127,  85 },
  {   0, 127, 127 },
  {   0, 127, 170 },
  {   0, 127, 212 },
  {   0, 170,   0 },
  {   0, 170,  42 },
  {   0, 170,  85 },
  {   0, 170, 127 },
  {   0, 170, 170 },
  {   0, 170, 212 },
  {   0, 212,   0 },
  {   0, 212,  42 },
  {   0, 212,  85 },
  {   0, 212, 127 },
  {   0, 212, 170 },
  {   0, 212, 212 },
  {  42,   0,   0 },
  {  42,   0,  42 },
  {  42,   0,  85 },
  {  42,   0, 127 },
  {  42,   0, 170 },
  {  42,   0, 212 },
  {  42,  42,   0 },
  {  42,  42,  42 }, /* 0.165  59 */
  {  42,  42,  85 },
  {  42,  42, 127 },
  {  42,  42, 170 },
  {  42,  42, 212 },
  {  42,  85,   0 },
  {  42,  85,  42 },
  {  42,  85,  85 },
  {  42,  85, 127 },
  {  42,  85, 170 },
  {  42,  85, 212 },
  {  42, 127,   0 },
  {  42, 127,  42 },
  {  42, 127,  85 },
  {  42, 127, 127 },
  {  42, 127, 170 },
  {  42, 127, 212 },
  {  42, 170,   0 },
  {  42, 170,  42 },
  {  42, 170,  85 },
  {  42, 170, 127 },
  {  42, 170, 170 },
  {  42, 170, 212 },
  {  42, 212,   0 },
  {  42, 212,  42 },
  {  42, 212,  85 },
  {  42, 212, 127 },
  {  42, 212, 170 },
  {  42, 212, 212 },
  {  85,   0,   0 },
  {  85,   0,  42 },
  {  85,   0,  85 },
  {  85,   0, 127 },
  {  85,   0, 170 },
  {  85,   0, 212 },
  {  85,  42,   0 },
  {  85,  42,  42 },
  {  85,  42,  85 },
  {  85,  42, 127 },
  {  85,  42, 170 },
  {  85,  42, 212 },
  {  85,  85,   0 },
  {  85,  85,  42 },
  {  85,  85,  85 }, /* 0.333 102 */
  {  85,  85, 127 },
  {  85,  85, 170 },
  {  85,  85, 212 },
  {  85, 127,   0 },
  {  85, 127,  42 },
  {  85, 127,  85 },
  {  85, 127, 127 },
  {  85, 127, 170 },
  {  85, 127, 212 },
  {  85, 170,   0 },
  {  85, 170,  42 },
  {  85, 170,  85 },
  {  85, 170, 127 },
  {  85, 170, 170 },
  {  85, 170, 212 },
  {  85, 212,   0 },
  {  85, 212,  42 },
  {  85, 212,  85 },
  {  85, 212, 127 },
  {  85, 212, 170 },
  {  85, 212, 212 },
  { 127,   0,   0 },
  { 127,   0,  42 },
  { 127,   0,  85 },
  { 127,   0, 127 },
  { 127,   0, 170 },
  { 127,   0, 212 },
  { 127,  42,   0 },
  { 127,  42,  42 },
  { 127,  42,  85 },
  { 127,  42, 127 },
  { 127,  42, 170 },
  { 127,  42, 212 },
  { 127,  85,   0 },
  { 127,  85,  42 },
  { 127,  85,  85 },
  { 127,  85, 127 },
  { 127,  85, 170 },
  { 127,  85, 212 },
  { 127, 127,   0 },
  { 127, 127,  42 },
  { 127, 127,  85 },
  { 127, 127, 127 }, /* 0.498 145 */
  { 127, 127, 170 },
  { 127, 127, 212 },
  { 127, 170,   0 },
  { 127, 170,  42 },
  { 127, 170,  85 },
  { 127, 170, 127 },
  { 127, 170, 170 },
  { 127, 170, 212 },
  { 127, 212,   0 },
  { 127, 212,  42 },
  { 127, 212,  85 },
  { 127, 212, 127 },
  { 127, 212, 170 },
  { 127, 212, 212 },
  { 170,   0,   0 },
  { 170,   0,  42 },
  { 170,   0,  85 },
  { 170,   0, 127 },
  { 170,   0, 170 },
  { 170,   0, 212 },
  { 170,  42,   0 },
  { 170,  42,  42 },
  { 170,  42,  85 },
  { 170,  42, 127 },
  { 170,  42, 170 },
  { 170,  42, 212 },
  { 170,  85,   0 },
  { 170,  85,  42 },
  { 170,  85,  85 },
  { 170,  85, 127 },
  { 170,  85, 170 },
  { 170,  85, 212 },
  { 170, 127,   0 },
  { 170, 127,  42 },
  { 170, 127,  85 },
  { 170, 127, 127 },
  { 170, 127, 170 },
  { 170, 127, 212 },
  { 170, 170,   0 },
  { 170, 170,  42 },
  { 170, 170,  85 },
  { 170, 170, 127 },
  { 170, 170, 170 }, /* 0.667 188 */
  { 170, 170, 212 },
  { 170, 212,   0 },
  { 170, 212,  42 },
  { 170, 212,  85 },
  { 170, 212, 127 },
  { 170, 212, 170 },
  { 170, 212, 212 },
  { 212,   0,   0 },
  { 212,   0,  42 },
  { 212,   0,  85 },
  { 212,   0, 127 },
  { 212,   0, 170 },
  { 212,   0, 212 },
  { 212,  42,   0 },
  { 212,  42,  42 },
  { 212,  42,  85 },
  { 212,  42, 127 },
  { 212,  42, 170 },
  { 212,  42, 212 },
  { 212,  85,   0 },
  { 212,  85,  42 },
  { 212,  85,  85 },
  { 212,  85, 127 },
  { 212,  85, 170 },
  { 212,  85, 212 },
  { 212, 127,   0 },
  { 212, 127,  42 },
  { 212, 127,  85 },
  { 212, 127, 127 },
  { 212, 127, 170 },
  { 212, 127, 212 },
  { 212, 170,   0 },
  { 212, 170,  42 },
  { 212, 170,  85 },
  { 212, 170, 127 },
  { 212, 170, 170 },
  { 212, 170, 212 },
  { 212, 212,   0 },
  { 212, 212,  42 },
  { 212, 212,  85 },
  { 212, 212, 127 },
  { 212, 212, 170 },
  { 212, 212, 212 }, /* 0.831 231 */
  {   8,   8,   8 }, /* 0.031 232 */
  {  18,  18,  18 }, /* 0.071 233 */
  {  28,  28,  28 }, /* 0.110 234 */
  {  38,  38,  38 }, /* 0.149 235 */
  {  48,  48,  48 }, /* 0.188 236 */
  {  58,  58,  58 }, /* 0.227 237 */
  {  68,  68,  68 }, /* 0.267 238 */
  {  78,  78,  78 }, /* 0.306 239 */
  {  88,  88,  88 }, /* 0.345 240 */
  {  98,  98,  98 }, /* 0.384 241 */
  { 108, 108, 108 }, /* 0.424 242 */
  { 118, 118, 118 }, /* 0.463 243 */
  { 128, 128, 128 }, /* 0.502 244 */
  { 138, 138, 138 }, /* 0.541 245 */
  { 148, 148, 148 }, /* 0.580 246 */
  { 158, 158, 158 }, /* 0.620 247 */
  { 168, 168, 168 }, /* 0.659 248 */
  { 178, 178, 178 }, /* 0.698 249 */
  { 188, 188, 188 }, /* 0.737 250 */
  { 198, 198, 198 }, /* 0.776 251 */
  { 208, 208, 208 }, /* 0.816 252 */
  { 218, 218, 218 }, /* 0.855 253 */
  { 228, 228, 228 }, /* 0.894 254 */
  { 238, 238, 238 }  /* 0.933 255 */
};

static inline term_color_t
rgb_to_color_xterm256 (int r, int g, int b)
{
  rgb_t color;
  hsv_t hsv;

  color.red = r; color.green = g; color.blue = b;
  rgb_to_hsv (color, &hsv);

  if (hsv.saturation < 0.065f)
    {
      /* Greyscale approximation.  */
      float luminance = color_luminance (r, g, b);
      if (luminance < 0.015f)
        return 0;
      else if (luminance < 0.051f)
        return 232;
      else if (luminance < 0.090f)
        return 233;
      else if (luminance < 0.129f)
        return 234;
      else if (luminance < 0.157f)
        return 235;
      else if (luminance < 0.177f)
        return 59;
      else if (luminance < 0.207f)
        return 236;
      else if (luminance < 0.247f)
        return 237;
      else if (luminance < 0.284f)
        return 238;
      else if (luminance < 0.304f)
        return 8;
      else if (luminance < 0.319f)
        return 239;
      else if (luminance < 0.339f)
        return 102;
      else if (luminance < 0.364f)
        return 240;
      else if (luminance < 0.404f)
        return 241;
      else if (luminance < 0.443f)
        return 242;
      else if (luminance < 0.480f)
        return 243;
      else if (luminance < 0.500f)
        return 145;
      else if (luminance < 0.521f)
        return 244;
      else if (luminance < 0.560f)
        return 245;
      else if (luminance < 0.600f)
        return 246;
      else if (luminance < 0.639f)
        return 247;
      else if (luminance < 0.663f)
        return 248;
      else if (luminance < 0.682f)
        return 188;
      else if (luminance < 0.717f)
        return 249;
      else if (luminance < 0.756f)
        return 250;
      else if (luminance < 0.796f)
        return 251;
      else if (luminance < 0.823f)
        return 252;
      else if (luminance < 0.843f)
        return 231;
      else if (luminance < 0.874f)
        return 253;
      else if (luminance < 0.896f)
        return 254;
      else if (luminance < 0.915f)
        return 7;
      else if (luminance < 0.966f)
        return 255;
      else
        return 15;
    }
  else
    /* Color approximation.  */
    return nearest_color (color, colors_of_xterm256, 256);
}


/* ============================= attributes_t ============================= */

/* ANSI C and ISO C99 6.7.2.1.(4) forbid use of bit fields for types other
   than 'int' or 'unsigned int'.
   On the other hand, C++ forbids conversion between enum types and integer
   types without an explicit cast.  */
#ifdef __cplusplus
# define BITFIELD_TYPE(orig_type,integer_type) orig_type
#else
# define BITFIELD_TYPE(orig_type,integer_type) integer_type
#endif

/* Attributes that can be set on a character.  */
typedef struct
{
  BITFIELD_TYPE(term_color_t,     signed int)   color     : 9;
  BITFIELD_TYPE(term_color_t,     signed int)   bgcolor   : 9;
  BITFIELD_TYPE(term_weight_t,    unsigned int) weight    : 1;
  BITFIELD_TYPE(term_posture_t,   unsigned int) posture   : 1;
  BITFIELD_TYPE(term_underline_t, unsigned int) underline : 1;
} attributes_t;


/* ============================ term_ostream_t ============================ */

struct term_ostream : struct ostream
{
fields:
  /* The file descriptor used for output.  Note that ncurses termcap emulation
     uses the baud rate information from file descriptor 1 (stdout) if it is
     a tty, or from file descriptor 2 (stderr) otherwise.  */
  int fd;
  char *filename;
  /* Values from the terminal type's terminfo/termcap description.
     See terminfo(5) for details.  */
                                /* terminfo  termcap */
  int max_colors;               /* colors    Co */
  int no_color_video;           /* ncv       NC */
  char *set_a_foreground;       /* setaf     AF */
  char *set_foreground;         /* setf      Sf */
  char *set_a_background;       /* setab     AB */
  char *set_background;         /* setb      Sb */
  char *orig_pair;              /* op        op */
  char *enter_bold_mode;        /* bold      md */
  char *enter_italics_mode;     /* sitm      ZH */
  char *exit_italics_mode;      /* ritm      ZR */
  char *enter_underline_mode;   /* smul      us */
  char *exit_underline_mode;    /* rmul      ue */
  char *exit_attribute_mode;    /* sgr0      me */
  /* Inferred values.  */
  bool supports_foreground;
  bool supports_background;
  colormodel_t colormodel;
  bool supports_weight;
  bool supports_posture;
  bool supports_underline;
  /* Variable state.  */
  char *buffer;                 /* Buffer for the current line.  */
  attributes_t *attrbuffer;     /* Buffer for the simplified attributes; same
                                   length as buffer.  */
  size_t buflen;                /* Number of bytes stored so far.  */
  size_t allocated;             /* Allocated size of the buffer.  */
  attributes_t curr_attr;       /* Current attributes.  */
  attributes_t simp_attr;       /* Simplified current attributes.  */
};

/* Simplify attributes, according to the terminal's capabilities.  */
static attributes_t
simplify_attributes (term_ostream_t stream, attributes_t attr)
{
  if ((attr.color != COLOR_DEFAULT || attr.bgcolor != COLOR_DEFAULT)
      && stream->no_color_video > 0)
    {
      /* When colors and attributes can not be represented simultaneously,
         we give preference to the color.  */
      if (stream->no_color_video & 2)
        /* Colors conflict with underlining.  */
        attr.underline = UNDERLINE_OFF;
      if (stream->no_color_video & 32)
        /* Colors conflict with bold weight.  */
        attr.weight = WEIGHT_NORMAL;
    }
  if (!stream->supports_foreground)
    attr.color = COLOR_DEFAULT;
  if (!stream->supports_background)
    attr.bgcolor = COLOR_DEFAULT;
  if (!stream->supports_weight)
    attr.weight = WEIGHT_DEFAULT;
  if (!stream->supports_posture)
    attr.posture = POSTURE_DEFAULT;
  if (!stream->supports_underline)
    attr.underline = UNDERLINE_DEFAULT;
  return attr;
}

/* While a line is being output, we need to be careful to restore the
   terminal's settings in case of a fatal signal or an exit() call.  */

/* File descriptor to which out_char shall output escape sequences.  */
static int out_fd = -1;

/* Filename of out_fd.  */
static const char *out_filename;

/* Output a single char to out_fd.  Ignore errors.  */
static int
out_char_unchecked (int c)
{
  char bytes[1];

  bytes[0] = (char)c;
  full_write (out_fd, bytes, 1);
  return 0;
}

/* State that informs the exit handler what to do.  */
static const char *restore_colors;
static const char *restore_weight;
static const char *restore_posture;
static const char *restore_underline;

/* The exit handler.  */
static void
restore (void)
{
  /* Only do something while some output was interrupted.  */
  if (out_fd >= 0)
    {
      if (restore_colors != NULL)
        tputs (restore_colors, 1, out_char_unchecked);
      if (restore_weight != NULL)
        tputs (restore_weight, 1, out_char_unchecked);
      if (restore_posture != NULL)
        tputs (restore_posture, 1, out_char_unchecked);
      if (restore_underline != NULL)
        tputs (restore_underline, 1, out_char_unchecked);
    }
}

/* The list of signals whose default behaviour is to stop the program.  */
static int stopping_signals[] =
  {
#ifdef SIGTSTP
    SIGTSTP,
#endif
#ifdef SIGTTIN
    SIGTTIN,
#endif
#ifdef SIGTTOU
    SIGTTOU,
#endif
    0
  };

#define num_stopping_signals (SIZEOF (stopping_signals) - 1)

static sigset_t stopping_signal_set;

static void
init_stopping_signal_set ()
{
  static bool stopping_signal_set_initialized = false;
  if (!stopping_signal_set_initialized)
    {
      size_t i;

      sigemptyset (&stopping_signal_set);
      for (i = 0; i < num_stopping_signals; i++)
        sigaddset (&stopping_signal_set, stopping_signals[i]);

      stopping_signal_set_initialized = true;
    }
}

/* Temporarily delay the stopping signals.  */
static inline void
block_stopping_signals ()
{
  init_stopping_signal_set ();
  sigprocmask (SIG_BLOCK, &stopping_signal_set, NULL);
}

/* Stop delaying the stopping signals.  */
static inline void
unblock_stopping_signals ()
{
  init_stopping_signal_set ();
  sigprocmask (SIG_UNBLOCK, &stopping_signal_set, NULL);
}

/* Compare two sets of attributes for equality.  */
static inline bool
equal_attributes (attributes_t attr1, attributes_t attr2)
{
  return (attr1.color == attr2.color
          && attr1.bgcolor == attr2.bgcolor
          && attr1.weight == attr2.weight
          && attr1.posture == attr2.posture
          && attr1.underline == attr2.underline);
}

/* Signal error after full_write failed.  */
static void
out_error ()
{
  error (EXIT_FAILURE, errno, _("error writing to %s"), out_filename);
}

/* Output a single char to out_fd.  */
static int
out_char (int c)
{
  char bytes[1];

  bytes[0] = (char)c;
  /* We have to write directly to the file descriptor, not to a buffer with
     the same destination, because of the padding and sleeping that tputs()
     does.  */
  if (full_write (out_fd, bytes, 1) < 1)
    out_error ();
  return 0;
}

/* Output escape sequences to switch from OLD_ATTR to NEW_ATTR.  */
static void
out_attr_change (term_ostream_t stream,
                 attributes_t old_attr, attributes_t new_attr)
{
  bool cleared_attributes;

  /* We don't know the default colors of the terminal.  The only way to switch
     back to a default color is to use stream->orig_pair.  */
  if ((new_attr.color == COLOR_DEFAULT && old_attr.color != COLOR_DEFAULT)
      || (new_attr.bgcolor == COLOR_DEFAULT && old_attr.bgcolor != COLOR_DEFAULT))
    {
      assert (stream->supports_foreground || stream->supports_background);
      tputs (stream->orig_pair, 1, out_char);
      old_attr.color = COLOR_DEFAULT;
      old_attr.bgcolor = COLOR_DEFAULT;
    }

  /* To turn off WEIGHT_BOLD, the only way is to output the exit_attribute_mode
     sequence.  (With xterm, you can also do it with "Esc [ 0 m", but this
     escape sequence is not contained in the terminfo description.)  It may
     also clear the colors; this is the case e.g. when TERM="xterm" or
     TERM="ansi".
     To turn off UNDERLINE_ON, we can use the exit_underline_mode or the
     exit_attribute_mode sequence.  In the latter case, it will not only
     turn off UNDERLINE_ON, but also the other attributes, and possibly also
     the colors.
     To turn off POSTURE_ITALIC, we can use the exit_italics_mode or the
     exit_attribute_mode sequence.  Again, in the latter case, it will not
     only turn off POSTURE_ITALIC, but also the other attributes, and possibly
     also the colors.
     There is no point in setting an attribute just before emitting an
     escape sequence that may again turn off the attribute.  Therefore we
     proceed in two steps: First, clear the attributes that need to be
     cleared; then - taking into account that this may have cleared all
     attributes and all colors - set the colors and the attributes.
     The variable 'cleared_attributes' tells whether an escape sequence
     has been output that may have cleared all attributes and all color
     settings.  */
  cleared_attributes = false;
  if (old_attr.posture != POSTURE_NORMAL
      && new_attr.posture == POSTURE_NORMAL
      && stream->exit_italics_mode != NULL)
    {
      tputs (stream->exit_italics_mode, 1, out_char);
      old_attr.posture = POSTURE_NORMAL;
      cleared_attributes = true;
    }
  if (old_attr.underline != UNDERLINE_OFF
      && new_attr.underline == UNDERLINE_OFF
      && stream->exit_underline_mode != NULL)
    {
      tputs (stream->exit_underline_mode, 1, out_char);
      old_attr.underline = UNDERLINE_OFF;
      cleared_attributes = true;
    }
  if ((old_attr.weight != WEIGHT_NORMAL
       && new_attr.weight == WEIGHT_NORMAL)
      || (old_attr.posture != POSTURE_NORMAL
          && new_attr.posture == POSTURE_NORMAL
          /* implies stream->exit_italics_mode == NULL */)
      || (old_attr.underline != UNDERLINE_OFF
          && new_attr.underline == UNDERLINE_OFF
          /* implies stream->exit_underline_mode == NULL */))
    {
      tputs (stream->exit_attribute_mode, 1, out_char);
      /* We don't know exactly what effects exit_attribute_mode has, but
         this is the minimum effect:  */
      old_attr.weight = WEIGHT_NORMAL;
      if (stream->exit_italics_mode == NULL)
        old_attr.posture = POSTURE_NORMAL;
      if (stream->exit_underline_mode == NULL)
        old_attr.underline = UNDERLINE_OFF;
      cleared_attributes = true;
    }

  /* Turn on the colors.  */
  if (new_attr.color != old_attr.color
      || (cleared_attributes && new_attr.color != COLOR_DEFAULT))
    {
      assert (stream->supports_foreground);
      assert (new_attr.color != COLOR_DEFAULT);
      switch (stream->colormodel)
        {
        case cm_common8:
          assert (new_attr.color >= 0 && new_attr.color < 8);
          if (stream->set_a_foreground != NULL)
            tputs (tparm (stream->set_a_foreground,
                          color_bgr (new_attr.color)),
                   1, out_char);
          else
            tputs (tparm (stream->set_foreground, new_attr.color),
                   1, out_char);
          break;
        /* When we are dealing with an xterm, there is no need to go through
           tputs() because we know there is no padding and sleeping.  */
        case cm_xterm8:
          assert (new_attr.color >= 0 && new_attr.color < 8);
          {
            char bytes[5];
            bytes[0] = 0x1B; bytes[1] = '[';
            bytes[2] = '3'; bytes[3] = '0' + new_attr.color;
            bytes[4] = 'm';
            if (full_write (out_fd, bytes, 5) < 5)
              out_error ();
          }
          break;
        case cm_xterm16:
          assert (new_attr.color >= 0 && new_attr.color < 16);
          {
            char bytes[5];
            bytes[0] = 0x1B; bytes[1] = '[';
            if (new_attr.color < 8)
              {
                bytes[2] = '3'; bytes[3] = '0' + new_attr.color;
              }
            else
              {
                bytes[2] = '9'; bytes[3] = '0' + (new_attr.color - 8);
              }
            bytes[4] = 'm';
            if (full_write (out_fd, bytes, 5) < 5)
              out_error ();
          }
          break;
        case cm_xterm88:
          assert (new_attr.color >= 0 && new_attr.color < 88);
          {
            char bytes[10];
            char *p;
            bytes[0] = 0x1B; bytes[1] = '[';
            bytes[2] = '3'; bytes[3] = '8'; bytes[4] = ';';
            bytes[5] = '5'; bytes[6] = ';';
            p = bytes + 7;
            if (new_attr.color >= 10)
              *p++ = '0' + (new_attr.color / 10);
            *p++ = '0' + (new_attr.color % 10);
            *p++ = 'm';
            if (full_write (out_fd, bytes, p - bytes) < p - bytes)
              out_error ();
          }
          break;
        case cm_xterm256:
          assert (new_attr.color >= 0 && new_attr.color < 256);
          {
            char bytes[11];
            char *p;
            bytes[0] = 0x1B; bytes[1] = '[';
            bytes[2] = '3'; bytes[3] = '8'; bytes[4] = ';';
            bytes[5] = '5'; bytes[6] = ';';
            p = bytes + 7;
            if (new_attr.color >= 100)
              *p++ = '0' + (new_attr.color / 100);
            if (new_attr.color >= 10)
              *p++ = '0' + ((new_attr.color % 100) / 10);
            *p++ = '0' + (new_attr.color % 10);
            *p++ = 'm';
            if (full_write (out_fd, bytes, p - bytes) < p - bytes)
              out_error ();
          }
          break;
        default:
          abort ();
        }
    }
  if (new_attr.bgcolor != old_attr.bgcolor
      || (cleared_attributes && new_attr.bgcolor != COLOR_DEFAULT))
    {
      assert (stream->supports_background);
      assert (new_attr.bgcolor != COLOR_DEFAULT);
      switch (stream->colormodel)
        {
        case cm_common8:
          assert (new_attr.bgcolor >= 0 && new_attr.bgcolor < 8);
          if (stream->set_a_background != NULL)
            tputs (tparm (stream->set_a_background,
                          color_bgr (new_attr.bgcolor)),
                   1, out_char);
          else
            tputs (tparm (stream->set_background, new_attr.bgcolor),
                   1, out_char);
          break;
        /* When we are dealing with an xterm, there is no need to go through
           tputs() because we know there is no padding and sleeping.  */
        case cm_xterm8:
          assert (new_attr.bgcolor >= 0 && new_attr.bgcolor < 8);
          {
            char bytes[5];
            bytes[0] = 0x1B; bytes[1] = '[';
            bytes[2] = '4'; bytes[3] = '0' + new_attr.bgcolor;
            bytes[4] = 'm';
            if (full_write (out_fd, bytes, 5) < 5)
              out_error ();
          }
          break;
        case cm_xterm16:
          assert (new_attr.bgcolor >= 0 && new_attr.bgcolor < 16);
          {
            char bytes[6];
            bytes[0] = 0x1B; bytes[1] = '[';
            if (new_attr.bgcolor < 8)
              {
                bytes[2] = '4'; bytes[3] = '0' + new_attr.bgcolor;
                bytes[4] = 'm';
                if (full_write (out_fd, bytes, 5) < 5)
                  out_error ();
              }
            else
              {
                bytes[2] = '1'; bytes[3] = '0';
                bytes[4] = '0' + (new_attr.bgcolor - 8); bytes[5] = 'm';
                if (full_write (out_fd, bytes, 6) < 6)
                  out_error ();
              }
          }
          break;
        case cm_xterm88:
          assert (new_attr.bgcolor >= 0 && new_attr.bgcolor < 88);
          {
            char bytes[10];
            char *p;
            bytes[0] = 0x1B; bytes[1] = '[';
            bytes[2] = '4'; bytes[3] = '8'; bytes[4] = ';';
            bytes[5] = '5'; bytes[6] = ';';
            p = bytes + 7;
            if (new_attr.bgcolor >= 10)
              *p++ = '0' + (new_attr.bgcolor / 10);
            *p++ = '0' + (new_attr.bgcolor % 10);
            *p++ = 'm';
            if (full_write (out_fd, bytes, p - bytes) < p - bytes)
              out_error ();
          }
          break;
        case cm_xterm256:
          assert (new_attr.bgcolor >= 0 && new_attr.bgcolor < 256);
          {
            char bytes[11];
            char *p;
            bytes[0] = 0x1B; bytes[1] = '[';
            bytes[2] = '4'; bytes[3] = '8'; bytes[4] = ';';
            bytes[5] = '5'; bytes[6] = ';';
            p = bytes + 7;
            if (new_attr.bgcolor >= 100)
              *p++ = '0' + (new_attr.bgcolor / 100);
            if (new_attr.bgcolor >= 10)
              *p++ = '0' + ((new_attr.bgcolor % 100) / 10);
            *p++ = '0' + (new_attr.bgcolor % 10);
            *p++ = 'm';
            if (full_write (out_fd, bytes, p - bytes) < p - bytes)
              out_error ();
          }
          break;
        default:
          abort ();
        }
    }

  if (new_attr.weight != old_attr.weight
      || (cleared_attributes && new_attr.weight != WEIGHT_DEFAULT))
    {
      assert (stream->supports_weight);
      assert (new_attr.weight != WEIGHT_DEFAULT);
      /* This implies:  */
      assert (new_attr.weight == WEIGHT_BOLD);
      tputs (stream->enter_bold_mode, 1, out_char);
    }
  if (new_attr.posture != old_attr.posture
      || (cleared_attributes && new_attr.posture != POSTURE_DEFAULT))
    {
      assert (stream->supports_posture);
      assert (new_attr.posture != POSTURE_DEFAULT);
      /* This implies:  */
      assert (new_attr.posture == POSTURE_ITALIC);
      tputs (stream->enter_italics_mode, 1, out_char);
    }
  if (new_attr.underline != old_attr.underline
      || (cleared_attributes && new_attr.underline != UNDERLINE_DEFAULT))
    {
      assert (stream->supports_underline);
      assert (new_attr.underline != UNDERLINE_DEFAULT);
      /* This implies:  */
      assert (new_attr.underline == UNDERLINE_ON);
      tputs (stream->enter_underline_mode, 1, out_char);
    }
}

/* Output the buffered line atomically.
   The terminal is assumed to have the default state (regarding colors and
   attributes) before this call.  It is left in default state after this
   call (regardless of stream->curr_attr).  */
static void
output_buffer (term_ostream_t stream)
{
  attributes_t default_attr;
  attributes_t attr;
  const char *cp;
  const attributes_t *ap;
  size_t len;
  size_t n;

  default_attr.color = COLOR_DEFAULT;
  default_attr.bgcolor = COLOR_DEFAULT;
  default_attr.weight = WEIGHT_DEFAULT;
  default_attr.posture = POSTURE_DEFAULT;
  default_attr.underline = UNDERLINE_DEFAULT;

  attr = default_attr;

  cp = stream->buffer;
  ap = stream->attrbuffer;
  len = stream->buflen;

  /* See how much we can output without blocking signals.  */
  for (n = 0; n < len && equal_attributes (ap[n], attr); n++)
    ;
  if (n > 0)
    {
      if (full_write (stream->fd, cp, n) < n)
        error (EXIT_FAILURE, errno, _("error writing to %s"), stream->filename);
      cp += n;
      ap += n;
      len -= n;
    }
  if (len > 0)
    {
      /* Block fatal signals, so that a SIGINT or similar doesn't interrupt
         us without the possibility of restoring the terminal's state.  */
      block_fatal_signals ();
      /* Likewise for SIGTSTP etc.  */
      block_stopping_signals ();

      /* Enable the exit handler for restoring the terminal's state.  */
      restore_colors =
        (stream->supports_foreground || stream->supports_background
         ? stream->orig_pair
         : NULL);
      restore_weight =
        (stream->supports_weight ? stream->exit_attribute_mode : NULL);
      restore_posture =
        (stream->supports_posture
         ? (stream->exit_italics_mode != NULL
            ? stream->exit_italics_mode
            : stream->exit_attribute_mode)
         : NULL);
      restore_underline =
        (stream->supports_underline
         ? (stream->exit_underline_mode != NULL
            ? stream->exit_underline_mode
            : stream->exit_attribute_mode)
         : NULL);
      out_fd = stream->fd;
      out_filename = stream->filename;

      while (len > 0)
        {
          /* Activate the attributes in *ap.  */
          out_attr_change (stream, attr, *ap);
          attr = *ap;
          /* See how many characters we can output without further attribute
             changes.  */
          for (n = 1; n < len && equal_attributes (ap[n], attr); n++)
            ;
          if (full_write (stream->fd, cp, n) < n)
            error (EXIT_FAILURE, errno, _("error writing to %s"),
                   stream->filename);
          cp += n;
          ap += n;
          len -= n;
        }

      /* Switch back to the default attributes.  */
      out_attr_change (stream, attr, default_attr);

      /* Disable the exit handler.  */
      out_fd = -1;
      out_filename = NULL;

      /* Unblock fatal and stopping signals.  */
      unblock_stopping_signals ();
      unblock_fatal_signals ();
    }
  stream->buflen = 0;
}

/* Implementation of ostream_t methods.  */

static term_color_t
term_ostream::rgb_to_color (term_ostream_t stream, int red, int green, int blue)
{
  switch (stream->colormodel)
    {
    case cm_monochrome:
      return rgb_to_color_monochrome ();
    case cm_common8:
      return rgb_to_color_common8 (red, green, blue);
    case cm_xterm8:
      return rgb_to_color_xterm8 (red, green, blue);
    case cm_xterm16:
      return rgb_to_color_xterm16 (red, green, blue);
    case cm_xterm88:
      return rgb_to_color_xterm88 (red, green, blue);
    case cm_xterm256:
      return rgb_to_color_xterm256 (red, green, blue);
    default:
      abort ();
    }
}

static void
term_ostream::write_mem (term_ostream_t stream, const void *data, size_t len)
{
  const char *cp = (const char *) data;
  while (len > 0)
    {
      /* Look for the next newline.  */
      const char *newline = (const char *) memchr (cp, '\n', len);
      size_t n = (newline != NULL ? newline - cp : len);

      /* Copy n bytes into the buffer.  */
      if (n > stream->allocated - stream->buflen)
        {
          size_t new_allocated =
            xmax (xsum (stream->buflen, n),
                  xsum (stream->allocated, stream->allocated));
          if (size_overflow_p (new_allocated))
            error (EXIT_FAILURE, 0,
                   _("%s: too much output, buffer size overflow"),
                   "term_ostream");
          stream->buffer = (char *) xrealloc (stream->buffer, new_allocated);
          stream->attrbuffer =
            (attributes_t *)
            xrealloc (stream->attrbuffer,
                      new_allocated * sizeof (attributes_t));
          stream->allocated = new_allocated;
        }
      memcpy (stream->buffer + stream->buflen, cp, n);
      {
        attributes_t attr = stream->simp_attr;
        attributes_t *ap = stream->attrbuffer + stream->buflen;
        attributes_t *ap_end = ap + n;
        for (; ap < ap_end; ap++)
          *ap = attr;
      }
      stream->buflen += n;

      if (newline != NULL)
        {
          output_buffer (stream);
          if (full_write (stream->fd, "\n", 1) < 1)
            error (EXIT_FAILURE, errno, _("error writing to %s"),
                   stream->filename);
          cp += n + 1; /* cp = newline + 1; */
          len -= n + 1;
        }
      else
        break;
    }
}

static void
term_ostream::flush (term_ostream_t stream)
{
  output_buffer (stream);
}

static void
term_ostream::free (term_ostream_t stream)
{
  term_ostream_flush (stream);
  free (stream->filename);
  if (stream->set_a_foreground != NULL)
    free (stream->set_a_foreground);
  if (stream->set_foreground != NULL)
    free (stream->set_foreground);
  if (stream->set_a_background != NULL)
    free (stream->set_a_background);
  if (stream->set_background != NULL)
    free (stream->set_background);
  if (stream->orig_pair != NULL)
    free (stream->orig_pair);
  if (stream->enter_bold_mode != NULL)
    free (stream->enter_bold_mode);
  if (stream->enter_italics_mode != NULL)
    free (stream->enter_italics_mode);
  if (stream->exit_italics_mode != NULL)
    free (stream->exit_italics_mode);
  if (stream->enter_underline_mode != NULL)
    free (stream->enter_underline_mode);
  if (stream->exit_underline_mode != NULL)
    free (stream->exit_underline_mode);
  if (stream->exit_attribute_mode != NULL)
    free (stream->exit_attribute_mode);
  free (stream->buffer);
  free (stream);
}

/* Implementation of term_ostream_t methods.  */

static term_color_t
term_ostream::get_color (term_ostream_t stream)
{
  return stream->curr_attr.color;
}

static void
term_ostream::set_color (term_ostream_t stream, term_color_t color)
{
  stream->curr_attr.color = color;
  stream->simp_attr = simplify_attributes (stream, stream->curr_attr);
}

static term_color_t
term_ostream::get_bgcolor (term_ostream_t stream)
{
  return stream->curr_attr.bgcolor;
}

static void
term_ostream::set_bgcolor (term_ostream_t stream, term_color_t color)
{
  stream->curr_attr.bgcolor = color;
  stream->simp_attr = simplify_attributes (stream, stream->curr_attr);
}

static term_weight_t
term_ostream::get_weight (term_ostream_t stream)
{
  return stream->curr_attr.weight;
}

static void
term_ostream::set_weight (term_ostream_t stream, term_weight_t weight)
{
  stream->curr_attr.weight = weight;
  stream->simp_attr = simplify_attributes (stream, stream->curr_attr);
}

static term_posture_t
term_ostream::get_posture (term_ostream_t stream)
{
  return stream->curr_attr.posture;
}

static void
term_ostream::set_posture (term_ostream_t stream, term_posture_t posture)
{
  stream->curr_attr.posture = posture;
  stream->simp_attr = simplify_attributes (stream, stream->curr_attr);
}

static term_underline_t
term_ostream::get_underline (term_ostream_t stream)
{
  return stream->curr_attr.underline;
}

static void
term_ostream::set_underline (term_ostream_t stream, term_underline_t underline)
{
  stream->curr_attr.underline = underline;
  stream->simp_attr = simplify_attributes (stream, stream->curr_attr);
}

/* Constructor.  */

static inline char *
xstrdup0 (const char *str)
{
  if (str == NULL)
    return NULL;
#if HAVE_TERMINFO
  if (str == (const char *)(-1))
    return NULL;
#endif
  return xstrdup (str);
}

term_ostream_t
term_ostream_create (int fd, const char *filename)
{
  term_ostream_t stream = XMALLOC (struct term_ostream_representation);
  const char *term;

  stream->base.vtable = &term_ostream_vtable;
  stream->fd = fd;
  stream->filename = xstrdup (filename);

  /* Defaults.  */
  stream->max_colors = -1;
  stream->no_color_video = -1;
  stream->set_a_foreground = NULL;
  stream->set_foreground = NULL;
  stream->set_a_background = NULL;
  stream->set_background = NULL;
  stream->orig_pair = NULL;
  stream->enter_bold_mode = NULL;
  stream->enter_italics_mode = NULL;
  stream->exit_italics_mode = NULL;
  stream->enter_underline_mode = NULL;
  stream->exit_underline_mode = NULL;
  stream->exit_attribute_mode = NULL;

  /* Retrieve the terminal type.  */
  term = getenv ("TERM");
  if (term != NULL && term[0] != '\0')
    {
      /* When the terminfo function are available, we prefer them over the
         termcap functions because
           1. they don't risk a buffer overflow,
           2. on OSF/1, for TERM=xterm, the tiget* functions provide access
              to the number of colors and the color escape sequences, whereas
              the tget* functions don't provide them.  */
#if HAVE_TERMINFO
      int err = 1;

      if (setupterm (term, fd, &err) || err == 1)
        {
          /* Retrieve particular values depending on the terminal type.  */
          stream->max_colors = tigetnum ("colors");
          stream->no_color_video = tigetnum ("ncv");
          stream->set_a_foreground = xstrdup0 (tigetstr ("setaf"));
          stream->set_foreground = xstrdup0 (tigetstr ("setf"));
          stream->set_a_background = xstrdup0 (tigetstr ("setab"));
          stream->set_background = xstrdup0 (tigetstr ("setb"));
          stream->orig_pair = xstrdup0 (tigetstr ("op"));
          stream->enter_bold_mode = xstrdup0 (tigetstr ("bold"));
          stream->enter_italics_mode = xstrdup0 (tigetstr ("sitm"));
          stream->exit_italics_mode = xstrdup0 (tigetstr ("ritm"));
          stream->enter_underline_mode = xstrdup0 (tigetstr ("smul"));
          stream->exit_underline_mode = xstrdup0 (tigetstr ("rmul"));
          stream->exit_attribute_mode = xstrdup0 (tigetstr ("sgr0"));
        }
#elif HAVE_TERMCAP
      struct { char buf[1024]; char canary[4]; } termcapbuf;
      int retval;

      /* Call tgetent, being defensive against buffer overflow.  */
      memcpy (termcapbuf.canary, "CnRy", 4);
      retval = tgetent (termcapbuf.buf, term);
      if (memcmp (termcapbuf.canary, "CnRy", 4) != 0)
        /* Buffer overflow!  */
        abort ();

      if (retval > 0)
        {
          struct { char buf[1024]; char canary[4]; } termentrybuf;
          char *termentryptr;

          /* Prepare for calling tgetstr, being defensive against buffer
             overflow.  ncurses' tgetstr() supports a second argument NULL,
             but NetBSD's tgetstr() doesn't.  */
          memcpy (termentrybuf.canary, "CnRz", 4);
          #define TEBP ((termentryptr = termentrybuf.buf), &termentryptr)

          /* Retrieve particular values depending on the terminal type.  */
          stream->max_colors = tgetnum ("Co");
          stream->no_color_video = tgetnum ("NC");
          stream->set_a_foreground = xstrdup0 (tgetstr ("AF", TEBP));
          stream->set_foreground = xstrdup0 (tgetstr ("Sf", TEBP));
          stream->set_a_background = xstrdup0 (tgetstr ("AB", TEBP));
          stream->set_background = xstrdup0 (tgetstr ("Sb", TEBP));
          stream->orig_pair = xstrdup0 (tgetstr ("op", TEBP));
          stream->enter_bold_mode = xstrdup0 (tgetstr ("md", TEBP));
          stream->enter_italics_mode = xstrdup0 (tgetstr ("ZH", TEBP));
          stream->exit_italics_mode = xstrdup0 (tgetstr ("ZR", TEBP));
          stream->enter_underline_mode = xstrdup0 (tgetstr ("us", TEBP));
          stream->exit_underline_mode = xstrdup0 (tgetstr ("ue", TEBP));
          stream->exit_attribute_mode = xstrdup0 (tgetstr ("me", TEBP));

# ifdef __BEOS__
          /* The BeOS termcap entry for "beterm" is broken: For "AF" and "AB"
             it contains balues in terminfo syntax but the system's tparam()
             function understands only the termcap syntax.  */
          if (stream->set_a_foreground != NULL
              && strcmp (stream->set_a_foreground, "\033[3%p1%dm") == 0)
            {
              free (stream->set_a_foreground);
              stream->set_a_foreground = xstrdup ("\033[3%dm");
            }
          if (stream->set_a_background != NULL
              && strcmp (stream->set_a_background, "\033[4%p1%dm") == 0)
            {
              free (stream->set_a_background);
              stream->set_a_background = xstrdup ("\033[4%dm");
            }
# endif

          /* The termcap entry for cygwin is broken: It has no "ncv" value,
             but bold and underline are actually rendered through colors.  */
          if (strcmp (term, "cygwin") == 0)
            stream->no_color_video |= 2 | 32;

          /* Done with tgetstr.  Detect possible buffer overflow.  */
          #undef TEBP
          if (memcmp (termentrybuf.canary, "CnRz", 4) != 0)
            /* Buffer overflow!  */
            abort ();
        }
#else
    /* Fallback code for platforms with neither the terminfo nor the termcap
       functions, such as mingw.
       Assume the ANSI escape sequences.  Extracted through
       "TERM=ansi infocmp", replacing \E with \033.  */
      stream->max_colors = 8;
      stream->no_color_video = 3;
      stream->set_a_foreground = xstrdup ("\033[3%p1%dm");
      stream->set_a_background = xstrdup ("\033[4%p1%dm");
      stream->orig_pair = xstrdup ("\033[39;49m");
      stream->enter_bold_mode = xstrdup ("\033[1m");
      stream->enter_underline_mode = xstrdup ("\033[4m");
      stream->exit_underline_mode = xstrdup ("\033[m");
      stream->exit_attribute_mode = xstrdup ("\033[0;10m");
#endif

      /* AIX 4.3.2, IRIX 6.5, HP-UX 11, Solaris 7..10 all lack the
         description of color capabilities of "xterm" and "xterms"
         in their terminfo database.  But it is important to have
         color in xterm.  So we provide the color capabilities here.  */
      if (stream->max_colors <= 1
          && (strcmp (term, "xterm") == 0 || strcmp (term, "xterms") == 0))
        {
          stream->max_colors = 8;
          stream->set_a_foreground = xstrdup ("\033[3%p1%dm");
          stream->set_a_background = xstrdup ("\033[4%p1%dm");
          stream->orig_pair = xstrdup ("\033[39;49m");
        }
    }

  /* Infer the capabilities.  */
  stream->supports_foreground =
    (stream->max_colors >= 8
     && (stream->set_a_foreground != NULL || stream->set_foreground != NULL)
     && stream->orig_pair != NULL);
  stream->supports_background =
    (stream->max_colors >= 8
     && (stream->set_a_background != NULL || stream->set_background != NULL)
     && stream->orig_pair != NULL);
  stream->colormodel =
    (stream->supports_foreground || stream->supports_background
     ? (term != NULL
        && (/* Recognize xterm-16color, xterm-88color, xterm-256color.  */
            (strlen (term) >= 5 && memcmp (term, "xterm", 5) == 0)
            || /* Recognize rxvt-16color.  */
               (strlen (term) >= 4 && memcmp (term, "rxvt", 7) == 0)
            || /* Recognize konsole-16color.  */
               (strlen (term) >= 7 && memcmp (term, "konsole", 7) == 0))
        ? (stream->max_colors == 256 ? cm_xterm256 :
           stream->max_colors == 88 ? cm_xterm88 :
           stream->max_colors == 16 ? cm_xterm16 :
           cm_xterm8)
        : cm_common8)
     : cm_monochrome);
  stream->supports_weight =
    (stream->enter_bold_mode != NULL && stream->exit_attribute_mode != NULL);
  stream->supports_posture =
    (stream->enter_italics_mode != NULL
     && (stream->exit_italics_mode != NULL
         || stream->exit_attribute_mode != NULL));
  stream->supports_underline =
    (stream->enter_underline_mode != NULL
     && (stream->exit_underline_mode != NULL
         || stream->exit_attribute_mode != NULL));

  /* Initialize the buffer.  */
  stream->allocated = 120;
  stream->buffer = XNMALLOC (stream->allocated, char);
  stream->attrbuffer = XNMALLOC (stream->allocated, attributes_t);
  stream->buflen = 0;

  /* Initialize the current attributes.  */
  stream->curr_attr.color = COLOR_DEFAULT;
  stream->curr_attr.bgcolor = COLOR_DEFAULT;
  stream->curr_attr.weight = WEIGHT_DEFAULT;
  stream->curr_attr.posture = POSTURE_DEFAULT;
  stream->curr_attr.underline = UNDERLINE_DEFAULT;
  stream->simp_attr = simplify_attributes (stream, stream->curr_attr);

  /* Register an exit handler.  */
  {
    static bool registered = false;
    if (!registered)
      {
        atexit (restore);
        registered = true;
      }
  }

  return stream;
}
