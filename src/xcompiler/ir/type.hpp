
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "llvm/IR/Value.h"

#include "sema/sign.hpp"
#include "sema/type.hpp"

namespace xcompiler {
    class IRGen;

    class MethodImpl {
    public:
        const sema::FnSign* link_fnsign_ = nullptr;

        MethodImpl(const sema::FnSign* link_fnsign)
        :   link_fnsign_(link_fnsign) {}

        virtual ~MethodImpl() = default;
    };

    class NativeMethodImpl : public MethodImpl {
    public:
        using NativeFn = llvm::Value* (*)(IRGen&, const std::vector<llvm::Value*>&);
        NativeFn fn_   = nullptr;

        NativeMethodImpl(const sema::FnSign* fnsign, NativeFn fn)
        :   MethodImpl(fnsign), fn_(fn) {}
    };

    class LangMethodImpl   : public MethodImpl {
    public:
        llvm::Function* fn_ = nullptr;

        LangMethodImpl(const sema::FnSign* fnsign,llvm::Function* fn)
        :   MethodImpl(fnsign), fn_(fn) {}
    };

    class TypeImpl {
    private:
        const sema::Type* link_type_ = nullptr;

    public:
        std::string name_ = "";
        size_t      size_ = 0;
        std::unordered_map<std::string, std::vector<std::unique_ptr<MethodImpl>>> methods_;

        TypeImpl(
            const sema::Type* link_type,
            size_t size
        )
        :   link_type_(link_type),
            name_(link_type->name_),
            size_(size)
        {}

        const sema::Type* link_type() const { return link_type_; } 

        void MethodAdd(const std::string& name, NativeMethodImpl::NativeFn fn, const sema::FnSign& fnsign);
        void MethodAdd(const std::string& name, llvm::Function* fn, const sema::FnSign& fnsign);
        
        std::vector<std::unique_ptr<MethodImpl>>* MethodGet(const std::string& method_name) {
            auto it = methods_.find(method_name);
            if (it == methods_.end()) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined method '{}' on type '{}'",
                    method_name, name_
                ));
            }
            return &it->second;
        }
        std::vector<std::unique_ptr<MethodImpl>>* TryMethodGet(const std::string& method_name) {
            auto it = methods_.find(method_name);
            return it == methods_.end() ? nullptr : &it->second;
        }
    };

    class TypeImplTable {
    private:
        static inline std::unordered_map<const sema::Type*, std::unique_ptr<TypeImpl>> table_;
        static inline std::unordered_map<TypeImpl*, const sema::Type*> table_reverse_;

    public:
        static void      Init();
        static TypeImpl* Set(TypeImpl&& type_impl) {
            auto type    = type_impl.link_type();
            table_[type] = std::make_unique<TypeImpl>(std::move(type_impl));
            auto impl    = table_[type].get();
            table_reverse_[impl] = type;

            return impl;
        }
        
        static TypeImpl*         Lookup(const sema::Type* type) {
            auto it = table_.find(type);
            if (it == table_.end()) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined implement on type '{}'", type->name_
                ));
            }
            return it->second.get();
        }
        static const sema::Type* Lookup(TypeImpl* type_impl) {
            auto it = table_reverse_.find(type_impl);
            if (it == table_reverse_.end()) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined type on implement '{}'", type_impl->name_
                ));
            }
            return table_reverse_.find(type_impl)->second;
        }
    
        static llvm::Value* Cast(
            IRGen& gen, llvm::Value* val,
            const sema::Type* from, const sema::Type* to
        );
    };
}
