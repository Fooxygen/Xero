
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>

#include "runtime/table/type.hpp"
#include "runtime/obj/impl/string.hpp"

namespace rt {

    class Obj {
    public:
        class RefData {
        public:
            void*  data = nullptr;
            size_t cnt  = 0;

            RefData(void* data_) : data(data_) {
                cnt = 1;
            }
        };

        using Fn = Obj(*)(std::vector<Obj>&);

    private:
        union {
            void*       ref_;
            int32_t     i32_;
            int64_t     i64_;
            float       f32_;
            double      f64_;
        } data_;
        const Type* type_;

    public:
        Obj() {
            type_ = TypeTable::Get("none");
            data_.ref_ = nullptr;
        }
        Obj(const Obj& other)
        :   type_(other.type_)
        {
            if (type_->isRef) {
                data_.ref_ = other.data_.ref_;
                ((RefData*)data_.ref_)->cnt++;
            }
            else data_ = other.data_;
        }
        Obj& operator=(const Obj& other) {
            if (this == &other) return *this;

            // Destroy this
            if (type_->isRef && data_.ref_) {
                auto refdata = (RefData*)data_.ref_;

                // No obj uses
                if (--refdata->cnt == 0) {
                    type_->destroy(refdata->data);
                    delete refdata;
                }
            }

            // Copy other to this
            type_ = other.type_;
            if (type_->isRef) {
                data_.ref_ = other.data_.ref_;
                ((RefData*)data_.ref_)->cnt++;
            }
            else data_ = other.data_;

            return *this;
        }
        ~Obj() {
            if (hasRef()) {
                auto refdata = (RefData*)data_.ref_;

                // No obj uses
                if (--refdata->cnt == 0) {
                    type_->destroy(refdata->data);
                    delete refdata;
                }
            }
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
        static Obj Make_string(std::string s) {
            Obj o;
            o.type_ = TypeTable::Get("string");
            o.data_.ref_ = new RefData(new String(s));
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
        bool hasRef() const {
            return type_->isRef && data_.ref_;
        }

        int32_t  Get_i32()       const { return data_.i32_;   }
        int64_t  Get_i64()       const { return data_.i64_;   }
        float    Get_f32()       const { return data_.f32_;   }
        double   Get_f64()       const { return data_.f64_;   }
        String&  Get_string()    const {
            RefData* ref = (RefData*)(data_.ref_);
            String*  str = (String*)(ref->data);
            return *str;
        }
    };
}
