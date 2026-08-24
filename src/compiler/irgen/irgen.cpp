
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>
#include <vector>

#include "common/log.hpp"
#include "common/opertype.hpp"
#include "compiler/irgen/irgen.hpp"
#include "compiler/irgen/type.hpp"

namespace compiler {

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
            throw LogErr(LogModule::Compiler, std::format(
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

        throw LogErr(LogModule::Compiler, std::format(
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

        try {
            for (auto& child : node.children_) Exec(*child);
        }
        catch (...) {
            value_table_.ScopePop();
            throw;
        }

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
        auto var_type = node.resolved_type_;
        auto slot     = EntryBlockSlotCreate(LlvmType(var_type), node.id_);

        // Value
        if (node.value_) {
            auto val = Exec(*node.value_);
            auto val_type = node.value_->resolved_type_;
            builder_.CreateStore(
                TypeGenTable::Cast(*this, val, val_type, var_type),
                slot
            );
        }
        
        value_table_.Declare(node.id_, slot);
        return nullptr;
    }

    llvm::Value* IRGen::Exec(OperExpr& node) {
        using enum OperType;

        // Unary
        auto lval = Exec(*node.lexpr_);
        {
            switch (node.oper_type_) {
                case Neg:   return node.lexpr_->resolved_type_->gen()->neg_(*this, lval);
                case Not:   return node.lexpr_->resolved_type_->gen()->not_(*this, lval);
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
                throw LogErr(LogModule::Compiler, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    ltype->name_, rtype->name_
                ), node.loc_);
            }
            auto gen = common_type->gen();

            lval = TypeGenTable::Cast(*this, lval, ltype, common_type);
            rval = TypeGenTable::Cast(*this, rval, rtype, common_type);

            switch (node.oper_type_) {
                case Plus:  return gen->plus_ (*this, lval, rval);
                case Minus: return gen->minus_(*this, lval, rval);
                case Star:  return gen->star_ (*this, lval, rval);
                case Slash: return gen->slash_(*this, lval, rval);
                case ModT:  return gen->modt_ (*this, lval, rval);
                case ModF:  return gen->modf_ (*this, lval, rval);
                case Gt:    return gen->gt_   (*this, lval, rval);
                case Lt:    return gen->lt_   (*this, lval, rval);
                case Ge:    return gen->ge_   (*this, lval, rval);
                case Le:    return gen->le_   (*this, lval, rval);
                case Eq:    return gen->eq_   (*this, lval, rval);
                case Neq:   return gen->neq_  (*this, lval, rval);
                default:    return nullptr;
            }
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

    llvm::Value* IRGen::Exec(AssignStmt& node) {
        auto var      = (IdExpr*)(node.target_.get());
        auto var_type = var->resolved_type_;
        auto slot     = value_table_.Lookup(var->value_);
        auto val      = Exec(*node.value_);
        auto val_type = node.value_->resolved_type_;

        builder_.CreateStore(
            TypeGenTable::Cast(*this, val, val_type, var_type),
            slot
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
