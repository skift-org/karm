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
    auto programsFlag = Cli::flag(NONE, "programs"s, "Dump programs"s);
    auto sectionsFlag = Cli::flag(NONE, "sections"s, "Dump symbols"s);
    auto symbolsFlag = Cli::flag(NONE, "symbols"s, "Dump symbols"s);

    Cli::Command cmd{
        "elf-dump"s,
        "Dump the content of an ELF file."s,
        {{
            "Arguments"s,
            {
                inputArg,
                allFlag,
                programsFlag,
                sectionsFlag,
                symbolsFlag,
            },
        }},
    };

    co_trya$(cmd.execAsync(env));

    if (not cmd)
        co_return Ok();

    auto file = co_try$(Sys::File::open(inputArg.value()));
    auto mmap = co_try$(Sys::mmap(file));
    auto object = Elf::ElfObject<Elf::Elf64LeAbi>{mmap.bytes()};

    if (sectionsFlag.value() or allFlag.value()) {
        Sys::println("Sections:");
        auto shstr = co_try$(object.section<Elf::ElfStrTab>(object.header().e_shstrndx));
        for (auto section : object.iterSection())
            Sys::println(" - {#}", shstr.string(section.sh_name));
        Sys::println();
    }

    if (symbolsFlag.value() or allFlag.value()) {
        Sys::println("Program:");
        for (auto program : object.iterProgram()) {
            Sys::println(" - {} {:#08x}:{:#08x} {}", program.type(), program.p_paddr, program.p_vaddr, DataSize{program.p_memsz});
        }
        Sys::println();
    }

    if (symbolsFlag.value() or allFlag.value()) {
        if (auto const [symtab] = object.section<Elf::ElfSymTabSection>().ok()) {
            Sys::println("Symbols:");
            auto symstr = co_try$(object.section<Elf::ElfStrTab>(symtab.sh_link));
            for (auto sym : symtab.iterSym())
                Sys::println(" - {#}", symstr.string(sym.st_name));
        } else {
            Sys::println("ELF Object has no symbols");
        }
        Sys::println();
    }

    co_return Ok();
}
