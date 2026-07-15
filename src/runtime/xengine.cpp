
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

        auto pick = [&](const Type* t) -> Obj(*)(const Obj&, const Obj&) {
            switch (node.opertype_) {
                case Token::Type::Plus:  return t->plus;
                case Token::Type::Minus: return t->minus;
                case Token::Type::Star:  return t->star;
                case Token::Type::Slash: return t->slash;
                case Token::Type::Gt:    return t->gt;
                case Token::Type::Lt:    return t->lt;
                case Token::Type::Ge:    return t->ge;
                case Token::Type::Le:    return t->le;
                case Token::Type::Eq:    return t->eq;
                case Token::Type::Neq:   return t->neq;
                case Token::Type::And:   return t->and_;
                case Token::Type::Or:    return t->or_;
                default:                 return nullptr;
            }
        };

        auto obj = CallTry(pick(lobj.type()), lobj, robj);
        if (!obj.isNone()) return obj;

        obj = CallTry(pick(robj.type()), robj, lobj);
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

    Obj Xengine::Exec(BlockStmt& node) {
        for (auto& child : node.children()) {
            Exec(*child.get());
        }
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

    Obj Xengine::Exec(Program& node) {
        for (auto& child : node.children()) {
            Exec(*child);
        }
        return Obj();
    }
}
