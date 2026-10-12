// Copyright (C) 2026 Henrik Juhl. Part of SubForge, licensed under the GNU AGPLv3 (see LICENSE).
// SPDX-License-Identifier: AGPL-3.0-only

// Renders the interface with the default sound, pattern and accent colour to a PNG,
// for the README: SubForge_Screenshot docs/images/screenshot.png [scale]
// Uses its own look-and-feel, so the user's saved accent colour and UI size are ignored.

#include "MainView.h"

int main (int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: SubForge_Screenshot <output.png> [scale]\n";
        return 1;
    }

    const juce::ScopedJuceInitialiser_GUI gui;
    const std::unique_ptr<juce::AudioProcessor> plugin (createPluginFilter());
    auto& processor = dynamic_cast<SubForgeProcessor&> (*plugin);
    SubForgeLookAndFeel lookAndFeel;
    const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 1.0f;

    // sized by itself: with the sequencer steps unless they are hidden in the user's settings
    auto view = std::make_unique<MainView> (processor, lookAndFeel, [] (float) {}, [scale] { return scale; }, [] {});

    const auto image = view->createComponentSnapshot (view->getLocalBounds(), true, scale);
    view.reset();

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
