export module Karm.Dl.Elf:abi;

import Karm.Core;

namespace Karm::Elf {

export enum struct ElfClass : u8 {
    ELFCLASSNONE = 0,
    ELFCLASS32 = 1,
    ELFCLASS64 = 2,
    _LEN,
};

export enum struct ElfData : u8 {
    ELFDATANONE = 0,
    ELFDATA2LSB = 1,
    ELFDATA2MSB = 2,
    _LEN,
};

export struct Elf32LeAbi {
    using Addr = u32le;
    using Off = u32le;
    using Half = u16le;
    using Word = u32le;
    using Sword = i32le;
    using Xword = u32le;
    using Sxword = i32le;

    static constexpr ElfClass CLASS = ElfClass::ELFCLASS32;
    static constexpr ElfData DATA = ElfData::ELFDATA2LSB;
};

export struct Elf32BeAbi {
    using Addr = u32be;
    using Off = u32be;
    using Half = u16be;
    using Word = u32be;
    using Sword = i32be;
    using Xword = u32be;
    using Sxword = i32be;

    static constexpr ElfClass CLASS = ElfClass::ELFCLASS32;
    static constexpr ElfData DATA = ElfData::ELFDATA2MSB;
};

export struct Elf64LeAbi {
    using Addr = u64le;
    using Off = u64le;
    using Half = u16le;
    using Word = u32le;
    using Sword = i32le;
    using Xword = u64le;
    using Sxword = i64le;

    static constexpr ElfClass CLASS = ElfClass::ELFCLASS64;
    static constexpr ElfData DATA = ElfData::ELFDATA2LSB;
};

export struct Elf64BeAbi {
    using Addr = u64be;
    using Off = u64be;
    using Half = u16be;
    using Word = u32be;
    using Sword = i32be;
    using Xword = u64be;
    using Sxword = i64be;

    static constexpr ElfClass CLASS = ElfClass::ELFCLASS64;
    static constexpr ElfData DATA = ElfData::ELFDATA2MSB;
};

export using ElfAbi = Union<
    Elf32LeAbi,
    Elf32BeAbi,
    Elf64LeAbi,
    Elf64BeAbi>;

#ifdef __ck_bits_64__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
export using CurrentAbi = Elf64LeAbi;
#    else
export using CurrentAbi = Elf64BeAbi;
#    endif
#elifdef __ck_bits_32__
#    if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
export using CurrentAbi = Elf32LeAbi;
#    else
export using CurrentAbi = Elf32BeAbi;
#    endif
#else
#endif

} // namespace Karm::Elf
