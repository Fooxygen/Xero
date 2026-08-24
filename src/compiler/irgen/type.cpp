
//  Xero
//  Copyright (c) 2026 Fooxygen.
//  Licensed under the MIT License.

#include <format>

#include "compiler/irgen/irgen.hpp"
#include "compiler/irgen/type.hpp"

#include "common/log.hpp"

namespace compiler {

    // TypeGenTable
    
    void TypeGenTable::Init() {

        // Infos
        {
            // none
            TypeGenTable::Set(sema::TypeTable::Lookup("none"), TypeGen{});

            // bool
            TypeGenTable::Set(sema::TypeTable::Lookup("bool"), TypeGen{
                .eq_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpEQ(l, r);
                },
                .neq_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpNE(l, r);
                },
                .and_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateAnd(l, r);
                },
                .or_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateOr(l, r);
                },
                .not_ = [](IRGen& gen, llvm::Value* v) {
                    return gen.builder().CreateNot(v);
                },
            });

            // i32
            TypeGenTable::Set(sema::TypeTable::Lookup("i32"), TypeGen{
                .plus_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateAdd(l, r);
                },
                .minus_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateSub(l, r);
                },
                .star_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateMul(l, r);
                },
                .slash_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateSDiv(l, r);
                },
                .neg_   = [](IRGen& gen, llvm::Value* v) {
                    return gen.builder().CreateNeg(v);
                },
                .modt_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateSRem(l, r);
                },
                .modf_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    auto& builder   = gen.builder();
                    auto  zero      = builder.getInt32(0);
                    auto  modt      = builder.CreateSRem(l, r);             // l % r
                    
                    auto  isModTNeqZero = builder.CreateICmpNE(modt, zero); // modt != 0
                    auto  isDiffSign    = builder.CreateXor(                // (modt < 0) != (r < 0)
                        builder.CreateICmpSLT(modt, zero),
                        builder.CreateICmpSLT(r, zero)
                    );
                    auto  hasRevise = builder.CreateAnd(
                        isModTNeqZero, isDiffSign
                    );

                    return builder.CreateAdd(modt,
                        builder.CreateSelect(hasRevise, r, zero)
                    );
                },
                .gt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSGT(l, r);
                },
                .lt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSLT(l, r);
                },
                .ge_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSGE(l, r);
                },
                .le_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSLE(l, r);
                },
                .eq_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpEQ(l, r);
                },
                .neq_   = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpNE(l, r);
                },
            });

            // i64
            TypeGenTable::Set(sema::TypeTable::Lookup("i64"), TypeGen{
                .plus_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateAdd(l, r);
                },
                .minus_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateSub(l, r);
                },
                .star_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateMul(l, r);
                },
                .slash_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateSDiv(l, r);
                },
                .neg_   = [](IRGen& gen, llvm::Value* v) {
                    return gen.builder().CreateNeg(v);
                },
                .modt_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateSRem(l, r);
                },
                .modf_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    auto& builder   = gen.builder();
                    auto  zero      = builder.getInt64(0);
                    auto  modt      = builder.CreateSRem(l, r);             // l % r
                    
                    auto  isModTNeqZero = builder.CreateICmpNE(modt, zero); // modt != 0
                    auto  isDiffSign    = builder.CreateXor(                // (modt < 0) != (r < 0)
                        builder.CreateICmpSLT(modt, zero),
                        builder.CreateICmpSLT(r, zero)
                    );
                    auto  hasRevise = builder.CreateAnd(
                        isModTNeqZero, isDiffSign
                    );

                    return builder.CreateAdd(modt,
                        builder.CreateSelect(hasRevise, r, zero)
                    );
                },
                .gt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSGT(l, r);
                },
                .lt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSLT(l, r);
                },
                .ge_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSGE(l, r);
                },
                .le_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpSLE(l, r);
                },
                .eq_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpEQ(l, r);
                },
                .neq_   = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateICmpNE(l, r);
                },
            });

            // f32
            TypeGenTable::Set(sema::TypeTable::Lookup("f32"), TypeGen{
                .plus_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFAdd(l, r);
                },
                .minus_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFSub(l, r);
                },
                .star_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFMul(l, r);
                },
                .slash_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFDiv(l, r);
                },
                .neg_   = [](IRGen& gen, llvm::Value* v) {
                    return gen.builder().CreateFNeg(v);
                },
                .modt_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFRem(l, r);
                },
                .modf_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    auto& builder   = gen.builder();
                    auto  zero      = llvm::ConstantFP::get(builder.getFloatTy(), 0.0);
                    auto  modt      = builder.CreateFRem(l, r);                 // l % r
                    
                    auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);    // modt != 0
                    auto  isDiffSign    = builder.CreateXor(                    // (modt < 0) != (r < 0)
                        builder.CreateFCmpOLT(modt, zero),
                        builder.CreateFCmpOLT(r, zero)
                    );
                    auto  hasRevise = builder.CreateAnd(
                        isModTNeqZero, isDiffSign
                    );

                    return builder.CreateFAdd(modt,
                        builder.CreateSelect(hasRevise, r, zero)
                    );
                },
                .gt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOGT(l, r);
                },
                .lt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOLT(l, r);
                },
                .ge_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOGE(l, r);
                },
                .le_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOLE(l, r);
                },
                .eq_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOEQ(l, r);
                },
                .neq_   = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpONE(l, r);
                },
            });

            // f64
            TypeGenTable::Set(sema::TypeTable::Lookup("f64"), TypeGen{
                .plus_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFAdd(l, r);
                },
                .minus_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFSub(l, r);
                },
                .star_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFMul(l, r);
                },
                .slash_ = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFDiv(l, r);
                },
                .neg_   = [](IRGen& gen, llvm::Value* v) {
                    return gen.builder().CreateFNeg(v);
                },
                .modt_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFRem(l, r);
                },
                .modf_  = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    auto& builder   = gen.builder();
                    auto  zero      = llvm::ConstantFP::get(builder.getDoubleTy(), 0.0);
                    auto  modt      = builder.CreateFRem(l, r);                 // l % r
                    
                    auto  isModTNeqZero = builder.CreateFCmpONE(modt, zero);    // modt != 0
                    auto  isDiffSign    = builder.CreateXor(                    // (modt < 0) != (r < 0)
                        builder.CreateFCmpOLT(modt, zero),
                        builder.CreateFCmpOLT(r, zero)
                    );
                    auto  hasRevise = builder.CreateAnd(
                        isModTNeqZero, isDiffSign
                    );

                    return builder.CreateFAdd(modt,
                        builder.CreateSelect(hasRevise, r, zero)
                    );
                },
                .gt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOGT(l, r);
                },
                .lt_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOLT(l, r);
                },
                .ge_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOGE(l, r);
                },
                .le_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOLE(l, r);
                },
                .eq_    = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpOEQ(l, r);
                },
                .neq_   = [](IRGen& gen, llvm::Value* l, llvm::Value* r) {
                    return gen.builder().CreateFCmpONE(l, r);
                },
            });
        }

        // Casts
        {
            auto i32_ = sema::TypeTable::Lookup("i32");
            auto i64_ = sema::TypeTable::Lookup("i64");
            auto f32_ = sema::TypeTable::Lookup("f32");
            auto f64_ = sema::TypeTable::Lookup("f64");

            CastSet(i32_, i64_, [](IRGen& gen, llvm::Value* v) {
                return gen.builder().CreateSExt(v, gen.builder().getInt64Ty());
            });
            CastSet(i32_, f32_, [](IRGen& gen, llvm::Value* v) {
                return gen.builder().CreateSIToFP(v, gen.builder().getFloatTy());
            });
            CastSet(i32_, f64_, [](IRGen& gen, llvm::Value* v) {
                return gen.builder().CreateSIToFP(v, gen.builder().getDoubleTy());
            });
            CastSet(i64_, f32_, [](IRGen& gen, llvm::Value* v) {
                return gen.builder().CreateSIToFP(v, gen.builder().getFloatTy());
            });
            CastSet(i64_, f64_, [](IRGen& gen, llvm::Value* v) {
                return gen.builder().CreateSIToFP(v, gen.builder().getDoubleTy());
            });
            CastSet(f32_, f64_, [](IRGen& gen, llvm::Value* v) {
                return gen.builder().CreateFPExt(v, gen.builder().getDoubleTy());
            });
        }
    }

    llvm::Value* TypeGenTable::Cast(
        IRGen& gen, llvm::Value* value,
        const sema::Type* from, const sema::Type* to)
    {
        if (from == to) return value;
        auto it = casts_.find(std::pair{ from, to });
        if (it != casts_.end()) return it->second(gen, value);
        
        throw LogErr(LogModule::Compiler, std::format(
            "cannot make type '{}' compatible with '{}'",
            from->name_, to->name_
        ));
    }
}
