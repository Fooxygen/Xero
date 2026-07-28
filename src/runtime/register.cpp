
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine.hpp"
#include "table/fn.hpp"
#include "table/method.hpp"
#include "obj/impl/string.hpp"
#include "obj/impl/stringview.hpp"
#include "obj/impl/array.hpp"

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
                    if (value.type() == TypeTable::Get("bool")) {
                        *target = value;
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'bool'", value.type()->name
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
                    if (value.type() == TypeTable::Get("i32")) {
                        *target = value;
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'i32'", value.type()->name
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
                    if (value.type() == TypeTable::Get("i64")) {
                        *target = value;
                        return;
                    }

                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'i64'", value.type()->name
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
                    if (value.type() == TypeTable::Get("f32")) {
                        *target = value;
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'f32'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() + b.Get_f32()); },
                .minus_     = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() - b.Get_f32()); },
                .star_      = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() * b.Get_f32()); },
                .slash_     = [](const Obj& a, const Obj& b) { return Obj::Make_f32(a.Get_f32() / b.Get_f32()); },
                .neg_       = [](const Obj& o) { return Obj::Make_f32(-o.Get_f32()); },
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
                    if (value.type() == TypeTable::Get("f64")) {
                        *target = value;
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'f64'", value.type()->name
                    ));
                },
                .plus_      = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() + b.Get_f64()); },
                .minus_     = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() - b.Get_f64()); },
                .star_      = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() * b.Get_f64()); },
                .slash_     = [](const Obj& a, const Obj& b) { return Obj::Make_f64(a.Get_f64() / b.Get_f64()); },
                .neg_       = [](const Obj& o) { return Obj::Make_f64(-o.Get_f64()); },
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
                    if (value.type() == TypeTable::Get("char")) {
                        *target = value;
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'char'", value.type()->name
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
                    if (value.type() == TypeTable::Get("string")) {
                        *target = value;
                        return;
                    }
                    if (value.type() == TypeTable::Get("stringview")) {
                        *target = TypeTable::Convert(value, TypeTable::Get("string"));
                        return;
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'string'", value.type()->name
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
                    if (pick.type() != TypeTable::Get("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "pick must be range, not {}",
                            pick.type()->name
                        ));
                    }
        
                    auto& src   = target.Get_string_ref();
                    auto& range = pick.Get_range_ref();

                    if (range.itertype() != TypeTable::Get("i32") &&
                        range.itertype() != TypeTable::Get("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "iterator type of range must be i32 or i64, not {}",
                            range.itertype()->name
                        ));
                    }

                    if (range.step()->Get_i32() != 1) {
                        throw LogErr(LogModule::Runtime, "assignment with step in '[]' not allowed");
                    }

                    auto* sv = new StringView(
                        &target.Get_string_ref(), new Range(range)
                    );
                    return Obj::Make_stringview(sv);
                }
            });

            // stringview
            TypeTable::Set(Type{
                .name       = "stringview", .size = 0, .isHeapStored = true,
                .clone_     = [](const Obj& o) {
                    return Obj::Make_stringview(new StringView(o.Get_stringview_ref()));
                },
                .destroy_   = [](void* data) { delete (StringView*)data; },
                .to_string_ = [](const Obj& o) {
                    auto str = TypeTable::Convert(o, TypeTable::Get("string"));
                    return str.Get_string_ref().ToCppString();
                },
                .assign_    = [](Obj* target, const Obj& value) {
                    auto& sv    = target->Get_stringview_ref();
                    auto& dst   = *sv.str();
                    auto& range = *sv.range();

                    auto assign_from_string = [&](const String& src) {

                        size_t beg = range.left()->Get_i32();
                        size_t len = range.isClosed()
                            ? range.right()->Get_i32() - beg + 1
                            : range.right()->Get_i32() - beg;
                        size_t len_common = std::min(len, src.size());

                        // Replace Common
                        // aaa bbbbb ccc
                        // aaa ddddd ccc
                        for (size_t i = 0; i < len_common; i++) {
                            *dst.Get(beg + i) = *src.Get(i);
                        }

                        // Remove redundant chars
                        // aaa bbbbb ccc
                        // aaa eee   ccc
                        for (size_t i = len - 1; i >= len_common; i--) {
                            dst.Remove(beg + i);
                        }

                        // Add missing chars
                        // aaa bbbbb   ccc
                        // aaa eeeeeee ccc
                        for (size_t i = len_common; i < src.size(); i++) {
                            dst.Insert(beg + i, new Obj(*src.Get(i)));
                        }
                    };

                    if (range.isSingle()) {
                        if (value.type() == TypeTable::Get("char")) {
                            *dst.Get(range.left()->Get_i32()) = Obj::Make_char(value.Get_char());
                            return;
                        }
                        if (value.type() == TypeTable::Get("string")) {
                            auto& str = value.Get_string_ref();
                            if (str.size() == 1) {
                                *dst.Get(range.left()->Get_i32()) = Obj::Make_char(str.Get(0)->Get_char());
                                return;
                            }
                            throw LogErr(LogModule::Runtime, std::format(
                                "cannot assign type '{}' to type 'char'", value.type()->name
                            ));
                        }
                        if (value.type() == TypeTable::Get("stringview")) {
                            auto str = value.Get_stringview_ref().str();
                            if (str->size() == 1) {
                                *dst.Get(range.left()->Get_i32()) = Obj::Make_char(str->Get(0)->Get_char());
                                return;
                            }
                            throw LogErr(LogModule::Runtime, std::format(
                                "cannot assign type '{}' to type 'char'", value.type()->name
                            ));
                        }
                    }
                    else {
                        if (value.type() == TypeTable::Get("char")) {
                            auto str = String(std::string(1, value.Get_char()));
                            assign_from_string(str);
                            return;
                        }
                        if (value.type() == TypeTable::Get("string")) {
                            assign_from_string(value.Get_string_ref());
                            return;
                        }
                        if (value.type() == TypeTable::Get("stringview")) {
                            auto str = TypeTable::Convert(value, TypeTable::Get("string"));
                            assign_from_string(str.Get_string_ref());
                            return;
                        }
                    }
                    
                    throw LogErr(LogModule::Runtime, std::format(
                        "cannot assign type '{}' to type 'stringview'", value.type()->name
                    ));
                },
                .pick_      = [](const Obj& target, const Obj& pick) {
                    if (pick.type() != TypeTable::Get("range")) {
                        throw LogErr(LogModule::Runtime, std::format(
                            "pick must be range, not {}",
                            pick.type()->name
                        ));
                    }

                    auto& sv = target.Get_stringview_ref();
                    auto& range_org = *sv.range();
                    auto& range_nes = pick.Get_range_ref();

                    if (range_nes.itertype() != TypeTable::Get("i32") &&
                        range_nes.itertype() != TypeTable::Get("i64"))
                    {
                        throw LogErr(LogModule::Runtime, std::format(
                            "iterator type of range must be i32 or i64, not {}",
                            range_nes.itertype()->name
                        ));
                    }

                    if (range_nes.step()->Get_i32() != 1) {
                        throw LogErr(LogModule::Runtime, "assignment with step in '[]' not allowed");
                    }

                    // Range Update

                    auto org_closed = range_org.ToClosed();
                    auto nes_closed = range_nes.ToClosed();
                    
                    auto lorg = *org_closed.left();
                    auto rorg = *org_closed.right();
                    auto lnes = TypeTable::Convert(*nes_closed.left(), nes_closed.itertype());
                    auto rnes = TypeTable::Convert(*nes_closed.right(), nes_closed.itertype());

                    auto lmeg = nes_closed.itertype()->plus_(
                        lorg, lnes
                    );
                    auto len  = nes_closed.itertype()->minus_(
                        rnes, lnes
                    );
                    auto rmeg = nes_closed.itertype()->plus_(
                        lmeg, len
                    );
                    rmeg =  nes_closed.itertype()->le_(rmeg, rorg).Get_bool()
                            ? rmeg : rorg;

                    auto range_merge = new Range(
                        lmeg, rmeg, Obj::Make_i32(1), true, nes_closed.itertype()
                    );

                    auto* sv_new = new StringView(sv.str(), range_merge);
                    return Obj::Make_stringview(sv_new);
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
                    std::string res = "[";
                    auto& arr = o.Get_array_ref();
                    for (size_t i = 0; i < arr.size(); i++) {
                        auto o = arr.Get(i);
                        if (i != 0) res += ", ";
                        res += o->type()->to_string_(*o);
                    }
                    res += "]";
                    return res;
                },
                .assign_        = [](Obj* target, const Obj& value) {
                    auto& dst = target->Get_array_ref();

                    // = [x, y, z]
                    if (value.type() == TypeTable::Get("array")) {
                        auto& src = value.Get_array_ref();
                        if (src.size() != dst.size()) {
                            throw LogErr(LogModule::Runtime,
                                "cannot assign source of different size to 'array'"
                            );
                        }

                        for (size_t i = 0; i < dst.size(); i++) {
                            *dst.Get(i)->Origin() = *src.Get(i);
                        }
                        return;
                    }

                    // = x
                    else {
                        // [1, 2, ...] = 0 -> [0, 0, ...]
                        for (size_t i = 0; i < dst.size(); i++) {
                            *dst.Get(i)->Origin() = value;
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
                    auto& src   = target.Get_array_ref();
                    auto& range = pick.Get_range_ref();

                    if (range.isSingle()) {
                        return Obj::MakeRef(src.Get(range.left()->Get_i32()));
                    }

                    else {
                        auto* dst = new Array();
                        for (Obj o = *range.left(); ; o = range.itertype()->plus_(o, *range.step())) {
                            if (!range.isClosed() &&
                                 range.itertype()->ge_(o, *range.right()).Get_bool()) break;
                            if ( range.isClosed() &&
                                 range.itertype()->gt_(o, *range.right()).Get_bool()) break;
                            dst->Insert(dst->size(), new Obj(Obj::MakeRef(src.Get(o.Get_i32()))));
                        }
                        return Obj::Make_array(dst);
                    }
                }
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
                return Obj::Make_string(new String(std::to_string(o.Get_char())));
            });
            TypeTable::ConvertSet(stringview_, string_, [](const Obj& o) {
                auto& sv    = o.Get_stringview_ref();
                auto& str   = *sv.str();
                auto& range = *sv.range();

                std::string res = "";
                for (Obj o = *range.left(); ; o = range.itertype()->plus_(o, *range.step())) {
                    if (!range.isClosed() &&
                         range.itertype()->ge_(o, *range.right()).Get_bool()) break;
                    if ( range.isClosed() &&
                         range.itertype()->gt_(o, *range.right()).Get_bool()) break;
                    res += str.Get(o.Get_i32())->Get_char();
                }

                return Obj::Make_string(new String(res));
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
