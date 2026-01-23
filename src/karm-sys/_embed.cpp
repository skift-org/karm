export module Karm.Sys:_embed;

import Karm.Core;
import Karm.Ref;
import Karm.Sys.Base;

import :addr;
import :types;

namespace Karm::Sys {

export struct Intent;
export struct Fd;
export struct Pid;
export struct Sched;
export struct SysInfo;
export struct MemInfo;
export struct CpuInfo;
export struct UserInfo;
export struct Command;

} // namespace Karm::Sys

namespace Karm::Sys::_Embed {

// MARK: Fd --------------------------------------------------------------------

export Res<Rc<Fd>> deserializeFd(Serde::Deserializer&);

// MARK: File I/O --------------------------------------------------------------

export Res<Rc<Fd>> openFile(Ref::Url const& url);

export Res<Rc<Fd>> createFile(Ref::Url const& url);

export Res<Rc<Fd>> openOrCreateFile(Ref::Url const& url);

export Res<Pair<Rc<Fd>, Rc<Fd>>> createPipe();

export Res<Rc<Fd>> createIn();

export Res<Rc<Fd>> createOut();

export Res<Rc<Fd>> createErr();

export Res<Vec<DirEntry>> readDir(Ref::Url const& url);

export Res<> createDir(Ref::Url const& url);

export Res<Vec<DirEntry>> readDirOrCreate(Ref::Url const& url);

export Res<Stat> stat(Ref::Url const& url);

// MARK: User interactions -----------------------------------------------------

export Res<> launch(Intent intent);

export Async::Task<> launchAsync(Intent intent);

// MARK: Process ---------------------------------------------------------------

export Res<Rc<Pid>> run(Command const&);

// MARK: Sockets ---------------------------------------------------------------

export Res<Rc<Fd>> listenUdp(SocketAddr addr);

export Res<Rc<Fd>> connectTcp(SocketAddr addr);

export Res<Rc<Fd>> listenTcp(SocketAddr addr);

export Res<Rc<Fd>> connectIpc(Ref::Url url);

export Res<Rc<Fd>> listenIpc(Ref::Url url);

// MARK: Time ------------------------------------------------------------------

export SystemTime now();

export Instant instant();

export Duration uptime();

// MARK: Memory Managment ------------------------------------------------------

export Res<MmapResult> memMap(MmapProps const& options);

export Res<MmapResult> memMap(MmapProps const& options, Rc<Fd> fd);

export Res<> memUnmap(void const* buf, usize len);

export Res<> memFlush(void* flush, usize len);

export usize pageSize();

// MARK: System Information ----------------------------------------------------

export Res<> populate(SysInfo&);

export Res<> populate(MemInfo&);

export Res<> populate(Vec<CpuInfo>&);

export Res<> populate(UserInfo&);

export Res<> populate(Vec<UserInfo>&);

// MARK: Process Management ----------------------------------------------------

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
