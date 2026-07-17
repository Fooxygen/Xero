
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"

namespace rt {

    void Xengine::TypeRegister() {

        // none
        TypeTable::Set(Type{
            .name = "none", .size = 0
        });

        // bool
        TypeTable::Set(Type{
            .name = "bool", .size = 1,
            .to_string  = [](const Obj& o) {
                return o.Get_bool() ? std::string("true") : std::string("false");
            },
            .gt   = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() >  r.Get_bool()); },
            .lt   = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() <  r.Get_bool()); },
            .ge   = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() >= r.Get_bool()); },
            .le   = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() <= r.Get_bool()); },
            .eq   = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() == r.Get_bool()); },
            .neq  = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() != r.Get_bool()); },
            .and_ = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() && r.Get_bool()); },
            .or_  = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() || r.Get_bool()); },
            .not_ = [](const Obj& o) { return Obj::Make_bool(!o.Get_bool()); },
        });

        // i32
        TypeTable::Set(Type{
            .name  = "i32", .size = 4,
            .reach = { "i64", "f32", "f64" },
            .to_string  = [](const Obj& o) { return std::to_string(o.Get_i32()); },
            .plus       = [](const Obj& a, const Obj& b) {
                if (b.is("i32")) return Obj::Make_i32(a.Get_i32() + b.Get_i32());
                return Obj();
            },
            .minus      = [](const Obj& a, const Obj& b) {
                if (b.is("i32")) return Obj::Make_i32(a.Get_i32() - b.Get_i32());
                return Obj();
            },
            .star       = [](const Obj& a, const Obj& b) {
                if (b.is("i32")) return Obj::Make_i32(a.Get_i32() * b.Get_i32());
                return Obj();
            },
            .slash      = [](const Obj& a, const Obj& b) {
                int32_t xb = 0;
                if (b.is("i32")) xb = b.Get_i32();
                else return Obj();
                if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                return Obj::Make_i32(a.Get_i32() / xb);
            },
            .neg        = [](const Obj& o) {
                return Obj::Make_i32(-o.Get_i32());
            },
            .gt         = [](const Obj& a, const Obj& b) { if (!b.is("i32")) return Obj(); return Obj::Make_bool(a.Get_i32() >  b.Get_i32()); },
            .lt         = [](const Obj& a, const Obj& b) { if (!b.is("i32")) return Obj(); return Obj::Make_bool(a.Get_i32() <  b.Get_i32()); },
            .ge         = [](const Obj& a, const Obj& b) { if (!b.is("i32")) return Obj(); return Obj::Make_bool(a.Get_i32() >= b.Get_i32()); },
            .le         = [](const Obj& a, const Obj& b) { if (!b.is("i32")) return Obj(); return Obj::Make_bool(a.Get_i32() <= b.Get_i32()); },
            .eq         = [](const Obj& a, const Obj& b) { if (!b.is("i32")) return Obj(); return Obj::Make_bool(a.Get_i32() == b.Get_i32()); },
            .neq        = [](const Obj& a, const Obj& b) { if (!b.is("i32")) return Obj(); return Obj::Make_bool(a.Get_i32() != b.Get_i32()); },
        });

        // i64
        TypeTable::Set(Type{
            .name  = "i64", .size = 8,
            .reach = { "f32", "f64" },
            .to_string  = [](const Obj& o) { return std::to_string(o.Get_i64()); },
            .plus       = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_i64(a.Get_i64() + xb);
            },
            .minus      = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_i64(a.Get_i64() - xb);
            },
            .star       = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_i64(a.Get_i64() * xb);
            },
            .slash      = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                return Obj::Make_i64(a.Get_i64() / xb);
            },
            .neg        = [](const Obj& o) {
                return Obj::Make_i64(-o.Get_i64());
            },
            .gt         = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_bool(a.Get_i64() > xb);
            },
            .lt         = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_bool(a.Get_i64() < xb);
            },
            .ge         = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_bool(a.Get_i64() >= xb);
            },
            .le         = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_bool(a.Get_i64() <= xb);
            },
            .eq         = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_bool(a.Get_i64() == xb);
            },
            .neq        = [](const Obj& a, const Obj& b) {
                int64_t xb = 0;
                if      (b.is("i64")) xb = b.Get_i64();
                else if (b.is("i32")) xb = b.Get_i32();
                else                  return Obj();
                return Obj::Make_bool(a.Get_i64() != xb);
            },
        });

        // f32
        TypeTable::Set(Type{
            .name  = "f32", .size = 4,
            .reach = { "f64" },
            .to_string  = [](const Obj& o) { return std::to_string(o.Get_f32()); },
            .plus       = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() + xb);
            },
            .minus      = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() - xb);
            },
            .star       = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() * xb);
            },
            .slash      = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_f32(a.Get_f32() / xb);
            },
            .neg        = [](const Obj& o) {
                return Obj::Make_f32(-o.Get_f32());
            },
            .gt         = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f32() > xb);
            },
            .lt         = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f32() < xb);
            },
            .ge         = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f32() >= xb);
            },
            .le         = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f32() <= xb);
            },
            .eq         = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f32() == xb);
            },
            .neq        = [](const Obj& a, const Obj& b) {
                float xb = 0.0f;
                if      (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i32"))  xb = (float)b.Get_i32();
                else if (b.is("i64"))  xb = (float)b.Get_i64();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f32() != xb);
            },
        });

        // f64
        TypeTable::Set(Type{
            .name = "f64", .size = 8,
            .to_string  = [](const Obj& o) { return std::to_string(o.Get_f64()); },
            .plus       = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() + xb);
            },
            .minus      = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() - xb);
            },
            .star       = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() * xb);
            },
            .slash      = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_f64(a.Get_f64() / xb);
            },
            .neg        = [](const Obj& o) {
                return Obj::Make_f64(-o.Get_f64());
            },
            .gt         = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f64() > xb);
            },
            .lt         = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f64() < xb);
            },
            .ge         = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f64() >= xb);
            },
            .le         = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f64() <= xb);
            },
            .eq         = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f64() == xb);
            },
            .neq        = [](const Obj& a, const Obj& b) {
                double xb = 0.0;
                if      (b.is("f64"))  xb = b.Get_f64();
                else if (b.is("f32"))  xb = b.Get_f32();
                else if (b.is("i64"))  xb = (double)b.Get_i64();
                else if (b.is("i32"))  xb = (double)b.Get_i32();
                else                   return Obj();
                return Obj::Make_bool(a.Get_f64() != xb);
            },
        });

        // string
        TypeTable::Set(Type{
            .name = "string", .size = 0, .isRef = true,
            .destroy   = [](void* data) { delete (String*)data; },
            .to_string = [](const Obj& o) { return o.Get_string().ToCppString(); },
            .plus      = [](const Obj& a, const Obj& b) {
                return Obj::Make_string((a.Get_string() + b.Get_string()).ToCppString());
            },
            .neg       = [](const Obj& o) {
                auto s = o.Get_string();
                s.Reverse();
                return Obj::Make_string(s.ToCppString());
            },
            .eq        = [](const Obj& a, const Obj& b) {
                if (!b.is("string")) return Obj();
                return Obj::Make_bool(a.Get_string().ToCppString() == b.Get_string().ToCppString());
            },
            .neq       = [](const Obj& a, const Obj& b) {
                if (!b.is("string")) return Obj();
                return Obj::Make_bool(a.Get_string().ToCppString() != b.Get_string().ToCppString());
            },        
        });
    }

    void Xengine::FnRegister() {
        using ARGS = std::vector<Obj>&;
        
        // type
        {
            FnTable::Set("type", [](ARGS args) {
                return Obj::Make_string(
                    std::string(args[0].type()->name)
                );
            });
        }

        // print and println
        {
            static const auto impl = [](ARGS args) {
                for (size_t i = 0; i < args.size(); i++) {
                    auto& arg = args[i];
                    if (i > 0) std::cout << " ";
                    std::cout << CallThrow(arg.type()->to_string, arg);
                }
                return Obj{};
            };

            FnTable::Set("print", [](ARGS args) {
                return impl(args);
            });
            FnTable::Set("println", [](ARGS args) {
                auto obj = impl(args);
                std::cout << std::endl;
                return obj;
            });
        }
    }

    void Xengine::MethodRegister() {
        using ARGS = std::vector<Obj>&;
        
        // string
        {
            auto type = TypeTable::Get("string");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_string().length());
            });
        }
    }
}
