
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <charconv>

#include "log.hpp"
#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"

namespace rt {
    
    // Exec

    // └─ Expr

    Obj Xengine::Exec(IdExpr& node) {
        return *env_.Get(node.value_);
    }

    Obj Xengine::Exec(OperExpr& node) {
        auto lobj = Exec(*node.lexpr_);
        auto robj = Exec(*node.rexpr_);
        auto type = TypeTable::Common({ lobj.type(), robj.type() });

        if (!type) {
            throw LogErr(LogModule::Runtime, std::format(
                "incompatible types '{}' and '{}'",
                lobj.type()->name, robj.type()->name
            ));
        }

        lobj = TypeTable::Convert(lobj, type);
        robj = TypeTable::Convert(robj, type);
        
        auto method_pick = [&](const Type* t) -> Obj(*)(const Obj&, const Obj&) {
            using enum Token::Type;
            switch (node.opertype_) {
                case Plus:  return t->plus;
                case Minus: return t->minus;
                case Star:  return t->star;
                case Slash: return t->slash;
                case Gt:    return t->gt;
                case Lt:    return t->lt;
                case Ge:    return t->ge;
                case Le:    return t->le;
                case Eq:    return t->eq;
                case Neq:   return t->neq;
                case And:   return t->and_;
                case Or:    return t->or_;
                default:                 return nullptr;
            }
        };

        auto obj = CallTry(method_pick(type), lobj, robj);
        if (!obj.isNone()) return obj;

        throw LogErr(LogModule::Runtime, std::format(
            "unsupported operator '{}' between '{}' and '{}'",
            Token::TypeName(node.opertype_),
            lobj.type()->name,
            robj.type()->name
        ));
    }

    Obj Xengine::Exec(PickExpr& node) {
        auto target = Exec(*node.target_);

        // Slice by range
        if (node.pick_->type_ == AstType::RangeExpr) {
            if (!target.type()->slice_clone) {
                throw LogErr(LogModule::Runtime, std::format(
                    "unsupported 'slice' in operator '[]' for '{}'", target.type()->name
                ));
            }

            auto [itertype, rangetype, l, r, s] = Exec((RangeExpr&)*node.pick_);

            if (itertype != TypeTable::Get("i32") &&
                itertype != TypeTable::Get("i64"))
            {
                throw LogErr(LogModule::Runtime, std::format(
                    "incompatible type '{}' as iterator type for '[]'", itertype->name
                ));
            }
            
            return target.type()->slice_clone(
                target, itertype, rangetype == Token::Type::DotDotEq, l, r, s
            );
        }
        
        // At by index
        else {
            if (!target.type()->at_clone) {
                throw LogErr(LogModule::Runtime, std::format(
                    "unsupported 'at' in operator '[]' for '{}'", node.pick_->TypeName()
                ));
            }

            auto idx = Exec(*node.pick_);

            if (idx.type() != TypeTable::Get("i32") &&
                idx.type() != TypeTable::Get("i64"))
            {
                throw LogErr(LogModule::Runtime, std::format(
                    "incompatible type '{}' as iterator type for '[]'", idx.type()->name
                ));
            }

            return target.type()->at_clone(target, idx);
        }
    }

    std::tuple<const Type*, Token::Type, Obj, Obj, Obj>
        Xengine::Exec(RangeExpr& node)
    {
        
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

        return std::make_tuple(
            itertype,
            node.rangetype_,
            TypeTable::Convert(lobj, itertype),
            TypeTable::Convert(robj, itertype),
            hasStep ? sobj : Obj::Make_i32(1)
        );
    }
    
    Obj Xengine::Exec(NegExpr& node) {
        auto obj = Exec(*node.expr_);
        auto neg = CallTry(obj.type()->neg, obj);
        if (neg.isNone()) {
            throw LogErr(LogModule::Runtime, std::format(
                "unsupported '-' for '{}'",
                obj.type()->name
            ));
        }
        return neg;
    }

    Obj Xengine::Exec(NotExpr& node) {
        auto obj = Exec(*node.expr_);
        auto not_ = CallTry(obj.type()->not_, obj);
        if (not_.isNone()) {
            throw LogErr(LogModule::Runtime, std::format(
                "unsupported '!' for '{}'",
                obj.type()->name
            ));
        }
        return not_;
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

    // └─ Const

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

    Obj Xengine::Exec(StringConst& node) {
        return Obj::Make_string(new String(node.value_));
    }

    // └─ Stmt

    Obj Xengine::Exec(BlockStmt& node, std::function<void()> OnScopeReady) {
        env_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        for (auto& child : node.children_) Exec(*child);
        
        env_.ScopePop();
        return Obj();
    }

    Obj Xengine::Exec(DeclStmt& node) {
        auto obj  = Exec(*node.value_);
        auto type = TypeTable::Get(node.value_type_->value_);

        // Try Convert Type
        auto obj_convert = TypeTable::Convert(obj, type);
        if (obj_convert.isNone()) {
            throw LogErr(LogModule::Runtime, std::format(
                "cannot assign type '{}' to type '{}'",
                obj.type()->name, type->name
            ));
        }

        env_.Declare(node.id_->value_, obj_convert);
        return Obj();
    }

    Obj Xengine::Exec(AssignStmt& node) {
        auto target = Origin(*node.target_);
        auto value  = Exec(*node.value_);

        target->type()->assign(target, value);
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
            else throw LogErr(LogModule::Runtime, "condition must be bool");
        }

        if (isPass && node.block_) Exec(*node.block_);
        else if (node.sub_)        Exec(*node.sub_);
        
        return Obj();
    }

    Obj Xengine::Exec(ForStmt& node) {
        bool isCatch = false;

        // Range
        if (node.data_->type_ == AstType::RangeExpr) {

            auto [itertype, rangetype, l, r, s] = Exec((RangeExpr&)*node.data_);
            bool isEqRightBoundary = rangetype == Token::Type::DotDotEq;

            // Execute
            for (Obj o = l; ; o = itertype->plus(o, s)) {
                if (!isEqRightBoundary) {
                    if (itertype->ge(o, r).Get_bool()) break;
                }
                else {
                    if (itertype->gt(o, r).Get_bool()) break;
                }

                Exec(*node.block_, [&]() {
                    env_.Declare(node.iter_->value_, o);
                });
            }

            isCatch = true;
        }

        else {
            auto obj  = Exec(*node.data_);
            auto type = obj.type();

            // Array
            if (type->name == "array") {
                auto& array = obj.Get_array_ref();

                for (size_t i = 0; i < array.size(); i++) {
                    Exec(*node.block_, [&]() {
                        env_.Declare(node.iter_->value_, *array.Get(i));
                    });
                }

                isCatch = true;
            }
        }

        if (!isCatch) {
            throw LogErr(LogModule::Runtime, "unsupported 'for' statement");
        }
        return Obj();
    }

    // └─ Common

    Obj Xengine::Exec(Program& node) {
        Exec((BlockStmt&)node);
        return Obj();
    }

    // Origin

    // └─ Expr

    Obj* Xengine::Origin(IdExpr& node) {
        return env_.Get(node.value_);
    }

    Obj* Xengine::Origin(PickExpr& node) {
        auto target = Origin(*node.target_);

        // Slice by range
        if (node.pick_->type_ == AstType::RangeExpr) {

            auto [itertype, rangetype, l, r, s] = Exec((RangeExpr&)*node.pick_);
            if (itertype != TypeTable::Get("i32") &&
                itertype != TypeTable::Get("i64"))
            {
                throw LogErr(LogModule::Runtime, std::format(
                    "incompatible type '{}' as iterator type for '[]'", itertype->name
                ));
            }
            
            if (!target->type()->slice_ref) {
                throw LogErr(LogModule::Runtime, std::format(
                    "unsupported 'slice' in operator '[]' for '{}'", target->type()->name
                ));
            }
            return target->type()->slice_ref(
                    *target, itertype, rangetype == Token::Type::DotDotEq, l, r, s
            );
        }
        
        // At by index
        else {

            auto idx = Exec(*node.pick_);
            if (idx.type() != TypeTable::Get("i32") &&
                idx.type() != TypeTable::Get("i64"))
            {
                throw LogErr(LogModule::Runtime, std::format(
                    "incompatible type '{}' as iterator type for '[]'", idx.type()->name
                ));
            }

            if (!target->type()->at_ref) {
                throw LogErr(LogModule::Runtime, std::format(
                    "unsupported 'at' in operator '[]' for '{}'", node.pick_->TypeName()
                ));
            }
            
            return target->type()->at_ref(*target, idx);
        }
    }
}
