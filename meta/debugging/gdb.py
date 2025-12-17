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


class AuPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        val = int(self.val["_val"])
        return "%.2f" % (val / (1 << 8))


def build_opt_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("karm_opt")
    pp.add_printer("Opt", "^(Karm::)?Opt<.*>$", OptPrinter)
    return pp


def build_vec2_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("karm_vec2")
    pp.add_printer("Opt", "^(Karm::)?Vec2<.*>$", Vec2Printer)
    return pp


def build_au_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("karm_au")
    pp.add_printer("Opt", "^(Karm::Math::)?Fixed<int, 8ul>$", AuPrinter)
    return pp


gdb.printing.register_pretty_printer(None, build_opt_pretty_printer(), replace=True)
gdb.printing.register_pretty_printer(None, build_vec2_pretty_printer(), replace=True)
gdb.printing.register_pretty_printer(None, build_au_pretty_printer(), replace=True)
