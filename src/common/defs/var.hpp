
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#pragma once

enum VarModifier : int {
    None  = 0,
    Const = 1 << 0,
};

inline VarModifier operator |(VarModifier a, VarModifier b) {
    return VarModifier((int)a | (int)b);
}
inline VarModifier operator &(VarModifier a, VarModifier b) {
    return VarModifier((int)a & (int)b);
}
inline VarModifier operator |=(VarModifier& a, VarModifier b) {
    a = a | b;
    return a;
}
inline VarModifier operator &=(VarModifier& a, VarModifier b) {
    a = a & b;
    return a;
}