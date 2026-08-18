
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine.hpp"
#include "common/log.hpp"
#include "common/opertype.hpp"
#include "common/signal.hpp"

namespace rt {
    
    // Expr

    Obj Xengine::Exec(BlockExpr& node, std::function<void()> OnScopeReady) {
        env_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        try {
            for (auto& child : node.children_) Exec(*child);
        }
        catch (...) {
            env_.ScopePop();
            throw;
        }
        
        env_.ScopePop();
        return Obj();
    }

    Obj Xengine::Exec(IdExpr& node) {
        return Obj::MakeRef(env_.Lookup(node.value_, node.loc_));
    }

    Obj Xengine::Exec(DeclExpr& node) {
        auto bind_type = node.resolved_type_;

        // obj: type = value;
        if (node.value_) {
            auto value = Exec(*node.value_);

            // Replace
            if (value.type() == bind_type) {
                env_.Declare(node.id_, value);
                return Obj();
            }
            
            // Try Convert Type
            auto value_convert = TypeImplTable::Convert(value, bind_type);
            if (value_convert.isNone()) {
                throw LogErr(LogModule::Runtime, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    value.type()->name_, bind_type->name_
                ), node.loc_);
            }

            // Assign Obj Clone
            auto empty = Obj::MakeEmpty(bind_type);
            bind_type->impl_->assign_(empty.Origin(), value_convert.Clone());
            env_.Declare(node.id_, empty);
            return Obj();
        }

        // obj: type;
        else {
            auto empty = Obj::MakeEmpty(bind_type);
            env_.Declare(node.id_, empty);
            return Obj();
        }
    }

    Obj Xengine::Exec(OperExpr& node) {

        // Unary
        auto lobj = Exec(*node.lexpr_);
        {
            if (node.oper_type_ == OperType::Neg) {
                auto obj = CallTry(lobj.type()->impl_->neg_, lobj);
                if (!obj.isNone()) return obj;
            }
            if (node.oper_type_ == OperType::Not) {
                auto obj = CallTry(lobj.type()->impl_->not_, lobj);
                if (!obj.isNone()) return obj;
            }
        }

        // Binary

        // Short Circuit Boolean Evaluation
        if (node.oper_type_ == OperType::And && lobj.is("bool") && !lobj.Get_bool())
            return Obj::Make_bool(false);
        if (node.oper_type_ == OperType::Or  && lobj.is("bool") &&  lobj.Get_bool())
            return Obj::Make_bool(true);

        auto robj = Exec(*node.rexpr_);
        {
            // Unnecessary Type Conversation
            {
                if (node.oper_type_ == OperType::Pick && lobj.type()->impl_->pick_) {

                    // Range
                    if (node.rexpr_->type_ != AstType::RangeExpr) {
                        auto one = TypeImplTable::Convert(Obj::Make_i32(1), robj.type());
                        robj = Obj::Make_range(new Range(robj, robj, one, true, robj.type()));
                    }

                    auto obj = lobj.type()->impl_->pick_(lobj, robj);
                    if (!obj.isNone()) return obj;
                }
            }

            // Necessary
            {
                auto common_type = sema::TypeTable::Common({ lobj.type(), robj.type() });
                if (!common_type) {
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot match type '{}' to type '{}'",
                        robj.type()->name_, lobj.type()->name_
                    ), node.loc_);
                }

                lobj = TypeImplTable::Convert(lobj, common_type);
                robj = TypeImplTable::Convert(robj, common_type);

                if (node.oper_type_ == OperType::Plus) {
                    auto obj = CallTry(common_type->impl_->plus_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Minus) {
                    auto obj = CallTry(common_type->impl_->minus_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Star) {
                    auto obj = CallTry(common_type->impl_->star_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Slash) {
                    auto obj = CallTry(common_type->impl_->slash_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::ModT) {
                    auto obj = CallTry(common_type->impl_->modt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::ModF) {
                    auto obj = CallTry(common_type->impl_->modf_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Gt) {
                    auto obj = CallTry(common_type->impl_->gt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Lt) {
                    auto obj = CallTry(common_type->impl_->lt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Ge) {
                    auto obj = CallTry(common_type->impl_->ge_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Le) {
                    auto obj = CallTry(common_type->impl_->le_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Eq) {
                    auto obj = CallTry(common_type->impl_->eq_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Neq) {
                    auto obj = CallTry(common_type->impl_->neq_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::And) {
                    auto obj = CallTry(common_type->impl_->and_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Or) {
                    auto obj = CallTry(common_type->impl_->or_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
            }
        }
        
        // Failure
        throw LogErr(LogModule::Runtime, std::format(
            "unsupported operator '{}' between '{}' and '{}'",
            OperTypeName(node.oper_type_),
            lobj.type()->name_,
            robj.type()->name_
        ), node.loc_);
    }

    Obj Xengine::Exec(RangeExpr& node) {
        
        // Boundary
        bool hasStep = node.step_ ? true : false;

        auto lobj  = Exec(*node.lexpr_);
        auto robj  = Exec(*node.rexpr_);
        auto ltype = lobj.type();
        auto rtype = robj.type();

        // Step
        Obj sobj = Obj();
        const sema::Type* stype = nullptr;
        if (hasStep) {
            sobj  = Exec(*node.step_);
            stype = sobj.type();
        }

        // Iterator
        const sema::Type* iter_type = nullptr;
        if (hasStep) {
            iter_type = sema::TypeTable::Common({ ltype, rtype, stype });
        }
        else {
            iter_type = sema::TypeTable::Common({ ltype, rtype });
        }

        if (!iter_type) {
            if (hasStep) {
                throw LogErr(LogModule::Runtime, std::format(
                    "failed to generate valid range iterator with '{}', '{}' and '{}'",
                    ltype->name_, rtype->name_, stype->name_
                ), node.loc_);
            }
            else {
                throw LogErr(LogModule::Runtime,std::format(
                    "failed to generate valid range iterator with '{}' and '{}'",
                    ltype->name_, rtype->name_
                ), node.loc_);
            }
        }

        return Obj::Make_range(new Range(
            TypeImplTable::Convert(lobj, iter_type),
            TypeImplTable::Convert(robj, iter_type),
            hasStep ? sobj : Obj::Make_i32(1),
            node.isClosed_,
            iter_type
        ));
    }

    Obj Xengine::Exec(ArrayExpr& node) {
        size_t size  = node.elements_->exprs_.size();
        auto   obj   = Obj::Make_array(size);
        auto&  array = obj.Get_array_ref();
        
        for (auto& e : node.elements_->exprs_) {

            // ref count = 1
            Obj element = Exec(*e);
            array.Insert(array.size(), new Obj(element));   // ref count = 2
        }
        // ref count = 1;

        return obj;
    }

    Obj Xengine::Exec(FnCallExpr& node) {

        // Args
        std::vector<Obj> args;
        for (auto& e : node.args_->exprs_) {
            args.emplace_back(Exec(*e));
        }

        // Callee
        auto callee = Exec(*node.callee_);
        if (callee.is("function")) {
            auto& fn = callee.Get_function_ref();

            // Cpp Lambda
            if (fn.isNative()) return CallThrow(fn.native(), args);

            // Xero AstNode
            auto& fnexpr   = fn.expr();
            auto& params   = fnexpr->params_->exprs_;
            auto  ret_type = fnexpr->ret_resolved_type_;

            Obj ret;
            try {
                // Without return, get none
                ret = Exec(*fnexpr->block_, [&]() {
                    for (size_t i = 0; i < params.size(); i++) {
                        auto param = (DeclExpr*)params[i].get();
                        auto param_type = param->resolved_type_;
                        env_.Declare(param->id_, TypeImplTable::Convert(args[i], param_type));
                    }
                });
            }
            catch (ReturnSignal e) {
                ret = *e.value;
            }

            // Check return type.
            // - has return stmt:   value must be compatible with ret type
            // - missing return:    exec() return 'none', which isn't compatible with ret type
            if (!ret_type->isNone() && ret.type()->isNone()) {
                throw LogErr(LogModule::Runtime, "missing return in function", node.loc_);
            }
            if (!ret.type()->converts_.contains(ret_type)) {
                throw LogErr(LogModule::Runtime, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    ret.type()->name_, ret_type->name_
                ), node.loc_);
            }

            return TypeImplTable::Convert(ret, ret_type);
        }

        throw LogErr(LogModule::Runtime, std::format(
            "undefined function '{}'", node.callee_->value_
        ), node.loc_);
    }

    Obj Xengine::Exec(MethodCallExpr& node) {
        auto  target = Exec(*node.target_);
        auto& callee = *node.callee_;
        auto& args   = *node.args_;

        std::vector<Obj> objs;
        objs.emplace_back(target);  // args[0] as target, args[1...n] as arguments
        for (auto& e : args.exprs_) {
            objs.emplace_back(Exec(*e));
        }

        auto fn = MethodTable::Get(target.type(), callee.value_);
        if (!fn) {
            throw LogErr(LogModule::Runtime, std::format(
                "no method '{}' on type '{}'",
                callee.value_, target.type()->name_
            ), node.loc_);
        }

        // Cpp Lambda
        if (fn->isNative()) return CallThrow(fn->native(), objs);

        // Xero AstNode
        throw LogErr(LogModule::Runtime, std::format(
            "method '{}' not implemented as script", callee.value_
        ), node.loc_);
    }

    Obj Xengine::Exec(FnExpr& node) {
        auto fn = new Function(
            std::shared_ptr<FnExpr>((FnExpr*)(node.Clone().release()))
        );
        auto obj = Obj::Make_function(fn);
        if (!node.name_.empty()) env_.Declare(node.name_, obj);
        return obj;
    }

    // Const

    Obj Xengine::Exec(NumConst& node) {
        if (node.resolved_type_->is("i32")){
            return Obj::Make_i32((int32_t)node.resolved_value_.integer);
        }
        if (node.resolved_type_->is("i64")){
            return Obj::Make_i64((int64_t)node.resolved_value_.integer);
        }
        if (node.resolved_type_->is("f32")){
            return Obj::Make_f32((float)node.resolved_value_.floating);
        }
        if (node.resolved_type_->is("f64")){
            return Obj::Make_f64((double)node.resolved_value_.floating);
        }

        if (!node.resolved_type_) {
            throw LogErr(LogModule::Runtime, "number literal not resolved", node.loc_);
        }
        throw LogErr(LogModule::Runtime, std::format(
            "number literal must be i32, i64, f32 or f64, not {}",
            node.resolved_type_->name_
        ), node.loc_);
    }

    Obj Xengine::Exec(BoolConst& node) {
        return Obj::Make_bool(node.value_);
    }

    Obj Xengine::Exec(CharConst& node) {
        return Obj::Make_char(node.value_[0]);
    }

    Obj Xengine::Exec(StringConst& node) {
        return Obj::Make_string(new String(node.value_));
    }

    // Stmt

    Obj Xengine::Exec(ExprStmt& node) {
        return Exec(*node.expr_);
    }

    Obj Xengine::Exec(AssignStmt& node) {
        auto target = Exec(*node.target_);
        auto value  = Exec(*node.value_);
        target.type()->impl_->assign_(target.Origin(), value.Clone());
        return Obj();
    }

    Obj Xengine::Exec(CondStmt& node) {

        // else
        if (!node.cond_) {
            Exec(*node.block_);
            return Obj();
        }

        // if OR elif
        auto cond = Exec(*node.cond_);
        if (cond.Get_bool())    Exec(*node.block_);
        else if (node.sub_)     Exec(*node.sub_);
        
        return Obj();
    }

    Obj Xengine::Exec(LoopSignalStmt& node) {
        throw node.signal_;
    }

    Obj Xengine::Exec(ReturnSignalStmt& node) {
        if (node.value_)
            throw ReturnSignal{ std::make_shared<Obj>(Exec(*node.value_).Clone()) };
        throw ReturnSignal{ std::make_shared<Obj>() };
    }

    Obj Xengine::Exec(ForStmt& node) {
        auto data = Exec(*node.data_);

        // Range

        if (data.is("range")) {
            auto& range = data.Get_range_ref();
            for (Obj o = *range.left(); ; o = range.iter_type()->impl_->plus_(o, *range.step())) {
                if (!range.isClosed() &&
                     range.iter_type()->impl_->ge_(o, *range.right()).Get_bool()) break;
                if ( range.isClosed() &&
                     range.iter_type()->impl_->gt_(o, *range.right()).Get_bool()) break;

                try {
                    Exec(*node.block_, [&]() {
                        env_.Declare(node.iter_->value_, o);
                    });
                }
                catch (LoopSignal e) {
                    if      (e == LoopSignal::Break) break;
                    else if (e == LoopSignal::Continue) continue;
                }
            }

            return Obj();
        }

        // Other

        size_t len = 0;
        std::function<Obj(size_t)> at;

        if      (data.is("array")) {
            auto& arr = data.Get_array_ref();
            len = arr.size();
            at = [&](size_t i) { return *arr.Get(i); };
        }      
        else if (data.is("arrayview")) {
            auto& view = data.Get_arrayview_ref();
            len = view.len();
            at = [&](size_t i) { return *view.org()->Get(view.offset() + i); };
        }
        else if (data.is("string")) {
            auto& str = data.Get_string_ref();
            len = str.size();
            at = [&](size_t i) { return *str.Get(i); };
        }  
        else if (data.is("stringview")) {
            auto& view = data.Get_stringview_ref();
            len = view.len();
            at = [&](size_t i) { return *view.org()->Get(view.offset() + i); };
        }
        else {
            throw LogErr(LogModule::Runtime, "unsupported 'for' statement", node.loc_);
        }

        for (size_t i = 0; i < len; i++) {
            try {
                Exec(*node.block_, [&]() { env_.Declare(node.iter_->value_, at(i)); });
            }
            catch (LoopSignal e) {
                if      (e == LoopSignal::Break) break;
                else if (e == LoopSignal::Continue) continue;
            }
        }
        
        return Obj();
    }

    Obj Xengine::Exec(WhileStmt& node) {
        while (true) {
            auto cond = Exec(*node.cond_);
            if (!cond.Get_bool()) break;

            try {
                Exec(*node.block_);
            }
            catch (LoopSignal e) {
                if      (e == LoopSignal::Break)    break;
                else if (e == LoopSignal::Continue) continue;
            }
        }

        return Obj();
    }

    // Common

    Obj Xengine::Exec(Program& node) {
        try {
            Exec((BlockExpr&)node, [&]() {
                BuiltinFnImplRegister();
            });
        }
        catch (LoopSignal e) {
            if      (e == LoopSignal::Break) {
                throw LogErr(LogModule::Runtime, "'break' outside of loop");
            }
            else if (e == LoopSignal::Continue) {
                throw LogErr(LogModule::Runtime, "'continue' outside of loop");
            }
        }
        catch (ReturnSignal e) {
            throw LogErr(LogModule::Runtime, "'return' outside of function");
        }
        
        return Obj();
    }
}
