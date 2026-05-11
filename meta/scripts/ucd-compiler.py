import dataclasses as dc

CPP_TYPES = {
    "Binary": "bool",
    "String": "Str",
}


def cppEscape(value: str) -> str:
    return f"\"{value}\""


class PropertyType:
    pass


@dc.dataclass
class Property:
    type: str
    alias: list[str]
    values: list[list[str]] = dc.field(default_factory=list)

    _longName: str | None = None

    def longname(self):
        if not self._longName:
            result = self.alias[1].replace("_", "")
            self._longName = result[0].upper() + result[1:]
        return self._longName

    def shortname(self):
        alias = self.alias
        return alias[0] if alias[0] else alias[1]

    def cppType(self):
        return self.longname() if property.type in ["Catalog", "Enumerated"] else CPP_TYPES[
            self.type] if self.type in CPP_TYPES else self.type

    def cppValue(self, value):
        if type == "string":
            return cppEscape(value)
        else:
            return self.indexOf(value)

    def indexOf(self, value):
        index = 0
        for v in self.values:
            if value in v:
                return index
            index += 1
        return index


@dc.dataclass
class Database:
    ucdVersion: str = ""
    properties: list[Property] = dc.field(default_factory=list)
    alias: dict[str, Property] = dc.field(default_factory=dict)
    codepoints: list[dict[str, str]] = dc.field(default_factory=list)

    @staticmethod
    def load():
        database = Database()

        with open("src/karm-core/icu/res/ppucd.txt", "r") as propertiesFile:
            for line in propertiesFile:
                line = line.strip()
                try:
                    if line.startswith("#") or line == "":
                        continue
                    attr = line.split(";")

                    if attr[0] == "ucd":
                        database.ucdVersion = attr[1]
                    elif attr[0] == "property":
                        p = Property(attr[1], attr[2:])
                        for a in attr[2:]:
                            database.alias[a] = p
                        database.properties.append(p)
                    elif attr[0] == "binary":
                        for p in database.properties:
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


def paginateProperty(database: Database, property: Property):
    print("Paginating property " + property.longname())
    pages = []
    indirect = []
    for pi in range(0, 0x10FFFF, PAGE_SIZE):
        page = [""] * PAGE_SIZE
        for cp in range(pi, pi + PAGE_SIZE):
            try:
                value = database.codepoints[cp].get(property.shortname(), "False")
                page[cp % PAGE_SIZE] = value
            except Exception as e:
                print(f"fail {cp:x}")
                raise e
        index = 0
        if page in pages:
            index = pages.index(page)
        else:
            index = len(pages)
            pages.append(page)
        indirect.append(index)
    print(f"pages: {len(pages)}")
    return sum(pages, []), indirect


database = Database.load()

with open("src/karm-icu/ucd.cpp", "w") as propertiesFile:
    print("export module Karm.Icu:ucd;\n", file=propertiesFile)
    print("import Karm.Core;\n", file=propertiesFile)
    print("namespace Karm::Icu {", file=propertiesFile)

    print("", file=propertiesFile)
    print("// Unicode Character Database", file=propertiesFile)
    print("// https://unicode.org/reports/tr44/", file=propertiesFile)
    print("", file=propertiesFile)

    print(f"export constexpr Str UCD_VERSION = \"{database.ucdVersion}\";\n", file=propertiesFile)

    for property in database.properties:
        if property.type in ["Catalog", "Enumerated"]:
            print(f"export enum struct {property.longname()} {{", file=propertiesFile)
            for v in property.values:
                print(f"    {v[1].upper()},", file=propertiesFile)
            print("\n", file=propertiesFile)
            print("    _LEN,", file=propertiesFile)
            print("};\n", file=propertiesFile)

            pages, indirect = paginateProperty(database, property)

            type = "u8"
            if len(property.values) > 0xff:
                type = "u16"
            print(
                f"static constexpr {type} _{property.longname()}Pages[] = {{{", ".join(str(property.indexOf(x)) for x in pages)}}};\n",
                file=propertiesFile)
            print(
                f"static constexpr u8 _{property.longname()}Indirect[] = {{{", ".join(str(x) for x in indirect)}}};\n",
                file=propertiesFile)
        else:
            pass

    print("export struct Properties {", file=propertiesFile)
    print("    Rune _rune;\n", file=propertiesFile)
    print("    static Properties of(Rune r) { return Properties(r); }\n", file=propertiesFile)
    for property in database.properties:
        if property.type in ["Catalog", "Enumerated"]:
            print(f"    {property.cppType()} {property.longname()[0].lower() + property.longname()[1:]}() {{",
                  file=propertiesFile)
            print(
                f"        return static_cast<{property.cppType()}>(_{property.longname()}Pages[(_{property.longname()}Indirect[_rune >> 8] << 8) + (_rune & 255)]);",
                file=propertiesFile)
            print("    }\n", file=propertiesFile)
        else:
            print(f"    // TODO: Ignored property {property.longname()}\n", file=propertiesFile)

    print("};\n", file=propertiesFile)

    print("} // namespace Karm::Icu", file=propertiesFile)
