// Hallen – den gatade reverben.
//
// Ett återkopplat fördröjningsnät (FDN) med åtta linjer och Hadamard-blandning. Lokalen bestämmer
// rummets karaktär: linjelängder, dämpning, fördröjning före rummet och hur mycket linjerna svajar.
//   VERKSTAD litet och ljust, täta reflexer      HALL  stort och jämnt
//   VALV     sten: långt, mörkt, mullrande       GRUVA metalliskt, svajande, mycket långt
// Storlek skalar rummet och efterklangstiden (0,3–6 s).
// GRIND: rummet öppnas när ljudet kommer (tröskel −40 dBFS), står öppet Hålltid efter att ljudet
// tystnat och stängs sedan på Fall – det tvärt avhuggna virvelljudet. Utan grind klingar rummet ut.
// Ett högpass på ingången (150 Hz) håller bastrumman ute ur rummet.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>
#include "Platen.h"

namespace pr::dsp
{
using namespace krang::dsp;

struct HallParams
{
    int lokal = 1;
    float storlek = 0.7f, halltidMs = 180.0f, fallMs = 30.0f, blandning = 0.35f;
    bool grind = true;
};

class Hallen
{
public:
    static constexpr int linjer = 8;

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        for (auto& l : linje) l.forbered ((int) (fs * 0.5));
        fore.forbered ((int) (fs * 0.1));
        for (auto& h : hp) h.satt (Biquad::Typ::hogpass, fs, 150.0, 0.7);
        detektor.satt (fs, 0.0005f, 0.05f);
        reset();
    }
    void reset()
    {
        for (auto& l : linje) l.reset();
        for (auto& d : damp) d.reset();
        for (auto& h : hp) h.reset();
        fore.reset(); detektor.reset();
        grind = 0.0f; hall = 0; fas = 0.0;
    }

    void process (float* const* ljud, int kanaler, int n, const HallParams& p)
    {
        // Lokalernas karaktär: längdskala, dämpning (Hz), fördröjning före rummet (ms), svaj (sample).
        struct Lokal { float skala, dampHz, foreMs, svaj, rtSkala; };
        static const Lokal lokaler[4] { { 0.45f, 9000.0f, 4.0f, 0.0f, 0.5f },
                                        { 1.0f,  6500.0f, 18.0f, 1.5f, 1.0f },
                                        { 1.25f, 3000.0f, 30.0f, 0.8f, 1.5f },
                                        { 1.6f,  4500.0f, 45.0f, 6.0f, 1.8f } };
        const auto& lk = lokaler[std::clamp (p.lokal, 0, 3)];
        static const float basMs[linjer] { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.1f, 73.3f };
        const float skala = lk.skala * (0.45f + p.storlek);
        const float rt60 = (0.3f + 5.7f * p.storlek * p.storlek) * lk.rtSkala;
        float lang[linjer], g[linjer];
        for (int l = 0; l < linjer; ++l)
        {
            lang[l] = std::min ((float) fs * 0.48f, basMs[l] * 0.001f * skala * (float) fs);
            g[l] = std::pow (10.0f, -3.0f * lang[l] / (rt60 * (float) fs));
            damp[(size_t) l].satt (fs, lk.dampHz);
        }
        fore.sattLangd ((int) (lk.foreMs * 0.001f * fs));
        const float oppna = dbTillGain (-40.0f);
        const int hallSampel = (int) (p.halltidMs * 0.001f * fs);
        const float fallSteg = 1.0f / std::max (1.0f, p.fallMs * 0.001f * (float) fs);
        float gTorr, gVat;
        blandaTorrVat (p.blandning, gTorr, gVat);
        const bool stereo = kanaler > 1;

        for (int i = 0; i < n; ++i)
        {
            const float inL = ljud[0][i], inR = stereo ? ljud[1][i] : inL;
            const float mono = 0.5f * (inL + inR);
            // Grinden styrs av det torra ljudet.
            if (detektor.process (mono) > oppna) { grind = 1.0f; hall = hallSampel; }
            else if (hall > 0) --hall;
            else grind = std::max (0.0f, grind - fallSteg);

            const float x = fore.process (hp[0].process (mono)) * 0.6f;
            fas += 0.7 / fs; if (fas >= 1.0) fas -= 1.0;
            float y[linjer];
            for (int l = 0; l < linjer; ++l)
            {
                const float svaj = lk.svaj * (float) std::sin (2.0 * pi * (fas + 0.125 * l));
                y[l] = damp[(size_t) l].process (linje[(size_t) l].las (lang[l] + svaj)) * g[l];
            }
            // Hadamard 8×8 (normaliserad), i tre steg.
            for (int s = 1; s < linjer; s <<= 1)
                for (int a = 0; a < linjer; a += s << 1)
                    for (int b = a; b < a + s; ++b) { const float u = y[b], v = y[b + s]; y[b] = u + v; y[b + s] = u - v; }
            const float norm = 0.35355339f;   // 1/√8
            float utL = 0.0f, utR = 0.0f;
            for (int l = 0; l < linjer; ++l)
            {
                const float v = y[l] * norm;
                linje[(size_t) l].skriv (v + (l % 2 == 0 ? x : -x));
                if (l % 2 == 0) utL += v; else utR += v;
            }
            const float gate = p.grind ? grind * grind * (3.0f - 2.0f * grind) : 1.0f;
            utL *= gate; utR *= gate;
            ljud[0][i] = inL * gTorr + utL * gVat;
            if (stereo) ljud[1][i] = inR * gTorr + utR * gVat;
            else        ljud[0][i] = inL * gTorr + 0.5f * (utL + utR) * gVat;
        }
    }

private:
    double fs = 48000.0, fas = 0.0;
    std::array<Fordrojning, linjer> linje;
    std::array<EnPol, linjer> damp;
    std::array<Biquad, 1> hp;
    Fordrojning fore;
    Foljare detektor;
    float grind = 0.0f;
    int hall = 0;
};

} // namespace pr::dsp
