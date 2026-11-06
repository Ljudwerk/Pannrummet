// Pressen – drivet.
//
// Kedjan: Tyngd (kompression före drivet) → driv → formare → ton (tilt runt 900 Hz) → nivåkompensation → Nivå.
// RÖR: asymmetrisk tanh med jämna övertoner. TRANSISTOR: hårdare, nästan rak klippning.
// LIKRIKTARE: helvågslikriktning blandad med signalen – oktav upp och smuts.
// BITKROSS: färre bitar och lägre samplingsfrekvens, avsiktligt utan översampling.
// RÖR, TRANSISTOR och LIKRIKTARE körs 2× eller 4× översamplat (FIR-halvband, heltalslatens).
// Latensen är alltid 4×-latensen, även i 2×, BITKROSS och när Pressen är av, så att DAW:en
// aldrig behöver räkna om den.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_dsp/juce_dsp.h>
#include <krang/dsp/Grund.h>
#include <array>
#include <memory>

namespace pr::dsp
{
using namespace krang::dsp;

struct PressParams
{
    bool pa = false;
    int lage = 0;                // 0 rör, 1 transistor, 2 likriktare, 3 bitkross
    float driv = 0.4f, ton = 0.5f, tyngd = 0.0f, nivaDb = 0.0f;
    bool over4 = false;
};

class Pressen
{
public:
    static constexpr int maxKanaler = 2;

    void forbered (double samplingsfrekvens, int maxBlock, int kanaler)
    {
        fs = samplingsfrekvens;
        nk = juce::jlimit (1, maxKanaler, kanaler);
        using OS = juce::dsp::Oversampling<float>;
        os2 = std::make_unique<OS> ((size_t) nk, 1, OS::filterHalfBandFIREquiripple, false, true);
        os4 = std::make_unique<OS> ((size_t) nk, 2, OS::filterHalfBandFIREquiripple, false, true);
        os2->initProcessing ((size_t) maxBlock);
        os4->initProcessing ((size_t) maxBlock);
        lat2 = (int) std::lround (os2->getLatencyInSamples());
        lat4 = (int) std::lround (os4->getLatencyInSamples());
        for (int k = 0; k < maxKanaler; ++k)
        {
            utfyllnad2[(size_t) k].forbered (lat4); utfyllnad2[(size_t) k].sattLangd (lat4 - lat2);
            heltFordr[(size_t) k].forbered (lat4);  heltFordr[(size_t) k].sattLangd (lat4);
            matFordr[(size_t) k].forbered (lat4);   matFordr[(size_t) k].sattLangd (lat4);
            dc[(size_t) k].satt (fs);
            tonLag[(size_t) k].satt (fs, 900.0);
            komp[(size_t) k].satt (fs);
            dcOs[(size_t) k].satt (fs * 4.0);
        }
        tyngdFoljare.satt (fs, 0.005f, 0.080f);
        reset();
    }

    void reset()
    {
        if (os2) os2->reset();
        if (os4) os4->reset();
        for (int k = 0; k < maxKanaler; ++k)
        {
            utfyllnad2[(size_t) k].reset(); heltFordr[(size_t) k].reset(); matFordr[(size_t) k].reset(); dc[(size_t) k].reset(); dcOs[(size_t) k].reset();
            tonLag[(size_t) k].reset(); komp[(size_t) k].reset(); haller[(size_t) k] = 0.0f;
        }
        tyngdFoljare.reset(); tyngdGain = 1.0f; holdRakn = 0;
    }

    int latens() const noexcept { return lat4; }

    void process (juce::dsp::AudioBlock<float> block, const PressParams& p)
    {
        const int n = (int) block.getNumSamples();
        const int kan = juce::jmin (nk, (int) block.getNumChannels());
        if (! p.pa)
        {
            for (int k = 0; k < kan; ++k)
            {
                auto* d = block.getChannelPointer ((size_t) k);
                for (int i = 0; i < n; ++i) d[i] = heltFordr[(size_t) k].process (d[i]);
            }
            return;
        }

        // Tyngd och nivåmätningen före, i grundtakt.
        const float tTrosk = dbTillGain (-30.0f * p.tyngd), kvot = 4.0f;
        const float tyngdMakeup = dbTillGain (30.0f * p.tyngd * (1.0f - 1.0f / kvot) * 0.6f);
        for (int i = 0; i < n; ++i)
        {
            float x = 0.0f;
            for (int k = 0; k < kan; ++k) x = juce::jmax (x, std::abs (block.getSample (k, i)));
            if (p.tyngd > 0.001f)
            {
                const float e = tyngdFoljare.process (x);
                tyngdGain = e > tTrosk ? std::pow (e / tTrosk, 1.0f / kvot - 1.0f) * tyngdMakeup : tyngdMakeup;
            }
            else tyngdGain = 1.0f;
            for (int k = 0; k < kan; ++k)
            {
                const float v = block.getSample (k, i);
                komp[(size_t) k].matIn (matFordr[(size_t) k].process (v));   // i fas med utgången
                block.setSample (k, i, v * tyngdGain);
            }
        }

        {
            const float forv = p.lage == 3 ? 1.0f / dbTillGain (p.driv * 12.0f) : 1.0f / dbTillGain (p.driv * 36.0f);
            for (int k = 0; k < kan; ++k) komp[(size_t) k].forvanta (forv);
        }
        if (p.lage == 3) bitkross (block, kan, n, p);
        else             formaOversamplat (block, kan, n, p);

        // Ton, likspänning, nivåkompensation och nivå.
        const float tilt = (p.ton - 0.5f) * 24.0f;
        const float gh = dbTillGain (tilt * 0.5f), gl = dbTillGain (-tilt * 0.5f), niva = dbTillGain (p.nivaDb);
        for (int k = 0; k < kan; ++k)
        {
            auto* d = block.getChannelPointer ((size_t) k);
            for (int i = 0; i < n; ++i)
            {
                const float lag = tonLag[(size_t) k].process (d[i]);
                float y = lag * gl + (d[i] - lag) * gh;
                y = dc[(size_t) k].process (y);
                d[i] = komp[(size_t) k].process (y) * niva;
            }
        }
    }

private:
    static float forma (int lage, float x) noexcept
    {
        switch (lage)
        {
            case 0: { const float bias = 0.22f; return std::tanh (x + bias) - std::tanh (bias); }
            case 1: return x / std::pow (1.0f + std::pow (std::abs (x), 4.0f), 0.25f);
            case 2: return std::tanh (0.45f * x + 0.9f * std::abs (x));
            default: return x;
        }
    }

    void formaOversamplat (juce::dsp::AudioBlock<float>& block, int kan, int n, const PressParams& p)
    {
        auto& os = p.over4 ? *os4 : *os2;
        const float g = dbTillGain (p.driv * 36.0f);
        auto upp = os.processSamplesUp (block.getSubsetChannelBlock (0, (size_t) kan));
        const int m = (int) upp.getNumSamples();
        for (int k = 0; k < kan; ++k)
        {
            auto* d = upp.getChannelPointer ((size_t) k);
            for (int i = 0; i < m; ++i)
            {
                float y = forma (p.lage, d[i] * g);
                if (p.lage == 2) y = dcOs[(size_t) k].process (y);
                d[i] = y;
            }
        }
        auto ut = block.getSubsetChannelBlock (0, (size_t) kan);
        os.processSamplesDown (ut);
        if (! p.over4)
            for (int k = 0; k < kan; ++k)
            {
                auto* d = block.getChannelPointer ((size_t) k);
                for (int i = 0; i < n; ++i) d[i] = utfyllnad2[(size_t) k].process (d[i]);
            }
    }

    void bitkross (juce::dsp::AudioBlock<float>& block, int kan, int n, const PressParams& p)
    {
        const float bitar = 16.0f - 13.0f * p.driv;
        const float steg = std::pow (2.0f, bitar - 1.0f);
        const int halla = 1 + (int) (p.driv * p.driv * 15.0f);
        const float g = dbTillGain (p.driv * 12.0f);
        for (int i = 0; i < n; ++i)
        {
            const bool nytt = holdRakn == 0;
            holdRakn = (holdRakn + 1) % halla;
            for (int k = 0; k < kan; ++k)
            {
                if (nytt)
                {
                    const float x = std::tanh (block.getSample (k, i) * g);
                    haller[(size_t) k] = std::round (x * steg) / steg;
                }
                block.setSample (k, i, heltFordr[(size_t) k].process (haller[(size_t) k]));
            }
        }
    }

    double fs = 48000.0;
    int nk = 2, lat2 = 0, lat4 = 0, holdRakn = 0;
    std::unique_ptr<juce::dsp::Oversampling<float>> os2, os4;
    std::array<Fordrojning, maxKanaler> utfyllnad2, heltFordr, matFordr;
    std::array<DcSparr, maxKanaler> dc, dcOs;
    std::array<EnPol, maxKanaler> tonLag;
    std::array<Nivakomp, maxKanaler> komp;
    std::array<float, maxKanaler> haller {};
    Foljare tyngdFoljare;
    float tyngdGain = 1.0f;
};

} // namespace pr::dsp
