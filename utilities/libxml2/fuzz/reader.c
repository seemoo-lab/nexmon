/*
 * reader.c: a libFuzzer target to test the XML Reader API.
 *
 * See Copyright for the status of this software.
 */

#include <libxml/catalog.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlsave.h>
#include "fuzz.h"

#include <string.h>

#if 0
  #define DEBUG
#endif

typedef enum {
    OP_READ = 1,
    OP_READ_INNER_XML,
    OP_READ_OUTER_XML,
    OP_READ_STRING,
    OP_READ_ATTRIBUTE_VALUE,
    OP_ATTRIBUTE_COUNT,
    OP_DEPTH,
    OP_HAS_ATTRIBUTES,
    OP_HAS_VALUE,
    OP_IS_DEFAULT,
    OP_IS_EMPTY_ELEMENT,
    OP_NODE_TYPE,
    OP_QUOTE_CHAR,
    OP_READ_STATE,
    OP_IS_NAMESPACE_DECL,
    OP_CONST_BASE_URI,
    OP_CONST_LOCAL_NAME,
    OP_CONST_NAME,
    OP_CONST_NAMESPACE_URI,
    OP_CONST_PREFIX,
    OP_CONST_XML_LANG,
    OP_CONST_VALUE,
    OP_BASE_URI,
    OP_LOCAL_NAME,
    OP_NAME,
    OP_NAMESPACE_URI,
    OP_PREFIX,
    OP_XML_LANG,
    OP_VALUE,
    OP_CLOSE,
    OP_GET_ATTRIBUTE_NO,
    OP_GET_ATTRIBUTE,
    OP_GET_ATTRIBUTE_NS,
    OP_GET_REMAINDER,
    OP_LOOKUP_NAMESPACE,
    OP_MOVE_TO_ATTRIBUTE_NO,
    OP_MOVE_TO_ATTRIBUTE,
    OP_MOVE_TO_ATTRIBUTE_NS,
    OP_MOVE_TO_FIRST_ATTRIBUTE,
    OP_MOVE_TO_NEXT_ATTRIBUTE,
    OP_MOVE_TO_ELEMENT,
    OP_NORMALIZATION,
    OP_CONST_ENCODING,
    OP_GET_PARSER_PROP,
    OP_CURRENT_NODE,
    OP_GET_PARSER_LINE_NUMBER,
    OP_GET_PARSER_COLUMN_NUMBER,
    OP_PRESERVE,
    OP_CURRENT_DOC,
    OP_EXPAND,
    OP_NEXT,
    OP_NEXT_SIBLING,
    OP_IS_VALID,
    OP_CONST_XML_VERSION,
    OP_STANDALONE,
    OP_BYTE_CONSUMED,

    OP_MAX
} opType;

static void
startOp(const char *name) {
    (void) name;
#ifdef DEBUG
    fprintf(stderr, "%s\n", name);
#endif
}

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
    xmlTextReaderPtr reader;
    xmlDocPtr doc = NULL;
    const xmlError *error;
    const char *docBuffer;
    const unsigned char *program;
    size_t failurePos, docSize, programSize, i;
    size_t totalStringSize = 0;
    int opts;
    int oomReport = 0;

    xmlFuzzDataInit(data, size);
    opts = (int) xmlFuzzReadInt(4);
    failurePos = xmlFuzzReadInt(4) % (size + 100);

    program = (const unsigned char *) xmlFuzzReadString(&programSize);
    if (programSize > 1000)
        programSize = 1000;

    xmlFuzzReadEntities();
    docBuffer = xmlFuzzMainEntity(&docSize);
    if (docBuffer == NULL)
        goto exit;

#ifdef DEBUG
    fprintf(stderr, "Input document (%d bytes):\n", (int) docSize);
    for (i = 0; (size_t) i < docSize; i++) {
        int c = (unsigned char) docBuffer[i];

        if ((c == '\n' || (c >= 0x20 && c <= 0x7E)))
            putc(c, stderr);
        else
            fprintf(stderr, "\\x%02X", c);
    }
    fprintf(stderr, "\nEOF\n");
#endif

    xmlFuzzInjectFailure(failurePos);
    reader = xmlReaderForMemory(docBuffer, docSize, NULL, NULL, opts);
    if (reader == NULL)
        goto exit;

    xmlTextReaderSetStructuredErrorHandler(reader, xmlFuzzSErrorFunc, NULL);
    xmlTextReaderSetResourceLoader(reader, xmlFuzzResourceLoader, NULL);

    i = 0;
    while (i < programSize) {
        int op = program[i++];

#define READ_BYTE() (i < programSize ? program[i++] : 0)
#define FREE_STRING(str) \
    do { \
        if (str != NULL) { \
            totalStringSize += strlen((char *) str); \
            xmlFree(str); \
        } \
    } while (0)

        switch (op & 0x3F) {
            case OP_READ:
            default:
                startOp("Read");
                xmlTextReaderRead(reader);
                break;

            case OP_READ_INNER_XML: {
                xmlChar *result;

                startOp("ReadInnerXml");
                result = xmlTextReaderReadInnerXml(reader);
                FREE_STRING(result);
                break;
            }

            case OP_READ_OUTER_XML: {
                xmlChar *result;

                startOp("ReadOuterXml");
                result = xmlTextReaderReadOuterXml(reader);
                FREE_STRING(result);
                break;
            }

            case OP_READ_STRING: {
                xmlChar *result;

                startOp("ReadString");
                result = xmlTextReaderReadString(reader);
                FREE_STRING(result);
                break;
            }

            case OP_READ_ATTRIBUTE_VALUE:
                startOp("ReadAttributeValue");
                xmlTextReaderReadAttributeValue(reader);
                break;

            case OP_ATTRIBUTE_COUNT:
                startOp("AttributeCount");
                xmlTextReaderAttributeCount(reader);
                break;

            case OP_DEPTH:
                startOp("Depth");
                xmlTextReaderDepth(reader);
                break;

            case OP_HAS_ATTRIBUTES:
                startOp("HasAttributes");
                xmlTextReaderHasAttributes(reader);
                break;

            case OP_HAS_VALUE:
                startOp("HasValue");
                xmlTextReaderHasValue(reader);
                break;

            case OP_IS_DEFAULT:
                startOp("IsDefault");
                xmlTextReaderIsDefault(reader);
                break;

            case OP_IS_EMPTY_ELEMENT:
                startOp("IsEmptyElement");
                xmlTextReaderIsEmptyElement(reader);
                break;

            case OP_NODE_TYPE:
                startOp("NodeType");
                xmlTextReaderNodeType(reader);
                break;

            case OP_QUOTE_CHAR:
                startOp("QuoteChar");
                xmlTextReaderQuoteChar(reader);
                break;

            case OP_READ_STATE:
                startOp("ReadState");
                xmlTextReaderReadState(reader);
                break;

            case OP_IS_NAMESPACE_DECL:
                startOp("IsNamespaceDecl");
                xmlTextReaderIsNamespaceDecl(reader);
                break;

            case OP_CONST_BASE_URI:
                startOp("ConstBaseUri");
                xmlTextReaderConstBaseUri(reader);
                break;

            case OP_CONST_LOCAL_NAME:
                startOp("ConstLocalName");
                xmlTextReaderConstLocalName(reader);
                break;

            case OP_CONST_NAME:
                startOp("ConstName");
                xmlTextReaderConstName(reader);
                break;

            case OP_CONST_NAMESPACE_URI:
                startOp("ConstNamespaceUri");
                xmlTextReaderConstNamespaceUri(reader);
                break;

            case OP_CONST_PREFIX:
                startOp("ConstPrefix");
                xmlTextReaderConstPrefix(reader);
                break;

            case OP_CONST_XML_LANG:
                startOp("ConstXmlLang");
                xmlTextReaderConstXmlLang(reader);
                oomReport = -1;
                break;

            case OP_CONST_VALUE:
                startOp("ConstValue");
                xmlTextReaderConstValue(reader);
                break;

            case OP_BASE_URI: {
                xmlChar *result;

                startOp("BaseUri");
                result = xmlTextReaderBaseUri(reader);
                FREE_STRING(result);
                break;
            }

            case OP_LOCAL_NAME: {
                xmlChar *result;

                startOp("LocalName");
                result = xmlTextReaderLocalName(reader);
                FREE_STRING(result);
                break;
            }

            case OP_NAME: {
                xmlChar *result;

                startOp("Name");
                result = xmlTextReaderName(reader);
                FREE_STRING(result);
                break;
            }

            case OP_NAMESPACE_URI: {
                xmlChar *result;

                startOp("NamespaceUri");
                result = xmlTextReaderNamespaceUri(reader);
                FREE_STRING(result);
                break;
            }

            case OP_PREFIX: {
                xmlChar *result;

                startOp("Prefix");
                result = xmlTextReaderPrefix(reader);
                FREE_STRING(result);
                break;
            }

            case OP_XML_LANG: {
                xmlChar *result;

                startOp("XmlLang");
                result = xmlTextReaderXmlLang(reader);
                oomReport = -1;
                FREE_STRING(result);
                break;
            }

            case OP_VALUE: {
                xmlChar *result;

                startOp("Value");
                result = xmlTextReaderValue(reader);
                FREE_STRING(result);
                break;
            }

            case OP_CLOSE:
                startOp("Close");
                if (doc == NULL)
                    doc = xmlTextReaderCurrentDoc(reader);
                xmlTextReaderClose(reader);
                break;

            case OP_GET_ATTRIBUTE_NO: {
                xmlChar *result;
                int no = READ_BYTE();

                startOp("GetAttributeNo");
                result = xmlTextReaderGetAttributeNo(reader, no);
                FREE_STRING(result);
                break;
            }

            case OP_GET_ATTRIBUTE: {
                const xmlChar *name = xmlTextReaderConstName(reader);
                xmlChar *result;

                startOp("GetAttribute");
                result = xmlTextReaderGetAttribute(reader, name);
                FREE_STRING(result);
                break;
            }

            case OP_GET_ATTRIBUTE_NS: {
                const xmlChar *localName, *namespaceUri;
                xmlChar *result;

                startOp("GetAttributeNs");
                localName = xmlTextReaderConstLocalName(reader);
                namespaceUri = xmlTextReaderConstNamespaceUri(reader);
                result = xmlTextReaderGetAttributeNs(reader, localName,
                                                     namespaceUri);
                FREE_STRING(result);
                break;
            }

            case OP_GET_REMAINDER:
                startOp("GetRemainder");
                if (doc == NULL)
                    doc = xmlTextReaderCurrentDoc(reader);
                xmlFreeParserInputBuffer(xmlTextReaderGetRemainder(reader));
                break;

            case OP_LOOKUP_NAMESPACE: {
                const xmlChar *prefix = xmlTextReaderConstPrefix(reader);
                xmlChar *result;

                startOp("LookupNamespace");
                result = xmlTextReaderLookupNamespace(reader, prefix);
                FREE_STRING(result);
                break;
            }

            case OP_MOVE_TO_ATTRIBUTE_NO: {
                int no = READ_BYTE();

                startOp("MoveToAttributeNo");
                xmlTextReaderMoveToAttributeNo(reader, no);
                break;
            }

            case OP_MOVE_TO_ATTRIBUTE: {
                const xmlChar *name = xmlTextReaderConstName(reader);

                startOp("MoveToAttribute");
                xmlTextReaderMoveToAttribute(reader, name);
                break;
            }

            case OP_MOVE_TO_ATTRIBUTE_NS: {
                const xmlChar *localName, *namespaceUri;

                startOp("MoveToAttributeNs");
                localName = xmlTextReaderConstLocalName(reader);
                namespaceUri = xmlTextReaderConstNamespaceUri(reader);
                xmlTextReaderMoveToAttributeNs(reader, localName,
                                               namespaceUri);
                break;
            }

            case OP_MOVE_TO_FIRST_ATTRIBUTE:
                startOp("MoveToFirstAttribute");
                xmlTextReaderMoveToFirstAttribute(reader);
                break;

            case OP_MOVE_TO_NEXT_ATTRIBUTE:
                startOp("MoveToNextAttribute");
                xmlTextReaderMoveToNextAttribute(reader);
                break;

            case OP_MOVE_TO_ELEMENT:
                startOp("MoveToElement");
                xmlTextReaderMoveToElement(reader);
                break;

            case OP_NORMALIZATION:
                startOp("Normalization");
                xmlTextReaderNormalization(reader);
                break;

            case OP_CONST_ENCODING:
                startOp("ConstEncoding");
                xmlTextReaderConstEncoding(reader);
                break;

            case OP_GET_PARSER_PROP: {
                int prop = READ_BYTE();

                startOp("GetParserProp");
                xmlTextReaderGetParserProp(reader, prop);
                break;
            }

            case OP_CURRENT_NODE:
                startOp("CurrentNode");
                xmlTextReaderCurrentNode(reader);
                break;

            case OP_GET_PARSER_LINE_NUMBER:
                startOp("GetParserLineNumber");
                xmlTextReaderGetParserLineNumber(reader);
                break;

            case OP_GET_PARSER_COLUMN_NUMBER:
                startOp("GetParserColumnNumber");
                xmlTextReaderGetParserColumnNumber(reader);
                break;

            case OP_PRESERVE:
                startOp("Preserve");
                xmlTextReaderPreserve(reader);
                break;

            case OP_CURRENT_DOC: {
                xmlDocPtr result;

                startOp("CurrentDoc");
                result = xmlTextReaderCurrentDoc(reader);
                if (doc == NULL)
                    doc = result;
                break;
            }

            case OP_EXPAND:
                startOp("Expand");
                xmlTextReaderExpand(reader);
                break;

            case OP_NEXT:
                startOp("Next");
                xmlTextReaderNext(reader);
                break;

            case OP_NEXT_SIBLING:
                startOp("NextSibling");
                xmlTextReaderNextSibling(reader);
                break;

            case OP_IS_VALID:
                startOp("IsValid");
                xmlTextReaderIsValid(reader);
                break;

            case OP_CONST_XML_VERSION:
                startOp("ConstXmlVersion");
                xmlTextReaderConstXmlVersion(reader);
                break;

            case OP_STANDALONE:
                startOp("Standalone");
                xmlTextReaderStandalone(reader);
                break;

            case OP_BYTE_CONSUMED:
                startOp("ByteConsumed");
                xmlTextReaderByteConsumed(reader);
                oomReport = -1;
                break;
        }

        if (totalStringSize > docSize * 2)
            break;
    }

    error = xmlTextReaderGetLastError(reader);
    if (error->code == XML_ERR_NO_MEMORY)
        oomReport = 1;
    xmlFuzzCheckFailureReport("reader", oomReport, error->code == XML_IO_EIO);

    xmlFreeTextReader(reader);

    if (doc != NULL)
        xmlFreeDoc(doc);

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

