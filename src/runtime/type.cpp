
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <cmath>

#include "sema/type.hpp"
#include "runtime/type.hpp"
#include "runtime/obj/obj.hpp"

namespace rt {

    // TypeImplTable

    void TypeImplTable::Init() {

        // Infos
        {
            // none
            TypeImplTable::Set(sema::TypeTable::Lookup("none"), TypeImpl{});

            // bool
            TypeImplTable::Set(sema::TypeTable::Lookup("bool"), TypeImpl{
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
                        "cannot make type '{}' compatible with 'bool'", value.type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("i32"), TypeImpl{
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
                        "cannot make type '{}' compatible with 'i32'", value.type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("i64"), TypeImpl{
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
                        *target = TypeImplTable::Convert(value, sema::TypeTable::Lookup("i64"));
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'i64'", value.type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("f32"), TypeImpl{
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
                        *target = TypeImplTable::Convert(value, sema::TypeTable::Lookup("f32"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'f32'", value.type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("f64"), TypeImpl{
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
                        *target = TypeImplTable::Convert(value, sema::TypeTable::Lookup("f64"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'f64'", value.type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("char"), TypeImpl{
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
                        "cannot make type '{}' compatible with 'char'", value.type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("string"), TypeImpl{
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
                        *target = TypeImplTable::Convert(value, sema::TypeTable::Lookup("string"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'string'", value.type()->name_
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
                            "'parameter of pick' must be 'range', not '{}'",
                            pick.type()->name_
                        ));
                    }
        
                    auto& src   = target.Get_string_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.iter_type()->is("i32") &&
                        !range.iter_type()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "'iterator type of range' must be 'i32' or 'i64', not '{}'",
                            range.iter_type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("stringview"), TypeImpl{
                .clone_     = [](const Obj& o) {
                    return Obj::Make_stringview(new SliceView<String>(o.Get_stringview_ref()));
                },
                .destroy_   = [](void* data) { delete (SliceView<String>*)data; },
                .to_string_ = [](const Obj& o) {
                    auto str = TypeImplTable::Convert(o, sema::TypeTable::Lookup("string"));
                    return str.Get_string_ref().ToCppString();
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    auto& target_view = target->Get_stringview_ref();
                    auto  target_str  = target_view.org();
                    if (!target_str) {
                        throw LogErr(LogModule::Runtime, "cannot assign to 'empty stringview'");
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
                                "cannot make type '{}' compatible with 'char'", value.type()->name_
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
                                "cannot make type '{}' compatible with 'char'", value.type()->name_
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
                            auto str = TypeImplTable::Convert(value, sema::TypeTable::Lookup("string"));
                            assign_from_string(str.Get_string_ref());
                            return;
                        }
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot make type '{}' compatible with 'stringview'", value.type()->name_
                    ));
                },
                .pick_      = [](const Obj& target, const Obj& pick) {
                    if (!pick.is("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "'parameter of pick' must be 'range', not '{}'",
                            pick.type()->name_
                        ));
                    }

                    auto& src   = target.Get_stringview_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.iter_type()->is("i32") &&
                        !range.iter_type()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "'iterator type of range' must be 'i32' or 'i64', not '{}'",
                            range.iter_type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("array"), TypeImpl{
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
                        dst = TypeImplTable::Convert(value, sema::TypeTable::Lookup("array")).Get_array_ref();
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
                            "'parameter of pick' must be 'range', not '{}'",
                            pick.type()->name_
                        ));
                    }

                    auto& src   = target.Get_array_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.iter_type()->is("i32") &&
                        !range.iter_type()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "'iterator type of range' must be 'i32' or 'i64', not '{}'",
                            range.iter_type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("arrayview"), TypeImpl{
                .clone_     = [](const Obj& o) {
                    return Obj::Make_arrayview(new SliceView<Array>(o.Get_arrayview_ref()));
                },
                .destroy_   = [](void* data) { delete (SliceView<Array>*)data; },
                .to_string_ = [](const Obj& o) {
                    auto arr = TypeImplTable::Convert(o, sema::TypeTable::Lookup("array"));
                    return arr.Get_array_ref().ToCppString();
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    auto& target_view = target->Get_arrayview_ref();
                    auto  target_arr  = target_view.org();
                    if (!target_arr) {
                        throw LogErr(LogModule::Runtime, "cannot assign to 'empty arrayview'");
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
                        auto arr = TypeImplTable::Convert(value, sema::TypeTable::Lookup("array"));
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
                            "'parameter of pick' must be 'range', not '{}'",
                            pick.type()->name_
                        ));
                    }

                    auto& src   = target.Get_arrayview_ref();
                    auto& range = pick.Get_range_ref();

                    if (!range.iter_type()->is("i32") &&
                        !range.iter_type()->is("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "'iterator type of range' must be 'i32' or 'i64', not '{}'",
                            range.iter_type()->name_
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
            TypeImplTable::Set(sema::TypeTable::Lookup("range"), TypeImpl{
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
                            range.left()->type()->impl()->to_string_(*range.left()),
                            range.step()->type()->impl()->to_string_(*range.step()),
                            range.right()->type()->impl()->to_string_(*range.right()),
                            boundary
                        );
                    }
                    else {
                        return std::format(
                            "[{} : {}{}",
                            range.left()->type()->impl()->to_string_(*range.left()),
                            range.right()->type()->impl()->to_string_(*range.right()),
                            boundary
                        );
                    }
                }
            });

            // function
            TypeImplTable::Set(sema::TypeTable::Lookup("function"), TypeImpl{
                .clone_     = [](const Obj& o) {
                    return Obj::Make_function(new Function(o.Get_function_ref()));
                },
                .destroy_   = [](void* data) { delete (Function*)data; },
                .to_string_ = [](const Obj& o) {
                    auto& fn = o.Get_function_ref();

                    // Native
                    if     (fn.usingtype() == Function::UsingType::Native) {
                        return std::string("<native function>");
                    }

                    // LangFn
                    else if (fn.usingtype() == Function::UsingType::Lang) {
                        auto& lang = fn.lang();
                        std::string res = "";

                        // Params
                        res += '(';
                        for (size_t i = 0; i < lang->params_->exprs_.size(); i++) {
                            auto param = (DeclExpr*)lang->params_->exprs_[i].get();
                            if (i != 0) res += ", ";
                            res += param->bind_type_->resolved_type_->base()->name_;
                        }
                        res += ')';

                        // Return Type
                        if (lang->ret_type_) {
                            res += " -> " + lang->ret_type_->resolved_type_->name_;
                        }

                        return res;
                    }
                    
                    return std::string("<empty function>");
                }
            });
        }

        // Convert
        {
            auto* i32_ = sema::TypeTable::Lookup("i32");
            auto* i64_ = sema::TypeTable::Lookup("i64");
            auto* f32_ = sema::TypeTable::Lookup("f32");
            auto* f64_ = sema::TypeTable::Lookup("f64");

            TypeImplTable::ConvertSet(i32_, i64_, [](const Obj& o) { return Obj::Make_i64(o.Get_i32());         });
            TypeImplTable::ConvertSet(i32_, f32_, [](const Obj& o) { return Obj::Make_f32((float)o.Get_i32());  });
            TypeImplTable::ConvertSet(i32_, f64_, [](const Obj& o) { return Obj::Make_f64((double)o.Get_i32()); });
            TypeImplTable::ConvertSet(i64_, f32_, [](const Obj& o) { return Obj::Make_f32((float)o.Get_i64());  });
            TypeImplTable::ConvertSet(i64_, f64_, [](const Obj& o) { return Obj::Make_f64((double)o.Get_i64()); });
            TypeImplTable::ConvertSet(f32_, f64_, [](const Obj& o) { return Obj::Make_f64((double)o.Get_f32()); });
            
            auto* char_         = sema::TypeTable::Lookup("char");
            auto* string_       = sema::TypeTable::Lookup("string");
            auto* stringview_   = sema::TypeTable::Lookup("stringview");

            TypeImplTable::ConvertSet(char_, string_, [](const Obj& o) {
                return Obj::Make_string(new String(std::string(1, o.Get_char())));
            });
            TypeImplTable::ConvertSet(stringview_, string_, [](const Obj& o) {
                auto& view = o.Get_stringview_ref();

                std::string res = "";
                for (size_t i = 0; i < view.len(); i++) {
                    res += view.org()->Get(view.offset() + i)->Get_char();
                }

                return Obj::Make_string(new String(res));
            });

            auto* array_       = sema::TypeTable::Lookup("array");
            auto* arrayview_   = sema::TypeTable::Lookup("arrayview");

            TypeImplTable::ConvertSet(arrayview_, array_, [](const Obj& o) {
                auto& view = o.Get_arrayview_ref();

                auto* arr = new Array(
                    view.org() ? view.org()->elem_type() : nullptr,
                    view.len()
                );
                for (size_t i = 0; i < view.len(); i++) {
                    arr->Insert(arr->size(), new Obj(view.org()->Get(view.offset() + i)->Clone()));
                }

                return Obj::Make_array(arr);
            });
        }
    }

    Obj TypeImplTable::Convert(const Obj& obj, const sema::Type* type) {
        auto from = obj.type()->base();
        auto to   = type->base();

        if (from == to) return obj;
        auto it = converts_.find(std::pair{ from, to });
        if (it != converts_.end()) return it->second(obj);
        return Obj();
    }
}
