/* adapted from xdg-user-dir-lookup.c
 *
 * Copyright (C) 2007 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
static void
load_user_special_dirs_from_string (const gchar *string, const gchar *home_dir, gchar **special_dirs)
{
  gchar **lines;
  gint n_lines, i;
  size_t min_len;

  lines = g_strsplit (string, "\n", -1);
  n_lines = g_strv_length (lines);

  for (i = 0; i < n_lines; i++)
    {
      gchar *buffer = lines[i];
      gchar *d, *p;
      size_t len;
      gboolean is_relative = FALSE;
      GUserDirectory directory;

      /* Remove newline at end */
      len = strlen (buffer);
      if (len > 0 && buffer[len - 1] == '\n')
        buffer[len - 1] = 0;

      p = buffer;
      while (*p == ' ' || *p == '\t')
        p++;

      if (strncmp (p, "XDG_DESKTOP_DIR", strlen ("XDG_DESKTOP_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DESKTOP;
          p += strlen ("XDG_DESKTOP_DIR");
        }
      else if (strncmp (p, "XDG_DOCUMENTS_DIR", strlen ("XDG_DOCUMENTS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DOCUMENTS;
          p += strlen ("XDG_DOCUMENTS_DIR");
        }
      else if (strncmp (p, "XDG_DOWNLOAD_DIR", strlen ("XDG_DOWNLOAD_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_DOWNLOAD;
          p += strlen ("XDG_DOWNLOAD_DIR");
        }
      else if (strncmp (p, "XDG_MUSIC_DIR", strlen ("XDG_MUSIC_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_MUSIC;
          p += strlen ("XDG_MUSIC_DIR");
        }
      else if (strncmp (p, "XDG_PICTURES_DIR", strlen ("XDG_PICTURES_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PICTURES;
          p += strlen ("XDG_PICTURES_DIR");
        }
      else if (strncmp (p, "XDG_PUBLICSHARE_DIR", strlen ("XDG_PUBLICSHARE_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PUBLIC_SHARE;
          p += strlen ("XDG_PUBLICSHARE_DIR");
        }
      else if (strncmp (p, "XDG_TEMPLATES_DIR", strlen ("XDG_TEMPLATES_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_TEMPLATES;
          p += strlen ("XDG_TEMPLATES_DIR");
        }
      else if (strncmp (p, "XDG_VIDEOS_DIR", strlen ("XDG_VIDEOS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_VIDEOS;
          p += strlen ("XDG_VIDEOS_DIR");
        }
      else if (strncmp (p, "XDG_PROJECTS_DIR", strlen ("XDG_PROJECTS_DIR")) == 0)
        {
          directory = G_USER_DIRECTORY_PROJECTS;
          p += strlen ("XDG_PROJECTS_DIR");
        }
      else
        continue;

      while (*p == ' ' || *p == '\t')
        p++;

      if (*p != '=')
        continue;
      p++;

      while (*p == ' ' || *p == '\t')
        p++;

      if (*p != '"')
        continue;
      p++;

      if (strncmp (p, "$HOME", 5) == 0)
        {
          p += 5;
          if (*p != '/' && *p != '"')
            continue;
          is_relative = TRUE;
        }
      else if (*p != '/')
        continue;

      d = strrchr (p, '"');
      if (d < p)
        continue;
      *d = 0;

      d = p;

      /* remove trailing slashes, but keep first slash in absolute path */
      min_len = is_relative ? 0 : 1;
      for (len = strlen (d); len > min_len && d[len - 1] == '/'; len--)
        d[len - 1] = 0;

      /* Duplicates override the previous value. This is not explicit in the
       * spec, but given that the spec[1] is designed to allow user-dirs.dirs to
       * be sourced in a shell, overriding is the behaviour that would imply.
       *
       * [1]: https://www.freedesktop.org/wiki/Software/xdg-user-dirs/ */
      g_clear_pointer (&special_dirs[directory], g_free);

      if (is_relative)
        special_dirs[directory] = g_build_filename (home_dir, d, NULL);
      else
        special_dirs[directory] = g_strdup (d);
    }

  g_strfreev (lines);
}
