
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xcompiler/defs/fn.hpp"
#include "xcompiler/defs/type.hpp"
#include "xcompiler/ir/gen.hpp"

namespace xcompiler {

    // Type

    llvm::Type*       IRGen::LlvmType(sema::Type* type) {
        if (dynamic_cast<sema::ReferenceType*>(type)) {
            return llvm::PointerType::get(llvm_context(), 0);
        }

        if (dynamic_cast<sema::BindingType*>(type)) {
            throw LogErr(LogStage::Xcompiler, std::format(
                "unresolved binding type '{}'", type->name()
            ));
        }

        if (type->Is("none")) {
            return llvm::Type::getVoidTy(llvm_context());
        }
        if (type->Is("bool")) {
            return llvm::Type::getInt1Ty(llvm_context());
        }
        if (type->Is("i32")) {
            return llvm::Type::getInt32Ty(llvm_context());
        }
        if (type->Is("i64")) {
            return llvm::Type::getInt64Ty(llvm_context());
        }
        if (type->Is("f32")) {
            return llvm::Type::getFloatTy(llvm_context());
        }
        if (type->Is("f64")) {
            return llvm::Type::getDoubleTy(llvm_context());
        }
        if (type->Is("char")) {
            return llvm::Type::getInt32Ty(llvm_context());
        }
        if (type->Is("string") || type->Is("array"))  {
            return llvm::StructType::get(llvm_context(), {
                    llvm::PointerType::get(llvm_context(), 0),      // data
                    llvm::Type::getInt64Ty(llvm_context())          // len
                }
            );
        }
        if (type->Is("arrayview") || type->Is("stringview")) {
            return llvm::StructType::get(llvm_context(), {
                    llvm::PointerType::get(llvm_context(), 0),      // org
                    llvm::Type::getInt64Ty(llvm_context()),         // offset
                    llvm::Type::getInt64Ty(llvm_context())          // len
                }
            );
        }
        if (type->Is("range")) {
            auto parametric_type = (sema::ParametricType*)type;
            auto elem_type       = LlvmType(parametric_type->params()[0]);
            return llvm::StructType::get(llvm_context(),
                { elem_type, elem_type, elem_type, llvm_builder().getInt1Ty() }
            );
        }
        if (auto struct_type = dynamic_cast<sema::StructType*>(type)) {
            return StructTypeGet(struct_type);
        }

        throw LogErr(LogStage::Xcompiler, std::format(
            "undefined type '{}'", type->name()
        ));
    }

    llvm::StructType* IRGen::StructTypeGet(sema::StructType* type) {
        auto it = struct_types_.find(type);
        if (it != struct_types_.end()) return it->second;

        auto llvm_type = llvm::StructType::create(llvm_context(), type->name());
        struct_types_[type] = llvm_type;

        std::vector<llvm::Type*> fields = {};
        for (auto& field : type->fields()) {
            fields.emplace_back(LlvmType(field.type_));
        }
        llvm_type->setBody(fields);
        return llvm_type;
    }

    // IR

    llvm::AllocaInst* IRGen::SlotCreate(llvm::Type* type, const std::string& name) {
        llvm::IRBuilder<> builder_alloc(
            &state_.fn_->getEntryBlock(), state_.fn_->getEntryBlock().begin()
        );
        return builder_alloc.CreateAlloca(type, nullptr, name);
    }

    llvm::BasicBlock* IRGen::BlockCreate(const std::string& name, llvm::Function* fn) {
        return llvm::BasicBlock::Create(llvm_context(), name, fn);
    }

    void              IRGen::BlockTermCreate(llvm::BasicBlock* term) {
        if (!HasBlockTerm()) llvm_builder().CreateBr(term);
    }

    void              IRGen::BlockTermCreate(std::function<void()> callback) {
        if (!HasBlockTerm()) callback();
    }

    bool              IRGen::HasBlockTerm() {
        // Program is a top-level node and has no BasicBlock
        auto block = llvm_builder().GetInsertBlock();
        return block && block->getTerminator() != nullptr;
    }

    void              IRGen::ReleaseEmit(size_t target_depth) {
        for (size_t depth = state_.release_var_scopes_.size(); depth > target_depth; depth--) {

            // Variables
            {
                auto& scope = state_.release_var_scopes_[depth - 1];
                for (auto scope_it = scope.rbegin(); scope_it != scope.rend(); ++scope_it) {
                    TypeImplTable::Lookup(scope_it->type)->MethodCall(*this, "@release", {
                        Arg(scope_it->slot, sema::TypeTable::ReferenceTypeGet(scope_it->type))
                    });
                }
            }

            // Values
            {
                auto& scope = state_.release_val_scopes_[depth - 1];
                for (auto scope_it = scope.rbegin(); scope_it != scope.rend(); ++scope_it) {
                    TypeImplTable::Lookup(scope_it->type)->MethodCall(*this, "@release", {
                        Arg(scope_it->slot, sema::TypeTable::ReferenceTypeGet(scope_it->type))
                    });
                }
            }
        }
    }

    llvm::Value*      IRGen::StructTypeValCreate(llvm::Type* type, llvm::ArrayRef<llvm::Value*> fields) {
        auto value = (llvm::Value*)llvm::PoisonValue::get(type);
        int  idx   = 0;
        for (auto field : fields) {
            value = llvm_builder().CreateInsertValue(value, field, idx++);
        }
        return value;
    }

    // Constant

    llvm::Constant* IRGen::ConstValueGet(const DeclExpr* node) {
        if (auto it = const_values_.find(node); it != const_values_.end()) {
            return it->second;
        }

        if (!const_visiting_.insert(node).second) {
            throw LogErr(LogStage::Xcompiler, std::format(
                "cyclic constant definition involving '{}'", node->id_
            ), node->loc_);
        }

        auto value = ConstEval(*node->value_, node->resolved_type_);

        const_visiting_.erase(node);
        const_values_[node] = value;
        return value;
    }

    llvm::Constant* IRGen::ConstCast(
        llvm::Constant* value,
        sema::Type* from,
        sema::Type* to
    ) {
        if (from == to) return value;

        const auto& f = from->name();
        const auto& t = to->name();

        if (f == "i32" && t == "i64")
            return llvm::ConstantExpr::getCast(
                llvm::Instruction::SExt, value,
                llvm::Type::getInt64Ty(llvm_context())
            );
        if ((f == "i32" || f == "i64") && t == "f32")
            return llvm::ConstantExpr::getCast(
                llvm::Instruction::SIToFP, value,
                llvm::Type::getFloatTy(llvm_context())
            );
        if ((f == "i32" || f == "i64") && t == "f64")
            return llvm::ConstantExpr::getCast(
                llvm::Instruction::SIToFP, value,
                llvm::Type::getDoubleTy(llvm_context())
            );
        if (f == "f32" && t == "f64")
            return llvm::ConstantExpr::getCast(
                llvm::Instruction::FPExt, value,
                llvm::Type::getDoubleTy(llvm_context())
            );
        if (f == "f64" && t == "f32")
            return llvm::ConstantExpr::getCast(
                llvm::Instruction::FPTrunc, value,
                llvm::Type::getFloatTy(llvm_context())
            );

        throw LogErr(LogStage::Xcompiler, std::format(
            "cannot cast constant from '{}' to '{}'", f, t
        ));
    }

    llvm::Constant* IRGen::ConstEval(Expr& node, sema::Type* expected) {
        using enum OperType;

        if (auto* num = dynamic_cast<NumConst*>(&node)) {
            return ConstCast(
                (llvm::Constant*)Process(*num),
                num->resolved_type_,
                expected
            );
        }
        if (auto* boolean = dynamic_cast<BoolConst*>(&node)) {
            return ConstCast(
                (llvm::Constant*)Process(*boolean),
                boolean->resolved_type_,
                expected
            );
        }
        if (auto* character = dynamic_cast<CharConst*>(&node)) {
            return ConstCast(
                (llvm::Constant*)Process(*character),
                character->resolved_type_,
                expected
            );
        }
        if (auto* id = dynamic_cast<IdExpr*>(&node)) {
            if (!id->const_decl_) {
                throw LogErr(LogStage::Xcompiler, std::format(
                    "identifier '{}' is not a constant", id->name_
                ), id->loc_);
            }
            return ConstCast(
                ConstValueGet(id->const_decl_),
                id->const_decl_->resolved_type_,
                expected
            );
        }
        if (auto* path = dynamic_cast<PathExpr*>(&node)) {
            if (!path->const_decl_) {
                throw LogErr(LogStage::Xcompiler,
                    "path is not a constant", path->loc_);
            }
            return ConstCast(
                ConstValueGet(path->const_decl_),
                path->const_decl_->resolved_type_,
                expected
            );
        }

        auto* op = dynamic_cast<OperExpr*>(&node);
        if (!op) {
            throw LogErr(LogStage::Xcompiler, std::format(
                "expression '{}' is not a constant expression",
                node.TypeName()
            ), node.loc_);
        }

        if (op->oper_type_ == Neg) {
            auto* type  = op->lexpr_->resolved_type_->ReferenceUnwrap();
            auto* value = ConstEval(*op->lexpr_, type);
            auto* result = value->getType()->isFloatingPointTy()
                ? llvm::ConstantFoldUnaryInstruction(llvm::Instruction::FNeg, value)
                : llvm::ConstantExpr::getNeg(value);
            return ConstCast(result, op->resolved_type_, expected);
        }

        auto* ltype  = op->lexpr_->resolved_type_->ReferenceUnwrap();
        auto* rtype  = op->rexpr_->resolved_type_->ReferenceUnwrap();
        auto* common = sema::TypeTable::CommonTypeGet({ ltype, rtype });
        if (!common) {
            throw LogErr(LogStage::Xcompiler, std::format(
                "cannot make type '{}' compatible with '{}'",
                ltype->name(), rtype->name()
            ), op->loc_);
        }

        auto* lhs = ConstEval(*op->lexpr_, common);
        auto* rhs = ConstEval(*op->rexpr_, common);
        auto  is_float = lhs->getType()->isFloatingPointTy();

        llvm::Constant* result = nullptr;
        switch (op->oper_type_) {
            case Plus:
                result = llvm::ConstantFoldBinaryInstruction(
                    is_float ? llvm::Instruction::FAdd : llvm::Instruction::Add,
                    lhs, rhs
                );
                break;
            case Minus:
                result = llvm::ConstantFoldBinaryInstruction(
                    is_float ? llvm::Instruction::FSub : llvm::Instruction::Sub,
                    lhs, rhs
                );
                break;
            case Star:
                result = llvm::ConstantFoldBinaryInstruction(
                    is_float ? llvm::Instruction::FMul : llvm::Instruction::Mul,
                    lhs, rhs
                );
                break;
            case Slash:
                result = llvm::ConstantFoldBinaryInstruction(
                    is_float ? llvm::Instruction::FDiv : llvm::Instruction::SDiv,
                    lhs, rhs
                );
                break;
            case ModT:
                result = llvm::ConstantFoldBinaryInstruction(
                    is_float ? llvm::Instruction::FRem : llvm::Instruction::SRem,
                    lhs, rhs
                );
                break;
            case Eq:
                result = llvm::ConstantFoldCompareInstruction(
                    is_float ? llvm::CmpInst::FCMP_OEQ : llvm::CmpInst::ICMP_EQ,
                    lhs, rhs
                );
                break;
            case Neq:
                result = llvm::ConstantFoldCompareInstruction(
                    is_float ? llvm::CmpInst::FCMP_ONE : llvm::CmpInst::ICMP_NE,
                    lhs, rhs
                );
                break;
            default:
                throw LogErr(LogStage::Xcompiler, std::format(
                    "unsupported constant operator '{}'",
                    OperTypeName(op->oper_type_)
                ), op->loc_);
        }

        return ConstCast(result, op->resolved_type_, expected);
    }

    // Expr

    llvm::Value*      IRGen::IdAddrGet(IdExpr& node) {
        auto var = slot_scope_.Lookup(node.name_, node.loc_);

        // Reference Type
        if (dynamic_cast<sema::ReferenceType*>(node.resolved_type_)) {
            var = llvm_builder().CreateLoad(
                llvm::PointerType::get(llvm_context(), 0), var,
                std::format(".{}.ref", node.name_)
            );
        }

        return var;
    }

    llvm::Value*      IRGen::ExprValGet(Expr& node) {
        auto val = Process(node);

        // Reference Type
        // Non-IdExpr
        if (node.resolved_type_ &&
            dynamic_cast<sema::ReferenceType*>(node.resolved_type_) &&
            !dynamic_cast<IdExpr*>(&node))
        {
            return llvm_builder().CreateLoad(
                LlvmType(node.resolved_type_->ReferenceUnwrap()), val
            );
        }
        return val;
    }

    llvm::Value*      IRGen::ValBorrow(llvm::Value* val, sema::Type* type) {
        auto slot = SlotCreate(LlvmType(type), ".val.slot");
        llvm_builder().CreateStore(val, slot);
        return slot;
    }

    llvm::Value*      IRGen::ValOwn(llvm::Value* val, sema::Type* type) {

        // Nothing to own
        if (!val || !type) return val;
        if (val->getType()->isVoidTy() || type->Is("none")) return val;

        // Reference Type
        if (dynamic_cast<sema::ReferenceType*>(type)) return val;

        // No block scope to release the value in
        if (state_.release_val_scopes_.empty()) return val;

        auto slot = ValBorrow(val, type);
        state_.release_val_scopes_.back().emplace_back(slot, type);
        return val;
    }

    llvm::Value*      IRGen::ValCopy(Expr& node) {
        auto* type = node.resolved_type_->ReferenceUnwrap();

        // = a, IdExpr
        if (auto* idexpr = dynamic_cast<IdExpr*>(&node)) {
            return TypeImplTable::Lookup(type)->MethodCall(*this, "@copy", {
                Arg(IdAddrGet(*idexpr), sema::TypeTable::ReferenceTypeGet(type))
            });
        }

        // = s[0], Address
        if (dynamic_cast<sema::ReferenceType*>(node.resolved_type_)) {
            return TypeImplTable::Lookup(type)->MethodCall(*this, "@copy", {
                Arg(Process(node), sema::TypeTable::ReferenceTypeGet(type))
            });
        }

        // = 1 + 2, Value
        // The value is owned by its producer, so borrow its address for copy
        auto addr = ValBorrow(ExprValGet(node), type);
        return TypeImplTable::Lookup(type)->MethodCall(*this, "@copy", {
            Arg(addr, sema::TypeTable::ReferenceTypeGet(type))
        });
    }

    Arg               IRGen::ArgRefMake(llvm::Value* val, sema::Type* type) {
        // Reference Type
        if (dynamic_cast<sema::ReferenceType*>(type)) return Arg(val, type);
        return Arg(ValBorrow(val, type), sema::TypeTable::ReferenceTypeGet(type));
    }
    
    llvm::Value*      IRGen::ArgValGet(const Arg& arg) {
        if (arg.IsReferenceType()) {
            auto reference_type = (sema::ReferenceType*)arg.type();
            return llvm_builder().CreateLoad(
                LlvmType(reference_type->referred()),
                arg.val()
            );
        }
        return arg.val();
    }
    
    llvm::Value*      IRGen::ArgAddrGet(const Arg& arg) {
        if (arg.IsReferenceType()) return arg.val();
        return ValBorrow(arg.val(), arg.type());
    }
}
