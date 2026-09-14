/*
 * testFuzzer.c: Test program for the custom entity loader used to fuzz
 * with multiple inputs.
 *
 * See Copyright for the status of this software.
 */

#ifndef XML_DEPRECATED
  #define XML_DEPRECATED
#endif

#include <string.h>
#include <glob.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include "fuzz.h"

size_t
LLVMFuzzerMutate(char *data, size_t size, size_t maxSize) {
    (void) data;
    (void) maxSize;

    return size;
}

#ifdef HAVE_HTML_FUZZER
int fuzzHtmlInit(int *argc, char ***argv);
int fuzzHtml(const char *data, size_t size);
size_t fuzzHtmlMutate(char *data, size_t size, size_t maxSize,
                      unsigned seed);
#define LLVMFuzzerInitialize fuzzHtmlInit
#define LLVMFuzzerTestOneInput fuzzHtml
#define LLVMFuzzerCustomMutator fuzzHtmlMutate
#include "html.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_READER_FUZZER
int fuzzReaderInit(int *argc, char ***argv);
int fuzzReader(const char *data, size_t size);
size_t fuzzReaderMutate(char *data, size_t size, size_t maxSize,
                        unsigned seed);
#define LLVMFuzzerInitialize fuzzReaderInit
#define LLVMFuzzerTestOneInput fuzzReader
#define LLVMFuzzerCustomMutator fuzzReaderMutate
#include "reader.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_REGEXP_FUZZER
int fuzzRegexpInit(int *argc, char ***argv);
int fuzzRegexp(const char *data, size_t size);
size_t fuzzRegexpMutate(char *data, size_t size, size_t maxSize,
                        unsigned seed);
#define LLVMFuzzerInitialize fuzzRegexpInit
#define LLVMFuzzerTestOneInput fuzzRegexp
#define LLVMFuzzerCustomMutator fuzzRegexpMutate
#include "regexp.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_SCHEMA_FUZZER
int fuzzSchemaInit(int *argc, char ***argv);
int fuzzSchema(const char *data, size_t size);
size_t fuzzSchemaMutate(char *data, size_t size, size_t maxSize,
                        unsigned seed);
#define LLVMFuzzerInitialize fuzzSchemaInit
#define LLVMFuzzerTestOneInput fuzzSchema
#define LLVMFuzzerCustomMutator fuzzSchemaMutate
#include "schema.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_URI_FUZZER
int fuzzUriInit(int *argc, char ***argv);
int fuzzUri(const char *data, size_t size);
size_t fuzzUriMutate(char *data, size_t size, size_t maxSize,
                     unsigned seed);
#define LLVMFuzzerInitialize fuzzUriInit
#define LLVMFuzzerTestOneInput fuzzUri
#define LLVMFuzzerCustomMutator fuzzUriMutate
#include "uri.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_VALID_FUZZER
int fuzzValidInit(int *argc, char ***argv);
int fuzzValid(const char *data, size_t size);
size_t fuzzValidMutate(char *data, size_t size, size_t maxSize,
                       unsigned seed);
#define LLVMFuzzerInitialize fuzzValidInit
#define LLVMFuzzerTestOneInput fuzzValid
#define LLVMFuzzerCustomMutator fuzzValidMutate
#include "valid.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_XINCLUDE_FUZZER
int fuzzXIncludeInit(int *argc, char ***argv);
int fuzzXInclude(const char *data, size_t size);
size_t fuzzXIncludeMutate(char *data, size_t size, size_t maxSize,
                          unsigned seed);
#define LLVMFuzzerInitialize fuzzXIncludeInit
#define LLVMFuzzerTestOneInput fuzzXInclude
#define LLVMFuzzerCustomMutator fuzzXIncludeMutate
#include "xinclude.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_XML_FUZZER
int fuzzXmlInit(int *argc, char ***argv);
int fuzzXml(const char *data, size_t size);
size_t fuzzXmlMutate(char *data, size_t size, size_t maxSize,
                     unsigned seed);
#define LLVMFuzzerInitialize fuzzXmlInit
#define LLVMFuzzerTestOneInput fuzzXml
#define LLVMFuzzerCustomMutator fuzzXmlMutate
#include "xml.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

#ifdef HAVE_XPATH_FUZZER
int fuzzXPathInit(int *argc, char ***argv);
int fuzzXPath(const char *data, size_t size);
size_t fuzzXPathMutate(char *data, size_t size, size_t maxSize,
                       unsigned seed);
#define LLVMFuzzerInitialize fuzzXPathInit
#define LLVMFuzzerTestOneInput fuzzXPath
#define LLVMFuzzerCustomMutator fuzzXPathMutate
#include "xpath.c"
#undef LLVMFuzzerInitialize
#undef LLVMFuzzerTestOneInput
#undef LLVMFuzzerCustomMutator
#endif

typedef int
(*initFunc)(int *argc, char ***argv);
typedef int
(*fuzzFunc)(const char *data, size_t size);
typedef size_t
(*mutateFunc)(char *data, size_t size, size_t maxSize, unsigned seed);

int numInputs;

static int
testFuzzer(initFunc init, fuzzFunc fuzz, mutateFunc mutate,
           const char *pattern) {
    glob_t globbuf;
    int ret = -1;
    size_t i;

    (void) mutate;

    if (glob(pattern, 0, NULL, &globbuf) != 0) {
        fprintf(stderr, "pattern %s matches no files\n", pattern);
        return(-1);
    }

    if (init != NULL)
        init(NULL, NULL);

    for (i = 0; i < globbuf.gl_pathc; i++) {
        const char *path = globbuf.gl_pathv[i];
        char *data;
        size_t size;

        data = xmlSlurpFile(path, &size);
        if (data == NULL) {
            fprintf(stderr, "couldn't read %s\n", path);
            goto error;
        }
        fuzz(data, size);
        xmlFree(data);

        numInputs++;
    }

    ret = 0;
error:
    globfree(&globbuf);
    return(ret);
}

#ifdef HAVE_XML_FUZZER
static int
testEntityLoader(void) {
    xmlParserCtxtPtr ctxt;
    static const char data[] =
        "doc.xml\\\n"
        "<!DOCTYPE doc SYSTEM \"doc.dtd\">\n"
        "<doc>&ent;</doc>\\\n"
        "doc.dtd\\\n"
        "<!ELEMENT doc (#PCDATA)>\n"
        "<!ENTITY ent SYSTEM \"ent.txt\">\\\n"
        "ent.txt\\\n"
        "Hello, world!\\\n";
    const char *docBuffer, *url;
    size_t docSize;
    xmlDocPtr doc;
    int ret = 0;

    xmlFuzzDataInit(data, sizeof(data) - 1);
    xmlFuzzReadEntities();

    url = xmlFuzzMainUrl();
    if (strcmp(url, "doc.xml") != 0) {
        fprintf(stderr, "unexpected main url: %s\n", url);
        ret = 1;
    }

    url = xmlFuzzSecondaryUrl();
    if (strcmp(url, "doc.dtd") != 0) {
        fprintf(stderr, "unexpected secondary url: %s\n", url);
        ret = 1;
    }

    docBuffer = xmlFuzzMainEntity(&docSize);
    ctxt = xmlNewParserCtxt();
    xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
    doc = xmlCtxtReadMemory(ctxt, docBuffer, docSize, NULL, NULL,
                            XML_PARSE_NOENT | XML_PARSE_DTDLOAD);
    xmlFreeParserCtxt(ctxt);

#ifdef LIBXML_OUTPUT_ENABLED
    {
        static xmlChar expected[] =
            "<?xml version=\"1.0\"?>\n"
            "<!DOCTYPE doc SYSTEM \"doc.dtd\">\n"
            "<doc>Hello, world!</doc>\n";
        xmlChar *out;

        xmlDocDumpMemory(doc, &out, NULL);
        if (xmlStrcmp(out, expected) != 0) {
            fprintf(stderr, "Expected:\n%sGot:\n%s", expected, out);
            ret = 1;
        }
        xmlFree(out);
    }
#endif

    xmlFreeDoc(doc);
    xmlFuzzDataCleanup();

    return(ret);
}
#endif

int
main(void) {
    int ret = 0;

#ifdef HAVE_XML_FUZZER
    if (testEntityLoader() != 0)
        ret = 1;
#endif
#ifdef HAVE_HTML_FUZZER
    if (testFuzzer(fuzzHtmlInit, fuzzHtml, fuzzHtmlMutate,
                   "seed/html/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_READER_FUZZER
    if (testFuzzer(fuzzReaderInit, fuzzReader, fuzzReaderMutate,
                   "seed/reader/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_REGEXP_FUZZER
    if (testFuzzer(fuzzRegexpInit, fuzzRegexp, fuzzRegexpMutate,
                   "seed/regexp/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_SCHEMA_FUZZER
    if (testFuzzer(fuzzSchemaInit, fuzzSchema, fuzzSchemaMutate,
                   "seed/schema/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_URI_FUZZER
    if (testFuzzer(fuzzUriInit, fuzzUri, fuzzUriMutate,
                   "seed/uri/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_VALID_FUZZER
    if (testFuzzer(fuzzValidInit, fuzzValid, fuzzValidMutate,
                   "seed/valid/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_XINCLUDE_FUZZER
    if (testFuzzer(fuzzXIncludeInit, fuzzXInclude, fuzzXIncludeMutate,
                   "seed/xinclude/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_XML_FUZZER
    if (testFuzzer(fuzzXmlInit, fuzzXml, fuzzXmlMutate,
                   "seed/xml/*") != 0)
        ret = 1;
#endif
#ifdef HAVE_XPATH_FUZZER
    if (testFuzzer(fuzzXPathInit, fuzzXPath, fuzzXPathMutate,
                   "seed/xpath/*") != 0)
        ret = 1;
#endif

    if (ret == 0)
        printf("Successfully tested %d inputs\n", numInputs);

    return(ret);
}

