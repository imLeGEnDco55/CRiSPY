#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// CRiSPY VST3 Plugin Editor
// Futuristic cyberpunk GUI with animated background grid, neon color palette,
// rotating dashboard rings, and drag-in/drag-out file support.
//==============================================================================

class CrispyEditor : public juce::AudioProcessorEditor,
                     public juce::Timer,
                     public juce::FileDragAndDropTarget
{
public:
    CrispyEditor(CrispyProcessor&);
    ~CrispyEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    // Mouse interactions
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

    // File drag-and-drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

private:
    CrispyProcessor& processor;

    // Neon Colour Palette
    juce::Colour colBgStart      { 0xff06060c };
    juce::Colour colBgEnd        { 0xff0f0f1c };
    juce::Colour colCardBg       { 0xff121222 };
    juce::Colour colCardBorder   { 0xff22223a };
    juce::Colour colLimeGreen    { 0xff32CD32 };
    juce::Colour colHotPink      { 0xffFF69B4 };
    juce::Colour colCoral        { 0xffFF7F50 };
    juce::Colour colCyan         { 0xff00FFFF };
    juce::Colour colRoyalBlue    { 0xff4169E1 };
    juce::Colour colTextWhite    { 0xffFFFFFF };
    juce::Colour colTextGray     { 0xff8e8e9f };
    juce::Colour colGrid         { 0x144169E1 }; // Royal Blue 8% opacity

    // Animation state
    float spinnerAngle = 0.0f;
    float gridOffset = 0.0f;

    // UI regions
    juce::Rectangle<float> rectBtnSave;
    juce::Rectangle<float> rectBtnReset;
    juce::Rectangle<float> rectBtnAgain;
    juce::Rectangle<float> rectDragCard;

    // Mouse position (updated each frame)
    juce::Point<float> mousePos;

    // API Key text editor
    juce::TextEditor apiKeyEditor;

    // Logo image
    juce::Image logoImage;

    // Drawing helpers
    void drawLogo(juce::Graphics& g, float centerX, float centerY);
    void drawCheckmarkIcon(juce::Graphics& g, float cx, float cy, float radius, bool filled);
    void drawRestartIcon(juce::Graphics& g, float cx, float cy, float radius, bool hover);
    void drawSoundwaveIcon(juce::Graphics& g, float centerX, float centerY);
    void drawFileIcon(juce::Graphics& g, float centerX, float centerY, bool glow);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrispyEditor)
};
