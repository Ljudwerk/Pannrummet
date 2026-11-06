// Pannrummet – processorn: kedjan Grinden → Pressen → Plåten (eller Plåten före Pressen) → Megafonen →
// Schaktet → Hallen, torr/våt, utnivå, limiter och nödstopp. Trycket drar alla påslagna moduler mot
// mer energi.
//
// Latensen är konstant (Pressens 4×-översampling) och rapporteras till DAW:en. Den torra signalen och
// nödstoppets förbikoppling fördröjs lika mycket, så att torr/våt inte kamfiltrerar och förbikopplingen
// är bitexakt.
//
// MIDI in: tonerna stämmer Plåten (senaste ton gäller) när Plåten följer MIDI.
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "Parameters.h"
#include "presets/Fabriksbandet.h"
#include "dsp/Grinden.h"
#include "dsp/Pressen.h"
#include "dsp/Megafonen.h"
#include "dsp/Platen.h"
#include "dsp/Schaktet.h"
#include "dsp/Hallen.h"
#include "dsp/Utgangen.h"

class PannrummetProcessor : public juce::AudioProcessor
{
public:
    PannrummetProcessor();

    void prepareToPlay (double fs, int maxBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Pannrummet"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }   // Hallen och Schaktet
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (pr::id::nodstopp); }

    // Värdens programlista är fabriksbandet; egna program nås från panelen.
    int getNumPrograms() override { return programbank.antalFabrik(); }
    int getCurrentProgram() override { return juce::jmin (programbank.aktuellt(), programbank.antalFabrik() - 1); }
    // Samma program igen gör ingenting: vissa värdar ropar på programmet efter att ha läst in ett projekt,
    // och det får inte nollställa ändringar som gjorts efter att programmet laddades.
    void setCurrentProgram (int i) override { if (i != programbank.aktuellt()) programbank.ladda (i); }
    const juce::String getProgramName (int i) override { return programbank.program (i).namn; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    // Programmen: fabriksbandet och egna program. Förbikopplingen och MIDI in rörs aldrig av ett program.
    krang::presets::Programbank programbank { apvts, "Pannrummet", "prprog", pr::fabriksbandet(), { pr::id::nodstopp, pr::id::midiIn } };

    // Mätvärden till panelen (dBFS, skrivs av ljudtråden).
    std::atomic<float> inNivaDb { -120.0f }, utNivaDb { -120.0f }, limiterDb { 0.0f }, grindOppning { 0.0f };
    std::atomic<int> plattTon { -1 }, grindSteg { -1 };   // senaste MIDI-ton för Plåten, −1 = ingen

    static constexpr int formatVersion = 1;
    static constexpr int bit = 256;     // kedjan körs i bitar om högst så här många sample

private:
    static BusesProperties bussar();
    void processBit (float* const* ljud, int kanaler, int n, const float* const* sido, int sidoKanaler, const pr::dsp::Takt& takt);
    float las (const char* pid) const { return apvts.getRawParameterValue (pid)->load(); }
    bool  pa  (const char* pid) const { return las (pid) > 0.5f; }

    double fs = 48000.0;
    int kanalerForberedda = 2, latens = 0;
    pr::dsp::Grinden grinden;
    pr::dsp::Pressen pressen;
    pr::dsp::Megafonen megafonen;
    pr::dsp::Platen platen;
    pr::dsp::Schaktet schaktet;
    pr::dsp::Hallen hallen;
    std::array<bool, 6> varPa {}, nystartad {};
    pr::dsp::Limiter limitern;
    std::array<krang::dsp::Fordrojning, 2> torrFordr, raFordr;
    juce::AudioBuffer<float> torr, ra;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inGain, utGain;
    juce::SmoothedValue<float> blandning;
    krang::dsp::Ramp forbi;
    float inTopp = 0.0f, utTopp = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PannrummetProcessor)
};
