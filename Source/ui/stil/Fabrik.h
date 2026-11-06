// Fabrikspanelens hårdvara: plåten, skyltarna och reglagen, ritade som fria funktioner.
//
// Varje funktion ritar ett reglage i ett givet läge och känner inte till någon parameter. Fas 3 lägger
// tunna juce::Component-omslag runt dem; stilprovet anropar dem direkt. Ljuset faller uppifrån vänster.
// Allt mäts i designpixlar (1200 × 760-panelen).
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_graphics/juce_graphics.h>
#include <krang/stil/Linjeskrift.h>
#include <krang/stil/Textur.h>
#include <cmath>
#include <vector>
#include "Palett.h"

namespace pr::stil
{

using juce::Colour;
using juce::Point;
using juce::Rectangle;
namespace ls = krang::stil::linjeskrift;
constexpr float pi = juce::MathConstants<float>::pi;

inline Point<float> polar (Point<float> c, float vinkel, float radie)   // vinkel 0 = uppåt, medurs
{
    return c + Point<float> (std::sin (vinkel), -std::cos (vinkel)) * radie;
}

// ---------------------------------------------------------------------------------------------
// Texturer

// Hammarlack: små runda gropar i lacken (cellbrus), belysta uppifrån vänster, och ett fint korn.
// Skapas i fysiska pixlar som texturerna i krang-libs; samma frö ger samma plåt.
inline juce::Image hammarlack (int w, int h, float skala, Colour bas, float styrka, uint32_t fro)
{
    const int pw = juce::jmax (1, (int) std::ceil (w * skala)), ph = juce::jmax (1, (int) std::ceil (h * skala));
    const float cell = 5.5f * skala;
    const int gx = (int) ((float) pw / cell) + 2, gy = (int) ((float) ph / cell) + 2;
    krang::stil::Slump r (fro);
    std::vector<Point<float>> pkt ((size_t) (gx * gy));
    for (int iy = 0; iy < gy; ++iy)
        for (int ix = 0; ix < gx; ++ix)
            pkt[(size_t) (iy * gx + ix)] = { ((float) ix + 0.1f + 0.8f * r.enhet()) * cell,
                                             ((float) iy + 0.1f + 0.8f * r.enhet()) * cell };
    std::vector<float> hf ((size_t) pw * (size_t) ph);
    for (int y = 0; y < ph; ++y)
    {
        const int iy = (int) ((float) y / cell);
        for (int x = 0; x < pw; ++x)
        {
            const int ix = (int) ((float) x / cell);
            float f1 = 1.0e9f;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                {
                    const int jx = ix + dx, jy = iy + dy;
                    if (jx < 0 || jy < 0 || jx >= gx || jy >= gy) continue;
                    const auto p = pkt[(size_t) (jy * gx + jx)];
                    const float ax = p.x - (float) x, ay = p.y - (float) y;
                    f1 = juce::jmin (f1, ax * ax + ay * ay);
                }
            const float t = juce::jlimit (0.0f, 1.0f, std::sqrt (f1) / (cell * 0.72f));
            hf[(size_t) y * (size_t) pw + (size_t) x] = t * t * (3.0f - 2.0f * t);
        }
    }
    juce::Image bild (juce::Image::RGB, pw, ph, false);
    juce::Image::BitmapData bd (bild, juce::Image::BitmapData::writeOnly);
    const float br = bas.getFloatRed() * 255.0f, bgr = bas.getFloatGreen() * 255.0f, bb = bas.getFloatBlue() * 255.0f;
    auto H = [&] (int x, int y) { x = juce::jlimit (0, pw - 1, x); y = juce::jlimit (0, ph - 1, y); return hf[(size_t) y * (size_t) pw + (size_t) x]; };
    for (int y = 0; y < ph; ++y)
    {
        // Långsamma fläckar: lacken är aldrig helt jämn.
        const float flack = 2.2f * std::sin ((float) y * 0.011f / skala + 1.3f);
        for (int x = 0; x < pw; ++x)
        {
            const float dx = (H (x + 1, y) - H (x - 1, y)) * 0.5f * cell;
            const float dy = (H (x, y + 1) - H (x, y - 1)) * 0.5f * cell;
            const float ljus = (dx + dy) * styrka + flack * std::sin ((float) x * 0.007f / skala) + r.bipolar() * 1.6f;
            bd.setPixelColour (x, y, Colour (krang::stil::tillByte (br + ljus), krang::stil::tillByte (bgr + ljus * 1.05f),
                                             krang::stil::tillByte (bb + ljus * 0.95f)));
        }
    }
    return bild;
}

// Panelens ytor, skapade en gång per skalfaktor och delade av alla reglage.
struct Ytor
{
    juce::Image lack, modul, alu;
    float skala = 0.0f;

    void forbered (int w, int h, float s)
    {
        if (std::abs (s - skala) < 0.001f && lack.isValid()) return;
        skala = s;
        lack  = hammarlack (w, h, s, farg::lack, 5.0f, 89u);
        modul = hammarlack (w, h, s, farg::lackModul, 5.5f, 1989u);
        alu   = krang::stil::borstadMetall (w, h, s, farg::aluminium, 7.5f, 242u);
    }
    juce::FillType fyll (const juce::Image& i) const { return juce::FillType (i, juce::AffineTransform::scale (1.0f / skala)); }
};

// ---------------------------------------------------------------------------------------------
// Tryck

// Vitt silkscreentryck på lacken.
inline void tryck (juce::Graphics& g, const juce::String& text, Rectangle<float> ruta, float hojd = 6.0f,
                   juce::Justification just = juce::Justification::centred, Colour f = farg::tryckVitt,
                   float tjocklek = 0.85f, float spar = 2.0f)
{
    g.setColour (f);
    ls::rita (g, text, ruta, { hojd, tjocklek, spar, 1.0f }, just);
}

// Svart ifylld gravyr i aluminium: kanalen fångar ljuset i underkanten.
inline void gravyr (juce::Graphics& g, const juce::String& text, Rectangle<float> ruta, float hojd = 7.0f,
                    juce::Justification just = juce::Justification::centredLeft, float tjocklek = 1.1f, float spar = 2.0f,
                    Colour f = farg::gravyr)
{
    const ls::Stil s { hojd, tjocklek, spar, 1.0f };
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    ls::rita (g, text, ruta.translated (0.0f, 0.7f), s, just);
    g.setColour (f);
    ls::rita (g, text, ruta, s, just);
}

// Text längs en cirkelbåge (nödstoppsskylten). ovansida = läses över mitten, annars under.
inline void tryckPaBage (juce::Graphics& g, const juce::String& text, Point<float> c, float radie, bool ovansida,
                         const ls::Stil& s, Colour f)
{
    const float enhet = s.hojd / 8.0f;
    std::vector<float> bredder;
    float total = 0.0f;
    for (auto p = text.getCharPointer(); ! p.isEmpty();)
    {
        const auto ch = p.getAndAdvance();
        const auto* gl = ls::glyf (ch);
        const float b = ((gl != nullptr ? gl->bredd : 4.0f) * s.smal + s.spar) * enhet;
        bredder.push_back (b);
        total += b;
    }
    total -= s.spar * enhet;
    const float spann = total / radie;
    float a = ovansida ? -spann * 0.5f : pi + spann * 0.5f;
    g.setColour (f);
    int i = 0;
    for (auto p = text.getCharPointer(); ! p.isEmpty(); ++i)
    {
        const auto ch = p.getAndAdvance();
        const float b = bredder[(size_t) i];
        const float bGlyf = b - s.spar * enhet;
        const float mitt = ovansida ? a + (bGlyf * 0.5f) / radie : a - (bGlyf * 0.5f) / radie;
        auto bana = ls::bana (juce::String::charToString (ch), -bGlyf * 0.5f, ovansida ? -s.hojd : 0.0f, s);
        const auto t = ovansida ? juce::AffineTransform::translation (0.0f, -radie).rotated (mitt).translated (c)
                                : juce::AffineTransform::translation (0.0f, radie).rotated (mitt - pi).translated (c);
        bana.applyTransform (t);
        g.strokePath (bana, juce::PathStrokeType (s.tjocklek, juce::PathStrokeType::mitered, juce::PathStrokeType::square));
        a = ovansida ? a + b / radie : a - b / radie;
    }
}

// ---------------------------------------------------------------------------------------------
// Plåt och fästelement

// Spårskruv med försänkt huvud.
inline void ritaSkruv (juce::Graphics& g, Point<float> c, float r, float vinkel = 0.6f)
{
    const auto ruta = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillEllipse (ruta.expanded (1.0f).translated (0.3f, 0.6f));
    g.setGradientFill ({ Colour (0xff9a9c94), c.x - r, c.y - r, Colour (0xff2c2e2a), c.x + r * 0.8f, c.y + r, false });
    g.fillEllipse (ruta);
    const Point<float> u (std::cos (vinkel), std::sin (vinkel));
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawLine ({ c - u * (r * 0.8f), c + u * (r * 0.8f) }, r * 0.32f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawLine ({ c - u * (r * 0.8f) + Point<float> (0.0f, r * 0.2f), c + u * (r * 0.8f) + Point<float> (0.0f, r * 0.2f) }, 0.5f);
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawEllipse (ruta.reduced (0.3f), 0.5f);
}

// Kupad nit (skyltarnas hörn).
inline void ritaNit (juce::Graphics& g, Point<float> c, float r)
{
    const auto ruta = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (ruta.translated (0.4f, 0.7f).expanded (0.4f));
    juce::ColourGradient k (Colour (0xffeeeee8), c.translated (-r * 0.4f, -r * 0.45f), Colour (0xff55575a), c.translated (r, r), true);
    k.addColour (0.5, Colour (0xff9fa29e));
    g.setGradientFill (k);
    g.fillEllipse (ruta);
}

// En plåt i hammarlack med fog runt om och fyra skruvar (modulernas täckplåtar).
inline void ritaTackplat (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor, bool skruvar = true)
{
    g.setColour (farg::fog);
    g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
    juce::Path form;
    form.addRoundedRectangle (r, 2.0f);
    g.setFillType (ytor.fyll (ytor.modul));
    g.fillPath (form);
    // Fasen: ljus överkant och vänsterkant, mörk underkant och högerkant.
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawLine (r.getX() + 2.0f, r.getY() + 0.6f, r.getRight() - 2.0f, r.getY() + 0.6f, 1.2f);
    g.drawLine (r.getX() + 0.6f, r.getY() + 2.0f, r.getX() + 0.6f, r.getBottom() - 2.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.drawLine (r.getX() + 2.0f, r.getBottom() - 0.6f, r.getRight() - 2.0f, r.getBottom() - 0.6f, 1.2f);
    g.drawLine (r.getRight() - 0.6f, r.getY() + 2.0f, r.getRight() - 0.6f, r.getBottom() - 2.0f, 1.0f);
    if (skruvar)
    {
        const float i = 8.0f;
        ritaSkruv (g, { r.getX() + i, r.getY() + i }, 3.4f, 0.3f);
        ritaSkruv (g, { r.getRight() - i, r.getY() + i }, 3.4f, 1.4f);
        ritaSkruv (g, { r.getX() + i, r.getBottom() - i }, 3.4f, 2.2f);
        ritaSkruv (g, { r.getRight() - i, r.getBottom() - i }, 3.4f, 0.9f);
    }
}

// Borstad aluminiumskylt med fasad kant och nitar i hörnen.
inline void ritaSkylt (juce::Graphics& g, Rectangle<float> r, const Ytor& ytor, bool nitar = true, float radie = 2.5f)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.8f, 1.4f).expanded (0.6f), radie);
    juce::Path form;
    form.addRoundedRectangle (r, radie);
    g.setFillType (ytor.fyll (ytor.alu));
    g.fillPath (form);
    {
        juce::Graphics::ScopedSaveState s (g);
        g.reduceClipRegion (form);
        g.setGradientFill ({ juce::Colours::white.withAlpha (0.22f), r.getX(), r.getY(),
                             juce::Colours::black.withAlpha (0.18f), r.getX() + r.getWidth() * 0.35f, r.getBottom(), false });
        g.fillRect (r);
    }
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), radie, 0.8f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawLine (r.getX() + radie, r.getBottom() - 0.4f, r.getRight() - radie, r.getBottom() - 0.4f, 0.8f);
    if (nitar)
    {
        const float i = juce::jmin (6.0f, r.getHeight() * 0.22f), nr = juce::jmin (2.4f, r.getHeight() * 0.08f);
        ritaNit (g, { r.getX() + i, r.getY() + i }, nr);
        ritaNit (g, { r.getRight() - i, r.getY() + i }, nr);
        ritaNit (g, { r.getX() + i, r.getBottom() - i }, nr);
        ritaNit (g, { r.getRight() - i, r.getBottom() - i }, nr);
    }
}

// Präglade versaler i skylten (modellnamnet): upphöjd metall med ljus vänster överkant och skugga.
inline void ritaPraglat (juce::Graphics& g, const juce::String& text, Rectangle<float> ruta, const ls::Stil& s, const Ytor& ytor,
                         juce::Justification just = juce::Justification::centredLeft)
{
    const float b = ls::bredd (text, s);
    float x = ruta.getX();
    if (just.testFlags (juce::Justification::horizontallyCentred)) x = ruta.getCentreX() - b * 0.5f;
    const float y = ruta.getCentreY() - s.hojd * 0.5f;
    const auto bana = ls::bana (text, x, y, s);
    juce::Path fylld;
    juce::PathStrokeType (s.tjocklek, juce::PathStrokeType::mitered, juce::PathStrokeType::square).createStrokedPath (fylld, bana);
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillPath (fylld, juce::AffineTransform::translation (1.4f, 1.8f));
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.fillPath (fylld, juce::AffineTransform::translation (-0.8f, -0.9f));
    g.setFillType (ytor.fyll (ytor.alu));
    g.fillPath (fylld);
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.18f), 0.0f, y, juce::Colours::black.withAlpha (0.22f), 0.0f, y + s.hojd, false });
    g.fillPath (fylld);
}

// Gul-svart varningsrand.
inline void ritaVarningsrand (juce::Graphics& g, Rectangle<float> r, float bredd = 9.0f)
{
    juce::Graphics::ScopedSaveState s (g);
    g.reduceClipRegion (r.toNearestInt());
    g.setColour (farg::signalGul);
    g.fillRect (r);
    g.setColour (Colour (0xff121212));
    const float h = r.getHeight();
    for (float x = r.getX() - h; x < r.getRight() + h; x += bredd * 2.0f)
    {
        juce::Path p;
        p.addQuadrilateral (x, r.getBottom(), x + bredd, r.getBottom(), x + bredd + h, r.getY(), x + h, r.getY());
        g.fillPath (p);
    }
    // Slitage: lacken har nötts av längs kanterna.
    krang::stil::Slump sl ((uint32_t) (r.getX() * 13.0f + r.getY() * 7.0f) + 5u);
    for (int i = 0; i < (int) (r.getWidth() * r.getHeight() / 60.0f); ++i)
    {
        g.setColour (farg::lack.darker (0.3f).withAlpha (0.35f + 0.4f * sl.enhet()));
        const float px = r.getX() + sl.enhet() * r.getWidth(), py = r.getY() + sl.enhet() * r.getHeight();
        g.fillEllipse (px, py, 0.6f + sl.enhet() * 1.8f, 0.5f + sl.enhet() * 1.2f);
    }
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.15f), r.getX(), r.getY(), juce::Colours::black.withAlpha (0.2f), r.getX(), r.getBottom(), false });
    g.fillRect (r);
}

// ---------------------------------------------------------------------------------------------
// Reglage

// Skala runt en ratt: streck och siffror 0–10, från −135° till +135°.
inline void ritaRattskala (juce::Graphics& g, Point<float> c, float r, bool siffror = true)
{
    const float a0 = -pi * 0.75f, a1 = pi * 0.75f;
    for (int i = 0; i <= 20; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 20.0f;
        const bool stort = i % 2 == 0;
        g.setColour (stort ? farg::tryckVitt : farg::tryckMatt);
        g.drawLine ({ polar (c, a, r * 1.14f), polar (c, a, r * (stort ? 1.32f : 1.24f)) }, stort ? 1.0f : 0.7f);
    }
    if (siffror)
    {
        const float hs = juce::jlimit (4.5f, 7.0f, r * 0.28f);
        for (int n = 0; n <= 10; n += 2)
        {
            const float a = a0 + (a1 - a0) * (float) n / 10.0f;
            const auto p = polar (c, a, r * 1.32f + hs * 1.15f);
            tryck (g, juce::String (n), Rectangle<float> (24.0f, hs + 2.0f).withCentre (p), hs, juce::Justification::centred,
                   farg::tryckVitt, 0.75f, 1.4f);
        }
    }
}

// Bakelitratt: svart, blank, räfflad kant och vit visarlinje. varde 0..1 (−135° … +135°).
inline void ritaBakelitRatt (juce::Graphics& g, Point<float> c, float r, float varde, bool skala = true,
                             bool siffror = true, Colour visare = farg::tryckVitt)
{
    if (skala) ritaRattskala (g, c, r, siffror);
    // Skugga.
    {
        const auto sc = c.translated (r * 0.15f, r * 0.32f);
        juce::ColourGradient sk (juce::Colours::black.withAlpha (0.7f), sc, juce::Colours::black.withAlpha (0.0f), sc.translated (r * 1.25f, 0.0f), true);
        g.setGradientFill (sk);
        g.fillEllipse (Rectangle<float> (r * 2.5f, r * 2.5f).withCentre (sc));
    }
    // Kjolen.
    const auto kjol = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setGradientFill ({ Colour (0xff2c2825), c.x - r, c.y - r, Colour (0xff050404), c.x + r * 0.6f, c.y + r, false });
    g.fillEllipse (kjol);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (kjol.reduced (0.4f), 0.7f);

    // Den räfflade toppen.
    const int raffor = 14;
    const float rk = r * 0.80f;
    juce::Path topp;
    for (int i = 0; i <= 140; ++i)
    {
        const float a = 2.0f * pi * (float) i / 140.0f;
        const float rr = rk * (1.0f - 0.07f * (0.5f + 0.5f * std::cos (a * (float) raffor)));
        const auto p = polar (c, a, rr);
        if (i == 0) topp.startNewSubPath (p); else topp.lineTo (p);
    }
    topp.closeSubPath();
    g.setColour (juce::Colours::black);
    g.strokePath (topp, juce::PathStrokeType (1.2f));
    g.setGradientFill ({ farg::bakelitGlans, c.x - rk, c.y - rk, farg::bakelit, c.x + rk * 0.5f, c.y + rk * 0.7f, false });
    g.fillPath (topp);
    // Räfflornas ljus: de som vetter uppåt vänster är ljusare.
    for (int i = 0; i < raffor; ++i)
    {
        const float a = 2.0f * pi * ((float) i + 0.5f) / (float) raffor;
        const float l = 0.5f + 0.5f * std::cos (a + pi * 0.25f);
        g.setColour (juce::Colours::white.withAlpha (0.05f + 0.16f * l * l));
        g.drawLine ({ polar (c, a, rk * 0.80f), polar (c, a, rk * 0.95f) }, juce::jmax (0.6f, r * 0.06f));
    }
    // Kupan: blank bakelit med en mjuk reflex.
    const float rd = rk * 0.74f;
    const auto kupa = Rectangle<float> (rd * 2.0f, rd * 2.0f).withCentre (c);
    {
        juce::ColourGradient kg (Colour (0xff3d3834), c.translated (-rd * 0.45f, -rd * 0.55f), Colour (0xff080707), c.translated (rd * 0.8f, rd * 0.9f), true);
        g.setGradientFill (kg);
        g.fillEllipse (kupa);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillEllipse (Rectangle<float> (rd * 0.9f, rd * 0.42f).withCentre (c.translated (-rd * 0.28f, -rd * 0.5f)));
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (kupa, 0.6f);
    }
    // Visarlinjen, ingraverad och vitfylld.
    const float a = -pi * 0.75f + pi * 1.5f * juce::jlimit (0.0f, 1.0f, varde);
    const float tj = juce::jmax (1.4f, r * 0.11f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine ({ polar (c, a, r * 0.18f), polar (c, a, r * 0.98f) }, tj + 1.0f);
    g.setColour (visare);
    g.drawLine ({ polar (c, a, r * 0.2f), polar (c, a, r * 0.96f) }, tj);
}

// Vridomkopplare med "kycklinghuvud": lägena tryckta runt om, spetsig ratt som pekar på valt läge.
inline void ritaVridomkopplare (juce::Graphics& g, Point<float> c, float r, int lage, const juce::StringArray& lagen,
                                float spann = -1.0f)
{
    const int n = juce::jmax (2, lagen.size());
    if (spann < 0.0f) spann = juce::jmin (pi * 1.4f, (float) (n - 1) * pi / 3.0f);
    auto vinkel = [&] (int i) { return -spann * 0.5f + spann * (float) i / (float) (n - 1); };
    const ls::Stil s { 5.5f, 0.8f, 1.5f, 1.0f };
    for (int i = 0; i < n; ++i)
    {
        const float a = vinkel (i);
        g.setColour (farg::tryckVitt);
        g.drawLine ({ polar (c, a, r * 1.16f), polar (c, a, r * 1.34f) }, 1.1f);
        const float b = ls::bredd (lagen[i], s);
        const float ut = r * 1.42f + 4.0f + 0.5f * b * std::abs (std::sin (a)) + 2.5f * std::abs (std::cos (a));
        const auto p = polar (c, a, ut);
        g.setColour (i == lage ? farg::tryckVitt : farg::tryckVitt.withAlpha (0.7f));
        ls::rita (g, lagen[i], Rectangle<float> (b + 4.0f, 8.0f).withCentre (p), s, juce::Justification::centred);
    }
    // Basplattan.
    {
        const auto sc = c.translated (r * 0.18f, r * 0.35f);
        juce::ColourGradient sk (juce::Colours::black.withAlpha (0.7f), sc, juce::Colours::black.withAlpha (0.0f), sc.translated (r * 1.2f, 0.0f), true);
        g.setGradientFill (sk);
        g.fillEllipse (Rectangle<float> (r * 2.4f, r * 2.4f).withCentre (sc));
    }
    const auto bas = Rectangle<float> (r * 1.5f, r * 1.5f).withCentre (c);
    g.setGradientFill ({ Colour (0xff2a2724), c.x - r, c.y - r, Colour (0xff060505), c.x + r, c.y + r, false });
    g.fillEllipse (bas);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawEllipse (bas.reduced (0.4f), 0.6f);

    // Kycklinghuvudet, ritat uppåt och vridet till läget.
    const float a = vinkel (juce::jlimit (0, n - 1, lage));
    juce::Path huvud;
    huvud.startNewSubPath (0.0f, -r * 1.12f);
    huvud.quadraticTo (r * 0.22f, -r * 0.62f, r * 0.36f, -r * 0.05f);
    huvud.quadraticTo (r * 0.40f, r * 0.55f, r * 0.20f, r * 0.80f);
    huvud.quadraticTo (0.0f, r * 0.92f, -r * 0.20f, r * 0.80f);
    huvud.quadraticTo (-r * 0.40f, r * 0.55f, -r * 0.36f, -r * 0.05f);
    huvud.quadraticTo (-r * 0.22f, -r * 0.62f, 0.0f, -r * 1.12f);
    huvud.closeSubPath();
    const auto t = juce::AffineTransform::rotation (a).translated (c);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillPath (huvud, t.translated (r * 0.12f, r * 0.22f));
    g.setGradientFill ({ farg::bakelitGlans.brighter (0.15f), c.x - r * 0.6f, c.y - r * 0.8f, farg::bakelit, c.x + r * 0.4f, c.y + r * 0.6f, false });
    g.fillPath (huvud, t);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.strokePath (huvud, juce::PathStrokeType (0.7f), t);
    // Ryggen och den vita visaren.
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawLine ({ polar (c, a - 0.25f, r * 0.15f), polar (c, a - 0.08f, r * 0.85f) }, r * 0.12f);
    g.setColour (farg::tryckVitt);
    g.drawLine ({ polar (c, a, r * 0.45f), polar (c, a, r * 1.04f) }, juce::jmax (1.3f, r * 0.09f));
}

// Vippströmbrytare: förkromad mutter och arm. pa = armen uppåt (lodrät) eller åt höger (vågrät).
inline void ritaVippa (juce::Graphics& g, Point<float> c, bool pa, bool lodrat = true, float storlek = 1.0f)
{
    const float rm = 8.0f * storlek;
    // Sexkantsmuttern.
    juce::Path mutter;
    for (int i = 0; i < 6; ++i)
    {
        const auto p = polar (c, pi / 6.0f + (float) i * pi / 3.0f, rm);
        if (i == 0) mutter.startNewSubPath (p); else mutter.lineTo (p);
    }
    mutter.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillPath (mutter, juce::AffineTransform::translation (1.0f, 1.6f));
    g.setGradientFill ({ Colour (0xffe8e8e4), c.x - rm, c.y - rm, Colour (0xff4b4d4c), c.x + rm, c.y + rm, false });
    g.fillPath (mutter);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.strokePath (mutter, juce::PathStrokeType (0.6f));
    // Bussningen.
    const float rb = rm * 0.58f;
    g.setGradientFill ({ Colour (0xff4a4c4b), c.x - rb, c.y - rb, Colour (0xffd8d9d4), c.x + rb, c.y + rb, false });
    g.fillEllipse (Rectangle<float> (rb * 2.0f, rb * 2.0f).withCentre (c));
    g.setColour (Colour (0xff0e0e0e));
    g.fillEllipse (Rectangle<float> (rb * 1.1f, rb * 1.1f).withCentre (c));

    // Armen.
    const Point<float> dir = lodrat ? Point<float> (0.0f, pa ? -1.0f : 1.0f) : Point<float> (pa ? 1.0f : -1.0f, 0.0f);
    const Point<float> n (-dir.y, dir.x);
    const float len = 17.0f * storlek;
    const auto spets = c + dir * len;
    juce::Path arm;
    arm.startNewSubPath (c + n * (1.6f * storlek));
    arm.lineTo (spets + n * (2.3f * storlek));
    arm.lineTo (spets - n * (2.3f * storlek));
    arm.lineTo (c - n * (1.6f * storlek));
    arm.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillPath (arm, juce::AffineTransform::translation (2.0f * storlek, 3.0f * storlek));
    g.setGradientFill ({ Colour (0xfff4f4f0), (c - n * 2.0f).x, (c - n * 2.0f).y, Colour (0xff5d5f5e), (c + n * 2.5f).x, (c + n * 2.5f).y, false });
    g.fillPath (arm);
    const float rk = 3.1f * storlek;
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillEllipse (Rectangle<float> (rk * 2.0f, rk * 2.0f).withCentre (spets.translated (1.6f * storlek, 2.4f * storlek)));
    juce::ColourGradient kula (juce::Colours::white, spets.translated (-rk * 0.4f, -rk * 0.45f), Colour (0xff4e504f), spets.translated (rk, rk), true);
    g.setGradientFill (kula);
    g.fillEllipse (Rectangle<float> (rk * 2.0f, rk * 2.0f).withCentre (spets));
}

// Signallampa: förkromad räfflad infattning och slipat glas. styrka 0 = släckt, 1 = full.
inline void ritaSignallampa (juce::Graphics& g, Point<float> c, float r, Colour f, float styrka)
{
    if (styrka > 0.01f)
    {
        juce::ColourGradient glod (f.withAlpha (0.38f * styrka), c, f.withAlpha (0.0f), c.translated (r * 4.0f, 0.0f), true);
        g.setGradientFill (glod);
        g.fillEllipse (Rectangle<float> (r * 8.0f, r * 8.0f).withCentre (c));
    }
    const float ri = r * 1.45f;
    const auto ring = Rectangle<float> (ri * 2.0f, ri * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (ring.translated (0.6f, 1.2f).expanded (0.6f));
    g.setGradientFill ({ Colour (0xfff0f0ec), c.x - ri, c.y - ri, Colour (0xff3e403f), c.x + ri * 0.7f, c.y + ri, false });
    g.fillEllipse (ring);
    for (int i = 0; i < 32; ++i)
    {
        const float a = 2.0f * pi * (float) i / 32.0f;
        g.setColour ((i % 2 == 0 ? juce::Colours::black : juce::Colours::white).withAlpha (0.16f));
        g.drawLine ({ polar (c, a, r * 1.12f), polar (c, a, ri * 0.98f) }, 0.6f);
    }
    g.setColour (juce::Colours::black);
    g.fillEllipse (Rectangle<float> (r * 2.2f, r * 2.2f).withCentre (c));

    // Glaset: fasetter i en krans runt en plan mitt.
    const auto lins = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    const Colour kropp = styrka > 0.01f ? f.interpolatedWith (f.darker (0.6f), 0.3f) : f.darker (2.6f).withMultipliedSaturation (0.7f);
    {
        juce::ColourGradient lg (styrka > 0.01f ? f.brighter (0.8f).interpolatedWith (juce::Colours::white, 0.4f) : f.darker (1.8f),
                                 c.translated (-r * 0.15f, -r * 0.2f), kropp.darker (0.5f), c.translated (r, r), true);
        lg.addColour (0.5, kropp);
        g.setGradientFill (lg);
        g.fillEllipse (lins);
    }
    const int fasetter = 10;
    for (int i = 0; i < fasetter; ++i)
    {
        const float a0 = 2.0f * pi * (float) i / (float) fasetter, a1 = a0 + 2.0f * pi / (float) fasetter;
        juce::Path fas;
        fas.startNewSubPath (polar (c, a0, r * 0.98f));
        fas.lineTo (polar (c, a1, r * 0.98f));
        fas.lineTo (polar (c, (a0 + a1) * 0.5f, r * 0.45f));
        fas.closeSubPath();
        const float l = 0.5f + 0.5f * std::cos ((a0 + a1) * 0.5f + pi * 0.25f);
        g.setColour ((l > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (0.10f + 0.14f * std::abs (l - 0.5f) * 2.0f));
        g.fillPath (fas);
    }
    g.setColour (juce::Colours::white.withAlpha (styrka > 0.01f ? 0.7f : 0.35f));
    g.fillEllipse (Rectangle<float> (r * 0.55f, r * 0.35f).withCentre (c.translated (-r * 0.35f, -r * 0.45f)));
}

// Rund tryckknapp: förkromad ring och svart kåpa. Ikonen (om någon) ritas vitt på kåpan.
inline void ritaTryckknapp (juce::Graphics& g, Point<float> c, float r, const juce::Path& ikon = {}, Colour kapa = Colour (0xff1b1a18))
{
    const float ri = r * 1.22f;
    const auto ring = Rectangle<float> (ri * 2.0f, ri * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (ring.translated (0.7f, 1.4f).expanded (0.5f));
    g.setGradientFill ({ Colour (0xffe6e6e2), c.x - ri, c.y - ri, Colour (0xff3a3c3b), c.x + ri * 0.6f, c.y + ri, false });
    g.fillEllipse (ring);
    g.setColour (juce::Colours::black);
    g.fillEllipse (Rectangle<float> (r * 2.08f, r * 2.08f).withCentre (c));
    const auto k = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setGradientFill ({ kapa.brighter (0.5f), c.x, c.y - r, kapa.darker (0.6f), c.x, c.y + r, false });
    g.fillEllipse (k);
    g.setGradientFill ({ kapa.darker (0.4f), c.x, c.y - r * 0.8f, kapa.brighter (0.25f), c.x, c.y + r * 0.8f, false });
    g.fillEllipse (k.reduced (r * 0.16f));
    if (! ikon.isEmpty())
    {
        g.setColour (farg::tryckVitt);
        g.fillPath (ikon);
    }
}

// En trekant som ikon (◀ ▶) i en ruta.
inline juce::Path trekant (Rectangle<float> r, bool hoger)
{
    juce::Path p;
    if (hoger) p.addTriangle (r.getX(), r.getY(), r.getRight(), r.getCentreY(), r.getX(), r.getBottom());
    else       p.addTriangle (r.getRight(), r.getY(), r.getX(), r.getCentreY(), r.getRight(), r.getBottom());
    return p;
}

// ---------------------------------------------------------------------------------------------
// Instrument

// Ljudnivå i dB → visarläge 0..1 på en VU-lik skala (−20 … +3 dB).
inline float vuLage (float dB)
{
    const float lo = std::pow (10.0f, -20.0f / 20.0f), hi = std::pow (10.0f, 3.0f / 20.0f);
    return juce::jlimit (0.0f, 1.0f, (std::pow (10.0f, dB / 20.0f) - lo) / (hi - lo));
}

// Visarinstrument i fyrkantig bakelitlåda med benvit tavla. dB −20 … +3.
inline void ritaVisarinstrument (juce::Graphics& g, Rectangle<float> r, float dB, const juce::String& etikett)
{
    // Lådan.
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (r.translated (1.0f, 2.0f).expanded (1.0f), 6.0f);
    g.setGradientFill ({ Colour (0xff2b2724), r.getX(), r.getY(), Colour (0xff070606), r.getRight(), r.getBottom(), false });
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawRoundedRectangle (r.reduced (0.6f), 6.0f, 0.8f);
    for (auto p : { r.getTopLeft().translated (6.0f, 6.0f), r.getTopRight().translated (-6.0f, 6.0f),
                    r.getBottomLeft().translated (6.0f, -6.0f), r.getBottomRight().translated (-6.0f, -6.0f) })
        ritaSkruv (g, p, 2.6f, p.x * 0.1f);

    // Tavlan.
    const auto tavla = r.reduced (13.0f, 11.0f);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (tavla.expanded (1.5f), 2.5f);
    g.setGradientFill ({ farg::benvitt.brighter (0.05f), tavla.getX(), tavla.getY(), farg::benvitt.darker (0.25f), tavla.getX(), tavla.getBottom(), false });
    g.fillRoundedRectangle (tavla, 2.0f);
    juce::Graphics::ScopedSaveState spar (g);
    g.reduceClipRegion (tavla.toNearestInt());

    const Point<float> nav (tavla.getCentreX(), tavla.getBottom() + tavla.getHeight() * 0.32f);
    const float radie = tavla.getHeight() * 1.0f;
    const float a0 = -0.78f, a1 = 0.78f;
    auto vinkel = [&] (float dBv) { return a0 + (a1 - a0) * vuLage (dBv); };
    const Colour svart (0xff1d1b18);
    // Röda fältet över 0 dB.
    {
        juce::Path rott;
        rott.addCentredArc (nav.x, nav.y, radie, radie, 0.0f, vinkel (0.0f), a1, true);
        g.setColour (farg::signalRod);
        g.strokePath (rott, juce::PathStrokeType (3.0f));
    }
    juce::Path bage;
    bage.addCentredArc (nav.x, nav.y, radie - 2.0f, radie - 2.0f, 0.0f, a0, a1, true);
    g.setColour (svart);
    g.strokePath (bage, juce::PathStrokeType (0.8f));
    for (float v : { -20.0f, -10.0f, -7.0f, -5.0f, -3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f })
    {
        const float a = vinkel (v);
        const int vi = (int) v;
        const bool stort = vi == -20 || vi == -10 || vi == -5 || vi == -3 || vi == 0 || vi == 3;
        g.setColour (v > 0.0f ? farg::signalRod : svart);
        g.drawLine ({ polar (nav, a, radie - 2.0f), polar (nav, a, radie + (stort ? 5.0f : 3.0f)) }, stort ? 1.0f : 0.7f);
        if (stort)
        {
            const juce::String t = v > 0.0f ? "+" + juce::String ((int) v) : (v < 0.0f ? sv ("−") + juce::String ((int) -v) : "0");
            g.setColour (v > 0.0f ? farg::signalRod : svart);
            ls::rita (g, t, Rectangle<float> (20.0f, 6.0f).withCentre (polar (nav, a, radie + 10.0f)), { 4.6f, 0.75f, 1.2f, 1.0f });
        }
    }
    g.setColour (svart);
    ls::rita (g, etikett, Rectangle<float> (tavla.getWidth(), 10.0f).withCentre ({ tavla.getCentreX(), tavla.getBottom() - 14.0f }), { 8.0f, 1.3f, 2.0f, 1.0f });
    ls::rita (g, "DB", Rectangle<float> (30.0f, 6.0f).withCentre ({ tavla.getRight() - 14.0f, tavla.getBottom() - 8.0f }), { 4.5f, 0.7f, 1.2f, 1.0f });

    // Visaren.
    const float a = vinkel (dB);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawLine ({ nav.translated (1.5f, 1.0f), polar (nav, a, radie + 6.0f).translated (1.5f, 1.0f) }, 1.2f);
    g.setColour (svart);
    g.drawLine ({ nav, polar (nav, a, radie + 6.0f) }, 1.1f);
    // Täcklisten över navet och glaset.
    const auto list = tavla.withTop (tavla.getBottom() - 6.0f);
    g.setGradientFill ({ Colour (0xff1f1c19), list.getX(), list.getY(), Colour (0xff050505), list.getX(), list.getBottom(), false });
    g.fillRect (list);
    g.setGradientFill ({ juce::Colours::white.withAlpha (0.28f), tavla.getX(), tavla.getY(),
                         juce::Colours::white.withAlpha (0.0f), tavla.getX() + tavla.getWidth() * 0.5f, tavla.getY() + tavla.getHeight() * 0.6f, false });
    juce::Path glans;
    glans.addQuadrilateral (tavla.getX(), tavla.getY(), tavla.getX() + tavla.getWidth() * 0.55f, tavla.getY(),
                            tavla.getX() + tavla.getWidth() * 0.25f, tavla.getBottom(), tavla.getX(), tavla.getBottom());
    g.fillPath (glans);
}

// Manometer: förkromad ring, benvit tavla 0–10 bar med rött fält, svart visare. varde 0..1.
inline void ritaManometer (juce::Graphics& g, Point<float> c, float r, float varde, const juce::String& etikett)
{
    const auto yttre = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (yttre.translated (1.5f, 3.0f).expanded (1.0f));
    g.setGradientFill ({ Colour (0xfff4f4f0), c.x - r, c.y - r, Colour (0xff35373a), c.x + r * 0.7f, c.y + r, false });
    g.fillEllipse (yttre);
    g.setGradientFill ({ Colour (0xff3a3c3e), c.x - r, c.y - r, Colour (0xffeeeeea), c.x + r * 0.7f, c.y + r, false });
    g.fillEllipse (yttre.reduced (r * 0.07f));
    const float rt = r * 0.84f;
    const auto tavla = Rectangle<float> (rt * 2.0f, rt * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black);
    g.fillEllipse (tavla.expanded (0.8f));
    {
        juce::ColourGradient tg (farg::benvitt.brighter (0.06f), c.translated (-rt * 0.3f, -rt * 0.4f), farg::benvitt.darker (0.3f), c.translated (rt, rt), true);
        g.setGradientFill (tg);
        g.fillEllipse (tavla);
    }
    const float a0 = -pi * 0.75f, a1 = pi * 0.75f;
    const Colour svart (0xff1d1b18);
    {
        juce::Path rott;
        rott.addCentredArc (c.x, c.y, rt * 0.80f, rt * 0.80f, 0.0f, a0 + (a1 - a0) * 0.8f, a1, true);
        g.setColour (farg::signalRod);
        g.strokePath (rott, juce::PathStrokeType (rt * 0.09f));
    }
    for (int i = 0; i <= 50; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 50.0f;
        const bool stort = i % 5 == 0;
        g.setColour (i > 40 ? farg::signalRod.darker (0.2f) : svart);
        g.drawLine ({ polar (c, a, rt * (stort ? 0.72f : 0.78f)), polar (c, a, rt * 0.86f) }, stort ? 1.1f : 0.6f);
        if (stort && i % 10 == 0)
            ls::rita (g, juce::String (i / 5), Rectangle<float> (20.0f, 8.0f).withCentre (polar (c, a, rt * 0.58f)), { rt * 0.12f, 1.0f, 1.2f, 1.0f });
    }
    g.setColour (svart);
    ls::rita (g, etikett, Rectangle<float> (rt * 1.6f, 8.0f).withCentre (c.translated (0.0f, -rt * 0.30f)), { rt * 0.10f, 0.9f, 1.8f, 1.0f });
    ls::rita (g, "BAR", Rectangle<float> (rt, 6.0f).withCentre (c.translated (0.0f, rt * 0.32f)), { rt * 0.08f, 0.7f, 1.5f, 1.0f });
    // Visaren med motvikt.
    const float a = a0 + (a1 - a0) * juce::jlimit (0.0f, 1.0f, varde);
    juce::Path visare;
    visare.startNewSubPath (0.0f, -rt * 0.84f);
    visare.lineTo (rt * 0.035f, 0.0f);
    visare.lineTo (rt * 0.06f, rt * 0.22f);
    visare.lineTo (-rt * 0.06f, rt * 0.22f);
    visare.lineTo (-rt * 0.035f, 0.0f);
    visare.closeSubPath();
    const auto t = juce::AffineTransform::rotation (a).translated (c);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillPath (visare, t.translated (2.0f, 2.5f));
    g.setColour (svart);
    g.fillPath (visare, t);
    juce::ColourGradient nav (Colour (0xffeeeeea), c.translated (-2.0f, -2.0f), Colour (0xff3a3c3b), c.translated (4.0f, 4.0f), true);
    g.setGradientFill (nav);
    g.fillEllipse (Rectangle<float> (rt * 0.14f, rt * 0.14f).withCentre (c));
    // Glasets reflex.
    juce::Path skara;
    skara.addCentredArc (c.x, c.y, rt * 0.92f, rt * 0.92f, 0.0f, -pi * 0.62f, -pi * 0.05f, true);
    skara.addCentredArc (c.x - rt * 0.08f, c.y + rt * 0.06f, rt * 0.80f, rt * 0.80f, 0.0f, -pi * 0.05f, -pi * 0.62f, false);
    skara.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.32f));
    g.fillPath (skara);
}

// Nödstopp: röd svampknapp på gul rund skylt med text runt om.
inline void ritaNodstopp (juce::Graphics& g, Point<float> c, float r, bool intryckt)
{
    const auto skylt = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (skylt.translated (1.0f, 2.0f).expanded (0.8f));
    g.setGradientFill ({ farg::signalGul.brighter (0.15f), c.x - r, c.y - r, farg::signalGul.darker (0.35f), c.x + r, c.y + r, false });
    g.fillEllipse (skylt);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (skylt.reduced (0.6f), 1.0f);
    const Colour svart (0xff141210);
    tryckPaBage (g, sv ("NÖDSTOPP"), c, r * 0.76f, true, { r * 0.14f, r * 0.028f, 2.6f, 1.0f }, svart);
    tryckPaBage (g, sv ("FÖRBIKOPPLING"), c, r * 0.78f, false, { r * 0.085f, r * 0.02f, 2.0f, 1.0f }, svart);

    // Kragen.
    const float rk = r * 0.62f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (Rectangle<float> (rk * 2.0f, rk * 2.0f).withCentre (c.translated (1.0f, 2.0f)));
    g.setGradientFill ({ Colour (0xff3a3a38), c.x - rk, c.y - rk, Colour (0xff080808), c.x + rk, c.y + rk, false });
    g.fillEllipse (Rectangle<float> (rk * 2.0f, rk * 2.0f).withCentre (c));
    // Svampen.
    const float rs = rk * (intryckt ? 0.86f : 0.92f);
    const auto sc = c.translated (0.0f, intryckt ? 0.0f : -r * 0.04f);
    const auto svamp = Rectangle<float> (rs * 2.0f, rs * 2.0f).withCentre (sc);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (svamp.translated (r * 0.05f, r * 0.10f));
    juce::ColourGradient sg (Colour (0xffff6b55), sc.translated (-rs * 0.35f, -rs * 0.45f), Colour (0xff520406), sc.translated (rs, rs), true);
    sg.addColour (0.45, farg::signalRod);
    g.setGradientFill (sg);
    g.fillEllipse (svamp);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawEllipse (svamp.reduced (rs * 0.12f), 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.fillEllipse (Rectangle<float> (rs * 0.7f, rs * 0.36f).withCentre (sc.translated (-rs * 0.28f, -rs * 0.52f)));
}

// Mekaniskt räkneverk: vita siffror på svarta valsar bakom ett förkromat fönster.
// rullning 0..1 vrider sista valsen en bit mot nästa siffra.
inline void ritaRakneverk (juce::Graphics& g, Rectangle<float> r, const juce::String& siffror, float rullning = 0.0f)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.8f, 1.5f).expanded (1.5f), 3.0f);
    g.setGradientFill ({ Colour (0xffeeeeea), r.getX(), r.getY(), Colour (0xff3b3d3c), r.getX(), r.getBottom(), false });
    g.fillRoundedRectangle (r.expanded (3.0f), 3.5f);
    g.setColour (Colour (0xff050505));
    g.fillRect (r);
    const int n = siffror.length();
    const float cw = r.getWidth() / (float) n;
    const ls::Stil s { r.getHeight() * 0.56f, juce::jmax (1.6f, r.getHeight() * 0.075f), 1.0f, 1.0f };
    for (int i = 0; i < n; ++i)
    {
        const auto cell = Rectangle<float> (r.getX() + cw * (float) i, r.getY(), cw, r.getHeight()).reduced (1.0f, 0.0f);
        g.setGradientFill ({ Colour (0xff050505), cell.getX(), cell.getY(), Colour (0xff050505), cell.getX(), cell.getBottom(), false });
        juce::ColourGradient vals (Colour (0xff060606), cell.getX(), cell.getY(), Colour (0xff060606), cell.getX(), cell.getBottom(), false);
        vals.addColour (0.5, Colour (0xff2c2b29));
        g.setGradientFill (vals);
        g.fillRect (cell);
        juce::Graphics::ScopedSaveState spar (g);
        g.reduceClipRegion (cell.toNearestInt());
        const bool sista = i == n - 1;
        const float forskj = sista ? rullning * cell.getHeight() * 0.9f : 0.0f;
        g.setColour (farg::tryckVitt);
        const auto ch = siffror.substring (i, i + 1);
        ls::rita (g, ch, cell.translated (0.0f, -forskj), s);
        if (sista && rullning > 0.0f)
        {
            const int nasta = (ch.getIntValue() + 1) % 10;
            ls::rita (g, juce::String (nasta), cell.translated (0.0f, cell.getHeight() * 0.9f - forskj), s);
        }
        // Valsens rundning.
        juce::ColourGradient skugga (juce::Colours::black.withAlpha (0.85f), cell.getX(), cell.getY(), juce::Colours::black.withAlpha (0.85f), cell.getX(), cell.getBottom(), false);
        skugga.addColour (0.3, juce::Colours::transparentBlack);
        skugga.addColour (0.7, juce::Colours::transparentBlack);
        g.setGradientFill (skugga);
        g.fillRect (cell);
    }
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.fillRect (r.withHeight (r.getHeight() * 0.35f));
}

// Dymo-remsa: präglade vita versaler på röd plasttejp, lite snett fastklistrad.
inline void ritaDymo (juce::Graphics& g, Rectangle<float> r, const juce::String& text, float lutning = -0.012f)
{
    juce::Graphics::ScopedSaveState spar (g);
    g.addTransform (juce::AffineTransform::rotation (lutning, r.getCentreX(), r.getCentreY()));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.6f, 1.2f), 1.5f);
    g.setGradientFill ({ farg::dymo.brighter (0.25f), r.getX(), r.getY(), farg::dymo.darker (0.4f), r.getX(), r.getBottom(), false });
    g.fillRoundedRectangle (r, 1.5f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawLine (r.getX() + 2.0f, r.getY() + 1.0f, r.getRight() - 2.0f, r.getY() + 1.0f, 0.8f);
    const ls::Stil s { r.getHeight() * 0.48f, r.getHeight() * 0.11f, 2.4f, 1.0f };
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    ls::rita (g, text, r.translated (0.6f, 0.9f), s);
    g.setColour (Colour (0xfff4eee6));
    ls::rita (g, text, r, s);
}

// Signalröret: ett zinkat rör med rundade böjar, flänsar och målade flödespilar.
inline void ritaRor (juce::Graphics& g, const juce::Path& bana, float bredd)
{
    const auto rund = bana.createPathWithRoundedCorners (bredd * 1.6f);
    const juce::PathStrokeType::JointStyle led = juce::PathStrokeType::curved;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (rund, juce::PathStrokeType (bredd + 2.5f, led), juce::AffineTransform::translation (1.5f, 2.5f));
    const float lager[] = { 1.0f, 0.82f, 0.62f, 0.40f, 0.18f };
    const Colour farger[] = { Colour (0xff3c3b36), farg::ror.darker (0.45f), farg::ror.darker (0.1f), farg::ror.brighter (0.15f), Colour (0xfff2f0e6) };
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (farger[i]);
        g.strokePath (rund, juce::PathStrokeType (bredd * lager[i], led), i == 4 ? juce::AffineTransform::translation (-bredd * 0.12f, -bredd * 0.12f) : juce::AffineTransform());
    }
}

// Fläns där röret går in i en plåt. lodrat = röret går lodrätt genom flänsen.
inline void ritaFlans (juce::Graphics& g, Point<float> c, float bredd, bool lodrat)
{
    const auto r = lodrat ? Rectangle<float> (bredd * 2.0f, bredd * 0.7f).withCentre (c) : Rectangle<float> (bredd * 0.7f, bredd * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (1.0f, 1.8f), 1.5f);
    g.setGradientFill ({ Colour (0xffdcdad0), r.getX(), r.getY(), Colour (0xff45443f), r.getRight(), r.getBottom(), false });
    g.fillRoundedRectangle (r, 1.5f);
    for (int i = -1; i <= 1; i += 2)
        ritaNit (g, lodrat ? c.translated ((float) i * bredd * 0.68f, 0.0f) : c.translated (0.0f, (float) i * bredd * 0.68f), 1.4f);
}

// Målad flödespil på röret (svart schablonpil på gul botten).
inline void ritaFlodespil (juce::Graphics& g, Point<float> c, float bredd, Point<float> riktning)
{
    const Point<float> u = riktning / juce::jmax (0.001f, riktning.getDistanceFromOrigin());
    const Point<float> n (-u.y, u.x);
    const float l = bredd * 1.5f, h = bredd * 0.42f;
    juce::Path p;
    p.startNewSubPath (c - u * l + n * h);
    p.lineTo (c + u * (l * 0.25f) + n * h);
    p.lineTo (c + u * l);
    p.lineTo (c + u * (l * 0.25f) - n * h);
    p.lineTo (c - u * l - n * h);
    p.closeSubPath();
    g.setColour (farg::signalGul.withAlpha (0.92f));
    g.fillPath (p);
    juce::Path spets;
    spets.addTriangle (c + u * (l * 0.0f) + n * (h * 0.6f), c + u * (l * 0.75f), c + u * (l * 0.0f) - n * (h * 0.6f));
    g.setColour (Colour (0xff141210));
    g.fillPath (spets);
    g.drawLine ({ c - u * (l * 0.8f), c + u * (l * 0.05f) }, h * 0.5f);
}

} // namespace pr::stil
