#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        types.hpp
 * @brief       Single Source of Truth for sentinel values and foundation types
 * @description All sentinel values used across the codebase are defined here.
 *              NO runtime values - those belong in Components!
 *
 *              Import via:
 *                using ase::types::InvalidEntityId;
 *                using ase::types::FloatNotFound;
 *                using ase::types::is_negative_float;
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @created     2026-01-01
 * @modified    2026-02-02
 * @version     1.0.0
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] NO structs (structs belong in Components)
 */

#include <cstdint>
#include <cfloat>

#include <ase/types/option.hpp>
#include <ase/types/result.hpp>

namespace ase::types {

/**
 * SENTINEL VALUES (SSOT)
 * Special marker values signaling "invalid/not found/unset".
 * Used across ALL modules for consistent error detection.
 */
constexpr uint32_t InvalidEntityId = UINT32_MAX;   // Invalid entity (Entity 0 is VALID in EnTT!)
constexpr float FloatNotFound = -FLT_MAX;          // Float "not found" sentinel (-3.4e38)
constexpr float FloatUnset = FLT_MAX;              // Float "unset/default" sentinel (+3.4e38)
constexpr double DoubleNotFound = -DBL_MAX;        // Double "not found" sentinel (-1.7e308)
constexpr double DoubleUnset = DBL_MAX;            // Double "unset/default" sentinel (+1.7e308)
constexpr uint8_t InvalidUint8Id = 255;            // Invalid uint8 ID sentinel (UINT8_MAX)
constexpr uint16_t InvalidUint16Id = 65535;        // Invalid uint16 ID sentinel (UINT16_MAX)
constexpr uint32_t InvalidUint32Id = UINT32_MAX;   // Invalid uint32 ID sentinel
constexpr uint64_t InvalidUint64Id = UINT64_MAX;   // Invalid uint64 ID sentinel
constexpr uint64_t InvalidHash = 0;                // Invalid hash sentinel (0 = no hash computed)

/**
 * FLOAT SENTINEL CHECKS
 * Functions for consistent float sentinel checking across modules.
 */
constexpr bool is_val_float(float v) { return v > FloatNotFound && v < FloatUnset; }  // Valid (not sentinel)
constexpr bool is_neg_float(float v) { return v > FloatNotFound && v < 0.0f; }        // Negative (excluding NOT_FOUND)
constexpr bool is_pos_float(float v) { return v > 0.0f && v < FloatUnset; }           // Positive (excluding UNSET)
constexpr bool is_zero_float(float v) { return v == 0.0f; }                           // Exactly zero
constexpr bool is_not_found(float v) { return v <= FloatNotFound; }                   // Is NOT_FOUND sentinel
constexpr bool is_unset(float v) { return v >= FloatUnset; }                          // Is UNSET sentinel

/**
 * ENTITY ID SENTINEL CHECKS
 * Functions for consistent entity ID checking across modules.
 */
constexpr bool is_val_entity(uint32_t id) { return id != InvalidEntityId; }           // Valid entity ID
constexpr bool is_inv_entity(uint32_t id) { return id == InvalidEntityId; }           // Invalid entity ID

/**
 * UINT8 SENTINEL CHECKS
 * Functions for consistent uint8 ID checking across modules.
 */
constexpr bool is_val_uint8(uint8_t id) { return id != InvalidUint8Id; }              // Valid uint8 ID
constexpr bool is_inv_uint8(uint8_t id) { return id == InvalidUint8Id; }              // Invalid uint8 ID

/**
 * UINT16 SENTINEL CHECKS
 * Functions for consistent uint16 ID checking across modules.
 */
constexpr bool is_val_uint16(uint16_t id) { return id != InvalidUint16Id; }           // Valid uint16 ID
constexpr bool is_inv_uint16(uint16_t id) { return id == InvalidUint16Id; }           // Invalid uint16 ID

/**
 * UINT32 SENTINEL CHECKS
 * Functions for consistent uint32 ID checking across modules.
 */
constexpr bool is_val_uint32(uint32_t id) { return id != InvalidUint32Id; }           // Valid uint32 ID
constexpr bool is_inv_uint32(uint32_t id) { return id == InvalidUint32Id; }           // Invalid uint32 ID

/**
 * UINT64 SENTINEL CHECKS
 * Functions for consistent uint64 ID checking across modules.
 */
constexpr bool is_val_uint64(uint64_t id) { return id != InvalidUint64Id; }           // Valid uint64 ID
constexpr bool is_inv_uint64(uint64_t id) { return id == InvalidUint64Id; }           // Invalid uint64 ID

/**
 * HASH SENTINEL CHECKS
 * Functions for hash validity (0 = invalid/not computed).
 */
constexpr bool is_val_hash(uint64_t h) { return h != InvalidHash; }                   // Valid hash (non-zero)
constexpr bool is_inv_hash(uint64_t h) { return h == InvalidHash; }                   // Invalid hash (zero)

/**
 * DOUBLE SENTINEL CHECKS
 * Functions for consistent double sentinel checking across modules.
 */
constexpr bool is_val_double(double v) { return v > DoubleNotFound && v < DoubleUnset; }  // Valid (not sentinel)
constexpr bool is_neg_double(double v) { return v > DoubleNotFound && v < 0.0; }          // Negative (excluding NOT_FOUND)
constexpr bool is_pos_double(double v) { return v > 0.0 && v < DoubleUnset; }             // Positive (excluding UNSET)
constexpr bool is_zero_double(double v) { return v == 0.0; }                              // Exactly zero

/**
 * RANGE CHECKS
 * Functions for value range validation (inclusive bounds).
 */
constexpr bool is_in_rng_f(float v, float min, float max) { return v >= min && v <= max; }      // Float in range
constexpr bool is_in_rng_d(double v, double min, double max) { return v >= min && v <= max; }   // Double in range
constexpr bool is_in_rng_u8(uint8_t v, uint8_t min, uint8_t max) { return v >= min && v <= max; }   // uint8 in range
constexpr bool is_in_rng_u16(uint16_t v, uint16_t min, uint16_t max) { return v >= min && v <= max; }  // uint16 in range
constexpr bool is_in_rng_u32(uint32_t v, uint32_t min, uint32_t max) { return v >= min && v <= max; }  // uint32 in range
constexpr bool is_in_rng_u64(uint64_t v, uint64_t min, uint64_t max) { return v >= min && v <= max; }  // uint64 in range
constexpr bool is_in_rng_i32(int32_t v, int32_t min, int32_t max) { return v >= min && v <= max; }     // int32 in range

/**
 * ABBREVIATIONS (Documentation)
 * See: WRFL_ASE_NAMING_SCHEMA.md and data/taxonomy/*.json
 *
 * │ Full Word │ Abbr │ Example                    │
 * │───────────│──────│────────────────────────────│
 * │ types     │ -    │ types.hpp (no abbreviation)│
 * │ option    │ opt  │ option.hpp                 │
 * │ result    │ res  │ result.hpp                 │
 */

}  // namespace ase::types
