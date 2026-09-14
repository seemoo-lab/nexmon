/*
 * xinclude.c: a libFuzzer target to test the XInclude engine.
 *
 * See Copyright for the status of this software.
 */

#include <libxml/catalog.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xinclude.h>
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
    xmlSetGenericErrorFunc(NULL, xmlFuzzErrorFunc);

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
    opts &= ~XML_PARSE_DTDVALID &
            ~XML_PARSE_SAX1;
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
        xmlXIncludeCtxtPtr xinc;
        xmlDocPtr copy;

        xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);

        doc = xmlCtxtReadMemory(ctxt, docBuffer, docSize, docUrl, NULL, opts);
        xmlFuzzCheckFailureReport("xmlCtxtReadMemory",
                doc == NULL && ctxt->errNo == XML_ERR_NO_MEMORY,
                doc == NULL && ctxt->errNo == XML_IO_EIO);

        xinc = xmlXIncludeNewContext(doc);
        xmlXIncludeSetResourceLoader(xinc, xmlFuzzResourceLoader, NULL);
        xmlXIncludeSetFlags(xinc, opts);
        xmlXIncludeProcessNode(xinc, (xmlNodePtr) doc);
        if (doc != NULL) {
            xmlFuzzCheckFailureReport("xmlXIncludeProcessNode",
                    xinc == NULL ||
                    xmlXIncludeGetLastError(xinc) == XML_ERR_NO_MEMORY,
                    xinc != NULL &&
                    xmlXIncludeGetLastError(xinc) == XML_IO_EIO);
        }
        xmlXIncludeFreeContext(xinc);

        xmlFuzzResetFailure();
        copy = xmlCopyDoc(doc, 1);
        if (doc != NULL)
            xmlFuzzCheckFailureReport("xmlCopyNode", copy == NULL, 0);
        xmlFreeDoc(copy);

        xmlFreeDoc(doc);
        xmlFreeParserCtxt(ctxt);
    }

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

