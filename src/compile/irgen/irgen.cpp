
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>
#include <vector>

#include "common/log.hpp"
#include "compile/irgen/irgen.hpp"

namespace compile {

    IRGen::IRGen(std::string_view module_name, std::string path, AstNode& root)
    :   context_(),
        module_(std::make_unique<llvm::Module>(module_name, context_)),
        builder_(context_)
    {
        // Open File
        if (path.empty()) {
            throw LogErr(LogModule::File, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Compile, std::format(
                "failed to open file '{}'", path
            ));
        }

        // Exec
        Exec(root);

        // Print
        module_->print(file, nullptr);
        file.flush();
    }

    llvm::Type* IRGen::LlvmType(const sema::Type* type) {
        if (type->is("none"))   return llvm::Type::getVoidTy(context_);
        if (type->is("bool"))   return llvm::Type::getInt1Ty(context_);
        if (type->is("i32"))    return llvm::Type::getInt32Ty(context_);
        if (type->is("i64"))    return llvm::Type::getInt64Ty(context_);
        if (type->is("f32"))    return llvm::Type::getFloatTy(context_);
        if (type->is("f64"))    return llvm::Type::getDoubleTy(context_);
        if (type->is("char"))   return llvm::Type::getInt32Ty(context_);

        throw LogErr(LogModule::Compile, std::format(
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
        value_table_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        for (auto& child : node.children_) Exec(*child);

        value_table_.ScopePop();
        return nullptr;
    }
    
    llvm::Value* IRGen::Exec(IdExpr& node) {
        auto slot = value_table_.Lookup(node.value_);
        auto type = LlvmType(node.resolved_type_);
        return builder_.CreateLoad(type, slot, node.value_);
    }

    llvm::Value* IRGen::Exec(DeclExpr& node) {

        // Slot
        auto type = LlvmType(node.resolved_type_);
        auto slot = EntryBlockSlotCreate(type, node.id_);

        // Value
        if (node.value_) {
            builder_.CreateStore(Exec(*node.value_), slot);
        }
        
        value_table_.Declare(node.id_, slot);
        return nullptr;
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
            for (auto& arg : fn->args()) {
                auto name = arg.getName().str();
                auto slot = EntryBlockSlotCreate(arg.getType(), name);
                builder_.CreateStore(&arg, slot);
                value_table_.Declare(name, slot);
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

    // Stmt

    llvm::Value* IRGen::Exec(ExprStmt& node) {
        if (node.expr_) return Exec(*node.expr_);
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
