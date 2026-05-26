export module Karm.Dl.Elf:parser;

import Karm.Core;
import :base;

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

    auto iterSym() const {
        struct Iter {
            Io::BScan s;
            usize entsize;

            Opt<ElfSym<Abi>> next() {
                if (s.ended())
                    return NONE;
                auto sym = s.peek<ElfSym<Abi>>();
                s.skip(entsize);
                return sym;
            }
        };

        return Iter{this->data, this->sh_entsize};
    }
};

export template <typename Abi>
struct ElfStrTab : ElfSection<Abi> {
    static constexpr auto TYPE = ElfShdrType::SHT_STRTAB;

    Str string(usize off) const {
        if (off == 0)
            return "";
        return Io::BScan{this->data}
            .skip(off)
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

    auto iterDyn() const {
        struct Iter {
            Io::BScan s;
            usize entsize;

            Opt<ElfDyn<Abi>> next() {
                if (s.ended())
                    return NONE;
                auto sym = s.peek<ElfDyn<Abi>>();
                s.skip(entsize);
                return sym;
            }
        };

        return Iter{this->data, this->sh_entsize};
    }

    Opt<ElfDyn<Abi>> dyn(Elf::ElfDynTag tag) {
        for (ElfDyn dyn : iterDyn())
            if (dyn.tag() == tag)
                return dyn;
        
        return NONE;
    }
};

export template <typename Abi>
struct ElfObject : Io::BChunk {
    ElfHeader<Abi> header() const {
        return begin().template next<ElfHeader<Abi>>();
    }

    ElfProgram<Abi> program(usize index) const {
        auto h = header();
        auto phdr =
            begin()
                .skip(h.e_phoff)
                .skip(h.e_phentsize * index)
                .template next<ElfPhdr<Abi>>();

        auto data =
            begin()
                .skip(phdr.p_offset)
                .nextBytes(phdr.p_filesz);

        return {phdr, data};
    }

    auto iterProgram() const {
        auto h = header();
        return urange(0, h.e_phnum) |
               Select([&](auto i) {
                   return program(i);
               });
    }

    Opt<Str> interp() const {
        for (ElfProgram pg : iterProgram()) {
            if (pg.type() == ElfPhdrType::PT_INTERP) {
                return Str{sub(pg.data, 0, pg.data.len() - 1).template cast<char>()};
            }
        }
        return NONE;
    }

    ElfSection<Abi> section(usize index) const {
        auto h = header();

        auto shdr =
            begin()
                .skip(h.e_shoff)
                .skip(h.e_shentsize * index)
                .template next<ElfShdr<Abi>>();

        auto data =
            begin()
                .skip(shdr.sh_offset)
                .nextBytes(shdr.sh_size);

        return ElfSection<Abi>{shdr, data};
    }

    auto iterSection() {
        auto h = header();
        return urange(0, h.e_shnum) |
               Select([&](auto i) {
                   return section(i);
               });
    }

    template <template <typename> typename Section>
    Res<Section<Abi>> section(usize index) {
        auto sec = section(index);
        if (sec.type() != Section<Abi>::TYPE)
            return Error::invalidData("invalid section type");
        return Ok(sec);
    }

    template <template <typename> typename Section>
    Res<Section<Abi>> section() {
        for (ElfSection<Abi> sec : iterSection()) {
            if (sec.type() == Section<Abi>::TYPE)
                return Ok<Section<Abi>>(sec);
        }
        return Error::invalidData("section not found");
    }
};

} // namespace Karm::Elf