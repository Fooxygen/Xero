
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <charconv>

#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"
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
        return Obj::MakeRef(env_.Get(node.value_, node.loc_));
    }

    Obj Xengine::Exec(DeclExpr& node) {
        auto bind_type = sema::TypeTable::Get(node.bind_type_);

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
                    value.type()->name, bind_type->name
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
                auto type = sema::TypeTable::Common({ lobj.type(), robj.type() });
                if (!type) {
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot match type '{}' to type '{}'",
                        robj.type()->name, lobj.type()->name
                    ), node.loc_);
                }

                lobj = TypeImplTable::Convert(lobj, type);
                robj = TypeImplTable::Convert(robj, type);

                if (node.oper_type_ == OperType::Plus) {
                    auto obj = CallTry(type->impl_->plus_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Minus) {
                    auto obj = CallTry(type->impl_->minus_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Star) {
                    auto obj = CallTry(type->impl_->star_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Slash) {
                    auto obj = CallTry(type->impl_->slash_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::ModT) {
                    auto obj = CallTry(type->impl_->modt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::ModF) {
                    auto obj = CallTry(type->impl_->modf_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Gt) {
                    auto obj = CallTry(type->impl_->gt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Lt) {
                    auto obj = CallTry(type->impl_->lt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Ge) {
                    auto obj = CallTry(type->impl_->ge_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Le) {
                    auto obj = CallTry(type->impl_->le_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Eq) {
                    auto obj = CallTry(type->impl_->eq_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Neq) {
                    auto obj = CallTry(type->impl_->neq_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::And) {
                    auto obj = CallTry(type->impl_->and_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.oper_type_ == OperType::Or) {
                    auto obj = CallTry(type->impl_->or_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
            }
        }
        
        // Failure
        throw LogErr(LogModule::Runtime, std::format(
            "unsupported operator '{}' between '{}' and '{}'",
            OperTypeName(node.oper_type_),
            lobj.type()->name,
            robj.type()->name
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
                    ltype->name, rtype->name, stype->name
                ), node.loc_);
            }
            else {
                throw LogErr(LogModule::Runtime,std::format(
                    "failed to generate valid range iterator with '{}' and '{}'",
                    ltype->name, rtype->name
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
        auto& callee_name = node.callee_->value_;

        std::vector<Obj> args;
        std::vector<const sema::Type*> args_type;
        for (auto& e : node.args_->exprs_) {
            args.emplace_back(Exec(*e));
            args_type.emplace_back(args[args.size() - 1].type());
        }


        // Build-in Function
        if (auto fn = FnTable::Get(callee_name)) {
            return CallThrow(fn, args);
        }

        // Obj-Stored Function
        else {
            auto callee = Exec(*node.callee_);
            if (callee.is("function")) {
                auto& fn = callee.Get_function_ref();
                fn.Check(callee_name, args_type);

                auto& fnexpr   = fn.expr();
                auto& params   = fnexpr->params_->exprs_;
                auto  ret_type = sema::TypeTable::Get(fnexpr->ret_type_);

                Obj ret;
                try {
                    // Without return, get none
                    ret = Exec(*fnexpr->block_, [&]() {
                        for (size_t i = 0; i < params.size(); i++) {
                            auto param = (DeclExpr*)params[i].get();
                            auto param_type = sema::TypeTable::Get(param->bind_type_);
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
                if (!ret.type()->converts.contains(ret_type)) {
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with '{}'",
                        ret.type()->name, ret_type->name
                    ), node.loc_);
                }

                return TypeImplTable::Convert(ret, ret_type);
            }

            throw LogErr(LogModule::Runtime, std::format(
                "undefined function '{}'",
                node.callee_->value_
            ));
        }
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

        return CallThrow(MethodTable::Get(target.type(), callee.value_), objs);
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
        const auto& numstr = node.value_;

        // integer
        if (!numstr.contains(".")) {
            
            // i32
            {
                int32_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_i32(x);
            }

            // i64
            {
                int64_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_i64(x);
            }
        }

        // float
        else {
            // error: 3.14.15
            if (numstr.substr(numstr.find(".") + 1).contains(".")) {
                throw LogErr(LogModule::Runtime, std::format(
                    "invalid float format '{}'", numstr
                ), node.loc_);
            }

            // f32
            {
                float x = 0.0f;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_f32(x);
            }

            // f64
            {
                double x = 0.0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_f64(x);
            }
        }

        throw LogErr(LogModule::Runtime, std::format(
            "numeric overflow '{}'", numstr
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
        bool isPass = false;
        if (!node.cond_) isPass = true;
        else {
            auto cond = Exec(*node.cond_);
            if (cond.is("bool")) {
                isPass = cond.Get_bool();
            }
            else {
                throw LogErr(LogModule::Runtime, std::format(
                    "condition must be bool, not {}",
                    cond.type()->name
                ), node.loc_);
            }
        }

        if (isPass && node.block_) Exec(*node.block_);
        else if (node.sub_)        Exec(*node.sub_);
        
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
            if (!cond.is("bool")) {
                throw LogErr(LogModule::Runtime, std::format(
                    "condition must be bool, not {}",
                    cond.type()->name
                ), node.loc_);
            }

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
            Exec((BlockExpr&)node);
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
