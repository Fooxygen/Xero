
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <charconv>

#include "log.hpp"
#include "xengine.hpp"

namespace rt {
        
    void Xengine::OperTableRegister() {
        using TT = Token::Type;

        // i32
        {
            OperTable::Set(TypeTable::Get("i32"), TT::Plus, [](const Obj& a, const Obj& b) {
                if (b.is("i32")) return Obj::Make_i32(a.Get_i32() + b.Get_i32());
                return Obj();    // unsupported
            });
            OperTable::Set(TypeTable::Get("i32"), TT::Minus, [](const Obj& a, const Obj& b) {
                if (b.is("i32")) return Obj::Make_i32(a.Get_i32() - b.Get_i32());
                return Obj();
            });
            OperTable::Set(TypeTable::Get("i32"), TT::Star, [](const Obj& a, const Obj& b) {
                if (b.is("i32")) return Obj::Make_i32(a.Get_i32() * b.Get_i32());
                return Obj();
            });
            OperTable::Set(TypeTable::Get("i32"), TT::Slash, [](const Obj& a, const Obj& b) {
                int32_t xb = 0;
                if (b.is("i32")) xb = b.Get_i32();
                else return Obj();

                if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                return Obj::Make_i32(a.Get_i32() / xb);
            });
        }

        // i64
        {
            OperTable::Set(TypeTable::Get("i64"), TT::Plus, [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                     if (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_i64(a.Get_i64() + xb);
            });
            OperTable::Set(TypeTable::Get("i64"), TT::Minus, [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                     if (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_i64(a.Get_i64() - xb);
            });
            OperTable::Set(TypeTable::Get("i64"), TT::Star, [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                     if (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_i64(a.Get_i64() * xb);
            });
            OperTable::Set(TypeTable::Get("i64"), TT::Slash, [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                     if (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();

                if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                return Obj::Make_i64(a.Get_i64() / xb);
            });
        }

        // f32
        {
            OperTable::Set(TypeTable::Get("f32"), TT::Plus, [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                     if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() + xb);
            });
            OperTable::Set(TypeTable::Get("f32"), TT::Minus, [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                     if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() - xb);
            });
            OperTable::Set(TypeTable::Get("f32"), TT::Star, [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                     if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() * xb);
            });
            OperTable::Set(TypeTable::Get("f32"), TT::Slash, [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                     if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() / xb);
            });
        }

        // f64
        {
            OperTable::Set(TypeTable::Get("f64"), TT::Plus, [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                     if (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() + xb);
            });
            OperTable::Set(TypeTable::Get("f64"), TT::Minus, [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                     if (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() - xb);
            });
            OperTable::Set(TypeTable::Get("f64"), TT::Star, [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                     if (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() * xb);
            });
            OperTable::Set(TypeTable::Get("f64"), TT::Slash, [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                     if (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() / xb);
            });
        }
    }
        
    Obj Xengine::Exec(IdExpr& node) {
        return *env_.Get(node.value_);
    }

    Obj Xengine::Exec(OperExpr& node) {
        auto lobj = Exec(*node.lexpr_);
        auto robj = Exec(*node.rexpr_);

        auto func = OperTable::Get(lobj.type(), node.opertype_);
        if (func) {
            auto obj = func(lobj, robj);
            if (!obj.isNone()) return obj;
        }

        func = OperTable::Get(robj.type(), node.opertype_);
        if (func) {
            auto obj = func(robj, lobj);
            if (!obj.isNone()) return obj;
        }

        throw LogErr(LogModule::Runtime, "unsupported operator");
    }

    Obj Xengine::Exec(NegExpr& node) {
        auto obj = Exec(*node.expr_);

        if (obj.is("i32"))  return Obj::Make_i32(-obj.Get_i32());
        if (obj.is("i64"))  return Obj::Make_i64(-obj.Get_i64());
        if (obj.is("f32"))  return Obj::Make_f32(-obj.Get_f32());
        if (obj.is("f64"))  return Obj::Make_f64(-obj.Get_f64());

        throw LogErr(LogModule::Runtime, "unsupported unary negation");
    }

    Obj Xengine::Exec(NumConst& node) {
        const auto& numstr = node.value_str_;

        // integer
        if (numstr.find(".") == std::string::npos) {
            
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

        throw LogErr(LogModule::Runtime, "numeric overflow");
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

    Obj Xengine::Exec(Program& node) {
        for (auto& child : node.children()) {
            Exec(*child);
        }
        return Obj();
    }
}
