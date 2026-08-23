
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>
#include <vector>

#include "common/log.hpp"
#include "compile/irgen.hpp"

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

    // Expr

    llvm::Value* IRGen::Exec(BlockExpr& node) {
        for (auto& child : node.children_) Exec(*child);
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

        // Block
        Exec(*node.block_);

        // Return Stmt Check
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
