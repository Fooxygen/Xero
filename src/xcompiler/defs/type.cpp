
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>

#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/gen.hpp"
#include "xcompiler/builtin.hpp"

#include "common/log.hpp"

namespace xcompiler {

    // TypeImpl

    void         TypeImpl::MethodAdd(const std::string& name, NativeFnImpl::Impl impl, const sema::FnSign& sign) {
        auto  def_basic   = (sema::BasicType*)def_;
        auto& method      = def_basic->method_table().Lookup(name);
        auto  method_sign = method.SignLookup(sign);

        auto it = methods_.find(method_sign);
        if (it != methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implementation of method {} for type '{}', signature is {}",
                name, def_basic->name(), sign.ParamsPrint()
            ));
        }
        methods_[method_sign] = std::make_unique<NativeFnImpl>(impl);
    }
    
    void         TypeImpl::MethodAdd(const std::string& name, LangFnImpl::Impl impl, const sema::FnSign& sign) {
        auto  def_basic   = (sema::BasicType*)def_;
        auto& method      = def_basic->method_table().Lookup(name);
        auto  method_sign = method.SignLookup(sign);

        auto it = methods_.find(method_sign);
        if (it != methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implementation of method {} for type '{}', signature is {}",
                name, def_basic->name(), sign.ParamsPrint()
            ));
        }
        methods_[method_sign] = std::make_unique<LangFnImpl>(impl);
    }

    FnImpl*      TypeImpl::MethodLookup(const sema::FnSign* sign) {
        auto key = sign->template_sign();
        auto it  = methods_.find(key);
        if (it == methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "undefined implementation of method {} for type '{}', signature it attempts to obtain is {}",
                sign->name(), def_->name(), sign->ParamsPrint()
            ));
        }
        return it->second.get();
    }

    FnImpl*      TypeImpl::MethodLookupTry(const sema::FnSign* sign) {
        auto it = methods_.find(sign);
        return it == methods_.end() ? nullptr : it->second.get();
    }

    llvm::Value* TypeImpl::MethodCall(IRGen& gen, const std::string& name, const std::vector<Arg>& args) {
        auto  method = sema::TypeTable::MethodLookup(args[0].type(), name);

        std::vector<sema::Type*> args_type = {};
        for (size_t i = 1; i < args.size(); i++) {      // elem[0] is caller
            args_type.emplace_back(args[i].type());
        }
        
        auto  method_sign = method->SignLookup(args[0].type(), args_type);
        auto  method_impl = MethodLookup(method_sign);

        if (auto native = dynamic_cast<NativeFnImpl*>(method_impl)) {
            return native->impl()(gen, args);
        }
        if (auto lang = dynamic_cast<LangFnImpl*>(method_impl)) {
            std::vector<llvm::Value*> vals = {};
            for (auto& arg : args) {
                vals.emplace_back(arg.val());
            }
            return gen.llvm_builder().CreateCall(lang->impl(), vals);
        }

        throw LogErr(LogModule::Xcompiler, std::format(
            "unsupported method '{}'", name
        ));
    }

    // TypeImplTable

    void         TypeImplTable::Init() {
        Init_bool();
        Init_i32();
        Init_i64();
        Init_f32();
        Init_f64();
        Init_char();
        Init_string();
        Init_stringview();
        Init_array();
        Init_arrayview();
        Init_range();
    }

    llvm::Value* TypeImplTable::Cast(IRGen& gen, llvm::Value* val, sema::Type* from, sema::Type* to) {
        if (from == to) return val;

        auto from_basic = (sema::BasicType*)from->BasicTypeGet();
        const sema::FnSign* method_sign = nullptr;

        if (auto from_parametric = dynamic_cast<sema::ParametricType*>(from)) {
            for (auto& [cast_type, sign] : from_basic->casts_fnsign()) {
                auto resolved = sema::TypeTable::BindingTypeReplace(cast_type, from_basic, from_parametric->params());
                if (resolved == to) {
                    method_sign = sign;
                    break;
                }
            }
        }
        else {
            auto it = from_basic->casts_fnsign().find(to);
            if (it != from_basic->casts_fnsign().end()) method_sign = it->second;
        }

        if (!method_sign) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "cannot cast type from '{}' to '{}'", from->name(), to->name()
            ));
        }

        auto from_impl   = TypeImplTable::Lookup(from_basic);
        auto method_impl = from_impl->MethodLookup(method_sign);
        auto caller     = gen.ArgRefMake(val, from);

        if (auto native = dynamic_cast<NativeFnImpl*>(method_impl)) {
            return native->impl()(gen, { caller });
        }
        if (auto lang = dynamic_cast<LangFnImpl*>(method_impl)) {
            return gen.llvm_builder().CreateCall(lang->impl(), { caller.val() });
        }

        std::unreachable();
    }
}
