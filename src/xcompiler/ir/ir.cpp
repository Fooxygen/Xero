
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
#include "common/defs/opertype.hpp"
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

        llvm_module()->print(file, nullptr);
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
        llvm_module()->setDataLayout(target_machine->createDataLayout());
        llvm_module()->setTargetTriple(target_triple);

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

        pass.run(*llvm_module());
        file.flush();
    }

    // Utility

    llvm::Type*       IRGen::LLVMType(sema::Type* type) {
        if (type->is("none"))   return llvm::Type::getVoidTy(llvm_context());
        if (type->is("bool"))   return llvm::Type::getInt1Ty(llvm_context());
        if (type->is("i32"))    return llvm::Type::getInt32Ty(llvm_context());
        if (type->is("i64"))    return llvm::Type::getInt64Ty(llvm_context());
        if (type->is("f32"))    return llvm::Type::getFloatTy(llvm_context());
        if (type->is("f64"))    return llvm::Type::getDoubleTy(llvm_context());
        if (type->is("char"))   return llvm::Type::getInt32Ty(llvm_context());
        if (type->is("array"))  return llvm::PointerType::get(llvm_context(), 0);   // AddresSpace: normal memory, 0
        if (type->is("range")) {
            auto type_param = (sema::ParametricType*)type;
            auto elem_type  = LLVMType(type_param->params_type()[0]);
            return llvm::StructType::get(llvm_context(),
                { elem_type, elem_type, elem_type, llvm_builder().getInt1Ty() });
        }

        throw LogErr(LogModule::Xcompiler, std::format(
            "undefined type '{}'", type->name()
        ));
    }

    llvm::AllocaInst* IRGen::SlotCreate(llvm::Type* type, const std::string& name) {
        llvm::IRBuilder<> builder_alloc(
            &state_.fn_->getEntryBlock(), state_.fn_->getEntryBlock().begin()
        );
        return builder_alloc.CreateAlloca(type, nullptr, name);
    }

    bool              IRGen::hasBlockTerm() {
        // Program is a top-level node and has no BasicBlock
        auto block = llvm_builder().GetInsertBlock();
        return block && block->getTerminator() != nullptr;
    }

    llvm::BasicBlock* IRGen::BlockCreate(const std::string& name, llvm::Function* fn) {
        return llvm::BasicBlock::Create(llvm_context(), name, fn);
    }

    void              IRGen::BlockTermCreate(llvm::BasicBlock* term) {
        if (!hasBlockTerm()) llvm_builder().CreateBr(term);
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
        auto var       = var_table_.Lookup(node.name_);
        auto type_llvm = LLVMType(node.resolved_type_);
        return llvm_builder().CreateLoad(type_llvm, var, node.name_);
    }

    llvm::Value* IRGen::Exec(DeclExpr& node) {

        // Variable
        auto var_type = node.resolved_type_;
        auto var_slot = SlotCreate(LLVMType(var_type), node.id_);

        // Value
        if (node.value_) {
            auto val      = Exec(*node.value_);
            auto val_type = node.value_->resolved_type_;
            llvm_builder().CreateStore(
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
            std::vector<sema::Type*> args_type(args.size(), type);
            return TypeImplTable::Lookup(type)->MethodCall(*this, name, args, args_type);
        };

        // Unary
        auto lval = Exec(*node.lexpr_);
        {
            switch (node.oper_type_) {
                case Neg: return call(node.lexpr_->resolved_type_, "@neg", { lval });
                case Not: return call(node.lexpr_->resolved_type_, "@not", { lval });
                default:  break;
            }
        }

        // Binary

        // Short Circuit Boolean Evaluation
        if (node.oper_type_ == OperType::And || node.oper_type_ == OperType::Or) {

            // Blocks
            auto fn = llvm_builder().GetInsertBlock()->getParent();
            auto block_lval = llvm_builder().GetInsertBlock();
            auto block_rval = BlockCreate(".sc.rval", fn);
            auto block_end  = BlockCreate(".sc.end", fn);

            // Lval Block: Add Jump Instruction
            if (node.oper_type_ == OperType::And) {
                // true && -> rval block
                // false   -> end  block
                llvm_builder().CreateCondBr(lval, block_rval, block_end);
            }
            else {
                llvm_builder().CreateCondBr(lval, block_end, block_rval);
            }

            // Rval Block
            llvm_builder().SetInsertPoint(block_rval);  // write in rval block
            auto rval = Exec(*node.rexpr_);
            BlockTermCreate(block_end);

            // End Block
            llvm_builder().SetInsertPoint(block_end);
            auto phi = llvm_builder().CreatePHI(
                llvm_builder().getInt1Ty(), 2
            );
            
            if (node.oper_type_ == OperType::And) {
                // false &&
                phi->addIncoming(llvm_builder().getFalse(), block_lval);
                // true &&
                phi->addIncoming(rval, block_rval);
            }
            else {
                // true ||
                phi->addIncoming(llvm_builder().getTrue(), block_lval);
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
                case Plus:  return call(com_type, "@plus",  { lval, rval });
                case Minus: return call(com_type, "@minus", { lval, rval });
                case Star:  return call(com_type, "@star",  { lval, rval });
                case Slash: return call(com_type, "@slash", { lval, rval });
                case ModT:  return call(com_type, "@modt",  { lval, rval });
                case ModF:  return call(com_type, "@modf",  { lval, rval });
                case Gt:    return call(com_type, "@gt",    { lval, rval });
                case Lt:    return call(com_type, "@lt",    { lval, rval });
                case Ge:    return call(com_type, "@ge",    { lval, rval });
                case Le:    return call(com_type, "@le",    { lval, rval });
                case Eq:    return call(com_type, "@eq",    { lval, rval });
                case Neq:   return call(com_type, "@neq",   { lval, rval });
                case And:   return call(com_type, "@and",   { lval, rval });
                case Or:    return call(com_type, "@or",    { lval, rval });
                default:    return nullptr;
            }
        }
    }
    
    llvm::Value* IRGen::Exec(RangeExpr& node) {

        // Value
        auto iter_type      = node.iter_type_;
        auto iter_type_llvm = LLVMType(iter_type);
        auto left_val  = TypeImplTable::Cast(*this, Exec(*node.lexpr_), node.lexpr_->resolved_type_, iter_type);
        auto right_val = TypeImplTable::Cast(*this, Exec(*node.rexpr_), node.rexpr_->resolved_type_, iter_type);
        
        llvm::Value* step_val = nullptr;
        if (node.step_) {
            step_val = TypeImplTable::Cast(*this, Exec(*node.step_), node.step_->resolved_type_, iter_type);
        }
        else {
            if (iter_type->is("i32") || iter_type->is("i64")) {
                step_val = llvm::ConstantInt::get(iter_type_llvm, 1);
            }
            else {
                step_val = llvm::ConstantFP::get(iter_type_llvm, 1.0);
            }
        }

        auto isClosed_val = llvm_builder().getInt1(node.isClosed_);

        // Range Type
        auto range_type = llvm::StructType::get(
            llvm_context(),
            // left, right, step, isClosed
            { iter_type_llvm, iter_type_llvm, iter_type_llvm, llvm_builder().getInt1Ty() }
        );
        auto range_val  = (llvm::Value*)llvm::UndefValue::get(range_type);
        range_val = llvm_builder().CreateInsertValue(range_val, left_val, 0);
        range_val = llvm_builder().CreateInsertValue(range_val, right_val, 1);
        range_val = llvm_builder().CreateInsertValue(range_val, step_val, 2);
        range_val = llvm_builder().CreateInsertValue(range_val, isClosed_val, 3);
        
        return range_val;
    }

    llvm::Value* IRGen::Exec(ArrayExpr& node) {
        
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
                return llvm_builder().CreateCall(lang->impl(), args);
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

        // Call
        return target_impl->MethodCall(
            *this, node.callee_->name_, args, args_type
        );
    }

    llvm::Value* IRGen::Exec(FnExpr& node) {

        // Return Type
        auto return_type_llvm = LLVMType(node.ret_resolved_type_);
        if (node.name_ == "main") {
            return_type_llvm = llvm::Type::getInt32Ty(llvm_context());
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
            return_type_llvm, params_type, false    // No variable params
        );
        auto fn = llvm::Function::Create(
            fntype,
            llvm::Function::ExternalLinkage,
            node.name_,
            llvm_module()
        );
        auto block = BlockCreate("entry", fn);
        llvm_builder().SetInsertPoint(block);

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
        Exec(*node.body_, [&]() {
            // Args
            for (auto& arg : fn->args()) {
                auto var_name = arg.getName().str();
                auto var_slot = SlotCreate(arg.getType(), var_name);
                llvm_builder().CreateStore(&arg, var_slot);
                var_table_.Declare(var_name, var_slot);
            }
        });

        // Return Stmt
        // Each block requires a terminal symbol
        // including return value, the unreachable stmt...
        BlockTermCreate([&]() {
            if (return_type_llvm->isVoidTy())
                llvm_builder().CreateRetVoid();
            else
                llvm_builder().CreateRet(llvm::ConstantInt::get(return_type_llvm, 0));
        });

        if (node.name_ != "main") {
            FnImplTable::Add(node.name_, node.fnsign_, std::make_unique<LangFnImpl>(fn));
        }

        return nullptr;
    }

    // Const

    llvm::Value* IRGen::Exec(NumConst& node) {
        auto type = node.resolved_type_;
        if (type->is("i32")) return llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_context()), node.resolved_value_.integer_);
        if (type->is("i64")) return llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_context()), node.resolved_value_.integer_);
        if (type->is("f32")) return llvm::ConstantFP::get(llvm::Type::getFloatTy(llvm_context()),  node.resolved_value_.floating_);
        if (type->is("f64")) return llvm::ConstantFP::get(llvm::Type::getDoubleTy(llvm_context()), node.resolved_value_.floating_);
        std::unreachable();
    }

    llvm::Value* IRGen::Exec(BoolConst& node) {
        return node.value_
            ? llvm::ConstantInt::getTrue(llvm_context())
            : llvm::ConstantInt::getFalse(llvm_context());
    }

    llvm::Value* IRGen::Exec(CharConst& node) {
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(llvm_context()), node.codepoint_);
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

        llvm_builder().CreateStore(
            TypeImplTable::Cast(*this, val, val_type, var_type),
            var
        );
        return nullptr;
    }

    llvm::Value* IRGen::Exec(CondStmt& node) {
        auto fn = llvm_builder().GetInsertBlock()->getParent();
        auto block_end = BlockCreate(".cond.end", fn);

        std::function<void(CondStmt&)> emit = [&](CondStmt& node_sub) {

            // Cond
            if (!node_sub.cond_) {
                Exec(*node_sub.then_);
                BlockTermCreate(block_end);
                return;
            }
            auto cond_val = Exec(*node_sub.cond_);

            // Blocks
            auto fn_sub = llvm_builder().GetInsertBlock()->getParent();
            auto block_then = BlockCreate(".cond.then", fn_sub);
            auto block_else = BlockCreate(".cond.else", fn_sub);

            // This Block
            llvm_builder().CreateCondBr(cond_val, block_then, block_else);

            // Then Block
            llvm_builder().SetInsertPoint(block_then);
            Exec(*node_sub.then_);
            BlockTermCreate(block_end);

            // Else Block
            llvm_builder().SetInsertPoint(block_else);
            if (node_sub.next_)
                emit(*node_sub.next_);
            else
                BlockTermCreate(block_end);
        };

        emit(node);
        llvm_builder().SetInsertPoint(block_end);
        return nullptr;
    }
        
    llvm::Value* IRGen::Exec(LoopSignalStmt& node) {
        auto& nextblock = state_.loop_nextblocks_.back();
        llvm_builder().CreateBr(
            node.signal_ == LoopSignal::Continue ?
                nextblock.continue_ : nextblock.break_
        );
        return nullptr;
    }

    llvm::Value* IRGen::Exec(ReturnSignalStmt& node) {
        if (node.value_) {
            auto val = Exec(*node.value_);
            llvm_builder().CreateRet(
                TypeImplTable::Cast(*this, val, node.value_->resolved_type_, state_.fn_return_type_)
            );
        }
        else llvm_builder().CreateRetVoid();
        return nullptr;
    }

    llvm::Value* IRGen::Exec(ForStmt& node) {
        auto data = Exec(*node.data_);

        // Range
        if (node.data_->resolved_type_->is("range")) {
            auto range_type     = (sema::ParametricType*)node.data_->resolved_type_;
            auto iter_type      = range_type->params_type()[0];
            auto iter_type_impl = TypeImplTable::Lookup(iter_type);
            auto left_val       = llvm_builder().CreateExtractValue(data, 0);
            auto right_val      = llvm_builder().CreateExtractValue(data, 1);
            auto step_val       = llvm_builder().CreateExtractValue(data, 2);
            auto isClosed_val   = llvm_builder().CreateExtractValue(data, 3);

            std::vector<sema::Type*> cmp_args_type = { iter_type, iter_type };

            // Iterator
            auto iter_slot = SlotCreate(LLVMType(iter_type), node.iter_->name_);
            llvm_builder().CreateStore(left_val, iter_slot);

            // Blocks
            auto fn = llvm_builder().GetInsertBlock()->getParent();
            auto block_cond = BlockCreate(".for.cond", fn);
            auto block_body = BlockCreate(".for.body", fn);
            auto block_step = BlockCreate(".for.step", fn);
            auto block_end  = BlockCreate(".for.end", fn);

            // Cond Block
            llvm_builder().CreateBr(block_cond);
            llvm_builder().SetInsertPoint(block_cond);
            {
                auto iter_val = llvm_builder().CreateLoad(LLVMType(iter_type), iter_slot);
                auto isIncreasing = iter_type_impl->MethodCall(
                    *this, "@ge", { right_val, left_val }, cmp_args_type
                );

                auto ge = iter_type_impl->MethodCall(*this, "@ge", { iter_val, right_val }, cmp_args_type);
                auto gt = iter_type_impl->MethodCall(*this, "@gt", { iter_val, right_val }, cmp_args_type);
                auto le = iter_type_impl->MethodCall(*this, "@le", { iter_val, right_val }, cmp_args_type);
                auto lt = iter_type_impl->MethodCall(*this, "@lt", { iter_val, right_val }, cmp_args_type);

                auto overstep_inc = llvm_builder().CreateSelect(isClosed_val, gt, ge);
                auto overstep_dec = llvm_builder().CreateSelect(isClosed_val, lt, le);
                auto overstep     = llvm_builder().CreateSelect(isIncreasing, overstep_inc, overstep_dec);
                
                llvm_builder().CreateCondBr(overstep, block_end, block_body);
            }

            // Body Block
            llvm_builder().SetInsertPoint(block_body);
            {
                state_.loop_nextblocks_.emplace_back(State::LoopNextBlock{ block_step, block_end });
                Exec(*node.body_, [&]{ var_table_.Declare(node.iter_->name_, iter_slot); });
                state_.loop_nextblocks_.pop_back();

                // Entry step block to iterate var
                // Before this, there might already be a term here, such as 'break'
                // So it is necessary to determine the existence of term
                BlockTermCreate(block_step);
            }

            // Step Block
            llvm_builder().SetInsertPoint(block_step);
            {
                auto iter_val      = llvm_builder().CreateLoad(LLVMType(iter_type), iter_slot);
                auto iter_val_next = iter_type_impl->MethodCall(
                    *this, "@plus", { iter_val, step_val }, cmp_args_type
                );
                llvm_builder().CreateStore(iter_val_next, iter_slot);
                llvm_builder().CreateBr(block_cond);
            }

            // End Block
            llvm_builder().SetInsertPoint(block_end);
        }

        return nullptr;
    }

    llvm::Value* IRGen::Exec(WhileStmt& node) {

        // Blocks
        auto fn = llvm_builder().GetInsertBlock()->getParent();
        auto block_cond = BlockCreate(".while.cond", fn);
        auto block_body = BlockCreate(".while.body", fn);
        auto block_end  = BlockCreate(".while.end", fn);
    
        // This Block
        llvm_builder().CreateBr(block_cond);

        // Cond Block
        llvm_builder().SetInsertPoint(block_cond);
        auto cond_val = Exec(*node.cond_);
        llvm_builder().CreateCondBr(cond_val, block_body, block_end);

        // Body Block
        llvm_builder().SetInsertPoint(block_body);
        state_.loop_nextblocks_.emplace_back(State::LoopNextBlock{
           .continue_ = block_cond,
           .break_    = block_end
        });
        Exec(*node.body_);
        state_.loop_nextblocks_.pop_back();
        BlockTermCreate(block_cond);

        llvm_builder().SetInsertPoint(block_end);
        return nullptr;
    }
    
    // Common

    llvm::Value* IRGen::Exec(Program& node) {
        for (auto& child : node.children_) Exec(*child);
        return nullptr;
    }
}
