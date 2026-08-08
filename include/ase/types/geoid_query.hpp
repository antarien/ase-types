#pragma once

/**
 * ASE Layer 0 POD - Lattice Query Seam
 *
 * @file        geoid_query.hpp
 * @brief       The PODs two Layer 3 modules share to ask the planetary lattice a question
 * @description `modules/ase-geoid` owns the planetary hexagon lattice, and other Layer 3 modules
 *              need its answers. A Layer 3 module must never include another Layer 3 module
 *              (WRFL_ASE_MODULE_DEPENDENCIES.md, Section 3), and a Hub key cannot carry a cell
 *              address either - the Hub value is a float and its owner may not be a coordinate
 *              (PLAN_ASE_COMPUTE.md:246, :326). The one carrier that stays inside the layer rule
 *              is a POD whose TYPE lives HERE, below both modules: the consumer emplaces the
 *              question, `ase-geoid` emplaces the answer on the same entity, and both include
 *              downward only. That is the established pattern of `region_wire.hpp`, where
 *              `ase-capacity` and `ase-pl-capacity-orch` meet the same way.
 *
 *              NO ECS HERE. These are plain structs plus empty tags - no registry, no EnTT, no
 *              behaviour. Layer 0 stays ECS free; the ECS meaning is given by the modules that
 *              emplace them.
 *
 *              THE ADDRESS IS THE CHUNK ADDRESS. (cx,cz) as int32 is the cell address of Master
 *              binding decision 1, and there is no second cell id world. The address travels in
 *              the PAYLOAD of these PODs, never as an owner hash.
 *
 * @module      ase-types
 * @layer       0 (Foundation)
 * @created     2026-08-06
 * @modified    2026-08-06
 * @version     1.0.0
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

}  // namespace ase::types
