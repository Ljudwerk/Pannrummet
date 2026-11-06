// Fabriksbandet: Pannrummets 30 fabriksprogram i fyra kategorier – Trummor, EBM, Röst och Industri –
// och Init. Varje program anger bara det som skiljer sig från grundvärdena (alla moduler av).
// Värdena är parametrarnas riktiga värden: dB, ms, sekunder, procent för Schaktets återkoppling,
// 0–1 för rattar, index för vridomkopplare och 0/1 för brytare.
//
// Utnivån är satt så att programmen ligger ungefär lika starkt som det torra ljudet (mätt med
// PRRender --program), så att man kan bläddra utan stora nivåhopp.
//
// Lägg nya program sist i en kategori; programnumren visas i räkneverket.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/presets/Programbank.h>
#include "../Parameters.h"

namespace pr
{

inline std::vector<krang::presets::Program> fabriksbandet()
{
    using P = krang::presets::Program;
    auto prog = [] (const char* namn, const char* kategori, std::initializer_list<std::pair<const char*, float>> v)
    {
        P p;
        p.namn = svt (namn);
        p.kategori = svt (kategori);
        for (auto& [id, varde] : v) p.varden.push_back ({ id, varde });
        return p;
    };
    // Grindens taktmönster som 16 tecken: x = steg på, . = av.
    auto steg = [] (const char* monster, std::vector<std::pair<juce::String, float>>& ut)
    {
        for (int i = 0; i < 16; ++i) ut.push_back ({ id::grindSteg (i), monster[i] == 'x' ? 1.0f : 0.0f });
    };
    auto medSteg = [&] (P p, const char* monster) { steg (monster, p.varden); return p; };

    return {
        prog ("Init", "Grund", {}),

        // --- Trummor -------------------------------------------------------------------------
        prog ("Gatad virvel", "Trummor", { { id::utNiva, -2.5f },
            { id::grindPa, 1 }, { id::grindLage, 1 }, { id::grindTroskel, -32 }, { id::grindHall, 40 }, { id::grindSlapp, 30 },
            { id::hallPa, 1 }, { id::hallLokal, 1 }, { id::hallStorlek, 0.8f }, { id::hallHalltid, 220 }, { id::hallFall, 25 },
            { id::hallBlandning, 0.5f }, { id::hallGrind, 1 } }),
        prog ("Trumbuss 1987", "Trummor", {
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.35f }, { id::pressTyngd, 0.5f }, { id::pressTon, 0.55f },
            { id::hallPa, 1 }, { id::hallLokal, 0 }, { id::hallStorlek, 0.6f }, { id::hallHalltid, 120 }, { id::hallFall, 20 },
            { id::hallBlandning, 0.3f }, { id::hallGrind, 1 } }),
        prog ("Stålverkstad", "Trummor", {
            { id::platPa, 1 }, { id::platMaterial, 0 }, { id::platStamning, 45 }, { id::platKlang, 1.2f }, { id::platLjushet, 0.6f },
            { id::platBlandning, 0.35f }, { id::platMidi, 0 },
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.3f } }),
        prog ("Fabriksport", "Trummor", { { id::utNiva, -2.0f },
            { id::pressPa, 1 }, { id::pressLage, 2 }, { id::pressDriv, 0.4f }, { id::pressOver4, 1 },
            { id::hallPa, 1 }, { id::hallLokal, 3 }, { id::hallStorlek, 0.9f }, { id::hallBlandning, 0.35f }, { id::hallGrind, 0 } }),
        medSteg (prog ("Hackad loop", "Trummor", {
            { id::grindPa, 1 }, { id::grindLage, 2 }, { id::grindHall, 70 }, { id::grindSlapp, 15 },
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.3f } }), "x.xx.x.xx.x.xx.x"),
        prog ("Smäll i valvet", "Trummor", { { id::utNiva, -2.5f },
            { id::grindPa, 1 }, { id::grindLage, 1 }, { id::grindTroskel, -30 },
            { id::hallPa, 1 }, { id::hallLokal, 2 }, { id::hallStorlek, 0.9f }, { id::hallHalltid, 400 }, { id::hallFall, 80 },
            { id::hallBlandning, 0.45f }, { id::hallGrind, 1 } }),
        prog ("Bitkrossade trummor", "Trummor", {
            { id::pressPa, 1 }, { id::pressLage, 3 }, { id::pressDriv, 0.55f }, { id::pressTon, 0.6f }, { id::torrVat, 0.7f } }),
        prog ("Klockslag", "Trummor", {
            { id::platPa, 1 }, { id::platMaterial, 3 }, { id::platStamning, 52 }, { id::platKlang, 3.0f }, { id::platLjushet, 0.5f },
            { id::platBlandning, 0.3f }, { id::platMidi, 0 },
            { id::schaktPa, 1 }, { id::schaktTid, 5 }, { id::schaktAterk, 35 }, { id::schaktMorker, 0.6f }, { id::schaktBlandning, 0.25f } }),

        // --- EBM -----------------------------------------------------------------------------
        prog ("Kolvbas", "EBM", {
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.5f }, { id::pressTon, 0.55f }, { id::pressTyngd, 0.3f } }),
        prog ("Distad bas 2", "EBM", {
            { id::pressPa, 1 }, { id::pressLage, 2 }, { id::pressDriv, 0.45f }, { id::pressOver4, 1 },
            { id::platPa, 1 }, { id::platMaterial, 4 }, { id::platStamning, 45 }, { id::platKlang, 0.3f }, { id::platLjushet, 0.3f },
            { id::platBlandning, 0.2f }, { id::platMidi, 0 } }),
        medSteg (prog ("Pumpande bas", "EBM", {
            { id::grindPa, 1 }, { id::grindLage, 2 }, { id::grindHall, 90 }, { id::grindSlapp, 25 }, { id::grindDjup, 0.85f },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.35f } }), ".x.x.x.x.x.x.x.x"),
        prog ("Marschbas", "EBM", {
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.4f }, { id::pressTon, 0.45f },
            { id::schaktPa, 1 }, { id::schaktTid, 3 }, { id::schaktAterk, 35 }, { id::schaktMorker, 0.7f }, { id::schaktBlandning, 0.2f },
            { id::schaktPingpong, 0 } }),
        prog ("Stötar i hallen", "EBM", { { id::utNiva, -2.0f },
            { id::schaktPa, 1 }, { id::schaktTid, 2 }, { id::schaktAterk, 40 }, { id::schaktBlandning, 0.25f },
            { id::hallPa, 1 }, { id::hallLokal, 1 }, { id::hallStorlek, 0.7f }, { id::hallBlandning, 0.3f }, { id::hallGrind, 0 } }),
        prog ("Synklead i schaktet", "EBM", {
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.25f },
            { id::schaktPa, 1 }, { id::schaktTid, 3 }, { id::schaktAterk, 55 }, { id::schaktSvaj, 0.3f }, { id::schaktBlandning, 0.35f } }),
        prog ("Hela kedjan", "EBM", {
            { id::grindPa, 1 }, { id::grindLage, 1 }, { id::grindTroskel, -40 },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.4f }, { id::pressTyngd, 0.3f },
            { id::schaktPa, 1 }, { id::schaktTid, 3 }, { id::schaktBlandning, 0.15f },
            { id::hallPa, 1 }, { id::hallLokal, 1 }, { id::hallBlandning, 0.25f }, { id::hallGrind, 1 } }),
        prog ("Spänningsfält", "EBM", { { id::utNiva, -3.5f },
            { id::grindPa, 1 }, { id::grindLage, 1 }, { id::grindTroskel, -42 },
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.35f },
            { id::platPa, 1 }, { id::platMaterial, 0 }, { id::platStamning, 45 }, { id::platBlandning, 0.15f }, { id::platMidi, 0 },
            { id::hallPa, 1 }, { id::hallBlandning, 0.2f }, { id::trycket, 0.6f } }),
        prog ("Kroppen i rörelse", "EBM", {
            { id::grindPa, 1 }, { id::grindLage, 0 }, { id::grindTroskel, -45 }, { id::grindSlapp, 60 },
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.3f },
            { id::schaktPa, 1 }, { id::schaktTid, 0 }, { id::schaktAterk, 25 }, { id::schaktBlandning, 0.15f } }),

        // --- Röst ----------------------------------------------------------------------------
        prog ("Megafon", "Röst", {
            { id::megaPa, 1 }, { id::megaLage, 0 }, { id::megaBand, 1600 }, { id::megaResonans, 0.5f }, { id::megaSmuts, 0.5f },
            { id::hallPa, 1 }, { id::hallLokal, 0 }, { id::hallBlandning, 0.15f }, { id::hallGrind, 0 } }),
        prog ("Paroll i schaktet", "Röst", {
            { id::megaPa, 1 }, { id::megaLage, 0 }, { id::megaSmuts, 0.5f },
            { id::schaktPa, 1 }, { id::schaktTid, 4 }, { id::schaktAterk, 50 }, { id::schaktMorker, 0.6f }, { id::schaktSvaj, 0.3f },
            { id::schaktBlandning, 0.35f },
            { id::hallPa, 1 }, { id::hallLokal, 3 }, { id::hallStorlek, 0.7f }, { id::hallBlandning, 0.2f }, { id::hallGrind, 0 } }),
        prog ("Telefonlinje", "Röst", {
            { id::megaPa, 1 }, { id::megaLage, 1 }, { id::megaBand, 1200 }, { id::megaSmuts, 0.6f } }),
        prog ("Radiostörning", "Röst", {
            { id::megaPa, 1 }, { id::megaLage, 2 }, { id::megaBand, 1400 }, { id::megaSmuts, 0.4f }, { id::megaBrus, 1 } }),
        prog ("Trattrop", "Röst", {
            { id::megaPa, 1 }, { id::megaLage, 3 }, { id::megaBand, 900 }, { id::megaResonans, 0.6f }, { id::megaSmuts, 0.5f },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.3f } }),
        prog ("Robotröst", "Röst", {
            { id::platPa, 1 }, { id::platMaterial, 4 }, { id::platStamning, 48 }, { id::platKlang, 0.4f }, { id::platLjushet, 0.6f },
            { id::platBlandning, 0.6f }, { id::platMidi, 1 },
            { id::megaPa, 1 }, { id::megaLage, 0 }, { id::megaSmuts, 0.3f }, { id::megaBlandning, 0.6f } }),
        prog ("Kör i valvet", "Röst", { { id::utNiva, -5.0f },
            { id::hallPa, 1 }, { id::hallLokal, 2 }, { id::hallStorlek, 0.9f }, { id::hallBlandning, 0.4f }, { id::hallGrind, 0 },
            { id::schaktPa, 1 }, { id::schaktTid, 6 }, { id::schaktAterk, 30 }, { id::schaktMorker, 0.7f }, { id::schaktBlandning, 0.2f } }),

        // --- Industri ------------------------------------------------------------------------
        prog ("Ras i schaktet", "Industri", { { id::utNiva, -2.5f },
            { id::platPa, 1 }, { id::platMaterial, 0 }, { id::platStamning, 38 }, { id::platKlang, 3.0f }, { id::platLjushet, 0.7f },
            { id::platBlandning, 0.5f }, { id::platMidi, 0 },
            { id::pressPa, 1 }, { id::pressLage, 2 }, { id::pressDriv, 0.4f } }),
        prog ("Städet", "Industri", { { id::utNiva, -2.5f },
            { id::platPa, 1 }, { id::platMaterial, 1 }, { id::platStamning, 43 }, { id::platKlang, 0.6f }, { id::platBlandning, 0.5f },
            { id::platFore, 1 }, { id::platMidi, 0 },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.6f } }),
        prog ("Fjäderhus", "Industri", { { id::utNiva, -4.5f },
            { id::platPa, 1 }, { id::platMaterial, 2 }, { id::platStamning, 33 }, { id::platKlang, 2.0f }, { id::platLjushet, 0.4f },
            { id::platBlandning, 0.45f }, { id::platMidi, 0 },
            { id::hallPa, 1 }, { id::hallLokal, 3 }, { id::hallBlandning, 0.3f }, { id::hallGrind, 0 } }),
        prog ("Gruvgång", "Industri", { { id::utNiva, -5.0f },
            { id::hallPa, 1 }, { id::hallLokal, 3 }, { id::hallStorlek, 1.0f }, { id::hallBlandning, 0.55f }, { id::hallGrind, 0 },
            { id::schaktPa, 1 }, { id::schaktTid, 6 }, { id::schaktAterk, 70 }, { id::schaktMorker, 0.8f }, { id::schaktSvaj, 0.5f },
            { id::schaktBlandning, 0.3f } }),
        medSteg (prog ("Haveri", "Industri", { { id::utNiva, -4.0f },
            { id::grindPa, 1 }, { id::grindLage, 2 }, { id::grindHall, 50 }, { id::grindSlapp, 10 },
            { id::pressPa, 1 }, { id::pressLage, 3 }, { id::pressDriv, 0.5f },
            { id::platPa, 1 }, { id::platMaterial, 0 }, { id::platStamning, 40 }, { id::platBlandning, 0.3f }, { id::platMidi, 0 },
            { id::schaktPa, 1 }, { id::schaktTid, 1 }, { id::schaktAterk, 60 }, { id::schaktBlandning, 0.3f },
            { id::hallPa, 1 }, { id::hallLokal, 3 }, { id::hallBlandning, 0.3f }, { id::hallGrind, 0 },
            { id::trycket, 0.8f } }), "xx.x..x.xxx..x.x"),
    };
}

} // namespace pr
