// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace pr;

namespace
{
    // dB till gain, där värden nära 0 dB blir exakt 1 (parametrarnas normalisering ger annars 1,0000001
    // och förstör det bitexakta genomsläppet).
    float gainAv (float dB) { return std::abs (dB) < 0.01f ? 1.0f : krang::dsp::dbTillGain (dB); }
}

// Bussarna: stereo in och ut, och en sidokedja – utom i AU. I AU blir sidokedjan ett extra
// ingångselement som värdar hanterar olika (Reaper visar fyra ingångsstift); utan den är Pannrummet en
// vanlig 2 in / 2 ut-effekt. Sidokedjan finns i VST3 och CLAP.
juce::AudioProcessor::BusesProperties PannrummetProcessor::bussar()
{
    auto b = BusesProperties()
                 .withInput  ("In", juce::AudioChannelSet::stereo(), true)
                 .withOutput ("Ut", juce::AudioChannelSet::stereo(), true);
    if (juce::PluginHostType::getPluginLoadedAs() != juce::AudioProcessor::wrapperType_AudioUnit)
        b = b.withInput ("Sidokedja", juce::AudioChannelSet::stereo(), true);   // på från början, så att Reaper visar kanal 3/4
    return b;
}

PannrummetProcessor::PannrummetProcessor()
    : AudioProcessor (bussar()),
      apvts (*this, nullptr, "Pannrummet", skapaLayout())
{
}

bool PannrummetProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto ut = layouts.getMainOutputChannelSet();
    if (ut != juce::AudioChannelSet::mono() && ut != juce::AudioChannelSet::stereo()) return false;
    if (ut != layouts.getMainInputChannelSet()) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sido = layouts.getChannelSet (true, 1);
        if (! sido.isDisabled() && sido != juce::AudioChannelSet::mono() && sido != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void PannrummetProcessor::prepareToPlay (double samplingsfrekvens, int)
{
    fs = samplingsfrekvens;
    kanalerForberedda = juce::jlimit (1, 2, getMainBusNumOutputChannels());
    grinden.forbered (fs);
    pressen.forbered (fs, bit, kanalerForberedda);
    megafonen.forbered (fs);
    platen.forbered (fs);
    schaktet.forbered (fs);
    hallen.forbered (fs);
    limitern.forbered (fs);
    latens = pressen.latens();
    for (int k = 0; k < 2; ++k)
    {
        torrFordr[(size_t) k].forbered (latens); torrFordr[(size_t) k].sattLangd (latens);
        raFordr[(size_t) k].forbered (latens);   raFordr[(size_t) k].sattLangd (latens);
    }
    torr.setSize (2, bit);
    ra.setSize (2, bit);
    inGain.reset (fs, 0.02);  inGain.setCurrentAndTargetValue (gainAv (las (id::inNiva)));
    utGain.reset (fs, 0.02);  utGain.setCurrentAndTargetValue (gainAv (las (id::utNiva)));
    blandning.reset (fs, 0.02); blandning.setCurrentAndTargetValue (las (id::torrVat));
    forbi.satt (pa (id::nodstopp) ? 1.0f : 0.0f);
    setLatencySamples (latens);
}

void PannrummetProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals ingaDenormaler;
    // MIDI in: av = Pannrummet bryr sig inte om några toner (Plåten följer ratten), och den senaste
    // tonen glöms, så att presetbyten och MIDI på spåret aldrig sätter igång något.
    if (pa (id::midiIn))
    {
        for (const auto m : midi)
        {
            const auto msg = m.getMessage();
            if (msg.isNoteOn()) plattTon.store (msg.getNoteNumber());
        }
    }
    else plattTon.store (-1);

    auto main = getBusBuffer (buffer, true, 0);
    const int kanaler = juce::jmin (kanalerForberedda, main.getNumChannels());
    for (int k = getTotalNumInputChannels(); k < getTotalNumOutputChannels(); ++k)
        buffer.clear (k, 0, buffer.getNumSamples());

    const float* sido[2] { nullptr, nullptr };
    int sidoKanaler = 0;
    if (getBusCount (true) > 1 && getBus (true, 1)->isEnabled())
    {
        auto s = getBusBuffer (buffer, true, 1);
        sidoKanaler = juce::jmin (2, s.getNumChannels());
        for (int k = 0; k < sidoKanaler; ++k) sido[k] = s.getReadPointer (k);
    }

    // Takten för Grindens taktläge.
    dsp::Takt takt;
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) takt.ppq = *q;
            takt.spelar = pos->getIsPlaying();
        }
    takt.ppqPerSampel = bpm / 60.0 / fs;

    const int n = main.getNumSamples();
    for (int start = 0; start < n; start += bit)
    {
        const int len = juce::jmin (bit, n - start);
        float* ljud[2] { main.getWritePointer (0) + start, kanaler > 1 ? main.getWritePointer (1) + start : nullptr };
        const float* s[2] { sido[0] ? sido[0] + start : nullptr, sido[1] ? sido[1] + start : nullptr };
        processBit (ljud, kanaler, len, sidoKanaler > 0 ? s : nullptr, sidoKanaler, takt);
        takt.ppq += takt.ppqPerSampel * len;
    }
}

void PannrummetProcessor::processBit (float* const* ljud, int kanaler, int n, const float* const* sido, int sidoKanaler, const dsp::Takt& takt)
{
    // Inmätning, förbikopplingens råa kopia och in-nivån.
    for (int k = 0; k < kanaler; ++k)
    {
        auto* r = ra.getWritePointer (k);
        for (int i = 0; i < n; ++i) { inTopp = std::max (inTopp, std::abs (ljud[k][i])); r[i] = raFordr[(size_t) k].process (ljud[k][i]); }
    }
    inGain.setTargetValue (gainAv (las (id::inNiva)));
    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        for (int k = 0; k < kanaler; ++k) ljud[k][i] *= g;
    }
    for (int k = 0; k < kanaler; ++k)
    {
        auto* t = torr.getWritePointer (k);
        for (int i = 0; i < n; ++i) t[i] = torrFordr[(size_t) k].process (ljud[k][i]);
    }

    // Trycket drar alla påslagna moduler mot mer energi (se PLAN.md).
    const float T = las (id::trycket);
    auto klipp = [] (float v, float lo, float hi) { return juce::jlimit (lo, hi, v); };

    // En modul som slås på börjar från tystnad (inga rester i ekon och rum).
    auto pasl = [this] (const char* pid, int nr) { const bool p = pa (pid); if (p && ! varPa[(size_t) nr]) nystartad[(size_t) nr] = true; varPa[(size_t) nr] = p; return p; };

    // Grinden.
    if (pasl (id::grindPa, 0))
    {
        dsp::GrindParams gp;
        gp.lage = (int) las (id::grindLage);
        gp.troskelDb = klipp (las (id::grindTroskel) + 10.0f * T, -80.0f, 0.0f);
        gp.hallMs = las (id::grindHall) * (1.0f - 0.4f * T);
        gp.slappMs = las (id::grindSlapp) * (1.0f - 0.6f * T);
        gp.djup = las (id::grindDjup);
        gp.sidokedja = pa (id::grindSidokedja);
        for (int i = 0; i < 16; ++i) gp.steg[(size_t) i] = apvts.getRawParameterValue (id::grindSteg (i))->load() > 0.5f;
        grinden.process (ljud, kanaler, n, sido, sidoKanaler, gp, takt);
        grindOppning.store (grinden.oppning());
        grindSteg.store (gp.lage == 2 ? grinden.steg() : -1);
    }
    else { grindOppning.store (0.0f); grindSteg.store (-1); }

    // Plåten – före eller efter Pressen.
    auto platen_ = [&]
    {
        if (! pasl (id::platPa, 2)) return;
        if (nystartad[2]) { platen.reset(); nystartad[2] = false; }
        dsp::PlatParams pp;
        pp.material = (int) las (id::platMaterial);
        const int ton = plattTon.load();
        pp.ton = pa (id::platMidi) && ton >= 0 ? (float) ton : las (id::platStamning);
        pp.klangSek = las (id::platKlang) * (1.0f + T);
        pp.ljushet = las (id::platLjushet);
        pp.blandning = klipp (las (id::platBlandning) + 0.3f * T, 0.0f, 1.0f);
        platen.process (ljud, kanaler, n, pp);
    };
    const bool platenFore = pa (id::platFore);
    if (platenFore) platen_();

    // Pressen (sköter själv latensen även när den är av).
    {
        dsp::PressParams pp;
        pp.pa = pa (id::pressPa);
        pp.lage = (int) las (id::pressLage);
        pp.driv = klipp (las (id::pressDriv) + 0.5f * T, 0.0f, 1.0f);
        pp.ton = las (id::pressTon);
        pp.tyngd = klipp (las (id::pressTyngd) + 0.4f * T, 0.0f, 1.0f);
        pp.nivaDb = las (id::pressNiva);
        pp.over4 = pa (id::pressOver4);
        juce::dsp::AudioBlock<float> block (const_cast<float**> (ljud), (size_t) kanaler, (size_t) n);
        pressen.process (block, pp);
    }

    if (! platenFore) platen_();

    // Megafonen.
    if (pasl (id::megaPa, 3))
    {
        dsp::MegaParams mp;
        mp.lage = (int) las (id::megaLage);
        mp.bandHz = las (id::megaBand);
        mp.resonans = las (id::megaResonans);
        mp.smuts = klipp (las (id::megaSmuts) + 0.5f * T, 0.0f, 1.0f);
        mp.blandning = las (id::megaBlandning);
        mp.brus = pa (id::megaBrus);
        megafonen.process (ljud, kanaler, n, mp);
    }

    // Schaktet.
    if (pasl (id::schaktPa, 4))
    {
        if (nystartad[4]) { schaktet.reset(); nystartad[4] = false; }
        dsp::SchaktParams sp;
        sp.fjardedelar = lagen::tidFjardedelar ((int) las (id::schaktTid));
        sp.bpm = takt.ppqPerSampel * 60.0 * fs;
        sp.aterkoppling = klipp ((las (id::schaktAterk) + 25.0f * T) / 100.0f, 0.0f, 0.95f);
        sp.morker = las (id::schaktMorker);
        sp.svaj = las (id::schaktSvaj);
        sp.blandning = klipp (las (id::schaktBlandning) + 0.2f * T, 0.0f, 1.0f);
        sp.pingpong = pa (id::schaktPingpong);
        schaktet.process (ljud, kanaler, n, sp);
    }

    // Hallen.
    if (pasl (id::hallPa, 5))
    {
        if (nystartad[5]) { hallen.reset(); nystartad[5] = false; }
        dsp::HallParams hp;
        hp.lokal = (int) las (id::hallLokal);
        hp.storlek = klipp (las (id::hallStorlek) + 0.3f * T, 0.0f, 1.0f);
        hp.halltidMs = las (id::hallHalltid);
        hp.fallMs = las (id::hallFall);
        hp.blandning = klipp (las (id::hallBlandning) + 0.2f * T, 0.0f, 1.0f);
        hp.grind = pa (id::hallGrind);
        hallen.process (ljud, kanaler, n, hp);
    }

    // Torr/våt, utnivå, limiter.
    blandning.setTargetValue (las (id::torrVat));
    utGain.setTargetValue (gainAv (las (id::utNiva)));
    for (int i = 0; i < n; ++i)
    {
        const float m = blandning.getNextValue(), g = utGain.getNextValue();
        for (int k = 0; k < kanaler; ++k)
        {
            const float t = torr.getSample (k, i);
            ljud[k][i] = (m >= 1.0f ? ljud[k][i] : t + (ljud[k][i] - t) * m) * g;
        }
    }
    if (pa (id::limiter)) { limitern.process (ljud, kanaler, n); const float d = limitern.hamtaDampning(); if (d < limiterDb.load()) limiterDb.store (d); }

    // Nödstoppet: 20 ms övertoning till den råa, fördröjda ingången.
    forbi.mot (pa (id::nodstopp) ? 1.0f : 0.0f, (int) (0.02 * fs));
    for (int i = 0; i < n; ++i)
    {
        const float b = forbi.nasta();
        for (int k = 0; k < kanaler; ++k)
        {
            const float r = ra.getSample (k, i);
            ljud[k][i] = b <= 0.0f ? ljud[k][i] : (b >= 1.0f ? r : ljud[k][i] + (r - ljud[k][i]) * b);
            utTopp = std::max (utTopp, std::abs (ljud[k][i]));
        }
    }
    // Panelen läser och nollställer (exchange); här höjs bara värdet.
    if (krang::dsp::gainTillDb (inTopp) > inNivaDb.load()) inNivaDb.store (krang::dsp::gainTillDb (inTopp));
    if (krang::dsp::gainTillDb (utTopp) > utNivaDb.load()) utNivaDb.store (krang::dsp::gainTillDb (utTopp));
    inTopp = utTopp = 0.0f;
}

void PannrummetProcessor::getStateInformation (juce::MemoryBlock& data)
{
    auto tillstand = apvts.copyState();
    tillstand.setProperty ("stFormat", formatVersion, nullptr);
    if (auto xml = tillstand.createXml()) copyXmlToBinary (*xml, data);
}

void PannrummetProcessor::setStateInformation (const void* data, int storlek)
{
    if (auto xml = getXmlFromBinary (data, storlek))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            programbank.aterstall();
        }
}

juce::AudioProcessorEditor* PannrummetProcessor::createEditor() { return new PannrummetEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PannrummetProcessor(); }
