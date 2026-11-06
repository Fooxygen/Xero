
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"

#include "common/defs/ast.hpp"
#include "sema/defs/type.hpp"
#include "xcompiler/defs/var.hpp"

namespace xcompiler {

    class IRGen {
    private:
        class  LlvmCore {
        public:
            llvm::LLVMContext             context_;
            std::unique_ptr<llvm::Module> module_;
            llvm::IRBuilder<>             builder_;

            LlvmCore(std::string_view module_name)
            :   context_(),
                module_(std::make_unique<llvm::Module>(module_name, context_)),
                builder_(context_)
            {}
        };

        struct State {
            // Function

            llvm::Function* fn_               = nullptr;
            sema::Type*     fn_return_type_   = nullptr;

            // Loop Structure
            struct LoopNextBlock {
                llvm::BasicBlock* continue_            = nullptr;
                llvm::BasicBlock* break_               = nullptr;
                size_t            release_scope_depth_ = 0;
            };
            
            std::vector<LoopNextBlock> loop_nextblocks_ = {};

            // Release
            struct ReleaseSlot {
                llvm::Value* slot = nullptr;
                sema::Type*  type = nullptr;
            };

            std::vector<std::vector<ReleaseSlot>> release_scopes_ = {};
        };
    
    private:
        LlvmCore    llvmcore_;
        State       state_;
        SlotScope   slot_scope_;
        LlvmFnTable llvm_fn_table_;

        // Module
        std::string module_name_full_ = "";

        // Struct

        std::unordered_map<sema::StructType*, llvm::StructType*> struct_types_ = {};

    private:
        // Declare
        void Declare(FnExpr& node);
        void Declare(StructDeclExpr& node);

        void Declare(Program& node);

        // Process

        llvm::Value* Process(BlockExpr& node, const std::function<void()>& on_scope_ready = nullptr);
        llvm::Value* Process(IdExpr& node);
        llvm::Value* Process(RefExpr& node);
        llvm::Value* Process(DeclExpr& node);
        llvm::Value* Process(OperExpr& node);
        llvm::Value* Process(RangeExpr& node);
        llvm::Value* Process(ArrayLitExpr& node);
        llvm::Value* Process(FnExpr& node);
        llvm::Value* Process(FnCallExpr& node);
        llvm::Value* Process(MethodCallExpr& node);
        llvm::Value* Process(StructDeclExpr& node);
        llvm::Value* Process(StructLitExpr& node);
        llvm::Value* Process(FieldAccessExpr& node);

        llvm::Value* Process(NumConst& node);
        llvm::Value* Process(BoolConst& node);
        llvm::Value* Process(CharConst& node);
        llvm::Value* Process(StringConst& node);

        llvm::Value* Process(ExprStmt& node);
        llvm::Value* Process(AssignStmt& node);
        llvm::Value* Process(CondStmt& node);
        llvm::Value* Process(LoopSignalStmt& node);
        llvm::Value* Process(ReturnSignalStmt& node);
        llvm::Value* Process(ForStmt& node);
        llvm::Value* Process(WhileStmt& node);

        llvm::Value* Process(Program& node);

    public:
        IRGen(std::string_view module_name)
        :   llvmcore_(module_name) {}

        llvm::LLVMContext& llvm_context() { return llvmcore_.context_; }
        llvm::Module*      llvm_module()  { return llvmcore_.module_.get(); }
        llvm::IRBuilder<>& llvm_builder() { return llvmcore_.builder_; }

    public:
        // Utility

        // └─ Type

        llvm::Type*       LlvmType(sema::Type* type);
        llvm::StructType* StructTypeGet(sema::StructType* type);

        // └─ IR

        llvm::AllocaInst* SlotCreate(llvm::Type* type, const std::string& name);
        llvm::BasicBlock* BlockCreate(const std::string& name, llvm::Function* fn);     // create basic block
        void              BlockTermCreate(llvm::BasicBlock* term);
        void              BlockTermCreate(std::function<void()> callback);
        bool              HasBlockTerm();
        void              ReleaseEmit(size_t target_depth);
        llvm::Value*      StructTypeValCreate(llvm::Type* type, llvm::ArrayRef<llvm::Value*> fields);

        // └─ Expr

        // e.g. x: i32 = 3; z: i32& = x;
        //      IdAddrGet(x): get address of x
        //      IdAddrGet(z): get address of x actually
        llvm::Value*      IdAddrGet(IdExpr& node);                                      // get address from idexpr
        llvm::Value*      ValMaterialize(llvm::Value* val, sema::Type* type);           // allocate memory for value to store
        llvm::Value*      ExprValGet(Expr& node);                                       // get value from expr
        
        Arg               ArgRefMake(llvm::Value* val, sema::Type* type);               // wrap value as ref arg
        llvm::Value*      ArgLoad(const Arg& arg);                                      // take value from arg
        llvm::Value*      ArgAddr(const Arg& arg);                                      // take address from arg

        // Module

        void ModuleSet(const std::string& name_full) { module_name_full_ = name_full; }

        // Declare

        void Declare(AstNode& node) {
            switch (node.type_) {
                case AstType::FnExpr:         Declare((FnExpr&)node);         return;
                case AstType::StructDeclExpr: Declare((StructDeclExpr&)node); return;

                case AstType::Program:        Declare((Program&)node);        return;

                default: return;
            }
        }

        // Process

        llvm::Value* Process(AstNode& node) {
            switch (node.type_) {
                case AstType::BlockExpr:        return Process((BlockExpr&)node);
                case AstType::IdExpr:           return Process((IdExpr&)node);
                case AstType::RefExpr:          return Process((RefExpr&)node);
                case AstType::DeclExpr:         return Process((DeclExpr&)node);
                case AstType::OperExpr:         return Process((OperExpr&)node);
                case AstType::RangeExpr:        return Process((RangeExpr&)node);
                case AstType::ArrayLitExpr:     return Process((ArrayLitExpr&)node);
                case AstType::FnExpr:           return Process((FnExpr&)node);
                case AstType::FnCallExpr:       return Process((FnCallExpr&)node);
                case AstType::MethodCallExpr:   return Process((MethodCallExpr&)node);
                case AstType::StructDeclExpr:   return Process((StructDeclExpr&)node);
                case AstType::StructLitExpr:    return Process((StructLitExpr&)node);
                case AstType::FieldAccessExpr:  return Process((FieldAccessExpr&)node);
                
                case AstType::NumConst:         return Process((NumConst&)node);
                case AstType::BoolConst:        return Process((BoolConst&)node);
                case AstType::CharConst:        return Process((CharConst&)node);
                case AstType::StringConst:      return Process((StringConst&)node);
                
                case AstType::ExprStmt:         return Process((ExprStmt&)node);
                case AstType::AssignStmt:       return Process((AssignStmt&)node);
                case AstType::CondStmt:         return Process((CondStmt&)node);
                case AstType::LoopSignalStmt:   return Process((LoopSignalStmt&)node);
                case AstType::ReturnSignalStmt: return Process((ReturnSignalStmt&)node);
                case AstType::ForStmt:          return Process((ForStmt&)node);
                case AstType::WhileStmt:        return Process((WhileStmt&)node);

                case AstType::Program:          return Process((Program&)node);

                default: return nullptr;
            }
        }
    };
}
