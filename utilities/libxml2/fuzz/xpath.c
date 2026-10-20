/*
 * xpath.c: a libFuzzer target to test XPath and XPointer expressions.
 *
 * See Copyright for the status of this software.
 */

#include <libxml/catalog.h>
#include <libxml/parser.h>
#include <libxml/xpointer.h>
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
    xmlDocPtr doc;
    const char *expr, *xml;
    size_t failurePos, exprSize, xmlSize;

    if (size > 10000)
        return(0);

    xmlFuzzDataInit(data, size);

    failurePos = xmlFuzzReadInt(4) % (size + 100);
    expr = xmlFuzzReadString(&exprSize);
    xml = xmlFuzzReadString(&xmlSize);

    /* Recovery mode allows more input to be fuzzed. */
    doc = xmlReadMemory(xml, xmlSize, NULL, NULL, XML_PARSE_RECOVER);
    if (doc != NULL) {
        xmlXPathContextPtr xpctxt;

        xmlFuzzInjectFailure(failurePos);

        xpctxt = xmlXPathNewContext(doc);
        if (xpctxt != NULL) {
            int res;

            /* Operation limit to avoid timeout */
            xpctxt->opLimit = 500000;

            res = xmlXPathContextSetCache(xpctxt, 1, 4, 0);
            xmlFuzzCheckFailureReport("xmlXPathContextSetCache", res == -1, 0);

            xmlFuzzResetFailure();
            xmlXPathFreeObject(xmlXPtrEval(BAD_CAST expr, xpctxt));
            xmlFuzzCheckFailureReport("xmlXPtrEval",
                    xpctxt->lastError.code == XML_ERR_NO_MEMORY, 0);
            xmlXPathFreeContext(xpctxt);
        }

        xmlFuzzInjectFailure(0);
        xmlFreeDoc(doc);
    }

    xmlFuzzDataCleanup();
    xmlResetLastError();

    return(0);
}

size_t
LLVMFuzzerCustomMutator(char *data, size_t size, size_t maxSize,
                        unsigned seed) {
    static const xmlFuzzChunkDesc chunks[] = {
        { 4, XML_FUZZ_PROB_ONE / 10 }, /* failurePos */
        { 0, 0 }
    };

    return xmlFuzzMutateChunks(chunks, data, size, maxSize, seed,
                               LLVMFuzzerMutate);
}

