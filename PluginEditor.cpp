#include "PluginEditor.h"

RealLabEditor::RealLabEditor (RealLabProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (1100, 580);

    // Library (preset list)
    addAndMakeVisible (library);
    library.setRowHeight (28);
    library.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    library.setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);

    // Mouse / computer-keyboard playing keyboard
    addAndMakeVisible (keyboard);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, RLColours::accent);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, RLColours::accent2.withAlpha (0.35f));

    // Source
    juce::StringArray waves { "Saw", "Square", "Triangle", "Sine" };
    waveBox.addItemList (waves, 1);
    addAndMakeVisible (waveBox);
    waveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (processor.apvts, "wave", waveBox);

    // Bypass
    bypassButton.setButtonText ("Bypass FX");
    addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, "bypassFX", bypassButton);

    // Header buttons
    addAndMakeVisible (loadButton);
    addAndMakeVisible (clearButton);
    addAndMakeVisible (saveButton);

    sampleLabel.setText (processor.getSampleStatus(), juce::dontSendNotification);
    sampleLabel.setFont (juce::Font (12.0f));
    sampleLabel.setColour (juce::Label::textColourId, RLColours::accent2);
    addAndMakeVisible (sampleLabel);

    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose a sample or a folder of samples", juce::File {}, "");
        const auto flags = juce::FileBrowserComponent::openMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::canSelectDirectories;

        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            const auto result = fc.getResult();
            if (result.exists())
            {
                processor.loadSampleFolder (result);
                sampleLabel.setText (processor.getSampleStatus(), juce::dontSendNotification);
            }
        });
    };

    clearButton.onClick = [this]
    {
        processor.clearSamples();
        sampleLabel.setText (processor.getSampleStatus(), juce::dontSendNotification);
    };

    saveButton.onClick = [this]
    {
        RealLabProcessor::getUserPresetFolder().createDirectory();
        chooser = std::make_unique<juce::FileChooser> ("Save preset",
                    RealLabProcessor::getUserPresetFolder().getChildFile ("My Preset.xml"), "*.xml");
        const auto flags = juce::FileBrowserComponent::saveMode
                         | juce::FileBrowserComponent::canSelectFiles
                         | juce::FileBrowserComponent::warnAboutOverwriting;

        chooser->launchAsync (flags, [this] (const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file == juce::File {})
                return;
            if (file.getFileExtension().isEmpty())
                file = file.withFileExtension (".xml");

            processor.savePresetTo (file);
            refreshLibrary();
        });
    };

    // Knobs and faders, mapped to the same parameter IDs as the processor
    makeControl ("attack",     "Attack",     true);
    makeControl ("release",    "Release",    true);
    makeControl ("volume",     "Volume",     true);
    makeControl ("unison",     "Unison",     false);
    makeControl ("cutoff",     "Filter",     false);
    makeControl ("resonance",  "Resonance",  false);
    makeControl ("vibRate",    "Vib Rate",   false);
    makeControl ("vibDepth",   "Vibrato",    false);
    makeControl ("gateRate",   "Gate Rate",  false);
    makeControl ("gateAmt",    "Gated",      false);
    makeControl ("character",  "Character",  false);
    makeControl ("reverbMix",  "Reverb",     false);
    makeControl ("reverbSize", "Room",       false);
    makeControl ("delayTime",  "Time",       false);
    makeControl ("delayFb",    "Feedback",   false);
    makeControl ("delayMix",   "Delay",      false);

    refreshLibrary();
}

RealLabEditor::~RealLabEditor()
{
    setLookAndFeel (nullptr);
}

//==============================================================================
RealLabEditor::Control* RealLabEditor::makeControl (const char* paramId, const juce::String& caption, bool isFader)
{
    auto c = std::make_unique<Control>();
    c->id = paramId;
    c->fader = isFader;

    c->slider.setSliderStyle (isFader ? juce::Slider::LinearVertical : juce::Slider::RotaryVerticalDrag);
    c->slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    c->slider.setPopupDisplayEnabled (true, false, this);

    c->label.setText (caption, juce::dontSendNotification);
    c->label.setJustificationType (juce::Justification::centred);
    c->label.setFont (juce::Font (11.0f));

    addAndMakeVisible (c->slider);
    addAndMakeVisible (c->label);

    c->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, c->slider);

    auto* raw = c.get();
    controls.push_back (std::move (c));
    return raw;
}

RealLabEditor::Control* RealLabEditor::findControl (const char* paramId)
{
    for (auto& c : controls)
        if (c->id == paramId)
            return c.get();
    return nullptr;
}

void RealLabEditor::layoutGrid (juce::Rectangle<int> area, int columns, const std::vector<Control*>& items)
{
    const int count = (int) items.size();
    if (count == 0 || columns <= 0)
        return;

    const int rows = (count + columns - 1) / columns;
    const int cellW = area.getWidth() / columns;
    const int cellH = area.getHeight() / rows;

    for (int i = 0; i < count; ++i)
    {
        auto* c = items[(size_t) i];
        if (c == nullptr)
            continue;

        auto cell = juce::Rectangle<int> (area.getX() + (i % columns) * cellW,
                                          area.getY() + (i / columns) * cellH,
                                          cellW, cellH);
        c->label.setBounds (cell.removeFromBottom (16));

        if (c->fader)
        {
            c->slider.setBounds (cell.getCentreX() - 15, cell.getY() + 2, 30, cell.getHeight() - 4);
        }
        else
        {
            const int d = juce::jmin (juce::jmin (cell.getWidth(), cell.getHeight()) - 6, 64);
            c->slider.setBounds (cell.getCentreX() - d / 2, cell.getCentreY() - d / 2, d, d);
        }
    }
}

void RealLabEditor::resized()
{
    groups[0] = { 12,  68, 200, 380 };   // Library
    groups[1] = { 224, 68, 220, 186 };   // Source
    groups[2] = { 454, 68, 160, 186 };   // Envelope
    groups[3] = { 624, 68, 260, 186 };   // Filter
    groups[4] = { 894, 68, 194, 186 };   // Master
    groups[5] = { 224, 262, 300, 186 };  // Modulation
    groups[6] = { 534, 262, 120, 186 };  // Character
    groups[7] = { 664, 262, 200, 186 };  // Reverb
    groups[8] = { 874, 262, 214, 186 };  // Delay

    // Header
    loadButton.setBounds (620, 14, 130, 28);
    clearButton.setBounds (758, 14, 80, 28);
    saveButton.setBounds (846, 14, 130, 28);
    sampleLabel.setBounds (260, 16, 350, 24);

    // Library list
    library.setBounds (groups[0].reduced (8, 26));

    // Source
    waveBox.setBounds (groups[1].getX() + 12, groups[1].getY() + 30, groups[1].getWidth() - 24, 26);
    layoutGrid (groups[1].withTrimmedTop (62).reduced (8, 4), 1, { findControl ("unison") });

    // Envelope (faders)
    layoutGrid (groups[2].withTrimmedTop (26).reduced (8, 4), 2, { findControl ("attack"), findControl ("release") });

    // Filter
    layoutGrid (groups[3].withTrimmedTop (26).reduced (8, 4), 2, { findControl ("cutoff"), findControl ("resonance") });

    // Master
    bypassButton.setBounds (groups[4].getX() + 12, groups[4].getY() + 30, groups[4].getWidth() - 24, 26);
    layoutGrid (groups[4].withTrimmedTop (62).reduced (8, 4), 1, { findControl ("volume") });

    // Modulation
    layoutGrid (groups[5].withTrimmedTop (26).reduced (8, 4), 4,
                { findControl ("vibRate"), findControl ("vibDepth"), findControl ("gateRate"), findControl ("gateAmt") });

    // Character
    layoutGrid (groups[6].withTrimmedTop (26).reduced (8, 4), 1, { findControl ("character") });

    // Reverb
    layoutGrid (groups[7].withTrimmedTop (26).reduced (8, 4), 2, { findControl ("reverbMix"), findControl ("reverbSize") });

    // Delay
    layoutGrid (groups[8].withTrimmedTop (26).reduced (8, 4), 3,
                { findControl ("delayTime"), findControl ("delayFb"), findControl ("delayMix") });

    keyboard.setBounds (12, 458, 1076, 112);
}

void RealLabEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg (RLColours::bgTop, 0.0f, 0.0f, RLColours::bgBottom, 0.0f, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();

    // Header bar
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    g.fillRect (0, 0, getWidth(), 56);
    g.setColour (RLColours::accent);
    g.fillRect (0, 55, getWidth(), 2);

    g.setColour (RLColours::text);
    g.setFont (juce::Font (24.0f, juce::Font::bold));
    g.drawText ("REAL LAB", 16, 6, 220, 30, juce::Justification::left);
    g.setColour (RLColours::dim);
    g.setFont (juce::Font (11.0f));
    g.drawText ("SAMPLER / SYNTH / FX", 18, 34, 220, 16, juce::Justification::left);

    // Panels
    static const char* titles[] = { "LIBRARY", "SOURCE", "ENVELOPE", "FILTER", "MASTER",
                                    "MODULATION", "CHARACTER", "REVERB", "DELAY" };
    for (int i = 0; i < 9; ++i)
    {
        auto r = groups[i].toFloat();
        g.setColour (RLColours::panel.withAlpha (0.92f));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (RLColours::outline);
        g.drawRoundedRectangle (r, 10.0f, 1.0f);

        g.setColour (RLColours::accent2);
        g.setFont (juce::Font (10.5f, juce::Font::bold));
        g.drawText (titles[i], groups[i].getX() + 12, groups[i].getY() + 6,
                    groups[i].getWidth() - 24, 16, juce::Justification::left);
    }
}

//==============================================================================
void RealLabEditor::refreshLibrary()
{
    presets = processor.getPresetList();
    library.updateContent();
    library.repaint();
}

int RealLabEditor::getNumRows()
{
    return (int) presets.size();
}

void RealLabEditor::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) presets.size()))
        return;

    if (selected)
    {
        g.setColour (RLColours::accent.withAlpha (0.22f));
        g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 1.0f, (float) width - 4.0f, (float) height - 2.0f), 5.0f);
        g.setColour (RLColours::accent);
        g.fillRoundedRectangle (juce::Rectangle<float> (2.0f, 6.0f, 3.0f, (float) height - 12.0f), 1.5f);
    }

    g.setColour (selected ? RLColours::text : RLColours::dim);
    g.setFont (juce::Font (13.0f));
    g.drawText (presets[(size_t) row].name, 12, 0, width - 16, height, juce::Justification::centredLeft, true);
}

void RealLabEditor::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (! juce::isPositiveAndBelow (row, (int) presets.size()))
        return;

    processor.applyPreset (presets[(size_t) row]);
    repaint();
}
