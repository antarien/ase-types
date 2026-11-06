#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        hexgrid_wire.hpp
 * @design      DSGN_019
 * @brief       Frozen hex-lattice seam - chunk-id packing SSOT and the hub bit-pattern codec
 * @description The ONE place the planetary hex lattice crosses a module boundary. Two subjects
 *              live here, both fixed by PLAN_ASE_LATTICE_PHASE_00_CONTRACT.md:
 *
 *              1. THE CHUNK-ID PACKING (WS-K.4, DECISION D5 option A). The u64 packing of a chunk
 *                 address is needed by L3 terrain and L3 gis alike; an L3-to-L3 include would be a
 *                 layer violation, so the one definition lives HERE, below every consumer.
 *
 *                 The 2D cell form packs 32 bits per axis (PLAN_ASE_PRESSURE_PHASE_01_ADDR.md),
 *                 the 3D form keeps its 20-bit layout. The lattice address space reaches
 *                 cx = 10223616 (derived in PLAN_ASE_PRESSURE_PHASE_01_ADDR.md), which a 20-bit
 *                 axis cannot hold: in that window every second icosahedron face lands on the
 *                 same key - 5242900 valid cx gave 524290 distinct keys, group size uniformly 10.
 *
 *              2. THE HUB BIT-PATTERN CODEC (WS-K.2 trap 3). A cell coordinate is an int32 and
 *                 must never enter float arithmetic - the hub value slot is f32 and everything
 *                 above 2^24 would come back corrupt. The codec moves the BIT PATTERN, never the
 *                 number, so producer and consumer agree byte for byte.
 *
 *              WHAT THIS HEADER MAY CARRY. A packing rule and a codec - a way to COMPUTE a
 *              number, never an ECS type and never a datum between two modules of one tier. A
 *              place in particular does not travel through Layer 0: ase-terrain and ase-geoid are
 *              both L3 in the World tier and share one registry, so a POD between them is a direct
 *              channel past the middle of the star (ARCH_ASE_HUB.md, Star Schema). The geodetic
 *              place plus its meta ride GEO_POS_LAT_DEG / GEO_POS_LON_DEG / GEO_POS_ALT_M /
 *              GEO_POS_META under the reporting entity as owner. The project spawn-cell request
 *              is a module row of ase-geoid (GeoidReqSpwnComponent, GeoidReqSpwnPendTag), because
 *              both its writer and its reader are systems of that module.
 *
 *              Import via:
 *                #include <ase/types/hexgrid_wire.hpp>
 *                using ase::types::cell_to_chunk_id;
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @category    structure/reference/identifier
 * @usecase     usecase:101035 - define geoid lattice constants
 * @actor       Active Cell Tracker (ACTICELTRA)
 * @created     2026-07-31
 * @modified    2026-10-30
 * @version     1.2.1
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

// CHUNK-ID PACKING (SSOT) - TWO forms, and they are deliberately NOT the same
// number for the same place.
//
// 3D form, chunk_coords_to_id: 20 bits per axis, centre-offset,
// x | y<<20 | z<<40. ase-terrain publishes this layout to the hub from the CHUNK_ID_*
// values of this header, as TER_CONST_CHUNK_BITS and its neighbours (TerrainLyrRegSystem).
// Three axes in a u64 cap at 21 bits
// each, so this form cannot carry the lattice address space and does not try to - it
// carries the HEIGHT axis, where cy really varies.
//
// 2D form, cell_to_chunk_id: 32 bits per axis, no mask and no offset,
// u32(cx)<<32 | u32(cz). A lattice cell has no height, so the 20 bits the 3D form
// spends on the constant CELL_CHUNK_Y are exactly the bits the X axis was
// missing. Redistributing them keeps the key at 64 bits and makes it injective
// over the whole int32 plane. The form is not invented here: it is the one
// already used productively for the same purpose in
// `pack_cell_coord` in modules/ase-replication/src/resource/replica_cell_tplg_resource_manager.cpp
// and in replica_rgn_chnk_resource_manager.cpp.

/** Bits one axis occupies inside the packed chunk id. */
constexpr uint32_t CHUNK_ID_BITS = 20u;

/** Mask of one packed axis - the low CHUNK_ID_BITS bits. */
constexpr uint32_t CHUNK_ID_MASK = (1u << CHUNK_ID_BITS) - 1u;

/** Centre offset that maps the signed axis range onto the unsigned packed range. */
constexpr int32_t CHUNK_ID_OFFSET = 1 << (CHUNK_ID_BITS - 1);

/**
 * The vertical slice a lattice cell sits on - the frozen address invariant.
 *
 * A lattice cell IS a chunk address (cx,cz) - two dimensions. Zero is the only reading the
 * existing code allows: the chunk grid is the coordinate system and y is intra-chunk height, not
 * a partition axis (PLAN_ASE_COMPUTE.md:193), the region rect carries no y at all
 * (the RegionRect declaration in region_wire.hpp carries cx0/cz0/cx1/cz1 and no vertical
 * axis), and the terrain coordinate component documents its vertical slice as
 * 0 in the flat case. That reading is frozen as point (5) of the master freeze row
 * (PLAN_ASE_LATTICE.md:108) and stays frozen.
 *
 * What it is NOT: the arity closer of the 2D cell packing. cell_to_chunk_id packs two axes at
 * full width and passes no y at all. CELL_CHUNK_Y is the slice a caller hands to
 * chunk_coords_to_id when it wants the GROUND CHUNK of a cell in the 3D key space - a different
 * number in a different key space, see cell_to_chunk_id below.
 */
constexpr int32_t CELL_CHUNK_Y = 0;

/** Bits one axis occupies inside the packed 2D cell id - the full width of an int32 axis. */
constexpr uint32_t CELL_ID_AXIS_BITS = 32u;

/** Mask of one packed 2D cell axis - the low CELL_ID_AXIS_BITS bits. */
constexpr uint64_t CELL_ID_AXIS_MASK = 0xFFFFFFFFull;

/**
 * @brief Pack a three dimensional chunk address into its u64 id
 * @param cx Chunk grid X
 * @param cy Chunk grid Y (vertical slice)
 * @param cz Chunk grid Z
 * @return Packed 64 bit chunk id
 *
 * This is the ONE packing of the engine. Every other spelling of it is a second definition to be
 * migrated onto this function.
 */
constexpr uint64_t chunk_coords_to_id(int32_t cx, int32_t cy, int32_t cz) {
    uint64_t x_part = static_cast<uint64_t>(cx + CHUNK_ID_OFFSET) & CHUNK_ID_MASK;
    uint64_t y_part = static_cast<uint64_t>(cy + CHUNK_ID_OFFSET) & CHUNK_ID_MASK;
    uint64_t z_part = static_cast<uint64_t>(cz + CHUNK_ID_OFFSET) & CHUNK_ID_MASK;
    return x_part | (y_part << CHUNK_ID_BITS) | (z_part << (CHUNK_ID_BITS * 2));
}

/**
 * @brief Read a three dimensional chunk address back out of its u64 id
 * @param chunk_id A value produced by chunk_coords_to_id
 * @param[out] cx Chunk grid X
 * @param[out] cy Chunk grid Y (vertical slice)
 * @param[out] cz Chunk grid Z
 *
 * The exact inverse of chunk_coords_to_id, and it lives HERE for the same reason the forward form
 * does: a consumer that needs the vertical slice of a stored chunk id would otherwise restate the
 * bit layout in its own module. Addresses outside the +/- CHUNK_ID_OFFSET window were already
 * folded by the forward masking; this reads back what the id actually carries, never what the
 * caller originally meant.
 */
constexpr void chunk_id_to_coords(uint64_t chunk_id, int32_t& cx, int32_t& cy, int32_t& cz) {
    cx = static_cast<int32_t>(chunk_id & CHUNK_ID_MASK) - CHUNK_ID_OFFSET;
    cy = static_cast<int32_t>((chunk_id >> CHUNK_ID_BITS) & CHUNK_ID_MASK) - CHUNK_ID_OFFSET;
    cz = static_cast<int32_t>((chunk_id >> (CHUNK_ID_BITS * 2)) & CHUNK_ID_MASK) - CHUNK_ID_OFFSET;
}

/**
 * @brief Pack a lattice cell address into its u64 cell id
 * @param cx Cell chunk X, any int32
 * @param cz Cell chunk Z, any int32
 * @return Packed 64 bit cell id, injective over the whole int32 plane
 *
 * The 2D form every lattice consumer uses. Both axes travel at full int32 width, so the lattice
 * address cx = face * HEXGRID_FACE_STRIDE + i survives whole and two different cells can never
 * meet on one key.
 *
 * A cell id and the ground level chunk id of the same address are DIFFERENT numbers in different
 * key spaces. Forcing them to be equal would push the cell key through the 20-bit window of the
 * 3D form, where face and face+2 land on one key and each key carries 10 of the 20 faces. Every
 * productive key space is fed by exactly ONE of the two forms and no productive reader mixes them
 * (symbol sweep over cell_to_chunk_id and chunk_coords_to_id, the CellMap surface store_cell /
 * get_cell / remove_cell / has_cell, and a semantic sweep against taxonomy namesakes); the case in
 * modules/ase-gis/tests/test_gis_hxgn.cpp pins the two forms apart.
 */
constexpr uint64_t cell_to_chunk_id(int32_t cx, int32_t cz) {
    return (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << CELL_ID_AXIS_BITS) |
           static_cast<uint64_t>(static_cast<uint32_t>(cz));
}

/**
 * @brief Read the cell X axis back out of a packed 2D cell id
 * @param cell_id A value produced by cell_to_chunk_id
 * @return The original cx, sign included
 *
 * The round trip is exact because the packing masks nothing away. The unsigned to signed cast is
 * the two's-complement reinterpretation the language guarantees from C++20 on, which is the same
 * discipline cell_coord_to_decimal already uses for the durable form: the sign comes out of the
 * REPRESENTATION, never out of a comparison.
 */
constexpr int32_t chunk_id_to_cell_x(uint64_t cell_id) {
    return static_cast<int32_t>(
        static_cast<uint32_t>((cell_id >> CELL_ID_AXIS_BITS) & CELL_ID_AXIS_MASK));
}

/**
 * @brief Read the cell Z axis back out of a packed 2D cell id
 * @param cell_id A value produced by cell_to_chunk_id
 * @return The original cz, sign included
 */
constexpr int32_t chunk_id_to_cell_z(uint64_t cell_id) {
    return static_cast<int32_t>(static_cast<uint32_t>(cell_id & CELL_ID_AXIS_MASK));
}

// HUB BIT-PATTERN CODEC - an int32 coordinate through an f32 hub slot
//
// The hub value slot is a float. A coordinate is NOT a number here: the f32
// carries the int32 BIT PATTERN, so nothing ever rounds and the 2^24 mantissa
// cliff cannot be reached. The pattern is composed of the two 16 bit halves
// HI (upper) and LO (lower); the sign of a negative coordinate is restored from
// the halves, never from a float comparison.
//
// House convention, measured: the SPT_CHUNK_ID_HIGH / SPT_CHUNK_ID_LOW pair
// already moves u32 halves this way - grep SPT_CHUNK_ID_HIGH to reach the sites
// (ase-gis, ase-spatial and ase-capacity all carry one).

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

/**
 * @brief Write an int32 cell coordinate as a DECIMAL STRING, sign included
 * @param out Destination buffer
 * @param cap Capacity of out, including the terminator
 * @param coord Cell coordinate, any int32 including the most negative value
 * @return Written length, terminator excluded
 *
 * The durable form beside the hub-pattern form above, and it lives here for the same reason: a
 * coordinate has ONE encoding per channel, and every writer and reader of the durable form calls
 * this pair instead of encoding in its own anonymous namespace. The Mongo read path returns
 * numbers as float32, so wide and signed values travel as quoted strings - a writer that emitted
 * only digits would turn -40 into 40 and place a region on the wrong side of the origin, silently
 * and durably.
 *
 * The sign is read from the two's-complement REPRESENTATION and the magnitude accumulated in
 * unsigned space: the magnitude of the most negative int32 is not representable as a positive
 * int32, so negating in signed space would be undefined for exactly one input.
 */
inline uint32_t cell_coord_to_decimal(char* out, uint32_t cap, int32_t coord) {
    const uint32_t bits = static_cast<uint32_t>(coord);
    uint32_t magnitude = bits;
    uint32_t off = 0;
    if ((bits >> 31u) == 1u) {
        if (off + 1u < cap) {
            out[off++] = '-';
        }
        magnitude = 0u - bits;
    }
    char digits[12] = {};
    uint32_t len = 0;
    if (magnitude == 0u) {
        digits[len++] = '0';
    }
    while (magnitude > 0u && len < 11u) {
        digits[len++] = static_cast<char>('0' + (magnitude % 10u));
        magnitude /= 10u;
    }
    while (len > 0u && off + 1u < cap) {
        out[off++] = digits[--len];
    }
    if (off < cap) {
        out[off] = '\0';
    }
    return off;
}

/**
 * @brief Read an int32 cell coordinate back from its DECIMAL STRING form
 * @param text NUL-terminated decimal, optionally signed
 * @return The coordinate; 0 for an empty or non-numeric buffer
 *
 * The exact inverse of cell_coord_to_decimal - the pair is what makes the durable round trip
 * provable. The magnitude accumulates unsigned so the most negative value survives it.
 */
inline int32_t decimal_to_cell_coord(const char* text) {
    uint32_t at = 0;
    uint32_t negative = 0;
    if (text[0] == '-') {
        negative = 1u;
        at = 1u;
    }
    uint32_t magnitude = 0;
    for (uint32_t i = at; text[i] >= '0' && text[i] <= '9'; ++i) {
        magnitude = magnitude * 10u + static_cast<uint32_t>(text[i] - '0');
    }
    if (negative == 1u) {
        return static_cast<int32_t>(0u - magnitude);
    }
    return static_cast<int32_t>(magnitude);
}

}  // namespace ase::types
