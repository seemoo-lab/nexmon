/*
 * xml.c: a libFuzzer target to test several XML parser interfaces.
 *
 * See Copyright for the status of this software.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/catalog.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlsave.h>
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
    xmlDocPtr doc;
    const char *docBuffer, *docUrl;
    size_t failurePos, docSize, maxChunkSize;
    int opts;
    int errorCode;
#ifdef LIBXML_OUTPUT_ENABLED
    xmlBufferPtr outbuf = NULL;
    const char *saveEncoding;
    int saveOpts;
#endif

    xmlFuzzDataInit(data, size);
    opts = (int) xmlFuzzReadInt(4);
    /*
     * Disable options that are known to cause timeouts
     */
    opts &= ~XML_PARSE_DTDVALID &
            ~XML_PARSE_SAX1;
    failurePos = xmlFuzzReadInt(4) % (size + 100);

    maxChunkSize = xmlFuzzReadInt(4) % (size + size / 8 + 1);
    if (maxChunkSize == 0)
        maxChunkSize = 1;

#ifdef LIBXML_OUTPUT_ENABLED
    /* TODO: Take from fuzz data */
    saveOpts = 0;
    saveEncoding = NULL;
#endif

    xmlFuzzReadEntities();
    docBuffer = xmlFuzzMainEntity(&docSize);
    docUrl = xmlFuzzMainUrl();
    if (docBuffer == NULL)
        goto exit;

    /* Pull parser */

    xmlFuzzInjectFailure(failurePos);
    ctxt = xmlNewParserCtxt();
    if (ctxt == NULL) {
        errorCode = XML_ERR_NO_MEMORY;
    } else {
        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
        doc = xmlCtxtReadMemory(ctxt, docBuffer, docSize, docUrl, NULL, opts);
        errorCode = ctxt->errNo;
        xmlFuzzCheckFailureReport("xmlCtxtReadMemory",
                doc == NULL && errorCode == XML_ERR_NO_MEMORY,
                doc == NULL && errorCode == XML_IO_EIO);

        if (doc != NULL) {
#ifdef LIBXML_OUTPUT_ENABLED
            xmlSaveCtxtPtr save;

            outbuf = xmlBufferCreate();

            /* Also test the serializer. */
            save = xmlSaveToBuffer(outbuf, saveEncoding, saveOpts);

            if (save == NULL) {
                xmlBufferFree(outbuf);
                outbuf = NULL;
            } else {
                int saveErr;

                xmlSaveDoc(save, doc);
                saveErr = xmlSaveFinish(save);
                xmlFuzzCheckFailureReport("xmlSaveToBuffer",
                                          saveErr == XML_ERR_NO_MEMORY,
                                          saveErr == XML_IO_EIO);
                if (saveErr != XML_ERR_OK) {
                    xmlBufferFree(outbuf);
                    outbuf = NULL;
                }
            }
#endif
            xmlFreeDoc(doc);
        }

        xmlFreeParserCtxt(ctxt);
    }

    /* Push parser */

#ifdef LIBXML_PUSH_ENABLED
    xmlFuzzInjectFailure(failurePos);
    /*
     * FIXME: xmlCreatePushParserCtxt can still report OOM errors
     * to stderr.
     */
    xmlSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);
    ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, docUrl);
    xmlSetGenericErrorFunc(NULL, NULL);

    if (ctxt != NULL) {
        size_t consumed;
        int errorCodePush, numChunks, maxChunks;

        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
        xmlCtxtUseOptions(ctxt, opts);

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

            xmlParseChunk(ctxt, docBuffer + consumed, chunkSize, terminate);
            consumed += chunkSize;
        }

        errorCodePush = ctxt->errNo;
        xmlFuzzCheckFailureReport("xmlParseChunk",
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
            FILE *f = fopen("c.xml", "wb");
            fwrite(docBuffer, docSize, 1, f);
            fclose(f);
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
            outbuf != NULL) {
            xmlBufferPtr outbufPush;
            xmlSaveCtxtPtr save;

            outbufPush = xmlBufferCreate();

            save = xmlSaveToBuffer(outbufPush, saveEncoding, saveOpts);

            if (save != NULL) {
                int saveErr;

                xmlSaveDoc(save, doc);
                saveErr = xmlSaveFinish(save);

                if (saveErr == XML_ERR_OK) {
                    int outbufSize = xmlBufferLength(outbuf);

                    if (outbufSize != xmlBufferLength(outbufPush) ||
                        memcmp(xmlBufferContent(outbuf),
                               xmlBufferContent(outbufPush),
                               outbufSize) != 0) {
                        fprintf(stderr, "pull/push parser roundtrip "
                                "mismatch\n");
#if 0
                        FILE *f = fopen("c.xml", "wb");
                        fwrite(docBuffer, docSize, 1, f);
                        fclose(f);
                        fprintf(stderr, "opts: %X\n", opts);
                        fprintf(stderr, "---\n%s\n---\n%s\n---\n",
                                xmlBufferContent(outbuf),
                                xmlBufferContent(outbufPush));
#endif
                        abort();
                    }
                }
            }

            xmlBufferFree(outbufPush);
        }
#endif

        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
    }
#endif

exit:
#ifdef LIBXML_OUTPUT_ENABLED
    xmlBufferFree(outbuf);
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
        { 4, XML_FUZZ_PROB_ONE / 10 }, /* maxChunkSize */
        { 0, 0 }
    };

    return xmlFuzzMutateChunks(chunks, data, size, maxSize, seed,
                               LLVMFuzzerMutate);
}

