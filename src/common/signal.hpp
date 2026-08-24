
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>

namespace xengine {
    class Obj;
}

enum class LoopSignal {
    Break, Continue
};

struct ReturnSignal {
    std::shared_ptr<xengine::Obj> value_;
};
