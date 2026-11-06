// Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
#include "PluginEditor.h"

PannrummetEditor::PannrummetEditor (PannrummetProcessor& p) : AudioProcessorEditor (p), proc (p), panel (p)
{
    addAndMakeVisible (panel);
    panel.onStorlek = [this] (float s) { sattStorlek (s); };
    sattStorlek ((float) proc.apvts.state.getProperty ("storlek", 0.85));
}

void PannrummetEditor::sattStorlek (float s)
{
    s = juce::jlimit (0.6f, 1.6f, s);
    proc.apvts.state.setProperty ("storlek", s, nullptr);
    panel.storlek = s;
    panel.setTransform (juce::AffineTransform::scale (s));
    setSize ((int) std::round (pr::stil::panel::bredd * s), (int) std::round (pr::stil::panel::hojd * s));
}

void PannrummetEditor::resized()
{
    panel.setBounds (0, 0, pr::stil::panel::bredd, pr::stil::panel::hojd);
}
