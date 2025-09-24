export module Karm.Sys:_embed;

import Karm.Core;
import Karm.Ref;

import :addr;
import :types;

namespace Karm::Sys {

export struct Intent;
export struct Fd;
export struct Sched;
export struct MessageReader;
export struct DirEntry;
export struct Stat;
export struct SysInfo;
export struct MemInfo;
export struct CpuInfo;
export struct UserInfo;

} // namespace Karm::Sys

namespace Karm::Sys::_Embed {

// MARK: Fd --------------------------------------------------------------------

export Res<Rc<Sys::Fd>> unpackFd(MessageReader&);

// MARK: File I/O --------------------------------------------------------------

export Res<Rc<Sys::Fd>> openFile(Ref::Url const& url);

export Res<Rc<Sys::Fd>> createFile(Ref::Url const& url);

export Res<Rc<Sys::Fd>> openOrCreateFile(Ref::Url const& url);

export Res<Pair<Rc<Sys::Fd>, Rc<Sys::Fd>>> createPipe();

export Res<Rc<Sys::Fd>> createIn();

export Res<Rc<Sys::Fd>> createOut();

export Res<Rc<Sys::Fd>> createErr();

export Res<Vec<Sys::DirEntry>> readDir(Ref::Url const& url);

export Res<> createDir(Ref::Url const& url);

export Res<Vec<Sys::DirEntry>> readDirOrCreate(Ref::Url const& url);

export Res<Stat> stat(Ref::Url const& url);

// MARK: User interactions -----------------------------------------------------

export Res<> launch(Intent intent);

export Async::Task<> launchAsync(Intent intent);

// MARK: Sockets ---------------------------------------------------------------

export Res<Rc<Sys::Fd>> listenUdp(SocketAddr addr);

export Res<Rc<Sys::Fd>> connectTcp(SocketAddr addr);

export Res<Rc<Sys::Fd>> listenTcp(SocketAddr addr);

export Res<Rc<Sys::Fd>> listenIpc(Ref::Url url);

// MARK: Time ------------------------------------------------------------------

export SystemTime now();

export Instant instant();

export Duration uptime();

// MARK: Memory Managment ------------------------------------------------------

export Res<Sys::MmapResult> memMap(Sys::MmapProps const& options);

export Res<Sys::MmapResult> memMap(Sys::MmapProps const& options, Rc<Sys::Fd> fd);

export Res<> memUnmap(void const* buf, usize len);

export Res<> memFlush(void* flush, usize len);

// MARK: System Informations ---------------------------------------------------

export Res<> populate(Sys::SysInfo&);

export Res<> populate(Sys::MemInfo&);

export Res<> populate(Vec<Sys::CpuInfo>&);

export Res<> populate(Sys::UserInfo&);

export Res<> populate(Vec<Sys::UserInfo>&);

// MARK: Process Managment -----------------------------------------------------

export Res<> sleep(Duration);

export Res<> sleepUntil(Instant);

export Res<> exit(i32);

export Res<Ref::Url> pwd();

// MARK: Sandboxing ------------------------------------------------------------

export Res<> hardenSandbox();

// MARK: Addr ------------------------------------------------------------------

export Async::Task<Vec<Ip>> ipLookupAsync(Str host);

// MARK: Asynchronous I/O ------------------------------------------------------

export Sched& globalSched();

// MARK: Bundle ----------------------------------------------------------------

export Res<Vec<String>> installedBundles();

export Res<String> currentBundle();

} // namespace Karm::Sys::_Embed
