
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstdint>
#include <variant>

#include "sema/semantics.hpp"
#include "runtime/runtime.hpp"
#include "runtime/obj/impl/string.hpp"
#include "runtime/obj/impl/array.hpp"
#include "runtime/obj/impl/sliceview.hpp"
#include "runtime/obj/impl/range.hpp"
#include "runtime/obj/impl/function.hpp"

namespace rt {

    class Obj {
    public:
        enum class UsingType {
            Value, Ref
        };

        class Value {
        private:
            union Data {
                void*       ptr_;
                bool        bool_;
                int32_t     i32_;
                int64_t     i64_;
                float       f32_;
                double      f64_;
                char        char_;
            };

            const sema::Type* type_;
            Data              data_;

        public:
            Value(const sema::Type* type = sema::TypeTable::Get("none")) {
                type_      = type;
                data_.ptr_ = nullptr;
            }

            const sema::Type* type() const { return type_; }
            const Data&       data() const { return data_; }      
            Data&             data()       { return data_; }      
        
            bool hasHeapData() const {
                return type_->isHeapStored && data_.ptr_;
            }
        };

        class Ref {
        private:
            Obj* obj_;

        public:
            Ref(Obj* obj) : obj_(obj) {}

            Obj* obj() const { return obj_; }
        };

        class HeapData {
        public:
            void*  data = nullptr;
            size_t cnt  = 0;

            HeapData(void* data_) : data(data_) {
                cnt = 1;
            }
        };

    private:
        UsingType                   usingtype = UsingType::Value;
        std::variant<Value, Ref>    data_{Value{}};

        Value&       value()        { return std::get<Value>(data_); }
        const Value& value() const  { return std::get<Value>(data_); }
        Ref&         ref()          { return std::get<Ref>(data_); }
        const Ref&   ref()   const  { return std::get<Ref>(data_); }

        void Destroy() {
            if (usingtype == UsingType::Value && hasHeapData()) {
                auto heapdata = (HeapData*)value().data().ptr_;
                if (--heapdata->cnt == 0) {
                    type()->impl_->destroy_(heapdata->data);
                    delete heapdata;
                }
            }
        }
        void AssignFrom(const Obj& other) {
            usingtype = other.usingtype;

            if (other.usingtype == UsingType::Ref) {
                data_.emplace<Ref>(other.ref().obj());
            }

            else {
                data_.emplace<Value>(other.value().type());

                if (value().type()->isHeapStored) {
                    value().data().ptr_ = other.value().data().ptr_;
                    ((HeapData*)value().data().ptr_)->cnt++;
                }
                else value().data() = other.value().data();
            }
        }

    public:
        Obj() {}
        Obj(const Obj& other) {
            AssignFrom(other);
        }
        ~Obj() {
            if (usingtype == UsingType::Value) {
                if (hasHeapData()) {
                    auto heapdata = (HeapData*)value().data().ptr_;
                    if (--heapdata->cnt == 0) {
                        type()->impl_->destroy_(heapdata->data);
                        delete heapdata;
                    }
                }
            }
        }

        const sema::Type* type()  const  {
            switch (usingtype) {
                case UsingType::Value:  return value().type();
                case UsingType::Ref:    return ref().obj()->type();
                default:                __builtin_unreachable();
            }
        }
        
        bool isNone() const {
            return type()->isNone();
        }
        bool is(std::string_view type_name) const {
            return type()->is(type_name);
        }
        bool hasHeapData() const {
            switch (usingtype) {
                case UsingType::Value:  return value().hasHeapData();
                case UsingType::Ref:    return ref().obj()->hasHeapData();
                default:                __builtin_unreachable();
            }
        }
        
        Obj        Clone()  const {
            switch (usingtype) {
                case UsingType::Value: {
                    if (type()->isHeapStored)   return type()->impl_->clone_(*this);
                    else                        return *this;
                }
                case UsingType::Ref: return ref().obj()->type()->impl_->clone_(*ref().obj());
                default:             __builtin_unreachable();
            }
        }
        Obj*       Origin() {
            if (usingtype == UsingType::Ref)
                return ref().obj();
            return this;
        }
        const Obj* Origin() const {
            if (usingtype == UsingType::Ref)
                return ref().obj();
            return this;
        }

        static Obj MakeRef(Obj* org) {
            while (org->usingtype == UsingType::Ref) {
                org = org->ref().obj();
            }
                
            Obj o;
            o.usingtype = UsingType::Ref;
            o.data_.emplace<Ref>(org);
            return o;
        }
        static Obj MakeEmpty(const sema::Type* type) {
            auto& name = type->name;

            if (name == "none")        return Obj();
            if (name == "bool")        return Make_bool(false);
            if (name == "char")        return Make_char('\0');
            if (name == "i32")         return Make_i32(0);
            if (name == "i64")         return Make_i64(0);
            if (name == "f32")         return Make_f32(0.0f);
            if (name == "f64")         return Make_f64(0.0);
            if (name == "string")      return Make_string(new String());
            if (name == "stringview")  return Make_stringview(new SliceView<String>());
            if (name == "array")       return Make_array();
            if (name == "arrayview")   return Make_arrayview(new SliceView<Array>());
            if (name == "range")       return Make_range(new Range());
            if (name == "function")    return Make_function(new Function());

            throw LogErr(LogModule::Runtime, std::format(
                "cannot make empty value for type '{}'",
                name
            ));
        }
        
        static Obj Make_bool(bool b) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("bool"));
            v.data().bool_ = b;
            return o;
        }
        static Obj Make_i32(int32_t x) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("i32"));
            v.data().i32_ = x;
            return o;
        }
        static Obj Make_i64(int64_t x) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("i64"));
            v.data().i64_ = x;
            return o;
        }
        static Obj Make_f32(float x) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("f32"));
            v.data().f32_ = x;
            return o;
        }
        static Obj Make_f64(double x) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("f64"));
            v.data().f64_ = x;
            return o;
        }
        static Obj Make_char(char c) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("char"));
            v.data().char_ = c;
            return o;
        }
        static Obj Make_string(String* str) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("string"));
            v.data().ptr_ = new HeapData(str);
            return o;
        }
        static Obj Make_stringview(SliceView<String>* view) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("stringview"));
            v.data().ptr_ = new HeapData(view);
            return o;
        }
        static Obj Make_array(size_t size = 1) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("array"));
            v.data().ptr_ = new HeapData(new Array(size));
            return o;
        }
        static Obj Make_array(Array* arr) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("array"));
            v.data().ptr_ = new HeapData(arr);
            return o;
        }
        static Obj Make_arrayview(SliceView<Array>* view) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("arrayview"));
            v.data().ptr_ = new HeapData(view);
            return o;
        }
        static Obj Make_range(Range* rge) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("range"));
            v.data().ptr_ = new HeapData(rge);
            return o;
        }
        static Obj Make_function(Function* fn) {
            Obj o;
            auto& v = o.data_.emplace<Value>(sema::TypeTable::Get("function"));
            v.data().ptr_ = new HeapData(fn);
            return o;
        }

        bool        Get_bool()              const {
            if (usingtype == UsingType::Ref)
                return ref().obj()->Get_bool();
            return value().data().bool_;
        }
        int32_t     Get_i32()               const {
            if (usingtype == UsingType::Ref)
                return ref().obj()->Get_i32();
            return value().data().i32_;
        }
        int64_t     Get_i64()               const {
            if (usingtype == UsingType::Ref)
                return ref().obj()->Get_i64();
            return value().data().i64_;
        }
        float       Get_f32()               const {
            if (usingtype == UsingType::Ref)
                return ref().obj()->Get_f32();
            return value().data().f32_;
        }
        double      Get_f64()               const {
            if (usingtype == UsingType::Ref)
                return ref().obj()->Get_f64();
            return value().data().f64_;
        }
        char        Get_char()              const {
            if (usingtype == UsingType::Ref)
                return ref().obj()->Get_char();
            return value().data().char_;
        }
        String&     Get_string_ref()        const {
            switch (usingtype) {
                case UsingType::Ref:
                    return ref().obj()->Get_string_ref();
                case UsingType::Value: {
                    auto heap = (HeapData*)value().data().ptr_;
                    return *(String*)heap->data;
                }
                default: __builtin_unreachable();
            }
        }
        SliceView<String>&
                    Get_stringview_ref()    const {
            switch (usingtype) {
                case UsingType::Ref:
                    return ref().obj()->Get_stringview_ref();
                case UsingType::Value: {
                    auto heap = (HeapData*)value().data().ptr_;
                    return *(SliceView<String>*)heap->data;
                }
                default: __builtin_unreachable();
            }
        }
        Array&      Get_array_ref()         const {
            switch (usingtype) {
                case UsingType::Ref:
                    return ref().obj()->Get_array_ref();
                case UsingType::Value: {
                    auto heap = (HeapData*)value().data().ptr_;
                    return *(Array*)heap->data;
                }
                default: __builtin_unreachable();
            }
        }
        SliceView<Array>&
                    Get_arrayview_ref()     const {
            switch (usingtype) {
                case UsingType::Ref:
                    return ref().obj()->Get_arrayview_ref();
                case UsingType::Value: {
                    auto heap = (HeapData*)value().data().ptr_;
                    return *(SliceView<Array>*)heap->data;
                }
                default: __builtin_unreachable();
            }
        }
        Range&      Get_range_ref()         const {
            switch (usingtype) {
                case UsingType::Ref:
                    return ref().obj()->Get_range_ref();
                case UsingType::Value: {
                    auto heap = (HeapData*)value().data().ptr_;
                    return *(Range*)heap->data;
                }
                default: __builtin_unreachable();
            }
        }
        Function&   Get_function_ref()      const {
            switch (usingtype) {
                case UsingType::Ref:
                    return ref().obj()->Get_function_ref();
                case UsingType::Value: {
                    auto heap = (HeapData*)value().data().ptr_;
                    return *(Function*)heap->data;
                }
                default: __builtin_unreachable();
            }
        }
    
        Obj& operator=(const Obj& other) {
            if (this == &other) return *this;
            Destroy();
            AssignFrom(other);
            return *this;
        }
    };
}
