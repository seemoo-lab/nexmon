/*
 * libxml.c: this modules implements the main part of the glue of the
 *           libxml2 library and the Python interpreter. It provides the
 *           entry points where an automatically generated stub is either
 *           unpractical or would not match cleanly the Python model.
 *
 * If compiled with MERGED_MODULES, the entry point will be used to
 * initialize both the libxml2 and the libxslt wrappers
 *
 * See Copyright for the status of this software.
 *
 * daniel@veillard.com
 */
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <fileobject.h>
/* #include "config.h" */
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xmlerror.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlIO.h>
#include <libxml/c14n.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlsave.h>
#include "libxml_wrap.h"
#include "libxml2-py.h"

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_libxml2mod(void);

#define PY_IMPORT_STRING_SIZE PyUnicode_FromStringAndSize
#define PY_IMPORT_STRING PyUnicode_FromString
#else
void initlibxml2mod(void);
#define PY_IMPORT_STRING_SIZE PyString_FromStringAndSize
#define PY_IMPORT_STRING PyString_FromString
#endif


/**
 * TODO:
 *
 * macro to flag unimplemented blocks
 */
#define TODO 								\
    xmlGenericError(xmlGenericErrorContext,				\
	    "Unimplemented block at %s:%d\n",				\
            __FILE__, __LINE__);

#ifdef LIBXML_XPATH_ENABLED
/*
 * the following vars are used for XPath extensions, but
 * are also referenced within the parser cleanup routine.
 */
static int libxml_xpathCallbacksInitialized = 0;

typedef struct libxml_xpathCallback {
    xmlXPathContextPtr ctx;
    xmlChar *name;
    xmlChar *ns_uri;
    PyObject *function;
} libxml_xpathCallback, *libxml_xpathCallbackPtr;
typedef libxml_xpathCallback libxml_xpathCallbackArray[];
static int libxml_xpathCallbacksAllocd = 10;
static libxml_xpathCallbackArray *libxml_xpathCallbacks = NULL;
static int libxml_xpathCallbacksNb = 0;
#endif /* LIBXML_XPATH_ENABLED */

/************************************************************************
 *									*
 *		Memory debug interface					*
 *									*
 ************************************************************************/

#if 0
extern void xmlMemFree(void *ptr);
extern void *xmlMemMalloc(size_t size);
extern void *xmlMemRealloc(void *ptr, size_t size);
extern char *xmlMemoryStrdup(const char *str);
#endif

static int libxmlMemoryDebugActivated = 0;
static long libxmlMemoryAllocatedBase = 0;

static int libxmlMemoryDebug = 0;
static xmlFreeFunc freeFunc = NULL;
static xmlMallocFunc mallocFunc = NULL;
static xmlReallocFunc reallocFunc = NULL;
static xmlStrdupFunc strdupFunc = NULL;

static void
libxml_xmlErrorInitialize(void); /* forward declare */

PyObject *
libxml_xmlMemoryUsed(PyObject * self ATTRIBUTE_UNUSED, 
        PyObject * args ATTRIBUTE_UNUSED)
{
    long ret;
    PyObject *py_retval;

    ret = xmlMemUsed();

    py_retval = libxml_longWrap(ret);
    return (py_retval);
}

PyObject *
libxml_xmlDebugMemory(PyObject * self ATTRIBUTE_UNUSED, PyObject * args)
{
    int activate;
    PyObject *py_retval;
    long ret;

    if (!PyArg_ParseTuple(args, "i:xmlDebugMemory", &activate))
        return (NULL);

    if (activate != 0) {
        if (libxmlMemoryDebug == 0) {
            /*
             * First initialize the library and grab the old memory handlers
             * and switch the library to memory debugging
             */
            xmlMemGet((xmlFreeFunc *) & freeFunc,
                      (xmlMallocFunc *) & mallocFunc,
                      (xmlReallocFunc *) & reallocFunc,
                      (xmlStrdupFunc *) & strdupFunc);
            if ((freeFunc == xmlMemFree) && (mallocFunc == xmlMemMalloc) &&
                (reallocFunc == xmlMemRealloc) &&
                (strdupFunc == xmlMemoryStrdup)) {
            } else {
                ret = (long) xmlMemSetup(xmlMemFree, xmlMemMalloc,
                                         xmlMemRealloc, xmlMemoryStrdup);
                if (ret < 0)
                    goto error;
            }
            libxmlMemoryAllocatedBase = xmlMemUsed();
            ret = 0;
        } else if (libxmlMemoryDebugActivated == 0) {
            libxmlMemoryAllocatedBase = xmlMemUsed();
            ret = 0;
        } else {
            ret = xmlMemUsed() - libxmlMemoryAllocatedBase;
        }
        libxmlMemoryDebug = 1;
        libxmlMemoryDebugActivated = 1;
    } else {
        if (libxmlMemoryDebugActivated == 1)
            ret = xmlMemUsed() - libxmlMemoryAllocatedBase;
        else
            ret = 0;
        libxmlMemoryDebugActivated = 0;
    }
  error:
    py_retval = libxml_longWrap(ret);
    return (py_retval);
}

PyObject *
libxml_xmlPythonCleanupParser(PyObject *self ATTRIBUTE_UNUSED,
                              PyObject *args ATTRIBUTE_UNUSED) {

#ifdef LIBXML_XPATH_ENABLED
    int ix;

    /*
     * Need to confirm whether we really want to do this (required for
     * memcheck) in all cases...
     */
   
    if (libxml_xpathCallbacks != NULL) {	/* if ext funcs declared */
	for (ix=0; ix<libxml_xpathCallbacksNb; ix++) {
	    if ((*libxml_xpathCallbacks)[ix].name != NULL)
	        xmlFree((*libxml_xpathCallbacks)[ix].name);
	    if ((*libxml_xpathCallbacks)[ix].ns_uri != NULL)
	        xmlFree((*libxml_xpathCallbacks)[ix].ns_uri);
	}
	libxml_xpathCallbacksNb = 0;
        xmlFree(libxml_xpathCallbacks);
	libxml_xpathCallbacks = NULL;
    }
#endif /* LIBXML_XPATH_ENABLED */

    xmlCleanupParser();

    Py_INCREF(Py_None);
    return(Py_None);
}

/************************************************************************
 *									*
 *		Handling Python FILE I/O at the C level			*
 *	The raw I/O attack directly the File objects, while the		*
 *	other routines address the ioWrapper instance instead		*
 *									*
 ************************************************************************/

/**
 * xmlPythonFileCloseUnref:
 * @context:  the I/O context
 *
 * Close an I/O channel
 */
static int
xmlPythonFileCloseRaw (void * context) {
    PyObject *file, *ret;

    file = (PyObject *) context;
    if (file == NULL) return(-1);
    ret = PyObject_CallMethod(file, "close", "()");
    if (ret != NULL) {
	Py_DECREF(ret);
    }
    Py_DECREF(file);
    return(0);
}

/**
 * xmlPythonFileReadRaw:
 * @context:  the I/O context
 * @buffer:  where to drop data
 * @len:  number of bytes to write
 *
 * Read @len bytes to @buffer from the Python file in the I/O channel
 *
 * Returns the number of bytes read
 */
static int
xmlPythonFileReadRaw (void * context, char * buffer, int len) {
    PyObject *file;
    PyObject *ret;
    int lenread = -1;
    char *data;
#ifdef PyUnicode_Check
    PyObject *b = NULL;
#endif

    file = (PyObject *) context;
    if (file == NULL) return(-1);
    /* When read() returns a string, the length is in characters not bytes, so
       request at most len / 4 characters to leave space for UTF-8 encoding. */
    ret = PyObject_CallMethod(file, "read", "(i)", len / 4);
    if (ret == NULL) {
	printf("xmlPythonFileReadRaw: result is NULL\n");
	return(-1);
    } else if (PyBytes_Check(ret)) {
	lenread = PyBytes_Size(ret);
	data = PyBytes_AsString(ret);
#ifdef PyUnicode_Check
    } else if (PyUnicode_Check (ret)) {
#if PY_VERSION_HEX >= 0x03030000
        Py_ssize_t size;
	const char *tmp;

	/* tmp doesn't need to be deallocated */
        tmp = PyUnicode_AsUTF8AndSize(ret, &size);

	lenread = (int) size;
	data = (char *) tmp;
#else
	b = PyUnicode_AsUTF8String(ret);
	if (b == NULL) {
	    Py_DECREF(ret);
	    printf("xmlPythonFileReadRaw: failed to convert to UTF-8\n");
	    return(-1);
	}
	lenread = PyBytes_Size(b);
	data = PyBytes_AsString(b);
#endif
#endif
    } else {
	printf("xmlPythonFileReadRaw: result is not a String\n");
	Py_DECREF(ret);
	return(-1);
    }
    if (lenread < 0 || lenread > len) {
	printf("xmlPythonFileReadRaw: invalid lenread\n");
	Py_DECREF(ret);
#ifdef PyUnicode_Check
	if (b != NULL) {
	    Py_DECREF(b);
	}
#endif
	return(-1);
    }
    memcpy(buffer, data, lenread);
    Py_DECREF(ret);
#ifdef PyUnicode_Check
    if (b != NULL) {
        Py_DECREF(b);
    }
#endif
    return(lenread);
}

/**
 * xmlPythonFileRead:
 * @context:  the I/O context
 * @buffer:  where to drop data
 * @len:  number of bytes to write
 *
 * Read @len bytes to @buffer from the I/O channel.
 *
 * Returns the number of bytes read
 */
static int
xmlPythonFileRead (void * context, char * buffer, int len) {
    PyObject *file;
    PyObject *ret;
    int lenread = -1;
    char *data;
#ifdef PyUnicode_Check
    PyObject *b = NULL;
#endif

    file = (PyObject *) context;
    if (file == NULL) return(-1);
    /* When io_read() returns a string, the length is in characters not bytes, so
       request at most len / 4 characters to leave space for UTF-8 encoding. */
    ret = PyObject_CallMethod(file, "io_read", "(i)", len / 4);
    if (ret == NULL) {
	printf("xmlPythonFileRead: result is NULL\n");
	return(-1);
    } else if (PyBytes_Check(ret)) {
	lenread = PyBytes_Size(ret);
	data = PyBytes_AsString(ret);
#ifdef PyUnicode_Check
    } else if (PyUnicode_Check (ret)) {
#if PY_VERSION_HEX >= 0x03030000
        Py_ssize_t size;
	const char *tmp;

	/* tmp doesn't need to be deallocated */
        tmp = PyUnicode_AsUTF8AndSize(ret, &size);

	lenread = (int) size;
	data = (char *) tmp;
#else
	b = PyUnicode_AsUTF8String(ret);
	if (b == NULL) {
	    Py_DECREF(ret);
	    printf("xmlPythonFileRead: failed to convert to UTF-8\n");
	    return(-1);
	}
	lenread = PyBytes_Size(b);
	data = PyBytes_AsString(b);
#endif
#endif
    } else {
	printf("xmlPythonFileRead: result is not a String\n");
	Py_DECREF(ret);
	return(-1);
    }
    if (lenread < 0 || lenread > len) {
	printf("xmlPythonFileRead: invalid lenread\n");
	Py_DECREF(ret);
#ifdef PyUnicode_Check
	if (b != NULL) {
	    Py_DECREF(b);
	}
#endif
	return(-1);
    }
    memcpy(buffer, data, lenread);
    Py_DECREF(ret);
#ifdef PyUnicode_Check
    if (b != NULL) {
	Py_DECREF(b);
    }
#endif
    return(lenread);
}

#ifdef LIBXML_OUTPUT_ENABLED
/**
 * xmlFileWrite:
 * @context:  the I/O context
 * @buffer:  where to drop data
 * @len:  number of bytes to write
 *
 * Write @len bytes from @buffer to the I/O channel.
 *
 * Returns the number of bytes written
 */
static int
xmlPythonFileWrite (void * context, const char * buffer, int len) {
    PyObject *file;
    PyObject *string;
    PyObject *ret = NULL;
    int written = -1;

    file = (PyObject *) context;
    if (file == NULL) return(-1);
    string = PY_IMPORT_STRING_SIZE(buffer, len);
    if (string == NULL) return(-1);
    if (PyObject_HasAttrString(file, "io_write")) {
        ret = PyObject_CallMethod(file, "io_write", "(O)",
	                        string);
    } else if (PyObject_HasAttrString(file, "write")) {
        ret = PyObject_CallMethod(file, "write", "(O)",
	                        string);
    }
    Py_DECREF(string);
    if (ret == NULL) {
	printf("xmlPythonFileWrite: result is NULL\n");
	return(-1);
    } else if (PyLong_Check(ret)) {
	written = (int) PyLong_AsLong(ret);
	Py_DECREF(ret);
    } else if (ret == Py_None) {
	written = len;
	Py_DECREF(ret);
    } else {
	printf("xmlPythonFileWrite: result is not an Int nor None\n");
	Py_DECREF(ret);
    }
    return(written);
}
#endif /* LIBXML_OUTPUT_ENABLED */

/**
 * xmlPythonFileClose:
 * @context:  the I/O context
 *
 * Close an I/O channel
 */
static int
xmlPythonFileClose (void * context) {
    PyObject *file, *ret = NULL;

    file = (PyObject *) context;
    if (file == NULL) return(-1);
    if (PyObject_HasAttrString(file, "io_close")) {
        ret = PyObject_CallMethod(file, "io_close", "()");
    } else if (PyObject_HasAttrString(file, "flush")) {
        ret = PyObject_CallMethod(file, "flush", "()");
    }
    if (ret != NULL) {
	Py_DECREF(ret);
    }
    return(0);
}

#ifdef LIBXML_OUTPUT_ENABLED
/**
 * xmlOutputBufferCreatePythonFile:
 * @file:  a PyFile_Type
 * @encoder:  the encoding converter or NULL
 *
 * Create a buffered output for the progressive saving to a PyFile_Type
 * buffered C I/O
 *
 * Returns the new parser output or NULL
 */
static xmlOutputBufferPtr
xmlOutputBufferCreatePythonFile(PyObject *file, 
	                        xmlCharEncodingHandlerPtr encoder) {
    xmlOutputBufferPtr ret;

    if (file == NULL) return(NULL);

    ret = xmlOutputBufferCreateIO(xmlPythonFileWrite, xmlPythonFileClose,
                                  file, encoder);

    return(ret);
}

PyObject *
libxml_xmlCreateOutputBuffer(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) {
    PyObject *py_retval;
    PyObject *file;
    xmlChar  *encoding;
    xmlCharEncodingHandlerPtr handler = NULL;
    xmlOutputBufferPtr buffer;


    if (!PyArg_ParseTuple(args, "Oz:xmlOutputBufferCreate",
		&file, &encoding))
	return(NULL);
    if ((encoding != NULL) && (encoding[0] != 0)) {
	handler = xmlFindCharEncodingHandler((const char *) encoding);
    }
    buffer = xmlOutputBufferCreatePythonFile(file, handler);
    if (buffer == NULL)
	printf("libxml_xmlCreateOutputBuffer: buffer == NULL\n");
    py_retval = libxml_xmlOutputBufferPtrWrap(buffer);
    return(py_retval);
}

/**
 * libxml_outputBufferGetPythonFile:
 * @buffer:  the I/O buffer
 *
 * read the Python I/O from the CObject
 *
 * Returns the new parser output or NULL
 */
static PyObject *
libxml_outputBufferGetPythonFile(ATTRIBUTE_UNUSED PyObject *self,
                                    PyObject *args) {
    PyObject *buffer;
    PyObject *file;
    xmlOutputBufferPtr obj;

    if (!PyArg_ParseTuple(args, "O:outputBufferGetPythonFile",
			  &buffer))
	return(NULL);

    obj = PyoutputBuffer_Get(buffer);
    if (obj == NULL) {
	fprintf(stderr,
	        "outputBufferGetPythonFile: obj == NULL\n");
	Py_INCREF(Py_None);
	return(Py_None);
    }
XML_IGNORE_DEPRECATION_WARNINGS
    if (obj->closecallback != xmlPythonFileClose) {
	fprintf(stderr,
	        "outputBufferGetPythonFile: not a python file wrapper\n");
	Py_INCREF(Py_None);
	return(Py_None);
    }
    file = (PyObject *) obj->context;
    if (file == NULL) {
	Py_INCREF(Py_None);
	return(Py_None);
    }
XML_POP_WARNINGS
    Py_INCREF(file);
    return(file);
}

static PyObject *
libxml_xmlOutputBufferClose(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlOutputBufferPtr out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, "O:xmlOutputBufferClose", &pyobj_out))
        return(NULL);
    out = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_out);
    /* Buffer may already have been destroyed elsewhere. This is harmless. */
    if (out == NULL) {
	Py_INCREF(Py_None);
	return(Py_None);
    }

    c_retval = xmlOutputBufferClose(out);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

static PyObject *
libxml_xmlOutputBufferFlush(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlOutputBufferPtr out;
    PyObject *pyobj_out;

    if (!PyArg_ParseTuple(args, "O:xmlOutputBufferFlush", &pyobj_out))
        return(NULL);
    out = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_out);

    c_retval = xmlOutputBufferFlush(out);
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

static PyObject *
libxml_xmlSaveFileTo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;

    if (!PyArg_ParseTuple(args, "OOz:xmlSaveFileTo", &pyobj_buf, &pyobj_cur, &encoding))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlSaveFileTo(buf, cur, encoding);
	/* xmlSaveTo() freed the memory pointed to by buf, so record that in the
	 * Python object. */
    ((PyoutputBuffer_Object *)(pyobj_buf))->obj = NULL;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}

static PyObject *
libxml_xmlSaveFormatFileTo(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    PyObject *py_retval;
    int c_retval;
    xmlOutputBufferPtr buf;
    PyObject *pyobj_buf;
    xmlDocPtr cur;
    PyObject *pyobj_cur;
    char * encoding;
    int format;

    if (!PyArg_ParseTuple(args, "OOzi:xmlSaveFormatFileTo", &pyobj_buf, &pyobj_cur, &encoding, &format))
        return(NULL);
    buf = (xmlOutputBufferPtr) PyoutputBuffer_Get(pyobj_buf);
    cur = (xmlDocPtr) PyxmlNode_Get(pyobj_cur);

    c_retval = xmlSaveFormatFileTo(buf, cur, encoding, format);
	/* xmlSaveFormatFileTo() freed the memory pointed to by buf, so record that
	 * in the Python object */
	((PyoutputBuffer_Object *)(pyobj_buf))->obj = NULL;
    py_retval = libxml_intWrap((int) c_retval);
    return(py_retval);
}
#endif /* LIBXML_OUTPUT_ENABLED */


/**
 * xmlParserInputBufferCreatePythonFile:
 * @file:  a PyFile_Type
 * @encoder:  the encoding converter or NULL
 *
 * Create a buffered output for the progressive saving to a PyFile_Type
 * buffered C I/O
 *
 * Returns the new parser output or NULL
 */
static xmlParserInputBufferPtr
xmlParserInputBufferCreatePythonFile(PyObject *file, 
	                        xmlCharEncoding encoding) {
    xmlParserInputBufferPtr ret;

    if (file == NULL) return(NULL);

    ret = xmlParserInputBufferCreateIO(xmlPythonFileRead, xmlPythonFileClose,
                                       file, encoding);

    return(ret);
}

PyObject *
libxml_xmlCreateInputBuffer(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) {
    PyObject *py_retval;
    PyObject *file;
    xmlChar  *encoding;
    xmlCharEncoding enc = XML_CHAR_ENCODING_NONE;
    xmlParserInputBufferPtr buffer;


    if (!PyArg_ParseTuple(args, "Oz:xmlParserInputBufferCreate",
		&file, &encoding))
	return(NULL);
    if ((encoding != NULL) && (encoding[0] != 0)) {
	enc = xmlParseCharEncoding((const char *) encoding);
    }
    buffer = xmlParserInputBufferCreatePythonFile(file, enc);
    if (buffer == NULL)
	printf("libxml_xmlParserInputBufferCreate: buffer == NULL\n");
    py_retval = libxml_xmlParserInputBufferPtrWrap(buffer);
    return(py_retval);
}

/************************************************************************
 *									*
 *		Providing the resolver at the Python level		*
 *									*
 ************************************************************************/

static xmlExternalEntityLoader defaultExternalEntityLoader = NULL;
static PyObject *pythonExternalEntityLoaderObjext;

static xmlParserInputPtr
pythonExternalEntityLoader(const char *URL, const char *ID,
			   xmlParserCtxtPtr ctxt) {
    xmlParserInputPtr result = NULL;
    if (pythonExternalEntityLoaderObjext != NULL) {
	PyObject *ret;
	PyObject *ctxtobj;

	ctxtobj = libxml_xmlParserCtxtPtrWrap(ctxt);

	ret = PyObject_CallFunction(pythonExternalEntityLoaderObjext,
		      "(ssO)", URL, ID, ctxtobj);
	Py_XDECREF(ctxtobj);

	if (ret != NULL) {
	    if (PyObject_HasAttrString(ret, "read")) {
		xmlParserInputBufferPtr buf;

		buf = xmlParserInputBufferCreateIO(xmlPythonFileReadRaw,
                                                   xmlPythonFileCloseRaw,
                                                   ret, XML_CHAR_ENCODING_NONE);
		if (buf != NULL) {
		    result = xmlNewIOInputStream(ctxt, buf,
			                         XML_CHAR_ENCODING_NONE);
		}
#if 0
	    } else {
		if (URL != NULL)
		    printf("pythonExternalEntityLoader: can't read %s\n",
		           URL);
#endif
	    }
	    if (result == NULL) {
		Py_DECREF(ret);
	    } else if (URL != NULL) {
		result->filename = (char *) xmlStrdup((const xmlChar *)URL);
	    }
	}
    }
    if ((result == NULL) && (defaultExternalEntityLoader != NULL)) {
	result = defaultExternalEntityLoader(URL, ID, ctxt);
    }
    return(result);
}

PyObject *
libxml_xmlSetEntityLoader(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) {
    PyObject *py_retval;
    PyObject *loader;

    if (!PyArg_ParseTuple(args, "O:libxml_xmlSetEntityLoader",
		&loader))
	return(NULL);

    if (!PyCallable_Check(loader)) {
	PyErr_SetString(PyExc_ValueError, "entity loader is not callable");
	return(NULL);
    }

    if (defaultExternalEntityLoader == NULL) 
	defaultExternalEntityLoader = xmlGetExternalEntityLoader();

    Py_XDECREF(pythonExternalEntityLoaderObjext);
    pythonExternalEntityLoaderObjext = loader;
    Py_XINCREF(pythonExternalEntityLoaderObjext);
    xmlSetExternalEntityLoader(pythonExternalEntityLoader);

    py_retval = PyLong_FromLong(0);
    return(py_retval);
}

/************************************************************************
 *									*
 *		Input callback registration				*
 *									*
 ************************************************************************/
static PyObject *pythonInputOpenCallbackObject;
static int pythonInputCallbackID = -1;

static int
pythonInputMatchCallback(ATTRIBUTE_UNUSED const char *URI)
{
    /* Always return success, real decision whether URI is supported will be
     * made in open callback.  */
    return 1;
}

static void *
pythonInputOpenCallback(const char *URI)
{
    PyObject *ret;

    ret = PyObject_CallFunction(pythonInputOpenCallbackObject,
	    "s", URI);
    if (ret == Py_None) {
	Py_DECREF(Py_None);
	return NULL;
    }
    return ret;
}

PyObject *
libxml_xmlRegisterInputCallback(ATTRIBUTE_UNUSED PyObject *self,
                                PyObject *args) {
    PyObject *cb;

    if (!PyArg_ParseTuple(args,
		(const char *)"O:libxml_xmlRegisterInputCallback", &cb))
	return(NULL);

    if (!PyCallable_Check(cb)) {
	PyErr_SetString(PyExc_ValueError, "input callback is not callable");
	return(NULL);
    }

    /* Python module registers a single callback and manages the list of
     * all callbacks internally. This is necessitated by xmlInputMatchCallback
     * API, which does not allow for passing of data objects to discriminate
     * different Python methods.  */
    if (pythonInputCallbackID == -1) {
	pythonInputCallbackID = xmlRegisterInputCallbacks(
		pythonInputMatchCallback, pythonInputOpenCallback,
		xmlPythonFileReadRaw, xmlPythonFileCloseRaw);
	if (pythonInputCallbackID == -1)
	    return PyErr_NoMemory();
	pythonInputOpenCallbackObject = cb;
	Py_INCREF(pythonInputOpenCallbackObject);
    }

    Py_INCREF(Py_None);
    return(Py_None);
}

PyObject *
libxml_xmlUnregisterInputCallback(ATTRIBUTE_UNUSED PyObject *self,
                                ATTRIBUTE_UNUSED PyObject *args) {
    int ret;

    ret = xmlPopInputCallbacks();
    if (pythonInputCallbackID != -1) {
	/* Assert that the right input callback was popped. libxml's API does not
	 * allow removal by ID, so all that could be done is an assert.  */
	if (pythonInputCallbackID == ret) {
	    pythonInputCallbackID = -1;
	    Py_DECREF(pythonInputOpenCallbackObject);
	    pythonInputOpenCallbackObject = NULL;
	} else {
	    PyErr_SetString(PyExc_AssertionError, "popped non-python input callback");
	    return(NULL);
	}
    } else if (ret == -1) {
	/* No more callbacks to pop */
	PyErr_SetString(PyExc_IndexError, "no input callbacks to pop");
	return(NULL);
    }

    Py_INCREF(Py_None);
    return(Py_None);
}

/************************************************************************
 *									*
 *		Handling SAX/xmllib/sgmlop callback interfaces		*
 *									*
 ************************************************************************/

static void
pythonStartElement(void *user_data, const xmlChar * name,
                   const xmlChar ** attrs)
{
    int i;
    PyObject *handler;
    PyObject *dict;
    PyObject *attrname;
    PyObject *attrvalue;
    PyObject *result = NULL;
    int type = 0;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "startElement"))
        type = 1;
    else if (PyObject_HasAttrString(handler, "start"))
        type = 2;
    if (type != 0) {
        /*
         * the xmllib interface always generates a dictionary,
         * possibly empty
         */
        if ((attrs == NULL) && (type == 1)) {
            Py_XINCREF(Py_None);
            dict = Py_None;
        } else if (attrs == NULL) {
            dict = PyDict_New();
        } else {
            dict = PyDict_New();
            for (i = 0; attrs[i] != NULL; i++) {
                attrname = PY_IMPORT_STRING((char *) attrs[i]);
                i++;
                if (attrs[i] != NULL) {
                    attrvalue = PY_IMPORT_STRING((char *) attrs[i]);
                } else {
                    Py_XINCREF(Py_None);
                    attrvalue = Py_None;
                }
                PyDict_SetItem(dict, attrname, attrvalue);
		Py_DECREF(attrname);
		Py_DECREF(attrvalue);
            }
        }

        if (type == 1)
            result = PyObject_CallMethod(handler, "startElement",
                                         "sO", name, dict);
        else if (type == 2)
            result = PyObject_CallMethod(handler, "start",
                                         "sO", name, dict);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(dict);
        Py_XDECREF(result);
    }
}

static void
pythonStartDocument(void *user_data)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "startDocument")) {
        result = PyObject_CallMethod(handler, "startDocument", NULL);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonEndDocument(void *user_data)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "endDocument")) {
        result = PyObject_CallMethod(handler, "endDocument", NULL);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
    /*
     * The reference to the handler is released there
     */
    Py_XDECREF(handler);
}

static void
pythonEndElement(void *user_data, const xmlChar * name)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "endElement")) {
        result = PyObject_CallMethod(handler, "endElement",
                                     "s", name);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    } else if (PyObject_HasAttrString(handler, "end")) {
        result = PyObject_CallMethod(handler, "end",
                                     "s", name);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonReference(void *user_data, const xmlChar * name)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "reference")) {
        result = PyObject_CallMethod(handler, "reference",
                                     "s", name);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonCharacters(void *user_data, const xmlChar * ch, int len)
{
    PyObject *handler;
    PyObject *result = NULL;
    int type = 0;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "characters"))
        type = 1;
    else if (PyObject_HasAttrString(handler, "data"))
        type = 2;
    if (type != 0) {
        if (type == 1)
            result = PyObject_CallMethod(handler, "characters",
                                         "s#", ch, len);
        else if (type == 2)
            result = PyObject_CallMethod(handler, "data",
                                         "s#", ch, len);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonIgnorableWhitespace(void *user_data, const xmlChar * ch, int len)
{
    PyObject *handler;
    PyObject *result = NULL;
    int type = 0;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "ignorableWhitespace"))
        type = 1;
    else if (PyObject_HasAttrString(handler, "data"))
        type = 2;
    if (type != 0) {
        if (type == 1)
            result = PyObject_CallMethod(handler, "ignorableWhitespace",
                                         "s#", ch, len);
        else if (type == 2)
            result = PyObject_CallMethod(handler, "data",
                                         "s#", ch, len);
        Py_XDECREF(result);
    }
}

static void
pythonProcessingInstruction(void *user_data,
                            const xmlChar * target, const xmlChar * data)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "processingInstruction")) {
        result = PyObject_CallMethod(handler, "processingInstruction",
                                     "ss", target, data);
        Py_XDECREF(result);
    }
}

static void
pythonComment(void *user_data, const xmlChar * value)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "comment")) {
        result = PyObject_CallMethod(handler, "comment", "s", value);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonWarning(void *user_data, const char *msg, ...)
{
    PyObject *handler;
    PyObject *result;
    va_list ap;
    char buf[1024];

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "warning")) {
        va_start(ap, msg);
        vsnprintf(buf, sizeof(buf), msg, ap);
        va_end(ap);
        result = PyObject_CallMethod(handler, "warning", "s", buf);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonError(void *user_data, const char *msg, ...)
{
    PyObject *handler;
    PyObject *result;
    va_list ap;
    char buf[1024];

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "error")) {
        va_start(ap, msg);
        vsnprintf(buf, sizeof(buf), msg, ap);
        va_end(ap);
        result = PyObject_CallMethod(handler, "error", "s", buf);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonFatalError(void *user_data, const char *msg, ...)
{
    PyObject *handler;
    PyObject *result;
    va_list ap;
    char buf[1024];

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "fatalError")) {
        va_start(ap, msg);
        vsnprintf(buf, sizeof(buf), msg, ap);
        va_end(ap);
        result = PyObject_CallMethod(handler, "fatalError", "s", buf);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonCdataBlock(void *user_data, const xmlChar * ch, int len)
{
    PyObject *handler;
    PyObject *result = NULL;
    int type = 0;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "cdataBlock"))
        type = 1;
    else if (PyObject_HasAttrString(handler, "cdata"))
        type = 2;
    if (type != 0) {
        if (type == 1)
            result = PyObject_CallMethod(handler, "cdataBlock", "s#", ch, len);
        else if (type == 2)
            result = PyObject_CallMethod(handler, "cdata", "s#", ch, len);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonExternalSubset(void *user_data,
                     const xmlChar * name,
                     const xmlChar * externalID, const xmlChar * systemID)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "externalSubset")) {
        result = PyObject_CallMethod(handler, "externalSubset", "sss",
                                     name, externalID, systemID);
        Py_XDECREF(result);
    }
}

static void
pythonEntityDecl(void *user_data,
                 const xmlChar * name,
                 int type,
                 const xmlChar * publicId,
                 const xmlChar * systemId, xmlChar * content)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "entityDecl")) {
        result = PyObject_CallMethod(handler, "entityDecl",
                                     "sisss", name, type,
                                     publicId, systemId, content);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}



static void

pythonNotationDecl(void *user_data,
                   const xmlChar * name,
                   const xmlChar * publicId, const xmlChar * systemId)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "notationDecl")) {
        result = PyObject_CallMethod(handler, "notationDecl",
                                     "sss", name, publicId,
                                     systemId);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonAttributeDecl(void *user_data,
                    const xmlChar * elem,
                    const xmlChar * name,
                    int type,
                    int def,
                    const xmlChar * defaultValue, xmlEnumerationPtr tree)
{
    PyObject *handler;
    PyObject *nameList;
    PyObject *newName;
    xmlEnumerationPtr node;
    PyObject *result;
    int count;

XML_IGNORE_DEPRECATION_WARNINGS
    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "attributeDecl")) {
        count = 0;
        for (node = tree; node != NULL; node = node->next) {
            count++;
        }
        nameList = PyList_New(count);
        count = 0;
        for (node = tree; node != NULL; node = node->next) {
            newName = PY_IMPORT_STRING((char *) node->name);
            PyList_SetItem(nameList, count, newName);
            count++;
        }
        result = PyObject_CallMethod(handler, "attributeDecl",
                                     "ssiisO", elem, name, type,
                                     def, defaultValue, nameList);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(nameList);
        Py_XDECREF(result);
    }
XML_POP_WARNINGS
}

static void
pythonElementDecl(void *user_data,
                  const xmlChar * name,
                  int type, ATTRIBUTE_UNUSED xmlElementContentPtr content)
{
    PyObject *handler;
    PyObject *obj;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "elementDecl")) {
        /* TODO: wrap in an elementContent object */
        printf
            ("pythonElementDecl: xmlElementContentPtr wrapper missing !\n");
        obj = Py_None;
        /* Py_XINCREF(Py_None); isn't the reference just borrowed ??? */
        result = PyObject_CallMethod(handler, "elementDecl",
                                     "siO", name, type, obj);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonUnparsedEntityDecl(void *user_data,
                         const xmlChar * name,
                         const xmlChar * publicId,
                         const xmlChar * systemId,
                         const xmlChar * notationName)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "unparsedEntityDecl")) {
        result = PyObject_CallMethod(handler, "unparsedEntityDecl", "ssss",
                                     name, publicId, systemId, notationName);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static void
pythonInternalSubset(void *user_data, const xmlChar * name,
                     const xmlChar * ExternalID, const xmlChar * SystemID)
{
    PyObject *handler;
    PyObject *result;

    handler = (PyObject *) user_data;
    if (PyObject_HasAttrString(handler, "internalSubset")) {
        result = PyObject_CallMethod(handler, "internalSubset", "sss", name,
                                     ExternalID, SystemID);
        if (PyErr_Occurred())
            PyErr_Print();
        Py_XDECREF(result);
    }
}

static xmlSAXHandler pythonSaxHandler = {
    pythonInternalSubset,
    NULL,                       /* TODO pythonIsStandalone, */
    NULL,                       /* TODO pythonHasInternalSubset, */
    NULL,                       /* TODO pythonHasExternalSubset, */
    NULL,                       /* TODO pythonResolveEntity, */
    NULL,                       /* TODO pythonGetEntity, */
    pythonEntityDecl,
    pythonNotationDecl,
    pythonAttributeDecl,
    pythonElementDecl,
    pythonUnparsedEntityDecl,
    NULL,                       /* OBSOLETED pythonSetDocumentLocator, */
    pythonStartDocument,
    pythonEndDocument,
    pythonStartElement,
    pythonEndElement,
    pythonReference,
    pythonCharacters,
    pythonIgnorableWhitespace,
    pythonProcessingInstruction,
    pythonComment,
    pythonWarning,
    pythonError,
    pythonFatalError,
    NULL,                       /* TODO pythonGetParameterEntity, */
    pythonCdataBlock,
    pythonExternalSubset,
    1,
    NULL,			/* TODO migrate to SAX2 */
    NULL,
    NULL,
    NULL
};

/************************************************************************
 *									*
 *		Handling of specific parser context			*
 *									*
 ************************************************************************/

#ifdef LIBXML_PUSH_ENABLED
PyObject *
libxml_xmlCreatePushParser(ATTRIBUTE_UNUSED PyObject * self,
                           PyObject * args)
{
    const char *chunk;
    int size;
    const char *URI;
    PyObject *pyobj_SAX = NULL;
    xmlSAXHandlerPtr SAX = NULL;
    xmlParserCtxtPtr ret;
    PyObject *pyret;

    if (!PyArg_ParseTuple
        (args, "Oziz:xmlCreatePushParser", &pyobj_SAX, &chunk,
         &size, &URI))
        return (NULL);

    if (pyobj_SAX != Py_None) {
        SAX = &pythonSaxHandler;
        Py_INCREF(pyobj_SAX);
        /* The reference is released in pythonEndDocument() */
    }
    ret = xmlCreatePushParserCtxt(SAX, pyobj_SAX, chunk, size, URI);
    pyret = libxml_xmlParserCtxtPtrWrap(ret);
    return (pyret);
}

#ifdef LIBXML_HTML_ENABLED
PyObject *
libxml_htmlCreatePushParser(ATTRIBUTE_UNUSED PyObject * self,
                            PyObject * args)
{
    const char *chunk;
    int size;
    const char *URI;
    PyObject *pyobj_SAX = NULL;
    xmlSAXHandlerPtr SAX = NULL;
    xmlParserCtxtPtr ret;
    PyObject *pyret;

    if (!PyArg_ParseTuple
        (args, "Oziz:htmlCreatePushParser", &pyobj_SAX, &chunk,
         &size, &URI))
        return (NULL);

    if (pyobj_SAX != Py_None) {
        SAX = &pythonSaxHandler;
        Py_INCREF(pyobj_SAX);
        /* The reference is released in pythonEndDocument() */
    }
    ret = htmlCreatePushParserCtxt(SAX, pyobj_SAX, chunk, size, URI,
                                   XML_CHAR_ENCODING_NONE);
    pyret = libxml_xmlParserCtxtPtrWrap(ret);
    return (pyret);
}
#endif /* LIBXML_HTML_ENABLED */
#endif /* LIBXML_PUSH_ENABLED */

#ifdef LIBXML_SAX1_ENABLED
PyObject *
libxml_xmlSAXParseFile(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    int recover;
    const char *URI;
    PyObject *pyobj_SAX = NULL;
    xmlSAXHandlerPtr SAX = NULL;
    xmlParserCtxtPtr ctxt;

    if (!PyArg_ParseTuple(args, "Osi:xmlSAXParseFile", &pyobj_SAX,
                          &URI, &recover))
        return (NULL);

    if (pyobj_SAX == Py_None) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    SAX = &pythonSaxHandler;
    Py_INCREF(pyobj_SAX);
    /* The reference is released in pythonEndDocument() */
    ctxt = xmlNewSAXParserCtxt(SAX, pyobj_SAX);
    xmlCtxtReadFile(ctxt, URI, NULL, 0);
    xmlFreeParserCtxt(ctxt);
    Py_INCREF(Py_None);
    return (Py_None);
}
#endif /* LIBXML_SAX1_ENABLED */

#ifdef LIBXML_HTML_ENABLED
PyObject *
libxml_htmlSAXParseFile(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    const char *URI;
    const char *encoding;
    PyObject *pyobj_SAX = NULL;
    xmlSAXHandlerPtr SAX = NULL;
    htmlParserCtxtPtr ctxt;

    if (!PyArg_ParseTuple
        (args, "Osz:htmlSAXParseFile", &pyobj_SAX, &URI,
         &encoding))
        return (NULL);

    if (pyobj_SAX == Py_None) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    SAX = &pythonSaxHandler;
    Py_INCREF(pyobj_SAX);
    /* The reference is released in pythonEndDocument() */
    ctxt = htmlNewSAXParserCtxt(SAX, pyobj_SAX);
    htmlCtxtReadFile(ctxt, URI, encoding, 0);
    htmlFreeParserCtxt(ctxt);
    Py_INCREF(Py_None);
    return (Py_None);
}
#endif /* LIBXML_HTML_ENABLED */

/************************************************************************
 *									*
 *			Error message callback				*
 *									*
 ************************************************************************/

static PyObject *libxml_xmlPythonErrorFuncHandler = NULL;
static PyObject *libxml_xmlPythonErrorFuncCtxt = NULL;

/* helper to build a xmlMalloc'ed string from a format and va_list */
/* 
 * disabled the loop, the repeated call to vsnprintf without reset of ap
 * in case the initial buffer was too small segfaulted on x86_64
 * we now directly vsnprintf on a large buffer.
 */
static char *
libxml_buildMessage(const char *msg, va_list ap)
{
    size_t len = 1024;
    char *str;

    str = xmlMalloc(len);
    if (str == NULL)
        return NULL;

    vsnprintf(str, len, msg, ap);

    return str;
}

static void
libxml_xmlErrorFuncHandler(ATTRIBUTE_UNUSED void *ctx, const char *msg,
                           ...)
{
    va_list ap;
    PyObject *list;
    PyObject *message;
    PyObject *result;
    char str[1024];

    if (libxml_xmlPythonErrorFuncHandler == NULL) {
        va_start(ap, msg);
        vfprintf(stderr, msg, ap);
        va_end(ap);
    } else {
        va_start(ap, msg);
        vsnprintf(str, sizeof(str), msg, ap);
        va_end(ap);

        list = PyTuple_New(2);
        PyTuple_SetItem(list, 0, libxml_xmlPythonErrorFuncCtxt);
        Py_XINCREF(libxml_xmlPythonErrorFuncCtxt);
        message = libxml_charPtrConstWrap(str);
        PyTuple_SetItem(list, 1, message);
        result = PyObject_CallObject(libxml_xmlPythonErrorFuncHandler, list);
        Py_XDECREF(list);
        Py_XDECREF(result);
    }
}

static void
libxml_xmlErrorInitialize(void)
{
    xmlSetGenericErrorFunc(NULL, libxml_xmlErrorFuncHandler);
XML_IGNORE_DEPRECATION_WARNINGS
    xmlThrDefSetGenericErrorFunc(NULL, libxml_xmlErrorFuncHandler);
XML_POP_WARNINGS
}

static PyObject *
libxml_xmlRegisterErrorHandler(ATTRIBUTE_UNUSED PyObject * self,
                               PyObject * args)
{
    PyObject *py_retval;
    PyObject *pyobj_f;
    PyObject *pyobj_ctx;

    if (!PyArg_ParseTuple
        (args, "OO:xmlRegisterErrorHandler", &pyobj_f,
         &pyobj_ctx))
        return (NULL);

    if (libxml_xmlPythonErrorFuncHandler != NULL) {
        Py_XDECREF(libxml_xmlPythonErrorFuncHandler);
    }
    if (libxml_xmlPythonErrorFuncCtxt != NULL) {
        Py_XDECREF(libxml_xmlPythonErrorFuncCtxt);
    }

    Py_XINCREF(pyobj_ctx);
    Py_XINCREF(pyobj_f);

    /* TODO: check f is a function ! */
    libxml_xmlPythonErrorFuncHandler = pyobj_f;
    libxml_xmlPythonErrorFuncCtxt = pyobj_ctx;

    py_retval = libxml_intWrap(1);
    return (py_retval);
}


/************************************************************************
 *									*
 *                      Per parserCtxt error handler                    *
 *									*
 ************************************************************************/

typedef struct 
{
    PyObject *f;
    PyObject *arg;
} xmlParserCtxtPyCtxt;
typedef xmlParserCtxtPyCtxt *xmlParserCtxtPyCtxtPtr;

static void
libxml_xmlParserCtxtErrorHandler(void *ctx, const xmlError *error)
{
    PyObject *list;
    PyObject *result;
    xmlParserCtxtPtr ctxt;
    xmlParserCtxtPyCtxtPtr pyCtxt;
    int severity;
    
    ctxt = (xmlParserCtxtPtr)ctx;
    pyCtxt = (xmlParserCtxtPyCtxtPtr)ctxt->_private;

    if ((error->domain == XML_FROM_VALID) ||
        (error->domain == XML_FROM_DTD)) {
        if (error->level == XML_ERR_WARNING)
            severity = XML_PARSER_SEVERITY_VALIDITY_WARNING;
        else
            severity = XML_PARSER_SEVERITY_VALIDITY_ERROR;
    } else {
        if (error->level == XML_ERR_WARNING)
            severity = XML_PARSER_SEVERITY_WARNING;
        else
            severity = XML_PARSER_SEVERITY_ERROR;
    }

    list = PyTuple_New(4);
    PyTuple_SetItem(list, 0, pyCtxt->arg);
    Py_XINCREF(pyCtxt->arg);
    PyTuple_SetItem(list, 1, libxml_constcharPtrWrap(error->message));
    PyTuple_SetItem(list, 2, libxml_intWrap(severity));
    PyTuple_SetItem(list, 3, Py_None);
    Py_INCREF(Py_None);
    result = PyObject_CallObject(pyCtxt->f, list);
    if (result == NULL) 
    {
	/* TODO: manage for the exception to be propagated... */
	PyErr_Print();
    }
    Py_XDECREF(list);
    Py_XDECREF(result);
}

static PyObject *
libxml_xmlParserCtxtSetErrorHandler(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) 
{
    PyObject *py_retval;
    xmlParserCtxtPtr ctxt;
    xmlParserCtxtPyCtxtPtr pyCtxt;
    PyObject *pyobj_ctxt;
    PyObject *pyobj_f;
    PyObject *pyobj_arg;

    if (!PyArg_ParseTuple(args, "OOO:xmlParserCtxtSetErrorHandler",
		          &pyobj_ctxt, &pyobj_f, &pyobj_arg))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);
    if (ctxt->_private == NULL) {
	pyCtxt = xmlMalloc(sizeof(xmlParserCtxtPyCtxt));
	if (pyCtxt == NULL) {
	    py_retval = libxml_intWrap(-1);
	    return(py_retval);
	}
	memset(pyCtxt,0,sizeof(xmlParserCtxtPyCtxt));
	ctxt->_private = pyCtxt;
    }
    else {
	pyCtxt = (xmlParserCtxtPyCtxtPtr)ctxt->_private;
    }
    /* TODO: check f is a function ! */
    Py_XDECREF(pyCtxt->f);
    Py_XINCREF(pyobj_f);
    pyCtxt->f = pyobj_f;
    Py_XDECREF(pyCtxt->arg);
    Py_XINCREF(pyobj_arg);
    pyCtxt->arg = pyobj_arg;

    if (pyobj_f != Py_None) {
        xmlCtxtSetErrorHandler(ctxt, libxml_xmlParserCtxtErrorHandler, ctxt);
    }
    else {
        xmlCtxtSetErrorHandler(ctxt, NULL, NULL);
    }

    py_retval = libxml_intWrap(1);
    return(py_retval);
}

static PyObject *
libxml_xmlParserCtxtGetErrorHandler(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) 
{
    PyObject *py_retval;
    xmlParserCtxtPtr ctxt;
    xmlParserCtxtPyCtxtPtr pyCtxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, "O:xmlParserCtxtGetErrorHandler",
		          &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);
    py_retval = PyTuple_New(2);
    if (ctxt->_private != NULL) {
	pyCtxt = (xmlParserCtxtPyCtxtPtr)ctxt->_private;

	PyTuple_SetItem(py_retval, 0, pyCtxt->f);
	Py_XINCREF(pyCtxt->f);
	PyTuple_SetItem(py_retval, 1, pyCtxt->arg);
	Py_XINCREF(pyCtxt->arg);
    }
    else {
	/* no python error handler registered */
	PyTuple_SetItem(py_retval, 0, Py_None);
	Py_XINCREF(Py_None);
	PyTuple_SetItem(py_retval, 1, Py_None);
	Py_XINCREF(Py_None);
    }
    return(py_retval);
}

static PyObject *
libxml_xmlFreeParserCtxt(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) {
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;
    xmlParserCtxtPyCtxtPtr pyCtxt;

    if (!PyArg_ParseTuple(args, "O:xmlFreeParserCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    if (ctxt != NULL) {
	pyCtxt = (xmlParserCtxtPyCtxtPtr)((xmlParserCtxtPtr)ctxt)->_private;
	if (pyCtxt) {
	    Py_XDECREF(pyCtxt->f);
	    Py_XDECREF(pyCtxt->arg);
	    xmlFree(pyCtxt);
	}
	xmlFreeParserCtxt(ctxt);
    }

    Py_INCREF(Py_None);
    return(Py_None);
}

#ifdef LIBXML_VALID_ENABLED
/***
 * xmlValidCtxt stuff
 */

typedef struct 
{
    PyObject *warn;
    PyObject *error;
    PyObject *arg;
} xmlValidCtxtPyCtxt;
typedef xmlValidCtxtPyCtxt *xmlValidCtxtPyCtxtPtr;

static void
libxml_xmlValidCtxtGenericErrorFuncHandler(void *ctx, ATTRIBUTE_UNUSED int severity, char *str)
{
    PyObject *list;
    PyObject *result;
    xmlValidCtxtPyCtxtPtr pyCtxt;
    
    pyCtxt = (xmlValidCtxtPyCtxtPtr)ctx;
    
    list = PyTuple_New(2);
    PyTuple_SetItem(list, 0, libxml_charPtrWrap(str));
    PyTuple_SetItem(list, 1, pyCtxt->arg);
    Py_XINCREF(pyCtxt->arg);
    result = PyObject_CallObject(pyCtxt->error, list);
    if (result == NULL) 
    {
	/* TODO: manage for the exception to be propagated... */
	PyErr_Print();
    }
    Py_XDECREF(list);
    Py_XDECREF(result);
}

static void
libxml_xmlValidCtxtGenericWarningFuncHandler(void *ctx, ATTRIBUTE_UNUSED int severity, char *str)
{
    PyObject *list;
    PyObject *result;
    xmlValidCtxtPyCtxtPtr pyCtxt;
    
    pyCtxt = (xmlValidCtxtPyCtxtPtr)ctx;

    list = PyTuple_New(2);
    PyTuple_SetItem(list, 0, libxml_charPtrWrap(str));
    PyTuple_SetItem(list, 1, pyCtxt->arg);
    Py_XINCREF(pyCtxt->arg);
    result = PyObject_CallObject(pyCtxt->warn, list);
    if (result == NULL) 
    {
	/* TODO: manage for the exception to be propagated... */
	PyErr_Print();
    }
    Py_XDECREF(list);
    Py_XDECREF(result);
}

static void 
libxml_xmlValidCtxtErrorFuncHandler(void *ctx, const char *msg, ...) 
{
    va_list ap;

    va_start(ap, msg);
    libxml_xmlValidCtxtGenericErrorFuncHandler(ctx,XML_PARSER_SEVERITY_VALIDITY_ERROR,libxml_buildMessage(msg,ap));
    va_end(ap);
}

static void 
libxml_xmlValidCtxtWarningFuncHandler(void *ctx, const char *msg, ...) 
{
    va_list ap;

    va_start(ap, msg);
    libxml_xmlValidCtxtGenericWarningFuncHandler(ctx,XML_PARSER_SEVERITY_VALIDITY_WARNING,libxml_buildMessage(msg,ap));
    va_end(ap);
}

static PyObject *
libxml_xmlSetValidErrors(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval;
    PyObject *pyobj_error;
    PyObject *pyobj_warn;
    PyObject *pyobj_ctx;
    PyObject *pyobj_arg = Py_None;
    xmlValidCtxtPtr ctxt;
    xmlValidCtxtPyCtxtPtr pyCtxt;

    if (!PyArg_ParseTuple
        (args, "OOO|O:xmlSetValidErrors", &pyobj_ctx, &pyobj_error, &pyobj_warn, &pyobj_arg))
        return (NULL);

    ctxt = PyValidCtxt_Get(pyobj_ctx);
    pyCtxt = xmlMalloc(sizeof(xmlValidCtxtPyCtxt));
    if (pyCtxt == NULL) {
            py_retval = libxml_intWrap(-1);
            return(py_retval);
    }
    memset(pyCtxt, 0, sizeof(xmlValidCtxtPyCtxt));

    
    /* TODO: check warn and error is a function ! */
    Py_XDECREF(pyCtxt->error);
    Py_XINCREF(pyobj_error);
    pyCtxt->error = pyobj_error;
    
    Py_XDECREF(pyCtxt->warn);
    Py_XINCREF(pyobj_warn);
    pyCtxt->warn = pyobj_warn;
    
    Py_XDECREF(pyCtxt->arg);
    Py_XINCREF(pyobj_arg);
    pyCtxt->arg = pyobj_arg;

    ctxt->error = libxml_xmlValidCtxtErrorFuncHandler;
    ctxt->warning = libxml_xmlValidCtxtWarningFuncHandler;
    ctxt->userData = pyCtxt;

    py_retval = libxml_intWrap(1);
    return (py_retval);
}


static PyObject *
libxml_xmlFreeValidCtxt(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    xmlValidCtxtPtr cur;
    xmlValidCtxtPyCtxtPtr pyCtxt;
    PyObject *pyobj_cur;

    if (!PyArg_ParseTuple(args, "O:xmlFreeValidCtxt", &pyobj_cur))
        return(NULL);
    cur = (xmlValidCtxtPtr) PyValidCtxt_Get(pyobj_cur);

    pyCtxt = (xmlValidCtxtPyCtxtPtr)(cur->userData);
    if (pyCtxt != NULL)
    {
            Py_XDECREF(pyCtxt->error);
            Py_XDECREF(pyCtxt->warn);
            Py_XDECREF(pyCtxt->arg);
            xmlFree(pyCtxt);
    }

    xmlFreeValidCtxt(cur);
    Py_INCREF(Py_None);
    return(Py_None);
}
#endif /* LIBXML_VALID_ENABLED */

#ifdef LIBXML_READER_ENABLED
/************************************************************************
 *									*
 *                      Per xmlTextReader error handler                 *
 *									*
 ************************************************************************/

typedef struct 
{
    PyObject *f;
    PyObject *arg;
} xmlTextReaderPyCtxt;
typedef xmlTextReaderPyCtxt *xmlTextReaderPyCtxtPtr;

static void 
libxml_xmlTextReaderErrorCallback(void *arg, 
				  const char *msg,
				  xmlParserSeverities severity,
				  xmlTextReaderLocatorPtr locator)
{
    xmlTextReaderPyCtxt *pyCtxt = (xmlTextReaderPyCtxt *)arg;
    PyObject *list;
    PyObject *result;
    
    list = PyTuple_New(4);
    PyTuple_SetItem(list, 0, pyCtxt->arg);
    Py_XINCREF(pyCtxt->arg);
    PyTuple_SetItem(list, 1, libxml_charPtrConstWrap(msg));
    PyTuple_SetItem(list, 2, libxml_intWrap(severity));
    PyTuple_SetItem(list, 3, libxml_xmlTextReaderLocatorPtrWrap(locator));
    result = PyObject_CallObject(pyCtxt->f, list);
    if (result == NULL)
    {
	/* TODO: manage for the exception to be propagated... */
	PyErr_Print();
    }
    Py_XDECREF(list);
    Py_XDECREF(result);
}

static PyObject *
libxml_xmlTextReaderSetErrorHandler(ATTRIBUTE_UNUSED PyObject *self, PyObject *args)
{
    xmlTextReaderPtr reader;
    xmlTextReaderPyCtxtPtr pyCtxt;
    xmlTextReaderErrorFunc f;
    void *arg;
    PyObject *pyobj_reader;
    PyObject *pyobj_f;
    PyObject *pyobj_arg;
    PyObject *py_retval;

    if (!PyArg_ParseTuple(args, "OOO:xmlTextReaderSetErrorHandler", &pyobj_reader, &pyobj_f, &pyobj_arg))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);
    /* clear previous error handler */
    xmlTextReaderGetErrorHandler(reader,&f,&arg);
    if (arg != NULL) {
	if (f == (xmlTextReaderErrorFunc) libxml_xmlTextReaderErrorCallback) {
	    /* ok, it's our error handler! */
	    pyCtxt = (xmlTextReaderPyCtxtPtr)arg;
	    Py_XDECREF(pyCtxt->f);
	    Py_XDECREF(pyCtxt->arg);
	    xmlFree(pyCtxt);
	}
	else {
	    /* 
	     * there already an arg, and it's not ours,
	     * there is definitely something wrong going on here...
	     * we don't know how to free it, so we bail out... 
	     */
	    py_retval = libxml_intWrap(-1);
	    return(py_retval);
	}
    }
    xmlTextReaderSetErrorHandler(reader,NULL,NULL);
    /* set new error handler */
    if (pyobj_f != Py_None)
    {
	pyCtxt = (xmlTextReaderPyCtxtPtr)xmlMalloc(sizeof(xmlTextReaderPyCtxt));
	if (pyCtxt == NULL) {
	    py_retval = libxml_intWrap(-1);
	    return(py_retval);
	}
	Py_XINCREF(pyobj_f);
	pyCtxt->f = pyobj_f;
	Py_XINCREF(pyobj_arg);
	pyCtxt->arg = pyobj_arg;
	xmlTextReaderSetErrorHandler(reader,
	                             libxml_xmlTextReaderErrorCallback,
	                             pyCtxt);
    }

    py_retval = libxml_intWrap(1);
    return(py_retval);
}

static PyObject *
libxml_xmlTextReaderGetErrorHandler(ATTRIBUTE_UNUSED PyObject *self, PyObject *args)
{
    xmlTextReaderPtr reader;
    xmlTextReaderPyCtxtPtr pyCtxt;
    xmlTextReaderErrorFunc f;
    void *arg;
    PyObject *pyobj_reader;
    PyObject *py_retval;

    if (!PyArg_ParseTuple(args, "O:xmlTextReaderSetErrorHandler", &pyobj_reader))
        return(NULL);
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);
    xmlTextReaderGetErrorHandler(reader,&f,&arg);
    py_retval = PyTuple_New(2);
    if (f == (xmlTextReaderErrorFunc)libxml_xmlTextReaderErrorCallback) {
	/* ok, it's our error handler! */
	pyCtxt = (xmlTextReaderPyCtxtPtr)arg;
	PyTuple_SetItem(py_retval, 0, pyCtxt->f);
	Py_XINCREF(pyCtxt->f);
	PyTuple_SetItem(py_retval, 1, pyCtxt->arg);
	Py_XINCREF(pyCtxt->arg);
    }
    else
    {
	/* f is null or it's not our error handler */
	PyTuple_SetItem(py_retval, 0, Py_None);
	Py_XINCREF(Py_None);
	PyTuple_SetItem(py_retval, 1, Py_None);
	Py_XINCREF(Py_None);
    }
    return(py_retval);
}

static PyObject *
libxml_xmlFreeTextReader(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) {
    xmlTextReaderPtr reader;
    PyObject *pyobj_reader;
    xmlTextReaderPyCtxtPtr pyCtxt;
    xmlTextReaderErrorFunc f;
    void *arg;

    if (!PyArg_ParseTuple(args, "O:xmlFreeTextReader", &pyobj_reader))
        return(NULL);
    if (!PyCapsule_CheckExact(pyobj_reader)) {
	Py_INCREF(Py_None);
	return(Py_None);
    }
    reader = (xmlTextReaderPtr) PyxmlTextReader_Get(pyobj_reader);
    if (reader == NULL) {
	Py_INCREF(Py_None);
	return(Py_None);
    }

    xmlTextReaderGetErrorHandler(reader,&f,&arg);
    if (arg != NULL) {
	if (f == (xmlTextReaderErrorFunc) libxml_xmlTextReaderErrorCallback) {
	    /* ok, it's our error handler! */
	    pyCtxt = (xmlTextReaderPyCtxtPtr)arg;
	    Py_XDECREF(pyCtxt->f);
	    Py_XDECREF(pyCtxt->arg);
	    xmlFree(pyCtxt);
	}
	/* 
	 * else, something wrong happened, because the error handler is
	 * not owned by the python bindings...
	 */
    }

    xmlFreeTextReader(reader);
    Py_INCREF(Py_None);
    return(Py_None);
}
#endif

/************************************************************************
 *									*
 *			XPath extensions				*
 *									*
 ************************************************************************/

#ifdef LIBXML_XPATH_ENABLED
static void
libxml_xmlXPathFuncCallback(xmlXPathParserContextPtr ctxt, int nargs)
{
    PyObject *list, *cur, *result;
    xmlXPathObjectPtr obj;
    xmlXPathContextPtr rctxt;
    PyObject *current_function = NULL;
    const xmlChar *name;
    const xmlChar *ns_uri;
    int i;

    if (ctxt == NULL)
        return;
    rctxt = ctxt->context;
    if (rctxt == NULL)
        return;
    name = rctxt->function;
    ns_uri = rctxt->functionURI;

    /*
     * Find the function, it should be there it was there at lookup
     */
    for (i = 0; i < libxml_xpathCallbacksNb; i++) {
        if (                    /* TODO (ctxt == libxml_xpathCallbacks[i].ctx) && */
						(xmlStrEqual(name, (*libxml_xpathCallbacks)[i].name)) &&
               (xmlStrEqual(ns_uri, (*libxml_xpathCallbacks)[i].ns_uri))) {
					current_function = (*libxml_xpathCallbacks)[i].function;
        }
    }
    if (current_function == NULL) {
        printf
            ("libxml_xmlXPathFuncCallback: internal error %s not found !\n",
             name);
        return;
    }

    list = PyTuple_New(nargs + 1);
    PyTuple_SetItem(list, 0, libxml_xmlXPathParserContextPtrWrap(ctxt));
    for (i = nargs - 1; i >= 0; i--) {
        obj = xmlXPathValuePop(ctxt);
        cur = libxml_xmlXPathObjectPtrWrap(obj);
        PyTuple_SetItem(list, i + 1, cur);
    }
    result = PyObject_CallObject(current_function, list);
    Py_DECREF(list);

    obj = libxml_xmlXPathObjectPtrConvert(result);
    xmlXPathValuePush(ctxt, obj);
}

static xmlXPathFunction
libxml_xmlXPathFuncLookupFunc(void *ctxt, const xmlChar * name,
                              const xmlChar * ns_uri)
{
    int i;

    /*
     * This is called once only. The address is then stored in the
     * XPath expression evaluation, the proper object to call can
     * then still be found using the execution context function
     * and functionURI fields.
     */
    for (i = 0; i < libxml_xpathCallbacksNb; i++) {
			if ((ctxt == (*libxml_xpathCallbacks)[i].ctx) &&
					(xmlStrEqual(name, (*libxml_xpathCallbacks)[i].name)) &&
					(xmlStrEqual(ns_uri, (*libxml_xpathCallbacks)[i].ns_uri))) {
            return (libxml_xmlXPathFuncCallback);
        }
    }
    return (NULL);
}

static void
libxml_xpathCallbacksInitialize(void)
{
    int i;

    if (libxml_xpathCallbacksInitialized != 0)
        return;

    libxml_xpathCallbacks = (libxml_xpathCallbackArray*)xmlMalloc(
    		libxml_xpathCallbacksAllocd*sizeof(libxml_xpathCallback));

    for (i = 0; i < libxml_xpathCallbacksAllocd; i++) {
			(*libxml_xpathCallbacks)[i].ctx = NULL;
			(*libxml_xpathCallbacks)[i].name = NULL;
			(*libxml_xpathCallbacks)[i].ns_uri = NULL;
			(*libxml_xpathCallbacks)[i].function = NULL;
    }
    libxml_xpathCallbacksInitialized = 1;
}

PyObject *
libxml_xmlRegisterXPathFunction(ATTRIBUTE_UNUSED PyObject * self,
                                PyObject * args)
{
    PyObject *py_retval;
    int c_retval = 0;
    xmlChar *name;
    xmlChar *ns_uri;
    xmlXPathContextPtr ctx;
    PyObject *pyobj_ctx;
    PyObject *pyobj_f;
    int i;

    if (!PyArg_ParseTuple
        (args, "OszO:registerXPathFunction", &pyobj_ctx, &name,
         &ns_uri, &pyobj_f))
        return (NULL);

    ctx = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctx);
    if (libxml_xpathCallbacksInitialized == 0)
        libxml_xpathCallbacksInitialize();
    xmlXPathRegisterFuncLookup(ctx, libxml_xmlXPathFuncLookupFunc, ctx);

    if ((pyobj_ctx == NULL) || (name == NULL) || (pyobj_f == NULL)) {
        py_retval = libxml_intWrap(-1);
        return (py_retval);
    }
    for (i = 0; i < libxml_xpathCallbacksNb; i++) {
	if ((ctx == (*libxml_xpathCallbacks)[i].ctx) &&
            (xmlStrEqual(name, (*libxml_xpathCallbacks)[i].name)) &&
            (xmlStrEqual(ns_uri, (*libxml_xpathCallbacks)[i].ns_uri))) {
            Py_XINCREF(pyobj_f);
            Py_XDECREF((*libxml_xpathCallbacks)[i].function);
            (*libxml_xpathCallbacks)[i].function = pyobj_f;
            c_retval = 1;
            goto done;
        }
    }
    if (libxml_xpathCallbacksNb >= libxml_xpathCallbacksAllocd) {
			libxml_xpathCallbacksAllocd+=10;
	libxml_xpathCallbacks = (libxml_xpathCallbackArray*)xmlRealloc(
		libxml_xpathCallbacks,
		libxml_xpathCallbacksAllocd*sizeof(libxml_xpathCallback));
    } 
    i = libxml_xpathCallbacksNb++;
    Py_XINCREF(pyobj_f);
    (*libxml_xpathCallbacks)[i].ctx = ctx;
    (*libxml_xpathCallbacks)[i].name = xmlStrdup(name);
    (*libxml_xpathCallbacks)[i].ns_uri = xmlStrdup(ns_uri);
    (*libxml_xpathCallbacks)[i].function = pyobj_f;
        c_retval = 1;
    
  done:
    py_retval = libxml_intWrap((int) c_retval);
    return (py_retval);
}

PyObject *
libxml_xmlXPathRegisterVariable(ATTRIBUTE_UNUSED PyObject * self,
                                PyObject * args)
{
    PyObject *py_retval;
    int c_retval = 0;
    xmlChar *name;
    xmlChar *ns_uri;
    xmlXPathContextPtr ctx;
    xmlXPathObjectPtr val;
    PyObject *pyobj_ctx;
    PyObject *pyobj_value;

    if (!PyArg_ParseTuple
        (args, "OszO:xpathRegisterVariable", &pyobj_ctx, &name,
         &ns_uri, &pyobj_value))
        return (NULL);

    ctx = (xmlXPathContextPtr) PyxmlXPathContext_Get(pyobj_ctx);
    val = libxml_xmlXPathObjectPtrConvert(pyobj_value);

    c_retval = xmlXPathRegisterVariableNS(ctx, name, ns_uri, val);
    py_retval = libxml_intWrap(c_retval);
    return (py_retval);
}
#endif /* LIBXML_XPATH_ENABLED */

/************************************************************************
 *									*
 *			Global properties access			*
 *									*
 ************************************************************************/
static PyObject *
libxml_name(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    const xmlChar *res;

    if (!PyArg_ParseTuple(args, "O:name", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:{
                xmlDocPtr doc = (xmlDocPtr) cur;

                res = doc->URL;
                break;
            }
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = attr->name;
                break;
            }
        case XML_NAMESPACE_DECL:{
                xmlNsPtr ns = (xmlNsPtr) cur;

                res = ns->prefix;
                break;
            }
        default:
            res = cur->name;
            break;
    }
    resultobj = libxml_constxmlCharPtrWrap(res);

    return resultobj;
}

static PyObject *
libxml_doc(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlDocPtr res;

    if (!PyArg_ParseTuple(args, "O:doc", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
            res = NULL;
            break;
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = attr->doc;
                break;
            }
        case XML_NAMESPACE_DECL:
            res = NULL;
            break;
        default:
            res = cur->doc;
            break;
    }
    resultobj = libxml_xmlDocPtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_properties(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlAttrPtr res;

    if (!PyArg_ParseTuple(args, "O:properties", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);
    if ((cur != NULL) && (cur->type == XML_ELEMENT_NODE))
        res = cur->properties;
    else
        res = NULL;
    resultobj = libxml_xmlAttrPtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_next(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlNodePtr res;

    if (!PyArg_ParseTuple(args, "O:next", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
            res = NULL;
            break;
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = (xmlNodePtr) attr->next;
                break;
            }
        case XML_NAMESPACE_DECL:{
                xmlNsPtr ns = (xmlNsPtr) cur;

                res = (xmlNodePtr) ns->next;
                break;
            }
        default:
            res = cur->next;
            break;

    }
    resultobj = libxml_xmlNodePtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_prev(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlNodePtr res;

    if (!PyArg_ParseTuple(args, "O:prev", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
            res = NULL;
            break;
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = (xmlNodePtr) attr->prev;
            }
            break;
        case XML_NAMESPACE_DECL:
            res = NULL;
            break;
        default:
            res = cur->prev;
            break;
    }
    resultobj = libxml_xmlNodePtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_children(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlNodePtr res;

    if (!PyArg_ParseTuple(args, "O:children", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_ELEMENT_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_ENTITY_NODE:
        case XML_PI_NODE:
        case XML_COMMENT_NODE:
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
        case XML_DTD_NODE:
            res = cur->children;
            break;
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = attr->children;
                break;
            }
        default:
            res = NULL;
            break;
    }
    resultobj = libxml_xmlNodePtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_last(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlNodePtr res;

    if (!PyArg_ParseTuple(args, "O:last", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_ELEMENT_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_ENTITY_NODE:
        case XML_PI_NODE:
        case XML_COMMENT_NODE:
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
        case XML_DTD_NODE:
            res = cur->last;
            break;
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = attr->last;
		break;
            }
        default:
            res = NULL;
            break;
    }
    resultobj = libxml_xmlNodePtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_parent(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    xmlNodePtr res;

    if (!PyArg_ParseTuple(args, "O:parent", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);

    switch (cur->type) {
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
            res = NULL;
            break;
        case XML_ATTRIBUTE_NODE:{
                xmlAttrPtr attr = (xmlAttrPtr) cur;

                res = attr->parent;
            }
	    break;
        case XML_ENTITY_DECL:
        case XML_NAMESPACE_DECL:
        case XML_XINCLUDE_START:
        case XML_XINCLUDE_END:
            res = NULL;
            break;
        default:
            res = cur->parent;
            break;
    }
    resultobj = libxml_xmlNodePtrWrap(res);
    return resultobj;
}

static PyObject *
libxml_type(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *resultobj, *obj;
    xmlNodePtr cur;
    const xmlChar *res = NULL;

    if (!PyArg_ParseTuple(args, "O:last", &obj))
        return NULL;
    cur = PyxmlNode_Get(obj);
    if (cur == NULL) {
        Py_INCREF(Py_None);
	return (Py_None);
    }

    switch (cur->type) {
        case XML_ELEMENT_NODE:
            res = (const xmlChar *) "element";
            break;
        case XML_ATTRIBUTE_NODE:
            res = (const xmlChar *) "attribute";
            break;
        case XML_TEXT_NODE:
            res = (const xmlChar *) "text";
            break;
        case XML_CDATA_SECTION_NODE:
            res = (const xmlChar *) "cdata";
            break;
        case XML_ENTITY_REF_NODE:
            res = (const xmlChar *) "entity_ref";
            break;
        case XML_ENTITY_NODE:
            res = (const xmlChar *) "entity";
            break;
        case XML_PI_NODE:
            res = (const xmlChar *) "pi";
            break;
        case XML_COMMENT_NODE:
            res = (const xmlChar *) "comment";
            break;
        case XML_DOCUMENT_NODE:
            res = (const xmlChar *) "document_xml";
            break;
        case XML_DOCUMENT_TYPE_NODE:
            res = (const xmlChar *) "doctype";
            break;
        case XML_DOCUMENT_FRAG_NODE:
            res = (const xmlChar *) "fragment";
            break;
        case XML_NOTATION_NODE:
            res = (const xmlChar *) "notation";
            break;
        case XML_HTML_DOCUMENT_NODE:
            res = (const xmlChar *) "document_html";
            break;
        case XML_DTD_NODE:
            res = (const xmlChar *) "dtd";
            break;
        case XML_ELEMENT_DECL:
            res = (const xmlChar *) "elem_decl";
            break;
        case XML_ATTRIBUTE_DECL:
            res = (const xmlChar *) "attribute_decl";
            break;
        case XML_ENTITY_DECL:
            res = (const xmlChar *) "entity_decl";
            break;
        case XML_NAMESPACE_DECL:
            res = (const xmlChar *) "namespace";
            break;
        case XML_XINCLUDE_START:
            res = (const xmlChar *) "xinclude_start";
            break;
        case XML_XINCLUDE_END:
            res = (const xmlChar *) "xinclude_end";
            break;
    }

    resultobj = libxml_constxmlCharPtrWrap(res);
    return resultobj;
}

/************************************************************************
 *									*
 *			Specific accessor functions			*
 *									*
 ************************************************************************/
PyObject *
libxml_xmlNodeGetNsDefs(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple
        (args, "O:xmlNodeGetNsDefs", &pyobj_node))
        return (NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    if ((node == NULL) || (node->type != XML_ELEMENT_NODE)) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    c_retval = node->nsDef;
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return (py_retval);
}

PyObject *
libxml_xmlNodeRemoveNsDef(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval;
    xmlNsPtr ns, prev;
    xmlNodePtr node;
    PyObject *pyobj_node;
    xmlChar *href;
    xmlNsPtr c_retval;

    if (!PyArg_ParseTuple
        (args, "Oz:xmlNodeRemoveNsDef", &pyobj_node, &href))
        return (NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    ns = NULL;

    if ((node == NULL) || (node->type != XML_ELEMENT_NODE)) {
        Py_INCREF(Py_None);
        return (Py_None);
    }

    if (href == NULL) {
	ns = node->nsDef;
	node->nsDef = NULL;
	c_retval = 0;
    }
    else {
	prev = NULL;
	ns = node->nsDef;
	while (ns != NULL) {
	    if (xmlStrEqual(ns->href, href)) {
		if (prev != NULL)
		    prev->next = ns->next;
		else
		    node->nsDef = ns->next;
		ns->next = NULL;
		c_retval = 0;
		break;
	    }
	    prev = ns;
	    ns = ns->next;
	}
    }

    c_retval = ns;
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return (py_retval);
}

PyObject *
libxml_xmlNodeGetNs(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval;
    xmlNsPtr c_retval;
    xmlNodePtr node;
    PyObject *pyobj_node;

    if (!PyArg_ParseTuple(args, "O:xmlNodeGetNs", &pyobj_node))
        return (NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    if ((node == NULL) ||
        ((node->type != XML_ELEMENT_NODE) &&
	 (node->type != XML_ATTRIBUTE_NODE))) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    c_retval = node->ns;
    py_retval = libxml_xmlNsPtrWrap((xmlNsPtr) c_retval);
    return (py_retval);
}

#ifdef LIBXML_OUTPUT_ENABLED
/************************************************************************
 *									*
 *			Serialization front-end				*
 *									*
 ************************************************************************/

static PyObject *
libxml_serializeNode(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval = NULL;
    xmlChar *c_retval;
    PyObject *pyobj_node;
    xmlNodePtr node;
    xmlDocPtr doc;
    const char *encoding;
    int format;
    xmlSaveCtxtPtr ctxt;
    xmlBufferPtr buf;
    int options = 0;

    if (!PyArg_ParseTuple(args, "Ozi:serializeNode", &pyobj_node,
                          &encoding, &format))
        return (NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);

    if (node == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    if (node->type == XML_DOCUMENT_NODE) {
        doc = (xmlDocPtr) node;
	node = NULL;
#ifdef LIBXML_HTML_ENABLED
    } else if (node->type == XML_HTML_DOCUMENT_NODE) {
        doc = (xmlDocPtr) node;
	node = NULL;
#endif
    } else {
        if (node->type == XML_NAMESPACE_DECL)
	    doc = NULL;
	else
            doc = node->doc;
        if ((doc == NULL) || (doc->type == XML_DOCUMENT_NODE)) {
#ifdef LIBXML_HTML_ENABLED
        } else if (doc->type == XML_HTML_DOCUMENT_NODE) {
#endif /* LIBXML_HTML_ENABLED */
        } else {
            Py_INCREF(Py_None);
            return (Py_None);
        }
    }


    buf = xmlBufferCreate();
    if (buf == NULL) {
	Py_INCREF(Py_None);
	return (Py_None);
    }
    if (format) options |= XML_SAVE_FORMAT;
    ctxt = xmlSaveToBuffer(buf, encoding, options);
    if (ctxt == NULL) {
	xmlBufferFree(buf);
	Py_INCREF(Py_None);
	return (Py_None);
    }
    if (node == NULL)
	xmlSaveDoc(ctxt, doc);
    else
	xmlSaveTree(ctxt, node);
    xmlSaveClose(ctxt);

    c_retval = xmlBufferDetach(buf);

    xmlBufferFree(buf);
    py_retval = libxml_charPtrWrap((char *) c_retval);

    return (py_retval);
}

static PyObject *
libxml_saveNodeTo(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_file = NULL;
    FILE *output;
    PyObject *pyobj_node;
    xmlNodePtr node;
    xmlDocPtr doc;
    const char *encoding;
    int format;
    int len;
    xmlOutputBufferPtr buf;
    xmlCharEncodingHandlerPtr handler = NULL;

    if (!PyArg_ParseTuple(args, "OOzi:serializeNode", &pyobj_node,
                          &py_file, &encoding, &format))
        return (NULL);
    node = (xmlNodePtr) PyxmlNode_Get(pyobj_node);
    if (node == NULL) {
        return (PyLong_FromLong((long) -1));
    }
    output = PyFile_Get(py_file);
    if (output == NULL) {
        return (PyLong_FromLong((long) -1));
    }

    if (node->type == XML_DOCUMENT_NODE) {
        doc = (xmlDocPtr) node;
    } else if (node->type == XML_HTML_DOCUMENT_NODE) {
        doc = (xmlDocPtr) node;
    } else {
        doc = node->doc;
    }
#ifdef LIBXML_HTML_ENABLED
    if (doc->type == XML_HTML_DOCUMENT_NODE) {
        if (encoding == NULL)
            encoding = (const char *) htmlGetMetaEncoding(doc);
    }
#endif /* LIBXML_HTML_ENABLED */
    if (encoding != NULL) {
        handler = xmlFindCharEncodingHandler(encoding);
        if (handler == NULL) {
            PyFile_Release(output);
            return (PyLong_FromLong((long) -1));
        }
    }
    if (doc->type == XML_HTML_DOCUMENT_NODE) {
        if (handler == NULL)
            handler = xmlFindCharEncodingHandler("HTML");
        if (handler == NULL)
            handler = xmlFindCharEncodingHandler("ascii");
    }

    buf = xmlOutputBufferCreateFile(output, handler);
    if (node->type == XML_DOCUMENT_NODE) {
        len = xmlSaveFormatFileTo(buf, doc, encoding, format);
#ifdef LIBXML_HTML_ENABLED
    } else if (node->type == XML_HTML_DOCUMENT_NODE) {
        htmlDocContentDumpFormatOutput(buf, doc, encoding, format);
        len = xmlOutputBufferClose(buf);
    } else if (doc->type == XML_HTML_DOCUMENT_NODE) {
        htmlNodeDumpFormatOutput(buf, doc, node, encoding, format);
        len = xmlOutputBufferClose(buf);
#endif /* LIBXML_HTML_ENABLED */
    } else {
        xmlNodeDumpOutput(buf, doc, node, 0, format, encoding);
        len = xmlOutputBufferClose(buf);
    }
    PyFile_Release(output);
    return (PyLong_FromLong((long) len));
}
#endif /* LIBXML_OUTPUT_ENABLED */

/************************************************************************
 *									*
 *			Extra stuff					*
 *									*
 ************************************************************************/
PyObject *
libxml_xmlNewNode(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval;
    xmlChar *name;
    xmlNodePtr node;

    if (!PyArg_ParseTuple(args, "s:xmlNewNode", &name))
        return (NULL);
    node = (xmlNodePtr) xmlNewNode(NULL, name);

    if (node == NULL) {
        Py_INCREF(Py_None);
        return (Py_None);
    }
    py_retval = libxml_xmlNodePtrWrap(node);
    return (py_retval);
}


/************************************************************************
 *									*
 *			Local Catalog stuff				*
 *									*
 ************************************************************************/

#ifdef LIBXML_CATALOG_ENABLED
static PyObject *
libxml_addLocalCatalog(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    xmlChar *URL;
    xmlParserCtxtPtr ctxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, "Os:addLocalCatalog", &pyobj_ctxt, &URL))
        return(NULL);

    ctxt = (xmlParserCtxtPtr) PyparserCtxt_Get(pyobj_ctxt);

    if (URL != NULL) {
        void *catalogs = xmlCtxtGetCatalogs(ctxt);
        xmlCtxtSetCatalogs(ctxt, xmlCatalogAddLocal(catalogs, URL));
    }

    Py_INCREF(Py_None);
    return (Py_None);
}
#endif /* LIBXML_CATALOG_ENABLED */

/************************************************************************
 *                                                                      *
 * RelaxNG error handler registration                                   *
 *                                                                      *
 ************************************************************************/

#ifdef LIBXML_RELAXNG_ENABLED
typedef struct 
{
    PyObject *warn;
    PyObject *error;
    PyObject *arg;
} xmlRelaxNGValidCtxtPyCtxt;
typedef xmlRelaxNGValidCtxtPyCtxt *xmlRelaxNGValidCtxtPyCtxtPtr;

static void
libxml_xmlRelaxNGValidityGenericErrorFuncHandler(void *ctx, char *str) 
{
    PyObject *list;
    PyObject *result;
    xmlRelaxNGValidCtxtPyCtxtPtr pyCtxt;
    
    pyCtxt = (xmlRelaxNGValidCtxtPyCtxtPtr)ctx;

    list = PyTuple_New(2);
    PyTuple_SetItem(list, 0, libxml_charPtrWrap(str));
    PyTuple_SetItem(list, 1, pyCtxt->arg);
    Py_XINCREF(pyCtxt->arg);
    result = PyObject_CallObject(pyCtxt->error, list);
    if (result == NULL) 
    {
        /* TODO: manage for the exception to be propagated... */
        PyErr_Print();
    }
    Py_XDECREF(list);
    Py_XDECREF(result);
}

static void
libxml_xmlRelaxNGValidityGenericWarningFuncHandler(void *ctx, char *str) 
{
    PyObject *list;
    PyObject *result;
    xmlRelaxNGValidCtxtPyCtxtPtr pyCtxt;
    
    pyCtxt = (xmlRelaxNGValidCtxtPyCtxtPtr)ctx;

    list = PyTuple_New(2);
    PyTuple_SetItem(list, 0, libxml_charPtrWrap(str));
    PyTuple_SetItem(list, 1, pyCtxt->arg);
    Py_XINCREF(pyCtxt->arg);
    result = PyObject_CallObject(pyCtxt->warn, list);
    if (result == NULL) 
    {
        /* TODO: manage for the exception to be propagated... */
        PyErr_Print();
    }
    Py_XDECREF(list);
    Py_XDECREF(result);
}

static void
libxml_xmlRelaxNGValidityErrorFunc(void *ctx, const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    libxml_xmlRelaxNGValidityGenericErrorFuncHandler(ctx, libxml_buildMessage(msg, ap));
    va_end(ap);
}

static void
libxml_xmlRelaxNGValidityWarningFunc(void *ctx, const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    libxml_xmlRelaxNGValidityGenericWarningFuncHandler(ctx, libxml_buildMessage(msg, ap));
    va_end(ap);
}

static PyObject *
libxml_xmlRelaxNGSetValidErrors(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
    PyObject *py_retval;
    PyObject *pyobj_error;
    PyObject *pyobj_warn;
    PyObject *pyobj_ctx;
    PyObject *pyobj_arg = Py_None;
    xmlRelaxNGValidCtxtPtr ctxt;
    xmlRelaxNGValidCtxtPyCtxtPtr pyCtxt;

    if (!PyArg_ParseTuple
        (args, "OOO|O:xmlRelaxNGSetValidErrors", &pyobj_ctx, &pyobj_error, &pyobj_warn, &pyobj_arg))
        return (NULL);

    ctxt = PyrelaxNgValidCtxt_Get(pyobj_ctx);
    if (xmlRelaxNGGetValidErrors(ctxt, NULL, NULL, (void **) &pyCtxt) == -1)
    {
        py_retval = libxml_intWrap(-1);
        return(py_retval);
    }
    
    if (pyCtxt == NULL)
    {
        /* first time to set the error handlers */
        pyCtxt = xmlMalloc(sizeof(xmlRelaxNGValidCtxtPyCtxt));
        if (pyCtxt == NULL) {
            py_retval = libxml_intWrap(-1);
            return(py_retval);
        }
        memset(pyCtxt, 0, sizeof(xmlRelaxNGValidCtxtPyCtxt));
    }
    
    /* TODO: check warn and error is a function ! */
    Py_XDECREF(pyCtxt->error);
    Py_XINCREF(pyobj_error);
    pyCtxt->error = pyobj_error;
    
    Py_XDECREF(pyCtxt->warn);
    Py_XINCREF(pyobj_warn);
    pyCtxt->warn = pyobj_warn;
    
    Py_XDECREF(pyCtxt->arg);
    Py_XINCREF(pyobj_arg);
    pyCtxt->arg = pyobj_arg;

    xmlRelaxNGSetValidErrors(ctxt, &libxml_xmlRelaxNGValidityErrorFunc, &libxml_xmlRelaxNGValidityWarningFunc, pyCtxt);

    py_retval = libxml_intWrap(1);
    return (py_retval);
}

static PyObject *
libxml_xmlRelaxNGFreeValidCtxt(ATTRIBUTE_UNUSED PyObject *self, PyObject *args) {
    xmlRelaxNGValidCtxtPtr ctxt;
    xmlRelaxNGValidCtxtPyCtxtPtr pyCtxt;
    PyObject *pyobj_ctxt;

    if (!PyArg_ParseTuple(args, "O:xmlRelaxNGFreeValidCtxt", &pyobj_ctxt))
        return(NULL);
    ctxt = (xmlRelaxNGValidCtxtPtr) PyrelaxNgValidCtxt_Get(pyobj_ctxt);

    if (xmlRelaxNGGetValidErrors(ctxt, NULL, NULL, (void **) &pyCtxt) == 0)
    {
        if (pyCtxt != NULL)
        {
            Py_XDECREF(pyCtxt->error);
            Py_XDECREF(pyCtxt->warn);
            Py_XDECREF(pyCtxt->arg);
            xmlFree(pyCtxt);
        }
    }
    
    xmlRelaxNGFreeValidCtxt(ctxt);
    Py_INCREF(Py_None);
    return(Py_None);
}
#endif /* LIBXML_RELAXNG_ENABLED */

#ifdef LIBXML_SCHEMAS_ENABLED
typedef struct
{
	PyObject *warn;
	PyObject *error;
	PyObject *arg;
} xmlSchemaValidCtxtPyCtxt;
typedef xmlSchemaValidCtxtPyCtxt *xmlSchemaValidCtxtPyCtxtPtr;

static void
libxml_xmlSchemaValidityGenericErrorFuncHandler(void *ctx, char *str)
{
	PyObject *list;
	PyObject *result;
	xmlSchemaValidCtxtPyCtxtPtr pyCtxt;

	pyCtxt = (xmlSchemaValidCtxtPyCtxtPtr) ctx;

	list = PyTuple_New(2);
	PyTuple_SetItem(list, 0, libxml_charPtrWrap(str));
	PyTuple_SetItem(list, 1, pyCtxt->arg);
	Py_XINCREF(pyCtxt->arg);
	result = PyObject_CallObject(pyCtxt->error, list);
	if (result == NULL) 
	{
		/* TODO: manage for the exception to be propagated... */
		PyErr_Print();
	}
	Py_XDECREF(list);
	Py_XDECREF(result);
}

static void
libxml_xmlSchemaValidityGenericWarningFuncHandler(void *ctx, char *str)
{
	PyObject *list;
	PyObject *result;
	xmlSchemaValidCtxtPyCtxtPtr pyCtxt;

	pyCtxt = (xmlSchemaValidCtxtPyCtxtPtr) ctx;

	list = PyTuple_New(2);
	PyTuple_SetItem(list, 0, libxml_charPtrWrap(str));
	PyTuple_SetItem(list, 1, pyCtxt->arg);
	Py_XINCREF(pyCtxt->arg);
	result = PyObject_CallObject(pyCtxt->warn, list);
	if (result == NULL)
	{
		/* TODO: manage for the exception to be propagated... */
		PyErr_Print();
	}
	Py_XDECREF(list);
	Py_XDECREF(result);
}

static void
libxml_xmlSchemaValidityErrorFunc(void *ctx, const char *msg, ...)
{
	va_list ap;
	
	va_start(ap, msg);
	libxml_xmlSchemaValidityGenericErrorFuncHandler(ctx, libxml_buildMessage(msg, ap));
	va_end(ap);
}

static void
libxml_xmlSchemaValidityWarningFunc(void *ctx, const char *msg, ...)
{
	va_list ap;

	va_start(ap, msg);
	libxml_xmlSchemaValidityGenericWarningFuncHandler(ctx, libxml_buildMessage(msg, ap));
	va_end(ap);
}

PyObject *
libxml_xmlSchemaSetValidErrors(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
	PyObject *py_retval;
	PyObject *pyobj_error;
	PyObject *pyobj_warn;
	PyObject *pyobj_ctx;
	PyObject *pyobj_arg = Py_None;
	xmlSchemaValidCtxtPtr ctxt;
	xmlSchemaValidCtxtPyCtxtPtr pyCtxt;

	if (!PyArg_ParseTuple
		(args, "OOO|O:xmlSchemaSetValidErrors", &pyobj_ctx, &pyobj_error, &pyobj_warn, &pyobj_arg))
		return (NULL);

	ctxt = PySchemaValidCtxt_Get(pyobj_ctx);
	if (xmlSchemaGetValidErrors(ctxt, NULL, NULL, (void **) &pyCtxt) == -1)
	{
		py_retval = libxml_intWrap(-1);
		return(py_retval);
	}

	if (pyCtxt == NULL)
	{
		/* first time to set the error handlers */
		pyCtxt = xmlMalloc(sizeof(xmlSchemaValidCtxtPyCtxt));
		if (pyCtxt == NULL) {
			py_retval = libxml_intWrap(-1);
			return(py_retval);
		}
		memset(pyCtxt, 0, sizeof(xmlSchemaValidCtxtPyCtxt));
	}

	/* TODO: check warn and error is a function ! */
	Py_XDECREF(pyCtxt->error);
	Py_XINCREF(pyobj_error);
	pyCtxt->error = pyobj_error;

	Py_XDECREF(pyCtxt->warn);
	Py_XINCREF(pyobj_warn);
	pyCtxt->warn = pyobj_warn;

	Py_XDECREF(pyCtxt->arg);
	Py_XINCREF(pyobj_arg);
	pyCtxt->arg = pyobj_arg;

	xmlSchemaSetValidErrors(ctxt, &libxml_xmlSchemaValidityErrorFunc, &libxml_xmlSchemaValidityWarningFunc, pyCtxt);

	py_retval = libxml_intWrap(1);
	return(py_retval);
}

static PyObject *
libxml_xmlSchemaFreeValidCtxt(ATTRIBUTE_UNUSED PyObject * self, PyObject * args)
{
	xmlSchemaValidCtxtPtr ctxt;
	xmlSchemaValidCtxtPyCtxtPtr pyCtxt;
	PyObject *pyobj_ctxt;

	if (!PyArg_ParseTuple(args, "O:xmlSchemaFreeValidCtxt", &pyobj_ctxt))
		return(NULL);
	ctxt = (xmlSchemaValidCtxtPtr) PySchemaValidCtxt_Get(pyobj_ctxt);

	if (xmlSchemaGetValidErrors(ctxt, NULL, NULL, (void **) &pyCtxt) == 0)
	{
		if (pyCtxt != NULL)
		{
			Py_XDECREF(pyCtxt->error);
			Py_XDECREF(pyCtxt->warn);
			Py_XDECREF(pyCtxt->arg);
			xmlFree(pyCtxt);
		}
	}

	xmlSchemaFreeValidCtxt(ctxt);
	Py_INCREF(Py_None);
	return(Py_None);
}
#endif /* LIBXML_SCHEMAS_ENABLED */

#ifdef LIBXML_C14N_ENABLED
#ifdef LIBXML_OUTPUT_ENABLED

/************************************************************************
 *                                                                      *
 * XML Canonicalization c14n                                            *
 *                                                                      *
 ************************************************************************/

static int
PyxmlNodeSet_Convert(PyObject *py_nodeset, xmlNodeSetPtr *result)
{
    xmlNodeSetPtr nodeSet;
    int is_tuple = 0;

    if (PyTuple_Check(py_nodeset))
        is_tuple = 1;
    else if (PyList_Check(py_nodeset))
        is_tuple = 0;
    else if (py_nodeset == Py_None) {
        *result = NULL;
        return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "must be a tuple or list of nodes.");
        return -1;
    }

    nodeSet = (xmlNodeSetPtr) xmlMalloc(sizeof(xmlNodeSet));
    if (nodeSet == NULL) {
        PyErr_SetString(PyExc_MemoryError, "");
        return -1;
    }

    nodeSet->nodeNr = 0;
    nodeSet->nodeMax = (is_tuple
                        ? PyTuple_GET_SIZE(py_nodeset)
                        : PyList_GET_SIZE(py_nodeset));
    nodeSet->nodeTab
        = (xmlNodePtr *) xmlMalloc (nodeSet->nodeMax
                                    * sizeof(xmlNodePtr));
    if (nodeSet->nodeTab == NULL) {
        xmlFree(nodeSet);
        PyErr_SetString(PyExc_MemoryError, "");
        return -1;
    }
    memset(nodeSet->nodeTab, 0 ,
           nodeSet->nodeMax * sizeof(xmlNodePtr));

    {
        int idx;
        for (idx=0; idx < nodeSet->nodeMax; ++idx) {
            xmlNodePtr pynode =
                PyxmlNode_Get (is_tuple
                               ? PyTuple_GET_ITEM(py_nodeset, idx)
                               : PyList_GET_ITEM(py_nodeset, idx));
            if (pynode)
                nodeSet->nodeTab[nodeSet->nodeNr++] = pynode;
        }
    }
    *result = nodeSet;
    return 0;
}

static int
PystringSet_Convert(PyObject *py_strings, xmlChar *** result)
{
    /* NOTE: the array should be freed, but the strings are shared
       with the python strings and so must not be freed. */

    xmlChar ** strings;
    int is_tuple = 0;
    int count;
    int init_index = 0;

    if (PyTuple_Check(py_strings))
        is_tuple = 1;
    else if (PyList_Check(py_strings))
        is_tuple = 0;
    else if (py_strings == Py_None) {
        *result = NULL;
        return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "must be a tuple or list of strings.");
        return -1;
    }

    count = (is_tuple
             ? PyTuple_GET_SIZE(py_strings)
             : PyList_GET_SIZE(py_strings));

    strings = (xmlChar **) xmlMalloc(sizeof(xmlChar *) * count);

    if (strings == NULL) {
        PyErr_SetString(PyExc_MemoryError, "");
        return -1;
    }

    memset(strings, 0 , sizeof(xmlChar *) * count);

    {
        int idx;
        for (idx=0; idx < count; ++idx) {
            char* s = PyBytes_AsString
                (is_tuple
                 ? PyTuple_GET_ITEM(py_strings, idx)
                 : PyList_GET_ITEM(py_strings, idx));
            if (s)
                strings[init_index++] = (xmlChar *)s;
            else {
                xmlFree(strings);
                PyErr_SetString(PyExc_TypeError,
                                "must be a tuple or list of strings.");
                return -1;
            }
        }
    }

    *result = strings;
    return 0;
}

static PyObject *
libxml_C14NDocDumpMemory(ATTRIBUTE_UNUSED PyObject * self,
                         PyObject * args)
{
    PyObject *py_retval = NULL;

    PyObject *pyobj_doc;
    PyObject *pyobj_nodes;
    int exclusive;
    PyObject *pyobj_prefixes;
    int with_comments;

    xmlDocPtr doc;
    xmlNodeSetPtr nodes;
    xmlChar **prefixes = NULL;
    xmlChar *doc_txt;

    int result;

    if (!PyArg_ParseTuple(args, "OOiOi:C14NDocDumpMemory",
                          &pyobj_doc,
                          &pyobj_nodes,
                          &exclusive,
                          &pyobj_prefixes,
                          &with_comments))
        return (NULL);

    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    if (!doc) {
        PyErr_SetString(PyExc_TypeError, "bad document.");
        return NULL;
    }

    result = PyxmlNodeSet_Convert(pyobj_nodes, &nodes);
    if (result < 0) return NULL;

    if (exclusive) {
        result = PystringSet_Convert(pyobj_prefixes, &prefixes);
        if (result < 0) {
            if (nodes) {
                xmlFree(nodes->nodeTab);
                xmlFree(nodes);
            }
            return NULL;
        }
    }

    result = xmlC14NDocDumpMemory(doc,
                                  nodes,
                                  exclusive,
                                  prefixes,
                                  with_comments,
                                  &doc_txt);

    if (nodes) {
        xmlFree(nodes->nodeTab);
        xmlFree(nodes);
    }
    if (prefixes) {
        xmlChar ** idx = prefixes;
        while (*idx) xmlFree(*(idx++));
        xmlFree(prefixes);
    }

    if (result < 0) {
        PyErr_SetString(PyExc_Exception,
                        "libxml2 xmlC14NDocDumpMemory failure.");
        return NULL;
    }
    else {
        py_retval = PY_IMPORT_STRING_SIZE((const char *) doc_txt,
                                                result);
        xmlFree(doc_txt);
        return py_retval;
    }
}

static PyObject *
libxml_C14NDocSaveTo(ATTRIBUTE_UNUSED PyObject * self,
                     PyObject * args)
{
    PyObject *pyobj_doc;
    PyObject *py_file;
    PyObject *pyobj_nodes;
    int exclusive;
    PyObject *pyobj_prefixes;
    int with_comments;

    xmlDocPtr doc;
    xmlNodeSetPtr nodes;
    xmlChar **prefixes = NULL;
    FILE * output;
    xmlOutputBufferPtr buf;

    int result;
    int len;

    if (!PyArg_ParseTuple(args, "OOiOiO:C14NDocSaveTo",
                          &pyobj_doc,
                          &pyobj_nodes,
                          &exclusive,
                          &pyobj_prefixes,
                          &with_comments,
                          &py_file))
        return (NULL);

    doc = (xmlDocPtr) PyxmlNode_Get(pyobj_doc);
    if (!doc) {
        PyErr_SetString(PyExc_TypeError, "bad document.");
        return NULL;
    }

    output = PyFile_Get(py_file);
    if (output == NULL) {
        PyErr_SetString(PyExc_TypeError, "bad file.");
        return NULL;
    }
    buf = xmlOutputBufferCreateFile(output, NULL);

    result = PyxmlNodeSet_Convert(pyobj_nodes, &nodes);
    if (result < 0) {
        xmlOutputBufferClose(buf);
        return NULL;
    }

    if (exclusive) {
        result = PystringSet_Convert(pyobj_prefixes, &prefixes);
        if (result < 0) {
            if (nodes) {
                xmlFree(nodes->nodeTab);
                xmlFree(nodes);
            }
            xmlOutputBufferClose(buf);
            return NULL;
        }
    }

    result = xmlC14NDocSaveTo(doc,
                              nodes,
                              exclusive,
                              prefixes,
                              with_comments,
                              buf);

    if (nodes) {
        xmlFree(nodes->nodeTab);
        xmlFree(nodes);
    }
    if (prefixes) {
        xmlChar ** idx = prefixes;
        while (*idx) xmlFree(*(idx++));
        xmlFree(prefixes);
    }

    PyFile_Release(output);
    len = xmlOutputBufferClose(buf);

    if (result < 0) {
        PyErr_SetString(PyExc_Exception,
                        "libxml2 xmlC14NDocSaveTo failure.");
        return NULL;
    }
    else
        return PyLong_FromLong((long) len);
}

#endif
#endif

static PyObject *
libxml_getObjDesc(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {

    PyObject *obj;
    char *str;

    if (!PyArg_ParseTuple(args, "O:getObjDesc", &obj))
        return NULL;
    str = PyCapsule_GetPointer(obj, PyCapsule_GetName(obj));
    return Py_BuildValue("s", str);
}

static PyObject *
libxml_compareNodesEqual(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {
    
    PyObject *py_node1, *py_node2;
    xmlNodePtr node1, node2;

    if (!PyArg_ParseTuple(args, "OO:compareNodesEqual",
		&py_node1, &py_node2))
        return NULL;
    /* To compare two node objects, we compare their pointer addresses */
    node1 = PyxmlNode_Get(py_node1);
    node2 = PyxmlNode_Get(py_node2);
    if ( node1 == node2 )
        return Py_BuildValue("i", 1);
    else
        return Py_BuildValue("i", 0);
    
}

static PyObject *
libxml_nodeHash(PyObject *self ATTRIBUTE_UNUSED, PyObject *args) {

    PyObject *py_node1;
    xmlNodePtr node1;

    if (!PyArg_ParseTuple(args, "O:nodeHash", &py_node1))
	    return NULL;
    /* For simplicity, we use the node pointer address as a hash value */
    node1 = PyxmlNode_Get(py_node1);

    return PyLong_FromVoidPtr(node1);

}

/************************************************************************
 *									*
 *		Deprecation warnings					*
 *									*
 ************************************************************************/

int
libxml_deprecationWarning(const char *func) {
#if PY_VERSION_HEX >= 0x03020000
    return PyErr_WarnFormat(PyExc_PendingDeprecationWarning, 1,
            "libxml2mod.%s is deprecated and will be removed "
            "in future versions", func);
#else
    return PyErr_WarnEx(PyExc_PendingDeprecationWarning, func, 1);
#endif
}

/************************************************************************
 *									*
 *			The registration stuff				*
 *									*
 ************************************************************************/
static PyMethodDef libxmlMethods[] = {
#include "libxml2-export.c"
    {"name", libxml_name, METH_VARARGS, NULL},
    {"children", libxml_children, METH_VARARGS, NULL},
    {"properties", libxml_properties, METH_VARARGS, NULL},
    {"last", libxml_last, METH_VARARGS, NULL},
    {"prev", libxml_prev, METH_VARARGS, NULL},
    {"next", libxml_next, METH_VARARGS, NULL},
    {"parent", libxml_parent, METH_VARARGS, NULL},
    {"type", libxml_type, METH_VARARGS, NULL},
    {"doc", libxml_doc, METH_VARARGS, NULL},
    {"xmlNewNode", libxml_xmlNewNode, METH_VARARGS, NULL},
    {"xmlNodeRemoveNsDef", libxml_xmlNodeRemoveNsDef, METH_VARARGS, NULL},
#ifdef LIBXML_VALID_ENABLED
    {"xmlSetValidErrors", libxml_xmlSetValidErrors, METH_VARARGS, NULL},
    {"xmlFreeValidCtxt", libxml_xmlFreeValidCtxt, METH_VARARGS, NULL},
#endif /* LIBXML_VALID_ENABLED */
#ifdef LIBXML_OUTPUT_ENABLED
    {"serializeNode", libxml_serializeNode, METH_VARARGS, NULL},
    {"saveNodeTo", libxml_saveNodeTo, METH_VARARGS, NULL},
    {"outputBufferCreate", libxml_xmlCreateOutputBuffer, METH_VARARGS, NULL},
    {"outputBufferGetPythonFile", libxml_outputBufferGetPythonFile, METH_VARARGS, NULL},
    {"xmlOutputBufferClose", libxml_xmlOutputBufferClose, METH_VARARGS, NULL},
    {"xmlOutputBufferFlush", libxml_xmlOutputBufferFlush, METH_VARARGS, NULL },
    {"xmlSaveFileTo", libxml_xmlSaveFileTo, METH_VARARGS, NULL },
    {"xmlSaveFormatFileTo", libxml_xmlSaveFormatFileTo, METH_VARARGS, NULL },
#endif /* LIBXML_OUTPUT_ENABLED */
    {"inputBufferCreate", libxml_xmlCreateInputBuffer, METH_VARARGS, NULL},
    {"setEntityLoader", libxml_xmlSetEntityLoader, METH_VARARGS, NULL},
    {"xmlRegisterErrorHandler", libxml_xmlRegisterErrorHandler, METH_VARARGS, NULL },
    {"xmlParserCtxtSetErrorHandler", libxml_xmlParserCtxtSetErrorHandler, METH_VARARGS, NULL },
    {"xmlParserCtxtGetErrorHandler", libxml_xmlParserCtxtGetErrorHandler, METH_VARARGS, NULL },
    {"xmlFreeParserCtxt", libxml_xmlFreeParserCtxt, METH_VARARGS, NULL },
#ifdef LIBXML_READER_ENABLED
    {"xmlTextReaderSetErrorHandler", libxml_xmlTextReaderSetErrorHandler, METH_VARARGS, NULL },
    {"xmlTextReaderGetErrorHandler", libxml_xmlTextReaderGetErrorHandler, METH_VARARGS, NULL },
    {"xmlFreeTextReader", libxml_xmlFreeTextReader, METH_VARARGS, NULL },
#endif
#ifdef LIBXML_CATALOG_ENABLED
    {"addLocalCatalog", libxml_addLocalCatalog, METH_VARARGS, NULL },
#endif
#ifdef LIBXML_RELAXNG_ENABLED
    {"xmlRelaxNGSetValidErrors", libxml_xmlRelaxNGSetValidErrors, METH_VARARGS, NULL},
    {"xmlRelaxNGFreeValidCtxt", libxml_xmlRelaxNGFreeValidCtxt, METH_VARARGS, NULL},
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
    {"xmlSchemaSetValidErrors", libxml_xmlSchemaSetValidErrors, METH_VARARGS, NULL},
    {"xmlSchemaFreeValidCtxt", libxml_xmlSchemaFreeValidCtxt, METH_VARARGS, NULL},
#endif
#ifdef LIBXML_C14N_ENABLED
#ifdef LIBXML_OUTPUT_ENABLED
    {"xmlC14NDocDumpMemory", libxml_C14NDocDumpMemory, METH_VARARGS, NULL},
    {"xmlC14NDocSaveTo", libxml_C14NDocSaveTo, METH_VARARGS, NULL},
#endif
#endif
    {"getObjDesc", libxml_getObjDesc, METH_VARARGS, NULL},
    {"compareNodesEqual", libxml_compareNodesEqual, METH_VARARGS, NULL},
    {"nodeHash", libxml_nodeHash, METH_VARARGS, NULL},
    {"xmlRegisterInputCallback", libxml_xmlRegisterInputCallback, METH_VARARGS, NULL},
    {"xmlUnregisterInputCallback", libxml_xmlUnregisterInputCallback, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
#define INITERROR return NULL

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "libxml2mod",
        NULL,
        -1,
        libxmlMethods,
        NULL,
        NULL,
        NULL,
        NULL
};

#else
#define INITERROR return

#ifdef MERGED_MODULES
extern void initlibxsltmod(void);
#endif

#endif

#if PY_MAJOR_VERSION >= 3
PyMODINIT_FUNC PyInit_libxml2mod(void)
#else
void initlibxml2mod(void)
#endif
{
    PyObject *module;

#if PY_MAJOR_VERSION >= 3
    module = PyModule_Create(&moduledef);
#else
    /* initialize the python extension module */
    module = Py_InitModule("libxml2mod", libxmlMethods);
#endif
    if (module == NULL)
        INITERROR;

    /* initialize libxml2 */
    xmlInitParser();
    /* TODO this probably need to be revamped for Python3 */
    libxml_xmlErrorInitialize();

#if PY_MAJOR_VERSION < 3
#ifdef MERGED_MODULES
    initlibxsltmod();
#endif
#endif

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
