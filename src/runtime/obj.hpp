
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "runtime/table/table_type.hpp"

namespace rt {

    class Obj {
    public:
        using binfn = Obj(*)(const std::vector<Obj>&);

    private:
        union {
            void*       ptr_;
            int32_t     i32_;
            int64_t     i64_;
            float       f32_;
            double      f64_;
            binfn       binfn_;
        } data_;
        const Type* type_;

    public:
        Obj() {
            type_ = TypeTable::Get("none");
        }

        static Obj Make_none() {
            Obj o;
            return o;
        }
        static Obj Make_i32(int32_t x) {
            Obj o;
            o.type_ = TypeTable::Get("i32");
            o.data_.i32_ = x;
            return o;
        }
        static Obj Make_i64(int64_t x) {
            Obj o;
            o.type_ = TypeTable::Get("i64");
            o.data_.i64_ = x;
            return o;
        }
        static Obj Make_f32(float x) {
            Obj o;
            o.type_ = TypeTable::Get("f32");
            o.data_.f32_ = x;
            return o;
        }
        static Obj Make_f64(double x) {
            Obj o;
            o.type_ = TypeTable::Get("f64");
            o.data_.f64_ = x;
            return o;
        }
        static Obj Make_binfn(binfn f) {
            Obj o;
            o.type_ = TypeTable::Get("binfn");
            o.data_.binfn_ = f;
            return o;
        }

        const Type* type() {
            return type_;
        }
        bool isNone() const {
            return type_ == TypeTable::Get("none");
        }
        bool is(std::string_view type_name) const {
            return type_->name == type_name;
        }

        int32_t Get_i32()   const { return data_.i32_;   }
        int64_t Get_i64()   const { return data_.i64_;   }
        float   Get_f32()   const { return data_.f32_;   }
        double  Get_f64()   const { return data_.f64_;   }
        binfn   Get_binfn() const { return data_.binfn_; }
    };
}
