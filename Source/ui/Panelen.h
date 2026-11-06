// Panelen – fabrikspanelen kopplad till parametrarna.
//
// Allt ritas av panel::rita (samma kod som stilprovet). Bakgrunden – allt utom mätarna och de lampor
// som rör sig med ljudet – cachas som en bild och ritas bara om när en parameter ändras. Mätarna och
// lamporna ritas ovanpå i högst 30 bilder/s, och bara deras ytor görs om.
//
// Reglagen: rattar dras upp/ned (Skift = fint), rullas, dubbelklick ger grundvärdet.
// Vridomkopplare: klick = nästa, Skift-klick = förra, rulla, högerklick = lista.
// Vippor, steg, MIDI-lampan, limiterlampan och nödstoppet: klick växlar.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <krang/ui/ValueFormat.h>
#include "../PluginProcessor.h"
#include "stil/Stilprov.h"

namespace pr::ui
{

using namespace pr::stil;

class Panelen : public juce::Component, private juce::Timer, private juce::AudioProcessorValueTreeState::Listener
{
public:
    std::function<void (float)> onStorlek;       // MENY → Storlek
    float storlek = 0.85f;

    explicit Panelen (PannrummetProcessor& p) : proc (p)
    {
        setSize (panel::bredd, panel::hojd);
        setOpaque (true);
        byggTraffar();
        for (auto* par : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par))
                proc.apvts.addParameterListener (r->getParameterID(), this);
        uppdateraTillstand();
        startTimerHz (30);
    }

    ~Panelen() override
    {
        for (auto* par : proc.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par))
                proc.apvts.removeParameterListener (r->getParameterID(), this);
    }

    void paint (juce::Graphics& g) override
    {
        const float skala = juce::jmax (1.0f, g.getInternalContext().getPhysicalPixelScaleFactor());
        ytor.forbered (panel::bredd, panel::hojd, skala);
        if (! cache.isValid() || std::abs (cacheSkala - skala) > 0.001f || cacheSmutsig)
        {
            cacheSkala = skala;
            cache = juce::Image (juce::Image::RGB, (int) std::ceil (panel::bredd * skala), (int) std::ceil (panel::hojd * skala), false);
            juce::Graphics cg (cache);
            cg.addTransform (juce::AffineTransform::scale (skala));
            panel::rita (cg, t, ytor, false);
            cacheSmutsig = false;
        }
        g.drawImageTransformed (cache, juce::AffineTransform::scale (1.0f / cacheSkala));
        panel::ritaDynamiskt (g, t);
    }

    // ------------------------------------------------------------------------------------------
    // Musen

    void mouseDown (const juce::MouseEvent& e) override
    {
        aktiv = hitta (e.position);
        if (aktiv < 0) return;
        auto& h = traffar[(size_t) aktiv];
        auto* par = parameter (h.pid);
        if (h.typ == Typ::knapp) { tryckKnapp (h.index); return; }
        if (par == nullptr) return;
        if (h.typ == Typ::ratt)
        {
            startVarde = par->getValue(); startY = e.position.y;
            par->beginChangeGesture();
            return;
        }
        if (h.typ == Typ::valjare)
        {
            if (e.mods.isPopupMenu()) { valjarLista (*par); aktiv = -1; return; }
            stegaValjare (*par, e.mods.isShiftDown() ? -1 : 1, true);
            aktiv = -1;
            return;
        }
        // Brytare.
        par->beginChangeGesture();
        par->setValueNotifyingHost (par->getValue() > 0.5f ? 0.0f : 1.0f);
        par->endChangeGesture();
        aktiv = -1;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (aktiv < 0 || traffar[(size_t) aktiv].typ != Typ::ratt) return;
        if (auto* par = parameter (traffar[(size_t) aktiv].pid))
        {
            const float kanslighet = e.mods.isShiftDown() ? 1000.0f : 200.0f;
            par->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, startVarde + (startY - e.position.y) / kanslighet));
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (aktiv >= 0 && traffar[(size_t) aktiv].typ == Typ::ratt)
            if (auto* par = parameter (traffar[(size_t) aktiv].pid)) par->endChangeGesture();
        aktiv = -1;
    }

    void mouseDoubleClick (const juce::MouseEvent& e) override
    {
        const int i = hitta (e.position);
        if (i < 0 || traffar[(size_t) i].typ != Typ::ratt) return;
        if (auto* par = parameter (traffar[(size_t) i].pid))
        {
            par->beginChangeGesture();
            par->setValueNotifyingHost (par->getDefaultValue());
            par->endChangeGesture();
        }
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        const int i = hitta (e.position);
        if (i < 0) return;
        auto& h = traffar[(size_t) i];
        auto* par = parameter (h.pid);
        if (par == nullptr) return;
        const float d = std::abs (w.deltaY) > 0.0f ? w.deltaY : -w.deltaX;
        if (h.typ == Typ::ratt)
        {
            par->beginChangeGesture();
            par->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, par->getValue() + d * (e.mods.isShiftDown() ? 0.01f : 0.05f)));
            par->endChangeGesture();
        }
        else if (h.typ == Typ::valjare && std::abs (d) > 0.0f)
            stegaValjare (*par, d > 0 ? 1 : -1, false);
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const int i = hitta (e.position);
        setMouseCursor (i >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
    }

    // ------------------------------------------------------------------------------------------
    // För PRSnapshot (klicktestet och prestandamätningen).

    enum class Typ { ratt, valjare, brytare, knapp };
    struct Traff { Rectangle<float> yta; Typ typ; juce::String pid; int index = 0; };
    const std::vector<Traff>& allaTraffar() const noexcept { return traffar; }
    // Om träffytan går att klicka just nu (Grindens steg syns bara i TAKT, sidokedjan bara utanför).
    bool synlig (int i) const
    {
        const auto& h = traffar[(size_t) i];
        return hitta (h.yta.getCentre()) == i;
    }
    // Läser om parametrarna och ritar om bakgrunden vid nästa paint (som när en parameter ändras).
    void lasOmAllt() { uppdateraTillstand(); repaint(); }

private:

    static juce::StringArray rattId (int modul)
    {
        switch (modul)
        {
            case 0: return { id::grindTroskel, id::grindHall, id::grindSlapp, id::grindDjup };
            case 1: return { id::pressDriv, id::pressTon, id::pressTyngd, id::pressNiva };
            case 2: return { id::platStamning, id::platKlang, id::platLjushet, id::platBlandning };
            case 3: return { id::megaBand, id::megaResonans, id::megaSmuts, id::megaBlandning };
            case 4: return { id::schaktAterk, id::schaktMorker, id::schaktSvaj, id::schaktBlandning };
            default: return { id::hallStorlek, id::hallHalltid, id::hallFall, id::hallBlandning };
        }
    }
    static const char* valjarId (int m) { const char* v[] { id::grindLage, id::pressLage, id::platMaterial, id::megaLage, id::schaktTid, id::hallLokal }; return v[m]; }
    static const char* paId (int m)     { const char* v[] { id::grindPa, id::pressPa, id::platPa, id::megaPa, id::schaktPa, id::hallPa }; return v[m]; }
    static const char* extraId (int m)  { const char* v[] { id::grindSidokedja, id::pressOver4, id::platFore, id::megaBrus, id::schaktPingpong, id::hallGrind }; return v[m]; }

    void byggTraffar()
    {
        using namespace panel;
        auto cirkel = [] (Point<float> c, float r) { return Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c); };
        for (int m = 0; m < 6; ++m)
        {
            const auto ids = rattId (m);
            for (int j = 0; j < 4; ++j) traffar.push_back ({ cirkel (geo::ratt (m, j), geo::rattR * 1.3f), Typ::ratt, ids[j] });
            traffar.push_back ({ cirkel (geo::valjare (m), geo::valjareR * 1.4f), Typ::valjare, valjarId (m) });
            traffar.push_back ({ Rectangle<float> (26.0f, 48.0f).withCentre (geo::huvudVippa (m)), Typ::brytare, paId (m) });
            if (m == 0)   // stegen ligger över extrabrytaren och väljs bort i hitta() när de inte syns
                for (int s = 0; s < 16; ++s) traffar.push_back ({ geo::steg (0, s).expanded (1.0f, 2.0f), Typ::brytare, id::grindSteg (s), 100 + s });
            traffar.push_back ({ Rectangle<float> (56.0f, 24.0f).withCentre (geo::extraVippa (m)), Typ::brytare, extraId (m), m == 0 ? 99 : 0 });
            if (m == 2) traffar.push_back ({ cirkel (geo::midiKnapp (2), 11.0f), Typ::brytare, id::platMidi });
        }
        traffar.push_back ({ cirkel (geo::trycket(), geo::trycketR * 1.3f), Typ::ratt, id::trycket });
        traffar.push_back ({ cirkel (geo::manometer(), geo::manometerR), Typ::ratt, id::trycket });
        traffar.push_back ({ cirkel (geo::nodstopp(), geo::nodstoppR * 0.65f), Typ::brytare, id::nodstopp });
        traffar.push_back ({ cirkel (geo::midiIn(), 12.0f), Typ::brytare, id::midiIn });
        traffar.push_back ({ cirkel (geo::limiterLampa(), 12.0f), Typ::brytare, id::limiter });
        const char* ut[] { id::inNiva, id::torrVat, id::utNiva };
        for (int i = 0; i < 3; ++i) traffar.push_back ({ cirkel (geo::utgang (i), geo::utgangR * 1.3f), Typ::ratt, ut[i] });
        for (int i = 0; i < 4; ++i) traffar.push_back ({ cirkel (geo::knapp (i), geo::knappR * 1.3f), Typ::knapp, {}, i });
        // Räkneverket och Dymo-remsan: programlistan.
        traffar.push_back ({ geo::rakneverk().getUnion (geo::dymo()).expanded (4.0f), Typ::knapp, {}, 4 });
    }

    int hitta (Point<float> p) const
    {
        const bool takt = t.moduler[0].visaSteg;
        for (size_t i = 0; i < traffar.size(); ++i)
        {
            const auto& h = traffar[i];
            if (h.index >= 100 && ! takt) continue;      // Grindens steg syns bara i TAKT
            if (h.index == 99 && takt) continue;         // … och då är sidokedjebrytaren dold
            if (h.yta.contains (p)) return (int) i;
        }
        return -1;
    }

    juce::RangedAudioParameter* parameter (const juce::String& pid) const { return pid.isEmpty() ? nullptr : proc.apvts.getParameter (pid); }

    void stegaValjare (juce::RangedAudioParameter& par, int riktning, bool runt)
    {
        const int antal = juce::jmax (2, par.getNumSteps());
        int nu = (int) std::lround (par.convertFrom0to1 (par.getValue()) - par.getNormalisableRange().start);
        nu += riktning;
        if (runt) nu = (nu + antal) % antal; else nu = juce::jlimit (0, antal - 1, nu);
        par.beginChangeGesture();
        par.setValueNotifyingHost (par.convertTo0to1 (par.getNormalisableRange().start + (float) nu));
        par.endChangeGesture();
    }

    void valjarLista (juce::RangedAudioParameter& par)
    {
        juce::PopupMenu meny;
        const int antal = juce::jmax (2, par.getNumSteps());
        const int nu = (int) std::lround (par.convertFrom0to1 (par.getValue()));
        for (int i = 0; i < antal; ++i)
            meny.addItem (i + 1, par.getText (par.convertTo0to1 ((float) i), 0), true, i == nu);
        auto* p = &par;
        meny.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [p] (int val)
        {
            if (val <= 0) return;
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) (val - 1)));
            p->endChangeGesture();
        });
    }

    void tryckKnapp (int i)
    {
        auto& bank = proc.programbank;
        if (i == 0 || i == 1) { bank.stega (i == 0 ? -1 : 1); andrad.store (true); return; }
        if (i == 2) { sparaDialog(); return; }
        if (i == 4) { programLista(); return; }
        juce::PopupMenu storlekar;
        for (int s : { 60, 70, 85, 100, 115, 130, 160 })
            storlekar.addItem (1000 + s, juce::String (s) + " %", true, std::abs (storlek * 100.0f - (float) s) < 1.0f);
        juce::PopupMenu meny;
        meny.addSectionHeader ("Pannrummet PR-89");
        meny.addSubMenu ("Storlek", storlekar);
        meny.addItem (2, svt ("Visa mappen med egna program"));
        meny.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this] (int val)
        {
            if (val >= 1000 && onStorlek) onStorlek ((float) (val - 1000) / 100.0f);
            if (val == 2)
            {
                proc.programbank.egenMapp().createDirectory();
                proc.programbank.egenMapp().revealToUser();
            }
        });
    }

    // Programlistan, en undermeny per kategori.
    void programLista()
    {
        auto& bank = proc.programbank;
        bank.lasEgna();
        juce::PopupMenu meny;
        for (const auto& kat : bank.kategorier())
        {
            juce::PopupMenu under;
            for (int i = 0; i < bank.antal(); ++i)
                if (bank.program (i).kategori == kat)
                    under.addItem (i + 1, juce::String (i + 1).paddedLeft ('0', 2) + "  " + bank.program (i).namn, true, i == bank.aktuellt());
            meny.addSubMenu (kat, under);
        }
        meny.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [this] (int val)
        {
            if (val > 0) { proc.programbank.ladda (val - 1); andrad.store (true); }
        });
    }

    // SPARA: namnet frågas efter i en liten ruta; programmet hamnar under "Egna".
    void sparaDialog()
    {
        auto* ruta = new juce::AlertWindow (svt ("Spara program"), svt ("Namn på programmet (sparas under Egna):"), juce::MessageBoxIconType::NoIcon, this);
        const auto namn = proc.programbank.aktuelltNamn();
        ruta->addTextEditor ("namn", proc.programbank.program (proc.programbank.aktuellt()).fabrik() ? namn + " 2" : namn);
        ruta->addButton ("Spara", 1, juce::KeyPress (juce::KeyPress::returnKey));
        ruta->addButton ("Avbryt", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        ruta->enterModalState (true, juce::ModalCallbackFunction::create ([this, ruta] (int val)
        {
            if (val == 1)
            {
                const auto n = ruta->getTextEditorContents ("namn");
                if (! proc.programbank.spara (n))
                    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, "Pannrummet",
                                                            svt ("Kunde inte spara programmet i ") + proc.programbank.egenMapp().getFullPathName());
                andrad.store (true);
            }
        }), true);
    }

    // ------------------------------------------------------------------------------------------
    // Tillståndet

    float varde01 (const char* pid) const { auto* p = parameter (pid); return p ? p->getValue() : 0.0f; }
    float rattVarde (const juce::String& pid) const { auto* p = parameter (pid); return p ? p->getValue() : 0.0f; }
    bool pa (const juce::String& pid) const { return rattVarde (pid) > 0.5f; }
    int lage (const char* pid) const { auto* p = parameter (pid); return p ? (int) std::lround (p->convertFrom0to1 (p->getValue())) : 0; }
    juce::String text (const juce::String& pid) const
    {
        auto* p = parameter (pid);
        return p ? krang::ui::formatValue (*p, p->convertFrom0to1 (p->getValue())).toUpperCase() : juce::String();
    }

    void uppdateraTillstand()
    {
        for (int m = 0; m < 6; ++m)
        {
            auto mod = panel::grundModul (m);
            const auto ids = rattId (m);
            for (int j = 0; j < 4; ++j) { mod.rattar[(size_t) j].varde = rattVarde (ids[j]); mod.rattar[(size_t) j].text = text (ids[j]); }
            mod.lage = lage (valjarId (m));
            mod.pa = pa (paId (m));
            mod.vippa = pa (extraId (m));
            t.moduler[(size_t) m] = mod;
        }
        auto& g = t.moduler[0];
        g.visaSteg = g.lage == 2;
        for (int s = 0; s < 16; ++s) g.steg[(size_t) s] = pa (id::grindSteg (s));
        auto& pl = t.moduler[2];
        pl.midiPa = pa (id::platMidi) && pa (id::midiIn);   // lyser bara när MIDI faktiskt kommer fram
        if (pl.midiPa && proc.plattTon.load() >= 0) pl.rattar[0].text = "MIDI " + tonnamn (proc.plattTon.load());
        t.trycket = rattVarde (id::trycket); t.trycketText = text (id::trycket);
        t.forbi = pa (id::nodstopp);
        t.midiIn = pa (id::midiIn);
        t.limiterPa = pa (id::limiter);
        t.utgang = { panel::Ratt { svt ("IN-NIVÅ"), rattVarde (id::inNiva), text (id::inNiva) },
                     panel::Ratt { svt ("TORR/VÅT"), rattVarde (id::torrVat), text (id::torrVat) },
                     panel::Ratt { svt ("UTNIVÅ"), rattVarde (id::utNiva), text (id::utNiva) } };
        {
            const auto& bank = proc.programbank;
            const auto& p = bank.program (bank.aktuellt());
            t.programNr = juce::String (bank.aktuellt() + 1).paddedLeft ('0', 2).getLastCharacters (2);
            t.programNamn = p.namn.toUpperCase();
            t.kategori = p.kategori.toUpperCase();
            sistaProgram = bank.aktuellt();
        }
        sistaTon = proc.plattTon.load();
        cacheSmutsig = true;
    }

    void parameterChanged (const juce::String&, float) override { andrad.store (true); }

    void timerCallback() override
    {
        if (andrad.exchange (false) || proc.plattTon.load() != sistaTon || proc.programbank.aktuellt() != sistaProgram)
        {
            uppdateraTillstand();
            repaint();
        }
        // Visarna: snabbt upp, långsamt ned (ungefär som ett VU-instrument).
        auto visare = [] (float& v, float mal) { v += (mal - v) * (mal > v ? 0.45f : 0.12f); };
        visare (inVisning, proc.inNivaDb.exchange (-120.0f));
        visare (utVisning, proc.utNivaDb.exchange (-120.0f));
        const float lim = juce::jlimit (0.0f, 1.0f, -proc.limiterDb.exchange (0.0f) / 6.0f);
        limVisning = juce::jmax (lim, limVisning * 0.8f);
        auto& gm = t.moduler[0];
        const float drift = 0.25f + 0.75f * proc.grindOppning.load();
        const int stegNu = proc.grindSteg.load();
        const bool andrat = std::abs (inVisning - t.inDb) > 0.05f || std::abs (utVisning - t.utDb) > 0.05f
                         || std::abs (limVisning - t.limiter) > 0.01f || std::abs (drift - gm.drift) > 0.01f || stegNu != gm.stegNu;
        t.inDb = inVisning; t.utDb = utVisning; t.limiter = limVisning; gm.drift = drift; gm.stegNu = stegNu;
        if (andrat)
            for (auto r : panel::dynamiskaYtor()) repaint (r.getSmallestIntegerContainer());
    }

    PannrummetProcessor& proc;
    Ytor ytor;
    panel::Tillstand t;
    juce::Image cache;
    float cacheSkala = 0.0f;
    bool cacheSmutsig = true;
    std::atomic<bool> andrad { false };
    std::vector<Traff> traffar;
    int aktiv = -1, sistaTon = -1, sistaProgram = -1;
    float startVarde = 0.0f, startY = 0.0f;
    float inVisning = -120.0f, utVisning = -120.0f, limVisning = 0.0f;
};

} // namespace pr::ui
