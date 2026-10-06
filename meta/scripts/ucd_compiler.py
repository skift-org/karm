import dataclasses as dc
import change_case

@dc.dataclass
class PropertyType:
    type : str
    name: str
    values: list[list[str]] = dc.field(default_factory=list)
    _ordinalOf: dict[str, int] = dc.field(default_factory=dict)

    def addValue(self, value):
        if value not in self.values:
            self.values.append(value)

    def ordinalOf(self, value) -> int:
        for index, v in enumerate(self.values):
            if value in v:
                return int(v[0]) if v[0].isdigit() else index
        raise ValueError(f"{self.name}: unknown value {value!r}")

@dc.dataclass
class PropertyDescriptor:
    key : str
    name: str
    type : PropertyType


@dc.dataclass
class Database:
    ucdVersion: str = ""
    descriptors: list[PropertyDescriptor] = dc.field(default_factory=list)
    typeDict: dict[str, PropertyType] = dc.field(default_factory=dict)
    alias: dict[str, PropertyDescriptor] = dc.field(default_factory=dict)
    codepoints: list[dict[str, str]] = dc.field(default_factory=list)

    def typeFor(self, type : str, name : str):
        if name.endswith("QuickCheck"):
            name = "QuickCheck"
        elif name.endswith("CanonicalCombiningClass"):
            name = "CanonicalCombiningClass"

        if name in self.typeDict:
            return self.typeDict[name]
        ty = PropertyType(type, name)
        self.typeDict[name] = ty
        return ty

    @staticmethod
    def load():
        database = Database()

        with open("src/karm-icu/res/ppucd.txt", "r") as propertiesFile:
            defaults: dict[str, str] = {}
            currentBlock: dict[str, str] = {}

            for line in propertiesFile:
                line = line.strip()
                try:
                    if line.startswith("#") or line == "":
                        continue
                    attr = line.split(";")

                    if attr[0] == "ucd":
                        database.ucdVersion = attr[1]
                    elif attr[0] == "property":
                        alias = attr[2:]
                        key = alias[0] if alias[0] else alias[1]
                        name = change_case.to_pascal_case(alias[1])
                        p = PropertyDescriptor(key, name, database.typeFor(attr[1], name))
                        for a in attr[2:]:
                            database.alias[a] = p
                        database.descriptors.append(p)
                    elif attr[0] == "binary":
                        for p in database.descriptors:
                            if p.type == "Binary":
                                p.type.addValue(attr[1:])
                    elif attr[0] == "value":
                        database.alias[attr[1]].type.addValue(attr[2:])
                    elif attr[0] in ["defaults", "block", "cp", "unassigned"]:
                        start, end = [int(attr[1], 16)] * 2 if ".." not in attr[1] else [int(i, 16) for i in attr[1].split("..")]

                        props = {}
                        for p in attr[2:]:
                            if "=" in p:
                                k, v = p.split("=", 1)
                            elif p.startswith("-"):
                                k, v = p[1:], "False"
                            else:
                                k, v = p, "True"
                            props[k] = v

                        if attr[0] == "defaults":
                            defaults = props
                        elif attr[0] == "block":
                            currentBlock = props

                        for cp in range(start, end + 1):
                            while cp >= len(database.codepoints):
                                database.codepoints.append({})
                            if attr[0] == "unassigned":
                                database.codepoints[cp] = {**defaults, "blk": currentBlock.get("blk", "NB"), **props}
                            else:
                                database.codepoints[cp].update(props)
                    elif attr[0] == "unassigned":
                        pass
                    elif attr[0] == "algnamesrange":
                        pass
                    else:
                        print("unknow line type: " + attr[0])
                except Exception as e:
                    print("Failed to process: " + line)
                    raise e

        return database

PAGE_SIZE = 256
def paginateProperty(database: Database, descriptor: PropertyDescriptor, toCpp, default):
    pages = []
    indirect = []
    for pi in range(0, 0x10FFFF, PAGE_SIZE):
        page = [0] * PAGE_SIZE
        for cp in range(pi, pi + PAGE_SIZE):
            page[cp % PAGE_SIZE] = toCpp(database.codepoints[cp].get(descriptor.key, default))
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

    def emitTable(self, database: Database, out):
        pass

    def emitAccessor(self, out):
        pass

ENUM_DEFAULTS = {"CanonicalCombiningClass": "0"}

class EnumProperty(Property):
    def emitTable(self, database: Database, out):
        ty =  self.descriptor.type
        default = ENUM_DEFAULTS.get(ty.name)
        pages, indirect = paginateProperty(database, self.descriptor, lambda v: ty.ordinalOf(v), default)
        type = "u8"
        if len(ty.values) > 0xff:
            type = "u16"
        print(
            f"static constexpr {type} _{self.descriptor.name}Pages[] = {{{", ".join(str(x) for x in pages)}}};\n",
            file=out)
        print(
            f"static constexpr u8 _{self.descriptor.name}Indirect[] = {{{", ".join(str(x) for x in indirect)}}};\n",
            file=out)

    def emitAccessor(self, out):
        name = change_case.to_camel_case(self.descriptor.name)

        lookupIndirect = f"(_{self.descriptor.name}Indirect[_rune >> 8] << 8) + (_rune & 255)"
        lookupPage = f"_{self.descriptor.name}Pages[{lookupIndirect}]"

        print(f"    {self.descriptor.type.name} {name}() const {{", file=out)
        print(
            f"        return static_cast<{self.descriptor.type.name}>({lookupPage});",
            file=out)
        print("    }\n", file=out)


class RuneProperty(Property):
    def emitTable(self, database: Database, out):
        pages, indirect = paginateProperty(database, self.descriptor, lambda v: int(v, 16), "0")
        print(
            f"static constexpr Rune _{self.descriptor.name}Pages[] = {{{", ".join(str(x) for x in pages)}}};\n",
            file=out)
        print(
            f"static constexpr u8 _{self.descriptor.name}Indirect[] = {{{", ".join(str(x) for x in indirect)}}};\n",
            file=out)

    def emitAccessor(self, out):
        name = change_case.to_camel_case(self.descriptor.name)

        lookupIndirect = f"(_{self.descriptor.name}Indirect[_rune >> 8] << 8) + (_rune & 255)"
        lookupPage = f"_{self.descriptor.name}Pages[{lookupIndirect}]"

        print(f"    Rune {name}() const {{", file=out)
        print(
            f"        return static_cast<Rune>({lookupPage});",
            file=out)
        print("    }\n", file=out)


class BoolProperty(Property):
    def emitTable(self, database: Database, out):
        pages, indirect = paginateProperty(database, self.descriptor, lambda v: 1 if v == "True" else 0, "False")

        packed_pages = []
        for i in range(0, len(pages), 16):
            chunk = pages[i:i + 16]
            val = 0
            for bit_idx, bit_val in enumerate(chunk):
                if int(bit_val):
                    val |= (1 << bit_idx)
            packed_pages.append(val)

        longname = self.descriptor.name

        print(
            f"static constexpr u16 _{longname}Pages[] = {{{', '.join(hex(x) for x in packed_pages)}}};\n",
            file=out)
        print(
            f"static constexpr u8 _{longname}Indirect[] = {{{', '.join(str(x) for x in indirect)}}};\n",
            file=out)

    def emitAccessor(self, out):
        longname = self.descriptor.name
        name = change_case.to_camel_case(self.descriptor.name)

        lookupWordIndex = f"(_{longname}Indirect[_rune >> 8] << 4) + ((_rune & 255) >> 4)"
        lookupWord = f"_{longname}Pages[{lookupWordIndex}]"

        print(f"    bool {name}() const {{", file=out)
        print(
            f"        return ({lookupWord} >> (_rune & 15)) & 1;",
            file=out)
        print("    }\n", file=out)


class UnknowProperty(Property):
    def emitAccessor(self, out):
        print(f"    // TODO: Ignored property {self.descriptor.name}\n", file=out)


def propertyFor(descriptor: PropertyDescriptor) -> Property:
    match (descriptor.type.type, descriptor.name):
        case ("Catalog" | "Enumerated", _):
            return EnumProperty(descriptor)
        case ("Binary", _):
            return BoolProperty(descriptor)
        case (_, "BidiMirroringGlyph"):
            return RuneProperty(descriptor)
        case _:
            return UnknowProperty(descriptor)

def emitTypeDeclaration(ty : PropertyType, out):
    if ty.type in ("Catalog", "Enumerated"):
        print(f"export enum struct {ty.name} {{", file=out)
        for v in ty.values:
            if v[0].isdigit():
                print(f"    {v[2].upper()} = {v[0]},", file=out)
            else:
                print(f"    {v[1].upper()},", file=out)
        print("", file=out)
        print("    _LEN,", file=out)
        print("};\n", file=out)

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

    for ty in database.typeDict.values():
        emitTypeDeclaration(ty, out)

    for descriptor in database.descriptors:
        print("Compiling " + descriptor.name)
        property = propertyFor(descriptor)
        property.emitTable(database, out)

    print("export struct Properties {", file=out)
    print("    Rune _rune;\n", file=out)
    print("    static Properties of(Rune r) { return Properties(r); }\n", file=out)

    for descriptor in database.descriptors:
        property = propertyFor(descriptor)
        property.emitAccessor(out)

    print("    bool bmp() const { return _rune >= 0x0000 and _rune <= 0xffff; }\n", file=out)
    print("    bool supplementary() const { return _rune >= 0x10000 and _rune <= 0x10ffff; }\n", file=out)

    print("};\n", file=out)

    print("} // namespace Karm::Icu", file=out)
