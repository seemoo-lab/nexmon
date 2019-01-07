import gdb
import sys

if sys.version_info[0] >= 3:
    long = int

# This is not quite right, as local vars may override symname
def read_global_var (symname):
    return gdb.selected_frame().read_var(symname)

def g_quark_to_string (quark):
    if quark == None:
        return None
    quark = long(quark)
    if quark == 0:
        return None
    try:
        val = read_global_var ("quarks")
        max_q = long(read_global_var ("quark_seq_id"))
    except:
        try:
            val = read_global_var ("g_quarks")
            max_q = long(read_global_var ("g_quark_seq_id"))
        except:
            return None;
    if quark < max_q:
        return val[quark].string()
    return None

# We override the node printers too, so that node->next is not expanded
class GListNodePrinter:
    "Prints a GList node"

    def __init__ (self, val):
        self.val = val

    def to_string (self):
        return "{data=%s, next=0x%x, prev=0x%x}" % (str(self.val["data"]), long(self.val["next"]), long(self.val["prev"]))

class GSListNodePrinter:
    "Prints a GSList node"

    def __init__ (self, val):
        self.val = val

    def to_string (self):
        return "{data=%s, next=0x%x}" % (str(self.val["data"]), long(self.val["next"]))

class GListPrinter:
    "Prints a GList"

    class _iterator:
        def __init__(self, head, listtype):
            self.link = head
            self.listtype = listtype
            self.count = 0

        def __iter__(self):
            return self

        def next(self):
            if self.link == 0:
                raise StopIteration
            data = self.link['data']
            self.link = self.link['next']
            count = self.count
            self.count = self.count + 1
            return ('[%d]' % count, data)

        __next__ = next

    def __init__ (self, val, listtype):
        self.val = val
        self.listtype = listtype

    def children(self):
        return self._iterator(self.val, self.listtype)

    def to_string (self):
        return  "0x%x" % (long(self.val))

    def display_hint (self):
        return "array"

class GHashPrinter:
    "Prints a GHashTable"

    class _iterator:
        def __init__(self, ht, keys_are_strings):
            self.ht = ht
            if ht != 0:
                self.keys = ht["keys"]
                self.values = ht["values"]
                self.hashes = ht["hashes"]
                self.size = ht["size"]
            self.pos = 0
            self.keys_are_strings = keys_are_strings
            self.value = None

        def __iter__(self):
            return self

        def next(self):
            if self.ht == 0:
                raise StopIteration
            if self.value != None:
                v = self.value
                self.value = None
                return v
            while long(self.pos) < long(self.size):
                self.pos = self.pos + 1
                if long (self.hashes[self.pos]) >= 2:
                    key = self.keys[self.pos]
                    val = self.values[self.pos]

                    if self.keys_are_strings:
                        key = key.cast (gdb.lookup_type("char").pointer())

                    # Queue value for next result
                    self.value = ('[%dv]'% (self.pos), val)

                    # Return key
                    return ('[%dk]'% (self.pos), key)
            raise StopIteration

        __next__ = next

    def __init__ (self, val):
        self.val = val
        self.keys_are_strings = False
        try:
            string_hash = read_global_var ("g_str_hash")
        except:
            string_hash = None
        if self.val != 0 and string_hash != None and self.val["hash_func"] == string_hash:
            self.keys_are_strings = True

    def children(self):
        return self._iterator(self.val, self.keys_are_strings)

    def to_string (self):
        return  "0x%x" % (long(self.val))

    def display_hint (self):
        return "map"

def pretty_printer_lookup (val):
    # None yet, want things like hash table and list

    type = val.type.unqualified()

    # If it points to a reference, get the reference.
    if type.code == gdb.TYPE_CODE_REF:
        type = type.target ()

    if type.code == gdb.TYPE_CODE_PTR:
        type = type.target().unqualified()
        t = str(type)
        if t == "GList":
            return GListPrinter(val, "GList")
        if t == "GSList":
            return GListPrinter(val, "GSList")
        if t == "GHashTable":
            return GHashPrinter(val)
    else:
        t = str(type)
        if t == "GList":
            return GListNodePrinter(val)
        if t == "GSList *":
            return GListPrinter(val, "GSList")
    return None

def register (obj):
    if obj == None:
        obj = gdb

    obj.pretty_printers.append(pretty_printer_lookup)

class ForeachCommand (gdb.Command):
    """Foreach on list"""

    def __init__ (self):
        super (ForeachCommand, self).__init__ ("gforeach",
                                               gdb.COMMAND_DATA,
                                               gdb.COMPLETE_SYMBOL)

    def valid_name (self, name):
        if not name[0].isalpha():
            return False
        return True

    def parse_args (self, arg):
        i = arg.find(" ")
        if i <= 0:
            raise Exception ("No var specified")
        var = arg[:i]
        if not self.valid_name(var):
            raise Exception ("Invalid variable name")

        while i < len (arg) and arg[i].isspace():
            i = i + 1

        if arg[i:i+2] != "in":
            raise Exception ("Invalid syntax, missing in")

        i = i + 2

        while i < len (arg) and arg[i].isspace():
            i = i + 1

        colon = arg.find (":", i)
        if colon == -1:
            raise Exception ("Invalid syntax, missing colon")

        val = arg[i:colon]

        colon = colon + 1
        while colon < len (arg) and arg[colon].isspace():
            colon = colon + 1

        command = arg[colon:]

        return (var, val, command)

    def do_iter(self, arg, item, command):
        item = item.cast (gdb.lookup_type("void").pointer())
        item = long(item)
        to_eval = "set $%s = (void *)0x%x\n"%(arg, item)
        gdb.execute(to_eval)
        gdb.execute(command)

    def slist_iterator (self, arg, container, command):
        l = container.cast (gdb.lookup_type("GSList").pointer())
        while long(l) != 0:
            self.do_iter (arg, l["data"], command)
            l = l["next"]

    def list_iterator (self, arg, container, command):
        l = container.cast (gdb.lookup_type("GList").pointer())
        while long(l) != 0:
            self.do_iter (arg, l["data"], command)
            l = l["next"]

    def pick_iterator (self, container):
        t = container.type.unqualified()
        if t.code == gdb.TYPE_CODE_PTR:
            t = t.target().unqualified()
            t = str(t)
            if t == "GSList":
                return self.slist_iterator
            if t == "GList":
                return self.list_iterator
        raise Exception("Invalid container type %s"%(str(container.type)))

    def invoke (self, arg, from_tty):
        (var, container, command) = self.parse_args(arg)
        container = gdb.parse_and_eval (container)
        func = self.pick_iterator(container)
        func(var, container, command)

ForeachCommand ()
