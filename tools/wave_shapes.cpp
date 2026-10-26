// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Draws the square wave at Width 45 % with Sag 0, 50 and 100 % (two cycles of A2, as the oscillator
// and the sag stage make them) to a PNG, for the README:
//   SubForge_WaveShapes docs/images/square-shaping.png [scale]
// Uses the default look, so the user's saved accent colour is ignored.

#include "BassSynth.h"
#include "SubForgeLookAndFeel.h"

#include <iostream>

namespace
{
constexpr double kSampleRate = 192000.0; // the voice runs 4x oversampled
constexpr double kPitchHz = 110.0;
constexpr float kWidth = 0.45f;
constexpr int kCycles = 2;

// Two cycles of the shaped square, after the sag has settled.
std::vector<float> render (float sag)
{
    const float inc = (float) (kPitchHz / kSampleRate);
    const int period = (int) std::round (kSampleRate / kPitchHz);
    subforge::Oscillator osc;
    subforge::SquareSag shaper;
    std::vector<float> out;
    for (int i = 0; i < (40 + kCycles) * period; ++i)
    {
        const float y = shaper.process (osc.next (inc, 1, kWidth), inc, sag);
        if (i >= 40 * period)
            out.push_back (y);
    }
    return out;
}

juce::Path pathOf (const std::vector<float>& samples, juce::Rectangle<float> area, float peak)
{
    juce::Path path;
    for (size_t i = 0; i < samples.size(); ++i)
    {
        const float x = area.getX() + area.getWidth() * (float) i / (float) (samples.size() - 1);
        const float y = area.getCentreY() - samples[i] / peak * area.getHeight() * 0.5f;
        if (i == 0)
            path.startNewSubPath (x, y);
        else
            path.lineTo (x, y);
    }
    return path;
}
} // namespace

int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: SubForge_WaveShapes <output.png> [scale]\n";
        return 1;
    }

    const juce::ScopedJuceInitialiser_GUI gui;
    SubForgeLookAndFeel look;
    using P = SubForgeLookAndFeel::Palette;
    const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 2.0f;

    constexpr int width = 920, rowHeight = 180, margin = 16, titleHeight = 30;
    const float sags[] = { 0.0f, 0.5f, 1.0f };
    const int height = margin + 3 * (rowHeight + margin);

    const auto plain = render (0.0f);
    float peak = 0.0f;
    for (float s : plain)
        peak = std::max (peak, std::abs (s));

    juce::Image image (juce::Image::ARGB, juce::roundToInt (width * scale), juce::roundToInt (height * scale), true);
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (scale));
    g.fillAll (P::background);

    for (int row = 0; row < 3; ++row)
    {
        const auto panel = juce::Rectangle<float> ((float) margin, (float) (margin + row * (rowHeight + margin)),
                                                   (float) (width - 2 * margin), (float) rowHeight);
        g.setColour (P::panel);
        g.fillRoundedRectangle (panel, 10.0f);
        g.setColour (P::panelEdge);
        g.drawRoundedRectangle (panel.reduced (0.5f), 10.0f, 1.0f);

        g.setColour (P::dimText);
        g.setFont (look.titleFont (12.0f).withExtraKerningFactor (0.12f));
        const auto title = juce::String ("SQR ") + juce::String::fromUTF8 ("\xc2\xb7") + " Width "
                           + juce::String (juce::roundToInt (kWidth * 100.0f)) + " % " + juce::String::fromUTF8 ("\xc2\xb7")
                           + " Sag " + juce::String (juce::roundToInt (sags[row] * 100.0f)) + " %";
        g.drawText (title, panel.withTrimmedLeft (16.0f).withHeight ((float) titleHeight), juce::Justification::centredLeft);

        const auto wave = panel.withTrimmedTop ((float) titleHeight).reduced (16.0f, 12.0f);
        g.setColour (P::track);
        g.drawHorizontalLine (juce::roundToInt (wave.getCentreY()), wave.getX(), wave.getRight());

        if (row > 0) // the plain square behind, for comparison
        {
            g.setColour (P::dimText.withAlpha (0.45f));
            g.strokePath (pathOf (plain, wave, peak), juce::PathStrokeType (1.5f));
        }
        g.setColour (look.getAccent());
        g.strokePath (pathOf (render (sags[row]), wave, peak), juce::PathStrokeType (2.0f));
    }

    const juce::File file = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
    file.deleteFile();
    juce::FileOutputStream out (file);
    if (! out.openedOk() || ! juce::PNGImageFormat().writeImageToStream (image, out))
    {
        std::cerr << "could not write " << file.getFullPathName() << "\n";
        return 1;
    }
    std::cout << "wrote " << file.getFullPathName() << " (" << image.getWidth() << " x " << image.getHeight() << ")\n";
    return 0;
}
