#include <karm/test>

import Karm.Ml;

namespace Karm::Ml::Tests {

test$("matmul") {
    Tensor a = {
        {1, 0, 1},
        {2, 1, 1},
        {0, 1, 1},
        {1, 1, 2},
    };

    Tensor b = {
        {1, 2, 1},
        {2, 3, 1},
        {4, 2, 2},
    };

    Tensor c = {
        {5, 4, 3},
        {8, 9, 5},
        {6, 5, 3},
        {11, 9, 6},
    };

    auto out = Tensor::alloc(c.shape);
    Kernels::matMul(out, a, b);
    expectEq$(c, out);
    return Ok();
}

} // namespace Karm::Ml::Tests
