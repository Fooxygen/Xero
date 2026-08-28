
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "llvm/IR/Value.h"

#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"

namespace xcompiler {
    class IRGen;

    class FnImpl {
    public:
        virtual ~FnImpl() = default;
    };

    class NativeFnImpl : public FnImpl {
    public:
        using Impl  = std::function<llvm::Value*(
            IRGen&,
            const std::vector<llvm::Value*>&,
            const std::vector<sema::Type*>&         // Built-in Fn required
        )>;

    private:
        Impl impl_ = nullptr;

    public:
        NativeFnImpl(Impl impl) : impl_(impl) {}

        Impl& impl() { return impl_; }
    };

    class LangFnImpl   : public FnImpl {
    public:
        using Impl  = llvm::Function*;

    private:
        Impl  impl_ = nullptr;

    public:
        LangFnImpl(Impl impl) : impl_(impl) {}

        Impl& impl() { return impl_; }
    };

    class FnImplTable {
    private:
        static inline std::unordered_map<const sema::FnSign*, std::unique_ptr<FnImpl>> table_;

    public:
        static void    Add(const sema::FnSign* sign, std::unique_ptr<FnImpl>&& impl);
        static FnImpl* Lookup(const sema::FnSign* sign);
        static FnImpl* LookupTry(const sema::FnSign* sign);
    };
}
