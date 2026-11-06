// PRSnapshot – ritar Pannrummets panel till PNG, så att gränssnittet kan granskas utan DAW.
//
//   PRSnapshot fil.png [skala] [program=N] [id=värde ...]  den riktiga panelen (program N från fabriksbandet, givna värden)
//   PRSnapshot --stilprov fil.png [skala]            fabrikspanelens stilprov (fasta exempelvärden)
//   PRSnapshot --stilprov-detaljer fil.png [skala]   närbilder av reglagen
//   PRSnapshot --manual mapp                         manualens bilder (manual_*.png, utsnitt i 200 %)
//   PRSnapshot --klicktest                           drar, klickar och rullar på varje reglage och kontrollerar
//                                                    att rätt parameter (och bara den) ändrades; OK/FEL
//   PRSnapshot --prestanda                           ritningstider: hela panelen och de rörliga delarna
//
// Skalan är fysisk (2 = Retina). Stilproven använder varken processorn eller parametrar.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginEditor.h"
#include <iostream>
#include <map>
#include <chrono>

static int sparaPNG (juce::Component& c, const juce::File& fil, float skala, const char* vad)
{
    const auto bild = c.createComponentSnapshot (c.getLocalBounds(), true, skala);
    fil.deleteFile();
    juce::FileOutputStream ut (fil);
    juce::PNGImageFormat png;
    if (! ut.openedOk() || ! png.writeImageToStream (bild, ut))
    {
        std::cout << "Kunde inte skriva " << fil.getFullPathName() << "\n";
        return 1;
    }
    std::cout << vad << " ritat till " << fil.getFullPathName() << "\n";
    return 0;
}


namespace
{
using Panelen = pr::ui::Panelen;

// En processor med panel; 'satt' tar parametrarnas riktiga värden.
struct Rigg
{
    PannrummetProcessor p;
    std::unique_ptr<juce::AudioProcessorEditor> ed;
    Panelen* panel = nullptr;

    Rigg()
    {
        p.setPlayConfigDetails (2, 2, 48000.0, 512);
        p.prepareToPlay (48000.0, 512);
        ed.reset (p.createEditor());
        panel = dynamic_cast<Panelen*> (ed->getChildComponent (0));
        panel->setTransform ({});
    }
    void satt (const juce::String& id, float v)
    {
        if (auto* par = p.apvts.getParameter (id)) par->setValueNotifyingHost (par->convertTo0to1 (v));
    }
    std::map<juce::String, float> alla() const
    {
        std::map<juce::String, float> v;
        for (auto* par : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par)) v[r->getParameterID()] = r->getValue();
        return v;
    }
    void spela (int block = 40)
    {
        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        for (int k = 0; k < block; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float v = 0.5f * std::sin ((float) (k * 512 + i) * 0.03f); b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        }
    }
};

juce::String utf8 (const char* s) { return juce::String::fromUTF8 (s); }

int fel = 0, antalOk = 0;
void kontroll (bool ok, const juce::String& vad)
{
    if (ok) ++antalOk; else ++fel;
    if (! ok) std::cout << "  FEL   " << vad << "\n";
}

// Klicktestet: varje träffyta på panelen, med mus som en människa.
int klicktest()
{
    Rigg r;
    auto& panel = *r.panel;
    auto kalla = juce::Desktop::getInstance().getMainMouseSource();
    const auto t = juce::Time::getCurrentTime();
    auto mus = [&] (juce::Point<float> p, juce::ModifierKeys m, float dy, int klick = 1)
    {
        const auto knapp = m.withFlags (juce::ModifierKeys::leftButtonModifier);
        juce::MouseEvent ned (kalla, p, knapp, 1.0f, 0, 0, 0, 0, &panel, &panel, t, p, t, klick, false);
        juce::MouseEvent drag (kalla, p.translated (0.0f, dy), knapp, 1.0f, 0, 0, 0, 0, &panel, &panel, t, p, t, klick, true);
        juce::MouseEvent upp (kalla, p.translated (0.0f, dy), m, 1.0f, 0, 0, 0, 0, &panel, &panel, t, p, t, klick, std::abs (dy) > 0.0f);
        panel.mouseDown (ned);
        if (std::abs (dy) > 0.0f) panel.mouseDrag (drag);
        panel.mouseUp (upp);
        if (klick == 2) panel.mouseDoubleClick (ned);
    };
    auto hjul = [&] (juce::Point<float> p, float d)
    {
        juce::MouseEvent e (kalla, p, {}, 1.0f, 0, 0, 0, 0, &panel, &panel, t, p, t, 0, false);
        panel.mouseWheelMove (e, { 0.0f, d, false, false, false });
    };
    // Bara 'id' får ha ändrats sedan 'fore'.
    auto bara = [&] (const std::map<juce::String, float>& fore, const juce::String& id)
    {
        for (auto& [k, v] : r.alla())
            if (k != id && std::abs (v - fore.at (k)) > 1.0e-6f) return false;
        return true;
    };

    int ratt = 0, valjare = 0, brytare = 0, knappar = 0;
    auto provaAlla = [&] (bool takt)
    {
        r.satt (pr::id::grindLage, takt ? 2.0f : 1.0f);
        panel.lasOmAllt();
        const auto& traffar = panel.allaTraffar();
        for (int i = 0; i < (int) traffar.size(); ++i)
        {
            const auto h = traffar[(size_t) i];
            if (! panel.synlig (i)) continue;
            if (takt && h.typ != Panelen::Typ::brytare) continue;   // i andra varvet bara Grindens steg
            const auto c = h.yta.getCentre();
            auto* par = r.p.apvts.getParameter (h.pid);
            const juce::String namn = par != nullptr ? par->getName (60) + " (" + h.pid + ")" : "knapp " + juce::String (h.index);
            if (h.typ == Panelen::Typ::ratt)
            {
                r.satt (h.pid, par->convertFrom0to1 (0.3f));
                auto fore = r.alla();
                mus (c, {}, -40.0f);
                const float upp = par->getValue();
                kontroll (upp > 0.3f + 1.0e-3f && bara (fore, h.pid), namn + ": dra uppåt höjer");
                fore = r.alla();
                mus (c, juce::ModifierKeys::shiftModifier, 40.0f);
                kontroll (par->getValue() < upp && par->getValue() > upp - 0.1f && bara (fore, h.pid), namn + ": Skift-dra nedåt sänker lite");
                fore = r.alla();
                hjul (c, 0.5f);
                kontroll (bara (fore, h.pid), namn + ": hjulet rör bara ratten");
                mus (c, {}, 0.0f, 2);
                kontroll (std::abs (par->getValue() - par->getDefaultValue()) < 1.0e-4f, namn + ": dubbelklick ger grundvärdet");
                ++ratt;
            }
            else if (h.typ == Panelen::Typ::valjare)
            {
                const int antal = par->getNumSteps();
                r.satt (h.pid, 0.0f);
                auto fore = r.alla();
                mus (c, {}, 0.0f);
                kontroll (std::lround (par->convertFrom0to1 (par->getValue())) == 1 && bara (fore, h.pid), namn + ": klick ger nästa läge");
                mus (c, juce::ModifierKeys::shiftModifier, 0.0f);
                kontroll (std::lround (par->convertFrom0to1 (par->getValue())) == 0, namn + ": Skift-klick ger förra läget");
                mus (c, juce::ModifierKeys::shiftModifier, 0.0f);
                kontroll (std::lround (par->convertFrom0to1 (par->getValue())) == antal - 1, namn + ": förra läget från det första går runt");
                hjul (c, -0.5f);
                kontroll (std::lround (par->convertFrom0to1 (par->getValue())) == antal - 2, namn + ": hjulet stegar");
                r.satt (h.pid, par->convertFrom0to1 (par->getDefaultValue()));
                ++valjare;
            }
            else if (h.typ == Panelen::Typ::brytare)
            {
                if (takt && h.index < 100) continue;
                const float var = par->getValue();
                auto fore = r.alla();
                mus (c, {}, 0.0f);
                kontroll (std::abs (par->getValue() - (1.0f - var)) < 1.0e-6f && bara (fore, h.pid), namn + ": klick växlar");
                mus (c, {}, 0.0f);
                kontroll (std::abs (par->getValue() - var) < 1.0e-6f, namn + ": klick igen växlar tillbaka");
                ++brytare;
            }
            else if (h.index == 0 || h.index == 1)
            {
                // FÖRRA och NÄSTA (SPARA, MENY och listan öppnar dialoger och menyer och provas på Macen).
                auto& bank = r.p.programbank;
                const int nu = bank.aktuellt();
                mus (c, {}, 0.0f);
                const int vantat = (nu + (h.index == 0 ? -1 : 1) + bank.antal()) % bank.antal();
                kontroll (bank.aktuellt() == vantat, utf8 (h.index == 0 ? "FÖRRA" : "NÄSTA") + ": byter program");
                bank.ladda (nu);
                ++knappar;
            }
        }
    };
    provaAlla (false);
    provaAlla (true);
    std::cout << antalOk << " kontroller OK (" << ratt << " rattar, " << valjare << " vridomkopplare, " << brytare
              << " brytare och steg, " << knappar << " knappar)\n";
    std::cout << (fel == 0 ? utf8 ("Klicktestet gick igenom.\n") : juce::String (fel) + " fel.\n");
    return fel == 0 ? 0 : 1;
}

// Manualens bilder: utsnitt ur panelen i 200 %.
int manualbilder (const juce::File& mapp)
{
    mapp.createDirectory();
    auto bild = [] (std::initializer_list<std::pair<const char*, float>> varden, int program)
    {
        Rigg r;
        r.p.programbank.ladda (program - 1);
        for (auto& [k, v] : varden) r.satt (k, v);
        r.panel->lasOmAllt();
        r.spela();
        return r.panel->createComponentSnapshot (r.panel->getLocalBounds(), true, 2.0f);
    };
    auto spara = [&] (const juce::Image& im, juce::Rectangle<int> ruta, const char* namn)
    {
        const auto utsnitt = im.getClippedImage (ruta * 2);
        auto fil = mapp.getChildFile (namn);
        fil.deleteFile();
        juce::FileOutputStream ut (fil);
        juce::PNGImageFormat png;
        png.writeImageToStream (utsnitt, ut);
        std::cout << fil.getFileName() << "\n";
    };
    const auto alla = bild ({ { pr::id::platPa, 1 }, { pr::id::platMaterial, 0 }, { pr::id::platBlandning, 0.3f }, { pr::id::megaPa, 1 },
                              { pr::id::trycket, 0.35f }, { pr::id::midiIn, 1 }, { pr::id::platMidi, 1 } }, 16);
    const char* moduler[] { "manual_grinden.png", "manual_pressen.png", "manual_platen.png", "manual_megafonen.png", "manual_schaktet.png", "manual_hallen.png" };
    for (int i = 0; i < 6; ++i)
        spara (alla, pr::stil::panel::geo::modul (i).toNearestInt().expanded (8), moduler[i]);
    spara (alla, { 0, 0, 1200, 148 }, "manual_toppen.png");
    spara (alla, { 0, 652, 1200, 108 }, "manual_botten.png");
    const auto takt = bild ({}, 6);
    spara (takt, pr::stil::panel::geo::modul (0).toNearestInt().expanded (8), "manual_grinden_takt.png");
    return 0;
}

// Ritningstider: hela panelen (bakgrunden ritas om, som när en parameter ändras) och de rörliga delarna
// (mätarna och lamporna, upp till 30 gånger i sekunden).
int prestanda()
{
    Rigg r;
    r.spela (10);
    for (float skala : { 1.0f, 2.0f })
    {
        const int n = 10;
        double hela = 0.0, rorliga = 0.0;
        juce::Image im (juce::Image::RGB, (int) (1200 * skala), (int) (760 * skala), true);
        for (int i = 0; i < n; ++i)
        {
            r.panel->lasOmAllt();
            auto t0 = std::chrono::steady_clock::now();
            { juce::Graphics g (im); g.addTransform (juce::AffineTransform::scale (skala)); r.panel->paintEntireComponent (g, true); }
            auto t1 = std::chrono::steady_clock::now();
            for (auto yta : pr::stil::panel::dynamiskaYtor())
            {
                juce::Graphics g (im);
                g.addTransform (juce::AffineTransform::scale (skala));
                g.reduceClipRegion (yta.getSmallestIntegerContainer());
                r.panel->paintEntireComponent (g, true);
            }
            auto t2 = std::chrono::steady_clock::now();
            hela += std::chrono::duration<double, std::milli> (t1 - t0).count();
            rorliga += std::chrono::duration<double, std::milli> (t2 - t1).count();
        }
        hela /= n; rorliga /= n;
        std::cout << "Skala " << skala << ": hela panelen " << juce::String (hela, 1) << " ms, de rörliga delarna "
                  << juce::String (rorliga, 2) << " ms per bild = " << juce::String (rorliga * 30.0 / 10.0, 1)
                  << utf8 (" % av en kärna i 30 bilder/s\n");
    }
    std::cout << utf8 ("(Mjukvarurenderaren i molnet; på Macen ritar CoreGraphics och siffran för målet mäts där.)\n");
    return 0;
}
} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc < 2)
    {
        std::cout << "Användning: PRSnapshot fil.png [skala] [program=N] [id=värde ...] | --stilprov | --stilprov-detaljer fil.png [skala] | --manual mapp | --klicktest | --prestanda\n";
        return 1;
    }
    const juce::String lage (argv[1]);
    if (! lage.startsWith ("--"))
    {
        // Den riktiga panelen: processorn med givna värden, ljud genom kedjan så att mätarna rör sig.
        PannrummetProcessor p;
        p.setPlayConfigDetails (2, 2, 48000.0, 512);
        for (int i = 3; i < argc; ++i)
        {
            const juce::String a (argv[i]);
            if (a.startsWith ("program=")) { p.programbank.ladda (a.getTrailingIntValue() - 1); continue; }
            if (auto* par = p.apvts.getParameter (a.upToFirstOccurrenceOf ("=", false, false)))
                par->setValueNotifyingHost (par->convertTo0to1 (a.fromFirstOccurrenceOf ("=", false, false).getFloatValue()));
        }
        p.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        for (int k = 0; k < 40; ++k)
        {
            for (int i = 0; i < 512; ++i) { const float v = 0.5f * std::sin ((float) (k * 512 + i) * 0.03f); b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
        }
        auto* panel = ed->getChildComponent (0);
        panel->setTransform ({});
        const float skala = argc > 2 ? (float) juce::String (argv[2]).getDoubleValue() : 1.0f;
        return sparaPNG (*panel, juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]), skala, "Panelen");
    }
    if (lage == "--klicktest") return klicktest();
    if (lage == "--prestanda") return prestanda();
    if (lage == "--manual" && argc >= 3) return manualbilder (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
    if (argc < 3) return 1;
    const juce::File fil = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
    const float skala = argc > 3 ? (float) juce::String (argv[3]).getDoubleValue() : 1.0f;

    if (lage == "--stilprov")
    {
        pr::stil::prov::Panel panel;
        panel.setBounds (0, 0, pr::stil::prov::Panel::bredd, pr::stil::prov::Panel::hojd);
        return sparaPNG (panel, fil, skala, "Stilprovet");
    }
    if (lage == "--stilprov-detaljer")
    {
        pr::stil::prov::Detaljer d;
        d.setBounds (0, 0, pr::stil::prov::Detaljer::bredd, pr::stil::prov::Detaljer::hojd);
        return sparaPNG (d, fil, skala, "Detaljarket");
    }
    std::cout << "Okänt läge: " << lage << "\n";
    return 1;
}
