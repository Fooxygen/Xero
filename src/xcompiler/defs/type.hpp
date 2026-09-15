
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

        // FnSign: Registed in Sema Stage, Linked to the unique implementation
        std::unordered_map<const sema::FnSign*, std::unique_ptr<FnImpl>> methods_;

    public:
        TypeImpl(sema::Type* link_type)
        :   link_type_(link_type), name_(link_type->name()) {}

        sema::Type*        link_type() const { return link_type_; }
        const std::string& name()      const { return name_; }

    public:
        void    MethodAdd(const std::string& name, NativeFnImpl::Impl impl, const sema::FnSign& sign);
        void    MethodAdd(const std::string& name, LangFnImpl::Impl   impl, const sema::FnSign& sign);
        
        FnImpl* MethodGet(const sema::FnSign* sign);
        FnImpl* MethodGetTry(const sema::FnSign* sign);
    
        llvm::Value* MethodCall(IRGen& gen, const std::string& name, const std::vector<Arg>& args);
    };

    class TypeImplTable {
    private:
        static inline std::unordered_map<sema::Type*, std::unique_ptr<TypeImpl>> table_;
        static inline std::unordered_map<TypeImpl*, sema::Type*>                 table_reverse_;

    private:
        // Init
        
        static void Init_bool();
        static void Init_i32();
        static void Init_i64();
        static void Init_f32();
        static void Init_f64();
        static void Init_char();
        static void Init_array();
        static void Init_range();

    public:
        // Init

        static void         Init();
        static TypeImpl*    Set(TypeImpl&& type_impl) {
            auto type = type_impl.link_type();
            auto impl = table_.emplace(
                type, std::make_unique<TypeImpl>(std::move(type_impl))
            ).first->second.get();
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
            return it->second;
        }
    
        static llvm::Value* Cast(IRGen& gen, llvm::Value* val, sema::Type* from, sema::Type* to);
    };
}
