/*
 * xmlSeed.c: Generate the XML seed corpus for fuzzing.
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>
#include <string.h>
#include <glob.h>
#include <libgen.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/HTMLparser.h>
#include <libxml/xinclude.h>
#include <libxml/xmlschemas.h>
#include "fuzz.h"

#define PATH_SIZE 500
#define SEED_BUF_SIZE 16384
#define EXPR_SIZE 4500

#define FLAG_READER             (1 << 0)
#define FLAG_LINT               (1 << 1)
#define FLAG_PUSH_CHUNK_SIZE    (1 << 2)

typedef int
(*fileFunc)(const char *base, FILE *out);

typedef int
(*mainFunc)(const char *arg);

static struct {
    FILE *out;
    xmlHashTablePtr entities; /* Maps URLs to xmlFuzzEntityInfos */
    xmlExternalEntityLoader oldLoader;
    fileFunc processFile;
    const char *fuzzer;
    int counter;
    char cwd[PATH_SIZE];
    int flags;
} globalData;

#if defined(HAVE_SCHEMA_FUZZER) || \
    defined(HAVE_XML_FUZZER)
/*
 * A custom resource loader that writes all external DTDs or entities to a
 * single file in the format expected by xmlFuzzResourceLoader.
 */
static xmlParserErrors
fuzzResourceRecorder(void *data ATTRIBUTE_UNUSED, const char *URL,
                     const char *ID ATTRIBUTE_UNUSED,
                     xmlResourceType type ATTRIBUTE_UNUSED,
                     xmlParserInputFlags flags,
                     xmlParserInputPtr *out) {
    *out = NULL;

    if (globalData.entities == NULL ||
        xmlHashLookup(globalData.entities, BAD_CAST URL) == NULL) {
        data = xmlSlurpFile(URL, NULL);

        if (globalData.entities == NULL)
            globalData.entities = xmlHashCreate(4);

        xmlHashAddEntry(globalData.entities, (const xmlChar *) URL, data);
    }

    return(xmlNewInputFromUrl(URL, flags, out));
}

static void
fuzzRecorderInit(FILE *out) {
    globalData.out = out;
    globalData.entities = xmlHashCreate(8);
    globalData.oldLoader = xmlGetExternalEntityLoader();
}

static void
fuzzRecorderWriteAndFree(void *entry, const xmlChar *file) {
    char *data = entry;
    xmlFuzzWriteString(globalData.out, (const char *) file);
    xmlFuzzWriteString(globalData.out, data);
    xmlFree(data);
}

static void
fuzzRecorderWrite(const char *file) {
    xmlHashRemoveEntry(globalData.entities, (const xmlChar *) file,
                       fuzzRecorderWriteAndFree);
}

static void
fuzzRecorderCleanup(void) {
    /* Write remaining entities (in random order). */
    xmlHashFree(globalData.entities, fuzzRecorderWriteAndFree);
    globalData.out = NULL;
    globalData.entities = NULL;
    globalData.oldLoader = NULL;
}
#endif

#ifdef HAVE_XML_FUZZER
static int
processXml(const char *docFile, FILE *out) {
    int opts = XML_PARSE_NOENT | XML_PARSE_DTDLOAD;
    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;

    if (globalData.flags & FLAG_LINT) {
        /* Switches */
        xmlFuzzWriteInt(out, 0, 4);
        xmlFuzzWriteInt(out, 0, 4);
        /* maxmem */
        xmlFuzzWriteInt(out, 0, 4);
        /* max-ampl */
        xmlFuzzWriteInt(out, 0, 1);
        /* pretty */
        xmlFuzzWriteInt(out, 0, 1);
        /* encode */
        xmlFuzzWriteString(out, "");
        /* pattern */
        xmlFuzzWriteString(out, "");
        /* xpath */
        xmlFuzzWriteString(out, "");
    } else {
        /* Parser options. */
        xmlFuzzWriteInt(out, opts, 4);
        /* Max allocations. */
        xmlFuzzWriteInt(out, 0, 4);

        if (globalData.flags & FLAG_PUSH_CHUNK_SIZE) {
            /* Chunk size for push parser */
            xmlFuzzWriteInt(out, 256, 4);
        }

        if (globalData.flags & FLAG_READER) {
            /* Initial reader program with a couple of OP_READs */
            xmlFuzzWriteString(out, "\x01\x01\x01\x01\x01\x01\x01\x01");
        }
    }

    fuzzRecorderInit(out);

    ctxt = xmlNewParserCtxt();
    xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
    xmlCtxtSetResourceLoader(ctxt, fuzzResourceRecorder, NULL);
    doc = xmlCtxtReadFile(ctxt, docFile, NULL, opts);
#ifdef LIBXML_XINCLUDE_ENABLED
    {
        xmlXIncludeCtxtPtr xinc = xmlXIncludeNewContext(doc);

        xmlXIncludeSetErrorHandler(xinc, xmlFuzzSErrorFunc, NULL);
        xmlXIncludeSetResourceLoader(xinc, fuzzResourceRecorder, NULL);
        xmlXIncludeSetFlags(xinc, opts);
        xmlXIncludeProcessNode(xinc, (xmlNodePtr) doc);
        xmlXIncludeFreeContext(xinc);
    }
#endif
    xmlFreeDoc(doc);
    xmlFreeParserCtxt(ctxt);

    fuzzRecorderWrite(docFile);
    fuzzRecorderCleanup();

    return(0);
}
#endif

#ifdef HAVE_HTML_FUZZER
static int
processHtml(const char *docFile, FILE *out) {
    char buf[SEED_BUF_SIZE];
    FILE *file;
    size_t size;

    /* Parser options. */
    xmlFuzzWriteInt(out, 0, 4);
    /* Max allocations. */
    xmlFuzzWriteInt(out, 0, 4);

    /* Copy file */
    file = fopen(docFile, "rb");
    if (file == NULL) {
        fprintf(stderr, "couldn't open %s\n", docFile);
        return(0);
    }
    do {
        size = fread(buf, 1, SEED_BUF_SIZE, file);
        if (size > 0)
            fwrite(buf, 1, size, out);
    } while (size == SEED_BUF_SIZE);
    fclose(file);

    return(0);
}
#endif

#if defined(HAVE_HTML_FUZZER) || \
    defined(HAVE_XML_FUZZER)
static int
processPattern(const char *pattern) {
    glob_t globbuf;
    int ret = 0;
    int res;
    size_t i;

    res = glob(pattern, 0, NULL, &globbuf);
    if (res == GLOB_NOMATCH)
        return(0);
    if (res != 0) {
        fprintf(stderr, "couldn't match pattern %s\n", pattern);
        return(-1);
    }

    for (i = 0; i < globbuf.gl_pathc; i++) {
        struct stat statbuf;
        char outPath[PATH_SIZE];
        char *dirBuf = NULL;
        char *baseBuf = NULL;
        const char *path, *dir, *base;
        FILE *out = NULL;
        int dirChanged = 0;
        size_t size;

        path = globbuf.gl_pathv[i];

        if ((stat(path, &statbuf) != 0) || (!S_ISREG(statbuf.st_mode)))
            continue;

        dirBuf = (char *) xmlCharStrdup(path);
        baseBuf = (char *) xmlCharStrdup(path);
        if ((dirBuf == NULL) || (baseBuf == NULL)) {
            fprintf(stderr, "memory allocation failed\n");
            ret = -1;
            goto error;
        }
        dir = dirname(dirBuf);
        base = basename(baseBuf);

        size = snprintf(outPath, sizeof(outPath), "seed/%s/%s",
                        globalData.fuzzer, base);
        if (size >= PATH_SIZE) {
            fprintf(stderr, "creating path failed\n");
            ret = -1;
            goto error;
        }
        out = fopen(outPath, "wb");
        if (out == NULL) {
            fprintf(stderr, "couldn't open %s for writing\n", outPath);
            ret = -1;
            goto error;
        }
        if (chdir(dir) != 0) {
            fprintf(stderr, "couldn't chdir to %s\n", dir);
            ret = -1;
            goto error;
        }
        dirChanged = 1;
        if (globalData.processFile(base, out) != 0)
            ret = -1;

error:
        if (out != NULL)
            fclose(out);
        xmlFree(dirBuf);
        xmlFree(baseBuf);
        if ((dirChanged) && (chdir(globalData.cwd) != 0)) {
            fprintf(stderr, "couldn't chdir to %s\n", globalData.cwd);
            ret = -1;
            break;
        }
    }

    globfree(&globbuf);
    return(ret);
}
#endif

#if defined(HAVE_SCHEMA_FUZZER)
static int
processSchema(const char *xsdFile, const char *xmlFile, FILE *out) {
    xmlSchemaPtr schema;
    xmlSchemaParserCtxtPtr pctxt;

    /* Max allocations. */
    xmlFuzzWriteInt(out, 0, 4);

    fuzzRecorderInit(out);

    pctxt = xmlSchemaNewParserCtxt(xsdFile);
    xmlSchemaSetParserStructuredErrors(pctxt, xmlFuzzSErrorFunc, NULL);
    xmlSchemaSetResourceLoader(pctxt, fuzzResourceRecorder, NULL);
    schema = xmlSchemaParse(pctxt);
    xmlSchemaFreeParserCtxt(pctxt);

    if (schema != NULL) {
        xmlSchemaValidCtxtPtr vctxt;
        xmlParserCtxtPtr ctxt;
        xmlDocPtr doc;

        ctxt = xmlNewParserCtxt();
        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        xmlCtxtSetResourceLoader(ctxt, fuzzResourceRecorder, NULL);
        doc = xmlCtxtReadFile(ctxt, xmlFile, NULL, XML_PARSE_NOENT);
        xmlFreeParserCtxt(ctxt);

        vctxt = xmlSchemaNewValidCtxt(schema);
        xmlSchemaSetValidStructuredErrors(vctxt, xmlFuzzSErrorFunc, NULL);
        xmlSchemaValidateDoc(vctxt, doc);
        xmlSchemaFreeValidCtxt(vctxt);

        xmlFreeDoc(doc);
        xmlSchemaFree(schema);
    }

    fuzzRecorderWrite(xsdFile);
    fuzzRecorderWrite(xmlFile);
    fuzzRecorderCleanup();

    return(0);
}

static int
processSchemaPattern(const char *pattern) {
    glob_t globbuf;
    int ret = 0;
    int res;
    size_t i;

    res = glob(pattern, 0, NULL, &globbuf);
    if (res == GLOB_NOMATCH)
        return(0);
    if (res != 0) {
        fprintf(stderr, "couldn't match pattern %s\n", pattern);
        return(-1);
    }

    for (i = 0; i < globbuf.gl_pathc; i++) {
        glob_t globbuf2;
        struct stat statbuf;
        char xmlPattern[PATH_SIZE];
        char *dirBuf = NULL;
        char *baseBuf = NULL;
        const char *path, *dir, *base;
        size_t size, dirLen, baseLen, len, j;

        path = globbuf.gl_pathv[i];

        if ((stat(path, &statbuf) != 0) || (!S_ISREG(statbuf.st_mode)))
            continue;

        dirBuf = (char *) xmlCharStrdup(path);
        baseBuf = (char *) xmlCharStrdup(path);
        if ((dirBuf == NULL) || (baseBuf == NULL)) {
            fprintf(stderr, "memory allocation failed\n");
            ret = -1;
            goto error;
        }
        dir = dirname(dirBuf);
        dirLen = strlen(dir);
        base = basename(baseBuf);
        baseLen = strlen(base);

        len = strlen(path);
        if (len <= 5)
            continue;
        /* Strip .xsl or _0.xsd suffix */
        if (len > 6 && path[len - 6] == '_')
            len -= 6;
        else
            len -= 4;
        size = snprintf(xmlPattern, sizeof(xmlPattern), "%.*s_*.xml",
                        (int) len, path);
        if (size >= PATH_SIZE) {
            fprintf(stderr, "creating path failed\n");
            ret = -1;
            goto error;
        }

        res = glob(xmlPattern, 0, NULL, &globbuf2);
        if (res == GLOB_NOMATCH)
            goto error;
        if (res != 0) {
            fprintf(stderr, "couldn't match pattern %s\n", xmlPattern);
            ret = -1;
            goto error;
        }

        for (j = 0; j < globbuf2.gl_pathc; j++) {
            char outPath[PATH_SIZE];
            const char *xmlFile;
            FILE *out = NULL;

            xmlFile = globbuf2.gl_pathv[j];

            len = strlen(xmlFile);
            if (len < dirLen + 7)
                continue;
            if (len >= 6 && xmlFile[len - 6] == '_')
                size = snprintf(outPath, sizeof(outPath), "seed/%s/%.*s_%c",
                                globalData.fuzzer, (int) baseLen - 4, base,
                                xmlFile[len - 5]);
            else
                size = snprintf(outPath, sizeof(outPath), "seed/%s/%.*s",
                                globalData.fuzzer, (int) baseLen - 4, base);

            if (size >= PATH_SIZE) {
                fprintf(stderr, "creating path failed\n");
                ret = -1;
                continue;
            }
            out = fopen(outPath, "wb");
            if (out == NULL) {
                fprintf(stderr, "couldn't open %s for writing\n", outPath);
                ret = -1;
                continue;
            }

            if (chdir(dir) != 0) {
                fprintf(stderr, "couldn't chdir to %s\n", dir);
                ret = -1;
            } else {
                if (processSchema(base, xmlFile + dirLen + 1, out) != 0)
                    ret = -1;
            }

            fclose(out);

            if (chdir(globalData.cwd) != 0) {
                fprintf(stderr, "couldn't chdir to %s\n", globalData.cwd);
                ret = -1;
                break;
            }
        }

        globfree(&globbuf2);

error:
        xmlFree(dirBuf);
        xmlFree(baseBuf);
    }

    globfree(&globbuf);
    return(ret);
}
#endif

#ifdef HAVE_XPATH_FUZZER
static int
processXPath(const char *testDir, const char *prefix, const char *name,
             const char *data, const char *subdir, int xptr) {
    char pattern[PATH_SIZE];
    glob_t globbuf;
    size_t i, size;
    int ret = 0, res;

    size = snprintf(pattern, sizeof(pattern), "%s/%s/%s*",
                    testDir, subdir, prefix);
    if (size >= PATH_SIZE)
        return(-1);
    res = glob(pattern, 0, NULL, &globbuf);
    if (res == GLOB_NOMATCH)
        return(0);
    if (res != 0) {
        fprintf(stderr, "couldn't match pattern %s\n", pattern);
        return(-1);
    }

    for (i = 0; i < globbuf.gl_pathc; i++) {
        char *path = globbuf.gl_pathv[i];
        struct stat statbuf;
        FILE *in;
        char expr[EXPR_SIZE];

        if ((stat(path, &statbuf) != 0) || (!S_ISREG(statbuf.st_mode)))
            continue;

        in = fopen(path, "rb");
        if (in == NULL) {
            ret = -1;
            continue;
        }

        while (fgets(expr, EXPR_SIZE, in) != NULL) {
            char outPath[PATH_SIZE];
            FILE *out;
            int j;

            for (j = 0; expr[j] != 0; j++)
                if (expr[j] == '\r' || expr[j] == '\n')
                    break;
            expr[j] = 0;

            size = snprintf(outPath, sizeof(outPath), "seed/xpath/%s-%d",
                            name, globalData.counter);
            if (size >= PATH_SIZE) {
                ret = -1;
                continue;
            }
            out = fopen(outPath, "wb");
            if (out == NULL) {
                ret = -1;
                continue;
            }

            /* Max allocations. */
            xmlFuzzWriteInt(out, 0, 4);

            if (xptr) {
                xmlFuzzWriteString(out, expr);
            } else {
                char xptrExpr[EXPR_SIZE+100];

                /* Wrap XPath expressions as XPointer */
                snprintf(xptrExpr, sizeof(xptrExpr), "xpointer(%s)", expr);
                xmlFuzzWriteString(out, xptrExpr);
            }

            xmlFuzzWriteString(out, data);

            fclose(out);
            globalData.counter++;
        }

        fclose(in);
    }

    globfree(&globbuf);

    return(ret);
}

static int
processXPathDir(const char *testDir) {
    char pattern[PATH_SIZE];
    glob_t globbuf;
    size_t i, size;
    int ret = 0;

    globalData.counter = 1;
    if (processXPath(testDir, "", "expr", "<d></d>", "expr", 0) != 0)
        ret = -1;

    size = snprintf(pattern, sizeof(pattern), "%s/docs/*", testDir);
    if (size >= PATH_SIZE)
        return(1);
    if (glob(pattern, 0, NULL, &globbuf) != 0)
        return(1);

    for (i = 0; i < globbuf.gl_pathc; i++) {
        char *path = globbuf.gl_pathv[i];
        char *data;
        const char *docFile;

        data = xmlSlurpFile(path, NULL);
        if (data == NULL) {
            ret = -1;
            continue;
        }
        docFile = basename(path);

        globalData.counter = 1;
        if (processXPath(testDir, docFile, docFile, data, "tests", 0) != 0)
            ret = -1;
        if (processXPath(testDir, docFile, docFile, data, "xptr", 1) != 0)
            ret = -1;
        if (processXPath(testDir, docFile, docFile, data, "xptr-xp1", 1) != 0)
            ret = -1;

        xmlFree(data);
    }

    globfree(&globbuf);

    return(ret);
}
#endif

int
main(int argc, const char **argv) {
    mainFunc processArg = NULL;
    const char *fuzzer;
    int ret = 0;
    int i;

    if (argc < 3) {
        fprintf(stderr, "usage: seed [FUZZER] [PATTERN...]\n");
        return(1);
    }

    fuzzer = argv[1];
    if (strcmp(fuzzer, "html") == 0) {
#ifdef HAVE_HTML_FUZZER
        processArg = processPattern;
        globalData.flags |= FLAG_PUSH_CHUNK_SIZE;
        globalData.processFile = processHtml;
#endif
    } else if (strcmp(fuzzer, "lint") == 0) {
#ifdef HAVE_LINT_FUZZER
        processArg = processPattern;
        globalData.flags |= FLAG_LINT;
        globalData.processFile = processXml;
#endif
    } else if (strcmp(fuzzer, "reader") == 0) {
#ifdef HAVE_READER_FUZZER
        processArg = processPattern;
        globalData.flags |= FLAG_READER;
        globalData.processFile = processXml;
#endif
    } else if (strcmp(fuzzer, "schema") == 0) {
#ifdef HAVE_SCHEMA_FUZZER
        processArg = processSchemaPattern;
#endif
    } else if (strcmp(fuzzer, "valid") == 0) {
#ifdef HAVE_VALID_FUZZER
        processArg = processPattern;
        globalData.processFile = processXml;
#endif
    } else if (strcmp(fuzzer, "xinclude") == 0) {
#ifdef HAVE_XINCLUDE_FUZZER
        processArg = processPattern;
        globalData.processFile = processXml;
#endif
    } else if (strcmp(fuzzer, "xml") == 0) {
#ifdef HAVE_XML_FUZZER
        processArg = processPattern;
        globalData.flags |= FLAG_PUSH_CHUNK_SIZE;
        globalData.processFile = processXml;
#endif
    } else if (strcmp(fuzzer, "xpath") == 0) {
#ifdef HAVE_XPATH_FUZZER
        processArg = processXPathDir;
#endif
    } else {
        fprintf(stderr, "unknown fuzzer %s\n", fuzzer);
        return(1);
    }
    globalData.fuzzer = fuzzer;

    if (getcwd(globalData.cwd, PATH_SIZE) == NULL) {
        fprintf(stderr, "couldn't get current directory\n");
        return(1);
    }

    if (processArg != NULL)
        for (i = 2; i < argc; i++)
            processArg(argv[i]);

    return(ret);
}

