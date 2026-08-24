
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xengine/obj/impl/array.hpp"
#include "xengine/obj/obj.hpp"

namespace xengine {

    Array::Array(const sema::Type* elem_type, size_t size) {
        data_ = new Obj*[size];
        memset(data_, 0, sizeof(Obj*) * size);
        capacity_ = size;
        elem_type_ = elem_type;
    }

    Array::Array(const Array& other) {
        data_ = new Obj*[other.capacity_];
        memset(data_, 0, sizeof(Obj*) * other.capacity_);
        capacity_  = other.capacity_;
        elem_type_ = other.elem_type_;

        for (size_t i = 0; i < other.size_; i++) {
            Insert(i, new Obj(other.Get(i)->Clone()));
        }
    }

    void Array::ElemTypeCheck(const sema::Type* type) {
        if (!elem_type_ || !type) return;
        else if (type != elem_type_) {
            throw LogErr(LogModule::Xengine, std::format(
                "cannot make type '{}' compatible with '{}'",
                type->name_, elem_type_->name_
            ));
        }
    }

    void Array::Expand(size_t size) {
        if (size + 1 < capacity_) return;

        size_t capa   = std::max((size_t)((size + 1) * 1.5f), (size_t)((capacity_ + 1) * 1.5f));
        auto   expand = new Obj*[capa];
        memset(expand, 0, sizeof(Obj*) * capa);
        memcpy(expand, data_, sizeof(Obj*) * capacity_);

        delete[] data_;
        data_ = expand;
        capacity_ = capa;
    }

    std::string Array::ToCppString() const {
        std::string res = "[";
        for (size_t i = 0; i < size_; i++) {
            auto o = Get(i);
            if (i != 0) res += ", ";
            res += o->type()->impl()->to_string_(*o);
        }
        res += "]";
        return res;
    }

    void Array::Clear() {
        for (size_t i = 0; i < size_; i++) {
            if (data_[i]) delete data_[i];
        }
        if (data_) delete[] data_;

        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    void Array::Reverse() {
        if (size_ <= 1) return;
        for (size_t i = 0, j = size_ - 1; i < j; i++, j--) {
            auto obj = data_[i];
            data_[i] = data_[j];
            data_[j] = obj;
        }
    }

    void Array::Insert(size_t idx, Obj* obj) {
        IndexCheck(idx, true);
        Expand(size_ + 1);
        ElemTypeCheck(obj->type());
        memmove(data_ + idx + 1, data_ + idx, sizeof(Obj*) * (size_ - idx));
        data_[idx] = obj;
        size_++;
    }

    void Array::Replace(size_t idx, const Obj& obj) {
        IndexCheck(idx, false);
        ElemTypeCheck(obj.type());
        *data_[idx]->Origin() = obj;
    }

    void Array::Remove(size_t idx) {
        IndexCheck(idx);
        delete data_[idx];
        memmove(data_ + idx, data_ + idx + 1, sizeof(Obj*) * (size_ - idx - 1));
        size_--;
    }

    Obj* Array::Get(size_t idx) const {
        IndexCheck(idx);
        return data_[idx];
    }

    Array* Array::operator +(const Array& other) {
        ElemTypeCheck(other.elem_type());

        if (!other.elem_type())
            return new Array(*this);            // other: [], empty elem type

        auto arr = new Array(other.elem_type_);
        arr->Expand(size_ + other.size_);
        
        for (size_t i = 0; i < size_; i++) {
            arr->Insert(arr->size_, new Obj(Get(i)->Clone()));
        }
        for (size_t i = 0; i < other.size_; i++) {
            arr->Insert(arr->size_, new Obj(other.Get(i)->Clone()));
        }

        return arr;
    }

    Array& Array::operator =(const Array& other) {
        if (this == &other) return *this;

        ElemTypeCheck(other.elem_type());
        Clear();
        data_ = new Obj*[other.capacity_];
        capacity_ = other.capacity_;
        if (other.elem_type_)
            elem_type_ = other.elem_type_;      // other: [], empty elem type

        for (size_t i = 0; i < other.size_; i++) {
            Insert(i, new Obj(other.Get(i)->Clone()));
        }

        return *this;
    }

    Array& Array::operator +=(const Array& other) {
        for (size_t i = 0; i < other.size_; i++)
            Insert(size_, new Obj(other.Get(i)->Clone()));
        return *this;
    }
}
