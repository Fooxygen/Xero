
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine.hpp"
#include "table/binfn.hpp"

namespace rt {

    void Xengine::TypeRegister() {

        // none
        TypeTable::Set(Type{
            .name = "none", .size = 0
        });

        // i32
        TypeTable::Set(Type{
            .name = "i32", .size = 4,
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
            .neg        = [](const Obj& a) {
                return Obj::Make_i32(-a.Get_i32());
            }
        });

        // i64
        TypeTable::Set(Type{
            .name = "i64", .size = 8,
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
            .neg        = [](const Obj& a) {
                return Obj::Make_i64(-a.Get_i64());
            }
        });

        // f32
        TypeTable::Set(Type{
            .name = "f32", .size = 4,
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
            .neg        = [](const Obj& a) {
                return Obj::Make_f32(-a.Get_f32());
            }
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
            .neg        = [](const Obj& a) {
                return Obj::Make_f64(-a.Get_f64());
            }
        });

        // string
        TypeTable::Set(Type{
            .name = "string", .size = 0, .isRef = true,
            .destroy    = [](void* data) { delete (String*)data; },
            .to_string  = [](const Obj& o) { return o.Get_string(); },
        });
    }

    void Xengine::BinfnRegister() {
        using ARGS = std::vector<Obj>&;
        
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

            BinfnTable::Set("print", [](ARGS args) {
                return impl(args);
            });
            BinfnTable::Set("println", [](ARGS args) {
                auto obj = impl(args);
                std::cout << std::endl;
                return obj;
            });
        }
    }
}
