// Pannrummet – editorn: fabrikspanelen (ui/Panelen.h) i vald storlek, 60–160 % (85 % från början).
//
// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#pragma once

#include "PluginProcessor.h"
#include "ui/Panelen.h"

class PannrummetEditor : public juce::AudioProcessorEditor
{
public:
    explicit PannrummetEditor (PannrummetProcessor& p);
    void resized() override;

private:
    void sattStorlek (float s);
    PannrummetProcessor& proc;
    pr::ui::Panelen panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PannrummetEditor)
};
