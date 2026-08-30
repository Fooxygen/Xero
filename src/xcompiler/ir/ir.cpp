
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
#include "xcompiler/builtin.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/defs/fn.hpp"
#include "xcompiler/ir/ir.hpp"

namespace xcompiler {

    // Output

    void IRGen::IROutput(const std::string& path) {

        // Open File
        if (path.empty()) {
            throw LogErr(LogModule::Xcompiler, "empty file path");
        }

        std::error_code ec;
        llvm::raw_fd_ostream file(path, ec);
        if (ec) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "failed to open file '{}'", path
            ));
        }

        llvmcore_.module_->print(file, nullptr);
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
        llvmcore_.module_->setDataLayout(target_machine->createDataLayout());
        llvmcore_.module_->setTargetTriple(target_triple);

        // └─ Open File
        if (path.empty()) {
            throw LogErr(LogModule::Xcompiler, "empty file path");
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

        pass.run(*llvmcore_.module_);
        file.flush();
    }

    // Utility

    llvm::Type*       IRGen::LLVMType(sema::Type* type) {
        if (type->is("none"))   return llvm::Type::getVoidTy(llvmcore_.context_);
        if (type->is("bool"))   return llvm::Type::getInt1Ty(llvmcore_.context_);
        if (type->is("i32"))    return llvm::Type::getInt32Ty(llvmcore_.context_);
        if (type->is("i64"))    return llvm::Type::getInt64Ty(llvmcore_.context_);
        if (type->is("f32"))    return llvm::Type::getFloatTy(llvmcore_.context_);
        if (type->is("f64"))    return llvm::Type::getDoubleTy(llvmcore_.context_);
        if (type->is("char"))   return llvm::Type::getInt32Ty(llvmcore_.context_);

        throw LogErr(LogModule::Xcompiler, std::format(
            "undefined type '{}'", type->name()
        ));
    }

    llvm::AllocaInst* IRGen::EntryBlockSlotCreate(llvm::Type* type, const std::string& name) {
        llvm::IRBuilder<> builder_alloc(
            &state_.fn_->getEntryBlock(), state_.fn_->getEntryBlock().begin()
        );
        return builder_alloc.CreateAlloca(type, nullptr, name);
    }

    bool              IRGen::hasBlockTerm() {
        // Program is a top-level node and has no BasicBlock
        auto block = llvmcore_.builder_.GetInsertBlock();
        return block && block->getTerminator() != nullptr;
    }

    void              IRGen::BlockTermCreate(llvm::BasicBlock* term) {
        if (!hasBlockTerm()) llvmcore_.builder_.CreateBr(term);
    }

    void              IRGen::BlockTermCreate(std::function<void()> callback) {
        if (!hasBlockTerm()) callback();
    }

    // Expr

    llvm::Value* IRGen::Exec(BlockExpr& node, std::function<void()> OnScopeReady) {
        var_table_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        try {
            for (auto& child : node.children_) {
                if (hasBlockTerm()) continue;   // invalid stmts come after term
                Exec(*child);
            }
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
        auto type = LLVMType(node.resolved_type_);
        return llvmcore_.builder_.CreateLoad(type, var, node.name_);
    }

    llvm::Value* IRGen::Exec(DeclExpr& node) {

        // Variable
        auto var_type = node.resolved_type_;
        auto var_slot = EntryBlockSlotCreate(LLVMType(var_type), node.id_);

        // Value
        if (node.value_) {
            auto val      = Exec(*node.value_);
            auto val_type = node.value_->resolved_type_;
            llvmcore_.builder_.CreateStore(
                TypeImplTable::Cast(*this, val, val_type, var_type),
                var_slot
            );
        }
        
        var_table_.Declare(node.id_, var_slot);
        return nullptr;
    }

    llvm::Value* IRGen::Exec(OperExpr& node) {
        using enum OperType;

        auto call = [&](sema::Type* type, const std::string& name, std::vector<llvm::Value*> args) {

            // TypeImpl
            auto type_basic = (sema::BasicType*)type->BasicTypeGet();
            auto type_impl  = TypeImplTable::Lookup(type);

            // Args Type
            // Due to implicit type conversion, all args type are consistent
            std::vector<sema::Type*> args_type(args.size(), type);

            // Method
            auto& method      = type_basic->method_table().Lookup(name);
            auto  method_sign = method.SignLookup(args_type);
            auto  method_impl = type_impl->MethodGet(method_sign);
            return ((NativeFnImpl*)method_impl)->impl()(*this, args, args_type);
        };

        // Unary
        auto lval = Exec(*node.lexpr_);
        {
            switch (node.oper_type_) {
                case Neg: return call(node.lexpr_->resolved_type_, "neg", { lval });
                case Not: return call(node.lexpr_->resolved_type_, "not", { lval });
                default:  break;
            }
        }

        // Binary

        // Short Circuit Boolean Evaluation
        if (node.oper_type_ == OperType::And || node.oper_type_ == OperType::Or) {

            // Blocks
            auto fn = llvmcore_.builder_.GetInsertBlock()->getParent();
            auto block_lval = llvmcore_.builder_.GetInsertBlock();
            auto block_rval = llvm::BasicBlock::Create(llvmcore_.context_, ".sc.rval", fn);
            auto block_end  = llvm::BasicBlock::Create(llvmcore_.context_, ".sc.end", fn);

            // Lval Block: Add Jump Instruction
            if (node.oper_type_ == OperType::And) {
                // true && -> rval block
                // false   -> end  block
                llvmcore_.builder_.CreateCondBr(lval, block_rval, block_end);
            }
            else {
                llvmcore_.builder_.CreateCondBr(lval, block_end, block_rval);
            }

            // Rval Block
            llvmcore_.builder_.SetInsertPoint(block_rval);  // write in rval block
            auto rval = Exec(*node.rexpr_);
            BlockTermCreate(block_end);

            // End Block
            llvmcore_.builder_.SetInsertPoint(block_end);
            auto phi = llvmcore_.builder_.CreatePHI(
                llvmcore_.builder_.getInt1Ty(), 2
            );
            
            if (node.oper_type_ == OperType::And) {
                // false &&
                phi->addIncoming(llvmcore_.builder_.getFalse(), block_lval);
                // true &&
                phi->addIncoming(rval, block_rval);
            }
            else {
                // true ||
                phi->addIncoming(llvmcore_.builder_.getTrue(), block_lval);
                // false ||
                phi->addIncoming(rval, block_rval);
            }

            return phi;
        }

        auto rval = Exec(*node.rexpr_);
        {
            auto ltype = node.lexpr_->resolved_type_;
            auto rtype = node.rexpr_->resolved_type_;
            auto com_type = sema::TypeTable::Common({ ltype, rtype });
            if (!com_type) {
                throw LogErr(LogModule::Xcompiler, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    ltype->name(), rtype->name()
                ), node.loc_);
            }

            lval = TypeImplTable::Cast(*this, lval, ltype, com_type);
            rval = TypeImplTable::Cast(*this, rval, rtype, com_type);

            switch (node.oper_type_) {
                case Plus:  return call(com_type, "plus",  { lval, rval });
                case Minus: return call(com_type, "minus", { lval, rval });
                case Star:  return call(com_type, "star",  { lval, rval });
                case Slash: return call(com_type, "slash", { lval, rval });
                case ModT:  return call(com_type, "modt",  { lval, rval });
                case ModF:  return call(com_type, "modf",  { lval, rval });
                case Gt:    return call(com_type, "gt",    { lval, rval });
                case Lt:    return call(com_type, "lt",    { lval, rval });
                case Ge:    return call(com_type, "ge",    { lval, rval });
                case Le:    return call(com_type, "le",    { lval, rval });
                case Eq:    return call(com_type, "eq",    { lval, rval });
                case Neq:   return call(com_type, "neq",   { lval, rval });
                case And:   return call(com_type, "and",   { lval, rval });
                case Or:    return call(com_type, "or",    { lval, rval });
                default:    return nullptr;
            }
        }
    }
    
    llvm::Value* IRGen::Exec(FnCallExpr& node) {
        auto fnsign = node.callee_fnsign_;

        // Args
        std::vector<llvm::Value*> args      = {};
        std::vector<sema::Type*>  args_type = {};
        if (node.args_) {
            for (auto& e : node.args_->exprs_) {
                args.emplace_back(Exec(*e));
                args_type.emplace_back(e->resolved_type_);
            }
        }

        // Stored in FnTable
        if (auto impl = FnImplTable::LookupTry(fnsign)) {
            if (auto native = dynamic_cast<NativeFnImpl*>(impl)) {
                return native->impl()(*this, args, args_type);
            }
            if (auto lang   = dynamic_cast<LangFnImpl*>(impl)) {
                return llvmcore_.builder_.CreateCall(lang->impl(), args);
            }
        }

        throw LogErr(LogModule::Xcompiler, std::format(
            "undefined function '{}'", node.callee_->name_
        ), node.loc_);
    }
    
    llvm::Value* IRGen::Exec(MethodCallExpr& node) {
        
        // Target
        auto target_val   = Exec(*node.target_);
        auto target_type  = node.target_->resolved_type_;
        auto target_basic = target_type->BasicTypeGet();
        auto target_impl  = TypeImplTable::Lookup(target_basic);

        // Args
        std::vector<llvm::Value*> args      = { target_val };
        std::vector<sema::Type*>  args_type = { target_type };
        if (node.args_) {
            for (auto& e : node.args_->exprs_) {
                args.emplace_back(Exec(*e));
                args_type.emplace_back(e->resolved_type_);
            }
        }

        // Method
        auto method_impl = target_impl->MethodGet(node.callee_fnsign_);

        if (auto native = dynamic_cast<NativeFnImpl*>(method_impl)) {
            return native->impl()(*this, args, args_type);
        }
        if (auto lang = dynamic_cast<LangFnImpl*>(method_impl)) {
            return llvmcore_.builder_.CreateCall(lang->impl(), args);
        }

        throw LogErr(LogModule::Xcompiler, std::format(
            "unsupported method '{}'", node.callee_->name_
        ), node.loc_);
    }

    llvm::Value* IRGen::Exec(FnExpr& node) {

        // Return Type
        auto ret_type = LLVMType(node.ret_resolved_type_);
        if (node.name_ == "main") {
            ret_type = llvm::Type::getInt32Ty(llvmcore_.context_);
        }

        // Params Type
        std::vector<llvm::Type*> params_type = {};
        if (node.params_) {
            for (auto& e : node.params_->exprs_) {
                auto param = (DeclExpr*)(e.get());
                params_type.emplace_back(LLVMType(param->resolved_type_));
            }
        }

        // Fn
        auto fntype = llvm::FunctionType::get(
            ret_type, params_type, false        // No variable params
        );
        auto fn = llvm::Function::Create(
            fntype,
            llvm::Function::ExternalLinkage,
            node.name_,
            llvmcore_.module_.get()
        );
        auto block = llvm::BasicBlock::Create(llvmcore_.context_, "entry", fn);
        llvmcore_.builder_.SetInsertPoint(block);

        // Processing Fn
        state_.fn_ = fn;
        state_.fn_return_type_ = node.ret_resolved_type_;

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
                llvmcore_.builder_.CreateStore(&arg, var_slot);
                var_table_.Declare(var_name, var_slot);
            }
        });

        // Return Stmt
        // Each block requires a terminal symbol
        // including return value, the unreachable stmt...
        BlockTermCreate([&]() {
            if (ret_type->isVoidTy())
                llvmcore_.builder_.CreateRetVoid();
            else
                llvmcore_.builder_.CreateRet(llvm::ConstantInt::get(ret_type, 0));
        });

        if (node.name_ != "main") {
            FnImplTable::Add(node.name_, node.fnsign_, std::make_unique<LangFnImpl>(fn));
        }

        return nullptr;
    }

    // Const

    llvm::Value* IRGen::Exec(NumConst& node) {
        auto type = node.resolved_type_;
        if (type->is("i32")) return llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvmcore_.context_), node.resolved_value_.integer_);
        if (type->is("i64")) return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvmcore_.context_), node.resolved_value_.integer_);
        if (type->is("f32")) return llvm::ConstantFP::get(llvm::Type::getFloatTy(llvmcore_.context_),  node.resolved_value_.floating_);
        if (type->is("f64")) return llvm::ConstantFP::get(llvm::Type::getDoubleTy(llvmcore_.context_), node.resolved_value_.floating_);
        std::unreachable();
    }

    llvm::Value* IRGen::Exec(BoolConst& node) {
        return node.value_
            ? llvm::ConstantInt::getTrue(llvmcore_.context_)
            : llvm::ConstantInt::getFalse(llvmcore_.context_);
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

        llvmcore_.builder_.CreateStore(
            TypeImplTable::Cast(*this, val, val_type, var_type),
            var
        );
        return nullptr;
    }

    llvm::Value* IRGen::Exec(CondStmt& node) {
        auto fn = llvmcore_.builder_.GetInsertBlock()->getParent();
        auto block_end = llvm::BasicBlock::Create(llvmcore_.context_, ".cond.end", fn);

        std::function<void(CondStmt&)> emit = [&](CondStmt& node_sub) {

            // Cond
            if (!node_sub.cond_) {
                Exec(*node_sub.then_);
                BlockTermCreate(block_end);
                return;
            }
            auto cond_val = Exec(*node_sub.cond_);

            // Blocks
            auto fn_sub = llvmcore_.builder_.GetInsertBlock()->getParent();
            auto block_then = llvm::BasicBlock::Create(llvmcore_.context_, ".cond.then", fn_sub);
            auto block_else = llvm::BasicBlock::Create(llvmcore_.context_, ".cond.else", fn_sub);

            // This Block
            llvmcore_.builder_.CreateCondBr(cond_val, block_then, block_else);

            // Then Block
            llvmcore_.builder_.SetInsertPoint(block_then);
            Exec(*node_sub.then_);
            BlockTermCreate(block_end);

            // Else Block
            llvmcore_.builder_.SetInsertPoint(block_else);
            if (node_sub.next_)
                emit(*node_sub.next_);
            else
                BlockTermCreate(block_end);
        };

        emit(node);
        llvmcore_.builder_.SetInsertPoint(block_end);
        return nullptr;
    }
        
    llvm::Value* IRGen::Exec(LoopSignalStmt& node) {
        auto& nextblock = state_.loop_nextblocks_.back();
        llvmcore_.builder_.CreateBr(
            node.signal_ == LoopSignal::Continue ?
                nextblock.continue_ : nextblock.break_
        );
        return nullptr;
    }

    llvm::Value* IRGen::Exec(ReturnSignalStmt& node) {
        if (node.value_) {
            auto val = Exec(*node.value_);
            llvmcore_.builder_.CreateRet(
                TypeImplTable::Cast(*this, val, node.value_->resolved_type_, state_.fn_return_type_)
            );
        }
        else llvmcore_.builder_.CreateRetVoid();
        return nullptr;
    }

    llvm::Value* IRGen::Exec(WhileStmt& node) {

        // Blocks
        auto fn = llvmcore_.builder_.GetInsertBlock()->getParent();
        auto block_cond = llvm::BasicBlock::Create(llvmcore_.context_, ".while.cond", fn);
        auto block_then = llvm::BasicBlock::Create(llvmcore_.context_, ".while.then", fn);
        auto block_end  = llvm::BasicBlock::Create(llvmcore_.context_, ".while.end", fn);
    
        // This Block
        llvmcore_.builder_.CreateBr(block_cond);

        // Cond Block
        llvmcore_.builder_.SetInsertPoint(block_cond);
        auto cond_val = Exec(*node.cond_);
        llvmcore_.builder_.CreateCondBr(cond_val, block_then, block_end);

        // Then Block
        llvmcore_.builder_.SetInsertPoint(block_then);
        state_.loop_nextblocks_.emplace_back(State::LoopNextBlock{
           .continue_ = block_cond,
           .break_    = block_end
        });
        Exec(*node.then_);
        state_.loop_nextblocks_.pop_back();
        BlockTermCreate(block_cond);

        llvmcore_.builder_.SetInsertPoint(block_end);
        return nullptr;
    }
    
    // Common

    llvm::Value* IRGen::Exec(Program& node) {
        return Exec((BlockExpr&)node);
    }
}
