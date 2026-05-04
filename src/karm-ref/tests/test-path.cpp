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
    expectEq$(""_path.str(), ".");
    expectEq$("/a/b/c"_path.str(), "/a/b/c");
    expectEq$("a/b/c"_path.str(), "a/b/c");
    expectEq$("a/b/c/"_path.str(), "a/b/c");
    expectEq$("a/b/c/."_path.str(), "a/b/c/.");
    expectEq$("a/b/c/.."_path.str(), "a/b/c/..");
    expectEq$("a/b/c/../"_path.str(), "a/b/c/..");

    return Ok();
}

test$("karm-ref-path-basename-stem-suffix") {
    auto path = "file.txt"_path;
    expectEq$(path.basename(), "file.txt");
    expectEq$(path.stem(), "file");
    expectEq$(path.suffix(), "txt");

    auto path2 = "file"_path;
    expectEq$(path2.basename(), "file");
    expectEq$(path2.stem(), "file");
    expectEq$(path2.suffix(), "");

    auto path3 = "file."_path;
    expectEq$(path3.basename(), "file.");
    expectEq$(path3.stem(), "file");
    expectEq$(path3.suffix(), "");

    auto path4 = "file.name.txt"_path;
    expectEq$(path4.basename(), "file.name.txt");
    expectEq$(path4.stem(), "file.name");
    expectEq$(path4.suffix(), "txt");

    auto path5 = ""_path;
    expectEq$(path5.basename(), "");
    expectEq$(path5.stem(), "");
    expectEq$(path5.suffix(), "");

    auto path6 = "/"_path;
    expectEq$(path6.basename(), "");
    expectEq$(path6.stem(), "");
    expectEq$(path6.suffix(), "");

    auto path7 = "/dir/file"_path;
    expectEq$(path7.basename(), "file");
    expectEq$(path7.stem(), "file");
    expectEq$(path7.suffix(), "");

    auto path8 = "/dir/file.txt"_path;
    expectEq$(path8.basename(), "file.txt");
    expectEq$(path8.stem(), "file");
    expectEq$(path8.suffix(), "txt");

    return Ok();
}

} // namespace Karm::Ref::Tests
