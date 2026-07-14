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

    co_trya$(cmd.execAsync(env));

    if (not cmd)
        co_return Ok();

    auto file = co_try$(Sys::File::open(inputArg.value()));
    auto mmap = co_try$(Sys::mmap(file));
    auto abi = co_try$(Elf::sniffAbi(mmap.bytes()));

    co_return abi.visit([&]<typename Abi>(Abi) -> Res<> {
        auto object = Elf::ElfObject<Abi>{mmap.bytes()};

        if (interpFlag.value() or allFlag.value()) {
            Sys::println("Interp:");
            Sys::println("{:#}", object.interp());
        }

        if (sectionsFlag.value() or allFlag.value()) {
            Sys::println("Sections:");
            auto shstr = try$(object.template section<Elf::ElfStrTab>(object.header().e_shstrndx));
            for (auto section : object.iterSection())
                Sys::println(" - {:#} {} {}", shstr.string(section.sh_name), section.type(), section.data.len());
            Sys::println();
        }

        if (programsFlag.value() or allFlag.value()) {
            Sys::println("Program:");
            for (auto program : object.iterProgram())
                Sys::println(" - {} {:#08x}:{:#08x} {} {}", program.type(), program.p_paddr, program.p_vaddr, DataSize{program.p_filesz}, DataSize{program.p_memsz});
            Sys::println();
        }

        if (symbolsFlag.value() or allFlag.value()) {
            Sys::println("Symbols:");
            if (auto const [symtab] = object.template section<Elf::ElfSymTabSection>().ok()) {
                Sys::println("Symbols:");
                auto symstr = try$(object.template section<Elf::ElfStrTab>(symtab.sh_link));
                for (auto sym : symtab.iterSym())
                    Sys::println(" - {:#}", symstr.string(sym.st_name));
            } else {
                Sys::println("ELF Object has no symbols");
            }
            Sys::println();
        }

        if (neededFlag.value() or allFlag.value()) {
            Sys::println("Needed:");
            Elf::ElfDynamicSection dynamic = try$(object.template section<Elf::ElfDynamicSection>());
            auto strtabAddr = try$(dynamic.dyn(Elf::ElfDynTag::DT_STRTAB)).d_ptr;
            auto strsz = try$(dynamic.dyn(Elf::ElfDynTag::DT_STRSZ)).d_val;
            urange strrange = {strtabAddr, strsz};

            Bytes strtab = {};
            for (Elf::ElfProgram program : object.iterProgram()) {
                if (program.type() == Elf::ElfPhdrType::PT_LOAD and
                    program.vrange().contains(strrange)) {
                    auto offset = strtabAddr - program.p_vaddr;
                    strtab = Karm::sub(program.data, offset, offset + strsz);
                    break;
                }
            }

            for (Elf::ElfDyn dyn : dynamic.iterDyn()) {
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
