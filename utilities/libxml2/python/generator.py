#!/usr/bin/env python3
#
# generate python wrappers from the XML API description
#

functions = {
    'xmlRegisterXPathFunction': (
        'Register a Python written function to the XPath interpreter',
        ['int', '1 in case of success, 0 or -1 in case of error', None],
        [['ctx', 'xmlXPathContext *', 'the xpathContext'], ['name', 'xmlChar *', 'the function name'], ['ns_uri', 'xmlChar *', 'the namespace or NULL'], ['f', 'pythonObject', 'the python function']],
        'python', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathRegisterVariable': (
        'Register a variable with the XPath context',
        ['int', '1 in case of success, 0 or -1 in case of error', None],
        [['ctx', 'xmlXPathContext *', 'the xpathContext'], ['name', 'xmlChar *', 'the variable name'], ['ns_uri', 'xmlChar *', 'the namespace or NULL'], ['value', 'pythonObject', 'the value']],
        'python', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlNewNode': (
        'Create a new Node',
        ['xmlNode *', 'A new element node', None],
        [['name', 'xmlChar *', 'the node name']],
        'python', None),
    'xmlCreatePushParser': (
        'Create a progressive XML parser context to build either an event flow if the SAX object is not None, or a DOM tree otherwise.',
        ['xmlParserCtxt *', 'the parser context or None in case of error', None],
        [['SAX', 'pythonObject', 'the SAX callback object or None'], ['chunk', 'xmlChar *', 'the initial data'], ['size', 'int', 'the size of the initial data'], ['URI', 'xmlChar *', 'The URI used for base computations']],
        'python', None),
    'htmlCreatePushParser': (
        'Create a progressive HTML parser context to build either an event flow if the SAX object is not None, or a DOM tree otherwise.',
        ['xmlParserCtxt *', 'the parser context or None in case of error', None],
        [['SAX', 'pythonObject', 'the SAX callback object or None'], ['chunk', 'xmlChar *', 'the initial data'], ['size', 'int', 'the size of the initial data'], ['URI', 'xmlChar *', 'The URI used for base computations']],
        'python', 'defined(LIBXML_HTML_ENABLED)'),
    'xmlSAXParseFile': (
        'Interface to parse an XML file or resource pointed by an URI to build an event flow to the SAX object',
        ['void', None, None],
        [['SAX', 'pythonObject', 'the SAX callback object or None'], ['URI', 'xmlChar *', 'The URI of the resource'], ['recover', 'int', 'allow recovery in case of error']],
        'python', None),
    'htmlSAXParseFile': (
        'Interface to parse an HTML file or resource pointed by an URI to build an event flow to the SAX object',
        ['void', None, None],
        [['SAX', 'pythonObject', 'the SAX callback object or None'], ['URI', 'xmlChar *', 'The URI of the resource'], ['encoding', 'const char *', 'encoding or None']],
        'python', 'defined(LIBXML_HTML_ENABLED)'),
    'xmlCreateOutputBuffer': (
        'Create a libxml2 output buffer from a Python file',
        ['xmlOutputBuffer *', 'the output buffer', None],
        [['file', 'pythonObject', 'the Python file'], ['encoding', 'xmlChar *', 'an optional encoding']],
        'python', None),
    'xmlCreateInputBuffer': (
        'Create a libxml2 input buffer from a Python file',
        ['xmlParserInputBuffer *', 'the input buffer', None],
        [['file', 'pythonObject', 'the Python file'], ['encoding', 'xmlChar *', 'an optional encoding']],
        'python', None),
    'xmlSetEntityLoader': (
        'Set the entity resolver as a python function',
        ['int', '0 in case of success, -1 for error', None],
        [['resolver', 'pythonObject', 'the Python function']],
        'python', None),
    'xmlParserGetDoc': (
        'Get the document tree from a parser context.',
        ['xmlDoc *', 'the document tree', 'myDoc'],
        [['ctxt', 'xmlParserCtxt *', 'the parser context']],
        'python_accessor', None),
    'xmlParserGetWellFormed': (
        'Get the well formed information from a parser context.',
        ['int', 'the wellFormed field', 'wellFormed'],
        [['ctxt', 'xmlParserCtxt *', 'the parser context']],
        'python_accessor', None),
    'xmlParserGetIsValid': (
        'Get the validity information from a parser context.',
        ['int', 'the valid field', 'valid'],
        [['ctxt', 'xmlParserCtxt *', 'the parser context']],
        'python_accessor', None),
    'xmlParserSetValidate': (
        'Switch the parser to validation mode.',
        ['void', None, None],
        [['ctxt', 'xmlParserCtxt *', 'the parser context'], ['validate', 'int', '1 to activate validation']],
        'python_accessor', None),
    'xmlParserSetReplaceEntities': (
        'Switch the parser to replace entities.',
        ['void', None, None],
        [['ctxt', 'xmlParserCtxt *', 'the parser context'], ['replaceEntities', 'int', '1 to replace entities']],
        'python_accessor', None),
    'xmlParserSetPedantic': (
        'Switch the parser to be pedantic.',
        ['void', None, None],
        [['ctxt', 'xmlParserCtxt *', 'the parser context'], ['pedantic', 'int', '1 to run in pedantic mode']],
        'python_accessor', None),
    'xmlParserSetLoadSubset': (
        'Switch the parser to load the DTD without validating.',
        ['void', None, None],
        [['ctxt', 'xmlParserCtxt *', 'the parser context'], ['loadsubset', 'int', '1 to load the DTD']],
        'python_accessor', None),
    'xmlParserSetLineNumbers': (
        'Switch on the generation of line number for elements nodes.',
        ['void', None, None],
        [['ctxt', 'xmlParserCtxt *', 'the parser context'], ['linenumbers', 'int', 'unused']],
        'python_accessor', None),
    'xmlDebugMemory': (
        'Switch on the generation of line number for elements nodes. Also returns the number of bytes allocated and not freed by libxml2 since memory debugging was switched on.',
        ['int', 'returns the number of bytes allocated and not freed', None],
        [['activate', 'int', '1 switch on memory debugging 0 switch it off']],
        'python', None),
    'xmlNodeGetNs': (
        'Get the namespace of a node',
        ['xmlNs *', 'The namespace or None', None],
        [['node', 'xmlNode *', 'the node']],
        'python_accessor', None),
    'xmlNodeGetNsDefs': (
        'Get the namespace of a node',
        ['xmlNs *', 'The namespace or None', None],
        [['node', 'xmlNode *', 'the node']],
        'python_accessor', None),
    'xmlXPathParserGetContext': (
        'Get the xpathContext from an xpathParserContext',
        ['xmlXPathContext *', 'The XPath context', 'context'],
        [['ctxt', 'xmlXPathParserContext *', 'the XPath parser context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathGetContextDoc': (
        'Get the doc from an xpathContext',
        ['xmlDoc *', 'The doc context', 'doc'],
        [['ctxt', 'xmlXPathContext *', 'the XPath context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathGetContextNode': (
        'Get the current node from an xpathContext',
        ['xmlNode *', 'The node context', 'node'],
        [['ctxt', 'xmlXPathContext *', 'the XPath context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathSetContextDoc': (
        'Set the doc of an xpathContext',
        ['void', None, None],
        [['ctxt', 'xmlXPathContext *', 'the XPath context'], ['doc', 'xmlDoc *', 'The doc context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathSetContextNode': (
        'Set the current node of an xpathContext',
        ['void', None, None],
        [['ctxt', 'xmlXPathContext *', 'the XPath context'], ['node', 'xmlNode *', 'The node context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathGetContextPosition': (
        'Get the current node from an xpathContext',
        ['int', 'The node context', 'proximityPosition'],
        [['ctxt', 'xmlXPathContext *', 'the XPath context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathGetContextSize': (
        'Get the current node from an xpathContext',
        ['int', 'The node context', 'contextSize'],
        [['ctxt', 'xmlXPathContext *', 'the XPath context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathGetFunction': (
        'Get the current function name xpathContext',
        ['const xmlChar *', 'The function name', 'function'],
        [['ctxt', 'xmlXPathContext *', 'the XPath context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlXPathGetFunctionURI': (
        'Get the current function name URI xpathContext',
        ['const xmlChar *', 'The function name URI', 'functionURI'],
        [['ctxt', 'xmlXPathContext *', 'the XPath context']],
        'python_accessor', 'defined(LIBXML_XPATH_ENABLED)'),
    'xmlURIGetScheme': (
        'Get the scheme part from an URI',
        ['const char *', 'The URI scheme', 'scheme'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetScheme': (
        'Set the scheme part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['scheme', 'char *', 'The URI scheme part']],
        'python_accessor', None),
    'xmlURIGetOpaque': (
        'Get the opaque part from an URI',
        ['const char *', 'The URI opaque', 'opaque'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetOpaque': (
        'Set the opaque part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['opaque', 'char *', 'The URI opaque part']],
        'python_accessor', None),
    'xmlURIGetAuthority': (
        'Get the authority part from an URI',
        ['const char *', 'The URI authority', 'authority'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetAuthority': (
        'Set the authority part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['authority', 'char *', 'The URI authority part']],
        'python_accessor', None),
    'xmlURIGetServer': (
        'Get the server part from an URI',
        ['const char *', 'The URI server', 'server'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetServer': (
        'Set the server part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['server', 'char *', 'The URI server part']],
        'python_accessor', None),
    'xmlURIGetUser': (
        'Get the user part from an URI',
        ['const char *', 'The URI user', 'user'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetUser': (
        'Set the user part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['user', 'char *', 'The URI user part']],
        'python_accessor', None),
    'xmlURIGetPath': (
        'Get the path part from an URI',
        ['const char *', 'The URI path', 'path'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetPath': (
        'Set the path part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['path', 'char *', 'The URI path part']],
        'python_accessor', None),
    'xmlURIGetQuery': (
        'Get the query part from an URI',
        ['const char *', 'The URI query', 'query'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetQuery': (
        'Set the query part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['query', 'char *', 'The URI query part']],
        'python_accessor', None),
    'xmlURIGetQueryRaw': (
        'Get the raw query part from an URI (i.e. the unescaped form).',
        ['const char *', 'The URI query', 'query_raw'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetQueryRaw': (
        'Set the raw query part of an URI (i.e. the unescaped form).',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['query_raw', 'char *', 'The raw URI query part']],
        'python_accessor', None),
    'xmlURIGetFragment': (
        'Get the fragment part from an URI',
        ['const char *', 'The URI fragment', 'fragment'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetFragment': (
        'Set the fragment part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['fragment', 'char *', 'The URI fragment part']],
        'python_accessor', None),
    'xmlURIGetPort': (
        'Get the port part from an URI',
        ['int', 'The URI port', 'port'],
        [['URI', 'xmlURI *', 'the URI']],
        'python_accessor', None),
    'xmlURISetPort': (
        'Set the port part of an URI.',
        ['void', None, None],
        [['URI', 'xmlURI *', 'the URI'], ['port', 'int', 'The URI port part']],
        'python_accessor', None),
    'xmlErrorGetDomain': (
        'What part of the library raised this error',
        ['int', 'The error domain', 'domain'],
        [['Error', 'xmlError *', 'the Error']],
        'python_accessor', None),
    'xmlErrorGetCode': (
        'The error code, e.g. an xmlParserError',
        ['int', 'The error code', 'code'],
        [['Error', 'xmlError *', 'the Error']],
        'python_accessor', None),
    'xmlErrorGetMessage': (
        'human-readable informative error message',
        ['const char *', 'The error message', 'message'],
        [['Error', 'xmlError *', 'the Error']],
        'python_accessor', None),
    'xmlErrorGetLevel': (
        'how consequent is the error',
        ['int', 'The error level', 'level'],
        [['Error', 'xmlError *', 'the Error']],
        'python_accessor', None),
    'xmlErrorGetFile': (
        'the filename',
        ['const char *', 'The error file', 'file'],
        [['Error', 'xmlError *', 'the Error']],
        'python_accessor', None),
    'xmlErrorGetLine': (
        'the line number if available',
        ['int', 'The error line', 'line'],
        [['Error', 'xmlError *', 'the Error']],
        'python_accessor', None),
    'xmlPythonCleanupParser': (
        "Cleanup function for the XML library. It tries to reclaim all parsing related global memory allocated for the library processing. It doesn't deallocate any document related memory. Calling this function should not prevent reusing the library but one should call xmlCleanupParser() only when the process has finished using the library or XML document built with it.",
        ['void', None, None],
        [],
        'python', None),
    'xmlMemoryUsed': (
        'Returns the total amount of memory allocated by libxml2',
        ['int', 'number of bytes allocated', None],
        [],
        'python', None)
}

enums = {} # { enumType: { enumConstant: enumValue } }

import os
import sys
import string

if __name__ == "__main__":
    # launched as a script
    srcPref = os.path.dirname(sys.argv[0])
    try:
        dstPref = sys.argv[1]
    except IndexError:
        dstPref = os.getcwd()
else:
    # imported
    srcPref = os.path.dirname(__file__)
    dstPref = os.getcwd()

#######################################################################
#
#  Some filtering rukes to drop functions/types which should not
#  be exposed as-is on the Python interface
#
#######################################################################

skipped_modules = {
    'xmlmemory': None,
    'SAX': None,
    'hash': None,
    'list': None,
    'threads': None,
#    'xpointer': None,
}
skipped_types = {
    'int *': "usually a return type",
    'xmlSAXHandler *': "not the proper interface for SAX",
    'htmlSAXHandler *': "not the proper interface for SAX",
    'xmlRMutex *': "thread specific, skipped",
    'xmlMutex *': "thread specific, skipped",
    'xmlGlobalState *': "thread specific, skipped",
    'xmlList *': "internal representation not suitable for python",
    'xmlBuffer *': "internal representation not suitable for python",
    'FILE *': None,
}

#######################################################################
#
#  That part if purely the API acquisition phase from the
#  XML API description
#
#######################################################################
import os
import xml.etree.ElementTree as etree

sys.path.append(srcPref + '/../codegen')
import xmlmod

xmlDocDir = dstPref + '/../doc/xml'
if not os.path.isdir(xmlDocDir):
    xmlDocDir = dstPref + '/doc/xml'
    if not os.path.isdir(xmlDocDir):
        raise Exception('Doxygen XML not found in %s' % dstPref)

def extractDocs(node):
    text = ''

    if node.text is not None:
        text = node.text.strip()
        if text == 'Deprecated':
            text = 'DEPRECATED:'

    i = 0
    n = len(node)
    for child in node:
        i += 1

        if (child.tag != 'parameterlist' and
            (child.tag != 'simplesect' or child.get('kind') != 'return')):
            childtext = extractDocs(child)
            if childtext != '':
                if text != '':
                    text += ' '
                text += childtext

        tail = child.tail
        if tail is not None:
            tail = tail.strip()
            if tail != '':
                if text != '':
                    text += ' '
                text += child.tail.strip()

    return text

for file in os.listdir(xmlDocDir):
    if not file.endswith('_8h.xml'):
        continue

    doc = etree.parse(xmlDocDir + '/' + file)

    compound = doc.find('compounddef')
    module = compound.find('compoundname').text
    if not module.endswith('.h'):
        continue
    module = module[:-2]
    if module in skipped_modules:
        continue

    for section in compound.findall('sectiondef'):
        kind = section.get('kind')

        if kind == 'func':
            for func in section.findall('memberdef'):
                name = func.find('name').text
                if name in functions:
                    continue

                docs = extractDocs(func.find('detaileddescription'))

                # python 2.7 compat, https://gitlab.gnome.org/GNOME/libxml2/-/issues/1092
                encoding = 'unicode'
                if sys.version_info[0] == 2:
                    encoding = 'utf-8'

                rtype = etree.tostring(func.find('type'),
                    method='text', encoding=encoding).rstrip()

                valid = True
                args = []
                for arg in func.findall('param'):
                    atype = etree.tostring(arg.find('type'),
                        method='text', encoding=encoding).rstrip()
                    if atype == 'void':
                        continue

                    aname = arg.find('declname')
                    if aname is None:
                        valid = False
                        break

                    args.append([aname.text, atype])

                if not valid:
                    continue

                module1, module2 = xmlmod.findModules(module, name)

                cond = None
                if module1 != '':
                    cond = 'defined(LIBXML_%s_ENABLED)' % module1
                if module2 != '':
                    cond += ' && defined(LIBXML_%s_ENABLED)' % module2

                functions[name] = (docs, [rtype], args, module, cond)
        elif kind == 'enum':
            for enum in section.findall('memberdef'):
                name = enum.find('name').text
                edict = {}
                enums[name] = edict
                prev = -1

                for value in enum.findall('enumvalue'):
                    ename = value.find('name').text

                    init = value.find('initializer')
                    if init is None:
                        evalue = prev + 1
                    else:
                        evalue = init.text.lstrip()
                        if evalue[0] != '=':
                            raise Exception('invalid init value %s' % init)
                        evalue = eval(evalue[1:].strip())

                    edict[ename] = evalue
                    prev = evalue

#######################################################################
#
#  Table of remapping to/from the python type or class to the C
#  counterpart.
#
#######################################################################

py_types = {
    'void': (None, None, None, None),
    'int':  ('i', None, "int", "int"),
    'long':  ('l', None, "long", "long"),
    'double':  ('d', None, "double", "double"),
    'unsigned int':  ('i', None, "int", "int"),
    'xmlChar':  ('c', None, "int", "int"),
    'unsigned char *':  ('z', None, "charPtr", "char *"),
    'char *':  ('z', None, "charPtr", "char *"),
    'const char *':  ('z', None, "charPtrConst", "const char *"),
    'xmlChar *':  ('z', None, "xmlCharPtr", "xmlChar *"),
    'const xmlChar *':  ('z', None, "xmlCharPtrConst", "const xmlChar *"),
    'xmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'const xmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'xmlDtd *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'const xmlDtd *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'xmlAttr *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'const xmlAttr *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'xmlEntity *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'const xmlEntity *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'xmlElement *':  ('O', "xmlElement", "xmlElementPtr", "xmlElement *"),
    'const xmlElement *':  ('O', "xmlElement", "xmlElementPtr", "xmlElement *"),
    'xmlAttribute *':  ('O', "xmlAttribute", "xmlAttributePtr", "xmlAttribute *"),
    'const xmlAttribute *':  ('O', "xmlAttribute", "xmlAttributePtr", "xmlAttribute *"),
    'xmlNs *':  ('O', "xmlNode", "xmlNsPtr", "xmlNs *"),
    'const xmlNs *':  ('O', "xmlNode", "xmlNsPtr", "xmlNs *"),
    'xmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDoc *"),
    'const xmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDoc *"),
    'htmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDoc *"),
    'const htmlDoc *':  ('O', "xmlNode", "xmlDocPtr", "xmlDoc *"),
    'htmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'const htmlNode *':  ('O', "xmlNode", "xmlNodePtr", "xmlNode *"),
    'xmlXPathContext *':  ('O', "xmlXPathContext", "xmlXPathContextPtr", "xmlXPathContext *"),
    'xmlXPathParserContext *':  ('O', "xmlXPathParserContext", "xmlXPathParserContextPtr", "xmlXPathParserContext *"),
    'xmlParserCtxt *': ('O', "parserCtxt", "xmlParserCtxtPtr", "xmlParserCtxt *"),
    'htmlParserCtxt *': ('O', "parserCtxt", "xmlParserCtxtPtr", "xmlParserCtxt *"),
    'xmlValidCtxt *': ('O', "ValidCtxt", "xmlValidCtxtPtr", "xmlValidCtxt *"),
    'xmlCatalog *': ('O', "catalog", "xmlCatalogPtr", "xmlCatalog *"),
    'FILE *': ('O', "File", "FILEPtr", "FILE *"),
    'xmlURI *': ('O', "URI", "xmlURIPtr", "xmlURI *"),
    'const xmlError *': ('O', "Error", "xmlErrorPtr", "const xmlError *"),
    'xmlError *': ('O', "Error", "xmlErrorPtr", "xmlError *"),
    'xmlOutputBuffer *': ('O', "outputBuffer", "xmlOutputBufferPtr", "xmlOutputBuffer *"),
    'xmlParserInputBuffer *': ('O', "inputBuffer", "xmlParserInputBufferPtr", "xmlParserInputBuffer *"),
    'xmlRegexp *': ('O', "xmlReg", "xmlRegexpPtr", "xmlRegexp *"),
    'xmlTextReaderLocatorPtr': ('O', "xmlTextReaderLocator", "xmlTextReaderLocatorPtr", "xmlTextReaderLocatorPtr"),
    'xmlTextReader *': ('O', "xmlTextReader", "xmlTextReaderPtr", "xmlTextReader *"),
    'xmlRelaxNG *': ('O', "relaxNgSchema", "xmlRelaxNGPtr", "xmlRelaxNG *"),
    'xmlRelaxNGParserCtxt *': ('O', "relaxNgParserCtxt", "xmlRelaxNGParserCtxtPtr", "xmlRelaxNGParserCtxt *"),
    'xmlRelaxNGValidCtxt *': ('O', "relaxNgValidCtxt", "xmlRelaxNGValidCtxtPtr", "xmlRelaxNGValidCtxt *"),
    'xmlSchema *': ('O', "Schema", "xmlSchemaPtr", "xmlSchema *"),
    'xmlSchemaParserCtxt *': ('O', "SchemaParserCtxt", "xmlSchemaParserCtxtPtr", "xmlSchemaParserCtxt *"),
    'xmlSchemaValidCtxt *': ('O', "SchemaValidCtxt", "xmlSchemaValidCtxtPtr", "xmlSchemaValidCtxt *"),
}

py_return_types = {
    'xmlXPathObject *':  ('O', "foo", "xmlXPathObjectPtr", "xmlXPathObject *"),
}

unknown_types = {}

foreign_encoding_args = (
    'htmlCreateMemoryParserCtxt',
    'htmlCtxtReadMemory',
    'htmlParseChunk',
    'htmlReadMemory',
    'xmlCreateMemoryParserCtxt',
    'xmlCtxtReadMemory',
    'xmlCtxtResetPush',
    'xmlParseChunk',
    'xmlParseMemory',
    'xmlReadMemory',
    'xmlRecoverMemory',
)

#######################################################################
#
#  This part writes the C <-> Python stubs libxml2-py.[ch] and
#  the table libxml2-export.c to add when registrering the Python module
#
#######################################################################

# Class methods which are written by hand in libxml.c but the Python-level
# code is still automatically generated (so they are not in skip_function()).
skip_impl = (
    'xmlSaveFileTo',
    'xmlSaveFormatFileTo',
)

deprecated_funcs = {
    'htmlAutoCloseTag': True,
    'htmlDefaultSAXHandlerInit': True,
    'htmlHandleOmittedElem': True,
    'htmlInitAutoClose': True,
    'htmlIsAutoClosed': True,
    'htmlIsBooleanAttr': True,
    'htmlIsScriptAttribute': True,
    'htmlParseCharRef': True,
    'htmlParseElement': True,
    'xmlACatalogAdd': True,
    'xmlACatalogDump': True,
    'xmlACatalogRemove': True,
    'xmlACatalogResolve': True,
    'xmlACatalogResolvePublic': True,
    'xmlACatalogResolveSystem': True,
    'xmlACatalogResolveURI': True,
    'xmlAddEncodingAlias': True,
    'xmlByteConsumed': True,
    'xmlCatalogConvert': True,
    'xmlCatalogGetPublic': True,
    'xmlCatalogGetSystem': True,
    'xmlCatalogIsEmpty': True,
    'xmlCheckFilename': True,
    'xmlCheckLanguageID': True,
    'xmlCleanupCharEncodingHandlers': True,
    'xmlCleanupEncodingAliases': True,
    'xmlCleanupGlobals': True,
    'xmlClearParserCtxt': True,
    'xmlConvertSGMLCatalog': True,
    'xmlCopyChar': True,
    'xmlCopyCharMultiByte': True,
    'xmlCreateEntityParserCtxt': True,
    'xmlDefaultSAXHandlerInit': True,
    'xmlDelEncodingAlias': True,
    'xmlDictCleanup': True,
    'xmlFileMatch': True,
    'xmlFreeCatalog': True,
    'xmlGetCompressMode': True,
    'xmlGetEncodingAlias': True,
    'xmlInitCharEncodingHandlers': True,
    'xmlInitGlobals': True,
    'xmlInitializeDict': True,
    'xmlIOFTPMatch': True,
    'xmlIOHTTPMatch': True,
    'xmlIsBaseChar': True,
    'xmlIsBlank': True,
    'xmlIsChar': True,
    'xmlIsCombining': True,
    'xmlIsDigit': True,
    'xmlIsExtender': True,
    'xmlIsIdeographic': True,
    'xmlIsLetter': True,
    'xmlIsMixedElement': True,
    'xmlIsPubidChar': True,
    'xmlIsRef': True,
    'xmlKeepBlanksDefault': True,
    'xmlLineNumbersDefault': True,
    'xmlLoadACatalog': True,
    'xmlLoadSGMLSuperCatalog': True,
    'xmlNanoHTTPCleanup': True,
    'xmlNanoHTTPInit': True,
    'xmlNanoHTTPScanProxy': True,
    'xmlNewCatalog': True,
    'xmlNextChar': True,
    'xmlNormalizeWindowsPath': True,
    'xmlParseAttValue': True,
    'xmlParseAttributeListDecl': True,
    'xmlParseCDSect': True,
    'xmlParseCatalogFile': True,
    'xmlParseCharData': True,
    'xmlParseCharRef': True,
    'xmlParseComment': True,
    'xmlParseDocTypeDecl': True,
    'xmlParseElement': True,
    'xmlParseElementDecl': True,
    'xmlParseEncName': True,
    'xmlParseEncodingDecl': True,
    'xmlParseEndTag': True,
    'xmlParseEntity': True,
    'xmlParseEntityDecl': True,
    'xmlParseEntityRef': True,
    'xmlParseExtParsedEnt': True,
    'xmlParseExternalSubset': True,
    'xmlParseMarkupDecl': True,
    'xmlParseMisc': True,
    'xmlParseName': True,
    'xmlParseNmtoken': True,
    'xmlParseNotationDecl': True,
    'xmlParsePEReference': True,
    'xmlParsePI': True,
    'xmlParsePITarget': True,
    'xmlParsePubidLiteral': True,
    'xmlParseReference': True,
    'xmlParseSDDecl': True,
    'xmlParseStartTag': True,
    'xmlParseSystemLiteral': True,
    'xmlParseTextDecl': True,
    'xmlParseVersionInfo': True,
    'xmlParseVersionNum': True,
    'xmlParseXMLDecl': True,
    'xmlParserHandlePEReference': True,
    'xmlParserInputBufferGrow': True,
    'xmlParserInputBufferPush': True,
    'xmlParserInputBufferRead': True,
    'xmlParserSetLineNumbers': True,
    'xmlParserSetLoadSubset': True,
    'xmlParserSetPedantic': True,
    'xmlParserSetReplaceEntities': True,
    'xmlParserSetValidate': True,
    'xmlPedanticParserDefault': True,
    'xmlPopInput': True,
    'xmlRecoverDoc': True,
    'xmlRecoverFile': True,
    'xmlRecoverMemory': True,
    'xmlRegexpPrint': True,
    'xmlRegisterHTTPPostCallbacks': True,
    'xmlRelaxNGCleanupTypes': True,
    'xmlRelaxNGInitTypes': True,
    'xmlRemoveRef': True,
    'xmlSAXDefaultVersion': True,
    'xmlSchemaCleanupTypes': True,
    'xmlSchemaInitTypes': True,
    'xmlSetCompressMode': True,
    'xmlSetupParserForBuffer': True,
    'xmlSkipBlankChars': True,
    'xmlStringDecodeEntities': True,
    'xmlStringLenDecodeEntities': True,
    'xmlSubstituteEntitiesDefault': True,
    'xmlThrDefDoValidityCheckingDefaultValue': True,
    'xmlThrDefGetWarningsDefaultValue': True,
    'xmlThrDefIndentTreeOutput': True,
    'xmlThrDefKeepBlanksDefaultValue': True,
    'xmlThrDefLineNumbersDefaultValue': True,
    'xmlThrDefLoadExtDtdDefaultValue': True,
    'xmlThrDefPedanticParserDefaultValue': True,
    'xmlThrDefSaveNoEmptyTags': True,
    'xmlThrDefSubstituteEntitiesDefaultValue': True,
    'xmlThrDefTreeIndentString': True,
    'xmlValidCtxtNormalizeAttributeValue': True,
    'xmlValidNormalizeAttributeValue': True,
    'xmlValidateAttributeValue': True,
    'xmlValidateDocumentFinal': True,
    'xmlValidateDtdFinal': True,
    'xmlValidateNotationUse': True,
    'xmlValidateOneAttribute': True,
    'xmlValidateOneElement': True,
    'xmlValidateOneNamespace': True,
    'xmlValidatePopElement': True,
    'xmlValidatePushCData': True,
    'xmlValidatePushElement': True,
    'xmlValidateRoot': True,
    'xmlValidate': True,
    'xmlXPathEvalExpr': True,
    'xmlXPathInit': True,
    'xmlXPtrEvalRangePredicate': True,
    'xmlXPtrNewCollapsedRange': True,
    'xmlXPtrNewContext': True,
    'xmlXPtrNewLocationSetNodes': True,
    'xmlXPtrNewRange': True,
    'xmlXPtrNewRangeNodes': True,
    'xmlXPtrRangeToFunction': True,
}

def skip_function(name):
    if name[0:12] == "xmlXPathWrap":
        return 1
    if name == "xmlFreeParserCtxt":
        return 1
    if name == "xmlCleanupParser":
        return 1
    if name == "xmlFreeTextReader":
        return 1
#    if name[0:11] == "xmlXPathNew":
#        return 1
    # the next function is defined in libxml.c
    if name == "xmlRelaxNGFreeValidCtxt":
        return 1
    if name == "xmlFreeValidCtxt":
        return 1
    if name == "xmlSchemaFreeValidCtxt":
        return 1
    if name[0:5] == "__xml":
        return 1

#
# Those are skipped because the Const version is used of the bindings
# instead.
#
    if name == "xmlTextReaderBaseUri":
        return 1
    if name == "xmlTextReaderLocalName":
        return 1
    if name == "xmlTextReaderName":
        return 1
    if name == "xmlTextReaderNamespaceUri":
        return 1
    if name == "xmlTextReaderPrefix":
        return 1
    if name == "xmlTextReaderXmlLang":
        return 1
    if name == "xmlTextReaderValue":
        return 1
    if name == "xmlOutputBufferClose": # handled by by the superclass
        return 1
    if name == "xmlOutputBufferFlush": # handled by by the superclass
        return 1
    if name == "xmlErrMemory":
        return 1

    if name == "xmlValidBuildContentModel":
        return 1
    if name == "xmlValidateElementDecl":
        return 1
    if name == "xmlValidateAttributeDecl":
        return 1
    if name == "xmlPopInputCallbacks":
        return 1

    return 0

def print_function_wrapper(name, output, export, include):
    global py_types
    global unknown_types
    global functions
    global skipped_modules

    try:
        (desc, ret, args, file, cond) = functions[name]
    except:
        print("failed to get function %s infos")
        return

    if file in skipped_modules:
        return 0
    if skip_function(name) == 1:
        return 0
    if name in skip_impl:
        # Don't delete the function entry in the caller.
        return 1

    if name.startswith('xmlUCSIs'):
        is_deprecated = name != 'xmlUCSIsBlock' and name != 'xmlUCSIsCat'
    else:
        is_deprecated = name in deprecated_funcs

    c_call = ""
    format=""
    format_args=""
    c_args=""
    c_return=""
    c_convert=""
    c_release=""
    num_bufs=0
    for arg in args:
        # This should be correct
        if arg[1][0:6] == "const ":
            arg[1] = arg[1][6:]
        c_args = c_args + "    %s %s;\n" % (arg[1], arg[0])
        if arg[1] in py_types:
            (f, t, n, c) = py_types[arg[1]]
            if (f == 'z') and (name in foreign_encoding_args) and (num_bufs == 0):
                f = 's#'
            if f != None:
                format = format + f
            if t != None:
                format_args = format_args + ", &pyobj_%s" % (arg[0])
                c_args = c_args + "    PyObject *pyobj_%s;\n" % (arg[0])
                c_convert = c_convert + \
                   "    %s = (%s) Py%s_Get(pyobj_%s);\n" % (arg[0],
                   arg[1], t, arg[0])
            else:
                format_args = format_args + ", &%s" % (arg[0])
            if f == 's#':
                format_args = format_args + ", &py_buffsize%d" % num_bufs
                c_args = c_args + "    Py_ssize_t  py_buffsize%d;\n" % num_bufs
                num_bufs = num_bufs + 1
            if c_call != "":
                c_call = c_call + ", "
            c_call = c_call + "%s" % (arg[0])
            if t == "File":
                c_release = c_release + \
		            "    PyFile_Release(%s);\n" % (arg[0])
        else:
            if arg[1] in skipped_types:
                return 0
            if arg[1] in unknown_types:
                lst = unknown_types[arg[1]]
                lst.append(name)
            else:
                unknown_types[arg[1]] = [name]
            return -1
    if format != "":
        format = format + ":%s" % (name)

    if ret[0] == 'void':
        if file == "python_accessor":
            if args[1][1] == "char *" or args[1][1] == "xmlChar *":
                c_call = "\n    if (%s->%s != NULL) xmlFree(%s->%s);\n" % (
                                 args[0][0], args[1][0], args[0][0], args[1][0])
                c_call = c_call + "    %s->%s = (%s)xmlStrdup((const xmlChar *)%s);\n" % (args[0][0],
                                 args[1][0], args[1][1], args[1][0])
            else:
                c_call = "\n    %s->%s = %s;\n" % (args[0][0], args[1][0],
                                                   args[1][0])
        else:
            c_call = "\n    %s(%s);\n" % (name, c_call)
        ret_convert = "    Py_INCREF(Py_None);\n    return(Py_None);\n"
    elif ret[0] in py_types:
        (f, t, n, c) = py_types[ret[0]]
        c_return = c_return + "    %s c_retval;\n" % (ret[0])
        if file == "python_accessor" and ret[2] != None:
            c_call = "\n    c_retval = %s->%s;\n" % (args[0][0], ret[2])
        else:
            c_call = "\n    c_retval = %s(%s);\n" % (name, c_call)
        ret_convert = "    py_retval = libxml_%sWrap((%s) c_retval);\n" % (n,c)
        ret_convert = ret_convert + "    return(py_retval);\n"
    elif ret[0] in py_return_types:
        (f, t, n, c) = py_return_types[ret[0]]
        c_return = c_return + "    %s c_retval;\n" % (ret[0])
        c_call = "\n    c_retval = %s(%s);\n" % (name, c_call)
        ret_convert = "    py_retval = libxml_%sWrap((%s) c_retval);\n" % (n,c)
        ret_convert = ret_convert + "    return(py_retval);\n"
    else:
        if ret[0] in skipped_types:
            return 0
        if ret[0] in unknown_types:
            lst = unknown_types[ret[0]]
            lst.append(name)
        else:
            unknown_types[ret[0]] = [name]
        return -1

    if cond != None and cond != "":
        include.write("#if %s\n" % cond)
        export.write("#if %s\n" % cond)
        output.write("#if %s\n" % cond)

    include.write("PyObject * ")
    include.write("libxml_%s(PyObject *self, PyObject *args);\n" % (name))

    export.write("    { \"%s\", libxml_%s, METH_VARARGS, NULL },\n" %
                 (name, name))

    if file == "python":
        # Those have been manually generated
        if cond != None and cond != "":
            include.write("#endif\n")
            export.write("#endif\n")
            output.write("#endif\n")
        return 1
    if file == "python_accessor" and ret[0] != "void" and ret[2] is None:
        # Those have been manually generated
        if cond != None and cond != "":
            include.write("#endif\n")
            export.write("#endif\n")
            output.write("#endif\n")
        return 1

    if is_deprecated:
        output.write("XML_IGNORE_DEPRECATION_WARNINGS\n")
    output.write("PyObject *\n")
    output.write("libxml_%s(PyObject *self ATTRIBUTE_UNUSED," % (name))
    output.write(" PyObject *args")
    if format == "":
        output.write(" ATTRIBUTE_UNUSED")
    output.write(") {\n")
    if ret[0] != 'void':
        output.write("    PyObject *py_retval;\n")
    if c_return != "":
        output.write(c_return)
    if c_args != "":
        output.write(c_args)
    if is_deprecated:
        output.write("\n    if (libxml_deprecationWarning(\"%s\") == -1)\n" %
                     name)
        output.write("        return(NULL);\n")
    if format != "":
        output.write("\n    if (!PyArg_ParseTuple(args, \"%s\"%s))\n" %
                     (format, format_args))
        output.write("        return(NULL);\n")
    if c_convert != "":
        output.write(c_convert)

    output.write(c_call)
    if c_release != "":
        output.write(c_release)
    output.write(ret_convert)
    output.write("}\n")
    if is_deprecated:
        output.write("XML_POP_WARNINGS\n")
    output.write("\n")

    if cond != None and cond != "":
        include.write("#endif /* %s */\n" % cond)
        export.write("#endif /* %s */\n" % cond)
        output.write("#endif /* %s */\n" % cond)
    return 1

def buildStubs():
    global py_types
    global py_return_types
    global unknown_types

    py_types['pythonObject'] = ('O', "pythonObject", "pythonObject", "pythonObject")
    nb_wrap = 0
    failed = 0
    skipped = 0

    include = open(os.path.join(dstPref, "libxml2-py.h"), "w")
    include.write("/* Generated */\n\n")
    export = open(os.path.join(dstPref, "libxml2-export.c"), "w")
    export.write("/* Generated */\n\n")
    wrapper = open(os.path.join(dstPref, "libxml2-py.c"), "w")
    wrapper.write("/* Generated */\n\n")
    wrapper.write("#define PY_SSIZE_T_CLEAN\n")
    wrapper.write("#include <Python.h>\n")
    wrapper.write("#include <libxml/xmlversion.h>\n")
    wrapper.write("#include <libxml/tree.h>\n")
    wrapper.write("#include <libxml/xmlschemastypes.h>\n")
    wrapper.write("#include \"libxml_wrap.h\"\n")
    wrapper.write("#include \"libxml2-py.h\"\n\n")
    for function in sorted(functions.keys()):
        ret = print_function_wrapper(function, wrapper, export, include)
        if ret < 0:
            failed = failed + 1
            del functions[function]
        if ret == 0:
            skipped = skipped + 1
            del functions[function]
        if ret == 1:
            nb_wrap = nb_wrap + 1
    include.close()
    export.close()
    wrapper.close()

#    print("Generated %d wrapper functions, %d failed, %d skipped" % (nb_wrap,
#                                                              failed, skipped))
#    print("Missing type converters: ")
#    for type in list(unknown_types.keys()):
#        print("%s:%d " % (type, len(unknown_types[type])))
#    print()

#######################################################################
#
#  This part writes part of the Python front-end classes based on
#  mapping rules between types and classes and also based on function
#  renaming to get consistent function names at the Python level
#
#######################################################################

#
# The type automatically remapped to generated classes
#
classes_type = {
    "xmlNode *": ("._o", "xmlNode(_obj=%s)", "xmlNode"),
    "xmlDoc *": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "htmlDoc *": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "htmlxmlDoc * *": ("._o", "xmlDoc(_obj=%s)", "xmlDoc"),
    "xmlAttr *": ("._o", "xmlAttr(_obj=%s)", "xmlAttr"),
    "xmlNs *": ("._o", "xmlNs(_obj=%s)", "xmlNs"),
    "xmlDtd *": ("._o", "xmlDtd(_obj=%s)", "xmlDtd"),
    "xmlEntity *": ("._o", "xmlEntity(_obj=%s)", "xmlEntity"),
    "xmlElement *": ("._o", "xmlElement(_obj=%s)", "xmlElement"),
    "xmlAttribute *": ("._o", "xmlAttribute(_obj=%s)", "xmlAttribute"),
    "xmlXPathContext *": ("._o", "xpathContext(_obj=%s)", "xpathContext"),
    "xmlXPathParserContext *": ("._o", "xpathParserContext(_obj=%s)", "xpathParserContext"),
    "xmlParserCtxt *": ("._o", "parserCtxt(_obj=%s)", "parserCtxt"),
    "htmlParserCtxt *": ("._o", "parserCtxt(_obj=%s)", "parserCtxt"),
    "xmlValidCtxt *": ("._o", "ValidCtxt(_obj=%s)", "ValidCtxt"),
    "xmlCatalog *": ("._o", "catalog(_obj=%s)", "catalog"),
    "xmlURI *": ("._o", "URI(_obj=%s)", "URI"),
    "const xmlError *": ("._o", "Error(_obj=%s)", "Error"),
    "xmlError *": ("._o", "Error(_obj=%s)", "Error"),
    "xmlOutputBuffer *": ("._o", "outputBuffer(_obj=%s)", "outputBuffer"),
    "xmlParserInputBuffer *": ("._o", "inputBuffer(_obj=%s)", "inputBuffer"),
    "xmlRegexp *": ("._o", "xmlReg(_obj=%s)", "xmlReg"),
    "xmlTextReaderLocatorPtr": ("._o", "xmlTextReaderLocator(_obj=%s)", "xmlTextReaderLocator"),
    "xmlTextReader *": ("._o", "xmlTextReader(_obj=%s)", "xmlTextReader"),
    'xmlRelaxNG *': ('._o', "relaxNgSchema(_obj=%s)", "relaxNgSchema"),
    'xmlRelaxNGParserCtxt *': ('._o', "relaxNgParserCtxt(_obj=%s)", "relaxNgParserCtxt"),
    'xmlRelaxNGValidCtxt *': ('._o', "relaxNgValidCtxt(_obj=%s)", "relaxNgValidCtxt"),
    'xmlSchema *': ("._o", "Schema(_obj=%s)", "Schema"),
    'xmlSchemaParserCtxt *': ("._o", "SchemaParserCtxt(_obj=%s)", "SchemaParserCtxt"),
    'xmlSchemaValidCtxt *': ("._o", "SchemaValidCtxt(_obj=%s)", "SchemaValidCtxt"),
}

converter_type = {
    "xmlXPathObject *": "xpathObjectRet(%s)",
}

primary_classes = ["xmlNode", "xmlDoc"]

classes_ancestor = {
    "xmlNode" : "xmlCore",
    "xmlDtd" : "xmlNode",
    "xmlDoc" : "xmlNode",
    "xmlAttr" : "xmlNode",
    "xmlNs" : "xmlNode",
    "xmlEntity" : "xmlNode",
    "xmlElement" : "xmlNode",
    "xmlAttribute" : "xmlNode",
    "outputBuffer": "ioWriteWrapper",
    "inputBuffer": "ioReadWrapper",
    "parserCtxt": "parserCtxtCore",
    "xmlTextReader": "xmlTextReaderCore",
    "ValidCtxt": "ValidCtxtCore",
    "SchemaValidCtxt": "SchemaValidCtxtCore",
    "relaxNgValidCtxt": "relaxNgValidCtxtCore",
}
classes_destructors = {
    "parserCtxt": "xmlFreeParserCtxt",
    "catalog": "xmlFreeCatalog",
    "URI": "xmlFreeURI",
#    "outputBuffer": "xmlOutputBufferClose",
    "inputBuffer": "xmlFreeParserInputBuffer",
    "xmlReg": "xmlRegFreeRegexp",
    "xmlTextReader": "xmlFreeTextReader",
    "relaxNgSchema": "xmlRelaxNGFree",
    "relaxNgParserCtxt": "xmlRelaxNGFreeParserCtxt",
    "relaxNgValidCtxt": "xmlRelaxNGFreeValidCtxt",
        "Schema": "xmlSchemaFree",
        "SchemaParserCtxt": "xmlSchemaFreeParserCtxt",
        "SchemaValidCtxt": "xmlSchemaFreeValidCtxt",
        "ValidCtxt": "xmlFreeValidCtxt",
}

functions_noexcept = {
    "xmlHasProp": 1,
    "xmlHasNsProp": 1,
    "xmlDocSetRootElement": 1,
    "xmlNodeGetNs": 1,
    "xmlNodeGetNsDefs": 1,
    "xmlNextElementSibling": 1,
    "xmlPreviousElementSibling": 1,
    "xmlFirstElementChild": 1,
    "xmlLastElementChild": 1,
}

reference_keepers = {
    "xmlTextReader": [('inputBuffer', 'input')],
    "relaxNgValidCtxt": [('relaxNgSchema', 'schema')],
        "SchemaValidCtxt": [('Schema', 'schema')],
}

function_classes = {}

function_classes["None"] = []

def nameFixup(name, classe, type, file):
    listname = classe + "List"
    ll = len(listname)
    l = len(classe)
    if name[0:l] == listname:
        func = name[l:]
        func = func[0:1].lower() + func[1:]
    elif name[0:12] == "xmlParserGet" and file == "python_accessor":
        func = name[12:]
        func = func[0:1].lower() + func[1:]
    elif name[0:12] == "xmlParserSet" and file == "python_accessor":
        func = name[12:]
        func = func[0:1].lower() + func[1:]
    elif name[0:10] == "xmlNodeGet" and file == "python_accessor":
        func = name[10:]
        func = func[0:1].lower() + func[1:]
    elif name[0:9] == "xmlURIGet" and file == "python_accessor":
        func = name[9:]
        func = func[0:1].lower() + func[1:]
    elif name[0:9] == "xmlURISet" and file == "python_accessor":
        func = name[6:]
        func = func[0:1].lower() + func[1:]
    elif name[0:11] == "xmlErrorGet" and file == "python_accessor":
        func = name[11:]
        func = func[0:1].lower() + func[1:]
    elif name[0:17] == "xmlXPathParserGet" and file == "python_accessor":
        func = name[17:]
        func = func[0:1].lower() + func[1:]
    elif name[0:11] == "xmlXPathGet" and file == "python_accessor":
        func = name[11:]
        func = func[0:1].lower() + func[1:]
    elif name[0:11] == "xmlXPathSet" and file == "python_accessor":
        func = name[8:]
        func = func[0:1].lower() + func[1:]
    elif name[0:15] == "xmlOutputBuffer" and file != "python":
        func = name[15:]
        func = func[0:1].lower() + func[1:]
    elif name[0:20] == "xmlParserInputBuffer" and file != "python":
        func = name[20:]
        func = func[0:1].lower() + func[1:]
    elif name[0:9] == "xmlRegexp" and file == "xmlregexp":
        func = "regexp" + name[9:]
    elif name[0:6] == "xmlReg" and file == "xmlregexp":
        func = "regexp" + name[6:]
    elif name[0:20] == "xmlTextReaderLocator" and file == "xmlreader":
        func = name[20:]
    elif name[0:18] == "xmlTextReaderConst" and file == "xmlreader":
        func = name[18:]
    elif name[0:13] == "xmlTextReader" and file == "xmlreader":
        func = name[13:]
    elif name[0:12] == "xmlReaderNew" and file == "xmlreader":
        func = name[9:]
    elif name[0:11] == "xmlACatalog":
        func = name[11:]
        func = func[0:1].lower() + func[1:]
    elif name[0:l] == classe:
        func = name[l:]
        func = func[0:1].lower() + func[1:]
    elif name[0:7] == "libxml_":
        func = name[7:]
        func = func[0:1].lower() + func[1:]
    elif name[0:6] == "xmlGet":
        func = name[6:]
        func = func[0:1].lower() + func[1:]
    elif name[0:3] == "xml":
        func = name[3:]
        func = func[0:1].lower() + func[1:]
    else:
        func = name
    if func[0:5] == "xPath":
        func = "xpath" + func[5:]
    elif func[0:4] == "xPtr":
        func = "xpointer" + func[4:]
    elif func[0:8] == "xInclude":
        func = "xinclude" + func[8:]
    elif func[0:2] == "iD":
        func = "ID" + func[2:]
    elif func[0:3] == "uRI":
        func = "URI" + func[3:]
    elif func[0:4] == "uTF8":
        func = "UTF8" + func[4:]
    elif func[0:3] == 'sAX':
        func = "SAX" + func[3:]
    return func


def functionCompare(info1, info2):
    (index1, func1, name1, ret1, args1, file1) = info1
    (index2, func2, name2, ret2, args2, file2) = info2
    if file1 == file2:
        if func1 < func2:
            return -1
        if func1 > func2:
            return 1
    if file1 == "python_accessor":
        return -1
    if file2 == "python_accessor":
        return 1
    if file1 < file2:
        return -1
    if file1 > file2:
        return 1
    return 0

def cmp_to_key(mycmp):
    'Convert a cmp= function into a key= function'
    class K(object):
        def __init__(self, obj, *args):
            self.obj = obj
        def __lt__(self, other):
            return mycmp(self.obj, other.obj) < 0
        def __gt__(self, other):
            return mycmp(self.obj, other.obj) > 0
        def __eq__(self, other):
            return mycmp(self.obj, other.obj) == 0
        def __le__(self, other):
            return mycmp(self.obj, other.obj) <= 0
        def __ge__(self, other):
            return mycmp(self.obj, other.obj) >= 0
        def __ne__(self, other):
            return mycmp(self.obj, other.obj) != 0
    return K
def writeDoc(name, args, indent, output):
     if functions[name][0] is None or functions[name][0] == "":
         return
     val = functions[name][0]
     val = val.replace("NULL", "None")
     val = val.replace("\\", "\\\\")
     output.write(indent)
     output.write('"""')
     while len(val) > 60:
         if val[0] == " ":
             val = val[1:]
             continue
         str = val[0:60]
         i = str.rfind(" ")
         if i < 0:
             i = 60
         str = val[0:i]
         val = val[i:]
         output.write(str)
         output.write('\n  ')
         output.write(indent)
     output.write(val)
     output.write(' """\n')

def buildWrappers():
    global ctypes
    global py_types
    global py_return_types
    global unknown_types
    global functions
    global function_classes
    global classes_type
    global classes_list
    global converter_type
    global primary_classes
    global converter_type
    global classes_ancestor
    global converter_type
    global primary_classes
    global classes_ancestor
    global classes_destructors
    global functions_noexcept

    for type in classes_type.keys():
        function_classes[classes_type[type][2]] = []

    #
    # Build the list of C types to look for ordered to start
    # with primary classes
    #
    ctypes = []
    classes_list = []
    ctypes_processed = {}
    classes_processed = {}
    for classe in primary_classes:
        classes_list.append(classe)
        classes_processed[classe] = ()
        for type in classes_type.keys():
            tinfo = classes_type[type]
            if tinfo[2] == classe:
                ctypes.append(type)
                ctypes_processed[type] = ()
    for type in sorted(classes_type.keys()):
        if type in ctypes_processed:
            continue
        tinfo = classes_type[type]
        if tinfo[2] not in classes_processed:
            classes_list.append(tinfo[2])
            classes_processed[tinfo[2]] = ()

        ctypes.append(type)
        ctypes_processed[type] = ()

    for name in functions.keys():
        found = 0
        (desc, ret, args, file, cond) = functions[name]
        for type in ctypes:
            classe = classes_type[type][2]

            if name[0:3] == "xml" and len(args) >= 1 and args[0][1] == type:
                found = 1
                func = nameFixup(name, classe, type, file)
                info = (0, func, name, ret, args, file)
                function_classes[classe].append(info)
            elif name[0:3] == "xml" and len(args) >= 2 and args[1][1] == type \
                and file != "python_accessor":
                found = 1
                func = nameFixup(name, classe, type, file)
                info = (1, func, name, ret, args, file)
                function_classes[classe].append(info)
            elif name[0:4] == "html" and len(args) >= 1 and args[0][1] == type:
                found = 1
                func = nameFixup(name, classe, type, file)
                info = (0, func, name, ret, args, file)
                function_classes[classe].append(info)
            elif name[0:4] == "html" and len(args) >= 2 and args[1][1] == type \
                and file != "python_accessor":
                found = 1
                func = nameFixup(name, classe, type, file)
                info = (1, func, name, ret, args, file)
                function_classes[classe].append(info)
        if found == 1:
            continue
        if name[0:8] == "xmlXPath":
            continue
        if name[0:6] == "xmlStr":
            continue
        if name[0:10] == "xmlCharStr":
            continue
        func = nameFixup(name, "None", file, file)
        info = (0, func, name, ret, args, file)
        function_classes['None'].append(info)

    libxml_content = ""
    try:
        with open(os.path.join(srcPref, "libxml.py"), "r") as libxml_file:
            libxml_content = libxml_file.read()
    except IOError as msg:
        print("Error reading libxml.py:", msg)
        sys.exit(1)

    classes = open(os.path.join(dstPref, "libxml2.py"), "w")

    classes.write(libxml_content)

    if "None" in function_classes:
        flist = function_classes["None"]
        flist = sorted(flist, key=cmp_to_key(functionCompare))
        oldfile = ""
        for info in flist:
            (index, func, name, ret, args, file) = info
            if file != oldfile:
                classes.write("#\n# Functions from module %s\n#\n\n" % file)
                oldfile = file
            classes.write("def %s(" % func)
            n = 0
            for arg in args:
                if n != 0:
                    classes.write(", ")
                classes.write("%s" % arg[0])
                n = n + 1
            classes.write("):\n")
            writeDoc(name, args, '    ', classes)

            for arg in args:
                if arg[1] in classes_type:
                    classes.write("    if %s is None: %s__o = None\n" %
                                  (arg[0], arg[0]))
                    classes.write("    else: %s__o = %s%s\n" %
                                  (arg[0], arg[0], classes_type[arg[1]][0]))
                if arg[1] in py_types:
                    (f, t, n, c) = py_types[arg[1]]
                    if t == "File":
                        classes.write("    if %s is not None: %s.flush()\n" % (
                                      arg[0], arg[0]))

            if ret[0] != "void":
                classes.write("    ret = ")
            else:
                classes.write("    ")
            classes.write("libxml2mod.%s(" % name)
            n = 0
            for arg in args:
                if n != 0:
                    classes.write(", ")
                classes.write("%s" % arg[0])
                if arg[1] in classes_type:
                    classes.write("__o")
                n = n + 1
            classes.write(")\n")

# This may be needed to reposition the I/O, but likely to cause more harm
# than good. Those changes in Python3 really break the model.
#           for arg in args:
#               if arg[1] in py_types:
#                   (f, t, n, c) = py_types[arg[1]]
#                   if t == "File":
#                       classes.write("    if %s is not None: %s.seek(0,0)\n"%(
#                                     arg[0], arg[0]))

            if ret[0] != "void":
                if ret[0] in classes_type:
                    #
                    # Raise an exception
                    #
                    if name in functions_noexcept:
                        classes.write("    if ret is None:return None\n")
                    elif name.find("URI") >= 0:
                        classes.write(
                        "    if ret is None:raise uriError('%s() failed')\n"
                                      % (name))
                    elif name.find("XPath") >= 0:
                        classes.write(
                        "    if ret is None:raise xpathError('%s() failed')\n"
                                      % (name))
                    elif name.find("Parse") >= 0:
                        classes.write(
                        "    if ret is None:raise parserError('%s() failed')\n"
                                      % (name))
                    else:
                        classes.write(
                        "    if ret is None:raise treeError('%s() failed')\n"
                                      % (name))
                    classes.write("    return ")
                    classes.write(classes_type[ret[0]][1] % ("ret"))
                    classes.write("\n")
                else:
                    classes.write("    return ret\n")
            classes.write("\n")

    for classname in classes_list:
        if classname == "None":
            pass
        else:
            if classname in classes_ancestor:
                classes.write("class %s(%s):\n" % (classname,
                              classes_ancestor[classname]))
                classes.write("    def __init__(self, _obj=None):\n")
                if classes_ancestor[classname] == "xmlCore" or \
                   classes_ancestor[classname] == "xmlNode":
                    classes.write("        if checkWrapper(_obj) != 0:")
                    classes.write("            raise TypeError")
                    classes.write("('%s got a wrong wrapper object type')\n" % \
                                classname)
                if classname in reference_keepers:
                    rlist = reference_keepers[classname]
                    for ref in rlist:
                        classes.write("        self.%s = None\n" % ref[1])
                classes.write("        self._o = _obj\n")
                classes.write("        %s.__init__(self, _obj=_obj)\n\n" % (
                              classes_ancestor[classname]))
                if classes_ancestor[classname] == "xmlCore" or \
                   classes_ancestor[classname] == "xmlNode":
                    classes.write("    def __repr__(self):\n")
                    format = "<%s (%%s) object at 0x%%x>" % (classname)
                    classes.write("        return \"%s\" %% (self.name, int(pos_id (self)))\n\n" % (
                                  format))
            else:
                classes.write("class %s:\n" % (classname))
                classes.write("    def __init__(self, _obj=None):\n")
                if classname in reference_keepers:
                    list = reference_keepers[classname]
                    for ref in list:
                        classes.write("        self.%s = None\n" % ref[1])
                classes.write("        if _obj != None:self._o = _obj;return\n")
                classes.write("        self._o = None\n\n")
            destruct=None
            if classname in classes_destructors:
                classes.write("    def __del__(self):\n")
                classes.write("        if self._o != None:\n")
                classes.write("            libxml2mod.%s(self._o)\n" %
                              classes_destructors[classname])
                classes.write("        self._o = None\n\n")
                destruct=classes_destructors[classname]
            flist = function_classes[classname]
            flist = sorted(flist, key=cmp_to_key(functionCompare))
            oldfile = ""
            for info in flist:
                (index, func, name, ret, args, file) = info
                #
                # Do not provide as method the destructors for the class
                # to avoid double free
                #
                if name == destruct:
                    continue
                if file != oldfile:
                    if file == "python_accessor":
                        classes.write("    # accessors for %s\n" % (classname))
                    else:
                        classes.write("    #\n")
                        classes.write("    # %s functions from module %s\n" % (
                                      classname, file))
                        classes.write("    #\n\n")
                oldfile = file
                classes.write("    def %s(self" % func)
                n = 0
                for arg in args:
                    if n != index:
                        classes.write(", %s" % arg[0])
                    n = n + 1
                classes.write("):\n")
                writeDoc(name, args, '        ', classes)
                n = 0
                for arg in args:
                    if arg[1] in classes_type:
                        if n != index:
                            classes.write("        if %s is None: %s__o = None\n" %
                                          (arg[0], arg[0]))
                            classes.write("        else: %s__o = %s%s\n" %
                                          (arg[0], arg[0], classes_type[arg[1]][0]))
                    n = n + 1
                if ret[0] != "void":
                    classes.write("        ret = ")
                else:
                    classes.write("        ")
                classes.write("libxml2mod.%s(" % name)
                n = 0
                for arg in args:
                    if n != 0:
                        classes.write(", ")
                    if n != index:
                        classes.write("%s" % arg[0])
                        if arg[1] in classes_type:
                            classes.write("__o")
                    else:
                        classes.write("self")
                        if arg[1] in classes_type:
                            classes.write(classes_type[arg[1]][0])
                    n = n + 1
                classes.write(")\n")
                if ret[0] != "void":
                    if ret[0] in classes_type:
                        #
                        # Raise an exception
                        #
                        if name in functions_noexcept:
                            classes.write(
                                "        if ret is None:return None\n")
                        elif name.find("URI") >= 0:
                            classes.write(
                    "        if ret is None:raise uriError('%s() failed')\n"
                                          % (name))
                        elif name.find("XPath") >= 0:
                            classes.write(
                    "        if ret is None:raise xpathError('%s() failed')\n"
                                          % (name))
                        elif name.find("Parse") >= 0:
                            classes.write(
                    "        if ret is None:raise parserError('%s() failed')\n"
                                          % (name))
                        else:
                            classes.write(
                    "        if ret is None:raise treeError('%s() failed')\n"
                                          % (name))

                        #
                        # generate the returned class wrapper for the object
                        #
                        classes.write("        __tmp = ")
                        classes.write(classes_type[ret[0]][1] % ("ret"))
                        classes.write("\n")

                        #
                        # Sometime one need to keep references of the source
                        # class in the returned class object.
                        # See reference_keepers for the list
                        #
                        tclass = classes_type[ret[0]][2]
                        if tclass in reference_keepers:
                            list = reference_keepers[tclass]
                            for pref in list:
                                if pref[0] == classname:
                                    classes.write("        __tmp.%s = self\n" %
                                                  pref[1])
                        #
                        # return the class
                        #
                        classes.write("        return __tmp\n")
                    elif ret[0] in converter_type:
                        #
                        # Raise an exception
                        #
                        if name in functions_noexcept:
                            classes.write(
                                "        if ret is None:return None")
                        elif name.find("URI") >= 0:
                            classes.write(
                    "        if ret is None:raise uriError('%s() failed')\n"
                                          % (name))
                        elif name.find("XPath") >= 0:
                            classes.write(
                    "        if ret is None:raise xpathError('%s() failed')\n"
                                          % (name))
                        elif name.find("Parse") >= 0:
                            classes.write(
                    "        if ret is None:raise parserError('%s() failed')\n"
                                          % (name))
                        else:
                            classes.write(
                    "        if ret is None:raise treeError('%s() failed')\n"
                                          % (name))
                        classes.write("        return ")
                        classes.write(converter_type[ret[0]] % ("ret"))
                        classes.write("\n")
                    else:
                        classes.write("        return ret\n")
                classes.write("\n")

    #
    # Generate enum constants
    #
    for type in sorted(enums.keys()):
        enum = enums[type]
        classes.write("# %s\n" % type)
        items = enum.items()
        items = sorted(items, key=(lambda i: int(i[1])))
        for name,value in items:
            classes.write("%s = %s\n" % (name,value))
        classes.write("\n")

    classes.close()

buildStubs()
buildWrappers()
