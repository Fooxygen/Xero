
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>

#include "xcompiler/builtin/builtin.hpp"
#include "xcompiler/ir/ir.hpp"
#include "xcompiler/ir/type.hpp"

#include "common/log.hpp"

namespace xcompiler {

    // TypeImpl

    void TypeImpl::MethodAdd(const std::string& name, NativeMethodImpl::Impl impl, const sema::FnSign& sign) {
        auto  type        = (sema::BasicType*)link_type_;
        auto& method      = type->methods().Lookup(name);
        auto  method_sign = method.SignLookup(sign);

        auto it = methods_.find(method_sign);
        if (it != methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implement of method {} for type '{}'",
                sign.ParamsPrint(), type->name()
            ));
        }
        methods_[method_sign] = std::make_unique<NativeMethodImpl>(impl);
    }
    
    void TypeImpl::MethodAdd(const std::string& name, LangMethodImpl::Impl impl, const sema::FnSign& sign) {
        auto  type        = (sema::BasicType*)link_type_;
        auto& method      = type->methods().Lookup(name);
        auto  method_sign = method.SignLookup(sign);

        auto it = methods_.find(method_sign);
        if (it != methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implement of method {} for type '{}'",
                sign.ParamsPrint(), type->name()
            ));
        }
        methods_[method_sign] = std::make_unique<LangMethodImpl>(impl);
    }

    MethodImpl* TypeImpl::MethodGet(const sema::FnSign* sign) {
        auto it = methods_.find(sign);
        if (it == methods_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "undefined implement of method {} for type '{}'",
                sign->ParamsPrint(), link_type_->name()
            ));
        }
        return it->second.get();
    }

    MethodImpl* TypeImpl::MethodGetTry(const sema::FnSign* sign) {
        auto it = methods_.find(sign);
        return it == methods_.end() ? nullptr : it->second.get();
    }

    // TypeImplTable

    void TypeImplTable::Init() {

        auto none_ = sema::TypeTable::Lookup("none");
        auto bool_ = sema::TypeTable::Lookup("bool");
        auto i32_  = sema::TypeTable::Lookup("i32");
        auto i64_  = sema::TypeTable::Lookup("i64");
        auto f32_  = sema::TypeTable::Lookup("f32");
        auto f64_  = sema::TypeTable::Lookup("f64");

        // Impl
        {
            // bool
            {
                auto impl = TypeImplTable::Set(TypeImpl(bool_, 1));
                
                impl->MethodAdd("print", [](IRGen& gen, const std::vector<llvm::Value*>& args) -> llvm::Value* {
                    auto& builder   = gen.builder();
                    auto  str_fmt   = builder.CreateGlobalString("%s", ".fmt.bool");
                    auto  str_true  = builder.CreateGlobalString("true", ".true");
                    auto  str_false = builder.CreateGlobalString("false", ".false");
                    auto  str       = builder.CreateSelect(args[0], str_true, str_false);
                    builder.CreateCall(LibC_printf(gen), { str_fmt, str });
                    return nullptr;
                }, sema::FnSign(none_, { bool_ }));
                impl->MethodAdd("eq",   [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("neq",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("and",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateAnd(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("or",   [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateOr(args[0], args[1]);
                }, sema::FnSign(bool_, { bool_, bool_ }));
                impl->MethodAdd("not",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateNot(args[0]);
                }, sema::FnSign(bool_, { bool_ }));
            }

            // i32
            {
                auto impl = TypeImplTable::Set(TypeImpl(i32_, 4));

                impl->MethodAdd("print", [](IRGen& gen, const std::vector<llvm::Value*>& args) -> llvm::Value* {
                    auto& builder = gen.builder();
                    auto  str_fmt = builder.CreateGlobalString("%d", ".fmt.i32");
                    builder.CreateCall(LibC_printf(gen), { str_fmt, args[0] });
                    return nullptr;
                }, sema::FnSign(none_, { i32_ }));
                impl->MethodAdd("plus",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateAdd(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("minus", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSub(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("star",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateMul(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("slash", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSDiv(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("neg",   [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateNeg(args[0]);
                }, sema::FnSign(i32_,  { i32_ }));
                impl->MethodAdd("modt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSRem(args[0], args[1]);
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("modf",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    auto& builder = gen.builder();
                    auto  zero    = builder.getInt32(0);
                    auto  modt    = builder.CreateSRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateICmpNE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateICmpSLT(modt, zero), builder.CreateICmpSLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(i32_,  { i32_, i32_ }));
                impl->MethodAdd("gt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSGT(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("lt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSLT(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("ge",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSGE(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("le",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSLE(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("eq",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("neq", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { i32_, i32_ }));
                impl->MethodAdd("cast",[](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSExt(args[0], gen.builder().getInt64Ty());
                }, sema::FnSign(i64_, { i32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
                impl->MethodAdd("cast", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSIToFP(args[0], gen.builder().getFloatTy());
                }, sema::FnSign(f32_, { i32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
                impl->MethodAdd("cast", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSIToFP(args[0], gen.builder().getDoubleTy());
                }, sema::FnSign(f64_, { i32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
            }

            // i64
            {
                auto impl = TypeImplTable::Set(TypeImpl(i64_, 8));

                impl->MethodAdd("print", [](IRGen& gen, const std::vector<llvm::Value*>& args) -> llvm::Value* {
                    auto& builder = gen.builder();
                    auto  str_fmt = builder.CreateGlobalString("%lld", ".fmt.i64");
                    builder.CreateCall(LibC_printf(gen), { str_fmt, args[0] });
                    return nullptr;
                }, sema::FnSign(none_, { i64_ }));
                impl->MethodAdd("plus",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateAdd(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("minus", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSub(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("star",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateMul(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("slash", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSDiv(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("neg",   [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateNeg(args[0]);
                }, sema::FnSign(i64_,  { i64_ }));
                impl->MethodAdd("modt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSRem(args[0], args[1]);
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("modf",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    auto& builder = gen.builder();
                    auto  zero    = builder.getInt64(0);
                    auto  modt    = builder.CreateSRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateICmpNE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateICmpSLT(modt, zero), builder.CreateICmpSLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(i64_,  { i64_, i64_ }));
                impl->MethodAdd("gt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSGT(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("lt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSLT(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("ge",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSGE(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("le",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpSLE(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("eq",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("neq", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateICmpNE(args[0], args[1]);
                }, sema::FnSign(bool_, { i64_, i64_ }));
                impl->MethodAdd("cast", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSIToFP(args[0], gen.builder().getFloatTy());
                }, sema::FnSign(f32_, { i64_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
                impl->MethodAdd("cast", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateSIToFP(args[0], gen.builder().getDoubleTy());
                }, sema::FnSign(f64_, { i64_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
            }

            // f32
            {
                auto impl = TypeImplTable::Set(TypeImpl(f32_, 4));

                impl->MethodAdd("print", [](IRGen& gen, const std::vector<llvm::Value*>& args) -> llvm::Value* {
                    auto& builder = gen.builder();
                    auto  str_fmt = builder.CreateGlobalString("%f", ".fmt.f32");
                    builder.CreateCall(LibC_printf(gen), {
                        str_fmt,
                        builder.CreateFPExt(args[0], builder.getDoubleTy())
                    });
                    return nullptr;
                }, sema::FnSign(none_, { f32_ }));
                impl->MethodAdd("plus",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFAdd(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("minus", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFSub(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("star",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFMul(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("slash", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFDiv(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("neg",   [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFNeg(args[0]);
                }, sema::FnSign(f32_,  { f32_ }));
                impl->MethodAdd("modt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFRem(args[0], args[1]);
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("modf",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    auto& builder = gen.builder();
                    auto  zero    = llvm::ConstantFP::get(builder.getFloatTy(), 0.0);
                    auto  modt    = builder.CreateFRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateFCmpOLT(modt, zero), builder.CreateFCmpOLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateFAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(f32_,  { f32_, f32_ }));
                impl->MethodAdd("gt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOGT(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("lt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOLT(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("ge",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOGE(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("le",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOLE(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("eq",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("neq", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpONE(args[0], args[1]);
                }, sema::FnSign(bool_, { f32_, f32_ }));
                impl->MethodAdd("cast", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFPExt(args[0], gen.builder().getDoubleTy());
                }, sema::FnSign(f64_, { f32_ }, std::nullopt, sema::FnModifier{ .hasCast_ = true }));
            }

            // f64
            {
                auto impl = TypeImplTable::Set(TypeImpl(f64_, 8));

                impl->MethodAdd("print", [](IRGen& gen, const std::vector<llvm::Value*>& args) -> llvm::Value* {
                    auto& builder = gen.builder();
                    auto  str_fmt = builder.CreateGlobalString("%f", ".fmt.f64");
                    builder.CreateCall(LibC_printf(gen), { str_fmt, args[0] });
                    return nullptr;
                }, sema::FnSign(none_, { f64_ }));
                impl->MethodAdd("plus",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFAdd(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("minus", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFSub(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("star",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFMul(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("slash", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFDiv(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("neg",   [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFNeg(args[0]);
                }, sema::FnSign(f64_,  { f64_ }));
                impl->MethodAdd("modt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFRem(args[0], args[1]);
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("modf",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    auto& builder = gen.builder();
                    auto  zero    = llvm::ConstantFP::get(builder.getDoubleTy(), 0.0);
                    auto  modt    = builder.CreateFRem(args[0], args[1]);
                    auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);
                    auto  isDiffSign    = builder.CreateXor(builder.CreateFCmpOLT(modt, zero), builder.CreateFCmpOLT(args[1], zero));
                    auto  hasRevise     = builder.CreateAnd(isModTNeqZero, isDiffSign);
                    return builder.CreateFAdd(modt, builder.CreateSelect(hasRevise, args[1], zero));
                }, sema::FnSign(f64_,  { f64_, f64_ }));
                impl->MethodAdd("gt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOGT(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("lt",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOLT(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("ge",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOGE(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("le",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOLE(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("eq",  [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpOEQ(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
                impl->MethodAdd("neq", [](IRGen& gen, const std::vector<llvm::Value*>& args) {
                    return gen.builder().CreateFCmpONE(args[0], args[1]);
                }, sema::FnSign(bool_, { f64_, f64_ }));
            }
        }
    }

    llvm::Value* TypeImplTable::Cast(IRGen& gen, llvm::Value* val, sema::Type* from, sema::Type* to) {
        if (from == to) return val;

        auto  from_basic = (sema::BasicType*)from->BasicTypeGet();
        auto& methods    = from_basic->methods();

        for (auto& [name, method] : methods.table()) {
            for (auto& s : method.signs()) {
                if (s->modifier().hasCast_ && s->ret_type() == to) {
                    auto impl = TypeImplTable::Lookup(from)->MethodGet(s.get());
                    return ((NativeMethodImpl*)impl)->impl_(gen, { val });
                }
            }
        }
        
        throw LogErr(LogModule::Xcompiler, std::format(
            "cannot cast type from '{}' to '{}'", from->name(), to->name()
        ));
    }
}
