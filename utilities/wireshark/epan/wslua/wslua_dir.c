/*
 *  wslua_dir.c
 *
 * (c) 2014, Hadriel Kaplan <hadrielk at yahoo dot com>
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

/* WSLUA_MODULE Dir Directory handling functions */

#include "wslua.h"
#include <wsutil/file_util.h>

WSLUA_CLASS_DEFINE(Dir,FAIL_ON_NULL("Dir")); /* A Directory object, as well as associated functions. */

WSLUA_CONSTRUCTOR Dir_make(lua_State* L) {
    /* Creates a directory.

       The created directory is set for permission mode 0755 (octal), meaning it is
       read+write+execute by owner, but only read+execute by group and others.

       IF the directory was created successfully, a boolean `true` is returned.
       If the directory cannot be made because it already exists, `false` is returned.
       If the directory cannot be made because an error occurred, `nil` is returned.

       @since 1.11.3
    */
#define WSLUA_ARG_Dir_make_NAME 1 /* The name of the directory, possibly including path. */

    const char *dir_path = luaL_checkstring(L, WSLUA_ARG_Dir_make_NAME);
    ws_statb64 s_buf;
    int ret;

    if (ws_stat64(dir_path, &s_buf) != 0 && errno == ENOENT) {
        ret = ws_mkdir(dir_path, 0755);
        if (ret == -1) {
            lua_pushnil(L);
        } else {
            lua_pushboolean(L, 1);
        }
    } else {
        lua_pushboolean(L, 0);
    }

    WSLUA_RETURN(1); /* Boolean `true` on success, `false` if already exists, `nil` on error. */
}

WSLUA_CONSTRUCTOR Dir_exists(lua_State* L) {
    /* Returns true if the given directory name exists.

       If the directory exists, a boolean `true` is returned.
       If the path is a file instead, `false` is returned.
       If the path does not exist or an error occurred, `nil` is returned.

       @since 1.11.3
    */
#define WSLUA_ARG_Dir_exists_NAME 1 /* The name of the directory, possibly including path. */

    const char *dir_path = luaL_checkstring(L, WSLUA_ARG_Dir_exists_NAME);
    int ret;

    if ((ret = test_for_directory (dir_path)) == EISDIR) {
        lua_pushboolean(L, 1);
    } else {
        if (ret == 0) {
            lua_pushboolean(L, 0);
        } else {
            lua_pushnil(L);
        }
    }

    WSLUA_RETURN(1); /* Boolean `true` if the directory exists, `false` if it's a file, `nil` on error/not-exist. */
}

WSLUA_CONSTRUCTOR Dir_remove(lua_State* L) {
    /* Removes an empty directory.

       If the directory was removed successfully, a boolean `true` is returned.
       If the directory cannot be removed because it does not exist, `false` is returned.
       If the directory cannot be removed because an error occurred, `nil` is returned.

       This function only removes empty directories. To remove a directory regardless,
       use `Dir.remove_all()`.

       @since 1.11.3
    */
#define WSLUA_ARG_Dir_remove_NAME 1 /* The name of the directory, possibly including path. */

    const char *dir_path = luaL_checkstring(L, WSLUA_ARG_Dir_remove_NAME);
    int ret;

    if (test_for_directory (dir_path) == EISDIR) {
        ret = ws_remove(dir_path);
        if (ret != 0) {
            lua_pushnil(L);
        } else {
            lua_pushboolean(L, 1);
        }
    } else {
        lua_pushboolean(L, 0);
    }

    WSLUA_RETURN(1); /* Boolean `true` on success, `false` if does not exist, `nil` on error. */
}

static int delete_directory(const char *directory) {
    WS_DIR *dir;
    WS_DIRENT *file;
    gchar *filename;
    int ret = 0;

    /* delete all contents of directory */
    if ((dir = ws_dir_open(directory, 0, NULL)) != NULL) {
        while ((file = ws_dir_read_name(dir)) != NULL) {
            filename = g_strdup_printf ("%s%s%s", directory, G_DIR_SEPARATOR_S,
                            ws_dir_get_name(file));
            if (test_for_directory(filename) != EISDIR) {
                ret = ws_remove(filename);
            } else {
                /* recurse */
                ret = delete_directory (filename);
            }
            if (ret != 0) {
                break;
            }
            g_free (filename);
        }
        ws_dir_close(dir);
    }

    if (ret == 0) {
        ret = ws_remove(directory);
    }

    return ret;
}


WSLUA_CONSTRUCTOR Dir_remove_all(lua_State* L) {
    /* Removes an empty or non-empty directory.

       If the directory was removed successfully, a boolean `true` is returned.
       If the directory cannot be removed because it does not exist, `false` is returned.
       If the directory cannot be removed because an error occurred, `nil` is returned.

       @since 1.11.3
    */
#define WSLUA_ARG_Dir_remove_all_NAME 1 /* The name of the directory, possibly including path. */

    const char *dir_path = luaL_checkstring(L, WSLUA_ARG_Dir_remove_all_NAME);
    int ret;

    if (test_for_directory (dir_path) == EISDIR) {
        ret = delete_directory(dir_path);
        if (ret != 0) {
            lua_pushnil(L);
        } else {
            lua_pushboolean(L, 1);
        }
    } else {
        lua_pushboolean(L, 0);
    }

    WSLUA_RETURN(1); /* Boolean `true` on success, `false` if does not exist, `nil` on error. */
}

WSLUA_CONSTRUCTOR Dir_open(lua_State* L) {
    /* Opens a directory and returns a `Dir` object representing the files in the directory.

       @code for filename in Dir.open(path) do ... end @endcode
    */
#define WSLUA_ARG_Dir_open_PATHNAME 1 /* The pathname of the directory. */
#define WSLUA_OPTARG_Dir_open_EXTENSION 2 /* If given, only files with this extension will be returned. */

    const char* dirname = luaL_checkstring(L,WSLUA_ARG_Dir_open_PATHNAME);
    const char* extension = luaL_optstring(L,WSLUA_OPTARG_Dir_open_EXTENSION,NULL);
    Dir dir;
    char* dirname_clean;

    dirname_clean = wslua_get_actual_filename(dirname);
    if (!dirname_clean) {
        WSLUA_ARG_ERROR(Dir_open,PATHNAME,"directory does not exist");
        return 0;
    }

    if (!test_for_directory(dirname_clean))  {
        g_free(dirname_clean);
        WSLUA_ARG_ERROR(Dir_open,PATHNAME, "must be a directory");
        return 0;
    }

    dir = (Dir)g_malloc(sizeof(struct _wslua_dir));
    dir->dir = g_dir_open(dirname_clean, 0, dir->dummy);
    g_free(dirname_clean);
    dir->ext = extension ? g_strdup(extension) : NULL;
    dir->dummy = (GError **)g_malloc(sizeof(GError *));
    *(dir->dummy) = NULL;

    if (dir->dir == NULL) {
        g_free(dir->dummy);
        g_free(dir);

        WSLUA_ARG_ERROR(Dir_open,PATHNAME,"could not open directory");
        return 0;
    }

    pushDir(L,dir);
    WSLUA_RETURN(1); /* the `Dir` object. */
}

WSLUA_METAMETHOD Dir__call(lua_State* L) {
    /* At every invocation will return one file (nil when done). */

    Dir dir = checkDir(L,1);
    const gchar* file;
    const gchar* filename;
    const char* ext;

    if (!dir->dir) {
        return 0;
    }

    if ( ! ( file = g_dir_read_name(dir->dir ) )) {
        g_dir_close(dir->dir);
        dir->dir = NULL;
        return 0;
    }


    if ( ! dir->ext ) {
        lua_pushstring(L,file);
        return 1;
    }

    do {
        filename = file;

        /* XXX strstr returns ptr to first match,
            this fails ext=".xxx" filename="aaa.xxxz.xxx"  */
        if ( ( ext = strstr(filename,dir->ext)) && g_str_equal(ext,dir->ext) ) {
            lua_pushstring(L,filename);
            return 1;
        }
    } while(( file = g_dir_read_name(dir->dir) ));

    g_dir_close(dir->dir);
    dir->dir = NULL;
    return 0;
}

WSLUA_METHOD Dir_close(lua_State* L) {
    /* Closes the directory. */
    Dir dir = checkDir(L,1);

    if (dir->dir) {
        g_dir_close(dir->dir);
        dir->dir = NULL;
    }

    return 0;
}

WSLUA_CONSTRUCTOR Dir_personal_config_path(lua_State* L) {
    /* Gets the personal configuration directory path, with filename if supplied.

       @since 1.11.3
    */
#define WSLUA_OPTARG_personal_config_path_FILENAME 1 /* A filename. */
    const char *fname = luaL_optstring(L, WSLUA_OPTARG_personal_config_path_FILENAME,"");
    char* filename = get_persconffile_path(fname,FALSE);

    lua_pushstring(L,filename);
    g_free(filename);
    WSLUA_RETURN(1); /* The full pathname for a file in the personal configuration directory. */
}

WSLUA_CONSTRUCTOR Dir_global_config_path(lua_State* L) {
    /* Gets the global configuration directory path, with filename if supplied.

       @since 1.11.3
    */
#define WSLUA_OPTARG_global_config_path_FILENAME 1 /* A filename */
    const char *fname = luaL_optstring(L, WSLUA_OPTARG_global_config_path_FILENAME,"");
    char* filename;

    if (running_in_build_directory()) {
        /* Running in build directory, try the source directory (Autotools) */
        filename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "epan" G_DIR_SEPARATOR_S "wslua"
                                   G_DIR_SEPARATOR_S "%s", get_datafile_dir(), fname);
        if (( ! file_exists(filename))) {
            /* Try the CMake output directory */
            g_free(filename);
            filename = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s",
                                       get_progfile_dir(), fname);
        }
    } else {
        filename = get_datafile_path(fname);
    }

    lua_pushstring(L,filename);
    g_free(filename);
    WSLUA_RETURN(1); /* The full pathname for a file in wireshark's configuration directory. */
}

WSLUA_CONSTRUCTOR Dir_personal_plugins_path(lua_State* L) {
    /* Gets the personal plugins directory path.

       @since 1.11.3
    */
    char* filename = get_plugins_pers_dir();
    lua_pushstring(L,filename);
    g_free(filename);
    WSLUA_RETURN(1); /* The pathname for the personal plugins directory. */
}

WSLUA_CONSTRUCTOR Dir_global_plugins_path(lua_State* L) {
    /* Gets the global plugins directory path.

       @since 1.11.3
    */
    lua_pushstring(L, get_plugin_dir());
    WSLUA_RETURN(1); /* The pathname for the global plugins directory. */
}

/* Gets registered as metamethod automatically by WSLUA_REGISTER_CLASS/META */
static int Dir__gc(lua_State* L) {
    Dir dir = toDir(L,1);

    if(!dir) return 0;

    if (dir->dir) {
        g_dir_close(dir->dir);
    }

    g_free(dir->dummy);

    if (dir->ext) g_free(dir->ext);

    g_free(dir);

    return 0;
}

WSLUA_METHODS Dir_methods[] = {
    WSLUA_CLASS_FNREG(Dir,make),
    WSLUA_CLASS_FNREG(Dir,exists),
    WSLUA_CLASS_FNREG(Dir,remove),
    WSLUA_CLASS_FNREG(Dir,remove_all),
    WSLUA_CLASS_FNREG(Dir,open),
    WSLUA_CLASS_FNREG(Dir,close),
    WSLUA_CLASS_FNREG(Dir,personal_config_path),
    WSLUA_CLASS_FNREG(Dir,global_config_path),
    WSLUA_CLASS_FNREG(Dir,personal_plugins_path),
    WSLUA_CLASS_FNREG(Dir,global_plugins_path),
    { NULL, NULL }
};

WSLUA_META Dir_meta[] = {
    WSLUA_CLASS_MTREG(Dir,call),
    { NULL, NULL }
};

int Dir_register(lua_State* L) {
    WSLUA_REGISTER_CLASS(Dir);
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
