import gdb
import gdb.printing


class OptPrinter:
    def __init__(self, val):
        self.val = val
        self.store = self.val["_store"]

    def to_string(self):
        if bool(self.store["_present"]):
            return "Some(%s)" % self.store["_inner"]["_value"]
        else:
            return "None"

    def children(self):
        if bool(self.store["_present"]):
            yield ("0", self.store["_inner"]["_value"])

    def display_hint(self):
        return "array"


class Vec2Printer:
    def __init__(self, val):
        self.val = val

    def children(self):
        for i in range(2):
            yield (str(i), self.val["_els"]["_buf"][i])

    def display_hint(self):
        return "array"

class VecPrinter:
    def __init__(self, val):
        self.val = val
        buf = val["_buf"]
        self.len = int(buf["_len"])

        manual_ptr = buf["_buf"]
        if manual_ptr:
            elem_type = manual_ptr.type.target().template_argument(0)
            self.data = manual_ptr.cast(elem_type.pointer())
        else:
            self.data = None

    def children(self):
        if self.data is None:
            return
        for i in range(self.len):
            yield (f"[{i}]", (self.data + i).dereference())

    def display_hint(self):
        return "array"


class BufPrinter:
    def __init__(self, val):
        self.val = val
        self.len = int(val["_len"])
        manual_ptr = val["_buf"]
        if manual_ptr:
            elem_type = manual_ptr.type.target().template_argument(0)
            self.data = manual_ptr.cast(elem_type.pointer())
        else:
            self.data = None

    def children(self):
        if self.data is None:
            return
        for i in range(self.len):
            yield (f"[{i}]", (self.data + i).dereference())

    def display_hint(self):
        return "array"

class AuPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        val = int(self.val["_val"])
        return "%.2f" % (val / 60)

def build_karm_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("karm")
    pp.add_printer("Opt", "^(Karm::)?Opt<.*>$", OptPrinter)
    pp.add_printer("Vec2", "^(Karm::)?(Math::)?Vec2<.*>$", Vec2Printer)
    pp.add_printer("Au", "^(Karm::)?(Math::)?Au$", AuPrinter)
    pp.add_printer("Vec", "^(Karm::)?_Vec<.*>$", VecPrinter)
    pp.add_printer("Buf", "^(Karm::)?Buf<.*>$", BufPrinter)
    return pp

gdb.printing.register_pretty_printer(None, build_karm_pretty_printer(), replace=True)
