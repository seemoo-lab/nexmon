/*
 * schema.c: a libFuzzer target to test the XML Schema processor.
 *
 * See Copyright for the status of this software.
 */

#ifndef XML_DEPRECATED
  #define XML_DEPRECATED
#endif

#include <libxml/catalog.h>
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
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
    xmlSchemaParserCtxtPtr pctxt;
    xmlSchemaPtr schema;
    size_t failurePos;

    if (size > 200000)
        return(0);

    xmlFuzzDataInit(data, size);

    failurePos = xmlFuzzReadInt(4) % (size + 100);

    xmlFuzzReadEntities();

    xmlFuzzInjectFailure(failurePos);
    pctxt = xmlSchemaNewParserCtxt(xmlFuzzMainUrl());
    xmlSchemaSetParserStructuredErrors(pctxt, xmlFuzzSErrorFunc, NULL);
    xmlSchemaSetResourceLoader(pctxt, xmlFuzzResourceLoader, NULL);
    schema = xmlSchemaParse(pctxt);
    xmlSchemaFreeParserCtxt(pctxt);

    if (schema != NULL) {
        xmlSchemaValidCtxtPtr vctxt;
        xmlParserCtxtPtr ctxt;
        xmlDocPtr doc;

        ctxt = xmlNewParserCtxt();
        xmlCtxtSetErrorHandler(ctxt, xmlFuzzSErrorFunc, NULL);
        xmlCtxtSetResourceLoader(ctxt, xmlFuzzResourceLoader, NULL);
        doc = xmlCtxtReadFile(ctxt, xmlFuzzSecondaryUrl(), NULL,
                              XML_PARSE_NOENT);
        xmlFreeParserCtxt(ctxt);

        vctxt = xmlSchemaNewValidCtxt(schema);
        xmlSchemaSetValidStructuredErrors(vctxt, xmlFuzzSErrorFunc, NULL);
        xmlSchemaValidateDoc(vctxt, doc);
        xmlSchemaFreeValidCtxt(vctxt);

        xmlFreeDoc(doc);
        xmlSchemaFree(schema);
    }

    xmlFuzzInjectFailure(0);
    xmlFuzzDataCleanup();
    xmlResetLastError();
    xmlSchemaCleanupTypes();

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

