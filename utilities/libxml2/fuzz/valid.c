/*
 * valid.c: a libFuzzer target to test DTD validation.
 *
 * See Copyright for the status of this software.
 */

#include <libxml/catalog.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
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
    size_t failurePos, docSize;
    int opts;

    xmlFuzzDataInit(data, size);
    opts = (int) xmlFuzzReadInt(4);
    opts |= XML_PARSE_DTDVALID;
    failurePos = xmlFuzzReadInt(4) % (size + 100);

    xmlFuzzReadEntities();
    docBuffer = xmlFuzzMainEntity(&docSize);
    docUrl = xmlFuzzMainUrl();
    if (docBuffer == NULL)
        goto exit;

    /* Pull parser */

    xmlFuzzInjectFailure(failurePos);
    ctxt = xmlNewParserCtxt();
    if (ctxt != NULL) {
        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
        doc = xmlCtxtReadMemory(ctxt, docBuffer, docSize, docUrl, NULL, opts);
        xmlFuzzCheckFailureReport("xmlCtxtReadMemory",
                                  ctxt->errNo == XML_ERR_NO_MEMORY,
                                  ctxt->errNo == XML_IO_EIO);
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
    }

    /* Post validation */

    xmlFuzzInjectFailure(failurePos);
    ctxt = xmlNewParserCtxt();
    if (ctxt != NULL) {
        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
        doc = xmlCtxtReadMemory(ctxt, docBuffer, docSize, docUrl, NULL,
                                opts & ~XML_PARSE_DTDVALID);
        xmlFuzzCheckFailureReport("xmlCtxtReadMemory",
                doc == NULL && ctxt->errNo == XML_ERR_NO_MEMORY,
                doc == NULL && ctxt->errNo == XML_IO_EIO);
        if (doc != NULL) {
            int valid = xmlCtxtValidateDocument(ctxt, doc);

            xmlFuzzCheckFailureReport("xmlCtxtValidateDocument",
                    !valid && ctxt->errNo == XML_ERR_NO_MEMORY,
                    !valid && ctxt->errNo == XML_IO_EIO);
        }
        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
    }

    /* Push parser */

#ifdef LIBXML_PUSH_ENABLED
    {
        static const size_t maxChunkSize = 128;
        size_t consumed, chunkSize;

        xmlFuzzInjectFailure(failurePos);
        /*
         * FIXME: xmlCreatePushParserCtxt can still report OOM errors
         * to stderr.
         */
        xmlSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);
        ctxt = xmlCreatePushParserCtxt(NULL, NULL, NULL, 0, docUrl);
        xmlSetGenericErrorFunc(NULL, NULL);
        if (ctxt != NULL) {
            xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
            xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
            xmlCtxtUseOptions(ctxt, opts);

            for (consumed = 0; consumed < docSize; consumed += chunkSize) {
                chunkSize = docSize - consumed;
                if (chunkSize > maxChunkSize)
                    chunkSize = maxChunkSize;
                xmlParseChunk(ctxt, docBuffer + consumed, chunkSize, 0);
            }

            xmlParseChunk(ctxt, NULL, 0, 1);
            xmlFuzzCheckFailureReport("xmlParseChunk",
                                      ctxt->errNo == XML_ERR_NO_MEMORY,
                                      ctxt->errNo == XML_IO_EIO);
            xmlFreeDoc(ctxt->myDoc);
            xmlFreeParserCtxt(ctxt);
        }
    }
#endif

exit:
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

