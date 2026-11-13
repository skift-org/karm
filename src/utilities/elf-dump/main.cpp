#include <karm/entry>

import Karm.Sys;
import Karm.Cli;
import Karm.Dl.Elf;

using namespace Karm;
using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

Async::Task<> entryPointAsync(Sys::Env& env, Async::CancellationToken) {
    auto inputArg = Cli::operand<Ref::Url>("input"s, "ELF file to dump (default: a.out)"s, "a.out"_url);

    auto allFlag = Cli::flag(NONE, "all"s, "Dump everything"s);
    auto interpFlag = Cli::flag(NONE, "interp"s, "Dump interp"s);
    auto neededFlag = Cli::flag(NONE, "needed"s, "Dump needed"s);
    auto programsFlag = Cli::flag(NONE, "programs"s, "Dump programs"s);
    auto sectionsFlag = Cli::flag(NONE, "sections"s, "Dump sections"s);
    auto symbolsFlag = Cli::flag(NONE, "symbols"s, "Dump symbols"s);

    Cli::Command cmd{
        "elf-dump"s,
        "Dump the content of an ELF file."s,
        {
            {
                "Arguments"s,
                {
                    inputArg,
                    allFlag,
                    interpFlag,
                    neededFlag,
                    programsFlag,
                    sectionsFlag,
                    symbolsFlag,
                },
            },
        },
    };

    co_try$(cmd.exec(env));

    if (not cmd)
        co_return Ok();

    auto file = co_try$(Sys::File::open(inputArg.value()));
    auto mmap = co_try$(Sys::mmap(file));
    auto abi = co_try$(Elf::sniffAbi(mmap.bytes()));

    co_return abi.visit([&]<typename Abi>(Abi) -> Res<> {
        auto object = Elf::ElfObject<Abi>{mmap.bytes()};

        if (interpFlag.value() or allFlag.value()) {
            Sys::println("Interp:");
            Sys::println("{:#}", try$(object.interp()));
        }

        if (sectionsFlag.value() or allFlag.value()) {
            Sys::println("Sections:");
            auto shstr = try$(object.template section<Elf::ElfStrTab>(try$(object.header()).e_shstrndx));
            auto sections = object.iterSection();
            for (auto section : sections)
                Sys::println(" - {:#} {} {}", try$(shstr.string(section.sh_name)), section.type(), section.data.len());
            try$(sections.status());
            Sys::println();
        }

        if (programsFlag.value() or allFlag.value()) {
            Sys::println("Program:");
            auto programs = object.iterProgram();
            for (auto program : programs)
                Sys::println(" - {} {:#08x}:{:#08x} {} {}", program.type(), program.p_paddr, program.p_vaddr, DataSize{program.p_filesz}, DataSize{program.p_memsz});
            try$(programs.status());
            Sys::println();
        }

        if (symbolsFlag.value() or allFlag.value()) {
            Sys::println("Symbols:");
            if (auto const [symtab] = object.template section<Elf::ElfSymTabSection>().ok()) {
                Sys::println("Symbols:");
                auto symstr = try$(object.template section<Elf::ElfStrTab>(symtab.sh_link));
                for (auto sym : try$(symtab.iterSym()))
                    Sys::println(" - {:#}", try$(symstr.string(sym.st_name)));
            } else {
                Sys::println("ELF Object has no symbols");
            }
            Sys::println();
        }

        if (neededFlag.value() or allFlag.value()) {
            Sys::println("Needed:");
            Elf::ElfDynamicSection dynamic = try$(object.template section<Elf::ElfDynamicSection>());
            auto strtabAddr = try$(try$(dynamic.dyn(Elf::ElfDynTag::DT_STRTAB))).d_ptr;
            auto strsz = try$(try$(dynamic.dyn(Elf::ElfDynTag::DT_STRSZ))).d_val;
            urange strrange = {strtabAddr, strsz};

            Bytes strtab = {};
            auto programs = object.iterProgram();
            for (Elf::ElfProgram program : programs) {
                if (program.type() == Elf::ElfPhdrType::PT_LOAD and
                    program.vrange().contains(strrange)) {
                    auto offset = strtabAddr - program.p_vaddr;
                    strtab = Karm::sub(program.data, offset, offset + strsz);
                    break;
                }
            }
            try$(programs.status());

            for (Elf::ElfDyn dyn : try$(dynamic.iterDyn())) {
                if (dyn.tag() == Elf::ElfDynTag::DT_NEEDED) {
                    Str soname = Str::fromNullterminated(next(strtab, dyn.d_val).template cast<char>());
                    Sys::println("- {:#}", soname);
                }
            }
            Sys::println();
        }

        return Ok();
    });
}
