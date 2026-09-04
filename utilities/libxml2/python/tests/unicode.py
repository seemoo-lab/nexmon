from io import BytesIO, StringIO
import libxml2
import sys
from xml.sax.handler import ContentHandler
from xml.sax.xmlreader import InputSource
import xml.sax


# Test data: an XML file with a 100,000 Unicode smileys, which expand
# into 400,000 bytes after UTF-8 encoding.
SMILEY = '\U0001f600'
TEXT = 100_000 * SMILEY
XML_STRING = '<?xml version="1.0" encoding="UTF-8" ?>\n<root>' + TEXT + '</root>'
XML_BYTES = XML_STRING.encode('utf-8')


def RunTest(test_name, source):
    expected = TEXT
    received = ''

    class TestHandler(ContentHandler):
        def characters(self, content):
            nonlocal received
            received += content

    reader = xml.sax.make_parser(['drv_libxml2'])
    reader.setContentHandler(TestHandler())
    reader.parse(source)
    if expected != received:
        print(test_name, 'failed!')
        print('Expected length:', len(expected))
        print('Received length:', len(received))
        print('Expected text: (prefix only)', expected[:100])
        print('Received text: (prefix only)', received[:100])
        sys.exit(1)


def TestBytesInput():
    source = InputSource()
    source.setByteStream(BytesIO(XML_BYTES))
    RunTest('TestBytesInput', source)


def TestStringInput():
    source = InputSource()
    source.setCharacterStream(StringIO(XML_STRING))
    RunTest('TestStringInput', source)


# Memory debug specific
libxml2.debugMemory(1)

TestBytesInput()
TestStringInput()

# Memory debug specific
libxml2.cleanupParser()
if libxml2.debugMemory(1) == 0:
    print("OK")
else:
    print("Memory leak %d bytes" % (libxml2.debugMemory(1)))
