
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <charconv>

#include "log.hpp"
#include "xengine.hpp"

namespace rt {
        
    Obj Engine::Exec(IdExpr& node) {
        return *env_.Get(node.value_);
    }

    Obj Engine::Exec(OperExpr& node) {
        
    }

    Obj Engine::Exec(NumConst& node) {
        const auto& numstr = node.value_str_;

        // integer
        if (numstr.find(".") == std::string::npos) {
            
            // i32
            {
                int32_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_i32(x);
            }

            // i64
            {
                int64_t x = 0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_i64(x);
            }
        }

        // float
        else {
            // f32
            {
                float x = 0.0f;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_f32(x);
            }

            // f64
            {
                double x = 0.0;
                auto [ptr, ec] = std::from_chars(numstr.data(), numstr.data() + numstr.size(), x);
                if (ec == std::errc{}) return Obj::Make_f64(x);
            }
        }

        throw LogErr(LogModule::Runtime, "numeric overflow");
        return Obj();
    }

    Obj Engine::Exec(DeclStmt& node) {
        auto value = Exec(*node.value_);
        //auto type = TypeTable::Get(node.id_->value_);
        env_.Declare(node.id_->value_, value);
        return Obj();
    }

    Obj Engine::Exec(Program& node) {
        for (auto& child : node.children()) {
            Exec(*child);
        }
        return Obj();
    }
}
