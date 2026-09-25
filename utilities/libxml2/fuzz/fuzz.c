/*
 * fuzz.c: Common functions for fuzzing.
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <libxml/hash.h>
#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/tree.h>
#include <libxml/xmlIO.h>
#include "fuzz.h"

typedef struct {
    const char *data;
    size_t size;
} xmlFuzzEntityInfo;

/* Single static instance for now */
static struct {
    /* Original data */
    const char *data;
    size_t size;

    /* Remaining data */
    const char *ptr;
    size_t remaining;

    /* Buffer for unescaped strings */
    char *outBuf;
    char *outPtr; /* Free space at end of buffer */

    xmlHashTablePtr entities; /* Maps URLs to xmlFuzzEntityInfos */

    /* The first entity is the main entity. */
    const char *mainUrl;
    xmlFuzzEntityInfo *mainEntity;
    const char *secondaryUrl;
    xmlFuzzEntityInfo *secondaryEntity;
} fuzzData;

size_t fuzzNumAttempts;
size_t fuzzFailurePos;
int fuzzAllocFailed;
int fuzzIoFailed;

/**
 * xmlFuzzErrorFunc:
 *
 * An error function that simply discards all errors.
 */
void
xmlFuzzErrorFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg ATTRIBUTE_UNUSED,
                 ...) {
}

/**
 * xmlFuzzSErrorFunc:
 *
 * A structured error function that simply discards all errors.
 */
void
xmlFuzzSErrorFunc(void *ctx ATTRIBUTE_UNUSED,
                  const xmlError *error ATTRIBUTE_UNUSED) {
}

/*
 * Failure injection.
 *
 * To debug issues involving injected failures, it's often helpful to set
 * FAILURE_ABORT to 1. This should provide a backtrace of the failed
 * operation.
 */

#define XML_FUZZ_FAILURE_ABORT   0

void
xmlFuzzInjectFailure(size_t failurePos) {
    fuzzNumAttempts = 0;
    fuzzFailurePos = failurePos;
    fuzzAllocFailed = 0;
    fuzzIoFailed = 0;
}

static int
xmlFuzzTryMalloc(void) {
    if (fuzzFailurePos > 0) {
        fuzzNumAttempts += 1;
        if (fuzzNumAttempts == fuzzFailurePos) {
#if XML_FUZZ_FAILURE_ABORT
            abort();
#endif
            fuzzAllocFailed = 1;
            return -1;
        }
    }

    return 0;
}

static int
xmlFuzzTryIo(void) {
    if (fuzzFailurePos > 0) {
        fuzzNumAttempts += 1;
        if (fuzzNumAttempts == fuzzFailurePos) {
#if XML_FUZZ_FAILURE_ABORT
            abort();
#endif
            fuzzIoFailed = 1;
            return -1;
        }
    }

    return 0;
}

static void *
xmlFuzzMalloc(size_t size) {
    void *ret;

    if (xmlFuzzTryMalloc() < 0)
        return NULL;

    ret = malloc(size);
    if (ret == NULL)
        fuzzAllocFailed = 1;

    return ret;
}

static void *
xmlFuzzRealloc(void *ptr, size_t size) {
    void *ret;

    if (xmlFuzzTryMalloc() < 0)
        return NULL;

    ret = realloc(ptr, size);
    if (ret == NULL)
        fuzzAllocFailed = 1;

    return ret;
}

void
xmlFuzzMemSetup(void) {
    xmlMemSetup(free, xmlFuzzMalloc, xmlFuzzRealloc, xmlMemStrdup);
}

int
xmlFuzzMallocFailed(void) {
    return fuzzAllocFailed;
}

void
xmlFuzzResetFailure(void) {
    fuzzAllocFailed = 0;
    fuzzIoFailed = 0;
}

void
xmlFuzzCheckFailureReport(const char *func, int oomReport, int ioReport) {
    if (oomReport >= 0 && fuzzAllocFailed != oomReport) {
        fprintf(stderr, "%s: malloc failure %s reported\n",
                func, fuzzAllocFailed ? "not" : "erroneously");
        abort();
    }
    if (ioReport >= 0 && fuzzIoFailed != ioReport) {
        fprintf(stderr, "%s: IO failure %s reported\n",
                func, fuzzIoFailed ? "not" : "erroneously");
        abort();
    }
    fuzzAllocFailed = 0;
    fuzzIoFailed = 0;
}

/**
 * xmlFuzzDataInit:
 *
 * Initialize fuzz data provider.
 */
void
xmlFuzzDataInit(const char *data, size_t size) {
    fuzzData.data = data;
    fuzzData.size = size;
    fuzzData.ptr = data;
    fuzzData.remaining = size;

    fuzzData.outBuf = xmlMalloc(size + 1);
    fuzzData.outPtr = fuzzData.outBuf;

    fuzzData.entities = xmlHashCreate(8);
    fuzzData.mainUrl = NULL;
    fuzzData.mainEntity = NULL;
    fuzzData.secondaryUrl = NULL;
    fuzzData.secondaryEntity = NULL;
}

/**
 * xmlFuzzDataFree:
 *
 * Cleanup fuzz data provider.
 */
void
xmlFuzzDataCleanup(void) {
    xmlFree(fuzzData.outBuf);
    xmlHashFree(fuzzData.entities, xmlHashDefaultDeallocator);
}

/**
 * xmlFuzzWriteInt:
 * @out:  output file
 * @v:  integer to write
 * @size:  size of integer in bytes
 *
 * Write an integer to the fuzz data.
 */
void
xmlFuzzWriteInt(FILE *out, size_t v, int size) {
    int shift;

    while (size > (int) sizeof(size_t)) {
        putc(0, out);
        size--;
    }

    shift = size * 8;
    while (shift > 0) {
        shift -= 8;
        putc((v >> shift) & 255, out);
    }
}

/**
 * xmlFuzzReadInt:
 * @size:  size of integer in bytes
 *
 * Read an integer from the fuzz data.
 */
size_t
xmlFuzzReadInt(int size) {
    size_t ret = 0;

    while ((size > 0) && (fuzzData.remaining > 0)) {
        unsigned char c = (unsigned char) *fuzzData.ptr++;
        fuzzData.remaining--;
        ret = (ret << 8) | c;
        size--;
    }

    return ret;
}

/**
 * xmlFuzzBytesRemaining:
 *
 * Return number of remaining bytes in fuzz data.
 */
size_t
xmlFuzzBytesRemaining(void) {
    return(fuzzData.remaining);
}

/**
 * xmlFuzzReadRemaining:
 * @size:  size of string in bytes
 *
 * Read remaining bytes from fuzz data.
 */
const char *
xmlFuzzReadRemaining(size_t *size) {
    const char *ret = fuzzData.ptr;

    *size = fuzzData.remaining;
    fuzzData.ptr += fuzzData.remaining;
    fuzzData.remaining = 0;

    return(ret);
}

/*
 * xmlFuzzWriteString:
 * @out:  output file
 * @str:  string to write
 *
 * Write a random-length string to file in a format similar to
 * FuzzedDataProvider. Backslash followed by newline marks the end of the
 * string. Two backslashes are used to escape a backslash.
 */
void
xmlFuzzWriteString(FILE *out, const char *str) {
    for (; *str; str++) {
        int c = (unsigned char) *str;
        putc(c, out);
        if (c == '\\')
            putc(c, out);
    }
    putc('\\', out);
    putc('\n', out);
}

/**
 * xmlFuzzReadString:
 * @size:  size of string in bytes
 *
 * Read a random-length string from the fuzz data.
 *
 * The format is similar to libFuzzer's FuzzedDataProvider but treats
 * backslash followed by newline as end of string. This makes the fuzz data
 * more readable. A backslash character is escaped with another backslash.
 *
 * Returns a zero-terminated string or NULL if the fuzz data is exhausted.
 */
const char *
xmlFuzzReadString(size_t *size) {
    const char *out = fuzzData.outPtr;

    while (fuzzData.remaining > 0) {
        int c = *fuzzData.ptr++;
        fuzzData.remaining--;

        if ((c == '\\') && (fuzzData.remaining > 0)) {
            int c2 = *fuzzData.ptr;

            if (c2 == '\n') {
                fuzzData.ptr++;
                fuzzData.remaining--;
                if (size != NULL)
                    *size = fuzzData.outPtr - out;
                *fuzzData.outPtr++ = '\0';
                return(out);
            }
            if (c2 == '\\') {
                fuzzData.ptr++;
                fuzzData.remaining--;
            }
        }

        *fuzzData.outPtr++ = c;
    }

    if (fuzzData.outPtr > out) {
        if (size != NULL)
            *size = fuzzData.outPtr - out;
        *fuzzData.outPtr++ = '\0';
        return(out);
    }

    if (size != NULL)
        *size = 0;
    return(NULL);
}

/**
 * xmlFuzzReadEntities:
 *
 * Read entities like the main XML file, external DTDs, external parsed
 * entities from fuzz data.
 */
void
xmlFuzzReadEntities(void) {
    size_t num = 0;

    while (1) {
        const char *url, *entity;
        size_t urlSize, entitySize;
        xmlFuzzEntityInfo *entityInfo;

        url = xmlFuzzReadString(&urlSize);
        if (url == NULL) break;

        entity = xmlFuzzReadString(&entitySize);
        if (entity == NULL) break;

        /*
         * Cap URL size to avoid quadratic behavior when generating
         * error messages or looking up entities.
         */
        if (urlSize < 50 &&
            xmlHashLookup(fuzzData.entities, (xmlChar *)url) == NULL) {
            entityInfo = xmlMalloc(sizeof(xmlFuzzEntityInfo));
            if (entityInfo == NULL)
                break;
            entityInfo->data = entity;
            entityInfo->size = entitySize;

            xmlHashAddEntry(fuzzData.entities, (xmlChar *)url, entityInfo);

            if (num == 0) {
                fuzzData.mainUrl = url;
                fuzzData.mainEntity = entityInfo;
            } else if (num == 1) {
                fuzzData.secondaryUrl = url;
                fuzzData.secondaryEntity = entityInfo;
            }

            num++;
        }
    }
}

/**
 * xmlFuzzMainUrl:
 *
 * Returns the main URL.
 */
const char *
xmlFuzzMainUrl(void) {
    return(fuzzData.mainUrl);
}

/**
 * xmlFuzzMainEntity:
 * @size:  size of the main entity in bytes
 *
 * Returns the main entity.
 */
const char *
xmlFuzzMainEntity(size_t *size) {
    if (fuzzData.mainEntity == NULL)
        return(NULL);
    *size = fuzzData.mainEntity->size;
    return(fuzzData.mainEntity->data);
}

/**
 * xmlFuzzSecondaryUrl:
 *
 * Returns the secondary URL.
 */
const char *
xmlFuzzSecondaryUrl(void) {
    return(fuzzData.secondaryUrl);
}

/**
 * xmlFuzzSecondaryEntity:
 * @size:  size of the secondary entity in bytes
 *
 * Returns the secondary entity.
 */
const char *
xmlFuzzSecondaryEntity(size_t *size) {
    if (fuzzData.secondaryEntity == NULL)
        return(NULL);
    *size = fuzzData.secondaryEntity->size;
    return(fuzzData.secondaryEntity->data);
}

/**
 * xmlFuzzResourceLoader:
 *
 * The resource loader for fuzz data.
 */
xmlParserErrors
xmlFuzzResourceLoader(void *data ATTRIBUTE_UNUSED, const char *URL,
                      const char *ID ATTRIBUTE_UNUSED,
                      xmlResourceType type ATTRIBUTE_UNUSED,
                      xmlParserInputFlags flags ATTRIBUTE_UNUSED,
                      xmlParserInputPtr *out) {
    xmlParserInputPtr input;
    xmlFuzzEntityInfo *entity;

    entity = xmlHashLookup(fuzzData.entities, (xmlChar *) URL);
    if (entity == NULL)
        return(XML_IO_ENOENT);

    /* IO failure injection */
    if (xmlFuzzTryIo() < 0)
        return(XML_IO_EIO);

    input = xmlNewInputFromMemory(URL, entity->data, entity->size,
                                  XML_INPUT_BUF_STATIC |
                                  XML_INPUT_BUF_ZERO_TERMINATED);
    if (input == NULL)
        return(XML_ERR_NO_MEMORY);

    *out = input;
    return(XML_ERR_OK);
}

char *
xmlSlurpFile(const char *path, size_t *sizeRet) {
    FILE *file;
    struct stat statbuf;
    char *data;
    size_t size;

    if ((stat(path, &statbuf) != 0) || (!S_ISREG(statbuf.st_mode)))
        return(NULL);
    size = statbuf.st_size;
    file = fopen(path, "rb");
    if (file == NULL)
        return(NULL);
    data = xmlMalloc(size + 1);
    if (data != NULL) {
        if (fread(data, 1, size, file) != size) {
            xmlFree(data);
            data = NULL;
        } else {
            data[size] = 0;
            if (sizeRet != NULL)
                *sizeRet = size;
        }
    }
    fclose(file);

    return(data);
}

int
xmlFuzzOutputWrite(void *ctxt ATTRIBUTE_UNUSED,
                   const char *buffer ATTRIBUTE_UNUSED, int len) {
    if (xmlFuzzTryIo() < 0)
        return -XML_IO_EIO;

    return len;
}

int
xmlFuzzOutputClose(void *ctxt ATTRIBUTE_UNUSED) {
    if (xmlFuzzTryIo() < 0)
        return XML_IO_EIO;

    return 0;
}

/**
 * xmlFuzzMutateChunks:
 * @chunks: array of chunk descriptions
 * @data: fuzz data (from LLVMFuzzerCustomMutator)
 * @size: data size (from LLVMFuzzerCustomMutator)
 * @maxSize: max data size (from LLVMFuzzerCustomMutator)
 * @seed: seed (from LLVMFuzzerCustomMutator)
 * @mutator: mutator function, use LLVMFuzzerMutate
 *
 * Mutates one of several chunks with a given probability.
 *
 * Probability is a value between 0 and XML_FUZZ_PROB_ONE.
 *
 * The last chunk has flexible size and must have size and
 * mutateProb set to 0.
 *
 * Returns the size of the mutated data like LLVMFuzzerCustomMutator.
 */
size_t
xmlFuzzMutateChunks(const xmlFuzzChunkDesc *chunks,
                    char *data, size_t size, size_t maxSize, unsigned seed,
                    xmlFuzzMutator mutator) {
    size_t off = 0;
    size_t ret, chunkSize, maxChunkSize, mutSize;
    unsigned prob = seed % XML_FUZZ_PROB_ONE;
    unsigned descSize = 0;
    int i = 0;

    while (1) {
        unsigned descProb;

        descSize = chunks[i].size;
        descProb = chunks[i].mutateProb;

        if (descSize == 0 ||
            off + descSize > size ||
            off + descSize >= maxSize ||
            prob < descProb)
            break;

        off += descSize;
        prob -= descProb;
        i += 1;
    }

    chunkSize = size - off;
    maxChunkSize = maxSize - off;

    if (descSize != 0) {
        if (chunkSize > descSize)
            chunkSize = descSize;
        if (maxChunkSize > descSize)
            maxChunkSize = descSize;
    }

    mutSize = mutator(data + off, chunkSize, maxChunkSize);

    if (size > off + chunkSize) {
        size_t j;

        for (j = mutSize; j < chunkSize; j++)
            data[off + j] = 0;

        ret = size;
    } else {
        ret = off + mutSize;
    }

    return ret;
}

