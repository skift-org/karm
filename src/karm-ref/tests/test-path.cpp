#include <karm/test>

import Karm.Ref;
import Karm.Logger;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Ref::Tests {

test$("karm-ref-path-up-down") {
    auto path = "/a/b/c/d/e/f"_path;

    auto up = path.parent();
    expectEq$(up.str(), "/a/b/c/d/e"s);

    auto up1 = path.parent(1);
    expectEq$(up1.str(), "/a/b/c/d/e"s);

    auto up2 = path.parent(2);
    expectEq$(up2.str(), "/a/b/c/d"s);

    auto up3 = path.parent(3);
    expectEq$(up3.str(), "/a/b/c"s);

    auto up4 = path.parent(4);
    expectEq$(up4.str(), "/a/b"s);

    auto up5 = path.parent(5);
    expectEq$(up5.str(), "/a"s);

    auto up6 = path.parent(6);
    expectEq$(up6.str(), "/"s);

    return Ok();
}

test$("karm-ref-path-parent-of") {
    expect$(""_path.isParentOf(""_path));
    expect$("/a"_path.isParentOf("/a"_path));
    expect$("/a"_path.isParentOf("/a/b"_path));
    expect$("/a"_path.isParentOf("/a/b/c"_path));
    expect$("/a/b"_path.isParentOf("/a/b/c"_path));
    expectNot$("/a/c"_path.isParentOf("/a/b/c"_path));
    expect$("."_path.isParentOf("."_path));

    return Ok();
}

test$("karm-ref-path-str") {
    expectEq$(""_path.str(), "."s);
    expectEq$("/a/b/c"_path.str(), "/a/b/c"s);
    expectEq$("a/b/c"_path.str(), "a/b/c"s);
    expectEq$("a/b/c/"_path.str(), "a/b/c"s);
    expectEq$("a/b/c/."_path.str(), "a/b/c/."s);
    expectEq$("a/b/c/.."_path.str(), "a/b/c/.."s);
    expectEq$("a/b/c/../"_path.str(), "a/b/c/.."s);

    return Ok();
}

test$("karm-ref-path-basename-stem-suffix") {
    auto path = "file.txt"_path;
    expectEq$(path.basename(), "file.txt"s);
    expectEq$(path.stem(), "file"s);
    expectEq$(path.suffix(), "txt"s);

    auto path2 = "file"_path;
    expectEq$(path2.basename(), "file"s);
    expectEq$(path2.stem(), "file"s);
    expectEq$(path2.suffix(), ""s);

    auto path3 = "file."_path;
    expectEq$(path3.basename(), "file."s);
    expectEq$(path3.stem(), "file"s);
    expectEq$(path3.suffix(), ""s);

    auto path4 = "file.name.txt"_path;
    expectEq$(path4.basename(), "file.name.txt"s);
    expectEq$(path4.stem(), "file.name"s);
    expectEq$(path4.suffix(), "txt"s);

    auto path5 = ""_path;
    expectEq$(path5.basename(), ""s);
    expectEq$(path5.stem(), ""s);
    expectEq$(path5.suffix(), ""s);

    auto path6 = "/"_path;
    expectEq$(path6.basename(), ""s);
    expectEq$(path6.stem(), ""s);
    expectEq$(path6.suffix(), ""s);

    auto path7 = "/dir/file"_path;
    expectEq$(path7.basename(), "file"s);
    expectEq$(path7.stem(), "file"s);
    expectEq$(path7.suffix(), ""s);

    auto path8 = "/dir/file.txt"_path;
    expectEq$(path8.basename(), "file.txt"s);
    expectEq$(path8.stem(), "file"s);
    expectEq$(path8.suffix(), "txt"s);

    return Ok();
}

} // namespace Karm::Ref::Tests
