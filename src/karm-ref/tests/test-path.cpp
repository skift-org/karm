#include <karm/test>

import Karm.Ref;
import Karm.Logger;

using namespace Karm::Literals;
using namespace Karm::Ref::Literals;

namespace Karm::Ref::Tests {

test$("path-up-down") {
    auto path = "/a/b/c/d/e/f"_path;

    auto up = path.parent();
    assertEq$(up.str(), "/a/b/c/d/e"s);

    auto up1 = path.parent(1);
    assertEq$(up1.str(), "/a/b/c/d/e"s);

    auto up2 = path.parent(2);
    assertEq$(up2.str(), "/a/b/c/d"s);

    auto up3 = path.parent(3);
    assertEq$(up3.str(), "/a/b/c"s);

    auto up4 = path.parent(4);
    assertEq$(up4.str(), "/a/b"s);

    auto up5 = path.parent(5);
    assertEq$(up5.str(), "/a"s);

    auto up6 = path.parent(6);
    assertEq$(up6.str(), "/"s);

    return Ok();
}

test$("path-parent-of") {
    assert$(""_path.parentOf(""_path));
    assert$("/a"_path.parentOf("/a"_path));
    assert$("/a"_path.parentOf("/a/b"_path));
    assert$("/a/"_path.parentOf("/a/b"_path));
    assert$("/a"_path.parentOf("/a/b/c"_path));
    assert$("/a/b"_path.parentOf("/a/b/c"_path));
    assertNot$("/a/c"_path.parentOf("/a/b/c"_path));
    assert$("."_path.parentOf("."_path));

    return Ok();
}

test$("path-str") {
    assertEq$(""_path.str(), "."s);
    assertEq$("/a/b/c"_path.str(), "/a/b/c"s);
    assertEq$("a/b/c"_path.str(), "a/b/c"s);

    assertEq$("a/b/c/"_path.str(), "a/b/c/"s);
    assertEq$("a/b/c/."_path.str(), "a/b/c/."s);
    assertEq$("a/b/c/.."_path.str(), "a/b/c/.."s);
    assertEq$("a/b/c/../"_path.str(), "a/b/c/../"s);

    return Ok();
}

test$("path-basename-stem-suffix") {
    auto path = "file.txt"_path;
    assertEq$(path.basename(), "file.txt"s);
    assertEq$(path.stem(), "file"s);
    assertEq$(path.suffix(), "txt"s);

    auto path2 = "file"_path;
    assertEq$(path2.basename(), "file"s);
    assertEq$(path2.stem(), "file"s);
    assertEq$(path2.suffix(), ""s);

    auto path3 = "file."_path;
    assertEq$(path3.basename(), "file."s);
    assertEq$(path3.stem(), "file"s);
    assertEq$(path3.suffix(), ""s);

    auto path4 = "file.name.txt"_path;
    assertEq$(path4.basename(), "file.name.txt"s);
    assertEq$(path4.stem(), "file.name"s);
    assertEq$(path4.suffix(), "txt"s);

    auto path5 = ""_path;
    assertEq$(path5.basename(), ""s);
    assertEq$(path5.stem(), ""s);
    assertEq$(path5.suffix(), ""s);

    auto path6 = "/"_path;
    assertEq$(path6.basename(), ""s);
    assertEq$(path6.stem(), ""s);
    assertEq$(path6.suffix(), ""s);

    auto path7 = "/dir/file"_path;
    assertEq$(path7.basename(), "file"s);
    assertEq$(path7.stem(), "file"s);
    assertEq$(path7.suffix(), ""s);

    auto path8 = "/dir/file.txt"_path;
    assertEq$(path8.basename(), "file.txt"s);
    assertEq$(path8.stem(), "file"s);
    assertEq$(path8.suffix(), "txt"s);

    auto path9 = "/dir/subdir/"_path;
    assertEq$(path9.basename(), ""s);
    assertEq$(path9.stem(), ""s);
    assertEq$(path9.suffix(), ""s);

    return Ok();
}

} // namespace Karm::Ref::Tests