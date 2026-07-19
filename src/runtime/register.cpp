
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"
#include "obj/impl/string.hpp"
#include "obj/impl/array.hpp"

namespace rt {

    void Xengine::TypeRegister() {

        // Basic
        {
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
                .gt         = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() >  r.Get_bool()); },
                .lt         = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() <  r.Get_bool()); },
                .ge         = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() >= r.Get_bool()); },
                .le         = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() <= r.Get_bool()); },
                .eq         = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() == r.Get_bool()); },
                .neq        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() != r.Get_bool()); },
                .and_       = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() && r.Get_bool()); },
                .or_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() || r.Get_bool()); },
                .not_       = [](const Obj& o) { return Obj::Make_bool(!o.Get_bool()); },
            });

            // i32
            TypeTable::Set(Type{
                .name  = "i32", .size = 4,
                .to_string  = [](const Obj& o) { return std::to_string(o.Get_i32()); },
                .plus       = [](const Obj& a, const Obj& b) { return Obj::Make_i32(a.Get_i32() + b.Get_i32()); },
                .minus      = [](const Obj& a, const Obj& b) { return Obj::Make_i32(a.Get_i32() - b.Get_i32()); },
                .star       = [](const Obj& a, const Obj& b) { return Obj::Make_i32(a.Get_i32() * b.Get_i32()); },
                .slash      = [](const Obj& a, const Obj& b) {
                    int32_t xb = b.Get_i32();
                    if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                    return Obj::Make_i32(a.Get_i32() / xb);
                },
                .neg        = [](const Obj& o) { return Obj::Make_i32(-o.Get_i32()); },
                .gt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() >  b.Get_i32()); },
                .lt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() <  b.Get_i32()); },
                .ge         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() >= b.Get_i32()); },
                .le         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() <= b.Get_i32()); },
                .eq         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() == b.Get_i32()); },
                .neq        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() != b.Get_i32()); },
            });

            // i64
            TypeTable::Set(Type{
                .name  = "i64", .size = 8,
                .to_string  = [](const Obj& o) { return std::to_string(o.Get_i64()); },
                .plus       = [](const Obj& a, const Obj& b) { return Obj::Make_i64(a.Get_i64() + b.Get_i64()); },
                .minus      = [](const Obj& a, const Obj& b) { return Obj::Make_i64(a.Get_i64() - b.Get_i64()); },
                .star       = [](const Obj& a, const Obj& b) { return Obj::Make_i64(a.Get_i64() * b.Get_i64()); },
                .slash      = [](const Obj& a, const Obj& b) {
                    int64_t xb = b.Get_i64();
                    if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                    return Obj::Make_i64(a.Get_i64() / xb);
                },
                .neg        = [](const Obj& o) { return Obj::Make_i64(-o.Get_i64()); },
                .gt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() > b.Get_i64()); },
                .lt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() < b.Get_i64()); },
                .ge         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() >= b.Get_i64()); },
                .le         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() <= b.Get_i64()); },
                .eq         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() == b.Get_i64()); },
                .neq        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() != b.Get_i64()); },
            });

            // f32
            TypeTable::Set(Type{
                .name  = "f32", .size = 4,
                .to_string  = [](const Obj& o) { return std::to_string(o.Get_f32()); },
                .plus       = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() + b.Get_f32()); },
                .minus      = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() - b.Get_f32()); },
                .star       = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() * b.Get_f32()); },
                .slash      = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() / b.Get_f32()); },
                .neg        = [](const Obj& o) { return Obj::Make_f32(-o.Get_f32()); },
                .gt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() > b.Get_f32()); },
                .lt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() < b.Get_f32()); },
                .ge         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() >= b.Get_f32()); },
                .le         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() <= b.Get_f32()); },
                .eq         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() == b.Get_f32()); },
                .neq        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() != b.Get_f32()); },
            });

            // f64
            TypeTable::Set(Type{
                .name = "f64", .size = 8,
                .to_string  = [](const Obj& o) { return std::to_string(o.Get_f64()); },
                .plus       = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() + b.Get_f64()); },
                .minus      = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() - b.Get_f64()); },
                .star       = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() * b.Get_f64()); },
                .slash      = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() / b.Get_f64()); },
                .neg        = [](const Obj& o) { return Obj::Make_f64(-o.Get_f64()); },
                .gt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() > b.Get_f64()); },
                .lt         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() < b.Get_f64()); },
                .ge         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() >= b.Get_f64()); },
                .le         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() <= b.Get_f64()); },
                .eq         = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() == b.Get_f64()); },
                .neq        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() != b.Get_f64()); },
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
                    return Obj::Make_bool(a.Get_string().ToCppString() == b.Get_string().ToCppString());
                },
                .neq       = [](const Obj& a, const Obj& b) {
                    return Obj::Make_bool(a.Get_string().ToCppString() != b.Get_string().ToCppString());
                },        
            });

            // array
            TypeTable::Set(Type{
                .name = "array", .size = 0, .isRef = true,
                .destroy = [](void* data) { delete (Array*)data; }      
            });
        }

        // Convert
        {
            auto* t_i32 = TypeTable::Get("i32");
            auto* t_i64 = TypeTable::Get("i64");
            auto* t_f32 = TypeTable::Get("f32");
            auto* t_f64 = TypeTable::Get("f64");

            TypeTable::ConvertSet(t_i32, t_i64, [](const Obj& o) { return Obj::Make_i64(o.Get_i32());         });
            TypeTable::ConvertSet(t_i32, t_f32, [](const Obj& o) { return Obj::Make_f32((float)o.Get_i32());  });
            TypeTable::ConvertSet(t_i32, t_f64, [](const Obj& o) { return Obj::Make_f64((double)o.Get_i32()); });
            TypeTable::ConvertSet(t_i64, t_f32, [](const Obj& o) { return Obj::Make_f32((float)o.Get_i64());  });
            TypeTable::ConvertSet(t_i64, t_f64, [](const Obj& o) { return Obj::Make_f64((double)o.Get_i64()); });
            TypeTable::ConvertSet(t_f32, t_f64, [](const Obj& o) { return Obj::Make_f64((double)o.Get_f32()); });
        
            TypeTable::ConvertsRecompute();
        }
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

        // array
        {
            auto type = TypeTable::Get("array");

            MethodTable::Set(type, "insert", [](ARGS args) {
                auto& array = args[0].Get_array_ref();
                array.Insert(args[1].Get_i32(), new Obj(args[2]));
                return Obj();
            });
            MethodTable::Set(type, "remove", [](ARGS args) {
                auto& array = args[0].Get_array_ref();
                array.Remove(args[1].Get_i32());
                return Obj();
            });
            MethodTable::Set(type, "pushfront", [](ARGS args) {
                auto& array = args[0].Get_array_ref();
                array.Insert(0, new Obj(args[1]));
                return Obj();
            });
            MethodTable::Set(type, "popfront", [](ARGS args) {
                auto& array = args[0].Get_array_ref();
                array.Remove(0);
                return Obj();
            });
            MethodTable::Set(type, "pushback", [](ARGS args) {
                auto& array = args[0].Get_array_ref();
                array.Insert(array.size(), new Obj(args[1]));
                return Obj();
            });
            MethodTable::Set(type, "popback", [](ARGS args) {
                auto& array = args[0].Get_array_ref();
                array.Remove(array.size() - 1);
                return Obj();
            });
        }
    }
}
