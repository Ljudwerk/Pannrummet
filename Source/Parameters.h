// Pannrummets parametrar. ID:na har formen st_<modul>_<vad> och får ALDRIG döpas om när de väl finns i
// ett släppt bygge. Nya parametrar läggs sist i listan, så att DAW-automation står kvar.
//
// Alla sex modulernas parametrar finns från fas 1, även för de moduler vars ljud kommer i fas 2.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace pr
{
inline juce::String svt (const char* utf8) { return juce::String::fromUTF8 (utf8); }

namespace id
{
    // Grinden
    inline constexpr const char* grindPa        = "pr_grind_pa";
    inline constexpr const char* grindLage      = "pr_grind_lage";
    inline constexpr const char* grindTroskel   = "pr_grind_troskel";
    inline constexpr const char* grindHall      = "pr_grind_hall";
    inline constexpr const char* grindSlapp     = "pr_grind_slapp";
    inline constexpr const char* grindDjup      = "pr_grind_djup";
    inline constexpr const char* grindSidokedja = "pr_grind_sidokedja";
    inline juce::String grindSteg (int i) { return "pr_grind_steg" + juce::String (i + 1).paddedLeft ('0', 2); }   // 01..16
    // Pressen
    inline constexpr const char* pressPa        = "pr_press_pa";
    inline constexpr const char* pressLage      = "pr_press_lage";
    inline constexpr const char* pressDriv      = "pr_press_driv";
    inline constexpr const char* pressTon       = "pr_press_ton";
    inline constexpr const char* pressTyngd     = "pr_press_tyngd";
    inline constexpr const char* pressNiva      = "pr_press_niva";
    inline constexpr const char* pressOver4     = "pr_press_over4";
    // Plåten
    inline constexpr const char* platPa         = "pr_plat_pa";
    inline constexpr const char* platMaterial   = "pr_plat_material";
    inline constexpr const char* platStamning   = "pr_plat_stamning";
    inline constexpr const char* platKlang      = "pr_plat_klang";
    inline constexpr const char* platLjushet    = "pr_plat_ljushet";
    inline constexpr const char* platBlandning  = "pr_plat_blandning";
    inline constexpr const char* platFore       = "pr_plat_fore";
    inline constexpr const char* platMidi       = "pr_plat_midi";
    // Megafonen
    inline constexpr const char* megaPa         = "pr_mega_pa";
    inline constexpr const char* megaLage       = "pr_mega_lage";
    inline constexpr const char* megaBand       = "pr_mega_band";
    inline constexpr const char* megaResonans   = "pr_mega_resonans";
    inline constexpr const char* megaSmuts      = "pr_mega_smuts";
    inline constexpr const char* megaBlandning  = "pr_mega_blandning";
    inline constexpr const char* megaBrus       = "pr_mega_brus";
    // Schaktet
    inline constexpr const char* schaktPa       = "pr_schakt_pa";
    inline constexpr const char* schaktTid      = "pr_schakt_tid";
    inline constexpr const char* schaktAterk    = "pr_schakt_aterkoppling";
    inline constexpr const char* schaktMorker   = "pr_schakt_morker";
    inline constexpr const char* schaktSvaj     = "pr_schakt_svaj";
    inline constexpr const char* schaktBlandning= "pr_schakt_blandning";
    inline constexpr const char* schaktPingpong = "pr_schakt_pingpong";
    // Hallen
    inline constexpr const char* hallPa         = "pr_hall_pa";
    inline constexpr const char* hallLokal      = "pr_hall_lokal";
    inline constexpr const char* hallStorlek    = "pr_hall_storlek";
    inline constexpr const char* hallHalltid    = "pr_hall_halltid";
    inline constexpr const char* hallFall       = "pr_hall_fall";
    inline constexpr const char* hallBlandning  = "pr_hall_blandning";
    inline constexpr const char* hallGrind      = "pr_hall_grind";
    // Trycket och utgången
    inline constexpr const char* trycket        = "pr_trycket";
    inline constexpr const char* inNiva         = "pr_ut_in";
    inline constexpr const char* torrVat        = "pr_ut_torrvat";
    inline constexpr const char* utNiva         = "pr_ut_niva";
    inline constexpr const char* limiter        = "pr_ut_limiter";
    inline constexpr const char* nodstopp       = "pr_nodstopp";
    inline constexpr const char* midiIn         = "pr_midi";      // MIDI in av/på för hela Pannrummet
}

namespace lagen
{
    inline juce::StringArray grind()    { return { "Mjuk", svt ("Hård"), "Takt" }; }
    inline juce::StringArray press()    { return { svt ("Rör"), "Transistor", "Likriktare", "Bitkross" }; }
    inline juce::StringArray material() { return { svt ("Plåt"), svt ("Rör"), svt ("Fjäder"), "Klocka", "Ring" }; }
    inline juce::StringArray mega()     { return { "Megafon", "Telefon", "Radio", "Tratt" }; }
    inline juce::StringArray tid()      { return { "1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4.", "1/2" }; }
    inline juce::StringArray lokal()    { return { "Verkstad", "Hall", "Valv", "Gruva" }; }
    // Längd i fjärdedelar för Schaktets tider.
    inline float tidFjardedelar (int i)
    {
        static const float t[] { 0.25f, 1.0f / 3.0f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f };
        return t[juce::jlimit (0, 6, i)];
    }
}

inline juce::String tonnamn (int ton)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (n[((ton % 12) + 12) % 12]) + juce::String (ton / 12 - 1);
}

inline juce::AudioProcessorValueTreeState::ParameterLayout skapaLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    int version = 1;
    auto pid  = [&] (const juce::String& s) { return ParameterID (s, version); };
    auto flyt = [&] (const juce::String& s, const char* namn, float lo, float hi, float std, const char* enhet = "", float mitt = -1.0f)
    {
        NormalisableRange<float> r (lo, hi);
        if (mitt > lo && mitt < hi) r.setSkewForCentre (mitt);
        l.add (std::make_unique<AudioParameterFloat> (pid (s), svt (namn), r, std, AudioParameterFloatAttributes().withLabel (enhet)));
    };
    auto val  = [&] (const juce::String& s, const char* namn, const StringArray& lista, int std)
    {
        l.add (std::make_unique<AudioParameterChoice> (pid (s), svt (namn), lista, std));
    };
    auto bry  = [&] (const juce::String& s, const char* namn, bool std)
    {
        l.add (std::make_unique<AudioParameterBool> (pid (s), svt (namn), std));
    };

    // Grinden
    bry  (id::grindPa, "Grinden på", false);
    val  (id::grindLage, "Grinden läge", lagen::grind(), 1);
    flyt (id::grindTroskel, "Grinden tröskel", -80.0f, 0.0f, -36.0f, "dB");
    flyt (id::grindHall, "Grinden hålltid", 1.0f, 1000.0f, 40.0f, "ms", 80.0f);
    flyt (id::grindSlapp, "Grinden släpp", 1.0f, 1000.0f, 25.0f, "ms", 80.0f);
    flyt (id::grindDjup, "Grinden djup", 0.0f, 1.0f, 1.0f);
    bry  (id::grindSidokedja, "Grinden sidokedja", false);
    for (int i = 0; i < 16; ++i)
        bry (id::grindSteg (i), ("Grinden steg " + juce::String (i + 1)).toRawUTF8(), i % 2 == 0);
    // Pressen
    bry  (id::pressPa, "Pressen på", false);
    val  (id::pressLage, "Pressen läge", lagen::press(), 0);
    flyt (id::pressDriv, "Pressen driv", 0.0f, 1.0f, 0.4f);
    flyt (id::pressTon, "Pressen ton", 0.0f, 1.0f, 0.5f);
    flyt (id::pressTyngd, "Pressen tyngd", 0.0f, 1.0f, 0.0f);
    flyt (id::pressNiva, "Pressen nivå", -24.0f, 12.0f, 0.0f, "dB");
    bry  (id::pressOver4, "Pressen översampling 4×", false);
    // Plåten
    bry  (id::platPa, "Plåten på", false);
    val  (id::platMaterial, "Plåten material", lagen::material(), 0);
    l.add (std::make_unique<AudioParameterInt> (pid (id::platStamning), svt ("Plåten stämning"), 24, 84, 45, AudioParameterIntAttributes()
              .withStringFromValueFunction ([] (int v, int) { return tonnamn (v); })));
    flyt (id::platKlang, "Plåten klang", 0.05f, 8.0f, 1.2f, "s", 1.0f);
    flyt (id::platLjushet, "Plåten ljushet", 0.0f, 1.0f, 0.5f);
    flyt (id::platBlandning, "Plåten blandning", 0.0f, 1.0f, 0.3f);
    bry  (id::platFore, "Plåten före Pressen", false);
    bry  (id::platMidi, "Plåten följer MIDI", true);
    // Megafonen
    bry  (id::megaPa, "Megafonen på", false);
    val  (id::megaLage, "Megafonen läge", lagen::mega(), 0);
    flyt (id::megaBand, "Megafonen band", 200.0f, 5000.0f, 1500.0f, "Hz", 1200.0f);
    flyt (id::megaResonans, "Megafonen resonans", 0.0f, 1.0f, 0.4f);
    flyt (id::megaSmuts, "Megafonen smuts", 0.0f, 1.0f, 0.4f);
    flyt (id::megaBlandning, "Megafonen blandning", 0.0f, 1.0f, 1.0f);
    bry  (id::megaBrus, "Megafonen brus", false);
    // Schaktet
    bry  (id::schaktPa, "Schaktet på", false);
    val  (id::schaktTid, "Schaktet tid", lagen::tid(), 3);
    flyt (id::schaktAterk, "Schaktet återkoppling", 0.0f, 95.0f, 45.0f, "%");
    flyt (id::schaktMorker, "Schaktet mörker", 0.0f, 1.0f, 0.5f);
    flyt (id::schaktSvaj, "Schaktet svaj", 0.0f, 1.0f, 0.2f);
    flyt (id::schaktBlandning, "Schaktet blandning", 0.0f, 1.0f, 0.3f);
    bry  (id::schaktPingpong, "Schaktet ping-pong", true);
    // Hallen
    bry  (id::hallPa, "Hallen på", false);
    val  (id::hallLokal, "Hallen lokal", lagen::lokal(), 1);
    flyt (id::hallStorlek, "Hallen storlek", 0.0f, 1.0f, 0.7f);
    flyt (id::hallHalltid, "Hallen hålltid", 10.0f, 1000.0f, 180.0f, "ms", 200.0f);
    flyt (id::hallFall, "Hallen fall", 5.0f, 2000.0f, 30.0f, "ms", 150.0f);
    flyt (id::hallBlandning, "Hallen blandning", 0.0f, 1.0f, 0.35f);
    bry  (id::hallGrind, "Hallen grind", true);
    // Trycket och utgången
    flyt (id::trycket, "Trycket", 0.0f, 1.0f, 0.0f);
    flyt (id::inNiva, "In-nivå", -24.0f, 24.0f, 0.0f, "dB");
    flyt (id::torrVat, "Torr/våt", 0.0f, 1.0f, 1.0f);
    flyt (id::utNiva, "Utnivå", -24.0f, 12.0f, 0.0f, "dB");
    bry  (id::limiter, "Limiter", true);
    bry  (id::nodstopp, "Nödstopp", false);
    // Tillagd efter fas 2 – ligger sist så att ordningen (och Reaper-automationen) står kvar.
    // Av från början: Pannrummet bryr sig inte om MIDI förrän man slår på det.
    bry  (id::midiIn, "MIDI in", false);
    return l;
}

} // namespace pr
