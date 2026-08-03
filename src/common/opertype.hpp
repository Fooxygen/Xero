
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

#include <string>

enum class OperType {
    Undefined,

    // Arith
    Plus, Minus, Star, Slash, Neg,
    ModT, ModF,

    // Relation
    Gt, Lt, Ge, Le, Eq, Neq,

    // Logical
    And, Or, Not,

    // Container
    Pick,

    // Assign
    // Not participating in OperExpr
};

static std::string OperTypeName(OperType type) {
    using enum OperType;
    switch(type) {
        case Plus:  return "Plus";
        case Minus: return "Minus";
        case Star:  return "Star";
        case Slash: return "Slash";
        case Neg:   return "Neg";
        case ModT:  return "ModT";
        case ModF:  return "ModF";

        case Gt:    return "Gt" ;
        case Lt:    return "Lt" ;
        case Ge:    return "Ge" ;
        case Le:    return "Le" ;
        case Eq:    return "Eq" ;
        case Neq:   return "Neq";

        case And:   return "And";
        case Or:    return "Or" ;
        case Not:   return "Not";

        case Pick:  return "Pick";

        default:    return "Undefined";
    }
}
