
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <cmath>
#include <cstdio>

#include "runtime/method.hpp"
#include "runtime/xengine.hpp"
#include "runtime/obj/impl/string.hpp"
#include "runtime/obj/impl/array.hpp"
#include "runtime/obj/impl/sliceview.hpp"

namespace rt {

    void Xengine::BuiltinFnImplRegister() {
        using ARGS = std::vector<Obj>&;
        
        // IO
        {
            // print AND println
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

                env_.Declare("print", Obj::Make_function(new Function([](ARGS args) {
                    return impl(args);
                })));
                env_.Declare("println", Obj::Make_function(new Function([](ARGS args) {
                    auto obj = impl(args);
                    std::fputc('\n', stdout);
                    std::fflush(stdout);
                    return obj;
                })));
            }
        }

        // Reflection
        {
            env_.Declare("type", Obj::Make_function(new Function([](ARGS args) {
                return Obj::Make_string(new String(std::string(args[0].type()->name_)));
            })));
        }
    }

    void MethodImplTable::BuiltinImplRegister() {
        using ARGS = std::vector<Obj>&;
        
        // string
        {
            auto type = sema::TypeTable::Lookup("string");

            Set(type, "len", Function([](ARGS args) {
                return Obj::Make_i32(args[0].Get_string_ref().size());
            }));
            Set(type, "clear", Function([](ARGS args) {
                args[0].Get_string_ref().Clear();
                return Obj();
            }));
        }

        // stringview
        {
            auto type = sema::TypeTable::Lookup("stringview");

            Set(type, "len", Function([](ARGS args) {
                return Obj::Make_i32(args[0].Get_stringview_ref().len());
            }));
            Set(type, "to_string", Function([](ARGS args) {
                return TypeImplTable::Convert(args[0], sema::TypeTable::Lookup("string"));
            }));
        }

        // array
        {
            auto type = sema::TypeTable::Lookup("array");

            Set(type, "len", Function([](ARGS args) {
                return Obj::Make_i32(args[0].Get_array_ref().size());
            }));
            Set(type, "clear", Function([](ARGS args) {
                args[0].Get_array_ref().Clear();
                return Obj();
            }));
            Set(type, "insert", Function([](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Insert(args[1].Get_i32(), new Obj(args[2].Clone()));
                return Obj();
            }));
            Set(type, "remove", Function([](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Remove(args[1].Get_i32());
                return Obj();
            }));
            Set(type, "push_front", Function([](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Insert(0, new Obj(args[1].Clone()));
                return Obj();
            }));
            Set(type, "pop_front", Function([](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Remove(0);
                return Obj();
            }));
            Set(type, "push_back", Function([](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Insert(arr.size(), new Obj(args[1].Clone()));
                return Obj();
            }));
            Set(type, "pop_back", Function([](ARGS args) {
                auto& arr = args[0].Get_array_ref();
                arr.Remove(arr.size() - 1);
                return Obj();
            }));
        }

        // arrayview
        {
            auto type = sema::TypeTable::Lookup("arrayview");

            Set(type, "len", Function([](ARGS args) {
                return Obj::Make_i32(args[0].Get_arrayview_ref().len());
            }));
            Set(type, "to_array", Function([](ARGS args) {
                return TypeImplTable::Convert(args[0], sema::TypeTable::Lookup("array"));
            }));
        }
    }
}
