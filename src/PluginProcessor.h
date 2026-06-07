#pragma once
#include <JuceHeader.h>

//==============================================================================
// CRiSPY VST3 Plugin Processor
// Handles state management and background Python processing.
// Audio pass-through only — actual enhancement is file-based via Nvidia API.
//==============================================================================

enum CrispyState {
    CRISPY_KEY_INPUT,
    CRISPY_READY,
    CRISPY_PROCESSING,
    CRISPY_COMPLETE,
    CRISPY_ERROR
};

class CrispyProcessor : public juce::AudioProcessor
{
public:
    CrispyProcessor();
    ~CrispyProcessor() override;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // CRiSPY-specific methods
    void setApiKey(const juce::String& key);
    juce::String getApiKey() const { return apiKey; }

    void launchProcessing(const juce::String& inputFilePath);

    CrispyState getAppState() const { return appState.load(); }
    juce::String getCleanFilePath() const { return cleanFilePath; }
    juce::String getErrorMessage() const { return errorMessage; }

    void resetToReady();

private:
    // Persistence helpers
    juce::File getConfigFile() const;
    void saveApiKey();
    void loadApiKey();

    // Python script helpers
    juce::File locatePythonExecutable() const;
    juce::File locatePythonScript() const;

    // State
    juce::String apiKey;
    juce::String cleanFilePath;
    juce::String errorMessage;
    std::atomic<CrispyState> appState { CRISPY_KEY_INPUT };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrispyProcessor)
};
