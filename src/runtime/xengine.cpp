
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <charconv>

#include "log.hpp"
#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"

namespace rt {
        
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

        throw LogErr(LogModule::Runtime,
            std::format("unsupported operator '{}'", Token::TypeName(node.opertype_)));
    }

    Obj Xengine::Exec(NegExpr& node) {
        auto obj = Exec(*node.expr_);
        auto neg = CallTry(obj.type()->neg, obj);
        if (neg.isNone()) {
            throw LogErr(LogModule::Runtime, "unsupported unary negation");
        }
        return neg;
    }

    Obj Xengine::Exec(NotExpr& node) {
        auto obj = Exec(*node.expr_);
        auto not_ = CallTry(obj.type()->not_, obj);
        if (not_.isNone()) {
            throw LogErr(LogModule::Runtime, "unsupported bool negation");
        }
        return not_;
    }

    Obj Xengine::Exec(FnCallExpr& node) {
        auto& callee = *node.callee();

        std::vector<Obj> args;
        for (auto& arg : node.arglist()->args()) {
            args.emplace_back(Exec(*arg));
        }

        return CallThrow(FnTable::Get(callee.value_), args);
    }

    Obj Xengine::Exec(MethodCallExpr& node) {
        auto  target = Exec(*node.target());
        auto& callee = *node.callee();

        std::vector<Obj> args;
        args.emplace_back(target);  // args[0] as target, args[1...n] as arguments
        for (auto& arg : node.arglist()->args()) {
            args.emplace_back(Exec(*arg));
        }

        return CallThrow(MethodTable::Get(target.type(), callee.value_), args);
    }

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
        return Obj::Make_string(node.value_);
    }

    Obj Xengine::Exec(BlockStmt& node, std::function<void()> OnScopeReady) {
        env_.ScopePush();
        if (OnScopeReady) OnScopeReady();

        for (auto& child : node.children()) {
            Exec(*child);
        }
        
        env_.ScopePop();
        return Obj();
    }

    Obj Xengine::Exec(DeclStmt& node) {
        auto value = Exec(*node.value_);
        //auto type = TypeTable::Get(node.id_->value_);
        env_.Declare(node.id_->value_, value);
        return Obj();
    }

    Obj Xengine::Exec(AssignStmt& node) {
        env_.Assign(node.id_->value_, Exec(*node.value_));
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

        if (isPass && node.block_)
            Exec(*node.block_);
        else if (node.sub_)
            Exec(*node.sub_);
        
        return Obj();
    }

    Obj Xengine::Exec(ForStmt& node) {
        if (node.data_->type_ == AstType::RangeExpr) {

            // Boundary
            auto range   = (RangeExpr*)node.data_.get();
            bool hasStep = range->step_ ? true : false;

            auto lobj  = Exec(*range->lexpr_);
            auto robj  = Exec(*range->rexpr_);
            auto ltype = lobj.type();
            auto rtype = robj.type();

            // Step
            Obj sobj = Obj();
            const Type* stype = nullptr;
            if (hasStep) {
                sobj  = Exec(*range->step_);
                stype = sobj.type();
            }

            // Iterator
            const Type* itype = nullptr;
            if (hasStep) {
                itype = TypeTable::Common({ ltype, rtype, stype });
            }
            else {
                itype = TypeTable::Common({ ltype, rtype });
            }

            if (!itype) {
                if (hasStep) {
                    throw LogErr(LogModule::Runtime,
                        std::format(
                            "range failed to produce a valid iterator with '{}', '{}' and '{}'",
                            ltype->name, rtype->name, stype->name
                        )
                    );
                }
                else {
                    throw LogErr(LogModule::Runtime,
                        std::format(
                            "range failed to produce a valid iterator with '{}' and '{}'",
                            ltype->name, rtype->name
                        )
                    );
                }
            }
            if (!itype->plus || !itype->ge) {
                throw LogErr(LogModule::Runtime, std::format(
                        "invalid range iterator type '{}'",
                        itype->name
                    )
                );
            }

            // Convert Type
            lobj = TypeTable::Convert(lobj, itype);
            robj = TypeTable::Convert(robj, itype);
            Obj step = hasStep ? sobj : Obj::Make_i32(1);

            // Execute
            for (Obj o = lobj; ; o = itype->plus(o, step)) {
                if (range->rangetype_ == Token::Type::DotDot) {
                    if (itype->ge(o, robj).Get_bool()) break;
                }
                else {
                    if (itype->gt(o, robj).Get_bool()) break;
                }

                Exec(*node.block_, [&]() {
                    env_.Declare(node.iter_->value_, o);
                });
            }
        }

        return Obj();
    }

    Obj Xengine::Exec(Program& node) {
        Exec((BlockStmt&)node);
        return Obj();
    }
}
