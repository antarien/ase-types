#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        hexgrid_wire.hpp
 * @brief       Frozen hex-lattice seam - chunk-id packing SSOT, hub bit-pattern codec, intent PODs
 * @description The ONE place the planetary hex lattice crosses a module boundary. Three subjects
 *              live here, all fixed by PLAN_ASE_LATTICE_PHASE_00_CONTRACT.md:
 *
 *              1. THE CHUNK-ID PACKING (WS-K.4, DECISION D5 option A). The u64 packing of a chunk
 *                 address used to sit in L3 terrain and had been copied twice into L3 gis - an
 *                 L3-to-L3 include would have been the layer violation that produced the copies in
 *                 the first place. The one definition lives HERE, below every consumer.
 *
 *              2. THE HUB BIT-PATTERN CODEC (WS-K.2 trap 3). A cell coordinate is an int32 and
 *                 must never enter float arithmetic - the hub value slot is f32 and everything
 *                 above 2^24 would come back corrupt. The codec moves the BIT PATTERN, never the
 *                 number, so producer and consumer agree byte for byte.
 *
 *              3. THE INTENT PODS (WS-K.2b). The cell-interaction channel and the project
 *                 spawn-cell request. A producer module emplaces the POD on a request entity, the
 *                 consumer module iterates a View over a type whose definition sits BELOW both -
 *                 no L3-to-L3 include, and Layer 0 stays ECS-free because only the POD and the
 *                 empty tags live here (emplace/View usage is L3/L4 only). Precedent: the
 *                 CapacityReqXmitComponent / CapacityReq*Tag pair in region_wire.hpp.
 *
 *              region_wire.hpp is UNTOUCHED by this plan - the lattice seam is a new header in the
 *              same L0 module, so there is no wire frame and no contract amendment (WS-K.5).
 *
 *              Import via:
 *                #include <ase/types/hexgrid_wire.hpp>
 *                using ase::types::cell_to_chunk_id;
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @created     2026-07-31
 * @modified    2026-07-31
 * @version     1.0.0
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed) - N/A, no entity reference travels here
 * [ ] Abbreviations documented
 * [ ] Wire layouts stated as offsets/sizes, not sizeof() of padded structs
 */

#include <cstdint>
#include <cstring>

namespace ase::types {

// ---------------------------------------------------------------------------
// CHUNK-ID PACKING (SSOT) - 20 bits per axis, centre-offset, x | y<<20 | z<<40
//
// Bit layout taken over UNCHANGED from the terrain constants it supersedes
// (modules/ase-terrain/include/ase/terrain/types.hpp:182-184 and :655-659). The
// values are mirrored, not included, because Layer 0 must not include Layer 3;
// the identity of both packings is pinned by a test.
// ---------------------------------------------------------------------------

/** Bits one axis occupies inside the packed chunk id. */
constexpr uint32_t CHUNK_ID_BITS = 20u;

/** Mask of one packed axis - the low CHUNK_ID_BITS bits. */
constexpr uint32_t CHUNK_ID_MASK = (1u << CHUNK_ID_BITS) - 1u;

/** Centre offset that maps the signed axis range onto the unsigned packed range. */
constexpr int32_t CHUNK_ID_OFFSET = 1 << (CHUNK_ID_BITS - 1);

/**
 * The fixed vertical axis of a 2D cell packing.
 *
 * A lattice cell IS a chunk address (cx,cz) - two dimensions - while the packing is three
 * dimensional. CELL_CHUNK_Y closes that arity gap once, so two modules can never derive a
 * different u64 for the same cell. Zero is the only reading the existing code allows: the chunk
 * grid is the coordinate system and y is intra-chunk height, not a partition axis
 * (PLAN_ASE_COMPUTE.md:193), the region rect carries no y at all (region_wire.hpp:151-163), and
 * the terrain coordinate component documents its vertical slice as 0 in the flat case.
 */
constexpr int32_t CELL_CHUNK_Y = 0;

/**
 * @brief Pack a three dimensional chunk address into its u64 id
 * @param cx Chunk grid X
 * @param cy Chunk grid Y (vertical slice)
 * @param cz Chunk grid Z
 * @return Packed 64 bit chunk id
 *
 * This is the ONE packing of the engine. Every other spelling of it is a clone to be migrated
 * onto this function, never a second definition.
 */
constexpr uint64_t chunk_coords_to_id(int32_t cx, int32_t cy, int32_t cz) {
    uint64_t x_part = static_cast<uint64_t>(cx + CHUNK_ID_OFFSET) & CHUNK_ID_MASK;
    uint64_t y_part = static_cast<uint64_t>(cy + CHUNK_ID_OFFSET) & CHUNK_ID_MASK;
    uint64_t z_part = static_cast<uint64_t>(cz + CHUNK_ID_OFFSET) & CHUNK_ID_MASK;
    return x_part | (y_part << CHUNK_ID_BITS) | (z_part << (CHUNK_ID_BITS * 2));
}

/**
 * @brief Pack a lattice cell address into its u64 chunk id
 * @param cx Cell chunk X
 * @param cz Cell chunk Z
 * @return Packed 64 bit chunk id at CELL_CHUNK_Y
 *
 * The 2D form every lattice consumer uses. It is the 3D packing at the fixed vertical axis, so a
 * cell id and the ground level chunk id of the same address are the SAME number by construction.
 */
constexpr uint64_t cell_to_chunk_id(int32_t cx, int32_t cz) {
    return chunk_coords_to_id(cx, CELL_CHUNK_Y, cz);
}

// ---------------------------------------------------------------------------
// HUB BIT-PATTERN CODEC - an int32 coordinate through an f32 hub slot
//
// The hub value slot is a float. A coordinate is NOT a number here: the f32
// carries the int32 BIT PATTERN, so nothing ever rounds and the 2^24 mantissa
// cliff cannot be reached. The pattern is composed of the two 16 bit halves
// HI (upper) and LO (lower); the sign of a negative coordinate is restored from
// the halves, never from a float comparison.
//
// House convention, measured: the SPT_CHUNK_ID_HIGH / SPT_CHUNK_ID_LOW pair
// already moves u32 halves this way (gis_qry_sys.cpp:206, gis_lyr_wrt_sys.cpp:175).
// ---------------------------------------------------------------------------

/** Bits one half of the coordinate pattern occupies. */
constexpr uint32_t CELL_COORD_HALF_BITS = 16u;

/** Mask of one 16 bit half of the coordinate pattern. */
constexpr uint32_t CELL_COORD_HALF_MASK = 0xFFFFu;

/**
 * @brief Encode an int32 cell coordinate into the f32 hub slot as a bit pattern
 * @param coord Cell coordinate, any int32 including negative values
 * @return The f32 whose bits ARE the coordinate - never read this as a number
 */
inline float cell_coord_to_hub_pattern(int32_t coord) {
    uint32_t raw = static_cast<uint32_t>(coord);
    uint32_t hi = (raw >> CELL_COORD_HALF_BITS) & CELL_COORD_HALF_MASK;
    uint32_t lo = raw & CELL_COORD_HALF_MASK;
    uint32_t bits = (hi << CELL_COORD_HALF_BITS) | lo;
    float pattern = 0.0f;
    std::memcpy(&pattern, &bits, sizeof(pattern));
    return pattern;
}

/**
 * @brief Decode an f32 hub slot back into the int32 cell coordinate it carries
 * @param pattern The f32 as read from the hub value
 * @return The original coordinate, sign restored from the two halves
 */
inline int32_t hub_pattern_to_cell_coord(float pattern) {
    uint32_t bits = 0u;
    std::memcpy(&bits, &pattern, sizeof(bits));
    uint32_t hi = (bits >> CELL_COORD_HALF_BITS) & CELL_COORD_HALF_MASK;
    uint32_t lo = bits & CELL_COORD_HALF_MASK;
    return static_cast<int32_t>((hi << CELL_COORD_HALF_BITS) | lo);
}

// ---------------------------------------------------------------------------
// CELL INTERACTION CHANNEL (frozen, WS-K.2b)
//
// One request entity per interaction. The producer creates it on
// Schedule::Production; the consumer drains it on Schedule::Integration of the
// following frame and is the ONLY party that destroys it. Nobody mutates a
// foreign request.
//
// There is deliberately NO owner: the cell address travels as two int32 IN the
// payload. That is what dissolves the coordinate-owner ban of
// PLAN_ASE_COMPUTE.md:326 instead of working around it - no hash, no u32
// truncation, no f32 mantissa cliff.
// ---------------------------------------------------------------------------

/**
 * One interaction reported on one lattice cell.
 *
 * weight is a DATA weight, never a type discriminator: how much this interaction counts towards
 * the durable suprastructure change that lifts a marker to a zone (DSGN_019, Z. 97). The producer
 * class is expressed by a tag, so a new class adds a tag and never a field.
 */
struct GisReqCellIntrComponent {
    int32_t cx = 0;       // cell chunk X of the interaction
    int32_t cz = 0;       // cell chunk Z of the interaction
    float weight = 0.0f;  // data weight of this interaction, 0 means no contribution
};

/** The request is pending: written by the producer, cleared by the consumer destroying the entity. */
struct GisReqCellIntrPendTag {};

/**
 * Producer class: an edge-face crossing into the cell.
 *
 * The transition between play zones is detected on the edge faces (DSGN_019, Z. 86) - this is the
 * trigger class the terrain phase produces.
 */
struct GisReqCellIntrEdgeTag {};

/**
 * Producer class: a durable suprastructure change inside the cell.
 *
 * Only a lasting change of the suprastructure carries a cell beyond the resting-place case into
 * persistence (DSGN_019, Z. 97).
 */
struct GisReqCellIntrSupraTag {};

// ---------------------------------------------------------------------------
// PROJECT SPAWN-CELL REQUEST (Master DECISION D4, link 1 of the genesis chain)
//
// A project's spawn cell is derived deterministically from its proj_hash, so the
// request carries nothing but the project label. Same carrier discipline as the
// interaction channel: POD plus tag in L0, emplace and View in L3.
// ---------------------------------------------------------------------------

/**
 * One project that wants its genesis spawn cell resolved.
 *
 * proj_hash is the FNV-1a32 of the project id - the same label the CAP_PROJ_* hub family already
 * uses as its owner, so the resolved cell is published under an owner the bridge already knows.
 */
struct GisReqProjSpwnComponent {
    uint32_t proj_hash = 0u;  // FNV-1a32(proj_id), 0 means unset
};

/** The spawn-cell request is pending: written by the producer, cleared by the consumer. */
struct GisReqProjSpwnPendTag {};

}  // namespace ase::types
