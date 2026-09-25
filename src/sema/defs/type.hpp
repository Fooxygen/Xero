
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <iterator>

#include "common/log.hpp"
#include "sema/defs/fn.hpp"

namespace sema {

    class Type {
    public:
        enum class Using {
            Basic,          // array
            Parametric,     // array[=i32=]
            Reference,      // array&
            Binding         // T
        };
    
    private:
        std::string name_       = "";
        Using       type_using_ = Using::Basic;

        std::set<Type*>                          casts_;
        std::unordered_map<Type*, const FnSign*> casts_fnsign_;     // signature of cast method

    public:
        Type(std::string name, Using type_using)
        :   name_(name), type_using_(type_using) {}
        
        virtual ~Type() = default;

        const std::string& name()       const { return name_; }
        Using              type_using() const { return type_using_; }

        std::set<Type*>& casts() { return casts_; }
        std::unordered_map<Type*, const FnSign*>& casts_fnsign() { return casts_fnsign_; }

    public:
        bool IsNone();
        bool Is(std::string_view name);

        virtual Type* BasicTypeGet() = 0;
        void          BasicTypeCheck() const;

        virtual Type* ReferenceUnwrap() = 0;
    };

    class BindingType    : public Type {
    public:
        BindingType(const std::string& name)
        :   Type(name, Using::Binding) {}

        Type* BasicTypeGet()    override { return this; }
        Type* ReferenceUnwrap() override { return this; }
    };

    class BasicType      : public Type {
    private:
        size_t                    params_cnt_     = 0;      // number of parameters in parametric type
        std::vector<BindingType*> params_binding_ = {};     // binding types
        FnTable                   method_table_;

    public:
        BasicType(std::string name, size_t params_cnt = 0)
        :   Type(name, Using::Basic),
            params_cnt_(params_cnt)
        {
            method_table_.OwnerSet(this);
        }

        size_t                     params_cnt() const { return params_cnt_; }
        std::vector<BindingType*>& params_binding()   { return params_binding_; }
        FnTable&                   method_table()     { return method_table_; }
    
    public:
        Type* BasicTypeGet()    override { return this; }
        Type* ReferenceUnwrap() override { return this; }
    };

    class ParametricType : public Type {
    private:
        Type*              basic_  = nullptr;
        std::vector<Type*> params_ = {};
        FnTable            method_table_;

    public:
        ParametricType(std::string name, Type* basic_type, const std::vector<Type*>& params)
        :   Type(name, Using::Parametric),
            basic_(basic_type),
            params_(params)
        {
            method_table_.OwnerSet(this);
        }

        Type*                     basic()  const { return basic_; }
        std::vector<Type*>&       params()       { return params_; }
        const std::vector<Type*>& params() const { return params_; }
        FnTable&                  method_table() { return method_table_; }

    public:
        static std::string ParamsPrint(Type* basic_type, const std::vector<Type*>& params);

        Type* BasicTypeGet()    override { return basic_; }
        Type* ReferenceUnwrap() override { return this; }
    };

    class ReferenceType  : public Type {
    private:
        Type* referred_  = nullptr;

    public:
        ReferenceType(std::string name, Type* referred)
        :   Type(name, Using::Reference),
            referred_(referred)
        {}

        Type* referred() const { return referred_; }

    public:
        Type* BasicTypeGet()    override { return referred_->BasicTypeGet(); }
        Type* ReferenceUnwrap() override { return referred_; }
    };

    class TypeTable {
    private:
        static inline std::unordered_map<std::string, std::unique_ptr<Type>> table_;
        static inline std::map<std::set<Type*>, Type*>                       commons_cache_;

    public:
        static void   Init();

        static BasicType*      Set(const BasicType& t);
        static ParametricType* Set(const ParametricType& t);
        static ReferenceType*  Set(const ReferenceType& t);

        static Type*  Lookup(std::string_view name, std::optional<Loc> loc = std::nullopt);
        static Type*  LookupTry(std::string_view name);
        
        static Type*  ParametricTypeGet(Type* type, const std::vector<Type*>& params, std::optional<Loc> loc = std::nullopt);
        static Type*  ReferenceTypeGet(Type* type);

        static bool   IsContainBindingType(Type* type);
        static bool   IsContainBindingType(const FnSign& sign);
        static Type*  BindingTypeReplace(Type* type, BasicType* owner, const std::vector<Type*>& params_replace);
        static FnSign SignInstantiate(const FnSign& sign, BasicType* owner, const std::vector<Type*>& params_replace);
        
        static Fn*    MethodLookup(Type* type, const std::string& name);
        static Fn*    MethodLookupTry(Type* type, const std::string& name);

        static void   CastRecompute();
        static Type*  CommonTypeGet(std::set<Type*> ts);
    };
}
