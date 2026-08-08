/**
 * Hex Lattice Wire Unit Tests
 *
 * The address cases of PLAN_ASE_PRESSURE_PHASE_01_ADDR.md: the pairs that collided under the
 * 20-bit packing, the round trip of the widened 2D cell form over all twenty icosahedron faces,
 * the injectivity of that form across the declared lattice range, and the proof that the 3D form
 * is untouched and its height axis still carries.
 *
 * Every case walks the public surface of hexgrid_wire.hpp only. Layer 0 carries no dependency, so
 * the two outside facts this file needs - the lattice address stride and the old 20-bit layout -
 * are restated here as named constants with the file and line they come from. A divergence
 * between a restatement and its source is exactly the drift these cases exist to catch.
 *
 * The doctest main is switched on from tests/CMakeLists.txt rather than from a define here, so
 * this file carries no macro definition of its own.
 */

#include <doctest/doctest.h>

#include <cstdint>

#include <ase/types/hexgrid_wire.hpp>

using namespace ase::types;

namespace {

constexpr int32_t FACE_COUNT = 20;              // icosahedron faces the lattice address spans
constexpr int32_t FACE_STRIDE = 524288;         // HEXGRID_FACE_STRIDE, ase-math/hexgrid.hpp:191
constexpr int32_t ADDRESS_FREQUENCY = 262144;   // HEXGRID_ADDRESS_FREQUENCY, ase-math/hexgrid.hpp:184
constexpr int32_t MAX_CX = 10223616;            // 19 * FACE_STRIDE + ADDRESS_FREQUENCY, hexgrid.hpp:189
constexpr int32_t ALIAS_PERIOD = 1048576;       // 2 * FACE_STRIDE, the window of the old packing
constexpr int32_t ALIAS_OF_MAX_CX = 786432;     // FACE_STRIDE + ADDRESS_FREQUENCY, old twin of MAX_CX
constexpr int32_t INNER_SEAT_I = 1000;          // inner seat of a face, all three weights non-zero
constexpr int32_t INNER_SEAT_J = 1000;          // inner seat of a face, never a pentagon corner
constexpr int32_t SECOND_SEAT_I = 12345;        // second inner seat, unrelated to the first
constexpr int32_t SECOND_SEAT_J = 6789;         // second inner seat, unrelated to the first
constexpr int32_t LIVE_BIG_CX = 5332818;        // measured live, test_replication_cap_node.cpp:1051
constexpr int32_t LIVE_ALIAS_CX = 4284242;      // LIVE_BIG_CX minus ALIAS_PERIOD, its old twin
constexpr int32_t LIVE_CZ = -40;                // the negative cz that pair was measured at
constexpr int32_t OLD_LAYOUT_BITS = 20;         // bits per axis of the 3D form
constexpr int32_t HEIGHT_SLICE_LOW = -8;        // lowest vertical slice the height case walks
constexpr int32_t HEIGHT_SLICE_HIGH = 8;        // highest vertical slice the height case walks
constexpr int32_t HEIGHT_SLICE_COUNT = 17;      // HEIGHT_SLICE_HIGH minus HEIGHT_SLICE_LOW plus one
constexpr uint32_t PAIR_COUNT = 4u;             // previously colliding pairs this file pins
constexpr uint32_t SEAT_COUNT = 2u;             // inner seats the round trip walks on each face
constexpr uint32_t SAMPLE_I_COUNT = 5u;         // i samples of the range sweep, bounds included
constexpr uint32_t SAMPLE_J_COUNT = 4u;         // j samples of the range sweep, bounds included
constexpr uint32_t SWEEP_COUNT = 400u;          // FACE_COUNT times SAMPLE_I_COUNT times SAMPLE_J_COUNT

/**
 * The terrain chunk id packing, restated here as the REFERENCE of the 3D identity case and as the
 * OLD rule of the collision case. Layer 0 must not include Layer 3 and neither may this test, so
 * the reference is the frozen bit layout itself (20 bits per axis, centre offset 1 << 19,
 * x | y<<20 | z<<40) written out once - mirrored from
 * modules/ase-terrain/include/ase/terrain/types.hpp:182-184 and :655-659.
 */
uint64_t terrain_reference_pack(int32_t cx, int32_t cy, int32_t cz) {
    const uint32_t bits = static_cast<uint32_t>(OLD_LAYOUT_BITS);
    const uint32_t mask = (1u << bits) - 1u;
    const int32_t offset = 1 << (bits - 1u);
    uint64_t x_part = static_cast<uint64_t>(cx + offset) & mask;
    uint64_t y_part = static_cast<uint64_t>(cy + offset) & mask;
    uint64_t z_part = static_cast<uint64_t>(cz + offset) & mask;
    return x_part | (y_part << bits) | (z_part << (bits * 2));
}

}  // namespace

// =============================================================================
// CASE 1 - the pairs that shared one key under the 20-bit packing are distinct
// =============================================================================

TEST_CASE("Lattice address: cells that aliased under the 20-bit packing carry distinct ids") {
    /**
     * Each pair is stated with its OLD key as well as its new one. Asserting only the new
     * difference would prove nothing about the repair - a pair that never collided would pass the
     * same check. The old rule is therefore recomputed here and its collision pinned first.
     */
    const int32_t pair_low[PAIR_COUNT] = {
        LIVE_ALIAS_CX,
        INNER_SEAT_I,
        INNER_SEAT_I + 2 * FACE_STRIDE,
        ALIAS_OF_MAX_CX,
    };
    const int32_t pair_high[PAIR_COUNT] = {
        LIVE_BIG_CX,
        INNER_SEAT_I + ALIAS_PERIOD,
        INNER_SEAT_I + 4 * FACE_STRIDE,
        MAX_CX,
    };
    const int32_t pair_cz[PAIR_COUNT] = {
        LIVE_CZ,
        INNER_SEAT_J,
        INNER_SEAT_J,
        ADDRESS_FREQUENCY,
    };

    for (uint32_t i = 0; i < PAIR_COUNT; ++i) {
        // The old rule put both members of the pair on one key - that WAS the defect.
        CHECK(terrain_reference_pack(pair_low[i], CELL_CHUNK_Y, pair_cz[i]) ==
              terrain_reference_pack(pair_high[i], CELL_CHUNK_Y, pair_cz[i]));

        // The widened 2D form keeps them apart.
        CHECK(cell_to_chunk_id(pair_low[i], pair_cz[i]) !=
              cell_to_chunk_id(pair_high[i], pair_cz[i]));
    }

    // The live-measured pair, spelled out rather than derived, so the value stays greppable.
    CHECK(LIVE_BIG_CX - LIVE_ALIAS_CX == ALIAS_PERIOD);
    CHECK(cell_to_chunk_id(LIVE_BIG_CX, LIVE_CZ) != cell_to_chunk_id(LIVE_ALIAS_CX, LIVE_CZ));
}

// =============================================================================
// CASE 2 - the round trip is the identity over all twenty faces
// =============================================================================

TEST_CASE("Lattice address: the cell id round trip is the identity on every face") {
    /**
     * The seats are INNER seats. (i=0, j=0) is an icosahedron corner, a pentagon that the marker
     * path rejects productively (gis_hxgn_mrkr_sys.cpp:274), so a proof carried there would be a
     * proof about a cell that never exists.
     */
    const int32_t seat_i[SEAT_COUNT] = {INNER_SEAT_I, SECOND_SEAT_I};
    const int32_t seat_j[SEAT_COUNT] = {INNER_SEAT_J, SECOND_SEAT_J};

    for (int32_t face = 0; face < FACE_COUNT; ++face) {
        for (uint32_t seat = 0; seat < SEAT_COUNT; ++seat) {
            const int32_t cx = face * FACE_STRIDE + seat_i[seat];
            const int32_t cz = seat_j[seat];
            const uint64_t id = cell_to_chunk_id(cx, cz);
            CHECK(chunk_id_to_cell_x(id) == cx);
            CHECK(chunk_id_to_cell_z(id) == cz);
        }

        // The negative half of the Z axis travels too - the region rect reaches below the origin.
        const int32_t cx = face * FACE_STRIDE + INNER_SEAT_I;
        const uint64_t below = cell_to_chunk_id(cx, LIVE_CZ);
        CHECK(chunk_id_to_cell_x(below) == cx);
        CHECK(chunk_id_to_cell_z(below) == LIVE_CZ);
    }

    // The extremes of the int32 plane survive, including the value whose magnitude has no positive
    // counterpart.
    const uint64_t extreme = cell_to_chunk_id(INT32_MIN, INT32_MAX);
    CHECK(chunk_id_to_cell_x(extreme) == INT32_MIN);
    CHECK(chunk_id_to_cell_z(extreme) == INT32_MAX);
}

// =============================================================================
// CASE 3 - the declared lattice range maps injectively
// =============================================================================

TEST_CASE("Lattice address: the declared cx and cz range produces no two equal cell ids") {
    /**
     * The sweep spans the range the DoD names: cx from 0 to MAX_CX across all twenty faces, cz
     * from 0 to ADDRESS_FREQUENCY. The face corners appear HERE, unlike in the round-trip case,
     * because this claim is about the address RANGE and its bounds, not about a cell that gets
     * marked.
     */
    const int32_t sample_i[SAMPLE_I_COUNT] = {0, 1, INNER_SEAT_I, SECOND_SEAT_I, ADDRESS_FREQUENCY};
    const int32_t sample_j[SAMPLE_J_COUNT] = {0, 1, INNER_SEAT_J, ADDRESS_FREQUENCY};

    uint64_t keys[SWEEP_COUNT] = {};
    uint32_t count = 0;

    for (int32_t face = 0; face < FACE_COUNT; ++face) {
        for (uint32_t i = 0; i < SAMPLE_I_COUNT; ++i) {
            for (uint32_t j = 0; j < SAMPLE_J_COUNT; ++j) {
                const int32_t cx = face * FACE_STRIDE + sample_i[i];
                CHECK(cx >= 0);
                CHECK(cx <= MAX_CX);
                keys[count] = cell_to_chunk_id(cx, sample_j[j]);
                ++count;
            }
        }
    }

    CHECK(count == SWEEP_COUNT);

    uint32_t duplicates = 0;
    for (uint32_t a = 0; a < count; ++a) {
        for (uint32_t b = a + 1u; b < count; ++b) {
            if (keys[a] == keys[b]) {
                ++duplicates;
            }
        }
    }
    CHECK(duplicates == 0u);
}

// =============================================================================
// CASE 4 - the 3D form is untouched and its height axis still carries
// =============================================================================

TEST_CASE("Lattice address: the 3D packing is unchanged and the vertical axis still separates") {
    // Identity with the terrain reference, the case the contract froze - unchanged for the 3D form.
    CHECK(chunk_coords_to_id(0, 0, 0) == terrain_reference_pack(0, 0, 0));
    CHECK(chunk_coords_to_id(7, 3, -9) == terrain_reference_pack(7, 3, -9));
    CHECK(chunk_coords_to_id(-9, -3, 7) == terrain_reference_pack(-9, -3, 7));
    CHECK(chunk_coords_to_id(524287, 524287, -524288) ==
          terrain_reference_pack(524287, 524287, -524288));

    // The height axis is what the 3D form exists for: every slice is its own key.
    uint64_t stack[HEIGHT_SLICE_COUNT] = {};
    uint32_t depth = 0;
    for (int32_t cy = HEIGHT_SLICE_LOW; cy <= HEIGHT_SLICE_HIGH; ++cy) {
        stack[depth] = chunk_coords_to_id(INNER_SEAT_I, cy, INNER_SEAT_J);
        ++depth;
    }
    CHECK(depth == static_cast<uint32_t>(HEIGHT_SLICE_COUNT));

    uint32_t collisions = 0;
    for (uint32_t a = 0; a < depth; ++a) {
        for (uint32_t b = a + 1u; b < depth; ++b) {
            if (stack[a] == stack[b]) {
                ++collisions;
            }
        }
    }
    CHECK(collisions == 0u);

    // The two forms are now deliberately different numbers for the same place. This is the claim
    // the header used to make in the opposite direction, and it is severed on purpose.
    CHECK(cell_to_chunk_id(INNER_SEAT_I, INNER_SEAT_J) !=
          chunk_coords_to_id(INNER_SEAT_I, CELL_CHUNK_Y, INNER_SEAT_J));
    CHECK(CELL_CHUNK_Y == 0);
    CHECK(CELL_ID_AXIS_BITS == 32u);
    CHECK(CHUNK_ID_BITS == 20u);
}
