module;

#include <karm/macros>

export module Karm.Dl.Elf:base;

import Karm.Core;

namespace Karm::Elf {

export constexpr Array<u8, 4> MAGIC = {
    0x7f, 0x45, 0x4C, 0x46
};

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

export struct ElfIdent {
    Array<u8, 4> ei_magic;
    ElfClass ei_class;
    ElfData ei_data;
    u8 ei_version;
    u8 ei_osabi;
    Array<u8, 8> _pad;
};

static_assert(sizeof(ElfIdent) == 16);

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
    EM_NONE = 0,              //< No machine
    EM_M32 = 1,               //< AT&T WE 32100
    EM_SPARC = 2,             //< SPARC
    EM_386 = 3,               //< Intel 80386
    EM_68K = 4,               //< Motorola 68000
    EM_88K = 5,               //< Motorola 88000
    EM_IAMCU = 6,             //< Intel MCU
    EM_860 = 7,               //< Intel 80860
    EM_MIPS = 8,              //< MIPS I Architecture
    EM_S370 = 9,              //< IBM System/370 Processor
    EM_MIPS_RS3_LE = 10,      //< MIPS RS3000 Little-endian
    EM_PARISC = 15,           //< Hewlett-Packard PA-RISC
    EM_VPP500 = 17,           //< Fujitsu VPP500
    EM_SPARC32PLUS = 18,      //< Enhanced instruction set SPARC
    EM_960 = 19,              //< Intel 80960
    EM_PPC = 20,              //< PowerPC
    EM_PPC64 = 21,            //< 64-bit PowerPC
    EM_S390 = 22,             //< IBM System/390 Processor
    EM_SPU = 23,              //< IBM SPU/SPC
    EM_V800 = 36,             //< NEC V800
    EM_FR20 = 37,             //< Fujitsu FR20
    EM_RH32 = 38,             //< TRW RH-32
    EM_RCE = 39,              //< Motorola RCE
    EM_ARM = 40,              //< ARM 32-bit architecture (AARCH32)
    EM_ALPHA = 41,            //< Digital Alpha
    EM_SH = 42,               //< Hitachi SH
    EM_SPARCV9 = 43,          //< SPARC Version 9
    EM_TRICORE = 44,          //< Siemens TriCore embedded processor
    EM_ARC = 45,              //< Argonaut RISC Core, Argonaut Technologies Inc.
    EM_H8_300 = 46,           //< Hitachi H8/300
    EM_H8_300H = 47,          //< Hitachi H8/300H
    EM_H8S = 48,              //< Hitachi H8S
    EM_H8_500 = 49,           //< Hitachi H8/500
    EM_IA_64 = 50,            //< Intel IA-64 processor architecture
    EM_MIPS_X = 51,           //< Stanford MIPS-X
    EM_COLDFIRE = 52,         //< Motorola ColdFire
    EM_68HC12 = 53,           //< Motorola M68HC12
    EM_MMA = 54,              //< Fujitsu MMA Multimedia Accelerator
    EM_PCP = 55,              //< Siemens PCP
    EM_NCPU = 56,             //< Sony nCPU embedded RISC processor
    EM_NDR1 = 57,             //< Denso NDR1 microprocessor
    EM_STARCORE = 58,         //< Motorola Star*Core processor
    EM_ME16 = 59,             //< Toyota ME16 processor
    EM_ST100 = 60,            //< STMicroelectronics ST100 processor
    EM_TINYJ = 61,            //< Advanced Logic Corp. TinyJ embedded processor family
    EM_X86_64 = 62,           //< AMD x86-64 architecture
    EM_PDSP = 63,             //< Sony DSP Processor
    EM_PDP10 = 64,            //< Digital Equipment Corp. PDP-10
    EM_PDP11 = 65,            //< Digital Equipment Corp. PDP-11
    EM_FX66 = 66,             //< Siemens FX66 microcontroller
    EM_ST9PLUS = 67,          //< STMicroelectronics ST9+ 8/16 bit microcontroller
    EM_ST7 = 68,              //< STMicroelectronics ST7 8-bit microcontroller
    EM_68HC16 = 69,           //< Motorola MC68HC16 Microcontroller
    EM_68HC11 = 70,           //< Motorola MC68HC11 Microcontroller
    EM_68HC08 = 71,           //< Motorola MC68HC08 Microcontroller
    EM_68HC05 = 72,           //< Motorola MC68HC05 Microcontroller
    EM_SVX = 73,              //< Silicon Graphics SVx
    EM_ST19 = 74,             //< STMicroelectronics ST19 8-bit microcontroller
    EM_VAX = 75,              //< Digital VAX
    EM_CRIS = 76,             //< Axis Communications 32-bit embedded processor
    EM_JAVELIN = 77,          //< Infineon Technologies 32-bit embedded processor
    EM_FIREPATH = 78,         //< Element 14 64-bit DSP Processor
    EM_ZSP = 79,              //< LSI Logic 16-bit DSP Processor
    EM_MMIX = 80,             //< Donald Knuth’s educational 64-bit processor
    EM_HUANY = 81,            //< Harvard University machine-independent object files
    EM_PRISM = 82,            //< SiTera Prism
    EM_AVR = 83,              //< Atmel AVR 8-bit microcontroller
    EM_FR30 = 84,             //< Fujitsu FR30
    EM_D10V = 85,             //< Mitsubishi D10V
    EM_D30V = 86,             //< Mitsubishi D30V
    EM_V850 = 87,             //< NEC v850
    EM_M32R = 88,             //< Mitsubishi M32R
    EM_MN10300 = 89,          //< Matsushita MN10300
    EM_MN10200 = 90,          //< Matsushita MN10200
    EM_PJ = 91,               //< picoJava
    EM_OPENRISC = 92,         //< OpenRISC 32-bit embedded processor
    EM_ARC_COMPACT = 93,      //< ARC International ARCompact processor
    EM_XTENSA = 94,           //< Tensilica Xtensa Architecture
    EM_VIDEOCORE = 95,        //< Alphamosaic VideoCore processor
    EM_TMM_GPP = 96,          //< Thompson Multimedia General Purpose Processor
    EM_NS32K = 97,            //< National Semiconductor 32000 series
    EM_TPC = 98,              //< Tenor Network TPC processor
    EM_SNP1K = 99,            //< Trebia SNP 1000 processor
    EM_ST200 = 100,           //< STMicroelectronics ST200 microcontroller
    EM_IP2K = 101,            //< Ubicom IP2xxx microcontroller family
    EM_MAX = 102,             //< MAX Processor
    EM_CR = 103,              //< National Semiconductor CompactRISC microprocessor
    EM_F2MC16 = 104,          //< Fujitsu F2MC16
    EM_MSP430 = 105,          //< Texas Instruments embedded microcontroller msp430
    EM_BLACKFIN = 106,        //< Analog Devices Blackfin (DSP) processor
    EM_SE_C33 = 107,          //< S1C33 Family of Seiko Epson processors
    EM_SEP = 108,             //< Sharp embedded microprocessor
    EM_ARCA = 109,            //< Arca RISC Microprocessor
    EM_UNICORE = 110,         //< Microprocessor series from PKU-Unity Ltd. and MPRC
    EM_EXCESS = 111,          //< eXcess: 16/32/64-bit configurable embedded CPU
    EM_DXP = 112,             //< Icera Semiconductor Inc. Deep Execution Processor
    EM_ALTERA_NIOS2 = 113,    //< Altera Nios II soft-core processor
    EM_CRX = 114,             //< National Semiconductor CompactRISC CRX microprocessor
    EM_XGATE = 115,           //< Motorola XGATE embedded processor
    EM_C166 = 116,            //< Infineon C16x/XC16x processor
    EM_M16C = 117,            //< Renesas M16C series microprocessors
    EM_DSPIC30F = 118,        //< Microchip Technology dsPIC30F Digital Signal Controller
    EM_CE = 119,              //< Freescale Communication Engine RISC core
    EM_M32C = 120,            //< Renesas M32C series microprocessors
    EM_TSK3000 = 131,         //< Altium TSK3000 core
    EM_RS08 = 132,            //< Freescale RS08 embedded processor
    EM_SHARC = 133,           //< Analog Devices SHARC family of 32-bit DSP processors
    EM_ECOG2 = 134,           //< Cyan Technology eCOG2 microprocessor
    EM_SCORE7 = 135,          //< Sunplus S+core7 RISC processor
    EM_DSP24 = 136,           //< New Japan Radio (NJR) 24-bit DSP Processor
    EM_VIDEOCORE3 = 137,      //< Broadcom VideoCore III processor
    EM_LATTICEMICO32 = 138,   //< RISC processor for Lattice FPGA architecture
    EM_SE_C17 = 139,          //< Seiko Epson C17 family
    EM_TI_C6000 = 140,        //< The Texas Instruments TMS320C6000 DSP family
    EM_TI_C2000 = 141,        //< The Texas Instruments TMS320C2000 DSP family
    EM_TI_C5500 = 142,        //< The Texas Instruments TMS320C55x DSP family
    EM_TI_ARP32 = 143,        //< Texas Instruments Application Specific RISC Processor
    EM_TI_PRU = 144,          //< Texas Instruments Programmable Realtime Unit
    EM_MMDSP_PLUS = 160,      //< STMicroelectronics 64bit VLIW Data Signal Processor
    EM_CYPRESS_M8C = 161,     //< Cypress M8C microprocessor
    EM_R32C = 162,            //< Renesas R32C series microprocessors
    EM_TRIMEDIA = 163,        //< NXP Semiconductors TriMedia architecture family
    EM_QDSP6 = 164,           //< QUALCOMM DSP6 Processor
    EM_8051 = 165,            //< Intel 8051 and variants
    EM_STXP7X = 166,          //< STMicroelectronics STxP7x family
    EM_NDS32 = 167,           //< Andes Technology compact code size embedded RISC
    EM_ECOG1 = 168,           //< Cyan Technology eCOG1X family
    EM_ECOG1X = 168,          //< Cyan Technology eCOG1X family (alias)
    EM_MAXQ30 = 169,          //< Dallas Semiconductor MAXQ30 Core Micro-controllers
    EM_XIMO16 = 170,          //< New Japan Radio (NJR) 16-bit DSP Processor
    EM_MANIK = 171,           //< M2000 Reconfigurable RISC Microprocessor
    EM_CRAYNV2 = 172,         //< Cray Inc. NV2 vector architecture
    EM_RX = 173,              //< Renesas RX family
    EM_METAG = 174,           //< Imagination Technologies META processor architecture
    EM_MCST_ELBRUS = 175,     //< MCST Elbrus general purpose hardware architecture
    EM_ECOG16 = 176,          //< Cyan Technology eCOG16 family
    EM_CR16 = 177,            //< National Semiconductor CompactRISC CR16 16-bit
    EM_ETPU = 178,            //< Freescale Extended Time Processing Unit
    EM_SLE9X = 179,           //< Infineon Technologies SLE9X core
    EM_L10M = 180,            //< Intel L10M
    EM_K10M = 181,            //< Intel K10M
    EM_AARCH64 = 183,         //< ARM 64-bit architecture (AARCH64)
    EM_AVR32 = 185,           //< Atmel Corporation 32-bit microprocessor family
    EM_STM8 = 186,            //< STMicroeletronics STM8 8-bit microcontroller
    EM_TILE64 = 187,          //< Tilera TILE64 multicore architecture family
    EM_TILEPRO = 188,         //< Tilera TILEPro multicore architecture family
    EM_MICROBLAZE = 189,      //< Xilinx MicroBlaze 32-bit RISC soft processor core
    EM_CUDA = 190,            //< NVIDIA CUDA architecture
    EM_TILEGX = 191,          //< Tilera TILE-Gx multicore architecture family
    EM_CLOUDSHIELD = 192,     //< CloudShield architecture family
    EM_COREA_1ST = 193,       //< KIPO-KAIST Core-A 1st generation processor family
    EM_COREA_2ND = 194,       //< KIPO-KAIST Core-A 2nd generation processor family
    EM_ARC_COMPACT2 = 195,    //< Synopsys ARCompact V2
    EM_OPEN8 = 196,           //< Open8 8-bit RISC soft processor core
    EM_RL78 = 197,            //< Renesas RL78 family
    EM_VIDEOCORE5 = 198,      //< Broadcom VideoCore V processor
    EM_78KOR = 199,           //< Renesas 78KOR family
    EM_56800EX = 200,         //< Freescale 56800EX Digital Signal Controller (DSC)
    EM_BA1 = 201,             //< Beyond BA1 CPU architecture
    EM_BA2 = 202,             //< Beyond BA2 CPU architecture
    EM_XCORE = 203,           //< XMOS xCORE processor family
    EM_MCHP_PIC = 204,        //< Microchip 8-bit PIC(r) family
    EM_INTEL205 = 205,        //< Reserved by Intel
    EM_INTEL206 = 206,        //< Reserved by Intel
    EM_INTEL207 = 207,        //< Reserved by Intel
    EM_INTEL208 = 208,        //< Reserved by Intel
    EM_INTEL209 = 209,        //< Reserved by Intel
    EM_KM32 = 210,            //< KM211 KM32 32-bit processor
    EM_KMX32 = 211,           //< KM211 KMX32 32-bit processor
    EM_KMX16 = 212,           //< KM211 KMX16 16-bit processor
    EM_KMX8 = 213,            //< KM211 KMX8 8-bit processor
    EM_KVARC = 214,           //< KM211 KVARC processor
    EM_CDP = 215,             //< Paneve CDP architecture family
    EM_COGE = 216,            //< Cognitive Smart Memory Processor
    EM_COOL = 217,            //< Bluechip Systems CoolEngine
    EM_NORC = 218,            //< Nanoradio Optimized RISC
    EM_CSR_KALIMBA = 219,     //< CSR Kalimba architecture family
    EM_Z80 = 220,             //< Zilog Z80
    EM_VISIUM = 221,          //< Controls and Data Services VISIUMcore processor
    EM_FT32 = 222,            //< FTDI Chip FT32 high performance 32-bit RISC
    EM_MOXIE = 223,           //< Moxie processor family
    EM_AMDGPU = 224,          //< AMD GPU architecture
    EM_RISCV = 243,           //< RISC-V
    EM_LANAI = 244,           //< Lanai processor
    EM_CEVA = 245,            //< CEVA Processor Architecture Family
    EM_CEVA_X2 = 246,         //< CEVA X2 Processor Family
    EM_BPF = 247,             //< Linux BPF – in-kernel virtual machine
    EM_GRAPHCORE_IPU = 248,   //< Graphcore Intelligent Processing Unit
    EM_IMG1 = 249,            //< Imagination Technologies
    EM_NFP = 250,             //< Netronome Flow Processor (NFP)
    EM_VE = 251,              //< NEC Vector Engine
    EM_CSKY = 252,            //< C-SKY processor family
    EM_ARC_COMPACT3_64 = 253, //< Synopsys ARCv2.3 64-bit
    EM_MCS6502 = 254,         //< MOS Technology MCS 6502 processor
    EM_ARC_COMPACT3 = 255,    //< Synopsys ARCv2.3 32-bit
    EM_KVX = 256,             //< Kalray VLIW core of the MPPA processor family
    EM_65816 = 257,           //< WDC 65816/65C816
    EM_LOONGARCH = 258,       //< Loongson Loongarch
    EM_KF32 = 259,            //< ChipON KungFu32
    EM_U16_U8CORE = 260,      //< LAPIS nX-U16/U8
    EM_TACHYUM = 261,         //< Reserved for Tachyum processor
    EM_56800EF = 262,         //< NXP 56800EF Digital Signal Controller (DSC)
    EM_SBF = 263,             //< Solana Bytecode Format
    EM_AIENGINE = 264,        //< AMD/Xilinx AIEngine architecture
    EM_SIMA_MLA = 265,        //< SiMa MLA
    EM_BANG = 266,            //< Cambricon BANG
    EM_LOONGGPU = 267,        //< Loongson LoongGPU
    EM_SW64 = 268,            //< Wuxi Institute of Advanced Technology SW64
    EM_AIECTRLCODE = 269,     //< AMD/Xilinx AIEngine ctrlcode
};

// MARK: 2.1. Contents of the ELF Header ---------------------------------------
// https://gabi.xinuos.com/elf/02-eheader.html#contents-of-the-elf-header
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
    _LEN,

    /// Bits reserved for operating system-specific semantics
    SHF_MASKOS = 0x0ff00000,

    /// Bits reserved for processor-specific semantics
    SHF_MASKPROC = 0xf0000000,
};

// MARK: 3.2. Section Header Table Entry ---------------------------------------
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

export template <typename Abi>
struct [[gnu::packed]] ElfChdr {
    Abi::Word ch_type;
    [[no_unique_address]] u8 ch_reserved[Abi::CLASS == ElfClass::ELFCLASS64 ? sizeof(typename Abi::Word) : 0];
    Abi::Xword ch_size;
    Abi::Xword ch_addralign;
};

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

/// 5.5 Special Section Indices
export enum struct ElfSectionIndex : u16 {
    SHN_UNDEF = 0,       //< Undefined symbol
    SHN_ABS = 0xfff1,    //< Absolute value, not affected by relocation
    SHN_COMMON = 0xfff2, //< Unallocated common block
    SHN_XINDEX = 0xffff, //< Escape value; index is in SHT_SYMTAB_SHNDX
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

export template <typename Abi>
struct ElfRel {
    Abi::Addr r_offset;
    Abi::Xword r_info;
};

export template <typename Abi>
struct ElfRela {
    Abi::Addr r_offset;
    Abi::Xword r_info;
    Abi::Sxword r_addend;
};

export template <typename Abi>
using ElfRelr = Abi::Xword;

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

// MARK: 7.1. Program Header Entry ---------------------------------------------
// https://gabi.xinuos.com/elf/07-pheader.html#program-header-entry

export template <typename Abi>
struct Elf32Phdr {
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
struct Elf64Phdr {
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
using ElfPhdr = Meta::Cond<Abi::CLASS == ElfClass::ELFCLASS64, Elf32Phdr<Abi>, Elf64Phdr<Abi>>;

} // namespace Karm::Elf
