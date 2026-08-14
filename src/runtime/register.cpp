
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <cmath>
#include <cstdio>

#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"
#include "obj/impl/string.hpp"
#include "obj/impl/array.hpp"
#include "obj/impl/sliceview.hpp"

namespace rt {

    void Xengine::FnRegister() {
        using ARGS = std::vector<Obj>&;
        
        // type
        {
            FnTable::Set("type", [](ARGS args) {
                return Obj::Make_string(new String(std::string(args[0].type()->name)));
            });
        }

        // print and println
        {
            static const auto impl = [](ARGS args) {
                for (size_t i = 0; i < args.size(); i++) {
                    auto& arg = args[i];
                    auto str = CallThrow(arg.type()->impl_->to_string_, arg);

                    if (i > 0) std::fwrite(" ", 1, 1, stdout);
                    std::fwrite(str.data(), 1, str.size(), stdout);
                }
                return Obj{};
            };

            FnTable::Set("print", [](ARGS args) {
                return impl(args);
            });
            FnTable::Set("println", [](ARGS args) {
                auto obj = impl(args);
                std::fputc('\n', stdout);
                std::fflush(stdout);
                return obj;
            });
        }
    
        // Math
        {
            FnTable::Set("abs", [](ARGS args) {
                auto& num = args[0];
                if (num.is("i32")) return Obj::Make_i32(std::abs(num.Get_i32()));
                if (num.is("i64")) return Obj::Make_i64(std::abs(num.Get_i64()));
                if (num.is("f32")) return Obj::Make_f32(std::abs(num.Get_f32()));
                if (num.is("f64")) return Obj::Make_f64(std::abs(num.Get_f64()));
                return Obj();
            });
        }
    }

    void Xengine::MethodRegister() {
        using ARGS = std::vector<Obj>&;
        
        // string
        {
            auto type = sema::TypeTable::Get("string");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_string_ref().size());
            });

            MethodTable::Set(type, "clear", [](ARGS args) {
                args[0].Get_string_ref().Clear();
                return Obj();
            });
        }

        // stringview
        {
            auto type = sema::TypeTable::Get("stringview");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_stringview_ref().len());
            });
            MethodTable::Set(type, "to_string", [](ARGS args) {
                return TypeImplTable::Convert(args[0], sema::TypeTable::Get("string"));
            });
        }

        // array
        {
            auto type = sema::TypeTable::Get("array");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_array_ref().size());
            });
            MethodTable::Set(type, "clear", [](ARGS args) {
                args[0].Get_array_ref().Clear();
                return Obj();
            });
            MethodTable::Set(type, "insert", [](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Insert(args[1].Get_i32(), new Obj(args[2].Clone()));
                return Obj();
            });
            MethodTable::Set(type, "remove", [](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Remove(args[1].Get_i32());
                return Obj();
            });
            MethodTable::Set(type, "push_front", [](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Insert(0, new Obj(args[1].Clone()));
                return Obj();
            });
            MethodTable::Set(type, "pop_front", [](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Remove(0);
                return Obj();
            });
            MethodTable::Set(type, "push_back", [](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Insert(arr.size(), new Obj(args[1].Clone()));
                return Obj();
            });
            MethodTable::Set(type, "pop_back", [](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Remove(arr.size() - 1);
                return Obj();
            });
        }

        // arrayview
        {
            auto type = sema::TypeTable::Get("arrayview");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_arrayview_ref().len());
            });
            MethodTable::Set(type, "to_array", [](ARGS args) {
                return TypeImplTable::Convert(args[0], sema::TypeTable::Get("array"));
            });
        }
    }
}
