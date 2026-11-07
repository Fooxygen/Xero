
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <optional>

#include "llvm/IR/Function.h"
#include "llvm/IR/Intrinsics.h"

#include "sema/defs/fn.hpp"
#include "sema/defs/type.hpp"
#include "xcompiler/builtin.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {
    
    // LibC

    llvm::Function* LibCLookup(IRGen& gen, const std::string& name, llvm::FunctionType* fntype) {
        auto module = gen.llvm_module();
        if (auto fn = module->getFunction(name)) return fn;

        return llvm::Function::Create(fntype, llvm::Function::ExternalLinkage, name, module);
    }
    
    llvm::Function* LibC_printf(IRGen& gen) {
        auto context = &gen.llvm_module()->getContext();
        return LibCLookup(gen, "printf", llvm::FunctionType::get(
            llvm::Type::getInt32Ty(*context),
            { llvm::PointerType::getUnqual(*context) },
            true    // variable parameters
        ));
    }

    llvm::Function* LibC_malloc(IRGen& gen) {
        auto context = &gen.llvm_module()->getContext();
        return LibCLookup(gen, "malloc", llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            { llvm::Type::getInt64Ty(*context) },
            false
        ));
    }

    llvm::Function* LibC_realloc(IRGen& gen) {
        auto context = &gen.llvm_module()->getContext();
        return LibCLookup(gen, "realloc", llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            {
                llvm::PointerType::getUnqual(*context),     // ptr
                llvm::Type::getInt64Ty(*context)            // size
            },
            false
        ));
    }

    llvm::Function* LibC_free(IRGen& gen) {
        auto context = &gen.llvm_module()->getContext();
        return LibCLookup(gen, "free", llvm::FunctionType::get(
            llvm::Type::getVoidTy(*context),
            { llvm::PointerType::getUnqual(*context) },
            false
        ));
    }

    llvm::Function* LibC_memmove(IRGen& gen) {
        auto context = &gen.llvm_module()->getContext();
        return LibCLookup(gen, "memmove", llvm::FunctionType::get(
            llvm::PointerType::getUnqual(*context),
            { llvm::PointerType::getUnqual(*context),   // dst
            llvm::PointerType::getUnqual(*context),     // src
            llvm::Type::getInt64Ty(*context) },         // cnt
            false
        ));
    }

    // Built-in Function

    void BuiltinFnRegister(sema::FnScope& fn_scope) {
        auto none_ = sema::TypeTable::Lookup("none");

        auto Register = [&](sema::FnSign sign_reference, NativeFnImpl::Impl impl) {
            auto sign = fn_scope.Lookup(sign_reference.name())->SignLookup(sign_reference);
            FnImplTable::Add(sign_reference.name(), sign, std::make_unique<NativeFnImpl>(impl));
        };

        // print and println
        {
            auto Impl_print_one = [](IRGen& gen, const Arg& arg) {
                auto caller      = gen.ArgRefMake(arg.val(), arg.type());  // x -> &x, &x -> &x
                auto type        = arg.type();
                auto method      = ((sema::BasicType*)type->BasicTypeGet())->method_table().Lookup("@print");
                auto method_sign = method->SignLookup(caller.type(), {});
                auto method_impl = TypeImplTable::Lookup(type)->MethodLookup(method_sign);
                ((NativeFnImpl*)method_impl)->impl()(gen, { caller });
            };
            auto Impl_print     = [Impl_print_one](IRGen& gen, const std::vector<Arg>& args) -> llvm::Value* {
                auto& builder = gen.llvm_builder();
                for (size_t i = 0; i < args.size(); i++) {
                    if (i != 0) builder.CreateCall(LibC_printf(gen), {
                        builder.CreateGlobalString(" ", ".delim")
                    });
                    Impl_print_one(gen, args[i]);
                }

                return nullptr;
            };
            auto Impl_println   = [Impl_print](IRGen& gen, const std::vector<Arg>& args) -> llvm::Value* {
                auto& builder = gen.llvm_builder();
                Impl_print(gen, args);

                // Line Break
                builder.CreateCall(LibC_printf(gen), {
                    builder.CreateGlobalString("\n", ".nl")
                });

                return nullptr;
            };

            Register(sema::FnSign(none_, {}, nullptr, {}, "print"),   Impl_print);
            Register(sema::FnSign(none_, {}, nullptr, {}, "println"), Impl_println);
        }

        // Math
        {
            auto f32_ = sema::TypeTable::Lookup("f32");
            auto f64_ = sema::TypeTable::Lookup("f64");

            auto Impl_sqrt = [](IRGen& gen, const std::vector<Arg>& args) -> llvm::Value* {
                auto type = gen.LlvmType(args[0].type());
                auto fn   = llvm::Intrinsic::getOrInsertDeclaration(
                    gen.llvm_module(), llvm::Intrinsic::sqrt, { type }
                );
                return gen.llvm_builder().CreateCall(fn, {
                    gen.ArgValGet(args[0])
                });
            };

            auto Impl_libm_unary = [](const std::string& name) -> NativeFnImpl::Impl {
                return [name](IRGen& gen, const std::vector<Arg>& args) -> llvm::Value* {
                    auto type   = gen.LlvmType(args[0].type());
                    auto fntype = llvm::FunctionType::get(type, { type }, false);
                    auto fn     = LibCLookup(gen, name, fntype);
                    return gen.llvm_builder().CreateCall(fn, {
                        gen.ArgValGet(args[0])
                    });
                };
            };

            auto Impl_libm_binary = [](const std::string& name) -> NativeFnImpl::Impl {
                return [name](IRGen& gen, const std::vector<Arg>& args) -> llvm::Value* {
                    auto type   = gen.LlvmType(args[0].type());
                    auto fntype = llvm::FunctionType::get(type, { type, type }, false);
                    auto fn     = LibCLookup(gen, name, fntype);
                    return gen.llvm_builder().CreateCall(fn, {
                        gen.ArgValGet(args[0]),
                        gen.ArgValGet(args[1])
                    });
                };
            };

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@sqrt"),     Impl_sqrt);
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@sqrt"),     Impl_sqrt);

            Register(sema::FnSign(f32_, { f32_, f32_ }, std::nullopt, {}, "@pow"),      Impl_libm_binary("powf"));
            Register(sema::FnSign(f64_, { f64_, f64_ }, std::nullopt, {}, "@pow"),      Impl_libm_binary("pow"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@exp"),      Impl_libm_unary("expf"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@exp"),      Impl_libm_unary("exp"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@ln"),       Impl_libm_unary("logf"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@ln"),       Impl_libm_unary("log"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@log2"),     Impl_libm_unary("log2f"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@log2"),     Impl_libm_unary("log2"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@log10"),    Impl_libm_unary("log10f"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@log10"),    Impl_libm_unary("log10"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@sin"),      Impl_libm_unary("sinf"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@sin"),      Impl_libm_unary("sin"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@cos"),      Impl_libm_unary("cosf"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@cos"),      Impl_libm_unary("cos"));

            Register(sema::FnSign(f32_, { f32_ },       std::nullopt, {}, "@tan"),      Impl_libm_unary("tanf"));
            Register(sema::FnSign(f64_, { f64_ },       std::nullopt, {}, "@tan"),      Impl_libm_unary("tan"));
        }
    }
}
