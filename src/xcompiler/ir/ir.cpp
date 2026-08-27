
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>
#include <vector>

#include "llvm/TargetParser/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/TargetParser/SubtargetFeature.h"
#include "llvm/IR/LegacyPassManager.h"

#include "common/log.hpp"
#include "common/opertype.hpp"
#include "xcompiler/builtin/builtin.hpp"
#include "xcompiler/ir/type.hpp"
#include "xcompiler/ir/ir.hpp"

namespace xcompiler {

    IRGen::IRGen(std::string_view module_name)
    :   context_(),
        module_(std::make_unique<llvm::Module>(module_name, context_)),
        builder_(context_)
    {}

    // Output

    void IRGen::IROutput(const std::string& path) {

        // Open File
        if (path.empty()) {
            throw LogErr(LogModule::File, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to open file '{}'", path
            ));
        }

        module_->print(file, nullptr);
        file.flush();
    }

    void IRGen::ObjectCodeOutput(const std::string& path) {

        // Configure
        
        llvm::InitializeAllTargetInfos();
        llvm::InitializeAllTargets();
        llvm::InitializeAllTargetMCs();
        llvm::InitializeAllAsmPrinters();

        // └─ Target Triple
        llvm::Triple target_triple(
            llvm::sys::getDefaultTargetTriple()
        );
        
        // └─ Target
        std::string target_err = "";
        auto target = llvm::TargetRegistry::lookupTarget(target_triple, target_err);
        if (!target) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to lookup target: {}", target_err
            ));
        }

        // └─ Target Machine
        auto cpu            = llvm::sys::getHostCPUName();
        auto features       = llvm::SubtargetFeatures(); {
            for (const auto& feature : llvm::sys::getHostCPUFeatures()) {
                features.AddFeature(feature.first(), feature.second);
            }
        }
        auto target_machine = std::unique_ptr<llvm::TargetMachine>(
            target->createTargetMachine(
                target_triple, cpu, features.getString(),
                llvm::TargetOptions{}, llvm::Reloc::PIC_
            )
        );

        // └─ Module Binding
        module_->setDataLayout(target_machine->createDataLayout());
        module_->setTargetTriple(target_triple);

        // └─ Open File
        if (path.empty()) {
            throw LogErr(LogModule::File, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to open file '{}'", path
            ));
        }

        // Execute

        // └─ Pass
        llvm::legacy::PassManager pass;
        if (target_machine->addPassesToEmitFile(
            pass, file, nullptr, llvm::CodeGenFileType::ObjectFile
        ))
        {
            throw LogErr(LogModule::Xcompiler, "failed to generate object code");
        }

        pass.run(*module_);
        file.flush();
    }

    // Utility

    llvm::Type* IRGen::LlvmType(const sema::Type* type) {
        if (type->is("none"))   return llvm::Type::getVoidTy(context_);
        if (type->is("bool"))   return llvm::Type::getInt1Ty(context_);
        if (type->is("i32"))    return llvm::Type::getInt32Ty(context_);
        if (type->is("i64"))    return llvm::Type::getInt64Ty(context_);
        if (type->is("f32"))    return llvm::Type::getFloatTy(context_);
        if (type->is("f64"))    return llvm::Type::getDoubleTy(context_);
        if (type->is("char"))   return llvm::Type::getInt32Ty(context_);

        throw LogErr(LogModule::Xcompiler, std::format(
            "undefined type '{}'", type->name_
        ));
    }

    llvm::AllocaInst* IRGen::EntryBlockSlotCreate(llvm::Type* type, const std::string& name) {
        llvm::IRBuilder<> builder_alloc(
            &current_fn_->getEntryBlock(), current_fn_->getEntryBlock().begin()
        );
        return builder_alloc.CreateAlloca(type, nullptr, name);
    }

    // Expr

    llvm::Value* IRGen::Exec(BlockExpr& node, std::function<void()> OnScopeReady) {
        var_table_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        try {
            for (auto& child : node.children_) Exec(*child);
        }
        catch (...) {
            var_table_.ScopePop();
            throw;
        }

        var_table_.ScopePop();
        return nullptr;
    }
    
    llvm::Value* IRGen::Exec(IdExpr& node) {
        auto var  = var_table_.Lookup(node.name_);
        auto type = LlvmType(node.resolved_type_);
        return builder_.CreateLoad(type, var, node.name_);
    }

    llvm::Value* IRGen::Exec(DeclExpr& node) {

        // Variable
        auto var_type = node.resolved_type_;
        auto var_slot = EntryBlockSlotCreate(LlvmType(var_type), node.id_);

        // Value
        if (node.value_) {
            auto val      = Exec(*node.value_);
            auto val_type = node.value_->resolved_type_;
            builder_.CreateStore(
                TypeImplTable::Cast(*this, val, val_type, var_type),
                var_slot
            );
        }
        
        var_table_.Declare(node.id_, var_slot);
        return nullptr;
    }

    llvm::Value* IRGen::Exec(OperExpr& node) {
        using enum OperType;

        auto call = [](IRGen& gen, const sema::Type* type, const std::string& name, std::vector<llvm::Value*> args) {
            auto impl   = TypeImplTable::Lookup(type);
            auto method = impl->TryMethodGet(name);

            return ((NativeMethodImpl*)(method->at(0).get()))->fn_(gen, args);
        };

        // Unary
        auto lval = Exec(*node.lexpr_);
        {
            switch (node.oper_type_) {
                case Neg:   return call(*this, node.lexpr_->resolved_type_, "neg", { lval });
                case Not:   return call(*this, node.lexpr_->resolved_type_, "not", { lval });
                default:    break;
            }
        }

        // Binary

        // Short Circuit Boolean Evaluation
        // TODO

        auto rval = Exec(*node.rexpr_);
        {
            auto ltype = node.lexpr_->resolved_type_;
            auto rtype = node.rexpr_->resolved_type_;
            auto common_type = sema::TypeTable::Common({ ltype, rtype });
            if (!common_type) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    ltype->name_, rtype->name_
                ), node.loc_);
            }

            lval = TypeImplTable::Cast(*this, lval, ltype, common_type);
            rval = TypeImplTable::Cast(*this, rval, rtype, common_type);

            switch (node.oper_type_) {
                case Plus:  return call(*this, common_type, "plus",  { lval, rval });
                case Minus: return call(*this, common_type, "minus", { lval, rval });
                case Star:  return call(*this, common_type, "star",  { lval, rval });
                case Slash: return call(*this, common_type, "slash", { lval, rval });
                case ModT:  return call(*this, common_type, "modt",  { lval, rval });
                case ModF:  return call(*this, common_type, "modf",  { lval, rval });
                case Gt:    return call(*this, common_type, "gt",    { lval, rval });
                case Lt:    return call(*this, common_type, "lt",    { lval, rval });
                case Ge:    return call(*this, common_type, "ge",    { lval, rval });
                case Le:    return call(*this, common_type, "le",    { lval, rval });
                case Eq:    return call(*this, common_type, "eq",    { lval, rval });
                case Neq:   return call(*this, common_type, "neq",   { lval, rval });
                default:    return nullptr;
            }
        }
    }
    
    llvm::Value* IRGen::Exec(FnCallExpr& node) {
        auto callee = node.callee_->name_;

        // Builtin Fn
        // Stored in BuiltinFnTable
        // Execute the predefined code
        {
            if (auto fn = BuiltinFnTable::LookupTry(callee)) {
                return (*fn)(*this, node);
            }
        }

        // Custom Fn
        // Stored in LLVM Module
        // Generate 'Call' Instruction in IR
        {
            auto fn = module_->getFunction(callee);
            if (!fn) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "undefined function '{}'", callee
                ), node.loc_);
            }

            // Args
            std::vector<llvm::Value*> args = {};
            if (node.args_) {
                for (auto& e : node.args_->exprs_) args.emplace_back(Exec(*e));
            }

            return builder_.CreateCall(fn, args);
        }
    }
    
    llvm::Value* IRGen::Exec(FnExpr& node) {

        // Return Type
        auto ret_type = LlvmType(node.ret_resolved_type_);
        if (node.name_ == "main") {
            ret_type = llvm::Type::getInt32Ty(context_);
        }

        // Params Type
        std::vector<llvm::Type*> params_type = {};
        if (node.params_) {
            for (auto& e : node.params_->exprs_) {
                auto param = (DeclExpr*)(e.get());
                params_type.emplace_back(LlvmType(param->resolved_type_));
            }
        }

        // Function
        auto fntype = llvm::FunctionType::get(
            ret_type, params_type, false        // No variable parameters
        );
        auto fn = llvm::Function::Create(
            fntype,
            llvm::Function::ExternalLinkage,
            node.name_,
            module_.get()
        );
        auto block = llvm::BasicBlock::Create(context_, "entry", fn);
        builder_.SetInsertPoint(block);
        current_fn_ = fn;

        // Args Name
        {
            size_t i = 0;
            for (auto& arg : fn->args()) {
                auto p = (DeclExpr*)(node.params_->exprs_[i].get());
                arg.setName(p->id_);
                i++;
            }
        }

        // Block
        Exec(*node.block_, [&]() {
            // Args
            for (auto& arg : fn->args()) {
                auto var_name = arg.getName().str();
                auto var_slot = EntryBlockSlotCreate(arg.getType(), var_name);
                builder_.CreateStore(&arg, var_slot);
                var_table_.Declare(var_name, var_slot);
            }
        });

        // Return Stmt
        // Each block requires a terminal symbol
        // including return value, the unreachable stmt...
        if (!builder_.GetInsertBlock()->getTerminator()) {
            if (ret_type->isVoidTy()) builder_.CreateRetVoid();
            else builder_.CreateRet(llvm::ConstantInt::get(ret_type, 0));
        }

        return nullptr;
    }

    // Const

    llvm::Value* IRGen::Exec(NumConst& node) {
        auto type = node.resolved_type_;
        if (type->is("i32")) return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), node.resolved_value_.integer_);
        if (type->is("i64")) return llvm::ConstantInt::get(llvm::Type::getInt64Ty(context_), node.resolved_value_.integer_);
        if (type->is("f32")) return llvm::ConstantFP::get(llvm::Type::getFloatTy(context_), node.resolved_value_.floating_);
        if (type->is("f64")) return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context_), node.resolved_value_.floating_);
        std::unreachable();
    }

    llvm::Value* IRGen::Exec(BoolConst& node) {
        return node.value_
            ? llvm::ConstantInt::getTrue(context_)
            : llvm::ConstantInt::getFalse(context_);
    }

    // Stmt

    llvm::Value* IRGen::Exec(ExprStmt& node) {
        if (node.expr_) return Exec(*node.expr_);
        return nullptr;
    }

    llvm::Value* IRGen::Exec(AssignStmt& node) {
        auto target   = (IdExpr*)(node.target_.get());
        auto var      = var_table_.Lookup(target->name_);
        auto var_type = target->resolved_type_;
        auto val      = Exec(*node.value_);
        auto val_type = node.value_->resolved_type_;

        builder_.CreateStore(
            TypeImplTable::Cast(*this, val, val_type, var_type),
            var
        );
        return nullptr;
    }

    llvm::Value* IRGen::Exec(ReturnSignalStmt& node) {
        if (node.value_) {
            builder_.CreateRet(Exec(*node.value_));
        }
        else {
            builder_.CreateRetVoid();
        }
        return nullptr;
    }

    // Common

    llvm::Value* IRGen::Exec(Program& node) {
        return Exec((BlockExpr&)node);
    }
}
