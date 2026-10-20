/*
 * fuzz.h: Common functions and macros for fuzzing.
 *
 * See Copyright for the status of this software.
 */

#ifndef __XML_FUZZERCOMMON_H__
#define __XML_FUZZERCOMMON_H__

#include <stddef.h>
#include <stdio.h>
#include <libxml/parser.h>

#ifdef __cplusplus
extern "C" {
#endif

#if __GNUC__ * 100 + __GNUC_MINOR__ >= 207 || defined(__clang__)
  #define ATTRIBUTE_UNUSED __attribute__((unused))
#else
  #define ATTRIBUTE_UNUSED
#endif

#if defined(LIBXML_HTML_ENABLED)
  #define HAVE_HTML_FUZZER
#endif
#if 1
  #define HAVE_LINT_FUZZER
#endif
#if defined(LIBXML_READER_ENABLED)
  #define HAVE_READER_FUZZER
#endif
#if defined(LIBXML_REGEXP_ENABLED)
  #define HAVE_REGEXP_FUZZER
#endif
#if defined(LIBXML_SCHEMAS_ENABLED)
  #define HAVE_SCHEMA_FUZZER
#endif
#if 1
  #define HAVE_URI_FUZZER
#endif
#if defined(LIBXML_VALID_ENABLED)
  #define HAVE_VALID_FUZZER
#endif
#if defined(LIBXML_XINCLUDE_ENABLED)
  #define HAVE_XINCLUDE_FUZZER
#endif
#if 1
  #define HAVE_XML_FUZZER
#endif
#if defined(LIBXML_XPTR_ENABLED)
  #define HAVE_XPATH_FUZZER
#endif

#define XML_FUZZ_PROB_ONE (1u << 16)

typedef size_t
(*xmlFuzzMutator)(char *data, size_t size, size_t maxSize);

typedef struct {
    unsigned size;
    unsigned mutateProb;
} xmlFuzzChunkDesc;

int
LLVMFuzzerInitialize(int *argc, char ***argv);

int
LLVMFuzzerTestOneInput(const char *data, size_t size);

size_t
LLVMFuzzerMutate(char *data, size_t size, size_t maxSize);

size_t
LLVMFuzzerCustomMutator(char *data, size_t size, size_t maxSize,
                        unsigned seed);

void
xmlFuzzErrorFunc(void *ctx, const char *msg, ...);

void
xmlFuzzSErrorFunc(void *ctx, const xmlError *error);

void
xmlFuzzMemSetup(void);

void
xmlFuzzInjectFailure(size_t failurePos);

int
xmlFuzzMallocFailed(void);

void
xmlFuzzResetFailure(void);

void
xmlFuzzCheckFailureReport(const char *func, int oomReport, int ioReport);

void
xmlFuzzDataInit(const char *data, size_t size);

void
xmlFuzzDataCleanup(void);

void
xmlFuzzWriteInt(FILE *out, size_t v, int size);

size_t
xmlFuzzReadInt(int size);

size_t
xmlFuzzBytesRemaining(void);

const char *
xmlFuzzReadRemaining(size_t *size);

void
xmlFuzzWriteString(FILE *out, const char *str);

const char *
xmlFuzzReadString(size_t *size);

void
xmlFuzzReadEntities(void);

const char *
xmlFuzzMainUrl(void);

const char *
xmlFuzzMainEntity(size_t *size);

const char *
xmlFuzzSecondaryUrl(void);

const char *
xmlFuzzSecondaryEntity(size_t *size);

xmlParserErrors
xmlFuzzResourceLoader(void *data, const char *URL, const char *ID,
                      xmlResourceType type, xmlParserInputFlags flags,
                      xmlParserInputPtr *out);

char *
xmlSlurpFile(const char *path, size_t *size);

int
xmlFuzzOutputWrite(void *ctxt, const char *buffer, int len);

int
xmlFuzzOutputClose(void *ctxt);

size_t
xmlFuzzMutateChunks(const xmlFuzzChunkDesc *chunks,
                    char *data, size_t size, size_t maxSize, unsigned seed,
                    xmlFuzzMutator mutator);

#ifdef __cplusplus
}
#endif

#endif /* __XML_FUZZERCOMMON_H__ */

