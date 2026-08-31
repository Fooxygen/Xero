
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <vector>
#include <unordered_map>

#include "llvm/IR/Value.h"

#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"
#include "xcompiler/defs/fn.hpp"

namespace xcompiler {
    class IRGen;

    class TypeImpl {
    private:
        sema::Type* link_type_ = nullptr;
        std::string name_      = "";
        size_t      size_      = 0;

        // FnSign: Registed in Sema Stage, Linked to the unique implementation
        std::unordered_map<const sema::FnSign*, std::unique_ptr<FnImpl>> methods_;

    public:
        TypeImpl(sema::Type* link_type, size_t size)
        :   link_type_(link_type), name_(link_type->name()), size_(size) {}

        sema::Type* link_type() const { return link_type_; }
        std::string name()      const { return name_; }
        size_t      size()      const { return size_; }

    public:
        void    MethodAdd(const std::string& name, NativeFnImpl::Impl impl, const sema::FnSign& sign);
        void    MethodAdd(const std::string& name, LangFnImpl::Impl   impl, const sema::FnSign& sign);
        
        FnImpl* MethodGet(const sema::FnSign* sign);
        FnImpl* MethodGetTry(const sema::FnSign* sign);
    
        llvm::Value* MethodCall(
            IRGen& gen, const std::string& name,
            const std::vector<llvm::Value*>& args, const std::vector<sema::Type*>& args_type
        );
    };

    class TypeImplTable {
    private:
        static inline std::unordered_map<sema::Type*, std::unique_ptr<TypeImpl>> table_;
        static inline std::unordered_map<TypeImpl*, sema::Type*>                 table_reverse_;

    public:
        static void         Init();
        static TypeImpl*    Set(TypeImpl&& type_impl) {
            auto type    = type_impl.link_type();
            table_[type] = std::make_unique<TypeImpl>(std::move(type_impl));
            auto impl    = table_[type].get();
            table_reverse_[impl] = type;
            return impl;
        }
        
        static TypeImpl*    Lookup(sema::Type* type) {
            auto it = table_.find(type->BasicTypeGet());
            if (it == table_.end()) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined implementation of type '{}'", type->name()
                ));
            }
            return it->second.get();
        }
        static sema::Type*  Lookup(TypeImpl* type_impl) {
            auto it = table_reverse_.find(type_impl);
            if (it == table_reverse_.end()) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined type with implementation '{}'", type_impl->name()
                ));
            }
            return table_reverse_.find(type_impl)->second;
        }
    
        static llvm::Value* Cast(IRGen& gen, llvm::Value* val, sema::Type* from, sema::Type* to);
    };
}
