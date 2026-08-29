
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include "xcompiler/defs/fn.hpp"

namespace xcompiler {
    
    // FnImplTable

    void    FnImplTable::Add(const std::string& name, const sema::FnSign* sign, std::unique_ptr<FnImpl>&& impl) {
        auto it = table_.find(sign);
        if (it != table_.end()) {
            throw LogErr(LogModule::Xcompiler, std::format(
                "redefinition implementation of function '{}', signature is {}",
                name, sign->ParamsPrint()
            ));
        }
        table_[sign] = std::move(impl);
    }

    FnImpl* FnImplTable::LookupTry(const sema::FnSign* sign) {
        auto it = table_.find(sign);
        return it == table_.end() ? nullptr : it->second.get();
    }

    FnImpl* FnImplTable::Lookup(const sema::FnSign* sign) {
        auto impl = LookupTry(sign);
        if (!impl) throw LogErr(LogModule::Xcompiler, std::format(
            "undefined implementation of function '{}', signature it attempts to obtain is {}",
            sign->name(), sign->ParamsPrint()
        ));
        return impl;
    }
}