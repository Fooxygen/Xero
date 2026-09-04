
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "sema/defs/type.hpp"

namespace sema {

    // TypeTable

    void  TypeTable::Init() {

        auto none_          = (BasicType*)TypeTable::Set(BasicType("none"));
        auto bool_          = (BasicType*)TypeTable::Set(BasicType("bool"));
        auto i32_           = (BasicType*)TypeTable::Set(BasicType("i32"));
        auto i64_           = (BasicType*)TypeTable::Set(BasicType("i64"));
        auto f32_           = (BasicType*)TypeTable::Set(BasicType("f32"));
        auto f64_           = (BasicType*)TypeTable::Set(BasicType("f64"));
        auto char_          = (BasicType*)TypeTable::Set(BasicType("char"));
        /*auto string_        = (BasicType*)*/TypeTable::Set(BasicType("string"));
        /*auto stringview_    = (BasicType*)*/TypeTable::Set(BasicType("stringview"));
        auto array_         = (BasicType*)TypeTable::Set(BasicType("array", 1));
        /*auto arrayview_     = (BasicType*)*/TypeTable::Set(BasicType("arrayview"));
        auto range_         = (BasicType*)TypeTable::Set(BasicType("range", 1));
        /*auto function_      = (BasicType*)*/TypeTable::Set(BasicType("function"));

        // Cast
        {
            i32_->method_table().Add("@cast", FnSign(i64_, { i32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
            i32_->method_table().Add("@cast", FnSign(f32_, { i32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
            i32_->method_table().Add("@cast", FnSign(f64_, { i32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));

            i64_->method_table().Add("@cast", FnSign(f32_, { i64_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
            i64_->method_table().Add("@cast", FnSign(f64_, { i64_ }, std::nullopt, FnModifier{ .hasCast_ = true }));

            f32_->method_table().Add("@cast", FnSign(f64_, { f32_ }, std::nullopt, FnModifier{ .hasCast_ = true }));
        
            TypeTable::CastRecompute();
        }

        // Deepcopy
        {
            bool_->method_table().Add("@deepcopy",  FnSign(bool_,   { bool_ }));
            i32_->method_table().Add("@deepcopy",   FnSign(i32_,    { i32_ }));
            i64_->method_table().Add("@deepcopy",   FnSign(i64_,    { i64_ }));
            f32_->method_table().Add("@deepcopy",   FnSign(f32_,    { f32_ }));
            f64_->method_table().Add("@deepcopy",   FnSign(f64_,    { f64_ }));
            char_->method_table().Add("@deepcopy",  FnSign(char_,   { char_ }));
            array_->method_table().Add("@deepcopy", FnSign(array_,  { array_ }));
            range_->method_table().Add("@deepcopy", FnSign(range_,  { range_ }));
        }

        // Other
        {
            // bool
            {
                bool_->method_table().Add("@print", FnSign(none_, { bool_ }));
                bool_->method_table().Add("@eq",    FnSign(bool_, { bool_, bool_ }));
                bool_->method_table().Add("@neq",   FnSign(bool_, { bool_, bool_ }));
                bool_->method_table().Add("@and",   FnSign(bool_, { bool_, bool_ }));
                bool_->method_table().Add("@or",    FnSign(bool_, { bool_, bool_ }));
                bool_->method_table().Add("@not",   FnSign(bool_, { bool_ }));
            }

            // i32
            {
                i32_->method_table().Add("@print", FnSign(none_, { i32_ }));
                i32_->method_table().Add("@plus",  FnSign(i32_,  { i32_, i32_ }));
                i32_->method_table().Add("@minus", FnSign(i32_,  { i32_, i32_ }));
                i32_->method_table().Add("@star",  FnSign(i32_,  { i32_, i32_ }));
                i32_->method_table().Add("@slash", FnSign(i32_,  { i32_, i32_ }));
                i32_->method_table().Add("@neg",   FnSign(i32_,  { i32_ }));
                i32_->method_table().Add("@modt",  FnSign(i32_,  { i32_, i32_ }));
                i32_->method_table().Add("@modf",  FnSign(i32_,  { i32_, i32_ }));
                i32_->method_table().Add("@gt",    FnSign(bool_, { i32_, i32_ }));
                i32_->method_table().Add("@lt",    FnSign(bool_, { i32_, i32_ }));
                i32_->method_table().Add("@ge",    FnSign(bool_, { i32_, i32_ }));
                i32_->method_table().Add("@le",    FnSign(bool_, { i32_, i32_ }));
                i32_->method_table().Add("@eq",    FnSign(bool_, { i32_, i32_ }));
                i32_->method_table().Add("@neq",   FnSign(bool_, { i32_, i32_ }));
            }

            // i64
            {
                i64_->method_table().Add("@print",  FnSign(none_, { i64_ }));
                i64_->method_table().Add("@plus",   FnSign(i64_,  { i64_, i64_ }));
                i64_->method_table().Add("@minus",  FnSign(i64_,  { i64_, i64_ }));
                i64_->method_table().Add("@star",   FnSign(i64_,  { i64_, i64_ }));
                i64_->method_table().Add("@slash",  FnSign(i64_,  { i64_, i64_ }));
                i64_->method_table().Add("@neg",    FnSign(i64_,  { i64_ }));
                i64_->method_table().Add("@modt",   FnSign(i64_,  { i64_, i64_ }));
                i64_->method_table().Add("@modf",   FnSign(i64_,  { i64_, i64_ }));
                i64_->method_table().Add("@gt",     FnSign(bool_, { i64_, i64_ }));
                i64_->method_table().Add("@lt",     FnSign(bool_, { i64_, i64_ }));
                i64_->method_table().Add("@ge",     FnSign(bool_, { i64_, i64_ }));
                i64_->method_table().Add("@le",     FnSign(bool_, { i64_, i64_ }));
                i64_->method_table().Add("@eq",     FnSign(bool_, { i64_, i64_ }));
                i64_->method_table().Add("@neq",    FnSign(bool_, { i64_, i64_ }));
            }

            // f32
            {
                f32_->method_table().Add("@print",  FnSign(none_, { f32_ }));
                f32_->method_table().Add("@plus",   FnSign(f32_,  { f32_, f32_ }));
                f32_->method_table().Add("@minus",  FnSign(f32_,  { f32_, f32_ }));
                f32_->method_table().Add("@star",   FnSign(f32_,  { f32_, f32_ }));
                f32_->method_table().Add("@slash",  FnSign(f32_,  { f32_, f32_ }));
                f32_->method_table().Add("@neg",    FnSign(f32_,  { f32_ }));
                f32_->method_table().Add("@modt",   FnSign(f32_,  { f32_, f32_ }));
                f32_->method_table().Add("@modf",   FnSign(f32_,  { f32_, f32_ }));
                f32_->method_table().Add("@gt",     FnSign(bool_, { f32_, f32_ }));
                f32_->method_table().Add("@lt",     FnSign(bool_, { f32_, f32_ }));
                f32_->method_table().Add("@ge",     FnSign(bool_, { f32_, f32_ }));
                f32_->method_table().Add("@le",     FnSign(bool_, { f32_, f32_ }));
                f32_->method_table().Add("@eq",     FnSign(bool_, { f32_, f32_ }));
                f32_->method_table().Add("@neq",    FnSign(bool_, { f32_, f32_ }));
            }

            // f64
            {
                f64_->method_table().Add("@print",  FnSign(none_, { f64_ }));
                f64_->method_table().Add("@plus",   FnSign(f64_,  { f64_, f64_ }));
                f64_->method_table().Add("@minus",  FnSign(f64_,  { f64_, f64_ }));
                f64_->method_table().Add("@star",   FnSign(f64_,  { f64_, f64_ }));
                f64_->method_table().Add("@slash",  FnSign(f64_,  { f64_, f64_ }));
                f64_->method_table().Add("@neg",    FnSign(f64_,  { f64_ }));
                f64_->method_table().Add("@modt",   FnSign(f64_,  { f64_, f64_ }));
                f64_->method_table().Add("@modf",   FnSign(f64_,  { f64_, f64_ }));
                f64_->method_table().Add("@gt",     FnSign(bool_, { f64_, f64_ }));
                f64_->method_table().Add("@lt",     FnSign(bool_, { f64_, f64_ }));
                f64_->method_table().Add("@ge",     FnSign(bool_, { f64_, f64_ }));
                f64_->method_table().Add("@le",     FnSign(bool_, { f64_, f64_ }));
                f64_->method_table().Add("@eq",     FnSign(bool_, { f64_, f64_ }));
                f64_->method_table().Add("@neq",    FnSign(bool_, { f64_, f64_ }));
            }

            // char
            {
                char_->method_table().Add("@print", FnSign(none_, { char_ }));
                char_->method_table().Add("@gt",    FnSign(bool_, { char_, char_ }));
                char_->method_table().Add("@lt",    FnSign(bool_, { char_, char_ }));
                char_->method_table().Add("@ge",    FnSign(bool_, { char_, char_ }));
                char_->method_table().Add("@le",    FnSign(bool_, { char_, char_ }));
                char_->method_table().Add("@eq",    FnSign(bool_, { char_, char_ }));
                char_->method_table().Add("@neq",   FnSign(bool_, { char_, char_ }));
            }
            
            /*
            // string
            {
                string_->method_table().Add("len",   FnSign(i32_));
                string_->method_table().Add("clear", FnSign(none_));
            }

            // stringview
            {
                stringview_->method_table().Add("len",       FnSign(i32_));
                stringview_->method_table().Add("to_string", FnSign(string_));
            }
            */

            // array
            {
                array_->method_table().Add("@len",          FnSign(i32_,  { array_ }));
                array_->method_table().Add("@clear",        FnSign(none_, { array_ }));
                array_->method_table().Add("@insert",       FnSign(none_, { array_, i32_, nullptr }));
                array_->method_table().Add("@remove",       FnSign(none_, { array_, i32_ }));
                array_->method_table().Add("@push_front",   FnSign(none_, { array_, nullptr }));
                array_->method_table().Add("@pop_front",    FnSign(none_, { array_ }));
                array_->method_table().Add("@push_back",    FnSign(none_, { array_, nullptr }));
                array_->method_table().Add("@pop_back",     FnSign(none_, { array_ }));
            }

            /*
            // arrayview
            {
                arrayview_->method_table().Add("len",        FnSign(i32_));
                arrayview_->method_table().Add("to_array",   FnSign(array_));
            }
            */

            // range
            {
                range_->method_table().Add("@print",    FnSign(none_, { range_ }));
            }

            /*
            // function
            {
                
            }
            */
        }
    }
}
