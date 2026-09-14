/*
 * uri.c: a libFuzzer target to test the URI module.
 *
 * See Copyright for the status of this software.
 */

#include <libxml/uri.h>
#include "fuzz.h"

int
LLVMFuzzerInitialize(int *argc ATTRIBUTE_UNUSED,
                     char ***argv ATTRIBUTE_UNUSED) {
    xmlFuzzMemSetup();

    return 0;
}

int
LLVMFuzzerTestOneInput(const char *data, size_t size) {
    xmlURIPtr uri;
    size_t failurePos;
    const char *str1, *str2;
    char *copy;
    xmlChar *strRes;
    int intRes;

    if (size > 10000)
        return(0);

    xmlFuzzDataInit(data, size);
    failurePos = xmlFuzzReadInt(4) % (size * 8 + 100);
    str1 = xmlFuzzReadString(NULL);
    str2 = xmlFuzzReadString(NULL);

    xmlFuzzInjectFailure(failurePos);

    xmlFuzzResetFailure();
    intRes = xmlParseURISafe(str1, &uri);
    xmlFuzzCheckFailureReport("xmlParseURISafe", intRes == -1, 0);

    if (uri != NULL) {
        xmlFuzzResetFailure();
        strRes = xmlSaveUri(uri);
        xmlFuzzCheckFailureReport("xmlSaveURI", strRes == NULL, 0);
        xmlFree(strRes);
        xmlFreeURI(uri);
    }

    xmlFreeURI(xmlParseURI(str1));

    uri = xmlParseURIRaw(str1, 1);
    xmlFree(xmlSaveUri(uri));
    xmlFreeURI(uri);

    xmlFuzzResetFailure();
    strRes = BAD_CAST xmlURIUnescapeString(str1, -1, NULL);
    xmlFuzzCheckFailureReport("xmlURIUnescapeString",
                              str1 != NULL && strRes == NULL, 0);
    xmlFree(strRes);

    xmlFree(xmlURIEscape(BAD_CAST str1));

    xmlFuzzResetFailure();
    strRes = xmlCanonicPath(BAD_CAST str1);
    xmlFuzzCheckFailureReport("xmlCanonicPath",
                              str1 != NULL && strRes == NULL, 0);
    xmlFree(strRes);

    xmlFuzzResetFailure();
    strRes = xmlPathToURI(BAD_CAST str1);
    xmlFuzzCheckFailureReport("xmlPathToURI",
                              str1 != NULL && strRes == NULL, 0);
    xmlFree(strRes);

    xmlFuzzResetFailure();
    intRes = xmlBuildURISafe(BAD_CAST str2, BAD_CAST str1, &strRes);
    xmlFuzzCheckFailureReport("xmlBuildURISafe", intRes == -1, 0);
    xmlFree(strRes);

    xmlFree(xmlBuildURI(BAD_CAST str2, BAD_CAST str1));

    xmlFuzzResetFailure();
    intRes = xmlBuildRelativeURISafe(BAD_CAST str2, BAD_CAST str1, &strRes);
    xmlFuzzCheckFailureReport("xmlBuildRelativeURISafe", intRes == -1, 0);
    xmlFree(strRes);

    xmlFree(xmlBuildRelativeURI(BAD_CAST str2, BAD_CAST str1));

    xmlFuzzResetFailure();
    strRes = xmlURIEscapeStr(BAD_CAST str1, BAD_CAST str2);
    xmlFuzzCheckFailureReport("xmlURIEscapeStr",
                              str1 != NULL && strRes == NULL, 0);
    xmlFree(strRes);

    copy = (char *) xmlCharStrdup(str1);
    xmlNormalizeURIPath(copy);
    xmlFree(copy);

    xmlFuzzInjectFailure(0);
    xmlFuzzDataCleanup();

    return 0;
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

