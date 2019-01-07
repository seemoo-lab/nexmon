
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <stdarg.h>
#include <fcntl.h>

#include <private/android_filesystem_config.h>

/* NOTES
**
** - see buffer-format.txt from the linux kernel docs for
**   an explanation of this file format
** - dotfiles are ignored
** - directories named 'root' are ignored
** - device notes, pipes, etc are not supported (error)
*/

void die(const char *why, ...)
{
    va_list ap;

    va_start(ap, why);
    fprintf(stderr,"error: ");
    vfprintf(stderr, why, ap);
    fprintf(stderr,"\n");
    va_end(ap);
    exit(1);
}

struct fs_config_entry {
    char* name;
    int uid, gid, mode;
};

static struct fs_config_entry* canned_config = NULL;
static char *target_out_path = NULL;

/* Each line in the canned file should be a path plus three ints (uid,
 * gid, mode). */
#ifdef PATH_MAX
#define CANNED_LINE_LENGTH  (PATH_MAX+100)
#else
#define CANNED_LINE_LENGTH  (1024)
#endif

static int verbose = 0;
static int total_size = 0;

static void fix_stat(const char *path, struct stat *s)
{
    uint64_t capabilities;
    if (canned_config) {
        // Use the list of file uid/gid/modes loaded from the file
        // given with -f.

        struct fs_config_entry* empty_path_config = NULL;
        struct fs_config_entry* p;
        for (p = canned_config; p->name; ++p) {
            if (!p->name[0]) {
                empty_path_config = p;
            }
            if (strcmp(p->name, path) == 0) {
                s->st_uid = p->uid;
                s->st_gid = p->gid;
                s->st_mode = p->mode | (s->st_mode & ~07777);
                return;
            }
        }
        s->st_uid = empty_path_config->uid;
        s->st_gid = empty_path_config->gid;
        s->st_mode = empty_path_config->mode | (s->st_mode & ~07777);
    } else {
        // Use the compiled-in fs_config() function.
        unsigned st_mode = s->st_mode;
        fs_config(path, S_ISDIR(s->st_mode), target_out_path,
                       &s->st_uid, &s->st_gid, &st_mode, &capabilities);
        s->st_mode = (typeof(s->st_mode)) st_mode;
    }
}

static void _eject(struct stat *s, char *out, int olen, char *data, unsigned datasize)
{
    // Nothing is special about this value, just picked something in the
    // approximate range that was being used already, and avoiding small
    // values which may be special.
    static unsigned next_inode = 300000;

    while(total_size & 3) {
        total_size++;
        putchar(0);
    }

    fix_stat(out, s);
//    fprintf(stderr, "_eject %s: mode=0%o\n", out, s->st_mode);

    printf("%06x%08x%08x%08x%08x%08x%08x"
           "%08x%08x%08x%08x%08x%08x%08x%s%c",
           0x070701,
           next_inode++,  //  s.st_ino,
           s->st_mode,
           0, // s.st_uid,
           0, // s.st_gid,
           1, // s.st_nlink,
           0, // s.st_mtime,
           datasize,
           0, // volmajor
           0, // volminor
           0, // devmajor
           0, // devminor,
           olen + 1,
           0,
           out,
           0
           );

    total_size += 6 + 8*13 + olen + 1;

    if(strlen(out) != (unsigned int)olen) die("ACK!");

    while(total_size & 3) {
        total_size++;
        putchar(0);
    }

    if(datasize) {
        fwrite(data, datasize, 1, stdout);
        total_size += datasize;
    }
}

static void _eject_trailer()
{
    struct stat s;
    memset(&s, 0, sizeof(s));
    _eject(&s, "TRAILER!!!", 10, 0, 0);

    while(total_size & 0xff) {
        total_size++;
        putchar(0);
    }
}

static void _archive(char *in, char *out, int ilen, int olen);

static int compare(const void* a, const void* b) {
  return strcmp(*(const char**)a, *(const char**)b);
}

static void _archive_dir(char *in, char *out, int ilen, int olen)
{
    int i, t;
    DIR *d;
    struct dirent *de;

    if(verbose) {
        fprintf(stderr,"_archive_dir('%s','%s',%d,%d)\n",
                in, out, ilen, olen);
    }

    d = opendir(in);
    if(d == 0) die("cannot open directory '%s'", in);

    int size = 32;
    int entries = 0;
    char** names = malloc(size * sizeof(char*));
    if (names == NULL) {
      fprintf(stderr, "failed to allocate dir names array (size %d)\n", size);
      exit(1);
    }

    while((de = readdir(d)) != 0){
            /* xxx: feature? maybe some dotfiles are okay */
        if(de->d_name[0] == '.') continue;

            /* xxx: hack. use a real exclude list */
        if(!strcmp(de->d_name, "root")) continue;

        if (entries >= size) {
          size *= 2;
          names = realloc(names, size * sizeof(char*));
          if (names == NULL) {
            fprintf(stderr, "failed to reallocate dir names array (size %d)\n",
                    size);
            exit(1);
          }
        }
        names[entries] = strdup(de->d_name);
        if (names[entries] == NULL) {
          fprintf(stderr, "failed to strdup name \"%s\"\n",
                  de->d_name);
          exit(1);
        }
        ++entries;
    }

    qsort(names, entries, sizeof(char*), compare);

    for (i = 0; i < entries; ++i) {
        t = strlen(names[i]);
        in[ilen] = '/';
        memcpy(in + ilen + 1, names[i], t + 1);

        if(olen > 0) {
            out[olen] = '/';
            memcpy(out + olen + 1, names[i], t + 1);
            _archive(in, out, ilen + t + 1, olen + t + 1);
        } else {
            memcpy(out, names[i], t + 1);
            _archive(in, out, ilen + t + 1, t);
        }

        in[ilen] = 0;
        out[olen] = 0;

        free(names[i]);
    }
    free(names);

    closedir(d);
}

static void _archive(char *in, char *out, int ilen, int olen)
{
    struct stat s;

    if(verbose) {
        fprintf(stderr,"_archive('%s','%s',%d,%d)\n",
                in, out, ilen, olen);
    }

    if(lstat(in, &s)) die("could not stat '%s'\n", in);

    if(S_ISREG(s.st_mode)){
        char *tmp;
        int fd;

        fd = open(in, O_RDONLY);
        if(fd < 0) die("cannot open '%s' for read", in);

        tmp = (char*) malloc(s.st_size);
        if(tmp == 0) die("cannot allocate %d bytes", s.st_size);

        if(read(fd, tmp, s.st_size) != s.st_size) {
            die("cannot read %d bytes", s.st_size);
        }

        _eject(&s, out, olen, tmp, s.st_size);

        free(tmp);
        close(fd);
    } else if(S_ISDIR(s.st_mode)) {
        _eject(&s, out, olen, 0, 0);
        _archive_dir(in, out, ilen, olen);
    } else if(S_ISLNK(s.st_mode)) {
        char buf[1024];
        int size;
        size = readlink(in, buf, 1024);
        if(size < 0) die("cannot read symlink '%s'", in);
        _eject(&s, out, olen, buf, size);
    } else {
        die("Unknown '%s' (mode %d)?\n", in, s.st_mode);
    }
}

void archive(const char *start, const char *prefix)
{
    char in[8192];
    char out[8192];

    strcpy(in, start);
    strcpy(out, prefix);

    _archive_dir(in, out, strlen(in), strlen(out));
}

static void read_canned_config(char* filename)
{
    int allocated = 8;
    int used = 0;

    canned_config =
        (struct fs_config_entry*)malloc(allocated * sizeof(struct fs_config_entry));

    char line[CANNED_LINE_LENGTH];
    FILE* f = fopen(filename, "r");
    if (f == NULL) die("failed to open canned file");

    while (fgets(line, CANNED_LINE_LENGTH, f) != NULL) {
        if (!line[0]) break;
        if (used >= allocated) {
            allocated *= 2;
            canned_config = (struct fs_config_entry*)realloc(
                canned_config, allocated * sizeof(struct fs_config_entry));
        }

        struct fs_config_entry* cc = canned_config + used;

        if (isspace(line[0])) {
            cc->name = strdup("");
            cc->uid = atoi(strtok(line, " \n"));
        } else {
            cc->name = strdup(strtok(line, " \n"));
            cc->uid = atoi(strtok(NULL, " \n"));
        }
        cc->gid = atoi(strtok(NULL, " \n"));
        cc->mode = strtol(strtok(NULL, " \n"), NULL, 8);
        ++used;
    }
    if (used >= allocated) {
        ++allocated;
        canned_config = (struct fs_config_entry*)realloc(
            canned_config, allocated * sizeof(struct fs_config_entry));
    }
    canned_config[used].name = NULL;

    fclose(f);
}


int main(int argc, char *argv[])
{
    argc--;
    argv++;

    if (argc > 1 && strcmp(argv[0], "-d") == 0) {
        target_out_path = argv[1];
        argc -= 2;
        argv += 2;
    }

    if (argc > 1 && strcmp(argv[0], "-f") == 0) {
        read_canned_config(argv[1]);
        argc -= 2;
        argv += 2;
    }

    if(argc == 0) die("no directories to process?!");

    while(argc-- > 0){
        char *x = strchr(*argv, '=');
        if(x != 0) {
            *x++ = 0;
        } else {
            x = "";
        }

        archive(*argv, x);

        argv++;
    }

    _eject_trailer();

    return 0;
}
