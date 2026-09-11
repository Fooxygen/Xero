
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include "llvm/IR/Value.h"

#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"

namespace xcompiler {
    class IRGen;

    class Arg {
    private:
        llvm::Value* val_  = nullptr;
        sema::Type*  type_ = nullptr;

    public:
        Arg(llvm::Value* val, sema::Type* type)
        :   val_(val), type_(type) {}

        llvm::Value* val()  const { return val_; }
        sema::Type*  type() const { return type_; }

    public:
        bool        isReferenceType() const {
            return dynamic_cast<sema::ReferenceType*>(type_) != nullptr;
        }
        sema::Type* ValueTypeGet() const {
            if (auto ref = dynamic_cast<sema::ReferenceType*>(type_))
                return ref->type_referred();
            return type_;
        }
    };

    class FnImpl {
    public:
        virtual ~FnImpl() = default;
    };

    class NativeFnImpl : public FnImpl {
    public:
        using Impl = std::function<llvm::Value*(
            IRGen&,
            const std::vector<Arg>&         // Built-in Fn required
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
        static void    Add(const std::string& name, const sema::FnSign* sign, std::unique_ptr<FnImpl>&& impl);
        static FnImpl* Lookup(const sema::FnSign* sign);
        static FnImpl* LookupTry(const sema::FnSign* sign);
    };
}
