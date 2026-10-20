/*
 * html.c: a libFuzzer target to test several HTML parser interfaces.
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/HTMLparser.h>
#include <libxml/HTMLtree.h>
#include <libxml/catalog.h>
#include "fuzz.h"

int
LLVMFuzzerInitialize(int *argc ATTRIBUTE_UNUSED,
                     char ***argv ATTRIBUTE_UNUSED) {
    xmlFuzzMemSetup();
    xmlInitParser();
#ifdef LIBXML_CATALOG_ENABLED
    xmlInitializeCatalog();
    xmlCatalogSetDefaults(XML_CATA_ALLOW_NONE);
#endif

    return 0;
}

int
LLVMFuzzerTestOneInput(const char *data, size_t size) {
    xmlParserCtxtPtr ctxt;
    htmlDocPtr doc;
    const char *docBuffer;
    size_t failurePos, docSize, maxChunkSize;
    int opts, errorCode;
#ifdef LIBXML_OUTPUT_ENABLED
    xmlOutputBufferPtr out = NULL;
#endif

    xmlFuzzDataInit(data, size);
    opts = (int) xmlFuzzReadInt(4);
    failurePos = xmlFuzzReadInt(4) % (size + 100);

    maxChunkSize = xmlFuzzReadInt(4) % (size + size / 8 + 1);
    if (maxChunkSize == 0)
        maxChunkSize = 1;

    docBuffer = xmlFuzzReadRemaining(&docSize);
    if (docBuffer == NULL) {
        xmlFuzzDataCleanup();
        return(0);
    }

    /* Pull parser */

    xmlFuzzInjectFailure(failurePos);
    ctxt = htmlNewParserCtxt();
    if (ctxt == NULL) {
        errorCode = XML_ERR_NO_MEMORY;
    } else {
        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        doc = htmlCtxtReadMemory(ctxt, docBuffer, docSize, NULL, NULL, opts);
        errorCode = ctxt->errNo;
        xmlFuzzCheckFailureReport("htmlCtxtReadMemory",
                                  errorCode == XML_ERR_NO_MEMORY,
                                  errorCode == XML_IO_EIO);

        if (doc != NULL) {
            xmlDocPtr copy;

#ifdef LIBXML_OUTPUT_ENABLED
            const xmlChar *content;

            /*
             * Also test the serializer. Call htmlDocContentDumpOutput with our
             * own buffer to avoid encoding the output. The HTML encoding is
             * excruciatingly slow (see htmlEntityValueLookup).
             */
            out = xmlAllocOutputBuffer(NULL);
            htmlDocContentDumpOutput(out, doc, NULL);
            content = xmlOutputBufferGetContent(out);
            xmlFuzzCheckFailureReport("htmlDocContentDumpOutput",
                                      content == NULL, 0);
            if (content == NULL) {
                xmlOutputBufferClose(out);
                out = NULL;
            }
#endif

            copy = xmlCopyDoc(doc, 1);
            xmlFuzzCheckFailureReport("xmlCopyNode", copy == NULL, 0);
            xmlFreeDoc(copy);

            xmlFreeDoc(doc);
        }

        htmlFreeParserCtxt(ctxt);
    }


    /* Push parser */

#ifdef LIBXML_PUSH_ENABLED
    xmlFuzzInjectFailure(failurePos);
    ctxt = htmlCreatePushParserCtxt(NULL, NULL, NULL, 0, NULL,
                                    XML_CHAR_ENCODING_NONE);

    if (ctxt != NULL) {
        size_t consumed;
        int errorCodePush, numChunks, maxChunks;

        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        htmlCtxtUseOptions(ctxt, opts);

        consumed = 0;
        numChunks = 0;
        maxChunks = 50 + docSize / 100;
        while (numChunks == 0 ||
               (consumed < docSize && numChunks < maxChunks)) {
            size_t chunkSize;
            int terminate;

            numChunks += 1;
            chunkSize = docSize - consumed;

            if (numChunks < maxChunks && chunkSize > maxChunkSize) {
                chunkSize = maxChunkSize;
                terminate = 0;
            } else {
                terminate = 1;
            }

            htmlParseChunk(ctxt, docBuffer + consumed, chunkSize, terminate);
            consumed += chunkSize;
        }

        errorCodePush = ctxt->errNo;
        xmlFuzzCheckFailureReport("htmlParseChunk",
                                  errorCodePush == XML_ERR_NO_MEMORY,
                                  errorCodePush == XML_IO_EIO);
        doc = ctxt->myDoc;

        /*
         * Push and pull parser differ in when exactly they
         * stop parsing, and the error code is the *last* error
         * reported, so we can't check whether the codes match.
         */
        if (errorCode != XML_ERR_NO_MEMORY &&
            errorCode != XML_IO_EIO &&
            errorCodePush != XML_ERR_NO_MEMORY &&
            errorCodePush != XML_IO_EIO &&
            (errorCode == XML_ERR_OK) != (errorCodePush == XML_ERR_OK)) {
            fprintf(stderr, "pull/push parser error mismatch: %d != %d\n",
                    errorCode, errorCodePush);
#if 0
            FILE *f = fopen("c.html", "wb");
            fwrite(docBuffer, docSize, 1, f);
            fclose(f);
            fprintf(stderr, "opts: %X\n", opts);
#endif
            abort();
        }

#ifdef LIBXML_OUTPUT_ENABLED
        /*
         * Verify that pull and push parser produce the same result.
         *
         * The NOBLANKS option doesn't work reliably in push mode.
         */
        if ((opts & XML_PARSE_NOBLANKS) == 0 &&
            errorCode == XML_ERR_OK &&
            errorCodePush == XML_ERR_OK &&
            out != NULL) {
            xmlOutputBufferPtr outPush;
            const xmlChar *content, *contentPush;

            outPush = xmlAllocOutputBuffer(NULL);
            htmlDocContentDumpOutput(outPush, doc, NULL);
            content = xmlOutputBufferGetContent(out);
            contentPush = xmlOutputBufferGetContent(outPush);

            if (content != NULL && contentPush != NULL) {
                size_t outSize = xmlOutputBufferGetSize(out);

                if (outSize != xmlOutputBufferGetSize(outPush) ||
                    memcmp(content, contentPush, outSize) != 0) {
                    fprintf(stderr, "pull/push parser roundtrip "
                            "mismatch\n");
#if 0
                    FILE *f = fopen("c.html", "wb");
                    fwrite(docBuffer, docSize, 1, f);
                    fclose(f);
                    fprintf(stderr, "opts: %X\n", opts);
                    fprintf(stderr, "---\n%s\n---\n%s\n---\n",
                            xmlOutputBufferGetContent(out),
                            xmlOutputBufferGetContent(outPush));
#endif
                    abort();
                }
            }

            xmlOutputBufferClose(outPush);
        }
#endif

        xmlFreeDoc(doc);
        htmlFreeParserCtxt(ctxt);
    }
#endif

    /* Cleanup */

#ifdef LIBXML_OUTPUT_ENABLED
    xmlOutputBufferClose(out);
#endif

    xmlFuzzInjectFailure(0);
    xmlFuzzDataCleanup();
    xmlResetLastError();

    return(0);
}

size_t
LLVMFuzzerCustomMutator(char *data, size_t size, size_t maxSize,
                        unsigned seed) {
    static const xmlFuzzChunkDesc chunks[] = {
        { 4, XML_FUZZ_PROB_ONE / 10 }, /* opts */
        { 4, XML_FUZZ_PROB_ONE / 10 }, /* failurePos */
        { 0, 0 }
    };

    return xmlFuzzMutateChunks(chunks, data, size, maxSize, seed,
                               LLVMFuzzerMutate);
}

