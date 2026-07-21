import dataclasses as dc
import change_case

@dc.dataclass
class PropertyDescriptor:
    type: str
    alias: list[str]
    values: list[list[str]] = dc.field(default_factory=list)

    _longName: str | None = None
    _indexOf: dict[str, int] = dc.field(default_factory=dict)

    def longname(self):
        if not self._longName:
            self._longName = change_case.to_pascal_case(self.alias[1])
        return self._longName

    def shortname(self):
        alias = self.alias
        return alias[0] if alias[0] else alias[1]

    def indexOf(self, value) -> int:
        res: int | None = self._indexOf.get(value)
        if res is not None:
            return res

        index = 0
        for v in self.values:
            if value in v:
                return index
            index += 1
        self._indexOf[value] = index
        return index


@dc.dataclass
class Database:
    ucdVersion: str = ""
    descriptors: list[PropertyDescriptor] = dc.field(default_factory=list)
    alias: dict[str, PropertyDescriptor] = dc.field(default_factory=dict)
    codepoints: list[dict[str, str]] = dc.field(default_factory=list)

    @staticmethod
    def load():
        database = Database()

        with open("src/karm-icu/res/ppucd.txt", "r") as propertiesFile:
            for line in propertiesFile:
                line = line.strip()
                try:
                    if line.startswith("#") or line == "":
                        continue
                    attr = line.split(";")

                    if attr[0] == "ucd":
                        database.ucdVersion = attr[1]
                    elif attr[0] == "property":
                        p = PropertyDescriptor(attr[1], attr[2:])
                        for a in attr[2:]:
                            database.alias[a] = p
                        database.descriptors.append(p)
                    elif attr[0] == "binary":
                        for p in database.descriptors:
                            if p.type == "Binary":
                                p.values.append(attr[1:])
                    elif attr[0] == "value":
                        database.alias[attr[1]].values.append(attr[2:])
                    elif attr[0] in ["cp", "block", "defaults"]:
                        start, end = [int(attr[1], 16), int(attr[1], 16)] if ".." not in attr[1] else (
                            [int(i, 16) for i in attr[1].split("..")])
                        props = dict([(p.split("=") if len(p.split("=")) == 2 else [p, "True"]) for p in attr[2:]])
                        for cp in range(start, end + 1):
                            if cp >= len(database.codepoints):
                                database.codepoints.insert(cp, {})
                            for k, v in props.items():
                                database.codepoints[cp][k] = v
                    elif attr[0] == "unassigned":
                        pass
                    elif attr[0] == "algnamesrange":
                        pass
                    else:
                        print("unknow line type: " + attr[0])
                except Exception as e:
                    print("Failled to process: " + line)
                    raise e

        return database


PAGE_SIZE = 256


def paginateProperty(database: Database, descriptor: PropertyDescriptor, toCpp, default):
    pages = []
    indirect = []
    for pi in range(0, 0x10FFFF, PAGE_SIZE):
        page = [0] * PAGE_SIZE
        for cp in range(pi, pi + PAGE_SIZE):
            page[cp % PAGE_SIZE] = toCpp(database.codepoints[cp].get(descriptor.shortname(), default))
        index = 0
        if page in pages:
            index = pages.index(page)
        else:
            index = len(pages)
            pages.append(page)
        indirect.append(index)
    return sum(pages, []), indirect


@dc.dataclass
class Property:
    descriptor: PropertyDescriptor

    def emitTypeDeclaration(self, out):
        pass

    def emitTable(self, database: Database, out):
        pass

    def emitAccessor(self, out):
        pass


class EnumProperty(Property):
    def emitTypeDeclaration(self, out):
        print(f"export enum struct {self.descriptor.longname()} {{", file=out)
        for v in self.descriptor.values:
            print(f"    {v[1].upper()},", file=out)
        print("", file=out)
        print("    _LEN,", file=out)
        print("};\n", file=out)

    def emitTable(self, database: Database, out):
        pages, indirect = paginateProperty(database, self.descriptor, lambda v: self.descriptor.indexOf(v), None)
        type = "u8"
        if len(self.descriptor.values) > 0xff:
            type = "u16"
        print(
            f"static constexpr {type} _{self.descriptor.longname()}Pages[] = {{{", ".join(str(x) for x in pages)}}};\n",
            file=out)
        print(
            f"static constexpr u8 _{self.descriptor.longname()}Indirect[] = {{{", ".join(str(x) for x in indirect)}}};\n",
            file=out)

    def emitAccessor(self, out):
        name = change_case.to_camel_case(self.descriptor.longname())

        lookupIndirect = f"(_{self.descriptor.longname()}Indirect[_rune >> 8] << 8) + (_rune & 255)"
        lookupPage = f"_{self.descriptor.longname()}Pages[{lookupIndirect}]"

        print(f"    {self.descriptor.longname()} {name}() const {{", file=out)
        print(
            f"        return static_cast<{self.descriptor.longname()}>({lookupPage});",
            file=out)
        print("    }\n", file=out)


class RuneProperty(Property):
    def emitTable(self, database: Database, out):
        pages, indirect = paginateProperty(database, self.descriptor, lambda v: int(v, 16), "0")
        print(
            f"static constexpr Rune _{self.descriptor.longname()}Pages[] = {{{", ".join(str(x) for x in pages)}}};\n",
            file=out)
        print(
            f"static constexpr u8 _{self.descriptor.longname()}Indirect[] = {{{", ".join(str(x) for x in indirect)}}};\n",
            file=out)

    def emitAccessor(self, out):
        name = change_case.to_camel_case(self.descriptor.longname())

        lookupIndirect = f"(_{self.descriptor.longname()}Indirect[_rune >> 8] << 8) + (_rune & 255)"
        lookupPage = f"_{self.descriptor.longname()}Pages[{lookupIndirect}]"

        print(f"    Rune {name}() const {{", file=out)
        print(
            f"        return static_cast<Rune>({lookupPage});",
            file=out)
        print("    }\n", file=out)


class BoolProperty(Property):
    def emitTable(self, database: Database, out):
        pages, indirect = paginateProperty(database, self.descriptor, lambda v: self.descriptor.indexOf(v), "False")

        packed_pages = []
        for i in range(0, len(pages), 16):
            chunk = pages[i:i + 16]
            val = 0
            for bit_idx, bit_val in enumerate(chunk):
                if int(bit_val):
                    val |= (1 << bit_idx)
            packed_pages.append(val)

        longname = self.descriptor.longname()

        print(
            f"static constexpr u16 _{longname}Pages[] = {{{', '.join(hex(x) for x in packed_pages)}}};\n",
            file=out)
        print(
            f"static constexpr u8 _{longname}Indirect[] = {{{', '.join(str(x) for x in indirect)}}};\n",
            file=out)

    def emitAccessor(self, out):
        longname = self.descriptor.longname()
        name = change_case.to_camel_case(self.descriptor.longname())

        lookupWordIndex = f"(_{longname}Indirect[_rune >> 8] << 4) + ((_rune & 255) >> 4)"
        lookupWord = f"_{longname}Pages[{lookupWordIndex}]"

        print(f"    bool {name}() const {{", file=out)
        print(
            f"        return ({lookupWord} >> (_rune & 15)) & 1;",
            file=out)
        print("    }\n", file=out)


class UnknowProperty(Property):
    def emitAccessor(self, out):
        print(f"    // TODO: Ignored property {descriptor.longname()}\n", file=out)


def propertyFor(descriptor: PropertyDescriptor) -> Property:
    match (descriptor.type, descriptor.longname()):
        case ("Catalog" | "Enumerated", _):
            return EnumProperty(descriptor)
        case ("Binary", _):
            return BoolProperty(descriptor)
        case (_, "BidiMirroringGlyph"):
            return RuneProperty(descriptor)
        case _:
            return UnknowProperty(descriptor)


database = Database.load()

with open("src/karm-icu/ucd.cpp", "w") as out:
    print("export module Karm.Icu:ucd;\n", file=out)
    print("import Karm.Core;\n", file=out)
    print("// This file is generated by meta/scripts/ucd_compiler.py", file=out)
    print("namespace Karm::Icu {", file=out)

    print("", file=out)
    print("// Unicode Character Database", file=out)
    print("// https://unicode.org/reports/tr44/", file=out)
    print("", file=out)

    print(f"export constexpr Str UCD_VERSION = \"{database.ucdVersion}\";\n", file=out)

    for descriptor in database.descriptors:
        print("Compiling " + descriptor.longname())
        property = propertyFor(descriptor)
        property.emitTypeDeclaration(out)
        property.emitTable(database, out)

    print("export struct Properties {", file=out)
    print("    Rune _rune;\n", file=out)
    print("    static Properties of(Rune r) { return Properties(r); }\n", file=out)

    for descriptor in database.descriptors:
        property = propertyFor(descriptor)
        property.emitAccessor(out)

    print("};\n", file=out)

    print("} // namespace Karm::Icu", file=out)
