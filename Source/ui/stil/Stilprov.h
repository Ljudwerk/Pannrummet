// Fabrikspanelen som bild: geometrin, tillståndet och ritningen, delade av stilprovet och den riktiga panelen.
//
// panel::Tillstand – allt som syns (värden, texter, lampor, mätare).
// panel::rita      – ritar hela panelen i designstorlek 1200 × 760 från ett tillstånd.
// panel::geo       – var allt sitter, så att panelen och musträffarna aldrig glider isär.
// prov::Panel      – stilprovet: rita() med fasta exempelvärden. prov::Detaljer – närbilder av reglagen.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "Fabrik.h"

namespace pr::stil::panel
{

constexpr int bredd = 1200, hojd = 760;

// ---------------------------------------------------------------------------------------------
// Tillståndet

struct Ratt { juce::String namn; float varde = 0.0f; juce::String text; };

struct Modul
{
    int nr = 1;
    juce::String namn, beskrivning, valjarTitel;
    juce::StringArray lagen;
    int lage = 0;
    std::array<Ratt, 4> rattar;
    juce::String vippaTitel, vippaAv, vippaPa;
    bool vippa = false, pa = false;
    float drift = 1.0f;                 // lampans styrka när modulen är på
    bool ejInkopplad = false;           // modulens ljud kommer i en senare fas
    bool visaSteg = false;              // Grinden i TAKT: stegraden i stället för extrabrytaren
    std::array<bool, 16> steg {};
    int stegNu = -1;                    // steget som spelas (−1 = inget)
    bool midiKnapp = false, midiPa = false;
};

struct Tillstand
{
    std::array<Modul, 6> moduler;
    float inDb = -120.0f, utDb = -120.0f;
    float trycket = 0.0f; juce::String trycketText;
    bool forbi = false;
    juce::String programNr = "01", programNamn = "INIT", kategori = "GRUND";
    std::array<Ratt, 3> utgang;         // in-nivå, torr/våt, utnivå
    float limiter = 0.0f;               // lampans styrka
    bool limiterPa = true;
    bool midiIn = false;                // MIDI in för hela Pannrummet
};

// ---------------------------------------------------------------------------------------------
// Geometrin

namespace geo
{
    constexpr float radY[2] { 162.0f, 420.0f };
    constexpr float modulB = 360.0f, modulH = 222.0f, rorY = 110.0f, rorB = 10.0f;

    inline Rectangle<float> modul (int i)          { return { 30.0f + (float) (i % 3) * 390.0f, radY[i / 3], modulB, modulH }; }
    inline Point<float> ratt (int i, int j)        { const auto r = modul (i); return { r.getX() + (j % 2 == 0 ? 232.0f : 312.0f), r.getY() + (j < 2 ? 102.0f : 172.0f) }; }
    constexpr float rattR = 16.0f;
    inline Point<float> valjare (int i)            { const auto r = modul (i); return { r.getX() + 86.0f, r.getY() + 124.0f }; }
    constexpr float valjareR = 20.0f;
    inline Point<float> huvudVippa (int i)         { const auto r = modul (i); return { r.getRight() - 44.0f, r.getY() + 30.0f }; }
    inline Point<float> driftLampa (int i)         { const auto r = modul (i); return { r.getRight() - 96.0f, r.getY() + 27.0f }; }
    inline Point<float> extraVippa (int i)         { const auto r = modul (i); return { r.getX() + 86.0f, r.getY() + 198.0f }; }
    inline Point<float> midiKnapp (int i)          { const auto r = modul (i); return { r.getX() + 228.0f, r.getY() + 27.0f }; }
    inline Rectangle<float> steg (int i, int s)
    {
        const auto r = modul (i);
        return { r.getX() + 16.0f + (float) s * 10.0f + (float) (s / 4) * 4.0f, r.getY() + 190.0f, 8.0f, 14.0f };
    }

    inline Rectangle<float> visarIn()  { return { 420.0f, 22.0f, 132.0f, 106.0f }; }
    inline Rectangle<float> visarUt()  { return { 566.0f, 22.0f, 132.0f, 106.0f }; }
    inline Point<float> manometer()    { return { 790.0f, 75.0f }; }
    constexpr float manometerR = 54.0f;
    inline Point<float> trycket()      { return { 912.0f, 66.0f }; }
    constexpr float trycketR = 24.0f;
    inline Point<float> driftHuvud()   { return { 1002.0f, 52.0f }; }
    inline Point<float> forbiLampa()   { return { 1002.0f, 96.0f }; }
    inline Point<float> midiIn()       { return { 1042.0f, 52.0f }; }
    inline Point<float> nodstopp()     { return { 1110.0f, 75.0f }; }
    constexpr float nodstoppR = 54.0f;

    inline Rectangle<float> rakneverk() { return { 32.0f, 682.0f, 50.0f, 34.0f }; }
    inline Rectangle<float> dymo()      { return { 98.0f, 688.0f, 210.0f, 24.0f }; }
    constexpr float knappY = 699.0f, knappR = 10.0f;
    inline Point<float> knapp (int i)  { const float x[] { 342.0f, 382.0f, 432.0f, 472.0f }; return { x[i], knappY }; }   // förra, nästa, spara, meny
    inline Point<float> utgang (int i) { return { 590.0f + (float) i * 100.0f, 698.0f }; }
    constexpr float utgangR = 18.0f;
    inline Point<float> limiterLampa() { return { 872.0f, 696.0f }; }
}

// ---------------------------------------------------------------------------------------------
// Ritningen

inline void rattMedText (juce::Graphics& g, Point<float> c, float r, float varde, const juce::String& namn, const juce::String& text,
                         Colour visare = farg::tryckVitt)
{
    ritaBakelitRatt (g, c, r, varde, true, true, visare);
    const float y = c.y + r * 1.32f + 12.0f;
    tryck (g, namn, Rectangle<float> (90.0f, 7.0f).withCentre ({ c.x, y }), 5.5f, juce::Justification::centred, farg::tryckVitt, 0.85f, 1.7f);
    tryck (g, text, Rectangle<float> (90.0f, 7.0f).withCentre ({ c.x, y + 10.0f }), 5.0f, juce::Justification::centred, farg::varde, 0.85f, 1.5f);
}

inline void vippaMedText (juce::Graphics& g, Point<float> c, bool pa)
{
    ritaVippa (g, c, pa);
    tryck (g, sv ("PÅ"), Rectangle<float> (30.0f, 7.0f).withCentre (c.translated (0.0f, -24.0f)), 5.0f);
    tryck (g, "AV", Rectangle<float> (30.0f, 7.0f).withCentre (c.translated (0.0f, 24.0f)), 5.0f);
}

// En liten fyrkantig stegknapp i bakelit med lysande topp (Grindens taktmönster).
inline void ritaStegknappLiten (juce::Graphics& g, Rectangle<float> r, bool pa, bool nu)
{
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.6f, 1.2f), 1.5f);
    const Colour f = pa ? farg::barnsten : Colour (0xff2a2622);
    g.setGradientFill ({ f.brighter (pa ? 0.3f : 0.4f), r.getX(), r.getY(), f.darker (0.5f), r.getX(), r.getBottom(), false });
    g.fillRoundedRectangle (r, 1.5f);
    if (pa)
    {
        g.setColour (farg::barnsten.withAlpha (0.25f));
        g.fillRoundedRectangle (r.expanded (2.0f), 3.0f);
    }
    if (nu)
    {
        g.setColour (farg::tryckVitt);
        g.drawRoundedRectangle (r.expanded (1.2f), 2.0f, 1.0f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.2f));
    g.drawLine (r.getX() + 1.0f, r.getY() + 0.8f, r.getRight() - 1.0f, r.getY() + 0.8f, 0.7f);
}

// Grindens lampa och stegrad ändras med ljudet; 'dynamiskt' = false hoppar över dem (panelens cache).
inline void ritaGrindensRorliga (juce::Graphics& g, const Modul& m);

inline void ritaModul (juce::Graphics& g, int i, const Modul& m, const Ytor& ytor, bool dynamiskt = true)
{
    const auto r = geo::modul (i);
    ritaTackplat (g, r, ytor);
    const float x = r.getX(), y = r.getY();

    const auto skylt = Rectangle<float> (x + 16.0f, y + 12.0f, 194.0f, 30.0f);
    ritaSkylt (g, skylt, ytor, false);
    ritaNit (g, { skylt.getRight() - 6.0f, skylt.getCentreY() }, 2.2f);
    const Point<float> nc (skylt.getX() + 17.0f, skylt.getCentreY());
    g.setColour (farg::gravyr);
    g.fillEllipse (Rectangle<float> (20.0f, 20.0f).withCentre (nc));
    g.setColour (farg::aluminium.brighter (0.3f));
    ls::rita (g, juce::String (m.nr), Rectangle<float> (20.0f, 12.0f).withCentre (nc), { 10.0f, 1.8f, 1.0f, 1.0f });
    gravyr (g, m.namn, skylt.withTrimmedLeft (34.0f), 12.0f, juce::Justification::centredLeft, 2.0f, 2.2f);
    if (m.ejInkopplad)
        tryck (g, sv ("EJ INKOPPLAD – LJUDET KOMMER I FAS 2"), { x + 18.0f, y + 51.0f, 260.0f, 6.0f }, 5.0f, juce::Justification::centredLeft,
               farg::signalRod.brighter (0.4f), 0.8f, 1.6f);
    else
        tryck (g, m.beskrivning, { x + 18.0f, y + 51.0f, 260.0f, 6.0f }, 5.0f, juce::Justification::centredLeft, farg::tryckMatt, 0.75f, 1.6f);

    if (m.midiKnapp)
    {
        ritaSignallampa (g, geo::midiKnapp (i), 5.5f, farg::gron, m.midiPa ? 1.0f : 0.0f);
        tryck (g, "MIDI", Rectangle<float> (40.0f, 7.0f).withCentre (geo::midiKnapp (i).translated (0.0f, 19.0f)), 4.8f);
    }
    if (dynamiskt || i != 0) ritaSignallampa (g, geo::driftLampa (i), 6.0f, farg::barnsten, m.pa ? m.drift : 0.0f);
    tryck (g, "DRIFT", Rectangle<float> (40.0f, 7.0f).withCentre (geo::driftLampa (i).translated (0.0f, 19.0f)), 4.8f);
    vippaMedText (g, geo::huvudVippa (i), m.pa);

    const auto vc = geo::valjare (i);
    tryck (g, m.valjarTitel, Rectangle<float> (120.0f, 7.0f).withCentre ({ vc.x, y + 70.0f }), 5.5f, juce::Justification::centred,
           farg::tryckVitt, 0.85f, 2.2f);
    ritaVridomkopplare (g, vc, geo::valjareR, m.lage, m.lagen);

    for (int j = 0; j < 4; ++j)
        rattMedText (g, geo::ratt (i, j), geo::rattR, m.rattar[(size_t) j].varde, m.rattar[(size_t) j].namn, m.rattar[(size_t) j].text);

    if (m.visaSteg)
    {
        tryck (g, "TAKTSTEG", Rectangle<float> (140.0f, 7.0f).withCentre ({ x + 96.0f, y + 180.0f }), 5.0f, juce::Justification::centred,
               farg::tryckVitt, 0.8f, 1.8f);
        if (dynamiskt) for (int s = 0; s < 16; ++s) ritaStegknappLiten (g, geo::steg (i, s), m.steg[(size_t) s], s == m.stegNu);
    }
    else
    {
        const auto ec = geo::extraVippa (i);
        tryck (g, m.vippaTitel, Rectangle<float> (140.0f, 7.0f).withCentre ({ ec.x, y + 178.0f }), 5.0f, juce::Justification::centred,
               farg::tryckVitt, 0.8f, 1.8f);
        ritaVippa (g, ec, m.vippa, false, 0.85f);
        tryck (g, m.vippaAv, Rectangle<float> (30.0f, 7.0f).withCentre (ec.translated (-30.0f, 0.0f)), 5.0f);
        tryck (g, m.vippaPa, Rectangle<float> (30.0f, 7.0f).withCentre (ec.translated (30.0f, 0.0f)), 5.0f);
    }
}

inline void fog (juce::Graphics& g, float y)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (10.0f, y - 1.0f, 1180.0f, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.fillRect (10.0f, y + 1.0f, 1180.0f, 1.0f);
}

// Röret ritas före plåtarna, så att plåtarna täcker rörändarna.
inline void ritaSignalvagen (juce::Graphics& g)
{
    using namespace geo;
    const float y1 = radY[0] + rorY, y2 = radY[1] + rorY, mellan = (radY[0] + modulH + radY[1]) * 0.5f;
    auto rak = [&] (float xa, float xb, float y) { juce::Path p; p.startNewSubPath (xa, y); p.lineTo (xb, y); ritaRor (g, p, rorB); };
    rak (-5.0f, 34.0f, y1);
    rak (386.0f, 424.0f, y1);
    rak (776.0f, 814.0f, y1);
    juce::Path retur;
    retur.startNewSubPath (1166.0f, y1);
    retur.lineTo (1185.0f, y1);
    retur.lineTo (1185.0f, mellan);
    retur.lineTo (15.0f, mellan);
    retur.lineTo (15.0f, y2);
    retur.lineTo (34.0f, y2);
    ritaRor (g, retur, rorB);
    rak (386.0f, 424.0f, y2);
    rak (776.0f, 814.0f, y2);
    rak (1166.0f, 1205.0f, y2);
    for (float x : { 210.0f, 600.0f, 990.0f })
        ritaFlodespil (g, { x, mellan }, rorB, { -1.0f, 0.0f });
    tryck (g, "IN", Rectangle<float> (24.0f, 7.0f).withCentre ({ 15.0f, y1 - 15.0f }), 5.5f);
    tryck (g, "UT", Rectangle<float> (24.0f, 7.0f).withCentre ({ 1185.0f, y2 - 15.0f }), 5.5f);
}

inline void ritaKopplingar (juce::Graphics& g)
{
    using namespace geo;
    for (int rad = 0; rad < 2; ++rad)
        for (int k = 0; k < 3; ++k)
        {
            const float x0 = 30.0f + (float) k * 390.0f;
            ritaFlans (g, { x0, radY[rad] + rorY }, rorB, false);
            ritaFlans (g, { x0 + modulB, radY[rad] + rorY }, rorB, false);
        }
}

inline void ritaModellskylt (juce::Graphics& g, const Ytor& ytor)
{
    const Rectangle<float> skylt (30.0f, 20.0f, 360.0f, 110.0f);
    ritaSkylt (g, skylt, ytor);
    ritaPraglat (g, "PANNRUMMET", { skylt.getX() + 20.0f, skylt.getY() + 18.0f, 300.0f, 52.0f }, { 36.0f, 6.0f, 1.6f, 0.86f }, ytor);
    gravyr (g, sv ("EFFEKTAGGREGAT FÖR KROPPSMUSIK OCH INDUSTRI"), { skylt.getX() + 24.0f, skylt.getY() + 84.0f, 320.0f, 7.0f },
            5.2f, juce::Justification::centredLeft, 0.85f, 1.7f);
    const Rectangle<float> modell (skylt.getRight() - 78.0f, skylt.getY() + 14.0f, 60.0f, 22.0f);
    g.setColour (farg::signalRod);
    g.fillRect (modell);
    g.setColour (farg::tryckVitt);
    ls::rita (g, "PR-89", modell, { 10.0f, 1.7f, 1.6f, 1.0f });
    gravyr (g, sv ("220 V · 50 HZ"), { skylt.getRight() - 90.0f, skylt.getY() + 46.0f, 72.0f, 6.0f }, 4.4f, juce::Justification::centred, 0.7f, 1.3f);
    gravyr (g, "SERIE 0042", { skylt.getRight() - 90.0f, skylt.getY() + 58.0f, 72.0f, 6.0f }, 4.4f, juce::Justification::centred, 0.7f, 1.3f);
}

// De delar som ändras med ljudet (mätare och lampor) ritas för sig, så att panelen kan rita om bara dem.
inline void ritaMatare (juce::Graphics& g, const Tillstand& t)
{
    ritaVisarinstrument (g, geo::visarIn(), t.inDb, "IN");
    ritaVisarinstrument (g, geo::visarUt(), t.utDb, "UT");
}

inline void ritaToppen (juce::Graphics& g, const Tillstand& t, const Ytor& ytor, bool dynamiskt = true)
{
    ritaModellskylt (g, ytor);
    if (dynamiskt) ritaMatare (g, t);
    ritaManometer (g, geo::manometer(), geo::manometerR, t.trycket, "TRYCKET");
    ritaBakelitRatt (g, geo::trycket(), geo::trycketR, t.trycket, true, true, farg::signalRod.brighter (0.4f));
    tryck (g, "TRYCKET", Rectangle<float> (90.0f, 8.0f).withCentre ({ 912.0f, 112.0f }), 7.0f, juce::Justification::centred, farg::tryckVitt, 1.1f, 2.2f);
    tryck (g, t.trycketText, Rectangle<float> (90.0f, 7.0f).withCentre ({ 912.0f, 125.0f }), 5.0f, juce::Justification::centred, farg::varde, 0.85f, 1.5f);
    ritaSignallampa (g, geo::driftHuvud(), 6.5f, farg::gron, t.forbi ? 0.0f : 1.0f);
    tryck (g, "DRIFT", Rectangle<float> (50.0f, 7.0f).withCentre (geo::driftHuvud().translated (0.0f, 19.0f)), 5.0f);
    ritaSignallampa (g, geo::forbiLampa(), 6.5f, farg::rod, t.forbi ? 1.0f : 0.0f);
    tryck (g, sv ("FÖRBI"), Rectangle<float> (50.0f, 7.0f).withCentre (geo::forbiLampa().translated (0.0f, 19.0f)), 5.0f);
    ritaSignallampa (g, geo::midiIn(), 6.5f, farg::gron, t.midiIn ? 1.0f : 0.0f);
    tryck (g, "MIDI", Rectangle<float> (50.0f, 7.0f).withCentre (geo::midiIn().translated (0.0f, 19.0f)), 5.0f);
    ritaNodstopp (g, geo::nodstopp(), geo::nodstoppR, t.forbi);
}

inline void ritaLimiterLampa (juce::Graphics& g, const Tillstand& t)
{
    ritaSignallampa (g, geo::limiterLampa(), 5.5f, farg::rod, t.limiterPa ? t.limiter : 0.0f);
}

inline void ritaBotten (juce::Graphics& g, const Tillstand& t, const Ytor& ytor, bool dynamiskt = true)
{
    tryck (g, "PROGRAM", { 30.0f, 664.0f, 200.0f, 7.0f }, 5.5f, juce::Justification::centredLeft, farg::tryckVitt, 0.85f, 2.2f);
    ritaRakneverk (g, geo::rakneverk(), t.programNr, 0.0f);
    tryck (g, t.kategori, { 98.0f, 677.0f, 120.0f, 6.0f }, 4.8f, juce::Justification::centredLeft, farg::tryckMatt, 0.75f, 1.6f);
    ritaDymo (g, geo::dymo(), t.programNamn);

    ritaTryckknapp (g, geo::knapp (0), geo::knappR, trekant (Rectangle<float> (7.0f, 8.0f).withCentre (geo::knapp (0).translated (-1.0f, 0.0f)), false));
    ritaTryckknapp (g, geo::knapp (1), geo::knappR, trekant (Rectangle<float> (7.0f, 8.0f).withCentre (geo::knapp (1).translated (1.0f, 0.0f)), true));
    ritaTryckknapp (g, geo::knapp (2), geo::knappR);
    ritaTryckknapp (g, geo::knapp (3), geo::knappR);
    const char* etiketter[] { "FÖRRA", "NÄSTA", "SPARA", "MENY" };
    for (int i = 0; i < 4; ++i)
        tryck (g, sv (etiketter[i]), Rectangle<float> (50.0f, 7.0f).withCentre (geo::knapp (i).translated (0.0f, 24.0f)), 4.8f);

    for (int i = 0; i < 3; ++i)
        rattMedText (g, geo::utgang (i), geo::utgangR, t.utgang[(size_t) i].varde, t.utgang[(size_t) i].namn, t.utgang[(size_t) i].text);
    if (dynamiskt) ritaLimiterLampa (g, t);
    tryck (g, t.limiterPa ? juce::String ("LIMITER") : sv ("LIMITER AV"), Rectangle<float> (60.0f, 7.0f).withCentre (geo::limiterLampa().translated (0.0f, 27.0f)), 4.8f);

    const Rectangle<float> varning (930.0f, 666.0f, 240.0f, 78.0f);
    ritaVarningsrand (g, varning);
    const auto inre = varning.reduced (8.0f);
    ritaSkylt (g, inre, ytor, true, 1.5f);
    gravyr (g, "VARNING", inre.withHeight (30.0f).withTrimmedTop (4.0f), 13.0f, juce::Justification::centred, 2.2f, 2.6f);
    gravyr (g, sv ("HÖG LJUDNIVÅ · ANVÄND HÖRSELSKYDD"), inre.withTrimmedTop (36.0f).withHeight (8.0f), 4.8f, juce::Justification::centred, 0.8f, 1.5f);
    gravyr (g, sv ("ÖPPNA EJ UNDER DRIFT"), inre.withTrimmedTop (48.0f).withHeight (8.0f), 4.8f, juce::Justification::centred, 0.8f, 1.5f,
            farg::signalRod.darker (0.2f));
}

// Hela panelen. dynamiskt = false ritar bakgrunden utan mätare och rörliga lampor (se ritaDynamiskt).
inline void rita (juce::Graphics& g, const Tillstand& t, const Ytor& ytor, bool dynamiskt = true)
{
    const Rectangle<float> hela (0.0f, 0.0f, (float) bredd, (float) hojd);
    g.setFillType (ytor.fyll (ytor.lack));
    g.fillRect (hela);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRect (hela.reduced (1.0f), 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRect (hela, 1.0f);
    fog (g, 148.0f);
    fog (g, 650.0f);
    ritaSignalvagen (g);
    for (int i = 0; i < 6; ++i) ritaModul (g, i, t.moduler[(size_t) i], ytor, dynamiskt);
    ritaKopplingar (g);
    ritaToppen (g, t, ytor, dynamiskt);
    ritaBotten (g, t, ytor, dynamiskt);
    for (auto p : { Point<float> (12.0f, 12.0f), Point<float> (1188.0f, 12.0f), Point<float> (12.0f, 748.0f), Point<float> (1188.0f, 748.0f) })
        ritaSkruv (g, p, 4.5f, p.x * 0.01f + p.y * 0.003f);
}

inline void ritaGrindensRorliga (juce::Graphics& g, const Modul& m)
{
    ritaSignallampa (g, geo::driftLampa (0), 6.0f, farg::barnsten, m.pa ? m.drift : 0.0f);
    if (m.visaSteg)
        for (int s = 0; s < 16; ++s) ritaStegknappLiten (g, geo::steg (0, s), m.steg[(size_t) s], s == m.stegNu);
}

// Det som rita (…, false) hoppade över: mätarna, limiterlampan och Grindens lampa och steg.
inline void ritaDynamiskt (juce::Graphics& g, const Tillstand& t)
{
    ritaMatare (g, t);
    ritaLimiterLampa (g, t);
    ritaGrindensRorliga (g, t.moduler[0]);
}

// Ytorna som ritaDynamiskt kan ändra, för repaint().
inline juce::Array<Rectangle<float>> dynamiskaYtor()
{
    return { geo::visarIn().expanded (4.0f), geo::visarUt().expanded (4.0f),
             Rectangle<float> (50.0f, 50.0f).withCentre (geo::limiterLampa()),
             Rectangle<float> (54.0f, 54.0f).withCentre (geo::driftLampa (0)),
             geo::steg (0, 0).getUnion (geo::steg (0, 15)).expanded (4.0f) };
}

// Modulernas fasta texter (namn, lägen, rattnamn).
inline juce::StringArray svLista (std::initializer_list<const char*> l)
{
    juce::StringArray a;
    for (auto* s : l) a.add (sv (s));
    return a;
}

inline Modul grundModul (int i)
{
    Modul m;
    m.nr = i + 1;
    auto ratt = [&] (int j, const char* namn) { m.rattar[(size_t) j].namn = sv (namn); };
    switch (i)
    {
        case 0: m.namn = "GRINDEN"; m.beskrivning = sv ("GATE · TRÖSKEL OCH HÅLLTID"); m.valjarTitel = sv ("LÄGE");
                m.lagen = svLista ({ "MJUK", "HÅRD", "TAKT" });
                ratt (0, "TRÖSKEL"); ratt (1, "HÅLLTID"); ratt (2, "SLÄPP"); ratt (3, "DJUP");
                m.vippaTitel = "SIDOKEDJA"; m.vippaAv = "AV"; m.vippaPa = sv ("PÅ"); break;
        case 1: m.namn = "PRESSEN"; m.beskrivning = sv ("DRIV · TYNGD · NIVÅKOMPENSATION"); m.valjarTitel = sv ("LÄGE");
                m.lagen = svLista ({ "RÖR", "TRANSISTOR", "LIKRIKTARE", "BITKROSS" });
                ratt (0, "DRIV"); ratt (1, "TON"); ratt (2, "TYNGD"); ratt (3, "NIVÅ");
                m.vippaTitel = sv ("ÖVERSAMPLING"); m.vippaAv = sv ("2×"); m.vippaPa = sv ("4×"); break;
        case 2: m.namn = sv ("PLÅTEN"); m.beskrivning = sv ("METALLRESONATOR · MODALSYNTES"); m.valjarTitel = "MATERIAL";
                m.lagen = svLista ({ "PLÅT", "RÖR", "FJÄDER", "KLOCKA", "RING" });
                ratt (0, "STÄMNING"); ratt (1, "KLANG"); ratt (2, "LJUSHET"); ratt (3, "BLANDNING");
                m.vippaTitel = sv ("FÖRE PRESSEN"); m.vippaAv = "AV"; m.vippaPa = sv ("PÅ"); m.midiKnapp = true; break;
        case 3: m.namn = "MEGAFONEN"; m.beskrivning = sv ("LO-FI FÖR RÖSTER OCH PAROLLER"); m.valjarTitel = sv ("LÄGE");
                m.lagen = svLista ({ "MEGAFON", "TELEFON", "RADIO", "TRATT" });
                ratt (0, "BAND"); ratt (1, "RESONANS"); ratt (2, "SMUTS"); ratt (3, "BLANDNING");
                m.vippaTitel = "BRUS"; m.vippaAv = "AV"; m.vippaPa = sv ("PÅ"); break;
        case 4: m.namn = "SCHAKTET"; m.beskrivning = sv ("SYNKAT EKO · MÖRKNAR FÖR VARJE VARV"); m.valjarTitel = "TID";
                m.lagen = svLista ({ "1/16", "1/8T", "1/8", "1/8.", "1/4", "1/4.", "1/2" });
                ratt (0, "ÅTERKOPPLING"); ratt (1, "MÖRKER"); ratt (2, "SVAJ"); ratt (3, "BLANDNING");
                m.vippaTitel = "PING-PONG"; m.vippaAv = "AV"; m.vippaPa = sv ("PÅ"); break;
        default: m.namn = "HALLEN"; m.beskrivning = sv ("GATAD REVERB · RUMMET STÄNGS TVÄRT"); m.valjarTitel = "LOKAL";
                m.lagen = svLista ({ "VERKSTAD", "HALL", "VALV", "GRUVA" });
                ratt (0, "STORLEK"); ratt (1, "HÅLLTID"); ratt (2, "FALL"); ratt (3, "BLANDNING");
                m.vippaTitel = "GRIND"; m.vippaAv = "AV"; m.vippaPa = sv ("PÅ"); break;
    }
    return m;
}

// Stilprovets exempelvärden.
inline Tillstand exempel()
{
    Tillstand t;
    struct V { int lage; float v[4]; const char* text[4]; bool vippa, pa; };
    const V v[6] {
        { 1, { 0.42f, 0.30f, 0.18f, 1.0f },  { "−32 DB", "40 MS", "25 MS", "100 %" }, false, true },
        { 1, { 0.68f, 0.55f, 0.40f, 0.5f },  { "68 %", "55 %", "40 %", "0,0 DB" }, true, true },
        { 0, { 0.45f, 0.62f, 0.50f, 0.25f }, { "A2", "1,4 S", "50 %", "25 %" }, false, false },
        { 0, { 0.55f, 0.40f, 0.60f, 1.0f },  { "1,8 KHZ", "40 %", "60 %", "100 %" }, false, true },
        { 3, { 0.45f, 0.60f, 0.20f, 0.30f }, { "45 %", "60 %", "20 %", "30 %" }, true, true },
        { 1, { 0.70f, 0.35f, 0.20f, 0.35f }, { "70 %", "180 MS", "30 MS", "35 %" }, true, true },
    };
    for (int i = 0; i < 6; ++i)
    {
        auto m = grundModul (i);
        m.lage = v[i].lage; m.vippa = v[i].vippa; m.pa = v[i].pa;
        for (int j = 0; j < 4; ++j) { m.rattar[(size_t) j].varde = v[i].v[j]; m.rattar[(size_t) j].text = sv (v[i].text[j]); }
        t.moduler[(size_t) i] = m;
    }
    t.moduler[2].midiPa = true;
    t.inDb = -4.0f; t.utDb = -1.0f; t.trycket = 0.62f; t.trycketText = "62 %";
    t.programNr = "07"; t.programNamn = "FABRIKSGOLV"; t.kategori = "EBM";
    t.utgang = { Ratt { sv ("IN-NIVÅ"), 0.5f, "0,0 DB" }, Ratt { sv ("TORR/VÅT"), 0.8f, "80 %" }, Ratt { "UTNIVÅ", 0.46f, sv ("−1,5 DB") } };
    t.limiter = 0.55f;
    return t;
}

} // namespace pr::stil::panel

namespace pr::stil::prov
{

class Panel : public juce::Component
{
public:
    static constexpr int bredd = panel::bredd, hojd = panel::hojd;
    Panel() { setSize (bredd, hojd); setOpaque (true); }
    void paint (juce::Graphics& g) override
    {
        ytor.forbered (bredd, hojd, juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor()));
        panel::rita (g, panel::exempel(), ytor);
    }
private:
    Ytor ytor;
};

// Närbilder av reglagen.
class Detaljer : public juce::Component
{
public:
    static constexpr int bredd = 1200, hojd = 600;

    Detaljer() { setSize (bredd, hojd); setOpaque (true); }

    void paint (juce::Graphics& g) override
    {
        ytor.forbered (bredd, hojd, juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor()));
        g.setFillType (ytor.fyll (ytor.lack));
        g.fillRect (getLocalBounds());

        const char* titlar[] { "BAKELITRATT", "VRIDOMKOPPLARE", "VIPPOR", "SIGNALLAMPOR", "VISARINSTRUMENT", "MANOMETER", "RÄKNEVERK OCH DYMO", "NÖDSTOPP" };
        for (int i = 0; i < 8; ++i)
        {
            const auto cell = Rectangle<float> ((float) (i % 4) * 300.0f, (float) (i / 4) * 300.0f, 300.0f, 300.0f).reduced (10.0f);
            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.drawRect (cell, 1.0f);
            tryck (g, sv (titlar[i]), cell.withHeight (20.0f), 6.0f, juce::Justification::centred, farg::tryckMatt, 0.85f, 2.0f);
            juce::Graphics::ScopedSaveState spar (g);
            g.reduceClipRegion (cell.toNearestInt());
            const auto c = cell.getCentre().translated (0.0f, 10.0f);
            auto zoom = [&] (float k) { g.addTransform (juce::AffineTransform::scale (k, k, c.x, c.y)); };
            switch (i)
            {
                case 0: zoom (3.2f); ritaBakelitRatt (g, c, 16.0f, 0.62f); break;
                case 1: zoom (2.4f); ritaVridomkopplare (g, c, 20.0f, 1, panel::svLista ({ "RÖR", "TRANSISTOR", "LIKRIKTARE", "BITKROSS" })); break;
                case 2: zoom (2.6f); ritaVippa (g, c.translated (-22.0f, 0.0f), true); ritaVippa (g, c.translated (22.0f, 0.0f), false);
                        ritaVippa (g, c.translated (0.0f, 36.0f), true, false, 0.85f); break;
                case 3: zoom (2.6f);
                        ritaSignallampa (g, c.translated (-30.0f, -14.0f), 6.0f, farg::barnsten, 1.0f);
                        ritaSignallampa (g, c.translated (0.0f, -14.0f), 6.0f, farg::gron, 1.0f);
                        ritaSignallampa (g, c.translated (30.0f, -14.0f), 6.0f, farg::rod, 1.0f);
                        ritaSignallampa (g, c.translated (-15.0f, 20.0f), 6.0f, farg::barnsten, 0.0f);
                        ritaSignallampa (g, c.translated (15.0f, 20.0f), 6.0f, farg::rod, 0.0f); break;
                case 4: zoom (1.9f); ritaVisarinstrument (g, Rectangle<float> (132.0f, 106.0f).withCentre (c), -2.0f, "UT"); break;
                case 5: zoom (2.1f); ritaManometer (g, c, 54.0f, 0.86f, "TRYCKET"); break;
                case 6: zoom (2.1f); ritaRakneverk (g, Rectangle<float> (50.0f, 34.0f).withCentre (c.translated (0.0f, -24.0f)), "07", 0.3f);
                        ritaDymo (g, Rectangle<float> (110.0f, 22.0f).withCentre (c.translated (0.0f, 24.0f)), "SMEDJAN"); break;
                case 7: zoom (2.1f); ritaNodstopp (g, c, 54.0f, false); break;
                default: break;
            }
        }
    }

private:
    Ytor ytor;
};

} // namespace pr::stil::prov
