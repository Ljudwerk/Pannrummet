// PRRender – kör ljud genom Pannrummet utan DAW, och testar kedjan.
//
//   PRRender --test                         testfallen (NaN, topp, DC, tystnad, bitexakt förbikoppling, latens)
//   PRRender in.wav ut.wav [bpm] [id=värde ...]
//        renderar en fil; värdena är parametrarnas riktiga värden (dB, ms, 0..1, läge som index, 0/1)
//   PRRender --exempel inmapp utmapp        grind 1- och grind 2-exemplen (se README)
//   PRRender --program inmapp utmapp        alla fabriksprogram, vart och ett på ett ljud som passar kategorin
//   PRRender --parametrar fil.txt           parametrarnas index, id och intervall (för Reaper-skriptet)
//   PRRender --state fil.xml [id=värde ...] pluginets läge som XML
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <map>

namespace
{
using namespace pr;

// En spelhuvud-attrapp: transporten går i givet tempo från takt 1.
struct Spelhuvud : juce::AudioPlayHead
{
    double bpm = 120.0, ppq = 0.0;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p;
        p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (true);
        return p;
    }
};

void satt (PannrummetProcessor& p, const juce::String& pid, float varde)
{
    auto* par = p.apvts.getParameter (pid);
    if (par == nullptr) { std::cout << "  okänd parameter: " << pid << "\n"; return; }
    par->setValueNotifyingHost (par->convertTo0to1 (varde));
}

using Installning = std::vector<std::pair<juce::String, float>>;

// Kör en buffert genom en ny processor med inställningen. Returnerar utgången (samma längd + svans).
juce::AudioBuffer<float> kor (const juce::AudioBuffer<float>& in, double fs, double bpm, const Installning& inst,
                              int svans = 0, int* latens = nullptr, int block = 512)
{
    PannrummetProcessor p;
    p.setPlayConfigDetails (in.getNumChannels(), in.getNumChannels(), fs, block);
    for (auto& [k, v] : inst) satt (p, k, v);
    Spelhuvud ph; ph.bpm = bpm;
    p.setPlayHead (&ph);
    p.prepareToPlay (fs, block);
    if (latens) *latens = p.getLatencySamples();
    const int n = in.getNumSamples() + svans;
    juce::AudioBuffer<float> ut (in.getNumChannels(), n);
    ut.clear();
    for (int k = 0; k < in.getNumChannels(); ++k) ut.copyFrom (k, 0, in, k, 0, in.getNumSamples());
    juce::MidiBuffer midi;
    for (int s = 0; s < n; s += block)
    {
        const int len = std::min (block, n - s);
        juce::AudioBuffer<float> bit (ut.getArrayOfWritePointers(), ut.getNumChannels(), s, len);
        p.processBlock (bit, midi);
        ph.ppq += ph.bpm / 60.0 / fs * len;
    }
    return ut;
}

bool lasWav (const juce::File& f, juce::AudioBuffer<float>& buf, double& fs)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (! r) { std::cout << "Kunde inte läsa " << f.getFullPathName() << "\n"; return false; }
    buf.setSize ((int) juce::jmin (2u, r->numChannels), (int) r->lengthInSamples);
    r->read (&buf, 0, (int) r->lengthInSamples, 0, true, true);
    fs = r->sampleRate;
    return true;
}

bool skrivWav (const juce::File& f, const juce::AudioBuffer<float>& buf, double fs)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> ut (f.createOutputStream());
    if (! ut) return false;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (ut.get(), fs, (unsigned) buf.getNumChannels(), 24, {}, 0));
    if (! w) return false;
    ut.release();
    return w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
}

// --------------------------------------------------------------------------------------------
// Testfallen

struct Matt { float topp = 0, dc = 0; bool nan = false; };
Matt mat (const juce::AudioBuffer<float>& b, int fran = 0)
{
    Matt m;
    for (int k = 0; k < b.getNumChannels(); ++k)
    {
        double summa = 0.0;
        for (int i = fran; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (k, i);
            if (! std::isfinite (v)) m.nan = true;
            m.topp = std::max (m.topp, std::abs (v));
            summa += v;
        }
        m.dc = std::max (m.dc, (float) std::abs (summa / std::max (1, b.getNumSamples() - fran)));
    }
    return m;
}

int fel = 0;
// Text som byggts av UTF-8-literaler via juce::String (const char*) blir dubbelkodad; laga den här.
std::string lagaUtf8 (const juce::String& s)
{
    std::string ut;
    for (auto p = s.getCharPointer(); ! p.isEmpty();)
    {
        const auto c = p.getAndAdvance();
        if (c < 256) ut.push_back ((char) c);
        else ut += juce::String::charToString (c).toStdString();
    }
    return juce::String::fromUTF8 (ut.c_str()).isEmpty() ? s.toStdString() : (juce::CharPointer_UTF8::isValidString (ut.c_str(), (int) ut.size()) ? ut : s.toStdString());
}

void kontroll (bool ok, const juce::String& vad)
{
    std::cout << (ok ? "  OK    " : "  FEL   ") << lagaUtf8 (vad) << "\n";
    if (! ok) ++fel;
}

juce::AudioBuffer<float> provsignal (double fs, float sek)
{
    // Brus i skurar, en sågtandsbas och ett par klick – ett grovt "musikaliskt" prov.
    juce::AudioBuffer<float> b (2, (int) (fs * sek));
    krang::dsp::Brus br;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / fs;
        const float skur = std::fmod (t, 0.48) < 0.12 ? 0.5f : 0.0f;
        const float saw = (float) (std::fmod (t * 55.0, 1.0) * 2.0 - 1.0) * 0.4f;
        const float klick = (i % (int) (fs * 0.25)) < 3 ? 0.9f : 0.0f;
        const float v = br.vitt() * skur + saw + klick;
        b.setSample (0, i, v * 0.45f);   // under limiterns tak, så att genomsläppet kan vara bitexakt
        b.setSample (1, i, v * 0.4f + br.vitt() * 0.03f);
    }
    return b;
}

int test()
{
    const double fs = 48000.0;
    auto prov = provsignal (fs, 3.0f);
    const int svans = (int) fs;

    std::cout << "Genomsläpp och förbikoppling\n";
    {
        int lat = 0;
        auto ut = kor (prov, fs, 120.0, {}, 0, &lat);
        bool lika = true;
        for (int k = 0; k < 2; ++k)
            for (int i = lat; i < prov.getNumSamples(); ++i)
                if (! juce::exactlyEqual (ut.getSample (k, i), prov.getSample (k, i - lat)))
                {
                    if (lika) std::cout << "  första skillnaden: kanal " << k << ", sample " << i << ": " << ut.getSample (k, i) << " mot " << prov.getSample (k, i - lat) << "\n";
                    lika = false; break;
                }
        kontroll (lika, "alla moduler av: bitexakt genomsläpp (latens " + juce::String (lat) + " sample)");
        auto ut2 = kor (prov, fs, 120.0, { { id::nodstopp, 1 }, { id::grindPa, 1 }, { id::pressPa, 1 }, { id::megaPa, 1 }, { id::pressDriv, 1 } }, 0);
        lika = true;
        for (int k = 0; k < 2; ++k)
            for (int i = lat; i < prov.getNumSamples(); ++i)
                if (! juce::exactlyEqual (ut2.getSample (k, i), prov.getSample (k, i - lat))) { lika = false; break; }
        kontroll (lika, "nödstopp: bitexakt förbikoppling med allt påslaget");
    }

    std::cout << "Latens\n";
    {
        juce::AudioBuffer<float> imp (2, 4096); imp.clear(); imp.setSample (0, 100, 0.5f); imp.setSample (1, 100, 0.5f);
        for (int lage = 0; lage < 3; ++lage)
            for (int o4 = 0; o4 < 2; ++o4)
            {
                int lat = 0;
                auto ut = kor (imp, fs, 120.0, { { id::pressPa, 1 }, { id::pressLage, (float) lage }, { id::pressDriv, 0.0f }, { id::pressOver4, (float) o4 } }, 0, &lat);
                int bast = 0;
                for (int i = 0; i < ut.getNumSamples(); ++i) if (std::abs (ut.getSample (0, i)) > std::abs (ut.getSample (0, bast))) bast = i;
                kontroll (std::abs (bast - (100 + lat)) <= 2, "Pressen läge " + juce::String (lage) + (o4 ? " 4×" : " 2×") + ": impulsen vid " + juce::String (bast - 100) + " (rapporterat " + juce::String (lat) + ")");
            }
    }

    std::cout << "Modulerna\n";
    struct Fall { const char* namn; Installning inst; };
    std::vector<Fall> fall;
    for (int l = 0; l < 3; ++l)
        fall.push_back ({ "Grinden", { { id::grindPa, 1 }, { id::grindLage, (float) l }, { id::grindTroskel, -20 } } });
    for (int l = 0; l < 4; ++l)
        for (float d : { 0.0f, 0.5f, 1.0f })
            fall.push_back ({ "Pressen", { { id::pressPa, 1 }, { id::pressLage, (float) l }, { id::pressDriv, d }, { id::pressTyngd, d }, { id::pressTon, d } } });
    for (int l = 0; l < 4; ++l)
        for (float r : { 0.0f, 1.0f })
            fall.push_back ({ "Megafonen", { { id::megaPa, 1 }, { id::megaLage, (float) l }, { id::megaResonans, r }, { id::megaSmuts, r }, { id::megaBand, r > 0 ? 4500.0f : 250.0f } } });
    // Moduler med svans: svanstestet görs inte (rummet och ekot ska klinga), men allt annat.
    std::vector<Fall> svansFall;
    for (int l = 0; l < 5; ++l)
        for (float k : { 0.05f, 8.0f })
            svansFall.push_back ({ "Plåten", { { id::platPa, 1 }, { id::platMaterial, (float) l }, { id::platKlang, k }, { id::platLjushet, k > 1 ? 1.0f : 0.0f }, { id::platBlandning, 1 }, { id::platStamning, k > 1 ? 84.0f : 24.0f } } });
    for (int t = 0; t < 7; t += 3)
        for (float a : { 0.0f, 95.0f })
            svansFall.push_back ({ "Schaktet", { { id::schaktPa, 1 }, { id::schaktTid, (float) t }, { id::schaktAterk, a }, { id::schaktSvaj, 1 }, { id::schaktBlandning, 1 }, { id::schaktPingpong, a > 0 ? 1.0f : 0.0f } } });
    for (int l = 0; l < 4; ++l)
        for (float g : { 0.0f, 1.0f })
            svansFall.push_back ({ "Hallen", { { id::hallPa, 1 }, { id::hallLokal, (float) l }, { id::hallStorlek, 1 }, { id::hallBlandning, 1 }, { id::hallGrind, g } } });
    svansFall.push_back ({ "Allt + Trycket 100 %", { { id::grindPa, 1 }, { id::pressPa, 1 }, { id::platPa, 1 }, { id::megaPa, 1 }, { id::schaktPa, 1 }, { id::hallPa, 1 }, { id::trycket, 1 }, { id::inNiva, 12 } } });
    fall.push_back ({ "Alla + in +24 dB", { { id::grindPa, 1 }, { id::pressPa, 1 }, { id::megaPa, 1 }, { id::inNiva, 24 }, { id::pressDriv, 1 }, { id::utNiva, 12 } } });

    juce::AudioBuffer<float> tyst (2, (int) fs * 2); tyst.clear();
    for (auto& f : fall)
    {
        juce::String namn = f.namn;
        for (auto& [k, v] : f.inst) if (k != id::grindPa && k != id::pressPa && k != id::megaPa) namn << " " << k.fromLastOccurrenceOf ("_", false, false) << "=" << v;
        auto ut = kor (prov, fs, 120.0, f.inst, svans);
        const auto m = mat (ut);
        const auto mSvans = mat (ut, prov.getNumSamples() + svans / 2);
        kontroll (! m.nan && m.topp <= 1.0f && m.dc < 0.01f && mSvans.topp < 1.0e-3f,
                  namn + ": topp " + juce::String (m.topp, 3) + ", dc " + juce::String (m.dc, 4) + ", svans " + juce::String (krang::dsp::gainTillDb (mSvans.topp), 0) + " dB");
        auto uts = kor (tyst, fs, 120.0, f.inst);
        kontroll (mat (uts).topp < 1.0e-6f, namn + ": tystnad in ger tystnad ut");
    }
    for (auto& f : svansFall)
    {
        juce::String namn = f.namn;
        for (auto& [k, v] : f.inst) if (! k.endsWith ("_pa")) namn << " " << k.fromLastOccurrenceOf ("_", false, false) << "=" << v;
        auto ut = kor (prov, fs, 120.0, f.inst, svans);
        const auto m = mat (ut);
        const auto mVat = mat (ut, prov.getNumSamples() / 2);
        kontroll (! m.nan && m.topp <= 1.0f && m.dc < 0.01f && mVat.topp > 1.0e-3f,
                  namn + ": topp " + juce::String (m.topp, 3) + ", dc " + juce::String (m.dc, 4));
        auto uts = kor (tyst, fs, 120.0, f.inst);
        kontroll (mat (uts).topp < 1.0e-6f, namn + ": tystnad in ger tystnad ut");
    }
    {
        // Hallens grind: efter ett slag ska rummet vara tyst när hålltid + fall har gått.
        juce::AudioBuffer<float> slag (2, (int) fs); slag.clear();
        krang::dsp::Brus br;
        for (int i = 0; i < 2400; ++i) { const float v = br.vitt() * 0.5f * (1.0f - i / 2400.0f); slag.setSample (0, i, v); slag.setSample (1, i, v); }
        int lat = 0;
        auto ut = kor (slag, fs, 120.0, { { id::hallPa, 1 }, { id::hallBlandning, 1 }, { id::hallStorlek, 1 }, { id::hallHalltid, 150 }, { id::hallFall, 20 } }, 0, &lat);
        const auto oppen = mat (ut, lat + 2400).topp;
        juce::AudioBuffer<float> sen (ut.getArrayOfWritePointers(), 2, lat + (int) (fs * 0.4), (int) (fs * 0.2));
        kontroll (oppen > 0.01f && mat (sen).topp < 1.0e-4f, "Hallen grind: rummet klingar (" + juce::String (oppen, 3) + ") och stängs efter hålltid + fall");
    }
    {
        auto ut = kor (tyst, fs, 120.0, { { id::megaPa, 1 }, { id::megaBrus, 1 }, { id::megaLage, 2 } });
        const auto m = mat (ut);
        kontroll (! m.nan && m.topp < 0.1f, "Megafonen brus i tystnad: topp " + juce::String (krang::dsp::gainTillDb (m.topp), 1) + " dB");
    }
    {
        // Grindens taktläge: åttondelar i 120 BPM ska ge ljud på steg 1, 3, 5 … och tystnad emellan.
        juce::AudioBuffer<float> ton (2, (int) fs * 2);
        for (int i = 0; i < ton.getNumSamples(); ++i) { const float v = 0.5f * std::sin ((float) i * 0.05f); ton.setSample (0, i, v); ton.setSample (1, i, v); }
        int lat = 0;
        auto ut = kor (ton, fs, 120.0, { { id::grindPa, 1 }, { id::grindLage, 2 }, { id::grindHall, 60 }, { id::grindSlapp, 5 } }, 0, &lat);
        const int sext = (int) (fs * 0.125);
        auto rms = [&] (int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += ut.getSample (0, i) * ut.getSample (0, i); return std::sqrt (s / (b - a)); };
        const double pa = rms (lat + 200, lat + 2000), av = rms (lat + sext + 200, lat + 2 * sext - 200);
        kontroll (pa > 0.2 && av < 1.0e-4, "Grinden takt: öppen " + juce::String (pa, 3) + ", stängd " + juce::String (av, 6));
    }
    std::cout << "Fabriksbandet\n";
    {
        const auto band = fabriksbandet();
        kontroll (band.size() == 30, "30 fabriksprogram (" + juce::String ((int) band.size()) + ")");
        juce::StringArray namn;
        for (auto& p : band)
        {
            namn.addIfNotAlreadyThere (p.namn);
            Installning inst (p.varden.begin(), p.varden.end());
            bool kantId = true;
            {
                PannrummetProcessor q;
                for (auto& [k, v] : inst) if (q.apvts.getParameter (k) == nullptr) kantId = false;
            }
            auto ut = kor (prov, fs, 120.0, inst, svans);
            const auto m = mat (ut);
            kontroll (kantId && ! m.nan && m.topp <= 1.0f && m.dc < 0.01f, p.namn + ": topp " + juce::String (m.topp, 3) + ", dc " + juce::String (m.dc, 4));
            bool brus = false;
            for (auto& [k, v] : inst) if (k == id::megaBrus && v > 0.5f) brus = true;
            if (! brus) kontroll (mat (kor (tyst, fs, 120.0, inst)).topp < 1.0e-6f, p.namn + ": tystnad in ger tystnad ut");
        }
        kontroll (namn.size() == (int) band.size(), "programnamnen är unika");
    }
    {
        // Programbanken: ladda, spara och läsa tillbaka ett eget program (i en tillfällig mapp).
        auto mapp = juce::File::createTempFile ("prprog");
        PannrummetProcessor p;
        krang::presets::Programbank bank (p.apvts, "Pannrummet", "prprog", fabriksbandet(), { id::nodstopp, id::midiIn }, mapp);
        satt (p, id::nodstopp, 1);
        bank.ladda (1);
        const bool laddat = p.apvts.getRawParameterValue (id::hallPa)->load() > 0.5f && p.apvts.getRawParameterValue (id::nodstopp)->load() > 0.5f;
        satt (p, id::hallBlandning, 0.77f);
        const bool sparat = bank.spara ("Mitt test");
        bank.ladda (0);
        const bool init = p.apvts.getRawParameterValue (id::hallPa)->load() < 0.5f;
        int mitt = -1;
        for (int i = 0; i < bank.antal(); ++i) if (bank.program (i).namn == "Mitt test") mitt = i;
        if (mitt >= 0) bank.ladda (mitt);
        const float bl = p.apvts.getRawParameterValue (id::hallBlandning)->load();
        kontroll (laddat && sparat && init && mitt == bank.antalFabrik() && std::abs (bl - 0.77f) < 1.0e-4f,
                  "programbanken: ladda, spara och läsa tillbaka (nödstoppet rörs inte)");
        mapp.deleteRecursively();
    }
    {
        // MIDI in: av (grundläget) = tonerna struntas i; på = Plåten får tonen.
        for (int midiPa = 0; midiPa < 2; ++midiPa)
        {
            PannrummetProcessor p;
            p.setPlayConfigDetails (2, 2, fs, 512);
            satt (p, id::midiIn, (float) midiPa);
            p.prepareToPlay (fs, 512);
            juce::AudioBuffer<float> b (2, 512); b.clear();
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            p.processBlock (b, m);
            kontroll (p.plattTon.load() == (midiPa ? 60 : -1), juce::String ("MIDI in ") + (midiPa ? "på: Plåten får tonen" : "av: tonen struntas i"));
        }
    }
    std::cout << (fel == 0 ? "Alla test gick igenom.\n" : juce::String (fel) + " test misslyckades.\n");
    return fel == 0 ? 0 : 1;
}

// --------------------------------------------------------------------------------------------
// Grind 1-exemplen

struct Exempel { const char* fil; const char* ut; double bpm; Installning inst; };

std::vector<Exempel> exempel()
{
    return {
        { "grind0_kolv125_trummor.wav", "1a_trummor_grind_hard_press", 125.0,
          { { id::grindPa, 1 }, { id::grindLage, 1 }, { id::grindTroskel, -28 }, { id::grindHall, 30 }, { id::grindSlapp, 45 },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.55f }, { id::pressTyngd, 0.5f }, { id::pressTon, 0.55f } } },
        { "grind1_ebm_128.wav", "1b_helmix_grind_takt", 128.0,
          { { id::grindPa, 1 }, { id::grindLage, 2 }, { id::grindHall, 70 }, { id::grindSlapp, 30 },
            { id::grindSteg (1), 0 }, { id::grindSteg (3), 1 }, { id::grindSteg (5), 0 }, { id::grindSteg (6), 1 }, { id::grindSteg (11), 1 }, { id::grindSteg (13), 0 },
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.35f } } },
        { "grind0_kolv125_bas.wav", "2a_bas_press_ror", 125.0,
          { { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.65f }, { id::pressTon, 0.6f }, { id::pressTyngd, 0.3f } } },
        { "grind0_kolv125_bas.wav", "2b_bas_press_likriktare", 125.0,
          { { id::pressPa, 1 }, { id::pressLage, 2 }, { id::pressDriv, 0.5f }, { id::pressTon, 0.45f }, { id::pressOver4, 1 } } },
        { "grind0_kolv125_bas.wav", "2c_bas_press_bitkross", 125.0,
          { { id::pressPa, 1 }, { id::pressLage, 3 }, { id::pressDriv, 0.6f }, { id::torrVat, 0.6f } } },
        { "rost.wav", "3a_rost_megafon", 120.0,
          { { id::megaPa, 1 }, { id::megaLage, 0 }, { id::megaBand, 1600 }, { id::megaResonans, 0.5f }, { id::megaSmuts, 0.5f } } },
        { "rost.wav", "3b_rost_telefon", 120.0,
          { { id::megaPa, 1 }, { id::megaLage, 1 }, { id::megaBand, 1200 }, { id::megaSmuts, 0.6f } } },
        { "rost.wav", "3c_rost_radio_brus", 120.0,
          { { id::megaPa, 1 }, { id::megaLage, 2 }, { id::megaBand, 1400 }, { id::megaSmuts, 0.3f }, { id::megaBrus, 1 } } },
        { "rost.wav", "3d_rost_tratt_press", 120.0,
          { { id::megaPa, 1 }, { id::megaLage, 3 }, { id::megaBand, 900 }, { id::megaResonans, 0.6f }, { id::megaSmuts, 0.5f },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.3f } } },
        // Grind 2: Plåten, Schaktet, Hallen och Trycket.
        { "grind0_kolv125_trummor.wav", "4a_trummor_platen_plat", 125.0,
          { { id::platPa, 1 }, { id::platMaterial, 0 }, { id::platStamning, 45 }, { id::platKlang, 1.5f }, { id::platLjushet, 0.6f }, { id::platBlandning, 0.4f } } },
        { "grind0_kolv125_trummor.wav", "4b_trummor_platen_klocka", 125.0,
          { { id::platPa, 1 }, { id::platMaterial, 3 }, { id::platStamning, 57 }, { id::platKlang, 3.0f }, { id::platLjushet, 0.5f }, { id::platBlandning, 0.35f } } },
        { "grind0_kolv125_trummor.wav", "4c_trummor_platen_ror_fore_pressen", 125.0,
          { { id::platPa, 1 }, { id::platMaterial, 1 }, { id::platStamning, 40 }, { id::platKlang, 0.8f }, { id::platBlandning, 0.5f }, { id::platFore, 1 },
            { id::pressPa, 1 }, { id::pressLage, 1 }, { id::pressDriv, 0.5f } } },
        { "grind0_kolv125_bas.wav", "4d_bas_platen_ring", 125.0,
          { { id::platPa, 1 }, { id::platMaterial, 4 }, { id::platStamning, 52 }, { id::platKlang, 0.3f }, { id::platLjushet, 0.3f }, { id::platBlandning, 0.5f } } },
        { "grind0_kolv125_bas.wav", "5a_bas_schaktet_punkterad", 125.0,
          { { id::schaktPa, 1 }, { id::schaktTid, 3 }, { id::schaktAterk, 50 }, { id::schaktMorker, 0.6f }, { id::schaktBlandning, 0.35f } } },
        { "rost.wav", "5b_rost_megafon_schaktet_pingpong", 120.0,
          { { id::megaPa, 1 }, { id::megaLage, 0 }, { id::megaSmuts, 0.5f },
            { id::schaktPa, 1 }, { id::schaktTid, 4 }, { id::schaktAterk, 60 }, { id::schaktMorker, 0.5f }, { id::schaktSvaj, 0.4f }, { id::schaktBlandning, 0.4f } } },
        { "grind0_kolv125_trummor.wav", "6a_trummor_hallen_gatad", 125.0,
          { { id::hallPa, 1 }, { id::hallLokal, 1 }, { id::hallStorlek, 0.8f }, { id::hallHalltid, 200 }, { id::hallFall, 30 }, { id::hallBlandning, 0.45f }, { id::hallGrind, 1 } } },
        { "grind0_kolv125_trummor.wav", "6b_trummor_hallen_valv_utan_grind", 125.0,
          { { id::hallPa, 1 }, { id::hallLokal, 2 }, { id::hallStorlek, 0.6f }, { id::hallBlandning, 0.35f }, { id::hallGrind, 0 } } },
        { "rost.wav", "6c_rost_hallen_gruva", 120.0,
          { { id::hallPa, 1 }, { id::hallLokal, 3 }, { id::hallStorlek, 0.8f }, { id::hallBlandning, 0.4f }, { id::hallGrind, 0 } } },
        { "grind1_ebm_128.wav", "7_helmix_allt_trycket", 128.0,
          { { id::grindPa, 1 }, { id::grindLage, 1 }, { id::grindTroskel, -40 },
            { id::pressPa, 1 }, { id::pressLage, 0 }, { id::pressDriv, 0.3f },
            { id::platPa, 1 }, { id::platMaterial, 0 }, { id::platStamning, 45 }, { id::platBlandning, 0.2f },
            { id::schaktPa, 1 }, { id::schaktTid, 3 }, { id::schaktBlandning, 0.2f },
            { id::hallPa, 1 }, { id::hallBlandning, 0.3f }, { id::trycket, 0.6f } } },
    };
}

// Varje fabriksprogram på ett ljud som passar kategorin, för lyssning.
int renderaProgram (const juce::File& inmapp, const juce::File& utmapp)
{
    utmapp.createDirectory();
    int nr = 0;
    for (auto& p : fabriksbandet())
    {
        ++nr;
        juce::String kalla = "grind1_ebm_128.wav"; double bpm = 128.0;
        if (p.kategori == "Trummor")       { kalla = "grind0_kolv125_trummor.wav"; bpm = 125.0; }
        else if (p.kategori == "EBM")      { kalla = nr <= 15 ? "grind0_kolv125_bas.wav" : "grind1_ebm_128.wav"; bpm = nr <= 15 ? 125.0 : 128.0; }
        else if (p.kategori == svt ("Röst")) { kalla = "rost.wav"; bpm = 120.0; }
        juce::AudioBuffer<float> in; double fs = 48000.0;
        if (! lasWav (inmapp.getChildFile (kalla), in, fs)) continue;
        int lat = 0;
        auto ut = kor (in, fs, bpm, Installning (p.varden.begin(), p.varden.end()), (int) fs, &lat);
        juce::AudioBuffer<float> rak (ut.getNumChannels(), ut.getNumSamples() - lat);
        for (int k = 0; k < ut.getNumChannels(); ++k) rak.copyFrom (k, 0, ut, k, lat, rak.getNumSamples());
        const auto fil = juce::String (nr).paddedLeft ('0', 2) + "_" + juce::File::createLegalFileName (p.namn).replaceCharacter (' ', '_') + ".wav";
        skrivWav (utmapp.getChildFile (fil), rak, fs);
        std::cout << lagaUtf8 (fil) << "  (" << kalla << ")\n";
    }
    return 0;
}

int renderaExempel (const juce::File& inmapp, const juce::File& utmapp)
{
    utmapp.createDirectory();
    for (auto& e : exempel())
    {
        juce::AudioBuffer<float> in; double fs = 48000.0;
        if (! lasWav (inmapp.getChildFile (e.fil), in, fs)) continue;
        int lat = 0;
        auto ut = kor (in, fs, e.bpm, e.inst, (int) fs, &lat);
        // Ta bort latensen, så att filerna ligger i fas med originalet.
        juce::AudioBuffer<float> rak (ut.getNumChannels(), ut.getNumSamples() - lat);
        for (int k = 0; k < ut.getNumChannels(); ++k) rak.copyFrom (k, 0, ut, k, lat, rak.getNumSamples());
        skrivWav (utmapp.getChildFile (juce::String (e.ut) + ".wav"), rak, fs);
        const auto m = mat (rak);
        std::cout << e.ut << ".wav  (" << e.fil << ", topp " << juce::String (krang::dsp::gainTillDb (m.topp), 1) << " dBFS)\n";
    }
    return 0;
}

} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI juce;
    if (argc < 2)
    {
        std::cout << "Användning: PRRender --test | --exempel inmapp utmapp | --parametrar fil | --state fil [id=värde ...] | in.wav ut.wav [bpm] [id=värde ...]\n";
        return 1;
    }
    const juce::String lage (argv[1]);
    auto fil = [] (const char* s) { return juce::File::getCurrentWorkingDirectory().getChildFile (s); };
    if (lage == "--test") return test();
    if (lage == "--exempel" && argc >= 4) return renderaExempel (fil (argv[2]), fil (argv[3]));
    if (lage == "--program" && argc >= 4) return renderaProgram (fil (argv[2]), fil (argv[3]));
    if (lage == "--parametrar" && argc >= 3)
    {
        // index, id, lägsta och högsta värde (för Reaper-automation: tools/reaper/gor_reaper_projekt.py)
        PannrummetProcessor p;
        juce::String ut;
        int i = 0;
        for (auto* par : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (par))
                ut << i++ << "\t" << r->getParameterID() << "\t" << r->getNormalisableRange().start << "\t" << r->getNormalisableRange().end << "\n";
        return fil (argv[2]).replaceWithText (ut) ? 0 : 1;
    }
    if (lage == "--state" && argc >= 3)
    {
        // Pluginets läge som XML (det som ligger i Reaper-projektet), med givna värden.
        PannrummetProcessor p;
        for (int i = 3; i < argc; ++i)
        {
            const juce::String a (argv[i]);
            satt (p, a.upToFirstOccurrenceOf ("=", false, false), a.fromFirstOccurrenceOf ("=", false, false).getFloatValue());
        }
        auto tillstand = p.apvts.copyState();
        tillstand.setProperty ("stFormat", PannrummetProcessor::formatVersion, nullptr);
        auto xml = tillstand.createXml();
        return fil (argv[2]).replaceWithText (xml->toString (juce::XmlElement::TextFormat().singleLine().withoutHeader())) ? 0 : 1;
    }
    if (argc < 3) return 1;

    juce::AudioBuffer<float> in; double fs = 48000.0;
    if (! lasWav (fil (argv[1]), in, fs)) return 1;
    double bpm = 120.0;
    Installning inst;
    for (int i = 3; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a.containsChar ('=')) inst.push_back ({ a.upToFirstOccurrenceOf ("=", false, false), a.fromFirstOccurrenceOf ("=", false, false).getFloatValue() });
        else bpm = a.getDoubleValue();
    }
    int lat = 0;
    auto ut = kor (in, fs, bpm, inst, (int) fs, &lat);
    juce::AudioBuffer<float> rak (ut.getNumChannels(), ut.getNumSamples() - lat);
    for (int k = 0; k < ut.getNumChannels(); ++k) rak.copyFrom (k, 0, ut, k, lat, rak.getNumSamples());
    return skrivWav (fil (argv[2]), rak, fs) ? 0 : 1;
}
