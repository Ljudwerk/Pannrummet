// Plåten – metallresonatorn.
//
// Modalsyntes: åtta resonatorer (tvåpoliga bandpass) stämda efter materialets deltoner. Ljudet slår an
// dem, och de klingar med sin egen efterklang. Grundtonen är Stämning, eller senaste MIDI-ton när
// Plåten följer MIDI (glider 20 ms mellan tonerna).
//   PLÅT   tät, oharmonisk (kvadratisk platta)
//   RÖR    nästan harmonisk, lätt sträckt (öppet rör)
//   FJÄDER glesa, dispersiva deltoner med kort klang i toppen
//   KLOCKA klockans deltoner: brum, grundton, ters, kvint, nominal …
//   RING   ringmodulator: ljudet gånger en bärvåg på grundtonen. Ljushet gör bärvågen fyrkantigare,
//          Klang lägger en kort kamresonans på bärvågens period.
// Klang är efterklangstiden för grundtonen; högre deltoner klingar kortare, mindre ju högre Ljushet.
// Ljushet tippar också deltonernas styrka. Nivån kompenseras; Blandning blandar med det torra.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <krang/dsp/Grund.h>
#include <array>

namespace pr::dsp
{
using namespace krang::dsp;

struct PlatParams
{
    int material = 0;           // 0 plåt, 1 rör, 2 fjäder, 3 klocka, 4 ring
    float ton = 45.0f;          // MIDI-tonnummer (kan vara fraktionellt)
    float klangSek = 1.2f, ljushet = 0.5f, blandning = 0.3f;
};

// Torr/våt-blandning där det torra står kvar fullt upp till 50 % och det våta når fullt vid 50 %.
inline void blandaTorrVat (float m, float& gTorr, float& gVat) noexcept
{
    gTorr = std::min (1.0f, 2.0f * (1.0f - m));
    gVat  = std::min (1.0f, 2.0f * m);
}

class Platen
{
public:
    static constexpr int maxKanaler = 2, lagen = 8;

    void forbered (double samplingsfrekvens)
    {
        fs = samplingsfrekvens;
        for (auto& k : kan)
        {
            k.komp.satt (fs, 1.5f, 1.0f, 0.3f); k.komp.minDb = -36.0f; k.komp.maxDb = 18.0f;
            k.dc.satt (fs);
            k.kam.forbered ((int) (fs * 0.05));
            k.inLag.satt (fs, 6000.0);
        }
        reset();
    }
    void reset()
    {
        for (auto& k : kan)
        {
            for (auto& r : k.res) r.x1 = r.x2 = r.y1 = r.y2 = 0.0f;
            k.komp.reset(); k.dc.reset(); k.kam.reset(); k.inLag.reset();
        }
        fas = 0.0; tonNu = -1.0f; uppdatera = 0;
    }

    void process (float* const* ljud, int kanaler, int n, const PlatParams& p)
    {
        kanaler = std::min (kanaler, maxKanaler);
        if (tonNu < 0.0f) tonNu = p.ton;
        const float glid = tidKoeff (0.02f, fs);
        float gTorr, gVat;
        blandaTorrVat (p.blandning, gTorr, gVat);

        for (int i = 0; i < n; ++i)
        {
            tonNu = p.ton + glid * (tonNu - p.ton);
            if (--uppdatera <= 0) { sattLagen (p); uppdatera = 32; }

            float barvag = 0.0f;
            if (p.material == 4)
            {
                fas += f0 / fs; if (fas >= 1.0) fas -= 1.0;
                const float s = (float) std::sin (2.0 * pi * fas);
                const float fyrkant = std::tanh (s * (1.0f + 12.0f * p.ljushet));
                barvag = s + (fyrkant - s) * p.ljushet;
            }

            for (int c = 0; c < kanaler; ++c)
            {
                auto& k = kan[(size_t) c];
                const float torr = ljud[c][i];
                k.komp.matIn (torr);
                float vat = 0.0f;
                if (p.material == 4)
                {
                    vat = torr * barvag;
                    if (p.klangSek > 0.06f)
                    {
                        const float fb = std::min (0.85f, p.klangSek / 8.0f * 1.2f);
                        const float ut = vat + fb * k.kam.las ((float) (fs / f0));
                        k.kam.skriv (ut);
                        vat = ut * (1.0f - fb * 0.5f);
                    }
                }
                else
                {
                    // Mjukare anslag när Ljushet är låg.
                    const float lag = k.inLag.process (torr);
                    const float x = lag + (torr - lag) * p.ljushet;
                    for (int m = 0; m < antalLagen; ++m)
                    {
                        auto& r = k.res[(size_t) m];
                        // Nollor i ±1 (bandpass): ingen likspänning, ingen ton över Nyquist.
                        const float y = r.g * (x - r.x2) + r.a1 * r.y1 + r.a2 * r.y2;
                        r.x2 = r.x1; r.x1 = x;
                        r.y2 = r.y1; r.y1 = y;
                        vat += y * r.niva;
                    }
                }
                vat = k.dc.process (vat);
                vat = std::tanh (k.komp.process (vat) * 0.9f) / 0.9f;
                ljud[c][i] = torr * gTorr + vat * gVat;
            }
        }
    }

private:
    void sattLagen (const PlatParams& p)
    {
        f0 = 440.0 * std::pow (2.0, (tonNu - 69.0) / 12.0);
        static const float kvoter[4][lagen] {
            { 1.0f, 1.58f, 2.0f, 2.24f, 2.55f, 3.0f, 3.16f, 3.54f },     // plåt
            { 1.0f, 2.01f, 3.03f, 4.06f, 5.1f, 6.15f, 7.2f, 8.27f },     // rör
            { 1.0f, 1.73f, 2.41f, 3.05f, 3.66f, 4.25f, 4.83f, 5.39f },   // fjäder
            { 0.5f, 1.0f, 1.2f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f } };        // klocka
        static const float niva[4][lagen] {
            { 1.0f, 0.9f, 0.8f, 0.8f, 0.7f, 0.6f, 0.6f, 0.5f },
            { 1.0f, 0.7f, 0.55f, 0.45f, 0.35f, 0.3f, 0.25f, 0.2f },
            { 1.0f, 0.8f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.4f },
            { 0.7f, 1.0f, 0.8f, 0.6f, 0.9f, 0.5f, 0.4f, 0.3f } };
        if (p.material == 4) return;
        const int mat = std::clamp (p.material, 0, 3);
        antalLagen = 0;
        for (int c = 0; c < maxKanaler; ++c)
        {
            int n = 0;
            for (int m = 0; m < lagen; ++m)
            {
                // Lite isärstämt mellan kanalerna ger bredd.
                const double f = f0 * kvoter[mat][m] * (c == 0 ? 1.0 : 1.0035);
                if (f > fs * 0.45) break;
                if (f < 25.0) continue;   // under hörbart bas: bara likspänning och mull
                // Högre deltoner klingar kortare; fjädern tappar toppen fortast.
                const double fall = std::pow ((double) kvoter[mat][m], mat == 2 ? -1.2 : -0.6 + 0.45 * p.ljushet);
                const double t60 = std::max (0.03, (double) p.klangSek * fall);
                const double r = std::pow (10.0, -3.0 / (t60 * fs));
                const double w = 2.0 * pi * f / fs;
                auto& res = kan[(size_t) c].res[(size_t) n];
                res.a1 = (float) (2.0 * r * std::cos (w));
                res.a2 = (float) (-r * r);
                res.g  = (float) ((1.0 - r * r) * 0.5);
                const float tilt = std::pow (kvoter[mat][m], (p.ljushet - 0.5f) * 1.6f);
                res.niva = niva[mat][m] * tilt;
                ++n;
            }
            antalLagen = n;
        }
    }

    struct Resonator { float a1 = 0, a2 = 0, g = 0, niva = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0; };
    struct Kanal
    {
        std::array<Resonator, lagen> res;
        Nivakomp komp;
        DcSparr dc;
        Fordrojning kam;
        EnPol inLag;
    };

    double fs = 48000.0, fas = 0.0, f0 = 110.0;
    float tonNu = -1.0f;
    int uppdatera = 0, antalLagen = 0;
    std::array<Kanal, maxKanaler> kan;
};

} // namespace pr::dsp
