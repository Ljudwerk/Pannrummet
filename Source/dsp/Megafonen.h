// Megafonen – lo-fi för röster och paroller.
//
// Band är mittfrekvensen; varje läge bygger sitt eget filter runt den.
// MEGAFON: bandpass med topp, hårt klippande liten högtalare och en kort hornresonans.
// TELEFON: 300–3400 Hz-ledning, 8 kHz samplingsfrekvens och µ-lag med färre bitar när Smuts ökar.
// RADIO:   bredare band som svajar långsamt (dålig mottagning), mjuk mättnad.
// TRATT:   smal lur med två resonanstoppar och kraftig kamfiltrering.
// BRUS lägger till ledningsbrus och sprak. Nivån kompenseras, Blandning blandar med det torra.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>

namespace pr::dsp
{
using namespace krang::dsp;

struct MegaParams
{
    int lage = 0;                // 0 megafon, 1 telefon, 2 radio, 3 tratt
    float bandHz = 1500.0f, resonans = 0.4f, smuts = 0.4f, blandning = 1.0f;
    bool brus = false;
};

class Megafonen
{
public:
    static constexpr int maxKanaler = 2;

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        for (auto& k : kan)
        {
            k.horn.forbered ((int) (fs * 0.01));
            k.komp.satt (fs);
            k.dc.satt (fs);
        }
        reset();
    }
    void reset()
    {
        for (auto& k : kan)
        {
            for (auto& f : k.f) f.reset();
            k.horn.reset(); k.komp.reset(); k.dc.reset(); k.hall = 0.0f;
        }
        svajFas = 0.0; holdFas = 0.0; sprak = 0.0f; uppdatera = 0;
    }

    void process (float* const* ljud, int kanaler, int n, const MegaParams& p)
    {
        kanaler = juce_min (kanaler, maxKanaler);
        const float driv = 1.0f + p.smuts * (p.lage == 0 ? 24.0f : 10.0f);
        const float hornFb = p.lage == 3 ? 0.35f + 0.45f * p.resonans : 0.15f + 0.35f * p.resonans;
        const float hornMs = p.lage == 3 ? 0.38f : 0.85f;
        const float bitar = 8.0f - 4.0f * p.smuts;
        const float mu = 255.0f, steg = std::pow (2.0f, bitar - 1.0f);
        const double holdSteg = 8000.0 / fs;
        const float brusNiva = p.brus ? (p.lage == 2 ? 0.010f : 0.004f) : 0.0f;

        for (int i = 0; i < n; ++i)
        {
            if (uppdatera <= 0) { sattFilter (p); uppdatera = 32; }
            --uppdatera;
            if (p.lage == 2) svajFas += 0.37 / fs;
            const bool nyttHall = (holdFas += holdSteg) >= 1.0;
            if (nyttHall) holdFas -= 1.0;
            // Sprak: korta knäppar några gånger i sekunden.
            float knapp = 0.0f;
            if (p.brus && brusGen.enhet() < 6.0f / (float) fs) sprak = 0.6f * (brusGen.enhet() * 0.5f + 0.5f);
            if (sprak > 0.0005f) { knapp = sprak * brusGen.vitt(); sprak *= 0.93f; }
            const float brus = brusGen.vitt() * brusNiva + knapp * (p.brus ? 0.12f : 0.0f);

            for (int c = 0; c < kanaler; ++c)
            {
                auto& k = kan[(size_t) c];
                const float torr = ljud[c][i];
                k.komp.matIn (torr);
                float y = torr;
                for (auto& f : k.f) y = f.process (y);
                switch (p.lage)
                {
                    case 0:  y = std::tanh (y * driv) / std::sqrt (driv); break;
                    case 1:
                    {
                        if (nyttHall) k.hall = y;
                        y = k.hall;
                        if (p.smuts > 0.01f)
                        {
                            const float s = juce_sgn (y) * std::log1p (mu * juce_min (1.0f, std::abs (y))) / std::log1p (mu);
                            const float q = std::round (s * steg) / steg;
                            const float u = juce_sgn (q) * (std::pow (1.0f + mu, std::abs (q)) - 1.0f) / mu;
                            y = y + (u - y) * juce_min (1.0f, p.smuts * 2.0f);
                        }
                        y = std::tanh (y * (1.0f + p.smuts * 3.0f));
                        break;
                    }
                    case 2:  y = std::tanh (y * driv * 0.6f); break;
                    default: y = std::tanh (y * driv * 0.8f); break;
                }
                if (p.lage == 0 || p.lage == 3)
                {
                    const float ut = y + hornFb * k.horn.las (hornMs * 0.001f * (float) fs);
                    k.horn.skriv (ut);
                    y = ut * (1.0f - hornFb * 0.5f);
                }
                y = k.dc.process (y + brus);
                y = k.komp.process (y);
                ljud[c][i] = torr + (y - torr) * p.blandning;
            }
        }
    }

private:
    static int juce_min (int a, int b) noexcept { return a < b ? a : b; }
    static float juce_min (float a, float b) noexcept { return a < b ? a : b; }
    static float juce_sgn (float x) noexcept { return x < 0.0f ? -1.0f : 1.0f; }

    void sattFilter (const MegaParams& p)
    {
        double f = p.bandHz;
        if (p.lage == 2) f *= 1.0 + 0.08 * std::sin (2.0 * pi * svajFas) + 0.03 * std::sin (2.0 * pi * svajFas * 5.3);
        const double q = 0.8 + 5.0 * p.resonans;
        using T = Biquad::Typ;
        for (auto& k : kan)
        {
            auto& b = k.f;
            switch (p.lage)
            {
                case 0: b[0].satt (T::hogpass, fs, f * 0.45, 0.9); b[1].satt (T::lagpass, fs, f * 2.4, 0.9);
                        b[2].satt (T::topp, fs, f, q, 6.0 + 9.0 * p.resonans); b[3].satt (T::hogpass, fs, f * 0.3, 0.7); break;
                case 1: b[0].satt (T::hogpass, fs, std::max (250.0, f * 0.3), 0.8); b[1].satt (T::lagpass, fs, std::min (3600.0, f * 2.4), 0.8);
                        b[2].satt (T::topp, fs, f, q, 3.0 + 6.0 * p.resonans); b[3].satt (T::lagpass, fs, 3800.0, 0.7); break;
                case 2: b[0].satt (T::hogpass, fs, f * 0.22, 0.7); b[1].satt (T::lagpass, fs, f * 3.0, 0.7);
                        b[2].satt (T::topp, fs, f, q * 0.6, 3.0 + 6.0 * p.resonans); b[3].satt (T::lagpass, fs, f * 3.4, 0.7); break;
                default: b[0].satt (T::hogpass, fs, f * 0.6, 1.0); b[1].satt (T::lagpass, fs, f * 1.9, 1.0);
                        b[2].satt (T::topp, fs, f, q * 1.5, 9.0 + 9.0 * p.resonans); b[3].satt (T::topp, fs, f * 2.4, q * 1.5, 6.0 + 6.0 * p.resonans); break;
            }
        }
    }

    struct Kanal
    {
        std::array<Biquad, 4> f;
        Fordrojning horn;
        Nivakomp komp;
        DcSparr dc;
        float hall = 0.0f;
    };

    double fs = 48000.0, svajFas = 0.0, holdFas = 0.0;
    std::array<Kanal, maxKanaler> kan;
    Brus brusGen;
    float sprak = 0.0f;
    int uppdatera = 0;
};

} // namespace pr::dsp
