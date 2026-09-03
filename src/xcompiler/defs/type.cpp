
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>

#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/ir.hpp"
#include "xcompiler/builtin.hpp"

#include "common/log.hpp"

namespace xcompiler {

    // TypeImpl

    void    TypeImpl::MethodAdd(const std::string& name, NativeFnImpl::Impl impl, const sema::FnSign& sign) {
        auto  type_basic  = (sema::BasicType*)link_type_;
        auto& method      = type_basic->method_table().Lookup(name);
        auto  method_sign = method.SignLookup(sign);

        auto it = methods_.find(method_sign);
        if (it != methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implementation of method {} for type '{}', signature is {}",
                name, type_basic->name(), sign.ParamsPrint()
            ));
        }
        methods_[method_sign] = std::make_unique<NativeFnImpl>(impl);
    }
    
    void    TypeImpl::MethodAdd(const std::string& name, LangFnImpl::Impl impl, const sema::FnSign& sign) {
        auto  type_basic  = (sema::BasicType*)link_type_;
        auto& method      = type_basic->method_table().Lookup(name);
        auto  method_sign = method.SignLookup(sign);

        auto it = methods_.find(method_sign);
        if (it != methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implementation of method {} for type '{}', signature is {}",
                name, type_basic->name(), sign.ParamsPrint()
            ));
        }
        methods_[method_sign] = std::make_unique<LangFnImpl>(impl);
    }

    FnImpl* TypeImpl::MethodGet(const sema::FnSign* sign) {
        auto it = methods_.find(sign);
        if (it == methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "undefined implementation of method {} for type '{}', signature it attempts to obtain is {}",
                sign->name(), link_type_->name(), sign->ParamsPrint()
            ));
        }
        return it->second.get();
    }

    FnImpl* TypeImpl::MethodGetTry(const sema::FnSign* sign) {
        auto it = methods_.find(sign);
        return it == methods_.end() ? nullptr : it->second.get();
    }

    llvm::Value* TypeImpl::MethodCall(
        IRGen& gen, const std::string& name,
        const std::vector<llvm::Value*>& args, const std::vector<sema::Type*>& args_type)
    {
        auto  type_basic  = (sema::BasicType*)link_type_->BasicTypeGet();
        auto& method      = type_basic->method_table().Lookup(name);
        auto  method_sign = method.SignLookup(args_type);
        auto  method_impl = MethodGet(method_sign);

        if (auto native = dynamic_cast<NativeFnImpl*>(method_impl)) {
            return native->impl()(gen, args, args_type);
        }
        if (auto lang = dynamic_cast<LangFnImpl*>(method_impl)) {
            return gen.llvm_builder().CreateCall(lang->impl(), args);
        }

        throw LogErr(LogModule::Xcompiler, std::format(
            "unsupported method '{}'", name
        ));
    }

    llvm::Value* TypeImplTable::Cast(IRGen& gen, llvm::Value* val, sema::Type* from, sema::Type* to) {
        if (from == to) return val;

        auto from_basic  = (sema::BasicType*)from->BasicTypeGet();
        auto method_sign = from_basic->casts_fnsign().find(to);
        if (method_sign == from_basic->casts_fnsign().end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "cannot cast type from '{}' to '{}'", from->name(), to->name()
            ));
        }

        auto from_impl   = TypeImplTable::Lookup(from_basic);
        auto method_impl = from_impl->MethodGet(method_sign->second);

        if (auto native = dynamic_cast<NativeFnImpl*>(method_impl)) {
            return native->impl()(gen, { val }, { from });
        }
        if (auto lang = dynamic_cast<LangFnImpl*>(method_impl)) {
            return gen.llvm_builder().CreateCall(lang->impl(), { val });
        }

        std::unreachable();
    }
}
