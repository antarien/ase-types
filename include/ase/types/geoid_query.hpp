#pragma once

/**
 * ASE Layer 0 POD - Lattice Query Seam
 *
 * @file        geoid_query.hpp
 * @brief       The address rule of the lattice - a buffer capacity and an owner fold, no ECS
 * @description `modules/ase-geoid` owns the planetary hexagon lattice, and other Layer 3 modules
 *              need its answers. A Layer 3 module must never include another Layer 3 module
 *              (WRFL_ASE_MODULE_DEPENDENCIES.md, Section 3), and a Hub key cannot carry a cell
 *              address either - the Hub value is a float and its owner may not be a coordinate
 *              (PLAN_ASE_COMPUTE.md:246, :326).
 *
 *              WHAT LAYER 0 CAN CARRY HERE, AND WHAT IT CANNOT. It cannot carry a DATUM between
 *              two Layer 3 modules of the same tier - the seven question and answer PODs that
 *              tried are documented below, and they fell away with the module boundary that made
 *              them necessary. It cannot carry the NAME OF A CLASS either: an empty tag IS an ECS
 *              component, and the three that stood here have moved to their owner module (note
 *              below). What it CAN carry is a rule for computing a number - `geoid_poi_owner`
 *              folds (project, ordinal) into a Hub owner, and that fold crosses a real TIER
 *              boundary: the World writes GEO_POIS_CX/CZ under it (geoid_pois_pub_sys.cpp:235)
 *              and the Replica reads them back (replica_cap_push_sys.cpp:604). Two processes,
 *              two registries, no shared Hub - which is exactly what Layer 0 exists for.
 *
 *              NO ECS HERE. What remains is a constant and a fold - no registry, no EnTT, no
 *              component, no tag, no behaviour. Layer 0 stays ECS free.
 *
 *              THE ADDRESS IS THE CHUNK ADDRESS. (cx,cz) as int32 is the cell address of Master
 *              binding decision 1, and there is no second cell id world. It is DERIVED from the
 *              place with the running rung and never travels: neither through this header nor
 *              beside a place, because a stored address is a second truth that the next epoch
 *              change makes wrong.
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @created     2026-08-06
 * @modified    2026-08-14
 * @version     1.2.0
 *
 * DRY / SOLID / SSOT COMPLIANCE:
 * - NO ECS: no registry, no EnTT, no component behaviour in this header
 * - NO allocation and no methods: pure data, zero initialised
 * - The lattice itself lives in modules/ase-geoid; this header only carries the seam
 */

#include <cstdint>

namespace ase::types {

/**
 * MEMORY CONTRACT OF THE ANSWER - not the parameters of the lattice
 *
 * Was hier steht, ist die groesste Ringlaenge, die eine Antwort je tragen kann. Ein Verbraucher
 * legt daraus einen Puffer an und iteriert die GEMELDETE Zahl (`ngbr_count`), nie diese hier. Das
 * ist eine Kapazitaet zur Uebersetzungszeit - ein Array laesst sich nicht aus einem Laufzeitwert
 * dimensionieren - und keine Aussage darueber, wie viele Nachbarn eine Zelle wirklich hat.
 *
 * MIGRATED 2026-08-06 (WRFL_ASE_CONSTANTS.md): vier Werte standen hier zusaetzlich und waren
 * PARAMETER der Konstruktion, keine Vertragsgroessen. Ein Parameter, den beide Seiten kennen
 * muessen, reist als Datum ueber den Hub, statt in zwei Schichten einkompiliert zu werden:
 *
 * ┌──────────────────────────────┬───────────────────────────────┬───────────────────────────────┐
 * │ war hier                     │ ist jetzt Hub-Schluessel      │ Befund bei der Ablesung        │
 * ├──────────────────────────────┼───────────────────────────────┼───────────────────────────────┤
 * │ GEOID_ADDRESS_FREQUENCY      │ GEO_CONST_ADDRESS_FREQUENCY   │ 0 Verbraucher ausserhalb      │
 * │ GEOID_ADDRESS_FACE_STRIDE    │ GEO_CONST_ADDRESS_FACE_STRIDE │ 0 Verbraucher ausserhalb      │
 * │ GEOID_ADDRESS_LEVEL_MAX      │ GEO_CONST_ADDRESS_LEVEL_MAX   │ Erzeuger der Vertragszeile    │
 * │ GEOID_PENTAGON_NEIGHBORS     │ GEO_CONST_PENTAGON_NEIGHBORS  │ semantischer Vergleich        │
 * └──────────────────────────────┴───────────────────────────────┴───────────────────────────────┘
 *
 * Die Werte selbst stehen in modules/ase-hub/data/hub_constants.json. Wer sie braucht, liest sie
 * dort und legt sie auf eine Eingangskomponente; ein TypeScript-Client kann den Hub nicht lesen,
 * deshalb endet jede Kette in einer Komponente, die beide Seiten gleich sehen.
 */
constexpr uint32_t GEOID_MAX_NEIGHBORS = 6;       // Ring buffer capacity of ONE answer

/**
 * DIE SIEBEN FRAGE- UND ANTWORT-PODS STANDEN HIER. SIE SIND ERSATZLOS ENTFALLEN.
 *
 * Es waren GeoidAskTag, GeoidAskRingTag, GeoidAskCellComponent, GeoidAskGeoComponent,
 * GeoidAnsCellComponent, GeoidAnsNgbrComponent und GeoidAnsNgbrTag. Ein Verbraucher legte eine
 * Frage auf eine Entity, `modules/ase-geoid` legte die Antwort auf dieselbe Entity, und beide
 * schlossen nur nach unten ein.
 *
 * DER GRUND FUER DIE ENTFERNUNG IST NICHT, DASS DIE NAHT SCHLECHT GEBAUT WAR.
 * Sie war die einzige Form, die die Schichtregeln zuliessen - und genau das war der Befund. Eine
 * entity-gebundene Frage zwischen zwei Layer-3-Modulen desselben Tiers ist nicht ausdrueckbar:
 * Layer 0 kann keine Paritaet tragen, ein Modul darf kein zweites seiner Schicht einschliessen, und
 * ein Hub-Wert traegt keine Koordinate. Wenn die einzige mogliche Form eine Behelfsform ist, steht
 * die Modulgrenze falsch. Der Fragesteller - der Zellzustand der Wabe - lag im falschen Modul.
 *
 * Er liegt jetzt in `modules/ase-geoid`, zusammen mit der Gitter-Pipeline, die seine Fragen
 * beantwortet. Damit gibt es keinen fremden Frager mehr: jeder Verbraucher legt
 * `geoid::GeoidStaCellComponent` selbst an und liest `geoid::GeoidStaAncrComponent` von derselben
 * Entity. Die zwei Adapter, die zwischen PODs und Modulkomponenten uebersetzten
 * (GeoidCellAskSystem, GeoidCellAnsSystem), sind mit den PODs verschwunden.
 *
 * WAS BLEIBT, ist die Konstante oben. Sie ist kein Naht-Bestandteil, sondern eine Puffer-Kapazitaet
 * zur Uebersetzungszeit - ein Array laesst sich nicht aus einem Laufzeitwert dimensionieren.
 */

/**
 * DER BESITZER EINER ORTSZEILE IM HUB - eine Adressregel, keine Frage-Antwort-Naht
 *
 * Der Ort k eines Projekts ist ein Datum, das der World kennt und der Replica beantworten muss:
 * `ase player spawn --place k` fragt nach der Zelle, auf der Ort k liegt, und der Weg dorthin ist
 * der Hub (GEO_POI_CX / GEO_POI_CZ). Ein Hub-Wert traegt keine Koordinate, aber er traegt einen
 * BESITZER - und ein Ort ist durch (Projekt, Ordinal) eindeutig benannt. Genau diese beiden Zahlen
 * faltet die Regel unten zu dem Besitzer, unter dem der Erzeuger schreibt und der Beantworter
 * liest.
 *
 * SIE STEHT IN LAYER 0, WEIL BEIDE SEITEN SIE BRAUCHEN UND KEINE DIE ANDERE EINSCHLIESSEN DARF:
 * `modules/ase-geoid` schreibt (World-Tier), `modules/ase-replication` liest (Replica-Tier), und
 * ein L3-Modul schliesst kein zweites seiner Schicht ein. Eine Adressregel ist das, was Layer 0
 * tragen KANN - anders als die entity-gebundene Frage, die oben aus genau diesem Grund entfiel:
 * hier reist keine Koordinate durch L0, nur die Rechenvorschrift fuer eine Zahl.
 *
 * Die Faltung ist die klassische Hash-Kombination (Streuung der einen Zahl gegen die andere, statt
 * eines nackten XOR): ohne sie lieferten Projekt A/Ort 1 und Projekt B/Ort 0 mit benachbarten
 * Projekt-Hashes systematisch denselben Besitzer, und ein Projekt bekaeme still die Orte eines
 * anderen. Ein Ordinal ist klein und dicht - gerade dort ist ein nacktes XOR am schwaechsten.
 */
constexpr uint32_t GEOID_POI_OWNER_MIX = 0x9E3779B9u;  // Golden-ratio odd word of the fold

constexpr uint32_t geoid_poi_owner(uint32_t proj_hash, uint32_t ordinal) {
    return proj_hash ^ (ordinal + GEOID_POI_OWNER_MIX + (proj_hash << 6) + (proj_hash >> 2));
}

/* DIE KLASSEN EINER ORTSMELDUNG UND DIE ORTSANFRAGE STEHEN NICHT MEHR HIER.
 *
 * Es waren `GeoidReqIntrPendTag`, `GeoidReqIntrEdgeTag` und `GeoidReqLocTag`, dazu
 * `GeoidReqIntrAirTag` und `GeoidReqIntrSprTag`. Sie zogen zuerst nach
 * `modules/ase-geoid/include/ase/geoid/components/tag/`, wo das Gitter seine uebrigen Tags fuehrt.
 * Davon steht heute (2026-08-15) nur noch `GeoidReqIntrPendTag` dort: er ist kein Klassen-Tag,
 * sondern der Pending-Marker, den `geoid_mrkr_sys.cpp:GeoidMrkrSystem` in seiner View liest. Die
 * DREI Klassen-Tags sind ersatzlos entfallen, weil die Klasse einer Meldung nicht beim Verbraucher
 * entsteht, sondern beim ERZEUGER - und der erreicht ein Modul-Tag des Gitters nicht. Sie steht
 * jetzt in `modules/ase-hub` (`HubGeoClsEdgeTag` und Geschwister), also in der Mitte des Sterns,
 * die beide Seiten kennen duerfen.
 *
 * DER GRUND IST DERSELBE, DER DIE BEIDEN ANDEREN SCHON BEWEGT HAT, UND ER GALT IMMER FUER ALLE
 * FUENF. Der Text, der hier stand, argumentierte die Platzierung selbst weg: ein LEERER Tag trage
 * nur den NAMEN einer Klasse und kein Datum, also duerfe er unter den Modulen sitzen. Das gilt fuer
 * eine Formel und bricht bei einem Tag. Ein Tag IST eine ECS-Komponente - er wird emplaced, er
 * filtert eine View, er hat eine Lebenszeit an einer Entitaet. Layer 0 ist als frei von
 * ECS-Abhaengigkeit definiert.
 *
 * WARUM ES SO LANGE UNBEMERKT BLIEB, IST DIE EIGENTLICHE LEHRE: beide Tore lesen hier gleichzeitig
 * leer. `foundation/` traegt keine `codegen.json`, ist also paritaetsunfaehig und erreicht becsy
 * nie; und der Struktur-Validator greift in dieser Schicht nicht. Kein Build meldet etwas, nichts
 * wird rot, und die Sache sieht sauber aus.
 *
 * KEIN VERBRAUCHER VERLOR DABEI ETWAS. Der Terrain-Erzeuger setzt keinen fremden MODUL-Tag mehr,
 * sondern stellt seine Tatsache als Wert im Stern fest; das Gitter hebt daraus seine EIGENEN Tags.
 * Der Stern hat wieder eine Mitte.
 *
 * NACHTRAG 2026-08-15 (Geovis Phase 02): der erste Ersatz schrieb diesen Wert als sieben eigene
 * Schluessel unter `hub::GLOBAL` (TER_CROSS_*), und ein modul-eigenes Spiegelsystem im Gitter las
 * sie. Beides ist entfallen. Ein Erzeuger schreibt jetzt die VIER Vertragsschluessel
 * GEO_POS_LAT_DEG/_LON_DEG/_ALT_M/_META unter SEINER MELDUNG als Owner und hebt dort
 * `hub::HubGeoPosTag` plus ein Klassen-Tag; ein einziger Leser im Gitter zaehlt ueber diesen Merker
 * auf. Zwei Gruende, und beide gehoeren zu der Lehre oben:
 *   - `hub::GLOBAL` ist EIN Platz. N Meldungen in einem Takt ueberschrieben einander, und N-1
 *     verschwanden lautlos, waehrend die Meldezeile weiterhin N nannte.
 *   - Ein Spiegel je Erzeuger ist ein Kanal je Domaene. Neun Erzeuger waeren 56 Schluessel gewesen.
 * Die Klassen-Tags liegen dabei in `modules/ase-hub` - in der MITTE des Sterns, die beide Seiten
 * kennen duerfen -, nicht hier. Dass zwei Module einen Typ sehen muessen, ist NIE das Argument fuer
 * Layer 0; hier gehoert nur, was eine TIER-Grenze quert. */

}  // namespace ase::types
