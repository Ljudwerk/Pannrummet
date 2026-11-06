// Paletten för fabrikspanelen: hammarlackad plåt, aluminiumskyltar, bakelit, benvita tavlor,
// signalgult och signalrött. Inga neonfärger – allt ska se ut att ha suttit i en fabrikshall i trettio år.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>

namespace pr::stil
{
// All text i koden är UTF-8 (ÅÄÖ, ▶ ◀ −); juce::String (const char*) tar bara ASCII.
inline juce::String sv (const char* utf8) { return juce::String::fromUTF8 (utf8); }
}

namespace pr::stil::farg
{
    // Chassit
    inline const juce::Colour lack         { 0xff2d3a32 };   // hammarlack, mörkt maskingrönt
    inline const juce::Colour lackModul    { 0xff34433a };   // modulernas täckplåtar, en nyans ljusare
    inline const juce::Colour fog          { 0xff0b0f0c };   // springan mellan plåtarna

    // Skyltar och tryck
    inline const juce::Colour aluminium    { 0xffaeb2ad };   // borstad aluminiumskylt
    inline const juce::Colour gravyr       { 0xff16181a };   // svart ifylld gravyr
    inline const juce::Colour tryckVitt    { 0xffe7e2d0 };   // vitt silkscreentryck på lacken
    inline const juce::Colour tryckMatt    { 0xff9aa79c };   // sekundärt tryck, skalstreck
    inline const juce::Colour benvitt      { 0xffe9e1c9 };   // instrumenttavlor
    inline const juce::Colour signalGul    { 0xffe8ad12 };
    inline const juce::Colour signalRod    { 0xffc4171d };
    inline const juce::Colour varde        { 0xffefc24a };   // värdetext under rattarna

    // Reglage
    inline const juce::Colour bakelit      { 0xff141210 };
    inline const juce::Colour bakelitGlans { 0xff4a4440 };
    inline const juce::Colour ror          { 0xffb9b4a4 };   // signalröret (zinkat stål)

    // Signallampor
    inline const juce::Colour barnsten     { 0xffffa21f };
    inline const juce::Colour gron         { 0xff55f26a };
    inline const juce::Colour rod          { 0xffff3a24 };

    // Dymo-remsan
    inline const juce::Colour dymo         { 0xffa8141c };
}
