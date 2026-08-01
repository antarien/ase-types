#pragma once

/**
 * ASE MODULE TYPES (SSOT)
 *
 * @file        region_wire.hpp
 * @brief       Frozen world-region wire contract - BIN_MSG ids, region rect, snapshot layouts
 * @description The ONE place every tier learns the world-region wire from. Frames 92-106 are the
 *              reserved region band (highest live id is 91 = RSN_TRIGGER_INJECT); network/types.hpp
 *              and replica_types.hpp INCLUDE this header instead of restating the values, which is
 *              what kills the 33/34/35-class drift where two modules disagreed on one id.
 *
 *              RENAMED 2026-07-28 from mesh_wire.hpp. "Mesh" reads as a polygon mesh in a 3D
 *              engine and misled a reader into thinking this file, and the module consuming it,
 *              were terrain geometry. The subject here is the REGION - a rectangle of chunks one
 *              World node owns - and the frames that move a region between nodes.
 *
 *              Layer 0 stays ECS-free: only constexpr and plain PODs live here - no registry, no
 *              EnTT include, no logic. The emplace/View usage of the intent PODs lives in L3/L4.
 *
 *              Byte layouts are stated as explicit offset/size constants, never as sizeof() of a
 *              padded struct: two independently built binaries must encode the same bytes.
 *
 *              Import via:
 *                #include <ase/types/region_wire.hpp>
 *                using ase::types::BIN_MSG_WORLD_REGION_ASSIGN;
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @created     2026-07-28
 * @modified    2026-07-28
 * @version     2.0.0
 *
 * ECS TYPES COMPLIANCE
 *
 * [ ] All constants defined (no magic numbers in code)
 * [ ] Every constant has inline comment (English, explains purpose)
 * [ ] NO enum class (only constexpr uint8_t for enumeration values)
 * [ ] Type aliases defined
 * [ ] InvalidEntityId = UINT32_MAX defined (if needed)
 * [ ] Abbreviations documented
 * [ ] Wire layouts stated as offsets/sizes, not sizeof() of padded structs
 */

#include <cstdint>

namespace ase::types {

// ---------------------------------------------------------------------------
// BIN_MSG band 92-106 - the reserved world-region frames (PROTOCOL SSOT).
// Cross-checked against the live registry: the highest live id is 91
// (RSN_TRIGGER_INJECT), 107+ is the quota/edge-workflow band already in use.
// ---------------------------------------------------------------------------
constexpr uint8_t BIN_MSG_WORLD_REGION_DECLARE    = 92u;  // World → Replica: the regions this node owns (RegionRect*N)
constexpr uint8_t BIN_MSG_WORLD_REGION_ASSIGN     = 93u;  // Replica → World: take ownership of this region
constexpr uint8_t BIN_MSG_WORLD_REGION_RELINQUISH = 94u;  // Replica → World: give up this region (teardown/merge)
constexpr uint8_t BIN_MSG_WORLD_SNAP_REQ          = 95u;  // Replica → World: serialize this region for a handoff
constexpr uint8_t BIN_MSG_WORLD_SNAP_PAGE         = 96u;  // both ways: one paged baseline slice of region state
constexpr uint8_t BIN_MSG_WORLD_HANDOFF_BEGIN     = 97u;  // Replica → World: begin taking over a region
constexpr uint8_t BIN_MSG_WORLD_HANDOFF_COMMIT    = 98u;  // Replica → World: ownership flips now
constexpr uint8_t BIN_MSG_WORLD_HANDOFF_ACK       = 99u;  // World → Replica: handoff step accepted or refused
constexpr uint8_t BIN_MSG_PLAYER_MIGRATE          = 100u; // World → Replica: move one player's compute to the neighbour region
constexpr uint8_t BIN_MSG_NODE_TOKEN_REQ          = 101u; // Engine → Replica: mint a node connection-token for a spawn
constexpr uint8_t BIN_MSG_NODE_TOKEN_RES          = 102u; // Replica → Engine: the minted token (secret never logged)
constexpr uint8_t BIN_MSG_S2S_CHALLENGE           = 103u; // server-to-server HMAC challenge before a privileged request

// Frame-103 layout. CONTRACT AMENDMENT 2026-07-28: the frozen plan fixes the shape as
// [103][nonce:32] / [103][hmac:32] (PLAN_ASE_COMPUTE_PHASE_03_WLIFE WS-D2) but the sizes were
// never written down here, so responder and verifier would each have had to restate them - the
// double-book drift this header exists to prevent. Declared once, read by both ends.
constexpr uint32_t S2S_NONCE_SZ = 32u;  // challenge nonce, from secure_random_bytes
constexpr uint32_t S2S_KEY_MAX  = 256u; // upper bound when measuring the env-provisioned key

// Frame-101/102 layout. CONTRACT AMENDMENT 2026-07-28: the frozen plan fixes the request as
// NODE_TOKEN_REQ(101){project_id:16} (PLAN_ASE_COMPUTE_PHASE_02_ORCH WS-D.3) and carries the same
// 16-byte project field on frame 93, but the width was never given a name here - so every decoder
// would have restated the literal, which is the double-book drift this header exists to prevent.
// The project id travels as the STRING because the Vault path needs it and proj_hash (FNV32)
// cannot be reversed into it.
constexpr uint32_t NODE_TOKEN_PROJ_ID_SZ = 16u;  // NUL-padded project id on frames 101/102
constexpr uint32_t NODE_TOKEN_MAX        = 191u; // "<org>.<token_id>.<secret>" upper bound on 102
constexpr uint8_t BIN_MSG_CAP_LEADER_CLAIM        = 105u; // Engine → Replica: capacity-scheduler HA lease claim (Mongo capacity_leader is authority)
constexpr uint8_t BIN_MSG_TERRAIN_DELTA           = 106u; // World → Replica: changed CELLS only - the steady-state terrain feed

// Frame-105 layout. CONTRACT AMENDMENT 2026-07-29: the frozen plan fixes the claim as
// [105][engine_id:u32][epoch_ms:u64] (PLAN_ASE_COMPUTE "Multi-Engine HA / leader election") and
// the Engine emitter already encodes exactly that (capacity_orch_ldr_clam_sys.cpp,
// ORCH_FRAME_LEAD_SZ = 13), but the offsets were never written down here - so the Replica decoder
// would have had to restate them, the double-book drift this header exists to prevent. Declared
// once, read by both ends. The claim has NO response frame: the Replica's single tick thread is
// the election mutex, the persisted capacity_leader."singleton" row is the authority.
constexpr uint32_t LEADER_CLAIM_FRAME_SZ      = 13u;  // [105](1) + engine_id:u32(4) + epoch_ms:u64(8)
constexpr uint32_t LEADER_CLAIM_OFF_ENGINE    = 1u;   // u32 offset of the claiming Engine's id
constexpr uint32_t LEADER_CLAIM_OFF_EPOCH_MS  = 5u;   // u64 offset of the claim's unix-epoch millisecond stamp

// Frame-121 layout (id lives OUTSIDE the 92-106 mesh band: the band is full and 104 stays
// deliberately unassigned; 121 is the PROTOCOL note-chain's next free id, registered 2026-07-30).
// [121][node_id:u32][port:u32][proj_hash:u32][spawn_wall_s:u32][state:u8] - the capacity
// orchestrator's per-node inventory row (CapacityOrchStaNodeComponent + state-tag ladder).
// The Engine re-sends EVERY ledger row each Maintenance pass; the Replica folds them as
// idempotent upserts, so a Replica restart re-seeds from the next pass without extra logic.
// The state byte is wire ENCODING only - both ECS ends carry the state as Tags (one tag per
// value, monotonic ladder pending → spawning → healthy → [hung] → dead), never a
// dispatched field. proj_hash is owner ATTRIBUTION (a field), never the delivery criterion.
constexpr uint8_t  BIN_MSG_CAP_NODE_STATUS    = 121u; // Engine → Replica: one capacity-node inventory row
constexpr uint32_t NODE_STATUS_FRAME_SZ       = 18u;  // [121](1) + 4x u32(16) + state:u8(1)
constexpr uint32_t NODE_STATUS_OFF_NODE       = 1u;   // u32 offset of the logical node id
constexpr uint32_t NODE_STATUS_OFF_PORT       = 5u;   // u32 offset of the listen port (= systemd instance id)
constexpr uint32_t NODE_STATUS_OFF_PROJ       = 9u;   // u32 offset of the seeding project's FNV-1a32 hash
constexpr uint32_t NODE_STATUS_OFF_SPAWN      = 13u;  // u32 offset of the unit-start wall second
constexpr uint32_t NODE_STATUS_OFF_STATE      = 17u;  // u8 offset of the CAP_NODE_STATE_* ladder code
// State ladder codes carried by the state byte. 0 is INVALID (a zeroed frame never reads as a
// legitimate state); the ladder is monotonic, which is what lets the browser store read the
// current rung by precedence even though its tag merge never removes an earlier rung's tag.
constexpr uint8_t CAP_NODE_STATE_PND  = 1u;  // pending: port reserved, unit not started
constexpr uint8_t CAP_NODE_STATE_SPWN = 2u;  // spawning: unit started, telemetry not yet seen
constexpr uint8_t CAP_NODE_STATE_HLTH = 3u;  // healthy: telemetry heartbeat armed and moving
constexpr uint8_t CAP_NODE_STATE_HANG = 4u;  // hang: silent past timeout while systemd calls the unit active (dead-by-hang)
constexpr uint8_t CAP_NODE_STATE_DEAD = 5u;  // dead: reaped or hung-restarted; the row is the audit trace

// 104 is DELIBERATELY UNASSIGNED. It was planned as a "Replica → browser terrain summary" frame,
// which contradicts the channel SSOT (WRFL_ASE_CHANNEL_ARCHITECTURE, ARCH_ASE_CODEGEN_NET): a browser
// receives exactly two things - hub_values/hub_almanach (scalars) and the codegen channels declared in
// codegen.json (component data + entity lifecycle, auto-generated DataChannels). Terrain therefore
// reaches the browser on the terrain CODEGEN CHANNEL, never on a BIN_MSG id; BIN_MSG stays the
// server-to-server binary lane. The id is left free rather than silently reused.

// Client region-subscribe is a JSON message on the browser reliable lane and deliberately has NO
// BIN_MSG id - it never crosses the server-to-server binary lane.

// Frame-122 layout (id continues the PROTOCOL note-chain after 121; the 92-106 mesh band is full
// and 104 stays deliberately unassigned). CONTRACT AMENDMENT 2026-07-31 (operator task #39,
// PLAN_ASE_LATTICE_PHASE_06_INTEG WS-I.1 link 9): the lattice cell-state seam had no wire leg -
// the only emplace<ReplicaStaCellComponent> was the Neo4j boot rehydrator reading back the very
// mirror that feeds the graph writer, so the circle had no entrance. This frame IS that entrance:
// [122][region_id:u32][cx:i32][cz:i32][state:u32][gate_mask:u32][sect_id:u32] - ONE persistent-
// zone cell row, World → Replica, staged by the gis zone egress on Dissemination and folded by
// the Replica as an idempotent upsert into the (proj_hash,cx,cz) cell mirror (proj_hash resolved
// via the region row join, behind the same conn + region + rect gates as TERRAIN_DELTA(106)).
// The state code below is wire ENCODING only: the Replica translates CELL_STATE_ZONE into its
// module-local TPLG_CELL_STATE_ZONE at the decode seam, so the two ladders cannot drift silently.
// A promotion row always carries CELL_SECT_NONE - at promotion time no sector has adopted the
// cell yet (sector aggregation happens strictly after the zone ascent, DSGN_019 Z.132).
constexpr uint8_t  BIN_MSG_GIS_CELL_ZONE = 122u; // World → Replica: one persistent-zone lattice cell row
constexpr uint32_t CELL_ZONE_FRAME_SZ    = 25u;  // [122](1) + region:u32(4) + cx:i32(4) + cz:i32(4) + state:u32(4) + gates:u32(4) + sect:u32(4)
constexpr uint32_t CELL_ZONE_OFF_REGION  = 1u;   // u32 offset of the routing region id
constexpr uint32_t CELL_ZONE_OFF_CX      = 5u;   // i32 offset of the cell chunk X
constexpr uint32_t CELL_ZONE_OFF_CZ      = 9u;   // i32 offset of the cell chunk Z
constexpr uint32_t CELL_ZONE_OFF_STATE   = 13u;  // u32 offset of the wire cell-state code
constexpr uint32_t CELL_ZONE_OFF_GATES   = 17u;  // u32 offset of the gate bitmask (bit N = edge N)
constexpr uint32_t CELL_ZONE_OFF_SECT    = 21u;  // u32 offset of the sector id (CELL_SECT_NONE = none)
constexpr uint32_t CELL_STATE_ZONE       = 3u;   // wire code: persistent zone (Replica-side TPLG_CELL_STATE_ZONE)
constexpr uint32_t CELL_SECT_NONE        = 0u;   // wire code: no sector assigned (Replica-side TPLG_SECT_NONE)

// ---------------------------------------------------------------------------
// Region identity + geometry
// ---------------------------------------------------------------------------
constexpr uint32_t REGION_ID_NONE       = 0u;    // sentinel: no region (never a valid region_id)
constexpr uint16_t WORLD_REGION_MAX     = 256u;  // max regions one World node may own at once
constexpr uint16_t REGION_RECT_WIRE_SZ  = 20u;   // bytes one RegionRect occupies on the wire (u32 + 4x i32)

// Frame-92 layout. CONTRACT AMENDMENT 2026-07-28: the frozen plan fixes the shape as
// [92][proto:u16][region_count:u16][RegionRect*N] (PLAN_ASE_COMPUTE master wire table) but the
// proto version value was never written down here, so sender and decoder would each have had to
// restate it - the double-book drift this header exists to prevent. Declared once, read by both ends.
constexpr uint16_t REGION_DECLARE_PROTO_VER = 1u;  // bumped only by a contract change, never silently
constexpr uint32_t REGION_DECLARE_HDR_SZ    = 5u;  // [92](1) + proto:u16(2) + region_count:u16(2)

/**
 * RegionRect - one region's chunk rectangle, half-open in both axes.
 *
 * Half-open (cx0 <= cx < cx1) is what makes a shared edge belong to exactly ONE region, so two
 * neighbouring Worlds can never both claim the same chunk.
 */
struct RegionRect {
    uint32_t region_id = 0;  // REGION_ID_NONE when unset
    int32_t  cx0 = 0;        // inclusive lower chunk X
    int32_t  cz0 = 0;        // inclusive lower chunk Z
    int32_t  cx1 = 0;        // exclusive upper chunk X
    int32_t  cz1 = 0;        // exclusive upper chunk Z
};

/** True when chunk (cx,cz) lies in the half-open rect - the single-sourced ownership test. */
constexpr bool chunk_in_region(int32_t cx, int32_t cz,
                               int32_t cx0, int32_t cz0, int32_t cx1, int32_t cz1) {
    return cx >= cx0 && cx < cx1 && cz >= cz0 && cz < cz1;
}

// ---------------------------------------------------------------------------
// ChunkSnapEntry - the full-fidelity per-chunk terrain slice (SNAP_PAGE payload).
// Header 18 B, then four fixed grids; 5138 B per chunk in total. The SAME schema
// serves the chunk persistence path, so there is exactly one encoder.
// ---------------------------------------------------------------------------
constexpr uint16_t SNAP_ENTRY_SCHEMA_VER      = 1u;     // bumped only by a contract change, never silently
constexpr uint32_t CHUNK_SNAP_HDR_SZ          = 18u;    // schema_ver(2) + cx(4) + cz(4) + version(8)
constexpr uint32_t CHUNK_SNAP_OFF_SCHEMA_VER  = 0u;     // u16 offset of the schema version
constexpr uint32_t CHUNK_SNAP_OFF_CX          = 2u;     // i32 offset of the chunk X
constexpr uint32_t CHUNK_SNAP_OFF_CZ          = 6u;     // i32 offset of the chunk Z
constexpr uint32_t CHUNK_SNAP_OFF_VERSION     = 10u;    // u64 offset of the chunk state version
constexpr uint32_t CHUNK_SNAP_CELLS           = 1024u;  // 32*32 cells per chunk grid
constexpr uint32_t CHUNK_SNAP_HEIGHTS_SZ      = 2048u;  // heights: 1024 cells * f16
constexpr uint32_t CHUNK_SNAP_MATERIALS_SZ    = 1024u;  // materials: 1024 cells * u8
constexpr uint32_t CHUNK_SNAP_MOISTURE_SZ     = 1024u;  // moisture: 1024 cells * u8
constexpr uint32_t CHUNK_SNAP_FERTILITY_SZ    = 1024u;  // fertility: 1024 cells * u8
constexpr uint32_t CHUNK_SNAP_OFF_HEIGHTS     = 18u;    // grid offsets follow the 18 B header in order
constexpr uint32_t CHUNK_SNAP_OFF_MATERIALS   = 2066u;  // 18 + 2048
constexpr uint32_t CHUNK_SNAP_OFF_MOISTURE    = 3090u;  // 2066 + 1024
constexpr uint32_t CHUNK_SNAP_OFF_FERTILITY   = 4114u;  // 3090 + 1024
constexpr uint32_t CHUNK_SNAP_ENTRY_SZ        = 5138u;  // 18 + 2048 + 3*1024 - one chunk on the wire

// ---------------------------------------------------------------------------
// SNAP_PAGE(96) framing - lane-fit paging of the baseline transfer.
// 11 entries * 5138 + 23 B header = 56541 B < 60000 < the 65536 lane buffer.
// ---------------------------------------------------------------------------
constexpr uint32_t SNAP_PAGE_MAX_BYTES     = 60000u;  // hard page ceiling kept below the lane buffer
constexpr uint32_t SNAP_PAGE_HDR_SZ        = 23u;     // id(1) + handoff_id(8) + region_id(4) + page_idx(4) + page_total(4) + entry_count(2)
constexpr uint32_t SNAP_PAGE_MAX_ENTRIES   = 11u;     // chunks per page at CHUNK_SNAP_ENTRY_SZ
constexpr uint32_t SNAP_PAGE_OFF_HANDOFF   = 1u;      // u64 offset of the handoff id
constexpr uint32_t SNAP_PAGE_OFF_REGION    = 9u;      // u32 offset of the region id
constexpr uint32_t SNAP_PAGE_OFF_PAGE_IDX  = 13u;     // u32 offset of this page's index
constexpr uint32_t SNAP_PAGE_OFF_PAGE_TOT  = 17u;     // u32 offset of the total page count
constexpr uint32_t SNAP_PAGE_OFF_ENTRY_CNT = 21u;     // u16 offset of the entry count in this page

// Frame-93/94/95/97/98/99 layouts. CONTRACT AMENDMENT 2026-07-29: the frozen plan fixes these
// shapes in the master wire table (PLAN_ASE_COMPUTE.md 200-216) - 93 as
// [93][region_id:u32][cx0,cz0,cx1,cz1:i32][project_id:16][epoch:u32][to_node_id:u32], 94 as
// [94][region_id:u32][epoch:u32], 95 as [95][region_id:u32][handoff_id:u64][source:u8], 97 as
// [97][handoff_id:u64][region_id:u32][from_conn:u32][to_conn:u32], 98 as
// [98][handoff_id:u64][region_id:u32][winner_conn:u32], 99 as
// [99][handoff_id:u64][region_id:u32][phase:u8] - but none of the header offsets, the SNAP_REQ
// source values or the ACK phase codes were ever written down here, so the Engine emitter, the
// Replica decoder and the World responder would each have had to restate them - the double-book
// drift this header exists to prevent. Declared once, read by every end.
constexpr uint32_t REGION_ASSIGN_FRAME_SZ  = 45u;  // [93](1) + region_id(4) + rect(16) + project_id(16) + epoch(4) + to_node_id(4)
constexpr uint32_t REGION_ASSIGN_OFF_REGION = 1u;  // u32 offset of the assigned region id
constexpr uint32_t REGION_ASSIGN_OFF_RECT   = 5u;  // 4x i32 offset of the half-open chunk rect (cx0,cz0,cx1,cz1)
constexpr uint32_t REGION_ASSIGN_OFF_PROJ   = 21u; // char[16] offset of the NUL-padded project id string
constexpr uint32_t REGION_ASSIGN_OFF_EPOCH  = 37u; // u32 offset of the assignment epoch
constexpr uint32_t REGION_ASSIGN_OFF_NODE   = 41u; // u32 offset of the target node id (the Replica relays to THAT conn only)
constexpr uint32_t REGION_RELINQ_FRAME_SZ  = 9u;   // [94](1) + region_id(4) + epoch(4)
constexpr uint32_t REGION_RELINQ_OFF_REGION = 1u;  // u32 offset of the relinquished region id
constexpr uint32_t REGION_RELINQ_OFF_EPOCH  = 5u;  // u32 offset of the relinquish epoch
constexpr uint32_t SNAP_REQ_FRAME_SZ    = 14u;  // [95](1) + region_id(4) + handoff_id(8) + source(1)
constexpr uint32_t SNAP_REQ_OFF_REGION  = 1u;   // u32 offset of the region to serialize
constexpr uint32_t SNAP_REQ_OFF_HANDOFF = 5u;   // u64 offset of the handoff id the pages will carry
constexpr uint32_t SNAP_REQ_OFF_SOURCE  = 13u;  // u8 offset of the snapshot source selector
constexpr uint8_t  SNAP_SRC_LIVE  = 0u;  // serialize from the live region state (normal handoff)
constexpr uint8_t  SNAP_SRC_MONGO = 1u;  // serialize from the Mongo cold copy (evicted region / Replica crash)
constexpr uint32_t HOFF_BEGIN_FRAME_SZ    = 21u;  // [97](1) + handoff_id(8) + region_id(4) + from_conn(4) + to_conn(4)
constexpr uint32_t HOFF_BEGIN_OFF_HANDOFF = 1u;   // u64 offset of the Engine-side handoff id (the Replica DERIVES its own: region_id<<32 | epoch)
constexpr uint32_t HOFF_BEGIN_OFF_REGION  = 9u;   // u32 offset of the region moving from A to B
constexpr uint32_t HOFF_BEGIN_OFF_FROM    = 13u;  // u32 offset of the current owner conn A (0 = dead-A, the Replica reseeds from its cache)
constexpr uint32_t HOFF_BEGIN_OFF_TO      = 17u;  // u32 offset of the incoming owner conn B
constexpr uint32_t HOFF_COMMIT_FRAME_SZ    = 17u;  // [98](1) + handoff_id(8) + region_id(4) + winner_conn(4)
constexpr uint32_t HOFF_COMMIT_OFF_HANDOFF = 1u;   // u64 offset of the handoff id
constexpr uint32_t HOFF_COMMIT_OFF_REGION  = 9u;   // u32 offset of the region that flipped
constexpr uint32_t HOFF_COMMIT_OFF_WINNER  = 13u;  // u32 offset of the conn that owns the region NOW (loser stops/discards)
constexpr uint32_t HOFF_ACK_FRAME_SZ    = 14u;  // [99](1) + handoff_id(8) + region_id(4) + phase(1)
constexpr uint32_t HOFF_ACK_OFF_HANDOFF = 1u;   // u64 offset of the handoff id
constexpr uint32_t HOFF_ACK_OFF_REGION  = 9u;   // u32 offset of the region id
constexpr uint32_t HOFF_ACK_OFF_PHASE   = 13u;  // u8 offset of the phase code below
constexpr uint8_t HOFF_ACK_SUBSCRIBED   = 1u;  // B subscribed to the region stream (phase 1)
constexpr uint8_t HOFF_ACK_CAUGHT_UP    = 2u;  // B's TERRAIN lag reached zero (phase 2 - never the commit gate)
constexpr uint8_t HOFF_ACK_ACTORS_READY = 3u;  // B instantiated EVERY resident actor (phase 3 - THE commit gate)
constexpr uint8_t HOFF_ACK_DRAINED      = 4u;  // A finished its ordered drain (actors destroyed, chunks unloaded)
constexpr uint8_t HOFF_ACK_ERROR        = 5u;  // refusal (schema mismatch, OOM) - A aborts, keeps actors, zero loss

// ---------------------------------------------------------------------------
// PlayerSnap - the fixed actor layout carried by PLAYER_MIGRATE(100) and the
// resident-actor pages of a region handoff. Sourced from the built player_st_*
// components; player_epoch makes a late or duplicate migrate idempotent.
// ---------------------------------------------------------------------------
constexpr uint32_t PLAYER_SNAP_SZ            = 42u;  // total bytes of one PlayerSnap
constexpr uint32_t PLAYER_SNAP_OFF_SCHEMA    = 0u;   // u16 schema version
constexpr uint32_t PLAYER_SNAP_OFF_ID        = 2u;   // u32 player id
constexpr uint32_t PLAYER_SNAP_OFF_EPOCH     = 6u;   // u32 per-player monotonic epoch (idempotency)
constexpr uint32_t PLAYER_SNAP_OFF_POS       = 10u;  // 4x f32: x, y, z, yaw (player_st_pos)
constexpr uint32_t PLAYER_SNAP_OFF_VEL       = 26u;  // 3x f32: vx, vy, vz (player_st_vel)
constexpr uint32_t PLAYER_SNAP_OFF_STATUS    = 38u;  // u32 status bits (player_st_sts)

// Frame-100 layout + PlayerSnap schema value. CONTRACT AMENDMENT 2026-07-28: the frozen plan fixes
// the frame as [100][player_epoch:u32][proj_hash:u32][dst_region:u32][PlayerSnap] (PLAN_ASE_COMPUTE
// master wire table) and the PlayerSnap offsets above begin with a schema field, but neither the
// frame-header offsets nor the schema VALUE were written down here - so the World encoder and the
// destination decoder would each have had to restate them, the double-book drift this header exists
// to prevent. Declared once, read by both ends.
constexpr uint16_t PLAYER_SNAP_SCHEMA_VER    = 1u;   // bumped only by a contract change, never silently
constexpr uint32_t PLAYER_MIGRATE_HDR_SZ     = 13u;  // [100](1) + player_epoch:u32(4) + proj_hash:u32(4) + dst_region:u32(4)
constexpr uint32_t PLAYER_MIGRATE_OFF_EPOCH  = 1u;   // u32 offset of the per-player monotonic epoch (idempotency)
constexpr uint32_t PLAYER_MIGRATE_OFF_PROJ   = 5u;   // u32 offset of the owning project hash (FNV-1a32 of the project id)
constexpr uint32_t PLAYER_MIGRATE_OFF_REGION = 9u;   // u32 offset of the destination region (REGION_ID_NONE = the Replica resolves it from the snap position)
constexpr uint32_t PLAYER_MIGRATE_FRAME_SZ   = PLAYER_MIGRATE_HDR_SZ + PLAYER_SNAP_SZ;  // 55 bytes: header(13) + PlayerSnap(42)

// ---------------------------------------------------------------------------
// EntitySnap - the type-generic TLV an entity's replicated components serialize
// into, so migration never needs a runtime type switch: one codegen-generated
// per-type emitter appends {type_id, len, bytes} to the entity's page.
// ---------------------------------------------------------------------------
constexpr uint32_t ENTITY_SNAP_HDR_SZ        = 12u;  // schema_ver(2) + entity_id(4) + entity_epoch(4) + component_count(2)
constexpr uint32_t ENTITY_SNAP_OFF_SCHEMA    = 0u;   // u16 schema version
constexpr uint32_t ENTITY_SNAP_OFF_ID        = 2u;   // u32 entity id
constexpr uint32_t ENTITY_SNAP_OFF_EPOCH     = 6u;   // u32 per-entity monotonic epoch
constexpr uint32_t ENTITY_SNAP_OFF_COMP_CNT  = 10u;  // u16 number of TLV component blocks that follow
constexpr uint32_t ENTITY_SNAP_TLV_HDR_SZ    = 4u;   // per block: type_id(2) + len(2), then len payload bytes

// ---------------------------------------------------------------------------
// Capacity intent PODs - the capacity scheduler's decision handed to the
// orchestration plane. Plain data at L0 so both the deciding module (ase-capacity,
// L3/Engine) and the acting plugin (ase-pl-cap-orch, L4/Engine) share ONE
// definition; the Tag structs carry no data and select the intent kind, so no
// type-discriminator field is ever needed.
// ---------------------------------------------------------------------------

/** One region-movement intent: which region moves from which node to which node, for which project. */
struct CapacityReqXmitComponent {
    uint32_t region_id = 0;  // REGION_ID_NONE until the scheduler picked the region
    uint32_t from_node = 0;  // node id giving the region up (0 = none, e.g. a fresh spawn)
    uint32_t to_node = 0;    // node id taking the region over (0 = still to be spawned)
    uint32_t proj_hash = 0;  // owning project (entt::hashed_string value of the project id)
    uint32_t epoch = 0;      // monotonic intent epoch, so a stale intent is dropped not replayed
};

struct CapacityReqSplitTag {};       // split the region into smaller ones (load above the band)
struct CapacityReqMergeTag {};       // merge neighbouring regions back together (load below the band)
struct CapacityReqRebalanceTag {};   // move the region to a less loaded node, geometry unchanged
struct CapacityReqSpawnTag {};       // a new node is needed before the region can be assigned
struct CapacityReqRelinquishTag {};  // the region is given up entirely (project teardown / merge tail)

// CONTRACT AMENDMENT 2026-07-31 (PLAN_ASE_LATTICE_PHASE_04_CAP WS-C.1). A SIXTH intent class, added
// under the same precedent as the five tags above. The Genesis rule creates the FIRST region of a
// project and hands it to a World node that is ALREADY live - no node may be spawned for it. None of
// the five classes above carries that meaning, measured at their drains: CapacityReqSplitTag turns
// into a spawn whenever to_node is 0 (capacity_orch_req_splt_sys.cpp:291-292), CapacityReqSpawnTag
// reserves a fresh port and node id unconditionally (capacity_orch_req_spwn_sys.cpp:306-336), and
// CapacityReqRebalanceTag DROPS an intent whose to_node is 0 (capacity_orch_req_blnc_sys.cpp). The
// scheduler never names a node - "to_node stays zero on purpose", capacity_rcn_blnc_sys.cpp EMIT
// PASS - so reusing any of them would spawn a second node while the node the FLOOR rule just
// provided sits idle. Frame 93 stays untouched (versionless, one-rect): this is an empty Tag, not a
// wire change; the class it selects is drained onto the EXISTING assign path.
struct CapacityReqAssignTag {};      // give an already-created region to an already-live node (no spawn)

// CONTRACT AMENDMENT 2026-07-28 (PLAN_ASE_COMPUTE_PHASE_02_ORCH WS-D.2). Two more shared PODs,
// added under the same precedent as CapacityReqXmitComponent above: ase-capacity (L3) and
// ase-pl-capacity-orch (L4) both touch them, and the ONLY legal shared home is this L0 header
// (an L4 plugin never includes an L3 module). Minimal by design - nothing else moves here.

/**
 * CapacityReqSpwnComponent - a spawn request materialized from a scheduler SPAWN intent.
 *
 * Carries the project_id STRING (not just proj_hash) - the Vault path of the node token needs it
 * and proj_hash (FNV32 via entt::hashed_string) is not reversible. The scheduler resolves it once
 * at intent creation; char[16] < 256B is allowed in a component per WRFL_ASE_STRING_HANDLING
 * Abschnitt 1 (NODE_TOKEN_PROJ_ID_SZ above fixes the same width on frames 93/101/102).
 */
struct CapacityReqSpwnComponent {
    char project_id[NODE_TOKEN_PROJ_ID_SZ] = {};  // NUL-padded project id string (Vault path + wire)
    uint32_t proj_hash = 0;                       // entt::hashed_string of project_id (FNV32)
    uint32_t want_region_id = 0;                  // region to assign once the node is live (REGION_ID_NONE = none yet)
};

/**
 * CapacityNodePendComponent - one node spawn in flight, visible to the scheduler.
 *
 * The orchestration plugin (L4) emplaces it when it reserves a node and removes it when the node
 * turns live; the scheduler (L3) counts these rows so its FLOOR/SPLIT deficit uses
 * effective_nodes = live_nodes + pending_nodes and a booting spawn is never re-spawned at the
 * next reconcile (PLAN_ASE_COMPUTE_PHASE_02_ORCH WS-D.2, fixes #6).
 */
struct CapacityNodePendComponent {
    uint32_t node_id = 0;    // Engine-side logical node handle of the in-flight spawn
    uint32_t proj_hash = 0;  // project the pending node was spawned for
};

/**
 * CapacityNodeAdopComponent - one durable node row delivered to the orchestrator for adoption.
 *
 * The third POD of the same amendment, and it closes the loop the other two opened. The
 * orchestrator's ledger is RAM-only, so every Engine restart forgets the compute nodes it started
 * - they keep running, unreachable and unaccounted (TASK_ASE_TOPOLOGY_BACKLOG #115). WS-D.2 routes
 * the durable copy back through the Replica ("the Engine holds no Mongo handle; the Replica reads
 * capacity_nodes from Mongo and re-publishes the node/port table owner-scoped over the filtered
 * bridge"), and the hub bridge lands in ase-capacity (L3) - the scheduler side, which may read the
 * hub. The plugin (L4) may not include it, so the delivered row crosses HERE, exactly like the
 * spawn request above.
 *
 * Carries the project_id STRING for the same reason CapacityReqSpwnComponent does: proj_hash is a
 * one-way FNV32, and a node reaped later re-queues its regions as spawn intents that need the
 * string. The scheduler resolves it once, at delivery, through the O(1) project-row address seam
 * (ENG_PROJ_ROW_HI/LO) - never by walking the project rows.
 *
 * The row is a REQUEST, not a claim of liveness: it states that a node with this identity was
 * durable, and the plugin's adopt pass is what confirms or drops it with systemctl is-active.
 */
struct CapacityNodeAdopComponent {
    char project_id[NODE_TOKEN_PROJ_ID_SZ] = {};  // NUL-padded project id string (empty = unresolved)
    uint32_t node_id = 0;                         // logical node handle (the durable identity)
    uint32_t port = 0;                            // listen port = systemd instance id to verify
    uint32_t proj_hash = 0;                       // project the node was spawned for
    uint32_t spawn_wall_s = 0;                    // unit-start wall second (0 = unknown)
};

}  // namespace ase::types
