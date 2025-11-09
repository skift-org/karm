module;

#include <ce-bootfs/bootfs.h>
#include <karm-core/macros.h>

export module Karm.Fs:bootfs;

import Karm.Sys;
import :node;
import :vfs;

namespace Karm::Fs {

Async::Task<Rc<Node>> mountBootfsAsync(Ref::Url url) {
    auto file = co_try$(Sys::File::open(url));
    auto fsMmap = co_try$(Sys::mmap(file));
    auto fsRoot = co_trya$(createAsync<VDir>());

    auto const* header = reinterpret_cast<bootfs_header_t const*>(fsMmap.bytes().buf());
    bootfs_iter_t iter = bootfs_iter(header);
    for (bootfs_dirent_t const* dirent = bootfs_next(&iter); dirent != nullptr; dirent = bootfs_next(&iter)) {
        auto filePath = Ref::Path::parse(Str::fromNullterminated(dirent->name));
        auto fileProps = Sys::MmapProps{
            .offset = dirent->offset,
            .size = alignUp(dirent->length, Sys::pageSize()),
        };
        auto fileMmap = co_try$(Sys::mmap(file, fileProps));

        auto fileParent = co_trya$(mkdirsAsync(fsRoot, filePath.parent()));
        auto fileNode = co_trya$(createAsync<VFileMmap>(std::move(fileMmap), dirent->length));
        co_trya$(fileParent->linkAsync(filePath.basename(), fileNode));
    }

    co_return Ok(fsRoot);
}

} // namespace Karm::Fs
