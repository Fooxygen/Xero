
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xcompiler/defs/fn.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    llvm::Type*       IRGen::LLVMType(sema::Type* type) {
        if (dynamic_cast<sema::ReferenceType*>(type)) {
            return llvm::PointerType::get(llvm_context(), 0);
        }

        if (type->is("none")) {
            return llvm::Type::getVoidTy(llvm_context());
        }
        if (type->is("bool")) {
            return llvm::Type::getInt1Ty(llvm_context());
        }
        if (type->is("i32")) {
            return llvm::Type::getInt32Ty(llvm_context());
        }
        if (type->is("i64")) {
            return llvm::Type::getInt64Ty(llvm_context());
        }
        if (type->is("f32")) {
            return llvm::Type::getFloatTy(llvm_context());
        }
        if (type->is("f64")) {
            return llvm::Type::getDoubleTy(llvm_context());
        }
        if (type->is("char")) {
            return llvm::Type::getInt32Ty(llvm_context());
        }
        if (type->is("string") || type->is("array"))  {
            return llvm::StructType::get(llvm_context(), {
                    llvm::PointerType::get(llvm_context(), 0),      // data
                    llvm::Type::getInt64Ty(llvm_context())          // len
                }
            );
        }
        if (type->is("range")) {
            auto parametric_type = (sema::ParametricType*)type;
            auto elem_type       = LLVMType(parametric_type->params_type()[0]);
            return llvm::StructType::get(llvm_context(),
                { elem_type, elem_type, elem_type, llvm_builder().getInt1Ty() }
            );
        }

        throw LogErr(LogModule::Xcompiler, std::format(
            "undefined type '{}'", type->name()
        ));
    }

    // e.g. x: i32 = 3; z: i32& = x;
    //      IdResolve(x): getting address of x
    //      IdResolve(z): getting address of x actually
    llvm::Value*      IRGen::IdResolve(IdExpr& node) {
        auto var = var_table_.Lookup(node.name_);

        // ReferenceType
        if (dynamic_cast<sema::ReferenceType*>(node.resolved_type_)) {
            var = llvm_builder().CreateLoad(
                llvm::PointerType::get(llvm_context(), 0), var,
                std::format(".{}.ref", node.name_)
            );
        }

        return var;
    }

    llvm::Value*      IRGen::ValMaterialize(llvm::Value* val, sema::Type* type) {
        auto slot = SlotCreate(LLVMType(type), ".arg.slot");
        llvm_builder().CreateStore(val, slot);
        return slot;
    }
    
    llvm::Value*      IRGen::ExprLoad(Expr& node) {
        auto val = Exec(node);

        // ReferenceType
        // Non-IdExpr
        if (node.resolved_type_ &&
            dynamic_cast<sema::ReferenceType*>(node.resolved_type_) &&
            !dynamic_cast<IdExpr*>(&node))
        {
            return llvm_builder().CreateLoad(
                LLVMType(node.resolved_type_->ReferenceUnwrap()), val
            );
        }
        return val;
    }
    
    Arg               IRGen::ArgRefMake(llvm::Value* val, sema::Type* type) {
        // RefArg
        if (dynamic_cast<sema::ReferenceType*>(type)) return Arg(val, type);
        return Arg(ValMaterialize(val, type), sema::TypeTable::ReferenceTypeGet(type));
    }
    
    llvm::Value*      IRGen::ArgLoad(const Arg& arg) {
        if (arg.isReferenceType()) {
            auto reference_type = (sema::ReferenceType*)arg.type();
            return llvm_builder().CreateLoad(LLVMType(reference_type->type_referred()), arg.val());
        }
        return arg.val();
    }
    
    llvm::Value*      IRGen::ArgAddr(const Arg& arg) {
        if (arg.isReferenceType()) return arg.val();

        auto slot = SlotCreate(LLVMType(arg.type()), ".arg.slot");
        llvm_builder().CreateStore(arg.val(), slot);
        return slot;
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
}
