module;

#include <karm/macros>

export module Karm.Dl.Elf:parser;

import Karm.Core;
import :base;
import :abi;

namespace Karm::Elf {

export using Karm::begin, Karm::end;

export template <typename Abi>
struct ElfObject;

export template <typename Abi>
struct ElfProgram : ElfPhdr<Abi> {
    Bytes data;

    ElfProgram(ElfPhdr<Abi> header, Bytes data)
        : ElfPhdr<Abi>(header), data(data) {}

    ElfPhdrType type() const {
        return static_cast<ElfPhdrType>(this->p_type.value());
    }

    urange vrange() const {
        return {this->p_vaddr, this->p_memsz};
    }
};

export template <typename Abi>
struct ElfSection : ElfShdr<Abi> {
    Bytes data;

    ElfSection(ElfShdr<Abi> header, Bytes data)
        : ElfShdr<Abi>(header), data(data) {}

    ElfShdrType type() const {
        return static_cast<ElfShdrType>(this->sh_type.value());
    }
};

export template <typename Abi>
struct ElfSymTabSection : ElfSection<Abi> {
    static constexpr auto TYPE = ElfShdrType::SHT_SYMTAB;

    ElfSymTabSection(ElfSection<Abi> sect)
        : ElfSection<Abi>(sect) {}

    Res<Slice<ElfSym<Abi>>> iterSym() const {
        if (this->sh_entsize != sizeof(ElfSym<Abi>))
            return Error::invalidData("unsupported symbol size");
        return Io::BScan{this->data}.template peekSlice<ElfSym<Abi>>(this->data.len() / sizeof(ElfSym<Abi>));
    }
};

export template <typename Abi>
struct ElfStrTab : ElfSection<Abi> {
    static constexpr auto TYPE = ElfShdrType::SHT_STRTAB;

    Res<Str> string(usize off) const {
        if (off == 0)
            return Ok(Str{""});
        return try$(Io::BScan{this->data}
                        .peek(off))
            .nextCStr();
    }
};

export template <typename Abi>
struct ElfDynSymSection : ElfSection<Abi> {
    static constexpr auto TYPE = ElfShdrType::SHT_DYNSYM;

    ElfDynSymSection(ElfSection<Abi> sect)
        : ElfSection<Abi>(sect) {}
};

export template <typename Abi>
struct ElfDynamicSection : ElfSection<Abi> {
    static constexpr auto TYPE = ElfShdrType::SHT_DYNAMIC;

    Res<Slice<ElfDyn<Abi>>> iterDyn() const {
        if (this->sh_entsize != sizeof(ElfDyn<Abi>))
            return Error::invalidData("unsupported dynamic entry size");
        return Io::BScan{this->data}.template peekSlice<ElfDyn<Abi>>(this->data.len() / sizeof(ElfDyn<Abi>));
    }

    Res<Opt<ElfDyn<Abi>>> dyn(Elf::ElfDynTag tag) {
        for (ElfDyn dyn : try$(iterDyn()))
            if (dyn.tag() == tag)
                return Ok(Some(dyn));

        return Ok(NONE);
    }
};

export template <typename Abi = CurrentAbi>
struct ElfObject : Io::BChunk {
    Res<ElfHeader<Abi>> header() const {
        return begin().template next<ElfHeader<Abi>>();
    }

    Res<> validate() const {
        if (bytes().len() < sizeof(ElfHeader<Abi>))
            return Error::invalidData("elf object too small");

        auto h = try$(header());
        if (h.e_ident.ei_magic != ElfIdent::MAGIC)
            return Error::invalidData("not an elf file");

        return Ok();
    }

    Res<ElfProgram<Abi>> program(usize index) const {
        auto h = try$(header());
        auto phdr =
            try$(try$(begin(h.e_phoff + h.e_phentsize * index))
                     .template next<ElfPhdr<Abi>>());

        auto data =
            try$(try$(begin(phdr.p_offset))
                     .nextBytes(phdr.p_filesz));

        return Ok(ElfProgram<Abi>{phdr, data});
    }

    struct ProgramIter {
        ElfObject object;
        usize index;
        usize len;
        Res<> _status = Ok();

        Opt<ElfProgram<Abi>> next() {
            if (index >= len or not _status)
                return NONE;

            auto program = object.program(index++);
            if (not program) {
                _status = program.none();
                return NONE;
            }
            return Some(program.take());
        }

        Res<> status() const {
            return _status;
        }
    };

    ProgramIter iterProgram() const {
        auto h = header();
        if (not h)
            return {*this, 0, 0, h.none()};
        return {*this, 0, h.take().e_phnum};
    }

    Res<Opt<urange>> vrange() const {
        Opt<urange> result;
        auto programs = iterProgram();
        for (auto p : programs) {
            if (p.type() != ElfPhdrType::PT_LOAD)
                continue;
            if (result)
                result = Some(result->merge(p.vrange()));
            else
                result = Some(p.vrange());
        }
        try$(programs.status());
        return Ok(result);
    }

    Res<Opt<Str>> interp() const {
        auto programs = iterProgram();
        for (ElfProgram pg : programs) {
            if (pg.type() == ElfPhdrType::PT_INTERP) {
                return Ok(Some(Str{sub(pg.data, 0, pg.data.len() - 1).template cast<char>()}));
            }
        }
        try$(programs.status());
        return Ok(NONE);
    }

    Res<ElfSection<Abi>> section(usize index) const {
        auto h = try$(header());

        auto shdr =
            try$(try$(begin(h.e_shoff + h.e_shentsize * index))
                     .template next<ElfShdr<Abi>>());

        // NOTE: SHT_NOBITS sections occupy no space in the file
        if (static_cast<ElfShdrType>(shdr.sh_type.value()) == ElfShdrType::SHT_NOBITS)
            return Ok(ElfSection<Abi>{shdr, Bytes{}});

        auto data =
            try$(try$(begin(shdr.sh_offset))
                     .nextBytes(shdr.sh_size));

        return Ok(ElfSection<Abi>{shdr, data});
    }

    struct SectionIter {
        ElfObject object;
        usize index;
        usize len;
        Res<> _status = Ok();

        Opt<ElfSection<Abi>> next() {
            if (index >= len or not _status)
                return NONE;

            auto section = object.section(index++);
            if (not section) {
                _status = section.none();
                return NONE;
            }
            return Some(section.take());
        }

        Res<> status() const {
            return _status;
        }
    };

    SectionIter iterSection() const {
        auto h = header();
        if (not h)
            return {*this, 0, 0, h.none()};
        return {*this, 0, h.take().e_shnum};
    }

    template <template <typename> typename Section>
    Res<Section<Abi>> section(usize index) {
        auto sec = try$(section(index));
        if (sec.type() != Section<Abi>::TYPE)
            return Error::invalidData("invalid section type");
        return Ok(sec);
    }

    template <template <typename> typename Section>
    Res<Section<Abi>> section() {
        auto sections = iterSection();
        for (ElfSection<Abi> sec : sections) {
            if (sec.type() == Section<Abi>::TYPE)
                return Ok<Section<Abi>>(sec);
        }
        try$(sections.status());
        return Error::invalidData("section not found");
    }

    Res<ElfSection<Abi>> section(Str name) {
        auto shstr = try$(section<Elf::ElfStrTab>(try$(header()).e_shstrndx));

        auto sections = iterSection();
        for (ElfSection<Abi> sec : sections) {
            if (try$(shstr.string(sec.sh_name)) == name)
                return Ok(sec);
        }
        try$(sections.status());
        return Error::invalidData("section not found");
    }
};

} // namespace Karm::Elf
