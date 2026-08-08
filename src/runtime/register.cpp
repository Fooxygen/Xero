
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <cmath>

#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"
#include "obj/impl/string.hpp"
#include "obj/impl/array.hpp"
#include "obj/impl/sliceview.hpp"

namespace rt {

    void Xengine::TypeRegister() {

        // Basic, Built-in (Oper and Method)
        {
            // none
            TypeTable::Set(Type{
                .name = "none", .size = 0
            });

            // bool
            TypeTable::Set(Type{
                .name       = "bool", .size = 1,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_bool(o.Get_bool());
                },
                .to_string_ = [](const Obj& o) {
                    return o.Get_bool() ? std::string("true") : std::string("false");
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("bool")) {
                        *target = value;
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with type 'bool'", value.type()->name
                    ));
                },
                .gt_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() >  r.Get_bool()); },
                .lt_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() <  r.Get_bool()); },
                .ge_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() >= r.Get_bool()); },
                .le_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() <= r.Get_bool()); },
                .eq_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() == r.Get_bool()); },
                .neq_       = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() != r.Get_bool()); },
                .and_       = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() && r.Get_bool()); },
                .or_        = [](const Obj& l, const Obj& r) { return Obj::Make_bool(l.Get_bool() || r.Get_bool()); },
                .not_       = [](const Obj& o) { return Obj::Make_bool(!o.Get_bool()); },
            });

            // i32
            TypeTable::Set(Type{
                .name       = "i32", .size = 4,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_i32(o.Get_i32());
                },
                .to_string_ = [](const Obj& o) {
                    return std::to_string(o.Get_i32());
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("i32")) {
                        *target = value;
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'i32'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) { return Obj::Make_i32(a.Get_i32() + b.Get_i32()); },
                .minus_     = [](const Obj& a, const Obj& b) { return Obj::Make_i32(a.Get_i32() - b.Get_i32()); },
                .star_      = [](const Obj& a, const Obj& b) { return Obj::Make_i32(a.Get_i32() * b.Get_i32()); },
                .slash_     = [](const Obj& a, const Obj& b) {
                    int32_t xb = b.Get_i32();
                    if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                    return Obj::Make_i32(a.Get_i32() / xb);
                },
                .neg_       = [](const Obj& o) { return Obj::Make_i32(-o.Get_i32()); },
                .modt_      = [](const Obj& a, const Obj& b) {
                    int32_t xa = a.Get_i32(), xb = b.Get_i32();
                    if (xb == 0)  throw LogErr(LogModule::Runtime, "division by zero");
                    if (xb == -1) return Obj::Make_i32(0);
                    return Obj::Make_i32(xa % xb);
                },
                .modf_      = [](const Obj& a, const Obj& b) {
                    int32_t xa = a.Get_i32(), xb = b.Get_i32();
                    if (xb == 0)  throw LogErr(LogModule::Runtime, "division by zero");
                    if (xb == -1) return Obj::Make_i32(0);
                    auto r = xa % xb;
                    if (r != 0 && ((r < 0) != (xb < 0))) r += xb;
                    return Obj::Make_i32(r);
                },
                .gt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() >  b.Get_i32()); },
                .lt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() <  b.Get_i32()); },
                .ge_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() >= b.Get_i32()); },
                .le_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() <= b.Get_i32()); },
                .eq_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() == b.Get_i32()); },
                .neq_       = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i32() != b.Get_i32()); },
            });

            // i64
            TypeTable::Set(Type{
                .name       = "i64", .size = 8,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_i64(o.Get_i64());
                },
                .to_string_ = [](const Obj& o) { 
                    return std::to_string(o.Get_i64());
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("i64")) {
                        *target = value;
                        return;
                    }
                    if (value.is("i32")) {
                        *target = TypeTable::Convert(value, TypeTable::Get("i64"));
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'i64'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) { return Obj::Make_i64(a.Get_i64() + b.Get_i64()); },
                .minus_     = [](const Obj& a, const Obj& b) { return Obj::Make_i64(a.Get_i64() - b.Get_i64()); },
                .star_      = [](const Obj& a, const Obj& b) { return Obj::Make_i64(a.Get_i64() * b.Get_i64()); },
                .slash_     = [](const Obj& a, const Obj& b) {
                    int64_t xb = b.Get_i64();
                    if (xb == 0) throw LogErr(LogModule::Runtime, "division by zero");
                    return Obj::Make_i64(a.Get_i64() / xb);
                },
                .neg_       = [](const Obj& o) { return Obj::Make_i64(-o.Get_i64()); },
                .modt_      = [](const Obj& a, const Obj& b) {
                    int64_t xa = a.Get_i64(), xb = b.Get_i64();
                    if (xb == 0)  throw LogErr(LogModule::Runtime, "division by zero");
                    if (xb == -1) return Obj::Make_i64(0);
                    return Obj::Make_i64(xa % xb);
                },
                .modf_      = [](const Obj& a, const Obj& b) {
                    int64_t xa = a.Get_i64(), xb = b.Get_i64();
                    if (xb == 0)  throw LogErr(LogModule::Runtime, "division by zero");
                    if (xb == -1) return Obj::Make_i64(0);
                    auto r = xa % xb;
                    if (r != 0 && ((r < 0) != (xb < 0))) r += xb;
                    return Obj::Make_i64(r);
                },
                .gt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() >  b.Get_i64()); },
                .lt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() <  b.Get_i64()); },
                .ge_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() >= b.Get_i64()); },
                .le_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() <= b.Get_i64()); },
                .eq_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() == b.Get_i64()); },
                .neq_       = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_i64() != b.Get_i64()); },
            });

            // f32
            TypeTable::Set(Type{
                .name       = "f32", .size = 4,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_f32(o.Get_f32());
                },
                .to_string_ = [](const Obj& o) {
                    return std::to_string(o.Get_f32());
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("f32")) {
                        *target = value;
                        return;
                    }
                    if (value.is("i32") || value.is("i64")) {
                        *target = TypeTable::Convert(value, TypeTable::Get("f32"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'f32'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() + b.Get_f32()); },
                .minus_     = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() - b.Get_f32()); },
                .star_      = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() * b.Get_f32()); },
                .slash_     = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() / b.Get_f32()); },
                .neg_       = [](const Obj& o) { return Obj::Make_f32(-o.Get_f32()); },
                .modt_      = [](const Obj& a, const Obj& b) {
                    return Obj::Make_f32(std::fmod(a.Get_f32(), b.Get_f32()));
                },
                .modf_      = [](const Obj& a, const Obj& b) {
                    float xa = a.Get_f32(), xb = b.Get_f32();
                    float r  = std::fmod(xa, xb);
                    if (r != 0 && ((r < 0) != (xb < 0))) r += xb;
                    return Obj::Make_f32(r);
                },
                .gt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() >  b.Get_f32()); },
                .lt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() <  b.Get_f32()); },
                .ge_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() >= b.Get_f32()); },
                .le_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() <= b.Get_f32()); },
                .eq_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() == b.Get_f32()); },
                .neq_       = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f32() != b.Get_f32()); },
            });

            // f64
            TypeTable::Set(Type{
                .name       = "f64", .size = 8,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_f64(o.Get_f64());
                },
                .to_string_ = [](const Obj& o) {
                    return std::to_string(o.Get_f64());
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("f64")) {
                        *target = value;
                        return;
                    }
                    if (value.is("i32") || value.is("i64") || value.is("f32")) {
                        *target = TypeTable::Convert(value, TypeTable::Get("f64"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'f64'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() + b.Get_f64()); },
                .minus_     = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() - b.Get_f64()); },
                .star_      = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() * b.Get_f64()); },
                .slash_     = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() / b.Get_f64()); },
                .neg_       = [](const Obj& o) { return Obj::Make_f64(-o.Get_f64()); },
                .modt_      = [](const Obj& a, const Obj& b) {
                    return Obj::Make_f64(std::fmod(a.Get_f64(), b.Get_f64()));
                },
                .modf_      = [](const Obj& a, const Obj& b) {
                    double xa = a.Get_f64(), xb = b.Get_f64();
                    double r  = std::fmod(xa, xb);
                    if (r != 0 && ((r < 0) != (xb < 0))) r += xb;
                    return Obj::Make_f64(r);
                },
                .gt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() >  b.Get_f64()); },
                .lt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() <  b.Get_f64()); },
                .ge_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() >= b.Get_f64()); },
                .le_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() <= b.Get_f64()); },
                .eq_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() == b.Get_f64()); },
                .neq_       = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_f64() != b.Get_f64()); },
            });

            // char
            TypeTable::Set(Type{
                .name       = "char", .size = 4,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_char(o.Get_char());
                },
                .to_string_ = [](const Obj& o) {
                    return std::string(1, o.Get_char());
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("char")) {
                        *target = value;
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'char'", value.type()->name
                    ));
                },
                .gt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_char() >  b.Get_char()); },
                .lt_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_char() <  b.Get_char()); },
                .ge_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_char() >= b.Get_char()); },
                .le_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_char() <= b.Get_char()); },
                .eq_        = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_char() == b.Get_char()); },
                .neq_       = [](const Obj& a, const Obj& b) { return Obj::Make_bool(a.Get_char() != b.Get_char()); },        
            });

            // string
            TypeTable::Set(Type{
                .name       = "string", .size = 0, .isHeapStored = true,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_string(new String(o.Get_string_ref()));
                },
                .destroy_   = [](void* data) { delete (String*)data; },
                .to_string_ = [](const Obj& o) {
                    return o.Get_string_ref().ToCppString();
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    if (value.is("string")) {
                        *target = value;
                        return;
                    }
                    if (value.is("stringview") ||
                        value.is("char"))
                    {
                        *target = TypeTable::Convert(value, TypeTable::Get("string"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'string'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) {
                    return Obj::Make_string(a.Get_string_ref() + b.Get_string_ref());
                },
                .neg_       = [](const Obj& o) {
                    auto oc = o.Clone();
                    oc.Get_string_ref().Reverse();
                    return oc;
                },
                .eq_        = [](const Obj& a, const Obj& b) {
                    return Obj::Make_bool(a.Get_string_ref().ToCppString() == b.Get_string_ref().ToCppString());
                },
                .neq_       = [](const Obj& a, const Obj& b) {
                    return Obj::Make_bool(a.Get_string_ref().ToCppString() != b.Get_string_ref().ToCppString());
                },        
                .pick_      = [](const Obj& target, const Obj& pick) {
                    if (!pick.is("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "pick must be range, not {}",
                            pick.type()->name
                        ));
                    }
        
                    auto& src   = target.Get_string_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.itertype()->is("i32") &&
                        !range.itertype()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "iterator type of range must be i32 or i64, not {}",
                            range.itertype()->name
                        ));
                    }

                    if (range.step()->Get_i32() != 1) {
                        throw LogErr(LogModule::Runtime, "assignment with step in '[]' not allowed");
                    }

                    if (range.isSingle()) {
                        return Obj::MakeRef(src.Get(range.left()->Get_i32()));
                    }

                    else {
                        size_t len = range.isClosed()
                            ? range.right()->Get_i32() - range.left()->Get_i32() + 1
                            : range.right()->Get_i32() - range.left()->Get_i32();

                        auto* view = new SliceView<String>(
                            &src, len, range.left()->Get_i32()
                        );
                        return Obj::Make_stringview(view);
                    }
                }
            });

            // stringview
            TypeTable::Set(Type{
                .name       = "stringview", .size = 0, .isHeapStored = true,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_stringview(new SliceView<String>(o.Get_stringview_ref()));
                },
                .destroy_   = [](void* data) { delete (SliceView<String>*)data; },
                .to_string_ = [](const Obj& o) {
                    auto str = TypeTable::Convert(o, TypeTable::Get("string"));
                    return str.Get_string_ref().ToCppString();
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    auto& target_view = target->Get_stringview_ref();
                    auto  target_str  = target_view.org();
                    if (!target_str) {
                        throw LogErr(LogModule::Runtime, "cannot assign to empty stringview");
                    }

                    auto assign_from_string = [&](const String& src) {

                        size_t beg = target_view.offset();
                        size_t len = target_view.len();
                        size_t len_common = std::min(len, src.size());

                        // Replace Common
                        // aaa bbbbb ccc
                        // aaa ddddd ccc
                        for (size_t i = 0; i < len_common; i++) {
                            target_str->Replace(beg + i, src.Get(i)->Clone());
                        }

                        // Remove redundant chars
                        // aaa bbbbb ccc
                        // aaa eee   ccc
                        if (len > len_common) {
                            for (size_t i = 0; i < len - len_common; i++) {
                                target_str->Remove(beg + len_common);
                            }
                        }

                        // Add missing chars
                        // aaa bbbbb   ccc
                        // aaa eeeeeee ccc
                        for (size_t i = len_common; i < src.size(); i++) {
                            target_str->Insert(beg + i, new Obj(*src.Get(i)));
                        }
                    };

                    if (target_view.len() == 1) {
                        if (value.is("char")) {
                            *target_str->Get(target_view.offset())
                                = Obj::Make_char(value.Get_char());
                            return;
                        }
                        if (value.is("string")) {
                            auto& value_str = value.Get_string_ref();
                            if (value_str.size() == 1) {
                                *target_str->Get(target_view.offset())
                                    = Obj::Make_char(value_str.Get(0)->Get_char());
                                return;
                            }
                            throw LogErr(LogModule::Runtime, std::format(
                                "cannot make type '{}' compatible with 'char'", value.type()->name
                            ));
                        }
                        if (value.is("stringview")) {
                            auto& value_view = value.Get_stringview_ref();
                            auto  value_str  = value_view.org();
                            if (value_view.len() == 1) {
                                *target_str->Get(target_view.offset())
                                    = Obj::Make_char(value_str->Get(value_view.offset())->Get_char());
                                return;
                            }
                            throw LogErr(LogModule::Runtime, std::format(
                                "cannot make type '{}' compatible with 'char'", value.type()->name
                            ));
                        }
                    }
                    else {
                        if (value.is("char")) {
                            auto str = String(std::string(1, value.Get_char()));
                            assign_from_string(str);
                            return;
                        }
                        if (value.is("string")) {
                            assign_from_string(value.Get_string_ref());
                            return;
                        }
                        if (value.is("stringview")) {
                            auto str = TypeTable::Convert(value, TypeTable::Get("string"));
                            assign_from_string(str.Get_string_ref());
                            return;
                        }
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'stringview'", value.type()->name
                    ));
                },
                .pick_      = [](const Obj& target, const Obj& pick) {
                    if (!pick.is("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "pick must be range, not {}",
                            pick.type()->name
                        ));
                    }

                    auto& src   = target.Get_stringview_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.itertype()->is("i32") &&
                        !range.itertype()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "iterator type of range must be i32 or i64, not {}",
                            range.itertype()->name
                        ));
                    }

                    if (range.step()->Get_i32() != 1) {
                        throw LogErr(LogModule::Runtime, "assignment with step in '[]' not allowed");
                    }

                    if (range.isSingle()) {
                        return Obj::MakeRef(src.org()->Get(range.left()->Get_i32() + src.offset()));
                    }

                    else {
                        size_t len = range.isClosed()
                            ? range.right()->Get_i32() - range.left()->Get_i32() + 1
                            : range.right()->Get_i32() - range.left()->Get_i32();

                        auto* view = new SliceView<String>(
                            src.org(),
                            std::min(len, src.len()),
                            range.left()->Get_i32() + src.offset()
                        );
                        return Obj::Make_stringview(view);
                    }
                },
            });

            // array
            TypeTable::Set(Type{
                .name           = "array", .size = 0, .isHeapStored = true,
                .clone_         = [](const Obj& o) {
                    return Obj::Make_array(new Array(o.Get_array_ref()));
                },
                .destroy_       = [](void* data) { delete (Array*)data; },
                .to_string_     = [](const Obj& o) {
                    return o.Get_array_ref().ToCppString();
                },
                .assign_        = [](Obj* target, const Obj& value) {
                    auto& dst = target->Get_array_ref();

                    // = [x, y, z]
                    if      (value.is("array")) {
                        dst = value.Get_array_ref();
                        return;
                    }

                    // = arr[x..y]
                    else if (value.is("arrayview")) {
                        dst = TypeTable::Convert(value, TypeTable::Get("array")).Get_array_ref();
                        return;
                    }

                    // = x
                    else {
                        for (size_t i = 0; i < dst.size(); i++) {
                            dst.Replace(i, value);
                        }
                        return;
                    }
                },
                .plus_          = [](const Obj& a, const Obj& b) {
                    return Obj::Make_array(a.Get_array_ref() + b.Get_array_ref());
                },
                .neg_           = [](const Obj& o) {
                    auto oc = o.Clone();
                    oc.Get_array_ref().Reverse();
                    return oc;
                },
                .pick_          = [](const Obj& target, const Obj& pick) {
                    if (!pick.is("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "pick must be range, not {}",
                            pick.type()->name
                        ));
                    }

                    auto& src   = target.Get_array_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.itertype()->is("i32") &&
                        !range.itertype()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "iterator type of range must be i32 or i64, not {}",
                            range.itertype()->name
                        ));
                    }

                    if (range.step()->Get_i32() != 1) {
                        throw LogErr(LogModule::Runtime, "assignment with step in '[]' not allowed");
                    }

                    if (range.isSingle()) {
                        return Obj::MakeRef(src.Get(range.left()->Get_i32()));
                    }

                    else {
                        size_t len = range.isClosed()
                            ? range.right()->Get_i32() - range.left()->Get_i32() + 1
                            : range.right()->Get_i32() - range.left()->Get_i32();

                        auto* view = new SliceView<Array>(
                            &src, len, range.left()->Get_i32()
                        );
                        return Obj::Make_arrayview(view);
                    }
                }
            });

            // arrayview
            TypeTable::Set(Type{
                .name       = "arrayview", .size = 0, .isHeapStored = true,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_arrayview(new SliceView<Array>(o.Get_arrayview_ref()));
                },
                .destroy_   = [](void* data) { delete (SliceView<Array>*)data; },
                .to_string_ = [](const Obj& o) {
                    auto arr = TypeTable::Convert(o, TypeTable::Get("array"));
                    return arr.Get_array_ref().ToCppString();
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    auto& target_view = target->Get_arrayview_ref();
                    auto  target_arr  = target_view.org();
                    if (!target_arr) {
                        throw LogErr(LogModule::Runtime, "cannot assign to empty arrayview");
                    }

                    auto assign_from_array = [&](const Array& src) {

                        size_t beg = target_view.offset();
                        size_t len = target_view.len();
                        size_t len_common = std::min(len, src.size());

                        // Replace Common
                        // aaa bbbbb ccc
                        // aaa ddddd ccc
                        for (size_t i = 0; i < len_common; i++) {
                            target_arr->Replace(beg + i, src.Get(i)->Clone());
                        }

                        // Remove redundant elements
                        // aaa bbbbb ccc
                        // aaa eee   ccc
                        if (len > len_common) {
                            for (size_t i = 0; i < len - len_common; i++) {
                                target_arr->Remove(beg + len_common);
                            }
                        }

                        // Add missing elements
                        // aaa bbbbb   ccc
                        // aaa eeeeeee ccc
                        for (size_t i = len_common; i < src.size(); i++) {
                            target_arr->Insert(beg + i, new Obj(*src.Get(i)));
                        }
                    };

                    if      (value.is("array")) {
                        assign_from_array(value.Get_array_ref());
                        return;
                    }
                    else if (value.is("arrayview")) {
                        auto arr = TypeTable::Convert(value, TypeTable::Get("array"));
                        assign_from_array(arr.Get_array_ref());
                        return;
                    }
                    else {
                        for (size_t i = 0; i < target_view.len(); i++) {
                            target_arr->Replace(target_view.offset() + i, value.Clone());
                        }
                        return;
                    }
                },
                .pick_      = [](const Obj& target, const Obj& pick) {
                    if (!pick.is("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "pick must be range, not {}",
                            pick.type()->name
                        ));
                    }

                    auto& src   = target.Get_arrayview_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.itertype()->is("i32") &&
                        !range.itertype()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "iterator type of range must be i32 or i64, not {}",
                            range.itertype()->name
                        ));
                    }

                    if (range.step()->Get_i32() != 1) {
                        throw LogErr(LogModule::Runtime, "assignment with step in '[]' not allowed");
                    }

                    if (range.isSingle()) {
                        return Obj::MakeRef(src.org()->Get(range.left()->Get_i32() + src.offset()));
                    }

                    else {
                        size_t len = range.isClosed()
                            ? range.right()->Get_i32() - range.left()->Get_i32() + 1
                            : range.right()->Get_i32() - range.left()->Get_i32();

                        auto* view = new SliceView<Array>(
                            src.org(),
                            std::min(len, src.len()),
                            range.left()->Get_i32() + src.offset()
                        );
                        return Obj::Make_arrayview(view);
                    }
                },
            });

             // range
            TypeTable::Set(Type{
                .name       = "range", .size = 0, .isHeapStored = true,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_range(new Range(o.Get_range_ref()));
                },
                .destroy_   = [](void* data) { delete (Range*)data; },
                .to_string_ = [](const Obj& o) {
                    auto& range = o.Get_range_ref();
                    char  boundary = range.isClosed() ? ']' : ')';

                    if (range.step()) {
                        return std::format(
                            "[{} : {} : {}{}",
                            range.left()->type()->to_string_(*range.left()),
                            range.step()->type()->to_string_(*range.step()),
                            range.right()->type()->to_string_(*range.right()),
                            boundary
                        );
                    }
                    else {
                        return std::format(
                            "[{} : {}{}",
                            range.left()->type()->to_string_(*range.left()),
                            range.right()->type()->to_string_(*range.right()),
                            boundary
                        );
                    }
                }
            });

            // function
            TypeTable::Set(Type{
                .name       = "function", .size = 0, .isHeapStored = true,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_function(new Function(o.Get_function_ref()));
                },
                .destroy_   = [](void* data) { delete (Function*)data; },
                .to_string_ = [](const Obj& o) {
                    auto& fn = o.Get_function_ref();
                    if (fn.expr()) {
                        auto& fnexpr = fn.expr();
                        std::string res = fnexpr->name_;

                        res += '(';
                        for (size_t i = 0; i < fnexpr->params_->exprs_.size(); i++) {
                            auto param = (DeclExpr*)fnexpr->params_->exprs_[i].get();
                            if (i != 0) res += ", ";
                            res += param->bindtype_;
                        }
                        res += ')';

                        if (fnexpr->ret_type_ != "none") {
                            res += " -> " + fnexpr->ret_type_;
                        }

                        return res;
                    }
                    return std::string("<undefined function>");
                }
            });
        }

        // Convert
        {
            auto* i32_ = TypeTable::Get("i32");
            auto* i64_ = TypeTable::Get("i64");
            auto* f32_ = TypeTable::Get("f32");
            auto* f64_ = TypeTable::Get("f64");

            TypeTable::ConvertSet(i32_, i64_, [](const Obj& o) { return Obj::Make_i64(o.Get_i32());         });
            TypeTable::ConvertSet(i32_, f32_, [](const Obj& o) { return Obj::Make_f32((float)o.Get_i32());  });
            TypeTable::ConvertSet(i32_, f64_, [](const Obj& o) { return Obj::Make_f64((double)o.Get_i32()); });
            TypeTable::ConvertSet(i64_, f32_, [](const Obj& o) { return Obj::Make_f32((float)o.Get_i64());  });
            TypeTable::ConvertSet(i64_, f64_, [](const Obj& o) { return Obj::Make_f64((double)o.Get_i64()); });
            TypeTable::ConvertSet(f32_, f64_, [](const Obj& o) { return Obj::Make_f64((double)o.Get_f32()); });
            
            auto* char_         = TypeTable::Get("char");
            auto* string_       = TypeTable::Get("string");
            auto* stringview_   = TypeTable::Get("stringview");

            TypeTable::ConvertSet(char_, string_, [](const Obj& o) {
                return Obj::Make_string(new String(std::string(1, o.Get_char())));
            });
            TypeTable::ConvertSet(stringview_, string_, [](const Obj& o) {
                auto& view = o.Get_stringview_ref();

                std::string res = "";
                for (size_t i = 0; i < view.len(); i++) {
                    res += view.org()->Get(view.offset() + i)->Get_char();
                }

                return Obj::Make_string(new String(res));
            });

            auto* array_       = TypeTable::Get("array");
            auto* arrayview_   = TypeTable::Get("arrayview");

            TypeTable::ConvertSet(arrayview_, array_, [](const Obj& o) {
                auto& view = o.Get_arrayview_ref();

                auto* arr = new Array(view.len());
                for (size_t i = 0; i < view.len(); i++) {
                    arr->Insert(arr->size(), new Obj(view.org()->Get(view.offset() + i)->Clone()));
                }

                return Obj::Make_array(arr);
            });
        
            TypeTable::ConvertsRecompute();
        }
    }

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
                    if (i > 0) std::cout << " ";
                    std::cout << CallThrow(arg.type()->to_string_, arg);
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
            auto type = TypeTable::Get("string");

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
            auto type = TypeTable::Get("stringview");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_stringview_ref().len());
            });
            MethodTable::Set(type, "to_string", [](ARGS args) {
                return TypeTable::Convert(args[0], TypeTable::Get("string"));
            });
        }

        // array
        {
            auto type = TypeTable::Get("array");

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
            auto type = TypeTable::Get("arrayview");

            MethodTable::Set(type, "len", [](ARGS args) {
                return Obj::Make_i32(args[0].Get_arrayview_ref().len());
            });
            MethodTable::Set(type, "to_array", [](ARGS args) {
                return TypeTable::Convert(args[0], TypeTable::Get("array"));
            });
        }
    }
}
