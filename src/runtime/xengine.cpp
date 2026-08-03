
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <charconv>

#include "log.hpp"
#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"
#include "common/opertype.hpp"
#include "common/loop.hpp"

namespace rt {
    
    // Expr

    Obj Xengine::Exec(IdExpr& node) {
        return Obj::MakeRef(env_.Get(node.value_));
    }

    Obj Xengine::Exec(OperExpr& node) {

        // Unary
        auto lobj = Exec(*node.lexpr_);
        {
            if (node.opertype_ == OperType::Neg) {
                return CallTry(lobj.type()->neg_, lobj);
            }
            if (node.opertype_ == OperType::Not) {
                return CallTry(lobj.type()->not_, lobj);
            }
        }

        // Binary

        // Short Circuit Boolean Evaluation
        if (node.opertype_ == OperType::And && lobj.is("bool") && !lobj.Get_bool())
            return Obj::Make_bool(false);
        if (node.opertype_ == OperType::Or  && lobj.is("bool") &&  lobj.Get_bool())
            return Obj::Make_bool(true);

        auto robj = Exec(*node.rexpr_);
        {
            // Unnecessary Type Conversation
            {
                if (node.opertype_ == OperType::Pick) {

                    // Range
                    if (node.rexpr_->type_ != AstType::RangeExpr) {
                        auto one = TypeTable::Convert(Obj::Make_i32(1), robj.type());
                        robj = Obj::Make_range(new Range(robj, robj, one, true, robj.type()));
                    }

                    auto obj = lobj.type()->pick_(lobj, robj);
                    if (!obj.isNone()) return obj;
                }
            }

            // Necessary
            {
                auto type = TypeTable::Common({ lobj.type(), robj.type() });
                if (!type) {
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot match type '{}' to type '{}'",
                        robj.type()->name, lobj.type()->name
                    ));
                }

                lobj = TypeTable::Convert(lobj, type);
                robj = TypeTable::Convert(robj, type);

                if (node.opertype_ == OperType::Plus) {
                    auto obj = CallTry(type->plus_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Minus) {
                    auto obj = CallTry(type->minus_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Star) {
                    auto obj = CallTry(type->star_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Slash) {
                    auto obj = CallTry(type->slash_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Gt) {
                    auto obj = CallTry(type->gt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Lt) {
                    auto obj = CallTry(type->lt_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Ge) {
                    auto obj = CallTry(type->ge_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Le) {
                    auto obj = CallTry(type->le_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Eq) {
                    auto obj = CallTry(type->eq_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Neq) {
                    auto obj = CallTry(type->neq_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::And) {
                    auto obj = CallTry(type->and_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
                if (node.opertype_ == OperType::Or) {
                    auto obj = CallTry(type->or_, lobj, robj);
                    if (!obj.isNone()) return obj;
                }
            }
        }
        
        // Failure
        throw LogErr(LogModule::Runtime, std::format(
            "unsupported operator '{}' between '{}' and '{}'",
            OperTypeName(node.opertype_),
            lobj.type()->name,
            robj.type()->name
        ));
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
        const Type* stype = nullptr;
        if (hasStep) {
            sobj  = Exec(*node.step_);
            stype = sobj.type();
        }

        // Iterator
        const Type* itertype = nullptr;
        if (hasStep) {
            itertype = TypeTable::Common({ ltype, rtype, stype });
        }
        else {
            itertype = TypeTable::Common({ ltype, rtype });
        }

        if (!itertype) {
            if (hasStep) {
                throw LogErr(LogModule::Runtime,
                    std::format(
                        "failed to generate valid range iterator with '{}', '{}' and '{}'",
                        ltype->name, rtype->name, stype->name
                    )
                );
            }
            else {
                throw LogErr(LogModule::Runtime,
                    std::format(
                        "failed to generate valid range iterator with '{}' and '{}'",
                        ltype->name, rtype->name
                    )
                );
            }
        }

        return Obj::Make_range(new Range(
            TypeTable::Convert(lobj, itertype),
            TypeTable::Convert(robj, itertype),
            hasStep ? sobj : Obj::Make_i32(1),
            node.isClosed_,
            itertype
        ));
    }

    Obj Xengine::Exec(FnCallExpr& node) {
        auto& callee = *node.callee_;
        auto& args   = *node.args_;

        std::vector<Obj> objs;
        for (auto& e : args.exprs_) {
            objs.emplace_back(Exec(*e));
        }

        return CallThrow(FnTable::Get(callee.value_), objs);
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
                throw LogErr(LogModule::Runtime, std::format("invalid float format '{}'", numstr));
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

        throw LogErr(LogModule::Runtime, std::format("numeric overflow '{}'", numstr));
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

    Obj Xengine::Exec(BlockStmt& node, std::function<void()> OnScopeReady) {
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

    Obj Xengine::Exec(DeclStmt& node) {
        auto type  = TypeTable::Get(node.value_type_->value_);

        // obj: type = value;
        if (node.value_) {
            auto value = Exec(*node.value_);

            // Replace
            if (value.type() == type) {
                env_.Declare(node.id_->value_, value);
                return Obj();
            }
            
            // Try Convert Type
            auto value_convert = TypeTable::Convert(value, type);
            if (value_convert.isNone()) {
                throw LogErr(LogModule::Runtime, std::format(
                    "cannot make type '{}' compatible with '{}'",
                    value.type()->name, type->name
                ));
            }

            // Assign Obj Clone
            auto empty = Obj::MakeEmpty(type);
            type->assign_(empty.Origin(), value_convert.Clone());
            env_.Declare(node.id_->value_, empty);
            return Obj();
        }

        // obj: type;
        else {
            auto empty = Obj::MakeEmpty(type);
            env_.Declare(node.id_->value_, empty);
            return Obj();
        }
    }

    Obj Xengine::Exec(AssignStmt& node) {
        auto target = Exec(*node.target_);
        auto value  = Exec(*node.value_);
        target.type()->assign_(target.Origin(), value.Clone());
        return Obj();
    }

    Obj Xengine::Exec(CondStmt& node) {
        bool isPass = false;
        if (!node.cond_) isPass = true;
        else {
            auto cond = Exec(*node.cond_);
            if (cond.type() == TypeTable::Get("bool")) {
                isPass = cond.Get_bool();
            }
            else {
                throw LogErr(LogModule::Runtime, std::format(
                    "condition must be bool, not {}",
                    cond.type()->name
                ));
            }
        }

        if (isPass && node.block_) Exec(*node.block_);
        else if (node.sub_)        Exec(*node.sub_);
        
        return Obj();
    }

    Obj Xengine::Exec(LoopSignalStmt& node) {
        throw node.status_;
    }

    Obj Xengine::Exec(ForStmt& node) {
        auto data = Exec(*node.data_);
        auto type = data.type();

        // Range

        if (type == TypeTable::Get("range")) {
            auto& range = data.Get_range_ref();
            for (Obj o = *range.left(); ; o = range.itertype()->plus_(o, *range.step())) {
                if (!range.isClosed() &&
                     range.itertype()->ge_(o, *range.right()).Get_bool()) break;
                if ( range.isClosed() &&
                     range.itertype()->gt_(o, *range.right()).Get_bool()) break;

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

        if      (type == TypeTable::Get("array")) {
            auto& arr = data.Get_array_ref();
            len = arr.size();
            at = [&](size_t i) { return *arr.Get(i); };
        }      
        else if (type == TypeTable::Get("arrayview")) {
            auto& view = data.Get_arrayview_ref();
            len = view.len();
            at = [&](size_t i) { return *view.org()->Get(view.offset() + i); };
        }
        else if (type == TypeTable::Get("string")) {
            auto& str = data.Get_string_ref();
            len = str.size();
            at = [&](size_t i) { return *str.Get(i); };
        }  
        else if (type == TypeTable::Get("stringview")) {
            auto& view = data.Get_stringview_ref();
            len = view.len();
            at = [&](size_t i) { return *view.org()->Get(view.offset() + i); };
        }
        else {
            throw LogErr(LogModule::Runtime, "unsupported 'for' statement");
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

    // Common

    Obj Xengine::Exec(Program& node) {
        try {
            Exec((BlockStmt&)node);
        }
        catch (LoopSignal e) {
            if      (e == LoopSignal::Break) {
                throw LogErr(LogModule::Runtime, "'break' outside of loop");
            }
            else if (e == LoopSignal::Continue) {
                throw LogErr(LogModule::Runtime, "'continue' outside of loop");
            }
        }
        
        return Obj();
    }
}
