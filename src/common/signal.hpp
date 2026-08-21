
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>

namespace rt {
    class Obj;
}

enum class LoopSignal {
    Break, Continue
};

struct ReturnSignal {
    std::shared_ptr<rt::Obj> value_;
};
