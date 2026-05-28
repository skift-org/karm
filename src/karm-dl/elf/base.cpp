module;

#include <karm/macros>

export module Karm.Dl.Elf:base;

import Karm.Core;
import :abi;

namespace Karm::Elf {

// Appendix B: Assigned OSABI Values
// https://gabi.xinuos.com/elf/b-osabi.html#appendixb-assigned-osabi-values

enum class ElfOsAbi : u8 {
    ELFOSABI_NONE = 0, // No extensions or unspecified

    ELFOSABI_HPUX = 1,      // Hewlett-Packard HP-UX
    ELFOSABI_NETBSD = 2,    // NetBSD
    ELFOSABI_GNU = 3,       // GNU
    ELFOSABI_LINUX = 3,     // Linux (historical—alias for ELFOSABI_GNU)
    ELFOSABI_SOLARIS = 6,   // Sun Solaris
    ELFOSABI_AIX = 7,       // AIX
    ELFOSABI_IRIX = 8,      // IRIX
    ELFOSABI_FREEBSD = 9,   // FreeBSD
    ELFOSABI_TRU64 = 10,    // Compaq TRU64 UNIX
    ELFOSABI_MODESTO = 11,  // Novell Modesto
    ELFOSABI_OPENBSD = 12,  // Open BSD
    ELFOSABI_OPENVMS = 13,  // Open VMS
    ELFOSABI_NSK = 14,      // Hewlett-Packard Non-Stop Kernel
    ELFOSABI_AROS = 15,     // Amiga Research OS
    ELFOSABI_FENIXOS = 16,  // The FenixOS highly scalable multi-core OS
    ELFOSABI_CLOUDABI = 17, // Nuxi CloudABI
    ELFOSABI_OPENVOS = 18,  // Stratus Technologies OpenVOS,

    _LEN,
};

// MARK: 2.2. ELF Identification ------------------------------------------
// https://gabi.xinuos.com/elf/02-eheader.html#elf-identification

export struct ElfIdent {
    static constexpr Array<u8, 4> MAGIC = {
        0x7f, 0x45, 0x4C, 0x46
    };
    Array<u8, 4> ei_magic;
    ElfClass ei_class;
    ElfData ei_data;
    u8 ei_version;
    u8 ei_osabi;
    Array<u8, 8> _pad;
};

static_assert(sizeof(ElfIdent) == 16);

export Res<ElfAbi> sniffAbi(Bytes buf) {
    if (buf.len() < sizeof(ElfIdent))
        return Error::invalidData("too small to be elf");

    auto ident = buf.cast<ElfIdent>()[0];
    if (ident.ei_magic != ElfIdent::MAGIC)
        return Error::invalidData("invalid elf magic");

    return ElfAbi::any([&]<typename Abi> -> Opt<ElfAbi> {
               if (ident.ei_class == Abi::CLASS and ident.ei_data == Abi::DATA)
                   return Abi{};
               return NONE;
           })
        .okOr(Error::invalidData("invalid or unsupported elf abi"));
}

// MARK: 2.1. Contents of the ELF Header ---------------------------------------
// https://gabi.xinuos.com/elf/02-eheader.html#contents-of-the-elf-header

export enum struct ElfHeaderType : u16 {
    ET_NONE = 0, //< No file type
    ET_REL = 1,  //< Relocatable file
    ET_EXEC = 2, //< Executable file
    ET_DYN = 3,  //< Shared object file
    ET_CORE = 4, //< Core file
    _LEN,

    /// Reserved for operating system-specific semantics
    ET_LOOS = 0xfe00,
    ET_HIOS = 0xfeff,

    /// reserved for processor-specific semantics
    ET_LOPROC = 0xff00,
    ET_HIPROC = 0xffff,
};

export enum struct ElfMachine : u16 {
#define MACHINE(ID, NAME, _) NAME = ID,

#include "defs/machine.inc"

#undef MACHINE
};

export Str toStr(ElfMachine m) {
    switch (m) {
#define MACHINE(_, NAME, DESC) \
    case ElfMachine::NAME:     \
        return DESC;
#include "defs/machine.inc"

#undef MACHINE
    default:
        return "Unknown";
    }
}

export template <typename Abi>
struct [[gnu::packed]] ElfHeader {
    ElfIdent e_ident;
    Abi::Half e_type;
    Abi::Half e_machine;
    Abi::Word e_version;
    Abi::Addr e_entry;
    Abi::Off e_phoff;
    Abi::Off e_shoff;
    Abi::Word e_flags;
    Abi::Half e_ehsize;
    Abi::Half e_phentsize;
    Abi::Half e_phnum;
    Abi::Half e_shentsize;
    Abi::Half e_shnum;
    Abi::Half e_shstrndx;
};

// MARK: 3. Sections -----------------------------------------------------------
// https://gabi.xinuos.com/elf/03-sheader.html#sections

// 3.1. Special Section Indexes
// https://gabi.xinuos.com/elf/03-sheader.html#special-section-indexes
// 5.5 Special Section Indices
// https://gabi.xinuos.com/elf/05-symtab.html#section-index
export enum struct ElfSectionIndex : u16 {
    SHN_UNDEF = 0, //< Undefined symbol

    SHN_LORESERVE = 0xff00,
    SHN_LOPROC = 0xff00,
    SHN_HIPROC = 0xff1f,
    SHN_LOOS = 0xff20,
    SHN_HIOS = 0xff3f,

    SHN_ABS = 0xfff1,    //< Absolute value, not affected by relocation
    SHN_COMMON = 0xfff2, //< Unallocated common block
    SHN_XINDEX = 0xffff, //< Escape value; index is in SHT_SYMTAB_SHNDX
    SHN_HIRESERVE = 0xffff,
};

// https://gabi.xinuos.com/elf/03-sheader.html#section-type
// https://gabi.xinuos.com/elf/03-sheader.html#section-type
export enum struct ElfShdrType : u32 {
    SHT_NULL = 0,           //< Inactive section header
    SHT_PROGBITS = 1,       //< Information defined by the program
    SHT_SYMTAB = 2,         //< Symbol table (for link editing)
    SHT_STRTAB = 3,         //< String table
    SHT_RELA = 4,           //< Relocation entries with explicit addends
    SHT_HASH = 5,           //< Symbol hash table
    SHT_DYNAMIC = 6,        //< Information for dynamic linking
    SHT_NOTE = 7,           //< Information that marks the file
    SHT_NOBITS = 8,         //< Section occupies no space in file (e.g., .bss)
    SHT_REL = 9,            //< Relocation entries without explicit addends
    SHT_SHLIB = 10,         //< Reserved with unspecified semantics
    SHT_DYNSYM = 11,        //< Minimal set of dynamic linking symbols
    SHT_INIT_ARRAY = 14,    //< Array of pointers to initialization functions
    SHT_FINI_ARRAY = 15,    //< Array of pointers to termination functions
    SHT_PREINIT_ARRAY = 16, //< Array of pointers to pre-initialization functions
    SHT_GROUP = 17,         //< Section group
    SHT_SYMTAB_SHNDX = 18,  //< Extended section indices for symbol table
    SHT_RELR = 19,          //< Relative relocation entries (compact)
    _LEN,

    /// Reserved for operating system-specific semantics
    SHT_LOOS = 0x60000000,
    SHT_HIOS = 0x6fffffff,

    /// Reserved for processor-specific semantics
    SHT_LOPROC = 0x70000000,
    SHT_HIPROC = 0x7fffffff,

    /// Reserved for application programs
    SHT_LOUSER = 0x80000000,
    SHT_HIUSER = 0xffffffff,
};

// 3.4. Section Flags
// https://gabi.xinuos.com/elf/03-sheader.html#section-flags
export enum struct ElfShdrFlags : u64 {
    SHF_NONE = 0x0,               //< No flags
    SHF_WRITE = 0x1,              //< Writable during execution
    SHF_ALLOC = 0x2,              //< Occupies memory during execution
    SHF_EXECINSTR = 0x4,          //< Contains executable instructions
    SHF_MERGE = 0x10,             //< Might be merged to save space
    SHF_STRINGS = 0x20,           //< Contains null-terminated strings
    SHF_INFO_LINK = 0x40,         //< sh_info contains SHT index
    SHF_LINK_ORDER = 0x80,        //< Preserve order after combining
    SHF_OS_NONCONFORMING = 0x100, //< Non-standard OS-specific handling required
    SHF_GROUP = 0x200,            //< Section is a member of a group
    SHF_TLS = 0x400,              //< Section holds thread-local storage
    SHF_COMPRESSED = 0x800,       //< Section with compressed data,

    /// Bits reserved for operating system-specific semantics
    SHF_MASKOS = 0x0ff00000,

    /// Bits reserved for processor-specific semantics
    SHF_MASKPROC = 0xf0000000,
};

// 3.2. Section Header Table Entry ---------------------------------------
// https://gabi.xinuos.com/elf/03-sheader.html#section-header-table-entry
export template <typename Abi>
struct [[gnu::packed]] ElfShdr {
    Abi::Word sh_name;
    Abi::Word sh_type;
    Abi::Xword sh_flags;
    Abi::Addr sh_addr;
    Abi::Off sh_offset;
    Abi::Xword sh_size;
    Abi::Word sh_link;
    Abi::Word sh_info;
    Abi::Xword sh_addralign;
    Abi::Xword sh_entsize;
};

// https://gabi.xinuos.com/elf/03-sheader.html#id13
enum struct ElfChdrType : u32 {
    ELFCOMPRESS_ZLIB = 1, //< ZLIB algorithm
    ELFCOMPRESS_ZSTD = 2, //< Zstandard algorithm (ZSTD)
    _LEN,

    /// Reserved for operating system-specific semantics
    ELFCOMPRESS_LOOS = 0x60000000,
    ELFCOMPRESS_HIOS = 0x6fffffff,

    /// Reserved for processor-specific semantics
    ELFCOMPRESS_LOPROC = 0x70000000,
    ELFCOMPRESS_HIPROC = 0x7fffffff,
};

// https://gabi.xinuos.com/elf/03-sheader.html#compressed-sections
export template <typename Abi>
struct [[gnu::packed]] ElfChdr {
    Abi::Word ch_type;
    [[no_unique_address]] u8 ch_reserved[Abi::CLASS == ElfClass::ELFCLASS64 ? sizeof(typename Abi::Word) : 0];
    Abi::Xword ch_size;
    Abi::Xword ch_addralign;
};

// MARK: 5. Symbol Table
// https://gabi.xinuos.com/elf/05-symtab.html

export enum struct ElfSymBinding : u8 {
    STB_LOCAL = 0,  //< Not visible outside the object file
    STB_GLOBAL = 1, //< Visible to all combined object files
    STB_WEAK = 2,   //< Global precedence, but lower than STB_GLOBAL
    _LEN,

    /// Reserved for operating system-specific semantics
    STB_LOOS = 10,
    STB_HIOS = 12,

    /// Reserved for processor-specific semantics
    STB_LOPROC = 13,
    STB_HIPROC = 15,
};

/// 5.3 Symbol Type
export enum struct ElfSymType : u8 {
    STT_NOTYPE = 0,  //< Type is not specified
    STT_OBJECT = 1,  //< Associated with a data object (variable, array, etc.)
    STT_FUNC = 2,    //< Associated with a function or executable code
    STT_SECTION = 3, //< Associated with a section (for relocation)
    STT_FILE = 4,    //< Source file name associated with the object
    STT_COMMON = 5,  //< Uninitialized common block
    STT_TLS = 6,     //< Thread-Local Storage entity
    _LEN,

    /// Reserved for operating system-specific semantics
    STT_LOOS = 10,
    STT_HIOS = 12,

    /// Reserved for processor-specific semantics
    STT_LOPROC = 13,
    STT_HIPROC = 15,
};

/// 5.4 Symbol Visibility
export enum struct ElfSymVisibility : u8 {
    STV_DEFAULT = 0,   //< Visibility specified by binding type
    STV_INTERNAL = 1,  //< Processor-specific constraints
    STV_HIDDEN = 2,    //< Not visible to other components
    STV_PROTECTED = 3, //< Visible but not preemptable
    STV_EXPORTED = 4,  //< Forces symbol to remain global/visible
    STV_SINGLETON = 5, //< Binds to a single instance in the process
    STV_ELIMINATE = 6, //< Prevents writing to the dynamic symbol table,
    _LEN
};

export template <typename Abi>
struct [[gnu::packed]] Elf32Sym {
    Abi::Word st_name;
    Abi::Addr st_value;
    Abi::Word st_size;
    u8 st_info;
    u8 st_other;
    Abi::Half st_shndx;
};

export template <typename Abi>
struct [[gnu::packed]] Elf64Sym {
    Abi::Word st_name;
    u8 st_info;
    u8 st_other;
    Abi::Half st_shndx;
    Abi::Addr st_value;
    Abi::Xword st_size;
};

export template <typename Abi>
using ElfSym = Meta::Cond<Abi::CLASS == ElfClass::ELFCLASS64, Elf64Sym<Abi>, Elf32Sym<Abi>>;

// MARK: 6. Relocation ----------------------------------------------------
// https://gabi.xinuos.com/elf/06-reloc.html#relocation

export template <typename Abi>
struct [[gnu::packed]] ElfRel {
    Abi::Addr r_offset;
    Abi::Xword r_info;
};

export template <typename Abi>
struct [[gnu::packed]] ElfRela {
    Abi::Addr r_offset;
    Abi::Xword r_info;
    Abi::Sxword r_addend;
};

export template <typename Abi>
using ElfRelr = Abi::Xword;

// MARK: 7.1. Program Header Entry ---------------------------------------------
// https://gabi.xinuos.com/elf/07-pheader.html#program-header-entry

export enum struct ElfPhdrType : u32 {
    PT_NULL = 0,    //< Unused entry
    PT_LOAD = 1,    //< Loadable segment
    PT_DYNAMIC = 2, //< Dynamic linking information
    PT_INTERP = 3,  //< Location and size of program interpreter path
    PT_NOTE = 4,    //< Auxiliary information
    PT_SHLIB = 5,   //< Reserved with unspecified semantics
    PT_PHDR = 6,    //< Location and size of the program header table
    PT_TLS = 7,     //< Thread-Local Storage template,
    _LEN,

    /// Reserved for operating system-specific semantics
    PT_LOOS = 0x60000000,
    PT_HIOS = 0x6fffffff,

    /// Reserved for processor-specific semantics
    PT_LOPROC = 0x70000000,
    PT_HIPROC = 0x7fffffff,
};

export enum struct ElfPhdrFlags : u32 {
    PF_X = 0x1,               //< Executable
    PF_W = 0x2,               //< Writable
    PF_R = 0x4,               //< Readable
    PF_MASKOS = 0x0ff00000,   //< Unspecified OS-specific
    PF_MASKPROC = 0xf0000000, //< Unspecified processor-specific
};

export template <typename Abi>
struct [[gnu::packed]] Elf32Phdr {
    Abi::Word p_type;
    Abi::Off p_offset;
    Abi::Addr p_vaddr;
    Abi::Addr p_paddr;
    Abi::Word p_filesz;
    Abi::Word p_memsz;
    Flags<ElfPhdrFlags, typename Abi::Word> p_flags;
    Abi::Word p_align;
};

export template <typename Abi>
struct [[gnu::packed]] Elf64Phdr {
    Abi::Word p_type;
    Flags<ElfPhdrFlags, typename Abi::Word> p_flags;
    Abi::Off p_offset;
    Abi::Addr p_vaddr;
    Abi::Addr p_paddr;
    Abi::Xword p_filesz;
    Abi::Xword p_memsz;
    Abi::Xword p_align;
};

export template <typename Abi>
using ElfPhdr = Meta::Cond<Abi::CLASS == ElfClass::ELFCLASS64, Elf64Phdr<Abi>, Elf32Phdr<Abi>>;

// MARK: 8.3. Dynamic Section --------------------------------------------------
// https://gabi.xinuos.com/elf/08-dynamic.html#dynamic-section

export enum struct ElfDynTag {
    DT_NULL = 0,
    DT_NEEDED = 1,
    DT_PLTRELSZ = 2,
    DT_PLTGOT = 3,
    DT_HASH = 4,
    DT_STRTAB = 5,
    DT_SYMTAB = 6,
    DT_RELA = 7,
    DT_RELASZ = 8,
    DT_RELAENT = 9,
    DT_STRSZ = 10,
    DT_SYMENT = 11,
    DT_INIT = 12,
    DT_FINI = 13,
    DT_SONAME = 14,
    DT_RPATH = 15,
    DT_SYMBOLIC = 16,
    DT_REL = 17,
    DT_RELSZ = 18,
    DT_RELENT = 19,
    DT_PLTREL = 20,
    DT_DEBUG = 21,
    DT_TEXTREL = 22,
    DT_JMPREL = 23,
    DT_BIND_NOW = 24,
    DT_INIT_ARRAY = 25,
    DT_FINI_ARRAY = 26,
    DT_INIT_ARRAYSZ = 27,
    DT_FINI_ARRAYSZ = 28,
    DT_RUNPATH = 29,
    DT_FLAGS = 30,
    DT_ENCODING = 32,
    DT_PREINIT_ARRAY = 32,
    DT_PREINIT_ARRAYSZ = 33,
    DT_SYMTAB_SHNDX = 34,
    DT_RELRSZ = 35,
    DT_RELR = 36,
    DT_RELRENT = 37,
    DT_SYMTABSZ = 39,

    DT_LOOS = 0x6000000D,
    DT_HIOS = 0x6ffff000,
    DT_LOPROC = 0x70000000,
    DT_HIPROC = 0x7fffffff
};

export template <typename Abi>
struct ElfDyn {
    Abi::Sxword d_tag;

    union {
        Abi::Xword d_val;
        Abi::Addr d_ptr;
    };

    ElfDynTag tag() const {
        return static_cast<ElfDynTag>(d_tag.value());
    }
};

} // namespace Karm::Elf
