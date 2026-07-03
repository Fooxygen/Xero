
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "typetable.hpp"

namespace rt {

    class Obj {
    private:
        union {
            void*   ptr_;
            int32_t i32_;
            float   f32_;
        } data_;
        const Type* type_;

    public:
        static Obj Make_none() {
            Obj o;
            o.type_ = TypeTable::Get("none");
            return o;
        }
        static Obj Make_i32(int32_t x) {
            Obj o;
            o.type_ = TypeTable::Get("i32");
            o.data_.i32_ = x;
            return o;
        }
        static Obj Make_f32(float x) {
            Obj o;
            o.type_ = TypeTable::Get("f32");
            o.data_.f32_ = x;
            return o;
        }

        bool    isNone() const { return type_ == TypeTable::Get("none"); }
        int32_t As_i32() const { return data_.i32_; }
        float   As_f32() const { return data_.f32_; }
    };
}
