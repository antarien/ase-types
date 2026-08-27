#pragma once

/**
 * ASE FOUNDATION TYPES - RESIDENCY SEAM (L0)
 *
 * @file        residency.hpp
 * @brief       ResAlivRect - one active simulation area on THIS node, in world metres
 * @description Die dritte CPU-Achse (Residenz entlang des Graphen) braucht genau EINE
 *              modulübergreifende Tatsache: WELCHE Flächen dieses Knotens gerade bespielt
 *              sind. Der Erzeuger ist ase-geoid (Sektor-Footprints seiner eigenen
 *              Zellaggregation plus Beobachter-Umfelder aus den Stern-Positionszeilen);
 *              Verbraucher sind die Familien-Sweeps (zuerst CharacterResSwpSystem), die
 *              ihre eigenen Arbeitskopien außerhalb jeder aktiven Fläche parken.
 *
 *              DIE NAHT IST L0 UND KEIN HUB-WERT, aus demselben Grund wie types::RegionRect:
 *              "Sibling World modules region-gate via view<types::RegionRect>() - keeps every
 *              L3->L3 edge out" (Audit code-wlife-world). Ein chunk-adressierter Hub-Owner
 *              für Zellzustände ist durch den Lattice-Contract ausdrücklich verboten
 *              (PLAN_ASE_LATTICE_PHASE_00_CONTRACT WS-K.2); diese Zeile adressiert keinen
 *              Chunk und verlässt den Knoten nie - sie ist eine prozesslokale Sicht-Naht,
 *              kein Transport.
 *
 *              METER, NICHT ZELLEN: Erzeuger und Verbraucher rechnen auf VERSCHIEDENEN
 *              Gittern (Hex-Waben, Spatial-Zellen, Chunks). Wer eine Zellgröße
 *              veröffentlicht, veröffentlicht eine Rechenvorschrift (Befund 2026-08-19,
 *              character_life_spwn_sys.cpp) - die Naht spricht deshalb die eine Einheit,
 *              die jede Seite ohne fremde Konstante versteht.
 *
 *              ENTHALTUNGSTEST: geschlossenes Rechteck, east0_m <= e <= east1_m und
 *              north0_m <= n <= north1_m. Ränder zählen als drinnen - an der Kante einer
 *              aktiven Fläche wird simuliert, nicht geparkt.
 *
 * @module      ase-types (Foundation)
 * @layer       0 (Foundation - NO ECS dependency)
 * @category    state
 * @created     2026-08-26
 * @modified    2026-08-26
 * @version     1.0.0
 *
 * ECS COMPONENT COMPLIANCE (L0 seam POD, ridden by entities like types::RegionRect)
 *
 * [ ] DATA fields ONLY - No methods
 * [ ] NO .cpp file - Header-only
 * [ ] ONLY zero-initialization
 * [ ] No magic numbers in defaults
 * [ ] Entity references - none
 * [ ] Single responsibility - one active area, nothing else
 * [ ] No God-Component
 * [ ] ONLY primitive types (float)
 */

namespace ase::types {

/**
 * @brief ResAlivRect - one active simulation area on this node, world metres, closed rect
 *
 * Erzeuger: GeoidResAlivSystem (eine Zeile je Sektor-Footprint, eine je Beobachter).
 * Verbraucher: Familien-Sweeps (view<types::ResAlivRect>, Menge einmal je Takt gelesen).
 * Lebensdauer: der Erzeuger legt an, führt nach und entfernt; Verbraucher lesen nur.
 * Null Zeilen bedeuten "Aktivmenge unbekannt" und sind für jeden Sweep ein HALT,
 * nie ein "alles ist inaktiv" - die lügende Leere darf keine Welt entladen.
 */
struct ResAlivRect {
    float east0_m = 0.0f;   // Westkante, Weltmeter
    float north0_m = 0.0f;  // Südkante, Weltmeter
    float east1_m = 0.0f;   // Ostkante, Weltmeter (einschließlich)
    float north1_m = 0.0f;  // Nordkante, Weltmeter (einschließlich)
};

}  // namespace ase::types
