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
constexpr uint8_t BIN_MSG_TERRAIN_PAGE_NACK       = 131u; // Replica → World: ONE chunk the receiver could not park - the back channel of the baseline lane, layout TRN_NACK_* below. Mirrored in ase-network types.hpp (value SSOT) and replica_types.hpp; registered in the PROTOCOL allocation table the same day (next free was 131)

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

// CONTRACT AMENDMENT 2026-08-10 - DIE BEGANGENE WABE BEKOMMT IHRE SPROSSE.
//
// Die Leiter oben nennt bisher nur ihre OBERSTE Stufe, und ihr eigener Kommentar sagt, dass es
// eine Leiter ist: Sprosse 3 ist die, "at which a cell IS a persistent zone". Was darunter liegt,
// war unbenannt - und genau dort steht der Zustand, den der Reiter zeigen soll.
//
// GEMESSEN 2026-08-10 an der laufenden Flotte: ein echter Spieler wandert seit 33 Minuten ueber
// die Kugel, der World fuehrt 21 lebende Zellen (GeoidMrkrSystem "21 active"), und im Browser
// stehen ZELLZEILEN 1. Die Ursache ist kein Fehler in einem der Glieder, sondern ein fehlendes
// Glied: `GeoidZonePubSystem` sieht ausschliesslich Zellen mit `GeoidZonePndTag`
// (geoid_zone_pub_sys.cpp:224) und schreibt fest CELL_STATE_ZONE (:262), die Gegenseite weist
// jeden anderen Code ausdruecklich ab (replica_rcv_sys.cpp:3133). Eine Wabe, die begangen aber
// noch keine Zone ist, hat damit UEBERHAUPT KEINEN Traeger zum Replica - und sie ist das, was die
// Spur ausmacht: die Zone waechst erst aus ihr.
//
// WARUM KEIN ZWEITER RAHMEN. Frame 122 traegt `state` als FELD. Ein Feld mit genau einem
// zulaessigen Wert haette niemand vier Byte gekostet, und die Ablehnung anderer Codes ist als
// eigener Pfad ausgeschrieben - beides sagt, dass die Erweiterung ueber den Code laeuft und nicht
// ueber eine neue Kennung. Ein eigener Rahmen waere hier die teurere und die unehrlichere Antwort:
// er wuerde dieselbe Zeile (Region, cx, cz) ein zweites Mal auf die Leitung legen.
//
// Die Sprosse ist damit die ERSTE, weil sie das Erste ist, was einer Wabe zustoesst: jemand war
// da. Alles Weitere - Sektor, Zone - kommt darueber. Der Name sagt die Tatsache, nicht ihre
// Wirkung: MARK, nicht "besucht" und nicht "Druck".
constexpr uint32_t CELL_STATE_MARK       = 1u;   // wire code: cell walked and standing, no zone yet (ladder step below CELL_STATE_ZONE)

// Frames 123/124 - RESERVED for the WRLD_LIFE operative lane (audit G1 fix, registered 2026-08-03).
// ase-pl-wrld-lifecycle had allocated 121/122 against a stale registry note ("highest live id is
// 120") while 121 (CAP_NODE_STATUS) and 122 (GIS_CELL_ZONE) were already LIVE above - the same
// collision took inbound lane 24, which is LANE_RGN carrying REGION_ASSIGN(93)/RELINQUISH(94), so
// the plugin drain could destructively eat region assignments. The plugin's operative pair now
// rides 123 (WRLD_LIFE_WIRE_MSG) / 124 (WRLD_LIFE_STATUS_RES_MSG) on lane 27; the value SSOT stays
// plugin-local (ase-pl-wrld-lifecycle types.hpp) until the Replica per-node forward is built, per
// the migration note there - THIS anchor only pins the ids so no later band reuses them.
// The module axis (MODSET/HOFF_MODSET) therefore starts at 125 below.

// ---------------------------------------------------------------------------
// Module-group registry - the module axis of the dynamic server meshing.
// CONTRACT AMENDMENT 2026-08-03 (PLAN_ASE_COMPUTE_MOD_AXIS.md T4, decision E4
// option I-B): the ASSIGNMENT UNIT of the module axis is the causality GROUP,
// never the single module - a per-module `if (module == ...)` dispatch is the
// type-discriminator the ECS rules forbid, and the dense CAUSA clique
// (INST_ASE_MOD_CAUSA.md Section 9: MTB→LFC→HRT→ENT chain, CMB↔SKL cycle, the
// AI plane reads ALL) makes single-module separation exceed the sync budget
// anyway. Groups are a FROZEN dense registry 0..63 so a set of groups rides a
// single u64 `grp_mask` (wire-mask precedent: gate_mask u32 on frame 122
// above, cred_avail_mask u32 on frame 78, collection_read_mask u16 on 85).
// The stable member key is the module NAME string - T2 measured NO numeric
// module identity anywhere (serial type_ids collide, codegen ids are strings,
// no name hashes, load order unstable); the name is fourfold consistent
// (module.toml, VERSION, AseModuleInfo.name, codegen.json). Membership changes
// are contract amendments to THIS block, exactly like BIN_MSG id allocations.
// Plan→real deviations, measured against modules/ (84 entries, 2026-08-03):
// "ase-ai" is a CAUSA-doc role, not a module - the real AI plane is ase-bdi
// plus the perception/signature/recognition clique; "ase-sky" is the L4
// plugin ase-pl-sky, not an L3 module, so the ENVR members are the five real
// environment modules. Unlisted modules resolve to MOD_GRP_ID_NONE and are
// NOT maskable: they ride every node like the KERN plane until an amendment
// assigns them - a silent default group would mis-attribute their load.
// ---------------------------------------------------------------------------
constexpr uint32_t MOD_GRP_KERN = 0u;  // infrastructure plane: loaded in EVERY tier build (T2: all four CMakes) - never assignable, registry slot 0 is the invariant
constexpr uint32_t MOD_GRP_TERR = 1u;  // terrain plane: space-bound, region-local (the "terrain = own instance" cut, ARCH_ASE_REP_SRV.md:591)
constexpr uint32_t MOD_GRP_ENTV = 2u;  // entity plane: the dense CAUSA clique - per-entity keys at tick rate, UNSEPARABLE inside, moves as a whole
constexpr uint32_t MOD_GRP_ENVR = 3u;  // environment plane: low-frequency GLOBAL/zonal producers (TIM_*/CAL_*, 1 Hz class) - separable
constexpr uint32_t MOD_GRP_COUNT = 4u;      // registered groups (dense, next amendment appends 4)
constexpr uint32_t MOD_GRP_ID_NONE = 0xFFFFFFFFu;  // resolver sentinel: module not in the registry (not maskable, rides every node)

constexpr uint64_t MOD_GRP_MASK_NONE = 0ull;   // empty group set
constexpr uint64_t MOD_GRP_MASK_ALL  = ~0ull;  // full group set - the pre-module-axis default (a node without a MODSET row simulates everything)

/** Bit of one group id inside a u64 grp_mask (dense registry 0..63). */
constexpr uint64_t mod_grp_bit(uint32_t grp_id) {
    return 1ull << grp_id;
}

// Member NAMES per group - the stable key (see block comment). KERN members are
// listed for completeness of the registry even though the group is never
// assignable; their presence on every node is what slot 0 encodes.
constexpr const char* MOD_GRP_KERN_MEMBERS[] = {"ase-hub", "ase-sdk", "ase-monitoring"};
constexpr const char* MOD_GRP_TERR_MEMBERS[] = {"ase-terrain", "ase-gis"};
constexpr const char* MOD_GRP_ENTV_MEMBERS[] = {"ase-player", "ase-combat", "ase-metabolism",
                                                "ase-lifecycle", "ase-heritage", "ase-entropy",
                                                "ase-skill", "ase-perception", "ase-signature",
                                                "ase-recognition", "ase-genetics", "ase-bdi"};
constexpr const char* MOD_GRP_ENVR_MEMBERS[] = {"ase-time", "ase-calendar", "ase-celestial",
                                                "ase-ephemeris", "ase-atmosphere"};
constexpr uint32_t MOD_GRP_KERN_MEMBER_CNT = sizeof(MOD_GRP_KERN_MEMBERS) / sizeof(MOD_GRP_KERN_MEMBERS[0]);  // 3
constexpr uint32_t MOD_GRP_TERR_MEMBER_CNT = sizeof(MOD_GRP_TERR_MEMBERS) / sizeof(MOD_GRP_TERR_MEMBERS[0]);  // 2
constexpr uint32_t MOD_GRP_ENTV_MEMBER_CNT = sizeof(MOD_GRP_ENTV_MEMBERS) / sizeof(MOD_GRP_ENTV_MEMBERS[0]);  // 12
constexpr uint32_t MOD_GRP_ENVR_MEMBER_CNT = sizeof(MOD_GRP_ENVR_MEMBERS) / sizeof(MOD_GRP_ENVR_MEMBERS[0]);  // 5

// Per-group load key NAMES (transport X-A, PLAN_ASE_COMPUTE_MOD_AXIS.md T3): the load VECTOR of a
// region is the set of its CAP_RGN_LOAD_<GRP> values under the UNCHANGED region_id owner - the
// group rides in the value_id NAMESPACE, never in owner bits (PLAN_ASE_COMPUTE.md:246), and their
// sum equals the aggregate CAP_RGN_LOAD, which stays live unchanged (no breaking change). Indexed
// by group id; L0 stays EnTT-free, so the names live here as strings and every end hashes them
// with entt::hashed_string at its own call site. Each key is registered in hub_metrics.json
// BEFORE any hub::set AND allowlisted in bridge_value_allowed - registration alone does not cross
// the bridge (measured trap, replica_hub_snd_sys.cpp).
constexpr const char* MOD_GRP_LOAD_KEYS[MOD_GRP_COUNT] = {
    "CAP_RGN_LOAD_KERN",  // MOD_GRP_KERN - measured for sum-consistency; never a split input (slot 0 invariant)
    "CAP_RGN_LOAD_TERR",  // MOD_GRP_TERR
    "CAP_RGN_LOAD_ENTV",  // MOD_GRP_ENTV
    "CAP_RGN_LOAD_ENVR",  // MOD_GRP_ENVR
};

/** Exact string equality - L0 stays std::-free, so the two-pointer walk lives here. */
constexpr bool mod_grp_name_eq(const char* a, const char* b) {
    uint32_t i = 0u;
    while (a[i] != '\0' && b[i] != '\0' && a[i] == b[i]) {
        ++i;
    }
    return a[i] == b[i];
}

/**
 * Resolve a module NAME to its group id - the registration-time System→Module
 * →Group attribution seam (M-B rollup uses the SystemInfo source string).
 * Returns MOD_GRP_ID_NONE for unregistered modules (see block comment).
 */
constexpr uint32_t mod_grp_of(const char* module_name) {
    for (uint32_t i = 0u; i < MOD_GRP_KERN_MEMBER_CNT; ++i) {
        if (mod_grp_name_eq(MOD_GRP_KERN_MEMBERS[i], module_name)) {
            return MOD_GRP_KERN;
        }
    }
    for (uint32_t i = 0u; i < MOD_GRP_TERR_MEMBER_CNT; ++i) {
        if (mod_grp_name_eq(MOD_GRP_TERR_MEMBERS[i], module_name)) {
            return MOD_GRP_TERR;
        }
    }
    for (uint32_t i = 0u; i < MOD_GRP_ENTV_MEMBER_CNT; ++i) {
        if (mod_grp_name_eq(MOD_GRP_ENTV_MEMBERS[i], module_name)) {
            return MOD_GRP_ENTV;
        }
    }
    for (uint32_t i = 0u; i < MOD_GRP_ENVR_MEMBER_CNT; ++i) {
        if (mod_grp_name_eq(MOD_GRP_ENVR_MEMBERS[i], module_name)) {
            return MOD_GRP_ENVR;
        }
    }
    return MOD_GRP_ID_NONE;
}

// ---------------------------------------------------------------------------
// Frames 125-128 - the module-axis MODSET band. CONTRACT AMENDMENT 2026-08-03
// (PLAN_ASE_COMPUTE_MOD_AXIS.md T4; ids registered in the PROTOCOL note chain,
// 123/124 = WRLD_LIFE reservation above). The frozen region band 92-106 stays
// byte-identical: the module axis extends ONLY over new ids with default
// semantics for existing nodes - a node without a MODSET row simulates ALL
// groups (MOD_GRP_MASK_ALL), a handoff without a 128 row moves the full set.
// No version handshake, no coexistence window (the NMQ/RMQ lesson, T5).
// All four ride the existing relay: ORCHESTRATOR → Replica park → targeted
// relay to ONE conn (frame-93 pattern), far under the 65536 lane fit.
// ---------------------------------------------------------------------------
constexpr uint8_t BIN_MSG_WORLD_MODSET_ASSIGN  = 125u; // Engine → Replica → World: WHICH groups of the region this node simulates
constexpr uint8_t BIN_MSG_WORLD_MODSET_DECLARE = 126u; // World → Replica: the complete (region, grp_mask) set this node serves
constexpr uint8_t BIN_MSG_WORLD_MODSET_RELINQ  = 127u; // Replica → World: give up this group subset of the region
constexpr uint8_t BIN_MSG_WORLD_HOFF_MODSET    = 128u; // Engine → Replica: the group subset of a handoff, declared BEFORE BEGIN(97)

// Frame-125 layout: [125][region_id:u32][epoch:u32][to_node_id:u32][grp_mask:u64] = 21 B.
// Rides BESIDE frame 93 (which stays byte-identical); without a 125 row the assignment
// covers ALL groups - today's behaviour is the default, never a break.
constexpr uint32_t MODSET_ASSIGN_FRAME_SZ   = 21u;  // [125](1) + region_id(4) + epoch(4) + to_node_id(4) + grp_mask(8)
constexpr uint32_t MODSET_ASSIGN_OFF_REGION = 1u;   // u32 offset of the assigned region id
constexpr uint32_t MODSET_ASSIGN_OFF_EPOCH  = 5u;   // u32 offset of the assignment epoch (stale drops, frame-93 discipline)
constexpr uint32_t MODSET_ASSIGN_OFF_NODE   = 9u;   // u32 offset of the target node id (the Replica relays to THAT conn only)
constexpr uint32_t MODSET_ASSIGN_OFF_MASK   = 13u;  // u64 offset of the group bitmask (mod_grp_bit over the dense registry)

// Frame-126 layout: [126][proto:u16][count:u16][{region_id:u32, grp_mask:u64}*N] - the
// REGION_DECLARE(92) pattern: the COMPLETE set each time, so a re-send is an idempotent
// upsert; the Replica reconciler checks coverage per (region, group).
constexpr uint16_t MODSET_DECLARE_PROTO_VER = 1u;   // bumped only by a contract change, never silently
constexpr uint32_t MODSET_DECLARE_HDR_SZ    = 5u;   // [126](1) + proto:u16(2) + count:u16(2)
constexpr uint32_t MODSET_DECLARE_ROW_SZ    = 12u;  // region_id(4) + grp_mask(8) per declared region
// MODSET_DECLARE_FRAME_MAX (the lane-fit ceiling) is declared below the region
// identity block - it derives from WORLD_REGION_MAX, which is declared there.

// Frame-127 layout: [127][region_id:u32][epoch:u32][grp_mask:u64] = 17 B - the subset
// relinquish, REGION_RELINQUISH(94) pattern (94 itself stays the full-region teardown).
constexpr uint32_t MODSET_RELINQ_FRAME_SZ   = 17u;  // [127](1) + region_id(4) + epoch(4) + grp_mask(8)
constexpr uint32_t MODSET_RELINQ_OFF_REGION = 1u;   // u32 offset of the region id
constexpr uint32_t MODSET_RELINQ_OFF_EPOCH  = 5u;   // u32 offset of the relinquish epoch
constexpr uint32_t MODSET_RELINQ_OFF_MASK   = 9u;   // u64 offset of the group bitmask being given up

// Frame-128 layout: [128][handoff_id:u64][grp_mask:u64] = 17 B - declares the group
// subset of a handoff BEFORE its BEGIN(97); without a 128 row the handoff moves ALL
// groups (today's behaviour). handoff_id = (region_id<<32)|epoch stays collision-free:
// every attempt bumps the epoch, so two partial handoffs are two epochs.
constexpr uint32_t HOFF_MODSET_FRAME_SZ    = 17u;   // [128](1) + handoff_id(8) + grp_mask(8)
constexpr uint32_t HOFF_MODSET_OFF_HANDOFF = 1u;    // u64 offset of the handoff id the mask scopes
constexpr uint32_t HOFF_MODSET_OFF_MASK    = 9u;    // u64 offset of the group bitmask of the partial handoff

// ---------------------------------------------------------------------------
// The lattice pressure lane - frames 129/130 (CONTRACT AMENDMENT 2026-08-04,
// PLAN_ASE_PRESSURE_PHASE_03_SEAM WS-C.1/WS-C.3).
//
// Frame 122 above opened the cell seam for the ZONE ASCENT - an event row that
// fires once per promotion and carries state, gate_mask and sect_id. It carries
// NEITHER the measured column load NOR the lattice contract the address is
// expressed in (that gap is stated in the phase plan, WS-C.4). Phase 02 built
// the measurement (GisColLodComponent: occupancy, dlt_rate and the folded 0..1
// pressure) but it never left the World: measured 2026-08-04, GisColLodComponent
// had zero readers outside ase-gis and its own tests, and the Replica had no
// cell-load mirror at all. These two frames are that missing leg.
//
// WHY TWO FRAMES AND NOT AN EXTENSION OF 122: the ascent is an EVENT, the load
// is a periodic SAMPLE of a cell that may never ascend (a marker column is
// measured too, gis_col_lod_sys.cpp pass 3 walks GisHxgnMrkrComponent). Riding
// the sample on the promotion frame would deliver each cell's load exactly once,
// at promotion time, and freeze it there - the "frozen boot values" failure the
// phase plan warns about, one layer up. 122 stays byte-identical.
//
// WHY THE HEADER TRAVELS AT ALL: the display level and the ellipsoid are World
// truth (ase-gis GIS_HXGN_ELSD_SEMI_MAJOR_M / _FLATTENING, math::HexgridEllipsoid).
// The Replica is L3 and may not include ase-gis, so without this frame it would
// have to restate those constants - a second truth, and an ellipsoid change in
// the backend would tear the picture silently (phase plan gap L-6). The header
// is planet-wide, not per project: the Replica joins it to each project it
// already owns, so no project identity rides the wire that the Replica cannot
// already resolve.
//
// Both are World → Replica on the existing binary WS lane, behind the same conn
// and region gates as TERRAIN_DELTA(106) and GIS_CELL_ZONE(122). Sizes are 13 B
// and 25 B - far under the 65536 lane fit, and a re-send is an idempotent upsert
// keyed on (proj_hash,cx,cz), so an at-least-once wire needs no extra logic.
// ---------------------------------------------------------------------------
constexpr uint8_t BIN_MSG_GIS_LATTICE_HEAD = 129u; // World → Replica: the lattice contract (level + ellipsoid) of the simulated planet
constexpr uint8_t BIN_MSG_GIS_CELL_LOAD    = 130u; // World → Replica: one measured cell column (occupancy, delta rate, folded load)

// Frame-129 layout: [129][level:u32][semi_major_axis_m:f32][flattening:f32] = 13 B.
// level is the address level the cx/cz of frame 130 (and of frame 122) are expressed in -
// math::HEXGRID_ADDRESS_LEVEL_MAX unless a coarser epoch is in use; the client derives its
// draw resolution from it (hexgrid_epoch_step). The two ellipsoid fields are the field names
// and units of math::HexgridEllipsoid: equatorial radius in metres and the dimensionless
// (a-b)/a. Floats ride bit-exact via memcpy, like every other float on this lane.
constexpr uint32_t LATTICE_HEAD_FRAME_SZ  = 13u;  // [129](1) + level:u32(4) + semi_major:f32(4) + flattening:f32(4)
constexpr uint32_t LATTICE_HEAD_OFF_LEVEL = 1u;   // u32 offset of the address level of the live lattice
constexpr uint32_t LATTICE_HEAD_OFF_SEMI  = 5u;   // f32 offset of the equatorial radius a, metres
constexpr uint32_t LATTICE_HEAD_OFF_FLAT  = 9u;   // f32 offset of the flattening (a-b)/a, 0 = sphere

// Frame-130 layout:
// [130][region_id:u32][cx:i32][cz:i32][occupancy:u32][dlt_rate:f32][load:f32] = 25 B.
// region_id routes the row through the SAME ownership gate as 122 (the sending conn must own
// the region and the cell must sit in its declared half-open rect), so proj_hash is resolved
// on the Replica side by the region join and never asserted by the sender.
// occupancy and dlt_rate are the two MEASURED terms; load is the fold of exactly those two
// (gis_col_lod_sys.cpp fold_column_pressure). All three travel, so the client can show why a
// tile is hot without recomputing a weight it does not own.
// ABSENCE IS NOT ZERO: a cell whose last occupancy row retired LOSES GisColLodComponent
// outright (gis_col_lod_sys.cpp pass 1). It then stops being published and despawns on the
// client - it never ships a fabricated 0.0.
constexpr uint32_t CELL_LOAD_FRAME_SZ   = 25u;  // [130](1) + region(4) + cx(4) + cz(4) + occupancy(4) + dlt_rate(4) + load(4)
constexpr uint32_t CELL_LOAD_OFF_REGION = 1u;   // u32 offset of the routing region id
constexpr uint32_t CELL_LOAD_OFF_CX     = 5u;   // i32 offset of the cell chunk X
constexpr uint32_t CELL_LOAD_OFF_CZ     = 9u;   // i32 offset of the cell chunk Z
constexpr uint32_t CELL_LOAD_OFF_OCC    = 13u;  // u32 offset of the column occupancy of this sample
constexpr uint32_t CELL_LOAD_OFF_DLT    = 17u;  // f32 offset of the cell delta rate, per second
constexpr uint32_t CELL_LOAD_OFF_LOAD   = 21u;  // f32 offset of the folded 0..1 column pressure

// RETIRE ROW - the honest opposite of a fabricated zero. A measured column that loses its last
// occupancy row is not "a cell with load 0", it is a cell that is no longer measured, and the
// tile must fall back to ABSENT rather than to a cold colour. The folded load is contractually
// clamped into 0..1 (ase-gis GIS_COL_LOD_MIN/_MAX), so a negative value can never collide with a
// measurement; this is the same -1 ABSENT sentinel the tier vitals already carry on this codebase
// (PLAN_ASE_HUB_OWNR_SCOPE, null-tile fix 2026-07-30). On receipt the Replica REMOVES the load
// mirror of that cell; the push row then carries the address without a load field, and the client
// draws ABSENT.
constexpr float CELL_LOAD_ABSENT = -1.0f;  // wire sentinel: this column is no longer measured

// THE RANGE THE FRAME MAY CARRY - declared HERE because both ends read it. The folded pressure is
// normalised, so a value outside this band is a producer defect and the decoder drops it rather
// than painting a tile that earned no colour. This is the WIRE contract; the fold clamp inside
// ase-gis is that module's own business, and the receiver may not include ase-gis to ask.
constexpr float CELL_LOAD_MIN = 0.0f;  // lowest folded pressure a measurement row may carry
constexpr float CELL_LOAD_MAX = 1.0f;  // highest folded pressure a measurement row may carry

// An equatorial radius at or below this is a zeroed frame, not a planet. Same reasoning as the
// load band: the decoder needs a validity bound it can read without asking the producing module.
constexpr float LATTICE_HEAD_SEMI_MIN_M = 1.0f;  // smallest radius that still describes a body

// Flattening is (a-b)/a and therefore lives in [0, 1): 0 is a sphere and 1 would be a disc with no
// polar axis at all. The projection clamps at 0.999 for exactly that reason, so a value at or above
// this bound is a corrupt frame, not an exotic planet - the decoder drops it instead of mirroring a
// contract that cannot be drawn.
constexpr float LATTICE_HEAD_FLAT_MAX = 0.999f;  // highest flattening a contract row may carry

// ---------------------------------------------------------------------------
// Region identity + geometry
// ---------------------------------------------------------------------------
constexpr uint32_t REGION_ID_NONE       = 0u;    // sentinel: no region (never a valid region_id)
constexpr uint16_t WORLD_REGION_MAX     = 256u;  // max regions one World node may own at once
constexpr uint16_t REGION_RECT_WIRE_SZ  = 20u;   // bytes one RegionRect occupies on the wire (u32 + 4x i32)
constexpr uint32_t MODSET_DECLARE_FRAME_MAX = MODSET_DECLARE_HDR_SZ
                                            + static_cast<uint32_t>(WORLD_REGION_MAX)
                                            * MODSET_DECLARE_ROW_SZ;  // frame-126 ceiling: 5 + 256*12 = 3077 B << 65536 lane fit

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

/**
 * RegionGrpDltComponent - per-(region, module-group) outbound-delta counter (World-local).
 *
 * CONTRACT AMENDMENT 2026-08-03 (PLAN_ASE_COMPUTE_MOD_AXIS.md T3): the group axis needs the
 * outbound-delta rate PER GROUP, and the producer of a delta is not the module that owns the
 * region rows - the terrain egress (ase-terrain, frame 106/SNAP staging) attributes its staged
 * deltas to MOD_GRP_TERR at its region gate, while the consumer (the World group-load system,
 * ase-world) differences the counter. Two L3 modules never include each other, so the row is an
 * L0 POD - the exact region_wire seam RegionRect already rides in the same egress loop.
 *
 * Counter discipline mirrors WorldCchRgnDltComponent: dlt_count only ever grows (producer-owned),
 * dlt_seen is the consumer's sample cursor. u32 wrap is harmless - the consumer differences.
 * Producers of the other groups join at their own egress attribution seams by amendment; a group
 * without a producer keeps count 0 and its rate honestly reads 0.
 */
struct RegionGrpDltComponent {
    uint32_t region_id = 0;  // join axis 1 (REGION_ID_NONE when unset)
    uint32_t grp_id = 0;     // join axis 2 (dense module-group id, registry above)
    uint32_t dlt_count = 0;  // running count of outbound deltas this group staged for this region (grows only)
    uint32_t dlt_seen = 0;   // consumer cursor: last sampled dlt_count
};

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

/**
 * TERRAIN_PAGE_NACK(131) - the back channel of the baseline lane (Replica → World)
 *
 * WHY IT EXISTS. The terrain publisher treats a successful ws->send() as delivery: it advances the
 * chunk's syn_ver and clears the staging row immediately afterwards. send() only means "handed to
 * the socket", so anything the receiver refuses is lost FOREVER - the version cursor says the chunk
 * is already replicated, and it only returns to the wire if the terrain changes again.
 *
 * MEASURED 2026-08-06 15:50:59: one observer appeared, 289 baseline pages went out in one burst,
 * the Replica's frame park (HOFF_PAGE_PARK_MAX = 256) overflowed and logged exactly 289 - 256 = 33
 * "frame park full ... dropped" warnings. The mirror indexed 0 chunks. Nothing on the sending side
 * noticed, because nothing on the sending side could.
 *
 * WHAT IT CARRIES. The identity of ONE refused chunk plus the reason - region, chunk address and
 * the version the sender believed it had delivered. The World rewinds that chunk's syn_ver below
 * its ver, which is all the existing machinery needs: TerrainDltMarkSystem re-stages the row and
 * the publisher ships it again. No new retransmission path, no shadow queue - the version cursor
 * IS the retransmission mechanism, it simply never learned that a frame did not arrive.
 *
 * SCOPE. Load-baseline pages (handoff_id == 0). Handoff pages already detect loss through the
 * per-handoff page bitset against page_total, so they need no NACK.
 *
 * The reason byte reuses the GATE_REASON_* vocabulary of ase-replication (LANE_FULL = 6 is the
 * park overflow) so the refusal reads the same on both ends.
 */
constexpr uint32_t TRN_NACK_FRAME_SZ   = 22u;  // id(1) + region(4) + cx(4) + cz(4) + version(8) + reason(1)
constexpr uint32_t TRN_NACK_OFF_REGION = 1u;   // u32 offset of the region the chunk belongs to
constexpr uint32_t TRN_NACK_OFF_CX     = 5u;   // i32 offset of the refused chunk X
constexpr uint32_t TRN_NACK_OFF_CZ     = 9u;   // i32 offset of the refused chunk Z
constexpr uint32_t TRN_NACK_OFF_VER    = 13u;  // u64 offset of the version the sender had settled
constexpr uint32_t TRN_NACK_OFF_REASON = 21u;  // u8 offset of the GATE_REASON_* refusal cause

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
constexpr uint8_t HOFF_ACK_ACTORS_READY = 3u;  // B instantiated EVERY resident actor (phase 3 - THE commit gate). Module axis: for a handoff scoped by HOFF_MODSET(128) the SAME ack means "every resident actor OF THE MASKED GROUPS" - B knows its scope from the 125 fold, the frame is unchanged (no per-group ack field, the mask row is the scope authority)
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
//
// MIRROR CONDITIONS A1-A8 (CONTRACT AMENDMENT 2026-08-03, PLAN_ASE_COMPUTE_MOD_AXIS
// T1 verdict "TRAGFAEHIG MIT-AUFLAGEN"): every process that instantiates entities
// from these pages - the module-axis mirror of a partial handoff as much as the
// full-region handoff - obeys ALL EIGHT, they are protocol discipline, not hints:
//   A1  one allocator per number space: ONLY the owner process calls create();
//       mirrors use create(hint) exclusively (EnTT hands indices out densely from
//       0, two independent creators collide immediately)
//   A2  CHECK the hint return: create(hint) != hint is a HARD error path
//       (log::error + re-sync over SNAP_REQ(95)), never a silent continue -
//       a live index makes EnTT return a DIFFERENT id without any error
//   A3  causal order on the seam: deliver destroy(i,v) BEFORE create(i,v+1)
//       (the per-entity sequence IS the epoch)
//   A4  entity_epoch (u32, header below) is the identity authority; the 12-bit
//       EnTT version is a local recycling detail (4095 versions, then ABA)
//   A5  NO owner/process/group bits inside the u32 id (PLAN_ASE_COMPUTE.md:246)
//   A6  id width frozen at the uint32 default; ENTT_ID_TYPE override forbidden
//       (enforced by the static_assert in core/ase-ecs system.hpp)
//   A7  entity ids never ride the hub as a numeric f32 (version bits sit above
//       2^24) - HI/LO bit patterns only (hub_metrics.json discipline)
//   A8  the lifecycle rides the WIRE; on_construct/on_destroy are in-process
//       delegates and fire NOTHING across processes - local taps only
// The per-type emitters land with the ase-entity/ase-creature codegen systems
// (PLAN_ASE_COMPUTE_PHASE_06_WORLD WS-H.4); PlayerSnap (frame 100) is the built
// precedent and already travels logical ids + epoch, never raw EnTT ids.
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

// CONTRACT AMENDMENT 2026-08-02 (Task 14 scale-in). A SEVENTH intent class, added under the same
// precedent as the six tags above. The surplus rule (capacity_rcn_srpl_sys.cpp) is the reverse of
// FLOOR: it retires ONE running, region-less World instance of a project that owns more instances
// than its target justifies - measured live 2026-08-02, two FLOOR-ordered instances (9100/9102)
// sat idle with no rule to take them back and had to be stopped by hand. None of the six classes
// above carries that meaning, measured at their drains: CapacityReqSplitTag turns into a spawn
// whenever to_node is 0 (capacity_orch_req_splt_sys.cpp:291-292), CapacityReqSpawnTag turns every
// drained intent into a fresh port + node id reservation (capacity_orch_req_spwn_sys.cpp, reserve
// pass), CapacityReqRebalanceTag DROPS a targetless intent (capacity_orch_req_blnc_sys.cpp),
// CapacityReqMergeTag moves regions between running nodes (capacity_orch_req_mrge_sys.cpp), and
// CapacityReqRelinquishTag gives up a REGION over frame 94 (capacity_orch_req_rels_sys.cpp) -
// every one of them is REGION-directed and none stops an instance. This class is INSTANCE-directed
// and never reaches the wire: the drain (capacity_orch_req_retr_sys.cpp) stops the unit through
// the launcher seam and walks the existing death ladder (DeadTag + retire sweep + token drop-in
// removal), so no frame id is allocated and frame 93/94 stay untouched. Which instance dies is
// orchestration state, not scheduler state - the intent names only the project, exactly as the
// FLOOR spawn intent names no node.
struct CapacityReqRetireTag {};      // retire one surplus region-less instance (stop unit via seam, death ladder)

// CONTRACT AMENDMENT 2026-08-03 (module axis, PLAN_ASE_COMPUTE_MOD_AXIS.md T3/T4). The EIGHTH and
// NINTH intent classes, added under the same precedent as the seven above - and a GROUP intent POD
// beside CapacityReqXmitComponent, because the module axis moves a GROUP SUBSET of a region while
// the geometry stays untouched: none of the region intents carries a grp_mask, and widening the
// frozen five-field Xmit POD would touch every existing drain. MSPL (module split) fires when one
// group dominates a region's load vector (share >= CAP_MODSPLIT_DOMINANCE); MFSE (module re-fuse)
// reverses it when the dominance decayed below the hysteresis floor. Both are GROUP-directed:
// the assignment unit is the causality GROUP, never the single module.

/** One group-subset movement intent: which groups of a region move from which node to which node. */
struct CapacityReqGrpXmitComponent {
    uint32_t region_id = 0;  // REGION_ID_NONE until the scheduler picked the region
    uint32_t from_node = 0;  // node id giving the group subset up (the current holder)
    uint32_t to_node = 0;    // node id taking the subset over (0 = drain refuses, BLNC discipline)
    uint64_t grp_mask = 0;   // group subset that moves (mod_grp_bit set over the dense registry)
    uint32_t epoch = 0;      // monotonic intent epoch, so a stale intent is dropped not replayed
};

struct CapacityReqModSplitTag {};    // MSPL: the masked groups leave the region onto a second node
struct CapacityReqModFuseTag {};     // MFSE: the masked groups return to the region's main owner

/**
 * CapacityGrpAsgnComponent - one standing group assignment the orchestrator relayed (Engine ledger).
 *
 * The SRPL liftability gate reads it: a node that carries the LAST instance of a group assignment
 * is NOT liftable (the analogue of the FLOOR-coverage gate) - without this row a module-split
 * target looks region-less to the surplus rule and would be retired while it simulates. Written by
 * the MSPL drain (L4), erased by the MFSE drain, read by the surplus rule (L3) - the shared home
 * is this L0 header, exactly like the intent PODs above.
 */
struct CapacityGrpAsgnComponent {
    uint32_t node_id = 0;    // node serving the group subset (the not-liftable carrier)
    uint32_t region_id = 0;  // region whose subset it serves
    uint64_t grp_mask = 0;   // the standing subset (mod_grp_bit set)
};

/**
 * CapacityRetireBusyComponent - the drain's answer when a retire order found only WORKING seeds.
 *
 * The fourth POD of the retire amendment, and it closes the attribution gap the two count axes
 * open: the scheduler's instance count is keyed by the SEED project while its coverage count is
 * keyed by the REGION's project - so a node seeded for P that covers only ANOTHER project's
 * regions inflates P's instance count forever, yet is never retirable (a holder is untouchable).
 * Without this answer the surplus rule re-raises the same order at every shard visit and the
 * drain drops it every time - an unbounded decide/drop loop of intent churn and warn spam.
 *
 * So the drain replaces such an order with this row (the Genesis-WAIT precedent: the state is
 * named ONCE, then the log stays quiet), stamped with the bridged instance count the verdict was
 * made at. The scheduler stands down while the count still matches and destroys the row the
 * moment it moved; the drain itself removes the row as soon as a retirable candidate exists
 * again, because "my seed went idle" changes NO number the scheduler can see.
 */
struct CapacityRetireBusyComponent {
    uint32_t proj_hash = 0;      // project whose retire order found only working seeds
    uint32_t inst_snapshot = 0;  // bridged instance count the verdict was made at (0 = unset)
};

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

/**
 * CapacityRegionAdopComponent - one ownership row delivered so a restarted Engine can rebuild it.
 *
 * The node row above restores WHICH compute nodes exist. This one restores WHAT THEY SERVE, and
 * without it a restored fleet is a set of anonymous processes: the orchestrator's assignment ledger
 * and the scheduler's region rows are both RAM-only, so an Engine restart forgets every region to
 * node binding while the Worlds keep serving their rects unchanged.
 *
 * That gap is not a blind spot, it is a standstill (measured 2026-08-01, Engine restart 13:27:34):
 * the Replica's surviving DECLARE holds CAP_PROJ_LIVE_REGIONS at 1, so GENESIS - which acts only on
 * a project with NO region - stands down; no intent exists for the assign path to work on; and the
 * coverage count reads 0 because the row names node 7, an identity the restarted Engine never
 * adopted. Nothing can create the state, nothing can repair it, and FLOOR reads that 0 as a deficit
 * and orders one more instance every cycle.
 *
 * The Replica holds the truth across the restart - ReplicaWrldRgnComponent carries region_id,
 * proj_hash, epoch and the mesh identity node_id, joined to its rect by region_id, and the durable
 * copy lives in the capacity_regions collection the handoff path already writes. So the row travels
 * the same filtered bridge and lands in ase-capacity (L3), which may read the hub. The plugin (L4)
 * may not, so the delivered row crosses HERE, exactly like the node row above.
 *
 * Like that one it is a REQUEST, not a claim: it states that this region was owned by this node.
 * Whether the node still exists is the adopt pass's question, and a row whose node was not adopted
 * is NOT restored but re-queued - an orphaned region belongs back in the assign loop, never into a
 * ledger that points at nobody.
 *
 * The rect rides beside it in the RegionRect POD above, on the same delivery entity: rect geometry
 * has one shape in this codebase and a second copy of the four edges would be a second truth.
 */
struct CapacityRegionAdopComponent {
    uint32_t region_id = 0;  // the region's durable identity
    uint32_t proj_hash = 0;  // project the region belongs to
    uint32_t node_id = 0;    // mesh identity that served it (0 = unbound, re-queue instead)
    uint32_t epoch = 0;      // assignment epoch, so a rebuilt row keeps its ordering
};

}  // namespace ase::types
