// Grinden – gaten.
//
// MJUK: vanlig gate, öppnar på 1 ms och stänger exponentiellt med Släpp.
// HÅRD: öppnar direkt och stänger linjärt på Släpp – avhugget, som EBM-trummor.
// TAKT: tröskeln används inte; grinden öppnar på de valda sextondelarna i DAW:ens takt, står öppen
//       Hålltid och stänger på Släpp. Står transporten still går en egen klocka i DAW:ens tempo.
// Tröskeln har 4 dB hysteres. Djup 100 % stänger helt; mindre djup släpper igenom en del.
// Sidokedjan (pluginets andra ingång) kan styra detektorn i MJUK och HÅRD.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>

namespace pr::dsp
{
using namespace krang::dsp;

struct GrindParams
{
    int lage = 1;                // 0 mjuk, 1 hård, 2 takt
    float troskelDb = -36.0f, hallMs = 40.0f, slappMs = 25.0f, djup = 1.0f;
    bool sidokedja = false;
    std::array<bool, 16> steg {};
};

// Taktläget för ett block: var vi är (i fjärdedelar) och hur fort det går.
struct Takt
{
    double ppq = 0.0;            // position i fjärdedelar vid blockets början
    double ppqPerSampel = 0.0;
    bool spelar = false;
};

class Grinden
{
public:
    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        detektor.satt (fs, 0.0002f, 0.030f);
        reset();
    }
    void reset()
    {
        detektor.reset(); gain = 0.0f; oppen = false; hallKvar = 0; egenPpq = 0.0; sistaSteg = -1; ramp = 0.0f;
    }

    // ljud: kanalerna, n sample. sido: sidokedjan (kan vara nullptr).
    void process (float* const* ljud, int kanaler, int n, const float* const* sido, int sidoKanaler,
                  const GrindParams& p, const Takt& takt)
    {
        const float oppna = dbTillGain (p.troskelDb), stang = dbTillGain (p.troskelDb - 4.0f);
        const float golv  = p.djup >= 0.999f ? 0.0f : dbTillGain (-80.0f * p.djup);
        const int hall    = (int) (p.hallMs * 0.001 * fs);
        const float slappSek = juce_max (0.0005f, p.slappMs * 0.001f);
        const float relExp   = tidKoeff (slappSek * 0.35f, fs);          // MJUK: exponentiellt
        const float relLin   = 1.0f / (float) (slappSek * fs);           // HÅRD/TAKT: linjärt
        const float attMjuk  = tidKoeff (0.001f, fs);
        const bool anvandSido = p.sidokedja && sido != nullptr && sidoKanaler > 0;
        double ppq = takt.spelar ? takt.ppq : egenPpq;
        const double dppq = takt.ppqPerSampel;

        for (int i = 0; i < n; ++i)
        {
            bool malOppen;
            if (p.lage == 2)
            {
                const int steg = (int) std::floor (ppq * 4.0) & 15;
                if (steg != sistaSteg)
                {
                    sistaSteg = steg;
                    if (p.steg[(size_t) steg]) hallKvar = juce_max (1, hall);
                }
                malOppen = hallKvar > 0;
                if (hallKvar > 0) --hallKvar;
                ppq += dppq;
            }
            else
            {
                float x = 0.0f;
                if (anvandSido) for (int k = 0; k < sidoKanaler; ++k) x = juce_max (x, std::abs (sido[k][i]));
                else            for (int k = 0; k < kanaler; ++k)     x = juce_max (x, std::abs (ljud[k][i]));
                const float niva = detektor.process (x);
                if (niva > oppna)      { oppen = true; hallKvar = hall; }
                else if (niva < stang) { if (hallKvar > 0) --hallKvar; else oppen = false; }
                else if (hallKvar > 0) --hallKvar;
                malOppen = oppen;
            }

            if (malOppen)
                ramp = p.lage == 0 ? 1.0f + attMjuk * (ramp - 1.0f) : 1.0f;
            else
                ramp = p.lage == 0 ? relExp * ramp : juce_max (0.0f, ramp - relLin);
            gain = golv + (1.0f - golv) * ramp;
            for (int k = 0; k < kanaler; ++k) ljud[k][i] *= gain;
        }
        egenPpq = ppq;
    }

    float oppning() const noexcept { return ramp; }   // 0..1, för lampan
    int steg() const noexcept { return sistaSteg; }   // steget som spelas i TAKT

private:
    static float juce_max (float a, float b) noexcept { return a > b ? a : b; }
    static int juce_max (int a, int b) noexcept { return a > b ? a : b; }

    double fs = 48000.0;
    Foljare detektor;
    float gain = 0.0f, ramp = 0.0f;
    bool oppen = false;
    int hallKvar = 0, sistaSteg = -1;
    double egenPpq = 0.0;
};

} // namespace pr::dsp
